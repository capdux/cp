// Reference-hardened against PSeitz/lz4_flex (MIT)
// Source: cp/lib/pack/lz4.cp (Pure CP LZ4 Block & Frame v1.6.1 Compression + XXH32 + 16-Lane SIMD)
// Features:
//   - LZ4 Raw Block codec: `compress`, `decompress`, zero-heap `compressInto`/`decompressInto`,
//     prefix/external dictionary `compressWithDict`/`decompressWithDict`/`decompressWithDictInto`
//   - Complete LZ4 Frame v1.6.1: `compressFrame`, `compressFrameWithOptions`, `decompressFrame`,
//     Independent & Linked blocks, 64KB/256KB/1MB/4MB BD, ContentSize, DictId, Block/Content XXH32,
//     Uncompressed block fallback (`0x80000000`), and Skippable Frames (`0x184D2A50..0x184D2A5F`)
//   - Hot-path 16-lane `@Vector(16, u8)` SIMD acceleration: non-overlapping match copy (`offset >= 16`),
//     RLE single-byte fill (`offset == 1`), literal copy, and encoder match-length extension (0-byte diff vs scalar)

const std = @import("std");

/// Error set for LZ4 compression, decompression, and framing operations.
pub err Lz4Error {
    CorruptStream,
    InvalidMagic,
    InvalidHeaderChecksum,
    InvalidBlockChecksum,
    InvalidContentChecksum,
    ContentSizeMismatch,
    InvalidBlockMaxSize,
    OutputTooLarge,
    BufferTooSmall,
    InvalidOffset,
    TruncatedInput,
    OutOfMemory,
}

pub const LZ4_FRAME_MAGIC: u32 = 0x184D2204;
pub const LZ4_SKIPPABLE_MAGIC_MIN: u32 = 0x184D2A50;
pub const LZ4_SKIPPABLE_MAGIC_MAX: u32 = 0x184D2A5F;
pub const LZ4_FRAME_BLOCK_SIZE: usize = 65536;

/// Configuration options for `Lz4Frame.compressFrameWithOptions`.
pub struct Lz4FrameOptions {
    block_max_size: usize = 65536;
    block_independent: bool = true;
    block_checksum: bool = false;
    content_checksum: bool = true;
    content_size: bool = false;
    has_dict_id: bool = false;
    dict_id: u32 = 0;
}

const PRIME32_1: u32 = 0x9E3779B1;
const PRIME32_2: u32 = 0x85EBCA77;
const PRIME32_3: u32 = 0xC2B2AE3D;
const PRIME32_4: u32 = 0x27D4EB2F;
const PRIME32_5: u32 = 0x165667B1;

const MINMATCH: usize = 4;
const LASTLITERALS: usize = 5;
const MFLIMIT: usize = 12;
const MAX_DISTANCE: usize = 65535;
const HASH_SIZE: usize = 4096;

// ============================================================================
// 1. Low-Level Binary, XXH32 & 16-Lane SIMD Copy / Match-Extend Primitives
// ============================================================================

@inline
fn rotl32(x: u32, r: u5): u32 {
    return std.math.rotl(u32, x, r);
}

@inline
fn readLE32(data: []const u8, pos: usize): u32 {
    return (data[pos] as u32) |
        ((data[pos + 1] as u32) << 8) |
        ((data[pos + 2] as u32) << 16) |
        ((data[pos + 3] as u32) << 24);
}

@inline
fn readLE64(data: []const u8, pos: usize): u64 {
    const lo: u64 = readLE32(data, pos) as u64;
    const hi: u64 = readLE32(data, pos + 4) as u64;
    return lo | (hi << 32);
}

@inline
fn writeLE32(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32): void ! Lz4Error {
    const buf: [4]u8 = [
        (val & 0xFF) as u8,
        ((val >> 8) & 0xFF) as u8,
        ((val >> 16) & 0xFF) as u8,
        ((val >> 24) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, buf[0..]) catch return Lz4Error.OutOfMemory;
}

@inline
fn writeLE64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): void ! Lz4Error {
    writeLE32(out, alloc, (val & 0xFFFF_FFFF) as u32)?;
    writeLE32(out, alloc, ((val >> 32) & 0xFFFF_FFFF) as u32)?;
}

@inline
fn xxh32Round(acc: u32, input: u32): u32 {
    let v = acc +% (input *% PRIME32_2);
    v = rotl32(v, 13);
    return v *% PRIME32_1;
}

/// Computes the standard 32-bit xxHash (XXH32) of `data` with `seed`.
pub fn xxhash32(data: []const u8, seed: u32): u32 {
    let h32: u32 = 0;
    let p: usize = 0;

    if (data.len >= 16) {
        let v1: u32 = seed +% PRIME32_1 +% PRIME32_2;
        let v2: u32 = seed +% PRIME32_2;
        let v3: u32 = seed;
        let v4: u32 = seed -% PRIME32_1;

        while (p + 16 <= data.len) {
            v1 = xxh32Round(v1, readLE32(data, p));
            v2 = xxh32Round(v2, readLE32(data, p + 4));
            v3 = xxh32Round(v3, readLE32(data, p + 8));
            v4 = xxh32Round(v4, readLE32(data, p + 12));
            p += 16;
        }

        h32 = rotl32(v1, 1) +% rotl32(v2, 7) +% rotl32(v3, 12) +% rotl32(v4, 18);
    } else {
        h32 = seed +% PRIME32_5;
    }

    h32 = h32 +% ((data.len & 0xFFFFFFFF) as u32);

    while (p + 4 <= data.len) {
        h32 = h32 +% (readLE32(data, p) *% PRIME32_3);
        h32 = rotl32(h32, 17) *% PRIME32_4;
        p += 4;
    }

    while (p < data.len) {
        h32 = h32 +% ((data[p] as u32) *% PRIME32_5);
        h32 = rotl32(h32, 11) *% PRIME32_1;
        p += 1;
    }

    h32 ^= h32 >> 15;
    h32 = h32 *% PRIME32_2;
    h32 ^= h32 >> 13;
    h32 = h32 *% PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
}

/// Scalar reference non-overlapping byte copy.
pub fn copyNonOverlappingScalar(dst: *[]u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

/// 16-lane `@Vector(16, u8)` SIMD non-overlapping byte copy (0-byte diff vs scalar).
pub fn copyNonOverlappingSimd(dst: *[]u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i + 16 <= src.len) {
        const v: @Vector(16, u8) = src[i..i + 16][0..16].*;
        dst[dst_off + i..dst_off + i + 16][0..16].* = v;
        i += 16;
    }
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

fn fillByteSimd(dst: *[]u8, dst_off: usize, count: usize, val: u8) {
    const vfill: @Vector(16, u8) = @splat(val);
    let i: usize = 0;
    while (i + 16 <= count) {
        dst[dst_off + i..dst_off + i + 16][0..16].* = vfill;
        i += 16;
    }
    while (i < count) : (i += 1) {
        dst[dst_off + i] = val;
    }
}

/// Scalar reference LZ77 match copy (handles arbitrary overlapping offsets `>= 1`).
pub fn copyMatchScalar(out: *[]u8, out_pos: usize, offset: usize, match_len: usize) {
    const src_pos: usize = out_pos - offset;
    let i: usize = 0;
    while (i < match_len) : (i += 1) {
        out[out_pos + i] = out[src_pos + i];
    }
}

/// 16-lane `@Vector(16, u8)` SIMD LZ77 match copy:
///   - `offset == 1`: 16-byte `@splat` RLE fill
///   - `offset >= 16`: 16-byte non-overlapping vector block copy
///   - `2 <= offset < 16`: exact overlapping byte copy (0-byte diff vs scalar)
pub fn copyMatchSimd(out: *[]u8, out_pos: usize, offset: usize, match_len: usize) {
    if (match_len == 0) return;
    if (offset == 1) {
        fillByteSimd(out, out_pos, match_len, out[out_pos - 1]);
        return;
    }
    const src_pos: usize = out_pos - offset;
    if (offset >= 16) {
        let i: usize = 0;
        while (i + 16 <= match_len) {
            const v: @Vector(16, u8) = out[src_pos + i..src_pos + i + 16][0..16].*;
            out[out_pos + i..out_pos + i + 16][0..16].* = v;
            i += 16;
        }
        while (i < match_len) : (i += 1) {
            out[out_pos + i] = out[src_pos + i];
        }
        return;
    }
    let i: usize = 0;
    while (i < match_len) : (i += 1) {
        out[out_pos + i] = out[src_pos + i];
    }
}

/// Scalar reference match-length extension comparing `src[a_off..]` and `src[b_off..limit]`.
pub fn extendMatchLenScalar(src: []const u8, a_off: usize, b_off: usize, limit: usize): usize {
    let len: usize = 0;
    while (b_off + len < limit && src[a_off + len] == src[b_off + len]) : (len += 1) {}
    return len;
}

/// 16-lane `@Vector(16, u8)` SIMD match-length extension (0-byte diff vs scalar).
pub fn extendMatchLenSimd(src: []const u8, a_off: usize, b_off: usize, limit: usize): usize {
    let len: usize = 0;
    while (b_off + len + 16 <= limit) {
        const va: @Vector(16, u8) = src[a_off + len..a_off + len + 16][0..16].*;
        const vb: @Vector(16, u8) = src[b_off + len..b_off + len + 16][0..16].*;
        const diff: @Vector(16, u8) = va ^ vb;
        if (@reduce(.Or, diff) != 0) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                if (src[a_off + len + j] != src[b_off + len + j]) return len + j;
            }
        }
        len += 16;
    }
    while (b_off + len < limit && src[a_off + len] == src[b_off + len]) : (len += 1) {}
    return len;
}

@inline
fn hash4(data: []const u8, pos: usize): usize {
    const seq = readLE32(data, pos);
    return (((seq *% 2654435761) >> 20) & 0xFFF) as usize;
}

fn writeExtLenSlice(dst: *[]u8, pos_in: usize, len_val: usize): usize ! Lz4Error {
    let pos: usize = pos_in;
    let rem: usize = len_val;
    while (rem >= 255) {
        if (pos >= dst.len) return Lz4Error.BufferTooSmall;
        dst[pos] = 255;
        pos += 1;
        rem -= 255;
    }
    if (pos >= dst.len) return Lz4Error.BufferTooSmall;
    dst[pos] = rem as u8;
    return pos + 1;
}

fn writeExtLen(out: *std.ArrayList(u8), alloc: std.mem.Allocator, len_val: usize): void ! Lz4Error {
    let rem = len_val;
    while (rem >= 255) {
        out.append(alloc, 255) catch return Lz4Error.OutOfMemory;
        rem -= 255;
    }
    out.append(alloc, rem as u8) catch return Lz4Error.OutOfMemory;
}

// ============================================================================
// 2. LZ4 Raw Block Codec (`Lz4Block`) — Zero-Heap, Dict & Hardened Bounds
// ============================================================================

/// Standard LZ4 raw block format compressor and decompressor.
pub static class Lz4Block {
    /// Conservative upper bound on LZ4 block compressed size for input of `src_len` bytes.
    pub fn maxCompressedSize(src_len: usize): usize {
        return src_len + (src_len / 255) + 16;
    }

    /// Zero-heap LZ4 raw block compressor writing into caller-owned `dst`.
    /// Uses 16-lane SIMD match-length extension and literal copy.
    pub fn compressInto(src: []const u8, dst: *[]u8): usize ! Lz4Error {
        if (src.len == 0) return 0;

        let op: usize = 0;
        if (src.len < MFLIMIT + 1) {
            const lit_len = src.len;
            const nibble: u8 = if (lit_len >= 15) 15 else (lit_len as u8);
            if (op >= dst.len) return Lz4Error.BufferTooSmall;
            dst[op] = nibble << 4;
            op += 1;
            if (lit_len >= 15) {
                op = writeExtLenSlice(dst, op, lit_len - 15)?;
            }
            if (dst.len - op < lit_len) return Lz4Error.BufferTooSmall;
            copyNonOverlappingSimd(dst, op, src);
            return op + lit_len;
        }

        let table: [HASH_SIZE]u32 = [0; HASH_SIZE];
        table[hash4(src, 0)] = 1;

        let anchor: usize = 0;
        let ip: usize = 1;
        const mf_limit: usize = src.len - MFLIMIT;
        const match_limit: usize = src.len - LASTLITERALS;

        while (ip <= mf_limit) {
            const h = hash4(src, ip);
            const ref_plus1 = table[h];
            table[h] = (ip + 1) as u32;

            if (ref_plus1 != 0) {
                const ref_pos: usize = (ref_plus1 - 1) as usize;
                const dist: usize = ip - ref_pos;
                if (dist > 0 && dist <= MAX_DISTANCE && readLE32(src, ref_pos) == readLE32(src, ip)) {
                    const lit_len: usize = ip - anchor;
                    if (op >= dst.len) return Lz4Error.BufferTooSmall;
                    const token_idx: usize = op;
                    op += 1;

                    const lit_nibble: u8 = if (lit_len >= 15) 15 else (lit_len as u8);
                    if (lit_len >= 15) {
                        op = writeExtLenSlice(dst, op, lit_len - 15)?;
                    }
                    if (dst.len - op < lit_len) return Lz4Error.BufferTooSmall;
                    copyNonOverlappingSimd(dst, op, src[anchor..ip]);
                    op += lit_len;

                    if (dst.len - op < 2) return Lz4Error.BufferTooSmall;
                    dst[op] = (dist & 0xFF) as u8;
                    dst[op + 1] = ((dist >> 8) & 0xFF) as u8;
                    op += 2;

                    const extra: usize = extendMatchLenSimd(src, ref_pos + MINMATCH, ip + MINMATCH, match_limit);
                    const match_len: usize = MINMATCH + extra;
                    const ml_base: usize = match_len - MINMATCH;
                    const ml_nibble: u8 = if (ml_base >= 15) 15 else (ml_base as u8);
                    dst[token_idx] = (lit_nibble << 4) | ml_nibble;

                    if (ml_base >= 15) {
                        op = writeExtLenSlice(dst, op, ml_base - 15)?;
                    }

                    ip = ip + match_len;
                    anchor = ip;
                    continue;
                }
            }
            ip += 1;
        }

        // Trailing literals (at least LASTLITERALS = 5 bytes when matches occurred)
        const last_lit_len: usize = src.len - anchor;
        const last_nibble: u8 = if (last_lit_len >= 15) 15 else (last_lit_len as u8);
        if (op >= dst.len) return Lz4Error.BufferTooSmall;
        dst[op] = last_nibble << 4;
        op += 1;
        if (last_lit_len >= 15) {
            op = writeExtLenSlice(dst, op, last_lit_len - 15)?;
        }
        if (dst.len - op < last_lit_len) return Lz4Error.BufferTooSmall;
        copyNonOverlappingSimd(dst, op, src[anchor..src.len]);
        return op + last_lit_len;
    }

    /// Compresses `src` into standard LZ4 raw block format.
    pub fn compress(alloc: std.mem.Allocator, src: []const u8): []u8 ! Lz4Error {
        if (src.len == 0) {
            return alloc.alloc(u8, 0) catch return Lz4Error.OutOfMemory;
        }
        const bound: usize = Lz4Block.maxCompressedSize(src.len);
        let buf = alloc.alloc(u8, bound) catch return Lz4Error.OutOfMemory;
        const written: usize = Lz4Block.compressInto(src, buf) catch |e| {
            alloc.free(buf);
            return e;
        };
        let exact = alloc.alloc(u8, written) catch {
            alloc.free(buf);
            return Lz4Error.OutOfMemory;
        };
        copyNonOverlappingSimd(exact, 0, buf[0..written]);
        alloc.free(buf);
        return exact;
    }

    /// Compresses `src` using up to the last 64KB of `dict` as a prefix history dictionary.
    pub fn compressWithDict(alloc: std.mem.Allocator, src: []const u8, dict: []const u8): []u8 ! Lz4Error {
        if (src.len == 0) return alloc.alloc(u8, 0) catch return Lz4Error.OutOfMemory;
        if (dict.len == 0) return Lz4Block.compress(alloc, src);

        const dict_tail: []const u8 = if (dict.len > MAX_DISTANCE) dict[dict.len - MAX_DISTANCE..dict.len] else dict;
        const combined_len: usize = dict_tail.len + src.len;
        let combined = alloc.alloc(u8, combined_len) catch return Lz4Error.OutOfMemory;
        defer alloc.free(combined);
        copyNonOverlappingSimd(combined, 0, dict_tail);
        copyNonOverlappingSimd(combined, dict_tail.len, src);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (src.len < MFLIMIT + 1) {
            const lit_len = src.len;
            const nibble: u8 = if (lit_len >= 15) 15 else (lit_len as u8);
            out.append(alloc, nibble << 4) catch return Lz4Error.OutOfMemory;
            if (lit_len >= 15) writeExtLen(*out, alloc, lit_len - 15)?;
            out.appendSlice(alloc, src) catch return Lz4Error.OutOfMemory;
            return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
        }

        let table: [HASH_SIZE]u32 = [0; HASH_SIZE];
        // Pre-populate hash table with dictionary positions
        if (dict_tail.len >= MINMATCH) {
            let dp: usize = 0;
            const d_lim: usize = dict_tail.len - MINMATCH;
            while (dp <= d_lim) : (dp += 1) {
                table[hash4(combined, dp)] = (dp + 1) as u32;
            }
        }

        let anchor: usize = dict_tail.len;
        let ip: usize = dict_tail.len;
        const mf_limit: usize = combined_len - MFLIMIT;
        const match_limit: usize = combined_len - LASTLITERALS;

        while (ip <= mf_limit) {
            const h = hash4(combined, ip);
            const ref_plus1 = table[h];
            table[h] = (ip + 1) as u32;

            if (ref_plus1 != 0) {
                const ref_pos: usize = (ref_plus1 - 1) as usize;
                const dist: usize = ip - ref_pos;
                if (dist > 0 && dist <= MAX_DISTANCE && readLE32(combined, ref_pos) == readLE32(combined, ip)) {
                    const lit_len: usize = ip - anchor;
                    const token_idx: usize = out.items.len;
                    out.append(alloc, 0) catch return Lz4Error.OutOfMemory;
                    const lit_nibble: u8 = if (lit_len >= 15) 15 else (lit_len as u8);
                    if (lit_len >= 15) writeExtLen(*out, alloc, lit_len - 15)?;
                    out.appendSlice(alloc, combined[anchor..ip]) catch return Lz4Error.OutOfMemory;
                    out.append(alloc, (dist & 0xFF) as u8) catch return Lz4Error.OutOfMemory;
                    out.append(alloc, ((dist >> 8) & 0xFF) as u8) catch return Lz4Error.OutOfMemory;

                    const extra: usize = extendMatchLenSimd(combined, ref_pos + MINMATCH, ip + MINMATCH, match_limit);
                    const match_len: usize = MINMATCH + extra;
                    const ml_base: usize = match_len - MINMATCH;
                    const ml_nibble: u8 = if (ml_base >= 15) 15 else (ml_base as u8);
                    out.items[token_idx] = (lit_nibble << 4) | ml_nibble;
                    if (ml_base >= 15) writeExtLen(*out, alloc, ml_base - 15)?;

                    ip = ip + match_len;
                    anchor = ip;
                    continue;
                }
            }
            ip += 1;
        }

        const last_lit_len: usize = combined_len - anchor;
        const last_nibble: u8 = if (last_lit_len >= 15) 15 else (last_lit_len as u8);
        out.append(alloc, last_nibble << 4) catch return Lz4Error.OutOfMemory;
        if (last_lit_len >= 15) writeExtLen(*out, alloc, last_lit_len - 15)?;
        out.appendSlice(alloc, combined[anchor..combined_len]) catch return Lz4Error.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }

    /// Zero-heap LZ4 raw block decompressor with optional prefix dictionary (`dict`) and 16-lane SIMD copy.
    /// Strict overflow-safe length accumulation (255-byte flood safe) and offset/bounds validation.
    pub fn decompressWithDictInto(
        compressed: []const u8,
        dict: []const u8,
        dst: *[]u8,
    ): usize ! Lz4Error {
        if (compressed.len == 0) return 0;

        const max_out: usize = dst.len;
        let ip: usize = 0;
        let op: usize = 0;

        while (ip < compressed.len) {
            const token: u8 = compressed[ip];
            ip += 1;

            let lit_len: usize = (token >> 4) as usize;
            if (lit_len == 15) {
                while (true) {
                    if (ip >= compressed.len) return Lz4Error.TruncatedInput;
                    const b: usize = compressed[ip] as usize;
                    ip += 1;
                    if (b > max_out || lit_len > max_out - b) return Lz4Error.OutputTooLarge;
                    lit_len += b;
                    if (b != 255) break;
                }
            }

            if (compressed.len - ip < lit_len) return Lz4Error.TruncatedInput;
            if (max_out - op < lit_len) return Lz4Error.OutputTooLarge;

            copyNonOverlappingSimd(dst, op, compressed[ip..ip + lit_len]);
            ip += lit_len;
            op += lit_len;

            if (ip == compressed.len) {
                if ((token & 0x0F) != 0) return Lz4Error.CorruptStream;
                break;
            }

            if (compressed.len - ip < 2) return Lz4Error.TruncatedInput;
            const offset: usize = (compressed[ip] as usize) | ((compressed[ip + 1] as usize) << 8);
            ip += 2;

            if (offset == 0) return Lz4Error.InvalidOffset;
            if (offset > op + dict.len) return Lz4Error.InvalidOffset;

            let match_len: usize = ((token & 0x0F) as usize) + MINMATCH;
            if ((token & 0x0F) == 15) {
                while (true) {
                    if (ip >= compressed.len) return Lz4Error.TruncatedInput;
                    const b: usize = compressed[ip] as usize;
                    ip += 1;
                    if (b > max_out || match_len > max_out - b) return Lz4Error.OutputTooLarge;
                    match_len += b;
                    if (b != 255) break;
                }
            }

            if (max_out - op < match_len) return Lz4Error.OutputTooLarge;

            if (offset <= op) {
                // Entire match resides inside `dst`: use 16-lane SIMD match/RLE copy
                copyMatchSimd(dst, op, offset, match_len);
                op += match_len;
            } else {
                // Match begins inside external prefix `dict` and may cross into `dst`
                const from_dict_dist: usize = offset - op;
                const dict_start: usize = dict.len - from_dict_dist;
                const dict_avail: usize = from_dict_dist;
                const take_dict: usize = if (match_len < dict_avail) match_len else dict_avail;
                copyNonOverlappingSimd(dst, op, dict[dict_start..dict_start + take_dict]);
                op += take_dict;
                const rem_match: usize = match_len - take_dict;
                if (rem_match > 0) {
                    copyMatchSimd(dst, op, offset, rem_match);
                    op += rem_match;
                }
            }
        }
        return op;
    }

    /// Zero-heap LZ4 raw block decompressor writing into caller-owned `dst`.
    pub fn decompressInto(compressed: []const u8, dst: *[]u8): usize ! Lz4Error {
        return Lz4Block.decompressWithDictInto(compressed, "", dst);
    }

    /// Decompresses a standard LZ4 raw block, enforcing `max_output_size` and strict offset/bounds checks.
    pub fn decompress(alloc: std.mem.Allocator, compressed: []const u8, max_output_size: usize): []u8 ! Lz4Error {
        return Lz4Block.decompressWithDict(alloc, compressed, "", max_output_size);
    }

    /// Decompresses an LZ4 raw block with optional external dictionary `dict` up to `max_output_size`.
    pub fn decompressWithDict(
        alloc: std.mem.Allocator,
        compressed: []const u8,
        dict: []const u8,
        max_output_size: usize,
    ): []u8 ! Lz4Error {
        if (compressed.len == 0) {
            return alloc.alloc(u8, 0) catch return Lz4Error.OutOfMemory;
        }
        let scratch = alloc.alloc(u8, max_output_size) catch return Lz4Error.OutOfMemory;
        const written: usize = Lz4Block.decompressWithDictInto(compressed, dict, scratch) catch |e| {
            alloc.free(scratch);
            return e;
        };
        let exact = alloc.alloc(u8, written) catch {
            alloc.free(scratch);
            return Lz4Error.OutOfMemory;
        };
        copyNonOverlappingSimd(exact, 0, scratch[0..written]);
        alloc.free(scratch);
        return exact;
    }
}

// ============================================================================
// 3. Complete LZ4 Frame Format v1.6.1 (`Lz4Frame`)
// ============================================================================

fn blockMaxSizeToBd(sz: usize): u8 ! Lz4Error {
    if (sz == 65536) return 0x40;
    if (sz == 262144) return 0x50;
    if (sz == 1048576) return 0x60;
    if (sz == 4194304) return 0x70;
    return Lz4Error.InvalidBlockMaxSize;
}

fn bdToBlockMaxSize(bd: u8): usize ! Lz4Error {
    const id: u8 = (bd >> 4) & 0x07;
    if (id == 4) return 65536;
    if (id == 5) return 262144;
    if (id == 6) return 1048576;
    if (id == 7) return 4194304;
    return Lz4Error.InvalidBlockMaxSize;
}

/// Standard LZ4 Frame Format v1.6.1 (`0x184D2204`) compressor and decompressor with XXH32 checksums,
/// Independent/Linked blocks, ContentSize, DictId, Block Checksum, and Skippable Frames.
pub static class Lz4Frame {
    /// Encodes a standard LZ4 Skippable Frame (`0x184D2A50..0x184D2A5F`).
    pub fn encodeSkippableFrame(
        alloc: std.mem.Allocator,
        nibble: u8,
        payload: []const u8,
    ): []u8 ! Lz4Error {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        const magic: u32 = LZ4_SKIPPABLE_MAGIC_MIN | ((nibble & 0x0F) as u32);
        writeLE32(*out, alloc, magic)?;
        writeLE32(*out, alloc, payload.len as u32)?;
        out.appendSlice(alloc, payload) catch return Lz4Error.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }

    /// Compresses `src` into a standard LZ4 Frame with full Frame v1.6.1 `Lz4FrameOptions`.
    pub fn compressFrameWithOptions(
        alloc: std.mem.Allocator,
        src: []const u8,
        opts: Lz4FrameOptions,
    ): []u8 ! Lz4Error {
        const bd: u8 = blockMaxSizeToBd(opts.block_max_size)?;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        writeLE32(*out, alloc, LZ4_FRAME_MAGIC)?;

        let flg: u8 = 0x40; // Version = 01
        if (opts.block_independent) flg |= 0x20;
        if (opts.block_checksum) flg |= 0x10;
        if (opts.content_size) flg |= 0x08;
        if (opts.content_checksum) flg |= 0x04;
        if (opts.has_dict_id) flg |= 0x01;

        const desc_start: usize = out.items.len;
        out.append(alloc, flg) catch return Lz4Error.OutOfMemory;
        out.append(alloc, bd) catch return Lz4Error.OutOfMemory;
        if (opts.content_size) {
            writeLE64(*out, alloc, src.len as u64)?;
        }
        if (opts.has_dict_id) {
            writeLE32(*out, alloc, opts.dict_id)?;
        }
        const hc: u8 = ((xxhash32(out.items[desc_start..out.items.len], 0) >> 8) & 0xFF) as u8;
        out.append(alloc, hc) catch return Lz4Error.OutOfMemory;

        let pos: usize = 0;
        while (pos < src.len) {
            const rem: usize = src.len - pos;
            const chunk_len: usize = if (rem > opts.block_max_size) opts.block_max_size else rem;
            const chunk: []const u8 = src[pos..pos + chunk_len];

            const comp = if (opts.block_independent || pos == 0)
                Lz4Block.compress(alloc, chunk)?
            else
                Lz4Block.compressWithDict(alloc, chunk, src[0..pos])?;
            defer alloc.free(comp);

            if (comp.len < chunk.len) {
                writeLE32(*out, alloc, comp.len as u32)?;
                out.appendSlice(alloc, comp) catch return Lz4Error.OutOfMemory;
                if (opts.block_checksum) {
                    writeLE32(*out, alloc, xxhash32(comp, 0))?;
                }
            } else {
                writeLE32(*out, alloc, (chunk.len as u32) | 0x80000000)?;
                out.appendSlice(alloc, chunk) catch return Lz4Error.OutOfMemory;
                if (opts.block_checksum) {
                    writeLE32(*out, alloc, xxhash32(chunk, 0))?;
                }
            }
            pos += chunk_len;
        }

        // EndMark (0x00000000)
        writeLE32(*out, alloc, 0)?;

        if (opts.content_checksum) {
            writeLE32(*out, alloc, xxhash32(src, 0))?;
        }
        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }

    /// Compresses `src` into a standard LZ4 Frame, splitting into 64KB blocks with optional XXH32 content checksum.
    pub fn compressFrame(alloc: std.mem.Allocator, src: []const u8, with_content_checksum: bool): []u8 ! Lz4Error {
        return Lz4Frame.compressFrameWithOptions(alloc, src, Lz4FrameOptions{
            block_max_size: LZ4_FRAME_BLOCK_SIZE,
            block_independent: true,
            block_checksum: false,
            content_checksum: with_content_checksum,
            content_size: false,
            has_dict_id: false,
            dict_id: 0,
        });
    }

    /// Decompresses a standard LZ4 Frame v1.6.1 (including Skippable Frames, Independent & Linked blocks,
    /// Block/Content XXH32 checksums, and ContentSize verification).
    pub fn decompressFrame(alloc: std.mem.Allocator, frame: []const u8, max_output_size: usize): []u8 ! Lz4Error {
        let fpos: usize = 0;

        // Skip any leading Skippable Frames (0x184D2A50..0x184D2A5F)
        while (fpos < frame.len) {
            if (frame.len - fpos < 4) return Lz4Error.TruncatedInput;
            const magic = readLE32(frame, fpos);
            if (magic >= LZ4_SKIPPABLE_MAGIC_MIN && magic <= LZ4_SKIPPABLE_MAGIC_MAX) {
                if (frame.len - fpos < 8) return Lz4Error.TruncatedInput;
                const skip_len: usize = readLE32(frame, fpos + 4) as usize;
                fpos += 8;
                if (frame.len - fpos < skip_len) return Lz4Error.TruncatedInput;
                fpos += skip_len;
                continue;
            }
            break;
        }

        if (fpos >= frame.len || frame.len - fpos < 7) return Lz4Error.TruncatedInput;

        const magic = readLE32(frame, fpos);
        if (magic != LZ4_FRAME_MAGIC) return Lz4Error.InvalidMagic;

        const flg = frame[fpos + 4];
        const bd = frame[fpos + 5];

        // Version bits (7..6) must be 01, reserved FLG bit 1 must be 0, reserved BD bits (7, 3..0) must be 0
        if ((flg & 0xC0) != 0x40 || (flg & 0x02) != 0 || (bd & 0x8F) != 0) {
            return Lz4Error.CorruptStream;
        }
        const block_max_size: usize = bdToBlockMaxSize(bd)?;

        const block_indep = (flg & 0x20) != 0;
        const has_block_checksum = (flg & 0x10) != 0;
        const has_content_size = (flg & 0x08) != 0;
        const has_content_checksum = (flg & 0x04) != 0;
        const has_dict_id = (flg & 0x01) != 0;

        let desc_end: usize = fpos + 6;
        let expected_content_size: u64 = 0;
        if (has_content_size) {
            if (frame.len - desc_end < 8) return Lz4Error.TruncatedInput;
            expected_content_size = readLE64(frame, desc_end);
            if (expected_content_size > (max_output_size as u64)) return Lz4Error.OutputTooLarge;
            desc_end += 8;
        }
        if (has_dict_id) {
            if (frame.len - desc_end < 4) return Lz4Error.TruncatedInput;
            desc_end += 4;
        }

        if (frame.len <= desc_end) return Lz4Error.TruncatedInput;

        const expected_hc: u8 = ((xxhash32(frame[fpos + 4..desc_end], 0) >> 8) & 0xFF) as u8;
        if (frame[desc_end] != expected_hc) return Lz4Error.InvalidHeaderChecksum;

        let pos: usize = desc_end + 1;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        while (true) {
            if (frame.len - pos < 4) return Lz4Error.TruncatedInput;
            const raw_size = readLE32(frame, pos);
            pos += 4;

            if (raw_size == 0) break;

            const is_uncompressed = (raw_size & 0x80000000) != 0;
            const block_len = (raw_size & 0x7FFFFFFF) as usize;
            if (block_len > block_max_size) return Lz4Error.CorruptStream;
            if (frame.len - pos < block_len) return Lz4Error.TruncatedInput;

            const block_data = frame[pos..pos + block_len];
            pos += block_len;

            if (has_block_checksum) {
                if (frame.len - pos < 4) return Lz4Error.TruncatedInput;
                const expected_blk_cksum = readLE32(frame, pos);
                pos += 4;
                if (xxhash32(block_data, 0) != expected_blk_cksum) {
                    return Lz4Error.InvalidBlockChecksum;
                }
            }

            if (is_uncompressed) {
                if (max_output_size - out.items.len < block_len) return Lz4Error.OutputTooLarge;
                out.appendSlice(alloc, block_data) catch return Lz4Error.OutOfMemory;
            } else {
                const rem_max: usize = max_output_size - out.items.len;
                const blk_limit: usize = if (rem_max < block_max_size) rem_max else block_max_size;
                const dict_slice: []const u8 = if (block_indep || out.items.len == 0)
                    ""
                else if (out.items.len > MAX_DISTANCE)
                    out.items[out.items.len - MAX_DISTANCE..out.items.len]
                else
                    out.items;
                const decoded = Lz4Block.decompressWithDict(alloc, block_data, dict_slice, blk_limit)?;
                defer alloc.free(decoded);
                out.appendSlice(alloc, decoded) catch return Lz4Error.OutOfMemory;
            }
        }

        if (has_content_checksum) {
            if (frame.len - pos < 4) return Lz4Error.TruncatedInput;
            const expected_c_cksum = readLE32(frame, pos);
            const actual_c_cksum = xxhash32(out.items, 0);
            if (actual_c_cksum != expected_c_cksum) return Lz4Error.InvalidContentChecksum;
        }

        if (has_content_size && (out.items.len as u64) != expected_content_size) {
            return Lz4Error.ContentSizeMismatch;
        }

        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }
}
