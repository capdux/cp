// Type definitions for CP

export class F16 {
    bits: u16,
    fn fromBits(bits: u16): F16;
    fn isNan(): bool;
    fn isInfinite(): bool;
    fn isZero(): bool;
    fn isSignNegative(): bool;
    fn isFinite(): bool;
    fn abs(): F16;
    fn neg(): F16;
}

export fn f32BitsToF16Bits(f32_bits: u32): u16;
export fn f16BitsToF32Bits(h_bits: u16): u32;
export class Bf16 {
    bits: u16,
    fn fromBits(bits: u16): Bf16;
    fn isNan(): bool;
    fn isInfinite(): bool;
    fn isZero(): bool;
    fn isSignNegative(): bool;
    fn abs(): Bf16;
    fn neg(): Bf16;
}

export fn f32BitsToBf16Bits(f32_bits: u32): u16;
export fn bf16BitsToF32Bits(b_bits: u16): u32;
