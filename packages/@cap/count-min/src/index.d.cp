// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export fn addWrap64(a: u64, b: u64): u64;
export fn mulWrap64(a: u64, b: u64): u64;
export fn splitMix64(x_in: u64): u64;
export fn rowSeed(row: usize): u64;
export fn hashBytesRow(s: string, row: usize): usize;
export fn hashU64Row(val: u64, row: usize): usize;
export fn satAddU32(a: u32, b: u32): u32;
export class CountMinSketch {
    counters: [CMS_CELLS]u32,
    total: u64,
    fn init(): CountMinSketch;
    fn clear(_: auto);
    fn width(): usize;
    fn depth(): usize;
    fn totalCount(): u64;
    fn add(_: auto, item: string, count: u32);
    fn increment(_: auto, item: string);
    fn addU64(_: auto, val: u64, count: u32);
    fn estimate(item: string): u32;
    fn estimateU64(val: u64): u32;
    fn merge(_: auto, other: CountMinSketch);
    fn isHeavyHitter(item: string, threshold_ratio: f64): bool;
}

