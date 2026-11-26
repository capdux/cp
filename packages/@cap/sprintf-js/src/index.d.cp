// Type definitions for CP

export enum FmtArgKind {
    Int,
    Uint,
    Float,
    Str,
    Char,
    Bool
}

export class FmtArg {
    kind: FmtArgKind,
    int_val: i64,
    uint_val: u64,
    float_val: f64,
    str_val: string,
    char_val: u8,
    bool_val: bool,
    fn int(v: i64): FmtArg;
    fn uint(v: u64): FmtArg;
    fn float(v: f64): FmtArg;
    fn str(v: string): FmtArg;
    fn ch(v: u8): FmtArg;
    fn boolean(v: bool): FmtArg;
}

export fn spfPutByte(out: []u8, pos: usize, b: u8): usize;
export fn spfPutBytes(out: []u8, pos: usize, s: &[]u8): usize;
export fn spfPutRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;
export fn spfEmitPadded(out: []u8, pos: usize, sign: u8, body: &[]u8, width: usize, left_align: bool, zero_pad: bool): usize;
export fn spfFormatUintRadix(val: u64, base: u64, upper: bool, buf: []u8): usize;
export fn spfArgAsSigned(arg: FmtArg): i64;
export fn spfArgAsUnsigned(arg: FmtArg): u64;
export fn spfArgAsFloat(arg: FmtArg): f64;
export fn spfFormatFloat(val: f64, prec: usize, plus_sign: bool, width: usize, left_align: bool, zero_pad: bool, out: []u8, pos: usize): usize;
export fn sprintf(fmt: string, args: &[]FmtArg, out: []u8): usize;
