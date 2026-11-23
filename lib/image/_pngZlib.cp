// ============================================================================
// lib/image/_pngZlib.cp — 零外部依赖纯 CP RFC 1950/1951 Zlib & IEEE CRC32 引擎
// 专供 lib/image/png.cp 使用，免外部 libz 链接依赖与跨模块符号冲突
// 支持完整 RFC 1951 Inflate (Stored / Fixed Huffman / Dynamic Huffman)
// 与 RFC 1950 Zlib 封装/解封、Adler32 校验及 IEEE 802.3 CRC32
// ============================================================================

const std = @import("std");

pub error PngZlibError {
    InvalidHeader,
    UnsupportedMethod,
    PresetDictUnsupported,
    TruncatedInput,
    InvalidBlockType,
    InvalidStoredLength,
    InvalidHuffmanTree,
    InvalidCode,
    InvalidDistance,
    Adler32Mismatch,
    OutputLimitExceeded,
    OutOfMemory,
}

/// IEEE 802.3 CRC32 (多项式 0xEDB88320)
pub fn pngCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= data[i] as u32;
        let b: u32 = 0;
        while (b < 8) : (b += 1) {
            let mask: u32 = if ((crc & 1) != 0) (0xEDB88320 as u32) else (0 as u32);
            crc = (crc >> 1) ^ mask;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/// RFC 1950 Adler32 校验和
pub fn pngAdler32(data: []const u8): u32 {
    let s1: u32 = 1;
    let s2: u32 = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        s1 = (s1 + (data[i] as u32)) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) | s1;
}

const LEN_BASE: [29]u16 = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
];

const LEN_EXTRA: [29]u8 = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
];

const DIST_BASE: [30]u16 = [
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
];

const DIST_EXTRA: [30]u8 = [
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
];

const CL_ORDER: [19]u8 = [
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
];

struct PngBitReader {
    data: []const u8,
    byte_pos: usize = 0,
    bit_buf: u32 = 0,
    bit_cnt: u8 = 0,

    fn ensureBits(*_, n: u8): bool {
        while (_.bit_cnt < n) {
            if (_.byte_pos >= _.data.len) return false;
            _.bit_buf |= (_.data[_.byte_pos] as u32) << (_.bit_cnt as u5);
            _.byte_pos += 1;
            _.bit_cnt += 8;
        }
        return true;
    }

    fn readBits(*_, n: u8): u32 ! PngZlibError {
        if (n == 0) return 0;
        if (!_.ensureBits(n)) return PngZlibError.TruncatedInput;
        let mask: u32 = ((1 as u32) << (n as u5)) - 1;
        let val = _.bit_buf & mask;
        _.bit_buf >>= (n as u5);
        _.bit_cnt -= n;
        return val;
    }

    fn alignToByte(*_) {
        let rem = _.bit_cnt & 7;
        if (rem > 0) {
            _.bit_buf >>= (rem as u5);
            _.bit_cnt -= rem;
        }
    }

    fn readAlignedU16Le(*_): u16 ! PngZlibError {
        _.alignToByte();
        let b0 = _.readBits(8) catch |e| return e;
        let b1 = _.readBits(8) catch |e| return e;
        return ((b1 << 8) | b0) as u16;
    }
}

/// 规范 Huffman 解码表（按位长排序，支持 RFC 1951 1..15 bit 编码）
struct PngHuffTable {
    counts: [16]u16 = [0; 16],
    symbols: [320]u16 = [0; 320],
    max_bits: u8 = 0,

    static fn build(lengths: []const u8): PngHuffTable ! PngZlibError {
        let tbl = PngHuffTable{};
        let i: usize = 0;
        while (i < lengths.len) : (i += 1) {
            let l = lengths[i];
            if (l > 15) return PngZlibError.InvalidHuffmanTree;
            if (l > 0) {
                tbl.counts[l as usize] += 1;
                if (l > tbl.max_bits) tbl.max_bits = l;
            }
        }
        if (tbl.max_bits == 0) {
            return tbl;
        }

        // 检查 Kraft 不等式防止超定树
        let left: i32 = 1;
        let bits: usize = 1;
        while (bits <= 15) : (bits += 1) {
            left = (left << 1) - (tbl.counts[bits] as i32);
            if (left < 0) return PngZlibError.InvalidHuffmanTree;
        }

        let offsets: [16]u16 = [0; 16];
        let acc: u16 = 0;
        bits = 1;
        while (bits <= 15) : (bits += 1) {
            offsets[bits] = acc;
            acc += tbl.counts[bits];
        }

        i = 0;
        while (i < lengths.len) : (i += 1) {
            let l = lengths[i];
            if (l > 0) {
                let idx = offsets[l as usize] as usize;
                if (idx >= 320) return PngZlibError.InvalidHuffmanTree;
                tbl.symbols[idx] = i as u16;
                offsets[l as usize] += 1;
            }
        }
        return tbl;
    }

    fn decode(reader: *PngBitReader): u16 ! PngZlibError {
        let code: i32 = 0;
        let first: i32 = 0;
        let index: i32 = 0;
        let len: usize = 1;
        while (len <= 15) : (len += 1) {
            let bit = reader.readBits(1) catch |e| return e;
            code = (code << 1) | (bit as i32);
            let count = _.counts[len] as i32;
            if (code - count < first) {
                let sym_idx = (index + (code - first)) as usize;
                if (sym_idx >= 320) return PngZlibError.InvalidCode;
                return _.symbols[sym_idx];
            }
            index += count;
            first = (first + count) << 1;
        }
        return PngZlibError.InvalidCode;
    }
}

fn buildFixedLitLen(): PngHuffTable ! PngZlibError {
    let lens: [288]u8 = [0; 288];
    let i: usize = 0;
    while (i <= 143) : (i += 1) lens[i] = 8;
    while (i <= 255) : (i += 1) lens[i] = 9;
    while (i <= 279) : (i += 1) lens[i] = 7;
    while (i <= 287) : (i += 1) lens[i] = 8;
    return PngHuffTable.build(lens[0..288]);
}

fn buildFixedDist(): PngHuffTable ! PngZlibError {
    let lens: [32]u8 = [5; 32];
    return PngHuffTable.build(lens[0..32]);
}

fn decodeHuffmanBlock(
    reader: *PngBitReader,
    lit_tbl: PngHuffTable,
    dist_tbl: PngHuffTable,
    out_buf: []u8,
    out_pos: *usize,
): void ! PngZlibError {
    while (true) {
        let sym = lit_tbl.decode(reader) catch |e| return e;
        if (sym < 256) {
            if (out_pos.* >= out_buf.len) return PngZlibError.OutputLimitExceeded;
            out_buf[out_pos.*] = sym as u8;
            out_pos.* += 1;
        } else if (sym == 256) {
            return;
        } else if (sym <= 285) {
            let li = (sym - 257) as usize;
            let extra_l = reader.readBits(LEN_EXTRA[li]) catch |e| return e;
            let match_len = (LEN_BASE[li] as usize) + (extra_l as usize);

            let dsym = dist_tbl.decode(reader) catch |e| return e;
            if (dsym >= 30) return PngZlibError.InvalidDistance;
            let di = dsym as usize;
            let extra_d = reader.readBits(DIST_EXTRA[di]) catch |e| return e;
            let dist = (DIST_BASE[di] as usize) + (extra_d as usize);
            if (dist == 0 || dist > out_pos.*) return PngZlibError.InvalidDistance;
            if (out_pos.* + match_len > out_buf.len) return PngZlibError.OutputLimitExceeded;

            let k: usize = 0;
            while (k < match_len) : (k += 1) {
                out_buf[out_pos.*] = out_buf[out_pos.* - dist];
                out_pos.* += 1;
            }
        } else {
            return PngZlibError.InvalidCode;
        }
    }
}

/// 解压 RFC 1950 Zlib 字节流到指定上限的输出缓冲，校验头部 FCHECK 与尾部 Adler32
pub fn pngZlibDecompress(
    allocator: std.mem.Allocator,
    zlib_bytes: []const u8,
    max_output_bytes: usize,
): ([]u8) ! PngZlibError {
    if (zlib_bytes.len < 6) return PngZlibError.InvalidHeader;

    let cmf = zlib_bytes[0];
    let flg = zlib_bytes[1];
    if ((cmf & 0x0F) != 8) return PngZlibError.UnsupportedMethod;
    if ((cmf >> 4) > 7) return PngZlibError.InvalidHeader;
    if ((((cmf as u16) << 8) | (flg as u16)) % 31 != 0) return PngZlibError.InvalidHeader;
    if ((flg & 0x20) != 0) return PngZlibError.PresetDictUnsupported;

    let out_buf = allocator.alloc(u8, max_output_bytes) catch return PngZlibError.OutOfMemory;
    errdefer allocator.free(out_buf);

    let reader = PngBitReader{ data: zlib_bytes[2 .. zlib_bytes.len - 4] };
    let out_pos: usize = 0;

    let bfinal: u32 = 0;
    while (bfinal == 0) {
        bfinal = reader.readBits(1) catch |e| return e;
        let btype = reader.readBits(2) catch |e| return e;

        if (btype == 0) {
            // Stored (uncompressed) block
            let len_u16 = reader.readAlignedU16Le() catch |e| return e;
            let nlen_u16 = reader.readAlignedU16Le() catch |e| return e;
            if ((len_u16 ^ nlen_u16) != 0xFFFF) return PngZlibError.InvalidStoredLength;
            let blk_len = len_u16 as usize;
            if (out_pos + blk_len > out_buf.len) return PngZlibError.OutputLimitExceeded;
            let k: usize = 0;
            while (k < blk_len) : (k += 1) {
                let b = reader.readBits(8) catch |e| return e;
                out_buf[out_pos] = b as u8;
                out_pos += 1;
            }
        } else if (btype == 1) {
            // Fixed Huffman block
            let lit_tbl = buildFixedLitLen() catch |e| return e;
            let dist_tbl = buildFixedDist() catch |e| return e;
            decodeHuffmanBlock(&reader, lit_tbl, dist_tbl, out_buf, &out_pos) catch |e| return e;
        } else if (btype == 2) {
            // Dynamic Huffman block
            let hlit = (reader.readBits(5) catch |e| return e) + 257;
            let hdist = (reader.readBits(5) catch |e| return e) + 1;
            let hclen = (reader.readBits(4) catch |e| return e) + 4;
            if (hlit > 286 || hdist > 30) return PngZlibError.InvalidHuffmanTree;

            let cl_lens: [19]u8 = [0; 19];
            let ci: usize = 0;
            while (ci < hclen as usize) : (ci += 1) {
                cl_lens[CL_ORDER[ci] as usize] = (reader.readBits(3) catch |e| return e) as u8;
            }
            let cl_tbl = PngHuffTable.build(cl_lens[0..19]) catch |e| return e;

            let total_codes = (hlit + hdist) as usize;
            let all_lens: [320]u8 = [0; 320];
            let idx: usize = 0;
            while (idx < total_codes) {
                let sym = cl_tbl.decode(&reader) catch |e| return e;
                if (sym < 16) {
                    all_lens[idx] = sym as u8;
                    idx += 1;
                } else if (sym == 16) {
                    if (idx == 0) return PngZlibError.InvalidHuffmanTree;
                    let rep = (reader.readBits(2) catch |e| return e) + 3;
                    let prev_l = all_lens[idx - 1];
                    if (idx + (rep as usize) > total_codes) return PngZlibError.InvalidHuffmanTree;
                    let r: usize = 0;
                    while (r < rep as usize) : (r += 1) {
                        all_lens[idx] = prev_l;
                        idx += 1;
                    }
                } else if (sym == 17) {
                    let rep = (reader.readBits(3) catch |e| return e) + 3;
                    if (idx + (rep as usize) > total_codes) return PngZlibError.InvalidHuffmanTree;
                    let r: usize = 0;
                    while (r < rep as usize) : (r += 1) {
                        all_lens[idx] = 0;
                        idx += 1;
                    }
                } else if (sym == 18) {
                    let rep = (reader.readBits(7) catch |e| return e) + 11;
                    if (idx + (rep as usize) > total_codes) return PngZlibError.InvalidHuffmanTree;
                    let r: usize = 0;
                    while (r < rep as usize) : (r += 1) {
                        all_lens[idx] = 0;
                        idx += 1;
                    }
                } else {
                    return PngZlibError.InvalidHuffmanTree;
                }
            }

            let lit_tbl = PngHuffTable.build(all_lens[0 .. hlit as usize]) catch |e| return e;
            let dist_tbl = PngHuffTable.build(all_lens[hlit as usize .. total_codes]) catch |e| return e;
            decodeHuffmanBlock(&reader, lit_tbl, dist_tbl, out_buf, &out_pos) catch |e| return e;
        } else {
            return PngZlibError.InvalidBlockType;
        }
    }

    // 校验尾部 4 字节大端 Adler32
    let adler_off = zlib_bytes.len - 4;
    let expected_adler: u32 = ((zlib_bytes[adler_off] as u32) << 24) |
        ((zlib_bytes[adler_off + 1] as u32) << 16) |
        ((zlib_bytes[adler_off + 2] as u32) << 8) |
        (zlib_bytes[adler_off + 3] as u32);
    let actual_adler = pngAdler32(out_buf[0..out_pos]);
    if (actual_adler != expected_adler) {
        return PngZlibError.Adler32Mismatch;
    }

    // 返回精确长度切片（若未填满则拷贝到精确大小的切片并释放大缓冲）
    if (out_pos == max_output_bytes) {
        return out_buf;
    }
    let exact = allocator.alloc(u8, out_pos) catch return PngZlibError.OutOfMemory;
    let cp_i: usize = 0;
    while (cp_i < out_pos) : (cp_i += 1) {
        exact[cp_i] = out_buf[cp_i];
    }
    allocator.free(out_buf);
    return exact;
}

/// 将原始扫描线封装为标准 RFC 1950 Zlib 字节流（RFC 1951 Stored 块 + Adler32，零外部依赖）
pub fn pngZlibCompress(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! PngZlibError {
    let max_blk: usize = 65535;
    let num_blocks: usize = if (input.len == 0) 1 else ((input.len + max_blk - 1) / max_blk);
    let total_len: usize = (2 as usize) + input.len + num_blocks * 5 + 4;

    let out = allocator.alloc(u8, total_len) catch return PngZlibError.OutOfMemory;
    let pos: usize = 0;

    // Zlib header: CMF=0x78 (deflate, 32K window), FLG=0x01 (FCHECK valid: 0x7801 % 31 == 0)
    out[pos] = 0x78;
    out[pos + 1] = 0x01;
    pos += 2;

    let src_off: usize = 0;
    let blk_idx: usize = 0;
    while (blk_idx < num_blocks) : (blk_idx += 1) {
        let remaining = input.len - src_off;
        let blk_len: usize = if (remaining > max_blk) max_blk else remaining;
        let is_last: u8 = if (blk_idx + 1 == num_blocks) 1 else 0;

        out[pos] = is_last; // BFINAL=is_last, BTYPE=00 (stored)
        pos += 1;

        let len16: u16 = blk_len as u16;
        let nlen16: u16 = len16 ^ 0xFFFF;
        out[pos] = (len16 & 0xFF) as u8;
        out[pos + 1] = ((len16 >> 8) & 0xFF) as u8;
        out[pos + 2] = (nlen16 & 0xFF) as u8;
        out[pos + 3] = ((nlen16 >> 8) & 0xFF) as u8;
        pos += 4;

        let i: usize = 0;
        while (i < blk_len) : (i += 1) {
            out[pos + i] = input[src_off + i];
        }
        pos += blk_len;
        src_off += blk_len;
    }

    let adler = pngAdler32(input);
    out[pos] = ((adler >> 24) & 0xFF) as u8;
    out[pos + 1] = ((adler >> 16) & 0xFF) as u8;
    out[pos + 2] = ((adler >> 8) & 0xFF) as u8;
    out[pos + 3] = (adler & 0xFF) as u8;
    return out;
}
