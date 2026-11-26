// Type definitions for CP

export fn hashBytes64(data: string): u64;
export fn hashU64(val: u64): u64;
export fn clz56(w: u64): u8;
export class HyperLogLog256 {
    regs: [HLL_M]u8,
    fn init(): HyperLogLog256;
    fn clear(_: auto);
    fn registerCount(): usize;
    fn insertHash(_: auto, h: u64);
    fn add(_: auto, item: string);
    fn addU64(_: auto, val: u64);
    fn count(): u64;
    fn merge(_: auto, other: HyperLogLog256);
    fn relativeError(): f64;
}

