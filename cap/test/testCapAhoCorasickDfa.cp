import { test, expect_eq } from "cap:test";
import { AcMatch, AhoCorasick } from "../../packages/@cap/aho-corasick-dfa/src/index.cp";

fn main() {
    test("1. @cap/aho-corasick-dfa — Classic he/she/his/hers overlapping matches in 'ushers'", () => {
        let ac = AhoCorasick.new(false);
        expect_eq(ac.addPattern("he"), true);   // id 0
        expect_eq(ac.addPattern("she"), true);  // id 1
        expect_eq(ac.addPattern("his"), true);  // id 2
        expect_eq(ac.addPattern("hers"), true); // id 3
        ac.build();

        expect_eq(ac.patternCount(), 4 as usize);
        expect_eq(ac.stateCount(), 10 as usize); // root + h,e,r,s + s,h,e + i,s
        expect_eq(ac.isMatch("ushers"), true);
        expect_eq(ac.countMatches("ushers"), 3 as usize);

        let out: [8]AcMatch = undefined;
        const n: usize = ac.findAll("ushers", out[0..8]);
        expect_eq(n, 3 as usize);
        // At end=4 ("ushe"): "he" (id 0, [2,4]) and "she" (id 1, [1,4])
        expect_eq(out[0].pattern_id, 0 as usize);
        expect_eq(out[0].start, 2 as usize);
        expect_eq(out[0].end, 4 as usize);
        expect_eq(out[1].pattern_id, 1 as usize);
        expect_eq(out[1].start, 1 as usize);
        expect_eq(out[1].end, 4 as usize);
        // At end=6 ("ushers"): "hers" (id 3, [2,6])
        expect_eq(out[2].pattern_id, 3 as usize);
        expect_eq(out[2].start, 2 as usize);
        expect_eq(out[2].end, 6 as usize);
    });

    test("2. @cap/aho-corasick-dfa — findFirst earliest-ending and leftmost-start tie breaking", () => {
        let ac = AhoCorasick.new(false);
        ac.addPattern("he");   // id 0
        ac.addPattern("she");  // id 1
        ac.addPattern("hers"); // id 2
        ac.build();

        const m = ac.findFirst("ushers");
        expect_eq(m != null, true);
        if (hit := m) {
            // Both "he" [2,4] and "she" [1,4] end at 4; leftmost start is "she" (id 1)
            expect_eq(hit.pattern_id, 1 as usize);
            expect_eq(hit.start, 1 as usize);
            expect_eq(hit.end, 4 as usize);
        }

        const m2 = ac.findFirst("xyz hers");
        expect_eq(m2 != null, true);
        if (hit2 := m2) {
            // "he" [4,6] ends earlier than "hers" [4,8]
            expect_eq(hit2.pattern_id, 0 as usize);
            expect_eq(hit2.start, 4 as usize);
            expect_eq(hit2.end, 6 as usize);
        }
    });

    test("3. @cap/aho-corasick-dfa — ASCII case-insensitive matching via @cap/unicase", () => {
        let ac = AhoCorasick.new(true);
        ac.addPattern("Error");
        ac.addPattern("WARN");
        ac.addPattern("fatal");
        ac.build();

        expect_eq(ac.isMatch("all good"), false);
        expect_eq(ac.isMatch("system ERROR detected"), true);
        expect_eq(ac.countMatches("warn: Fatal eRrOr"), 3 as usize);

        let out: [4]AcMatch = undefined;
        const n: usize = ac.findAll("warn: Fatal eRrOr", out[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0].pattern_id, 1 as usize); // WARN at [0,4]
        expect_eq(out[0].start, 0 as usize);
        expect_eq(out[0].end, 4 as usize);
        expect_eq(out[1].pattern_id, 2 as usize); // fatal at [6,11]
        expect_eq(out[1].start, 6 as usize);
        expect_eq(out[1].end, 11 as usize);
        expect_eq(out[2].pattern_id, 0 as usize); // Error at [12,17]
        expect_eq(out[2].start, 12 as usize);
        expect_eq(out[2].end, 17 as usize);
    });

    test("4. @cap/aho-corasick-dfa — Nested prefix/suffix patterns & output link propagation", () => {
        let ac = AhoCorasick.new(false);
        ac.addPattern("a");      // id 0
        ac.addPattern("ab");     // id 1
        ac.addPattern("bab");    // id 2
        ac.addPattern("bc");     // id 3
        ac.addPattern("bca");    // id 4
        ac.addPattern("c");      // id 5
        ac.addPattern("caa");    // id 6
        ac.build();

        // Haystack "abccab":
        // i=0 'a' -> "a" [0,1]
        // i=1 'b' -> "ab" [0,2]
        // i=2 'c' -> "bc" [1,3], "c" [2,3]
        // i=3 'c' -> "c" [3,4]
        // i=4 'a' -> "a" [4,5]
        // i=5 'b' -> "ab" [4,6]
        expect_eq(ac.countMatches("abccab"), 7 as usize);

        // Haystack "babcaa":
        // i=1 "a" [1,2], i=2 "ab" [1,3] + "bab" [0,3], i=3 "bc" [2,4] + "c" [3,4],
        // i=4 "bca" [2,5] + "a" [4,5], i=5 "caa" [3,6] + "a" [5,6] => 9 matches
        expect_eq(ac.countMatches("babcaa"), 9 as usize);
    });

    test("5. @cap/aho-corasick-dfa — Unmatched inputs, empty strings & small output buffer clamping", () => {
        let ac = AhoCorasick.new(false);
        expect_eq(ac.addPattern(""), false); // empty pattern rejected
        expect_eq(ac.addPattern("alpha"), true);
        expect_eq(ac.addPattern("beta"), true);
        ac.build();

        expect_eq(ac.isMatch(""), false);
        expect_eq(ac.findFirst("") == null, true);
        expect_eq(ac.countMatches(""), 0 as usize);
        expect_eq(ac.isMatch("gamma delta"), false);
        expect_eq(ac.findFirst("gamma delta") == null, true);

        let tiny: [1]AcMatch = undefined;
        const n: usize = ac.findAll("alpha beta alpha", tiny[0..1]);
        expect_eq(n, 1 as usize);
        expect_eq(tiny[0].pattern_id, 0 as usize);
        expect_eq(tiny[0].start, 0 as usize);
        expect_eq(tiny[0].end, 5 as usize);
        expect_eq(ac.countMatches("alpha beta alpha"), 3 as usize);
    });

    test("6. @cap/aho-corasick-dfa — Unbuilt guard & case-sensitive exact mismatch", () => {
        let ac = AhoCorasick.new(false);
        ac.addPattern("Needle");
        // Before build(), searches safely return empty / false.
        expect_eq(ac.isMatch("Needle"), false);
        expect_eq(ac.findFirst("Needle") == null, true);
        expect_eq(ac.countMatches("Needle"), 0 as usize);

        ac.build();
        expect_eq(ac.isMatch("needle"), false);
        expect_eq(ac.isMatch("Needle"), true);
        expect_eq(ac.countMatches("Needle in a Needle"), 2 as usize);
    });
}
