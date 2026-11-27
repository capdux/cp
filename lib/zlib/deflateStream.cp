// Source: cp/lib/zlib/deflate_stream.cp (RFC 1951 Prefix Decompressor & Stored-Block Compressor)

const std = @import("std");
import { ZlibError } from "./types.cp";

pub struct DeflateStreamResult {
    data: []u8,
    bytes_consumed: usize,
}

struct BitReader {
    data: []const u8,
    byte_pos: usize,
    bit_buf: u32,
    bit_count: u5,

    pub static fn init(data: []const u8): BitReader {
        return BitReader{
            .data = data,
            .byte_pos = 0,
            .bit_buf = 0,
            .bit_count = 0,
        };
    }

    pub fn ensureBits(*_, n: u5): void ! ZlibError {
        while (_.bit_count < n) {
            if (_.byte_pos >= _.data.len) {
                return ZlibError.UnexpectedEof;
            }
            const b = _.data[_.byte_pos] as u32;
            _.byte_pos += 1;
            _.bit_buf |= b << _.bit_count;
            _.bit_count += 8;
        }
    }

    pub fn readBits(*_, n: u5): u32 ! ZlibError {
        if (n == 0) {
            return 0;
        }
        _.ensureBits(n) ?;
        const mask: u32 = ((1 as u32) << n) - 1;
        const val = _.bit_buf & mask;
        _.bit_buf >>= n;
        _.bit_count -= n;
        return val;
    }

    pub fn alignToByte(*_) {
        const drop: u5 = _.bit_count & 7;
        _.bit_buf >>= drop;
        _.bit_count -= drop;
    }

    pub fn readByteAligned(*_): u8 ! ZlibError {
        if (_.bit_count >= 8) {
            const b = (_.bit_buf & 0xff) as u8;
            _.bit_buf >>= 8;
            _.bit_count -= 8;
            return b;
        }
        if (_.byte_pos >= _.data.len) {
            return ZlibError.UnexpectedEof;
        }
        const b = _.data[_.byte_pos];
        _.byte_pos += 1;
        return b;
    }

    pub fn bytesConsumed(): usize {
        const unused_bytes = (_.bit_count as usize) / 8;
        return _.byte_pos - unused_bytes;
    }
}

struct HuffTree {
    counts: [16]u16,
    symbols: [288]u16,

    pub static fn build(lengths: []const u8): HuffTree ! ZlibError {
        let tree = HuffTree{
            .counts = [0; 16],
            .symbols = [0; 288],
        };

        let i: usize = 0;
        while (i < lengths.len) {
            const len = lengths[i];
            if (len > 15) {
                return ZlibError.InvalidHuffmanCode;
            }
            if (len > 0) {
                tree.counts[len] += 1;
            }
            i += 1;
        }

        let left: i32 = 1;
        let bits: usize = 1;
        while (bits <= 15) {
            left <<= 1;
            left -= tree.counts[bits] as i32;
            if (left < 0) {
                return ZlibError.InvalidHuffmanCode;
            }
            bits += 1;
        }

        let offsets: [16]u16 = [0; 16];
        let total: u16 = 0;
        bits = 1;
        while (bits <= 15) {
            offsets[bits] = total;
            total += tree.counts[bits];
            bits += 1;
        }

        i = 0;
        while (i < lengths.len) {
            const len = lengths[i];
            if (len != 0) {
                const idx = offsets[len] as usize;
                if (idx >= 288) {
                    return ZlibError.InvalidHuffmanCode;
                }
                tree.symbols[idx] = i as u16;
                offsets[len] += 1;
            }
            i += 1;
        }

        return tree;
    }

    pub fn decode(br: *BitReader): u16 ! ZlibError {
        let code: i32 = 0;
        let first: i32 = 0;
        let index: i32 = 0;
        let len: usize = 1;
        while (len <= 15) {
            const bit = br.readBits(1) ?;
            code |= bit as i32;
            const count = _.counts[len] as i32;
            if (code - count < first) {
                const sym_idx = (index + (code - first)) as usize;
                return _.symbols[sym_idx];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
            len += 1;
        }
        return ZlibError.InvalidHuffmanCode;
    }
}

const LEN_BASE: [29]u16 = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
];
const LEN_EXTRA: [29]u5 = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
];
const DIST_BASE: [30]u16 = [
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
];
const DIST_EXTRA: [30]u5 = [
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
];
const CL_ORDER: [19]usize = [
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
];

fn buildFixedLitTree(): HuffTree ! ZlibError {
    let lengths: [288]u8 = undefined;
    let i: usize = 0;
    while (i <= 143) { lengths[i] = 8; i += 1; }
    while (i <= 255) { lengths[i] = 9; i += 1; }
    while (i <= 279) { lengths[i] = 7; i += 1; }
    while (i <= 287) { lengths[i] = 8; i += 1; }
    return HuffTree.build(lengths[0..]);
}

fn buildFixedDistTree(): HuffTree ! ZlibError {
    let lengths: [32]u8 = [5; 32];
    return HuffTree.build(lengths[0..]);
}

fn decodeHuffmanBlock(
    alloc: std.mem.Allocator,
    br: *BitReader,
    lit_tree: *const HuffTree,
    dist_tree: *const HuffTree,
    out: *std.ArrayList(u8),
    max_bytes: usize,
): void ! ZlibError {
    while (true) {
        const sym = lit_tree.decode(br) ?;
        if (sym < 256) {
            if (out.items.len >= max_bytes) {
                return ZlibError.OutputTooLarge;
            }
            out.append(alloc, sym as u8) catch return ZlibError.OutOfMemory;
        } else if (sym == 256) {
            break;
        } else {
            const len_idx = (sym - 257) as usize;
            if (len_idx >= 29) {
                return ZlibError.InvalidHuffmanCode;
            }
            const extra_len_bits = LEN_EXTRA[len_idx];
            const extra_len = br.readBits(extra_len_bits) ?;
            const match_len = (LEN_BASE[len_idx] as usize) + (extra_len as usize);

            const dist_sym = dist_tree.decode(br) ?;
            if (dist_sym >= 30) {
                return ZlibError.InvalidDistance;
            }
            const extra_dist_bits = DIST_EXTRA[dist_sym];
            const extra_dist = br.readBits(extra_dist_bits) ?;
            const dist = (DIST_BASE[dist_sym] as usize) + (extra_dist as usize);

            if (dist == 0 || dist > out.items.len) {
                return ZlibError.InvalidDistance;
            }
            if (out.items.len + match_len > max_bytes) {
                return ZlibError.OutputTooLarge;
            }

            let k: usize = 0;
            while (k < match_len) {
                const b = out.items[out.items.len - dist];
                out.append(alloc, b) catch return ZlibError.OutOfMemory;
                k += 1;
            }
        }
    }
}

/// Decompresses a single RFC 1951 DEFLATE stream prefix and returns both output and exact bytes consumed.
pub fn decompressDeflatePrefix(
    allocator: std.mem.Allocator,
    compressed: []const u8,
    max_bytes: usize,
): DeflateStreamResult ! ZlibError {
    let br = BitReader.init(compressed);
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    let bfinal: u32 = 0;
    while (bfinal == 0) {
        bfinal = br.readBits(1) ?;
        const btype = br.readBits(2) ?;

        if (btype == 0) {
            br.alignToByte();
            const l0 = br.readByteAligned() ? as u16;
            const l1 = br.readByteAligned() ? as u16;
            const n0 = br.readByteAligned() ? as u16;
            const n1 = br.readByteAligned() ? as u16;
            const len = l0 | (l1 << 8);
            const nlen = n0 | (n1 << 8);
            if ((len ^ 0xffff) != nlen) {
                return ZlibError.InvalidStoredLength;
            }
            const ulen = len as usize;
            if (out.items.len + ulen > max_bytes) {
                return ZlibError.OutputTooLarge;
            }
            let i: usize = 0;
            while (i < ulen) {
                const b = br.readByteAligned() ?;
                out.append(allocator, b) catch return ZlibError.OutOfMemory;
                i += 1;
            }
        } else if (btype == 1) {
            const lit_tree = buildFixedLitTree() ?;
            const dist_tree = buildFixedDistTree() ?;
            decodeHuffmanBlock(allocator, &br, &lit_tree, &dist_tree, &out, max_bytes) ?;
        } else if (btype == 2) {
            const hlit = (br.readBits(5) ?) as usize + 257;
            const hdist = (br.readBits(5) ?) as usize + 1;
            const hclen = (br.readBits(4) ?) as usize + 4;
            if (hlit > 286 || hdist > 32) {
                return ZlibError.InvalidHuffmanCode;
            }

            let cl_lengths: [19]u8 = [0; 19];
            let i: usize = 0;
            while (i < hclen) {
                cl_lengths[CL_ORDER[i]] = (br.readBits(3) ?) as u8;
                i += 1;
            }
            const cl_tree = HuffTree.build(cl_lengths[0..]) ?;

            let total_lengths: [320]u8 = undefined;
            const total_needed = hlit + hdist;
            let idx: usize = 0;
            while (idx < total_needed) {
                const sym = cl_tree.decode(&br) ?;
                if (sym < 16) {
                    total_lengths[idx] = sym as u8;
                    idx += 1;
                } else if (sym == 16) {
                    if (idx == 0) {
                        return ZlibError.InvalidHuffmanCode;
                    }
                    const repeat = (br.readBits(2) ?) as usize + 3;
                    const prev = total_lengths[idx - 1];
                    if (idx + repeat > total_needed) {
                        return ZlibError.InvalidHuffmanCode;
                    }
                    let r: usize = 0;
                    while (r < repeat) {
                        total_lengths[idx] = prev;
                        idx += 1;
                        r += 1;
                    }
                } else if (sym == 17) {
                    const repeat = (br.readBits(3) ?) as usize + 3;
                    if (idx + repeat > total_needed) {
                        return ZlibError.InvalidHuffmanCode;
                    }
                    let r: usize = 0;
                    while (r < repeat) {
                        total_lengths[idx] = 0;
                        idx += 1;
                        r += 1;
                    }
                } else if (sym == 18) {
                    const repeat = (br.readBits(7) ?) as usize + 11;
                    if (idx + repeat > total_needed) {
                        return ZlibError.InvalidHuffmanCode;
                    }
                    let r: usize = 0;
                    while (r < repeat) {
                        total_lengths[idx] = 0;
                        idx += 1;
                        r += 1;
                    }
                } else {
                    return ZlibError.InvalidHuffmanCode;
                }
            }

            const lit_tree = HuffTree.build(total_lengths[0..hlit]) ?;
            const dist_tree = HuffTree.build(total_lengths[hlit..total_needed]) ?;
            decodeHuffmanBlock(allocator, &br, &lit_tree, &dist_tree, &out, max_bytes) ?;
        } else {
            return ZlibError.InvalidBlockType;
        }
    }

    br.alignToByte();
    const consumed = br.bytesConsumed();
    const slice = out.toOwnedSlice(allocator) catch return ZlibError.OutOfMemory;
    return DeflateStreamResult{
        .data = slice,
        .bytes_consumed = consumed,
    };
}

/// Compresses `input` into uncompressed RFC 1951 stored blocks (level 0 / None).
pub fn compressStoredDeflate(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! ZlibError {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    if (input.len == 0) {
        // Single empty final stored block: BFINAL=1, BTYPE=00 -> 0x01, LEN=0, NLEN=0xffff
        out.append(allocator, 0x01) catch return ZlibError.OutOfMemory;
        out.append(allocator, 0x00) catch return ZlibError.OutOfMemory;
        out.append(allocator, 0x00) catch return ZlibError.OutOfMemory;
        out.append(allocator, 0xff) catch return ZlibError.OutOfMemory;
        out.append(allocator, 0xff) catch return ZlibError.OutOfMemory;
        return out.toOwnedSlice(allocator) catch return ZlibError.OutOfMemory;
    }

    let offset: usize = 0;
    while (offset < input.len) {
        const remaining = input.len - offset;
        const block_len = if (remaining > 65535) 65535 else remaining;
        const is_final = (offset + block_len) >= input.len;

        const header_byte: u8 = if (is_final) 0x01 else 0x00;
        const len_u16 = block_len as u16;
        const nlen_u16 = len_u16 ^ 0xffff;

        out.append(allocator, header_byte) catch return ZlibError.OutOfMemory;
        out.append(allocator, (len_u16 & 0xff) as u8) catch return ZlibError.OutOfMemory;
        out.append(allocator, ((len_u16 >> 8) & 0xff) as u8) catch return ZlibError.OutOfMemory;
        out.append(allocator, (nlen_u16 & 0xff) as u8) catch return ZlibError.OutOfMemory;
        out.append(allocator, ((nlen_u16 >> 8) & 0xff) as u8) catch return ZlibError.OutOfMemory;
        out.appendSlice(allocator, input[offset .. offset + block_len]) catch return ZlibError.OutOfMemory;

        offset += block_len;
    }

    return out.toOwnedSlice(allocator) catch return ZlibError.OutOfMemory;
}
