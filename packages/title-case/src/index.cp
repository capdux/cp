// @cap/title-case — Zero-allocation AP/Chicago Style Title Case & Sentence Case Engine
// Clean-room MIT implementation in pure CP.

import { isAlpha, isDigit, toLowerByte, toUpperByte, eqIgnoreCase } from "cap:ascii";

fn isUpper(b: u8) -> bool {
    return b >= 65 && b <= 90;
}

fn isLower(b: u8) -> bool {
    return b >= 97 && b <= 122;
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
    return isAlpha(b) || isDigit(b) || b == 39; // apostrophe '\'' inside words like "don't"
}

fn isAllCapsAcronym(word: string) -> bool {
    if (word.len < 2) {
        return false;
    }
    let upper_count: usize = 0;
    let i: usize = 0;
    while (i < word.len) {
        const ch = word[i];
        if (isLower(ch)) {
            return false;
        }
        if (isUpper(ch)) {
            upper_count += 1;
        }
        i += 1;
    }
    return upper_count >= 2;
}

fn hasMoreWords(input: string, start_pos: usize) -> bool {
    let i = start_pos;
    while (i < input.len) {
        if (isAlpha(input[i]) || isDigit(input[i])) {
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
            let k = w_start;
            while (k < w_end) {
                out[k] = input[k];
                k += 1;
            }
        } else {
            const force_cap = (word_idx == 0) || is_last || after_colon || !isSmallWord(word);
            let capped = false;
            let k = w_start;
            while (k < w_end) {
                const ch = input[k];
                if (isAlpha(ch)) {
                    if (force_cap && !capped) {
                        out[k] = toUpperByte(ch);
                        capped = true;
                    } else {
                        out[k] = toLowerByte(ch);
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

    let cap_next = true;
    let i: usize = 0;
    while (i < n) {
        const ch = input[i];
        if (isAlpha(ch)) {
            if (cap_next) {
                out[i] = toUpperByte(ch);
                cap_next = false;
            } else {
                out[i] = toLowerByte(ch);
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
    let done = false;
    let i: usize = 0;
    while (i < n) {
        const ch = input[i];
        if (!done && isAlpha(ch)) {
            out[i] = toUpperByte(ch);
            done = true;
        } else {
            out[i] = ch;
        }
        i += 1;
    }
    return n;
}
