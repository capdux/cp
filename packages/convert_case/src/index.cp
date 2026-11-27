// packages/@cap/convert-case/src/index.cp
// Zero-allocation 14-style string case conversion engine.
// Clean-room MIT parity with crates.io `convert_case` v0.6.
// Composes `@cap/heck-case` for word boundary detection & 8 core styles with zero duplication.

import { isAlpha, toLowerByte, toUpperByte } from "cap:ascii";
import {
    toUpperCamelCase,
    toLowerCamelCase,
    toSnakeCase,
    toShoutySnakeCase,
    toKebabCase,
    toShoutyKebabCase,
    toTitleCase,
    toTrainCase,
} from "../../heck/src/index.cp";

// --- 14 Case Style Constants (0..13) ---
pub const CASE_UPPER: u8 = 0;
pub const CASE_LOWER: u8 = 1;
pub const CASE_TITLE: u8 = 2;
pub const CASE_CAMEL: u8 = 3;
pub const CASE_PASCAL: u8 = 4;
pub const CASE_SNAKE: u8 = 5;
pub const CASE_UPPER_SNAKE: u8 = 6;
pub const CASE_KEBAB: u8 = 7;
pub const CASE_UPPER_KEBAB: u8 = 8;
pub const CASE_COBOL: u8 = 9;
pub const CASE_TRAIN: u8 = 10;
pub const CASE_FLAT: u8 = 11;
pub const CASE_UPPER_FLAT: u8 = 12;
pub const CASE_ALTERNATING: u8 = 13;

fn ccvReplaceUnderscoreWithSpace(out: []u8, len: usize) {
    let i: usize = 0;
    while (i < len) {
        if (out[i] == (0x5F as u8)) { // '_'
            out[i] = 0x20 as u8;      // ' '
        }
        i += 1;
    }
}

fn ccvStripUnderscores(out: []u8, len: usize): usize {
    let w: usize = 0;
    let r: usize = 0;
    while (r < len) {
        if (out[r] != (0x5F as u8)) {
            out[w] = out[r];
            w += 1;
        }
        r += 1;
    }
    return w;
}

/// Converts `src` to `flatcase` (`helloworld`).
pub fn toFlatCase(src: string, out: []u8): usize {
    const n = toSnakeCase(src, out);
    return ccvStripUnderscores(out, n);
}

/// Converts `src` to `UPPERFLATCASE` (`HELLOWORLD`).
pub fn toUpperFlatCase(src: string, out: []u8): usize {
    const n = toShoutySnakeCase(src, out);
    return ccvStripUnderscores(out, n);
}

/// Converts `src` to `lower case` words (`hello world`).
pub fn toLowerWords(src: string, out: []u8): usize {
    const n = toSnakeCase(src, out);
    ccvReplaceUnderscoreWithSpace(out, n);
    return n;
}

/// Converts `src` to `UPPER CASE` words (`HELLO WORLD`).
pub fn toUpperWords(src: string, out: []u8): usize {
    const n = toShoutySnakeCase(src, out);
    ccvReplaceUnderscoreWithSpace(out, n);
    return n;
}

/// Converts `src` to `aLtErNaTiNg cAsE` (`hElLo WoRlD`).
/// Alternates lowercase/uppercase across letters while preserving word spaces.
pub fn toAlternatingCase(src: string, out: []u8): usize {
    const n = toLowerWords(src, out);
    let letter_idx: usize = 0;
    let i: usize = 0;
    while (i < n) {
        const b = out[i];
        if (isAlpha(b)) {
            out[i] = if ((letter_idx & 1) == 1) toUpperByte(b) else toLowerByte(b);
            letter_idx += 1;
        }
        i += 1;
    }
    return n;
}

/// Unified dispatcher converting `src` into any of the 14 `CASE_*` styles,
/// writing into `out` with zero heap allocation and returning bytes written.
pub fn convertCase(src: string, style: u8, out: []u8): usize {
    if (style == CASE_UPPER) { return toUpperWords(src, out); }
    if (style == CASE_LOWER) { return toLowerWords(src, out); }
    if (style == CASE_TITLE) { return toTitleCase(src, out); }
    if (style == CASE_CAMEL) { return toLowerCamelCase(src, out); }
    if (style == CASE_PASCAL) { return toUpperCamelCase(src, out); }
    if (style == CASE_SNAKE) { return toSnakeCase(src, out); }
    if (style == CASE_UPPER_SNAKE) { return toShoutySnakeCase(src, out); }
    if (style == CASE_KEBAB) { return toKebabCase(src, out); }
    if (style == CASE_UPPER_KEBAB) { return toShoutyKebabCase(src, out); }
    if (style == CASE_COBOL) { return toShoutyKebabCase(src, out); }
    if (style == CASE_TRAIN) { return toTrainCase(src, out); }
    if (style == CASE_FLAT) { return toFlatCase(src, out); }
    if (style == CASE_UPPER_FLAT) { return toUpperFlatCase(src, out); }
    if (style == CASE_ALTERNATING) { return toAlternatingCase(src, out); }
    return 0;
}

export {
    CASE_UPPER,
    CASE_LOWER,
    CASE_TITLE,
    CASE_CAMEL,
    CASE_PASCAL,
    CASE_SNAKE,
    CASE_UPPER_SNAKE,
    CASE_KEBAB,
    CASE_UPPER_KEBAB,
    CASE_COBOL,
    CASE_TRAIN,
    CASE_FLAT,
    CASE_UPPER_FLAT,
    CASE_ALTERNATING,
    convertCase,
    toFlatCase,
    toUpperFlatCase,
    toAlternatingCase,
    toLowerWords,
    toUpperWords,
};
