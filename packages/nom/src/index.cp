// packages/@cap/nom-parse/src/index.cp
// Zero-allocation parser combinator primitives inspired by Rust `nom` v7.
// Clean-room MIT implementation.

import { eqAscii } from "../../unicase/src/index.cp";

// ---------------------------------------------------------------------------
// Public Result Types
// ---------------------------------------------------------------------------

pub struct ParseRes {
    pub ok: bool,
    pub rest: string,
    pub matched: string,
}

pub struct IntRes {
    pub ok: bool,
    pub rest: string,
    pub value: i64,
}

pub struct UintRes {
    pub ok: bool,
    pub rest: string,
    pub value: u64,
}

// ---------------------------------------------------------------------------
// Private Helpers (prefixed with nom)
// ---------------------------------------------------------------------------

fn nomFail(input: string): ParseRes {
    return ParseRes{ ok: false, rest: input, matched: "" };
}

fn nomOk(input: string, split_at: usize): ParseRes {
    return ParseRes{
        ok: true,
        rest: input[split_at..input.len],
        matched: input[0..split_at],
    };
}

fn nomContainsByte(set: string, b: u8): bool {
    let i: usize = 0;
    while (i < set.len) {
        if (set[i] == b) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn nomIsDigit(b: u8): bool {
    return b >= ('0' as u8) && b <= ('9' as u8);
}

fn nomIsHexDigit(b: u8): bool {
    return (b >= ('0' as u8) && b <= ('9' as u8))
        || (b >= ('a' as u8) && b <= ('f' as u8))
        || (b >= ('A' as u8) && b <= ('F' as u8));
}

fn nomHexVal(b: u8): u64 {
    if (b >= ('0' as u8) && b <= ('9' as u8)) {
        return (b - ('0' as u8)) as u64;
    }
    if (b >= ('a' as u8) && b <= ('f' as u8)) {
        return ((b - ('a' as u8)) as u64) + (10 as u64);
    }
    return ((b - ('A' as u8)) as u64) + (10 as u64);
}

fn nomIsAlpha(b: u8): bool {
    return (b >= ('a' as u8) && b <= ('z' as u8))
        || (b >= ('A' as u8) && b <= ('Z' as u8));
}

fn nomIsAlnum(b: u8): bool {
    return nomIsAlpha(b) || nomIsDigit(b);
}

fn nomIsSpace(b: u8): bool {
    return b == (' ' as u8) || b == ('\t' as u8);
}

fn nomIsMultispace(b: u8): bool {
    return b == (' ' as u8) || b == ('\t' as u8) || b == ('\r' as u8) || b == ('\n' as u8);
}

fn nomSliceEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 1. Byte & Tag Combinators
// ---------------------------------------------------------------------------

/// Recognizes an exact literal prefix `expected`.
pub fn tag(input: string, expected: string): ParseRes {
    if (input.len < expected.len) {
        return nomFail(input);
    }
    if (nomSliceEq(input[0..expected.len], expected)) {
        return nomOk(input, expected.len);
    }
    return nomFail(input);
}

/// Recognizes an ASCII case-insensitive literal prefix `expected` (reuses `@cap/unicase`).
pub fn tagNoCase(input: string, expected: string): ParseRes {
    if (input.len < expected.len) {
        return nomFail(input);
    }
    if (eqAscii(input[0..expected.len], expected)) {
        return nomOk(input, expected.len);
    }
    return nomFail(input);
}

/// Consumes exactly `count` bytes from `input`.
pub fn take(input: string, count: usize): ParseRes {
    if (input.len < count) {
        return nomFail(input);
    }
    return nomOk(input, count);
}

/// Returns the longest slice up to (but not including) the first occurrence of `needle`.
pub fn takeUntil(input: string, needle: string): ParseRes {
    if (needle.len == 0) {
        return nomOk(input, 0);
    }
    if (input.len < needle.len) {
        return nomFail(input);
    }
    const limit: usize = input.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        if (nomSliceEq(input[i..(i + needle.len)], needle)) {
            return nomOk(input, i);
        }
        i += 1;
    }
    return nomFail(input);
}

/// Matches the longest non-empty prefix containing none of `forbidden_chars`.
pub fn isNot(input: string, forbidden_chars: string): ParseRes {
    let i: usize = 0;
    while (i < input.len) {
        if (nomContainsByte(forbidden_chars, input[i])) {
            break;
        }
        i += 1;
    }
    if (i == 0) {
        return nomFail(input);
    }
    return nomOk(input, i);
}

/// Matches the longest non-empty prefix containing only bytes in `allowed_chars`.
pub fn isA(input: string, allowed_chars: string): ParseRes {
    let i: usize = 0;
    while (i < input.len) {
        if (!nomContainsByte(allowed_chars, input[i])) {
            break;
        }
        i += 1;
    }
    if (i == 0) {
        return nomFail(input);
    }
    return nomOk(input, i);
}

// ---------------------------------------------------------------------------
// 2. Character Class Combinators
// ---------------------------------------------------------------------------

pub fn digit0(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && nomIsDigit(input[i])) {
        i += 1;
    }
    return nomOk(input, i);
}

pub fn digit1(input: string): ParseRes {
    const r = digit0(input);
    if (r.matched.len == 0) {
        return nomFail(input);
    }
    return r;
}

pub fn hexDigit1(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && nomIsHexDigit(input[i])) {
        i += 1;
    }
    if (i == 0) {
        return nomFail(input);
    }
    return nomOk(input, i);
}

pub fn alpha0(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && nomIsAlpha(input[i])) {
        i += 1;
    }
    return nomOk(input, i);
}

pub fn alpha1(input: string): ParseRes {
    const r = alpha0(input);
    if (r.matched.len == 0) {
        return nomFail(input);
    }
    return r;
}

pub fn alphanumeric0(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && nomIsAlnum(input[i])) {
        i += 1;
    }
    return nomOk(input, i);
}

pub fn alphanumeric1(input: string): ParseRes {
    const r = alphanumeric0(input);
    if (r.matched.len == 0) {
        return nomFail(input);
    }
    return r;
}

pub fn space0(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && nomIsSpace(input[i])) {
        i += 1;
    }
    return nomOk(input, i);
}

pub fn space1(input: string): ParseRes {
    const r = space0(input);
    if (r.matched.len == 0) {
        return nomFail(input);
    }
    return r;
}

pub fn multispace0(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && nomIsMultispace(input[i])) {
        i += 1;
    }
    return nomOk(input, i);
}

pub fn multispace1(input: string): ParseRes {
    const r = multispace0(input);
    if (r.matched.len == 0) {
        return nomFail(input);
    }
    return r;
}

/// Recognizes `\r\n` or `\n`.
pub fn lineEnding(input: string): ParseRes {
    if (input.len >= 2 && input[0] == ('\r' as u8) && input[1] == ('\n' as u8)) {
        return nomOk(input, 2);
    }
    if (input.len >= 1 && input[0] == ('\n' as u8)) {
        return nomOk(input, 1);
    }
    return nomFail(input);
}

/// Recognizes zero or more bytes that are neither `\r` nor `\n`.
pub fn notLineEnding(input: string): ParseRes {
    let i: usize = 0;
    while (i < input.len && input[i] != ('\r' as u8) && input[i] != ('\n' as u8)) {
        i += 1;
    }
    return nomOk(input, i);
}

// ---------------------------------------------------------------------------
// 3. Sequence Combinators
// ---------------------------------------------------------------------------

/// Extracts the slice delimited by `open` and `close`, consuming both delimiters.
pub fn delimitedTag(input: string, open: string, close: string): ParseRes {
    const r_open = tag(input, open);
    if (!r_open.ok) {
        return nomFail(input);
    }
    const r_mid = takeUntil(r_open.rest, close);
    if (!r_mid.ok) {
        return nomFail(input);
    }
    const r_close = tag(r_mid.rest, close);
    if (!r_close.ok) {
        return nomFail(input);
    }
    return ParseRes{
        ok: true,
        rest: r_close.rest,
        matched: r_mid.matched,
    };
}

/// Consumes `prefix` and returns the remaining slice as both `matched` and `rest`.
pub fn precededTag(input: string, prefix: string): ParseRes {
    const r = tag(input, prefix);
    if (!r.ok) {
        return nomFail(input);
    }
    return ParseRes{
        ok: true,
        rest: r.rest,
        matched: r.rest,
    };
}

/// Extracts the slice preceding `suffix` and consumes `suffix`.
pub fn terminatedTag(input: string, suffix: string): ParseRes {
    const r_body = takeUntil(input, suffix);
    if (!r_body.ok) {
        return nomFail(input);
    }
    const r_suf = tag(r_body.rest, suffix);
    if (!r_suf.ok) {
        return nomFail(input);
    }
    return ParseRes{
        ok: true,
        rest: r_suf.rest,
        matched: r_body.matched,
    };
}

/// Splits `input` at the first `sep`: `matched` is the left side, `rest` is the right side.
pub fn separatedPairTag(input: string, sep: string): ParseRes {
    return terminatedTag(input, sep);
}

// ---------------------------------------------------------------------------
// 4. Numeric Parsers (overflow-checked)
// ---------------------------------------------------------------------------

const NOM_I64_MAX_U64: u64 = 9223372036854775807 as u64;
const NOM_I64_MIN_ABS_U64: u64 = (9223372036854775807 as u64) + (1 as u64);
const NOM_U64_MAX: u64 = ((9223372036854775807 as u64) << (1 as u6)) | (1 as u64);

/// Parses a base-10 unsigned 64-bit integer from the beginning of `input`.
pub fn parseU64(input: string): UintRes {
    if (input.len == 0 || !nomIsDigit(input[0])) {
        return UintRes{ ok: false, rest: input, value: 0 };
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < input.len && nomIsDigit(input[i])) {
        const d: u64 = (input[i] - ('0' as u8)) as u64;
        if (val > (NOM_U64_MAX - d) / (10 as u64)) {
            return UintRes{ ok: false, rest: input, value: 0 };
        }
        val = val * (10 as u64) + d;
        i += 1;
    }
    return UintRes{
        ok: true,
        rest: input[i..input.len],
        value: val,
    };
}

/// Parses a base-10 signed 64-bit integer (optional `+`/`-` sign) from the beginning of `input`.
pub fn parseI64(input: string): IntRes {
    if (input.len == 0) {
        return IntRes{ ok: false, rest: input, value: 0 };
    }
    let neg: bool = false;
    let i: usize = 0;
    if (input[0] == ('-' as u8)) {
        neg = true;
        i = 1;
    } else if (input[0] == ('+' as u8)) {
        i = 1;
    }
    if (i >= input.len || !nomIsDigit(input[i])) {
        return IntRes{ ok: false, rest: input, value: 0 };
    }

    const limit: u64 = if (neg) { NOM_I64_MIN_ABS_U64 } else { NOM_I64_MAX_U64 };
    let acc: u64 = 0;
    while (i < input.len && nomIsDigit(input[i])) {
        const d: u64 = (input[i] - ('0' as u8)) as u64;
        if (acc > (limit - d) / (10 as u64)) {
            return IntRes{ ok: false, rest: input, value: 0 };
        }
        acc = acc * (10 as u64) + d;
        i += 1;
    }

    let result: i64 = 0;
    if (neg) {
        if (acc == NOM_I64_MIN_ABS_U64) {
            result = (-9223372036854775807 as i64) - (1 as i64);
        } else {
            result = -(acc as i64);
        }
    } else {
        result = acc as i64;
    }

    return IntRes{
        ok: true,
        rest: input[i..input.len],
        value: result,
    };
}

/// Parses a hexadecimal unsigned 64-bit integer (optional `0x`/`0X` prefix) from `input`.
pub fn parseHexU64(input: string): UintRes {
    if (input.len == 0) {
        return UintRes{ ok: false, rest: input, value: 0 };
    }
    let i: usize = 0;
    if (input.len >= 3
        && input[0] == ('0' as u8)
        && (input[1] == ('x' as u8) || input[1] == ('X' as u8))
        && nomIsHexDigit(input[2]))
    {
        i = 2;
    }
    if (i >= input.len || !nomIsHexDigit(input[i])) {
        return UintRes{ ok: false, rest: input, value: 0 };
    }
    let val: u64 = 0;
    while (i < input.len && nomIsHexDigit(input[i])) {
        const d: u64 = nomHexVal(input[i]);
        if (val > (NOM_U64_MAX - d) / (16 as u64)) {
            return UintRes{ ok: false, rest: input, value: 0 };
        }
        val = (val << (4 as u6)) | d;
        i += 1;
    }
    return UintRes{
        ok: true,
        rest: input[i..input.len],
        value: val,
    };
}
