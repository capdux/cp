// Reference Only: Functional overlap with lib/pack/brotli.cp | Kept for comparison/analysis
// Source: cp/lib/brotli/brotli.cp (Pure CP RFC 7932 Brotli Meta-Block Framing & Codec)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");

/// Error set for Brotli framing, compression, and decompression.
pub err BrotliError {
    UnexpectedEof,
    InvalidWindowBits,
    InvalidMetaBlock,
    InvalidHuffmanCode,
    InvalidDistance,
    OutputTooLarge,
    WindowTooLarge,
    OutOfMemory,
}

/// Parsed RFC 7932 Brotli stream header.
pub struct BrotliHeader {
    pub window_bits: u8,
    pub window_size: usize,
}

pub const DEFAULT_WINDOW_BITS: u8 = 16;
pub const MIN_WINDOW_BITS: u8 = 10;
pub const MAX_WINDOW_BITS: u8 = 24;
pub const MAX_META_BLOCK_LEN: usize = 16777216;
pub const DEFAULT_MAX_WINDOW_BYTES: usize = 16777216;
pub const DEFAULT_MAX_OUTPUT_BYTES: usize = 67108864;

const MAX_HUFFMAN_BITS: usize = 15;
const NUM_LITERAL_SYMBOLS: usize = 256;
const NUM_INSERT_COPY_SYMBOLS: usize = 704;
const NUM_BLOCK_LEN_SYMBOLS: usize = 26;
const NUM_CODE_LEN_CODES: usize = 18;

struct BitReader {
    data: []const u8,
    byte_pos: usize,
    bit_pos: u4,

    static fn init(data: []const u8): BitReader {
        return BitReader{
            data: data,
            byte_pos: 0,
            bit_pos: 0,
        };
    }

    fn readBit(*_): u32 ! BrotliError {
        if (_.byte_pos >= _.data.len) {
            return BrotliError.UnexpectedEof;
        }
        const b = _.data[_.byte_pos];
        const bit = ((b >> (_.bit_pos as u3)) & 1) as u32;
        if (_.bit_pos == 7) {
            _.bit_pos = 0;
            _.byte_pos += 1;
        } else {
            _.bit_pos += 1;
        }
        return bit;
    }

    fn readBits(*_, n: u6): u32 ! BrotliError {
        if (n == 0) {
            return 0;
        }
        let val: u32 = 0;
        let i: u6 = 0;
        while (i < n) : (i += 1) {
            const bit = _.readBit()?;
            val |= (bit << (i as u5));
        }
        return val;
    }

    fn alignToByte(*_) {
        if (_.bit_pos != 0) {
            _.bit_pos = 0;
            _.byte_pos += 1;
        }
    }

    fn readBytes(*_, len: usize): []const u8 ! BrotliError {
        if (len > _.data.len - _.byte_pos) {
            return BrotliError.UnexpectedEof;
        }
        const slice = _.data[_.byte_pos..(_.byte_pos + len)];
        _.byte_pos += len;
        return slice;
    }

    fn skipBytes(*_, len: usize): !BrotliError {
        if (len > _.data.len - _.byte_pos) {
            return BrotliError.UnexpectedEof;
        }
        _.byte_pos += len;
    }
}

struct BitWriter {
    buf: std.ArrayList(u8),
    cur_byte: u8,
    bit_count: u4,

    static fn init(): BitWriter {
        return BitWriter{
            buf: std.ArrayList(u8).empty,
            cur_byte: 0,
            bit_count: 0,
        };
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    fn writeBit(*_, alloc: std.mem.Allocator, bit: u32): !BrotliError {
        if ((bit & 1) != 0) {
            _.cur_byte |= ((1 as u8) << (_.bit_count as u3));
        }
        if (_.bit_count == 7) {
            _.buf.append(alloc, _.cur_byte) catch return BrotliError.OutOfMemory;
            _.cur_byte = 0;
            _.bit_count = 0;
        } else {
            _.bit_count += 1;
        }
    }

    fn writeBits(*_, alloc: std.mem.Allocator, val: u32, n: u6): !BrotliError {
        let i: u6 = 0;
        while (i < n) : (i += 1) {
            const bit = (val >> (i as u5)) & 1;
            _.writeBit(alloc, bit)?;
        }
    }

    fn alignToByte(*_, alloc: std.mem.Allocator): !BrotliError {
        if (_.bit_count != 0) {
            _.buf.append(alloc, _.cur_byte) catch return BrotliError.OutOfMemory;
            _.cur_byte = 0;
            _.bit_count = 0;
        }
    }

    fn writeRawBytes(*_, alloc: std.mem.Allocator, bytes: []const u8): !BrotliError {
        _.buf.appendSlice(alloc, bytes) catch return BrotliError.OutOfMemory;
    }

    fn finish(*_, alloc: std.mem.Allocator): []u8 ! BrotliError {
        _.alignToByte(alloc)?;
        return _.buf.toOwnedSlice(alloc) catch return BrotliError.OutOfMemory;
    }
}

/// Computes RFC 7932 window size `(1 << window_bits) - 16` for valid `window_bits` (10..24).
pub fn windowSizeFromBits(window_bits: u8): usize ! BrotliError {
    if (window_bits < MIN_WINDOW_BITS || window_bits > MAX_WINDOW_BITS) {
        return BrotliError.InvalidWindowBits;
    }
    return ((1 as usize) << (window_bits as u6)) - 16;
}

fn decodeWindowBits(br: *BitReader): BrotliHeader ! BrotliError {
    const first = br.readBit()?;
    let wbits: u8 = 16;
    if (first != 0) {
        const n = br.readBits(3)?;
        if (n != 0) {
            wbits = (17 + n) as u8;
        } else {
            const m = br.readBits(3)?;
            if (m == 0) {
                wbits = 17;
            } else if (m == 1) {
                return BrotliError.InvalidWindowBits;
            } else {
                wbits = (8 + m) as u8;
            }
        }
    }
    const wsize = windowSizeFromBits(wbits)?;
    return BrotliHeader{
        window_bits: wbits,
        window_size: wsize,
    };
}

fn encodeWindowBits(bw: *BitWriter, alloc: std.mem.Allocator, window_bits: u8): !BrotliError {
    if (window_bits < MIN_WINDOW_BITS || window_bits > MAX_WINDOW_BITS) {
        return BrotliError.InvalidWindowBits;
    }
    if (window_bits == 16) {
        bw.writeBits(alloc, 0, 1)?;
    } else if (window_bits == 17) {
        bw.writeBits(alloc, 1, 1)?;
        bw.writeBits(alloc, 0, 3)?;
        bw.writeBits(alloc, 0, 3)?;
    } else if (window_bits >= 18) {
        bw.writeBits(alloc, 1, 1)?;
        bw.writeBits(alloc, (window_bits - 17) as u32, 3)?;
    } else {
        bw.writeBits(alloc, 1, 1)?;
        bw.writeBits(alloc, 0, 3)?;
        bw.writeBits(alloc, (window_bits - 8) as u32, 3)?;
    }
}

/// Parses only the RFC 7932 stream header (`WBITS`) from a Brotli payload.
pub fn parseHeader(compressed: []const u8): BrotliHeader ! BrotliError {
    let br = BitReader.init(compressed);
    return decodeWindowBits(&br);
}

/// Compresses `input` as RFC 7932 Uncompressed Meta-Blocks with explicit `window_bits` (10..24).
/// Automatically chunks inputs larger than 16 MiB (`MAX_META_BLOCK_LEN`) and encodes empty
/// inputs as a minimal `ISLAST=1, ISLASTEMPTY=1` frame (`0x06` for `window_bits=16`).
pub fn compressUncompressed(allocator: std.mem.Allocator, input: []const u8, window_bits: u8): []u8 ! BrotliError {
    if (window_bits < MIN_WINDOW_BITS || window_bits > MAX_WINDOW_BITS) {
        return BrotliError.InvalidWindowBits;
    }

    let bw = BitWriter.init();
    defer bw.deinit(allocator);

    encodeWindowBits(&bw, allocator, window_bits)?;

    if (input.len == 0) {
        // ISLAST = 1, ISLASTEMPTY = 1
        bw.writeBit(allocator, 1)?;
        bw.writeBit(allocator, 1)?;
        return bw.finish(allocator);
    }

    let offset: usize = 0;
    while (offset < input.len) {
        const remaining = input.len - offset;
        const chunk_len = if (remaining > MAX_META_BLOCK_LEN) MAX_META_BLOCK_LEN else remaining;
        const mlen_m1: u32 = (chunk_len - 1) as u32;

        let mnibbles: u32 = 0;
        let len_bits: u6 = 16;
        if (mlen_m1 <= 0xffff) {
            mnibbles = 0;
            len_bits = 16;
        } else if (mlen_m1 <= 0xfffff) {
            mnibbles = 1;
            len_bits = 20;
        } else {
            mnibbles = 2;
            len_bits = 24;
        }

        // ISLAST = 0 (uncompressed flag only exists when ISLAST == 0)
        bw.writeBit(allocator, 0)?;
        bw.writeBits(allocator, mnibbles, 2)?;
        bw.writeBits(allocator, mlen_m1, len_bits)?;
        // ISUNCOMPRESSED = 1
        bw.writeBit(allocator, 1)?;
        bw.alignToByte(allocator)?;
        bw.writeRawBytes(allocator, input[offset..(offset + chunk_len)])?;

        offset += chunk_len;
    }

    // Final empty meta-block: ISLAST = 1, ISLASTEMPTY = 1
    bw.writeBit(allocator, 1)?;
    bw.writeBit(allocator, 1)?;
    return bw.finish(allocator);
}

/// Compresses `input` using default `window_bits = 16` into RFC 7932 Brotli framing.
pub fn compress(allocator: std.mem.Allocator, input: []const u8): []u8 ! BrotliError {
    return compressUncompressed(allocator, input, DEFAULT_WINDOW_BITS);
}

// ============================================================================
// RFC 7932 Canonical Huffman Tree (LSB-first bit-reversed lookup)
// ============================================================================

struct HuffmanTree {
    counts: [16]u16,
    symbols: []u16,
    max_sym: usize,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.symbols.len > 0) {
            alloc.free(_.symbols);
        }
    }

    static fn build(alloc: std.mem.Allocator, lengths: []const u8): HuffmanTree ! BrotliError {
        let counts: [16]u16 = [0; 16];
        let nonzero_count: usize = 0;
        let single_sym: u16 = 0;

        let i: usize = 0;
        while (i < lengths.len) : (i += 1) {
            const len = lengths[i];
            if (len > MAX_HUFFMAN_BITS) {
                return BrotliError.InvalidHuffmanCode;
            }
            if (len > 0) {
                counts[len] += 1;
                nonzero_count += 1;
                single_sym = i as u16;
            }
        }

        if (nonzero_count == 0) {
            return BrotliError.InvalidHuffmanCode;
        }

        let symbols = alloc.alloc(u16, nonzero_count) catch return BrotliError.OutOfMemory;
        errdefer alloc.free(symbols);

        if (nonzero_count == 1) {
            // Single-symbol tree: 0-bit code
            symbols[0] = single_sym;
            return HuffmanTree{
                counts: counts,
                symbols: symbols,
                max_sym: lengths.len,
            };
        }

        // Check Kraft inequality for completeness / non-oversubscription
        let left: i32 = 1;
        let b: usize = 1;
        while (b <= MAX_HUFFMAN_BITS) : (b += 1) {
            left <<= 1;
            left -= counts[b] as i32;
            if (left < 0) {
                return BrotliError.InvalidHuffmanCode;
            }
        }
        if (left != 0) {
            return BrotliError.InvalidHuffmanCode;
        }

        let offsets: [16]u16 = [0; 16];
        let total: u16 = 0;
        b = 1;
        while (b <= MAX_HUFFMAN_BITS) : (b += 1) {
            offsets[b] = total;
            total += counts[b];
        }

        i = 0;
        while (i < lengths.len) : (i += 1) {
            const len = lengths[i];
            if (len > 0) {
                symbols[offsets[len]] = i as u16;
                offsets[len] += 1;
            }
        }

        return HuffmanTree{
            counts: counts,
            symbols: symbols,
            max_sym: lengths.len,
        };
    }

    fn decode(br: *BitReader): u16 ! BrotliError {
        if (_.symbols.len == 1) {
            return _.symbols[0];
        }
        let code: i32 = 0;
        let first: i32 = 0;
        let index: i32 = 0;
        let len: usize = 1;
        while (len <= MAX_HUFFMAN_BITS) : (len += 1) {
            const bit = br.readBit()? as i32;
            code |= bit;
            const count = _.counts[len] as i32;
            if (code - count < first) {
                const idx = (index + (code - first)) as usize;
                return _.symbols[idx];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return BrotliError.InvalidHuffmanCode;
    }
}

fn bitWidth(alphabet_size: usize): u6 {
    let bits: u6 = 0;
    let v: usize = 1;
    while (v < alphabet_size) {
        v <<= 1;
        bits += 1;
    }
    return bits;
}

const CODE_LEN_ORDER: [18]u8 = [1, 2, 3, 4, 0, 5, 17, 6, 16, 7, 8, 9, 10, 11, 12, 13, 14, 15];

fn readCodeLengthCode(br: *BitReader): u8 ! BrotliError {
    // RFC 7932 §3.5 fixed 4-bit code for code length code lengths (0..5)
    const b0 = br.readBit()?;
    const b1 = br.readBit()?;
    const low2 = b0 | (b1 << 1);
    if (low2 == 0) {
        return 0;
    } else if (low2 == 1) {
        const b2 = br.readBit()?;
        return if (b2 == 0) 2 else 4;
    } else if (low2 == 2) {
        return 3;
    } else {
        const b2 = br.readBit()?;
        if (b2 == 0) {
            return 1;
        }
        return 5;
    }
}

fn readPrefixCode(alloc: std.mem.Allocator, br: *BitReader, alphabet_size: usize): HuffmanTree ! BrotliError {
    let lengths = alloc.alloc(u8, alphabet_size) catch return BrotliError.OutOfMemory;
    defer alloc.free(lengths);
    @memset(lengths, 0);

    const hskip = br.readBits(2)?;
    if (hskip == 1) {
        // Simple prefix code
        const nsym = (br.readBits(2)? as usize) + 1;
        const max_bits = bitWidth(alphabet_size);
        let syms: [4]usize = [0; 4];
        let i: usize = 0;
        while (i < nsym) : (i += 1) {
            const sym = br.readBits(max_bits)? as usize;
            if (sym >= alphabet_size) {
                return BrotliError.InvalidHuffmanCode;
            }
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (syms[j] == sym) {
                    return BrotliError.InvalidHuffmanCode;
                }
            }
            syms[i] = sym;
        }
        if (nsym == 1) {
            lengths[syms[0]] = 1;
            // Single symbol handled in HuffmanTree.build (nonzero_count == 1)
            return HuffmanTree.build(alloc, lengths);
        } else if (nsym == 2) {
            lengths[syms[0]] = 1;
            lengths[syms[1]] = 1;
        } else if (nsym == 3) {
            lengths[syms[0]] = 1;
            lengths[syms[1]] = 2;
            lengths[syms[2]] = 2;
        } else {
            const tree_select = br.readBit()?;
            if (tree_select == 0) {
                lengths[syms[0]] = 2;
                lengths[syms[1]] = 2;
                lengths[syms[2]] = 2;
                lengths[syms[3]] = 2;
            } else {
                lengths[syms[0]] = 1;
                lengths[syms[1]] = 2;
                lengths[syms[2]] = 3;
                lengths[syms[3]] = 3;
            }
        }
        return HuffmanTree.build(alloc, lengths);
    }

    // Complex prefix code
    let cl_lengths: [18]u8 = [0; 18];
    let space: i32 = 32;
    let num_codes: usize = 0;
    let idx: usize = hskip as usize;
    while (idx < NUM_CODE_LEN_CODES && space > 0) : (idx += 1) {
        const code_idx = CODE_LEN_ORDER[idx] as usize;
        const cl_len = readCodeLengthCode(br)?;
        cl_lengths[code_idx] = cl_len;
        if (cl_len != 0) {
            space -= (32 >> (cl_len as u5));
            num_codes += 1;
        }
    }
    if (num_codes == 0 || (num_codes > 1 && space != 0)) {
        return BrotliError.InvalidHuffmanCode;
    }

    let cl_tree = HuffmanTree.build(alloc, &cl_lengths)?;
    defer cl_tree.deinit(alloc);

    let sym_idx: usize = 0;
    let prev_len: u8 = 8;
    let repeat_code: u16 = 0;
    let repeat: usize = 0;
    let cl_space: i32 = 32768;

    while (sym_idx < alphabet_size && cl_space > 0) {
        const code = cl_tree.decode(br)?;
        if (code < 16) {
            repeat = 0;
            lengths[sym_idx] = code as u8;
            sym_idx += 1;
            if (code != 0) {
                prev_len = code as u8;
                cl_space -= (32768 >> (code as u5));
            }
        } else {
            const extra_bits: u6 = if (code == 16) 2 else 3;
            const rep_val: u8 = if (code == 16) prev_len else 0;
            if (repeat_code != code) {
                repeat = 0;
                repeat_code = code;
            }
            const old_repeat = repeat;
            if (repeat > 0) {
                repeat -= 2;
                repeat <<= (extra_bits as u5);
            }
            repeat += (br.readBits(extra_bits)? as usize) + 3;
            const delta = repeat - old_repeat;
            if (sym_idx + delta > alphabet_size) {
                return BrotliError.InvalidHuffmanCode;
            }
            let k: usize = 0;
            while (k < delta) : (k += 1) {
                lengths[sym_idx] = rep_val;
                sym_idx += 1;
            }
            if (rep_val != 0) {
                cl_space -= (delta as i32) * (32768 >> (rep_val as u5));
            }
        }
    }

    if (cl_space != 0) {
        return BrotliError.InvalidHuffmanCode;
    }

    return HuffmanTree.build(alloc, lengths);
}

fn decodeVarUint8(br: *BitReader): usize ! BrotliError {
    const flag = br.readBit()?;
    if (flag == 0) {
        return 0;
    }
    const nbits = br.readBits(3)? as u6;
    if (nbits == 0) {
        return 1;
    }
    return ((1 as usize) << (nbits as u5)) + (br.readBits(nbits)? as usize);
}

const BLOCK_LEN_OFFSET: [26]u32 = [
    1, 5, 9, 13, 17, 25, 33, 41, 49, 65, 81, 97, 113,
    145, 177, 209, 241, 305, 369, 497, 753, 1265, 2289, 4337, 8433, 16625,
];
const BLOCK_LEN_NBITS: [26]u6 = [
    2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5,
    5, 5, 5, 6, 6, 7, 8, 9, 10, 11, 12, 13, 24,
];

fn decodeBlockLength(tree: *const HuffmanTree, br: *BitReader): usize ! BrotliError {
    const sym = tree.decode(br)? as usize;
    if (sym >= NUM_BLOCK_LEN_SYMBOLS) {
        return BrotliError.InvalidHuffmanCode;
    }
    const extra = br.readBits(BLOCK_LEN_NBITS[sym])?;
    return (BLOCK_LEN_OFFSET[sym] + extra) as usize;
}

fn decodeBlockType(
    num_types: usize,
    type_tree: *const HuffmanTree,
    br: *BitReader,
    cur_type: *usize,
    prev_type: *usize,
): !BrotliError {
    const sym = type_tree.decode(br)? as usize;
    let next_type: usize = 0;
    if (sym == 0) {
        next_type = prev_type.*;
    } else if (sym == 1) {
        next_type = cur_type.* + 1;
        if (next_type >= num_types) {
            next_type = 0;
        }
    } else {
        next_type = sym - 2;
        if (next_type >= num_types) {
            return BrotliError.InvalidMetaBlock;
        }
    }
    prev_type.* = cur_type.*;
    cur_type.* = next_type;
}

struct ContextMapResult {
    num_trees: usize,
    map: []u8,
}

fn decodeContextMap(
    alloc: std.mem.Allocator,
    br: *BitReader,
    map_size: usize,
): ContextMapResult ! BrotliError {
    const num_trees = decodeVarUint8(br)? + 1;
    let map = alloc.alloc(u8, map_size) catch return BrotliError.OutOfMemory;
    errdefer alloc.free(map);

    if (num_trees == 1) {
        @memset(map, 0);
        return ContextMapResult{ num_trees: 1, map: map };
    }

    const use_rle = br.readBit()?;
    let max_run_len_prefix: usize = 0;
    if (use_rle != 0) {
        max_run_len_prefix = (br.readBits(4)? as usize) + 1;
    }

    let tree = readPrefixCode(alloc, br, num_trees + max_run_len_prefix)?;
    defer tree.deinit(alloc);

    let i: usize = 0;
    while (i < map_size) {
        const code = tree.decode(br)? as usize;
        if (code == 0) {
            map[i] = 0;
            i += 1;
        } else if (code <= max_run_len_prefix) {
            const reps = ((1 as usize) << (code as u5)) + (br.readBits(code as u6)? as usize);
            let k: usize = 0;
            while (k < reps) : (k += 1) {
                if (i >= map_size) {
                    return BrotliError.InvalidMetaBlock;
                }
                map[i] = 0;
                i += 1;
            }
        } else {
            map[i] = (code - max_run_len_prefix) as u8;
            i += 1;
        }
    }

    const imtf = br.readBit()?;
    if (imtf != 0) {
        // Inverse Move-To-Front transform
        let mtf: [256]u8 = [0; 256];
        let m: usize = 0;
        while (m < 256) : (m += 1) {
            mtf[m] = m as u8;
        }
        i = 0;
        while (i < map_size) : (i += 1) {
            const idx = map[i] as usize;
            const val = mtf[idx];
            map[i] = val;
            let j = idx;
            while (j > 0) : (j -= 1) {
                mtf[j] = mtf[j - 1];
            }
            mtf[0] = val;
        }
    }

    return ContextMapResult{ num_trees: num_trees, map: map };
}

// RFC 7932 §5 Insert and Copy Length tables
const INSERT_OFFSET: [24]u32 = [
    0, 1, 2, 3, 4, 5, 6, 8, 10, 14, 18, 26,
    34, 50, 66, 98, 130, 194, 322, 578, 1090, 2114, 6210, 22594,
];
const INSERT_NBITS: [24]u6 = [
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3,
    4, 4, 5, 5, 6, 7, 8, 9, 10, 12, 14, 24,
];
const COPY_OFFSET: [24]u32 = [
    2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 18,
    22, 30, 38, 54, 70, 102, 134, 198, 326, 582, 1094, 2118,
];
const COPY_NBITS: [24]u6 = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2,
    3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 24,
];

const INSERT_RANGE_LUT: [11]u8 = [0, 0, 0, 8, 8, 16, 0, 8, 16, 16, 16];
const COPY_RANGE_LUT: [11]u8 = [0, 8, 0, 0, 8, 0, 16, 16, 8, 16, 16];

// RFC 7932 §7.1 Context lookup tables for modes 0..3 (LSB6, MSB6, UTF8, Signed)
fn literalContext(mode: u8, prev1: u8, prev2: u8): usize {
    if (mode == 0) {
        return (prev1 & 0x3f) as usize;
    } else if (mode == 1) {
        return (prev1 >> 2) as usize;
    } else if (mode == 2) {
        // UTF8 mode simplified high-bit classification
        let c1: usize = 0;
        if (prev1 >= 0xc0) {
            c1 = 12;
        } else if (prev1 >= 0x80) {
            c1 = 8;
        } else if (prev1 >= 0x40) {
            c1 = 4;
        }
        const c2 = (prev2 >> 6) as usize;
        return (c1 | c2) & 0x3f;
    } else {
        // Signed mode
        const c1 = ((prev1 >> 5) as usize) << 3;
        const c2 = (prev2 >> 5) as usize;
        return (c1 | c2) & 0x3f;
    }
}

fn freeTreeSlice(alloc: std.mem.Allocator, trees: []HuffmanTree) {
    let i: usize = 0;
    while (i < trees.len) : (i += 1) {
        trees[i].deinit(alloc);
    }
    alloc.free(trees);
}

fn decodeCompressedMetaBlock(
    alloc: std.mem.Allocator,
    br: *BitReader,
    out: *std.ArrayList(u8),
    mlen: usize,
    max_window_size: usize,
    dist_rb: *[4]usize,
    dist_rb_idx: *usize,
): !BrotliError {
    const start_out_len = out.items.len;
    const target_out_len = start_out_len + mlen;

    // 1. Block types and counts for Literal, Insert-and-Copy, Distance
    const nbltypes_l = decodeVarUint8(br)? + 1;
    let l_type_tree: HuffmanTree = undefined;
    let l_len_tree: HuffmanTree = undefined;
    let has_l_switch = false;
    let l_block_len: usize = mlen;
    if (nbltypes_l > 1) {
        l_type_tree = readPrefixCode(alloc, br, nbltypes_l + 2)?;
        has_l_switch = true;
        l_len_tree = readPrefixCode(alloc, br, NUM_BLOCK_LEN_SYMBOLS)?;
        l_block_len = decodeBlockLength(&l_len_tree, br)?;
    }
    defer if (has_l_switch) {
        l_type_tree.deinit(alloc);
        l_len_tree.deinit(alloc);
    }

    const nbltypes_i = decodeVarUint8(br)? + 1;
    let i_type_tree: HuffmanTree = undefined;
    let i_len_tree: HuffmanTree = undefined;
    let has_i_switch = false;
    let i_block_len: usize = mlen;
    if (nbltypes_i > 1) {
        i_type_tree = readPrefixCode(alloc, br, nbltypes_i + 2)?;
        has_i_switch = true;
        i_len_tree = readPrefixCode(alloc, br, NUM_BLOCK_LEN_SYMBOLS)?;
        i_block_len = decodeBlockLength(&i_len_tree, br)?;
    }
    defer if (has_i_switch) {
        i_type_tree.deinit(alloc);
        i_len_tree.deinit(alloc);
    }

    const nbltypes_d = decodeVarUint8(br)? + 1;
    let d_type_tree: HuffmanTree = undefined;
    let d_len_tree: HuffmanTree = undefined;
    let has_d_switch = false;
    let d_block_len: usize = mlen;
    if (nbltypes_d > 1) {
        d_type_tree = readPrefixCode(alloc, br, nbltypes_d + 2)?;
        has_d_switch = true;
        d_len_tree = readPrefixCode(alloc, br, NUM_BLOCK_LEN_SYMBOLS)?;
        d_block_len = decodeBlockLength(&d_len_tree, br)?;
    }
    defer if (has_d_switch) {
        d_type_tree.deinit(alloc);
        d_len_tree.deinit(alloc);
    }

    // 2. Distance parameters: NPOSTFIX (2 bits), NDIRECT (4 bits << NPOSTFIX)
    const npostfix = br.readBits(2)?;
    const ndirect = br.readBits(4)? << (npostfix as u5);
    const num_dist_syms = 16 + (ndirect as usize) + (48 << (npostfix as u5));

    // 3. Context modes for literal block types
    let context_modes = alloc.alloc(u8, nbltypes_l) catch return BrotliError.OutOfMemory;
    defer alloc.free(context_modes);
    let cm_i: usize = 0;
    while (cm_i < nbltypes_l) : (cm_i += 1) {
        context_modes[cm_i] = br.readBits(2)? as u8;
    }

    // 4. Context maps for Literals and Distances
    const lit_cmap = decodeContextMap(alloc, br, nbltypes_l * 64)?;
    defer alloc.free(lit_cmap.map);

    const dist_cmap = decodeContextMap(alloc, br, nbltypes_d * 4)?;
    defer alloc.free(dist_cmap.map);

    // 5. Prefix Code Trees
    let lit_trees = alloc.alloc(HuffmanTree, lit_cmap.num_trees) catch return BrotliError.OutOfMemory;
    let lit_built: usize = 0;
    errdefer freeTreeSlice(alloc, lit_trees[0..lit_built]);
    while (lit_built < lit_cmap.num_trees) : (lit_built += 1) {
        lit_trees[lit_built] = readPrefixCode(alloc, br, NUM_LITERAL_SYMBOLS)?;
    }
    defer freeTreeSlice(alloc, lit_trees);

    let iac_trees = alloc.alloc(HuffmanTree, nbltypes_i) catch return BrotliError.OutOfMemory;
    let iac_built: usize = 0;
    errdefer freeTreeSlice(alloc, iac_trees[0..iac_built]);
    while (iac_built < nbltypes_i) : (iac_built += 1) {
        iac_trees[iac_built] = readPrefixCode(alloc, br, NUM_INSERT_COPY_SYMBOLS)?;
    }
    defer freeTreeSlice(alloc, iac_trees);

    let dist_trees = alloc.alloc(HuffmanTree, dist_cmap.num_trees) catch return BrotliError.OutOfMemory;
    let dist_built: usize = 0;
    errdefer freeTreeSlice(alloc, dist_trees[0..dist_built]);
    while (dist_built < dist_cmap.num_trees) : (dist_built += 1) {
        dist_trees[dist_built] = readPrefixCode(alloc, br, num_dist_syms)?;
    }
    defer freeTreeSlice(alloc, dist_trees);

    // 6. Main Meta-Block Decode Loop
    let l_type: usize = 0;
    let l_prev_type: usize = 1;
    let i_type: usize = 0;
    let i_prev_type: usize = 1;
    let d_type: usize = 0;
    let d_prev_type: usize = 1;

    while (out.items.len < target_out_len) {
        if (has_i_switch && i_block_len == 0) {
            decodeBlockType(nbltypes_i, &i_type_tree, br, &i_type, &i_prev_type)?;
            i_block_len = decodeBlockLength(&i_len_tree, br)?;
        }
        i_block_len -= 1;

        const cmd = iac_trees[i_type].decode(br)? as usize;
        const range_idx = cmd >> 6;
        const use_zero_dist = range_idx < 2;
        const ins_code = (INSERT_RANGE_LUT[range_idx] as usize) + ((cmd >> 3) & 7);
        const copy_code = (COPY_RANGE_LUT[range_idx] as usize) + (cmd & 7);

        const ins_len = (INSERT_OFFSET[ins_code] + br.readBits(INSERT_NBITS[ins_code])?) as usize;
        const copy_len = (COPY_OFFSET[copy_code] + br.readBits(COPY_NBITS[copy_code])?) as usize;

        // Emit literals
        let k: usize = 0;
        while (k < ins_len) : (k += 1) {
            if (out.items.len >= target_out_len) {
                return BrotliError.InvalidMetaBlock;
            }
            if (has_l_switch && l_block_len == 0) {
                decodeBlockType(nbltypes_l, &l_type_tree, br, &l_type, &l_prev_type)?;
                l_block_len = decodeBlockLength(&l_len_tree, br)?;
            }
            l_block_len -= 1;

            const n = out.items.len;
            const prev1: u8 = if (n >= 1) out.items[n - 1] else 0;
            const prev2: u8 = if (n >= 2) out.items[n - 2] else 0;
            const ctx = literalContext(context_modes[l_type], prev1, prev2);
            const tree_idx = lit_cmap.map[l_type * 64 + ctx] as usize;
            const lit = lit_trees[tree_idx].decode(br)? as u8;
            out.append(alloc, lit) catch return BrotliError.OutOfMemory;
        }

        if (out.items.len >= target_out_len) {
            break;
        }

        // Decode distance
        let dist: usize = 0;
        if (use_zero_dist) {
            dist = dist_rb[(dist_rb_idx.* + 3) & 3];
        } else {
            if (has_d_switch && d_block_len == 0) {
                decodeBlockType(nbltypes_d, &d_type_tree, br, &d_type, &d_prev_type)?;
                d_block_len = decodeBlockLength(&d_len_tree, br)?;
            }
            d_block_len -= 1;

            const d_ctx: usize = if (copy_len > 4) 3 else (copy_len - 2);
            const d_tree_idx = dist_cmap.map[d_type * 4 + d_ctx] as usize;
            const d_sym = dist_trees[d_tree_idx].decode(br)? as usize;

            if (d_sym < 4) {
                dist = dist_rb[(dist_rb_idx.* + 3 - d_sym) & 3];
            } else if (d_sym < 10) {
                const last_d = dist_rb[(dist_rb_idx.* + 3) & 3];
                const delta = ((d_sym - 4) >> 1) + 1;
                if ((d_sym & 1) == 0) {
                    if (last_d <= delta) {
                        return BrotliError.InvalidDistance;
                    }
                    dist = last_d - delta;
                } else {
                    dist = last_d + delta;
                }
                dist_rb[dist_rb_idx.* & 3] = dist;
                dist_rb_idx.* += 1;
            } else if (d_sym < 16) {
                const second_last = dist_rb[(dist_rb_idx.* + 2) & 3];
                const delta = ((d_sym - 10) >> 1) + 1;
                if ((d_sym & 1) == 0) {
                    if (second_last <= delta) {
                        return BrotliError.InvalidDistance;
                    }
                    dist = second_last - delta;
                } else {
                    dist = second_last + delta;
                }
                dist_rb[dist_rb_idx.* & 3] = dist;
                dist_rb_idx.* += 1;
            } else if (d_sym < 16 + (ndirect as usize)) {
                dist = d_sym - 15;
                dist_rb[dist_rb_idx.* & 3] = dist;
                dist_rb_idx.* += 1;
            } else {
                const hcode = d_sym - 16 - (ndirect as usize);
                const ndistbits: u6 = (1 + (hcode >> ((npostfix as u5) + 1))) as u6;
                if (ndistbits > 24) {
                    return BrotliError.InvalidDistance;
                }
                const hbit = hcode & 1;
                const lcode = hcode >> 1;
                const offset_val: usize = (((2 + hbit) as usize) << (ndistbits as u5)) - 4;
                const extra_bits = br.readBits(ndistbits)? as usize;
                const postfix_mask = ((1 as usize) << (npostfix as u5)) - 1;
                const postfix = lcode & postfix_mask;
                dist = (((offset_val + extra_bits) << (npostfix as u5)) + postfix + (ndirect as usize) + 1);
                dist_rb[dist_rb_idx.* & 3] = dist;
                dist_rb_idx.* += 1;
            }
        }

        const max_backward = if (out.items.len < max_window_size) out.items.len else max_window_size;
        if (dist == 0 || dist > max_backward) {
            return BrotliError.InvalidDistance;
        }
        if (out.items.len + copy_len > target_out_len) {
            return BrotliError.InvalidMetaBlock;
        }

        let c: usize = 0;
        while (c < copy_len) : (c += 1) {
            const b = out.items[out.items.len - dist];
            out.append(alloc, b) catch return BrotliError.OutOfMemory;
        }
    }
}

/// Decompresses RFC 7932 Brotli data with explicit `max_window_bytes` and `max_output_bytes` limits.
pub fn decompressWithLimits(
    allocator: std.mem.Allocator,
    compressed: []const u8,
    max_window_bytes: usize,
    max_output_bytes: usize,
): []u8 ! BrotliError {
    if (compressed.len == 0) {
        return BrotliError.UnexpectedEof;
    }

    let br = BitReader.init(compressed);
    const hdr = decodeWindowBits(&br)?;
    if (hdr.window_size > max_window_bytes) {
        return BrotliError.WindowTooLarge;
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    let dist_rb: [4]usize = [16, 15, 11, 4];
    let dist_rb_idx: usize = 0;

    while (true) {
        const is_last = br.readBit()?;
        if (is_last != 0) {
            const is_last_empty = br.readBit()?;
            if (is_last_empty != 0) {
                break;
            }
        }

        const mnibbles = br.readBits(2)?;
        if (mnibbles == 3) {
            // Metadata meta-block (RFC 7932 §9.2)
            const reserved = br.readBit()?;
            if (reserved != 0) {
                return BrotliError.InvalidMetaBlock;
            }
            const mskipbits = br.readBits(2)?;
            let mskipbytes: usize = 0;
            if (mskipbits > 0) {
                const skip_val = br.readBits((mskipbits * 8) as u6)?;
                if (mskipbits > 1 && (skip_val >> (((mskipbits - 1) * 8) as u5)) == 0) {
                    return BrotliError.InvalidMetaBlock;
                }
                mskipbytes = (skip_val as usize) + 1;
            }
            br.alignToByte();
            br.skipBytes(mskipbytes)?;
            if (is_last != 0) {
                break;
            }
            continue;
        }

        const nibble_count = mnibbles + 4;
        const mlen_m1 = br.readBits((nibble_count * 4) as u6)?;
        if (mnibbles > 0 && (mlen_m1 >> (((nibble_count - 1) * 4) as u5)) == 0) {
            return BrotliError.InvalidMetaBlock;
        }
        const mlen = (mlen_m1 as usize) + 1;

        if (mlen > max_output_bytes || out.items.len > max_output_bytes - mlen) {
            return BrotliError.OutputTooLarge;
        }

        let is_uncompressed: u32 = 0;
        if (is_last == 0) {
            is_uncompressed = br.readBit()?;
        }

        if (is_uncompressed != 0) {
            br.alignToByte();
            const raw = br.readBytes(mlen)?;
            out.appendSlice(allocator, raw) catch return BrotliError.OutOfMemory;
        } else {
            decodeCompressedMetaBlock(
                allocator,
                &br,
                &out,
                mlen,
                hdr.window_size,
                &dist_rb,
                &dist_rb_idx,
            )?;
        }

        if (is_last != 0) {
            break;
        }
    }

    return out.toOwnedSlice(allocator) catch return BrotliError.OutOfMemory;
}

/// Decompresses RFC 7932 Brotli data using default safety limits.
pub fn decompress(allocator: std.mem.Allocator, compressed: []const u8): []u8 ! BrotliError {
    return decompressWithLimits(allocator, compressed, DEFAULT_MAX_WINDOW_BYTES, DEFAULT_MAX_OUTPUT_BYTES);
}
