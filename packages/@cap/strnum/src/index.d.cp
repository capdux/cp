// Type definitions for CP

export class StrNumOptions {
    hex: bool,
    leading_zeros: bool,
    e_notation: bool,
}

export class StrNumResult {
    is_number: bool,
    value: f64,
    is_int: bool,
    int_value: i64,
}

export fn snumInvalid(): StrNumResult;
export fn snumIsSpace(ch: u8): bool;
export fn snumHexDigit(ch: u8): i32;
export fn snumScalePow10(val: f64, exp10: i32): f64;
export fn parseStrNum(input: string, opts: StrNumOptions): StrNumResult;
