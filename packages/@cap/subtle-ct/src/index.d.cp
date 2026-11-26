// Type definitions for CP

export class Choice {
    val: u8,
    fn fromU8(v: u8): Choice;
    fn toBool(): bool;
    fn not(): Choice;
    fn and(other: Choice): Choice;
    fn or(other: Choice): Choice;
    fn xor(other: Choice): Choice;
}

export fn mask8(c: Choice): u8;
export fn mask32(c: Choice): u32;
export fn mask64(c: Choice): u64;
export fn ctIsZeroU8(x: u8): Choice;
export fn ctIsZeroU32(x: u32): Choice;
export fn ctIsZeroU64(x: u64): Choice;
export fn ctEqU8(a: u8, b: u8): Choice;
export fn ctEqU32(a: u32, b: u32): Choice;
export fn ctEqU64(a: u64, b: u64): Choice;
export fn ctEqBytes(a: &[]u8, b: &[]u8): Choice;
export fn ctLtU64(a: u64, b: u64): Choice;
export fn ctGtU64(a: u64, b: u64): Choice;
export fn ctSelectU8(a: u8, b: u8, c: Choice): u8;
export fn ctSelectU32(a: u32, b: u32, c: Choice): u32;
export fn ctSelectU64(a: u64, b: u64, c: Choice): u64;
export fn ctSelectBytes(dst: []u8, a: &[]u8, b: &[]u8, c: Choice): usize;
export fn ctCondSwapU64(a: &mut u64, b: &mut u64, c: Choice);
export fn zeroizeBytes(buf: []u8);
