// @cap/change-case — Zero-heap full-featured case conversion library (change-case v5 parity)
// Clean-room MIT CP implementation of npm `change-case` (80M+/wk).
// Reuses:
//   - `toCamelCase`, `toPascalCase` from `@cap/camelcase`
//   - `decamelize` from `@cap/decamelize/src/core.cp`

import { toCamelCase, toPascalCase } from "../../camelcase/src/index.cp";
import { decamelize } from "../../decamelize/src/core.cp";

fn ccaseIsUpper(c: u8): bool {
    return c >= 65 && c <= 90;
}

fn ccaseIsLower(c: u8): bool {
    return c >= 97 && c <= 122;
}

fn ccaseIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn ccaseIsAlnum(c: u8): bool {
    return ccaseIsUpper(c) || ccaseIsLower(c) || ccaseIsDigit(c);
}

fn ccaseToUpper(c: u8): u8 {
    if (ccaseIsLower(c)) {
        return c - 32;
    }
    return c;
}

fn ccaseToLower(c: u8): u8 {
    if (ccaseIsUpper(c)) {
        return c + 32;
    }
    return c;
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
        while (i < input.len && !ccaseIsAlnum(input[i])) {
            i = i + 1;
        }
        if (i >= input.len) {
            break;
        }

        const start: usize = i;
        while (i < input.len && ccaseIsAlnum(input[i])) {
            i = i + 1;
        }
        const end: usize = i;

        if (seg_count > 0) {
            let k: usize = 0;
            while (k < sep.len && out_pos < out.len) {
                out[out_pos] = sep[k];
                out_pos = out_pos + 1;
                k = k + 1;
            }
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
    let i: usize = 0;
    let cap_next = true;
    while (i < len) {
        if (out[i] == sep_byte) {
            cap_next = true;
        } else if (cap_next) {
            out[i] = ccaseToUpper(out[i]);
            cap_next = false;
        }
        i = i + 1;
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
    while (i < n) {
        out[i] = ccaseToUpper(out[i]);
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
        out[0] = ccaseToUpper(out[0]);
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
    while (i < limit) {
        const c = input[i];
        if (ccaseIsUpper(c)) {
            out[i] = ccaseToLower(c);
        } else if (ccaseIsLower(c)) {
            out[i] = ccaseToUpper(c);
        } else {
            out[i] = c;
        }
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
