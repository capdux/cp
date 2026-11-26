// @cap/roaring-bitmap — Zero-allocation compressed roaring bitmap for CP
// Clean-room MIT implementation inspired by Rust `roaring` v0.10 (450M+ downloads).
// High-16-bit sorted chunk keys + low-16-bit sorted sparse containers covering the full u32 domain.

pub const RBM_MAX_CHUNKS: usize = 16;
pub const RBM_CHUNK_CAP: usize = 64;
const RBM_TOTAL_SLOTS: usize = 1024;

fn rbmHigh16(val: u32) -> u16 {
    return ((val / (65536 as u32)) & (0xFFFF as u32)) as u16;
}

fn rbmLow16(val: u32) -> u16 {
    return (val & (0xFFFF as u32)) as u16;
}

fn rbmCombine(hi: u16, lo: u16) -> u32 {
    return ((hi as u32) * (65536 as u32)) | (lo as u32);
}

/// Zero-allocation roaring bitmap supporting arbitrary `u32` values (`0..=u32::MAX`).
/// Stores up to 16 high-16-bit chunks, each holding up to 64 sorted low-16-bit elements.
pub struct RoaringBitmap {
    pub chunk_keys: [16]u16,
    pub chunk_lens: [16]usize,
    pub vals: [1024]u16,
    pub chunk_count: usize,
    pub total_len: u64,

    pub static fn new() -> RoaringBitmap {
        let keys: [16]u16 = undefined;
        let lens: [16]usize = undefined;
        let vs: [1024]u16 = undefined;
        let i: usize = 0;
        while (i < RBM_MAX_CHUNKS) {
            keys[i] = 0 as u16;
            lens[i] = 0 as usize;
            i += 1;
        }
        let j: usize = 0;
        while (j < RBM_TOTAL_SLOTS) {
            vs[j] = 0 as u16;
            j += 1;
        }
        return RoaringBitmap{
            chunk_keys: keys,
            chunk_lens: lens,
            vals: vs,
            chunk_count: 0 as usize,
            total_len: 0 as u64,
        };
    }

    pub fn len() -> u64 {
        return _.total_len;
    }

    pub fn isEmpty() -> bool {
        return _.total_len == (0 as u64);
    }

    pub fn contains(val: u32) -> bool {
        if (_.chunk_count == (0 as usize)) {
            return false;
        }
        let hi = rbmHigh16(val);
        let lo = rbmLow16(val);

        // Binary search chunk_keys[0..chunk_count]
        let left: usize = 0;
        let right = _.chunk_count;
        while (left < right) {
            let mid = left + (right - left) / (2 as usize);
            if (_.chunk_keys[mid] < hi) {
                left = mid + (1 as usize);
            } else {
                right = mid;
            }
        }
        if (left >= _.chunk_count || _.chunk_keys[left] != hi) {
            return false;
        }

        // Binary search vals[base .. base + clen]
        let base = left * RBM_CHUNK_CAP;
        let clen = _.chunk_lens[left];
        let vl: usize = 0;
        let vr = clen;
        while (vl < vr) {
            let mid = vl + (vr - vl) / (2 as usize);
            if (_.vals[base + mid] < lo) {
                vl = mid + (1 as usize);
            } else {
                vr = mid;
            }
        }
        return vl < clen && _.vals[base + vl] == lo;
    }

    pub fn insert(*_, val: u32) -> bool {
        let hi = rbmHigh16(val);
        let lo = rbmLow16(val);

        // Locate chunk insertion/match index
        let ci: usize = 0;
        let cr = _.chunk_count;
        while (ci < cr) {
            let mid = ci + (cr - ci) / (2 as usize);
            if (_.chunk_keys[mid] < hi) {
                ci = mid + (1 as usize);
            } else {
                cr = mid;
            }
        }

        if (ci < _.chunk_count && _.chunk_keys[ci] == hi) {
            // Existing chunk: locate low-16 position
            let base = ci * RBM_CHUNK_CAP;
            let clen = _.chunk_lens[ci];
            let pos: usize = 0;
            let pr = clen;
            while (pos < pr) {
                let mid = pos + (pr - pos) / (2 as usize);
                if (_.vals[base + mid] < lo) {
                    pos = mid + (1 as usize);
                } else {
                    pr = mid;
                }
            }
            if (pos < clen && _.vals[base + pos] == lo) {
                return false;
            }
            if (clen >= RBM_CHUNK_CAP) {
                return false;
            }
            let k = clen;
            while (k > pos) {
                _.vals[base + k] = _.vals[base + k - (1 as usize)];
                k -= 1;
            }
            _.vals[base + pos] = lo;
            _.chunk_lens[ci] = clen + (1 as usize);
            _.total_len += 1;
            return true;
        }

        // New chunk needed
        if (_.chunk_count >= RBM_MAX_CHUNKS) {
            return false;
        }
        let move_c = _.chunk_count;
        while (move_c > ci) {
            let src_c = move_c - (1 as usize);
            _.chunk_keys[move_c] = _.chunk_keys[src_c];
            let slen = _.chunk_lens[src_c];
            _.chunk_lens[move_c] = slen;
            let dst_base = move_c * RBM_CHUNK_CAP;
            let src_base = src_c * RBM_CHUNK_CAP;
            let m: usize = 0;
            while (m < slen) {
                _.vals[dst_base + m] = _.vals[src_base + m];
                m += 1;
            }
            move_c -= 1;
        }

        _.chunk_keys[ci] = hi;
        _.chunk_lens[ci] = 1 as usize;
        _.vals[ci * RBM_CHUNK_CAP] = lo;
        _.chunk_count += 1;
        _.total_len += 1;
        return true;
    }

    pub fn remove(*_, val: u32) -> bool {
        if (_.chunk_count == (0 as usize)) {
            return false;
        }
        let hi = rbmHigh16(val);
        let lo = rbmLow16(val);

        let ci: usize = 0;
        let cr = _.chunk_count;
        while (ci < cr) {
            let mid = ci + (cr - ci) / (2 as usize);
            if (_.chunk_keys[mid] < hi) {
                ci = mid + (1 as usize);
            } else {
                cr = mid;
            }
        }
        if (ci >= _.chunk_count || _.chunk_keys[ci] != hi) {
            return false;
        }

        let base = ci * RBM_CHUNK_CAP;
        let clen = _.chunk_lens[ci];
        let pos: usize = 0;
        let pr = clen;
        while (pos < pr) {
            let mid = pos + (pr - pos) / (2 as usize);
            if (_.vals[base + mid] < lo) {
                pos = mid + (1 as usize);
            } else {
                pr = mid;
            }
        }
        if (pos >= clen || _.vals[base + pos] != lo) {
            return false;
        }

        let k = pos;
        while (k + (1 as usize) < clen) {
            _.vals[base + k] = _.vals[base + k + (1 as usize)];
            k += 1;
        }
        _.chunk_lens[ci] = clen - (1 as usize);
        _.total_len -= 1;

        // Compact away empty chunk so min()/max() stay O(1)
        if (_.chunk_lens[ci] == (0 as usize)) {
            let c = ci;
            while (c + (1 as usize) < _.chunk_count) {
                let next_c = c + (1 as usize);
                _.chunk_keys[c] = _.chunk_keys[next_c];
                let nlen = _.chunk_lens[next_c];
                _.chunk_lens[c] = nlen;
                let dst_base = c * RBM_CHUNK_CAP;
                let src_base = next_c * RBM_CHUNK_CAP;
                let m: usize = 0;
                while (m < nlen) {
                    _.vals[dst_base + m] = _.vals[src_base + m];
                    m += 1;
                }
                c += 1;
            }
            _.chunk_count -= 1;
            _.chunk_keys[_.chunk_count] = 0 as u16;
            _.chunk_lens[_.chunk_count] = 0 as usize;
        }
        return true;
    }

    pub fn min() -> u32? {
        if (_.chunk_count == (0 as usize)) {
            return null;
        }
        return rbmCombine(_.chunk_keys[0], _.vals[0]);
    }

    pub fn max() -> u32? {
        if (_.chunk_count == (0 as usize)) {
            return null;
        }
        let last_c = _.chunk_count - (1 as usize);
        let last_len = _.chunk_lens[last_c];
        return rbmCombine(_.chunk_keys[last_c], _.vals[last_c * RBM_CHUNK_CAP + last_len - (1 as usize)]);
    }

    pub fn rank(val: u32) -> u64 {
        let hi = rbmHigh16(val);
        let lo = rbmLow16(val);
        let acc: u64 = 0;
        let c: usize = 0;
        while (c < _.chunk_count) {
            let ck = _.chunk_keys[c];
            if (ck < hi) {
                acc += _.chunk_lens[c] as u64;
            } else if (ck == hi) {
                // Upper bound for <= lo
                let base = c * RBM_CHUNK_CAP;
                let left: usize = 0;
                let right = _.chunk_lens[c];
                while (left < right) {
                    let mid = left + (right - left) / (2 as usize);
                    if (_.vals[base + mid] <= lo) {
                        left = mid + (1 as usize);
                    } else {
                        right = mid;
                    }
                }
                acc += left as u64;
                break;
            } else {
                break;
            }
            c += 1;
        }
        return acc;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.chunk_count) {
            _.chunk_keys[i] = 0 as u16;
            _.chunk_lens[i] = 0 as usize;
            i += 1;
        }
        _.chunk_count = 0 as usize;
        _.total_len = 0 as u64;
    }

    pub fn insertRange(*_, start: u32, end: u32) -> u64 {
        if (start >= end) {
            return 0 as u64;
        }
        let inserted: u64 = 0;
        let cur = start;
        while (cur < end) {
            if (_.insert(cur)) {
                inserted += 1;
            }
            cur += 1;
        }
        return inserted;
    }

    pub fn unionWith(*_, other: RoaringBitmap) {
        let c: usize = 0;
        while (c < other.chunk_count) {
            let hi = other.chunk_keys[c];
            let base = c * RBM_CHUNK_CAP;
            let clen = other.chunk_lens[c];
            let j: usize = 0;
            while (j < clen) {
                * = _.insert(rbmCombine(hi, other.vals[base + j]));
                j += 1;
            }
            c += 1;
        }
    }

    pub fn intersectWith(*_, other: RoaringBitmap) {
        let res = RoaringBitmap.new();
        let c: usize = 0;
        while (c < _.chunk_count) {
            let hi = _.chunk_keys[c];
            let base = c * RBM_CHUNK_CAP;
            let clen = _.chunk_lens[c];
            let j: usize = 0;
            while (j < clen) {
                let v = rbmCombine(hi, _.vals[base + j]);
                if (other.contains(v)) {
                    * = res.insert(v);
                }
                j += 1;
            }
            c += 1;
        }
        _.chunk_keys = res.chunk_keys;
        _.chunk_lens = res.chunk_lens;
        _.vals = res.vals;
        _.chunk_count = res.chunk_count;
        _.total_len = res.total_len;
    }

    pub fn differenceWith(*_, other: RoaringBitmap) {
        let res = RoaringBitmap.new();
        let c: usize = 0;
        while (c < _.chunk_count) {
            let hi = _.chunk_keys[c];
            let base = c * RBM_CHUNK_CAP;
            let clen = _.chunk_lens[c];
            let j: usize = 0;
            while (j < clen) {
                let v = rbmCombine(hi, _.vals[base + j]);
                if (!other.contains(v)) {
                    * = res.insert(v);
                }
                j += 1;
            }
            c += 1;
        }
        _.chunk_keys = res.chunk_keys;
        _.chunk_lens = res.chunk_lens;
        _.vals = res.vals;
        _.chunk_count = res.chunk_count;
        _.total_len = res.total_len;
    }

    pub fn isSubset(other: RoaringBitmap) -> bool {
        if (_.total_len > other.total_len) {
            return false;
        }
        let c: usize = 0;
        while (c < _.chunk_count) {
            let hi = _.chunk_keys[c];
            let base = c * RBM_CHUNK_CAP;
            let clen = _.chunk_lens[c];
            let j: usize = 0;
            while (j < clen) {
                let v = rbmCombine(hi, _.vals[base + j]);
                if (!other.contains(v)) {
                    return false;
                }
                j += 1;
            }
            c += 1;
        }
        return true;
    }

    pub fn isDisjoint(other: RoaringBitmap) -> bool {
        let c: usize = 0;
        while (c < _.chunk_count) {
            let hi = _.chunk_keys[c];
            let base = c * RBM_CHUNK_CAP;
            let clen = _.chunk_lens[c];
            let j: usize = 0;
            while (j < clen) {
                let v = rbmCombine(hi, _.vals[base + j]);
                if (other.contains(v)) {
                    return false;
                }
                j += 1;
            }
            c += 1;
        }
        return true;
    }
}
