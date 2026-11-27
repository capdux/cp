// @cap/bstr — Zero-allocation byte string utilities for CP (Rust `bstr` v1 parity)
// Clean-room MIT implementation.
// Reuses `@cap/memchr` (`memchr`, `memrchr`, `memmemFind`, `memmemRfind`) for
// SIMD-accelerated byte/substring search, plus 16-lane @Vector(16, u8) SIMD ASCII fast-path
// in `bstrToUtf8Lossy`, comptime 256-entry whitespace LUT, and @memcpy in `bstrReplaceAll`.

import { isAscii, startsWith, endsWith, toLower, toUpper, eqIgnoreCase } from "cap:ascii";
import { isValidUtf8 } from "cap:utf8";
import {
    memchr,
    memrchr,
    memmemFind,
    memmemRfind
} from "../../memchr/src/index.cp";

type BsVec16 = @Vector(16, u8);

fn bsBuildSpaceLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[0x20] = 1; // ' '
    lut[0x09] = 1; // '\t'
    lut[0x0A] = 1; // '\n'
    lut[0x0B] = 1; // '\x0B'
    lut[0x0C] = 1; // '\x0C'
    lut[0x0D] = 1; // '\r'
    return lut;
}

const BS_SPACE_LUT: [256]u8 = bsBuildSpaceLut();

fn bsCopySlice(src: []const u8, out: []u8, pos: usize): usize {
    if (src.len == 0 || pos >= out.len) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (src.len <= avail) { src.len } else { avail };
    @memcpy(out[pos .. pos + n], src[0..n]);
    return pos + n;
}

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

// ============================================================================
// Public API — Search & Matching (delegating to @cap/memchr & cap:bin)
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
    return startsWith(s, prefix);
}

/// Returns `true` if `s` ends with `suffix`.
pub fn bstrEndsWith(s: []const u8, suffix: []const u8): bool {
    return endsWith(s, suffix);
}

// ============================================================================
// Public API — Zero-Copy Trimming (comptime 256-entry LUT)
// ============================================================================

/// Strips leading ASCII whitespace (` `, `\t`, `\n`, `\x0B`, `\x0C`, `\r`), returning a `SliceRange`.
pub fn bstrTrimStart(s: []const u8): SliceRange {
    let start: usize = 0;
    while (start < s.len && BS_SPACE_LUT[s[start] as usize] != 0) {
        start = start + 1;
    }
    return SliceRange { start: start, end: s.len };
}

/// Strips trailing ASCII whitespace (` `, `\t`, `\n`, `\x0B`, `\x0C`, `\r`), returning a `SliceRange`.
pub fn bstrTrimEnd(s: []const u8): SliceRange {
    let end: usize = s.len;
    while (end > 0 && BS_SPACE_LUT[s[end - 1] as usize] != 0) {
        end = end - 1;
    }
    return SliceRange { start: 0, end: end };
}

/// Strips leading and trailing ASCII whitespace (` `, `\t`, `\n`, `\x0B`, `\x0C`, `\r`), returning a `SliceRange`.
pub fn bstrTrim(s: []const u8): SliceRange {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && BS_SPACE_LUT[s[start] as usize] != 0) {
        start = start + 1;
    }
    while (end > start && BS_SPACE_LUT[s[end - 1] as usize] != 0) {
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
// Public API — Single-Pass Zero-Heap Replacement (@memcpy bulk copy)
// ============================================================================

/// Replaces all non-overlapping occurrences of `pattern` in `src` with `replacement`, writing into `out`.
/// Uses `@cap/memchr::memmemFind` for fast skipping. If `pattern` is empty, safely copies `src` unchanged.
/// Returns the number of bytes written to `out`.
pub fn bstrReplaceAll(src: []const u8, pattern: []const u8, replacement: []const u8, out: []u8): usize {
    if (out.len == 0 || src.len == 0) {
        return 0;
    }
    if (pattern.len == 0) {
        return bsCopySlice(src, out, 0);
    }

    let written: usize = 0;
    let pos: usize = 0;
    while (pos < src.len && written < out.len) {
        let hit = memmemFind(src[pos..src.len], pattern);
        if (hit < 0) {
            written = bsCopySlice(src[pos..src.len], out, written);
            break;
        }
        let matchStart = pos + (hit as usize);
        if (matchStart > pos) {
            written = bsCopySlice(src[pos..matchStart], out, written);
        }
        if (replacement.len > 0 && written < out.len) {
            written = bsCopySlice(replacement, out, written);
        }
        pos = matchStart + pattern.len;
    }
    return written;
}

// ============================================================================
// Public API — Strict UTF-8 Validation & Lossy Conversion (16-lane SIMD ASCII fast path)
// ============================================================================

/// Returns `true` if `src` is completely valid UTF-8 (RFC 3629).
pub fn bstrIsUtf8(src: []const u8): bool {
    return isValidUtf8(src);
}

/// Converts arbitrary bytes `src` into valid UTF-8 in `out`, replacing every invalid byte sequence
/// (overlong encodings, UTF-16 surrogates, out-of-range code points, truncated sequences, stray
/// continuation bytes) with the Unicode Replacement Character U+FFFD (`0xEF 0xBF 0xBD`).
/// Matches Rust `bstr::ByteSlice::to_str_lossy`. Returns bytes written to `out`.
pub fn bstrToUtf8Lossy(src: []const u8, out: []u8): usize {
    let i: usize = 0;
    let written: usize = 0;
    const v_80: BsVec16 = @splat(0x80 as u8);
    while (i < src.len && written < out.len) {
        // 16-lane SIMD fast-path for contiguous ASCII runs
        while (i + 16 <= src.len && written + 16 <= out.len) {
            const v: BsVec16 = src[i .. i + 16][0..16].*;
            if (@reduce(.Or, v >= v_80)) {
                break;
            }
            @memcpy(out[written .. written + 16], src[i .. i + 16]);
            i = i + 16;
            written = written + 16;
        }
        if (i >= src.len || written >= out.len) {
            break;
        }
        let seqLen = bsUtf8ValidLen(src, i);
        if (seqLen > 0) {
            if (written + seqLen > out.len) {
                break;
            }
            @memcpy(out[written .. written + seqLen], src[i .. i + seqLen]);
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
    return isAscii(s);
}

/// Copies `src` to `out`, converting ASCII uppercase `'A'..'Z'` to lowercase `'a'..'z'`.
/// Non-ASCII bytes are preserved verbatim. Returns bytes written to `out`.
pub fn bstrToLowerAscii(src: []const u8, out: []u8): usize {
    return toLower(src, out);
}

/// Copies `src` to `out`, converting ASCII lowercase `'a'..'z'` to uppercase `'A'..'Z'`.
/// Non-ASCII bytes are preserved verbatim. Returns bytes written to `out`.
pub fn bstrToUpperAscii(src: []const u8, out: []u8): usize {
    return toUpper(src, out);
}

/// Returns `true` if `a` and `b` are equal ignoring ASCII case.
pub fn bstrEqIgnoreAsciiCase(a: []const u8, b: []const u8): bool {
    return eqIgnoreCase(a, b);
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
