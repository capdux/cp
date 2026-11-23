// Reference-hardened against zip-rs/zip2 (MIT)
// Source: Absorbed from /data/dev/bun/src/libarchive/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pack/zip.cp — 工业级 PKZIP (APPNOTE 6.3.x) 归档解析、安全校验与解压引擎
// 对标 zip-rs/zip2 (MIT) 加固：
//   - 反向 EOCD 定位（支持最长 65535B 归档注释，16-lane SIMD 魔数预扫，无溢出边界校验）
//   - ZIP64 Extra Field (0x0001) 与 ZIP64 EOCD/Locator 完整解析
//   - 安全防护三板斧：
//     1) 路径遍历防护 (isSafeZipEntryPath / sanitizeZipEntryPath + cap:simd 预扫，
//        拦截 .. / 绝对路径 / 反斜杠 / Windows 盘符 / NUL)
//     2) Central Directory 与 Local File Header 一致性校验 + Data Descriptor (bit 3) 兼容
//     3) 解压体积上限防护 (extractEntryWithLimit) + 精确 IEEE CRC32 校验
//   - 纯 CP Store (16-lane SIMD 搬运) + Deflate (inflateRawWithLimit) 零 FFI 解压
//   - 100% 向后兼容 ZipLocalHeader / ZipCentralHeader / ZipEocd / ZipEntryView / ZipReader
// ============================================================================

const std = @import("std");
import { Crc32 } from "./checksum.cp";
import { inflateRawWithLimit, GzipError } from "./gzip.cp";
import { Simd } from "cap:simd";

pub const ZIP_LOCAL_HEADER_SIG: u32 = 0x04034b50;
pub const ZIP_CENTRAL_DIR_SIG: u32 = 0x02014b50;
pub const ZIP_EOCD_SIG: u32 = 0x06054b50;
pub const ZIP64_EOCD_SIG: u32 = 0x06064b50;
pub const ZIP64_EOCD_LOCATOR_SIG: u32 = 0x07064b50;
pub const ZIP_DATA_DESC_SIG: u32 = 0x08074b50;

pub const ZIP_LOCAL_HEADER_SIZE: usize = 30;
pub const ZIP_CENTRAL_DIR_SIZE: usize = 46;
pub const ZIP_EOCD_SIZE: usize = 22;
pub const ZIP64_EOCD_MIN_SIZE: usize = 56;
pub const ZIP64_EOCD_LOCATOR_SIZE: usize = 20;
pub const ZIP_MAX_COMMENT_LEN: usize = 65535;
pub const ZIP_DEFAULT_MAX_EXTRACT_SIZE: usize = 64 * 1024 * 1024;

pub const ZIP_METHOD_STORE: u16 = 0;
pub const ZIP_METHOD_DEFLATE: u16 = 8;

pub const ZIP64_EXTRA_HEADER_ID: u16 = 0x0001;
pub const ZIP_FLAG_ENCRYPTED: u16 = 0x0001;
pub const ZIP_FLAG_DATA_DESCRIPTOR: u16 = 0x0008;
pub const ZIP_FLAG_UTF8: u16 = 0x0800;

/// 16-lane SIMD 字节向量（用于反向 EOCD 魔数扫描与 Stored 条目块搬运）
type Vec16u8 = @Vector(16, u8);

pub error ZipError {
    InvalidArchive,
    EocdNotFound,
    UnsupportedMultiDisk,
    InvalidCentralDirectory,
    TruncatedEntry,
    InvalidLocalHeader,
    HeaderMismatch,
    UnsupportedCompression,
    EncryptedEntry,
    InvalidZip64Extra,
    UnsafePath,
    CrcMismatch,
    DecompressFailed,
    OutputTooLarge,
    OutOfMemory,
}

pub struct ZipEndian {
    pub static fn readU16LE(buf: []const u8, offset: usize): u16 {
        return (buf[offset] as u16) | ((buf[offset + 1] as u16) << 8);
    }

    pub static fn readU32LE(buf: []const u8, offset: usize): u32 {
        return (buf[offset] as u32)
            | ((buf[offset + 1] as u32) << 8)
            | ((buf[offset + 2] as u32) << 16)
            | ((buf[offset + 3] as u32) << 24);
    }

    pub static fn readU64LE(buf: []const u8, offset: usize): u64 {
        const lo = ZipEndian.readU32LE(buf, offset) as u64;
        const hi = ZipEndian.readU32LE(buf, offset + 4) as u64;
        return lo | (hi << 32);
    }

    pub static fn writeU16LE(out: []u8, offset: usize, val: u16) {
        out[offset] = (val & 0xFF) as u8;
        out[offset + 1] = ((val >> 8) & 0xFF) as u8;
    }

    pub static fn writeU32LE(out: []u8, offset: usize, val: u32) {
        out[offset] = (val & 0xFF) as u8;
        out[offset + 1] = ((val >> 8) & 0xFF) as u8;
        out[offset + 2] = ((val >> 16) & 0xFF) as u8;
        out[offset + 3] = ((val >> 24) & 0xFF) as u8;
    }

    pub static fn writeU64LE(out: []u8, offset: usize, val: u64) {
        ZipEndian.writeU32LE(out, offset, (val & 0xFFFFFFFF) as u32);
        ZipEndian.writeU32LE(out, offset + 4, ((val >> 32) & 0xFFFFFFFF) as u32);
    }
}

/// 标量参考：Stored 条目载荷拷贝（用于与 SIMD 0 误差对拍）
pub fn copyStoredScalar(dst: []u8, src: []const u8) {
    let n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD 加速 Stored 条目载荷拷贝（与标量参考 0 误差）
pub fn copyStoredSimd(dst: []u8, src: []const u8) {
    let n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        let v: Vec16u8 = src[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = v;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 校验单个 EOCD 候选偏移是否合法（魔数匹配 + 注释长度精确到达文件末尾）
fn isValidEocdAt(buf: []const u8, pos: usize): bool {
    if (pos + ZIP_EOCD_SIZE > buf.len) return false;
    if (ZipEndian.readU32LE(buf, pos) != ZIP_EOCD_SIG) return false;
    const comment_len = ZipEndian.readU16LE(buf, pos + 20) as usize;
    return pos + ZIP_EOCD_SIZE + comment_len == buf.len;
}

/// 标量参考：反向扫描 EOCD（最多回溯 65535 + 22 字节，用于与 SIMD 0 误差对拍）
pub fn findEocdOffsetScalar(buf: []const u8): usize? {
    if (buf.len < ZIP_EOCD_SIZE) return null;
    const max_back = ZIP_EOCD_SIZE + ZIP_MAX_COMMENT_LEN;
    const min_pos: usize = if (buf.len > max_back) (buf.len - max_back) else 0;
    let pos: usize = buf.len - ZIP_EOCD_SIZE;
    while (true) {
        if (isValidEocdAt(buf, pos)) {
            return pos;
        }
        if (pos == min_pos) break;
        pos -= 1;
    }
    return null;
}

/// 16-lane SIMD 加速反向 EOCD 魔数扫描（支持最长 65535B 归档注释，与标量结果 0 差异）
pub fn findEocdOffsetSimd(buf: []const u8): usize? {
    if (buf.len < ZIP_EOCD_SIZE) return null;
    const max_back = ZIP_EOCD_SIZE + ZIP_MAX_COMMENT_LEN;
    const min_pos: usize = if (buf.len > max_back) (buf.len - max_back) else 0;
    const max_pos: usize = buf.len - ZIP_EOCD_SIZE;
    const p_vec: Vec16u8 = @splat(0x50); // 'P' (LSB of 0x06054b50)

    let end: usize = max_pos + 1;
    while (end > min_pos) {
        if (end - min_pos >= 16) {
            const start = end - 16;
            const chunk: Vec16u8 = buf[start .. start + 16][0..16].*;
            const eq = chunk == p_vec;
            const mask: u16 = @bitCast(eq);
            if (mask != 0) {
                // 从高 lane 向低 lane 检查命中位置，确保返回最靠近文件尾的合法 EOCD
                let lane: usize = 16;
                while (lane > 0) {
                    lane -= 1;
                    if (((mask >> (lane as u4)) & 1) != 0) {
                        const cand = start + lane;
                        if (isValidEocdAt(buf, cand)) {
                            return cand;
                        }
                    }
                }
            }
            end = start;
        } else {
            end -= 1;
            if (isValidEocdAt(buf, end)) {
                return end;
            }
        }
    }
    return null;
}

/// 标量参考：安全路径校验（用于与 cap:simd 预扫路径 0 差异对拍）
pub fn isSafeZipEntryPathScalar(path: []const u8): bool {
    if (path.len == 0) return false;
    if (path[0] == '/' || path[0] == '\\') return false;
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        const c = path[i];
        if (c == 0x00 || c == '\\' || c == ':') {
            return false;
        }
        if (c == '/') {
            const seg = path[seg_start..i];
            if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
                return false;
            }
            seg_start = i + 1;
        }
    }
    const last_seg = path[seg_start..path.len];
    if (last_seg.len == 2 && last_seg[0] == '.' && last_seg[1] == '.') {
        return false;
    }
    return true;
}

/// 检查路径是否为安全的相对 ZIP 条目路径（对标 zip-rs/zip2 enclosed_name）
/// 严格拒绝：空路径、NUL 字节、绝对路径 (`/` / `\`)、Windows 盘符 (`C:`)、反斜杠与任意 `..` 路径段
pub fn isSafeZipEntryPath(path: []const u8): bool {
    if (path.len == 0) return false;
    if (path[0] == '/' || path[0] == '\\') return false;

    // 热路径 cap:simd 预扫：若整个路径不含 '\', ':', 0x00, '.'，直接 O(N/16) 安全放行
    if (Simd.indexOfAny4(path, '\\', ':', 0x00, '.') == null) {
        return true;
    }

    // 含潜在敏感字符，逐段严格校验
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        const c = path[i];
        if (c == 0x00 || c == '\\' || c == ':') {
            return false;
        }
        if (c == '/') {
            const seg = path[seg_start..i];
            if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
                return false;
            }
            seg_start = i + 1;
        }
    }
    const last_seg = path[seg_start..path.len];
    if (last_seg.len == 2 && last_seg[0] == '.' && last_seg[1] == '.') {
        return false;
    }
    return true;
}

/// 校验并返回安全的 ZIP 条目路径（非法路径返回 null）
pub fn sanitizeZipEntryPath(path: []const u8): ([]const u8)? {
    if (!isSafeZipEntryPath(path)) return null;
    return path;
}

/// ZIP64 Extra Field (0x0001) 解析结果
pub struct Zip64Fields {
    pub uncompressed_size: u64 = 0,
    pub compressed_size: u64 = 0,
    pub local_header_offset: u64 = 0,
    pub disk_start: u32 = 0,
    pub has_zip64: bool = false,
}

/// 按 PKWARE APPNOTE §4.5.3 解析 Extra Field 中的 ZIP64 (0x0001) 扩展字段
pub fn parseZip64ExtraField(
    extra: []const u8,
    need_uncomp: bool,
    need_comp: bool,
    need_offset: bool,
    need_disk: bool,
): Zip64Fields!ZipError {
    let out = Zip64Fields{};
    let pos: usize = 0;
    while (pos + 4 <= extra.len) {
        const header_id = ZipEndian.readU16LE(extra, pos);
        const data_size = ZipEndian.readU16LE(extra, pos + 2) as usize;
        pos += 4;
        if (data_size > extra.len - pos) {
            return ZipError.InvalidZip64Extra;
        }
        if (header_id == ZIP64_EXTRA_HEADER_ID) {
            const field = extra[pos .. pos + data_size];
            let fpos: usize = 0;
            if (need_uncomp) {
                if (fpos + 8 > field.len) return ZipError.InvalidZip64Extra;
                out.uncompressed_size = ZipEndian.readU64LE(field, fpos);
                fpos += 8;
            }
            if (need_comp) {
                if (fpos + 8 > field.len) return ZipError.InvalidZip64Extra;
                out.compressed_size = ZipEndian.readU64LE(field, fpos);
                fpos += 8;
            }
            if (need_offset) {
                if (fpos + 8 > field.len) return ZipError.InvalidZip64Extra;
                out.local_header_offset = ZipEndian.readU64LE(field, fpos);
                fpos += 8;
            }
            if (need_disk) {
                if (fpos + 4 > field.len) return ZipError.InvalidZip64Extra;
                out.disk_start = ZipEndian.readU32LE(field, fpos);
                fpos += 4;
            }
            out.has_zip64 = true;
            return out;
        }
        pos += data_size;
    }
    if (need_uncomp || need_comp || need_offset || need_disk) {
        return ZipError.InvalidZip64Extra;
    }
    return out;
}

/// PKZIP 30-byte Local File Header (0x04034b50)
pub struct ZipLocalHeader {
    version_needed: u16 = 20;
    flags: u16 = 0;
    compression_method: u16 = 0;
    mtime: u16 = 0;
    mdate: u16 = 0;
    crc32: u32 = 0;
    compressed_size: u32 = 0;
    uncompressed_size: u32 = 0;
    name_len: u16 = 0;
    extra_len: u16 = 0;

    pub fn write(out: []u8): usize {
        if (out.len < ZIP_LOCAL_HEADER_SIZE) {
            return 0;
        }
        ZipEndian.writeU32LE(out, 0, ZIP_LOCAL_HEADER_SIG);
        ZipEndian.writeU16LE(out, 4, _.version_needed);
        ZipEndian.writeU16LE(out, 6, _.flags);
        ZipEndian.writeU16LE(out, 8, _.compression_method);
        ZipEndian.writeU16LE(out, 10, _.mtime);
        ZipEndian.writeU16LE(out, 12, _.mdate);
        ZipEndian.writeU32LE(out, 14, _.crc32);
        ZipEndian.writeU32LE(out, 18, _.compressed_size);
        ZipEndian.writeU32LE(out, 22, _.uncompressed_size);
        ZipEndian.writeU16LE(out, 26, _.name_len);
        ZipEndian.writeU16LE(out, 28, _.extra_len);
        return ZIP_LOCAL_HEADER_SIZE;
    }

    pub static fn parse(buf: []const u8): ZipLocalHeader? {
        if (buf.len < ZIP_LOCAL_HEADER_SIZE) {
            return null;
        }
        if (ZipEndian.readU32LE(buf, 0) != ZIP_LOCAL_HEADER_SIG) {
            return null;
        }
        return ZipLocalHeader {
            version_needed: ZipEndian.readU16LE(buf, 4),
            flags: ZipEndian.readU16LE(buf, 6),
            compression_method: ZipEndian.readU16LE(buf, 8),
            mtime: ZipEndian.readU16LE(buf, 10),
            mdate: ZipEndian.readU16LE(buf, 12),
            crc32: ZipEndian.readU32LE(buf, 14),
            compressed_size: ZipEndian.readU32LE(buf, 18),
            uncompressed_size: ZipEndian.readU32LE(buf, 22),
            name_len: ZipEndian.readU16LE(buf, 26),
            extra_len: ZipEndian.readU16LE(buf, 28),
        };
    }

    /// Encodes a complete stored local file entry (30-byte header + file name + payload) into `out`.
    pub static fn encodeEntry(out: []u8, name: []const u8, payload: []const u8): usize {
        const total_len = ZIP_LOCAL_HEADER_SIZE + name.len + payload.len;
        if (out.len < total_len) {
            return 0;
        }
        const crc = Crc32.checksum(payload);
        const hdr = ZipLocalHeader {
            version_needed: 20,
            flags: 0,
            compression_method: ZIP_METHOD_STORE,
            mtime: 0,
            mdate: 0,
            crc32: crc,
            compressed_size: payload.len as u32,
            uncompressed_size: payload.len as u32,
            name_len: name.len as u16,
            extra_len: 0,
        };
        * = hdr.write(out[0..ZIP_LOCAL_HEADER_SIZE]);
        copyStoredSimd(out[ZIP_LOCAL_HEADER_SIZE .. ZIP_LOCAL_HEADER_SIZE + name.len], name);
        copyStoredSimd(out[ZIP_LOCAL_HEADER_SIZE + name.len .. total_len], payload);
        return total_len;
    }
}

/// PKZIP 46-byte Central Directory File Header (0x02014b50)
pub struct ZipCentralHeader {
    version_made_by: u16 = 20;
    version_needed: u16 = 20;
    flags: u16 = 0;
    compression_method: u16 = 0;
    mtime: u16 = 0;
    mdate: u16 = 0;
    crc32: u32 = 0;
    compressed_size: u32 = 0;
    uncompressed_size: u32 = 0;
    name_len: u16 = 0;
    extra_len: u16 = 0;
    comment_len: u16 = 0;
    disk_num_start: u16 = 0;
    internal_attrs: u16 = 0;
    external_attrs: u32 = 0;
    local_header_offset: u32 = 0;

    pub fn write(out: []u8): usize {
        if (out.len < ZIP_CENTRAL_DIR_SIZE) {
            return 0;
        }
        ZipEndian.writeU32LE(out, 0, ZIP_CENTRAL_DIR_SIG);
        ZipEndian.writeU16LE(out, 4, _.version_made_by);
        ZipEndian.writeU16LE(out, 6, _.version_needed);
        ZipEndian.writeU16LE(out, 8, _.flags);
        ZipEndian.writeU16LE(out, 10, _.compression_method);
        ZipEndian.writeU16LE(out, 12, _.mtime);
        ZipEndian.writeU16LE(out, 14, _.mdate);
        ZipEndian.writeU32LE(out, 16, _.crc32);
        ZipEndian.writeU32LE(out, 20, _.compressed_size);
        ZipEndian.writeU32LE(out, 24, _.uncompressed_size);
        ZipEndian.writeU16LE(out, 28, _.name_len);
        ZipEndian.writeU16LE(out, 30, _.extra_len);
        ZipEndian.writeU16LE(out, 32, _.comment_len);
        ZipEndian.writeU16LE(out, 34, _.disk_num_start);
        ZipEndian.writeU16LE(out, 36, _.internal_attrs);
        ZipEndian.writeU32LE(out, 38, _.external_attrs);
        ZipEndian.writeU32LE(out, 42, _.local_header_offset);
        return ZIP_CENTRAL_DIR_SIZE;
    }

    pub static fn parse(buf: []const u8): ZipCentralHeader? {
        if (buf.len < ZIP_CENTRAL_DIR_SIZE) {
            return null;
        }
        if (ZipEndian.readU32LE(buf, 0) != ZIP_CENTRAL_DIR_SIG) {
            return null;
        }
        return ZipCentralHeader {
            version_made_by: ZipEndian.readU16LE(buf, 4),
            version_needed: ZipEndian.readU16LE(buf, 6),
            flags: ZipEndian.readU16LE(buf, 8),
            compression_method: ZipEndian.readU16LE(buf, 10),
            mtime: ZipEndian.readU16LE(buf, 12),
            mdate: ZipEndian.readU16LE(buf, 14),
            crc32: ZipEndian.readU32LE(buf, 16),
            compressed_size: ZipEndian.readU32LE(buf, 20),
            uncompressed_size: ZipEndian.readU32LE(buf, 24),
            name_len: ZipEndian.readU16LE(buf, 28),
            extra_len: ZipEndian.readU16LE(buf, 30),
            comment_len: ZipEndian.readU16LE(buf, 32),
            disk_num_start: ZipEndian.readU16LE(buf, 34),
            internal_attrs: ZipEndian.readU16LE(buf, 36),
            external_attrs: ZipEndian.readU32LE(buf, 38),
            local_header_offset: ZipEndian.readU32LE(buf, 42),
        };
    }
}

/// PKZIP 22-byte End of Central Directory Record (0x06054b50)
pub struct ZipEocd {
    disk_num: u16 = 0;
    central_dir_disk: u16 = 0;
    entries_on_disk: u16 = 0;
    total_entries: u16 = 0;
    central_dir_size: u32 = 0;
    central_dir_offset: u32 = 0;
    comment_len: u16 = 0;

    pub fn write(out: []u8): usize {
        if (out.len < ZIP_EOCD_SIZE) {
            return 0;
        }
        ZipEndian.writeU32LE(out, 0, ZIP_EOCD_SIG);
        ZipEndian.writeU16LE(out, 4, _.disk_num);
        ZipEndian.writeU16LE(out, 6, _.central_dir_disk);
        ZipEndian.writeU16LE(out, 8, _.entries_on_disk);
        ZipEndian.writeU16LE(out, 10, _.total_entries);
        ZipEndian.writeU32LE(out, 12, _.central_dir_size);
        ZipEndian.writeU32LE(out, 16, _.central_dir_offset);
        ZipEndian.writeU16LE(out, 20, _.comment_len);
        return ZIP_EOCD_SIZE;
    }

    pub static fn parse(buf: []const u8): ZipEocd? {
        if (buf.len < ZIP_EOCD_SIZE) {
            return null;
        }
        if (ZipEndian.readU32LE(buf, 0) != ZIP_EOCD_SIG) {
            return null;
        }
        return ZipEocd {
            disk_num: ZipEndian.readU16LE(buf, 4),
            central_dir_disk: ZipEndian.readU16LE(buf, 6),
            entries_on_disk: ZipEndian.readU16LE(buf, 8),
            total_entries: ZipEndian.readU16LE(buf, 10),
            central_dir_size: ZipEndian.readU32LE(buf, 12),
            central_dir_offset: ZipEndian.readU32LE(buf, 16),
            comment_len: ZipEndian.readU16LE(buf, 20),
        };
    }

    /// Scans backward from the end of `buf` to locate and parse the End of Central Directory record.
    pub static fn findInBuffer(buf: []const u8): ZipEocd? {
        if (pos := findEocdOffsetSimd(buf)) {
            return ZipEocd.parse(buf[pos..]);
        }
        return null;
    }
}

pub struct ZipEntryView {
    name: []const u8 = "";
    compression_method: u16 = 0;
    crc32: u32 = 0;
    compressed_size: u32 = 0;
    uncompressed_size: u32 = 0;
    data: []const u8 = "";

    pub fn isStored(): bool {
        return _.compression_method == ZIP_METHOD_STORE;
    }

    pub fn verifyCrc(): bool {
        return Crc32.checksum(_.data) == _.crc32;
    }
}

/// Zero-copy sequential scanner over PKZIP Local File Entries in memory (100% backward compatible)
pub struct ZipReader {
    buf: []const u8;
    pos: usize = 0;

    pub static fn init(buf: []const u8): ZipReader {
        return ZipReader {
            buf: buf,
            pos: 0,
        };
    }

    pub fn next(*_): ZipEntryView? {
        if (_.pos + ZIP_LOCAL_HEADER_SIZE > _.buf.len) {
            return null;
        }
        if (hdr := ZipLocalHeader.parse(_.buf[_.pos..])) {
            const name_start = _.pos + ZIP_LOCAL_HEADER_SIZE;
            const name_len = hdr.name_len as usize;
            const extra_len = hdr.extra_len as usize;
            const comp_len = hdr.compressed_size as usize;
            if (name_len > _.buf.len - name_start) return null;
            const name_end = name_start + name_len;
            if (extra_len > _.buf.len - name_end) return null;
            const data_start = name_end + extra_len;
            if (comp_len > _.buf.len - data_start) return null;
            const data_end = data_start + comp_len;
            _.pos = data_end;
            return ZipEntryView {
                name: _.buf[name_start..name_end],
                compression_method: hdr.compression_method,
                crc32: hdr.crc32,
                compressed_size: hdr.compressed_size,
                uncompressed_size: hdr.uncompressed_size,
                data: _.buf[data_start..data_end],
            };
        }
        return null;
    }
}

/// Central Directory 条目完整描述符（含 ZIP64 解析后的 64-bit 尺寸与偏移）
pub struct ZipCdEntry {
    pub name: []const u8 = "",
    pub extra: []const u8 = "",
    pub comment: []const u8 = "",
    pub compression_method: u16 = 0,
    pub flags: u16 = 0,
    pub crc32: u32 = 0,
    pub compressed_size: u64 = 0,
    pub uncompressed_size: u64 = 0,
    pub local_header_offset: u64 = 0,
    pub is_utf8: bool = false,
    pub has_data_descriptor: bool = false,
    pub is_zip64: bool = false,

    pub fn isDirectory(): bool {
        return _.name.len > 0 && _.name[_.name.len - 1] == '/';
    }

    pub fn isSafePath(): bool {
        return isSafeZipEntryPath(_.name);
    }
}

/// 零堆 Central Directory 条目迭代器
pub struct ZipCdIter {
    cd_slice: []const u8 = "",
    pos: usize = 0,
    remaining_entries: u64 = 0,

    pub fn hasNext(): bool {
        return _.remaining_entries > 0;
    }

    pub fn next(*_): ZipCdEntry!ZipError {
        if (_.remaining_entries == 0) {
            return ZipError.InvalidCentralDirectory;
        }
        if (_.pos + ZIP_CENTRAL_DIR_SIZE > _.cd_slice.len) {
            return ZipError.InvalidCentralDirectory;
        }
        const hdr = ZipCentralHeader.parse(_.cd_slice[_.pos..]) orelse return ZipError.InvalidCentralDirectory;
        const name_start = _.pos + ZIP_CENTRAL_DIR_SIZE;
        const name_len = hdr.name_len as usize;
        const extra_len = hdr.extra_len as usize;
        const comment_len = hdr.comment_len as usize;

        if (name_len > _.cd_slice.len - name_start) return ZipError.InvalidCentralDirectory;
        const name_end = name_start + name_len;
        if (extra_len > _.cd_slice.len - name_end) return ZipError.InvalidCentralDirectory;
        const extra_end = name_end + extra_len;
        if (comment_len > _.cd_slice.len - extra_end) return ZipError.InvalidCentralDirectory;
        const next_pos = extra_end + comment_len;

        const name = _.cd_slice[name_start..name_end];
        const extra = _.cd_slice[name_end..extra_end];
        const comment = _.cd_slice[extra_end..next_pos];

        let uncomp: u64 = hdr.uncompressed_size as u64;
        let comp: u64 = hdr.compressed_size as u64;
        let lfh_off: u64 = hdr.local_header_offset as u64;
        let is_z64 = false;

        const need_uncomp = (hdr.uncompressed_size == 0xFFFFFFFF);
        const need_comp = (hdr.compressed_size == 0xFFFFFFFF);
        const need_off = (hdr.local_header_offset == 0xFFFFFFFF);
        const need_disk = (hdr.disk_num_start == 0xFFFF);
        if (need_uncomp || need_comp || need_off || need_disk) {
            const z64 = try parseZip64ExtraField(extra, need_uncomp, need_comp, need_off, need_disk);
            if (need_uncomp) uncomp = z64.uncompressed_size;
            if (need_comp) comp = z64.compressed_size;
            if (need_off) lfh_off = z64.local_header_offset;
            if (need_disk && z64.disk_start != 0) return ZipError.UnsupportedMultiDisk;
            is_z64 = z64.has_zip64;
        } else if (hdr.disk_num_start != 0) {
            return ZipError.UnsupportedMultiDisk;
        }

        _.pos = next_pos;
        _.remaining_entries -= 1;

        return ZipCdEntry{
            name: name,
            extra: extra,
            comment: comment,
            compression_method: hdr.compression_method,
            flags: hdr.flags,
            crc32: hdr.crc32,
            compressed_size: comp,
            uncompressed_size: uncomp,
            local_header_offset: lfh_off,
            is_utf8: (hdr.flags & ZIP_FLAG_UTF8) != 0,
            has_data_descriptor: (hdr.flags & ZIP_FLAG_DATA_DESCRIPTOR) != 0,
            is_zip64: is_z64,
        };
    }
}

/// 工业级基于 Central Directory 的 ZIP 归档读取与安全解压引擎（对标 zip-rs/zip2 ZipArchive）
pub struct ZipArchive {
    pub buf: []const u8 = "",
    pub eocd_offset: usize = 0,
    pub cd_offset: usize = 0,
    pub cd_size: usize = 0,
    pub total_entries: u64 = 0,
    pub comment: []const u8 = "",
    pub is_zip64: bool = false,

    /// 反向定位 EOCD（含 ZIP64 EOCD/Locator），无溢出校验 Central Directory 边界并打开归档
    pub static fn open(buf: []const u8): ZipArchive!ZipError {
        const eocd_off = findEocdOffsetSimd(buf) orelse return ZipError.EocdNotFound;
        const eocd = ZipEocd.parse(buf[eocd_off..]) orelse return ZipError.EocdNotFound;

        let total_entries: u64 = eocd.total_entries as u64;
        let cd_size: u64 = eocd.central_dir_size as u64;
        let cd_offset: u64 = eocd.central_dir_offset as u64;
        let is_z64 = false;

        const has_z64_marker = (eocd.disk_num == 0xFFFF) ||
            (eocd.central_dir_disk == 0xFFFF) ||
            (eocd.entries_on_disk == 0xFFFF) ||
            (eocd.total_entries == 0xFFFF) ||
            (eocd.central_dir_size == 0xFFFFFFFF) ||
            (eocd.central_dir_offset == 0xFFFFFFFF);

        if (has_z64_marker) {
            // 解析紧邻 EOCD 前 20 字节的 ZIP64 EOCD Locator (0x07064b50)
            if (eocd_off < ZIP64_EOCD_LOCATOR_SIZE) {
                return ZipError.InvalidZip64Extra;
            }
            const loc_off = eocd_off - ZIP64_EOCD_LOCATOR_SIZE;
            if (ZipEndian.readU32LE(buf, loc_off) != ZIP64_EOCD_LOCATOR_SIG) {
                return ZipError.InvalidZip64Extra;
            }
            const z64_disk = ZipEndian.readU32LE(buf, loc_off + 4);
            const z64_eocd_off_u64 = ZipEndian.readU64LE(buf, loc_off + 8);
            const total_disks = ZipEndian.readU32LE(buf, loc_off + 16);
            if (z64_disk != 0 || total_disks > 1) {
                return ZipError.UnsupportedMultiDisk;
            }
            if (z64_eocd_off_u64 > (loc_off as u64)) {
                return ZipError.InvalidZip64Extra;
            }
            const z64_off = z64_eocd_off_u64 as usize;
            if (z64_off + ZIP64_EOCD_MIN_SIZE > loc_off) {
                return ZipError.InvalidZip64Extra;
            }
            if (ZipEndian.readU32LE(buf, z64_off) != ZIP64_EOCD_SIG) {
                return ZipError.InvalidZip64Extra;
            }
            const z64_rec_size = ZipEndian.readU64LE(buf, z64_off + 4);
            if (z64_rec_size < 44) {
                return ZipError.InvalidZip64Extra;
            }
            const disk_no = ZipEndian.readU32LE(buf, z64_off + 16);
            const cd_disk_no = ZipEndian.readU32LE(buf, z64_off + 20);
            const entries_disk = ZipEndian.readU64LE(buf, z64_off + 24);
            const entries_total = ZipEndian.readU64LE(buf, z64_off + 32);
            if (disk_no != 0 || cd_disk_no != 0 || entries_disk != entries_total) {
                return ZipError.UnsupportedMultiDisk;
            }
            total_entries = entries_total;
            cd_size = ZipEndian.readU64LE(buf, z64_off + 40);
            cd_offset = ZipEndian.readU64LE(buf, z64_off + 48);
            is_z64 = true;
        } else {
            if (eocd.disk_num != 0 || eocd.central_dir_disk != 0 || eocd.entries_on_disk != eocd.total_entries) {
                return ZipError.UnsupportedMultiDisk;
            }
        }

        // 无溢出边界校验：cd_offset + cd_size <= eocd_offset
        if (cd_offset > (eocd_off as u64)) {
            return ZipError.InvalidCentralDirectory;
        }
        const cd_off_usize = cd_offset as usize;
        if (cd_size > ((eocd_off - cd_off_usize) as u64)) {
            return ZipError.InvalidCentralDirectory;
        }
        const cd_size_usize = cd_size as usize;

        // 防止伪造 total_entries 超出物理可能条目数（每条 CD 至少 46 字节）
        if (total_entries > ((cd_size_usize / ZIP_CENTRAL_DIR_SIZE) as u64)) {
            return ZipError.InvalidCentralDirectory;
        }

        const comment_start = eocd_off + ZIP_EOCD_SIZE;
        const comment = buf[comment_start..buf.len];

        return ZipArchive{
            buf: buf,
            eocd_offset: eocd_off,
            cd_offset: cd_off_usize,
            cd_size: cd_size_usize,
            total_entries: total_entries,
            comment: comment,
            is_zip64: is_z64,
        };
    }

    pub fn entryCount(): u64 {
        return _.total_entries;
    }

    pub fn archiveComment(): []const u8 {
        return _.comment;
    }

    pub fn iterEntries(): ZipCdIter {
        return ZipCdIter{
            cd_slice: _.buf[_.cd_offset .. _.cd_offset + _.cd_size],
            pos: 0,
            remaining_entries: _.total_entries,
        };
    }

    pub fn findEntry(name: []const u8): (ZipCdEntry?)!ZipError {
        let iter = _.iterEntries();
        while (iter.hasNext()) {
            const entry = iter.next() catch |e| return e;
            if (std.mem.eql(u8, entry.name, name)) {
                return entry;
            }
        }
        return null;
    }

    /// 安全解压单个条目（默认 64MiB 上限）：路径遍历校验 + CD/LFH 一致性校验 + Data Descriptor 兼容 + CRC32
    pub fn extractEntry(entry: ZipCdEntry): ([]u8)!ZipError {
        return _.extractEntryWithLimit(entry, ZIP_DEFAULT_MAX_EXTRACT_SIZE);
    }

    /// 带显式解压体积上限的安全解压（防异常膨胀耗尽资源）
    pub fn extractEntryWithLimit(entry: ZipCdEntry, max_output_size: usize): ([]u8)!ZipError {
        if (!isSafeZipEntryPath(entry.name)) {
            return ZipError.UnsafePath;
        }
        if ((entry.flags & ZIP_FLAG_ENCRYPTED) != 0) {
            return ZipError.EncryptedEntry;
        }
        if (entry.uncompressed_size > (max_output_size as u64)) {
            return ZipError.OutputTooLarge;
        }
        if (entry.local_header_offset > (_.cd_offset as u64)) {
            return ZipError.InvalidLocalHeader;
        }
        const lfh_off = entry.local_header_offset as usize;
        if (lfh_off + ZIP_LOCAL_HEADER_SIZE > _.buf.len) {
            return ZipError.InvalidLocalHeader;
        }
        const lfh = ZipLocalHeader.parse(_.buf[lfh_off..]) orelse return ZipError.InvalidLocalHeader;

        // CD 与 LFH 一致性校验（防双重解析歧义）
        if ((lfh.flags & ZIP_FLAG_ENCRYPTED) != 0) {
            return ZipError.EncryptedEntry;
        }
        if (lfh.compression_method != entry.compression_method) {
            return ZipError.HeaderMismatch;
        }
        const lfh_name_start = lfh_off + ZIP_LOCAL_HEADER_SIZE;
        const lfh_name_len = lfh.name_len as usize;
        const lfh_extra_len = lfh.extra_len as usize;
        if (lfh_name_len > _.buf.len - lfh_name_start) {
            return ZipError.TruncatedEntry;
        }
        const lfh_name = _.buf[lfh_name_start .. lfh_name_start + lfh_name_len];
        if (!std.mem.eql(u8, lfh_name, entry.name)) {
            return ZipError.HeaderMismatch;
        }
        if (!entry.has_data_descriptor && (lfh.flags & ZIP_FLAG_DATA_DESCRIPTOR) == 0) {
            if (lfh.crc32 != entry.crc32) {
                return ZipError.HeaderMismatch;
            }
            if (lfh.compressed_size != 0xFFFFFFFF && entry.compressed_size < 0xFFFFFFFF) {
                if ((lfh.compressed_size as u64) != entry.compressed_size) {
                    return ZipError.HeaderMismatch;
                }
            }
            if (lfh.uncompressed_size != 0xFFFFFFFF && entry.uncompressed_size < 0xFFFFFFFF) {
                if ((lfh.uncompressed_size as u64) != entry.uncompressed_size) {
                    return ZipError.HeaderMismatch;
                }
            }
        }

        const lfh_extra_start = lfh_name_start + lfh_name_len;
        if (lfh_extra_len > _.buf.len - lfh_extra_start) {
            return ZipError.TruncatedEntry;
        }
        const data_start = lfh_extra_start + lfh_extra_len;
        if (entry.compressed_size > ((_.buf.len - data_start) as u64)) {
            return ZipError.TruncatedEntry;
        }
        const comp_len = entry.compressed_size as usize;
        const comp_data = _.buf[data_start .. data_start + comp_len];
        const expected_len = entry.uncompressed_size as usize;

        if (entry.compression_method == ZIP_METHOD_STORE) {
            if (comp_len != expected_len) {
                return ZipError.TruncatedEntry;
            }
            const out = std.heap.page_allocator.alloc(u8, expected_len) catch return ZipError.OutOfMemory;
            copyStoredSimd(out, comp_data);
            if (Crc32.checksum(out) != entry.crc32) {
                std.heap.page_allocator.free(out);
                return ZipError.CrcMismatch;
            }
            return out;
        }

        if (entry.compression_method == ZIP_METHOD_DEFLATE) {
            const inflated = inflateRawWithLimit(comp_data, max_output_size) catch |e| {
                if (e == GzipError.OutputLimitExceeded) {
                    return ZipError.OutputTooLarge;
                }
                return ZipError.DecompressFailed;
            };
            if (inflated.len != expected_len) {
                std.heap.page_allocator.free(inflated);
                return ZipError.TruncatedEntry;
            }
            if (Crc32.checksum(inflated) != entry.crc32) {
                std.heap.page_allocator.free(inflated);
                return ZipError.CrcMismatch;
            }
            return inflated;
        }

        return ZipError.UnsupportedCompression;
    }

    /// 遍历并验证归档全部 Central Directory 条目与安全路径，返回条目总数
    pub fn validateAllEntries(): usize!ZipError {
        let iter = _.iterEntries();
        let count: usize = 0;
        while (iter.hasNext()) {
            const entry = iter.next() catch |e| return e;
            if (!isSafeZipEntryPath(entry.name)) {
                return ZipError.UnsafePath;
            }
            count += 1;
        }
        return count;
    }
}
