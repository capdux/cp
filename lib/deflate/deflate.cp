// Source: cp/lib/deflate/deflate.cp (Pure CP RFC 1951 DEFLATE, RFC 1952 GZIP & RFC 1950 ZLIB Codec)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");

/// Error set for DEFLATE, GZIP, and ZLIB compression and decompression.
pub err DeflateError {
    UnexpectedEof,
    InvalidBlockType,
    InvalidStoredLength,
    InvalidHuffmanCode,
    InvalidDistance,
    InvalidHeader,
    ChecksumMismatch,
    OutputTooLarge,
    TrailingBytes,
    OutOfMemory,
}

const WINDOW_SIZE: usize = 32768;
const HASH_SIZE: usize = 32768;
const MAX_MATCH_LEN: usize = 258;
const MIN_MATCH_LEN: usize = 3;
const MAX_CHAIN: usize = 32;
const CHUNK_SIZE: usize = 65535;

/// Computes IEEE 802.3 CRC-32 checksum for `data`.
pub fn crc32(data: []const u8): u32 {
    let crc: u32 = 0xffffffff;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= data[i] as u32;
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xedb88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xffffffff;
}

/// Computes RFC 1950 Adler-32 checksum for `data`.
pub fn adler32(data: []const u8): u32 {
    const mod_adler: u32 = 65521;
    let s1: u32 = 1;
    let s2: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        let chunk_end = i + 5552;
        if (chunk_end > data.len) {
            chunk_end = data.len;
        }
        while (i < chunk_end) : (i += 1) {
            s1 += data[i] as u32;
            s2 += s1;
        }
        s1 %= mod_adler;
        s2 %= mod_adler;
    }
    return (s2 << 16) | s1;
}

fn readU16LE(buf: []const u8, pos: usize): u16 {
    return (buf[pos] as u16) | ((buf[pos + 1] as u16) << 8);
}

fn readU32LE(buf: []const u8, pos: usize): u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn readU32BE(buf: []const u8, pos: usize): u32 {
    return ((buf[pos] as u32) << 24) |
        ((buf[pos + 1] as u32) << 16) |
        ((buf[pos + 2] as u32) << 8) |
        (buf[pos + 3] as u32);
}

fn appendU16LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u16): void ! DeflateError {
    out.append(alloc, (v & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return DeflateError.OutOfMemory;
}

fn appendU32LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void ! DeflateError {
    out.append(alloc, (v & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, ((v >> 16) & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, ((v >> 24) & 0xff) as u8) catch return DeflateError.OutOfMemory;
}

fn appendU32BE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void ! DeflateError {
    out.append(alloc, ((v >> 24) & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, ((v >> 16) & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return DeflateError.OutOfMemory;
    out.append(alloc, (v & 0xff) as u8) catch return DeflateError.OutOfMemory;
}

struct BitReader {
    data: []const u8,
    byte_pos: usize = 0,
    bit_buf: u64 = 0,
    bit_count: u6 = 0,

    fn ensureBits(*_, n: u6): void ! DeflateError {
        while (_.bit_count < n) {
            if (_.byte_pos >= _.data.len) {
                return DeflateError.UnexpectedEof;
            }
            _.bit_buf |= (_.data[_.byte_pos] as u64) << _.bit_count;
            _.byte_pos += 1;
            _.bit_count += 8;
        }
    }

    fn readBits(*_, n: u6): u32 ! DeflateError {
        if (n == 0) {
            return 0;
        }
        _.ensureBits(n)?;
        const mask: u64 = ((1 as u64) << n) - 1;
        const val = (_.bit_buf & mask) as u32;
        _.bit_buf >>= n;
        _.bit_count -= n;
        return val;
    }

    fn alignAndRewind(*_) {
        const discard: u6 = (_.bit_count % 8) as u6;
        _.bit_buf >>= discard;
        _.bit_count -= discard;
        const whole_bytes = (_.bit_count / 8) as usize;
        _.byte_pos -= whole_bytes;
        _.bit_buf = 0;
        _.bit_count = 0;
    }
}

struct BitWriter {
    alloc: std.mem.Allocator,
    out: std.ArrayList(u8) = .empty,
    bit_buf: u64 = 0,
    bit_count: u6 = 0,

    fn freeAll(*_) {
        _.out.deinit(_.alloc);
    }

    fn writeBits(*_, val: u32, n: u6): void ! DeflateError {
        if (n == 0) {
            return;
        }
        _.bit_buf |= (val as u64) << _.bit_count;
        _.bit_count += n;
        while (_.bit_count >= 8) {
            _.out.append(_.alloc, (_.bit_buf & 0xff) as u8) catch return DeflateError.OutOfMemory;
            _.bit_buf >>= 8;
            _.bit_count -= 8;
        }
    }

    fn writeReversedBits(*_, code: u32, n: u6): void ! DeflateError {
        let rev: u32 = 0;
        let i: u6 = 0;
        let c = code;
        while (i < n) : (i += 1) {
            rev = (rev << 1) | (c & 1);
            c >>= 1;
        }
        _.writeBits(rev, n)?;
    }

    fn flushBits(*_): void ! DeflateError {
        if (_.bit_count > 0) {
            _.out.append(_.alloc, (_.bit_buf & 0xff) as u8) catch return DeflateError.OutOfMemory;
            _.bit_buf = 0;
            _.bit_count = 0;
        }
    }
}

struct HuffTree {
    counts: [16]u16 = [0; 16],
    symbols: [288]u16 = [0; 288],

    fn build(lengths: []const u8): HuffTree ! DeflateError {
        let tree = HuffTree{};
        let i: usize = 0;
        while (i < lengths.len) : (i += 1) {
            const l = lengths[i] as usize;
            if (l > 15) {
                return DeflateError.InvalidHuffmanCode;
            }
            if (l > 0) {
                tree.counts[l] += 1;
            }
        }

        let left: i32 = 1;
        let len: usize = 1;
        while (len <= 15) : (len += 1) {
            left <<= 1;
            left -= tree.counts[len] as i32;
            if (left < 0) {
                return DeflateError.InvalidHuffmanCode;
            }
        }

        let offsets: [16]u16 = [0; 16];
        len = 1;
        while (len < 15) : (len += 1) {
            offsets[len + 1] = offsets[len] + tree.counts[len];
        }

        i = 0;
        while (i < lengths.len) : (i += 1) {
            const l = lengths[i] as usize;
            if (l != 0) {
                const pos = offsets[l] as usize;
                offsets[l] += 1;
                tree.symbols[pos] = i as u16;
            }
        }

        return tree;
    }

    fn decode(br: *BitReader): u16 ! DeflateError {
        let code: i32 = 0;
        let first: i32 = 0;
        let index: i32 = 0;
        let len: usize = 1;
        while (len <= 15) : (len += 1) {
            const bit = br.readBits(1)?;
            code |= bit as i32;
            const count = _.counts[len] as i32;
            if (code - count < first) {
                return _.symbols[(index + (code - first)) as usize];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return DeflateError.InvalidHuffmanCode;
    }
}

const LEN_BASE: [29]u16 = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
];

const LEN_EXTRA: [29]u6 = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0,
];

const DIST_BASE: [30]u16 = [
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
];

const DIST_EXTRA: [30]u6 = [
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
];

const CL_ORDER: [19]usize = [
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
];

fn buildFixedLitTree(): HuffTree ! DeflateError {
    let lens: [288]u8 = [0; 288];
    let i: usize = 0;
    while (i <= 143) : (i += 1) {
        lens[i] = 8;
    }
    while (i <= 255) : (i += 1) {
        lens[i] = 9;
    }
    while (i <= 279) : (i += 1) {
        lens[i] = 7;
    }
    while (i <= 287) : (i += 1) {
        lens[i] = 8;
    }
    return HuffTree.build(lens[0..288]);
}

fn buildFixedDistTree(): HuffTree ! DeflateError {
    let lens: [32]u8 = [5; 32];
    return HuffTree.build(lens[0..32]);
}

fn decodeHuffmanBlock(
    alloc: std.mem.Allocator,
    br: *BitReader,
    out: *std.ArrayList(u8),
    lit_tree: *const HuffTree,
    dist_tree: *const HuffTree,
    max_bytes: usize,
): void ! DeflateError {
    while (true) {
        const sym = lit_tree.decode(br)?;
        if (sym < 256) {
            if (out.items.len >= max_bytes) {
                return DeflateError.OutputTooLarge;
            }
            out.append(alloc, sym as u8) catch return DeflateError.OutOfMemory;
        } else if (sym == 256) {
            break;
        } else {
            if (sym > 285) {
                return DeflateError.InvalidHuffmanCode;
            }
            const l_idx = (sym - 257) as usize;
            const extra_l = br.readBits(LEN_EXTRA[l_idx])?;
            const match_len = (LEN_BASE[l_idx] as usize) + (extra_l as usize);

            const d_sym = dist_tree.decode(br)?;
            if (d_sym >= 30) {
                return DeflateError.InvalidDistance;
            }
            const extra_d = br.readBits(DIST_EXTRA[d_sym])?;
            const dist = (DIST_BASE[d_sym] as usize) + (extra_d as usize);

            if (dist == 0 || dist > out.items.len) {
                return DeflateError.InvalidDistance;
            }
            if (match_len > max_bytes || out.items.len > max_bytes - match_len) {
                return DeflateError.OutputTooLarge;
            }

            let copy_i: usize = 0;
            while (copy_i < match_len) : (copy_i += 1) {
                const b = out.items[out.items.len - dist];
                out.append(alloc, b) catch return DeflateError.OutOfMemory;
            }
        }
    }
}

struct DecompressStreamResult {
    data: []u8,
    bytes_consumed: usize,
}

fn decompressDeflateStream(
    allocator: std.mem.Allocator,
    compressed: []const u8,
    max_bytes: usize,
): DecompressStreamResult ! DeflateError {
    let br = BitReader{ data: compressed };
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    let bfinal: u32 = 0;
    while (bfinal == 0) {
        bfinal = br.readBits(1)?;
        const btype = br.readBits(2)?;

        if (btype == 0) {
            br.alignAndRewind();
            if (br.byte_pos + 4 > compressed.len) {
                return DeflateError.UnexpectedEof;
            }
            const len = readU16LE(compressed, br.byte_pos) as usize;
            const nlen = readU16LE(compressed, br.byte_pos + 2) as usize;
            br.byte_pos += 4;
            if ((len ^ 0xffff) != nlen) {
                return DeflateError.InvalidStoredLength;
            }
            if (br.byte_pos + len > compressed.len) {
                return DeflateError.UnexpectedEof;
            }
            if (len > max_bytes || out.items.len > max_bytes - len) {
                return DeflateError.OutputTooLarge;
            }
            out.appendSlice(allocator, compressed[br.byte_pos .. (br.byte_pos + len)]) catch return DeflateError.OutOfMemory;
            br.byte_pos += len;
        } else if (btype == 1) {
            const lit_tree = buildFixedLitTree()?;
            const dist_tree = buildFixedDistTree()?;
            decodeHuffmanBlock(allocator, &br, &out, &lit_tree, &dist_tree, max_bytes)?;
        } else if (btype == 2) {
            const hlit = (br.readBits(5)? as usize) + 257;
            const hdist = (br.readBits(5)? as usize) + 1;
            const hclen = (br.readBits(4)? as usize) + 4;
            if (hlit > 286 || hdist > 32) {
                return DeflateError.InvalidHuffmanCode;
            }

            let cl_lens: [19]u8 = [0; 19];
            let i: usize = 0;
            while (i < hclen) : (i += 1) {
                cl_lens[CL_ORDER[i]] = br.readBits(3)? as u8;
            }
            const cl_tree = HuffTree.build(cl_lens[0..19])?;

            const total_codes = hlit + hdist;
            let all_lens: [320]u8 = [0; 320];
            let idx: usize = 0;
            while (idx < total_codes) {
                const sym = cl_tree.decode(&br)?;
                if (sym < 16) {
                    all_lens[idx] = sym as u8;
                    idx += 1;
                } else {
                    let rep_val: u8 = 0;
                    let rep_count: usize = 0;
                    if (sym == 16) {
                        if (idx == 0) {
                            return DeflateError.InvalidHuffmanCode;
                        }
                        rep_val = all_lens[idx - 1];
                        rep_count = (3 as usize) + (br.readBits(2)? as usize);
                    } else if (sym == 17) {
                        rep_val = 0;
                        rep_count = (3 as usize) + (br.readBits(3)? as usize);
                    } else if (sym == 18) {
                        rep_val = 0;
                        rep_count = (11 as usize) + (br.readBits(7)? as usize);
                    } else {
                        return DeflateError.InvalidHuffmanCode;
                    }
                    if (idx + rep_count > total_codes) {
                        return DeflateError.InvalidHuffmanCode;
                    }
                    let r: usize = 0;
                    while (r < rep_count) : (r += 1) {
                        all_lens[idx] = rep_val;
                        idx += 1;
                    }
                }
            }

            if (all_lens[256] == 0) {
                return DeflateError.InvalidHuffmanCode;
            }

            const lit_tree = HuffTree.build(all_lens[0..hlit])?;
            const dist_tree = HuffTree.build(all_lens[hlit..total_codes])?;
            decodeHuffmanBlock(allocator, &br, &out, &lit_tree, &dist_tree, max_bytes)?;
        } else {
            return DeflateError.InvalidBlockType;
        }
    }

    br.alignAndRewind();
    const consumed = br.byte_pos;
    const owned = out.toOwnedSlice(allocator) catch return DeflateError.OutOfMemory;
    return DecompressStreamResult{
        data: owned,
        bytes_consumed: consumed,
    };
}

/// Decompresses a raw RFC 1951 DEFLATE stream with a maximum output size limit.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompressDeflateWithMaxSize(
    allocator: std.mem.Allocator,
    compressed: []const u8,
    max_bytes: usize,
): ([]u8) ! DeflateError {
    const res = decompressDeflateStream(allocator, compressed, max_bytes)?;
    if (res.bytes_consumed < compressed.len) {
        allocator.free(res.data);
        return DeflateError.TrailingBytes;
    }
    return res.data;
}

/// Decompresses a raw RFC 1951 DEFLATE stream without an explicit custom size cap.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompressDeflate(allocator: std.mem.Allocator, compressed: []const u8): ([]u8) ! DeflateError {
    return decompressDeflateWithMaxSize(allocator, compressed, std.math.maxInt(usize));
}

fn writeFixedLit(bw: *BitWriter, sym: u16): void ! DeflateError {
    if (sym <= 143) {
        bw.writeReversedBits(0x30 + (sym as u32), 8)?;
    } else if (sym <= 255) {
        bw.writeReversedBits(0x190 + ((sym - 144) as u32), 9)?;
    } else if (sym <= 279) {
        bw.writeReversedBits((sym - 256) as u32, 7)?;
    } else {
        bw.writeReversedBits(0xc0 + ((sym - 280) as u32), 8)?;
    }
}

fn writeFixedMatch(bw: *BitWriter, match_len: usize, dist: usize): void ! DeflateError {
    let l_idx: usize = 0;
    while (l_idx < 28) : (l_idx += 1) {
        const base = LEN_BASE[l_idx] as usize;
        const max_l = base + ((1 as usize) << LEN_EXTRA[l_idx]) - 1;
        if (match_len <= max_l) {
            break;
        }
    }
    writeFixedLit(bw, (257 + l_idx) as u16)?;
    const l_extra = LEN_EXTRA[l_idx];
    if (l_extra > 0) {
        bw.writeBits((match_len - (LEN_BASE[l_idx] as usize)) as u32, l_extra)?;
    }

    let d_idx: usize = 0;
    while (d_idx < 29) : (d_idx += 1) {
        const base = DIST_BASE[d_idx] as usize;
        const max_d = base + ((1 as usize) << DIST_EXTRA[d_idx]) - 1;
        if (dist <= max_d) {
            break;
        }
    }
    bw.writeReversedBits(d_idx as u32, 5)?;
    const d_extra = DIST_EXTRA[d_idx];
    if (d_extra > 0) {
        bw.writeBits((dist - (DIST_BASE[d_idx] as usize)) as u32, d_extra)?;
    }
}

fn hash3(data: []const u8, pos: usize): usize {
    const h = ((data[pos] as usize) << 10) ^
        ((data[pos + 1] as usize) << 5) ^
        (data[pos + 2] as usize);
    return h & (HASH_SIZE - 1);
}

fn compressFixedChunk(
    alloc: std.mem.Allocator,
    chunk: []const u8,
    is_final: bool,
    head: []i32,
    prev: []i32,
): ([]u8) ! DeflateError {
    let bw = BitWriter{ alloc: alloc };
    errdefer bw.freeAll();

    bw.writeBits(if (is_final) 1 else 0, 1)?;
    bw.writeBits(1, 2)?; // BTYPE=01 (Fixed Huffman)

    @memset(head, -1);

    let pos: usize = 0;
    while (pos < chunk.len) {
        let best_len: usize = 0;
        let best_dist: usize = 0;

        if (pos + MIN_MATCH_LEN <= chunk.len) {
            const h = hash3(chunk, pos);
            let cand = head[h];
            prev[pos] = cand;
            head[h] = pos as i32;

            let steps: usize = 0;
            while (cand >= 0 && steps < MAX_CHAIN) : (steps += 1) {
                const cand_pos = cand as usize;
                const dist = pos - cand_pos;
                if (dist > WINDOW_SIZE) {
                    break;
                }
                let max_l = chunk.len - pos;
                if (max_l > MAX_MATCH_LEN) {
                    max_l = MAX_MATCH_LEN;
                }
                if (max_l >= MIN_MATCH_LEN &&
                    chunk[cand_pos] == chunk[pos] &&
                    chunk[cand_pos + 1] == chunk[pos + 1] &&
                    chunk[cand_pos + 2] == chunk[pos + 2])
                {
                    let m_len: usize = 3;
                    while (m_len < max_l && chunk[cand_pos + m_len] == chunk[pos + m_len]) : (m_len += 1) {}
                    if (m_len > best_len) {
                        best_len = m_len;
                        best_dist = dist;
                        if (best_len == MAX_MATCH_LEN) {
                            break;
                        }
                    }
                }
                cand = prev[cand_pos];
            }
        }

        if (best_len >= MIN_MATCH_LEN) {
            writeFixedMatch(&bw, best_len, best_dist)?;
            let skip: usize = 1;
            while (skip < best_len) : (skip += 1) {
                const p = pos + skip;
                if (p + MIN_MATCH_LEN <= chunk.len) {
                    const h2 = hash3(chunk, p);
                    prev[p] = head[h2];
                    head[h2] = p as i32;
                }
            }
            pos += best_len;
        } else {
            writeFixedLit(&bw, chunk[pos] as u16)?;
            pos += 1;
        }
    }

    writeFixedLit(&bw, 256)?; // EOB
    bw.flushBits()?;
    return bw.out.toOwnedSlice(alloc) catch return DeflateError.OutOfMemory;
}

/// Compresses `input` into a raw RFC 1951 DEFLATE stream using LZ77 + Fixed Huffman / Stored fallback.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn compressDeflate(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! DeflateError {
    if (input.len == 0) {
        let out = allocator.alloc(u8, 2) catch return DeflateError.OutOfMemory;
        out[0] = 0x03;
        out[1] = 0x00;
        return out;
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    const head = allocator.alloc(i32, HASH_SIZE) catch return DeflateError.OutOfMemory;
    defer allocator.free(head);
    const prev_len = if (input.len < CHUNK_SIZE) input.len else CHUNK_SIZE;
    const prev = allocator.alloc(i32, prev_len) catch return DeflateError.OutOfMemory;
    defer allocator.free(prev);

    let offset: usize = 0;
    while (offset < input.len) {
        let end = offset + CHUNK_SIZE;
        if (end > input.len) {
            end = input.len;
        }
        const chunk = input[offset..end];
        const is_final = (end == input.len);

        const fixed_buf = compressFixedChunk(allocator, chunk, is_final, head, prev[0..chunk.len])?;
        defer allocator.free(fixed_buf);

        if (fixed_buf.len <= 5 + chunk.len) {
            out.appendSlice(allocator, fixed_buf) catch return DeflateError.OutOfMemory;
        } else {
            const hdr_byte: u8 = if (is_final) 0x01 else 0x00;
            out.append(allocator, hdr_byte) catch return DeflateError.OutOfMemory;
            const len16 = chunk.len as u16;
            const nlen16 = len16 ^ 0xffff;
            appendU16LE(allocator, &out, len16)?;
            appendU16LE(allocator, &out, nlen16)?;
            out.appendSlice(allocator, chunk) catch return DeflateError.OutOfMemory;
        }
        offset = end;
    }

    return out.toOwnedSlice(allocator) catch return DeflateError.OutOfMemory;
}

/// Compresses `input` into an RFC 1952 GZIP container.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn compressGzip(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! DeflateError {
    const deflated = compressDeflate(allocator, input)?;
    defer allocator.free(deflated);

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    const hdr: [10]u8 = [0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff];
    out.appendSlice(allocator, hdr[0..10]) catch return DeflateError.OutOfMemory;
    out.appendSlice(allocator, deflated) catch return DeflateError.OutOfMemory;

    const crc = crc32(input);
    const input_isize = (input.len & 0xffffffff) as u32;
    appendU32LE(allocator, &out, crc)?;
    appendU32LE(allocator, &out, input_isize)?;

    return out.toOwnedSlice(allocator) catch return DeflateError.OutOfMemory;
}

/// Decompresses an RFC 1952 GZIP container and verifies CRC-32 and ISIZE.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompressGzip(allocator: std.mem.Allocator, gz_bytes: []const u8): ([]u8) ! DeflateError {
    if (gz_bytes.len < 10) {
        return DeflateError.UnexpectedEof;
    }
    if (gz_bytes[0] != 0x1f || gz_bytes[1] != 0x8b || gz_bytes[2] != 0x08) {
        return DeflateError.InvalidHeader;
    }
    const flg = gz_bytes[3];
    if ((flg & 0xe0) != 0) {
        return DeflateError.InvalidHeader;
    }

    let pos: usize = 10;

    // FEXTRA (bit 2)
    if ((flg & 0x04) != 0) {
        if (pos + 2 > gz_bytes.len) {
            return DeflateError.UnexpectedEof;
        }
        const xlen = readU16LE(gz_bytes, pos) as usize;
        pos += 2;
        if (pos + xlen > gz_bytes.len) {
            return DeflateError.UnexpectedEof;
        }
        pos += xlen;
    }

    // FNAME (bit 3)
    if ((flg & 0x08) != 0) {
        while (pos < gz_bytes.len && gz_bytes[pos] != 0) : (pos += 1) {}
        if (pos >= gz_bytes.len) {
            return DeflateError.UnexpectedEof;
        }
        pos += 1;
    }

    // FCOMMENT (bit 4)
    if ((flg & 0x10) != 0) {
        while (pos < gz_bytes.len && gz_bytes[pos] != 0) : (pos += 1) {}
        if (pos >= gz_bytes.len) {
            return DeflateError.UnexpectedEof;
        }
        pos += 1;
    }

    // FHCRC (bit 1)
    if ((flg & 0x02) != 0) {
        if (pos + 2 > gz_bytes.len) {
            return DeflateError.UnexpectedEof;
        }
        pos += 2;
    }

    if (pos + 8 > gz_bytes.len) {
        return DeflateError.UnexpectedEof;
    }

    const res = decompressDeflateStream(allocator, gz_bytes[pos..], std.math.maxInt(usize))?;
    const trailer_pos = pos + res.bytes_consumed;
    if (trailer_pos + 8 > gz_bytes.len) {
        allocator.free(res.data);
        return DeflateError.UnexpectedEof;
    }
    if (trailer_pos + 8 < gz_bytes.len) {
        allocator.free(res.data);
        return DeflateError.TrailingBytes;
    }

    const expected_crc = readU32LE(gz_bytes, trailer_pos);
    const expected_isize = readU32LE(gz_bytes, trailer_pos + 4);
    const actual_crc = crc32(res.data);
    const actual_isize = (res.data.len & 0xffffffff) as u32;

    if (actual_crc != expected_crc || actual_isize != expected_isize) {
        allocator.free(res.data);
        return DeflateError.ChecksumMismatch;
    }

    return res.data;
}

/// Compresses `input` into an RFC 1950 ZLIB container.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn compressZlib(allocator: std.mem.Allocator, input: []const u8): ([]u8) ! DeflateError {
    const deflated = compressDeflate(allocator, input)?;
    defer allocator.free(deflated);

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    const hdr: [2]u8 = [0x78, 0x9c];
    out.appendSlice(allocator, hdr[0..2]) catch return DeflateError.OutOfMemory;
    out.appendSlice(allocator, deflated) catch return DeflateError.OutOfMemory;

    const adler = adler32(input);
    appendU32BE(allocator, &out, adler)?;

    return out.toOwnedSlice(allocator) catch return DeflateError.OutOfMemory;
}

/// Decompresses an RFC 1950 ZLIB container and verifies the big-endian Adler-32 checksum.
/// Caller owns the returned slice and must free it with `allocator.free(...)`.
pub fn decompressZlib(allocator: std.mem.Allocator, zlib_bytes: []const u8): ([]u8) ! DeflateError {
    if (zlib_bytes.len < 2) {
        return DeflateError.UnexpectedEof;
    }
    const cmf = zlib_bytes[0] as u32;
    const flg = zlib_bytes[1] as u32;

    if ((cmf & 0x0f) != 8 || (cmf >> 4) > 7 || ((cmf * 256 + flg) % 31) != 0 || (flg & 0x20) != 0) {
        return DeflateError.InvalidHeader;
    }

    const res = decompressDeflateStream(allocator, zlib_bytes[2..], std.math.maxInt(usize))?;
    const trailer_pos = 2 + res.bytes_consumed;
    if (trailer_pos + 4 > zlib_bytes.len) {
        allocator.free(res.data);
        return DeflateError.UnexpectedEof;
    }
    if (trailer_pos + 4 < zlib_bytes.len) {
        allocator.free(res.data);
        return DeflateError.TrailingBytes;
    }

    const expected_adler = readU32BE(zlib_bytes, trailer_pos);
    const actual_adler = adler32(res.data);
    if (actual_adler != expected_adler) {
        allocator.free(res.data);
        return DeflateError.ChecksumMismatch;
    }

    return res.data;
}
