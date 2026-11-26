// Type definitions for CP

export fn fnv1a64(data: &[]u8): u64;
export fn djb2Mix64(data: &[]u8): u64;
export fn popcount(bits: &[]u8): u64;
export fn fillRatio(bits: &[]u8): f64;
export fn optimalNumHashes(bit_count: usize, expected_items: usize): u32;
export class BloomFilter {
    num_hashes: u32,
    bit_count: usize,
    inserted: u64,
    fn new(bits: []u8, num_hashes: u32): BloomFilter;
    fn clear(_: auto, bits: []u8);
    fn insertBytes(_: auto, bits: []u8, item: &[]u8);
    fn insert(_: auto, bits: []u8, item: string);
    fn containsBytes(bits: &[]u8, item: &[]u8): bool;
    fn contains(bits: &[]u8, item: string): bool;
    fn merge(_: auto, dst_bits: []u8, src_bits: &[]u8): bool;
}

