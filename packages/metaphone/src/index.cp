// @cap/metaphone — Zero-allocation Lawrence Philips Metaphone Phonetic Encoder
// Clean-room MIT implementation in pure CP.

import { isAlpha, toUpperByte } from "cap:ascii";
import { bytesEq } from "cap:bin";

fn isVowel(b: u8) -> bool {
    return b == 65 || b == 69 || b == 73 || b == 79 || b == 85; // A E I O U
}

/// Encodes `word` into a Lawrence Philips Metaphone phonetic key of at most `max_len` bytes in `out`.
/// Returns bytes written, or `0` if `word` contains no ASCII letters, `max_len == 0`, or `out` is empty.
pub fn metaphone(word: string, max_len: usize, out: []u8) -> usize {
    if (word.len == 0 || max_len == 0 || out.len == 0) {
        return 0;
    }

    let limit = max_len;
    if (out.len < limit) {
        limit = out.len;
    }

    // Extract uppercase ASCII letters into stack buffer
    let clean: [128]u8 = undefined;
    let n: usize = 0;
    let wi: usize = 0;
    while (wi < word.len && n < 128) {
        if (isAlpha(word[wi])) {
            clean[n] = toUpperByte(word[wi]);
            n += 1;
        }
        wi += 1;
    }
    if (n == 0) {
        return 0;
    }

    let pos: usize = 0;
    let w: usize = 0;

    // Handle silent initial letter pairs: KN, GN, PN, AE, WR
    if (n >= 2) {
        const c0 = clean[0];
        const c1 = clean[1];
        if ((c0 == 75 && c1 == 78)
            || (c0 == 71 && c1 == 78)
            || (c0 == 80 && c1 == 78)
            || (c0 == 65 && c1 == 69)
            || (c0 == 87 && c1 == 82))
        {
            pos = 1;
        } else if (c0 == 87 && c1 == 72) {
            // WH -> W
            out[w] = 87;
            w += 1;
            pos = 2;
        }
    }

    // Initial X -> S
    if (pos == 0 && clean[0] == 88 && w < limit) {
        out[w] = 83;
        w += 1;
        pos = 1;
    }

    while (pos < n && w < limit) {
        const cur = clean[pos];
        let prev: u8 = 0;
        if (pos > 0) { prev = clean[pos - 1]; }
        let next: u8 = 0;
        if (pos + 1 < n) { next = clean[pos + 1]; }
        let next2: u8 = 0;
        if (pos + 2 < n) { next2 = clean[pos + 2]; }

        // Deduplicate adjacent identical letters except 'C'
        if (cur != 67 && cur == prev) {
            pos += 1;
            continue;
        }

        // Vowels are kept only at position 0
        if (isVowel(cur)) {
            if (pos == 0) {
                out[w] = cur;
                w += 1;
            }
            pos += 1;
            continue;
        }

        if (cur == 66) {
            // 'B': silent after 'M' at end of word
            if (!(prev == 77 && pos + 1 == n)) {
                out[w] = 66;
                w += 1;
            }
            pos += 1;
        } else if (cur == 67) {
            // 'C'
            if (next == 73 && next2 == 65) {
                out[w] = 88; // CIA -> X
                w += 1;
                pos += 1;
            } else if (next == 72) {
                if (prev == 83 || (pos == 0 && next2 != 0 && !isVowel(next2))) {
                    out[w] = 75; // SCH or CH + consonant -> K
                } else {
                    out[w] = 88; // CH -> X
                }
                w += 1;
                pos += 2;
            } else if (next == 73 || next == 69 || next == 89) {
                if (prev != 83) {
                    out[w] = 83; // CI/CE/CY -> S
                    w += 1;
                }
                pos += 1;
            } else if (next == 75) {
                // CK -> silent C, next K emits K
                pos += 1;
            } else {
                out[w] = 75; // K
                w += 1;
                pos += 1;
            }
        } else if (cur == 68) {
            // 'D'
            if (next == 71 && (next2 == 69 || next2 == 73 || next2 == 89)) {
                out[w] = 74; // DGE/DGI/DGY -> J
                w += 1;
                pos += 2;
            } else {
                out[w] = 84; // T
                w += 1;
                pos += 1;
            }
        } else if (cur == 70) {
            // 'F'
            out[w] = 70;
            w += 1;
            pos += 1;
        } else if (cur == 71) {
            // 'G'
            if (next == 72) {
                if (pos == 0) {
                    out[w] = 75; // initial GH -> K
                    w += 1;
                }
                pos += 2;
            } else if (next == 78 && (pos + 2 == n || (next2 == 69 && pos + 3 == n))) {
                // silent GN at end
                pos += 1;
            } else if ((next == 73 || next == 69 || next == 89) && prev != 71) {
                out[w] = 74; // J
                w += 1;
                pos += 1;
            } else {
                out[w] = 75; // K
                w += 1;
                pos += 1;
            }
        } else if (cur == 72) {
            // 'H': pronounced only before a vowel and not after C/S/P/T/G or vowel-H-consonant
            const after_digraph = prev == 67 || prev == 83 || prev == 80 || prev == 84 || prev == 71;
            if (!after_digraph && isVowel(next)) {
                out[w] = 72;
                w += 1;
            }
            pos += 1;
        } else if (cur == 74) {
            // 'J'
            out[w] = 74;
            w += 1;
            pos += 1;
        } else if (cur == 75) {
            // 'K'
            out[w] = 75;
            w += 1;
            pos += 1;
        } else if (cur == 76) {
            // 'L'
            out[w] = 76;
            w += 1;
            pos += 1;
        } else if (cur == 77) {
            // 'M'
            out[w] = 77;
            w += 1;
            pos += 1;
        } else if (cur == 78) {
            // 'N'
            out[w] = 78;
            w += 1;
            pos += 1;
        } else if (cur == 80) {
            // 'P'
            if (next == 72) {
                out[w] = 70; // PH -> F
                w += 1;
                pos += 2;
            } else {
                out[w] = 80;
                w += 1;
                pos += 1;
            }
        } else if (cur == 81) {
            // 'Q' -> K
            out[w] = 75;
            w += 1;
            pos += 1;
        } else if (cur == 82) {
            // 'R'
            out[w] = 82;
            w += 1;
            pos += 1;
        } else if (cur == 83) {
            // 'S'
            if (next == 72) {
                out[w] = 88; // SH -> X
                w += 1;
                pos += 2;
            } else if (next == 73 && (next2 == 79 || next2 == 65)) {
                out[w] = 88; // SIO/SIA -> X
                w += 1;
                pos += 1;
            } else {
                out[w] = 83;
                w += 1;
                pos += 1;
            }
        } else if (cur == 84) {
            // 'T'
            if (next == 72) {
                out[w] = 48; // TH -> '0'
                w += 1;
                pos += 2;
            } else if (next == 73 && (next2 == 79 || next2 == 65)) {
                out[w] = 88; // TIO/TIA -> X
                w += 1;
                pos += 1;
            } else if (next == 67 && next2 == 72) {
                // TCH -> silent T
                pos += 1;
            } else {
                out[w] = 84;
                w += 1;
                pos += 1;
            }
        } else if (cur == 86) {
            // 'V' -> F
            out[w] = 70;
            w += 1;
            pos += 1;
        } else if (cur == 87 || cur == 89) {
            // 'W' / 'Y': kept only if followed by a vowel
            if (isVowel(next)) {
                out[w] = cur;
                w += 1;
            }
            pos += 1;
        } else if (cur == 88) {
            // 'X' -> KS
            out[w] = 75;
            w += 1;
            if (w < limit) {
                out[w] = 83;
                w += 1;
            }
            pos += 1;
        } else if (cur == 90) {
            // 'Z' -> S
            out[w] = 83;
            w += 1;
            pos += 1;
        } else {
            pos += 1;
        }
    }

    return w;
}

/// Returns true if `a` and `b` share the same non-empty 4-character Metaphone phonetic key.
pub fn soundsLike(a: string, b: string) -> bool {
    let ka: [4]u8 = undefined;
    let kb: [4]u8 = undefined;
    const la = metaphone(a, 4, ka[0..4]);
    const lb = metaphone(b, 4, kb[0..4]);
    return la > 0 && bytesEq(ka[0..la], kb[0..lb]);
}
