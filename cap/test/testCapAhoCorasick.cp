// cap/test/testCapAhoCorasick.cp
// Comprehensive unit tests for @cap/aho-corasick

import { test, expect_eq } from "cap:test";
import {
    AcMatch,
    AhoCorasick,
} from "../../packages/@cap/aho-corasick/src/index.cp";

fn main() {
    test("1. @cap/aho-corasick — Classic paper vector {he, she, his, hers} on 'ushers'", () => {
        let ac = AhoCorasick.init();
        expect_eq(ac.addPattern("he"), 0 as i32);
        expect_eq(ac.addPattern("she"), 1 as i32);
        expect_eq(ac.addPattern("his"), 2 as i32);
        expect_eq(ac.addPattern("hers"), 3 as i32);
        expect_eq(ac.patternCount(), 4 as usize);
        expect_eq(ac.build(), true);

        let matches: [16]AcMatch = undefined;
        const n = ac.findAll("ushers", matches[0..16]);
        expect_eq(n, 3 as usize);
        expect_eq(ac.countMatches("ushers"), 3 as usize);

        // Expected: "she" [1,4), "he" [2,4), "hers" [2,6)
        expect_eq(matches[0].pattern_id, 1 as u16);
        expect_eq(matches[0].start, 1 as usize);
        expect_eq(matches[0].end, 4 as usize);

        expect_eq(matches[1].pattern_id, 0 as u16);
        expect_eq(matches[1].start, 2 as usize);
        expect_eq(matches[1].end, 4 as usize);

        expect_eq(matches[2].pattern_id, 3 as u16);
        expect_eq(matches[2].start, 2 as usize);
        expect_eq(matches[2].end, 6 as usize);
    });

    test("2. @cap/aho-corasick — Overlapping nested dictionary suffix chain (dict_link)", () => {
        let ac = AhoCorasick.init();
        * = ac.addPattern("a");
        * = ac.addPattern("ab");
        * = ac.addPattern("bab");
        * = ac.addPattern("bc");
        * = ac.addPattern("bca");
        * = ac.addPattern("c");
        * = ac.addPattern("caa");
        * = ac.build();

        // "abccab": a[0,1), ab[0,2), bc[1,3), c[2,3), c[3,4), a[4,5), ab[4,6) => 7 matches
        expect_eq(ac.countMatches("abccab"), 7 as usize);

        let out: [16]AcMatch = undefined;
        const n = ac.findAll("abccab", out[0..16]);
        expect_eq(n, 7 as usize);
        expect_eq(out[0].start, 0 as usize);
        expect_eq(out[0].end, 1 as usize);
        expect_eq(out[6].start, 4 as usize);
        expect_eq(out[6].end, 6 as usize);
    });

    test("3. @cap/aho-corasick — isMatch & findFirst exact byte span", () => {
        let ac = AhoCorasick.init();
        * = ac.addPattern("SELECT");
        * = ac.addPattern("DROP TABLE");
        * = ac.addPattern("UNION");
        * = ac.build();

        const query = "id=1; DROP TABLE users;";
        expect_eq(ac.isMatch(query), true);
        const first = ac.findFirst(query);
        expect_eq(first.found, true);
        expect_eq(first.pattern_id, 1 as u16);
        expect_eq(first.start, 6 as usize);
        expect_eq(first.end, 16 as usize);
    });

    test("4. @cap/aho-corasick — Unmatched text & empty text handling", () => {
        let ac = AhoCorasick.init();
        * = ac.addPattern("foo");
        * = ac.addPattern("bar");
        * = ac.build();

        expect_eq(ac.isMatch(""), false);
        expect_eq(ac.isMatch("hello world"), false);
        expect_eq(ac.findFirst("hello world").found, false);
        expect_eq(ac.countMatches("hello world"), 0 as usize);
    });

    test("5. @cap/aho-corasick — Single-byte patterns & repeated occurrence counting", () => {
        let ac = AhoCorasick.init();
        * = ac.addPattern("a");
        * = ac.addPattern("aa");
        * = ac.build();

        // "aaaa": 'a' hits 4 times, 'aa' hits 3 times => 7 total overlapping matches
        expect_eq(ac.countMatches("aaaa"), 7 as usize);
    });

    test("6. @cap/aho-corasick — Reject empty pattern & enforce node pool capacity", () => {
        let ac = AhoCorasick.init();
        expect_eq(ac.addPattern(""), -1 as i32);
        expect_eq(ac.patternCount(), 0 as usize);

        // Fill node pool safely with 30-byte distinct patterns (255 nodes available -> 8 patterns = 240 nodes, 9th rejected)
        let pat: [30]u8 = undefined;
        let p: usize = 0;
        let added: usize = 0;
        while (p < 12) {
            let k: usize = 0;
            while (k < 30) {
                pat[k] = ('a' as u8) + (p as u8);
                k += 1;
            }
            const id = ac.addPattern(pat[0..30]);
            if (id >= (0 as i32)) {
                added += 1;
            }
            p += 1;
        }
        expect_eq(added, 8 as usize);
        expect_eq(ac.build(), true);
        expect_eq(ac.node_count <= (256 as usize), true);
    });
}
