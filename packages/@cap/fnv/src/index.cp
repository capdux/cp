// @cap/fnv — Zero-allocation FNV-1a 32/64-bit & FxHash 64-bit streaming hashers for CP
// Clean-room MIT implementation inspired by Rust `fnv` and `rustc-hash`.

pub const FNV32_OFFSET_BASIS: u32 = 0x811c9dc5;
pub const FNV32_PRIME: u32 = 0x01000193;

pub const FNV64_OFFSET_BASIS: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
pub const FNV64_PRIME: u64 = ((0x00000100 as u64) << 32) | (0x000001b3 as u64);

pub const FX_HASH_K: u64 = ((0x517cc1b7 as u64) << 32) | (0x27220a95 as u64);

// ---------------------------------------------------------------------------
// Overflow-Safe Wrapping Arithmetic Helpers
// ---------------------------------------------------------------------------

fn mulWrap32(a: u32, b: u32) -> u32 {
    return (((a as u64) * (b as u64)) & 0xFFFFFFFF) as u32;
}

/// Exact 64-bit wrapping multiplication `(a * b) mod 2^64` without overflow.
fn mulWrap64(a: u64, b: u64) -> u64 {
    const a_lo = a & 0xFFFFFFFF;
    const a_hi = a >> 32;
    const b_lo = b & 0xFFFFFFFF;
    const b_hi = b >> 32;

    const p0 = a_lo * b_lo;
    const p1 = (a_lo * b_hi) & 0xFFFFFFFF;
    const p2 = (a_hi * b_lo) & 0xFFFFFFFF;
    const hi = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (p0 & 0xFFFFFFFF) | (hi << 32);
}

fn rotl64By5(x: u64) -> u64 {
    return (x << 5) | (x >> 59);
}

// ---------------------------------------------------------------------------
// 1. FNV-1a 32-bit Streaming Hasher
// ---------------------------------------------------------------------------

pub struct Fnv32Hasher {
    pub state: u32,

    pub static fn init() -> Fnv32Hasher {
        return Fnv32Hasher{ state: FNV32_OFFSET_BASIS };
    }

    pub fn writeByte(*_, b: u8) {
        _.state = mulWrap32(_.state ^ (b as u32), FNV32_PRIME);
    }

    pub fn write(*_, data: string) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulWrap32(_.state ^ (data[i] as u32), FNV32_PRIME);
            i += 1;
        }
    }

    pub fn finish() -> u32 {
        return _.state;
    }
}

/// Computes the standard FNV-1a 32-bit hash of `data`.
pub fn fnv1a32(data: string) -> u32 {
    let h = Fnv32Hasher.init();
    h.write(data);
    return h.finish();
}

// ---------------------------------------------------------------------------
// 2. FNV-1a 64-bit Streaming Hasher
// ---------------------------------------------------------------------------

pub struct Fnv64Hasher {
    pub state: u64,

    pub static fn init() -> Fnv64Hasher {
        return Fnv64Hasher{ state: FNV64_OFFSET_BASIS };
    }

    pub fn writeByte(*_, b: u8) {
        _.state = mulWrap64(_.state ^ (b as u64), FNV64_PRIME);
    }

    pub fn write(*_, data: string) {
        let i: usize = 0;
        while (i < data.len) {
            _.state = mulWrap64(_.state ^ (data[i] as u64), FNV64_PRIME);
            i += 1;
        }
    }

    pub fn finish() -> u64 {
        return _.state;
    }
}

/// Computes the standard FNV-1a 64-bit hash of `data`.
pub fn fnv1a64(data: string) -> u64 {
    let h = Fnv64Hasher.init();
    h.write(data);
    return h.finish();
}

// ---------------------------------------------------------------------------
// 3. FxHash 64-bit Streaming Hasher (`rustc-hash` algorithm)
// ---------------------------------------------------------------------------

pub struct FxHasher {
    pub state: u64,

    pub static fn init() -> FxHasher {
        return FxHasher{ state: 0 };
    }

    pub static fn withSeed(seed: u64) -> FxHasher {
        return FxHasher{ state: seed };
    }

    pub fn writeU64(*_, v: u64) {
        _.state = mulWrap64(rotl64By5(_.state) ^ v, FX_HASH_K);
    }

    pub fn writeBytes(*_, data: string) {
        let i: usize = 0;
        // Absorb 8-byte little-endian words
        while (i + 8 <= data.len) {
            const w: u64 = (data[i] as u64)
                | ((data[i + 1] as u64) << 8)
                | ((data[i + 2] as u64) << 16)
                | ((data[i + 3] as u64) << 24)
                | ((data[i + 4] as u64) << 32)
                | ((data[i + 5] as u64) << 40)
                | ((data[i + 6] as u64) << 48)
                | ((data[i + 7] as u64) << 56);
            _.writeU64(w);
            i += 8;
        }
        // Absorb remaining 4-byte chunk
        if (i + 4 <= data.len) {
            const w4: u64 = (data[i] as u64)
                | ((data[i + 1] as u64) << 8)
                | ((data[i + 2] as u64) << 16)
                | ((data[i + 3] as u64) << 24);
            _.writeU64(w4);
            i += 4;
        }
        // Absorb remaining 2-byte chunk
        if (i + 2 <= data.len) {
            const w2: u64 = (data[i] as u64) | ((data[i + 1] as u64) << 8);
            _.writeU64(w2);
            i += 2;
        }
        // Absorb final byte
        if (i < data.len) {
            _.writeU64(data[i] as u64);
        }
    }

    pub fn finish() -> u64 {
        return _.state;
    }
}

/// Computes the 64-bit FxHash (`rustc-hash`) of `data`.
pub fn fxHash64(data: string) -> u64 {
    let h = FxHasher.init();
    h.writeBytes(data);
    return h.finish();
}
