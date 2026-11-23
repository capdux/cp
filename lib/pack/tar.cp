// Reference-hardened against alexcrichton/tar-rs (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/libarchive/libarchive.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pack/tar.cp — 工业级 POSIX.1-1988 USTAR / GNU Tar 归档头与零拷贝读取器
// 对标 alexcrichton/tar-rs (MIT/Apache-2.0) 加固：
//   - 校验和双兼容（unsigned + 历史 signed 字节和校验，拦截篡改头）
//   - 数字字段支持标准八进制 + GNU Base-256 二进制扩展编码（>8GB 与负值防溢出）
//   - 16-lane @Vector(16, u8) SIMD 加速 512B 全零块 EOF 检测、校验和累加与载荷拷贝
//   - 100% 向后兼容 TarTypeFlag / TarEntryView / TarHeader / TarReader
// ============================================================================

const std = @import("std");

pub const TAR_BLOCK_SIZE: usize = 512;

/// 16-lane SIMD 字节向量（用于 512B 全零块检测、头校验和与载荷搬运）
type Vec16u8 = @Vector(16, u8);
type Vec16u16 = @Vector(16, u16);

pub enum TarTypeFlag: u8 {
    Regular = '0',
    HardLink = '1',
    Symlink = '2',
    CharDevice = '3',
    BlockDevice = '4',
    Directory = '5',
    Fifo = '6',
    PaxExtendedHeader = 'x',
    PaxGlobalHeader = 'g',
    GnuLongName = 'L',
    GnuLongLink = 'K',
}

pub struct TarEntryView {
    name: []const u8 = "";
    size: u64 = 0;
    mode: u32 = 0o644;
    mtime: u64 = 0;
    typeflag: u8 = '0';
    data: []const u8 = "";

    pub fn isDirectory(): bool {
        return _.typeflag == '5' || (_.name.len > 0 && _.name[_.name.len - 1] == '/');
    }

    pub fn isRegularFile(): bool {
        return _.typeflag == '0' || _.typeflag == 0;
    }

    pub fn isSymlink(): bool {
        return _.typeflag == '2';
    }
}

pub struct TarHeader {
    /// 标量参考：载荷拷贝（用于与 SIMD 0 误差对拍）
    pub static fn copyPayloadScalar(dst: []u8, src: []const u8) {
        let n = if (dst.len < src.len) dst.len else src.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            dst[i] = src[i];
        }
    }

    /// 16-lane SIMD 加速载荷拷贝（与标量参考 0 误差）
    pub static fn copyPayloadSimd(dst: []u8, src: []const u8) {
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

    /// Parses an octal ASCII field from a 512-byte USTAR header block.
    pub static fn parseOctal(field: []const u8): u64 {
        let result: u64 = 0;
        let i: usize = 0;
        while (i < field.len) {
            const c = field[i];
            if (c == 0 || c == ' ') {
                if (result > 0) {
                    break;
                }
                i += 1;
                continue;
            }
            if (c >= '0' && c <= '7') {
                result = (result << 3) | ((c - '0') as u64);
            } else {
                break;
            }
            i += 1;
        }
        return result;
    }

    /// 解析无符号数字字段：兼容标准八进制与 GNU Base-256 二进制扩展编码（首字节 MSB=1）
    pub static fn parseNumeric(field: []const u8): u64? {
        if (field.len == 0) {
            return null;
        }
        // GNU Base-256 二进制扩展：首字节 bit 7 为 1
        if ((field[0] & 0x80) != 0) {
            // bit 6 为符号位（0xFF 开头表示负数，无符号解析拒绝）
            if ((field[0] & 0x40) != 0) {
                return null;
            }
            let val: u64 = (field[0] & 0x3F) as u64;
            let i: usize = 1;
            while (i < field.len) : (i += 1) {
                if (val > 0x00FFFFFFFFFFFFFF) {
                    return null;
                }
                val = (val << 8) | (field[i] as u64);
            }
            return val;
        }

        // 标准八进制 ASCII（跳过前导空格/NUL，遇到尾随空格/NUL 结束）
        let started = false;
        let result: u64 = 0;
        let i: usize = 0;
        while (i < field.len) : (i += 1) {
            const c = field[i];
            if (c == 0 || c == ' ') {
                if (started) {
                    break;
                }
                continue;
            }
            if (c < '0' || c > '7') {
                return null;
            }
            started = true;
            if (result > 0x1FFFFFFFFFFFFFFF) {
                return null;
            }
            result = (result << 3) | ((c - '0') as u64);
        }
        return result;
    }

    /// 解析有符号数字字段（支持 GNU Base-256 负时间戳等二补码编码）
    pub static fn parseSignedNumeric(field: []const u8): i64? {
        if (field.len == 0) {
            return null;
        }
        if ((field[0] & 0x80) != 0) {
            const is_neg = (field[0] & 0x40) != 0;
            let acc: u64 = if (is_neg) (((~(0 as u64)) << 6) | ((field[0] & 0x3F) as u64)) else ((field[0] & 0x3F) as u64);
            let i: usize = 1;
            while (i < field.len) : (i += 1) {
                acc = (acc << 8) | (field[i] as u64);
            }
            return @bitCast(acc);
        }
        const u = TarHeader.parseNumeric(field) orelse return null;
        if (u > 0x7FFFFFFFFFFFFFFF) {
            return null;
        }
        return u as i64;
    }

    /// Writes a fixed-width octal ASCII string (null-terminated) into `out`.
    pub static fn writeOctal(out: []u8, val: u64) {
        if (out.len == 0) {
            return;
        }
        let i: usize = 0;
        while (i < out.len) {
            out[i] = '0';
            i += 1;
        }
        out[out.len - 1] = 0;
        let v = val;
        let pos: usize = out.len - 1;
        while (pos > 0 && v > 0) {
            pos -= 1;
            out[pos] = (48 as u8) + ((v & 7) as u8);
            v >>= 3;
        }
    }

    /// 标量参考：无符号 512B POSIX tar 头校验和（bytes 148..156 视作空格 ' '）
    pub static fn computeChecksumScalar(block: []const u8): u32 {
        if (block.len < TAR_BLOCK_SIZE) {
            return 0;
        }
        let sum: u32 = 0;
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            if (i >= 148 && i < 156) {
                sum += 32; // ' '
            } else {
                sum += (block[i] as u32);
            }
            i += 1;
        }
        return sum;
    }

    /// 16-lane SIMD 加速：无符号 512B tar 头校验和（与标量参考 0 误差）
    pub static fn computeChecksumSimd(block: []const u8): u32 {
        if (block.len < TAR_BLOCK_SIZE) {
            return 0;
        }
        let total: u32 = 0;
        let offset: usize = 0;
        while (offset < TAR_BLOCK_SIZE) {
            const v8: Vec16u8 = block[offset .. offset + 16][0..16].*;
            const v16: Vec16u16 = v8 as Vec16u16;
            total += @reduce(.Add, v16) as u32;
            offset += 16;
        }
        // 减去 bytes 148..156 原始值并加回 8 * 32 (' ')
        let j: usize = 148;
        while (j < 156) : (j += 1) {
            total -= (block[j] as u32);
            total += 32;
        }
        return total;
    }

    /// Computes the unsigned 512-byte POSIX tar header checksum (treating bytes 148..156 as ASCII spaces).
    pub static fn computeChecksum(block: []const u8): u32 {
        return TarHeader.computeChecksumSimd(block);
    }

    /// 计算历史旧式有符号字节和校验和（对标 tar-rs signed checksum 兼容）
    pub static fn computeSignedChecksum(block: []const u8): i32 {
        if (block.len < TAR_BLOCK_SIZE) {
            return 0;
        }
        let sum: i32 = 0;
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) : (i += 1) {
            if (i >= 148 && i < 156) {
                sum += 32;
            } else {
                const sb: i8 = @bitCast(block[i]);
                sum += (sb as i32);
            }
        }
        return sum;
    }

    /// 校验 512B tar 头校验和（同时兼容标准 unsigned 与历史 signed 校验和）
    pub static fn verifyChecksum(block: []const u8): bool {
        if (block.len < TAR_BLOCK_SIZE) {
            return false;
        }
        const expected = TarHeader.parseNumeric(block[148..156]) orelse return false;
        const actual_u = TarHeader.computeChecksumSimd(block) as u64;
        if (expected == actual_u) {
            return true;
        }
        const actual_s = TarHeader.computeSignedChecksum(block);
        if (actual_s >= 0 && expected == (actual_s as u64)) {
            return true;
        }
        return false;
    }

    /// 标量参考：检测 512B 全零 EOF 块
    pub static fn isZeroBlockScalar(block: []const u8): bool {
        if (block.len < TAR_BLOCK_SIZE) {
            return false;
        }
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            if (block[i] != 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 16-lane SIMD 加速：检测 512B 全零 EOF 块（与标量参考 0 误差）
    pub static fn isZeroBlockSimd(block: []const u8): bool {
        if (block.len < TAR_BLOCK_SIZE) {
            return false;
        }
        let acc: Vec16u8 = @splat(0);
        let offset: usize = 0;
        while (offset < TAR_BLOCK_SIZE) {
            const v: Vec16u8 = block[offset .. offset + 16][0..16].*;
            acc |= v;
            offset += 16;
        }
        return @reduce(.Or, acc) == 0;
    }

    /// Checks whether a 512-byte block is an all-zero End-Of-Archive marker.
    pub static fn isZeroBlock(block: []const u8): bool {
        return TarHeader.isZeroBlockSimd(block);
    }

    /// Encodes a 512-byte USTAR header block for a file with `name`, `size`, `mode`, `mtime`, and `typeflag`.
    pub static fn encodeHeader(
        out_block: []u8,
        name: []const u8,
        size: u64,
        mode: u32,
        mtime: u64,
        typeflag: u8
    ): bool {
        if (out_block.len < TAR_BLOCK_SIZE || name.len > 100) {
            return false;
        }
        let i: usize = 0;
        while (i < TAR_BLOCK_SIZE) {
            out_block[i] = 0;
            i += 1;
        }

        // Name (0..100)
        let n_idx: usize = 0;
        while (n_idx < name.len) {
            out_block[n_idx] = name[n_idx];
            n_idx += 1;
        }

        // Mode (100..108), Size (124..136), Mtime (136..148)
        TarHeader.writeOctal(out_block[100..108], mode as u64);
        TarHeader.writeOctal(out_block[108..116], 0);
        TarHeader.writeOctal(out_block[116..124], 0);
        TarHeader.writeOctal(out_block[124..136], size);
        TarHeader.writeOctal(out_block[136..148], mtime);

        // Typeflag (156)
        out_block[156] = if (typeflag == 0) ('0' as u8) else typeflag;

        // USTAR magic "ustar\x0000" (257..265)
        out_block[257] = 'u';
        out_block[258] = 's';
        out_block[259] = 't';
        out_block[260] = 'a';
        out_block[261] = 'r';
        out_block[262] = 0;
        out_block[263] = '0';
        out_block[264] = '0';

        // Checksum (148..156): 6 octal digits + '\0' + ' '
        const chksum = TarHeader.computeChecksum(out_block[0..TAR_BLOCK_SIZE]);
        TarHeader.writeOctal(out_block[148..155], chksum as u64);
        out_block[155] = ' ';
        return true;
    }
}

/// Zero-copy in-memory Tar archive iterator (BufferReadStream equivalent)
pub struct TarReader {
    buf: []const u8;
    pos: usize = 0;

    pub static fn init(buf: []const u8): TarReader {
        return TarReader {
            buf: buf,
            pos: 0,
        };
    }

    pub fn next(*_): TarEntryView? {
        if (_.pos + TAR_BLOCK_SIZE > _.buf.len) {
            return null;
        }
        const hdr = _.buf[_.pos .. _.pos + TAR_BLOCK_SIZE];
        if (TarHeader.isZeroBlock(hdr)) {
            return null;
        }

        // Verify checksum (unsigned + legacy signed)
        if (!TarHeader.verifyChecksum(hdr)) {
            return null;
        }

        // Extract null-terminated name (0..100)
        let name_end: usize = 0;
        while (name_end < 100 && hdr[name_end] != 0) {
            name_end += 1;
        }
        const name = hdr[0..name_end];
        const mode = (TarHeader.parseNumeric(hdr[100..108]) orelse 0o644) as u32;
        const size = TarHeader.parseNumeric(hdr[124..136]) orelse return null;
        const mtime = TarHeader.parseNumeric(hdr[136..148]) orelse 0;
        const typeflag = if (hdr[156] == 0) ('0' as u8) else hdr[156];

        _.pos += TAR_BLOCK_SIZE;
        if (size > ((_.buf.len - _.pos) as u64)) {
            return null;
        }
        const data_size = size as usize;
        const data = _.buf[_.pos .. _.pos + data_size];

        // Advance pos rounded up to 512-byte boundary (overflow-safe)
        const rem = data_size % TAR_BLOCK_SIZE;
        const pad = if (rem == 0) 0 else (TAR_BLOCK_SIZE - rem);
        if (pad <= _.buf.len - _.pos - data_size) {
            _.pos += data_size + pad;
        } else {
            _.pos += data_size;
        }

        return TarEntryView {
            name: name,
            size: size,
            mode: mode,
            mtime: mtime,
            typeflag: typeflag,
            data: data,
        };
    }
}
