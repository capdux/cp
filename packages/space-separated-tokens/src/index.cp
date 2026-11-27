// @cap/space-separated-tokens — HTML/SVG space & comma separated token lists in pure CP
// Clean-room MIT implementation of npm `space-separated-tokens` (#644, 90.1M/wk)
// and `comma-separated-tokens` (#648, 89.6M/wk).

import { isSpace, trim, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";

/// Parses HTML space-separated tokens into `out` slice; returns token count written.
pub fn parseSpaceTokens(input: string, out: []string): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        while (i < input.len && isSpace(input[i])) {
            i += 1;
        }
        if (i >= input.len) {
            break;
        }
        const start = i;
        while (i < input.len && !isSpace(input[i])) {
            i += 1;
        }
        if (count < out.len) {
            out[count] = input[start..i];
            count += 1;
        }
    }
    return count;
}

/// Checks whether `target` appears as a token in a space-separated token string.
pub fn hasSpaceToken(input: string, target: string): bool {
    if (target.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < input.len) {
        while (i < input.len && isSpace(input[i])) {
            i += 1;
        }
        if (i >= input.len) {
            break;
        }
        const start = i;
        while (i < input.len && !isSpace(input[i])) {
            i += 1;
        }
        if (strEq(input[start..i], target)) {
            return true;
        }
    }
    return false;
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
            pos = writeStr(tok, out, pos);
            written_any = true;
        }
        idx += 1;
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
    let i: usize = 0;
    while (i <= trimmed.len) {
        if (i == trimmed.len || trimmed[i] == 44) { // ','
            const seg = trim(trimmed[start..i]);
            if (seg.len > 0 || i < trimmed.len) {
                if (count < out.len) {
                    out[count] = seg;
                    count += 1;
                }
            }
            start = i + 1;
        }
        i += 1;
    }
    return count;
}

/// Serializes tokens into `out` buffer separated by `", "` (HTML spec `comma-separated-tokens`).
pub fn stringifyCommaTokens(tokens: []const string, out: []u8): usize {
    let pos: usize = 0;
    let idx: usize = 0;
    while (idx < tokens.len) {
        if (idx > 0) {
            pos = writeStr(", ", out, pos);
        }
        pos = writeStr(trim(tokens[idx]), out, pos);
        idx += 1;
    }
    return pos;
}

export {
    parseSpaceTokens,
    hasSpaceToken,
    stringifySpaceTokens,
    parseCommaTokens,
    stringifyCommaTokens,
};
