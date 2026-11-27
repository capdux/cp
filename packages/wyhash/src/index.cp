// @cap/wyhash — Zero-allocation Wang Yi wyhash v4 64-bit Hash & WyRng PRNG
// Clean-room MIT implementation in pure CP (overflow-free 32-bit split 128-bit multiply).

struct Mul128 {
    lo: u64,
    hi: u64,
}

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn mul128(a: u64, b: u64) -> Mul128 {
    const ha = a >> 32;
    const hb = b >> 32;
    const la = a & 0xFFFFFFFF;
    const lb = b & 0xFFFFFFFF;

    const rh = ha *% hb;
    const rm0 = ha *% lb;
    const rm1 = hb *% la;
    const rl = la *% lb;

    const t = rl +% (rm0 << 32);
    let c: u64 = 0;
    if (t < rl) {
        c = 1;
    }
    const lo = t +% (rm1 << 32);
    if (lo < t) {
        c += 1;
    }
    const hi = rh +% (rm0 >> 32) +% (rm1 >> 32) +% c;
    return Mul128 { lo: lo, hi: hi };
}

/// Overflow-free 128-bit multiply-and-XOR mix (`(a * b).lo ^ (a * b).hi`) using 32-bit splitting.
pub fn wymum(a: u64, b: u64) -> u64 {
    const m = mul128(a, b);
    return m.lo ^ m.hi;
}

fn read4(data: []const u8, off: usize) -> u64 {
    return (data[off] as u64)
        | ((data[off + 1] as u64) << 8)
        | ((data[off + 2] as u64) << 16)
        | ((data[off + 3] as u64) << 24);
}

fn read8(data: []const u8, off: usize) -> u64 {
    const lo = read4(data, off);
    const hi = read4(data, off + 4);
    return lo | (hi << 32);
}

/// Computes the wyhash v4 64-bit hash over a byte slice with the given `seed`.
pub fn wyhashBytes(data: []const u8, seed: u64) -> u64 {
    const s0 = makeU64(0xA0761D64, 0x78BD642F);
    const s1 = makeU64(0xE7037ED1, 0xA0B428DB);
    const s2 = makeU64(0x8EBC6AF0, 0x9C88C6E3);
    const s3 = makeU64(0x589965CC, 0x75374CC3);

    let s = seed ^ wymum(seed ^ s0, s1);
    const len = data.len;
    let a: u64 = 0;
    let b: u64 = 0;

    if (len <= 16) {
        if (len >= 4) {
            const shift_off = (len >> 3) << 2;
            a = (read4(data, 0) << 32) | read4(data, shift_off);
            b = (read4(data, len - 4) << 32) | read4(data, len - 4 - shift_off);
        } else if (len > 0) {
            a = ((data[0] as u64) << 16)
                | ((data[len >> 1] as u64) << 8)
                | (data[len - 1] as u64);
            b = 0;
        }
    } else {
        let rem = len;
        let p: usize = 0;
        if (rem > 48) {
            let see1 = s;
            let see2 = s;
            while (rem > 48) {
                s = wymum(read8(data, p) ^ s1, read8(data, p + 8) ^ s);
                see1 = wymum(read8(data, p + 16) ^ s2, read8(data, p + 24) ^ see1);
                see2 = wymum(read8(data, p + 32) ^ s3, read8(data, p + 40) ^ see2);
                p += 48;
                rem -= 48;
            }
            s ^= see1 ^ see2;
        }
        while (rem > 16) {
            s = wymum(read8(data, p) ^ s1, read8(data, p + 8) ^ s);
            p += 16;
            rem -= 16;
        }
        a = read8(data, len - 16);
        b = read8(data, len - 8);
    }

    const fin = mul128(a ^ s1, b ^ s);
    return wymum(fin.lo ^ s0 ^ (len as u64), fin.hi ^ s1);
}

/// Computes the wyhash v4 64-bit hash over a UTF-8 string with the given `seed`.
pub fn wyhash(data: string, seed: u64) -> u64 {
    return wyhashBytes(data, seed);
}

pub struct WyRng {
    pub state: u64,

    /// Creates a new `WyRng` PRNG initialized with `seed`.
    pub static fn new(seed: u64) -> WyRng {
        return WyRng { state: seed };
    }

    /// Generates the next 64-bit pseudorandom integer using the official `wyrand` formula.
    pub fn nextU64(*_) -> u64 {
        const c1 = makeU64(0xA0761D64, 0x78BD642F);
        const c2 = makeU64(0xE7037ED1, 0xA0B428DB);
        _.state = _.state +% c1;
        return wymum(_.state, _.state ^ c2);
    }

    /// Generates the next 32-bit pseudorandom unsigned integer.
    pub fn nextU32(*_) -> u32 {
        return (_.nextU64() >> 32) as u32;
    }

    /// Generates a uniformly distributed IEEE-754 `f64` in `[0.0, 1.0)`.
    pub fn nextF64(*_) -> f64 {
        return ((_.nextU64() >> 11) as f64) * (1.0 / 9007199254740992.0);
    }
}
