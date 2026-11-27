// Type definitions for CP

export class FormatSpec {
    valid: bool,
    fill: u8,
    @"align": u8,
    sign: u8,
    symbol: u8,
    zero: bool,
    width: usize,
    comma: bool,
    precision: i32,
    trim: bool,
    ty: u8,
}

export fn d3fIsAlign(ch: u8): bool;
export fn d3fIsType(ch: u8): bool;
export fn parseFormatSpec(spec: string): FormatSpec;
export fn d3fPow10F64(exp: i32): f64;
export fn d3fPow10U64(exp: i32): u64;
export fn d3fWriteU64Base(val: u64, base: u64, upper: bool, comma: bool, out: []u8): usize;
export fn d3fWriteFixed(abs_val: f64, prec_in: i32, comma: bool, trim: bool, out: []u8): usize;
export fn d3fAppendSiSuffix(exp3: i32, out: []u8, pos: usize): usize;
export fn d3fDisplayLen(slice: &[]u8): usize;
export fn d3fCopySlice(dst: []u8, pos: usize, src: &[]u8): usize;
export fn formatNumber(spec: string, value: f64, out: []u8): usize;
