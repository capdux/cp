// ident_case — Zero-heap identifier case converter with full Rust `ident_case::RenameRule` parity
// MIT License (clean-room CP implementation, reuses `cap:ascii`)

import { isDigit, isAlnum, toLowerByte, toUpperByte, strEq } from "cap:ascii";

/// All 8 naming conventions supported by Rust `ident_case` / `serde` `RenameRule`.
pub enum RenameRule {
    LowerCase,
    UpperCase,
    CamelCase,
    PascalCase,
    SnakeCase,
    ScreamingSnakeCase,
    KebabCase,
    ScreamingKebabCase,
}

fn isUpper(c: u8) -> bool {
    return c >= 'A' as u8 && c <= 'Z' as u8;
}

fn isLower(c: u8) -> bool {
    return c >= 'a' as u8 && c <= 'z' as u8;
}

fn nextWord(input: string, pos: usize, out_start: *usize, out_end: *usize) -> bool {
    let i: usize = pos;
    while (i < input.len && !isAlnum(input[i])) {
        i += 1;
    }
    if (i >= input.len) {
        return false;
    }

    const start: usize = i;
    i += 1;
    while (i < input.len && isAlnum(input[i])) {
        const prev: u8 = input[i - 1];
        const cur: u8 = input[i];
        // Boundary: lowercase/digit -> Uppercase (foo|Bar, v2|Beta)
        if ((isLower(prev) || isDigit(prev)) && isUpper(cur)) {
            break;
        }
        // Boundary: Acronym -> Title word (XML|Http, HTTP|Response)
        if (isUpper(prev) && isUpper(cur) && i + 1 < input.len && isLower(input[i + 1])) {
            break;
        }
        i += 1;
    }

    out_start.* = start;
    out_end.* = i;
    return true;
}

fn convertJoined(
    input: string,
    out: []u8,
    sep: u8,
    has_sep: bool,
    upper_all: bool,
    cap_first_word: bool,
    cap_rest_words: bool,
) -> usize {
    let pos: usize = 0;
    let out_pos: usize = 0;
    let word_idx: usize = 0;

    while (pos < input.len) {
        let ws: usize = 0;
        let we: usize = 0;
        if (!nextWord(input, pos, &ws, &we)) {
            break;
        }

        if (word_idx > 0 && has_sep) {
            if (out_pos >= out.len) {
                return 0;
            }
            out[out_pos] = sep;
            out_pos += 1;
        }

        const wlen: usize = we - ws;
        if (wlen > out.len - out_pos) {
            return 0;
        }

        let k: usize = 0;
        while (k < wlen) {
            const c: u8 = input[ws + k];
            if (upper_all) {
                out[out_pos] = toUpperByte(c);
            } else if (k == 0 && ((word_idx == 0 && cap_first_word) || (word_idx > 0 && cap_rest_words))) {
                out[out_pos] = toUpperByte(c);
            } else {
                out[out_pos] = toLowerByte(c);
            }
            out_pos += 1;
            k += 1;
        }

        word_idx += 1;
        pos = we;
    }

    return out_pos;
}

/// Converts `input` into `lowercase` (all words concatenated in ASCII lowercase).
pub fn toLowerCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, 0, false, false, false, false);
}

/// Converts `input` into `UPPERCASE` (all words concatenated in ASCII uppercase).
pub fn toUpperCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, 0, false, true, false, false);
}

/// Converts `input` into `camelCase`.
pub fn toCamelCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, 0, false, false, false, true);
}

/// Converts `input` into `PascalCase`.
pub fn toPascalCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, 0, false, false, true, true);
}

/// Converts `input` into `snake_case`.
pub fn toSnakeCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '_' as u8, true, false, false, false);
}

/// Converts `input` into `kebab-case`.
pub fn toKebabCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '-' as u8, true, false, false, false);
}

/// Converts `input` into `SCREAMING_SNAKE_CASE`.
pub fn toScreamingSnakeCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '_' as u8, true, true, false, false);
}

/// Converts `input` into `SCREAMING-KEBAB-CASE`.
pub fn toScreamingKebabCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '-' as u8, true, true, false, false);
}

/// Applies any of the 8 `RenameRule` variants to `input`, writing into `out`.
pub fn renameIdent(rule: RenameRule, input: string, out: []u8) -> usize {
    return switch (rule) {
        .LowerCase => toLowerCase(input, out),
        .UpperCase => toUpperCase(input, out),
        .CamelCase => toCamelCase(input, out),
        .PascalCase => toPascalCase(input, out),
        .SnakeCase => toSnakeCase(input, out),
        .ScreamingSnakeCase => toScreamingSnakeCase(input, out),
        .KebabCase => toKebabCase(input, out),
        .ScreamingKebabCase => toScreamingKebabCase(input, out),
    };
}

/// Parses a standard serde/ident_case rule name string into a `RenameRule`.
pub fn parseRenameRule(name: string) -> RenameRule? {
    if (strEq(name, "lowercase")) { return RenameRule.LowerCase; }
    if (strEq(name, "UPPERCASE")) { return RenameRule.UpperCase; }
    if (strEq(name, "camelCase")) { return RenameRule.CamelCase; }
    if (strEq(name, "PascalCase")) { return RenameRule.PascalCase; }
    if (strEq(name, "snake_case")) { return RenameRule.SnakeCase; }
    if (strEq(name, "SCREAMING_SNAKE_CASE")) { return RenameRule.ScreamingSnakeCase; }
    if (strEq(name, "kebab-case")) { return RenameRule.KebabCase; }
    if (strEq(name, "SCREAMING-KEBAB-CASE")) { return RenameRule.ScreamingKebabCase; }
    return null;
}

export {
    RenameRule,
    toLowerCase,
    toUpperCase,
    toCamelCase,
    toPascalCase,
    toSnakeCase,
    toKebabCase,
    toScreamingSnakeCase,
    toScreamingKebabCase,
    renameIdent,
    parseRenameRule,
};
