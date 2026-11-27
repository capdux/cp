// packages/@cap/heck-case/src/index.cp
// Zero-allocation Rust `heck` v0.5 compatible case conversion engine for CP.
// Clean-room MIT implementation.
// Accelerated with comptime 256-entry ASCII class/case LUTs and 16-lane @Vector(16, u8) SIMD case folding.

type HckVec16 = @Vector(16, u8);

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

fn hckBuildAsciiClsLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        let cls = HCK_CLS_SEP;
        if (i >= 0x41 && i <= 0x5A) {
            cls = HCK_CLS_UPPER;
        } else if (i >= 0x61 && i <= 0x7A) {
            cls = HCK_CLS_LOWER;
        } else if (i >= 0x30 && i <= 0x39) {
            cls = HCK_CLS_DIGIT;
        }
        tbl[i] = cls;
        i += 1;
    }
    return tbl;
}

fn hckBuildLowerLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[i] = if (i >= 0x41 && i <= 0x5A) { (i + 32) as u8 } else { i as u8 };
        i += 1;
    }
    return tbl;
}

fn hckBuildUpperLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[i] = if (i >= 0x61 && i <= 0x7A) { (i - 32) as u8 } else { i as u8 };
        i += 1;
    }
    return tbl;
}

const HCK_ASCII_CLS_LUT: [256]u8 = hckBuildAsciiClsLut();
const HCK_LOWER_LUT: [256]u8 = hckBuildLowerLut();
const HCK_UPPER_LUT: [256]u8 = hckBuildUpperLut();

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
        return HCK_ASCII_CLS_LUT[cp as usize];
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
    if (cp < 0x80) { return HCK_LOWER_LUT[cp as usize] as u32; }
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) { return cp + 32; }
    if (cp >= 0x0391 && cp <= 0x03AB) { return cp + 32; }
    if (cp >= 0x0410 && cp <= 0x042F) { return cp + 32; }
    return cp;
}

fn hckToUpperCp(cp: u32): u32 {
    if (cp < 0x80) { return HCK_UPPER_LUT[cp as usize] as u32; }
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
    const upper_all = style == HCK_STYLE_UPPER;
    const cap_first = style == HCK_STYLE_CAPITALIZE || (style == HCK_STYLE_LOWER_CAMEL && word_idx > 0);

    // 16-lane SIMD ASCII case-folding fast path
    const v_A: HckVec16 = @splat(65 as u8);
    const v_Z: HckVec16 = @splat(90 as u8);
    const v_a: HckVec16 = @splat(97 as u8);
    const v_z: HckVec16 = @splat(122 as u8);
    const v_32: HckVec16 = @splat(32 as u8);
    const v_0: HckVec16 = @splat(0 as u8);
    const v_1: HckVec16 = @splat(1 as u8);

    while (cur + 16 <= end && w + 16 <= out.len) {
        const v: HckVec16 = src[cur .. cur + 16][0..16].*;
        if (@reduce(.Max, v) >= (128 as u8)) {
            break;
        }
        let folded: HckVec16 = v;
        if (upper_all) {
            const is_lo = (@select(u8, v >= v_a, v_1, v_0) & @select(u8, v <= v_z, v_1, v_0)) == v_1;
            folded = v - @select(u8, is_lo, v_32, v_0);
        } else {
            const is_up = (@select(u8, v >= v_A, v_1, v_0) & @select(u8, v <= v_Z, v_1, v_0)) == v_1;
            folded = v + @select(u8, is_up, v_32, v_0);
        }
        const arr: [16]u8 = folded;
        @memcpy(out[w .. w + 16], arr[0..16]);
        if (is_first_char && cap_first) {
            out[w] = HCK_UPPER_LUT[src[cur] as usize];
        }
        is_first_char = false;
        w += 16;
        cur += 16;
    }

    while (cur < end) {
        const b0 = src[cur];
        if (b0 < (0x80 as u8)) {
            if (w >= out.len) {
                return w;
            }
            if (upper_all || (is_first_char && cap_first)) {
                out[w] = HCK_UPPER_LUT[b0 as usize];
            } else {
                out[w] = HCK_LOWER_LUT[b0 as usize];
            }
            w += 1;
            is_first_char = false;
            cur += 1;
            continue;
        }

        const r = hckDecodeUtf8(src, cur);
        if (r.len == 0) {
            break;
        }
        let mapped: u32 = r.cp;
        if (upper_all || (is_first_char && cap_first)) {
            mapped = hckToUpperCp(r.cp);
        } else {
            mapped = hckToLowerCp(r.cp);
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
