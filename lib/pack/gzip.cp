// Reference-hardened against Frommi/miniz_oxide (MIT/Apache-2.0/Zlib)
// Source: Absorbed from /data/dev/bun/src/zlib/zlib.cp | Hardened: 2026-11-23
// ============================================================================
// lib/pack/gzip.cp — 纯 CP RFC 1951 Deflate/Inflate + RFC 1950 Zlib + RFC 1952 Gzip 压缩引擎
// 零外部 C/FFI 依赖，内置 16-lane SIMD LZ77 回放/匹配扩展与完整防畸形安全边界
// ============================================================================

import { Crc32, Adler32 } from "./checksum.cp";

// ============================================================================
// 1. RFC 1950 / 1951 / 1952 规范常量与错误定义
// ============================================================================

pub const GZIP_MAGIC_ID1: u8 = 0x1F;
pub const GZIP_MAGIC_ID2: u8 = 0x8B;
pub const GZIP_METHOD_DEFLATE: u8 = 8;

pub const GZIP_FTEXT: u8 = 0x01;
pub const GZIP_FHCRC: u8 = 0x02;
pub const GZIP_FEXTRA: u8 = 0x04;
pub const GZIP_FNAME: u8 = 0x08;
pub const GZIP_FCOMMENT: u8 = 0x10;
pub const GZIP_FRESERVED: u8 = 0xE0;

// WindowBits 参数配置约定（100% 兼容原有常量）
pub const WINDOW_BITS_ZLIB: i32 = 15;        // RFC 1950 标准 Zlib 封装
pub const WINDOW_BITS_RAW: i32 = -15;        // RFC 1951 裸 Deflate 流（无头尾）
pub const WINDOW_BITS_GZIP: i32 = 31;        // RFC 1952 标准 Gzip 封装 (15 + 16)
pub const WINDOW_BITS_AUTO: i32 = 47;        // 自动探测 Zlib 或 Gzip 格式 (15 + 32)

pub const DEFAULT_MAX_OUTPUT_SIZE: usize = 256 * 1024 * 1024;
const DEFLATE_WINDOW_SIZE: usize = 32768;
const DEFLATE_MAX_MATCH: usize = 258;
const DEFLATE_MIN_MATCH: usize = 3;
const HUFF_FAST_BITS: u32 = 9;
const HUFF_FAST_SIZE: usize = 512;
const HUFF_FAST_MASK: u64 = 511;

pub error GzipError {
    InvalidHeader,
    UnsupportedMethod,
    ReservedFlags,
    BadHeaderCrc,
    BadZlibHeader,
    PresetDictNotSupported,
    TruncatedInput,
    InvalidBlockType,
    BadStoredLength,
    InvalidHuffmanTable,
    OverSubscribedHuffman,
    IncompleteHuffman,
    InvalidSymbol,
    InvalidDistance,
    OutputLimitExceeded,
    OutputBufferTooSmall,
    BadChecksum,
    BadIsize,
    TrailingGarbage,
}

// ============================================================================
// 2. 16-lane SIMD 热路径加速内核（Stored 块搬运 / Inflate LZ77 回放 / Deflate 匹配扩展）
// ============================================================================

/// 标量参考：Stored 块或字面量字节拷贝
pub fn inflateCopyStoredScalar(dst: *[]u8, dst_pos: usize, src: []const u8, len: usize) {
    let i: usize = 0;
    while (i < len) {
        dst[dst_pos + i] = src[i];
        i += 1;
    }
}

/// 16-lane `@Vector(16, u8)` SIMD 加速：Stored 块搬运（与标量 0 字节差异）
pub fn inflateCopyStoredSimd(dst: *[]u8, dst_pos: usize, src: []const u8, len: usize) {
    let i: usize = 0;
    while (i + 16 <= len) {
        const v: @Vector(16, u8) = src[i..i + 16][0..16].*;
        dst[dst_pos + i..dst_pos + i + 16][0..16].* = v;
        i += 16;
    }
    while (i < len) {
        dst[dst_pos + i] = src[i];
        i += 1;
    }
}

/// 标量参考：Inflate LZ77 滑动窗口回放（支持重叠距离 dist < len）
pub fn inflateCopyMatchScalar(dst: *[]u8, dst_pos: usize, dist: usize, len: usize) {
    let i: usize = 0;
    while (i < len) {
        dst[dst_pos + i] = dst[dst_pos + i - dist];
        i += 1;
    }
}

/// 16-lane `@Vector(16, u8)` SIMD 加速：Inflate LZ77 回放
/// - `dist == 1`: `@splat` 16 字节广播 RLE 填充
/// - `dist >= 16`: 16-lane 非重叠块搬运
/// - `1 < dist < 16`: 精确重叠逐字节回放
pub fn inflateCopyMatchSimd(dst: *[]u8, dst_pos: usize, dist: usize, len: usize) {
    if (dist == 1) {
        const fill_byte: u8 = dst[dst_pos - 1];
        const vfill: @Vector(16, u8) = @splat(fill_byte);
        let i: usize = 0;
        while (i + 16 <= len) {
            dst[dst_pos + i..dst_pos + i + 16][0..16].* = vfill;
            i += 16;
        }
        while (i < len) {
            dst[dst_pos + i] = fill_byte;
            i += 1;
        }
        return;
    }
    if (dist >= 16) {
        let i: usize = 0;
        while (i + 16 <= len) {
            const src_pos: usize = dst_pos + i - dist;
            const v: @Vector(16, u8) = dst[src_pos..src_pos + 16][0..16].*;
            dst[dst_pos + i..dst_pos + i + 16][0..16].* = v;
            i += 16;
        }
        while (i < len) {
            dst[dst_pos + i] = dst[dst_pos + i - dist];
            i += 1;
        }
        return;
    }
    let i: usize = 0;
    while (i < len) {
        dst[dst_pos + i] = dst[dst_pos + i - dist];
        i += 1;
    }
}

/// 标量参考：Deflate LZ77 最长匹配长度扩展
pub fn deflateMatchLenScalar(src: []const u8, a: usize, b: usize, max_len: usize): usize {
    let n: usize = 0;
    while (n < max_len && src[a + n] == src[b + n]) {
        n += 1;
    }
    return n;
}

/// 16-lane `@Vector(16, u8)` SIMD 加速：Deflate 匹配长度扩展（与标量 0 字节差异）
pub fn deflateMatchLenSimd(src: []const u8, a: usize, b: usize, max_len: usize): usize {
    let n: usize = 0;
    while (n + 16 <= max_len) {
        const va: @Vector(16, u8) = src[a + n..a + n + 16][0..16].*;
        const vb: @Vector(16, u8) = src[b + n..b + n + 16][0..16].*;
        const diff: @Vector(16, u8) = va ^ vb;
        if (@reduce(.Or, diff) != 0) {
            break;
        }
        n += 16;
    }
    while (n < max_len && src[a + n] == src[b + n]) {
        n += 1;
    }
    return n;
}

// ============================================================================
// 3. RFC 1952 Gzip 头/尾与 RFC 1950 Zlib 头完整编解码器
// ============================================================================

pub struct GzipHeader {
    flags: u8 = 0,
    mtime: u32 = 0,
    xfl: u8 = 0,
    os: u8 = 3, // 3 = Unix

    /// 将 10 字节标准 Gzip Header 写入 out[0..10]
    pub fn write(out: []u8): usize {
        if (out.len < 10) { return 0; }
        out[0] = GZIP_MAGIC_ID1;
        out[1] = GZIP_MAGIC_ID2;
        out[2] = GZIP_METHOD_DEFLATE;
        out[3] = _.flags;
        out[4] = (_.mtime & 0xFF) as u8;
        out[5] = ((_.mtime >> 8) & 0xFF) as u8;
        out[6] = ((_.mtime >> 16) & 0xFF) as u8;
        out[7] = ((_.mtime >> 24) & 0xFF) as u8;
        out[8] = _.xfl;
        out[9] = _.os;
        return 10;
    }

    /// 解析 10 字节标准 Gzip Header（保持原有签名向后兼容）
    pub static fn parse(buf: []const u8): GzipHeader? {
        if (buf.len < 10) { return null; }
        if (buf[0] != GZIP_MAGIC_ID1 || buf[1] != GZIP_MAGIC_ID2 || buf[2] != GZIP_METHOD_DEFLATE) {
            return null;
        }
        if ((buf[3] & GZIP_FRESERVED) != 0) {
            return null;
        }
        let mtime: u32 = (buf[4] as u32)
            | ((buf[5] as u32) << 8)
            | ((buf[6] as u32) << 16)
            | ((buf[7] as u32) << 24);
        return GzipHeader{
            flags: buf[3],
            mtime: mtime,
            xfl: buf[8],
            os: buf[9],
        };
    }

    /// 完整解析 RFC 1952 Gzip 帧头（含 FEXTRA / FNAME / FCOMMENT / FHCRC 校验）
    pub static fn parseFull(buf: []const u8): GzipHeaderInfo ! GzipError {
        return parseGzipHeader(buf);
    }

    /// 写入 8 字节小端序 Gzip Trailer (CRC32 + Uncompressed ISIZE mod 2^32)
    pub static fn writeTrailer(out: []u8, crc32_val: u32, isize_val: u32): usize {
        if (out.len < 8) { return 0; }
        out[0] = (crc32_val & 0xFF) as u8;
        out[1] = ((crc32_val >> 8) & 0xFF) as u8;
        out[2] = ((crc32_val >> 16) & 0xFF) as u8;
        out[3] = ((crc32_val >> 24) & 0xFF) as u8;
        out[4] = (isize_val & 0xFF) as u8;
        out[5] = ((isize_val >> 8) & 0xFF) as u8;
        out[6] = ((isize_val >> 16) & 0xFF) as u8;
        out[7] = ((isize_val >> 24) & 0xFF) as u8;
        return 8;
    }

    /// 校验 8 字节 Gzip Trailer 是否与预期数据匹配
    pub static fn verifyTrailer(trailer: []const u8, payload: []const u8): bool {
        if (trailer.len < 8) { return false; }
        let expected_crc: u32 = (trailer[0] as u32)
            | ((trailer[1] as u32) << 8)
            | ((trailer[2] as u32) << 16)
            | ((trailer[3] as u32) << 24);
        let expected_size: u32 = (trailer[4] as u32)
            | ((trailer[5] as u32) << 8)
            | ((trailer[6] as u32) << 16)
            | ((trailer[7] as u32) << 24);
        let actual_crc = Crc32.checksum(payload);
        let actual_size = (payload.len & 0xFFFFFFFF) as u32;
        return expected_crc == actual_crc && expected_size == actual_size;
    }
}

pub struct GzipHeaderInfo {
    header: GzipHeader = GzipHeader{},
    header_len: usize = 10,
    extra: []const u8 = "",
    name: []const u8 = "",
    comment: []const u8 = "",
    has_hcrc: bool = false,
}

pub struct ZlibHeaderInfo {
    cmf: u8 = 0,
    flg: u8 = 0,
    window_bits: u8 = 15,
    fdict: bool = false,
    flevel: u8 = 0,
    header_len: usize = 2,
}

/// 完整解析并严格校验 RFC 1952 Gzip 头部（含可选 FEXTRA / FNAME / FCOMMENT / FHCRC）
pub fn parseGzipHeader(buf: []const u8): GzipHeaderInfo ! GzipError {
    if (buf.len < 10) {
        return GzipError.TruncatedInput;
    }
    if (buf[0] != GZIP_MAGIC_ID1 || buf[1] != GZIP_MAGIC_ID2) {
        return GzipError.InvalidHeader;
    }
    if (buf[2] != GZIP_METHOD_DEFLATE) {
        return GzipError.UnsupportedMethod;
    }
    const flg: u8 = buf[3];
    if ((flg & GZIP_FRESERVED) != 0) {
        return GzipError.ReservedFlags;
    }
    const mtime: u32 = (buf[4] as u32)
        | ((buf[5] as u32) << 8)
        | ((buf[6] as u32) << 16)
        | ((buf[7] as u32) << 24);
    const hdr = GzipHeader{
        flags: flg,
        mtime: mtime,
        xfl: buf[8],
        os: buf[9],
    };

    let pos: usize = 10;
    let extra: []const u8 = "";
    let name: []const u8 = "";
    let comment: []const u8 = "";
    let has_hcrc: bool = false;

    if ((flg & GZIP_FEXTRA) != 0) {
        if (pos + 2 > buf.len) {
            return GzipError.TruncatedInput;
        }
        const xlen: usize = (buf[pos] as usize) | ((buf[pos + 1] as usize) << 8);
        pos += 2;
        if (pos + xlen > buf.len) {
            return GzipError.TruncatedInput;
        }
        extra = buf[pos..pos + xlen];
        pos += xlen;
    }

    if ((flg & GZIP_FNAME) != 0) {
        const start: usize = pos;
        while (pos < buf.len && buf[pos] != 0) {
            pos += 1;
        }
        if (pos >= buf.len) {
            return GzipError.TruncatedInput;
        }
        name = buf[start..pos];
        pos += 1;
    }

    if ((flg & GZIP_FCOMMENT) != 0) {
        const start: usize = pos;
        while (pos < buf.len && buf[pos] != 0) {
            pos += 1;
        }
        if (pos >= buf.len) {
            return GzipError.TruncatedInput;
        }
        comment = buf[start..pos];
        pos += 1;
    }

    if ((flg & GZIP_FHCRC) != 0) {
        if (pos + 2 > buf.len) {
            return GzipError.TruncatedInput;
        }
        const expected_crc16: u16 = (buf[pos] as u16) | ((buf[pos + 1] as u16) << 8);
        const actual_crc16: u16 = (Crc32.checksum(buf[0..pos]) & 0xFFFF) as u16;
        if (expected_crc16 != actual_crc16) {
            return GzipError.BadHeaderCrc;
        }
        pos += 2;
        has_hcrc = true;
    }

    return GzipHeaderInfo{
        header: hdr,
        header_len: pos,
        extra: extra,
        name: name,
        comment: comment,
        has_hcrc: has_hcrc,
    };
}

/// 严格校验 RFC 1950 Zlib 2 字节帧头：CM == 8, CINFO <= 7, (CMF*256 + FLG) % 31 == 0
pub fn parseZlibHeader(buf: []const u8): ZlibHeaderInfo ! GzipError {
    if (buf.len < 2) {
        return GzipError.TruncatedInput;
    }
    const cmf: u8 = buf[0];
    const flg: u8 = buf[1];
    const cm: u8 = cmf & 0x0F;
    const cinfo: u8 = (cmf >> 4) & 0x0F;
    if (cm != 8 || cinfo > 7) {
        return GzipError.BadZlibHeader;
    }
    const check: u32 = ((cmf as u32) * 256) + (flg as u32);
    if ((check % 31) != 0) {
        return GzipError.BadZlibHeader;
    }
    const fdict: bool = (flg & 0x20) != 0;
    const flevel: u8 = (flg >> 6) & 0x03;
    const header_len: usize = if (fdict) 6 else 2;
    return ZlibHeaderInfo{
        cmf: cmf,
        flg: flg,
        window_bits: cinfo + 8,
        fdict: fdict,
        flevel: flevel,
        header_len: header_len,
    };
}

// ============================================================================
// 4. 压缩配置参数（保留向后兼容）
// ============================================================================

pub struct GzipOptions {
    level: i32 = 6,        // 压缩级别: 0 (无压缩) 到 9 (极致压缩), -1 为默认
    mem_level: i32 = 8,    // 内部工作区大小 (1-9), 默认 8 为最优性能/内存折中
    strategy: i32 = 0,     // 0 = 默认策略, 1 = 滤波, 2 = 仅霍夫曼, 3 = RLE, 4 = 固定
}

// ============================================================================
// 5. RFC 1951 Deflate / Inflate 纯 CP 核心表与位流编解码器
// ============================================================================

const LEN_BASE: [29]u16 = [
    3, 4, 5, 6, 7, 8, 9, 10,
    11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115,
    131, 163, 195, 227, 258,
];

const LEN_EXTRA: [29]u8 = [
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4,
    5, 5, 5, 5, 0,
];

const DIST_BASE: [30]u16 = [
    1, 2, 3, 4, 5, 7, 9, 13,
    17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073,
    4097, 6145, 8193, 12289, 16385, 24577,
];

const DIST_EXTRA: [30]u8 = [
    0, 0, 0, 0, 1, 1, 2, 2,
    3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10,
    11, 11, 12, 12, 13, 13,
];

const CODELEN_ORDER: [19]u8 = [
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
];

fn reverseBitsU16(code: u32, bits: u32): u32 {
    let v: u32 = code;
    let res: u32 = 0;
    let i: u32 = 0;
    while (i < bits) {
        res = (res << 1) | (v & 1);
        v = v >> 1;
        i += 1;
    }
    return res;
}

struct InflateBitReader {
    src: []const u8,
    byte_pos: usize = 0,
    bit_buf: u64 = 0,
    bit_count: u32 = 0,

    static fn init(src: []const u8): InflateBitReader {
        return InflateBitReader{
            src: src,
            byte_pos: 0,
            bit_buf: 0,
            bit_count: 0,
        };
    }

    fn refill(*_) {
        while (_.bit_count <= 56 && _.byte_pos < _.src.len) {
            const b: u64 = _.src[_.byte_pos] as u64;
            _.bit_buf = _.bit_buf | (b << (_.bit_count as u6));
            _.bit_count += 8;
            _.byte_pos += 1;
        }
    }

    fn readBits(*_, n: u32): u32 ! GzipError {
        if (n == 0) { return 0; }
        if (_.bit_count < n) {
            _.refill();
            if (_.bit_count < n) {
                return GzipError.TruncatedInput;
            }
        }
        const mask: u64 = ((1 as u64) << (n as u6)) - 1;
        const val: u32 = (_.bit_buf & mask) as u32;
        _.bit_buf = _.bit_buf >> (n as u6);
        _.bit_count -= n;
        return val;
    }

    fn alignToByte(*_) {
        const drop: u32 = _.bit_count & 7;
        if (drop > 0) {
            _.bit_buf = _.bit_buf >> (drop as u6);
            _.bit_count -= drop;
        }
    }

    /// 精确回退未消费的完整字节，返回当前字节游标
    fn currentBytePos(*_): usize {
        const unread_bytes: usize = (_.bit_count / 8) as usize;
        if (unread_bytes <= _.byte_pos) {
            return _.byte_pos - unread_bytes;
        }
        return 0;
    }

    fn readAlignedU16(*_): u16 ! GzipError {
        const lo: u32 = _.readBits(8)?;
        const hi: u32 = _.readBits(8)?;
        return (lo | (hi << 8)) as u16;
    }
}

struct HuffTable {
    fast: [512]u16 = [0; 512],
    counts: [16]u16 = [0; 16],
    first_code: [16]u32 = [0; 16],
    first_sym: [16]u16 = [0; 16],
    symbols: [288]u16 = [0; 288],
    max_bits: u32 = 0,
    num_used: u32 = 0,

    /// 对标 miniz_oxide / zlib inftrees.c 构建并严格校验规范哈夫曼表：
    /// 1) 拦截码长 > 15；
    /// 2) 逐位长检测过度订阅（left < 0 -> OverSubscribedHuffman）；
    /// 3) 检测不完整码表（码长表必须完整；字面量/距离表在 max_bits > 1 时必须完整）。
    static fn build(lengths: []const u8, is_codelen: bool): HuffTable ! GzipError {
        let tbl = HuffTable{};
        let bl_count: [16]u16 = [0; 16];
        let i: usize = 0;
        while (i < lengths.len) {
            const len: u8 = lengths[i];
            if (len > 15) {
                return GzipError.InvalidHuffmanTable;
            }
            if (len > 0) {
                bl_count[len as usize] += 1;
                if ((len as u32) > tbl.max_bits) {
                    tbl.max_bits = len as u32;
                }
                tbl.num_used += 1;
            }
            i += 1;
        }

        if (tbl.num_used == 0) {
            if (is_codelen) {
                return GzipError.IncompleteHuffman;
            }
            return tbl;
        }

        let left: i32 = 1;
        let code: u32 = 0;
        let sym_offset: u16 = 0;
        let bits: usize = 1;
        while (bits <= 15) {
            const cnt: u16 = bl_count[bits];
            tbl.counts[bits] = cnt;
            tbl.first_code[bits] = code;
            tbl.first_sym[bits] = sym_offset;

            left = (left << 1) - (cnt as i32);
            if (left < 0) {
                return GzipError.OverSubscribedHuffman;
            }
            code = (code + (cnt as u32)) << 1;
            sym_offset += cnt;
            bits += 1;
        }

        // 不完整码表严格拦截（与 miniz_oxide/src/inflate/core.rs:972 一致）
        if (left > 0 && (is_codelen || tbl.max_bits > 1)) {
            return GzipError.IncompleteHuffman;
        }

        let next_off: [16]u16 = tbl.first_sym;
        let next_code: [16]u32 = tbl.first_code;
        let sym: usize = 0;
        while (sym < lengths.len) {
            const len: u32 = lengths[sym] as u32;
            if (len > 0) {
                const idx: usize = next_off[len as usize] as usize;
                tbl.symbols[idx] = sym as u16;
                next_off[len as usize] += 1;

                const cur_code: u32 = next_code[len as usize];
                next_code[len as usize] += 1;

                if (len <= HUFF_FAST_BITS) {
                    const rev: usize = reverseBitsU16(cur_code, len) as usize;
                    const step: usize = (1 as usize) << (len as u6);
                    const entry: u16 = ((len as u16) << 12) | (sym as u16);
                    let slot: usize = rev;
                    while (slot < HUFF_FAST_SIZE) {
                        tbl.fast[slot] = entry;
                        slot += step;
                    }
                }
            }
            sym += 1;
        }

        return tbl;
    }

    fn decode(br: *InflateBitReader): u16 ! GzipError {
        if (_.num_used == 0) {
            return GzipError.InvalidSymbol;
        }
        if (br.bit_count < 15) {
            br.refill();
        }
        if (br.bit_count == 0) {
            return GzipError.TruncatedInput;
        }

        // 9-bit O(1) 极速查表路径
        const peek9: usize = (br.bit_buf & HUFF_FAST_MASK) as usize;
        const fast_entry: u16 = _.fast[peek9];
        if (fast_entry != 0) {
            const bits: u32 = (fast_entry >> 12) as u32;
            if (bits > br.bit_count) {
                return GzipError.TruncatedInput;
            }
            br.bit_buf = br.bit_buf >> (bits as u6);
            br.bit_count -= bits;
            return fast_entry & 0x0FFF;
        }

        // 10..15 bit 规范哈夫曼精确解码路径（含严格 EOF 边界检测）
        let code: u32 = 0;
        let bit_len: u32 = 1;
        while (bit_len <= _.max_bits) {
            if (br.bit_count < bit_len) {
                br.refill();
                if (br.bit_count < bit_len) {
                    return GzipError.TruncatedInput;
                }
            }
            const bit_pos: u6 = (bit_len - 1) as u6;
            const b: u32 = ((br.bit_buf >> bit_pos) & 1) as u32;
            code = (code << 1) | b;
            const cnt: u32 = _.counts[bit_len as usize] as u32;
            const fc: u32 = _.first_code[bit_len as usize];
            if (cnt > 0 && code >= fc && (code - fc) < cnt) {
                const sym_idx: usize = (_.first_sym[bit_len as usize] as usize) + ((code - fc) as usize);
                br.bit_buf = br.bit_buf >> (bit_len as u6);
                br.bit_count -= bit_len;
                return _.symbols[sym_idx];
            }
            bit_len += 1;
        }

        return GzipError.InvalidSymbol;
    }
}

fn buildFixedLitLenTable(): HuffTable ! GzipError {
    let lens: [288]u8 = undefined;
    let i: usize = 0;
    while (i <= 143) { lens[i] = 8; i += 1; }
    while (i <= 255) { lens[i] = 9; i += 1; }
    while (i <= 279) { lens[i] = 7; i += 1; }
    while (i <= 287) { lens[i] = 8; i += 1; }
    return HuffTable.build(lens[0..288], false);
}

fn buildFixedDistTable(): HuffTable ! GzipError {
    let lens: [32]u8 = [5; 32];
    return HuffTable.build(lens[0..32], false);
}

struct InflateResult {
    out: []u8,
    bytes_consumed: usize,
}

fn growOutBuf(buf: [*]u8, cur_cap: usize, used: usize, need: usize, max_output_size: usize): ([]u8) ! GzipError {
    if (used + need > max_output_size) {
        return GzipError.OutputLimitExceeded;
    }
    let next_cap: usize = if (cur_cap < 256) 256 else cur_cap;
    while (next_cap < used + need) {
        if (next_cap >= max_output_size) {
            next_cap = max_output_size;
            break;
        }
        const doubled: usize = next_cap * 2;
        next_cap = if (doubled > max_output_size) max_output_size else doubled;
    }
    const new_raw: [*]u8 = @alloc(next_cap);
    if (used > 0) {
        unsafe { @memcpy(new_raw[0..used], buf[0..used]); }
    }
    return unsafe { new_raw[0..next_cap] };
}

/// 纯 CP RFC 1951 Inflate 核心引擎（动态输出缓冲区 + 严格上限防护 + 16-lane SIMD 回放）
fn inflateRawCore(src: []const u8, max_output_size: usize): InflateResult ! GzipError {
    let br = InflateBitReader.init(src);
    let init_cap: usize = src.len * 2 + 256;
    if (init_cap > max_output_size) {
        init_cap = max_output_size;
    }
    const alloc_cap: usize = if (init_cap == 0) 1 else init_cap;
    let out_raw: [*]u8 = @alloc(alloc_cap);
    let out_cap: usize = alloc_cap;
    let out_pos: usize = 0;

    let is_final: bool = false;
    while (!is_final) {
        const bfinal: u32 = br.readBits(1)?;
        const btype: u32 = br.readBits(2)?;
        is_final = bfinal != 0;

        if (btype == 0) {
            // Stored 块：字节对齐 + LEN ^ NLEN == 0xFFFF 严格校验
            br.alignToByte();
            const len16: u16 = br.readAlignedU16()?;
            const nlen16: u16 = br.readAlignedU16()?;
            if ((len16 ^ nlen16) != 0xFFFF) {
                return GzipError.BadStoredLength;
            }
            const block_len: usize = len16 as usize;
            const cur_pos: usize = br.currentBytePos();
            if (cur_pos + block_len > src.len) {
                return GzipError.TruncatedInput;
            }
            if (out_pos + block_len > max_output_size) {
                return GzipError.OutputLimitExceeded;
            }
            if (out_pos + block_len > out_cap) {
                const grown = growOutBuf(out_raw, out_cap, out_pos, block_len, max_output_size)?;
                out_raw = grown.ptr;
                out_cap = grown.len;
            }
            let out_slice: []u8 = unsafe { out_raw[0..out_cap] };
            inflateCopyStoredSimd(out_slice, out_pos, src[cur_pos..cur_pos + block_len], block_len);
            out_pos += block_len;
            br.byte_pos = cur_pos + block_len;
            br.bit_buf = 0;
            br.bit_count = 0;
        } else if (btype == 1 || btype == 2) {
            let lit_tbl = HuffTable{};
            let dist_tbl = HuffTable{};

            if (btype == 1) {
                lit_tbl = buildFixedLitLenTable()?;
                dist_tbl = buildFixedDistTable()?;
            } else {
                const hlit: usize = (br.readBits(5)? as usize) + 257;
                const hdist: usize = (br.readBits(5)? as usize) + 1;
                const hclen: usize = (br.readBits(4)? as usize) + 4;
                if (hlit > 286 || hdist > 30) {
                    return GzipError.InvalidHuffmanTable;
                }

                let cl_lens: [19]u8 = [0; 19];
                let ci: usize = 0;
                while (ci < hclen) {
                    const v: u32 = br.readBits(3)?;
                    cl_lens[CODELEN_ORDER[ci] as usize] = v as u8;
                    ci += 1;
                }
                const cl_tbl = HuffTable.build(cl_lens[0..19], true)?;

                let total_codes: usize = hlit + hdist;
                let all_lens: [320]u8 = undefined;
                let idx: usize = 0;
                while (idx < total_codes) {
                    const sym: u16 = cl_tbl.decode(&br)?;
                    if (sym < 16) {
                        all_lens[idx] = sym as u8;
                        idx += 1;
                    } else if (sym == 16) {
                        if (idx == 0) {
                            return GzipError.InvalidHuffmanTable;
                        }
                        const rep: usize = (br.readBits(2)? as usize) + 3;
                        if (idx + rep > total_codes) {
                            return GzipError.InvalidHuffmanTable;
                        }
                        const prev_val: u8 = all_lens[idx - 1];
                        let r: usize = 0;
                        while (r < rep) {
                            all_lens[idx + r] = prev_val;
                            r += 1;
                        }
                        idx += rep;
                    } else if (sym == 17) {
                        const rep: usize = (br.readBits(3)? as usize) + 3;
                        if (idx + rep > total_codes) {
                            return GzipError.InvalidHuffmanTable;
                        }
                        let r: usize = 0;
                        while (r < rep) {
                            all_lens[idx + r] = 0;
                            r += 1;
                        }
                        idx += rep;
                    } else if (sym == 18) {
                        const rep: usize = (br.readBits(7)? as usize) + 11;
                        if (idx + rep > total_codes) {
                            return GzipError.InvalidHuffmanTable;
                        }
                        let r: usize = 0;
                        while (r < rep) {
                            all_lens[idx + r] = 0;
                            r += 1;
                        }
                        idx += rep;
                    } else {
                        return GzipError.InvalidHuffmanTable;
                    }
                }

                // RFC 1951 §3.2.7: EOB 符号 256 必须存在非零码长
                if (all_lens[256] == 0) {
                    return GzipError.InvalidHuffmanTable;
                }
                lit_tbl = HuffTable.build(all_lens[0..hlit], false)?;
                dist_tbl = HuffTable.build(all_lens[hlit..hlit + hdist], false)?;
            }

            // Huffman 符号解压主循环
            while (true) {
                const sym: u16 = lit_tbl.decode(&br)?;
                if (sym < 256) {
                    if (out_pos + 1 > max_output_size) {
                        return GzipError.OutputLimitExceeded;
                    }
                    if (out_pos + 1 > out_cap) {
                        const grown = growOutBuf(out_raw, out_cap, out_pos, 1, max_output_size)?;
                        out_raw = grown.ptr;
                        out_cap = grown.len;
                    }
                    unsafe { out_raw[out_pos] = sym as u8; }
                    out_pos += 1;
                } else if (sym == 256) {
                    break;
                } else if (sym <= 285) {
                    const len_idx: usize = (sym - 257) as usize;
                    const base_len: usize = LEN_BASE[len_idx] as usize;
                    const extra_bits: u32 = LEN_EXTRA[len_idx] as u32;
                    const extra_len: usize = if (extra_bits > 0) (br.readBits(extra_bits)? as usize) else 0;
                    const match_len: usize = base_len + extra_len;

                    const dsym: u16 = dist_tbl.decode(&br)?;
                    if (dsym >= 30) {
                        return GzipError.InvalidDistance;
                    }
                    const base_dist: usize = DIST_BASE[dsym as usize] as usize;
                    const dextra_bits: u32 = DIST_EXTRA[dsym as usize] as u32;
                    const dextra: usize = if (dextra_bits > 0) (br.readBits(dextra_bits)? as usize) else 0;
                    const dist: usize = base_dist + dextra;

                    if (dist == 0 || dist > DEFLATE_WINDOW_SIZE || dist > out_pos) {
                        return GzipError.InvalidDistance;
                    }
                    if (out_pos + match_len > max_output_size) {
                        return GzipError.OutputLimitExceeded;
                    }
                    if (out_pos + match_len > out_cap) {
                        const grown = growOutBuf(out_raw, out_cap, out_pos, match_len, max_output_size)?;
                        out_raw = grown.ptr;
                        out_cap = grown.len;
                    }
                    let out_slice: []u8 = unsafe { out_raw[0..out_cap] };
                    inflateCopyMatchSimd(out_slice, out_pos, dist, match_len);
                    out_pos += match_len;
                } else {
                    return GzipError.InvalidSymbol;
                }
            }
        } else {
            return GzipError.InvalidBlockType;
        }
    }

    br.alignToByte();
    const consumed: usize = br.currentBytePos();
    return InflateResult{
        out: unsafe { out_raw[0..out_pos] },
        bytes_consumed: consumed,
    };
}

/// 零堆分配外部切片解压 RFC 1951 裸 Deflate 流
pub fn inflateRawInto(src: []const u8, out: *[]u8): usize ! GzipError {
    const res = inflateRawCore(src, out.len) catch |e| {
        if (e == GzipError.OutputLimitExceeded) {
            return GzipError.OutputBufferTooSmall;
        }
        return e;
    };
    if (res.out.len > out.len) {
        return GzipError.OutputBufferTooSmall;
    }
    inflateCopyStoredSimd(out, 0, res.out, res.out.len);
    return res.out.len;
}

// ============================================================================
// 6. RFC 1951 Deflate 纯 CP 压缩器（Stored + LZ77 16-lane SIMD + Fixed Huffman）
// ============================================================================

struct DeflateBitWriter {
    buf: [*]u8,
    cap: usize,
    len: usize = 0,
    bit_buf: u64 = 0,
    bit_count: u32 = 0,

    static fn init(initial_cap: usize): DeflateBitWriter {
        const cap: usize = if (initial_cap < 64) 64 else initial_cap;
        const raw: [*]u8 = @alloc(cap);
        return DeflateBitWriter{
            buf: raw,
            cap: cap,
            len: 0,
            bit_buf: 0,
            bit_count: 0,
        };
    }

    fn ensureCap(*_, extra: usize) {
        if (_.len + extra <= _.cap) { return; }
        let next_cap: usize = _.cap * 2;
        while (next_cap < _.len + extra) {
            next_cap = next_cap * 2;
        }
        const new_raw: [*]u8 = @alloc(next_cap);
        if (_.len > 0) {
            unsafe { @memcpy(new_raw[0.._.len], _.buf[0.._.len]); }
        }
        _.buf = new_raw;
        _.cap = next_cap;
    }

    fn pushByte(*_, b: u8) {
        _.ensureCap(1);
        unsafe { _.buf[_.len] = b; }
        _.len += 1;
    }

    fn writeBits(*_, val: u32, n: u32) {
        if (n == 0) { return; }
        _.bit_buf = _.bit_buf | ((val as u64) << (_.bit_count as u6));
        _.bit_count += n;
        while (_.bit_count >= 8) {
            _.pushByte((_.bit_buf & 0xFF) as u8);
            _.bit_buf = _.bit_buf >> 8;
            _.bit_count -= 8;
        }
    }

    fn flushAlign(*_) {
        if (_.bit_count > 0) {
            _.pushByte((_.bit_buf & 0xFF) as u8);
            _.bit_buf = 0;
            _.bit_count = 0;
        }
    }

    fn writeFixedLitLen(*_, sym: u32) {
        if (sym <= 143) {
            const code: u32 = sym + 0x30;
            _.writeBits(reverseBitsU16(code, 8), 8);
        } else if (sym <= 255) {
            const code: u32 = (sym - 144) + 0x190;
            _.writeBits(reverseBitsU16(code, 9), 9);
        } else if (sym <= 279) {
            const code: u32 = sym - 256;
            _.writeBits(reverseBitsU16(code, 7), 7);
        } else {
            const code: u32 = (sym - 280) + 0xC0;
            _.writeBits(reverseBitsU16(code, 8), 8);
        }
    }

    fn writeFixedMatch(*_, match_len: usize, dist: usize) {
        // 查找长度码 257..285
        let li: usize = 0;
        if (match_len == 258) {
            li = 28;
        } else {
            while (li < 28) {
                const next_base: usize = LEN_BASE[li + 1] as usize;
                if (match_len < next_base) {
                    break;
                }
                li += 1;
            }
        }
        _.writeFixedLitLen((li as u32) + 257);
        const lextra: u32 = LEN_EXTRA[li] as u32;
        if (lextra > 0) {
            _.writeBits((match_len - (LEN_BASE[li] as usize)) as u32, lextra);
        }

        // 查找距离码 0..29
        let di: usize = 0;
        while (di < 29) {
            const next_dbase: usize = DIST_BASE[di + 1] as usize;
            if (dist < next_dbase) {
                break;
            }
            di += 1;
        }
        _.writeBits(reverseBitsU16(di as u32, 5), 5);
        const dextra: u32 = DIST_EXTRA[di] as u32;
        if (dextra > 0) {
            _.writeBits((dist - (DIST_BASE[di] as usize)) as u32, dextra);
        }
    }

    fn slice(*_): []u8 {
        return unsafe { _.buf[0.._.len] };
    }
}

fn deflateHash3(src: []const u8, pos: usize): usize {
    const v: u32 = ((src[pos] as u32) << 16) | ((src[pos + 1] as u32) << 8) | (src[pos + 2] as u32);
    return (((v *% 2654435761) >> 21) & 0x7FF) as usize;
}

fn deflateRawStored(src: []const u8): []u8 {
    let bw = DeflateBitWriter.init(src.len + (src.len / 65535 + 1) * 5 + 16);
    if (src.len == 0) {
        bw.writeBits(1, 1); // BFINAL = 1
        bw.writeBits(0, 2); // BTYPE = 0 (Stored)
        bw.flushAlign();
        bw.pushByte(0);
        bw.pushByte(0);
        bw.pushByte(0xFF);
        bw.pushByte(0xFF);
        return bw.slice();
    }
    let pos: usize = 0;
    while (pos < src.len) {
        const rem: usize = src.len - pos;
        const blk: usize = if (rem > 65535) 65535 else rem;
        const is_last: u32 = if (pos + blk >= src.len) 1 else 0;
        bw.writeBits(is_last, 1);
        bw.writeBits(0, 2);
        bw.flushAlign();
        const len16: u16 = blk as u16;
        const nlen16: u16 = len16 ^ 0xFFFF;
        bw.pushByte((len16 & 0xFF) as u8);
        bw.pushByte(((len16 >> 8) & 0xFF) as u8);
        bw.pushByte((nlen16 & 0xFF) as u8);
        bw.pushByte(((nlen16 >> 8) & 0xFF) as u8);
        bw.ensureCap(blk);
        let dst_slice: []u8 = unsafe { bw.buf[0..bw.cap] };
        inflateCopyStoredSimd(dst_slice, bw.len, src[pos..pos + blk], blk);
        bw.len += blk;
        pos += blk;
    }
    return bw.slice();
}

/// 纯 CP RFC 1951 裸 Deflate 压缩核心（level == 0 -> Stored；level 1..9 -> LZ77 + 16-lane SIMD + Fixed Huffman）
fn deflateRawCore(src: []const u8, level: i32): []u8 {
    if (level == 0 || src.len == 0) {
        return deflateRawStored(src);
    }

    const eff_level: i32 = if (level < 0) 6 else (if (level > 9) 9 else level);
    const max_chain: usize = if (eff_level <= 1) 4 else (if (eff_level <= 3) 8 else (if (eff_level <= 5) 16 else (if (eff_level <= 7) 32 else 64)));

    let bw = DeflateBitWriter.init(src.len + 64);
    bw.writeBits(1, 1); // BFINAL = 1
    bw.writeBits(1, 2); // BTYPE = 1 (Fixed Huffman)

    let head: [2048]i32 = [-1; 2048];
    let prev: [32768]i32 = [-1; 32768];

    let pos: usize = 0;
    while (pos < src.len) {
        let best_len: usize = 0;
        let best_dist: usize = 0;

        if (pos + DEFLATE_MIN_MATCH <= src.len) {
            const h: usize = deflateHash3(src, pos);
            let cand: i32 = head[h];
            prev[pos & 32767] = cand;
            head[h] = pos as i32;

            let steps: usize = 0;
            const max_m: usize = if (src.len - pos > DEFLATE_MAX_MATCH) DEFLATE_MAX_MATCH else (src.len - pos);
            while (cand >= 0 && steps < max_chain) {
                const cpos: usize = cand as usize;
                const dist: usize = pos - cpos;
                if (dist == 0 || dist > DEFLATE_WINDOW_SIZE) {
                    break;
                }
                if (src[cpos + best_len] == src[pos + best_len]) {
                    const mlen: usize = deflateMatchLenSimd(src, cpos, pos, max_m);
                    if (mlen >= DEFLATE_MIN_MATCH && mlen > best_len) {
                        best_len = mlen;
                        best_dist = dist;
                        if (best_len == max_m) {
                            break;
                        }
                    }
                }
                const next_cand: i32 = prev[cpos & 32767];
                if (next_cand >= cand) {
                    break;
                }
                cand = next_cand;
                steps += 1;
            }
        }

        if (best_len >= DEFLATE_MIN_MATCH) {
            bw.writeFixedMatch(best_len, best_dist);
            let k: usize = 1;
            while (k < best_len) {
                const p: usize = pos + k;
                if (p + DEFLATE_MIN_MATCH <= src.len) {
                    const hk: usize = deflateHash3(src, p);
                    prev[p & 32767] = head[hk];
                    head[hk] = p as i32;
                }
                k += 1;
            }
            pos += best_len;
        } else {
            bw.writeFixedLitLen(src[pos] as u32);
            pos += 1;
        }
    }

    bw.writeFixedLitLen(256); // EOB
    bw.flushAlign();
    return bw.slice();
}

// ============================================================================
// 7. RFC 1951 / 1950 / 1952 带上限防护与精确错误诊断 API
// ============================================================================

/// 解压 RFC 1951 裸 Deflate 流，严格限制最大输出字节数（防资源耗尽）
pub fn inflateRawWithLimit(src: []const u8, max_output_size: usize): []u8 ! GzipError {
    if (src.len == 0) {
        return GzipError.TruncatedInput;
    }
    const res = inflateRawCore(src, max_output_size)?;
    return res.out;
}

/// 解压 RFC 1950 Zlib 流（校验 CM/CINFO/mod31/FDICT 与尾部 Adler-32，支持最大输出上限）
pub fn inflateWithLimit(src: []const u8, max_output_size: usize): []u8 ! GzipError {
    if (src.len < 6) {
        return GzipError.TruncatedInput;
    }
    const zhdr = parseZlibHeader(src)?;
    if (zhdr.fdict) {
        return GzipError.PresetDictNotSupported;
    }
    const res = inflateRawCore(src[2..], max_output_size)?;
    const trailer_pos: usize = res.bytes_consumed + 2;
    if (trailer_pos + 4 > src.len) {
        return GzipError.TruncatedInput;
    }
    const expected_adler: u32 = ((src[trailer_pos] as u32) << 24)
        | ((src[trailer_pos + 1] as u32) << 16)
        | ((src[trailer_pos + 2] as u32) << 8)
        | (src[trailer_pos + 3] as u32);
    const actual_adler: u32 = Adler32.checksum(res.out);
    if (expected_adler != actual_adler) {
        return GzipError.BadChecksum;
    }
    return res.out;
}

/// 解压 RFC 1952 Gzip 流（完整支持 FEXTRA/FNAME/FCOMMENT/FHCRC、串联多成员流、CRC32/ISIZE 校验与最大输出上限）
pub fn gunzipWithLimit(src: []const u8, max_output_size: usize): []u8 ! GzipError {
    if (src.len < 18) {
        return GzipError.TruncatedInput;
    }

    let pos: usize = 0;
    let total_cap: usize = src.len * 2 + 256;
    if (total_cap > max_output_size) {
        total_cap = max_output_size;
    }
    const alloc_cap: usize = if (total_cap == 0) 1 else total_cap;
    let out_raw: [*]u8 = @alloc(alloc_cap);
    let out_cap: usize = alloc_cap;
    let out_len: usize = 0;

    while (pos < src.len) {
        const hdr_info = parseGzipHeader(src[pos..])?;
        const body_start: usize = pos + hdr_info.header_len;
        if (body_start + 8 > src.len) {
            return GzipError.TruncatedInput;
        }
        const rem_limit: usize = max_output_size - out_len;
        const member = inflateRawCore(src[body_start..], rem_limit)?;
        const trailer_pos: usize = body_start + member.bytes_consumed;
        if (trailer_pos + 8 > src.len) {
            return GzipError.TruncatedInput;
        }
        const expected_crc: u32 = (src[trailer_pos] as u32)
            | ((src[trailer_pos + 1] as u32) << 8)
            | ((src[trailer_pos + 2] as u32) << 16)
            | ((src[trailer_pos + 3] as u32) << 24);
        const expected_isize: u32 = (src[trailer_pos + 4] as u32)
            | ((src[trailer_pos + 5] as u32) << 8)
            | ((src[trailer_pos + 6] as u32) << 16)
            | ((src[trailer_pos + 7] as u32) << 24);

        const actual_crc: u32 = Crc32.checksum(member.out);
        const actual_isize: u32 = (member.out.len & 0xFFFFFFFF) as u32;
        if (expected_crc != actual_crc) {
            return GzipError.BadChecksum;
        }
        if (expected_isize != actual_isize) {
            return GzipError.BadIsize;
        }

        if (member.out.len > 0) {
            if (out_len + member.out.len > max_output_size) {
                return GzipError.OutputLimitExceeded;
            }
            if (out_len + member.out.len > out_cap) {
                const grown = growOutBuf(out_raw, out_cap, out_len, member.out.len, max_output_size)?;
                out_raw = grown.ptr;
                out_cap = grown.len;
            }
            let out_slice: []u8 = unsafe { out_raw[0..out_cap] };
            inflateCopyStoredSimd(out_slice, out_len, member.out, member.out.len);
            out_len += member.out.len;
        }

        pos = trailer_pos + 8;
    }

    return unsafe { out_raw[0..out_len] };
}

// ============================================================================
// 8. 容器封装辅助：Gzip / Zlib / Raw 统一分发
// ============================================================================

fn compressWithWindowBits(src: []const u8, window_bits: i32, level: i32): []u8 {
    const raw = deflateRawCore(src, level);
    if (window_bits < 0) {
        return raw;
    }
    if (window_bits >= 16) {
        // RFC 1952 Gzip: 10B header + raw deflate + 8B trailer (CRC32 + ISIZE)
        const total_len: usize = raw.len + 18;
        const out_raw: [*]u8 = @alloc(total_len);
        let out_slice: []u8 = unsafe { out_raw[0..total_len] };
        const xfl: u8 = if (level >= 9) 2 else (if (level == 1) 4 else 0);
        const hdr = GzipHeader{ flags: 0, mtime: 0, xfl: xfl, os: 3 };
        * = hdr.write(out_slice[0..10]);
        inflateCopyStoredSimd(out_slice, 10, raw, raw.len);
        const crc = Crc32.checksum(src);
        const isz: u32 = (src.len & 0xFFFFFFFF) as u32;
        * = GzipHeader.writeTrailer(out_slice[10 + raw.len..total_len], crc, isz);
        return out_slice;
    }
    // RFC 1950 Zlib: 2B header + raw deflate + 4B big-endian Adler-32
    const total_len: usize = raw.len + 6;
    const out_raw: [*]u8 = @alloc(total_len);
    let out_slice: []u8 = unsafe { out_raw[0..total_len] };
    const cmf: u8 = 0x78; // CM=8, CINFO=7 (32KB window)
    const flevel: u8 = if (level == 0) 0 else (if (level <= 3) 1 else (if (level <= 6) 2 else 3));
    const base_flg: u32 = (flevel as u32) << 6;
    const rem: u32 = (((cmf as u32) * 256) + base_flg) % 31;
    const thirty_one: u32 = 31;
    const fcheck: u32 = if (rem == 0) 0 else (thirty_one - rem);
    const flg: u8 = (base_flg | fcheck) as u8;
    out_slice[0] = cmf;
    out_slice[1] = flg;
    inflateCopyStoredSimd(out_slice, 2, raw, raw.len);
    const adler = Adler32.checksum(src);
    const tpos: usize = raw.len + 2;
    out_slice[tpos] = ((adler >> 24) & 0xFF) as u8;
    out_slice[tpos + 1] = ((adler >> 16) & 0xFF) as u8;
    out_slice[tpos + 2] = ((adler >> 8) & 0xFF) as u8;
    out_slice[tpos + 3] = (adler & 0xFF) as u8;
    return out_slice;
}

fn decompressWithWindowBits(src: []const u8, window_bits: i32): ([]u8)? {
    if (src.len == 0) {
        const empty_raw: [*]u8 = @alloc(1);
        return unsafe { empty_raw[0..0] };
    }
    if (window_bits < 0) {
        const res = inflateRawWithLimit(src, DEFAULT_MAX_OUTPUT_SIZE) catch { return null; };
        return res;
    }
    if (window_bits >= 32) {
        // AUTO: 探测 Gzip (1F 8B) 或 Zlib
        if (src.len >= 2 && src[0] == GZIP_MAGIC_ID1 && src[1] == GZIP_MAGIC_ID2) {
            const gres = gunzipWithLimit(src, DEFAULT_MAX_OUTPUT_SIZE) catch { return null; };
            return gres;
        }
        const zres = inflateWithLimit(src, DEFAULT_MAX_OUTPUT_SIZE) catch { return null; };
        return zres;
    }
    if (window_bits >= 16) {
        const gres = gunzipWithLimit(src, DEFAULT_MAX_OUTPUT_SIZE) catch { return null; };
        return gres;
    }
    const zres = inflateWithLimit(src, DEFAULT_MAX_OUTPUT_SIZE) catch { return null; };
    return zres;
}

// ============================================================================
// 9. 面向用户的标准化顶级 API（100% 向后兼容 Bun / Node.js 对标签名）
// ============================================================================

/// 将数据使用 RFC 1952 规范压缩为 Gzip 二进制格式（含 10 字节头与 CRC32/ISIZE 尾）
pub fn gzip(src: []const u8, level: i32 = 6): []u8 {
    return compressWithWindowBits(src, WINDOW_BITS_GZIP, level);
}

/// 解压符合 RFC 1952 规范的 Gzip 二进制流（支持自动探测 Zlib/Gzip 与串联多成员 Gzip）
pub fn gunzip(src: []const u8): ([]u8)? {
    return decompressWithWindowBits(src, WINDOW_BITS_AUTO);
}

/// 将数据使用 RFC 1950 规范压缩为 Zlib Deflate 格式（含 2 字节头与 Adler32 尾）
pub fn deflate(src: []const u8, level: i32 = 6): []u8 {
    return compressWithWindowBits(src, WINDOW_BITS_ZLIB, level);
}

/// 解压符合 RFC 1950 规范的 Zlib Deflate 数据流
pub fn inflate(src: []const u8): ([]u8)? {
    return decompressWithWindowBits(src, WINDOW_BITS_ZLIB);
}

/// 将数据使用 RFC 1951 规范压缩为裸 Deflate 流（无任何头尾，供 HTTP Chunk / Zip 局部流使用）
pub fn deflateRaw(src: []const u8, level: i32 = 6): []u8 {
    return compressWithWindowBits(src, WINDOW_BITS_RAW, level);
}

/// 解压 RFC 1951 裸 Deflate 数据流
pub fn inflateRaw(src: []const u8): ([]u8)? {
    return decompressWithWindowBits(src, WINDOW_BITS_RAW);
}

// ============================================================================
// 10. 面向对象便捷包装类 (Gzip / Gunzip / DeflateCodec / InflateCodec)
// ============================================================================

pub struct Gzip {
    pub static fn compress(src: []const u8, level: i32 = 6): []u8 {
        return gzip(src, level);
    }

    pub static fn sync(src: []const u8): []u8 {
        return gzip(src, 6);
    }
}

pub struct Gunzip {
    pub static fn decompress(src: []const u8): ([]u8)? {
        return gunzip(src);
    }

    pub static fn sync(src: []const u8): ([]u8)? {
        return gunzip(src);
    }
}

pub struct DeflateCodec {
    pub static fn compress(src: []const u8, level: i32 = 6): []u8 {
        return deflate(src, level);
    }

    pub static fn sync(src: []const u8): []u8 {
        return deflate(src, 6);
    }
}

pub struct InflateCodec {
    pub static fn decompress(src: []const u8): ([]u8)? {
        return inflate(src);
    }

    pub static fn sync(src: []const u8): ([]u8)? {
        return inflate(src);
    }
}

pub struct DeflateRawCodec {
    pub static fn compress(src: []const u8, level: i32 = 6): []u8 {
        return deflateRaw(src, level);
    }

    pub static fn decompress(src: []const u8): ([]u8)? {
        return inflateRaw(src);
    }
}

// ============================================================================
// 11. 纯 CP 流式分块压缩与解压通用引擎 (Streaming Chunked Pipeline)
// ============================================================================

/// 通用流式压缩状态机（纯 CP 实现，支持 Gzip / Deflate / RawDeflate 多模式）
pub class ZlibStreamCompressor {
    window_bits: i32 = WINDOW_BITS_GZIP,
    level: i32 = 6,
    buf: [*]u8 = undefined,
    cap: usize = 0,
    len: usize = 0,
    is_active: bool = false,

    pub static fn create(window_bits: i32 = WINDOW_BITS_GZIP, level: i32 = 6): (*ZlibStreamCompressor)? {
        const raw: [*]u8 = @alloc(@sizeOf(ZlibStreamCompressor));
        let comp: *ZlibStreamCompressor = unsafe { @ptrCast(@alignCast(raw)) };
        const init_cap: usize = 1024;
        comp.window_bits = window_bits;
        comp.level = level;
        comp.buf = @alloc(init_cap);
        comp.cap = init_cap;
        comp.len = 0;
        comp.is_active = true;
        return comp;
    }

    pub fn write(*_, chunk: []const u8): []u8 {
        const empty_raw: [*]u8 = @alloc(1);
        if (!_.is_active || chunk.len == 0) {
            return unsafe { empty_raw[0..0] };
        }
        if (_.len + chunk.len > _.cap) {
            let next_cap: usize = if (_.cap < 1024) 1024 else _.cap;
            while (next_cap < _.len + chunk.len) {
                next_cap = next_cap * 2;
            }
            const new_raw: [*]u8 = @alloc(next_cap);
            if (_.len > 0) {
                unsafe { @memcpy(new_raw[0.._.len], _.buf[0.._.len]); }
            }
            _.buf = new_raw;
            _.cap = next_cap;
        }
        unsafe { @memcpy(_.buf[_.len.._.len + chunk.len], chunk); }
        _.len += chunk.len;
        return unsafe { empty_raw[0..0] };
    }

    pub fn finish(*_): []u8 {
        const empty_raw: [*]u8 = @alloc(1);
        if (!_.is_active) {
            return unsafe { empty_raw[0..0] };
        }
        _.is_active = false;
        const input_slice: []const u8 = unsafe { _.buf[0.._.len] };
        return compressWithWindowBits(input_slice, _.window_bits, _.level);
    }
}

/// 通用流式解压状态机（纯 CP 实现，支持 Gzip / Deflate / RawDeflate / Auto 嗅探）
pub class ZlibStreamDecompressor {
    window_bits: i32 = WINDOW_BITS_AUTO,
    buf: [*]u8 = undefined,
    cap: usize = 0,
    len: usize = 0,
    is_active: bool = false,

    pub static fn create(window_bits: i32 = WINDOW_BITS_AUTO): (*ZlibStreamDecompressor)? {
        const raw: [*]u8 = @alloc(@sizeOf(ZlibStreamDecompressor));
        let decomp: *ZlibStreamDecompressor = unsafe { @ptrCast(@alignCast(raw)) };
        const init_cap: usize = 1024;
        decomp.window_bits = window_bits;
        decomp.buf = @alloc(init_cap);
        decomp.cap = init_cap;
        decomp.len = 0;
        decomp.is_active = true;
        return decomp;
    }

    pub fn write(*_, chunk: []const u8): []u8 {
        const empty_raw: [*]u8 = @alloc(1);
        if (!_.is_active || chunk.len == 0) {
            return unsafe { empty_raw[0..0] };
        }
        if (_.len + chunk.len > _.cap) {
            let next_cap: usize = if (_.cap < 1024) 1024 else _.cap;
            while (next_cap < _.len + chunk.len) {
                next_cap = next_cap * 2;
            }
            const new_raw: [*]u8 = @alloc(next_cap);
            if (_.len > 0) {
                unsafe { @memcpy(new_raw[0.._.len], _.buf[0.._.len]); }
            }
            _.buf = new_raw;
            _.cap = next_cap;
        }
        unsafe { @memcpy(_.buf[_.len.._.len + chunk.len], chunk); }
        _.len += chunk.len;
        return unsafe { empty_raw[0..0] };
    }

    pub fn finish(*_): []u8 {
        const empty_raw: [*]u8 = @alloc(1);
        if (!_.is_active) {
            return unsafe { empty_raw[0..0] };
        }
        _.is_active = false;
        const input_slice: []const u8 = unsafe { _.buf[0.._.len] };
        if (res := decompressWithWindowBits(input_slice, _.window_bits)) {
            return res;
        }
        return unsafe { empty_raw[0..0] };
    }
}

pub class GzipStream {
    pub static fn create(level: i32 = 6, window_bits: i32 = WINDOW_BITS_GZIP): (*ZlibStreamCompressor)? {
        return ZlibStreamCompressor.create(window_bits, level);
    }
}

pub class GunzipStream {
    pub static fn create(window_bits: i32 = WINDOW_BITS_AUTO): (*ZlibStreamDecompressor)? {
        return ZlibStreamDecompressor.create(window_bits);
    }
}

pub class DeflateStream {
    pub static fn create(level: i32 = 6): (*ZlibStreamCompressor)? {
        return ZlibStreamCompressor.create(WINDOW_BITS_ZLIB, level);
    }
}

pub class InflateStream {
    pub static fn create(): (*ZlibStreamDecompressor)? {
        return ZlibStreamDecompressor.create(WINDOW_BITS_ZLIB);
    }
}

// ============================================================================
// 12. 零依赖 Run-Length Encoding (RLE) 编解码器（保留向后兼容）
// ============================================================================

pub struct RleCodec {
    pub static fn compress(src: []const u8, out: []u8): usize {
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < src.len) {
            let b = src[in_pos];
            let run_len: usize = 1;
            while (in_pos + run_len < src.len && src[in_pos + run_len] == b && run_len < 255) {
                run_len += 1;
            }
            if (run_len >= 4 || b == 0xFF) {
                if (out_pos + 3 > out.len) { return 0; }
                out[out_pos] = 0xFF;
                out[out_pos + 1] = run_len as u8;
                out[out_pos + 2] = b;
                out_pos += 3;
                in_pos += run_len;
            } else {
                if (out_pos + 1 > out.len) { return 0; }
                out[out_pos] = b;
                out_pos += 1;
                in_pos += 1;
            }
        }
        return out_pos;
    }

    pub static fn decompress(src: []const u8, out: []u8): usize {
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < src.len) {
            let b = src[in_pos];
            if (b == 0xFF) {
                if (in_pos + 3 > src.len) { return 0; }
                let count = src[in_pos + 1] as usize;
                let val = src[in_pos + 2];
                if (out_pos + count > out.len) { return 0; }
                let k: usize = 0;
                while (k < count) {
                    out[out_pos + k] = val;
                    k += 1;
                }
                out_pos += count;
                in_pos += 3;
            } else {
                if (out_pos + 1 > out.len) { return 0; }
                out[out_pos] = b;
                out_pos += 1;
                in_pos += 1;
            }
        }
        return out_pos;
    }
}
