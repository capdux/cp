// packages/@cap/nom-parse/src/index.cp
// Zero-allocation parser combinator primitives inspired by Rust `nom` v7.
// Clean-room MIT implementation.
// Accelerated with comptime 256-entry character-class & hex LUTs,
// and 16-lane @Vector(16, u8) SIMD scanning for tag/takeUntil/digit/alpha/alnum/space/lineEnding.

import { eqAscii } from "../../unicase/src/index.cp";

type NomVec16 = @Vector(16, u8);

const NOM_CLS_DIGIT: u8 = 1;
const NOM_CLS_HEX: u8 = 2;
const NOM_CLS_ALPHA: u8 = 4;
const NOM_CLS_SPACE: u8 = 8;
const NOM_CLS_MULTISPACE: u8 = 16;

fn nomBuildClassLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c >= 48 && c <= 57) {
            f |= NOM_CLS_DIGIT | NOM_CLS_HEX;
        }
        if ((c >= 97 && c <= 102) || (c >= 65 && c <= 70)) {
            f |= NOM_CLS_HEX;
        }
        if ((c >= 97 && c <= 122) || (c >= 65 && c <= 90)) {
            f |= NOM_CLS_ALPHA;
        }
        if (c == 32 || c == 9) {
            f |= NOM_CLS_SPACE | NOM_CLS_MULTISPACE;
        }
        if (c == 13 || c == 10) {
            f |= NOM_CLS_MULTISPACE;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

fn nomBuildHexValLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 48 && c <= 57) {
            lut[c] = (c - 48) as u8;
        } else if (c >= 97 && c <= 102) {
            lut[c] = (c - 97 + 10) as u8;
        } else if (c >= 65 && c <= 70) {
            lut[c] = (c - 65 + 10) as u8;
        }
        c += 1;
    }
    return lut;
}

const NOM_CLASS_LUT: [256]u8 = nomBuildClassLut();
const NOM_HEX_LUT: [256]u8 = nomBuildHexValLut();

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
    return (NOM_CLASS_LUT[b] & NOM_CLS_DIGIT) != 0;
}

fn nomIsHexDigit(b: u8): bool {
    return (NOM_CLASS_LUT[b] & NOM_CLS_HEX) != 0;
}

fn nomHexVal(b: u8): u64 {
    return NOM_HEX_LUT[b] as u64;
}

fn nomIsAlpha(b: u8): bool {
    return (NOM_CLASS_LUT[b] & NOM_CLS_ALPHA) != 0;
}

fn nomIsAlnum(b: u8): bool {
    return (NOM_CLASS_LUT[b] & (NOM_CLS_ALPHA | NOM_CLS_DIGIT)) != 0;
}

fn nomIsSpace(b: u8): bool {
    return (NOM_CLASS_LUT[b] & NOM_CLS_SPACE) != 0;
}

fn nomIsMultispace(b: u8): bool {
    return (NOM_CLASS_LUT[b] & NOM_CLS_MULTISPACE) != 0;
}

fn nomSliceEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: NomVec16 = a[i .. i + 16][0..16].*;
        const vb: NomVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va != vb)) {
            return false;
        }
        i += 16;
    }
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
    const first: u8 = needle[0];
    const v_first: NomVec16 = @splat(first);
    let i: usize = 0;
    while (i + 16 <= limit + 1) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_first)) {
            let k: usize = 0;
            while (k < 16 && i + k <= limit) {
                if (input[i + k] == first && nomSliceEq(input[(i + k)..(i + k + needle.len)], needle)) {
                    return nomOk(input, i + k);
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i <= limit) {
        if (input[i] == first && nomSliceEq(input[i..(i + needle.len)], needle)) {
            return nomOk(input, i);
        }
        i += 1;
    }
    return nomFail(input);
}

/// Matches the longest non-empty prefix containing none of `forbidden_chars`.
pub fn isNot(input: string, forbidden_chars: string): ParseRes {
    let i: usize = 0;
    if (forbidden_chars.len == 1) {
        const fb = forbidden_chars[0];
        const v_fb: NomVec16 = @splat(fb);
        while (i + 16 <= input.len) {
            const v: NomVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == v_fb)) {
                break;
            }
            i += 16;
        }
        while (i < input.len && input[i] != fb) {
            i += 1;
        }
    } else if (input.len >= 16 && forbidden_chars.len > 1) {
        let mask: [256]u8 = undefined;
        @memset(mask[0..256], 0);
        let j: usize = 0;
        while (j < forbidden_chars.len) {
            mask[forbidden_chars[j]] = 1;
            j += 1;
        }
        while (i < input.len && mask[input[i]] == 0) {
            i += 1;
        }
    } else {
        while (i < input.len) {
            if (nomContainsByte(forbidden_chars, input[i])) {
                break;
            }
            i += 1;
        }
    }
    if (i == 0) {
        return nomFail(input);
    }
    return nomOk(input, i);
}

/// Matches the longest non-empty prefix containing only bytes in `allowed_chars`.
pub fn isA(input: string, allowed_chars: string): ParseRes {
    let i: usize = 0;
    if (allowed_chars.len == 1) {
        const ab = allowed_chars[0];
        const v_ab: NomVec16 = @splat(ab);
        while (i + 16 <= input.len) {
            const v: NomVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, v != v_ab)) {
                break;
            }
            i += 16;
        }
        while (i < input.len && input[i] == ab) {
            i += 1;
        }
    } else if (input.len >= 16 && allowed_chars.len > 1) {
        let mask: [256]u8 = undefined;
        @memset(mask[0..256], 0);
        let j: usize = 0;
        while (j < allowed_chars.len) {
            mask[allowed_chars[j]] = 1;
            j += 1;
        }
        while (i < input.len && mask[input[i]] != 0) {
            i += 1;
        }
    } else {
        while (i < input.len) {
            if (!nomContainsByte(allowed_chars, input[i])) {
                break;
            }
            i += 1;
        }
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
    const v_0: NomVec16 = @splat('0' as u8);
    const v_9: NomVec16 = @splat('9' as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        if (!@reduce(.And, (v >= v_0) & (v <= v_9))) {
            break;
        }
        i += 16;
    }
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
    const v_0: NomVec16 = @splat('0' as u8);
    const v_9: NomVec16 = @splat('9' as u8);
    const v_a: NomVec16 = @splat('a' as u8);
    const v_f: NomVec16 = @splat('f' as u8);
    const v_lc: NomVec16 = @splat(0x20 as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        const vl: NomVec16 = v | v_lc;
        const ok = ((v >= v_0) & (v <= v_9)) | ((vl >= v_a) & (vl <= v_f));
        if (!@reduce(.And, ok)) {
            break;
        }
        i += 16;
    }
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
    const v_a: NomVec16 = @splat('a' as u8);
    const v_z: NomVec16 = @splat('z' as u8);
    const v_lc: NomVec16 = @splat(0x20 as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        const vl: NomVec16 = v | v_lc;
        if (!@reduce(.And, (vl >= v_a) & (vl <= v_z))) {
            break;
        }
        i += 16;
    }
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
    const v_0: NomVec16 = @splat('0' as u8);
    const v_9: NomVec16 = @splat('9' as u8);
    const v_a: NomVec16 = @splat('a' as u8);
    const v_z: NomVec16 = @splat('z' as u8);
    const v_lc: NomVec16 = @splat(0x20 as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        const vl: NomVec16 = v | v_lc;
        const ok = ((v >= v_0) & (v <= v_9)) | ((vl >= v_a) & (vl <= v_z));
        if (!@reduce(.And, ok)) {
            break;
        }
        i += 16;
    }
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
    const v_sp: NomVec16 = @splat(' ' as u8);
    const v_tb: NomVec16 = @splat('\t' as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        if (!@reduce(.And, (v == v_sp) | (v == v_tb))) {
            break;
        }
        i += 16;
    }
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
    const v_sp: NomVec16 = @splat(' ' as u8);
    const v_tb: NomVec16 = @splat('\t' as u8);
    const v_cr: NomVec16 = @splat('\r' as u8);
    const v_lf: NomVec16 = @splat('\n' as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        const ok = (v == v_sp) | (v == v_tb) | (v == v_cr) | (v == v_lf);
        if (!@reduce(.And, ok)) {
            break;
        }
        i += 16;
    }
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
    const v_cr: NomVec16 = @splat('\r' as u8);
    const v_lf: NomVec16 = @splat('\n' as u8);
    while (i + 16 <= input.len) {
        const v: NomVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_cr) | (v == v_lf))) {
            break;
        }
        i += 16;
    }
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
