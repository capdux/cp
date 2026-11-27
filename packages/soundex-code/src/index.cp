// @cap/soundex — Zero-heap American Soundex phonetic algorithm
// Clean-room MIT implementation for CP (compatible with npm soundex-code & Rust rphonetic).

fn toUpper(c: u8): u8 {
    if (c >= 'a' && c <= 'z') { return c - 32; }
    return c;
}

fn isAlpha(c: u8): bool {
    let u: u8 = toUpper(c);
    return u >= 'A' && u <= 'Z';
}

/// Map an uppercase ASCII letter to its American Soundex digit ('1'..'6'),
/// 'H'/'W' -> 'H' (transparent separator), vowels/Y -> '0'.
fn soundexDigit(u: u8): u8 {
    if (u == 'B' || u == 'F' || u == 'P' || u == 'V') { return '1'; }
    if (u == 'C' || u == 'G' || u == 'J' || u == 'K' || u == 'Q' || u == 'S' || u == 'X' || u == 'Z') { return '2'; }
    if (u == 'D' || u == 'T') { return '3'; }
    if (u == 'L') { return '4'; }
    if (u == 'M' || u == 'N') { return '5'; }
    if (u == 'R') { return '6'; }
    if (u == 'H' || u == 'W') { return 'H'; }
    return '0';
}

/// Encode `word` into a 4-character American Soundex code (e.g. `"Robert"` -> `"R163"`).
/// Follows NARA American Soundex rules: adjacent duplicates and duplicates separated only by H/W collapse;
/// duplicates separated by a vowel (A,E,I,O,U,Y) are encoded separately.
/// Returns 4 on success, or 0 if `word` has no ASCII letters or `out.len < 4`.
pub fn soundex(word: string, out: []u8): usize {
    if (out.len < 4) { return 0; }

    let i: usize = 0;
    while (i < word.len && !isAlpha(word[i])) {
        i += 1;
    }
    if (i >= word.len) { return 0; }

    let first: u8 = toUpper(word[i]);
    out[0] = first;
    let prev_code: u8 = soundexDigit(first);
    let pos: usize = 1;
    i += 1;

    while (i < word.len && pos < 4) {
        let ch: u8 = word[i];
        if (isAlpha(ch)) {
            let u: u8 = toUpper(ch);
            let code: u8 = soundexDigit(u);
            if (code == 'H') {
                // H and W do not reset prev_code (adjacent rule passes through H/W)
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

    while (pos < 4) {
        out[pos] = '0';
        pos += 1;
    }
    return 4;
}

/// Returns true if `a` and `b` produce the exact same 4-character American Soundex code.
pub fn soundsLike(a: string, b: string): bool {
    let ca: [4]u8 = undefined;
    let cb: [4]u8 = undefined;
    if (soundex(a, ca[0..]) != 4) { return false; }
    if (soundex(b, cb[0..]) != 4) { return false; }
    return ca[0] == cb[0] && ca[1] == cb[1] && ca[2] == cb[2] && ca[3] == cb[3];
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
