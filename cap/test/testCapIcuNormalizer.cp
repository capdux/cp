// cap/test/testCapIcuNormalizer.cp
// Comprehensive unit tests for @cap/icu-normalizer

import { test, expect_eq } from "cap:test";
import {
    canonicalCombiningClass,
    hangulDecompose,
    hangulCompose,
    latinCanonicalDecompose,
    latinCanonicalCompose,
    compatibilityDecompose,
    canonicalOrder,
    normalizeNfd,
    normalizeNfc,
    normalizeNfkd,
    normalizeNfkc,
    isNfc,
    isNfd,
    eqNfc,
} from "../../packages/icu_normalizer/src/index.cp";

fn inormTestEqBytes(a: []const u8, b: []const u8): bool {
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

fn main() {
    test("1. @cap/icu-normalizer — Latin-1 precomposed é (U+00E9) <-> e + U+0301 NFC/NFD round-trip", () => {
        // UTF-8 for "café" precomposed: 'c','a','f', 0xC3, 0xA9
        let pre: [5]u8 = ['c' as u8, 'a' as u8, 'f' as u8, 0xC3 as u8, 0xA9 as u8];
        // UTF-8 for "cafe\u{0301}" decomposed: 'c','a','f','e', 0xCC, 0x81
        let decomp: [6]u8 = ['c' as u8, 'a' as u8, 'f' as u8, 'e' as u8, 0xCC as u8, 0x81 as u8];

        let out_nfd: [32]u8 = [0; 32];
        const n_nfd = normalizeNfd(pre[0..5], out_nfd[0..32]);
        expect_eq(n_nfd, 6 as usize);
        expect_eq(inormTestEqBytes(out_nfd[0..n_nfd], decomp[0..6]), true);

        let out_nfc: [32]u8 = [0; 32];
        const n_nfc = normalizeNfc(decomp[0..6], out_nfc[0..32]);
        expect_eq(n_nfc, 5 as usize);
        expect_eq(inormTestEqBytes(out_nfc[0..n_nfc], pre[0..5]), true);

        // Direct Latin helpers
        let pair: [2]u32 = [0; 2];
        expect_eq(latinCanonicalDecompose(0x00E9 as u32, pair[0..2]), 2 as usize);
        expect_eq(pair[0], 'e' as u32);
        expect_eq(pair[1], 0x0301 as u32);
        expect_eq(latinCanonicalCompose('e' as u32, 0x0301 as u32), 0x00E9 as u32);
    });

    test("2. @cap/icu-normalizer — Algorithmic Hangul syllable 한 (U+D55C) NFD/NFC round-trip", () => {
        // U+D55C ('한') -> U+1112, U+1161, U+11AB
        let jamo: [3]u32 = [0; 3];
        const cnt = hangulDecompose(0xD55C as u32, jamo[0..3]);
        expect_eq(cnt, 3 as usize);
        expect_eq(jamo[0], 0x1112 as u32);
        expect_eq(jamo[1], 0x1161 as u32);
        expect_eq(jamo[2], 0x11AB as u32);
        expect_eq(hangulCompose(0x1112 as u32, 0x1161 as u32, 0x11AB as u32), 0xD55C as u32);

        // UTF-8 for U+D55C: E D 95 9C
        let han_nfc: [3]u8 = [0xED as u8, 0x95 as u8, 0x9C as u8];
        // UTF-8 for U+1112 (E1 84 92), U+1161 (E1 85 A1), U+11AB (E1 86 AB)
        let han_nfd: [9]u8 = [
            0xE1 as u8, 0x84 as u8, 0x92 as u8,
            0xE1 as u8, 0x85 as u8, 0xA1 as u8,
            0xE1 as u8, 0x86 as u8, 0xAB as u8,
        ];

        let buf_nfd: [32]u8 = [0; 32];
        const n1 = normalizeNfd(han_nfc[0..3], buf_nfd[0..32]);
        expect_eq(n1, 9 as usize);
        expect_eq(inormTestEqBytes(buf_nfd[0..n1], han_nfd[0..9]), true);

        let buf_nfc: [32]u8 = [0; 32];
        const n2 = normalizeNfc(han_nfd[0..9], buf_nfc[0..32]);
        expect_eq(n2, 3 as usize);
        expect_eq(inormTestEqBytes(buf_nfc[0..n2], han_nfc[0..3]), true);
    });

    test("3. @cap/icu-normalizer — Canonical Combining Class (CCC) & canonicalOrder reordering", () => {
        expect_eq(canonicalCombiningClass('a' as u32), 0 as u8);
        expect_eq(canonicalCombiningClass(0x0301 as u32), 230 as u8); // acute
        expect_eq(canonicalCombiningClass(0x0323 as u32), 220 as u8); // dot below
        expect_eq(canonicalCombiningClass(0x0327 as u32), 202 as u8); // cedilla

        // Unordered: 'a' + acute(230) + dot-below(220) -> 'a' + dot-below(220) + acute(230)
        let seq: [3]u32 = ['a' as u32, 0x0301 as u32, 0x0323 as u32];
        canonicalOrder(seq[0..3]);
        expect_eq(seq[0], 'a' as u32);
        expect_eq(seq[1], 0x0323 as u32);
        expect_eq(seq[2], 0x0301 as u32);

        // UTF-8 NFD reordering check: 'a' (61) + U+0301 (CC 81) + U+0323 (CC A3)
        let raw: [5]u8 = ['a' as u8, 0xCC as u8, 0x81 as u8, 0xCC as u8, 0xA3 as u8];
        let expected: [5]u8 = ['a' as u8, 0xCC as u8, 0xA3 as u8, 0xCC as u8, 0x81 as u8];
        let out: [16]u8 = [0; 16];
        const n = normalizeNfd(raw[0..5], out[0..16]);
        expect_eq(n, 5 as usize);
        expect_eq(inormTestEqBytes(out[0..n], expected[0..5]), true);
    });

    test("4. @cap/icu-normalizer — NFKC/NFKD fullwidth ＡＢＣ & ligature ﬁ decomposition", () => {
        // Fullwidth "ＡＢＣ" = U+FF21 (EF BC A1), U+FF22 (EF BC A2), U+FF23 (EF BC A3)
        let fw: [9]u8 = [
            0xEF as u8, 0xBC as u8, 0xA1 as u8,
            0xEF as u8, 0xBC as u8, 0xA2 as u8,
            0xEF as u8, 0xBC as u8, 0xA3 as u8,
        ];
        let out: [32]u8 = [0; 32];
        const n_fw = normalizeNfkc(fw[0..9], out[0..32]);
        expect_eq(n_fw, 3 as usize);
        expect_eq(inormTestEqBytes(out[0..n_fw], "ABC"), true);

        // Ligature "ﬁ" (U+FB01 = EF AC 81) + "le" -> "file"
        let lig: [5]u8 = [0xEF as u8, 0xAC as u8, 0x81 as u8, 'l' as u8, 'e' as u8];
        const n_lig = normalizeNfkd(lig[0..5], out[0..32]);
        expect_eq(n_lig, 4 as usize);
        expect_eq(inormTestEqBytes(out[0..n_lig], "file"), true);

        // Compatibility helper direct check for ² (U+00B2) -> '2'
        let cbuf: [4]u32 = [0; 4];
        expect_eq(compatibilityDecompose(0x00B2 as u32, cbuf[0..4]), 1 as usize);
        expect_eq(cbuf[0], '2' as u32);
    });

    test("5. @cap/icu-normalizer — isNfc, isNfd & eqNfc canonical equivalence", () => {
        let pre: [5]u8 = ['c' as u8, 'a' as u8, 'f' as u8, 0xC3 as u8, 0xA9 as u8];
        let decomp: [6]u8 = ['c' as u8, 'a' as u8, 'f' as u8, 'e' as u8, 0xCC as u8, 0x81 as u8];

        expect_eq(isNfc(pre[0..5]), true);
        expect_eq(isNfd(pre[0..5]), false);
        expect_eq(isNfd(decomp[0..6]), true);
        expect_eq(isNfc(decomp[0..6]), false);
        expect_eq(eqNfc(pre[0..5], decomp[0..6]), true);
        expect_eq(eqNfc(pre[0..5], "cafe"), false);
    });

    test("6. @cap/icu-normalizer — Pure ASCII fast path, empty string & truncated buffer safety", () => {
        expect_eq(isNfc("Hello, Unicode!"), true);
        expect_eq(isNfd("Hello, Unicode!"), true);
        expect_eq(eqNfc("ascii-123", "ascii-123"), true);
        expect_eq(eqNfc("ascii-123", "ascii-124"), false);

        let out: [32]u8 = [0; 32];
        expect_eq(normalizeNfc("", out[0..32]), 0 as usize);
        expect_eq(normalizeNfd("", out[0..32]), 0 as usize);

        const n_ascii = normalizeNfc("Hello, CP!", out[0..32]);
        expect_eq(n_ascii, 10 as usize);
        expect_eq(inormTestEqBytes(out[0..n_ascii], "Hello, CP!"), true);

        // Truncated output buffer (only 3 bytes available for 10-byte ASCII / multi-byte UTF-8)
        let small: [3]u8 = [0; 3];
        const n_trunc = normalizeNfc("Hello, CP!", small[0..3]);
        expect_eq(n_trunc, 3 as usize);
        expect_eq(inormTestEqBytes(small[0..3], "Hel"), true);

        // Truncated multi-byte UTF-8 buffer: "café" (5 bytes) into 4-byte buffer stops before partial 2-byte 'é'
        let pre: [5]u8 = ['c' as u8, 'a' as u8, 'f' as u8, 0xC3 as u8, 0xA9 as u8];
        let small4: [4]u8 = [0; 4];
        const n_utf8_trunc = normalizeNfc(pre[0..5], small4[0..4]);
        expect_eq(n_utf8_trunc, 3 as usize);
        expect_eq(inormTestEqBytes(small4[0..3], "caf"), true);
    });
}
