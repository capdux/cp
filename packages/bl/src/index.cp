// @cap/bl — Zero-heap segmented BufferList in pure CP
// Clean-room MIT implementation of npm `bl` (#450, 123M/wk).

import { indexOfBytes, readU16BE, readU16LE, readU32BE, readU32LE } from "cap:bin";

pub const BL_MAX_CHUNKS: usize = 64;

fn blReadU16BE(src: []const u8): u16? { return readU16BE(src, 0); }
fn blReadU16LE(src: []const u8): u16? { return readU16LE(src, 0); }
fn blReadU32BE(src: []const u8): u32? { return readU32BE(src, 0); }
fn blReadU32LE(src: []const u8): u32? { return readU32LE(src, 0); }

pub struct BufferList {
    pub chunks: [64][]const u8,
    pub chunk_count: usize,
    pub total_len: usize,

    pub fn len(): usize {
        return _.total_len;
    }

    pub fn isEmpty(): bool {
        return _.total_len == 0;
    }

    /// Appends a byte slice chunk without copying. Returns `false` if chunk table is full.
    pub fn append(*_, chunk: []const u8): bool {
        if (chunk.len == 0) {
            return true;
        }
        if (_.chunk_count >= BL_MAX_CHUNKS) {
            return false;
        }
        _.chunks[_.chunk_count] = chunk;
        _.chunk_count += 1;
        _.total_len += chunk.len;
        return true;
    }

    /// Returns the byte at logical `index`, or `null` if out of bounds.
    pub fn get(index: usize): u8? {
        if (index >= _.total_len) {
            return null;
        }
        let rem = index;
        let i: usize = 0;
        while (i < _.chunk_count) {
            const c = _.chunks[i];
            if (rem < c.len) {
                return c[rem];
            }
            rem -= c.len;
            i += 1;
        }
        return null;
    }

    /// Copies bytes in `[start..end)` into `dst`; returns number of bytes copied.
    pub fn copyRange(start: usize, end: usize, dst: []u8): usize {
        if (start >= end || start >= _.total_len) {
            return 0;
        }
        let actual_end = end;
        if (actual_end > _.total_len) {
            actual_end = _.total_len;
        }
        let need = actual_end - start;
        if (need > dst.len) {
            need = dst.len;
        }
        let pos: usize = 0;
        let offset = start;
        let i: usize = 0;
        while (i < _.chunk_count && pos < need) {
            const c = _.chunks[i];
            if (offset >= c.len) {
                offset -= c.len;
            } else {
                let j = offset;
                while (j < c.len && pos < need) {
                    dst[pos] = c[j];
                    pos += 1;
                    j += 1;
                }
                offset = 0;
            }
            i += 1;
        }
        return pos;
    }

    /// Consumes (drops) `n` bytes from the front of the BufferList in O(chunks).
    pub fn consume(*_, n: usize) {
        if (n == 0 || _.total_len == 0) {
            return;
        }
        if (n >= _.total_len) {
            _.chunk_count = 0;
            _.total_len = 0;
            return;
        }
        let rem = n;
        let drop_chunks: usize = 0;
        while (drop_chunks < _.chunk_count && rem >= _.chunks[drop_chunks].len) {
            rem -= _.chunks[drop_chunks].len;
            drop_chunks += 1;
        }
        if (drop_chunks > 0) {
            let k: usize = 0;
            const keep = _.chunk_count - drop_chunks;
            while (k < keep) {
                _.chunks[k] = _.chunks[k + drop_chunks];
                k += 1;
            }
            _.chunk_count = keep;
        }
        if (rem > 0 && _.chunk_count > 0) {
            const head = _.chunks[0];
            _.chunks[0] = head[rem..head.len];
        }
        _.total_len -= n;
    }

    /// Reads an unsigned 16-bit big-endian integer at `offset`.
    pub fn readU16BE(offset: usize): u16? {
        let tmp: [2]u8 = undefined;
        if (_.copyRange(offset, offset + 2, tmp[0..2]) < 2) { return null; }
        return blReadU16BE(tmp[0..2]);
    }

    /// Reads an unsigned 16-bit little-endian integer at `offset`.
    pub fn readU16LE(offset: usize): u16? {
        let tmp: [2]u8 = undefined;
        if (_.copyRange(offset, offset + 2, tmp[0..2]) < 2) { return null; }
        return blReadU16LE(tmp[0..2]);
    }

    /// Reads an unsigned 32-bit big-endian integer at `offset`.
    pub fn readU32BE(offset: usize): u32? {
        let tmp: [4]u8 = undefined;
        if (_.copyRange(offset, offset + 4, tmp[0..4]) < 4) { return null; }
        return blReadU32BE(tmp[0..4]);
    }

    /// Reads an unsigned 32-bit little-endian integer at `offset`.
    pub fn readU32LE(offset: usize): u32? {
        let tmp: [4]u8 = undefined;
        if (_.copyRange(offset, offset + 4, tmp[0..4]) < 4) { return null; }
        return blReadU32LE(tmp[0..4]);
    }

    /// Finds the first logical index >= `from_offset` where `needle` appears across chunk boundaries.
    pub fn indexOf(needle: []const u8, from_offset: usize): usize? {
        if (_.chunk_count == 1) {
            return indexOfBytes(_.chunks[0], needle, from_offset);
        }
        if (needle.len == 0) {
            if (from_offset <= _.total_len) { return from_offset; }
            return null;
        }
        if (from_offset + needle.len > _.total_len) {
            return null;
        }
        const limit = _.total_len - needle.len;
        let i = from_offset;
        while (i <= limit) {
            let ok = true;
            let j: usize = 0;
            while (j < needle.len) {
                if ((_.get(i + j) ?? 0) != needle[j]) {
                    ok = false;
                    break;
                }
                j += 1;
            }
            if (ok) {
                return i;
            }
            i += 1;
        }
        return null;
    }
}

/// Creates an empty `BufferList`.
pub fn newBufferList(): BufferList {
    return BufferList {
        chunks: undefined,
        chunk_count: 0,
        total_len: 0,
    };
}

export {
    BL_MAX_CHUNKS,
    BufferList,
    newBufferList,
};
