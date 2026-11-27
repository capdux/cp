// @cap/ucd-trie — Pure CP zero-allocation compressed two-level bitmap Trie for Unicode codepoints
// Parity with Rust `ucd-trie` (64-codepoint chunks, shared zero block, O(1) 2-load membership lookup).

const UT_UNICODE_MAX: u32 = 0x10FFFF;

fn utPopcount64(x: u64): u64 {
    let w = x;
    let cnt: u64 = 0;
    while (w != 0) {
        w = w & (w - 1);
        cnt = cnt + 1;
    }
    return cnt;
}

fn utAllOnes64(): u64 {
    return ((0xFFFFFFFF as u64) << (32 as u6)) | (0xFFFFFFFF as u64);
}

pub struct UcdTrie {
    pub max_cp: u32,
    pub chunk_count: usize,

    pub static fn new(): UcdTrie {
        return UcdTrie {
            max_cp: 0,
            chunk_count: 0,
        };
    }

    pub fn init(*_, level1: []u16, chunks: []u64): bool {
        if (level1.len == 0 || chunks.len == 0) {
            _.max_cp = 0;
            _.chunk_count = 0;
            return false;
        }
        let i: usize = 0;
        while (i < level1.len) {
            level1[i] = 0;
            i = i + 1;
        }
        // chunks[0] is permanently the shared all-zero empty block
        chunks[0] = 0;
        _.chunk_count = 1;

        let total_cps: u64 = (level1.len as u64) * 64;
        if (total_cps > 1114112) {
            _.max_cp = UT_UNICODE_MAX;
        } else {
            _.max_cp = (total_cps - 1) as u32;
        }
        return true;
    }

    pub fn insertCodepoint(*_, level1: []u16, chunks: []u64, cp: u32): bool {
        if (_.chunk_count == 0 || cp > _.max_cp) {
            return false;
        }
        let l1_idx: usize = (cp >> 6) as usize;
        if (l1_idx >= level1.len) {
            return false;
        }
        let bit_idx: u6 = (cp & 63) as u6;
        let mask: u64 = (1 as u64) << bit_idx;
        let cid: usize = level1[l1_idx] as usize;
        if (cid == 0) {
            if (_.chunk_count >= chunks.len || _.chunk_count > 65535) {
                return false;
            }
            cid = _.chunk_count;
            _.chunk_count = _.chunk_count + 1;
            chunks[cid] = mask;
            level1[l1_idx] = cid as u16;
            return true;
        }
        if (cid >= chunks.len) {
            return false;
        }
        chunks[cid] = chunks[cid] | mask;
        return true;
    }

    pub fn insertRange(*_, level1: []u16, chunks: []u64, lo: u32, hi: u32): bool {
        if (_.chunk_count == 0 || lo > hi || hi > _.max_cp) {
            return false;
        }
        let c_lo: u32 = lo >> 6;
        let c_hi: u32 = hi >> 6;
        let all_ones = utAllOnes64();

        let c: u32 = c_lo;
        while (c <= c_hi) {
            let l1_idx: usize = c as usize;
            if (l1_idx >= level1.len) {
                return false;
            }
            let b_start: u32 = 0;
            if (c == c_lo) {
                b_start = lo & 63;
            }
            let b_end: u32 = 63;
            if (c == c_hi) {
                b_end = hi & 63;
            }
            let width: u32 = (b_end - b_start) + 1;
            let mask: u64 = all_ones;
            if (width < 64) {
                mask = (((1 as u64) << (width as u6)) - (1 as u64)) << (b_start as u6);
            }

            let cid: usize = level1[l1_idx] as usize;
            if (cid == 0) {
                if (_.chunk_count >= chunks.len || _.chunk_count > 65535) {
                    return false;
                }
                cid = _.chunk_count;
                _.chunk_count = _.chunk_count + 1;
                chunks[cid] = mask;
                level1[l1_idx] = cid as u16;
            } else {
                if (cid >= chunks.len) {
                    return false;
                }
                chunks[cid] = chunks[cid] | mask;
            }
            c = c + 1;
        }
        return true;
    }

    pub fn contains(level1: []u16, chunks: []u64, cp: u32): bool {
        if (_.chunk_count == 0 || cp > _.max_cp) {
            return false;
        }
        let l1_idx: usize = (cp >> 6) as usize;
        if (l1_idx >= level1.len) {
            return false;
        }
        let cid: usize = level1[l1_idx] as usize;
        if (cid >= chunks.len) {
            return false;
        }
        let bit_idx: u6 = (cp & 63) as u6;
        return ((chunks[cid] >> bit_idx) & (1 as u64)) != (0 as u64);
    }

    pub fn countMembers(level1: []u16, chunks: []u64): u64 {
        if (_.chunk_count == 0) {
            return 0;
        }
        let total: u64 = 0;
        let i: usize = 0;
        while (i < level1.len) {
            let cid: usize = level1[i] as usize;
            if (cid != 0 && cid < chunks.len) {
                total = total + utPopcount64(chunks[cid]);
            }
            i = i + 1;
        }
        return total;
    }

    pub fn dedupChunkCount(): usize {
        if (_.chunk_count <= 1) {
            return 0;
        }
        return _.chunk_count - (1 as usize);
    }
}

export {
    UcdTrie
};
