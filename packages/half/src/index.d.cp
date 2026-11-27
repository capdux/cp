// Type definitions for CP

export fn f32ToBits(x: f32): u32;
export fn bitsToF32(b: u32): f32;
export class F16 {
    bits: u16,
    fn fromBits(b: u16): F16;
    fn fromLeBytes(b0: u8, b1: u8): F16;
    fn fromF32(x: f32): F16;
    fn toF32(): f32;
    fn isNan(): bool;
    fn isInfinite(): bool;
    fn isZero(): bool;
    fn neg(): F16;
    fn abs(): F16;
    fn toLeBytes(): [2]u8;
}

export class Bf16 {
    bits: u16,
    fn fromBits(b: u16): Bf16;
    fn fromF32(x: f32): Bf16;
    fn toF32(): f32;
    fn isNan(): bool;
    fn isInfinite(): bool;
    fn neg(): Bf16;
    fn abs(): Bf16;
}

export fn f32SliceToF16(src: &[]f32, dst: []u16): usize;
export fn f16SliceToF32(src: &[]u16, dst: []f32): usize;
