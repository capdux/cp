// @cap/bstr — Zero-allocation byte string utilities for CP (Rust `bstr` v1 parity)
// Clean-room MIT implementation.
// Reuses `@cap/memchr` (`memchr`, `memrchr`, `memmemFind`, `memmemRfind`) for
// SWAR-accelerated byte/substring search, line splitting, and single-pass replacement.

import {
    memchr,
    memrchr,
    memmemFind,
    memmemRfind
} from "../../memchr/src/index.cp";

/// Zero-copy half-open byte range `[start, end)` referencing an input slice.
pub struct SliceRange {
    pub start: usize,
    pub end: usize,

    pub fn len(): usize {
        if (_.end <= _.start) {
            return 0;
        }
        return _.end - _.start;
    }

    pub fn isEmpty(): bool {
        return _.start >= _.end;
    }
}

// ============================================================================
// Private helpers (`bs` prefix to avoid module-flattening symbol collisions)
// ============================================================================

fn bsIsAsciiWhitespace(b: u8): bool {
    return b == 0x20 || b == 0x09 || b == 0x0A || b == 0x0B || b == 0x0C || b == 0x0D;
}

fn bsBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn bsCopyBytes(dst: []u8, pos: usize, src: []const u8): usize {
    let cur = pos;
    let i: usize = 0;
    while (i < src.len && cur < dst.len) {
        dst[cur] = src[i];
        cur = cur + 1;
        i = i + 1;
    }
    return cur;
}

fn bsIsContByte(b: u8): bool {
    return (b & 0xC0) == 0x80;
}

/// Validates a UTF-8 sequence starting at `src[i]`.
/// Returns 1..4 if valid, or 0 if ill-formed (overlong, surrogate, >U+10FFFF, truncated, or invalid lead).
fn bsUtf8ValidLen(src: []const u8, i: usize): usize {
    let len = src.len;
    if (i >= len) {
        return 0;
    }
    let b0 = src[i];
    if (b0 < 0x80) {
        return 1;
    }
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (i + 1 < len && bsIsContByte(src[i + 1])) {
            return 2;
        }
        return 0;
    }
    if (b0 == 0xE0) {
        if (i + 2 < len && src[i + 1] >= 0xA0 && src[i + 1] <= 0xBF && bsIsContByte(src[i + 2])) {
            return 3;
        }
        return 0;
    }
    if ((b0 >= 0xE1 && b0 <= 0xEC) || (b0 >= 0xEE && b0 <= 0xEF)) {
        if (i + 2 < len && bsIsContByte(src[i + 1]) && bsIsContByte(src[i + 2])) {
            return 3;
        }
        return 0;
    }
    if (b0 == 0xED) {
        // Exclude UTF-16 surrogates U+D800..U+DFFF (second byte 0xA0..0xBF)
        if (i + 2 < len && src[i + 1] >= 0x80 && src[i + 1] <= 0x9F && bsIsContByte(src[i + 2])) {
            return 3;
        }
        return 0;
    }
    if (b0 == 0xF0) {
        if (i + 3 < len && src[i + 1] >= 0x90 && src[i + 1] <= 0xBF && bsIsContByte(src[i + 2]) && bsIsContByte(src[i + 3])) {
            return 4;
        }
        return 0;
    }
    if (b0 >= 0xF1 && b0 <= 0xF3) {
        if (i + 3 < len && bsIsContByte(src[i + 1]) && bsIsContByte(src[i + 2]) && bsIsContByte(src[i + 3])) {
            return 4;
        }
        return 0;
    }
    if (b0 == 0xF4) {
        // Max Unicode scalar value U+10FFFF (second byte 0x80..0x8F)
        if (i + 3 < len && src[i + 1] >= 0x80 && src[i + 1] <= 0x8F && bsIsContByte(src[i + 2]) && bsIsContByte(src[i + 3])) {
            return 4;
        }
        return 0;
    }
    return 0;
}

/// Computes the maximal subpart length (1..3) of an ill-formed UTF-8 subsequence at `src[i]`,
/// matching Unicode Standard / W3C / Rust `bstr::ByteSlice::to_str_lossy` replacement semantics.
fn bsUtf8InvalidSkip(src: []const u8, i: usize): usize {
    let len = src.len;
    let b0 = src[i];
    if (b0 == 0xE0) {
        if (i + 1 < len && src[i + 1] >= 0xA0 && src[i + 1] <= 0xBF) {
            return 2;
        }
        return 1;
    }
    if ((b0 >= 0xE1 && b0 <= 0xEC) || (b0 >= 0xEE && b0 <= 0xEF)) {
        if (i + 1 < len && bsIsContByte(src[i + 1])) {
            return 2;
        }
        return 1;
    }
    if (b0 == 0xED) {
        if (i + 1 < len && src[i + 1] >= 0x80 && src[i + 1] <= 0x9F) {
            return 2;
        }
        return 1;
    }
    if (b0 == 0xF0) {
        if (i + 1 < len && src[i + 1] >= 0x90 && src[i + 1] <= 0xBF) {
            if (i + 2 < len && bsIsContByte(src[i + 2])) {
                return 3;
            }
            return 2;
        }
        return 1;
    }
    if (b0 >= 0xF1 && b0 <= 0xF3) {
        if (i + 1 < len && bsIsContByte(src[i + 1])) {
            if (i + 2 < len && bsIsContByte(src[i + 2])) {
                return 3;
            }
            return 2;
        }
        return 1;
    }
    if (b0 == 0xF4) {
        if (i + 1 < len && src[i + 1] >= 0x80 && src[i + 1] <= 0x8F) {
            if (i + 2 < len && bsIsContByte(src[i + 2])) {
                return 3;
            }
            return 2;
        }
        return 1;
    }
    return 1;
}

fn bsToLowerByte(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn bsToUpperByte(b: u8): u8 {
    if (b >= 97 && b <= 122) {
        return b - 32;
    }
    return b;
}

// ============================================================================
// Public API — Search & Matching (delegating to @cap/memchr)
// ============================================================================

/// Finds the first occurrence of `needle` in `haystack` via `@cap/memchr::memmemFind`.
/// Returns the byte offset, or `-1` if not found.
pub fn bstrFind(haystack: []const u8, needle: []const u8): i64 {
    return memmemFind(haystack, needle);
}

/// Finds the last occurrence of `needle` in `haystack` via `@cap/memchr::memmemRfind`.
/// Returns the byte offset, or `-1` if not found.
pub fn bstrRfind(haystack: []const u8, needle: []const u8): i64 {
    return memmemRfind(haystack, needle);
}

/// Finds the first occurrence of byte `b` in `haystack` via `@cap/memchr::memchr`.
pub fn bstrFindByte(haystack: []const u8, b: u8): i64 {
    return memchr(b, haystack);
}

/// Finds the last occurrence of byte `b` in `haystack` via `@cap/memchr::memrchr`.
pub fn bstrRfindByte(haystack: []const u8, b: u8): i64 {
    return memrchr(b, haystack);
}

/// Returns `true` if `haystack` contains `needle`.
pub fn bstrContains(haystack: []const u8, needle: []const u8): bool {
    return memmemFind(haystack, needle) >= 0;
}

/// Returns `true` if `s` starts with `prefix`.
pub fn bstrStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    if (prefix.len == 0) {
        return true;
    }
    return bsBytesEqual(s[0..prefix.len], prefix);
}

/// Returns `true` if `s` ends with `suffix`.
pub fn bstrEndsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) {
        return false;
    }
    if (suffix.len == 0) {
        return true;
    }
    let start = s.len - suffix.len;
    return bsBytesEqual(s[start..s.len], suffix);
}

// ============================================================================
// Public API — Zero-Copy Trimming
// ============================================================================

/// Strips leading ASCII whitespace (` `, `\t`, `\n`, `\x0B`, `\x0C`, `\r`), returning a `SliceRange`.
pub fn bstrTrimStart(s: []const u8): SliceRange {
    let start: usize = 0;
    while (start < s.len && bsIsAsciiWhitespace(s[start])) {
        start = start + 1;
    }
    return SliceRange { start: start, end: s.len };
}

/// Strips trailing ASCII whitespace (` `, `\t`, `\n`, `\x0B`, `\x0C`, `\r`), returning a `SliceRange`.
pub fn bstrTrimEnd(s: []const u8): SliceRange {
    let end: usize = s.len;
    while (end > 0 && bsIsAsciiWhitespace(s[end - 1])) {
        end = end - 1;
    }
    return SliceRange { start: 0, end: end };
}

/// Strips leading and trailing ASCII whitespace (` `, `\t`, `\n`, `\x0B`, `\x0C`, `\r`), returning a `SliceRange`.
pub fn bstrTrim(s: []const u8): SliceRange {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && bsIsAsciiWhitespace(s[start])) {
        start = start + 1;
    }
    while (end > start && bsIsAsciiWhitespace(s[end - 1])) {
        end = end - 1;
    }
    return SliceRange { start: start, end: end };
}

// ============================================================================
// Public API — Zero-Copy Line Splitting (`\n` & `\r\n`, via @cap/memchr)
// ============================================================================

/// Counts lines in `s` delimited by `\n` or `\r\n` (matching Rust `bstr::ByteSlice::lines`).
/// Empty input has 0 lines; a trailing newline terminates the final line without adding an empty one.
pub fn bstrLineCount(s: []const u8): usize {
    if (s.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        let hit = memchr(0x0A, s[pos..s.len]);
        count = count + 1;
        if (hit < 0) {
            break;
        }
        pos = pos + (hit as usize) + 1;
    }
    return count;
}

/// Returns the zero-copy `SliceRange` of the `idx`-th line (0-based) in `s`,
/// with trailing `\n` or `\r\n` stripped. Returns an empty range at `s.len` if `idx` is out of bounds.
pub fn bstrLineAt(s: []const u8, idx: usize): SliceRange {
    let pos: usize = 0;
    let cur: usize = 0;
    while (pos < s.len) {
        let hit = memchr(0x0A, s[pos..s.len]);
        let lineEnd: usize = s.len;
        let nextPos: usize = s.len;
        if (hit >= 0) {
            let nlPos = pos + (hit as usize);
            lineEnd = nlPos;
            if (lineEnd > pos && s[lineEnd - 1] == 0x0D) {
                lineEnd = lineEnd - 1;
            }
            nextPos = nlPos + 1;
        }
        if (cur == idx) {
            return SliceRange { start: pos, end: lineEnd };
        }
        cur = cur + 1;
        pos = nextPos;
    }
    return SliceRange { start: s.len, end: s.len };
}

// ============================================================================
// Public API — Single-Pass Zero-Heap Replacement
// ============================================================================

/// Replaces all non-overlapping occurrences of `pattern` in `src` with `replacement`, writing into `out`.
/// Uses `@cap/memchr::memmemFind` for fast skipping. If `pattern` is empty, safely copies `src` unchanged.
/// Returns the number of bytes written to `out`.
pub fn bstrReplaceAll(src: []const u8, pattern: []const u8, replacement: []const u8, out: []u8): usize {
    if (out.len == 0 || src.len == 0) {
        return 0;
    }
    if (pattern.len == 0) {
        return bsCopyBytes(out, 0, src);
    }

    let written: usize = 0;
    let pos: usize = 0;
    while (pos < src.len && written < out.len) {
        let hit = memmemFind(src[pos..src.len], pattern);
        if (hit < 0) {
            written = bsCopyBytes(out, written, src[pos..src.len]);
            break;
        }
        let matchStart = pos + (hit as usize);
        if (matchStart > pos) {
            written = bsCopyBytes(out, written, src[pos..matchStart]);
        }
        if (replacement.len > 0 && written < out.len) {
            written = bsCopyBytes(out, written, replacement);
        }
        pos = matchStart + pattern.len;
    }
    return written;
}

// ============================================================================
// Public API — Strict UTF-8 Validation & Lossy Conversion
// ============================================================================

/// Returns `true` if `src` is completely valid UTF-8 (RFC 3629).
pub fn bstrIsUtf8(src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len) {
        let seqLen = bsUtf8ValidLen(src, i);
        if (seqLen == 0) {
            return false;
        }
        i = i + seqLen;
    }
    return true;
}

/// Converts arbitrary bytes `src` into valid UTF-8 in `out`, replacing every invalid byte sequence
/// (overlong encodings, UTF-16 surrogates, out-of-range code points, truncated sequences, stray
/// continuation bytes) with the Unicode Replacement Character U+FFFD (`0xEF 0xBF 0xBD`).
/// Matches Rust `bstr::ByteSlice::to_str_lossy`. Returns bytes written to `out`.
pub fn bstrToUtf8Lossy(src: []const u8, out: []u8): usize {
    let i: usize = 0;
    let written: usize = 0;
    while (i < src.len && written < out.len) {
        let seqLen = bsUtf8ValidLen(src, i);
        if (seqLen > 0) {
            if (written + seqLen > out.len) {
                break;
            }
            let k: usize = 0;
            while (k < seqLen) {
                out[written + k] = src[i + k];
                k = k + 1;
            }
            written = written + seqLen;
            i = i + seqLen;
        } else {
            if (written + 3 > out.len) {
                break;
            }
            out[written] = 0xEF;
            out[written + 1] = 0xBF;
            out[written + 2] = 0xBD;
            written = written + 3;
            i = i + bsUtf8InvalidSkip(src, i);
        }
    }
    return written;
}

// ============================================================================
// Public API — ASCII Predicates & Case Transforms
// ============================================================================

/// Returns `true` if every byte in `s` is 7-bit ASCII (`<= 0x7F`).
pub fn bstrIsAscii(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] > 0x7F) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Copies `src` to `out`, converting ASCII uppercase `'A'..'Z'` to lowercase `'a'..'z'`.
/// Non-ASCII bytes are preserved verbatim. Returns bytes written to `out`.
pub fn bstrToLowerAscii(src: []const u8, out: []u8): usize {
    let n = src.len;
    if (out.len < n) {
        n = out.len;
    }
    let i: usize = 0;
    while (i < n) {
        out[i] = bsToLowerByte(src[i]);
        i = i + 1;
    }
    return n;
}

/// Copies `src` to `out`, converting ASCII lowercase `'a'..'z'` to uppercase `'A'..'Z'`.
/// Non-ASCII bytes are preserved verbatim. Returns bytes written to `out`.
pub fn bstrToUpperAscii(src: []const u8, out: []u8): usize {
    let n = src.len;
    if (out.len < n) {
        n = out.len;
    }
    let i: usize = 0;
    while (i < n) {
        out[i] = bsToUpperByte(src[i]);
        i = i + 1;
    }
    return n;
}

/// Returns `true` if `a` and `b` are equal ignoring ASCII case.
pub fn bstrEqIgnoreAsciiCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (bsToLowerByte(a[i]) != bsToLowerByte(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

export {
    SliceRange,
    bstrFind,
    bstrRfind,
    bstrFindByte,
    bstrRfindByte,
    bstrContains,
    bstrStartsWith,
    bstrEndsWith,
    bstrTrimStart,
    bstrTrimEnd,
    bstrTrim,
    bstrLineCount,
    bstrLineAt,
    bstrReplaceAll,
    bstrIsUtf8,
    bstrToUtf8Lossy,
    bstrIsAscii,
    bstrToLowerAscii,
    bstrToUpperAscii,
    bstrEqIgnoreAsciiCase
};
