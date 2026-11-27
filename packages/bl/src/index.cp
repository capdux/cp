// @cap/bl — Zero-heap segmented BufferList in pure CP
// Clean-room MIT implementation of npm `bl` (#450, 123M/wk).

pub const BL_MAX_CHUNKS: usize = 64;

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
        if (offset + 2 > _.total_len) { return null; }
        const b0 = (_.get(offset) ?? 0) as u16;
        const b1 = (_.get(offset + 1) ?? 0) as u16;
        return (b0 << 8) | b1;
    }

    /// Reads an unsigned 16-bit little-endian integer at `offset`.
    pub fn readU16LE(offset: usize): u16? {
        if (offset + 2 > _.total_len) { return null; }
        const b0 = (_.get(offset) ?? 0) as u16;
        const b1 = (_.get(offset + 1) ?? 0) as u16;
        return b0 | (b1 << 8);
    }

    /// Reads an unsigned 32-bit big-endian integer at `offset`.
    pub fn readU32BE(offset: usize): u32? {
        if (offset + 4 > _.total_len) { return null; }
        const b0 = (_.get(offset) ?? 0) as u32;
        const b1 = (_.get(offset + 1) ?? 0) as u32;
        const b2 = (_.get(offset + 2) ?? 0) as u32;
        const b3 = (_.get(offset + 3) ?? 0) as u32;
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    /// Reads an unsigned 32-bit little-endian integer at `offset`.
    pub fn readU32LE(offset: usize): u32? {
        if (offset + 4 > _.total_len) { return null; }
        const b0 = (_.get(offset) ?? 0) as u32;
        const b1 = (_.get(offset + 1) ?? 0) as u32;
        const b2 = (_.get(offset + 2) ?? 0) as u32;
        const b3 = (_.get(offset + 3) ?? 0) as u32;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    /// Finds the first logical index >= `from_offset` where `needle` appears across chunk boundaries.
    pub fn indexOf(needle: []const u8, from_offset: usize): usize? {
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
    const empty: []const u8 = "";
    return BufferList {
        chunks: [empty; 64],
        chunk_count: 0,
        total_len: 0,
    };
}

export {
    BL_MAX_CHUNKS,
    BufferList,
    newBufferList,
};
