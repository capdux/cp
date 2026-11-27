// @cap/soundex — Zero-heap American Soundex phonetic algorithm
// Clean-room MIT implementation for CP (compatible with npm `soundex-code` & Rust `rphonetic`).
// Reuses `cap:ascii` & `cap:bin`.

import { isAlpha, toUpperByte, strEq } from "cap:ascii";
import { bytesEq } from "cap:bin";

pub const DEFAULT_SOUNDEX_LEN: usize = 4;

/// Map an ASCII letter (case-insensitive) to its American Soundex digit ('1'..'6'),
/// 'H'/'W' -> 'H' (transparent separator), vowels/Y/non-alpha -> '0'.
pub fn soundexCharCode(ch: u8): u8 {
    let u = toUpperByte(ch);
    if (u == 'B' || u == 'F' || u == 'P' || u == 'V') { return '1'; }
    if (u == 'C' || u == 'G' || u == 'J' || u == 'K' || u == 'Q' || u == 'S' || u == 'X' || u == 'Z') { return '2'; }
    if (u == 'D' || u == 'T') { return '3'; }
    if (u == 'L') { return '4'; }
    if (u == 'M' || u == 'N') { return '5'; }
    if (u == 'R') { return '6'; }
    if (u == 'H' || u == 'W') { return 'H'; }
    return '0';
}

/// Core American Soundex encoder with configurable `max_length` and minimum padding `min_pad`.
fn soundexImpl(word: string, max_length: usize, min_pad: usize, out: []u8): usize {
    if (max_length == 0 || out.len < min_pad) { return 0; }
    let cap = if (max_length < out.len) { max_length } else { out.len };
    if (cap == 0) { return 0; }

    let i: usize = 0;
    while (i < word.len && !isAlpha(word[i])) {
        i += 1;
    }
    if (i >= word.len) { return 0; }

    let first: u8 = toUpperByte(word[i]);
    out[0] = first;
    let prev_code: u8 = soundexCharCode(first);
    let pos: usize = 1;
    i += 1;

    while (i < word.len && pos < cap) {
        let ch: u8 = word[i];
        if (isAlpha(ch)) {
            let u: u8 = toUpperByte(ch);
            let code: u8 = soundexCharCode(u);
            if (code == 'H') {
                // H and W are transparent: do not reset prev_code
            } else if (code == '0') {
                // Vowels & Y break adjacency
                prev_code = '0';
            } else {
                if (code != prev_code) {
                    out[pos] = code;
                    pos += 1;
                }
                prev_code = code;
            }
        }
        i += 1;
    }

    let pad_target = if (min_pad < cap) { min_pad } else { cap };
    while (pos < pad_target) {
        out[pos] = '0';
        pos += 1;
    }
    return pos;
}

/// Encode `word` into a 4-character American Soundex code (e.g. `"Robert"` -> `"R163"`).
/// Follows NARA American Soundex rules: adjacent duplicates and duplicates separated only by H/W collapse;
/// duplicates separated by a vowel (A,E,I,O,U,Y) are encoded separately.
/// Returns 4 on success, or 0 if `word` has no ASCII letters or `out.len < 4`.
pub fn soundex(word: string, out: []u8): usize {
    if (out.len < DEFAULT_SOUNDEX_LEN) { return 0; }
    return soundexImpl(word, DEFAULT_SOUNDEX_LEN, DEFAULT_SOUNDEX_LEN, out);
}

/// Upstream npm `soundex-code(value, maxLength)` semantics:
/// Encodes up to `max_length` characters (default 4 when `max_length == 0`),
/// padding with `'0'` to at least `min(4, max_length)`.
/// Returns bytes written, or 0 if `word` has no ASCII letters or `out` cannot hold the minimum padded code.
pub fn soundexCode(word: string, max_length: usize, out: []u8): usize {
    let eff_max = if (max_length == 0) { DEFAULT_SOUNDEX_LEN } else { max_length };
    let min_pad = if (eff_max < DEFAULT_SOUNDEX_LEN) { eff_max } else { DEFAULT_SOUNDEX_LEN };
    return soundexImpl(word, eff_max, min_pad, out);
}

/// Encodes `word` into a fixed-length American Soundex code padded with `'0'` to exact `length`.
/// Returns `length` on success, or 0 if `length == 0`, `word` has no ASCII letters, or `out.len < length`.
pub fn soundexPadded(word: string, length: usize, out: []u8): usize {
    if (length == 0 || out.len < length) { return 0; }
    return soundexImpl(word, length, length, out);
}

/// Returns true if `a` and `b` produce the exact same 4-character American Soundex code.
pub fn soundsLike(a: string, b: string): bool {
    let ca: [4]u8 = undefined;
    let cb: [4]u8 = undefined;
    if (soundex(a, ca[0..]) != 4) { return false; }
    if (soundex(b, cb[0..]) != 4) { return false; }
    return strEq(ca[0..], cb[0..]);
}

/// Returns true if `a` and `b` produce the exact same Soundex code at custom `max_length` (up to 64).
pub fn soundsLikeWithLength(a: string, b: string, max_length: usize): bool {
    let eff = if (max_length == 0) { DEFAULT_SOUNDEX_LEN } else if (max_length > 64) { 64 } else { max_length };
    let ca: [64]u8 = undefined;
    let cb: [64]u8 = undefined;
    let la = soundexCode(a, eff, ca[0..64]);
    let lb = soundexCode(b, eff, cb[0..64]);
    if (la == 0 || lb == 0 || la != lb) { return false; }
    return bytesEq(ca[0..la], cb[0..lb]);
}

/// Compute phonetic similarity score 0..4 (number of matching positions in the 4-char Soundex code).
pub fn soundexDifference(a: string, b: string): u8 {
    let ca: [4]u8 = undefined;
    let cb: [4]u8 = undefined;
    if (soundex(a, ca[0..]) != 4) { return 0; }
    if (soundex(b, cb[0..]) != 4) { return 0; }
    let score: u8 = 0;
    let i: usize = 0;
    while (i < 4) {
        if (ca[i] == cb[i]) { score += 1; }
        i += 1;
    }
    return score;
}
