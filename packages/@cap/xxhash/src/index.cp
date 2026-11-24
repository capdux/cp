// @cap/xxhash — xxHash 0.8.3 extremely fast non-cryptographic hash (XXH32/XXH64/XXH3) for CP
// License: BSD-2-Clause (Upstream: Cyan4973/xxHash)

pub @c {
    fn XXH_versionNumber(): u32;
    fn XXH32(input: [*]const u8!, length: usize, seed: u32): u32;
    fn XXH64(input: [*]const u8!, length: usize, seed: u64): u64;
    fn XXH3_64bits(input: [*]const u8!, length: usize): u64;
}

/// Returns the linked xxHash library version number (e.g. 803 for 0.8.3).
pub fn versionNumber(): u32 {
    unsafe {
        return XXH_versionNumber();
    }
}

/// Computes the 32-bit XXH32 hash of `data` with the given `seed`.
pub fn xxh32(data: []const u8, seed: u32): u32 {
    unsafe {
        return XXH32(data.ptr, data.len, seed);
    }
}

/// Computes the 64-bit XXH64 hash of `data` with the given `seed`.
pub fn xxh64(data: []const u8, seed: u64): u64 {
    unsafe {
        return XXH64(data.ptr, data.len, seed);
    }
}

/// Computes the 64-bit XXH3 hash of `data` (memory-bandwidth speed).
pub fn xxh3_64bits(data: []const u8): u64 {
    unsafe {
        return XXH3_64bits(data.ptr, data.len);
    }
}
