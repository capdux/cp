// cap/test/testCapNaturalCmp.cp
// Comprehensive unit tests for @cap/natural-cmp (Natural Sort Order)

import { test, expect_eq } from "cap:test";
import {
    compareNatural,
    compareNaturalIgnoreCase,
    isSortedNatural,
    sortNatural,
} from "../../packages/natord/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/natural-cmp — file2 < file10 & version v1.2.10 > v1.2.2", () => {
        expect_eq(compareNatural("file2.txt", "file10.txt"), -1 as i32);
        expect_eq(compareNatural("file10.txt", "file2.txt"), 1 as i32);
        expect_eq(compareNatural("v1.2.10", "v1.2.2"), 1 as i32);
        expect_eq(compareNatural("v1.2", "v1.10"), -1 as i32);
    });

    test("2. @cap/natural-cmp — 30-digit huge numbers compare without overflow", () => {
        const a = "id_999999999999999999999999999998";
        const b = "id_999999999999999999999999999999";
        const c = "id_1000000000000000000000000000000";
        expect_eq(compareNatural(a, b), -1 as i32);
        expect_eq(compareNatural(b, c), -1 as i32);
        expect_eq(compareNatural(c, a), 1 as i32);
    });

    test("3. @cap/natural-cmp — leading zeros '1' < '01' < '02'", () => {
        expect_eq(compareNatural("item1", "item01"), -1 as i32);
        expect_eq(compareNatural("item01", "item2"), -1 as i32);
        expect_eq(compareNatural("item01", "item02"), -1 as i32);
        expect_eq(compareNatural("item00", "item0"), 1 as i32);
    });

    test("4. @cap/natural-cmp — compareNaturalIgnoreCase", () => {
        expect_eq(compareNaturalIgnoreCase("File2.txt", "file10.txt"), -1 as i32);
        expect_eq(compareNaturalIgnoreCase("ALPHA10", "alpha10"), 0 as i32);
        expect_eq(compareNaturalIgnoreCase("beta2", "ALPHA100"), 1 as i32);
    });

    test("5. @cap/natural-cmp — sortNatural & isSortedNatural in-place", () => {
        let items: [6]string = [
            "img12.png",
            "img2.png",
            "img1.png",
            "img10.png",
            "img01.png",
            "img20.png",
        ];
        expect_eq(isSortedNatural(items[0..6]), false);
        sortNatural(items[0..6]);
        expect_eq(isSortedNatural(items[0..6]), true);
        expect_eq(sliceEq(items[0], "img1.png"), true);
        expect_eq(sliceEq(items[1], "img01.png"), true);
        expect_eq(sliceEq(items[2], "img2.png"), true);
        expect_eq(sliceEq(items[3], "img10.png"), true);
        expect_eq(sliceEq(items[4], "img12.png"), true);
        expect_eq(sliceEq(items[5], "img20.png"), true);
    });

    test("6. @cap/natural-cmp — empty strings & equal strings edge cases", () => {
        expect_eq(compareNatural("", ""), 0 as i32);
        expect_eq(compareNatural("", "a"), -1 as i32);
        expect_eq(compareNatural("a", ""), 1 as i32);
        expect_eq(compareNatural("same123", "same123"), 0 as i32);

        let one: [1]string = ["solo"];
        expect_eq(isSortedNatural(one[0..1]), true);
        sortNatural(one[0..1]);
        expect_eq(isSortedNatural(one[0..0]), true);
    });
}
