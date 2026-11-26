// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export fn hashKey64(key: u64, seed: u64): u64;
export class Slots3 {
    h0: usize,
    h1: usize,
    h2: usize,
    fp: u8,
}

export fn deriveSlots(h: u64, block_len: usize): Slots3;
export class Xor8Filter {
    seed: u64,
    block_length: usize,
    fingerprints: [MAX_SLOTS]u8,
    key_count: usize,
    valid: bool,
    fn emptyWithStatus(is_valid: bool): Xor8Filter;
    fn build(keys: &[]u64): Xor8Filter;
    fn contains(key: u64): bool;
    fn len(): usize;
    fn fingerprintBytes(): usize;
    fn isValid(): bool;
}

