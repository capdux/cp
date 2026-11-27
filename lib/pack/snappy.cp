// Reference-hardened against BurntSushi/rust-snappy (BSD-3-Clause)
// Source: cp/lib/pack/snappy.cp (Pure CP Snappy Raw Block + Framing Format + Masked CRC32C + 16-Lane SIMD)
// Features:
//   - Snappy Raw Block codec: `compress`, `decompress`, `decompressWithMaxSize`, `uncompressedLength`,
//     zero-heap `compressInto` / `decompressInto` / `maxCompressLen` / `decompressLen`
//   - Complete Snappy Framing Format (`sNaPpY`): `compressFramed`, `decompressFramed`, `decompressFramedWithMaxSize`,
//     Stream Identifier (0xFF), Compressed (0x00), Uncompressed (0x01), Padding (0xFE),
//     Reserved Unskippable (0x02..0x7F rejected) & Skippable (0x80..0xFD skipped), Masked CRC32C
//   - Hot-path 16-lane `@Vector(16, u8)` SIMD acceleration: literal copy, non-overlapping match copy (`offset >= 16`),
//     RLE single-byte fill (`offset == 1`), and encoder match-length extension (0-byte diff vs scalar)

const std = @import("std");

/// Error set for Snappy raw block and framing compression/decompression.
pub err SnappyError {
    InvalidVarint,
    CorruptedData,
    TruncatedInput,
    InvalidOffset,
    OutputTooLarge,
    BufferTooSmall,
    InvalidStreamIdentifier,
    ChecksumMismatch,
    UnsupportedChunkType,
    ChunkTooLarge,
    OutOfMemory,
}

pub const SNAPPY_FRAME_CHUNK_SIZE: usize = 65536;
pub const SNAPPY_STREAM_IDENTIFIER: [10]u8 = [0xFF, 0x06, 0x00, 0x00, 's', 'N', 'a', 'P', 'p', 'Y'];

const BLOCK_SIZE: usize = 65536;
const HASH_TABLE_SIZE: usize = 16384;
const HASH_MULTIPLIER: u32 = 0x1e35a7bd;
const CRC32C_MASK_DELTA: u32 = 0xa282ead8;

// ============================================================================
// 1. Low-Level Binary, CRC32C (Castagnoli) Masked Checksum & 16-Lane SIMD
// ============================================================================

fn readU24LE(buf: []const u8, pos: usize): u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16);
}

fn readU32LE(buf: []const u8, pos: usize): u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn writeU24LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void ! SnappyError {
    const b: [3]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, b[0..]) catch return SnappyError.OutOfMemory;
}

fn writeU32LE(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void ! SnappyError {
    const b: [4]u8 = [
        (v & 0xFF) as u8,
        ((v >> 8) & 0xFF) as u8,
        ((v >> 16) & 0xFF) as u8,
        ((v >> 24) & 0xFF) as u8,
    ];
    out.appendSlice(alloc, b[0..]) catch return SnappyError.OutOfMemory;
}

/// Computes Castagnoli CRC32C (polynomial `0x82F63B78`, nibble-accelerated).
pub fn snappyCrc32c(data: []const u8): u32 {
    const lut: [16]u32 = [
        0x00000000, 0x105EC76F, 0x20BD8EDE, 0x30E349B1,
        0x417B1DBC, 0x5125DAD3, 0x61C69362, 0x7198540D,
        0x82F63B78, 0x92A8FC17, 0xA24BB5A6, 0xB21572C9,
        0xC38D26C4, 0xD3D3E1AB, 0xE330A81A, 0xF36E6F75,
    ];
    let crc: u32 = 0xFFFF_FFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const b: u32 = data[i] as u32;
        crc = (crc >> 4) ^ lut[((crc ^ b) & 0x0F) as usize];
        crc = (crc >> 4) ^ lut[((crc ^ (b >> 4)) & 0x0F) as usize];
    }
    return ~crc;
}

/// Applies Snappy Framing checksum masking: `((crc >> 15) | (crc << 17)) +% 0xa282ead8`.
pub fn snappyMaskChecksum(crc: u32): u32 {
    return ((crc >> 15) | (crc << 17)) +% CRC32C_MASK_DELTA;
}

/// Reverses Snappy Framing checksum masking.
pub fn snappyUnmaskChecksum(masked: u32): u32 {
    const rot: u32 = masked -% CRC32C_MASK_DELTA;
    return (rot >> 17) | (rot << 15);
}

/// Scalar reference non-overlapping literal copy.
pub fn snappyCopyLitScalar(dst: *[]u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

/// 16-lane `@Vector(16, u8)` SIMD non-overlapping literal copy (0-byte diff vs scalar).
pub fn snappyCopyLitSimd(dst: *[]u8, dst_off: usize, src: []const u8) {
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

/// Scalar reference Snappy match copy (supports arbitrary overlapping offsets `>= 1`).
pub fn snappyCopyMatchScalar(out: *[]u8, out_pos: usize, offset: usize, copy_len: usize) {
    const src_pos: usize = out_pos - offset;
    let i: usize = 0;
    while (i < copy_len) : (i += 1) {
        out[out_pos + i] = out[src_pos + i];
    }
}

/// 16-lane `@Vector(16, u8)` SIMD Snappy match copy:
///   - `offset == 1`: `@splat` 16-byte RLE fill
///   - `offset >= 16`: 16-byte non-overlapping vector block copy
///   - `2 <= offset < 16`: exact overlapping byte copy (0-byte diff vs scalar)
pub fn snappyCopyMatchSimd(out: *[]u8, out_pos: usize, offset: usize, copy_len: usize) {
    if (copy_len == 0) return;
    if (offset == 1) {
        fillByteSimd(out, out_pos, copy_len, out[out_pos - 1]);
        return;
    }
    const src_pos: usize = out_pos - offset;
    if (offset >= 16) {
        let i: usize = 0;
        while (i + 16 <= copy_len) {
            const v: @Vector(16, u8) = out[src_pos + i..src_pos + i + 16][0..16].*;
            out[out_pos + i..out_pos + i + 16][0..16].* = v;
            i += 16;
        }
        while (i < copy_len) : (i += 1) {
            out[out_pos + i] = out[src_pos + i];
        }
        return;
    }
    let i: usize = 0;
    while (i < copy_len) : (i += 1) {
        out[out_pos + i] = out[src_pos + i];
    }
}

/// Scalar reference match-length extension comparing `src[a_off..]` and `src[b_off..limit]`.
pub fn snappyExtendMatchScalar(src: []const u8, a_off: usize, b_off: usize, limit: usize): usize {
    let len: usize = 0;
    while (b_off + len < limit && src[a_off + len] == src[b_off + len]) : (len += 1) {}
    return len;
}

/// 16-lane `@Vector(16, u8)` SIMD match-length extension (0-byte diff vs scalar).
pub fn snappyExtendMatchSimd(src: []const u8, a_off: usize, b_off: usize, limit: usize): usize {
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

fn hashBytes(buf: []const u8, pos: usize): usize {
    const v = readU32LE(buf, pos);
    const prod = v *% HASH_MULTIPLIER;
    return (prod >> (32 - 14)) as usize;
}

fn writeVarint(out: *[]u8, start_pos: usize, value_in: usize): usize {
    let pos = start_pos;
    let v: u64 = value_in as u64;
    while (v >= 0x80) {
        out[pos] = ((v & 0x7f) | 0x80) as u8;
        pos += 1;
        v >>= 7;
    }
    out[pos] = (v & 0x7f) as u8;
    return pos + 1;
}

struct VarintResult {
    value: usize,
    next_pos: usize,
}

fn readVarint(input: []const u8): VarintResult ! SnappyError {
    if (input.len == 0) {
        return SnappyError.TruncatedInput;
    }

    let pos: usize = 0;
    let result: u64 = 0;
    let shift: u6 = 0;

    while (pos < input.len) {
        const b = input[pos];
        pos += 1;

        if (shift == 63 && b > 1) {
            return SnappyError.InvalidVarint;
        }

        result |= ((b & 0x7f) as u64) << shift;
        if ((b & 0x80) == 0) {
            if (result > (std.math.maxInt(usize) as u64)) {
                return SnappyError.OutputTooLarge;
            }
            return VarintResult{
                value: result as usize,
                next_pos: pos,
            };
        }

        if (shift >= 63) {
            return SnappyError.InvalidVarint;
        }
        shift += 7;
    }

    return SnappyError.TruncatedInput;
}

// ============================================================================
// 2. Snappy Raw Block Codec (Zero-Heap Into + Allocator APIs)
// ============================================================================

/// Returns the maximum possible compressed size in bytes for a Snappy raw block of `src_len` bytes.
pub fn maxCompressLen(src_len: usize): usize {
    return 32 + src_len + (src_len / 6);
}

/// Reads and returns the uncompressed byte length from the Snappy block header
/// without allocating decompression memory.
pub fn uncompressedLength(compressed: []const u8): usize ! SnappyError {
    const hdr = readVarint(compressed)?;
    return hdr.value;
}

/// Alias matching `rust-snappy` `decompress_len`.
pub fn decompressLen(compressed: []const u8): usize ! SnappyError {
    return uncompressedLength(compressed);
}

fn emitLiteral(out: *[]u8, start_pos: usize, lit: []const u8): usize {
    let pos = start_pos;
    const n = lit.len - 1;

    if (n < 60) {
        out[pos] = ((n as u8) << 2);
        pos += 1;
    } else if (n < 256) {
        out[pos] = (60 << 2) as u8;
        out[pos + 1] = n as u8;
        pos += 2;
    } else if (n < 65536) {
        out[pos] = (61 << 2) as u8;
        out[pos + 1] = (n & 0xff) as u8;
        out[pos + 2] = ((n >> 8) & 0xff) as u8;
        pos += 3;
    } else if (n < 16777216) {
        out[pos] = (62 << 2) as u8;
        out[pos + 1] = (n & 0xff) as u8;
        out[pos + 2] = ((n >> 8) & 0xff) as u8;
        out[pos + 3] = ((n >> 16) & 0xff) as u8;
        pos += 4;
    } else {
        out[pos] = (63 << 2) as u8;
        out[pos + 1] = (n & 0xff) as u8;
        out[pos + 2] = ((n >> 8) & 0xff) as u8;
        out[pos + 3] = ((n >> 16) & 0xff) as u8;
        out[pos + 4] = ((n >> 24) & 0xff) as u8;
        pos += 5;
    }

    snappyCopyLitSimd(out, pos, lit);
    return pos + lit.len;
}

fn emitCopy(out: *[]u8, start_pos: usize, offset: usize, len_in: usize): usize {
    let pos = start_pos;
    let rem = len_in;

    while (rem > 0) {
        if (rem >= 4 && rem <= 11 && offset < 2048) {
            const len_bits = ((rem - 4) & 0x07) << 2;
            const off_hi = ((offset >> 8) & 0x07) << 5;
            out[pos] = (0x01 | len_bits | off_hi) as u8;
            out[pos + 1] = (offset & 0xff) as u8;
            pos += 2;
            rem = 0;
        } else {
            const chunk = if (rem > 64) 64 else rem;
            const tag = 0x02 | (((chunk - 1) & 0x3f) << 2);
            out[pos] = tag as u8;
            out[pos + 1] = (offset & 0xff) as u8;
            out[pos + 2] = ((offset >> 8) & 0xff) as u8;
            pos += 3;
            rem -= chunk;
        }
    }

    return pos;
}

fn compressBlock(out: *[]u8, start_pos: usize, block: []const u8, table: *[]u16): usize {
    let pos = start_pos;

    if (block.len < 15) {
        return emitLiteral(out, pos, block);
    }

    let ti: usize = 0;
    while (ti < table.len) : (ti += 1) table[ti] = 0;
    table[hashBytes(block, 0)] = 0;

    let next_emit: usize = 0;
    let ip: usize = 1;
    const ip_limit = block.len - 15;
    let skip: usize = 32;

    while (ip <= ip_limit) {
        const h = hashBytes(block, ip);
        const candidate = table[h] as usize;
        table[h] = ip as u16;

        if (candidate < ip && readU32LE(block, candidate) == readU32LE(block, ip)) {
            if (ip > next_emit) {
                pos = emitLiteral(out, pos, block[next_emit..ip]);
            }

            const extra: usize = snappyExtendMatchSimd(block, candidate + 4, ip + 4, block.len);
            const match_len: usize = extra + 4;
            const offset = ip - candidate;
            pos = emitCopy(out, pos, offset, match_len);

            ip += match_len;
            next_emit = ip;
            skip = 32;

            if (ip <= ip_limit) {
                table[hashBytes(block, ip - 1)] = (ip - 1) as u16;
            }
        } else {
            const step = skip >> 5;
            ip += step;
            skip += 1;
        }
    }

    if (next_emit < block.len) {
        pos = emitLiteral(out, pos, block[next_emit..block.len]);
    }

    return pos;
}

/// Zero-heap Snappy raw block compression into caller-owned `dst` (must have `dst.len >= maxCompressLen(input.len)`).
pub fn compressInto(input: []const u8, dst: *[]u8): usize ! SnappyError {
    if (dst.len < maxCompressLen(input.len)) return SnappyError.BufferTooSmall;
    let out_pos: usize = writeVarint(dst, 0, input.len);
    if (input.len == 0) return out_pos;

    let table_arr: [HASH_TABLE_SIZE]u16 = undefined;
    let table_s: []u16 = table_arr[0..];
    let offset: usize = 0;
    while (offset < input.len) {
        const remaining = input.len - offset;
        const chunk_len = if (remaining > BLOCK_SIZE) BLOCK_SIZE else remaining;
        out_pos = compressBlock(dst, out_pos, input[offset..offset + chunk_len], table_s);
        offset += chunk_len;
    }
    return out_pos;
}

/// Compresses `input` using the Snappy Block Format.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn compress(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! SnappyError {
    const max_len = maxCompressLen(input.len);
    let scratch = allocator.alloc(u8, max_len) catch return SnappyError.OutOfMemory;
    defer allocator.free(scratch);

    const out_pos: usize = compressInto(input, scratch)?;
    let result = allocator.alloc(u8, out_pos) catch return SnappyError.OutOfMemory;
    snappyCopyLitSimd(result, 0, scratch[0..out_pos]);
    return result;
}

/// Zero-heap Snappy raw block decompression into caller-owned `dst`.
/// Overflow-safe across all 4 tag types (Literal, 1B/2B/4B Copy) and SIMD-accelerated.
pub fn decompressInto(compressed: []const u8, dst: *[]u8): usize ! SnappyError {
    const hdr = readVarint(compressed)?;
    const target_len: usize = hdr.value;
    if (target_len > dst.len) {
        return SnappyError.OutputTooLarge;
    }

    let in_pos: usize = hdr.next_pos;
    let out_pos: usize = 0;

    while (in_pos < compressed.len) {
        const tag = compressed[in_pos];
        in_pos += 1;
        const elem_type = tag & 0x03;

        if (elem_type == 0x00) {
            const len_code = (tag >> 2) & 0x3f;
            let lit_len: usize = 0;
            if (len_code < 60) {
                lit_len = (len_code as usize) + 1;
            } else {
                const extra_bytes = (len_code - 59) as usize;
                if (compressed.len - in_pos < extra_bytes) return SnappyError.TruncatedInput;
                let raw_len: u64 = 0;
                let b_idx: usize = 0;
                while (b_idx < extra_bytes) : (b_idx += 1) {
                    raw_len |= (compressed[in_pos + b_idx] as u64) << ((b_idx * 8) as u6);
                }
                in_pos += extra_bytes;
                if (raw_len >= (target_len as u64)) return SnappyError.CorruptedData;
                lit_len = (raw_len as usize) + 1;
            }

            if (compressed.len - in_pos < lit_len) return SnappyError.TruncatedInput;
            if (target_len - out_pos < lit_len) return SnappyError.CorruptedData;

            snappyCopyLitSimd(dst, out_pos, compressed[in_pos..in_pos + lit_len]);
            in_pos += lit_len;
            out_pos += lit_len;
        } else if (elem_type == 0x01) {
            if (in_pos >= compressed.len) return SnappyError.TruncatedInput;
            const low = compressed[in_pos] as usize;
            in_pos += 1;

            const copy_len = (((tag >> 2) & 0x07) as usize) + 4;
            const offset = ((((tag >> 5) & 0x07) as usize) << 8) | low;

            if (offset == 0 || offset > out_pos) return SnappyError.InvalidOffset;
            if (target_len - out_pos < copy_len) return SnappyError.CorruptedData;

            snappyCopyMatchSimd(dst, out_pos, offset, copy_len);
            out_pos += copy_len;
        } else if (elem_type == 0x02) {
            if (compressed.len - in_pos < 2) return SnappyError.TruncatedInput;
            const b0 = compressed[in_pos] as usize;
            const b1 = compressed[in_pos + 1] as usize;
            in_pos += 2;

            const copy_len = (((tag >> 2) & 0x3f) as usize) + 1;
            const offset = b0 | (b1 << 8);

            if (offset == 0 || offset > out_pos) return SnappyError.InvalidOffset;
            if (target_len - out_pos < copy_len) return SnappyError.CorruptedData;

            snappyCopyMatchSimd(dst, out_pos, offset, copy_len);
            out_pos += copy_len;
        } else {
            if (compressed.len - in_pos < 4) return SnappyError.TruncatedInput;
            const offset_u32: u32 = readU32LE(compressed, in_pos);
            in_pos += 4;

            const copy_len = (((tag >> 2) & 0x3f) as usize) + 1;
            const offset: usize = offset_u32 as usize;

            if (offset == 0 || offset > out_pos) return SnappyError.InvalidOffset;
            if (target_len - out_pos < copy_len) return SnappyError.CorruptedData;

            snappyCopyMatchSimd(dst, out_pos, offset, copy_len);
            out_pos += copy_len;
        }
    }

    if (out_pos != target_len) {
        return SnappyError.CorruptedData;
    }
    return out_pos;
}

/// Decompresses a Snappy block while enforcing an upper bound `max_bytes`
/// on the declared uncompressed output size.
pub fn decompressWithMaxSize(allocator: std.mem.Allocator, compressed: []const u8, max_bytes: usize): ([]u8) ! SnappyError {
    const hdr = readVarint(compressed)?;
    const target_len = hdr.value;
    if (target_len > max_bytes) {
        return SnappyError.OutputTooLarge;
    }

    let out = allocator.alloc(u8, target_len) catch return SnappyError.OutOfMemory;
    errdefer allocator.free(out);

    * = decompressInto(compressed, out)?;
    return out;
}

/// Decompresses a Snappy block without an explicit custom size cap.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompress(allocator: std.mem.Allocator, compressed: []const u8): ([]u8) ! SnappyError {
    return decompressWithMaxSize(allocator, compressed, std.math.maxInt(usize));
}

// ============================================================================
// 3. Complete Snappy Framing Format (`sNaPpY`) + Masked CRC32C
// ============================================================================

/// Encodes a Snappy Framing Padding chunk (`0xFE`) of `pad_len` zero bytes.
pub fn encodeFramedPaddingChunk(allocator: std.mem.Allocator, pad_len: usize): ([]u8) ! SnappyError {
    if (pad_len > 0x00FF_FFFF) return SnappyError.ChunkTooLarge;
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    out.append(allocator, 0xFE) catch return SnappyError.OutOfMemory;
    writeU24LE(*out, allocator, pad_len as u32)?;
    let i: usize = 0;
    while (i < pad_len) : (i += 1) {
        out.append(allocator, 0) catch return SnappyError.OutOfMemory;
    }
    return out.toOwnedSlice(allocator) catch return SnappyError.OutOfMemory;
}

/// Encodes a Snappy Framing Reserved Skippable chunk (`chunk_type` in `0x80..0xFD`).
pub fn encodeFramedSkippableChunk(
    allocator: std.mem.Allocator,
    chunk_type: u8,
    payload: []const u8,
): ([]u8) ! SnappyError {
    if (chunk_type < 0x80 || chunk_type > 0xFD) return SnappyError.UnsupportedChunkType;
    if (payload.len > 0x00FF_FFFF) return SnappyError.ChunkTooLarge;
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    out.append(allocator, chunk_type) catch return SnappyError.OutOfMemory;
    writeU24LE(*out, allocator, payload.len as u32)?;
    out.appendSlice(allocator, payload) catch return SnappyError.OutOfMemory;
    return out.toOwnedSlice(allocator) catch return SnappyError.OutOfMemory;
}

/// Compresses `input` into the official Snappy Framing Format (`sNaPpY`) with
/// 64KB chunking, uncompressed fallback when incompressible, and Masked CRC32C per chunk.
pub fn compressFramed(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! SnappyError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    // 10-byte Stream Identifier chunk
    out.appendSlice(allocator, SNAPPY_STREAM_IDENTIFIER[0..]) catch return SnappyError.OutOfMemory;

    let comp_scratch = allocator.alloc(u8, maxCompressLen(SNAPPY_FRAME_CHUNK_SIZE)) catch return SnappyError.OutOfMemory;
    defer allocator.free(comp_scratch);

    let pos: usize = 0;
    while (pos < input.len) {
        const rem: usize = input.len - pos;
        const chunk_len: usize = if (rem > SNAPPY_FRAME_CHUNK_SIZE) SNAPPY_FRAME_CHUNK_SIZE else rem;
        const chunk: []const u8 = input[pos..pos + chunk_len];
        const masked_crc: u32 = snappyMaskChecksum(snappyCrc32c(chunk));

        const comp_len: usize = compressInto(chunk, comp_scratch)?;
        if (comp_len < chunk.len) {
            // Compressed Data Chunk (0x00)
            out.append(allocator, 0x00) catch return SnappyError.OutOfMemory;
            writeU24LE(*out, allocator, (comp_len + 4) as u32)?;
            writeU32LE(*out, allocator, masked_crc)?;
            out.appendSlice(allocator, comp_scratch[0..comp_len]) catch return SnappyError.OutOfMemory;
        } else {
            // Uncompressed Data Chunk (0x01)
            out.append(allocator, 0x01) catch return SnappyError.OutOfMemory;
            writeU24LE(*out, allocator, (chunk.len + 4) as u32)?;
            writeU32LE(*out, allocator, masked_crc)?;
            out.appendSlice(allocator, chunk) catch return SnappyError.OutOfMemory;
        }
        pos += chunk_len;
    }

    return out.toOwnedSlice(allocator) catch return SnappyError.OutOfMemory;
}

/// Decompresses a Snappy Framing Format (`sNaPpY`) stream while enforcing `max_bytes`,
/// validating Stream Identifier, Masked CRC32C per chunk, 64KB chunk cap,
/// skipping Padding (`0xFE`) & Skippable (`0x80..0xFD`), and rejecting Unskippable (`0x02..0x7F`).
pub fn decompressFramedWithMaxSize(
    allocator: std.mem.Allocator,
    framed: []const u8,
    max_bytes: usize,
): ([]u8) ! SnappyError {
    if (framed.len < 10) return SnappyError.TruncatedInput;
    let si: usize = 0;
    while (si < 10) : (si += 1) {
        if (framed[si] != SNAPPY_STREAM_IDENTIFIER[si]) return SnappyError.InvalidStreamIdentifier;
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    let chunk_buf = allocator.alloc(u8, SNAPPY_FRAME_CHUNK_SIZE) catch return SnappyError.OutOfMemory;
    defer allocator.free(chunk_buf);

    let pos: usize = 10;
    while (pos < framed.len) {
        if (framed.len - pos < 4) return SnappyError.TruncatedInput;
        const ctype: u8 = framed[pos];
        const clen: usize = readU24LE(framed, pos + 1) as usize;
        pos += 4;
        if (framed.len - pos < clen) return SnappyError.TruncatedInput;
        const payload: []const u8 = framed[pos..pos + clen];
        pos += clen;

        if (ctype == 0xFF) {
            // Repeated Stream Identifier (concatenated streams)
            if (clen != 6) return SnappyError.InvalidStreamIdentifier;
            let k: usize = 0;
            while (k < 6) : (k += 1) {
                if (payload[k] != SNAPPY_STREAM_IDENTIFIER[4 + k]) return SnappyError.InvalidStreamIdentifier;
            }
        } else if (ctype == 0x00) {
            // Compressed Data Chunk
            if (clen < 4) return SnappyError.TruncatedInput;
            const expected_masked: u32 = readU32LE(payload, 0);
            const comp_data: []const u8 = payload[4..clen];
            const declen: usize = decompressLen(comp_data)?;
            if (declen > SNAPPY_FRAME_CHUNK_SIZE) return SnappyError.ChunkTooLarge;
            const written: usize = decompressInto(comp_data, chunk_buf)?;
            const actual_masked: u32 = snappyMaskChecksum(snappyCrc32c(chunk_buf[0..written]));
            if (actual_masked != expected_masked) return SnappyError.ChecksumMismatch;
            if (max_bytes - out.items.len < written) return SnappyError.OutputTooLarge;
            out.appendSlice(allocator, chunk_buf[0..written]) catch return SnappyError.OutOfMemory;
        } else if (ctype == 0x01) {
            // Uncompressed Data Chunk
            if (clen < 4) return SnappyError.TruncatedInput;
            const raw_len: usize = clen - 4;
            if (raw_len > SNAPPY_FRAME_CHUNK_SIZE) return SnappyError.ChunkTooLarge;
            const expected_masked: u32 = readU32LE(payload, 0);
            const raw_data: []const u8 = payload[4..clen];
            const actual_masked: u32 = snappyMaskChecksum(snappyCrc32c(raw_data));
            if (actual_masked != expected_masked) return SnappyError.ChecksumMismatch;
            if (max_bytes - out.items.len < raw_len) return SnappyError.OutputTooLarge;
            out.appendSlice(allocator, raw_data) catch return SnappyError.OutOfMemory;
        } else if (ctype == 0xFE || (ctype >= 0x80 && ctype <= 0xFD)) {
            // Padding or Reserved Skippable Chunk -> safely skip
            continue;
        } else {
            // Reserved Unskippable Chunk (0x02..0x7F) -> reject per spec
            return SnappyError.UnsupportedChunkType;
        }
    }

    return out.toOwnedSlice(allocator) catch return SnappyError.OutOfMemory;
}

/// Decompresses a Snappy Framing Format (`sNaPpY`) stream without a custom size cap.
pub fn decompressFramed(allocator: std.mem.Allocator, framed: []const u8): ([]u8) ! SnappyError {
    return decompressFramedWithMaxSize(allocator, framed, std.math.maxInt(usize));
}
