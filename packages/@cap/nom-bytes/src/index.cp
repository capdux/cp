// packages/@cap/nom-bytes/src/index.cp
// Zero-allocation byte-level zero-copy parser combinators (winnow / nom parity).
// MIT clean-room pure CP implementation.

/// Zero-copy byte cursor over an input string slice.
pub struct Cursor {
    pub data: string,
    pub pos: usize,

    pub static fn new(data: string): Cursor {
        return Cursor{
            data: data,
            pos: 0,
        };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    pub fn isEmpty(): bool {
        return _.pos >= _.data.len;
    }

    /// Peek at the next byte as `0..255`, or `-1` at EOF.
    pub fn peek(): i32 {
        if (_.pos >= _.data.len) {
            return -1 as i32;
        }
        return _.data[_.pos] as i32;
    }

    /// Return the unconsumed remainder slice.
    pub fn rest(): string {
        if (_.pos >= _.data.len) {
            return "";
        }
        return _.data[_.pos.._.data.len];
    }

    /// Return the slice from `start` up to current cursor position `pos`.
    pub fn sliceFrom(start: usize): string {
        if (start >= _.pos || _.pos > _.data.len) {
            return "";
        }
        return _.data[start.._.pos];
    }
}

/// Zero-copy span result referencing `cursor.data[start..(start + len)]`.
pub struct SpanResult {
    pub ok: bool,
    pub start: usize,
    pub len: usize,
}

/// Parsed signed 64-bit integer result.
pub struct IntResult {
    pub ok: bool,
    pub value: i64,
}

/// Parsed unsigned 64-bit integer result.
pub struct UintResult {
    pub ok: bool,
    pub value: u64,
}

fn nbToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn nbIsDigit(c: u8): bool {
    return c >= ('0' as u8) && c <= ('9' as u8);
}

fn nbIsAlpha(c: u8): bool {
    return (c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('A' as u8) && c <= ('Z' as u8));
}

fn nbIsHex(c: u8): bool {
    return nbIsDigit(c) || (c >= ('a' as u8) && c <= ('f' as u8)) || (c >= ('A' as u8) && c <= ('F' as u8));
}

fn nbFailSpan(start: usize): SpanResult {
    return SpanResult{ ok: false, start: start, len: 0 };
}

/// Match exact byte literal `lit` at cursor. Advances cursor on match; leaves cursor unchanged on failure.
pub fn tag(cur: *Cursor, lit: string): bool {
    const p = cur.*.pos;
    if (p + lit.len > cur.*.data.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (cur.*.data[p + i] != lit[i]) {
            return false;
        }
        i += 1;
    }
    cur.*.pos = p + lit.len;
    return true;
}

/// Case-insensitive ASCII literal match at cursor. Advances cursor on match; unchanged on failure.
pub fn tagNoCase(cur: *Cursor, lit: string): bool {
    const p = cur.*.pos;
    if (p + lit.len > cur.*.data.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (nbToLower(cur.*.data[p + i]) != nbToLower(lit[i])) {
            return false;
        }
        i += 1;
    }
    cur.*.pos = p + lit.len;
    return true;
}

/// Match a single byte `ch`. Advances cursor by 1 on match; unchanged on failure.
pub fn charLit(cur: *Cursor, ch: u8): bool {
    const p = cur.*.pos;
    if (p >= cur.*.data.len || cur.*.data[p] != ch) {
        return false;
    }
    cur.*.pos = p + 1;
    return true;
}

/// Consume exactly `count` bytes. Fails without advancing if fewer than `count` bytes remain.
pub fn take(cur: *Cursor, count: usize): SpanResult {
    const p = cur.*.pos;
    if (p + count > cur.*.data.len) {
        return nbFailSpan(p);
    }
    cur.*.pos = p + count;
    return SpanResult{ ok: true, start: p, len: count };
}

/// Consume bytes up to (but not including) the first occurrence of `needle`.
/// Fails without advancing if `needle` is not found.
pub fn takeUntil(cur: *Cursor, needle: string): SpanResult {
    const p = cur.*.pos;
    if (needle.len == 0) {
        return SpanResult{ ok: true, start: p, len: 0 };
    }
    const d = cur.*.data;
    if (p + needle.len > d.len) {
        return nbFailSpan(p);
    }
    let i = p;
    while (i + needle.len <= d.len) {
        let matched = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (d[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) {
            cur.*.pos = i;
            return SpanResult{ ok: true, start: p, len: i - p };
        }
        i += 1;
    }
    return nbFailSpan(p);
}

/// Consume bytes until byte `stop` is encountered or EOF (always succeeds, possibly with length 0).
pub fn takeTillByte(cur: *Cursor, stop: u8): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && d[i] != stop) {
        i += 1;
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Consume zero or more spaces or tabs (`' '`, `'\t'`).
pub fn space0(cur: *Cursor): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && (d[i] == (' ' as u8) || d[i] == ('\t' as u8))) {
        i += 1;
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Consume zero or more ASCII whitespace characters (`' '`, `'\t'`, `'\r'`, `'\n'`).
pub fn multispace0(cur: *Cursor): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && (d[i] == (' ' as u8) || d[i] == ('\t' as u8) || d[i] == ('\r' as u8) || d[i] == ('\n' as u8))) {
        i += 1;
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Consume one or more ASCII digits `'0'..'9'`.
pub fn digit1(cur: *Cursor): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && nbIsDigit(d[i])) {
        i += 1;
    }
    if (i == p) {
        return nbFailSpan(p);
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Consume one or more ASCII alphabetic letters `'a'..'z' | 'A'..'Z'`.
pub fn alpha1(cur: *Cursor): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && nbIsAlpha(d[i])) {
        i += 1;
    }
    if (i == p) {
        return nbFailSpan(p);
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Consume one or more ASCII alphanumeric characters.
pub fn alphanumeric1(cur: *Cursor): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && (nbIsAlpha(d[i]) || nbIsDigit(d[i]))) {
        i += 1;
    }
    if (i == p) {
        return nbFailSpan(p);
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Consume one or more ASCII hexadecimal digits.
pub fn hexDigit1(cur: *Cursor): SpanResult {
    const p = cur.*.pos;
    const d = cur.*.data;
    let i = p;
    while (i < d.len && nbIsHex(d[i])) {
        i += 1;
    }
    if (i == p) {
        return nbFailSpan(p);
    }
    cur.*.pos = i;
    return SpanResult{ ok: true, start: p, len: i - p };
}

/// Parse an unsigned 64-bit decimal integer at cursor with overflow protection.
/// Leaves cursor unchanged on failure or overflow.
pub fn parseU64(cur: *Cursor): UintResult {
    const saved = cur.*.pos;
    const d = cur.*.data;
    if (saved >= d.len || !nbIsDigit(d[saved])) {
        return UintResult{ ok: false, value: 0 as u64 };
    }

    const maxDiv10: u64 = 1844674407370955161 as u64;
    let acc: u64 = 0 as u64;
    let i = saved;
    while (i < d.len && nbIsDigit(d[i])) {
        const digit = (d[i] - ('0' as u8)) as u64;
        if (acc > maxDiv10 || (acc == maxDiv10 && digit > (5 as u64))) {
            return UintResult{ ok: false, value: 0 as u64 };
        }
        acc = (acc * (10 as u64)) + digit;
        i += 1;
    }

    cur.*.pos = i;
    return UintResult{ ok: true, value: acc };
}

/// Parse a signed 64-bit decimal integer (optional `+`/`-` prefix) at cursor with overflow protection.
/// Leaves cursor unchanged on failure or overflow.
pub fn parseI64(cur: *Cursor): IntResult {
    const saved = cur.*.pos;
    const d = cur.*.data;
    if (saved >= d.len) {
        return IntResult{ ok: false, value: 0 as i64 };
    }

    let i = saved;
    let neg = false;
    if (d[i] == ('-' as u8)) {
        neg = true;
        i += 1;
    } else if (d[i] == ('+' as u8)) {
        i += 1;
    }

    if (i >= d.len || !nbIsDigit(d[i])) {
        return IntResult{ ok: false, value: 0 as i64 };
    }

    const maxDiv10: u64 = 922337203685477580 as u64;
    let acc: u64 = 0 as u64;
    while (i < d.len && nbIsDigit(d[i])) {
        const digit = (d[i] - ('0' as u8)) as u64;
        let maxLast: u64 = 7 as u64;
        if (neg) {
            maxLast = 8 as u64;
        }
        if (acc > maxDiv10 || (acc == maxDiv10 && digit > maxLast)) {
            return IntResult{ ok: false, value: 0 as i64 };
        }
        acc = (acc * (10 as u64)) + digit;
        i += 1;
    }

    cur.*.pos = i;
    if (neg) {
        const maxPosU64: u64 = 9223372036854775807 as u64;
        if (acc > maxPosU64) {
            return IntResult{ ok: true, value: (-9223372036854775807 as i64) - (1 as i64) };
        }
        return IntResult{ ok: true, value: -(acc as i64) };
    }
    return IntResult{ ok: true, value: acc as i64 };
}

/// Parse content enclosed between `open` and the first matching `close` byte (e.g. `"(hello)"` -> `"hello"`).
/// Consumes both delimiters on success; rolls back cursor unchanged on failure.
pub fn delimited(cur: *Cursor, open: u8, close: u8): SpanResult {
    const saved = cur.*.pos;
    const d = cur.*.data;
    if (saved >= d.len || d[saved] != open) {
        return nbFailSpan(saved);
    }

    let i = saved + 1;
    while (i < d.len && d[i] != close) {
        i += 1;
    }
    if (i >= d.len) {
        return nbFailSpan(saved);
    }

    cur.*.pos = i + 1;
    return SpanResult{
        ok: true,
        start: saved + 1,
        len: i - (saved + 1),
    };
}

export {
    Cursor,
    SpanResult,
    IntResult,
    UintResult,
    tag,
    tagNoCase,
    charLit,
    take,
    takeUntil,
    takeTillByte,
    space0,
    multispace0,
    digit1,
    alpha1,
    alphanumeric1,
    hexDigit1,
    parseU64,
    parseI64,
    delimited,
};
