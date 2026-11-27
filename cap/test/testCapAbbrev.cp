import { test, expect_eq } from "cap:test";
import {
    AbbrevEntry,
    abbrev,
    shortestAbbrev,
    resolveAbbrev,
} from "../../packages/abbrev/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn findEntry(entries: []const AbbrevEntry, key: string): string? {
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (strEq(entries[i].abbrev, key)) {
            return entries[i].full;
        }
    }
    return null;
}

fn main() {
    test("1. @cap/abbrev — Classic npm:abbrev example (foo, fool, folding, flop)", () => {
        const words: [4]string = ["foo", "fool", "folding", "flop"];
        let out: [32]AbbrevEntry = undefined;
        const n = abbrev(words[0..], out[0..]);

        // flop: fl, flo, flop (3)
        // folding: fol, fold, foldi, foldin, folding (5)
        // foo: foo (1)
        // fool: fool (1)
        expect_eq(n, 10);

        if (v := findEntry(out[0..n], "fl")) { expect_eq(strEq(v, "flop"), true); } else { expect_eq(true, false); }
        if (v := findEntry(out[0..n], "fol")) { expect_eq(strEq(v, "folding"), true); } else { expect_eq(true, false); }
        if (v := findEntry(out[0..n], "foo")) { expect_eq(strEq(v, "foo"), true); } else { expect_eq(true, false); }
        if (v := findEntry(out[0..n], "fool")) { expect_eq(strEq(v, "fool"), true); } else { expect_eq(true, false); }

        // Ambiguous prefixes must NOT be in table
        expect_eq(findEntry(out[0..n], "f") == null, true);
        expect_eq(findEntry(out[0..n], "fo") == null, true);
    });

    test("2. @cap/abbrev — shortestAbbrev computes minimal unique prefixes", () => {
        const words: [4]string = ["ruby", "rubyist", "rules", "rust"];

        if (s := shortestAbbrev(words[0..], "ruby")) { expect_eq(strEq(s, "ruby"), true); } else { expect_eq(true, false); }
        if (s := shortestAbbrev(words[0..], "rubyist")) { expect_eq(strEq(s, "rubyi"), true); } else { expect_eq(true, false); }
        if (s := shortestAbbrev(words[0..], "rules")) { expect_eq(strEq(s, "rul"), true); } else { expect_eq(true, false); }
        if (s := shortestAbbrev(words[0..], "rust")) { expect_eq(strEq(s, "rus"), true); } else { expect_eq(true, false); }

        // Target not in word list returns null
        expect_eq(shortestAbbrev(words[0..], "python") == null, true);
    });

    test("3. @cap/abbrev — resolveAbbrev exact match precedence & ambiguity rejection", () => {
        const words: [4]string = ["install", "init", "info", "in"];

        // Exact match "in" wins even though "install"/"init"/"info" also start with "in"
        if (r := resolveAbbrev(words[0..], "in")) { expect_eq(strEq(r, "in"), true); } else { expect_eq(true, false); }

        // Ambiguous prefixes return null
        expect_eq(resolveAbbrev(words[0..], "i") == null, true);
        expect_eq(resolveAbbrev(words[0..], "inf") != null, true);
        if (r := resolveAbbrev(words[0..], "inf")) { expect_eq(strEq(r, "info"), true); } else { expect_eq(true, false); }
        if (r := resolveAbbrev(words[0..], "ini")) { expect_eq(strEq(r, "init"), true); } else { expect_eq(true, false); }
        if (r := resolveAbbrev(words[0..], "ins")) { expect_eq(strEq(r, "install"), true); } else { expect_eq(true, false); }

        // Non-matching prefix returns null
        expect_eq(resolveAbbrev(words[0..], "xyz") == null, true);
    });

    test("4. @cap/abbrev — Duplicate words deduplicated cleanly", () => {
        const words: [5]string = ["build", "build", "bench", "bench", "test"];
        let out: [32]AbbrevEntry = undefined;
        const n = abbrev(words[0..], out[0..]);

        // bench: be, ben, benc, bench (4)
        // build: bu, bui, buil, build (4)
        // test: t, te, tes, test (4)
        expect_eq(n, 12);

        if (r := resolveAbbrev(words[0..], "bu")) { expect_eq(strEq(r, "build"), true); } else { expect_eq(true, false); }
        if (r := resolveAbbrev(words[0..], "t")) { expect_eq(strEq(r, "test"), true); } else { expect_eq(true, false); }
        expect_eq(resolveAbbrev(words[0..], "b") == null, true);
    });

    test("5. @cap/abbrev — Single word & lexicographical output ordering", () => {
        const single: [1]string = ["cap"];
        let out: [8]AbbrevEntry = undefined;
        const n = abbrev(single[0..], out[0..]);
        expect_eq(n, 3);
        expect_eq(strEq(out[0].abbrev, "c"), true);
        expect_eq(strEq(out[1].abbrev, "ca"), true);
        expect_eq(strEq(out[2].abbrev, "cap"), true);

        if (s := shortestAbbrev(single[0..], "cap")) { expect_eq(strEq(s, "c"), true); } else { expect_eq(true, false); }
    });

    test("6. @cap/abbrev — Empty input & empty prefix edge cases", () => {
        const dummy: [1]string = [""];
        const empty = dummy[0..0];
        let out: [8]AbbrevEntry = undefined;
        expect_eq(abbrev(empty, out[0..]), 0);
        expect_eq(resolveAbbrev(empty, "a") == null, true);
        expect_eq(shortestAbbrev(empty, "a") == null, true);

        const words: [2]string = ["alpha", "beta"];
        expect_eq(resolveAbbrev(words[0..], "") == null, true);
    });
}
