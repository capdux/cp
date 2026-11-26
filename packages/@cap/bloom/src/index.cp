// @cap/bloom — Zero-allocation Kirsch-Mitzenmacher double-hashing Bloom filters for CP
// Clean-room MIT implementation inspired by Rust `bloomfilter` / `fastbloom` and npm `bloom-filters`.

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

struct HashPair {
    h1: u64,
    h2: u64,
}

fn hashBytesPair(s: string) -> HashPair {
    let h1 = makeU64(0xCBF29CE4 as u32, 0x84222325 as u32);
    const prime = makeU64(0x00000100 as u32, 0x000001B3 as u32);
    let i: usize = 0;
    while (i < s.len) {
        h1 = h1 ^ (s[i] as u64);
        h1 = mulWrap64(h1, prime);
        i += 1;
    }
    const seed2 = makeU64(0x517CC1B7 as u32, 0x27220A95 as u32);
    const h2 = splitMix64(h1 ^ seed2 ^ (s.len as u64)) | (1 as u64);
    return HashPair{ h1: h1, h2: h2 };
}

fn hashU64Pair(val: u64) -> HashPair {
    const h1 = splitMix64(val);
    const seed2 = makeU64(0x9E3779B9 as u32, 0x7F4A7C15 as u32);
    const h2 = splitMix64(val ^ seed2) | (1 as u64);
    return HashPair{ h1: h1, h2: h2 };
}

fn bitMask64(bit: usize) -> u64 {
    let v: u64 = 1;
    let i: usize = 0;
    while (i < bit) {
        v = v * (2 as u64);
        i += 1;
    }
    return v;
}

fn popcount64(x_in: u64) -> usize {
    let x = x_in;
    x = x - ((x / (2 as u64)) & (0x5555555555555555 as u64));
    x = (x & (0x3333333333333333 as u64)) + ((x / (4 as u64)) & (0x3333333333333333 as u64));
    x = (x + (x / (16 as u64))) & (0x0F0F0F0F0F0F0F0F as u64);
    let total: usize = 0;
    let i: usize = 0;
    while (i < 8) {
        total += (x & (0xFF as u64)) as usize;
        x = x / (256 as u64);
        i += 1;
    }
    return total;
}

fn clampHashes(k: u8) -> u8 {
    if (k < (1 as u8)) {
        return 1 as u8;
    }
    if (k > (16 as u8)) {
        return 16 as u8;
    }
    return k;
}

fn expNeg(x_in: f64) -> f64 {
    if (x_in <= 0.0) {
        return 1.0;
    }
    if (x_in >= 40.0) {
        return 0.0;
    }
    let x = x_in;
    let halvings: usize = 0;
    while (x > 0.5 && halvings < 16) {
        x *= 0.5;
        halvings += 1;
    }
    // Taylor series for exp(-x) on [0, 0.5]
    let sum: f64 = 1.0;
    let term: f64 = 1.0;
    let n: usize = 1;
    while (n <= 12) {
        term = term * (-x) / (n as f64);
        sum += term;
        n += 1;
    }
    let r = sum;
    let h: usize = 0;
    while (h < halvings) {
        r = r * r;
        h += 1;
    }
    return r;
}

fn computeFpr(k: u8, n: usize, m: usize) -> f64 {
    if (n == 0 || m == 0) {
        return 0.0;
    }
    const kf = k as f64;
    const nf = n as f64;
    const mf = m as f64;
    const pOne = 1.0 - expNeg((kf * nf) / mf);
    let acc: f64 = 1.0;
    let i: u8 = 0;
    while (i < k) {
        acc *= pOne;
        i += 1;
    }
    return acc;
}

pub struct BloomFilter1024 {
    pub words: [16]u64,
    pub num_hashes: u8,
    pub items: usize,

    pub static fn init(num_hashes: u8) -> BloomFilter1024 {
        let w: [16]u64 = undefined;
        let i: usize = 0;
        while (i < 16) {
            w[i] = 0 as u64;
            i += 1;
        }
        return BloomFilter1024{
            words: w,
            num_hashes: clampHashes(num_hashes),
            items: 0 as usize,
        };
    }

    pub fn insert(*_, item: string) {
        const hp = hashBytesPair(item);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (1023 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            _.words[wi] = _.words[wi] | bitMask64(bi);
            i += 1;
        }
        _.items += 1;
    }

    pub fn mightContain(item: string) -> bool {
        const hp = hashBytesPair(item);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (1023 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            if ((_.words[wi] & bitMask64(bi)) == (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn insertU64(*_, val: u64) {
        const hp = hashU64Pair(val);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (1023 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            _.words[wi] = _.words[wi] | bitMask64(bi);
            i += 1;
        }
        _.items += 1;
    }

    pub fn mightContainU64(val: u64) -> bool {
        const hp = hashU64Pair(val);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (1023 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            if ((_.words[wi] & bitMask64(bi)) == (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn bitCount() -> usize {
        return 1024 as usize;
    }

    pub fn setBits() -> usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 16) {
            total += popcount64(_.words[i]);
            i += 1;
        }
        return total;
    }

    pub fn itemCount() -> usize {
        return _.items;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = 0 as u64;
            i += 1;
        }
        _.items = 0 as usize;
    }

    pub fn unionWith(*_, other: BloomFilter1024) {
        let i: usize = 0;
        while (i < 16) {
            _.words[i] = _.words[i] | other.words[i];
            i += 1;
        }
        _.items += other.items;
    }

    pub fn estimatedFpr() -> f64 {
        return computeFpr(_.num_hashes, _.items, 1024 as usize);
    }
}

pub struct BloomFilter4096 {
    pub words: [64]u64,
    pub num_hashes: u8,
    pub items: usize,

    pub static fn init(num_hashes: u8) -> BloomFilter4096 {
        let w: [64]u64 = undefined;
        let i: usize = 0;
        while (i < 64) {
            w[i] = 0 as u64;
            i += 1;
        }
        return BloomFilter4096{
            words: w,
            num_hashes: clampHashes(num_hashes),
            items: 0 as usize,
        };
    }

    pub fn insert(*_, item: string) {
        const hp = hashBytesPair(item);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (4095 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            _.words[wi] = _.words[wi] | bitMask64(bi);
            i += 1;
        }
        _.items += 1;
    }

    pub fn mightContain(item: string) -> bool {
        const hp = hashBytesPair(item);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (4095 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            if ((_.words[wi] & bitMask64(bi)) == (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn insertU64(*_, val: u64) {
        const hp = hashU64Pair(val);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (4095 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            _.words[wi] = _.words[wi] | bitMask64(bi);
            i += 1;
        }
        _.items += 1;
    }

    pub fn mightContainU64(val: u64) -> bool {
        const hp = hashU64Pair(val);
        let i: u8 = 0;
        while (i < _.num_hashes) {
            const combined = addWrap64(hp.h1, mulWrap64(i as u64, hp.h2));
            const bit = (combined & (4095 as u64)) as usize;
            const wi = bit / (64 as usize);
            const bi = bit % (64 as usize);
            if ((_.words[wi] & bitMask64(bi)) == (0 as u64)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn bitCount() -> usize {
        return 4096 as usize;
    }

    pub fn setBits() -> usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < 64) {
            total += popcount64(_.words[i]);
            i += 1;
        }
        return total;
    }

    pub fn itemCount() -> usize {
        return _.items;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < 64) {
            _.words[i] = 0 as u64;
            i += 1;
        }
        _.items = 0 as usize;
    }

    pub fn unionWith(*_, other: BloomFilter4096) {
        let i: usize = 0;
        while (i < 64) {
            _.words[i] = _.words[i] | other.words[i];
            i += 1;
        }
        _.items += other.items;
    }

    pub fn estimatedFpr() -> f64 {
        return computeFpr(_.num_hashes, _.items, 4096 as usize);
    }
}

export {
    BloomFilter1024,
    BloomFilter4096,
};
