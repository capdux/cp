// @cap/nohash-hasher — Zero-overhead identity & integer-key hashers in pure CP
// Clean-room Apache-2.0 implementation of crates.io `nohash-hasher` (#860, 133.0M downloads).

pub struct IntHasher {
    pub state: u64,

    pub fn writeU64(*_, v: u64) {
        _.state = v;
    }

    pub fn writeU32(*_, v: u32) {
        _.state = v as u64;
    }

    pub fn writeI64(*_, v: i64) {
        _.state = @bitCast(v);
    }

    pub fn finish(): u64 {
        return _.state;
    }
}

pub fn newIntHasher(): IntHasher {
    return IntHasher { state: 0 };
}

/// Identity hash for already-random integer keys (zero arithmetic overhead).
pub fn nohashU64(key: u64): u64 {
    return key;
}

/// Identity hash for u32 keys.
pub fn nohashU32(key: u32): u64 {
    return key as u64;
}

/// Fast Fibonacci multiplication hash mapping a 64-bit integer key to a power-of-two bucket index (`0..2^bits - 1`).
pub fn fibBucketIndex(key: u64, bits: u6): usize {
    if (bits == 0) {
        return 0;
    }
    const shift: u6 = (64 - (bits as u32)) as u6;
    const golden: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
    const mixed = key *% golden;
    return (mixed >> shift) as usize;
}

/// Bijective SplitMix64 finalizer for dispersing sequential integer IDs into uniform 64-bit hashes.
pub fn mixU64(x: u64): u64 {
    const c1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
    const c2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
    let z = x;
    z = (z ^ (z >> 30)) *% c1;
    z = (z ^ (z >> 27)) *% c2;
    return z ^ (z >> 31);
}

export {
    IntHasher,
    newIntHasher,
    nohashU64,
    nohashU32,
    fibBucketIndex,
    mixU64,
};
