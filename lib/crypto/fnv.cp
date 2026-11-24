// Origin: Pure-CP Stdlib (Zero-Alloc IETF FNV-1/FNV-1a 32/64/128-bit & DJB2/SDBM/Jenkins OAAT Hash Suite) | Created: 2026-11-22

/// IETF draft-eastlake-fnv official offset basis & prime constants.
pub const FNV32_OFFSET_BASIS: u32 = 0x811C9DC5;
pub const FNV32_PRIME: u32 = 0x01000193;

pub const FNV64_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
pub const FNV64_PRIME: u64 = 0x00000100000001B3;

pub const FNV128_OFFSET_BASIS: u128 = ((0x6C62272E07BB0142 as u128) << 64) | (0x62B821756295C58D as u128);
pub const FNV128_PRIME: u128 = ((0x0000000001000000 as u128) << 64) | (0x000000000000013B as u128);

fn addWrap32(a: u32, b: u32): u32 {
    return (((a as u64) + (b as u64)) & 0xFFFFFFFF) as u32;
}

fn mulWrap32(a: u32, b: u32): u32 {
    return (((a as u64) * (b as u64)) & 0xFFFFFFFF) as u32;
}

fn mulWrap64(a: u64, b: u64): u64 {
    return (((a as u128) * (b as u128)) & 0xFFFFFFFFFFFFFFFF) as u64;
}

fn addWrap128(a: u128, b: u128): u128 {
    const mask64: u128 = 0xFFFFFFFFFFFFFFFF;
    const sum_lo = (a & mask64) + (b & mask64);
    const carry = sum_lo >> 64;
    const sum_hi = (((a >> 64) + (b >> 64) + carry) & mask64) << 64;
    return sum_hi | (sum_lo & mask64);
}

/// Computes `(h * FNV128_PRIME) mod 2^128` without overflow.
/// Uses `FNV128_PRIME = 2^88 + 0x13B`.
fn mulFnv128(h: u128): u128 {
    const mask64: u128 = 0xFFFFFFFFFFFFFFFF;
    const lo = h & mask64;
    const hi = h >> 64;
    const p_lo: u128 = 0x13B;

    const term_lo = lo * p_lo;
    const term_hi = ((hi * p_lo) & mask64) << 64;
    const t1 = addWrap128(term_lo, term_hi);

    // (h << 88) mod 2^128 = (lo & 0xFFFFFFFFFF) << 88
    const t2 = (lo & (0x000000FFFFFFFFFF as u128)) << 88;
    return addWrap128(t1, t2);
}

/// Streaming 32-bit FNV-1a hasher.
pub struct Fnv1a32 {
    state: u32;

    pub static fn init(): Fnv1a32 {
        return Fnv1a32 { state: FNV32_OFFSET_BASIS };
    }

    fn updateByte(*_, b: u8) {
        _.state = mulWrap32(_.state ^ (b as u32), FNV32_PRIME);
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulWrap32(_.state ^ (data[i] as u32), FNV32_PRIME);
            i += 1;
        }
    }

    fn finish(): u32 {
        return _.state;
    }
}

/// Streaming 64-bit FNV-1a hasher.
pub struct Fnv1a64 {
    state: u64;

    pub static fn init(): Fnv1a64 {
        return Fnv1a64 { state: FNV64_OFFSET_BASIS };
    }

    fn updateByte(*_, b: u8) {
        _.state = mulWrap64(_.state ^ (b as u64), FNV64_PRIME);
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulWrap64(_.state ^ (data[i] as u64), FNV64_PRIME);
            i += 1;
        }
    }

    fn finish(): u64 {
        return _.state;
    }
}

/// Streaming 128-bit FNV-1a hasher.
pub struct Fnv1a128 {
    state: u128;

    pub static fn init(): Fnv1a128 {
        return Fnv1a128 { state: FNV128_OFFSET_BASIS };
    }

    fn updateByte(*_, b: u8) {
        _.state = mulFnv128(_.state ^ (b as u128));
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulFnv128(_.state ^ (data[i] as u128));
            i += 1;
        }
    }

    fn finish(): u128 {
        return _.state;
    }
}

/// Streaming 32-bit FNV-1 hasher (multiply then XOR).
pub struct Fnv1_32 {
    state: u32;

    pub static fn init(): Fnv1_32 {
        return Fnv1_32 { state: FNV32_OFFSET_BASIS };
    }

    fn updateByte(*_, b: u8) {
        _.state = mulWrap32(_.state, FNV32_PRIME) ^ (b as u32);
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulWrap32(_.state, FNV32_PRIME) ^ (data[i] as u32);
            i += 1;
        }
    }

    fn finish(): u32 {
        return _.state;
    }
}

/// Streaming 64-bit FNV-1 hasher (multiply then XOR).
pub struct Fnv1_64 {
    state: u64;

    pub static fn init(): Fnv1_64 {
        return Fnv1_64 { state: FNV64_OFFSET_BASIS };
    }

    fn updateByte(*_, b: u8) {
        _.state = mulWrap64(_.state, FNV64_PRIME) ^ (b as u64);
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulWrap64(_.state, FNV64_PRIME) ^ (data[i] as u64);
            i += 1;
        }
    }

    fn finish(): u64 {
        return _.state;
    }
}

/// Streaming 128-bit FNV-1 hasher (multiply then XOR).
pub struct Fnv1_128 {
    state: u128;

    pub static fn init(): Fnv1_128 {
        return Fnv1_128 { state: FNV128_OFFSET_BASIS };
    }

    fn updateByte(*_, b: u8) {
        _.state = mulFnv128(_.state) ^ (b as u128);
    }

    fn update(*_, data: []const u8) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulFnv128(_.state) ^ (data[i] as u128);
            i += 1;
        }
    }

    fn finish(): u128 {
        return _.state;
    }
}

/// One-shot 32-bit FNV-1a hash.
pub fn fnv1a32(data: []const u8): u32 {
    let h = Fnv1a32.init();
    h.update(data);
    return h.finish();
}

/// One-shot 64-bit FNV-1a hash.
pub fn fnv1a64(data: []const u8): u64 {
    let h = Fnv1a64.init();
    h.update(data);
    return h.finish();
}

/// One-shot 128-bit FNV-1a hash.
pub fn fnv1a128(data: []const u8): u128 {
    let h = Fnv1a128.init();
    h.update(data);
    return h.finish();
}

/// One-shot 32-bit FNV-1 hash.
pub fn fnv1_32(data: []const u8): u32 {
    let h = Fnv1_32.init();
    h.update(data);
    return h.finish();
}

/// One-shot 64-bit FNV-1 hash.
pub fn fnv1_64(data: []const u8): u64 {
    let h = Fnv1_64.init();
    h.update(data);
    return h.finish();
}

/// One-shot 128-bit FNV-1 hash.
pub fn fnv1_128(data: []const u8): u128 {
    let h = Fnv1_128.init();
    h.update(data);
    return h.finish();
}

/// Official FNV XOR-folding from 64-bit hash `h64` into `bits` width (`1..63`).
/// Returns `null` if `bits == 0 || bits >= 64`.
pub fn fnvXorFold32(h64: u64, bits: u8): u64? {
    if (bits == 0 || bits >= 64) {
        return null;
    }
    const shift = bits as u6;
    const mask: u64 = ((1 as u64) << shift) - 1;
    let acc = h64;
    while (acc > mask) {
        acc = (acc >> shift) ^ (acc & mask);
    }
    return acc;
}

/// Classic Dan Bernstein DJB2 hash (init `5381`, `h * 33 + b`).
pub fn djb2(data: []const u8): u32 {
    let h: u32 = 5381;
    let i: usize = 0;
    while (i < data.len) {
        h = addWrap32(mulWrap32(h, 33), data[i] as u32);
        i += 1;
    }
    return h;
}

/// Classic Dan Bernstein DJB2 XOR variant (init `5381`, `(h * 33) ^ b`).
pub fn djb2Xor(data: []const u8): u32 {
    let h: u32 = 5381;
    let i: usize = 0;
    while (i < data.len) {
        h = mulWrap32(h, 33) ^ (data[i] as u32);
        i += 1;
    }
    return h;
}

/// Classic SDBM hash (`h * 65599 + b`).
pub fn sdbm(data: []const u8): u32 {
    let h: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        h = addWrap32(mulWrap32(h, 65599), data[i] as u32);
        i += 1;
    }
    return h;
}

/// Bob Jenkins One-at-a-Time (OAAT) 32-bit hash.
pub fn jenkinsOaat(data: []const u8): u32 {
    let h: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        h = addWrap32(h, data[i] as u32);
        h = addWrap32(h, h << 10);
        h = h ^ (h >> 6);
        i += 1;
    }
    h = addWrap32(h, h << 3);
    h = h ^ (h >> 11);
    h = addWrap32(h, h << 15);
    return h;
}
