// @cap/count-min — Zero-allocation Count-Min Sketch streaming frequency estimator for CP
// Clean-room MIT implementation inspired by Rust `count-min-sketch` and Redis `CMS.INCRBY`.

const CMS_DEPTH: usize = 4;
const CMS_WIDTH: usize = 256;
const CMS_CELLS: usize = 1024;
const U32_MAX: u32 = 0xFFFFFFFF;

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) * (0x100000000 as u64)) | (lo as u64);
}

fn addWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const lo = (a & mask32) + (b & mask32);
    const carry = lo / shift32;
    const hi = ((a / shift32) + (b / shift32) + carry) & mask32;
    return (hi * shift32) | (lo & mask32);
}

fn mulWrap64(a: u64, b: u64) -> u64 {
    const mask32: u64 = 0xFFFFFFFF;
    const shift32: u64 = 0x100000000;
    const a_lo = a & mask32;
    const a_hi = a / shift32;
    const b_lo = b & mask32;
    const b_hi = b / shift32;
    const p_lo = a_lo * b_lo;
    const mid1 = (a_lo * b_hi) & mask32;
    const mid2 = (a_hi * b_lo) & mask32;
    const hi_sum = ((p_lo / shift32) + mid1 + mid2) & mask32;
    return (hi_sum * shift32) | (p_lo & mask32);
}

fn splitMix64(x_in: u64) -> u64 {
    const c1 = makeU64(0x9E3779B9 as u32, 0x7F4A7C15 as u32);
    const m1 = makeU64(0xBF58476D as u32, 0x1CE4E5B9 as u32);
    const m2 = makeU64(0x94D049BB as u32, 0x133111EB as u32);
    let z = addWrap64(x_in, c1);
    z = mulWrap64(z ^ (z / (0x40000000 as u64)), m1);
    z = mulWrap64(z ^ (z / (0x8000000 as u64)), m2);
    return z ^ (z / (0x80000000 as u64));
}

fn rowSeed(row: usize) -> u64 {
    if (row == 0) { return makeU64(0xCBF29CE4 as u32, 0x84222325 as u32); }
    if (row == 1) { return makeU64(0x517CC1B7 as u32, 0x27220A95 as u32); }
    if (row == 2) { return makeU64(0x9E3779B9 as u32, 0x7F4A7C15 as u32); }
    return makeU64(0xD6E8FEB8 as u32, 0x6659FD93 as u32);
}

fn hashBytesRow(s: string, row: usize) -> usize {
    let h = rowSeed(row);
    const prime = makeU64(0x00000100 as u32, 0x000001B3 as u32);
    let i: usize = 0;
    while (i < s.len) {
        h = h ^ (s[i] as u64);
        h = mulWrap64(h, prime);
        i += 1;
    }
    const mixed = splitMix64(h ^ (row as u64));
    return (mixed & (255 as u64)) as usize;
}

fn hashU64Row(val: u64, row: usize) -> usize {
    const mixed = splitMix64(val ^ rowSeed(row));
    return (mixed & (255 as u64)) as usize;
}

fn satAddU32(a: u32, b: u32) -> u32 {
    if (a >= U32_MAX - b) {
        return U32_MAX;
    }
    return a + b;
}

pub struct CountMinSketch {
    pub counters: [CMS_CELLS]u32,
    pub total: u64,

    pub static fn init() -> CountMinSketch {
        let arr: [CMS_CELLS]u32 = undefined;
        let i: usize = 0;
        while (i < CMS_CELLS) {
            arr[i] = 0 as u32;
            i += 1;
        }
        return CountMinSketch{
            counters: arr,
            total: 0 as u64,
        };
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < CMS_CELLS) {
            _.counters[i] = 0 as u32;
            i += 1;
        }
        _.total = 0 as u64;
    }

    pub fn width() -> usize {
        return CMS_WIDTH;
    }

    pub fn depth() -> usize {
        return CMS_DEPTH;
    }

    pub fn totalCount() -> u64 {
        return _.total;
    }

    pub fn add(*_, item: string, count: u32) {
        let r: usize = 0;
        while (r < CMS_DEPTH) {
            const col = hashBytesRow(item, r);
            const idx = (r * CMS_WIDTH) + col;
            _.counters[idx] = satAddU32(_.counters[idx], count);
            r += 1;
        }
        _.total += count as u64;
    }

    pub fn increment(*_, item: string) {
        _.add(item, 1 as u32);
    }

    pub fn addU64(*_, val: u64, count: u32) {
        let r: usize = 0;
        while (r < CMS_DEPTH) {
            const col = hashU64Row(val, r);
            const idx = (r * CMS_WIDTH) + col;
            _.counters[idx] = satAddU32(_.counters[idx], count);
            r += 1;
        }
        _.total += count as u64;
    }

    pub fn estimate(item: string) -> u32 {
        let best: u32 = U32_MAX;
        let r: usize = 0;
        while (r < CMS_DEPTH) {
            const col = hashBytesRow(item, r);
            const v = _.counters[(r * CMS_WIDTH) + col];
            if (v < best) {
                best = v;
            }
            r += 1;
        }
        return best;
    }

    pub fn estimateU64(val: u64) -> u32 {
        let best: u32 = U32_MAX;
        let r: usize = 0;
        while (r < CMS_DEPTH) {
            const col = hashU64Row(val, r);
            const v = _.counters[(r * CMS_WIDTH) + col];
            if (v < best) {
                best = v;
            }
            r += 1;
        }
        return best;
    }

    pub fn merge(*_, other: CountMinSketch) {
        let i: usize = 0;
        while (i < CMS_CELLS) {
            _.counters[i] = satAddU32(_.counters[i], other.counters[i]);
            i += 1;
        }
        _.total += other.total;
    }

    pub fn isHeavyHitter(item: string, threshold_ratio: f64) -> bool {
        if (_.total == (0 as u64)) {
            return false;
        }
        const est = _.estimate(item) as f64;
        return (est / (_.total as f64)) >= threshold_ratio;
    }
}

export {
    CountMinSketch,
};
