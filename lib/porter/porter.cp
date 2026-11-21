// Source: Absorbed from /data/dev/bun/src/ (Martin Porter 1980 Zero-Alloc English Porter Stemmer)

/// Maximum supported input word length in bytes for zero-alloc stemming.
pub const PORTER_MAX_WORD_LEN: usize = 256;

fn isConsonant(buf: []const u8, idx: usize): bool {
    const ch = buf[idx];
    if (ch == 0x61 || ch == 0x65 || ch == 0x69 || ch == 0x6F || ch == 0x75) {
        return false; // a, e, i, o, u
    }
    if (ch == 0x79) {
        // 'y': consonant at start of word, or if preceded by a vowel
        if (idx == 0) {
            return true;
        }
        return !isConsonant(buf, idx - 1);
    }
    return true;
}

/// Computes the Porter `[C](VC)^m[V]` measure `m` for lowercase ASCII `stem`.
/// Properly handles contextual `'y'` (vowel when preceded by a consonant).
pub fn porterMeasure(stem: []const u8): usize {
    let i: usize = 0;
    const n = stem.len;

    // Skip leading consonants [C]
    while (i < n && isConsonant(stem, i)) {
        i += 1;
    }

    let m: usize = 0;
    while (i < n) {
        // Consume V+
        while (i < n && !isConsonant(stem, i)) {
            i += 1;
        }
        if (i >= n) {
            break;
        }
        // Consume C+
        while (i < n && isConsonant(stem, i)) {
            i += 1;
        }
        m += 1;
    }

    return m;
}

fn containsVowel(stem: []const u8): bool {
    let i: usize = 0;
    while (i < stem.len) {
        if (!isConsonant(stem, i)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn endsDoubleConsonant(stem: []const u8): bool {
    if (stem.len < 2) {
        return false;
    }
    const last = stem.len - 1;
    return stem[last] == stem[last - 1] && isConsonant(stem, last);
}

fn endsCvc(stem: []const u8): bool {
    if (stem.len < 3) {
        return false;
    }
    const last = stem.len - 1;
    const c = stem[last];
    if (c == 0x77 || c == 0x78 || c == 0x79) {
        return false; // w, x, y excluded by Porter's *o condition
    }
    return isConsonant(stem, last) && !isConsonant(stem, last - 1) && isConsonant(stem, last - 2);
}

fn endsWith(buf: []const u8, len: usize, suffix: []const u8): bool {
    if (len < suffix.len) {
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

fn replaceSuffix(buf: []u8, len: usize, suffix_len: usize, repl: []const u8): usize {
    const stem_len = len - suffix_len;
    let i: usize = 0;
    while (i < repl.len) {
        buf[stem_len + i] = repl[i];
        i += 1;
    }
    return stem_len + repl.len;
}

fn tryStep2Rule(buf: []u8, len: usize, suffix: []const u8, repl: []const u8): usize? {
    if (!endsWith(buf[0..], len, suffix)) {
        return null;
    }
    const stem_len = len - suffix.len;
    if (porterMeasure(buf[0..stem_len]) > 0) {
        return replaceSuffix(buf, len, suffix.len, repl);
    }
    return len;
}

fn step2(buf: []u8, len: usize): usize {
    if (len < 4) {
        return len;
    }
    // Dispatch on penultimate character (standard Porter optimization, longest suffix first)
    const penult = buf[len - 2];
    switch (penult) {
        0x61 => { // 'a'
            if (r := tryStep2Rule(buf, len, "ational", "ate")) { return r; }
            if (r := tryStep2Rule(buf, len, "tional", "tion")) { return r; }
        },
        0x63 => { // 'c'
            if (r := tryStep2Rule(buf, len, "enci", "ence")) { return r; }
            if (r := tryStep2Rule(buf, len, "anci", "ance")) { return r; }
        },
        0x65 => { // 'e'
            if (r := tryStep2Rule(buf, len, "izer", "ize")) { return r; }
        },
        0x6C => { // 'l'
            if (r := tryStep2Rule(buf, len, "abli", "able")) { return r; }
            if (r := tryStep2Rule(buf, len, "alli", "al")) { return r; }
            if (r := tryStep2Rule(buf, len, "entli", "ent")) { return r; }
            if (r := tryStep2Rule(buf, len, "eli", "e")) { return r; }
            if (r := tryStep2Rule(buf, len, "ousli", "ous")) { return r; }
        },
        0x6F => { // 'o'
            if (r := tryStep2Rule(buf, len, "ization", "ize")) { return r; }
            if (r := tryStep2Rule(buf, len, "ation", "ate")) { return r; }
            if (r := tryStep2Rule(buf, len, "ator", "ate")) { return r; }
        },
        0x73 => { // 's'
            if (r := tryStep2Rule(buf, len, "alism", "al")) { return r; }
            if (r := tryStep2Rule(buf, len, "iveness", "ive")) { return r; }
            if (r := tryStep2Rule(buf, len, "fulness", "ful")) { return r; }
            if (r := tryStep2Rule(buf, len, "ousness", "ous")) { return r; }
        },
        0x74 => { // 't'
            if (r := tryStep2Rule(buf, len, "aliti", "al")) { return r; }
            if (r := tryStep2Rule(buf, len, "iviti", "ive")) { return r; }
            if (r := tryStep2Rule(buf, len, "biliti", "ble")) { return r; }
        },
        else => {},
    }
    return len;
}

fn step3(buf: []u8, len: usize): usize {
    if (len < 4) {
        return len;
    }
    const last = buf[len - 1];
    switch (last) {
        0x65 => { // 'e'
            if (r := tryStep2Rule(buf, len, "icate", "ic")) { return r; }
            if (r := tryStep2Rule(buf, len, "ative", "")) { return r; }
            if (r := tryStep2Rule(buf, len, "alize", "al")) { return r; }
        },
        0x69 => { // 'i'
            if (r := tryStep2Rule(buf, len, "iciti", "ic")) { return r; }
        },
        0x6C => { // 'l'
            if (r := tryStep2Rule(buf, len, "ical", "ic")) { return r; }
            if (r := tryStep2Rule(buf, len, "ful", "")) { return r; }
        },
        0x73 => { // 's'
            if (r := tryStep2Rule(buf, len, "ness", "")) { return r; }
        },
        else => {},
    }
    return len;
}

fn tryStep4Strip(buf: []const u8, len: usize, suffix: []const u8): usize? {
    if (!endsWith(buf, len, suffix)) {
        return null;
    }
    const stem_len = len - suffix.len;
    if (porterMeasure(buf[0..stem_len]) > 1) {
        return stem_len;
    }
    return len;
}

fn step4(buf: []const u8, len: usize): usize {
    if (len < 3) {
        return len;
    }
    const penult = buf[len - 2];
    switch (penult) {
        0x61 => { // 'a'
            if (r := tryStep4Strip(buf, len, "al")) { return r; }
        },
        0x63 => { // 'c'
            if (r := tryStep4Strip(buf, len, "ance")) { return r; }
            if (r := tryStep4Strip(buf, len, "ence")) { return r; }
        },
        0x65 => { // 'e'
            if (r := tryStep4Strip(buf, len, "er")) { return r; }
        },
        0x69 => { // 'i'
            if (r := tryStep4Strip(buf, len, "ic")) { return r; }
        },
        0x6C => { // 'l'
            if (r := tryStep4Strip(buf, len, "able")) { return r; }
            if (r := tryStep4Strip(buf, len, "ible")) { return r; }
        },
        0x6E => { // 'n'
            if (r := tryStep4Strip(buf, len, "ant")) { return r; }
            if (r := tryStep4Strip(buf, len, "ement")) { return r; }
            if (r := tryStep4Strip(buf, len, "ment")) { return r; }
            if (r := tryStep4Strip(buf, len, "ent")) { return r; }
        },
        0x6F => { // 'o'
            if (endsWith(buf, len, "ion")) {
                const stem_len = len - 3;
                if (stem_len > 0 && (buf[stem_len - 1] == 0x73 || buf[stem_len - 1] == 0x74)) {
                    if (porterMeasure(buf[0..stem_len]) > 1) {
                        return stem_len;
                    }
                }
                return len;
            }
            if (r := tryStep4Strip(buf, len, "ou")) { return r; }
        },
        0x73 => { // 's'
            if (r := tryStep4Strip(buf, len, "ism")) { return r; }
        },
        0x74 => { // 't'
            if (r := tryStep4Strip(buf, len, "ate")) { return r; }
            if (r := tryStep4Strip(buf, len, "iti")) { return r; }
        },
        0x75 => { // 'u'
            if (r := tryStep4Strip(buf, len, "ous")) { return r; }
        },
        0x76 => { // 'v'
            if (r := tryStep4Strip(buf, len, "ive")) { return r; }
        },
        0x7A => { // 'z'
            if (r := tryStep4Strip(buf, len, "ize")) { return r; }
        },
        else => {},
    }
    return len;
}

/// Stems an ASCII alphabetic English `word` using Martin Porter's 1980 algorithm
/// into caller-provided `out` with zero heap allocation.
/// - Lowercases input; words of length `<= 2` are returned lowercased unchanged.
/// - Strictly rejects empty input, non-ASCII-letter bytes, words longer than
///   `PORTER_MAX_WORD_LEN (256)`, or insufficient `out` capacity by returning `null`.
pub fn porterStem(word: []const u8, out: []u8): usize? {
    if (word.len == 0 || word.len > PORTER_MAX_WORD_LEN) {
        return null;
    }

    // Extra 4 bytes of headroom in stack buffer for temporary Step 1b 'e' append.
    let buf: [260]u8 = [0; 260];
    let len: usize = word.len;
    let i: usize = 0;
    while (i < len) {
        const ch = word[i];
        if (ch >= 0x41 && ch <= 0x5A) {
            buf[i] = ch + 32;
        } else if (ch >= 0x61 && ch <= 0x7A) {
            buf[i] = ch;
        } else {
            return null;
        }
        i += 1;
    }

    if (len > 2) {
        // Step 1a
        if (endsWith(buf[0..], len, "sses")) {
            len -= 2;
        } else if (endsWith(buf[0..], len, "ies")) {
            len -= 2;
        } else if (!endsWith(buf[0..], len, "ss") && buf[len - 1] == 0x73) {
            len -= 1;
        }

        // Step 1b
        let step1b_extra = false;
        if (endsWith(buf[0..], len, "eed")) {
            if (porterMeasure(buf[0..(len - 3)]) > 0) {
                len -= 1;
            }
        } else if (endsWith(buf[0..], len, "ed") && containsVowel(buf[0..(len - 2)])) {
            len -= 2;
            step1b_extra = true;
        } else if (endsWith(buf[0..], len, "ing") && containsVowel(buf[0..(len - 3)])) {
            len -= 3;
            step1b_extra = true;
        }

        if (step1b_extra) {
            if (
                endsWith(buf[0..], len, "at") ||
                endsWith(buf[0..], len, "bl") ||
                endsWith(buf[0..], len, "iz")
            ) {
                buf[len] = 0x65; // 'e'
                len += 1;
            } else if (
                endsDoubleConsonant(buf[0..len]) &&
                buf[len - 1] != 0x6C && buf[len - 1] != 0x73 && buf[len - 1] != 0x7A
            ) {
                len -= 1;
            } else if (porterMeasure(buf[0..len]) == 1 && endsCvc(buf[0..len])) {
                buf[len] = 0x65; // 'e'
                len += 1;
            }
        }

        // Step 1c
        if (len > 1 && buf[len - 1] == 0x79 && containsVowel(buf[0..(len - 1)])) {
            buf[len - 1] = 0x69; // 'i'
        }

        // Step 2
        len = step2(buf[0..], len);

        // Step 3
        len = step3(buf[0..], len);

        // Step 4
        len = step4(buf[0..], len);

        // Step 5a
        if (len > 1 && buf[len - 1] == 0x65) {
            const m = porterMeasure(buf[0..(len - 1)]);
            if (m > 1 || (m == 1 && !endsCvc(buf[0..(len - 1)]))) {
                len -= 1;
            }
        }

        // Step 5b
        if (len > 1 && buf[len - 1] == 0x6C && endsDoubleConsonant(buf[0..len]) && porterMeasure(buf[0..len]) > 1) {
            len -= 1;
        }
    }

    if (out.len < len) {
        return null;
    }
    i = 0;
    while (i < len) {
        out[i] = buf[i];
        i += 1;
    }
    return len;
}

/// Returns `true` if `a` and `b` both produce valid and identical Porter stems (zero-alloc).
pub fn porterStemEquals(a: []const u8, b: []const u8): bool {
    let sa: [PORTER_MAX_WORD_LEN]u8 = [0; PORTER_MAX_WORD_LEN];
    let sb: [PORTER_MAX_WORD_LEN]u8 = [0; PORTER_MAX_WORD_LEN];
    const la = porterStem(a, sa[0..]) ?? return false;
    const lb = porterStem(b, sb[0..]) ?? return false;
    if (la != lb) {
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
