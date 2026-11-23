// Reference-hardened test suite against unicode-rs/unicode-segmentation (MIT/Apache-2.0) & Unicode UAX #29
// Covers:
//   1. UAX #29 core rules: CRLF (GB3), Control (GB4/5), Hangul L/V/T/LV/LVT (GB6-8), Combining marks (GB9/9a)
//   2. Emoji ZWJ chains (GB11: family, skin tone + ZWJ), Regional Indicators 2/3/4 (GB12/13), CJK & sliceByGraphemes
//   3. Malformed UTF-8 safety (lone continuation, truncated multi-byte, overlong, surrogates -> 0 infinite loop / 0 OOB)
//   4. 16-lane @Vector(16, u8) SIMD printable-ASCII fast-path 0-diff parity vs scalar (incl. e + U+0301 & CRLF boundaries)
//   5. Deterministic PRNG Fuzz (>= 1,600 valid/malformed UTF-8 byte streams: exact byte-concat reconstruction & count == iter)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    GraphemeIterator,
    count,
    countScalar,
    sliceByGraphemes,
    sliceByGraphemesScalar,
    simdPrintableAsciiRun,
    simdPrintableAsciiRunScalar,
    simdIsAllPrintableAscii,
    simdIsAllPrintableAsciiScalar,
} from "../../lib/text/grapheme.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn main() {
    test("1. UAX #29 core rules: CRLF, Control, Hangul syllables & combining marks", () => {
        // GB3/GB4/GB5: CR x LF stays 1 cluster, LF ÷ CR splits into 2
        expect_eq(count("\r\n"), 1 as usize);
        expect_eq(count("\n\r"), 2 as usize);
        expect_eq(count("a\r\nb\nc"), 5 as usize);
        expect_eq(sliceByGraphemes("a\r\nb", 1, 2), "\r\n");

        // GB9: combining acute accent e + U+0301 -> 1 grapheme
        const e_acute = "e\xcc\x81";
        expect_eq(count(e_acute), 1 as usize);
        expect_eq(count("cafe\xcc\x81!"), 5 as usize);
        expect_eq(sliceByGraphemes("cafe\xcc\x81!", 3, 4), e_acute);

        // Japanese dakuten combining mark: か + U+3099 -> 1 grapheme
        const ka_dakuten = "\xe3\x81\x8b\xe3\x82\x99";
        expect_eq(count(ka_dakuten), 1 as usize);

        // GB6-GB8: Hangul Jamo L + V + T (U+1100 + U+1161 + U+11A8 -> 1 syllable)
        const hangul_lvt = "\xe1\x84\x80\xe1\x85\xa1\xe1\x86\xa8";
        expect_eq(count(hangul_lvt), 1 as usize);
        // Precomposed Hangul LV (가 U+AC00) + T (U+11A8) -> 1 syllable
        const hangul_lv_t = "\xea\xb0\x80\xe1\x86\xa8";
        expect_eq(count(hangul_lv_t), 1 as usize);
    });

    test("2. Emoji ZWJ (GB11), skin tone, 2/3/4 Regional Indicators (GB12/13) & CJK slice", () => {
        // GB11: Family Emoji 👨‍👩‍👧‍👦 (7 codepoints, 25 UTF-8 bytes -> 1 grapheme)
        const family = "👨‍👩‍👧‍👦";
        expect_eq(count(family), 1 as usize);

        // Skin tone modifier 👋🏽 -> 1 grapheme
        const wave = "👋🏽";
        expect_eq(count(wave), 1 as usize);

        // Skin tone + ZWJ chain 👩🏽‍💻 -> 1 grapheme
        const dev = "👩🏽‍💻";
        expect_eq(count(dev), 1 as usize);

        // GB12/13: Regional Indicators (2 -> 1 flag, 3 -> 2+1=2 clusters, 4 -> 2+2=2 clusters)
        const ri2 = "🇺🇸";
        const ri3 = "🇺🇸🇨";
        const ri4 = "🇺🇸🇨🇳";
        expect_eq(count(ri2), 1 as usize);
        expect_eq(count(ri3), 2 as usize);
        expect_eq(count(ri4), 2 as usize);
        expect_eq(sliceByGraphemes(ri3, 0, 1), "🇺🇸");
        expect_eq(sliceByGraphemes(ri3, 1, 2), "🇨");
        expect_eq(sliceByGraphemes(ri4, 1, 2), "🇨🇳");

        // Mixed CJK + Emoji + ASCII zero-truncation slicing
        const mixed = "你好👨‍👩‍👧‍👦世界👋🏽!";
        expect_eq(count(mixed), 7 as usize);
        expect_eq(sliceByGraphemes(mixed, 2, 3), family);
        expect_eq(sliceByGraphemes(mixed, 5, 6), wave);
        expect_eq(sliceByGraphemes(mixed, 0, 2), "你好");
        expect_eq(sliceByGraphemes(mixed, 3, 5), "世界");
    });

    test("3. Malformed UTF-8 safety (lone continuation, truncated, overlong, surrogates)", () => {
        // Lone continuation bytes (0x80, 0xBF)
        const bad1 = "\x80\xbf";
        expect_eq(count(bad1), 2 as usize);
        // Truncated 3-byte & 4-byte sequences
        const bad2 = "\xe2\x82\xf0\x9f\x98";
        expect_eq(count(bad2), 5 as usize);
        // Overlong 2-byte NUL (0xC0 0x80) & UTF-16 surrogate U+D800 (0xED 0xA0 0x80)
        const bad3 = "\xc0\x80\xed\xa0\x80";
        expect_eq(count(bad3), 5 as usize);
        // Verify exact byte-length conservation via iterator
        let it = GraphemeIterator.init(bad3);
        let bytes_seen: usize = 0;
        let clusters: usize = 0;
        while (seg := it.next()) {
            bytes_seen += seg.len;
            clusters += 1;
        }
        expect_eq(bytes_seen, bad3.len);
        expect_eq(clusters, count(bad3));
    });

    test("4. 16-lane SIMD printable-ASCII fast-path 0-diff parity vs scalar", () => {
        const pure_ascii = "The quick brown fox jumps over the lazy dog 0123456789!@#";
        expect_eq(simdIsAllPrintableAscii(pure_ascii), true);
        expect_eq(simdIsAllPrintableAscii(pure_ascii), simdIsAllPrintableAsciiScalar(pure_ascii));
        expect_eq(simdPrintableAsciiRun(pure_ascii), pure_ascii.len);
        expect_eq(count(pure_ascii), pure_ascii.len);
        expect_eq(count(pure_ascii), countScalar(pure_ascii));
        expect_eq(sliceByGraphemes(pure_ascii, 4, 19), sliceByGraphemesScalar(pure_ascii, 4, 19));
        expect_eq(sliceByGraphemes(pure_ascii, 4, 19), "quick brown fox");

        // Boundary trap 1: >16 ASCII bytes followed by combining mark U+0301 on the last 'e'
        const ascii_plus_comb = "0123456789abcdef_cafe\xcc\x81_end_0123456789";
        expect_eq(simdIsAllPrintableAscii(ascii_plus_comb), false);
        expect_eq(simdPrintableAsciiRun(ascii_plus_comb), simdPrintableAsciiRunScalar(ascii_plus_comb));
        expect_eq(count(ascii_plus_comb), countScalar(ascii_plus_comb));
        expect_eq(sliceByGraphemes(ascii_plus_comb, 17, 21), "cafe\xcc\x81");

        // Boundary trap 2: >16 ASCII bytes followed by CRLF + Emoji + ASCII tail
        const ascii_crlf = "0123456789abcdef\r\n👨‍👩‍👧‍👦0123456789abcdef";
        expect_eq(simdPrintableAsciiRun(ascii_crlf), 16 as usize);
        expect_eq(count(ascii_crlf), countScalar(ascii_crlf));
        expect_eq(count(ascii_crlf), 34 as usize);
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 valid/malformed UTF-8 byte streams)", () => {
        let seed: u64 = 0x5EED_68AF_2026_0033;
        let buf: [80]u8 = undefined;
        let recon: [80]u8 = undefined;

        // 5A. 1,000 mixed valid/malformed UTF-8 + ASCII + CRLF + Emoji fuzz streams
        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const len: usize = (nextRng(&seed) % 72) as usize;
            let k: usize = 0;
            while (k < len) {
                const mode = nextRng(&seed) % 10;
                if (mode < 5) {
                    // Printable ASCII
                    buf[k] = (0x20 + (nextRng(&seed) % 95)) as u8;
                    k += 1;
                } else if (mode == 5 && k + 2 <= len) {
                    // Combining accent U+0301 (0xCC 0x81)
                    buf[k] = 0xCC;
                    buf[k + 1] = 0x81;
                    k += 2;
                } else if (mode == 6 && k + 3 <= len) {
                    // ZWJ U+200D (0xE2 0x80 0x8D)
                    buf[k] = 0xE2;
                    buf[k + 1] = 0x80;
                    buf[k + 2] = 0x8D;
                    k += 3;
                } else if (mode == 7 && k + 4 <= len) {
                    // Regional Indicator U+1F1E6..U+1F1FF (0xF0 0x9F 0x87 0xA6..0xBF)
                    buf[k] = 0xF0;
                    buf[k + 1] = 0x9F;
                    buf[k + 2] = 0x87;
                    buf[k + 3] = (0xA6 + (nextRng(&seed) % 26)) as u8;
                    k += 4;
                } else {
                    // Arbitrary raw byte (CR, LF, NUL, lone continuation, 0xFF, etc.)
                    buf[k] = (nextRng(&seed) & 0xFF) as u8;
                    k += 1;
                }
            }
            const sample = buf[0..len];

            // Verify SIMD vs scalar count 0-diff parity
            const c_simd = count(sample);
            const c_scal = countScalar(sample);
            expect_eq(c_simd, c_scal);

            // Verify iterator concatenation reconstructs the exact original byte stream
            let it = GraphemeIterator.init(sample);
            let out_pos: usize = 0;
            let n_clusters: usize = 0;
            while (seg := it.next()) {
                expect_eq(seg.len > 0, true);
                @memcpy(recon[out_pos .. out_pos + seg.len], seg);
                out_pos += seg.len;
                n_clusters += 1;
            }
            expect_eq(out_pos, len);
            expect_eq(n_clusters, c_simd);
            expect_eq(recon[0..out_pos], sample);
        }

        // 5B. 600 random slice SIMD printable-ASCII & sliceByGraphemes 0-diff parity checks
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            const rlen: usize = (nextRng(&seed) % 64) as usize;
            const force_ascii = (r_iter & 1) == 0;
            let j: usize = 0;
            while (j < rlen) : (j += 1) {
                if (force_ascii) {
                    buf[j] = (0x20 + (nextRng(&seed) % 95)) as u8;
                } else {
                    buf[j] = (nextRng(&seed) & 0xFF) as u8;
                }
            }
            const slice = buf[0..rlen];
            expect_eq(simdPrintableAsciiRun(slice), simdPrintableAsciiRunScalar(slice));
            expect_eq(simdIsAllPrintableAscii(slice), simdIsAllPrintableAsciiScalar(slice));
            const s_idx: usize = (nextRng(&seed) % 20) as usize;
            const e_idx: usize = s_idx + ((nextRng(&seed) % 20) as usize);
            expect_eq(sliceByGraphemes(slice, s_idx, e_idx), sliceByGraphemesScalar(slice, s_idx, e_idx));
        }
    });
}
