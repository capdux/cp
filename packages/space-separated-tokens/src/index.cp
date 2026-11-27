// space-separated-tokens — HTML/SVG space & comma separated token lists in pure CP
// Clean-room MIT implementation of unifiedjs `space-separated-tokens` (#644, 90.1M/wk)
// and `comma-separated-tokens` (#648, 89.6M/wk).
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry space LUT, and @memcpy.

import { trim, strEq } from "cap:ascii";

type SstVec16 = @Vector(16, u8);

fn sstBuildSpaceLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[0x20] = 1; // ' '
    lut[0x09] = 1; // '\t'
    lut[0x0A] = 1; // '\n'
    lut[0x0B] = 1; // '\v'
    lut[0x0C] = 1; // '\f'
    lut[0x0D] = 1; // '\r'
    return lut;
}

const SST_SPACE_LUT: [256]u8 = sstBuildSpaceLut();

fn sstSkipSpace(input: string, start: usize): usize {
    let i: usize = start;
    const len = input.len;
    const v_sp: SstVec16 = @splat(0x20 as u8);
    while (i + 16 <= len) {
        const v: SstVec16 = input[i .. i + 16][0..16].*;
        if (!@reduce(.And, v == v_sp)) {
            break;
        }
        i += 16;
    }
    while (i < len && SST_SPACE_LUT[input[i] as usize] != 0) {
        i += 1;
    }
    return i;
}

fn sstScanTokenEnd(input: string, start: usize): usize {
    let i: usize = start;
    const len = input.len;
    const v_sp: SstVec16 = @splat(0x20 as u8);
    while (i + 16 <= len) {
        const v: SstVec16 = input[i .. i + 16][0..16].*;
        // All ASCII whitespace chars satisfy b <= 0x20; fast-skip blocks with all bytes > 0x20
        if (@reduce(.Or, v <= v_sp)) {
            let k: usize = 0;
            while (k < 16) {
                if (SST_SPACE_LUT[input[i + k] as usize] != 0) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && SST_SPACE_LUT[input[i] as usize] == 0) {
        i += 1;
    }
    return i;
}

fn sstFindComma(input: string, start: usize): usize {
    let i: usize = start;
    const len = input.len;
    const v_comma: SstVec16 = @splat(44 as u8);
    while (i + 16 <= len) {
        const v: SstVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_comma)) {
            let k: usize = 0;
            while (k < 16) {
                if (input[i + k] == 44) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (input[i] == 44) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn sstWriteSlice(src: string, out: []u8, pos: usize): usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const rem: usize = out.len - pos;
    const n: usize = if (src.len < rem) src.len else rem;
    @memcpy(out[pos .. pos + n], src[0 .. n]);
    return pos + n;
}

/// Parses HTML space-separated tokens into `out` slice; returns token count written.
/// Handles leading/trailing whitespace and runs of spaces, tabs, newlines, and form feeds.
pub fn parseSpaceTokens(input: string, out: []string): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        i = sstSkipSpace(input, i);
        if (i >= input.len) {
            break;
        }
        const start = i;
        i = sstScanTokenEnd(input, i);
        if (count < out.len) {
            out[count] = input[start..i];
            count += 1;
        }
    }
    return count;
}

/// unifiedjs `parse` alias: parses space-separated tokens into `out`.
pub fn parse(input: string, out: []string): usize {
    return parseSpaceTokens(input, out);
}

/// Counts the number of space-separated tokens in `input` without writing any output.
pub fn countTokens(input: string): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        i = sstSkipSpace(input, i);
        if (i >= input.len) {
            break;
        }
        i = sstScanTokenEnd(input, i);
        count += 1;
    }
    return count;
}

/// Returns the zero-based `index`-th space-separated token from `input`, or `null` if out of bounds.
pub fn tokenAt(input: string, index: usize): string? {
    let count: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        i = sstSkipSpace(input, i);
        if (i >= input.len) {
            break;
        }
        const start = i;
        i = sstScanTokenEnd(input, i);
        if (count == index) {
            return input[start..i];
        }
        count += 1;
    }
    return null;
}

/// Checks whether `target` appears as a token in a space-separated token string.
pub fn hasSpaceToken(input: string, target: string): bool {
    if (target.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < input.len) {
        i = sstSkipSpace(input, i);
        if (i >= input.len) {
            break;
        }
        const start = i;
        i = sstScanTokenEnd(input, i);
        if (strEq(input[start..i], target)) {
            return true;
        }
    }
    return false;
}

/// Alias for `hasSpaceToken`: checks whether `target` is present in `input`.
pub fn containsToken(input: string, target: string): bool {
    return hasSpaceToken(input, target);
}

/// Serializes tokens into `out` buffer separated by single ASCII space; skips empty tokens.
pub fn stringifySpaceTokens(tokens: []const string, out: []u8): usize {
    let pos: usize = 0;
    let written_any = false;
    let idx: usize = 0;
    while (idx < tokens.len) {
        const tok = trim(tokens[idx]);
        if (tok.len > 0) {
            if (written_any && pos < out.len) {
                out[pos] = 32;
                pos += 1;
            }
            pos = sstWriteSlice(tok, out, pos);
            written_any = true;
        }
        idx += 1;
    }
    return pos;
}

/// unifiedjs `stringify` alias: serializes tokens separated by single ASCII space into `out`.
pub fn stringify(tokens: []const string, out: []u8): usize {
    return stringifySpaceTokens(tokens, out);
}

/// Normalizes any whitespace-separated token string into single-space-separated form in `out`.
pub fn normalizeSpaceTokens(input: string, out: []u8): usize {
    let pos: usize = 0;
    let written_any = false;
    let i: usize = 0;
    while (i < input.len) {
        i = sstSkipSpace(input, i);
        if (i >= input.len) {
            break;
        }
        const start = i;
        i = sstScanTokenEnd(input, i);
        if (written_any && pos < out.len) {
            out[pos] = 32;
            pos += 1;
        }
        pos = sstWriteSlice(input[start..i], out, pos);
        written_any = true;
    }
    return pos;
}

/// Parses HTML comma-separated tokens (trimmed, skipping trailing empty token) into `out`.
pub fn parseCommaTokens(input: string, out: []string): usize {
    const trimmed = trim(input);
    if (trimmed.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let start: usize = 0;
    while (start <= trimmed.len) {
        const comma: usize = sstFindComma(trimmed, start);
        const seg = trim(trimmed[start..comma]);
        if (seg.len > 0 || comma < trimmed.len) {
            if (count < out.len) {
                out[count] = seg;
                count += 1;
            }
        }
        if (comma >= trimmed.len) {
            break;
        }
        start = comma + 1;
    }
    return count;
}

/// Serializes tokens into `out` buffer separated by `", "` (HTML spec `comma-separated-tokens`).
pub fn stringifyCommaTokens(tokens: []const string, out: []u8): usize {
    let pos: usize = 0;
    let idx: usize = 0;
    while (idx < tokens.len) {
        if (idx > 0) {
            pos = sstWriteSlice(", ", out, pos);
        }
        pos = sstWriteSlice(trim(tokens[idx]), out, pos);
        idx += 1;
    }
    return pos;
}

export {
    parseSpaceTokens,
    parse,
    countTokens,
    tokenAt,
    hasSpaceToken,
    containsToken,
    stringifySpaceTokens,
    stringify,
    normalizeSpaceTokens,
    parseCommaTokens,
    stringifyCommaTokens,
};
