import { test, expect_eq } from "cap:test";
import {
    ccountByte,
    ccount,
    longestStreakByte,
    longestStreak,
} from "../../packages/ccount/src/index.cp";

fn main() {
    test("1. @cap/ccount — counts single-byte occurrences", () => {
        expect_eq(ccountByte("a\nb\nc\n", 10), 3 as usize);
        expect_eq(ccount("banana", "a"), 3 as usize);
        expect_eq(ccount("banana", "z"), 0 as usize);
    });

    test("2. @cap/ccount — counts non-overlapping multi-byte substrings", () => {
        expect_eq(ccount("aaaa", "aa"), 2 as usize);
        expect_eq(ccount("foo-bar-foo-baz-foo", "foo"), 3 as usize);
        expect_eq(ccount("short", "longer-needle"), 0 as usize);
    });

    test("3. @cap/ccount — handles empty inputs safely", () => {
        expect_eq(ccount("", "a"), 0 as usize);
        expect_eq(ccount("abc", ""), 0 as usize);
        expect_eq(longestStreak("", "`"), 0 as usize);
        expect_eq(longestStreak("```", ""), 0 as usize);
    });

    test("4. @cap/ccount — longestStreak finds longest markdown backtick fence", () => {
        expect_eq(longestStreak("`a` ```code``` ``b``", "`"), 3 as usize);
        expect_eq(longestStreak("no ticks here", "`"), 0 as usize);
        expect_eq(longestStreakByte("##### heading ## sub", 35), 5 as usize);
    });

    test("5. @cap/ccount — longestStreak supports multi-byte repeat sequences", () => {
        expect_eq(longestStreak("ab ababab cd abab", "ab"), 3 as usize);
        expect_eq(longestStreak("xyz", "ab"), 0 as usize);
    });

    test("6. @cap/ccount — streak at start and end of string", () => {
        expect_eq(longestStreak("----mid--", "-"), 4 as usize);
        expect_eq(longestStreak("--mid------", "-"), 6 as usize);
    });
}
