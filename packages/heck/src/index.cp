// packages/@cap/heck-case/src/index.cp
// Zero-allocation Rust `heck` v0.5 compatible case conversion engine for CP.
// Clean-room MIT implementation.

import { isDigit, toLowerByte, toUpperByte } from "cap:ascii";

// ---------------------------------------------------------------------------
// Private UTF-8 & Case Helpers (prefixed with hck)
// ---------------------------------------------------------------------------

struct HckRune {
    cp: u32,
    len: usize,
}

const HCK_CLS_SEP: u8 = 0;
const HCK_CLS_LOWER: u8 = 1;
const HCK_CLS_UPPER: u8 = 2;
const HCK_CLS_DIGIT: u8 = 3;

const HCK_STYLE_LOWER: u8 = 0;
const HCK_STYLE_UPPER: u8 = 1;
const HCK_STYLE_CAPITALIZE: u8 = 2;
const HCK_STYLE_LOWER_CAMEL: u8 = 3;

fn hckDecodeUtf8(src: string, offset: usize): HckRune {
    if (offset >= src.len) {
        return HckRune{ cp: 0, len: 0 };
    }
    const b0 = src[offset];
    if (b0 < (0x80 as u8)) {
        return HckRune{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && offset + 1 < src.len) {
        const b1 = src[offset + 1];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5))
                | ((b1 & (0x3F as u8)) as u32);
            if (cp >= (0x80 as u32)) {
                return HckRune{ cp: cp, len: 2 };
            }
        }
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && offset + 2 < src.len) {
        const b1 = src[offset + 1];
        const b2 = src[offset + 2];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8) && (b2 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp: u32 = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b2 & (0x3F as u8)) as u32);
            if (cp >= (0x800 as u32) && !(cp >= (0xD800 as u32) && cp <= (0xDFFF as u32))) {
                return HckRune{ cp: cp, len: 3 };
            }
        }
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && offset + 3 < src.len) {
        const b1 = src[offset + 1];
        const b2 = src[offset + 2];
        const b3 = src[offset + 3];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)
            && (b2 & (0xC0 as u8)) == (0x80 as u8)
            && (b3 & (0xC0 as u8)) == (0x80 as u8))
        {
            const cp: u32 = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (12 as u5))
                | (((b2 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b3 & (0x3F as u8)) as u32);
            if (cp >= (0x10000 as u32) && cp <= (0x10FFFF as u32)) {
                return HckRune{ cp: cp, len: 4 };
            }
        }
    }
    return HckRune{ cp: 0xFFFD as u32, len: 1 };
}

fn hckClassify(cp: u32): u8 {
    if (cp < 0x80) {
        const b = cp as u8;
        if (b >= 0x41 && b <= 0x5A) { return HCK_CLS_UPPER; }
        if (b >= 0x61 && b <= 0x7A) { return HCK_CLS_LOWER; }
        if (isDigit(b)) { return HCK_CLS_DIGIT; }
        return HCK_CLS_SEP;
    }
    // Latin-1 Supplement uppercase/lowercase letters
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) { return HCK_CLS_UPPER; }
    if ((cp >= 0x00DF && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x00FF)) { return HCK_CLS_LOWER; }
    // Greek & Cyrillic basic letters
    if ((cp >= 0x0391 && cp <= 0x03AB) || (cp >= 0x0410 && cp <= 0x042F)) { return HCK_CLS_UPPER; }
    if ((cp >= 0x03B1 && cp <= 0x03CB) || (cp >= 0x0430 && cp <= 0x044F)) { return HCK_CLS_LOWER; }
    // CJK / Hangul / Hiragana / Katakana & other Unicode letters
    if ((cp >= 0x0100 && cp <= 0x024F)
        || (cp >= 0x3040 && cp <= 0x30FF)
        || (cp >= 0x3400 && cp <= 0x9FFF)
        || (cp >= 0xAC00 && cp <= 0xD7A3))
    {
        return HCK_CLS_LOWER;
    }
    return HCK_CLS_SEP;
}

fn hckToLowerCp(cp: u32): u32 {
    if (cp < 0x80) { return toLowerByte(cp as u8) as u32; }
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) { return cp + 32; }
    if (cp >= 0x0391 && cp <= 0x03AB) { return cp + 32; }
    if (cp >= 0x0410 && cp <= 0x042F) { return cp + 32; }
    return cp;
}

fn hckToUpperCp(cp: u32): u32 {
    if (cp < 0x80) { return toUpperByte(cp as u8) as u32; }
    if ((cp >= 0x00E0 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x00FE)) { return cp - 32; }
    if (cp >= 0x03B1 && cp <= 0x03CB) { return cp - 32; }
    if (cp >= 0x0430 && cp <= 0x044F) { return cp - 32; }
    return cp;
}

fn hckEncodeUtf8(cp: u32, out: []u8, pos: usize): usize {
    if (cp < 0x80) {
        if (pos + 1 > out.len) { return 0; }
        out[pos] = cp as u8;
        return 1;
    }
    if (cp < 0x800) {
        if (pos + 2 > out.len) { return 0; }
        out[pos] = (0xC0 as u8) | ((cp >> (6 as u5)) as u8);
        out[pos + 1] = (0x80 as u8) | ((cp & (0x3F as u32)) as u8);
        return 2;
    }
    if (cp < 0x10000) {
        if (pos + 3 > out.len) { return 0; }
        out[pos] = (0xE0 as u8) | ((cp >> (12 as u5)) as u8);
        out[pos + 1] = (0x80 as u8) | (((cp >> (6 as u5)) & (0x3F as u32)) as u8);
        out[pos + 2] = (0x80 as u8) | ((cp & (0x3F as u32)) as u8);
        return 3;
    }
    if (pos + 4 > out.len) { return 0; }
    out[pos] = (0xF0 as u8) | ((cp >> (18 as u5)) as u8);
    out[pos + 1] = (0x80 as u8) | (((cp >> (12 as u5)) & (0x3F as u32)) as u8);
    out[pos + 2] = (0x80 as u8) | (((cp >> (6 as u5)) & (0x3F as u32)) as u8);
    out[pos + 3] = (0x80 as u8) | ((cp & (0x3F as u32)) as u8);
    return 4;
}

fn hckEmitWord(src: string, start: usize, end: usize, out: []u8, wpos: usize, word_idx: usize, sep: u8, style: u8): usize {
    let w: usize = wpos;
    if (word_idx > 0 && sep != (0 as u8)) {
        if (w >= out.len) {
            return w;
        }
        out[w] = sep;
        w += 1;
    }

    let cur: usize = start;
    let is_first_char: bool = true;
    while (cur < end) {
        const r = hckDecodeUtf8(src, cur);
        if (r.len == 0) {
            break;
        }
        let mapped: u32 = r.cp;
        if (style == HCK_STYLE_LOWER) {
            mapped = hckToLowerCp(r.cp);
        } else if (style == HCK_STYLE_UPPER) {
            mapped = hckToUpperCp(r.cp);
        } else if (style == HCK_STYLE_CAPITALIZE) {
            if (is_first_char) {
                mapped = hckToUpperCp(r.cp);
            } else {
                mapped = hckToLowerCp(r.cp);
            }
        } else if (style == HCK_STYLE_LOWER_CAMEL) {
            if (word_idx == 0) {
                mapped = hckToLowerCp(r.cp);
            } else if (is_first_char) {
                mapped = hckToUpperCp(r.cp);
            } else {
                mapped = hckToLowerCp(r.cp);
            }
        }

        const written: usize = hckEncodeUtf8(mapped, out, w);
        if (written == 0) {
            return w;
        }
        w += written;
        is_first_char = false;
        cur += r.len;
    }
    return w;
}

fn hckConvert(src: string, out: []u8, sep: u8, style: u8): usize {
    if (src.len == 0 || out.len == 0) {
        return 0;
    }

    let pos: usize = 0;
    let w: usize = 0;
    let word_idx: usize = 0;

    while (pos < src.len) {
        // Skip leading/inter-word separators
        const r0 = hckDecodeUtf8(src, pos);
        if (r0.len == 0) {
            break;
        }
        const c0: u8 = hckClassify(r0.cp);
        if (c0 == HCK_CLS_SEP) {
            pos += r0.len;
            continue;
        }

        // Segment one heck word starting at `pos`
        const word_start: usize = pos;
        let cur: usize = pos + r0.len;
        let prev_cls: u8 = c0;
        let prev_pos: usize = pos;
        let upper_run: usize = if (c0 == HCK_CLS_UPPER) { 1 as usize } else { 0 as usize };

        while (cur < src.len) {
            const rn = hckDecodeUtf8(src, cur);
            if (rn.len == 0) {
                break;
            }
            const cn: u8 = hckClassify(rn.cp);
            if (cn == HCK_CLS_SEP) {
                break;
            }

            // Boundary 1: lowercase or digit followed by uppercase (e.g., fooBar, foo2Bar)
            if (cn == HCK_CLS_UPPER && (prev_cls == HCK_CLS_LOWER || prev_cls == HCK_CLS_DIGIT)) {
                break;
            }

            // Boundary 2: acronym followed by capitalized word (e.g., XMLHttp -> XML + Http)
            if (cn == HCK_CLS_LOWER && prev_cls == HCK_CLS_UPPER && upper_run >= 2) {
                cur = prev_pos;
                break;
            }

            if (cn == HCK_CLS_UPPER) {
                upper_run += 1;
            } else {
                upper_run = 0;
            }
            prev_pos = cur;
            prev_cls = cn;
            cur += rn.len;
        }

        w = hckEmitWord(src, word_start, cur, out, w, word_idx, sep, style);
        word_idx += 1;
        pos = cur;
    }

    return w;
}

// ---------------------------------------------------------------------------
// Public Case Conversion API (8 heck v0.5 Styles, Zero-Heap)
// ---------------------------------------------------------------------------

/// Converts `src` to `UpperCamelCase` (`PascalCase`).
pub fn toUpperCamelCase(src: string, out: []u8): usize {
    return hckConvert(src, out, 0 as u8, HCK_STYLE_CAPITALIZE);
}

/// Converts `src` to `lowerCamelCase` (`camelCase`).
pub fn toLowerCamelCase(src: string, out: []u8): usize {
    return hckConvert(src, out, 0 as u8, HCK_STYLE_LOWER_CAMEL);
}

/// Converts `src` to `snake_case`.
pub fn toSnakeCase(src: string, out: []u8): usize {
    return hckConvert(src, out, '_' as u8, HCK_STYLE_LOWER);
}

/// Converts `src` to `SHOUTY_SNAKE_CASE`.
pub fn toShoutySnakeCase(src: string, out: []u8): usize {
    return hckConvert(src, out, '_' as u8, HCK_STYLE_UPPER);
}

/// Converts `src` to `kebab-case`.
pub fn toKebabCase(src: string, out: []u8): usize {
    return hckConvert(src, out, '-' as u8, HCK_STYLE_LOWER);
}

/// Converts `src` to `SHOUTY-KEBAB-CASE`.
pub fn toShoutyKebabCase(src: string, out: []u8): usize {
    return hckConvert(src, out, '-' as u8, HCK_STYLE_UPPER);
}

/// Converts `src` to `Title Case`.
pub fn toTitleCase(src: string, out: []u8): usize {
    return hckConvert(src, out, ' ' as u8, HCK_STYLE_CAPITALIZE);
}

/// Converts `src` to `Train-Case`.
pub fn toTrainCase(src: string, out: []u8): usize {
    return hckConvert(src, out, '-' as u8, HCK_STYLE_CAPITALIZE);
}
