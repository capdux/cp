// Type definitions for CP

export fn zeroizeBytes(buf: []u8): usize;
export fn zeroizeU32(buf: []u32): usize;
export fn zeroizeU64(buf: []u64): usize;
export fn isAllZeroBytes(buf: []u8): bool;
export class ZeroizingKey32 {
    bytes: [32]u8,
    active: bool,
    fn fromSlice(src: []u8): ZeroizingKey32?;
    fn zeroize(_: auto);
    fn isZeroed(): bool;
}

export class ZeroizingBuf64 {
    bytes: [64]u8,
    len: usize,
    active: bool,
    fn fromSlice(src: []u8): ZeroizingBuf64?;
    fn zeroize(_: auto);
    fn isZeroed(): bool;
}

