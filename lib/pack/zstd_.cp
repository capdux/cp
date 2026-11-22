// Reference Only: Functional overlap with lib/pack/zstd.cp | Kept for comparison/analysis
// Source: cp/lib/zstd/zstd.cp (Pure CP RFC 8878 Zstandard Frame & Block Codec + XXH64)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");

/// Error set for Zstandard compression, decompression, and frame inspection.
pub err ZstdError {
    InvalidMagic,
    CorruptedFrame,
    ChecksumMismatch,
    UnsupportedBlockType,
    WindowTooLarge,
    OutputTooLarge,
    TruncatedInput,
    OutOfMemory,
}

pub const ZSTD_MAGIC: u32 = 0xFD2FB528;
pub const ZSTD_SKIPPABLE_MAGIC_BASE: u32 = 0x184D2A50;
pub const ZSTD_MAX_BLOCK_SIZE: usize = 131072; // 128 KB

pub struct ZstdFrameInfo {
    pub frame_content_size: u64?,
    pub window_size: u64,
    pub has_checksum: bool,
    pub dictionary_id: u32?,
}

// ============================================================================
// XXH64 Implementation (RFC 8878 Content Checksum)
// ============================================================================

const PRIME64_1: u64 = 0x9E3779B185EBCA87;
const PRIME64_2: u64 = 0xC2B2AE3D27D4EB4F;
const PRIME64_3: u64 = 0x165667B19E3779F9;
const PRIME64_4: u64 = 0x85EBCA77C2B2AE63;
const PRIME64_5: u64 = 0x27D4EB2F165667C5;

@inline
fn rotl64(x: u64, r: u6): u64 {
    return std.math.rotl(u64, x, r);
}

@inline
fn readU16LE(data: []const u8, pos: usize): u16 {
    return (data[pos] as u16) | ((data[pos + 1] as u16) << 8);
}

@inline
fn readU32LE(data: []const u8, pos: usize): u32 {
    return (data[pos] as u32) |
        ((data[pos + 1] as u32) << 8) |
        ((data[pos + 2] as u32) << 16) |
        ((data[pos + 3] as u32) << 24);
}

@inline
fn readU64LE(data: []const u8, pos: usize): u64 {
    return (readU32LE(data, pos) as u64) | ((readU32LE(data, pos + 4) as u64) << 32);
}

@inline
fn xxh64Round(acc: u64, input: u64): u64 {
    let v = acc +% (input *% PRIME64_2);
    v = rotl64(v, 31);
    return v *% PRIME64_1;
}

@inline
fn xxh64MergeRound(acc: u64, val: u64): u64 {
    let v = xxh64Round(0, val);
    let res = acc ^ v;
    return res *% PRIME64_1 +% PRIME64_4;
}

/// Computes the standard 64-bit xxHash (XXH64) of `data` with `seed`.
pub fn xxhash64(data: []const u8, seed: u64): u64 {
    let h64: u64 = 0;
    let p: usize = 0;

    if (data.len >= 32) {
        let v1: u64 = seed +% PRIME64_1 +% PRIME64_2;
        let v2: u64 = seed +% PRIME64_2;
        let v3: u64 = seed;
        let v4: u64 = seed -% PRIME64_1;

        while (p + 32 <= data.len) {
            v1 = xxh64Round(v1, readU64LE(data, p));
            v2 = xxh64Round(v2, readU64LE(data, p + 8));
            v3 = xxh64Round(v3, readU64LE(data, p + 16));
            v4 = xxh64Round(v4, readU64LE(data, p + 24));
            p += 32;
        }

        h64 = rotl64(v1, 1) +% rotl64(v2, 7) +% rotl64(v3, 12) +% rotl64(v4, 18);
        h64 = xxh64MergeRound(h64, v1);
        h64 = xxh64MergeRound(h64, v2);
        h64 = xxh64MergeRound(h64, v3);
        h64 = xxh64MergeRound(h64, v4);
    } else {
        h64 = seed +% PRIME64_5;
    }

    h64 = h64 +% (data.len as u64);

    while (p + 8 <= data.len) {
        const k1 = xxh64Round(0, readU64LE(data, p));
        h64 ^= k1;
        h64 = rotl64(h64, 27) *% PRIME64_1 +% PRIME64_4;
        p += 8;
    }

    if (p + 4 <= data.len) {
        h64 ^= (readU32LE(data, p) as u64) *% PRIME64_1;
        h64 = rotl64(h64, 23) *% PRIME64_2 +% PRIME64_3;
        p += 4;
    }

    while (p < data.len) {
        h64 ^= (data[p] as u64) *% PRIME64_5;
        h64 = rotl64(h64, 11) *% PRIME64_1;
        p += 1;
    }

    h64 ^= h64 >> 33;
    h64 = h64 *% PRIME64_2;
    h64 ^= h64 >> 29;
    h64 = h64 *% PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}

// ============================================================================
// Low-level Output Helpers
// ============================================================================

fn appendU8(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u8): void ! ZstdError {
    out.append(alloc, v) catch return ZstdError.OutOfMemory;
}

fn appendU16LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u16): void ! ZstdError {
    const buf: [2]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, buf[0..]) catch return ZstdError.OutOfMemory;
}

fn appendU24LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void ! ZstdError {
    const buf: [3]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, buf[0..]) catch return ZstdError.OutOfMemory;
}

fn appendU32LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void ! ZstdError {
    const buf: [4]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 24) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, buf[0..]) catch return ZstdError.OutOfMemory;
}

fn appendU64LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u64): void ! ZstdError {
    appendU32LE(out, alloc, (v & 0xFFFFFFFF) as u32)?;
    appendU32LE(out, alloc, ((v >> 32) & 0xFFFFFFFF) as u32)?;
}

// ============================================================================
// Frame Header Inspection (RFC 8878 Section 3.1.1)
// ============================================================================

struct ParsedHeader {
    info: ZstdFrameInfo,
    header_end: usize,
}

fn isSkippableMagic(magic: u32): bool {
    return (magic & 0xFFFFFFF0) == ZSTD_SKIPPABLE_MAGIC_BASE;
}

fn parseFrameHeaderAt(bytes: []const u8, start_pos: usize): ParsedHeader ! ZstdError {
    let pos = start_pos;
    while (true) {
        if (pos + 4 > bytes.len) {
            return ZstdError.TruncatedInput;
        }
        const magic = readU32LE(bytes, pos);
        if (isSkippableMagic(magic)) {
            if (pos + 8 > bytes.len) {
                return ZstdError.TruncatedInput;
            }
            const skip_size = readU32LE(bytes, pos + 4) as usize;
            if (pos + 8 + skip_size > bytes.len) {
                return ZstdError.TruncatedInput;
            }
            pos += 8 + skip_size;
            if (pos == bytes.len) {
                return ZstdError.InvalidMagic;
            }
            continue;
        }
        if (magic != ZSTD_MAGIC) {
            return ZstdError.InvalidMagic;
        }
        pos += 4;
        break;
    }

    if (pos >= bytes.len) {
        return ZstdError.TruncatedInput;
    }
    const fhd = bytes[pos];
    pos += 1;

    const fcs_flag = (fhd >> 6) & 0x03;
    const single_segment = ((fhd >> 5) & 0x01) != 0;
    const reserved_bit = (fhd >> 3) & 0x01;
    const has_checksum = ((fhd >> 2) & 0x01) != 0;
    const dict_id_flag = fhd & 0x03;

    if (reserved_bit != 0) {
        return ZstdError.CorruptedFrame;
    }

    let window_size: u64 = 0;
    if (!single_segment) {
        if (pos >= bytes.len) {
            return ZstdError.TruncatedInput;
        }
        const wd = bytes[pos];
        pos += 1;
        const exponent = (wd >> 3) as u6;
        const mantissa = (wd & 0x07) as u64;
        const shift_amt: u6 = @intCast(10 + (exponent as u7));
        const window_base: u64 = (@as(u64, 1)) << shift_amt;
        const window_add: u64 = (window_base >> 3) * mantissa;
        window_size = window_base + window_add;
    }

    let dict_id: u32? = null;
    if (dict_id_flag == 1) {
        if (pos + 1 > bytes.len) return ZstdError.TruncatedInput;
        dict_id = bytes[pos] as u32;
        pos += 1;
    } else if (dict_id_flag == 2) {
        if (pos + 2 > bytes.len) return ZstdError.TruncatedInput;
        dict_id = readU16LE(bytes, pos) as u32;
        pos += 2;
    } else if (dict_id_flag == 3) {
        if (pos + 4 > bytes.len) return ZstdError.TruncatedInput;
        dict_id = readU32LE(bytes, pos);
        pos += 4;
    }

    let fcs: u64? = null;
    if (fcs_flag == 0) {
        if (single_segment) {
            if (pos + 1 > bytes.len) return ZstdError.TruncatedInput;
            fcs = bytes[pos] as u64;
            pos += 1;
        }
    } else if (fcs_flag == 1) {
        if (pos + 2 > bytes.len) return ZstdError.TruncatedInput;
        fcs = (readU16LE(bytes, pos) as u64) + 256;
        pos += 2;
    } else if (fcs_flag == 2) {
        if (pos + 4 > bytes.len) return ZstdError.TruncatedInput;
        fcs = readU32LE(bytes, pos) as u64;
        pos += 4;
    } else {
        if (pos + 8 > bytes.len) return ZstdError.TruncatedInput;
        fcs = readU64LE(bytes, pos);
        pos += 8;
    }

    if (single_segment) {
        if (sz := fcs) {
            window_size = if (sz > 0) sz else 1;
        } else {
            return ZstdError.CorruptedFrame;
        }
    }

    return ParsedHeader {
        info: ZstdFrameInfo {
            frame_content_size: fcs,
            window_size: window_size,
            has_checksum: has_checksum,
            dictionary_id: dict_id,
        },
        header_end: pos,
    };
}

/// Inspects the first Zstandard frame header in `bytes` (skipping any leading Skippable Frames).
pub fn inspectFrameHeader(bytes: []const u8): ZstdFrameInfo ! ZstdError {
    const parsed = parseFrameHeaderAt(bytes, 0)?;
    return parsed.info;
}

// ============================================================================
// Compression Engine (RFC 8878 Frame + RLE / Raw Mixed Blocks + XXH64)
// ============================================================================

fn emitBlockHeader(out: *std.ArrayList(u8), alloc: std.mem.Allocator, last: bool, block_type: u32, block_size: usize): void ! ZstdError {
    const last_bit: u32 = if (last) 1 else 0;
    const header: u32 = last_bit | ((block_type & 3) << 1) | ((block_size as u32) << 3);
    appendU24LE(out, alloc, header)?;
}

/// Compresses `input` into a valid RFC 8878 Zstandard frame using RLE and Raw blocks.
pub fn compress(allocator: std.mem.Allocator, input: []const u8, write_checksum: bool): []u8 ! ZstdError {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    // 1. Magic Number
    appendU32LE(&out, allocator, ZSTD_MAGIC)?;

    // 2. Frame Header Descriptor (Single_Segment = 1)
    let fcs_flag: u8 = 0;
    if (input.len < 256) {
        fcs_flag = 0;
    } else if (input.len <= 65791) {
        fcs_flag = 1;
    } else if (input.len <= 0xFFFFFFFF) {
        fcs_flag = 2;
    } else {
        fcs_flag = 3;
    }
    const cksum_bit: u8 = if (write_checksum) 0x04 else 0x00;
    const fhd: u8 = (fcs_flag << 6) | 0x20 | cksum_bit;
    appendU8(&out, allocator, fhd)?;

    // Frame_Content_Size
    if (fcs_flag == 0) {
        appendU8(&out, allocator, input.len as u8)?;
    } else if (fcs_flag == 1) {
        appendU16LE(&out, allocator, (input.len - 256) as u16)?;
    } else if (fcs_flag == 2) {
        appendU32LE(&out, allocator, input.len as u32)?;
    } else {
        appendU64LE(&out, allocator, input.len as u64)?;
    }

    // 3. Blocks
    if (input.len == 0) {
        emitBlockHeader(&out, allocator, true, 0, 0)?;
    } else {
        let pos: usize = 0;
        let raw_start: usize = 0;

        while (pos < input.len) {
            // Check for an RLE run starting at `pos`
            let run_len: usize = 1;
            const b = input[pos];
            while (pos + run_len < input.len and input[pos + run_len] == b and run_len < ZSTD_MAX_BLOCK_SIZE) {
                run_len += 1;
            }

            if (run_len >= 16) {
                // Flush any pending raw bytes before this RLE run
                while (raw_start < pos) {
                    let chunk = pos - raw_start;
                    if (chunk > ZSTD_MAX_BLOCK_SIZE) {
                        chunk = ZSTD_MAX_BLOCK_SIZE;
                    }
                    emitBlockHeader(&out, allocator, false, 0, chunk)?;
                    out.appendSlice(allocator, input[raw_start .. raw_start + chunk]) catch return ZstdError.OutOfMemory;
                    raw_start += chunk;
                }

                // Emit RLE block
                const is_last = (pos + run_len == input.len);
                emitBlockHeader(&out, allocator, is_last, 1, run_len)?;
                appendU8(&out, allocator, b)?;
                pos += run_len;
                raw_start = pos;
            } else {
                pos += 1;
                if (pos - raw_start == ZSTD_MAX_BLOCK_SIZE) {
                    const is_last = (pos == input.len);
                    emitBlockHeader(&out, allocator, is_last, 0, ZSTD_MAX_BLOCK_SIZE)?;
                    out.appendSlice(allocator, input[raw_start .. pos]) catch return ZstdError.OutOfMemory;
                    raw_start = pos;
                }
            }
        }

        // Flush remaining raw bytes
        if (raw_start < input.len) {
            while (raw_start < input.len) {
                let chunk = input.len - raw_start;
                if (chunk > ZSTD_MAX_BLOCK_SIZE) {
                    chunk = ZSTD_MAX_BLOCK_SIZE;
                }
                const is_last = (raw_start + chunk == input.len);
                emitBlockHeader(&out, allocator, is_last, 0, chunk)?;
                out.appendSlice(allocator, input[raw_start .. raw_start + chunk]) catch return ZstdError.OutOfMemory;
                raw_start += chunk;
            }
        }
    }

    // 4. Optional 32-bit XXH64 Content Checksum
    if (write_checksum) {
        const hash = xxhash64(input, 0);
        appendU32LE(&out, allocator, (hash & 0xFFFFFFFF) as u32)?;
    }

    return out.toOwnedSlice(allocator) catch return ZstdError.OutOfMemory;
}

// ============================================================================
// Backward Bitstream Reader & FSE / Huffman Decoders (RFC 8878 Section 4)
// ============================================================================

struct BackwardBitReader {
    data: []const u8,
    bit_pos: usize, // total bits remaining from start of buffer
}

fn initBackwardBitReader(data: []const u8): BackwardBitReader ! ZstdError {
    if (data.len == 0) {
        return ZstdError.CorruptedFrame;
    }
    const last_byte = data[data.len - 1];
    if (last_byte == 0) {
        return ZstdError.CorruptedFrame;
    }
    let highest_bit: usize = 7;
    while (((last_byte >> @intCast(highest_bit)) & 1) == 0) {
        highest_bit -= 1;
    }
    const total_bits = (data.len - 1) * 8 + highest_bit;
    return BackwardBitReader {
        data: data,
        bit_pos: total_bits,
    };
}

fn readBitsBackward(r: *BackwardBitReader, nb_bits: u6): u64 ! ZstdError {
    if (nb_bits == 0) return 0;
    const n = nb_bits as usize;
    if (r.bit_pos < n) {
        return ZstdError.CorruptedFrame;
    }
    r.bit_pos -= n;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const abs_bit = r.bit_pos + i;
        const byte_idx = abs_bit >> 3;
        const bit_idx = (abs_bit & 7) as u3;
        const bit = ((r.data[byte_idx] >> bit_idx) & 1) as u64;
        val |= bit << @intCast(i);
    }
    return val;
}

struct FseEntry {
    symbol: u8,
    nb_bits: u8,
    next_state: u16,
}

struct FseTable {
    accuracy_log: u5,
    entries: ([]FseEntry)?,
    is_rle: bool,
    rle_symbol: u8,
}

fn freeFseTable(alloc: std.mem.Allocator, tbl: *FseTable) {
    if (e := tbl.entries) {
        if (e.len > 0) {
            alloc.free(e);
        }
        tbl.entries = null;
    }
}

fn buildFseTableFromCounts(alloc: std.mem.Allocator, norm_counts: []const i16, max_symbol: usize, accuracy_log: u5): FseTable ! ZstdError {
    const table_size: usize = (@as(usize, 1)) << accuracy_log;
    let entries = alloc.alloc(FseEntry, table_size) catch return ZstdError.OutOfMemory;
    errdefer alloc.free(entries);

    let symbols = alloc.alloc(u8, table_size) catch return ZstdError.OutOfMemory;
    defer alloc.free(symbols);
    @memset(symbols, 0);

    let next_state_vals = alloc.alloc(u16, max_symbol + 1) catch return ZstdError.OutOfMemory;
    defer alloc.free(next_state_vals);
    @memset(next_state_vals, 0);

    let high_threshold = table_size;
    let s: usize = 0;
    while (s <= max_symbol) : (s += 1) {
        if (norm_counts[s] == -1) {
            if (high_threshold == 0) return ZstdError.CorruptedFrame;
            high_threshold -= 1;
            symbols[high_threshold] = s as u8;
            next_state_vals[s] = 1;
        } else if (norm_counts[s] > 0) {
            next_state_vals[s] = norm_counts[s] as u16;
        }
    }

    const step = (table_size >> 1) + (table_size >> 3) + 3;
    const mask = table_size - 1;
    let pos: usize = 0;

    s = 0;
    while (s <= max_symbol) : (s += 1) {
        if (norm_counts[s] > 0) {
            const cnt = norm_counts[s] as usize;
            let c: usize = 0;
            while (c < cnt) : (c += 1) {
                symbols[pos] = s as u8;
                pos = (pos + step) & mask;
                while (pos >= high_threshold) {
                    pos = (pos + step) & mask;
                }
            }
        }
    }
    if (pos != 0) {
        return ZstdError.CorruptedFrame;
    }

    let u: usize = 0;
    while (u < table_size) : (u += 1) {
        const sym = symbols[u] as usize;
        const ns = next_state_vals[sym];
        next_state_vals[sym] += 1;
        let nb_bits: u8 = 0;
        let shifted = ns as usize;
        while ((shifted << nb_bits) < table_size) {
            nb_bits += 1;
        }
        const baseline = ((ns as usize) << nb_bits) - table_size;
        entries[u] = FseEntry {
            symbol: sym as u8,
            nb_bits: nb_bits,
            next_state: baseline as u16,
        };
    }

    return FseTable {
        accuracy_log: accuracy_log,
        entries: entries,
        is_rle: false,
        rle_symbol: 0,
    };
}

// Predefined FSE distributions (RFC 8878 Section 3.1.1.3.2.2)
const LL_DEFAULT_DIST: [36]i16 = [
    4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1,
    -1, -1, -1, -1,
];

const ML_DEFAULT_DIST: [53]i16 = [
    1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1,
    -1, -1, -1, -1, -1,
];

const OF_DEFAULT_DIST: [29]i16 = [
    1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1,
];

struct FseHeaderResult {
    table: FseTable,
    bytes_read: usize,
}

fn loadBits(bc: *u32, bcnt: *u5, ip: *usize, buf: []const u8) {
    while (bcnt.* < 16 and ip.* < buf.len) {
        bc.* |= (buf[ip.*] as u32) << bcnt.*;
        bcnt.* += 8;
        ip.* += 1;
    }
}

fn readFseNCountHeader(alloc: std.mem.Allocator, src: []const u8, max_symbol_limit: usize, max_log: u5): FseHeaderResult ! ZstdError {
    if (src.len == 0) return ZstdError.TruncatedInput;
    let bit_container: u32 = 0;
    let bit_count: u5 = 0;
    let ipos: usize = 0;

    loadBits(&bit_container, &bit_count, &ipos, src);
    if (bit_count < 4) return ZstdError.TruncatedInput;
    const accuracy_log_raw = (bit_container & 0x0F) + 5;
    if (accuracy_log_raw > (max_log as u32)) return ZstdError.CorruptedFrame;
    const accuracy_log = accuracy_log_raw as u5;
    bit_container >>= 4;
    bit_count -= 4;

    let remaining: i32 = ((@as(i32, 1)) << accuracy_log) + 1;
    let threshold: i32 = (@as(i32, 1)) << accuracy_log;
    let nb_bits: u5 = (accuracy_log as u5) + 1;

    let counts = alloc.alloc(i16, max_symbol_limit + 1) catch return ZstdError.OutOfMemory;
    defer alloc.free(counts);
    @memset(counts, 0);

    let charnum: usize = 0;
    let previous0 = false;

    while (remaining > 1 and charnum <= max_symbol_limit) {
        loadBits(&bit_container, &bit_count, &ipos, src);
        if (previous0) {
            let n0 = charnum;
            while ((bit_container & 0xFFFF) == 0xFFFF) {
                n0 += 24;
                if (ipos < src.len) {
                    bit_container >>= 16;
                    bit_count -= 16;
                    loadBits(&bit_container, &bit_count, &ipos, src);
                } else {
                    break;
                }
            }
            while ((bit_container & 3) == 3) {
                n0 += 3;
                bit_container >>= 2;
                bit_count -= 2;
                loadBits(&bit_container, &bit_count, &ipos, src);
            }
            n0 += (bit_container & 3) as usize;
            bit_container >>= 2;
            bit_count -= 2;
            if (n0 > max_symbol_limit) return ZstdError.CorruptedFrame;
            charnum = n0;
            if (charnum > max_symbol_limit) break;
        }

        loadBits(&bit_container, &bit_count, &ipos, src);
        const max_val = (2 * threshold - 1) - remaining;
        let count: i32 = 0;
        const mask = ((@as(u32, 1)) << (nb_bits - 1)) - 1;
        if ((bit_container & mask) as i32 < max_val) {
            count = (bit_container & mask) as i32;
            bit_container >>= (nb_bits - 1);
            bit_count -= (nb_bits - 1);
        } else {
            const full_mask = ((@as(u32, 1)) << nb_bits) - 1;
            count = (bit_container & full_mask) as i32;
            if (count >= threshold) {
                count -= max_val;
            }
            bit_container >>= nb_bits;
            bit_count -= nb_bits;
        }

        count -= 1;
        remaining -= if (count < 0) -count else count;
        counts[charnum] = count as i16;
        charnum += 1;
        previous0 = (count == 0);

        while (remaining < threshold and nb_bits > 1) {
            nb_bits -= 1;
            threshold >>= 1;
        }
    }

    if (remaining != 1 or charnum == 0) {
        return ZstdError.CorruptedFrame;
    }

    // Unused trailing bits in last byte
    const bytes_read = ipos - ((bit_count as usize) >> 3);
    const tbl = buildFseTableFromCounts(alloc, counts, charnum - 1, accuracy_log)?;
    return FseHeaderResult {
        table: tbl,
        bytes_read: bytes_read,
    };
}

// ============================================================================
// Huffman Literal Decoder (RFC 8878 Section 4.2)
// ============================================================================

struct HuffEntry {
    symbol: u8,
    nb_bits: u8,
}

struct HuffTable {
    max_bits: u5,
    entries: ([]HuffEntry)?,
}

fn freeHuffTable(alloc: std.mem.Allocator, ht: *HuffTable) {
    if (e := ht.entries) {
        if (e.len > 0) {
            alloc.free(e);
        }
        ht.entries = null;
    }
}

fn buildHuffTableFromWeights(alloc: std.mem.Allocator, weights: []const u8, num_symbols: usize): HuffTable ! ZstdError {
    let weight_sum: u32 = 0;
    let i: usize = 0;
    while (i < num_symbols) : (i += 1) {
        if (weights[i] > 0) {
            if (weights[i] > 12) return ZstdError.CorruptedFrame;
            weight_sum += (@as(u32, 1)) << @intCast(weights[i] - 1);
        }
    }

    let max_bits: u5 = 0;
    while (((@as(u32, 1)) << max_bits) <= weight_sum) {
        max_bits += 1;
    }
    if (max_bits > 11) return ZstdError.CorruptedFrame;
    const total_weight: u32 = (@as(u32, 1)) << max_bits;
    const left = total_weight - weight_sum;
    if (left == 0 or (left & (left - 1)) != 0) {
        return ZstdError.CorruptedFrame;
    }

    let last_weight: u8 = 1;
    let tmp = left;
    while (tmp > 1) {
        tmp >>= 1;
        last_weight += 1;
    }

    const total_syms = num_symbols + 1;
    if (total_syms > 256) return ZstdError.CorruptedFrame;

    let nb_bits_arr: [256]u8 = undefined;
    @memset(nb_bits_arr[0..], 0);
    i = 0;
    while (i < num_symbols) : (i += 1) {
        if (weights[i] > 0) {
            nb_bits_arr[i] = (max_bits as u8) + 1 - weights[i];
        }
    }
    nb_bits_arr[num_symbols] = (max_bits as u8) + 1 - last_weight;

    // Count bits
    let rank_count: [16]u32 = undefined;
    @memset(rank_count[0..], 0);
    i = 0;
    while (i < total_syms) : (i += 1) {
        rank_count[nb_bits_arr[i]] += 1;
    }

    let rank_start: [16]u32 = undefined;
    @memset(rank_start[0..], 0);
    let cur: u32 = 0;
    let b: usize = (max_bits as usize);
    while (b >= 1) : (b -= 1) {
        rank_start[b] = cur;
        cur += rank_count[b] * ((@as(u32, 1)) << @intCast((max_bits as usize) - b));
    }

    const table_size: usize = (@as(usize, 1)) << max_bits;
    let entries = alloc.alloc(HuffEntry, table_size) catch return ZstdError.OutOfMemory;
    errdefer alloc.free(entries);

    i = 0;
    while (i < total_syms) : (i += 1) {
        const nb = nb_bits_arr[i];
        if (nb == 0) continue;
        const code_len = (@as(usize, 1)) << @intCast((max_bits as u8) - nb);
        const start = rank_start[nb] as usize;
        rank_start[nb] += code_len as u32;
        let k: usize = 0;
        while (k < code_len) : (k += 1) {
            entries[start + k] = HuffEntry {
                symbol: i as u8,
                nb_bits: nb,
            };
        }
    }

    return HuffTable {
        max_bits: max_bits,
        entries: entries,
    };
}

fn decodeHuffStream(ht: *const HuffTable, stream: []const u8, out: []u8): void ! ZstdError {
    const entries = ht.entries orelse return ZstdError.CorruptedFrame;
    let br = initBackwardBitReader(stream)?;
    const max_b = ht.max_bits as u6;
    let i: usize = 0;
    while (i < out.len) : (i += 1) {
        // Peek `max_b` bits (or fewer if near end of stream, zero-padded on LSB)
        let idx: usize = 0;
        let k: usize = 0;
        while (k < (max_b as usize)) : (k += 1) {
            if (br.bit_pos > k) {
                const abs_bit = br.bit_pos - 1 - k;
                const byte_idx = abs_bit >> 3;
                const bit_idx = (abs_bit & 7) as u3;
                const bit = ((br.data[byte_idx] >> bit_idx) & 1) as usize;
                idx = (idx << 1) | bit;
            } else {
                idx = idx << 1;
            }
        }
        const entry = entries[idx];
        if (entry.nb_bits == 0 or br.bit_pos < (entry.nb_bits as usize)) {
            return ZstdError.CorruptedFrame;
        }
        br.bit_pos -= entry.nb_bits as usize;
        out[i] = entry.symbol;
    }
    if (br.bit_pos != 0) {
        return ZstdError.CorruptedFrame;
    }
}

struct HuffParseResult {
    table: HuffTable,
    bytes_read: usize,
}

fn parseHuffTree(alloc: std.mem.Allocator, src: []const u8): HuffParseResult ! ZstdError {
    if (src.len == 0) return ZstdError.TruncatedInput;
    const header_byte = src[0];
    let weights: [256]u8 = undefined;
    @memset(weights[0..], 0);
    let num_symbols: usize = 0;
    let bytes_read: usize = 0;

    if (header_byte >= 128) {
        num_symbols = (header_byte - 127) as usize;
        const data_bytes = (num_symbols + 1) / 2;
        if (1 + data_bytes > src.len) return ZstdError.TruncatedInput;
        let i: usize = 0;
        while (i < num_symbols) : (i += 1) {
            const byte_val = src[1 + (i / 2)];
            weights[i] = if ((i & 1) == 0) (byte_val >> 4) else (byte_val & 0x0F);
        }
        bytes_read = @as(usize, 1) + data_bytes;
    } else {
        const comp_size = header_byte as usize;
        if (1 + comp_size > src.len) return ZstdError.TruncatedInput;
        const fse_res = readFseNCountHeader(alloc, src[1 .. 1 + comp_size], 255, 6)?;
        let fse_tbl = fse_res.table;
        defer freeFseTable(alloc, &fse_tbl);
        const fse_entries = fse_tbl.entries orelse return ZstdError.CorruptedFrame;

        if (fse_res.bytes_read >= comp_size) return ZstdError.CorruptedFrame;
        let br = initBackwardBitReader(src[1 + fse_res.bytes_read .. 1 + comp_size])?;
        let state1 = (readBitsBackward(&br, fse_tbl.accuracy_log as u6)?) as usize;
        let state2 = (readBitsBackward(&br, fse_tbl.accuracy_log as u6)?) as usize;

        while (num_symbols < 255) {
            const e1 = fse_entries[state1];
            weights[num_symbols] = e1.symbol;
            num_symbols += 1;
            if (br.bit_pos < (e1.nb_bits as usize)) break;
            const b1 = (readBitsBackward(&br, e1.nb_bits as u6)?) as usize;
            state1 = (e1.next_state as usize) + b1;

            const e2 = fse_entries[state2];
            weights[num_symbols] = e2.symbol;
            num_symbols += 1;
            if (br.bit_pos < (e2.nb_bits as usize)) break;
            const b2 = (readBitsBackward(&br, e2.nb_bits as u6)?) as usize;
            state2 = (e2.next_state as usize) + b2;
        }
        bytes_read = @as(usize, 1) + comp_size;
    }

    const tbl = buildHuffTableFromWeights(alloc, weights[0..], num_symbols)?;
    return HuffParseResult {
        table: tbl,
        bytes_read: bytes_read,
    };
}

// ============================================================================
// Compressed Block Decoder (RFC 8878 Section 3.1.1.3)
// ============================================================================

struct DecoderContext {
    huff_table: HuffTable,
    has_huff: bool,
    ll_table: FseTable,
    has_ll: bool,
    of_table: FseTable,
    has_of: bool,
    ml_table: FseTable,
    has_ml: bool,
    rep: [3]usize,
}

fn initDecoderContext(): DecoderContext {
    return DecoderContext {
        huff_table: HuffTable { max_bits: 0, entries: null },
        has_huff: false,
        ll_table: FseTable { accuracy_log: 0, entries: null, is_rle: false, rle_symbol: 0 },
        has_ll: false,
        of_table: FseTable { accuracy_log: 0, entries: null, is_rle: false, rle_symbol: 0 },
        has_of: false,
        ml_table: FseTable { accuracy_log: 0, entries: null, is_rle: false, rle_symbol: 0 },
        has_ml: false,
        rep: [1, 4, 8],
    };
}

fn freeDecoderContext(alloc: std.mem.Allocator, ctx: *DecoderContext) {
    freeHuffTable(alloc, &ctx.huff_table);
    freeFseTable(alloc, &ctx.ll_table);
    freeFseTable(alloc, &ctx.of_table);
    freeFseTable(alloc, &ctx.ml_table);
}

const LL_BASE: [36]u32 = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 18, 20, 22, 24, 28, 32, 40, 48, 64, 128, 256, 512, 1024, 2048, 4096,
    8192, 16384, 32768, 65536,
];

const LL_BITS: [36]u6 = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16,
];

const ML_BASE: [53]u32 = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 131, 259, 515, 1027, 2051,
    4099, 8195, 16387, 32771, 65539,
];

const ML_BITS: [53]u6 = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16,
];

fn selectSeqTable(
    alloc: std.mem.Allocator,
    mode: u8,
    src: []const u8,
    pos: *usize,
    prev: *FseTable,
    has_prev: *bool,
    default_dist: []const i16,
    default_log: u5,
    max_sym: usize,
    max_log: u5,
): void ! ZstdError {
    if (mode == 0) {
        freeFseTable(alloc, prev);
        prev.* = buildFseTableFromCounts(alloc, default_dist, default_dist.len - 1, default_log)?;
        has_prev.* = true;
    } else if (mode == 1) {
        if (pos.* >= src.len) return ZstdError.TruncatedInput;
        const sym = src[pos.*];
        pos.* += 1;
        freeFseTable(alloc, prev);
        prev.* = FseTable {
            accuracy_log: 0,
            entries: null,
            is_rle: true,
            rle_symbol: sym,
        };
        has_prev.* = true;
    } else if (mode == 2) {
        const res = readFseNCountHeader(alloc, src[pos.*..], max_sym, max_log)?;
        pos.* += res.bytes_read;
        freeFseTable(alloc, prev);
        prev.* = res.table;
        has_prev.* = true;
    } else {
        if (!has_prev.*) return ZstdError.CorruptedFrame;
    }
}

fn decodeCompressedBlock(
    alloc: std.mem.Allocator,
    block: []const u8,
    ctx: *DecoderContext,
    out: *std.ArrayList(u8),
    frame_start: usize,
    max_output_bytes: usize,
): void ! ZstdError {
    if (block.len == 0) return ZstdError.TruncatedInput;
    let pos: usize = 0;

    // 1. Literals Section
    const lit_b0 = block[pos];
    const lit_type = lit_b0 & 3;
    const lit_fmt = (lit_b0 >> 2) & 3;

    let literals = std.ArrayList(u8).empty;
    defer literals.deinit(alloc);

    if (lit_type == 0 or lit_type == 1) {
        let regen_size: usize = 0;
        if ((lit_fmt & 1) == 0) {
            regen_size = (lit_b0 >> 3) as usize;
            pos += 1;
        } else if (lit_fmt == 1) {
            if (pos + 2 > block.len) return ZstdError.TruncatedInput;
            regen_size = ((lit_b0 >> 4) as usize) | ((block[pos + 1] as usize) << 4);
            pos += 2;
        } else {
            if (pos + 3 > block.len) return ZstdError.TruncatedInput;
            regen_size = ((lit_b0 >> 4) as usize) | ((block[pos + 1] as usize) << 4) | ((block[pos + 2] as usize) << 12);
            pos += 3;
        }
        if (regen_size > ZSTD_MAX_BLOCK_SIZE) return ZstdError.CorruptedFrame;
        if (lit_type == 0) {
            if (pos + regen_size > block.len) return ZstdError.TruncatedInput;
            literals.appendSlice(alloc, block[pos .. pos + regen_size]) catch return ZstdError.OutOfMemory;
            pos += regen_size;
        } else {
            if (pos + 1 > block.len) return ZstdError.TruncatedInput;
            const val = block[pos];
            pos += 1;
            literals.appendNTimes(alloc, val, regen_size) catch return ZstdError.OutOfMemory;
        }
    } else {
        // Compressed (2) or Treeless (3)
        let regen_size: usize = 0;
        let comp_size: usize = 0;
        let num_streams: usize = 4;

        if (lit_fmt == 0 or lit_fmt == 1) {
            if (pos + 3 > block.len) return ZstdError.TruncatedInput;
            const v = (block[pos] as usize) | ((block[pos + 1] as usize) << 8) | ((block[pos + 2] as usize) << 16);
            regen_size = (v >> 4) & 0x3FF;
            comp_size = (v >> 14) & 0x3FF;
            if (lit_fmt == 0) num_streams = 1;
            pos += 3;
        } else if (lit_fmt == 2) {
            if (pos + 4 > block.len) return ZstdError.TruncatedInput;
            const v = readU32LE(block, pos) as usize;
            regen_size = (v >> 4) & 0x3FFF;
            comp_size = (v >> 18) & 0x3FFF;
            pos += 4;
        } else {
            if (pos + 5 > block.len) return ZstdError.TruncatedInput;
            const v0 = readU32LE(block, pos) as u64;
            const v = v0 | ((block[pos + 4] as u64) << 32);
            regen_size = ((v >> 4) & 0x3FFFF) as usize;
            comp_size = ((v >> 22) & 0x3FFFF) as usize;
            pos += 5;
        }

        if (pos + comp_size > block.len) return ZstdError.TruncatedInput;
        const comp_slice = block[pos .. pos + comp_size];
        pos += comp_size;

        let cpos: usize = 0;
        if (lit_type == 2) {
            const hres = parseHuffTree(alloc, comp_slice)?;
            freeHuffTable(alloc, &ctx.huff_table);
            ctx.huff_table = hres.table;
            ctx.has_huff = true;
            cpos = hres.bytes_read;
        } else {
            if (!ctx.has_huff) return ZstdError.CorruptedFrame;
        }

        literals.resize(alloc, regen_size) catch return ZstdError.OutOfMemory;
        if (num_streams == 1) {
            decodeHuffStream(&ctx.huff_table, comp_slice[cpos..], literals.items)?;
        } else {
            if (cpos + 6 > comp_slice.len) return ZstdError.TruncatedInput;
            const s1_len = readU16LE(comp_slice, cpos) as usize;
            const s2_len = readU16LE(comp_slice, cpos + 2) as usize;
            const s3_len = readU16LE(comp_slice, cpos + 4) as usize;
            const data_start = cpos + 6;
            if (data_start + s1_len + s2_len + s3_len > comp_slice.len) return ZstdError.TruncatedInput;
            const s4_len = comp_slice.len - (data_start + s1_len + s2_len + s3_len);
            const seg = (regen_size + 3) / 4;
            const l1 = if (seg < regen_size) seg else regen_size;
            const l2 = if (seg < regen_size - l1) seg else (regen_size - l1);
            const l3 = if (seg < regen_size - l1 - l2) seg else (regen_size - l1 - l2);
            const l4 = regen_size - l1 - l2 - l3;

            let sp = data_start;
            decodeHuffStream(&ctx.huff_table, comp_slice[sp .. sp + s1_len], literals.items[0 .. l1])?;
            sp += s1_len;
            decodeHuffStream(&ctx.huff_table, comp_slice[sp .. sp + s2_len], literals.items[l1 .. l1 + l2])?;
            sp += s2_len;
            decodeHuffStream(&ctx.huff_table, comp_slice[sp .. sp + s3_len], literals.items[l1 + l2 .. l1 + l2 + l3])?;
            sp += s3_len;
            decodeHuffStream(&ctx.huff_table, comp_slice[sp .. sp + s4_len], literals.items[l1 + l2 + l3 .. regen_size])?;
        }
    }

    // 2. Sequences Section
    if (pos >= block.len) return ZstdError.TruncatedInput;
    const nb_seq_b0 = block[pos];
    pos += 1;
    let num_sequences: usize = 0;
    if (nb_seq_b0 == 0) {
        num_sequences = 0;
    } else if (nb_seq_b0 < 128) {
        num_sequences = nb_seq_b0 as usize;
    } else if (nb_seq_b0 < 255) {
        if (pos >= block.len) return ZstdError.TruncatedInput;
        num_sequences = (((nb_seq_b0 - 128) as usize) << 8) + (block[pos] as usize);
        pos += 1;
    } else {
        if (pos + 2 > block.len) return ZstdError.TruncatedInput;
        num_sequences = (readU16LE(block, pos) as usize) + 0x7F00;
        pos += 2;
    }

    if (num_sequences == 0) {
        if (out.items.len + literals.items.len > max_output_bytes) {
            return ZstdError.OutputTooLarge;
        }
        out.appendSlice(alloc, literals.items) catch return ZstdError.OutOfMemory;
        return;
    }

    if (pos >= block.len) return ZstdError.TruncatedInput;
    const modes_byte = block[pos];
    pos += 1;
    if ((modes_byte & 0x03) != 0) return ZstdError.CorruptedFrame;

    const ll_mode = (modes_byte >> 6) & 3;
    const of_mode = (modes_byte >> 4) & 3;
    const ml_mode = (modes_byte >> 2) & 3;

    selectSeqTable(alloc, ll_mode, block, &pos, &ctx.ll_table, &ctx.has_ll, LL_DEFAULT_DIST[0..], 6, 35, 9)?;
    selectSeqTable(alloc, of_mode, block, &pos, &ctx.of_table, &ctx.has_of, OF_DEFAULT_DIST[0..], 5, 31, 8)?;
    selectSeqTable(alloc, ml_mode, block, &pos, &ctx.ml_table, &ctx.has_ml, ML_DEFAULT_DIST[0..], 6, 52, 9)?;

    let br = initBackwardBitReader(block[pos..])?;
    let ll_state: usize = if (ctx.ll_table.is_rle) 0 else (readBitsBackward(&br, ctx.ll_table.accuracy_log as u6)?) as usize;
    let of_state: usize = if (ctx.of_table.is_rle) 0 else (readBitsBackward(&br, ctx.of_table.accuracy_log as u6)?) as usize;
    let ml_state: usize = if (ctx.ml_table.is_rle) 0 else (readBitsBackward(&br, ctx.ml_table.accuracy_log as u6)?) as usize;

    let lit_pos: usize = 0;
    let seq_i: usize = 0;
    while (seq_i < num_sequences) : (seq_i += 1) {
        const of_code = if (ctx.of_table.is_rle) ctx.of_table.rle_symbol else (ctx.of_table.entries orelse return ZstdError.CorruptedFrame)[of_state].symbol;
        const ml_code = if (ctx.ml_table.is_rle) ctx.ml_table.rle_symbol else (ctx.ml_table.entries orelse return ZstdError.CorruptedFrame)[ml_state].symbol;
        const ll_code = if (ctx.ll_table.is_rle) ctx.ll_table.rle_symbol else (ctx.ll_table.entries orelse return ZstdError.CorruptedFrame)[ll_state].symbol;

        if (ll_code > 35 or ml_code > 52 or of_code > 31) {
            return ZstdError.CorruptedFrame;
        }

        const of_bits = of_code as u6;
        const of_extra = readBitsBackward(&br, of_bits)?;
        const ml_extra = readBitsBackward(&br, ML_BITS[ml_code])?;
        const ll_extra = readBitsBackward(&br, LL_BITS[ll_code])?;

        if (seq_i + 1 < num_sequences) {
            if (!ctx.ll_table.is_rle) {
                const ll_entries = ctx.ll_table.entries orelse return ZstdError.CorruptedFrame;
                const e = ll_entries[ll_state];
                ll_state = (e.next_state as usize) + ((readBitsBackward(&br, e.nb_bits as u6)?) as usize);
            }
            if (!ctx.ml_table.is_rle) {
                const ml_entries = ctx.ml_table.entries orelse return ZstdError.CorruptedFrame;
                const e = ml_entries[ml_state];
                ml_state = (e.next_state as usize) + ((readBitsBackward(&br, e.nb_bits as u6)?) as usize);
            }
            if (!ctx.of_table.is_rle) {
                const of_entries = ctx.of_table.entries orelse return ZstdError.CorruptedFrame;
                const e = of_entries[of_state];
                of_state = (e.next_state as usize) + ((readBitsBackward(&br, e.nb_bits as u6)?) as usize);
            }
        }

        const lit_len = (LL_BASE[ll_code] as usize) + (ll_extra as usize);
        const match_len = (ML_BASE[ml_code] as usize) + (ml_extra as usize);
        const raw_offset = ((@as(u64, 1)) << of_bits) + of_extra;

        let offset: usize = 0;
        if (of_code > 1) {
            offset = raw_offset as usize;
            ctx.rep[2] = ctx.rep[1];
            ctx.rep[1] = ctx.rep[0];
            ctx.rep[0] = offset;
        } else {
            let idx = (raw_offset as usize) - 1;
            if (lit_len == 0) idx += 1;
            if (idx == 0) {
                offset = ctx.rep[0];
            } else if (idx < 3) {
                offset = ctx.rep[idx];
                if (idx == 1) {
                    ctx.rep[1] = ctx.rep[0];
                } else {
                    ctx.rep[2] = ctx.rep[1];
                    ctx.rep[1] = ctx.rep[0];
                }
                ctx.rep[0] = offset;
            } else {
                if (ctx.rep[0] <= 1) return ZstdError.CorruptedFrame;
                offset = ctx.rep[0] - 1;
                ctx.rep[2] = ctx.rep[1];
                ctx.rep[1] = ctx.rep[0];
                ctx.rep[0] = offset;
            }
        }

        if (lit_pos + lit_len > literals.items.len) return ZstdError.CorruptedFrame;
        if (out.items.len + lit_len + match_len > max_output_bytes) return ZstdError.OutputTooLarge;

        if (lit_len > 0) {
            out.appendSlice(alloc, literals.items[lit_pos .. lit_pos + lit_len]) catch return ZstdError.OutOfMemory;
            lit_pos += lit_len;
        }

        const avail = out.items.len - frame_start;
        if (offset == 0 or offset > avail) return ZstdError.CorruptedFrame;

        let m: usize = 0;
        while (m < match_len) : (m += 1) {
            const copy_byte = out.items[out.items.len - offset];
            appendU8(out, alloc, copy_byte)?;
        }
    }

    if (br.bit_pos != 0) {
        return ZstdError.CorruptedFrame;
    }

    // Trailing literals
    const rem_lits = literals.items.len - lit_pos;
    if (rem_lits > 0) {
        if (out.items.len + rem_lits > max_output_bytes) return ZstdError.OutputTooLarge;
        out.appendSlice(alloc, literals.items[lit_pos..]) catch return ZstdError.OutOfMemory;
    }
}

// ============================================================================
// Decompression Entry Points (Multi-Frame, Skippable Frames, Security Limits)
// ============================================================================

/// Decompresses one or more concatenated Zstandard frames with configurable window and output limits.
pub fn decompressWithLimits(
    allocator: std.mem.Allocator,
    bytes: []const u8,
    max_window_bytes: u64,
    max_output_bytes: usize,
): []u8 ! ZstdError {
    if (bytes.len == 0) {
        return ZstdError.TruncatedInput;
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(allocator);

    let pos: usize = 0;
    let decoded_any_zstd_frame = false;

    while (pos < bytes.len) {
        if (pos + 4 > bytes.len) {
            return ZstdError.TruncatedInput;
        }
        const magic = readU32LE(bytes, pos);
        if (isSkippableMagic(magic)) {
            if (pos + 8 > bytes.len) {
                return ZstdError.TruncatedInput;
            }
            const skip_size = readU32LE(bytes, pos + 4) as usize;
            if (pos + 8 + skip_size > bytes.len) {
                return ZstdError.TruncatedInput;
            }
            pos += 8 + skip_size;
            continue;
        }
        if (magic != ZSTD_MAGIC) {
            return ZstdError.InvalidMagic;
        }

        decoded_any_zstd_frame = true;
        const parsed = parseFrameHeaderAt(bytes, pos)?;
        const info = parsed.info;
        pos = parsed.header_end;

        if (info.window_size > max_window_bytes) {
            return ZstdError.WindowTooLarge;
        }
        if (fcs := info.frame_content_size) {
            if ((out.items.len as u64) + fcs > (max_output_bytes as u64)) {
                return ZstdError.OutputTooLarge;
            }
        }

        const frame_start = out.items.len;
        let ctx = initDecoderContext();
        defer freeDecoderContext(allocator, &ctx);

        let last_block = false;
        while (!last_block) {
            if (pos + 3 > bytes.len) {
                return ZstdError.TruncatedInput;
            }
            const bh = (bytes[pos] as u32) | ((bytes[pos + 1] as u32) << 8) | ((bytes[pos + 2] as u32) << 16);
            pos += 3;

            last_block = (bh & 1) != 0;
            const block_type = (bh >> 1) & 3;
            const block_size = (bh >> 3) as usize;

            if (block_type == 3) {
                return ZstdError.UnsupportedBlockType;
            }
            if (block_size > ZSTD_MAX_BLOCK_SIZE) {
                return ZstdError.CorruptedFrame;
            }

            if (block_type == 0) {
                // Raw_Block
                if (pos + block_size > bytes.len) {
                    return ZstdError.TruncatedInput;
                }
                if (out.items.len + block_size > max_output_bytes) {
                    return ZstdError.OutputTooLarge;
                }
                out.appendSlice(allocator, bytes[pos .. pos + block_size]) catch return ZstdError.OutOfMemory;
                pos += block_size;
            } else if (block_type == 1) {
                // RLE_Block
                if (pos + 1 > bytes.len) {
                    return ZstdError.TruncatedInput;
                }
                if (out.items.len + block_size > max_output_bytes) {
                    return ZstdError.OutputTooLarge;
                }
                const fill_byte = bytes[pos];
                pos += 1;
                out.appendNTimes(allocator, fill_byte, block_size) catch return ZstdError.OutOfMemory;
            } else {
                // Compressed_Block (type == 2)
                if (pos + block_size > bytes.len) {
                    return ZstdError.TruncatedInput;
                }
                decodeCompressedBlock(allocator, bytes[pos .. pos + block_size], &ctx, &out, frame_start, max_output_bytes)?;
                pos += block_size;
            }
        }

        const frame_decoded_len = out.items.len - frame_start;
        if (fcs := info.frame_content_size) {
            if ((frame_decoded_len as u64) != fcs) {
                return ZstdError.CorruptedFrame;
            }
        }

        if (info.has_checksum) {
            if (pos + 4 > bytes.len) {
                return ZstdError.TruncatedInput;
            }
            const expected_cksum = readU32LE(bytes, pos);
            pos += 4;
            const actual_cksum = (xxhash64(out.items[frame_start..], 0) & 0xFFFFFFFF) as u32;
            if (actual_cksum != expected_cksum) {
                return ZstdError.ChecksumMismatch;
            }
        }
    }

    if (!decoded_any_zstd_frame) {
        return ZstdError.InvalidMagic;
    }

    return out.toOwnedSlice(allocator) catch return ZstdError.OutOfMemory;
}

/// Decompresses Zstandard frames using default safety limits (8 MiB window, 64 MiB output).
pub fn decompress(allocator: std.mem.Allocator, bytes: []const u8): []u8 ! ZstdError {
    return decompressWithLimits(allocator, bytes, 8 * 1024 * 1024, 64 * 1024 * 1024);
}
