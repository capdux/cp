// cap/test/testCapBmhSearch.cp
// Comprehensive unit tests for @cap/bmh-search (Boyer-Moore-Horspool)

import { test, expect_eq } from "cap:test";
import {
    BmhPattern,
    compilePattern,
    findWithCompiled,
    findFirst,
    findLast,
    countNonOverlapping,
    findIgnoreCaseAscii,
} from "../../packages/@cap/bmh-search/src/index.cp";

fn main() {
    test("1. @cap/bmh-search — compilePattern reused across multiple haystacks", () => {
        const needle = "capdux";
        const pat: BmhPattern = compilePattern(needle);
        expect_eq(pat.pat_len, 6 as usize);

        expect_eq(findWithCompiled("welcome to capdux registry", needle, pat), 11 as i64);
        expect_eq(findWithCompiled("capdux at start", needle, pat), 0 as i64);
        expect_eq(findWithCompiled("no match here", needle, pat), -1 as i64);
    });

    test("2. @cap/bmh-search — findFirst start, middle, end hits & misses", () => {
        expect_eq(findFirst("abcdefg", "abc"), 0 as i64);
        expect_eq(findFirst("abcdefg", "cde"), 2 as i64);
        expect_eq(findFirst("abcdefg", "efg"), 4 as i64);
        expect_eq(findFirst("abcdefg", "xyz"), -1 as i64);
        expect_eq(findFirst("abracadabra", "abra"), 0 as i64);
    });

    test("3. @cap/bmh-search — findLast reverse search", () => {
        expect_eq(findLast("abracadabra", "abra"), 7 as i64);
        expect_eq(findLast("one two one three one", "one"), 18 as i64);
        expect_eq(findLast("hello", "world"), -1 as i64);
        expect_eq(findLast("aaaaa", "a"), 4 as i64);
    });

    test("4. @cap/bmh-search — countNonOverlapping occurrences", () => {
        expect_eq(countNonOverlapping("aaaa", "aa"), 2 as usize);
        expect_eq(countNonOverlapping("aaaaa", "aa"), 2 as usize);
        expect_eq(countNonOverlapping("bananana", "na"), 3 as usize);
        expect_eq(countNonOverlapping("abcdef", "xyz"), 0 as usize);
    });

    test("5. @cap/bmh-search — findIgnoreCaseAscii case-insensitive search", () => {
        expect_eq(findIgnoreCaseAscii("Content-Type: Application/JSON", "application/json"), 14 as i64);
        expect_eq(findIgnoreCaseAscii("Hello World", "WORLD"), 6 as i64);
        expect_eq(findIgnoreCaseAscii("Hello World", "MISSING"), -1 as i64);
    });

    test("6. @cap/bmh-search — empty needle & needle longer than haystack edge cases", () => {
        expect_eq(findFirst("abc", ""), 0 as i64);
        expect_eq(findLast("abc", ""), 3 as i64);
        expect_eq(countNonOverlapping("abc", ""), 0 as usize);
        expect_eq(findIgnoreCaseAscii("abc", ""), 0 as i64);

        expect_eq(findFirst("ab", "abcd"), -1 as i64);
        expect_eq(findLast("ab", "abcd"), -1 as i64);
        expect_eq(countNonOverlapping("ab", "abcd"), 0 as usize);
        expect_eq(findIgnoreCaseAscii("ab", "abcd"), -1 as i64);
    });
}
