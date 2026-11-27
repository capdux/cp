// @cap/change-case — Zero-heap full-featured case conversion library (change-case v5 parity)
// Clean-room MIT CP implementation of npm `change-case` (80M+/wk).
// Reuses:
//   - `toCamelCase`, `toPascalCase` from `@cap/camelcase`
//   - `decamelize` from `@cap/decamelize/src/core.cp`
// Accelerated with comptime 256-entry alnum/upper/swap LUTs, 16-lane @Vector(16, u8) SIMD
// uppercase/swapCase/separator scanning, and @memcpy.

import { toCamelCase, toPascalCase } from "../../ident_case/src/index.cp";
import { decamelize } from "../../decamelize/src/core.cp";

type CcaseVec16 = @Vector(16, u8);

fn ccaseBuildAlnumLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57)) {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

fn ccaseBuildUpperLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 97 && c <= 122) {
            lut[c] = (c - 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

fn ccaseBuildSwapLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 65 && c <= 90) {
            lut[c] = (c + 32) as u8;
        } else if (c >= 97 && c <= 122) {
            lut[c] = (c - 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

const CCASE_ALNUM_LUT: [256]u8 = ccaseBuildAlnumLut();
const CCASE_UPPER_LUT: [256]u8 = ccaseBuildUpperLut();
const CCASE_SWAP_LUT: [256]u8 = ccaseBuildSwapLut();

fn ccaseFindByte(buf: []const u8, start: usize, len: usize, target: u8): usize {
    let i: usize = start;
    const vt: CcaseVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: CcaseVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == target) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (buf[i] == target) {
            return i;
        }
        i += 1;
    }
    return len;
}

/// Splits `input` on non-alphanumeric delimiters and delegates each alphanumeric
/// run to `@cap/decamelize`'s `decamelize`, joining words with `sep` in lowercase.
fn ccaseJoinLower(input: string, sep: string, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }

    let i: usize = 0;
    let out_pos: usize = 0;
    let seg_count: usize = 0;

    while (i < input.len && out_pos < out.len) {
        while (i < input.len && CCASE_ALNUM_LUT[input[i] as usize] == 0) {
            i = i + 1;
        }
        if (i >= input.len) {
            break;
        }

        const start: usize = i;
        while (i < input.len && CCASE_ALNUM_LUT[input[i] as usize] != 0) {
            i = i + 1;
        }
        const end: usize = i;

        if (seg_count > 0 && sep.len > 0) {
            const avail = out.len - out_pos;
            const copy_len = if (sep.len < avail) { sep.len } else { avail };
            @memcpy(out[out_pos .. out_pos + copy_len], sep[0..copy_len]);
            out_pos = out_pos + copy_len;
            if (out_pos >= out.len) {
                break;
            }
        }

        const written = decamelize(input[start..end], sep, out[out_pos..out.len]);
        out_pos = out_pos + written;
        seg_count = seg_count + 1;
    }

    return out_pos;
}

/// Capitalizes the first byte and every byte immediately following `sep_byte` in `out[0..len]`.
fn ccaseCapitalizeWords(out: []u8, len: usize, sep_byte: u8) {
    if (len == 0) {
        return;
    }
    if (out[0] != sep_byte) {
        out[0] = CCASE_UPPER_LUT[out[0] as usize];
    }
    let i: usize = 0;
    while (i < len) {
        const sep_pos = ccaseFindByte(out, i, len, sep_byte);
        if (sep_pos >= len) {
            break;
        }
        let next = sep_pos + 1;
        while (next < len && out[next] == sep_byte) {
            next += 1;
        }
        if (next < len) {
            out[next] = CCASE_UPPER_LUT[out[next] as usize];
        }
        i = next;
    }
}

/// Converts string to `camelCase` (e.g. `"foo_bar-baz"` -> `"fooBarBaz"`).
pub fn camelCase(input: string, out: []u8): usize {
    return toCamelCase(input, out);
}

/// Converts string to `PascalCase` (e.g. `"foo_bar-baz"` -> `"FooBarBaz"`).
pub fn pascalCase(input: string, out: []u8): usize {
    return toPascalCase(input, out);
}

/// Converts string to `snake_case` (e.g. `"fooBarBaz"` -> `"foo_bar_baz"`).
pub fn snakeCase(input: string, out: []u8): usize {
    return ccaseJoinLower(input, "_", out);
}

/// Converts string to `kebab-case` (e.g. `"fooBarBaz"` -> `"foo-bar-baz"`).
pub fn kebabCase(input: string, out: []u8): usize {
    return ccaseJoinLower(input, "-", out);
}

/// Converts string to `CONSTANT_CASE` (e.g. `"fooBarBaz"` -> `"FOO_BAR_BAZ"`).
pub fn constantCase(input: string, out: []u8): usize {
    const n = ccaseJoinLower(input, "_", out);
    let i: usize = 0;
    const v_la: CcaseVec16 = @splat(97 as u8);
    const v_26: CcaseVec16 = @splat(26 as u8);
    const v_32: CcaseVec16 = @splat(32 as u8);
    while (i + 16 <= n) {
        const v: CcaseVec16 = out[i .. i + 16][0..16].*;
        const res = @select(u8, (v -% v_la) < v_26, v -% v_32, v);
        const arr: [16]u8 = res;
        @memcpy(out[i .. i + 16], arr[0..16]);
        i += 16;
    }
    while (i < n) {
        out[i] = CCASE_UPPER_LUT[out[i] as usize];
        i = i + 1;
    }
    return n;
}

/// Converts string to `dot.case` (e.g. `"fooBarBaz"` -> `"foo.bar.baz"`).
pub fn dotCase(input: string, out: []u8): usize {
    return ccaseJoinLower(input, ".", out);
}

/// Converts string to `path/case` (e.g. `"fooBarBaz"` -> `"foo/bar/baz"`).
pub fn pathCase(input: string, out: []u8): usize {
    return ccaseJoinLower(input, "/", out);
}

/// Converts string to `Train-Case` / HTTP header style (e.g. `"content_type"` -> `"Content-Type"`).
pub fn trainCase(input: string, out: []u8): usize {
    const n = ccaseJoinLower(input, "-", out);
    ccaseCapitalizeWords(out, n, '-' as u8);
    return n;
}

/// Converts string to `Capital Case` (e.g. `"fooBarBaz"` -> `"Foo Bar Baz"`).
pub fn capitalCase(input: string, out: []u8): usize {
    const n = ccaseJoinLower(input, " ", out);
    ccaseCapitalizeWords(out, n, ' ' as u8);
    return n;
}

/// Converts string to `Sentence case` (e.g. `"fooBarBaz"` -> `"Foo bar baz"`).
pub fn sentenceCase(input: string, out: []u8): usize {
    const n = ccaseJoinLower(input, " ", out);
    if (n > 0) {
        out[0] = CCASE_UPPER_LUT[out[0] as usize];
    }
    return n;
}

/// Converts string to `no case` lowercase space-separated words (e.g. `"fooBarBaz"` -> `"foo bar baz"`).
pub fn noCase(input: string, out: []u8): usize {
    return ccaseJoinLower(input, " ", out);
}

/// Swaps ASCII letter case (e.g. `"FooBar"` -> `"fOObAR"`).
pub fn swapCase(input: string, out: []u8): usize {
    const limit = if (input.len < out.len) { input.len } else { out.len };
    let i: usize = 0;
    const v_la: CcaseVec16 = @splat(97 as u8);
    const v_ua: CcaseVec16 = @splat(65 as u8);
    const v_26: CcaseVec16 = @splat(26 as u8);
    const v_32: CcaseVec16 = @splat(32 as u8);
    while (i + 16 <= limit) {
        const v: CcaseVec16 = input[i .. i + 16][0..16].*;
        const s1 = @select(u8, (v -% v_ua) < v_26, v +% v_32, v);
        const s2 = @select(u8, (v -% v_la) < v_26, v -% v_32, s1);
        const arr: [16]u8 = s2;
        @memcpy(out[i .. i + 16], arr[0..16]);
        i += 16;
    }
    while (i < limit) {
        out[i] = CCASE_SWAP_LUT[input[i] as usize];
        i = i + 1;
    }
    return limit;
}

export {
    camelCase,
    pascalCase,
    snakeCase,
    kebabCase,
    constantCase,
    dotCase,
    pathCase,
    trainCase,
    capitalCase,
    sentenceCase,
    noCase,
    swapCase,
};
