// Type definitions for CP

import { formatU64 } from "../../itoa/src/index.cp";
export class RyuDecimal {
    sig: u64,
    exp10_msb: i32,
}

export fn ryuPow10U64(n: u8): u64;
export fn ryuScalePow10(x: f64, p_in: i32): f64;
export fn ryuStripTrailingZeros(sig_in: u64): u64;
export fn ryuRoundSig(m17: u64, drop_digits: u8): u64;
export fn ryuExtractShortestF64(abs_val: f64): RyuDecimal;
export fn ryuExtractShortestF32(abs_f32: f32): RyuDecimal;
export fn ryuCopySlice(dst: []u8, pos: &mut usize, src: &[]u8): bool;
export fn ryuPushByte(dst: []u8, pos: &mut usize, b: u8): bool;
export fn ryuRenderDecimal(neg: bool, dec_val: RyuDecimal, out: []u8): usize;
export fn ryuRoundAsciiDigits(digits: &[]u8, keep: usize, rounded: []u8): bool;
export fn isNanF64(val: f64): bool;
export fn isInfF64(val: f64): bool;
export fn isFiniteF64(val: f64): bool;
export fn formatF64(val: f64, out: []u8): usize;
export fn formatF64Finite(val: f64, out: []u8): usize;
export fn formatF32(val: f32, out: []u8): usize;
export fn formatFixedF64(val: f64, decimals: u8, out: []u8): usize;
export fn formatExpF64(val: f64, precision: u8, out: []u8): usize;
