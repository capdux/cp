// @cap/title-case — Zero-allocation AP/Chicago Style Title Case & Sentence Case Engine
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning/lowercasing, comptime 256-entry LUTs, and @memcpy.

import { eqIgnoreCase } from "cap:ascii";

type TcVec16 = @Vector(16, u8);

const TC_FLAG_UPPER: u8 = 1;
const TC_FLAG_LOWER: u8 = 2;
const TC_FLAG_ALPHA: u8 = 3;
const TC_FLAG_DIGIT: u8 = 4;
const TC_FLAG_WORD: u8 = 8;

fn tcBuildCharLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = TC_FLAG_DIGIT | TC_FLAG_WORD;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = TC_FLAG_UPPER | TC_FLAG_WORD;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = TC_FLAG_LOWER | TC_FLAG_WORD;
        c += 1;
    }
    lut[39] = TC_FLAG_WORD; // apostrophe '\'' inside words like "don't"
    return lut;
}

fn tcBuildLowerLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        lut[c] = if (c >= 65 && c <= 90) (c + 32) as u8 else c as u8;
        c += 1;
    }
    return lut;
}

fn tcBuildUpperLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        lut[c] = if (c >= 97 && c <= 122) (c - 32) as u8 else c as u8;
        c += 1;
    }
    return lut;
}

const TC_CHAR_LUT: [256]u8 = tcBuildCharLut();
const TC_LOWER_LUT: [256]u8 = tcBuildLowerLut();
const TC_UPPER_LUT: [256]u8 = tcBuildUpperLut();

fn tcIsUpper(b: u8) -> bool {
    return (TC_CHAR_LUT[b as usize] & TC_FLAG_UPPER) != 0;
}

fn tcIsLower(b: u8) -> bool {
    return (TC_CHAR_LUT[b as usize] & TC_FLAG_LOWER) != 0;
}

fn tcIsAlpha(b: u8) -> bool {
    return (TC_CHAR_LUT[b as usize] & TC_FLAG_ALPHA) != 0;
}

fn tcIsAlnum(b: u8) -> bool {
    return (TC_CHAR_LUT[b as usize] & (TC_FLAG_ALPHA | TC_FLAG_DIGIT)) != 0;
}

/// Returns true if `word` (case-insensitive) is a standard AP/Chicago English title small word:
/// `a, an, and, as, at, but, by, for, if, in, nor, of, on, or, per, so, the, to, up, via, yet`.
pub fn isSmallWord(word: string) -> bool {
    if (word.len == 1) {
        return eqIgnoreCase(word, "a");
    }
    if (word.len == 2) {
        return eqIgnoreCase(word, "an")
            || eqIgnoreCase(word, "as")
            || eqIgnoreCase(word, "at")
            || eqIgnoreCase(word, "by")
            || eqIgnoreCase(word, "if")
            || eqIgnoreCase(word, "in")
            || eqIgnoreCase(word, "of")
            || eqIgnoreCase(word, "on")
            || eqIgnoreCase(word, "or")
            || eqIgnoreCase(word, "so")
            || eqIgnoreCase(word, "to")
            || eqIgnoreCase(word, "up");
    }
    if (word.len == 3) {
        return eqIgnoreCase(word, "and")
            || eqIgnoreCase(word, "but")
            || eqIgnoreCase(word, "for")
            || eqIgnoreCase(word, "nor")
            || eqIgnoreCase(word, "per")
            || eqIgnoreCase(word, "the")
            || eqIgnoreCase(word, "via")
            || eqIgnoreCase(word, "yet");
    }
    return false;
}

fn isWordChar(b: u8) -> bool {
    return (TC_CHAR_LUT[b as usize] & TC_FLAG_WORD) != 0;
}

fn isAllCapsAcronym(word: string) -> bool {
    if (word.len < 2) {
        return false;
    }
    let upper_count: usize = 0;
    let i: usize = 0;
    const v_zero: TcVec16 = @splat(0 as u8);
    const v_one: TcVec16 = @splat(1 as u8);
    const v_lo_a: TcVec16 = @splat(97 as u8);
    const v_lo_z: TcVec16 = @splat(122 as u8);
    const v_up_a: TcVec16 = @splat(65 as u8);
    const v_up_z: TcVec16 = @splat(90 as u8);
    while (i + 16 <= word.len) {
        const v: TcVec16 = word[i .. i + 16][0..16].*;
        const m_lo = @select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero);
        if (@reduce(.Or, m_lo != v_zero)) {
            return false;
        }
        const m_up = @select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero);
        const up_arr: [16]u8 = m_up;
        let lane: usize = 0;
        while (lane < 16) {
            upper_count += up_arr[lane] as usize;
            lane += 1;
        }
        i += 16;
    }
    while (i < word.len) {
        const ch = word[i];
        const fl = TC_CHAR_LUT[ch as usize];
        if ((fl & TC_FLAG_LOWER) != 0) {
            return false;
        }
        if ((fl & TC_FLAG_UPPER) != 0) {
            upper_count += 1;
        }
        i += 1;
    }
    return upper_count >= 2;
}

fn hasMoreWords(input: string, start_pos: usize) -> bool {
    let i = start_pos;
    const v_zero: TcVec16 = @splat(0 as u8);
    const v_one: TcVec16 = @splat(1 as u8);
    const v_lo_a: TcVec16 = @splat(97 as u8);
    const v_lo_z: TcVec16 = @splat(122 as u8);
    const v_up_a: TcVec16 = @splat(65 as u8);
    const v_up_z: TcVec16 = @splat(90 as u8);
    const v_d0: TcVec16 = @splat(48 as u8);
    const v_d9: TcVec16 = @splat(57 as u8);
    while (i + 16 <= input.len) {
        const v: TcVec16 = input[i .. i + 16][0..16].*;
        const alnum = (@select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero))
            | (@select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero))
            | (@select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero));
        if (@reduce(.Or, alnum != v_zero)) {
            return true;
        }
        i += 16;
    }
    while (i < input.len) {
        if (tcIsAlnum(input[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Converts `input` to AP/Chicago style Title Case into `out`:
/// - First word, last word, and first word after `:` are always capitalized.
/// - All-caps acronyms (len >= 2, e.g. `REST`, `API`, `HTTP`, `CP`) are preserved.
/// - Intermediate small words are lowercased; hyphenated segments are capitalized individually.
/// Returns bytes written, or `0` if `input` is empty or `out` is too small.
pub fn toTitleCase(input: string, out: []u8) -> usize {
    const n = input.len;
    if (n == 0 || n > out.len) {
        return 0;
    }

    let i: usize = 0;
    let word_idx: usize = 0;
    let after_colon = false;

    while (i < n) {
        if (!isWordChar(input[i])) {
            const ch = input[i];
            out[i] = ch;
            if (ch == 58) {
                // ':'
                after_colon = true;
            }
            i += 1;
            continue;
        }

        const w_start = i;
        while (i < n && isWordChar(input[i])) {
            i += 1;
        }
        const w_end = i;
        const word = input[w_start..w_end];
        const is_last = !hasMoreWords(input, w_end);

        if (isAllCapsAcronym(word)) {
            @memcpy(out[w_start..w_end], input[w_start..w_end]);
        } else {
            const force_cap = (word_idx == 0) || is_last || after_colon || !isSmallWord(word);
            let capped = false;
            let k = w_start;
            while (k < w_end) {
                const ch = input[k];
                if (tcIsAlpha(ch)) {
                    if (force_cap && !capped) {
                        out[k] = TC_UPPER_LUT[ch as usize];
                        capped = true;
                    } else {
                        out[k] = TC_LOWER_LUT[ch as usize];
                    }
                } else {
                    out[k] = ch;
                }
                k += 1;
            }
        }

        word_idx += 1;
        after_colon = false;
    }

    return n;
}

/// Converts `input` to sentence case into `out`:
/// capitalizes the first letter and the first letter after `.`, `!`, or `?`, lowercasing the rest.
/// Returns bytes written, or `0` if `input` is empty or `out` is too small.
pub fn toSentenceCase(input: string, out: []u8) -> usize {
    const n = input.len;
    if (n == 0 || n > out.len) {
        return 0;
    }

    const v_zero: TcVec16 = @splat(0 as u8);
    const v_one: TcVec16 = @splat(1 as u8);
    const v_up_a: TcVec16 = @splat(65 as u8);
    const v_up_z: TcVec16 = @splat(90 as u8);
    const v_diff: TcVec16 = @splat(32 as u8);
    const v_dot: TcVec16 = @splat(46 as u8);
    const v_excl: TcVec16 = @splat(33 as u8);
    const v_q: TcVec16 = @splat(63 as u8);

    let cap_next = true;
    let i: usize = 0;
    while (i < n) {
        if (!cap_next && i + 16 <= n) {
            const v: TcVec16 = input[i .. i + 16][0..16].*;
            const has_term = @reduce(.Or, v == v_dot) || @reduce(.Or, v == v_excl) || @reduce(.Or, v == v_q);
            if (!has_term) {
                const is_up = @select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero);
                const lowered: TcVec16 = v + @select(u8, is_up != v_zero, v_diff, v_zero);
                const arr: [16]u8 = lowered;
                @memcpy(out[i .. i + 16], arr[0..16]);
                i += 16;
                continue;
            }
        }
        const ch = input[i];
        if (tcIsAlpha(ch)) {
            if (cap_next) {
                out[i] = TC_UPPER_LUT[ch as usize];
                cap_next = false;
            } else {
                out[i] = TC_LOWER_LUT[ch as usize];
            }
        } else {
            out[i] = ch;
            if (ch == 46 || ch == 33 || ch == 63) {
                // '.', '!', '?'
                cap_next = true;
            }
        }
        i += 1;
    }

    return n;
}

/// Capitalizes only the first ASCII letter in `input`, preserving all other bytes verbatim.
/// Returns bytes written, or `0` if `input` is empty or `out` is too small.
pub fn capitalizeFirst(input: string, out: []u8) -> usize {
    const n = input.len;
    if (n == 0 || n > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < n) {
        const ch = input[i];
        if (tcIsAlpha(ch)) {
            out[i] = TC_UPPER_LUT[ch as usize];
            if (i + 1 < n) {
                @memcpy(out[i + 1 .. n], input[i + 1 .. n]);
            }
            return n;
        }
        out[i] = ch;
        i += 1;
    }
    return n;
}
