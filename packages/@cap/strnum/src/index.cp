// @cap/strnum — Zero-heap configurable string-to-number parser
// Clean-room MIT implementation inspired by npm:strnum (NaturalIntelligence/strnum).

pub struct StrNumOptions {
    hex: bool,
    leading_zeros: bool,
    e_notation: bool,
}

pub struct StrNumResult {
    is_number: bool,
    value: f64,
    is_int: bool,
    int_value: i64,
}

fn snumInvalid(): StrNumResult {
    return StrNumResult{
        is_number: false,
        value: 0.0,
        is_int: false,
        int_value: 0,
    };
}

fn snumIsSpace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn snumHexDigit(ch: u8): i32 {
    if (ch >= '0' && ch <= '9') {
        return i32(ch - '0');
    }
    if (ch >= 'a' && ch <= 'f') {
        return i32(ch - 'a') + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return i32(ch - 'A') + 10;
    }
    return -1;
}

fn snumScalePow10(val: f64, exp10: i32): f64 {
    let r = val;
    if (exp10 > 0) {
        let e = exp10;
        while (e > 0) : (e -= 1) {
            r *= 10.0;
        }
    } else if (exp10 < 0) {
        let e = -exp10;
        while (e > 0) : (e -= 1) {
            r /= 10.0;
        }
    }
    return r;
}

/// Parse `input` into a numeric result according to `opts`.
/// Returns `is_number = false` safely on empty, whitespace-only, invalid, or disallowed formats.
pub fn parseStrNum(input: string, opts: StrNumOptions): StrNumResult {
    let start: usize = 0;
    let end: usize = input.len;

    while (start < end && snumIsSpace(input[start])) : (start += 1) {}
    while (end > start && snumIsSpace(input[end - 1])) : (end -= 1) {}

    if (start >= end) {
        return snumInvalid();
    }

    let i = start;
    let neg = false;
    if (input[i] == '-') {
        neg = true;
        i += 1;
    } else if (input[i] == '+') {
        i += 1;
    }

    if (i >= end) {
        return snumInvalid();
    }

    // Hexadecimal branch: 0x... / 0X...
    if (i + 1 < end && input[i] == '0' && (input[i + 1] == 'x' || input[i + 1] == 'X')) {
        if (!opts.hex) {
            return snumInvalid();
        }
        i += 2;
        if (i >= end) {
            return snumInvalid();
        }
        let acc: u64 = 0;
        let overflow = false;
        while (i < end) : (i += 1) {
            const d = snumHexDigit(input[i]);
            if (d < 0) {
                return snumInvalid();
            }
            if (acc > 0x07FF_FFFF_FFFF_FFFF) {
                overflow = true;
            } else {
                acc = (acc << 4) | u64(d);
            }
        }
        if (overflow) {
            return snumInvalid();
        }
        const signed: i64 = if (neg) -i64(acc) else i64(acc);
        return StrNumResult{
            is_number: true,
            value: f64(signed),
            is_int: true,
            int_value: signed,
        };
    }

    // Leading zeros check when opts.leading_zeros == false:
    // "0", "0.5", "-0.5", "0e2" are valid; "00", "007", "-01", "00.5" are rejected.
    if (!opts.leading_zeros && input[i] == '0' && i + 1 < end) {
        const next_ch = input[i + 1];
        if (next_ch >= '0' && next_ch <= '9') {
            return snumInvalid();
        }
    }

    // Integer part
    let int_digits: usize = 0;
    let int_acc: u64 = 0;
    let int_overflow = false;
    let fval: f64 = 0.0;

    while (i < end && input[i] >= '0' && input[i] <= '9') : (i += 1) {
        const d = u64(input[i] - '0');
        int_digits += 1;
        fval = fval * 10.0 + f64(d);
        if (!int_overflow) {
            if (int_acc > 922337203685477580) {
                int_overflow = true;
            } else {
                const next_acc = int_acc * 10 + d;
                if ((!neg && next_acc > 9223372036854775807) || (neg && next_acc > 9223372036854775807)) {
                    int_overflow = true;
                } else {
                    int_acc = next_acc;
                }
            }
        }
    }

    // Fractional part
    let has_dot = false;
    let frac_digits: usize = 0;
    if (i < end && input[i] == '.') {
        has_dot = true;
        i += 1;
        let place: f64 = 0.1;
        while (i < end && input[i] >= '0' && input[i] <= '9') : (i += 1) {
            const d = f64(input[i] - '0');
            fval += d * place;
            place *= 0.1;
            frac_digits += 1;
        }
        if (frac_digits == 0) {
            return snumInvalid();
        }
    }

    if (int_digits == 0 && frac_digits == 0) {
        return snumInvalid();
    }

    // Exponent part
    let has_exp = false;
    let exp_val: i32 = 0;
    if (i < end && (input[i] == 'e' || input[i] == 'E')) {
        if (!opts.e_notation) {
            return snumInvalid();
        }
        has_exp = true;
        i += 1;
        let exp_neg = false;
        if (i < end && (input[i] == '+' || input[i] == '-')) {
            exp_neg = input[i] == '-';
            i += 1;
        }
        let exp_digits: usize = 0;
        let exp_abs: i32 = 0;
        while (i < end && input[i] >= '0' && input[i] <= '9') : (i += 1) {
            exp_digits += 1;
            if (exp_abs < 10000) {
                exp_abs = exp_abs * 10 + i32(input[i] - '0');
            }
        }
        if (exp_digits == 0) {
            return snumInvalid();
        }
        exp_val = if (exp_neg) -exp_abs else exp_abs;
    }

    // Any trailing unconsumed character is invalid
    if (i != end) {
        return snumInvalid();
    }

    if (has_exp) {
        if (exp_val > 308 && fval != 0.0) {
            return snumInvalid();
        }
        if (exp_val < -324) {
            fval = 0.0;
        } else {
            fval = snumScalePow10(fval, exp_val);
            if (std.math.isInf(fval) || std.math.isNan(fval)) {
                return snumInvalid();
            }
        }
    }

    if (neg) {
        fval = -fval;
    }

    if (!has_dot && !has_exp && !int_overflow) {
        const signed: i64 = if (neg) -i64(int_acc) else i64(int_acc);
        return StrNumResult{
            is_number: true,
            value: fval,
            is_int: true,
            int_value: signed,
        };
    }

    return StrNumResult{
        is_number: true,
        value: fval,
        is_int: false,
        int_value: 0,
    };
}
