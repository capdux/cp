// @cap/escape-html — Zero-heap single-pass HTML entity escaper & unescaper in pure CP
// MIT License — Clean-room implementation inspired by npm `escape-html`
// Accelerated with comptime 256-entry LUT, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

import { startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";

type EhVec16 = @Vector(16, u8);

fn ehBuildLenLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [1; 256];
    lut[38] = 5; // '&' -> "&amp;"
    lut[60] = 4; // '<' -> "&lt;"
    lut[62] = 4; // '>' -> "&gt;"
    lut[34] = 6; // '"' -> "&quot;"
    lut[39] = 5; // '\'' -> "&#39;"
    return lut;
}

const EH_LEN_LUT: [256]u8 = ehBuildLenLut();

fn ehFindSpecial(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_amp: EhVec16 = @splat(38 as u8);
    const v_lt: EhVec16 = @splat(60 as u8);
    const v_gt: EhVec16 = @splat(62 as u8);
    const v_qt: EhVec16 = @splat(34 as u8);
    const v_sq: EhVec16 = @splat(39 as u8);
    while (i + 16 <= len) {
        const v: EhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_amp) ||
            @reduce(.Or, v == v_lt) ||
            @reduce(.Or, v == v_gt) ||
            @reduce(.Or, v == v_qt) ||
            @reduce(.Or, v == v_sq))
        {
            let k: usize = 0;
            while (k < 16) {
                if (EH_LEN_LUT[s[i + k] as usize] != 1) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (EH_LEN_LUT[s[i] as usize] != 1) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn ehFindAmp(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_amp: EhVec16 = @splat(38 as u8);
    while (i + 16 <= len) {
        const v: EhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_amp)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 38) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != 38) {
        i += 1;
    }
    return i;
}

/// Return true if `input` contains any of `&`, `<`, `>`, `"`, or `'`.
pub fn needsHtmlEscape(input: string): bool {
    return ehFindSpecial(input, 0) < input.len;
}

/// Precompute the exact escaped byte length of `input` in O(n) time without writing.
pub fn escapedHtmlLen(input: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const next = ehFindSpecial(input, i);
        total += next - i;
        if (next >= input.len) {
            break;
        }
        total += EH_LEN_LUT[input[next] as usize] as usize;
        i = next + 1;
    }
    return total;
}

/// Escape HTML special characters (`&`, `<`, `>`, `"`, `'`) into `out` in a single pass.
/// Safely truncates at `out.len` if `out` is smaller than `escapedHtmlLen(input)`.
/// Returns the number of bytes written to `out`.
pub fn escapeHtml(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len && w < out.len) {
        const next = ehFindSpecial(input, i);
        if (next > i) {
            const avail = out.len - w;
            const chunk = next - i;
            const copy_len = if (chunk < avail) chunk else avail;
            @memcpy(out[w .. w + copy_len], input[i .. i + copy_len]);
            w += copy_len;
            i += copy_len;
            if (w >= out.len || i >= input.len) {
                break;
            }
        }
        const ch: u8 = input[i];
        if (ch == 38) { // '&'
            w = writeStr("&amp;", out, w);
        } else if (ch == 60) { // '<'
            w = writeStr("&lt;", out, w);
        } else if (ch == 62) { // '>'
            w = writeStr("&gt;", out, w);
        } else if (ch == 34) { // '"'
            w = writeStr("&quot;", out, w);
        } else if (ch == 39) { // '\''
            w = writeStr("&#39;", out, w);
        } else {
            out[w] = ch;
            w += 1;
        }
        i += 1;
    }
    return w;
}

/// Unescape standard HTML entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&#x27;`, `&apos;`)
/// into `out`. Unknown or malformed entities (e.g. `&foo;`) are preserved verbatim.
/// Returns the number of bytes written to `out`.
pub fn unescapeHtml(input: string, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len && w < out.len) {
        const next = ehFindAmp(input, i);
        if (next > i) {
            const avail = out.len - w;
            const chunk = next - i;
            const copy_len = if (chunk < avail) chunk else avail;
            @memcpy(out[w .. w + copy_len], input[i .. i + copy_len]);
            w += copy_len;
            i += copy_len;
            if (w >= out.len || i >= input.len) {
                break;
            }
        }
        const rest = input[i..input.len];
        if (startsWith(rest, "&amp;")) {
            out[w] = 38;
            w += 1;
            i += 5;
            continue;
        }
        if (startsWith(rest, "&lt;")) {
            out[w] = 60;
            w += 1;
            i += 4;
            continue;
        }
        if (startsWith(rest, "&gt;")) {
            out[w] = 62;
            w += 1;
            i += 4;
            continue;
        }
        if (startsWith(rest, "&quot;")) {
            out[w] = 34;
            w += 1;
            i += 6;
            continue;
        }
        if (startsWith(rest, "&#39;")) {
            out[w] = 39;
            w += 1;
            i += 5;
            continue;
        }
        if (startsWith(rest, "&#x27;") || startsWith(rest, "&#X27;")) {
            out[w] = 39;
            w += 1;
            i += 6;
            continue;
        }
        if (startsWith(rest, "&apos;")) {
            out[w] = 39;
            w += 1;
            i += 6;
            continue;
        }
        out[w] = input[i];
        w += 1;
        i += 1;
    }
    return w;
}
