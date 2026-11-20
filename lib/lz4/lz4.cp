// Source: cp/lib/lz4/lz4.cp (Pure CP LZ4 Block & Frame Compression + XXH32 Checksum Engine)
// Zero-C-dependency LZ4 Block Format, standard LZ4 Frame Format (0x184D2204 + XXH32
// Header/Content Checksum), and standard XXH32 hash implementation.

const std = @import("std");

/// Error set for LZ4 compression, decompression, and framing operations.
pub err Lz4Error {
    CorruptStream,
    InvalidMagic,
    InvalidHeaderChecksum,
    InvalidContentChecksum,
    OutputTooLarge,
    InvalidOffset,
    TruncatedInput,
    OutOfMemory,
}

pub const LZ4_FRAME_MAGIC: u32 = 0x184D2204;
pub const LZ4_FRAME_BLOCK_SIZE: usize = 65536;

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

@inline
fn hash4(data: []const u8, pos: usize): usize {
    const seq = readLE32(data, pos);
    return (((seq *% 2654435761) >> 20) & 0xFFF) as usize;
}

fn writeExtLen(out: *std.ArrayList(u8), alloc: std.mem.Allocator, len_val: usize): void ! Lz4Error {
    let rem = len_val;
    while (rem >= 255) {
        out.append(alloc, 255) catch return Lz4Error.OutOfMemory;
        rem -= 255;
    }
    out.append(alloc, rem as u8) catch return Lz4Error.OutOfMemory;
}

/// Standard LZ4 raw block format compressor and decompressor.
pub static class Lz4Block {
    /// Compresses `src` into standard LZ4 raw block format.
    pub fn compress(alloc: std.mem.Allocator, src: []const u8): []u8 ! Lz4Error {
        if (src.len == 0) {
            return alloc.alloc(u8, 0) catch return Lz4Error.OutOfMemory;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (src.len < MFLIMIT + 1) {
            const lit_len = src.len;
            const nibble: u8 = if (lit_len >= 15) 15 else (lit_len as u8);
            out.append(alloc, nibble << 4) catch return Lz4Error.OutOfMemory;
            if (lit_len >= 15) {
                writeExtLen(*out, alloc, lit_len - 15)?;
            }
            out.appendSlice(alloc, src) catch return Lz4Error.OutOfMemory;
            return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
        }

        let table: [HASH_SIZE]u32 = [0; HASH_SIZE];
        table[hash4(src, 0)] = 1;

        let anchor: usize = 0;
        let ip: usize = 1;
        const mf_limit = src.len - MFLIMIT;
        const match_limit = src.len - LASTLITERALS;

        while (ip <= mf_limit) {
            const h = hash4(src, ip);
            const ref_plus1 = table[h];
            table[h] = (ip + 1) as u32;

            if (ref_plus1 != 0) {
                const ref_pos = (ref_plus1 - 1) as usize;
                const dist = ip - ref_pos;
                if (dist > 0 && dist <= MAX_DISTANCE && readLE32(src, ref_pos) == readLE32(src, ip)) {
                    const lit_len = ip - anchor;
                    const token_idx = out.items.len;
                    out.append(alloc, 0) catch return Lz4Error.OutOfMemory;

                    const lit_nibble: u8 = if (lit_len >= 15) 15 else (lit_len as u8);
                    if (lit_len >= 15) {
                        writeExtLen(*out, alloc, lit_len - 15)?;
                    }
                    out.appendSlice(alloc, src[anchor..ip]) catch return Lz4Error.OutOfMemory;

                    // Write 2-byte little-endian match offset
                    out.append(alloc, (dist & 0xFF) as u8) catch return Lz4Error.OutOfMemory;
                    out.append(alloc, ((dist >> 8) & 0xFF) as u8) catch return Lz4Error.OutOfMemory;

                    // Extend match forward within match_limit (leaving at least 5 literal bytes)
                    let ip_end = ip + MINMATCH;
                    let ref_end = ref_pos + MINMATCH;
                    while (ip_end < match_limit && src[ip_end] == src[ref_end]) {
                        ip_end += 1;
                        ref_end += 1;
                    }

                    const match_len = ip_end - ip;
                    const ml_base = match_len - MINMATCH;
                    const ml_nibble: u8 = if (ml_base >= 15) 15 else (ml_base as u8);
                    out.items[token_idx] = (lit_nibble << 4) | ml_nibble;

                    if (ml_base >= 15) {
                        writeExtLen(*out, alloc, ml_base - 15)?;
                    }

                    ip = ip_end;
                    anchor = ip;
                    continue;
                }
            }

            ip += 1;
        }

        // Emit trailing literals (at least 5 bytes when matches occurred)
        const last_lit_len = src.len - anchor;
        const last_nibble: u8 = if (last_lit_len >= 15) 15 else (last_lit_len as u8);
        out.append(alloc, last_nibble << 4) catch return Lz4Error.OutOfMemory;
        if (last_lit_len >= 15) {
            writeExtLen(*out, alloc, last_lit_len - 15)?;
        }
        out.appendSlice(alloc, src[anchor..src.len]) catch return Lz4Error.OutOfMemory;

        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }

    /// Decompresses a standard LZ4 raw block, enforcing `max_output_size` and strict offset/bounds checks.
    pub fn decompress(alloc: std.mem.Allocator, compressed: []const u8, max_output_size: usize): []u8 ! Lz4Error {
        if (compressed.len == 0) {
            return alloc.alloc(u8, 0) catch return Lz4Error.OutOfMemory;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let ip: usize = 0;
        while (ip < compressed.len) {
            const token = compressed[ip];
            ip += 1;

            let lit_len: usize = (token >> 4) as usize;
            if (lit_len == 15) {
                while (true) {
                    if (ip >= compressed.len) {
                        return Lz4Error.TruncatedInput;
                    }
                    const b = compressed[ip];
                    ip += 1;
                    if (lit_len > max_output_size) {
                        return Lz4Error.OutputTooLarge;
                    }
                    lit_len += b as usize;
                    if (b != 255) {
                        break;
                    }
                }
            }

            if (compressed.len - ip < lit_len) {
                return Lz4Error.TruncatedInput;
            }
            if (max_output_size - out.items.len < lit_len) {
                return Lz4Error.OutputTooLarge;
            }

            out.appendSlice(alloc, compressed[ip .. ip + lit_len]) catch return Lz4Error.OutOfMemory;
            ip += lit_len;

            // End of block: last sequence has literals only and no offset/match
            if (ip == compressed.len) {
                break;
            }

            if (compressed.len - ip < 2) {
                return Lz4Error.TruncatedInput;
            }
            const offset: usize = (compressed[ip] as usize) | ((compressed[ip + 1] as usize) << 8);
            ip += 2;

            if (offset == 0 || offset > out.items.len) {
                return Lz4Error.InvalidOffset;
            }

            let match_len: usize = ((token & 0x0F) as usize) + MINMATCH;
            if ((token & 0x0F) == 15) {
                while (true) {
                    if (ip >= compressed.len) {
                        return Lz4Error.TruncatedInput;
                    }
                    const b = compressed[ip];
                    ip += 1;
                    if (match_len > max_output_size) {
                        return Lz4Error.OutputTooLarge;
                    }
                    match_len += b as usize;
                    if (b != 255) {
                        break;
                    }
                }
            }

            if (max_output_size - out.items.len < match_len) {
                return Lz4Error.OutputTooLarge;
            }

            const match_start = out.items.len - offset;
            out.ensureTotalCapacity(alloc, out.items.len + match_len) catch return Lz4Error.OutOfMemory;
            for (i of 0..match_len) {
                out.append(alloc, out.items[match_start + i]) catch return Lz4Error.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }
}

/// Standard LZ4 Frame Format (0x184D2204) compressor and decompressor with XXH32 checksums.
pub static class Lz4Frame {
    /// Compresses `src` into a standard LZ4 Frame, splitting into 64KB blocks with optional XXH32 content checksum.
    pub fn compressFrame(alloc: std.mem.Allocator, src: []const u8, with_content_checksum: bool): []u8 ! Lz4Error {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        // 4-byte Magic Number (0x184D2204 little-endian)
        writeLE32(*out, alloc, LZ4_FRAME_MAGIC)?;

        // FLG: Version=01 (0x40), B.Indep=1 (0x20), C.Checksum=0x04 if enabled
        const flg: u8 = if (with_content_checksum) 0x64 else 0x60;
        // BD: Block MaxSize = 4 (64KB -> 0x40)
        const bd: u8 = 0x40;
        const header_fields: [2]u8 = [flg, bd];
        const hc: u8 = ((xxhash32(header_fields[0..], 0) >> 8) & 0xFF) as u8;

        out.append(alloc, flg) catch return Lz4Error.OutOfMemory;
        out.append(alloc, bd) catch return Lz4Error.OutOfMemory;
        out.append(alloc, hc) catch return Lz4Error.OutOfMemory;

        let pos: usize = 0;
        while (pos < src.len) {
            const rem = src.len - pos;
            const chunk_len = if (rem > LZ4_FRAME_BLOCK_SIZE) LZ4_FRAME_BLOCK_SIZE else rem;
            const chunk = src[pos .. pos + chunk_len];

            const comp = Lz4Block.compress(alloc, chunk)?;
            defer alloc.free(comp);

            if (comp.len < chunk.len) {
                writeLE32(*out, alloc, comp.len as u32)?;
                out.appendSlice(alloc, comp) catch return Lz4Error.OutOfMemory;
            } else {
                // High bit 1 marks uncompressed raw block fallback
                writeLE32(*out, alloc, (chunk.len as u32) | 0x80000000)?;
                out.appendSlice(alloc, chunk) catch return Lz4Error.OutOfMemory;
            }

            pos += chunk_len;
        }

        // EndMark (0x00000000)
        writeLE32(*out, alloc, 0)?;

        if (with_content_checksum) {
            const cksum = xxhash32(src, 0);
            writeLE32(*out, alloc, cksum)?;
        }

        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }

    /// Decompresses a standard LZ4 Frame, validating magic, header XXH32 checksum, blocks, and optional content checksum.
    pub fn decompressFrame(alloc: std.mem.Allocator, frame: []const u8, max_output_size: usize): []u8 ! Lz4Error {
        if (frame.len < 7) {
            return Lz4Error.TruncatedInput;
        }

        const magic = readLE32(frame, 0);
        if (magic != LZ4_FRAME_MAGIC) {
            return Lz4Error.InvalidMagic;
        }

        const flg = frame[4];
        const bd = frame[5];

        // Version bits (7..6) must be 01, reserved FLG bit 1 must be 0, reserved BD bits (7, 3..0) must be 0
        if ((flg & 0xC0) != 0x40 || (flg & 0x02) != 0 || (bd & 0x8F) != 0) {
            return Lz4Error.CorruptStream;
        }

        const has_block_checksum = (flg & 0x10) != 0;
        const has_content_size = (flg & 0x08) != 0;
        const has_content_checksum = (flg & 0x04) != 0;
        const has_dict_id = (flg & 0x01) != 0;

        let desc_end: usize = 6;
        if (has_content_size) {
            desc_end += 8;
        }
        if (has_dict_id) {
            desc_end += 4;
        }

        if (frame.len < desc_end + 1) {
            return Lz4Error.TruncatedInput;
        }

        const expected_hc: u8 = ((xxhash32(frame[4..desc_end], 0) >> 8) & 0xFF) as u8;
        if (frame[desc_end] != expected_hc) {
            return Lz4Error.InvalidHeaderChecksum;
        }

        let pos: usize = desc_end + 1;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        while (true) {
            if (frame.len - pos < 4) {
                return Lz4Error.TruncatedInput;
            }
            const raw_size = readLE32(frame, pos);
            pos += 4;

            if (raw_size == 0) {
                break;
            }

            const is_uncompressed = (raw_size & 0x80000000) != 0;
            const block_len = (raw_size & 0x7FFFFFFF) as usize;
            if (frame.len - pos < block_len) {
                return Lz4Error.TruncatedInput;
            }

            const block_data = frame[pos .. pos + block_len];
            pos += block_len;

            if (has_block_checksum) {
                if (frame.len - pos < 4) {
                    return Lz4Error.TruncatedInput;
                }
                const expected_blk_cksum = readLE32(frame, pos);
                pos += 4;
                if (xxhash32(block_data, 0) != expected_blk_cksum) {
                    return Lz4Error.CorruptStream;
                }
            }

            if (is_uncompressed) {
                if (max_output_size - out.items.len < block_len) {
                    return Lz4Error.OutputTooLarge;
                }
                out.appendSlice(alloc, block_data) catch return Lz4Error.OutOfMemory;
            } else {
                const rem_max = max_output_size - out.items.len;
                const decoded = Lz4Block.decompress(alloc, block_data, rem_max)?;
                defer alloc.free(decoded);
                out.appendSlice(alloc, decoded) catch return Lz4Error.OutOfMemory;
            }
        }

        if (has_content_checksum) {
            if (frame.len - pos < 4) {
                return Lz4Error.TruncatedInput;
            }
            const expected_c_cksum = readLE32(frame, pos);
            const actual_c_cksum = xxhash32(out.items, 0);
            if (actual_c_cksum != expected_c_cksum) {
                return Lz4Error.InvalidContentChecksum;
            }
        }

        return out.toOwnedSlice(alloc) catch return Lz4Error.OutOfMemory;
    }
}
