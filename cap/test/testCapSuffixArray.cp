// cap/test/testCapSuffixArray.cp
// Comprehensive unit tests for @cap/suffix-array (Rust suffix crate parity)

import { test, expect_eq } from "cap:test";
import { SuffixArray, LrsResult, MAX_TEXT } from "../../packages/@cap/suffix-array/src/index.cp";

fn main() {
    test("1. @cap/suffix-array — classic 'banana' exact SA and LCP arrays", () => {
        const sa: SuffixArray = SuffixArray.build("banana");
        expect_eq(sa.len(), 6 as usize);

        // Expected SA: [5, 3, 1, 0, 4, 2]
        expect_eq(sa.suffixAt(0), 5 as u16);
        expect_eq(sa.suffixAt(1), 3 as u16);
        expect_eq(sa.suffixAt(2), 1 as u16);
        expect_eq(sa.suffixAt(3), 0 as u16);
        expect_eq(sa.suffixAt(4), 4 as u16);
        expect_eq(sa.suffixAt(5), 2 as u16);

        // Expected LCP: [0, 1, 3, 0, 0, 2]
        expect_eq(sa.lcpAt(0), 0 as u16);
        expect_eq(sa.lcpAt(1), 1 as u16);
        expect_eq(sa.lcpAt(2), 3 as u16);
        expect_eq(sa.lcpAt(3), 0 as u16);
        expect_eq(sa.lcpAt(4), 0 as u16);
        expect_eq(sa.lcpAt(5), 2 as u16);
    });

    test("2. @cap/suffix-array — 'banana' longest repeated substring is 'ana' (len 3)", () => {
        const sa: SuffixArray = SuffixArray.build("banana");
        const lrs: LrsResult = sa.longestRepeatedSubstring();
        expect_eq(lrs.found, true);
        expect_eq(lrs.len, 3 as usize);
        expect_eq(sa.text[lrs.start], 'a' as u8);
        expect_eq(sa.text[lrs.start + 1], 'n' as u8);
        expect_eq(sa.text[lrs.start + 2], 'a' as u8);
    });

    test("3. @cap/suffix-array — contains & count binary search hits and misses", () => {
        const sa: SuffixArray = SuffixArray.build("mississippi");
        expect_eq(sa.contains("issi"), true);
        expect_eq(sa.count("issi"), 2 as usize);
        expect_eq(sa.contains("s"), true);
        expect_eq(sa.count("s"), 4 as usize);
        expect_eq(sa.contains("i"), true);
        expect_eq(sa.count("i"), 4 as usize);
        expect_eq(sa.contains("pp"), true);
        expect_eq(sa.count("pp"), 1 as usize);
        expect_eq(sa.contains("mississippi"), true);
        expect_eq(sa.count("mississippi"), 1 as usize);
        expect_eq(sa.contains("xyz"), false);
        expect_eq(sa.count("xyz"), 0 as usize);
        expect_eq(sa.contains("mississippix"), false);
        expect_eq(sa.count("mississippix"), 0 as usize);
        expect_eq(sa.contains(""), true);
        expect_eq(sa.count(""), 12 as usize);
    });

    test("4. @cap/suffix-array — empty string & single character edge cases", () => {
        const empty: SuffixArray = SuffixArray.build("");
        expect_eq(empty.len(), 0 as usize);
        expect_eq(empty.contains(""), true);
        expect_eq(empty.count(""), 1 as usize);
        expect_eq(empty.contains("a"), false);
        expect_eq(empty.count("a"), 0 as usize);
        const lrs_empty: LrsResult = empty.longestRepeatedSubstring();
        expect_eq(lrs_empty.found, false);
        expect_eq(lrs_empty.len, 0 as usize);

        const single: SuffixArray = SuffixArray.build("z");
        expect_eq(single.len(), 1 as usize);
        expect_eq(single.suffixAt(0), 0 as u16);
        expect_eq(single.lcpAt(0), 0 as u16);
        expect_eq(single.contains("z"), true);
        expect_eq(single.count("z"), 1 as usize);
        expect_eq(single.contains("a"), false);
        const lrs_single: LrsResult = single.longestRepeatedSubstring();
        expect_eq(lrs_single.found, false);
    });

    test("5. @cap/suffix-array — identical characters 'aaaaaa' LCP and count", () => {
        const sa: SuffixArray = SuffixArray.build("aaaaaa");
        expect_eq(sa.len(), 6 as usize);

        // SA should be [5, 4, 3, 2, 1, 0], LCP should be [0, 1, 2, 3, 4, 5]
        let i: usize = 0;
        while (i < 6) {
            expect_eq(sa.suffixAt(i), (5 - i) as u16);
            expect_eq(sa.lcpAt(i), i as u16);
            i += 1;
        }

        expect_eq(sa.count("a"), 6 as usize);
        expect_eq(sa.count("aa"), 5 as usize);
        expect_eq(sa.count("aaa"), 4 as usize);
        expect_eq(sa.count("aaaaaa"), 1 as usize);
        expect_eq(sa.count("aaaaaaa"), 0 as usize);

        const lrs: LrsResult = sa.longestRepeatedSubstring();
        expect_eq(lrs.found, true);
        expect_eq(lrs.len, 5 as usize);
    });

    test("6. @cap/suffix-array — 256-byte long text strict lexicographical monotonicity", () => {
        let buf: [256]u8 = undefined;
        let i: usize = 0;
        while (i < MAX_TEXT) {
            const v: usize = (i * 37 + 13) % 26;
            buf[i] = (97 + v) as u8;
            i += 1;
        }
        const sa: SuffixArray = SuffixArray.build(buf[0..MAX_TEXT]);
        expect_eq(sa.len(), MAX_TEXT);

        // Verify strict lexicographical order and LCP accuracy across all adjacent pairs
        let r: usize = 1;
        while (r < MAX_TEXT) {
            const prev: usize = sa.suffixAt(r - 1) as usize;
            const curr: usize = sa.suffixAt(r) as usize;
            let k: usize = 0;
            while (prev + k < MAX_TEXT && curr + k < MAX_TEXT && buf[prev + k] == buf[curr + k]) {
                k += 1;
            }
            expect_eq(sa.lcpAt(r), k as u16);
            const prev_ended: bool = prev + k == MAX_TEXT;
            const curr_ended: bool = curr + k == MAX_TEXT;
            expect_eq(curr_ended, false);
            if (!prev_ended) {
                expect_eq(buf[prev + k] < buf[curr + k], true);
            }
            r += 1;
        }
    });
}
