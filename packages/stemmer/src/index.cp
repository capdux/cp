// @cap/porter-stem — Zero-allocation Martin Porter (1980) English Stemmer
// Clean-room MIT implementation in pure CP (Steps 1a, 1b, 1c, 2, 3, 4, 5a, 5b).

const MAX_WORD_LEN: usize = 128;

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn isConsonant(buf: []const u8, idx: usize) -> bool {
    const ch = buf[idx];
    if (ch == 97 || ch == 101 || ch == 105 || ch == 111 || ch == 117) {
        return false; // a, e, i, o, u
    }
    if (ch == 121) {
        // 'y'
        if (idx == 0) {
            return true;
        }
        return !isConsonant(buf, idx - 1);
    }
    return true;
}

fn measurePrefix(buf: []const u8, end: usize) -> usize {
    let m: usize = 0;
    let i: usize = 0;
    // Skip leading consonants
    while (i < end && isConsonant(buf, i)) {
        i += 1;
    }
    while (i < end) {
        // Consume vowels
        while (i < end && !isConsonant(buf, i)) {
            i += 1;
        }
        if (i >= end) {
            break;
        }
        // Consume consonants -> completes one VC sequence
        while (i < end && isConsonant(buf, i)) {
            i += 1;
        }
        m += 1;
    }
    return m;
}

/// Computes the Porter consonant-vowel sequence measure `m` for `word`.
pub fn measure(word: []const u8) -> usize {
    let buf: [128]u8 = undefined;
    let n = word.len;
    if (n > MAX_WORD_LEN) {
        n = MAX_WORD_LEN;
    }
    let i: usize = 0;
    while (i < n) {
        buf[i] = toLowerAscii(word[i]);
        i += 1;
    }
    return measurePrefix(buf[0..n], n);
}

fn containsVowel(buf: []const u8, end: usize) -> bool {
    let i: usize = 0;
    while (i < end) {
        if (!isConsonant(buf, i)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn endsDoubleConsonant(buf: []const u8, end: usize) -> bool {
    if (end < 2) {
        return false;
    }
    if (buf[end - 1] != buf[end - 2]) {
        return false;
    }
    return isConsonant(buf, end - 1);
}

fn endsCvc(buf: []const u8, end: usize) -> bool {
    if (end < 3) {
        return false;
    }
    if (!isConsonant(buf, end - 1) || isConsonant(buf, end - 2) || !isConsonant(buf, end - 3)) {
        return false;
    }
    const last = buf[end - 1];
    // Final consonant cannot be w, x, or y
    return last != 119 && last != 120 && last != 121;
}

fn endsWith(buf: []const u8, len: usize, suffix: string) -> bool {
    if (suffix.len > len) {
        return false;
    }
    const start = len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (buf[start + i] != suffix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn replaceSuffix(buf: []u8, len: usize, suf_len: usize, rep: string) -> usize {
    const stem_len = len - suf_len;
    let i: usize = 0;
    while (i < rep.len && stem_len + i < buf.len) {
        buf[stem_len + i] = rep[i];
        i += 1;
    }
    return stem_len + i;
}

fn tryStep2(buf: []u8, len: usize) -> usize {
    if (len < 3) {
        return len;
    }
    const sufs: [21]string = [
        "ational", "tional", "enci", "anci", "izer", "abli", "alli",
        "entli", "eli", "ousli", "ization", "ation", "ator", "alism",
        "iveness", "fulness", "ousness", "aliti", "iviti", "biliti", "logi",
    ];
    const reps: [21]string = [
        "ate", "tion", "ence", "ance", "ize", "able", "al",
        "ent", "e", "ous", "ize", "ate", "ate", "al",
        "ive", "ful", "ous", "al", "ive", "ble", "log",
    ];
    let i: usize = 0;
    while (i < 21) {
        const s = sufs[i];
        if (endsWith(buf, len, s)) {
            const stem_len = len - s.len;
            if (measurePrefix(buf, stem_len) > 0) {
                return replaceSuffix(buf, len, s.len, reps[i]);
            }
            return len;
        }
        i += 1;
    }
    return len;
}

fn tryStep3(buf: []u8, len: usize) -> usize {
    const sufs: [7]string = ["icate", "ative", "alize", "iciti", "ical", "ful", "ness"];
    const reps: [7]string = ["ic", "", "al", "ic", "ic", "", ""];
    let i: usize = 0;
    while (i < 7) {
        const s = sufs[i];
        if (endsWith(buf, len, s)) {
            const stem_len = len - s.len;
            if (measurePrefix(buf, stem_len) > 0) {
                return replaceSuffix(buf, len, s.len, reps[i]);
            }
            return len;
        }
        i += 1;
    }
    return len;
}

fn tryStep4(buf: []u8, len: usize) -> usize {
    const sufs: [19]string = [
        "al", "ance", "ence", "er", "ic", "able", "ible", "ant",
        "ement", "ment", "ent", "ion", "ou", "ism", "ate", "iti",
        "ous", "ive", "ize",
    ];
    let i: usize = 0;
    while (i < 19) {
        const s = sufs[i];
        if (endsWith(buf, len, s)) {
            const stem_len = len - s.len;
            if (i == 11) {
                // "ion" requires stem ending in 's' or 't'
                if (stem_len == 0 || (buf[stem_len - 1] != 115 && buf[stem_len - 1] != 116)) {
                    return len;
                }
            }
            if (measurePrefix(buf, stem_len) > 1) {
                return stem_len;
            }
            return len;
        }
        i += 1;
    }
    return len;
}

/// Stems an English `word` using the Martin Porter (1980) algorithm (Steps 1a-5b).
/// Outputs lowercase stem into `out`. Words shorter than 3 bytes are lowercased verbatim.
/// Returns bytes written, or `0` if `word` is empty or `out` is too small.
pub fn stem(word: string, out: []u8) -> usize {
    if (word.len == 0) {
        return 0;
    }
    let n = word.len;
    if (n > MAX_WORD_LEN) {
        n = MAX_WORD_LEN;
    }

    let buf: [128]u8 = undefined;
    let i: usize = 0;
    while (i < n) {
        buf[i] = toLowerAscii(word[i]);
        i += 1;
    }

    if (n < 3) {
        if (n > out.len) {
            return 0;
        }
        let k: usize = 0;
        while (k < n) {
            out[k] = buf[k];
            k += 1;
        }
        return n;
    }

    // Step 1a
    if (endsWith(buf[0..n], n, "sses")) {
        n -= 2;
    } else if (endsWith(buf[0..n], n, "ies")) {
        n -= 2;
    } else if (endsWith(buf[0..n], n, "ss")) {
        // unchanged
    } else if (endsWith(buf[0..n], n, "s")) {
        n -= 1;
    }

    // Step 1b
    let step1b_extra = false;
    if (endsWith(buf[0..n], n, "eed")) {
        const stem_len = n - 3;
        if (measurePrefix(buf[0..n], stem_len) > 0) {
            n -= 1; // eed -> ee
        }
    } else if (endsWith(buf[0..n], n, "ed") && containsVowel(buf[0..n], n - 2)) {
        n -= 2;
        step1b_extra = true;
    } else if (endsWith(buf[0..n], n, "ing") && containsVowel(buf[0..n], n - 3)) {
        n -= 3;
        step1b_extra = true;
    }

    if (step1b_extra) {
        if (endsWith(buf[0..n], n, "at") || endsWith(buf[0..n], n, "bl") || endsWith(buf[0..n], n, "iz")) {
            if (n < MAX_WORD_LEN) {
                buf[n] = 101; // 'e'
                n += 1;
            }
        } else if (endsDoubleConsonant(buf[0..n], n)) {
            const last = buf[n - 1];
            if (last != 108 && last != 115 && last != 122) {
                // not l, s, z
                n -= 1;
            }
        } else if (measurePrefix(buf[0..n], n) == 1 && endsCvc(buf[0..n], n)) {
            if (n < MAX_WORD_LEN) {
                buf[n] = 101; // 'e'
                n += 1;
            }
        }
    }

    // Step 1c
    if (endsWith(buf[0..n], n, "y") && containsVowel(buf[0..n], n - 1)) {
        buf[n - 1] = 105; // 'i'
    }

    // Step 2
    n = tryStep2(buf[0..MAX_WORD_LEN], n);

    // Step 3
    n = tryStep3(buf[0..MAX_WORD_LEN], n);

    // Step 4
    n = tryStep4(buf[0..MAX_WORD_LEN], n);

    // Step 5a
    if (n > 0 && buf[n - 1] == 101) {
        const m = measurePrefix(buf[0..n], n - 1);
        if (m > 1 || (m == 1 && !endsCvc(buf[0..n], n - 1))) {
            n -= 1;
        }
    }

    // Step 5b
    if (n > 1 && buf[n - 1] == 108 && endsDoubleConsonant(buf[0..n], n) && measurePrefix(buf[0..n], n) > 1) {
        n -= 1;
    }

    if (n > out.len) {
        return 0;
    }
    let k: usize = 0;
    while (k < n) {
        out[k] = buf[k];
        k += 1;
    }
    return n;
}

/// Returns true if `a` and `b` reduce to the same non-empty Porter stem.
pub fn sameStem(a: string, b: string) -> bool {
    let sa: [128]u8 = undefined;
    let sb: [128]u8 = undefined;
    const la = stem(a, sa[0..128]);
    const lb = stem(b, sb[0..128]);
    if (la == 0 || la != lb) {
        return false;
    }
    let i: usize = 0;
    while (i < la) {
        if (sa[i] != sb[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}
