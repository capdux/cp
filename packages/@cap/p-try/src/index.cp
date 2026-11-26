// @cap/p-try — Zero-heap structured execution outcome wrapper.
// Clean-room MIT port of npm `p-try` v3 (160M+/week).

/// Unified outcome wrapper capturing success values (`i64` / `string`) or
/// structured errors (`err_code` / `err_msg`) without throwing or heap allocation.
pub struct TryOutcome {
    pub ok: bool,
    pub value_i64: i64,
    pub value_str: string,
    pub err_code: i32,
    pub err_msg: string,

    pub static fn okInt(v: i64): TryOutcome {
        return TryOutcome {
            ok: true,
            value_i64: v,
            value_str: "",
            err_code: 0,
            err_msg: "",
        };
    }

    pub static fn okStr(s: string): TryOutcome {
        return TryOutcome {
            ok: true,
            value_i64: 0,
            value_str: s,
            err_code: 0,
            err_msg: "",
        };
    }

    pub static fn err(code: i32, msg: string): TryOutcome {
        return TryOutcome {
            ok: false,
            value_i64: 0,
            value_str: "",
            err_code: code,
            err_msg: msg,
        };
    }

    pub fn isOk(): bool {
        return _.ok;
    }

    pub fn isErr(): bool {
        return !_.ok;
    }
}

fn ptryIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13;
}

fn ptryIsDigit(ch: u8): bool {
    return ch >= 48 && ch <= 57;
}

fn ptryI64Min(): i64 {
    return (-9223372036854775807 as i64) - (1 as i64);
}

fn ptryAbsU64(v: i64): u64 {
    if (v >= 0) {
        return v as u64;
    }
    if (v == ptryI64Min()) {
        return 9223372036854775808 as u64;
    }
    return (-v) as u64;
}

/// Constructs a successful `TryOutcome` holding an `i64` value.
pub fn tryOkInt(v: i64): TryOutcome {
    return TryOutcome.okInt(v);
}

/// Constructs a successful `TryOutcome` holding a `string` value.
pub fn tryOkStr(s: string): TryOutcome {
    return TryOutcome.okStr(s);
}

/// Constructs a failed `TryOutcome` holding an error code and message.
pub fn tryErr(code: i32, msg: string): TryOutcome {
    return TryOutcome.err(code, msg);
}

/// Returns `o.value_i64` when `o.ok` is true, otherwise returns `fallback`.
pub fn unwrapOrInt(o: TryOutcome, fallback: i64): i64 {
    if (o.ok) {
        return o.value_i64;
    }
    return fallback;
}

/// Returns `o.value_str` when `o.ok` is true, otherwise returns `fallback`.
pub fn unwrapOrStr(o: TryOutcome, fallback: string): string {
    if (o.ok) {
        return o.value_str;
    }
    return fallback;
}

/// Safely parses a signed 64-bit decimal integer from `s`.
/// Leading/trailing ASCII whitespace and an optional `+`/`-` sign are accepted.
/// Empty input (code 1), invalid characters (code 2), and overflow (code 3)
/// are returned as structured `TryOutcome` errors rather than panicking.
pub fn tryParseI64(s: string): TryOutcome {
    let start: usize = 0;
    let end: usize = s.len;

    while (start < end && ptryIsSpace(s[start])) {
        start = start + 1;
    }
    while (end > start && ptryIsSpace(s[end - 1])) {
        end = end - 1;
    }

    if (start >= end) {
        return tryErr(1, "empty input");
    }

    let neg = false;
    let pos = start;
    if (s[pos] == 45) {
        neg = true;
        pos = pos + 1;
    } else if (s[pos] == 43) {
        pos = pos + 1;
    }

    if (pos >= end) {
        return tryErr(2, "invalid integer");
    }

    let limit: u64 = if (neg) { 9223372036854775808 as u64 } else { 9223372036854775807 as u64 };
    let acc: u64 = 0;

    while (pos < end) {
        let ch = s[pos];
        if (!ptryIsDigit(ch)) {
            return tryErr(2, "invalid integer");
        }
        let digit: u64 = (ch - 48) as u64;
        if (acc > (limit - digit) / 10) {
            return tryErr(3, "integer overflow");
        }
        acc = (acc * 10) + digit;
        pos = pos + 1;
    }

    if (neg) {
        if (acc == (9223372036854775808 as u64)) {
            return tryOkInt(ptryI64Min());
        }
        return tryOkInt(-(acc as i64));
    }
    return tryOkInt(acc as i64);
}

/// Safely divides `a` by `b`.
/// Division by zero (code 4) and `i64::MIN / -1` overflow (code 3) are returned
/// as structured `TryOutcome` errors rather than panicking.
pub fn tryDivI64(a: i64, b: i64): TryOutcome {
    if (b == 0) {
        return tryErr(4, "division by zero");
    }
    if (a == ptryI64Min() && b == -1) {
        return tryErr(3, "integer overflow");
    }
    let neg = (a < 0) != (b < 0);
    let q = ptryAbsU64(a) / ptryAbsU64(b);
    if (neg) {
        if (q == (9223372036854775808 as u64)) {
            return tryOkInt(ptryI64Min());
        }
        return tryOkInt(-(q as i64));
    }
    return tryOkInt(q as i64);
}

/// Short-circuit chains a follow-up step onto `first`.
/// If `first` is already an error, returns `first` unchanged; otherwise
/// produces `tryOkInt(next_val)` when `next_ok` is true or `tryErr(next_err_code, next_err_msg)`.
pub fn tryChain(
    first: TryOutcome,
    next_ok: bool,
    next_val: i64,
    next_err_code: i32,
    next_err_msg: string,
): TryOutcome {
    if (!first.ok) {
        return first;
    }
    if (next_ok) {
        return tryOkInt(next_val);
    }
    return tryErr(next_err_code, next_err_msg);
}
