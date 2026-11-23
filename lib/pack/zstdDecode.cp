// Origin: Pure-CP Stdlib (RFC 8878 Pure-CP Zstandard Decompression Engine: FSE, Huffman 1/4-Stream, Sequences & xxHash64) | Created: 2026-11-23
// Reference-hardened against KillingSpark/zstd-rs (MIT)
// 100% pure CP, zero C/FFI dependencies, RFC 8878 compliant Zstandard (zstd) decoder:
//   - Standard frames (`0xFD2FB528`), Skippable frames (`0x184D2A50..0x184D2A5F`), and concatenated multi-frame streams
//   - Full Frame Header Descriptor parsing (Single_Segment, Content_Checksum, Dictionary_ID, Window_Descriptor, 1/2/4/8B FCS)
//   - Complete xxHash64 (seed=0) low-32-bit Content Checksum verification
//   - All 3 Block types: Raw (0), RLE (1), Compressed (2) with strict `Block_Maximum_Size = min(window_size, 128 KiB)` guard
//   - Full Literals Section: Raw, RLE, Compressed (1-stream & 4-stream Huffman with FSE-compressed or direct 4-bit weights) & Treeless
//   - Full Sequences Section: Predefined (RFC 8878 §3.1.1.3.2.2), RLE, FSE_Compressed, Repeat_Mode,
//     Repeat_Offset_1/2/3 (including LL==0 rule), and byte-wise overlapping match copy
//   - Full RFC 8878 §5 Dictionary support (`zstdDecompressWithDictInto`): Raw Content & Formatted (`0xEC30A437`) dictionaries
//     with Dict_ID check, pre-populated Huffman & OF/ML/LL FSE tables, initial Repeat_Offset_1/2/3 & dictionary history window
//   - Zero-heap external-workspace API (`zstdDecompressInto`, `zstdDecompressWithDictInto`) + convenience allocator wrapper (`zstdDecompress`)

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set & RFC 8878 Predefined Tables
// ============================================================================

/// Standard Zstandard frame magic (`0xFD2FB528`).
pub const ZSTD_DEC_MAGIC: u32 = 0xFD2FB528;

/// Standard Zstandard formatted dictionary magic (`0xEC30A437`).
pub const ZSTD_DICT_MAGIC: u32 = 0xEC30A437;

/// Lowest valid Skippable Frame magic (`0x184D2A50`).
pub const ZSTD_DEC_SKIP_MAGIC_LO: u32 = 0x184D2A50;

/// Highest valid Skippable Frame magic (`0x184D2A5F`).
pub const ZSTD_DEC_SKIP_MAGIC_HI: u32 = 0x184D2A5F;

/// Maximum single Zstandard block uncompressed size (128 KiB per RFC 8878 §3.1.1.2).
pub const ZSTD_MAX_BLOCK_BYTES: usize = 131072;

/// Default defensive window limit (128 MiB) preventing memory exhaustion on crafted headers.
pub const ZSTD_MAX_WINDOW_BYTES: u64 = 134217728;

const FSE_MAX_LOG: usize = 9;
const FSE_MAX_TABLE_SIZE: usize = 512; // 1 << 9
const HUFF_MAX_BITS: usize = 11;
const HUFF_MAX_TABLE_SIZE: usize = 2048; // 1 << 11

/// Error set returned by RFC 8878 Zstandard frame inspection and decompression.
pub err ZstdDecodeError {
    InvalidMagic,
    TruncatedInput,
    CorruptHeader,
    UnsupportedDictionary,
    WindowTooLarge,
    CorruptBlock,
    CorruptLiterals,
    CorruptHuffmanTable,
    CorruptFseTable,
    CorruptSequence,
    OffsetOutOfBounds,
    ChecksumMismatch,
    OutputTooSmall,
    ContentSizeMismatch,
    OutOfMemory,
}

// RFC 8878 §3.1.1.3.2.1.1 Literal Length Baseline & Extra Bits (36 codes)
const LL_BASE: [36]u32 = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 18, 20, 22, 24, 28, 32, 40, 48, 64, 128, 256, 512, 1024, 2048, 4096,
    8192, 16384, 32768, 65536,
];
const LL_BITS: [36]u8 = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16,
];

// RFC 8878 §3.1.1.3.2.1.2 Match Length Baseline & Extra Bits (53 codes)
const ML_BASE: [53]u32 = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 131, 259, 515, 1027, 2051,
    4099, 8195, 16387, 32771, 65539,
];
const ML_BITS: [53]u8 = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16,
];

// RFC 8878 §3.1.1.3.2.2 Predefined FSE Default Distributions
const PREDEF_LL_DIST: [36]i16 = [
    4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1,
    -1, -1, -1, -1,
];
const PREDEF_ML_DIST: [53]i16 = [
    1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1,
    -1, -1, -1, -1, -1,
];
const PREDEF_OF_DIST: [29]i16 = [
    1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
];

// ============================================================================
// 2. Little-Endian Readers & Full xxHash64 (seed = 0)
// ============================================================================

fn readU16Le(src: []const u8, off: usize): u16 {
    return @as(u16, src[off]) | (@as(u16, src[off + 1]) << 8);
}

fn readU24Le(src: []const u8, off: usize): u32 {
    return @as(u32, src[off]) |
        (@as(u32, src[off + 1]) << 8) |
        (@as(u32, src[off + 2]) << 16);
}

fn readU32Le(src: []const u8, off: usize): u32 {
    return @as(u32, src[off]) |
        (@as(u32, src[off + 1]) << 8) |
        (@as(u32, src[off + 2]) << 16) |
        (@as(u32, src[off + 3]) << 24);
}

fn readU64Le(src: []const u8, off: usize): u64 {
    const lo: u64 = @as(u64, readU32Le(src, off));
    const hi: u64 = @as(u64, readU32Le(src, off + 4));
    return lo | (hi << 32);
}

fn rotl64(x: u64, r: u6): u64 {
    const inv: u6 = @as(u6, @intCast(64 - @as(usize, r)));
    return (x << r) | (x >> inv);
}

const XXH_P1: u64 = (@as(u64, 0x9E3779B1) << 32) | 0x85EBCA87;
const XXH_P2: u64 = (@as(u64, 0xC2B2AE3D) << 32) | 0x27D4EB4F;
const XXH_P3: u64 = 0x165667B19E3779F9;
const XXH_P4: u64 = (@as(u64, 0x85EBCA77) << 32) | 0xC2B2AE63;
const XXH_P5: u64 = 0x27D4EB2F165667C5;

fn xxhRound(acc: u64, lane: u64): u64 {
    let a: u64 = acc +% (lane *% XXH_P2);
    a = rotl64(a, 31);
    return a *% XXH_P1;
}

fn xxhMergeRound(acc: u64, val: u64): u64 {
    const v: u64 = xxhRound(0, val);
    let a: u64 = acc ^ v;
    return a *% XXH_P1 +% XXH_P4;
}

/// Computes full 64-bit xxHash64 with `seed = 0` (RFC 8878 Content Checksum uses low 32 bits).
pub fn xxhash64Zstd(data: []const u8): u64 {
    const len: usize = data.len;
    let h64: u64 = 0;
    let p: usize = 0;

    if (len >= 32) {
        let v1: u64 = XXH_P1 +% XXH_P2;
        let v2: u64 = XXH_P2;
        let v3: u64 = 0;
        let v4: u64 = @as(u64, 0) -% XXH_P1;

        while (p + 32 <= len) : (p += 32) {
            v1 = xxhRound(v1, readU64Le(data, p + 0));
            v2 = xxhRound(v2, readU64Le(data, p + 8));
            v3 = xxhRound(v3, readU64Le(data, p + 16));
            v4 = xxhRound(v4, readU64Le(data, p + 24));
        }

        h64 = rotl64(v1, 1) +% rotl64(v2, 7) +% rotl64(v3, 12) +% rotl64(v4, 18);
        h64 = xxhMergeRound(h64, v1);
        h64 = xxhMergeRound(h64, v2);
        h64 = xxhMergeRound(h64, v3);
        h64 = xxhMergeRound(h64, v4);
    } else {
        h64 = XXH_P5;
    }

    h64 +%= @as(u64, @intCast(len));

    while (p + 8 <= len) : (p += 8) {
        const k1: u64 = xxhRound(0, readU64Le(data, p));
        h64 ^= k1;
        h64 = rotl64(h64, 27) *% XXH_P1 +% XXH_P4;
    }
    if (p + 4 <= len) {
        h64 ^= @as(u64, readU32Le(data, p)) *% XXH_P1;
        h64 = rotl64(h64, 23) *% XXH_P2 +% XXH_P3;
        p += 4;
    }
    while (p < len) : (p += 1) {
        h64 ^= @as(u64, data[p]) *% XXH_P5;
        h64 = rotl64(h64, 11) *% XXH_P1;
    }

    h64 ^= h64 >> 33;
    h64 *%= XXH_P2;
    h64 ^= h64 >> 29;
    h64 *%= XXH_P3;
    h64 ^= h64 >> 32;
    return h64;
}

fn highestBitU32(val: u32): usize {
    if (val == 0) return 0;
    let b: usize = 0;
    let v: u32 = val;
    while (v > 1) : (b += 1) {
        v >>= 1;
    }
    return b;
}

// ============================================================================
// 3. Frame Header Inspection (`ZstdFrameInfo`, `zstdInspectFrame`)
// ============================================================================

/// Decoded metadata for a single Zstandard standard or skippable frame.
pub struct ZstdFrameInfo {
    is_skippable: bool = false;
    skippable_payload_bytes: usize = 0;
    single_segment: bool = false;
    has_content_checksum: bool = false;
    has_content_size: bool = false;
    content_size: u64 = 0;
    window_size: u64 = 0;
    dictionary_id: u32 = 0;
    header_bytes: usize = 0;
}

/// Parses a Zstandard frame header (or skippable frame header) at `src[0..]`.
pub fn zstdInspectFrame(src: []const u8): ZstdFrameInfo ! ZstdDecodeError {
    if (src.len < 4) return ZstdDecodeError.TruncatedInput;
    const magic: u32 = readU32Le(src, 0);

    if (magic >= ZSTD_DEC_SKIP_MAGIC_LO && magic <= ZSTD_DEC_SKIP_MAGIC_HI) {
        if (src.len < 8) return ZstdDecodeError.TruncatedInput;
        const skip_sz: usize = @as(usize, readU32Le(src, 4));
        if (src.len - 8 < skip_sz) return ZstdDecodeError.TruncatedInput;
        return ZstdFrameInfo{
            is_skippable: true,
            skippable_payload_bytes: skip_sz,
            header_bytes: 8,
        };
    }

    if (magic != ZSTD_DEC_MAGIC) return ZstdDecodeError.InvalidMagic;
    if (src.len < 5) return ZstdDecodeError.TruncatedInput;

    const fhd: u8 = src[4];
    const fcs_flag: u8 = (fhd >> 6) & 3;
    const single_segment: bool = ((fhd >> 5) & 1) != 0;
    const unused_bit: u8 = (fhd >> 4) & 1;
    const reserved_bit: u8 = (fhd >> 3) & 1;
    const content_checksum: bool = ((fhd >> 2) & 1) != 0;
    const dict_id_flag: u8 = fhd & 3;

    if (unused_bit != 0 || reserved_bit != 0) return ZstdDecodeError.CorruptHeader;

    let pos: usize = 5;
    let window_size: u64 = 0;
    if (!single_segment) {
        if (pos >= src.len) return ZstdDecodeError.TruncatedInput;
        const wd: u8 = src[pos];
        pos += 1;
        const exp: u6 = @as(u6, @intCast((wd >> 3) & 31));
        const mant: u64 = @as(u64, wd & 7);
        if (exp > 30) return ZstdDecodeError.WindowTooLarge;
        const win_base: u64 = @as(u64, 1) << (@as(u6, 10) + exp);
        window_size = win_base + (win_base >> 3) * mant;
        if (window_size > ZSTD_MAX_WINDOW_BYTES) return ZstdDecodeError.WindowTooLarge;
    }

    let dict_id: u32 = 0;
    if (dict_id_flag == 1) {
        if (pos + 1 > src.len) return ZstdDecodeError.TruncatedInput;
        dict_id = @as(u32, src[pos]);
        pos += 1;
    } else if (dict_id_flag == 2) {
        if (pos + 2 > src.len) return ZstdDecodeError.TruncatedInput;
        dict_id = @as(u32, readU16Le(src, pos));
        pos += 2;
    } else if (dict_id_flag == 3) {
        if (pos + 4 > src.len) return ZstdDecodeError.TruncatedInput;
        dict_id = readU32Le(src, pos);
        pos += 4;
    }

    let has_fcs: bool = false;
    let content_size: u64 = 0;
    if (fcs_flag == 0) {
        if (single_segment) {
            if (pos + 1 > src.len) return ZstdDecodeError.TruncatedInput;
            content_size = @as(u64, src[pos]);
            has_fcs = true;
            pos += 1;
        }
    } else if (fcs_flag == 1) {
        if (pos + 2 > src.len) return ZstdDecodeError.TruncatedInput;
        content_size = @as(u64, readU16Le(src, pos)) + 256;
        has_fcs = true;
        pos += 2;
    } else if (fcs_flag == 2) {
        if (pos + 4 > src.len) return ZstdDecodeError.TruncatedInput;
        content_size = @as(u64, readU32Le(src, pos));
        has_fcs = true;
        pos += 4;
    } else {
        if (pos + 8 > src.len) return ZstdDecodeError.TruncatedInput;
        content_size = readU64Le(src, pos);
        has_fcs = true;
        pos += 8;
    }

    if (single_segment) {
        window_size = content_size;
        if (window_size > ZSTD_MAX_WINDOW_BYTES) return ZstdDecodeError.WindowTooLarge;
    }

    return ZstdFrameInfo{
        is_skippable: false,
        single_segment: single_segment,
        has_content_checksum: content_checksum,
        has_content_size: has_fcs,
        content_size: content_size,
        window_size: window_size,
        dictionary_id: dict_id,
        header_bytes: pos,
    };
}

// ============================================================================
// 4. Forward & Reverse Bit Readers (RFC 8878 §4.1.1 / §4.2.1)
// ============================================================================

struct FwdBitReader {
    data: []const u8 = [];
    bit_pos: usize = 0;

    fn readBits(*_, n: usize): u32 ! ZstdDecodeError {
        if (n == 0) return 0;
        if (n > 25) return ZstdDecodeError.CorruptFseTable;
        const total_bits: usize = _.data.len * 8;
        if (_.bit_pos + n > total_bits) return ZstdDecodeError.TruncatedInput;

        let val: u32 = 0;
        let shift: usize = 0;
        while (shift < n) : (shift += 1) {
            const byte_idx: usize = _.bit_pos >> 3;
            const bit_idx: u3 = @as(u3, @intCast(_.bit_pos & 7));
            const bit: u32 = @as(u32, (_.data[byte_idx] >> bit_idx) & 1);
            val |= bit << @as(u5, @intCast(shift));
            _.bit_pos += 1;
        }
        return val;
    }

    fn peekBits(n: usize): u32 {
        if (n == 0) return 0;
        const total_bits: usize = data.len * 8;
        let val: u32 = 0;
        let shift: usize = 0;
        while (shift < n) : (shift += 1) {
            const bp: usize = bit_pos + shift;
            if (bp >= total_bits) break;
            const byte_idx: usize = bp >> 3;
            const bit_idx: u3 = @as(u3, @intCast(bp & 7));
            const bit: u32 = @as(u32, (data[byte_idx] >> bit_idx) & 1);
            val |= bit << @as(u5, @intCast(shift));
        }
        return val;
    }

    fn consume(*_, n: usize) {
        _.bit_pos += n;
    }

    fn bytesConsumed(): usize {
        return (bit_pos + 7) >> 3;
    }
}

/// Reverse bit reader scanning from MSB of the last byte (below sentinel `1` bit) down to bit 0 of byte 0.
struct RevBitReader {
    data: []const u8 = [];
    bits_left: usize = 0;

    static fn init(data: []const u8): RevBitReader ! ZstdDecodeError {
        if (data.len == 0) return ZstdDecodeError.TruncatedInput;
        const last: u8 = data[data.len - 1];
        if (last == 0) return ZstdDecodeError.CorruptSequence; // missing sentinel 1-bit
        // Locate highest 1-bit in last byte
        let hi: usize = 7;
        while (hi > 0) : (hi -= 1) {
            if (((last >> @as(u3, @intCast(hi))) & 1) != 0) break;
        }
        const total_bits: usize = (data.len - 1) * 8 + hi;
        return RevBitReader{ data: data, bits_left: total_bits };
    }

    /// Reads `n` bits (`0 <= n <= 32`) from top of remaining reverse stream.
    fn readBits(*_, n: usize): u32 ! ZstdDecodeError {
        if (n == 0) return 0;
        if (n > 32) return ZstdDecodeError.CorruptSequence;
        if (n > _.bits_left) return ZstdDecodeError.TruncatedInput;
        let val: u32 = 0;
        let k: usize = 0;
        while (k < n) : (k += 1) {
            _.bits_left -= 1;
            const byte_idx: usize = _.bits_left >> 3;
            const bit_idx: u3 = @as(u3, @intCast(_.bits_left & 7));
            const bit: u32 = @as(u32, (_.data[byte_idx] >> bit_idx) & 1);
            val = (val << 1) | bit;
        }
        return val;
    }

    /// Reads up to `n` bits, zero-padding on the right (LSB) when fewer than `n` bits remain (Huffman fast peek).
    fn peekBitsZeroPad(n: usize): u32 {
        if (n == 0) return 0;
        let val: u32 = 0;
        let rem: usize = bits_left;
        let k: usize = 0;
        while (k < n) : (k += 1) {
            if (rem > 0) {
                rem -= 1;
                const byte_idx: usize = rem >> 3;
                const bit_idx: u3 = @as(u3, @intCast(rem & 7));
                const bit: u32 = @as(u32, (data[byte_idx] >> bit_idx) & 1);
                val = (val << 1) | bit;
            } else {
                val = val << 1;
            }
        }
        return val;
    }

    fn consume(*_, n: usize): void ! ZstdDecodeError {
        if (n > _.bits_left) return ZstdDecodeError.TruncatedInput;
        _.bits_left -= n;
    }
}

// ============================================================================
// 5. FSE Table & Huffman Table Structures + Decoders
// ============================================================================

struct FseEntry {
    symbol: u8 = 0;
    num_bits: u8 = 0;
    baseline: u16 = 0;
}

struct FseTable {
    accuracy_log: usize = 0;
    valid: bool = false;
    entries: [FSE_MAX_TABLE_SIZE]FseEntry = [FseEntry{}; FSE_MAX_TABLE_SIZE];

    fn buildFromNorm(*_, norm: []const i16, max_sym: usize, acc_log: usize): void ! ZstdDecodeError {
        if (acc_log > FSE_MAX_LOG) return ZstdDecodeError.CorruptFseTable;
        const table_size: usize = @as(usize, 1) << @as(u6, @intCast(acc_log));
        const table_mask: usize = table_size - 1;
        const step: usize = (table_size >> 1) + (table_size >> 3) + 3;

        let sym_next: [256]u16 = [0; 256];
        let high_thresh: usize = table_size - 1;
        let sym_count: usize = 0;

        // First pass: place "-1" (less-than-1 probability) symbols at the top of the table
        let s: usize = 0;
        while (s <= max_sym) : (s += 1) {
            if (norm[s] == -1) {
                _.entries[high_thresh].symbol = @as(u8, @intCast(s));
                if (high_thresh == 0) {
                    sym_next[s] = 1;
                    sym_count += 1;
                    break;
                }
                high_thresh -= 1;
                sym_next[s] = 1;
                sym_count += 1;
            } else if (norm[s] > 0) {
                sym_next[s] = @as(u16, @intCast(norm[s]));
                sym_count += @as(usize, @intCast(norm[s]));
            }
        }
        if (sym_count != table_size) return ZstdDecodeError.CorruptFseTable;

        // Second pass: spread positive-probability symbols
        let pos: usize = 0;
        s = 0;
        while (s <= max_sym) : (s += 1) {
            if (norm[s] <= 0) continue;
            const cnt: usize = @as(usize, @intCast(norm[s]));
            let k: usize = 0;
            while (k < cnt) : (k += 1) {
                _.entries[pos].symbol = @as(u8, @intCast(s));
                pos = (pos + step) & table_mask;
                while (pos > high_thresh) {
                    pos = (pos + step) & table_mask;
                }
            }
        }
        if (pos != 0) return ZstdDecodeError.CorruptFseTable;

        // Third pass: build baseline & num_bits for every table slot
        let i: usize = 0;
        while (i < table_size) : (i += 1) {
            const sym: usize = @as(usize, _.entries[i].symbol);
            const next_st: u32 = @as(u32, sym_next[sym]);
            sym_next[sym] += 1;
            const hb: usize = highestBitU32(next_st);
            const nb: usize = acc_log - hb;
            _.entries[i].num_bits = @as(u8, @intCast(nb));
            _.entries[i].baseline = @as(u16, @intCast((next_st << @as(u5, @intCast(nb))) - @as(u32, @intCast(table_size))));
        }

        _.accuracy_log = acc_log;
        _.valid = true;
    }

    fn buildRle(*_, sym: u8) {
        _.accuracy_log = 0;
        _.entries[0] = FseEntry{ symbol: sym, num_bits: 0, baseline: 0 };
        _.valid = true;
    }
}

/// Decodes an FSE normalized distribution table from `src[0..]` and builds `tbl`, returning bytes consumed.
fn decodeFseTable(src: []const u8, max_sym: usize, max_log: usize, tbl: *FseTable): usize ! ZstdDecodeError {
    if (src.len == 0) return ZstdDecodeError.TruncatedInput;
    let br: FwdBitReader = FwdBitReader{ data: src, bit_pos: 0 };
    const acc_log: usize = @as(usize, try br.readBits(4)) + 5;
    if (acc_log > max_log || acc_log > FSE_MAX_LOG) return ZstdDecodeError.CorruptFseTable;

    let norm: [256]i16 = [0; 256];
    let remaining: i32 = (@as(i32, 1) << @as(u5, @intCast(acc_log))) + 1;
    let threshold: i32 = @as(i32, 1) << @as(u5, @intCast(acc_log));
    let nb_bits: usize = acc_log + 1;
    let sym: usize = 0;
    let prev_zero: bool = false;

    while (remaining > 1 && sym <= max_sym) {
        if (prev_zero) {
            while (true) {
                const rep: u32 = try br.readBits(2);
                sym += @as(usize, rep);
                if (rep < 3) break;
                if (sym > max_sym + 1) return ZstdDecodeError.CorruptFseTable;
            }
            if (sym > max_sym) break;
            prev_zero = false;
        }

        const max_val: i32 = 2 * threshold - 1 - remaining;
        const low_bits: usize = nb_bits - 1;
        let val: i32 = @as(i32, @intCast(br.peekBits(low_bits)));

        if (val < max_val) {
            br.consume(low_bits);
        } else {
            val = @as(i32, @intCast(br.peekBits(nb_bits)));
            br.consume(nb_bits);
            if (val >= threshold) {
                val -= max_val;
            }
        }

        const count: i32 = val - 1;
        if (count == -1) {
            remaining -= 1;
        } else {
            remaining -= count;
        }
        if (remaining < 1) return ZstdDecodeError.CorruptFseTable;
        norm[sym] = @as(i16, @intCast(count));
        sym += 1;
        prev_zero = (count == 0);

        while (remaining < threshold && nb_bits > 1) {
            nb_bits -= 1;
            threshold >>= 1;
        }
    }

    if (remaining != 1) return ZstdDecodeError.CorruptFseTable;
    const consumed: usize = br.bytesConsumed();
    if (consumed > src.len) return ZstdDecodeError.TruncatedInput;

    const actual_max: usize = if (sym > 0) (sym - 1) else 0;
    try tbl.buildFromNorm(norm[0..], actual_max, acc_log);
    return consumed;
}

struct HuffTable {
    max_bits: usize = 0;
    valid: bool = false;
    syms: [HUFF_MAX_TABLE_SIZE]u8 = [0; HUFF_MAX_TABLE_SIZE];
    nbits: [HUFF_MAX_TABLE_SIZE]u8 = [0; HUFF_MAX_TABLE_SIZE];

    fn buildFromWeights(*_, weights: []const u8, num_weights: usize): void ! ZstdDecodeError {
        if (num_weights == 0 || num_weights > 255) return ZstdDecodeError.CorruptHuffmanTable;

        let weight_sum: u32 = 0;
        let i: usize = 0;
        while (i < num_weights) : (i += 1) {
            const w: u8 = weights[i];
            if (w > HUFF_MAX_BITS) return ZstdDecodeError.CorruptHuffmanTable;
            if (w > 0) {
                weight_sum += @as(u32, 1) << @as(u5, @intCast(w - 1));
            }
        }
        if (weight_sum == 0) return ZstdDecodeError.CorruptHuffmanTable;

        const max_b: usize = highestBitU32(weight_sum) + 1;
        if (max_b > HUFF_MAX_BITS) return ZstdDecodeError.CorruptHuffmanTable;
        const table_size: u32 = @as(u32, 1) << @as(u5, @intCast(max_b));
        const left_over: u32 = table_size - weight_sum;
        // left_over must be an exact power of 2
        if (left_over == 0 || (left_over & (left_over - 1)) != 0) {
            return ZstdDecodeError.CorruptHuffmanTable;
        }
        const last_weight: u8 = @as(u8, @intCast(highestBitU32(left_over) + 1));

        let all_w: [256]u8 = [0; 256];
        i = 0;
        while (i < num_weights) : (i += 1) all_w[i] = weights[i];
        all_w[num_weights] = last_weight;
        const total_syms: usize = num_weights + 1;

        // Count symbols per weight (1..=max_b)
        let rank_count: [HUFF_MAX_BITS + 2]u32 = [0; HUFF_MAX_BITS + 2];
        i = 0;
        while (i < total_syms) : (i += 1) {
            const w: usize = @as(usize, all_w[i]);
            if (w > 0) rank_count[w] += 1;
        }

        // Compute starting table index for each weight rank (RFC 8878 §4.2.2)
        let rank_idx: [HUFF_MAX_BITS + 2]usize = [0; HUFF_MAX_BITS + 2];
        let next_idx: usize = 0;
        let w: usize = 1;
        while (w <= max_b) : (w += 1) {
            rank_idx[w] = next_idx;
            const span: usize = @as(usize, 1) << @as(u6, @intCast(w - 1));
            next_idx += @as(usize, rank_count[w]) * span;
        }
        if (next_idx != @as(usize, table_size)) return ZstdDecodeError.CorruptHuffmanTable;

        // Populate flat lookup table
        i = 0;
        while (i < total_syms) : (i += 1) {
            const wi: usize = @as(usize, all_w[i]);
            if (wi == 0) continue;
            const n_bits: u8 = @as(u8, @intCast(max_b + 1 - wi));
            const span: usize = @as(usize, 1) << @as(u6, @intCast(wi - 1));
            const base: usize = rank_idx[wi];
            rank_idx[wi] += span;
            let k: usize = 0;
            while (k < span) : (k += 1) {
                _.syms[base + k] = @as(u8, @intCast(i));
                _.nbits[base + k] = n_bits;
            }
        }

        _.max_bits = max_b;
        _.valid = true;
    }
}

/// Decodes a Huffman table description at `src[0..]` into `htbl`, returning bytes consumed.
fn decodeHuffmanTable(src: []const u8, htbl: *HuffTable, fse_tmp: *FseTable): usize ! ZstdDecodeError {
    if (src.len == 0) return ZstdDecodeError.TruncatedInput;
    const hbyte: usize = @as(usize, src[0]);
    let weights: [256]u8 = [0; 256];

    if (hbyte >= 128) {
        // Direct 4-bit weights
        const num_w: usize = hbyte - 127;
        const need_bytes: usize = (num_w + 1) / 2;
        if (1 + need_bytes > src.len) return ZstdDecodeError.TruncatedInput;
        let i: usize = 0;
        while (i < num_w) : (i += 1) {
            const b: u8 = src[1 + (i >> 1)];
            weights[i] = if ((i & 1) == 0) (b >> 4) else (b & 0x0F);
        }
        try htbl.buildFromWeights(weights[0..], num_w);
        return 1 + need_bytes;
    }

    // FSE-compressed Huffman weights (interleaved 2-state reverse stream)
    const comp_size: usize = hbyte;
    if (1 + comp_size > src.len) return ZstdDecodeError.TruncatedInput;
    const fse_payload: []const u8 = src[1..1 + comp_size];
    const hdr_bytes: usize = try decodeFseTable(fse_payload, 12, 6, fse_tmp);
    if (hdr_bytes >= comp_size) return ZstdDecodeError.CorruptHuffmanTable;

    let rbr: RevBitReader = try RevBitReader.init(fse_payload[hdr_bytes..comp_size]);
    let st1: usize = @as(usize, try rbr.readBits(fse_tmp.accuracy_log));
    let st2: usize = @as(usize, try rbr.readBits(fse_tmp.accuracy_log));

    let num_w: usize = 0;
    while (true) {
        if (num_w >= 255) return ZstdDecodeError.CorruptHuffmanTable;
        const e1: FseEntry = fse_tmp.entries[st1];
        weights[num_w] = e1.symbol;
        num_w += 1;
        if (rbr.bits_left < @as(usize, e1.num_bits)) {
            if (num_w >= 255) return ZstdDecodeError.CorruptHuffmanTable;
            weights[num_w] = fse_tmp.entries[st2].symbol;
            num_w += 1;
            break;
        }
        st1 = @as(usize, e1.baseline) + @as(usize, try rbr.readBits(@as(usize, e1.num_bits)));

        if (num_w >= 255) return ZstdDecodeError.CorruptHuffmanTable;
        const e2: FseEntry = fse_tmp.entries[st2];
        weights[num_w] = e2.symbol;
        num_w += 1;
        if (rbr.bits_left < @as(usize, e2.num_bits)) {
            if (num_w >= 255) return ZstdDecodeError.CorruptHuffmanTable;
            weights[num_w] = fse_tmp.entries[st1].symbol;
            num_w += 1;
            break;
        }
        st2 = @as(usize, e2.baseline) + @as(usize, try rbr.readBits(@as(usize, e2.num_bits)));
    }

    try htbl.buildFromWeights(weights[0..], num_w);
    return 1 + comp_size;
}

fn decodeHuffmanStream(htbl: HuffTable, stream_bytes: []const u8, out: *[]u8): void ! ZstdDecodeError {
    if (!htbl.valid) return ZstdDecodeError.CorruptHuffmanTable;
    let rbr: RevBitReader = try RevBitReader.init(stream_bytes);
    const mb: usize = htbl.max_bits;
    let pos: usize = 0;
    while (pos < out.len) : (pos += 1) {
        if (rbr.bits_left == 0) return ZstdDecodeError.CorruptLiterals;
        const code: usize = @as(usize, rbr.peekBitsZeroPad(mb));
        const sym: u8 = htbl.syms[code];
        const nb: usize = @as(usize, htbl.nbits[code]);
        if (nb == 0 || nb > rbr.bits_left) return ZstdDecodeError.CorruptLiterals;
        try rbr.consume(nb);
        out[pos] = sym;
    }
    if (rbr.bits_left != 0) return ZstdDecodeError.CorruptLiterals;
}

// ============================================================================
// 6. ZstdWorkspace & Block-Level Literals + Sequences Decoding
// ============================================================================

/// Reusable zero-heap decompression workspace (holds FSE/Huffman tables, repeat offsets,
/// and a caller-supplied 128 KiB literal buffer slice).
pub struct ZstdWorkspace {
    lit_buf: []u8 = []; // caller provides >= ZSTD_MAX_BLOCK_BYTES (131072) for full blocks
    huff: HuffTable = HuffTable{},
    ll_tbl: FseTable = FseTable{},
    of_tbl: FseTable = FseTable{},
    ml_tbl: FseTable = FseTable{},
    fse_tmp: FseTable = FseTable{},
    rep_offsets: [3]u32 = [1, 4, 8];

    /// Initializes a workspace bound to caller's `lit_buf` (recommended `>= 131072` bytes).
    static fn init(lit_buf: []u8): ZstdWorkspace {
        return ZstdWorkspace{
            lit_buf: lit_buf,
        };
    }

    fn resetForFrame(*_) {
        _.huff.valid = false;
        _.ll_tbl.valid = false;
        _.of_tbl.valid = false;
        _.ml_tbl.valid = false;
        _.rep_offsets[0] = 1;
        _.rep_offsets[1] = 4;
        _.rep_offsets[2] = 8;
    }
}

struct LiteralsResult {
    lit_slice: []const u8 = [];
    bytes_consumed: usize = 0;
}

fn decodeLiteralsSection(src: []const u8, ws: *ZstdWorkspace, max_block_size: usize): LiteralsResult ! ZstdDecodeError {
    if (src.len == 0) return ZstdDecodeError.TruncatedInput;
    const b0: u8 = src[0];
    const lit_type: u8 = b0 & 3;
    const size_fmt: u8 = (b0 >> 2) & 3;

    if (lit_type == 0 || lit_type == 1) {
        // Raw (0) or RLE (1) Literals
        let regen_size: usize = 0;
        let hdr_len: usize = 0;
        if (size_fmt == 0 || size_fmt == 2) {
            regen_size = @as(usize, b0 >> 3);
            hdr_len = 1;
        } else if (size_fmt == 1) {
            if (src.len < 2) return ZstdDecodeError.TruncatedInput;
            regen_size = @as(usize, b0 >> 4) | (@as(usize, src[1]) << 4);
            hdr_len = 2;
        } else {
            if (src.len < 3) return ZstdDecodeError.TruncatedInput;
            regen_size = @as(usize, b0 >> 4) | (@as(usize, src[1]) << 4) | (@as(usize, src[2]) << 12);
            hdr_len = 3;
        }
        if (regen_size > max_block_size || regen_size > ZSTD_MAX_BLOCK_BYTES) return ZstdDecodeError.CorruptLiterals;

        if (lit_type == 0) {
            if (hdr_len + regen_size > src.len) return ZstdDecodeError.TruncatedInput;
            return LiteralsResult{
                lit_slice: src[hdr_len..hdr_len + regen_size],
                bytes_consumed: hdr_len + regen_size,
            };
        } else {
            if (hdr_len + 1 > src.len) return ZstdDecodeError.TruncatedInput;
            if (regen_size > ws.lit_buf.len) return ZstdDecodeError.OutputTooSmall;
            const fill_b: u8 = src[hdr_len];
            let k: usize = 0;
            while (k < regen_size) : (k += 1) ws.lit_buf[k] = fill_b;
            return LiteralsResult{
                lit_slice: ws.lit_buf[0..regen_size],
                bytes_consumed: hdr_len + 1,
            };
        }
    }

    // Compressed (2) or Treeless (3) Literals
    let regen_size: usize = 0;
    let comp_size: usize = 0;
    let hdr_len: usize = 0;
    let four_streams: bool = true;

    if (size_fmt == 0 || size_fmt == 1) {
        if (src.len < 3) return ZstdDecodeError.TruncatedInput;
        const v: u32 = readU24Le(src, 0);
        regen_size = @as(usize, (v >> 4) & 0x3FF);
        comp_size = @as(usize, (v >> 14) & 0x3FF);
        hdr_len = 3;
        four_streams = (size_fmt == 1);
    } else if (size_fmt == 2) {
        if (src.len < 4) return ZstdDecodeError.TruncatedInput;
        const v: u32 = readU32Le(src, 0);
        regen_size = @as(usize, (v >> 4) & 0x3FFF);
        comp_size = @as(usize, (v >> 18) & 0x3FFF);
        hdr_len = 4;
    } else {
        if (src.len < 5) return ZstdDecodeError.TruncatedInput;
        const v: u64 = @as(u64, readU32Le(src, 0)) | (@as(u64, src[4]) << 32);
        regen_size = @as(usize, @intCast((v >> 4) & 0x3FFFF));
        comp_size = @as(usize, @intCast((v >> 22) & 0x3FFFF));
        hdr_len = 5;
    }

    if (regen_size == 0 || regen_size > max_block_size || regen_size > ZSTD_MAX_BLOCK_BYTES) {
        return ZstdDecodeError.CorruptLiterals;
    }
    if (regen_size > ws.lit_buf.len) return ZstdDecodeError.OutputTooSmall;
    if (hdr_len + comp_size > src.len) return ZstdDecodeError.TruncatedInput;

    let stream_off: usize = hdr_len;
    let stream_bytes_len: usize = comp_size;

    if (lit_type == 2) {
        const tree_used: usize = try decodeHuffmanTable(src[hdr_len..hdr_len + comp_size], *ws.huff, *ws.fse_tmp);
        if (tree_used > comp_size) return ZstdDecodeError.CorruptHuffmanTable;
        stream_off += tree_used;
        stream_bytes_len -= tree_used;
    } else {
        if (!ws.huff.valid) return ZstdDecodeError.CorruptHuffmanTable;
    }

    const streams: []const u8 = src[stream_off..stream_off + stream_bytes_len];
    if (!four_streams) {
        if (streams.len == 0) return ZstdDecodeError.CorruptLiterals;
        let dst_s: []u8 = ws.lit_buf[0..regen_size];
        try decodeHuffmanStream(ws.huff, streams, dst_s);
    } else {
        if (regen_size < 6 || streams.len < 6) return ZstdDecodeError.CorruptLiterals;
        const s1: usize = @as(usize, readU16Le(streams, 0));
        const s2: usize = @as(usize, readU16Le(streams, 2));
        const s3: usize = @as(usize, readU16Le(streams, 4));
        if (6 + s1 + s2 + s3 > streams.len) return ZstdDecodeError.CorruptLiterals;
        const s4: usize = streams.len - 6 - s1 - s2 - s3;
        if (s1 == 0 || s2 == 0 || s3 == 0 || s4 == 0) return ZstdDecodeError.CorruptLiterals;

        const seg_out: usize = (regen_size + 3) / 4;
        if (seg_out == 0 || seg_out * 3 >= regen_size) return ZstdDecodeError.CorruptLiterals;
        const seg4_out: usize = regen_size - seg_out * 3;

        let d1: []u8 = ws.lit_buf[0..seg_out];
        let d2: []u8 = ws.lit_buf[seg_out..seg_out * 2];
        let d3: []u8 = ws.lit_buf[seg_out * 2..seg_out * 3];
        let d4: []u8 = ws.lit_buf[seg_out * 3..seg_out * 3 + seg4_out];

        const o1: usize = 6;
        const o2: usize = o1 + s1;
        const o3: usize = o2 + s2;
        const o4: usize = o3 + s3;

        try decodeHuffmanStream(ws.huff, streams[o1..o2], d1);
        try decodeHuffmanStream(ws.huff, streams[o2..o3], d2);
        try decodeHuffmanStream(ws.huff, streams[o3..o4], d3);
        try decodeHuffmanStream(ws.huff, streams[o4..o4 + s4], d4);
    }

    return LiteralsResult{
        lit_slice: ws.lit_buf[0..regen_size],
        bytes_consumed: hdr_len + comp_size,
    };
}

fn setupSequenceTable(
    mode: u8,
    src: []const u8,
    pos: *usize,
    predef_dist: []const i16,
    predef_max_sym: usize,
    predef_log: usize,
    max_sym: usize,
    max_log: usize,
    tbl: *FseTable,
): void ! ZstdDecodeError {
    if (mode == 0) {
        // Predefined_Mode
        try tbl.buildFromNorm(predef_dist, predef_max_sym, predef_log);
    } else if (mode == 1) {
        // RLE_Mode
        if (pos.* >= src.len) return ZstdDecodeError.TruncatedInput;
        const sym: u8 = src[pos.*];
        if (@as(usize, sym) > max_sym) return ZstdDecodeError.CorruptFseTable;
        pos.* += 1;
        tbl.buildRle(sym);
    } else if (mode == 2) {
        // FSE_Compressed_Mode
        const used: usize = try decodeFseTable(src[pos.*..src.len], max_sym, max_log, tbl);
        pos.* += used;
    } else {
        // Repeat_Mode
        if (!tbl.valid) return ZstdDecodeError.CorruptFseTable;
    }
}

/// Loads an optional Raw Content or Formatted (`0xEC30A437`) Zstandard dictionary into `ws`
/// (RFC 8878 §5) and returns the dictionary history content slice.
fn loadDictionaryForFrame(
    dict: []const u8,
    frame_dict_id: u32,
    ws: *ZstdWorkspace,
): ([]const u8) ! ZstdDecodeError {
    if (dict.len == 0) {
        if (frame_dict_id != 0) return ZstdDecodeError.UnsupportedDictionary;
        return dict;
    }

    if (dict.len >= 4 && readU32Le(dict, 0) == ZSTD_DICT_MAGIC) {
        // Formatted Dictionary (RFC 8878 §5.1)
        if (dict.len < 8) return ZstdDecodeError.TruncatedInput;
        const dict_id: u32 = readU32Le(dict, 4);
        if (dict_id == 0) return ZstdDecodeError.CorruptHeader;
        if (frame_dict_id != 0 && frame_dict_id != dict_id) {
            return ZstdDecodeError.UnsupportedDictionary;
        }

        let pos: usize = 8;
        const huff_used: usize = try decodeHuffmanTable(dict[pos..dict.len], *ws.huff, *ws.fse_tmp);
        pos += huff_used;
        if (pos > dict.len) return ZstdDecodeError.TruncatedInput;

        const of_used: usize = try decodeFseTable(dict[pos..dict.len], 31, 8, *ws.of_tbl);
        pos += of_used;
        if (pos > dict.len) return ZstdDecodeError.TruncatedInput;

        const ml_used: usize = try decodeFseTable(dict[pos..dict.len], 52, 9, *ws.ml_tbl);
        pos += ml_used;
        if (pos > dict.len) return ZstdDecodeError.TruncatedInput;

        const ll_used: usize = try decodeFseTable(dict[pos..dict.len], 35, 9, *ws.ll_tbl);
        pos += ll_used;
        if (pos + 12 > dict.len) return ZstdDecodeError.TruncatedInput;

        const rep0: u32 = readU32Le(dict, pos);
        const rep1: u32 = readU32Le(dict, pos + 4);
        const rep2: u32 = readU32Le(dict, pos + 8);
        pos += 12;

        const content: []const u8 = dict[pos..dict.len];
        const clen_u64: u64 = @as(u64, @intCast(content.len));
        if (rep0 == 0 || rep1 == 0 || rep2 == 0
            || @as(u64, rep0) > clen_u64
            || @as(u64, rep1) > clen_u64
            || @as(u64, rep2) > clen_u64)
        {
            return ZstdDecodeError.CorruptHeader;
        }
        ws.rep_offsets[0] = rep0;
        ws.rep_offsets[1] = rep1;
        ws.rep_offsets[2] = rep2;
        return content;
    }

    // Raw Content Dictionary: no dictionary ID or pre-populated entropy tables
    if (frame_dict_id != 0) return ZstdDecodeError.UnsupportedDictionary;
    return dict;
}

fn decodeCompressedBlock(
    block: []const u8,
    ws: *ZstdWorkspace,
    dict_content: []const u8,
    frame_start: usize,
    window_size: u64,
    max_block_size: usize,
    dst: *[]u8,
    out_pos: *usize,
): void ! ZstdDecodeError {
    const block_out_start: usize = out_pos.*;
    const lit_res: LiteralsResult = try decodeLiteralsSection(block, ws, max_block_size);
    if (lit_res.bytes_consumed > block.len) return ZstdDecodeError.CorruptBlock;
    const seq_sec: []const u8 = block[lit_res.bytes_consumed..block.len];
    if (seq_sec.len == 0) return ZstdDecodeError.TruncatedInput;

    // Parse Number_of_Sequences (RFC 8878 §3.1.1.3.2)
    let pos: usize = 0;
    const b0: usize = @as(usize, seq_sec[0]);
    let num_seq: usize = 0;
    if (b0 == 0) {
        pos = 1;
        num_seq = 0;
    } else if (b0 < 128) {
        pos = 1;
        num_seq = b0;
    } else if (b0 < 255) {
        if (seq_sec.len < 2) return ZstdDecodeError.TruncatedInput;
        pos = 2;
        num_seq = ((b0 - 128) << 8) + @as(usize, seq_sec[1]);
    } else {
        if (seq_sec.len < 3) return ZstdDecodeError.TruncatedInput;
        pos = 3;
        num_seq = @as(usize, seq_sec[1]) + (@as(usize, seq_sec[2]) << 8) + 0x7F00;
    }

    if (num_seq == 0) {
        if (pos != seq_sec.len) return ZstdDecodeError.CorruptSequence;
        if (lit_res.lit_slice.len > max_block_size) return ZstdDecodeError.CorruptBlock;
        if (out_pos.* + lit_res.lit_slice.len > dst.len) return ZstdDecodeError.OutputTooSmall;
        @memcpy(dst[out_pos.*..out_pos.* + lit_res.lit_slice.len], lit_res.lit_slice);
        out_pos.* += lit_res.lit_slice.len;
        return;
    }

    if (pos >= seq_sec.len) return ZstdDecodeError.TruncatedInput;
    const modes_byte: u8 = seq_sec[pos];
    pos += 1;
    if ((modes_byte & 3) != 0) return ZstdDecodeError.CorruptSequence; // reserved bits

    const ll_mode: u8 = (modes_byte >> 6) & 3;
    const of_mode: u8 = (modes_byte >> 4) & 3;
    const ml_mode: u8 = (modes_byte >> 2) & 3;

    try setupSequenceTable(ll_mode, seq_sec, *pos, PREDEF_LL_DIST[0..], 35, 6, 35, 9, *ws.ll_tbl);
    try setupSequenceTable(of_mode, seq_sec, *pos, PREDEF_OF_DIST[0..], 28, 5, 31, 8, *ws.of_tbl);
    try setupSequenceTable(ml_mode, seq_sec, *pos, PREDEF_ML_DIST[0..], 52, 6, 52, 9, *ws.ml_tbl);

    if (pos >= seq_sec.len) return ZstdDecodeError.TruncatedInput;
    let rbr: RevBitReader = try RevBitReader.init(seq_sec[pos..seq_sec.len]);

    let ll_st: usize = @as(usize, try rbr.readBits(ws.ll_tbl.accuracy_log));
    let of_st: usize = @as(usize, try rbr.readBits(ws.of_tbl.accuracy_log));
    let ml_st: usize = @as(usize, try rbr.readBits(ws.ml_tbl.accuracy_log));

    let lit_pos: usize = 0;
    let seq_i: usize = 0;
    while (seq_i < num_seq) : (seq_i += 1) {
        const ll_e: FseEntry = ws.ll_tbl.entries[ll_st];
        const of_e: FseEntry = ws.of_tbl.entries[of_st];
        const ml_e: FseEntry = ws.ml_tbl.entries[ml_st];

        const ll_code: usize = @as(usize, ll_e.symbol);
        const of_code: usize = @as(usize, of_e.symbol);
        const ml_code: usize = @as(usize, ml_e.symbol);
        if (ll_code > 35 || of_code > 31 || ml_code > 52) return ZstdDecodeError.CorruptSequence;

        // Extra bits order per RFC 8878 §3.1.1.3.2.1: OF -> ML -> LL
        const of_extra: u32 = try rbr.readBits(of_code);
        const raw_offset: u32 = (@as(u32, 1) << @as(u5, @intCast(of_code))) + of_extra;

        const ml_extra: u32 = try rbr.readBits(@as(usize, ML_BITS[ml_code]));
        const match_len: usize = @as(usize, ML_BASE[ml_code] + ml_extra);

        const ll_extra: u32 = try rbr.readBits(@as(usize, LL_BITS[ll_code]));
        const lit_len: usize = @as(usize, LL_BASE[ll_code] + ll_extra);

        // State update order (for all sequences except the last): LL -> ML -> OF
        if (seq_i + 1 < num_seq) {
            ll_st = @as(usize, ll_e.baseline) + @as(usize, try rbr.readBits(@as(usize, ll_e.num_bits)));
            ml_st = @as(usize, ml_e.baseline) + @as(usize, try rbr.readBits(@as(usize, ml_e.num_bits)));
            of_st = @as(usize, of_e.baseline) + @as(usize, try rbr.readBits(@as(usize, of_e.num_bits)));
        }

        // Resolve Repeat_Offset_1/2/3 (RFC 8878 §3.1.1.5)
        let offset: usize = 0;
        if (raw_offset > 3) {
            const off_u32: u32 = raw_offset - 3;
            ws.rep_offsets[2] = ws.rep_offsets[1];
            ws.rep_offsets[1] = ws.rep_offsets[0];
            ws.rep_offsets[0] = off_u32;
            offset = @as(usize, off_u32);
        } else if (lit_len == 0) {
            if (raw_offset == 1) {
                const off_u32: u32 = ws.rep_offsets[1];
                ws.rep_offsets[1] = ws.rep_offsets[0];
                ws.rep_offsets[0] = off_u32;
                offset = @as(usize, off_u32);
            } else if (raw_offset == 2) {
                const off_u32: u32 = ws.rep_offsets[2];
                ws.rep_offsets[2] = ws.rep_offsets[1];
                ws.rep_offsets[1] = ws.rep_offsets[0];
                ws.rep_offsets[0] = off_u32;
                offset = @as(usize, off_u32);
            } else {
                if (ws.rep_offsets[0] <= 1) return ZstdDecodeError.OffsetOutOfBounds;
                const off_u32: u32 = ws.rep_offsets[0] - 1;
                ws.rep_offsets[2] = ws.rep_offsets[1];
                ws.rep_offsets[1] = ws.rep_offsets[0];
                ws.rep_offsets[0] = off_u32;
                offset = @as(usize, off_u32);
            }
        } else {
            if (raw_offset == 1) {
                offset = @as(usize, ws.rep_offsets[0]);
            } else if (raw_offset == 2) {
                const off_u32: u32 = ws.rep_offsets[1];
                ws.rep_offsets[1] = ws.rep_offsets[0];
                ws.rep_offsets[0] = off_u32;
                offset = @as(usize, off_u32);
            } else {
                const off_u32: u32 = ws.rep_offsets[2];
                ws.rep_offsets[2] = ws.rep_offsets[1];
                ws.rep_offsets[1] = ws.rep_offsets[0];
                ws.rep_offsets[0] = off_u32;
                offset = @as(usize, off_u32);
            }
        }

        // Copy literals
        if (lit_pos + lit_len > lit_res.lit_slice.len) return ZstdDecodeError.CorruptSequence;
        if ((out_pos.* - block_out_start) + lit_len > max_block_size) return ZstdDecodeError.CorruptBlock;
        if (out_pos.* + lit_len > dst.len) return ZstdDecodeError.OutputTooSmall;
        if (lit_len > 0) {
            @memcpy(dst[out_pos.*..out_pos.* + lit_len], lit_res.lit_slice[lit_pos..lit_pos + lit_len]);
            lit_pos += lit_len;
            out_pos.* += lit_len;
        }

        // Copy match (supports dictionary prefix + overlapping RLE-style matches)
        if ((out_pos.* - block_out_start) + match_len > max_block_size) return ZstdDecodeError.CorruptBlock;
        if (out_pos.* + match_len > dst.len) return ZstdDecodeError.OutputTooSmall;
        const produced_in_frame: usize = out_pos.* - frame_start;
        if (offset == 0) return ZstdDecodeError.OffsetOutOfBounds;

        if (offset <= produced_in_frame) {
            if (window_size > 0 && @as(u64, @intCast(offset)) > window_size) {
                return ZstdDecodeError.OffsetOutOfBounds;
            }
            let m_i: usize = 0;
            while (m_i < match_len) : (m_i += 1) {
                dst[out_pos.*] = dst[out_pos.* - offset];
                out_pos.* += 1;
            }
        } else {
            // Reference back into dictionary content prefix (allowed while produced <= window_size per RFC 8878 §5)
            if (dict_content.len == 0) return ZstdDecodeError.OffsetOutOfBounds;
            if (window_size > 0 && @as(u64, @intCast(produced_in_frame)) > window_size) {
                return ZstdDecodeError.OffsetOutOfBounds;
            }
            const dict_back: usize = offset - produced_in_frame;
            if (dict_back > dict_content.len) return ZstdDecodeError.OffsetOutOfBounds;
            let dict_idx: usize = dict_content.len - dict_back;
            let m_i: usize = 0;
            while (m_i < match_len) : (m_i += 1) {
                if (dict_idx < dict_content.len) {
                    dst[out_pos.*] = dict_content[dict_idx];
                    dict_idx += 1;
                } else {
                    dst[out_pos.*] = dst[out_pos.* - offset];
                }
                out_pos.* += 1;
            }
        }
    }

    if (rbr.bits_left != 0) return ZstdDecodeError.CorruptSequence;

    // Trailing literals after final sequence
    const rem_lit: usize = lit_res.lit_slice.len - lit_pos;
    if (rem_lit > 0) {
        if ((out_pos.* - block_out_start) + rem_lit > max_block_size) return ZstdDecodeError.CorruptBlock;
        if (out_pos.* + rem_lit > dst.len) return ZstdDecodeError.OutputTooSmall;
        @memcpy(dst[out_pos.*..out_pos.* + rem_lit], lit_res.lit_slice[lit_pos..lit_res.lit_slice.len]);
        out_pos.* += rem_lit;
    }
}

// ============================================================================
// 7. Top-Level Decompression APIs (`zstdDecompressWithDictInto`, `zstdDecompressInto`, `zstdDecompress`)
// ============================================================================

/// Zero-heap RFC 8878 Zstandard decompressor with optional Raw Content or Formatted (`0xEC30A437`) dictionary `dict`.
/// Supports single frames, skippable frames, concatenated multi-frame streams, and xxHash64 checksum verification.
/// Returns total uncompressed bytes written into `dst`.
pub fn zstdDecompressWithDictInto(
    src: []const u8,
    dict: []const u8,
    dst: *[]u8,
    ws: *ZstdWorkspace,
): usize ! ZstdDecodeError {
    if (src.len == 0) return ZstdDecodeError.TruncatedInput;
    let in_pos: usize = 0;
    let out_pos: usize = 0;

    while (in_pos < src.len) {
        const info: ZstdFrameInfo = try zstdInspectFrame(src[in_pos..src.len]);
        if (info.is_skippable) {
            in_pos += info.header_bytes + info.skippable_payload_bytes;
            continue;
        }

        in_pos += info.header_bytes;
        ws.resetForFrame();
        const dict_content: []const u8 = try loadDictionaryForFrame(dict, info.dictionary_id, ws);
        const frame_out_start: usize = out_pos;

        // Block_Maximum_Size = min(Window_Size, 128 KiB) per RFC 8878 §3.1.1.2.4
        const max_block_size: usize = if (info.single_segment && info.window_size == 0)
            0
        else if (info.window_size > 0 && info.window_size < @as(u64, @intCast(ZSTD_MAX_BLOCK_BYTES)))
            @as(usize, @intCast(info.window_size))
        else
            ZSTD_MAX_BLOCK_BYTES;

        // Decode all blocks in this frame
        let last_block: bool = false;
        while (!last_block) {
            if (in_pos + 3 > src.len) return ZstdDecodeError.TruncatedInput;
            const bhdr: u32 = readU24Le(src, in_pos);
            in_pos += 3;

            last_block = (bhdr & 1) != 0;
            const block_type: u32 = (bhdr >> 1) & 3;
            const block_size: usize = @as(usize, bhdr >> 3);

            if (block_type == 3) return ZstdDecodeError.CorruptBlock; // Reserved
            if (block_size > max_block_size) return ZstdDecodeError.CorruptBlock;

            if (block_type == 0) {
                // Raw_Block
                if (in_pos + block_size > src.len) return ZstdDecodeError.TruncatedInput;
                if (out_pos + block_size > dst.len) return ZstdDecodeError.OutputTooSmall;
                if (block_size > 0) {
                    @memcpy(dst[out_pos..out_pos + block_size], src[in_pos..in_pos + block_size]);
                    out_pos += block_size;
                    in_pos += block_size;
                }
            } else if (block_type == 1) {
                // RLE_Block
                if (in_pos + 1 > src.len) return ZstdDecodeError.TruncatedInput;
                if (out_pos + block_size > dst.len) return ZstdDecodeError.OutputTooSmall;
                const fill_byte: u8 = src[in_pos];
                in_pos += 1;
                let k: usize = 0;
                while (k < block_size) : (k += 1) {
                    dst[out_pos + k] = fill_byte;
                }
                out_pos += block_size;
            } else {
                // Compressed_Block
                if (in_pos + block_size > src.len) return ZstdDecodeError.TruncatedInput;
                const blk_slice: []const u8 = src[in_pos..in_pos + block_size];
                in_pos += block_size;
                try decodeCompressedBlock(blk_slice, ws, dict_content, frame_out_start, info.window_size, max_block_size, dst, *out_pos);
            }
        }

        const frame_bytes: usize = out_pos - frame_out_start;
        if (info.has_content_size && @as(u64, @intCast(frame_bytes)) != info.content_size) {
            return ZstdDecodeError.ContentSizeMismatch;
        }

        if (info.has_content_checksum) {
            if (in_pos + 4 > src.len) return ZstdDecodeError.TruncatedInput;
            const expected_cksum: u32 = readU32Le(src, in_pos);
            in_pos += 4;
            const computed_cksum: u32 = @as(u32, @intCast(xxhash64Zstd(dst[frame_out_start..out_pos]) & 0xFFFF_FFFF));
            if (computed_cksum != expected_cksum) return ZstdDecodeError.ChecksumMismatch;
        }
    }

    return out_pos;
}

/// Zero-heap RFC 8878 Zstandard decompressor supporting single frames, skippable frames,
/// and concatenated multi-frame streams. Verifies xxHash64 content checksums when present.
/// Returns total uncompressed bytes written into `dst`.
pub fn zstdDecompressInto(
    src: []const u8,
    dst: *[]u8,
    ws: *ZstdWorkspace,
): usize ! ZstdDecodeError {
    const empty_dict: []const u8 = "";
    return zstdDecompressWithDictInto(src, empty_dict, dst, ws);
}

/// Convenience allocator-backed Zstandard decompressor. Caller owns returned slice (`alloc.free`).
pub fn zstdDecompress(
    alloc: std.mem.Allocator,
    src: []const u8,
    max_out_bytes: usize,
): []u8 ! ZstdDecodeError {
    if (src.len == 0 || max_out_bytes == 0) return ZstdDecodeError.TruncatedInput;

    // Determine initial output allocation from Frame_Content_Size if available, else bounded heuristic
    let alloc_len: usize = max_out_bytes;
    if (info := zstdInspectFrame(src) catch null) {
        if (!info.is_skippable && info.has_content_size) {
            if (info.content_size > @as(u64, @intCast(max_out_bytes))) {
                return ZstdDecodeError.OutputTooSmall;
            }
            const exact: usize = @as(usize, @intCast(info.content_size));
            alloc_len = if (exact > 0) exact else 1;
        }
    }

    const lit_buf: []u8 = alloc.alloc(u8, ZSTD_MAX_BLOCK_BYTES) catch return ZstdDecodeError.OutOfMemory;
    defer alloc.free(lit_buf);
    let ws: ZstdWorkspace = ZstdWorkspace.init(lit_buf);

    let out_buf: []u8 = alloc.alloc(u8, alloc_len) catch return ZstdDecodeError.OutOfMemory;
    const written: usize = zstdDecompressInto(src, out_buf, *ws) catch |e| {
        alloc.free(out_buf);
        return e;
    };

    if (written == out_buf.len) return out_buf;
    const shrunk: []u8 = alloc.alloc(u8, written) catch {
        alloc.free(out_buf);
        return ZstdDecodeError.OutOfMemory;
    };
    if (written > 0) @memcpy(shrunk, out_buf[0..written]);
    alloc.free(out_buf);
    return shrunk;
}
