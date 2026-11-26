// packages/@cap/icu-normalizer/src/index.cp
// Zero-allocation UAX #15 Unicode NFC / NFD / NFKC / NFKD normalizer in pure CP.
// Clean-room MIT implementation (ICU4X icu_normalizer parity).

pub const HANGUL_SBASE: u32 = 0xAC00;
pub const HANGUL_LBASE: u32 = 0x1100;
pub const HANGUL_VBASE: u32 = 0x1161;
pub const HANGUL_TBASE: u32 = 0x11A7;
pub const HANGUL_LCOUNT: u32 = 19;
pub const HANGUL_VCOUNT: u32 = 21;
pub const HANGUL_TCOUNT: u32 = 28;
pub const HANGUL_NCOUNT: u32 = 588;
pub const HANGUL_SCOUNT: u32 = 11172;

struct InormUtf8Dec {
    cp: u32,
    next: usize,
}

fn inormIsPureAscii(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] >= (0x80 as u8)) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn inormDecodeUtf8(s: string, pos: usize): InormUtf8Dec {
    if (pos >= s.len) {
        return InormUtf8Dec { cp: 0 as u32, next: pos };
    }
    const b0 = s[pos];
    if (b0 < (0x80 as u8)) {
        return InormUtf8Dec { cp: b0 as u32, next: pos + 1 };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && pos + 1 < s.len) {
        const b1 = s[pos + 1];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp = (((b0 & (0x1F as u8)) as u32) << (6 as u5))
                | ((b1 & (0x3F as u8)) as u32);
            return InormUtf8Dec { cp: cp, next: pos + 2 };
        }
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && pos + 2 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8) && (b2 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b2 & (0x3F as u8)) as u32);
            return InormUtf8Dec { cp: cp, next: pos + 3 };
        }
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && pos + 3 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        const b3 = s[pos + 3];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)
            && (b2 & (0xC0 as u8)) == (0x80 as u8)
            && (b3 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (12 as u5))
                | (((b2 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b3 & (0x3F as u8)) as u32);
            return InormUtf8Dec { cp: cp, next: pos + 4 };
        }
    }
    return InormUtf8Dec { cp: 0xFFFD as u32, next: pos + 1 };
}

fn inormUtf8Len(cp: u32): usize {
    if (cp < (0x80 as u32)) {
        return 1;
    }
    if (cp < (0x800 as u32)) {
        return 2;
    }
    if (cp < (0x10000 as u32)) {
        return 3;
    }
    return 4;
}

fn inormEncodeUtf8(cp: u32, out: []u8, pos: usize): usize {
    const need = inormUtf8Len(cp);
    if (pos + need > out.len) {
        return 0;
    }
    if (need == 1) {
        out[pos] = cp as u8;
        return 1;
    }
    if (need == 2) {
        out[pos] = (0xC0 as u8) | ((cp >> (6 as u5)) as u8);
        out[pos + 1] = (0x80 as u8) | ((cp & (0x3F as u32)) as u8);
        return 2;
    }
    if (need == 3) {
        out[pos] = (0xE0 as u8) | ((cp >> (12 as u5)) as u8);
        out[pos + 1] = (0x80 as u8) | (((cp >> (6 as u5)) & (0x3F as u32)) as u8);
        out[pos + 2] = (0x80 as u8) | ((cp & (0x3F as u32)) as u8);
        return 3;
    }
    out[pos] = (0xF0 as u8) | ((cp >> (18 as u5)) as u8);
    out[pos + 1] = (0x80 as u8) | (((cp >> (12 as u5)) & (0x3F as u32)) as u8);
    out[pos + 2] = (0x80 as u8) | (((cp >> (6 as u5)) & (0x3F as u32)) as u8);
    out[pos + 3] = (0x80 as u8) | ((cp & (0x3F as u32)) as u8);
    return 4;
}

fn inormBytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Returns the Unicode Canonical Combining Class (CCC) for code point `cp`
/// per UAX #15 / UnicodeData.txt (covering ASCII and Combining Diacritical Marks U+0300..U+036F).
pub fn canonicalCombiningClass(cp: u32): u8 {
    if (cp < (0x0300 as u32)) {
        return 0 as u8;
    }
    if (cp <= (0x0314 as u32)) {
        return 230 as u8;
    }
    if (cp == (0x0315 as u32)) {
        return 232 as u8;
    }
    if (cp <= (0x0319 as u32)) {
        return 220 as u8;
    }
    if (cp == (0x031A as u32)) {
        return 232 as u8;
    }
    if (cp == (0x031B as u32)) {
        return 216 as u8;
    }
    if (cp <= (0x0320 as u32)) {
        return 220 as u8;
    }
    if (cp <= (0x0322 as u32)) {
        return 202 as u8;
    }
    if (cp <= (0x0326 as u32)) {
        return 220 as u8;
    }
    if (cp <= (0x0328 as u32)) {
        return 202 as u8;
    }
    if (cp <= (0x0333 as u32)) {
        return 220 as u8;
    }
    if (cp <= (0x0338 as u32)) {
        return 1 as u8;
    }
    if (cp <= (0x033C as u32)) {
        return 220 as u8;
    }
    if (cp <= (0x0344 as u32)) {
        return 230 as u8;
    }
    if (cp == (0x0345 as u32)) {
        return 240 as u8;
    }
    if (cp == (0x0346 as u32)) {
        return 230 as u8;
    }
    if (cp <= (0x0349 as u32)) {
        return 220 as u8;
    }
    if (cp <= (0x034C as u32)) {
        return 230 as u8;
    }
    if (cp <= (0x034E as u32)) {
        return 220 as u8;
    }
    if (cp >= (0x0350 as u32) && cp <= (0x0352 as u32)) {
        return 230 as u8;
    }
    if (cp >= (0x0353 as u32) && cp <= (0x0356 as u32)) {
        return 220 as u8;
    }
    if (cp == (0x0357 as u32)) {
        return 230 as u8;
    }
    if (cp == (0x0358 as u32)) {
        return 232 as u8;
    }
    if (cp >= (0x0359 as u32) && cp <= (0x035A as u32)) {
        return 220 as u8;
    }
    if (cp == (0x035B as u32)) {
        return 230 as u8;
    }
    if (cp == (0x035C as u32)) {
        return 233 as u8;
    }
    if (cp >= (0x035D as u32) && cp <= (0x035E as u32)) {
        return 234 as u8;
    }
    if (cp == (0x035F as u32)) {
        return 233 as u8;
    }
    if (cp >= (0x0360 as u32) && cp <= (0x0361 as u32)) {
        return 234 as u8;
    }
    if (cp == (0x0362 as u32)) {
        return 233 as u8;
    }
    if (cp >= (0x0363 as u32) && cp <= (0x036F as u32)) {
        return 230 as u8;
    }
    return 0 as u8;
}

/// Algorithmically decomposes any of the 11,172 Hangul syllables (U+AC00..U+D7A3)
/// into 2 (L, V) or 3 (L, V, T) Jamo code points per Unicode Standard §3.12.
/// Returns 0 if `cp` is not a precomposed Hangul syllable or `out` is too small.
pub fn hangulDecompose(cp: u32, out: []u32): usize {
    if (cp < HANGUL_SBASE || cp >= HANGUL_SBASE + HANGUL_SCOUNT) {
        return 0;
    }
    const s_idx = cp - HANGUL_SBASE;
    const l = HANGUL_LBASE + (s_idx / HANGUL_NCOUNT);
    const v = HANGUL_VBASE + ((s_idx % HANGUL_NCOUNT) / HANGUL_TCOUNT);
    const t = s_idx % HANGUL_TCOUNT;
    if (t == (0 as u32)) {
        if (out.len < 2) {
            return 0;
        }
        out[0] = l;
        out[1] = v;
        return 2;
    }
    if (out.len < 3) {
        return 0;
    }
    out[0] = l;
    out[1] = v;
    out[2] = HANGUL_TBASE + t;
    return 3;
}

/// Algorithmically composes Hangul Jamo (L, V, optional T; pass `t = 0` for LV)
/// into a precomposed Hangul syllable per Unicode Standard §3.12.
/// Returns 0 if the input Jamo values are out of range.
pub fn hangulCompose(l: u32, v: u32, t: u32): u32 {
    if (l < HANGUL_LBASE || l >= HANGUL_LBASE + HANGUL_LCOUNT) {
        return 0 as u32;
    }
    if (v < HANGUL_VBASE || v >= HANGUL_VBASE + HANGUL_VCOUNT) {
        return 0 as u32;
    }
    const l_idx = l - HANGUL_LBASE;
    const v_idx = v - HANGUL_VBASE;
    if (t == (0 as u32)) {
        return HANGUL_SBASE + (l_idx * HANGUL_VCOUNT + v_idx) * HANGUL_TCOUNT;
    }
    if (t <= HANGUL_TBASE || t >= HANGUL_TBASE + HANGUL_TCOUNT) {
        return 0 as u32;
    }
    return HANGUL_SBASE + (l_idx * HANGUL_VCOUNT + v_idx) * HANGUL_TCOUNT + (t - HANGUL_TBASE);
}

fn inormHangulComposePair(first: u32, second: u32): u32 {
    // Case 1: L + V -> LV
    if (first >= HANGUL_LBASE && first < HANGUL_LBASE + HANGUL_LCOUNT
        && second >= HANGUL_VBASE && second < HANGUL_VBASE + HANGUL_VCOUNT) {
        return hangulCompose(first, second, 0 as u32);
    }
    // Case 2: LV + T -> LVT
    if (first >= HANGUL_SBASE && first < HANGUL_SBASE + HANGUL_SCOUNT
        && second > HANGUL_TBASE && second < HANGUL_TBASE + HANGUL_TCOUNT) {
        const s_idx = first - HANGUL_SBASE;
        if ((s_idx % HANGUL_TCOUNT) == (0 as u32)) {
            return first + (second - HANGUL_TBASE);
        }
    }
    return 0 as u32;
}

fn inormWrite2(out: []u32, a: u32, b: u32): usize {
    if (out.len < 2) {
        return 0;
    }
    out[0] = a;
    out[1] = b;
    return 2;
}

/// Canonical decomposition for Latin-1 Supplement (U+00C0..U+00FF) and common
/// Latin Extended-A precomposed characters. Returns 2 on match, 0 otherwise.
pub fn latinCanonicalDecompose(cp: u32, out: []u32): usize {
    // Uppercase Latin-1 accented letters
    if (cp == (0x00C0 as u32)) { return inormWrite2(out, 'A' as u32, 0x0300 as u32); }
    if (cp == (0x00C1 as u32)) { return inormWrite2(out, 'A' as u32, 0x0301 as u32); }
    if (cp == (0x00C2 as u32)) { return inormWrite2(out, 'A' as u32, 0x0302 as u32); }
    if (cp == (0x00C3 as u32)) { return inormWrite2(out, 'A' as u32, 0x0303 as u32); }
    if (cp == (0x00C4 as u32)) { return inormWrite2(out, 'A' as u32, 0x0308 as u32); }
    if (cp == (0x00C5 as u32) || cp == (0x212B as u32)) { return inormWrite2(out, 'A' as u32, 0x030A as u32); }
    if (cp == (0x00C7 as u32)) { return inormWrite2(out, 'C' as u32, 0x0327 as u32); }
    if (cp == (0x00C8 as u32)) { return inormWrite2(out, 'E' as u32, 0x0300 as u32); }
    if (cp == (0x00C9 as u32)) { return inormWrite2(out, 'E' as u32, 0x0301 as u32); }
    if (cp == (0x00CA as u32)) { return inormWrite2(out, 'E' as u32, 0x0302 as u32); }
    if (cp == (0x00CB as u32)) { return inormWrite2(out, 'E' as u32, 0x0308 as u32); }
    if (cp == (0x00CC as u32)) { return inormWrite2(out, 'I' as u32, 0x0300 as u32); }
    if (cp == (0x00CD as u32)) { return inormWrite2(out, 'I' as u32, 0x0301 as u32); }
    if (cp == (0x00CE as u32)) { return inormWrite2(out, 'I' as u32, 0x0302 as u32); }
    if (cp == (0x00CF as u32)) { return inormWrite2(out, 'I' as u32, 0x0308 as u32); }
    if (cp == (0x00D1 as u32)) { return inormWrite2(out, 'N' as u32, 0x0303 as u32); }
    if (cp == (0x00D2 as u32)) { return inormWrite2(out, 'O' as u32, 0x0300 as u32); }
    if (cp == (0x00D3 as u32)) { return inormWrite2(out, 'O' as u32, 0x0301 as u32); }
    if (cp == (0x00D4 as u32)) { return inormWrite2(out, 'O' as u32, 0x0302 as u32); }
    if (cp == (0x00D5 as u32)) { return inormWrite2(out, 'O' as u32, 0x0303 as u32); }
    if (cp == (0x00D6 as u32)) { return inormWrite2(out, 'O' as u32, 0x0308 as u32); }
    if (cp == (0x00D9 as u32)) { return inormWrite2(out, 'U' as u32, 0x0300 as u32); }
    if (cp == (0x00DA as u32)) { return inormWrite2(out, 'U' as u32, 0x0301 as u32); }
    if (cp == (0x00DB as u32)) { return inormWrite2(out, 'U' as u32, 0x0302 as u32); }
    if (cp == (0x00DC as u32)) { return inormWrite2(out, 'U' as u32, 0x0308 as u32); }
    if (cp == (0x00DD as u32)) { return inormWrite2(out, 'Y' as u32, 0x0301 as u32); }

    // Lowercase Latin-1 accented letters
    if (cp == (0x00E0 as u32)) { return inormWrite2(out, 'a' as u32, 0x0300 as u32); }
    if (cp == (0x00E1 as u32)) { return inormWrite2(out, 'a' as u32, 0x0301 as u32); }
    if (cp == (0x00E2 as u32)) { return inormWrite2(out, 'a' as u32, 0x0302 as u32); }
    if (cp == (0x00E3 as u32)) { return inormWrite2(out, 'a' as u32, 0x0303 as u32); }
    if (cp == (0x00E4 as u32)) { return inormWrite2(out, 'a' as u32, 0x0308 as u32); }
    if (cp == (0x00E5 as u32)) { return inormWrite2(out, 'a' as u32, 0x030A as u32); }
    if (cp == (0x00E7 as u32)) { return inormWrite2(out, 'c' as u32, 0x0327 as u32); }
    if (cp == (0x00E8 as u32)) { return inormWrite2(out, 'e' as u32, 0x0300 as u32); }
    if (cp == (0x00E9 as u32)) { return inormWrite2(out, 'e' as u32, 0x0301 as u32); }
    if (cp == (0x00EA as u32)) { return inormWrite2(out, 'e' as u32, 0x0302 as u32); }
    if (cp == (0x00EB as u32)) { return inormWrite2(out, 'e' as u32, 0x0308 as u32); }
    if (cp == (0x00EC as u32)) { return inormWrite2(out, 'i' as u32, 0x0300 as u32); }
    if (cp == (0x00ED as u32)) { return inormWrite2(out, 'i' as u32, 0x0301 as u32); }
    if (cp == (0x00EE as u32)) { return inormWrite2(out, 'i' as u32, 0x0302 as u32); }
    if (cp == (0x00EF as u32)) { return inormWrite2(out, 'i' as u32, 0x0308 as u32); }
    if (cp == (0x00F1 as u32)) { return inormWrite2(out, 'n' as u32, 0x0303 as u32); }
    if (cp == (0x00F2 as u32)) { return inormWrite2(out, 'o' as u32, 0x0300 as u32); }
    if (cp == (0x00F3 as u32)) { return inormWrite2(out, 'o' as u32, 0x0301 as u32); }
    if (cp == (0x00F4 as u32)) { return inormWrite2(out, 'o' as u32, 0x0302 as u32); }
    if (cp == (0x00F5 as u32)) { return inormWrite2(out, 'o' as u32, 0x0303 as u32); }
    if (cp == (0x00F6 as u32)) { return inormWrite2(out, 'o' as u32, 0x0308 as u32); }
    if (cp == (0x00F9 as u32)) { return inormWrite2(out, 'u' as u32, 0x0300 as u32); }
    if (cp == (0x00FA as u32)) { return inormWrite2(out, 'u' as u32, 0x0301 as u32); }
    if (cp == (0x00FB as u32)) { return inormWrite2(out, 'u' as u32, 0x0302 as u32); }
    if (cp == (0x00FC as u32)) { return inormWrite2(out, 'u' as u32, 0x0308 as u32); }
    if (cp == (0x00FD as u32)) { return inormWrite2(out, 'y' as u32, 0x0301 as u32); }
    if (cp == (0x00FF as u32)) { return inormWrite2(out, 'y' as u32, 0x0308 as u32); }

    // Common Latin Extended-A
    if (cp == (0x0100 as u32)) { return inormWrite2(out, 'A' as u32, 0x0304 as u32); }
    if (cp == (0x0101 as u32)) { return inormWrite2(out, 'a' as u32, 0x0304 as u32); }
    if (cp == (0x0104 as u32)) { return inormWrite2(out, 'A' as u32, 0x0328 as u32); }
    if (cp == (0x0105 as u32)) { return inormWrite2(out, 'a' as u32, 0x0328 as u32); }
    if (cp == (0x0106 as u32)) { return inormWrite2(out, 'C' as u32, 0x0301 as u32); }
    if (cp == (0x0107 as u32)) { return inormWrite2(out, 'c' as u32, 0x0301 as u32); }
    if (cp == (0x010C as u32)) { return inormWrite2(out, 'C' as u32, 0x030C as u32); }
    if (cp == (0x010D as u32)) { return inormWrite2(out, 'c' as u32, 0x030C as u32); }
    if (cp == (0x0112 as u32)) { return inormWrite2(out, 'E' as u32, 0x0304 as u32); }
    if (cp == (0x0113 as u32)) { return inormWrite2(out, 'e' as u32, 0x0304 as u32); }
    if (cp == (0x0143 as u32)) { return inormWrite2(out, 'N' as u32, 0x0301 as u32); }
    if (cp == (0x0144 as u32)) { return inormWrite2(out, 'n' as u32, 0x0301 as u32); }
    if (cp == (0x015A as u32)) { return inormWrite2(out, 'S' as u32, 0x0301 as u32); }
    if (cp == (0x015B as u32)) { return inormWrite2(out, 's' as u32, 0x0301 as u32); }
    if (cp == (0x0160 as u32)) { return inormWrite2(out, 'S' as u32, 0x030C as u32); }
    if (cp == (0x0161 as u32)) { return inormWrite2(out, 's' as u32, 0x030C as u32); }
    if (cp == (0x016A as u32)) { return inormWrite2(out, 'U' as u32, 0x0304 as u32); }
    if (cp == (0x016B as u32)) { return inormWrite2(out, 'u' as u32, 0x0304 as u32); }
    if (cp == (0x017D as u32)) { return inormWrite2(out, 'Z' as u32, 0x030C as u32); }
    if (cp == (0x017E as u32)) { return inormWrite2(out, 'z' as u32, 0x030C as u32); }

    return 0;
}

/// Canonical composition for Latin-1 Supplement & common Latin Extended-A pairs.
/// Returns the composed code point, or 0 if `(base, comb)` does not canonically compose.
pub fn latinCanonicalCompose(base: u32, comb: u32): u32 {
    if (base == ('A' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00C0 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00C1 as u32; }
        if (comb == (0x0302 as u32)) { return 0x00C2 as u32; }
        if (comb == (0x0303 as u32)) { return 0x00C3 as u32; }
        if (comb == (0x0308 as u32)) { return 0x00C4 as u32; }
        if (comb == (0x030A as u32)) { return 0x00C5 as u32; }
        if (comb == (0x0304 as u32)) { return 0x0100 as u32; }
        if (comb == (0x0328 as u32)) { return 0x0104 as u32; }
    }
    if (base == ('a' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00E0 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00E1 as u32; }
        if (comb == (0x0302 as u32)) { return 0x00E2 as u32; }
        if (comb == (0x0303 as u32)) { return 0x00E3 as u32; }
        if (comb == (0x0308 as u32)) { return 0x00E4 as u32; }
        if (comb == (0x030A as u32)) { return 0x00E5 as u32; }
        if (comb == (0x0304 as u32)) { return 0x0101 as u32; }
        if (comb == (0x0328 as u32)) { return 0x0105 as u32; }
    }
    if (base == ('C' as u32)) {
        if (comb == (0x0327 as u32)) { return 0x00C7 as u32; }
        if (comb == (0x0301 as u32)) { return 0x0106 as u32; }
        if (comb == (0x030C as u32)) { return 0x010C as u32; }
    }
    if (base == ('c' as u32)) {
        if (comb == (0x0327 as u32)) { return 0x00E7 as u32; }
        if (comb == (0x0301 as u32)) { return 0x0107 as u32; }
        if (comb == (0x030C as u32)) { return 0x010D as u32; }
    }
    if (base == ('E' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00C8 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00C9 as u32; }
        if (comb == (0x0302 as u32)) { return 0x00CA as u32; }
        if (comb == (0x0308 as u32)) { return 0x00CB as u32; }
        if (comb == (0x0304 as u32)) { return 0x0112 as u32; }
    }
    if (base == ('e' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00E8 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00E9 as u32; }
        if (comb == (0x0302 as u32)) { return 0x00EA as u32; }
        if (comb == (0x0308 as u32)) { return 0x00EB as u32; }
        if (comb == (0x0304 as u32)) { return 0x0113 as u32; }
    }
    if (base == ('I' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00CC as u32; }
        if (comb == (0x0301 as u32)) { return 0x00CD as u32; }
        if (comb == (0x0302 as u32)) { return 0x00CE as u32; }
        if (comb == (0x0308 as u32)) { return 0x00CF as u32; }
    }
    if (base == ('i' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00EC as u32; }
        if (comb == (0x0301 as u32)) { return 0x00ED as u32; }
        if (comb == (0x0302 as u32)) { return 0x00EE as u32; }
        if (comb == (0x0308 as u32)) { return 0x00EF as u32; }
    }
    if (base == ('N' as u32)) {
        if (comb == (0x0303 as u32)) { return 0x00D1 as u32; }
        if (comb == (0x0301 as u32)) { return 0x0143 as u32; }
    }
    if (base == ('n' as u32)) {
        if (comb == (0x0303 as u32)) { return 0x00F1 as u32; }
        if (comb == (0x0301 as u32)) { return 0x0144 as u32; }
    }
    if (base == ('O' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00D2 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00D3 as u32; }
        if (comb == (0x0302 as u32)) { return 0x00D4 as u32; }
        if (comb == (0x0303 as u32)) { return 0x00D5 as u32; }
        if (comb == (0x0308 as u32)) { return 0x00D6 as u32; }
    }
    if (base == ('o' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00F2 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00F3 as u32; }
        if (comb == (0x0302 as u32)) { return 0x00F4 as u32; }
        if (comb == (0x0303 as u32)) { return 0x00F5 as u32; }
        if (comb == (0x0308 as u32)) { return 0x00F6 as u32; }
    }
    if (base == ('S' as u32)) {
        if (comb == (0x0301 as u32)) { return 0x015A as u32; }
        if (comb == (0x030C as u32)) { return 0x0160 as u32; }
    }
    if (base == ('s' as u32)) {
        if (comb == (0x0301 as u32)) { return 0x015B as u32; }
        if (comb == (0x030C as u32)) { return 0x0161 as u32; }
    }
    if (base == ('U' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00D9 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00DA as u32; }
        if (comb == (0x0302 as u32)) { return 0x00DB as u32; }
        if (comb == (0x0308 as u32)) { return 0x00DC as u32; }
        if (comb == (0x0304 as u32)) { return 0x016A as u32; }
    }
    if (base == ('u' as u32)) {
        if (comb == (0x0300 as u32)) { return 0x00F9 as u32; }
        if (comb == (0x0301 as u32)) { return 0x00FA as u32; }
        if (comb == (0x0302 as u32)) { return 0x00FB as u32; }
        if (comb == (0x0308 as u32)) { return 0x00FC as u32; }
        if (comb == (0x0304 as u32)) { return 0x016B as u32; }
    }
    if (base == ('Y' as u32) && comb == (0x0301 as u32)) { return 0x00DD as u32; }
    if (base == ('y' as u32)) {
        if (comb == (0x0301 as u32)) { return 0x00FD as u32; }
        if (comb == (0x0308 as u32)) { return 0x00FF as u32; }
    }
    if (base == ('Z' as u32) && comb == (0x030C as u32)) { return 0x017D as u32; }
    if (base == ('z' as u32) && comb == (0x030C as u32)) { return 0x017E as u32; }
    return 0 as u32;
}

/// Compatibility decomposition (NFKD/NFKC) for fullwidth ASCII (`U+FF01..U+FF5E`),
/// ideographic/NBSP spaces (`U+3000`, `U+00A0`), superscripts (`¹`/`²`/`³`),
/// and typographic ligatures (`ﬀ`/`ﬁ`/`ﬂ`/`ﬃ`/`ﬄ`). Returns count written, or 0.
pub fn compatibilityDecompose(cp: u32, out: []u32): usize {
    if (cp >= (0xFF01 as u32) && cp <= (0xFF5E as u32)) {
        if (out.len < 1) { return 0; }
        out[0] = (cp - (0xFF01 as u32)) + (0x0021 as u32);
        return 1;
    }
    if (cp == (0x3000 as u32) || cp == (0x00A0 as u32)) {
        if (out.len < 1) { return 0; }
        out[0] = 0x0020 as u32;
        return 1;
    }
    if (cp == (0x00B9 as u32)) {
        if (out.len < 1) { return 0; }
        out[0] = '1' as u32;
        return 1;
    }
    if (cp == (0x00B2 as u32)) {
        if (out.len < 1) { return 0; }
        out[0] = '2' as u32;
        return 1;
    }
    if (cp == (0x00B3 as u32)) {
        if (out.len < 1) { return 0; }
        out[0] = '3' as u32;
        return 1;
    }
    if (cp == (0xFB00 as u32)) {
        if (out.len < 2) { return 0; }
        out[0] = 'f' as u32;
        out[1] = 'f' as u32;
        return 2;
    }
    if (cp == (0xFB01 as u32)) {
        if (out.len < 2) { return 0; }
        out[0] = 'f' as u32;
        out[1] = 'i' as u32;
        return 2;
    }
    if (cp == (0xFB02 as u32)) {
        if (out.len < 2) { return 0; }
        out[0] = 'f' as u32;
        out[1] = 'l' as u32;
        return 2;
    }
    if (cp == (0xFB03 as u32)) {
        if (out.len < 3) { return 0; }
        out[0] = 'f' as u32;
        out[1] = 'f' as u32;
        out[2] = 'i' as u32;
        return 3;
    }
    if (cp == (0xFB04 as u32)) {
        if (out.len < 3) { return 0; }
        out[0] = 'f' as u32;
        out[1] = 'f' as u32;
        out[2] = 'l' as u32;
        return 3;
    }
    return 0;
}

/// UAX #15 Canonical Ordering Algorithm: stably sorts adjacent runs of non-zero CCC
/// combining marks in ascending order of `canonicalCombiningClass`.
pub fn canonicalOrder(cps: []u32) {
    if (cps.len < 2) {
        return;
    }
    let i: usize = 1;
    while (i < cps.len) {
        const cur = cps[i];
        const cur_ccc = canonicalCombiningClass(cur);
        if (cur_ccc > (0 as u8)) {
            let j = i;
            while (j > 0) {
                const prev_ccc = canonicalCombiningClass(cps[j - 1]);
                if (prev_ccc > cur_ccc) {
                    cps[j] = cps[j - 1];
                    j = j - 1;
                } else {
                    break;
                }
            }
            cps[j] = cur;
        }
        i = i + 1;
    }
}

fn inormComposeInPlace(cps: []u32, len: usize): usize {
    let write_pos: usize = 0;
    let starter_idx: usize = 0;
    let has_starter = false;
    let last_ccc: u8 = 0 as u8;

    let i: usize = 0;
    while (i < len) {
        const ch = cps[i];
        const ch_ccc = canonicalCombiningClass(ch);
        let composed: u32 = 0 as u32;

        if (has_starter && (last_ccc == (0 as u8) || last_ccc < ch_ccc)) {
            const starter = cps[starter_idx];
            if (ch_ccc == (0 as u8) && last_ccc == (0 as u8)) {
                composed = inormHangulComposePair(starter, ch);
            }
            if (composed == (0 as u32)) {
                composed = latinCanonicalCompose(starter, ch);
            }
        }

        if (composed != (0 as u32)) {
            cps[starter_idx] = composed;
        } else {
            if (ch_ccc == (0 as u8)) {
                starter_idx = write_pos;
                has_starter = true;
                last_ccc = 0 as u8;
            } else {
                last_ccc = ch_ccc;
            }
            cps[write_pos] = ch;
            write_pos = write_pos + 1;
        }
        i = i + 1;
    }
    return write_pos;
}

fn inormNormalizeCore(input: string, compat: bool, compose: bool, out: []u8): usize {
    if (input.len == 0 || out.len == 0) {
        return 0;
    }
    // Fast path for pure ASCII strings (invariant under NFC, NFD, NFKC, NFKD)
    if (inormIsPureAscii(input)) {
        let take = input.len;
        if (out.len < take) {
            take = out.len;
        }
        let i: usize = 0;
        while (i < take) {
            out[i] = input[i];
            i = i + 1;
        }
        return take;
    }

    let cps: [512]u32 = [0; 512];
    let cp_len: usize = 0;
    let tmp: [4]u32 = [0; 4];

    let pos: usize = 0;
    while (pos < input.len && cp_len < 512) {
        const decoded = inormDecodeUtf8(input, pos);
        pos = decoded.next;
        const cp = decoded.cp;

        let d_len: usize = 0;
        if (compat) {
            d_len = compatibilityDecompose(cp, tmp[0..4]);
        }
        if (d_len == 0) {
            d_len = hangulDecompose(cp, tmp[0..4]);
        }
        if (d_len == 0) {
            d_len = latinCanonicalDecompose(cp, tmp[0..4]);
        }
        if (d_len > 0) {
            let k: usize = 0;
            while (k < d_len && cp_len < 512) {
                cps[cp_len] = tmp[k];
                cp_len = cp_len + 1;
                k = k + 1;
            }
        } else {
            cps[cp_len] = cp;
            cp_len = cp_len + 1;
        }
    }

    canonicalOrder(cps[0..cp_len]);
    if (compose) {
        cp_len = inormComposeInPlace(cps[0..cp_len], cp_len);
    }

    let out_pos: usize = 0;
    let j: usize = 0;
    while (j < cp_len) {
        const w = inormEncodeUtf8(cps[j], out, out_pos);
        if (w == 0) {
            break;
        }
        out_pos = out_pos + w;
        j = j + 1;
    }
    return out_pos;
}

/// Normalizes UTF-8 `input` to UAX #15 NFD (Canonical Decomposition) into `out`.
pub fn normalizeNfd(input: string, out: []u8): usize {
    return inormNormalizeCore(input, false, false, out);
}

/// Normalizes UTF-8 `input` to UAX #15 NFC (Canonical Decomposition + Composition) into `out`.
pub fn normalizeNfc(input: string, out: []u8): usize {
    return inormNormalizeCore(input, false, true, out);
}

/// Normalizes UTF-8 `input` to UAX #15 NFKD (Compatibility Decomposition) into `out`.
pub fn normalizeNfkd(input: string, out: []u8): usize {
    return inormNormalizeCore(input, true, false, out);
}

/// Normalizes UTF-8 `input` to UAX #15 NFKC (Compatibility Decomposition + Composition) into `out`.
pub fn normalizeNfkc(input: string, out: []u8): usize {
    return inormNormalizeCore(input, true, true, out);
}

/// Returns true if UTF-8 `input` is already in Unicode NFC form.
pub fn isNfc(input: string): bool {
    if (inormIsPureAscii(input)) {
        return true;
    }
    let buf: [1024]u8 = [0; 1024];
    const n = normalizeNfc(input, buf[0..1024]);
    return inormBytesEq(input, buf[0..n]);
}

/// Returns true if UTF-8 `input` is already in Unicode NFD form.
pub fn isNfd(input: string): bool {
    if (inormIsPureAscii(input)) {
        return true;
    }
    let buf: [1024]u8 = [0; 1024];
    const n = normalizeNfd(input, buf[0..1024]);
    return inormBytesEq(input, buf[0..n]);
}

/// Returns true if `a` and `b` are canonically equivalent under Unicode NFC.
pub fn eqNfc(a: string, b: string): bool {
    if (inormIsPureAscii(a) && inormIsPureAscii(b)) {
        return inormBytesEq(a, b);
    }
    let buf_a: [1024]u8 = [0; 1024];
    let buf_b: [1024]u8 = [0; 1024];
    const na = normalizeNfc(a, buf_a[0..1024]);
    const nb = normalizeNfc(b, buf_b[0..1024]);
    return inormBytesEq(buf_a[0..na], buf_b[0..nb]);
}
