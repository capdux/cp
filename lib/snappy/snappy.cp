// Source: cp/lib/snappy/snappy.cp (Pure CP Snappy Block Format Compressor & Decompressor)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");

/// Error set for Snappy block compression and decompression.
pub err SnappyError {
    InvalidVarint,
    CorruptedData,
    TruncatedInput,
    InvalidOffset,
    OutputTooLarge,
    OutOfMemory,
}

const BLOCK_SIZE: usize = 65536;
const HASH_BITS: u5 = 14;
const HASH_TABLE_SIZE: usize = 16384;
const HASH_MULTIPLIER: u32 = 0x1e35a7bd;

fn readU32LE(buf: []const u8, pos: usize): u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn hashBytes(buf: []const u8, pos: usize): usize {
    const v = readU32LE(buf, pos);
    const prod = v *% HASH_MULTIPLIER;
    return (prod >> (32 - 14)) as usize;
}

fn writeVarint(out: []u8, start_pos: usize, value_in: usize): usize {
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

fn emitLiteral(out: []u8, start_pos: usize, lit: []const u8): usize {
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

    @memcpy(out[pos..(pos + lit.len)], lit);
    return pos + lit.len;
}

fn emitCopy(out: []u8, start_pos: usize, offset: usize, len_in: usize): usize {
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

fn compressBlock(out: []u8, start_pos: usize, block: []const u8, table: []u16): usize {
    let pos = start_pos;

    if (block.len < 15) {
        return emitLiteral(out, pos, block);
    }

    @memset(table, 0);
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

            let match_len: usize = 4;
            while (ip + match_len < block.len && block[candidate + match_len] == block[ip + match_len]) {
                match_len += 1;
            }

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

/// Compresses `input` using the Snappy Block Format.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn compress(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! SnappyError {
    const max_len = 10 + 32 + input.len + (input.len / 6);
    let scratch = allocator.alloc(u8, max_len) catch return SnappyError.OutOfMemory;
    defer allocator.free(scratch);

    let out_pos = writeVarint(scratch, 0, input.len);

    if (input.len > 0) {
        let table = allocator.alloc(u16, HASH_TABLE_SIZE) catch return SnappyError.OutOfMemory;
        defer allocator.free(table);

        let offset: usize = 0;
        while (offset < input.len) {
            const remaining = input.len - offset;
            const chunk_len = if (remaining > BLOCK_SIZE) BLOCK_SIZE else remaining;
            out_pos = compressBlock(scratch, out_pos, input[offset..(offset + chunk_len)], table);
            offset += chunk_len;
        }
    }

    let result = allocator.alloc(u8, out_pos) catch return SnappyError.OutOfMemory;
    @memcpy(result, scratch[0..out_pos]);
    return result;
}

/// Reads and returns the uncompressed byte length from the Snappy block header
/// without allocating decompression memory.
pub fn uncompressedLength(compressed: []const u8): usize ! SnappyError {
    const hdr = readVarint(compressed)?;
    return hdr.value;
}

fn copyMatch(out: []u8, out_pos: usize, offset: usize, copy_len: usize) {
    if (offset >= copy_len) {
        const src_start = out_pos - offset;
        @memcpy(out[out_pos..(out_pos + copy_len)], out[src_start..(src_start + copy_len)]);
    } else {
        let i: usize = 0;
        while (i < copy_len) : (i += 1) {
            out[out_pos + i] = out[out_pos - offset + i];
        }
    }
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

    let in_pos = hdr.next_pos;
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
                if (in_pos + extra_bytes > compressed.len) {
                    return SnappyError.TruncatedInput;
                }
                let raw_len: usize = 0;
                let b_idx: usize = 0;
                while (b_idx < extra_bytes) : (b_idx += 1) {
                    raw_len |= (compressed[in_pos + b_idx] as usize) << ((b_idx * 8) as u6);
                }
                in_pos += extra_bytes;
                lit_len = raw_len + 1;
            }

            if (in_pos + lit_len > compressed.len) {
                return SnappyError.TruncatedInput;
            }
            if (out_pos + lit_len > target_len) {
                return SnappyError.CorruptedData;
            }

            @memcpy(out[out_pos..(out_pos + lit_len)], compressed[in_pos..(in_pos + lit_len)]);
            in_pos += lit_len;
            out_pos += lit_len;
        } else if (elem_type == 0x01) {
            if (in_pos >= compressed.len) {
                return SnappyError.TruncatedInput;
            }
            const low = compressed[in_pos] as usize;
            in_pos += 1;

            const copy_len = (((tag >> 2) & 0x07) as usize) + 4;
            const offset = ((((tag >> 5) & 0x07) as usize) << 8) | low;

            if (offset == 0 || offset > out_pos) {
                return SnappyError.InvalidOffset;
            }
            if (out_pos + copy_len > target_len) {
                return SnappyError.CorruptedData;
            }

            copyMatch(out, out_pos, offset, copy_len);
            out_pos += copy_len;
        } else if (elem_type == 0x02) {
            if (in_pos + 2 > compressed.len) {
                return SnappyError.TruncatedInput;
            }
            const b0 = compressed[in_pos] as usize;
            const b1 = compressed[in_pos + 1] as usize;
            in_pos += 2;

            const copy_len = (((tag >> 2) & 0x3f) as usize) + 1;
            const offset = b0 | (b1 << 8);

            if (offset == 0 || offset > out_pos) {
                return SnappyError.InvalidOffset;
            }
            if (out_pos + copy_len > target_len) {
                return SnappyError.CorruptedData;
            }

            copyMatch(out, out_pos, offset, copy_len);
            out_pos += copy_len;
        } else {
            if (in_pos + 4 > compressed.len) {
                return SnappyError.TruncatedInput;
            }
            const b0 = compressed[in_pos] as usize;
            const b1 = compressed[in_pos + 1] as usize;
            const b2 = compressed[in_pos + 2] as usize;
            const b3 = compressed[in_pos + 3] as usize;
            in_pos += 4;

            const copy_len = (((tag >> 2) & 0x3f) as usize) + 1;
            const offset = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

            if (offset == 0 || offset > out_pos) {
                return SnappyError.InvalidOffset;
            }
            if (out_pos + copy_len > target_len) {
                return SnappyError.CorruptedData;
            }

            copyMatch(out, out_pos, offset, copy_len);
            out_pos += copy_len;
        }
    }

    if (out_pos != target_len) {
        return SnappyError.CorruptedData;
    }

    return out;
}

/// Decompresses a Snappy block without an explicit custom size cap.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompress(allocator: std.mem.Allocator, compressed: []const u8): ([]u8) ! SnappyError {
    return decompressWithMaxSize(allocator, compressed, std.math.maxInt(usize));
}
