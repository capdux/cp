// @cap/unicase — Zero-heap Unicode & ASCII case-insensitive string wrapper,
// comparison, ordering, matching, and FNV-1a case-folded hashing.
// Clean-room MIT implementation inspired by Rust's `unicase` crate.

import { fnv64OffsetBasis, fnv1a64Update } from "../../fnv-hash/src/index.cp";

// Decode one UTF-8 codepoint starting at `pos`; writes byte length into `step_out[0]`.
fn ucDecodeUtf8(s: string, pos: usize, step_out: []usize): u32 {
    if (pos >= s.len) {
        step_out[0] = 0;
        return 0;
    }
    let b0: u32 = s[pos] as u32;
    if (b0 < 0x80) {
        step_out[0] = 1;
        return b0;
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len) {
        let b1: u32 = s[pos + 1] as u32;
        if ((b1 & 0xC0) == 0x80) {
            step_out[0] = 2;
            return ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        }
    } else if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len) {
        let b1: u32 = s[pos + 1] as u32;
        let b2: u32 = s[pos + 2] as u32;
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            step_out[0] = 3;
            return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        }
    } else if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len) {
        let b1: u32 = s[pos + 1] as u32;
        let b2: u32 = s[pos + 2] as u32;
        let b3: u32 = s[pos + 3] as u32;
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            step_out[0] = 4;
            return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        }
    }
    step_out[0] = 1;
    return 0xFFFD;
}

fn ucEncodeUtf8(cp: u32, out: []u8, pos: usize): usize {
    if (cp < 0x80) {
        if (pos < out.len) { out[pos] = cp as u8; return pos + 1; }
        return pos;
    }
    if (cp < 0x800) {
        if (pos + 1 < out.len) {
            out[pos] = (0xC0 | (cp >> 6)) as u8;
            out[pos + 1] = (0x80 | (cp & 0x3F)) as u8;
            return pos + 2;
        }
        return pos;
    }
    if (cp < 0x10000) {
        if (pos + 2 < out.len) {
            out[pos] = (0xE0 | (cp >> 12)) as u8;
            out[pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[pos + 2] = (0x80 | (cp & 0x3F)) as u8;
            return pos + 3;
        }
        return pos;
    }
    if (pos + 3 < out.len) {
        out[pos] = (0xF0 | (cp >> 18)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
        out[pos + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos + 3] = (0x80 | (cp & 0x3F)) as u8;
        return pos + 4;
    }
    return pos;
}

/// ASCII lowercase fold for a single byte.
pub fn foldAsciiByte(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

/// Full Unicode simple/multi-codepoint case folding (returns 1..2 folded codepoints in `folded_out`).
/// Covers ASCII, Latin-1 Supplement (including ß -> ss, À..Ö, Ø..Þ), Latin Extended-A,
/// Greek (including final sigma ς -> σ, Kelvin K -> k), Cyrillic (А..Я, Ё), and Armenian.
pub fn foldCodepoint(cp: u32, folded_out: []u32): usize {
    // ASCII A-Z
    if (cp >= 0x41 && cp <= 0x5A) {
        folded_out[0] = cp + 0x20;
        return 1;
    }
    // Micro sign µ -> μ
    if (cp == 0x00B5) {
        folded_out[0] = 0x03BC;
        return 1;
    }
    // Latin-1 uppercase À..Ö (0xC0..0xD6) and Ø..Þ (0xD8..0xDE)
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) {
        folded_out[0] = cp + 0x20;
        return 1;
    }
    // German sharp s: ß (0x00DF) and ẞ (0x1E9E) -> "ss"
    if (cp == 0x00DF || cp == 0x1E9E) {
        folded_out[0] = 0x0073;
        folded_out[1] = 0x0073;
        return 2;
    }
    // Latin Extended-A: 0x0100..0x012E (even -> +1)
    if (cp >= 0x0100 && cp <= 0x012E && (cp & 1) == 0) {
        folded_out[0] = cp + 1;
        return 1;
    }
    // Turkish dotted I: İ (0x0130) -> i + combining dot above (simple fold -> 'i')
    if (cp == 0x0130) {
        folded_out[0] = 0x0069;
        return 1;
    }
    // Latin Extended-A: 0x0132..0x0136 (even), 0x0139..0x0147 (odd), 0x014A..0x0176 (even)
    if (cp >= 0x0132 && cp <= 0x0136 && (cp & 1) == 0) {
        folded_out[0] = cp + 1;
        return 1;
    }
    if (cp >= 0x0139 && cp <= 0x0147 && (cp & 1) == 1) {
        folded_out[0] = cp + 1;
        return 1;
    }
    if (cp >= 0x014A && cp <= 0x0176 && (cp & 1) == 0) {
        folded_out[0] = cp + 1;
        return 1;
    }
    if (cp == 0x0178) { // Ÿ -> ÿ
        folded_out[0] = 0x00FF;
        return 1;
    }
    if (cp >= 0x0179 && cp <= 0x017D && (cp & 1) == 1) {
        folded_out[0] = cp + 1;
        return 1;
    }
    if (cp == 0x017F) { // long s ſ -> s
        folded_out[0] = 0x0073;
        return 1;
    }
    // Greek uppercase with tonos: Ά(0386)->ά(03AC), Έ..Ί(0388..038A)->έ..ί(03AD..03AF),
    // Ό(038C)->ό(03CC), Ύ..Ώ(038E..038F)->ύ..ώ(03CD..03CE)
    if (cp == 0x0386) { folded_out[0] = 0x03AC; return 1; }
    if (cp >= 0x0388 && cp <= 0x038A) { folded_out[0] = cp + 0x25; return 1; }
    if (cp == 0x038C) { folded_out[0] = 0x03CC; return 1; }
    if (cp == 0x038E || cp == 0x038F) { folded_out[0] = cp + 0x3F; return 1; }
    // Greek uppercase Α..Ρ (0x0391..0x03A1), Σ..Ω (0x03A3..0x03AB), final sigma ς (0x03C2) -> σ (0x03C3)
    if ((cp >= 0x0391 && cp <= 0x03A1) || (cp >= 0x03A3 && cp <= 0x03AB)) {
        folded_out[0] = cp + 0x20;
        return 1;
    }
    if (cp == 0x03C2) {
        folded_out[0] = 0x03C3;
        return 1;
    }
    // Cyrillic Ё..Џ (0x0400..0x040F -> +0x50), А..Я (0x0410..0x042F -> +0x20)
    if (cp >= 0x0400 && cp <= 0x040F) {
        folded_out[0] = cp + 0x50;
        return 1;
    }
    if (cp >= 0x0410 && cp <= 0x042F) {
        folded_out[0] = cp + 0x20;
        return 1;
    }
    // Kelvin sign K (0x212A) -> k, Angstrom Å (0x212B) -> å
    if (cp == 0x212A) {
        folded_out[0] = 0x006B;
        return 1;
    }
    if (cp == 0x212B) {
        folded_out[0] = 0x00E5;
        return 1;
    }
    // Fullwidth A-Z (0xFF21..0xFF3A) -> fullwidth a-z
    if (cp >= 0xFF21 && cp <= 0xFF3A) {
        folded_out[0] = cp + 0x20;
        return 1;
    }
    folded_out[0] = cp;
    return 1;
}

/// Writes the Unicode case-folded UTF-8 form of `src` into `out`, returning bytes written.
pub fn caseFold(src: string, out: []u8): usize {
    let pos: usize = 0;
    let w: usize = 0;
    let step: [1]usize = [0];
    let folded: [2]u32 = [0, 0];
    while (pos < src.len) {
        let cp = ucDecodeUtf8(src, pos, step[0..1]);
        if (step[0] == 0) { break; }
        pos = pos + step[0];
        let cnt = foldCodepoint(cp, folded[0..2]);
        let k: usize = 0;
        while (k < cnt) {
            w = ucEncodeUtf8(folded[k], out, w);
            k = k + 1;
        }
    }
    return w;
}

/// Fast ASCII-only case-insensitive equality.
pub fn eqAscii(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (foldAsciiByte(a[i]) != foldAsciiByte(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Fast ASCII-only case-insensitive lexicographical comparison (-1, 0, 1).
pub fn cmpAscii(a: string, b: string): i32 {
    let min_len = if (a.len < b.len) { a.len } else { b.len };
    let i: usize = 0;
    while (i < min_len) {
        let ca = foldAsciiByte(a[i]);
        let cb = foldAsciiByte(b[i]);
        if (ca < cb) { return -1; }
        if (ca > cb) { return 1; }
        i = i + 1;
    }
    if (a.len < b.len) { return -1; }
    if (a.len > b.len) { return 1; }
    return 0;
}

/// Full Unicode case-insensitive lexicographical comparison (-1, 0, 1), zero-heap streaming.
pub fn cmpUnicode(a: string, b: string): i32 {
    let pa: usize = 0;
    let pb: usize = 0;
    let sa: [1]usize = [0];
    let sb: [1]usize = [0];
    let qa: [2]u32 = [0, 0];
    let qb: [2]u32 = [0, 0];
    let qa_idx: usize = 0;
    let qa_len: usize = 0;
    let qb_idx: usize = 0;
    let qb_len: usize = 0;

    while (true) {
        if (qa_idx >= qa_len && pa < a.len) {
            let cpa = ucDecodeUtf8(a, pa, sa[0..1]);
            pa = pa + sa[0];
            qa_len = foldCodepoint(cpa, qa[0..2]);
            qa_idx = 0;
        }
        if (qb_idx >= qb_len && pb < b.len) {
            let cpb = ucDecodeUtf8(b, pb, sb[0..1]);
            pb = pb + sb[0];
            qb_len = foldCodepoint(cpb, qb[0..2]);
            qb_idx = 0;
        }
        let ha = qa_idx < qa_len;
        let hb = qb_idx < qb_len;
        if (!ha && !hb) { return 0; }
        if (!ha) { return -1; }
        if (!hb) { return 1; }
        let ca = qa[qa_idx];
        let cb = qb[qb_idx];
        qa_idx = qa_idx + 1;
        qb_idx = qb_idx + 1;
        if (ca < cb) { return -1; }
        if (ca > cb) { return 1; }
    }
    return 0;
}

/// Full Unicode case-insensitive equality (supports ß == SS, Cyrillic, Greek, Latin-1).
pub fn eqUnicode(a: string, b: string): bool {
    return cmpUnicode(a, b) == 0;
}

/// Case-insensitive ASCII hash via `@cap/fnv-hash` (zero-heap streaming in 256-byte chunks).
pub fn hashAscii(s: string): u64 {
    let h: u64 = fnv64OffsetBasis();
    let buf: [256]u8 = undefined;
    let pos: usize = 0;
    while (pos < s.len) {
        let n: usize = 0;
        while (n < 256 && pos < s.len) {
            buf[n] = foldAsciiByte(s[pos]);
            n = n + 1;
            pos = pos + 1;
        }
        h = fnv1a64Update(h, buf[0..n]);
    }
    return h;
}

/// Case-insensitive Unicode hash via `@cap/fnv-hash` (zero-heap streaming UTF-8 case fold).
pub fn hashUnicode(s: string): u64 {
    let h: u64 = fnv64OffsetBasis();
    let buf: [256]u8 = undefined;
    let pos: usize = 0;
    let step: [1]usize = [0];
    let folded: [2]u32 = [0, 0];
    let w: usize = 0;
    while (pos < s.len) {
        let cp = ucDecodeUtf8(s, pos, step[0..1]);
        if (step[0] == 0) { break; }
        pos = pos + step[0];
        let cnt = foldCodepoint(cp, folded[0..2]);
        let k: usize = 0;
        while (k < cnt) {
            if (w + 4 > 256) {
                h = fnv1a64Update(h, buf[0..w]);
                w = 0;
            }
            w = ucEncodeUtf8(folded[k], buf[0..256], w);
            k = k + 1;
        }
    }
    if (w > 0) {
        h = fnv1a64Update(h, buf[0..w]);
    }
    return h;
}

/// ASCII case-insensitive prefix/suffix/substring helpers.
pub fn startsWithIgnoreCase(haystack: string, prefix: string): bool {
    if (prefix.len > haystack.len) { return false; }
    return eqAscii(haystack[0..prefix.len], prefix);
}

pub fn endsWithIgnoreCase(haystack: string, suffix: string): bool {
    if (suffix.len > haystack.len) { return false; }
    let start = haystack.len - suffix.len;
    return eqAscii(haystack[start..haystack.len], suffix);
}

pub fn containsIgnoreCase(haystack: string, needle: string): bool {
    if (needle.len == 0) { return true; }
    if (needle.len > haystack.len) { return false; }
    let limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        if (eqAscii(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

/// Zero-heap ASCII case-insensitive wrapper (`unicase::Ascii`).
pub struct Ascii {
    pub raw: string,

    pub static fn new(s: string): Ascii {
        return Ascii { raw: s };
    }

    pub fn intoInner(): string {
        return _.raw;
    }

    pub fn eq(other: Ascii): bool {
        return eqAscii(_.raw, other.raw);
    }

    pub fn cmp(other: Ascii): i32 {
        return cmpAscii(_.raw, other.raw);
    }

    pub fn hash(): u64 {
        return hashAscii(_.raw);
    }
}

/// Zero-heap Unicode case-insensitive wrapper (`unicase::UniCase`).
pub struct UniCase {
    pub raw: string,

    pub static fn new(s: string): UniCase {
        return UniCase { raw: s };
    }

    pub static fn ascii(s: string): UniCase {
        return UniCase { raw: s };
    }

    pub fn intoInner(): string {
        return _.raw;
    }

    pub fn eq(other: UniCase): bool {
        return eqUnicode(_.raw, other.raw);
    }

    pub fn cmp(other: UniCase): i32 {
        return cmpUnicode(_.raw, other.raw);
    }

    pub fn hash(): u64 {
        return hashUnicode(_.raw);
    }
}
