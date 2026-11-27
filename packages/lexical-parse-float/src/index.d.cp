// Type definitions for CP

export class ParseFloatResult {
    ok: bool,
    value: f64,
    consumed: usize,
}

export class ParseIntResult {
    ok: bool,
    value: i64,
    consumed: usize,
}

export fn lexpToLower(c: u8): u8;
export fn lexpEqIgnoreCase(a: string, b: string): bool;
export fn lexpDecodeDigit(c: u8): i32;
export fn lexpScalePow10(val_in: f64, exp10: i32): f64;
export fn parseFloat(s: string): ParseFloatResult;
export fn parseIntRadix(s: string, radix: u32): ParseIntResult;
