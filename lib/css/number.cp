// Source: Absorbed from /data/dev/bun/src/css/values/number.cp | Synced: 2026-11-22T18:07:28Z
// ============================================================================
// lib/css/number.cp — Zero-Alloc CSS Values Level 4 `<number>` & `<integer>`
// ============================================================================
// Implements W3C CSS Syntax Level 3 / Values Level 4 `<number>` and `<integer>`
// parsing, sign/clamping helpers, and Bun-aligned minimal CSS serialization
// (normalizing `-0` to `0`, omitting leading `0` for `0 < |x| < 1` like `.5` / `-.25`,
// and stripping trailing fractional zeros with zero dynamic allocation).

pub const CSS_INT_MIN: i32 = -2147483648;
pub const CSS_INT_MAX: i32 = 2147483647;

pub err CssNumberError {
    EmptyInput,
    InvalidSyntax,
    Overflow,
    NotFinite,
    BufferTooSmall,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimSpaces(raw: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) start += 1;
    while (end > start && isSpace(raw[end - 1])) end -= 1;
    return raw[start..end];
}

fn isFiniteF64(x: f64): bool {
    return x == x && x <= 1.7976931348623157e308 && x >= -1.7976931348623157e308;
}

fn writeUint(out: []u8, pos: usize, val_in: u64): CssNumberError!usize {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            n += 1;
        }
    }
    if (pos + n > out.len) return CssNumberError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// W3C CSS Values Level 4 `<number>` value.
pub struct CssNumber {
    value: f64 = 0.0,

    static fn fromF64(v: f64): CssNumber {
        return CssNumber{ value: v };
    }

    /// Parses a CSS `<number-token>`: `[+-]? (digits ('.' digits)? | '.' digits) ([eE] [+-]? digits)?`.
    static fn parse(raw: []const u8): CssNumberError!CssNumber {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssNumberError.EmptyInput;

        let i: usize = 0;
        let neg = false;
        if (s[i] == '-') {
            neg = true;
            i += 1;
        } else if (s[i] == '+') {
            i += 1;
        }
        if (i >= s.len) return CssNumberError.InvalidSyntax;

        let int_part: f64 = 0.0;
        let has_int_digits = false;
        while (i < s.len && isDigit(s[i])) {
            has_int_digits = true;
            int_part = int_part * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
            i += 1;
        }

        let frac_part: f64 = 0.0;
        let has_frac_digits = false;
        if (i < s.len && s[i] == '.') {
            i += 1;
            let scale: f64 = 0.1;
            while (i < s.len && isDigit(s[i])) {
                has_frac_digits = true;
                frac_part += @as(f64, @floatFromInt(s[i] - '0')) * scale;
                scale *= 0.1;
                i += 1;
            }
            if (!has_frac_digits) return CssNumberError.InvalidSyntax;
        }

        if (!has_int_digits && !has_frac_digits) return CssNumberError.InvalidSyntax;

        let val = int_part + frac_part;

        // Optional exponent `[eE] [+-]? digits`
        if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
            i += 1;
            if (i >= s.len) return CssNumberError.InvalidSyntax;
            let exp_neg = false;
            if (s[i] == '-') {
                exp_neg = true;
                i += 1;
            } else if (s[i] == '+') {
                i += 1;
            }
            if (i >= s.len || !isDigit(s[i])) return CssNumberError.InvalidSyntax;
            let exp_val: i32 = 0;
            while (i < s.len && isDigit(s[i])) {
                if (exp_val < 1000) {
                    exp_val = exp_val * 10 + @as(i32, @intCast(s[i] - '0'));
                }
                i += 1;
            }
            if (exp_val > 308) {
                if (exp_neg) {
                    val = 0.0;
                } else if (val != 0.0) {
                    return CssNumberError.Overflow;
                }
            } else {
                let k: i32 = 0;
                while (k < exp_val) {
                    if (exp_neg) {
                        val *= 0.1;
                    } else {
                        val *= 10.0;
                    }
                    k += 1;
                }
            }
        }

        if (i != s.len) return CssNumberError.InvalidSyntax;
        if (!isFiniteF64(val)) return CssNumberError.Overflow;
        if (neg && val != 0.0) val = -val;
        return CssNumber{ value: val };
    }

    fn sign(): f64 {
        if (_.value == 0.0) return 0.0;
        return if (_.value > 0.0) 1.0 else -1.0;
    }

    fn isZero(): bool {
        return _.value == 0.0;
    }

    fn isNegative(): bool {
        return _.value < 0.0;
    }

    fn abs(): CssNumber {
        return CssNumber{ value: if (_.value < 0.0) -_.value else _.value };
    }

    fn clamp(lo: f64, hi: f64): CssNumber {
        let v = _.value;
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        return CssNumber{ value: v };
    }

    fn approxEql(other: CssNumber, eps: f64): bool {
        const d = if (_.value > other.value) (_.value - other.value) else (other.value - _.value);
        return d <= eps;
    }

    /// Serializes `<number>` in canonical minimal CSS form (up to 6 fractional digits):
    /// - `-0.0` / `0.0` -> `"0"`
    /// - `0 < |x| < 1` -> omits leading zero (`0.5` -> `".5"`, `-0.25` -> `"-.25"`)
    /// - strips trailing fractional zeros (`12.500` -> `"12.5"`, `42.0` -> `"42"`)
    fn toCss(out: []u8): CssNumberError!usize {
        if (!isFiniteF64(_.value)) return CssNumberError.NotFinite;
        if (_.value == 0.0) {
            if (out.len < 1) return CssNumberError.BufferTooSmall;
            out[0] = '0';
            return 1;
        }

        const neg = _.value < 0.0;
        const mag = if (neg) -_.value else _.value;
        if (mag >= 9.0e12) return CssNumberError.Overflow;

        // Round to 6 fractional digits
        const scaled: u64 = @as(u64, @intFromFloat(mag * 1000000.0 + 0.5));
        if (scaled == 0) {
            if (out.len < 1) return CssNumberError.BufferTooSmall;
            out[0] = '0';
            return 1;
        }

        const int_part: u64 = scaled / 1000000;
        let frac_part: u64 = scaled % 1000000;

        let pos: usize = 0;
        if (neg) {
            if (pos >= out.len) return CssNumberError.BufferTooSmall;
            out[pos] = '-';
            pos += 1;
        }

        // Omit leading '0' when int_part == 0 && frac_part > 0 (e.g. ".5", "-.25")
        if (int_part > 0 || frac_part == 0) {
            pos = writeUint(out, pos, int_part)?;
        }

        if (frac_part > 0) {
            if (pos >= out.len) return CssNumberError.BufferTooSmall;
            out[pos] = '.';
            pos += 1;

            let digits: [6]u8 = [0; 6];
            let k: usize = 6;
            while (k > 0) {
                k -= 1;
                digits[k] = @as(u8, @intCast('0' + (frac_part % 10)));
                frac_part /= 10;
            }
            let frac_len: usize = 6;
            while (frac_len > 0 && digits[frac_len - 1] == '0') {
                frac_len -= 1;
            }
            if (pos + frac_len > out.len) return CssNumberError.BufferTooSmall;
            let j: usize = 0;
            while (j < frac_len) {
                out[pos + j] = digits[j];
                j += 1;
            }
            pos += frac_len;
        }

        return pos;
    }
}

/// W3C CSS Values Level 4 `<integer>` value (`i32`).
pub struct CssInteger {
    value: i32 = 0,

    static fn fromI32(v: i32): CssInteger {
        return CssInteger{ value: v };
    }

    /// Parses a strict CSS `<integer>` (`[+-]? digits`, no decimal point or exponent).
    static fn parse(raw: []const u8): CssNumberError!CssInteger {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssNumberError.EmptyInput;

        let i: usize = 0;
        let neg = false;
        if (s[i] == '-') {
            neg = true;
            i += 1;
        } else if (s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || !isDigit(s[i])) return CssNumberError.InvalidSyntax;

        let acc: u64 = 0;
        const limit: u64 = if (neg) 2147483648 else 2147483647;
        while (i < s.len) {
            if (!isDigit(s[i])) return CssNumberError.InvalidSyntax;
            const d: u64 = @as(u64, s[i] - '0');
            if (acc > (limit - d) / 10) return CssNumberError.Overflow;
            acc = acc * 10 + d;
            i += 1;
        }

        if (neg) {
            if (acc == 2147483648) return CssInteger{ value: CSS_INT_MIN };
            return CssInteger{ value: -@as(i32, @intCast(acc)) };
        }
        return CssInteger{ value: @as(i32, @intCast(acc)) };
    }

    fn sign(): i32 {
        if (_.value == 0) return 0;
        return if (_.value > 0) 1 else -1;
    }

    fn clamp(lo: i32, hi: i32): CssInteger {
        let v = _.value;
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        return CssInteger{ value: v };
    }

    fn toCss(out: []u8): CssNumberError!usize {
        let pos: usize = 0;
        let mag: u64 = 0;
        if (_.value < 0) {
            if (out.len < 2) return CssNumberError.BufferTooSmall;
            out[0] = '-';
            pos = 1;
            if (_.value == CSS_INT_MIN) {
                mag = 2147483648;
            } else {
                mag = @as(u64, @intCast(-_.value));
            }
        } else {
            mag = @as(u64, @intCast(_.value));
        }
        return writeUint(out, pos, mag);
    }

    fn eql(other: CssInteger): bool {
        return _.value == other.value;
    }
}
