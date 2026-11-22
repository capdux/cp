// Origin: Pure-CP Stdlib Test Suite (lib/test_runner/expectMatch.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Bun/Jest-Compatible Expect Matchers:
//   1. equalIgnoringWhitespace & equalIgnoringWhitespacePos (multiline, control whitespace, mismatch pos)
//   2. collapseWhitespace (trim + collapse runs + buffer-too-small safety guard)
//   3. startsWith, endsWith, includesSubstring, countOccurrences (non-overlapping), includesRepeated
//   4. isCloseTo floating-point tolerance (0.1 + 0.2 vs 0.3, positive/negative num_digits, NaN guard)
//   5. isWithinI64 / isWithinF64 half-open [start, end) bounds & negative parity (isEvenI64/isOddI64)
//   6. Stateful ExpectMatcher positive & negated (.not) tracking with assertion/failure counters

import { test, expect_eq, expect_true } from "cap:test";
import {
    isAsciiWhitespace,
    equalIgnoringWhitespace,
    equalIgnoringWhitespacePos,
    collapseWhitespace,
    startsWith,
    endsWith,
    includesSubstring,
    countOccurrences,
    includesRepeated,
    isCloseTo,
    isWithinI64,
    isWithinF64,
    isEvenI64,
    isOddI64,
    ExpectMatcher,
} from "../../lib/test_runner/expectMatch.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: equalIgnoringWhitespace & mismatch offset reporting
// ============================================================================
test("expectMatch: equalIgnoringWhitespace across multiline text and mismatch offset", () => {
    expect_true(isAsciiWhitespace(' '));
    expect_true(isAsciiWhitespace('\t'));
    expect_true(isAsciiWhitespace('\n'));
    expect_true(isAsciiWhitespace('\r'));
    expect_true(!isAsciiWhitespace('a'));

    expect_true(equalIgnoringWhitespace("  hello \n\t world  ", "helloworld"));
    expect_true(equalIgnoringWhitespace("<div>\n  <span>ok</span>\n</div>", "<div><span>ok</span></div>"));
    expect_true(equalIgnoringWhitespace("   \t\r\n  ", ""));
    expect_true(!equalIgnoringWhitespace("abc", "abcd"));
    expect_true(!equalIgnoringWhitespace("abcd", "abc"));

    const ok = equalIgnoringWhitespacePos("  a b c ", "abc");
    expect_true(ok.passed);
    expect_true(ok.effectivePass());

    const diff = equalIgnoringWhitespacePos("  a x c", "abc");
    expect_true(!diff.passed);
    expect_eq(diff.mismatch_pos, 4); // 'x' is at byte offset 4
});

// ============================================================================
// Test 2: collapseWhitespace & small-buffer protection
// ============================================================================
test("expectMatch: collapseWhitespace trims, collapses runs, and guards tiny buffers", () => {
    let buf: [64]u8 = [0; 64];
    if (s := collapseWhitespace("  foo\t\n  bar   baz  ", buf[0..])) {
        expect_true(strEq(s, "foo bar baz"));
    } else {
        expect_true(false);
    }

    if (empty := collapseWhitespace("  \t\r\n ", buf[0..])) {
        expect_eq(empty.len, 0);
    } else {
        expect_true(false);
    }

    let tiny: [4]u8 = [0; 4];
    expect_true(collapseWhitespace("hello world", tiny[0..]) == null);
});

// ============================================================================
// Test 3: Prefix, suffix, substring, and non-overlapping repeat matchers
// ============================================================================
test("expectMatch: startsWith, endsWith, includesSubstring & includesRepeated", () => {
    expect_true(startsWith("bun:test", "bun:"));
    expect_true(!startsWith("bun:test", "node:"));
    expect_true(startsWith("abc", ""));
    expect_true(!startsWith("ab", "abc"));

    expect_true(endsWith("foo.test.cp", ".test.cp"));
    expect_true(!endsWith("foo.test.cp", ".spec.cp"));
    expect_true(endsWith("abc", ""));

    expect_true(includesSubstring("hello world", "world"));
    expect_true(includesSubstring("hello", ""));
    expect_true(!includesSubstring("hello", "xyz"));

    // Non-overlapping count: "aa" appears twice in "aaaa" and twice in "aaaaa"
    expect_eq(countOccurrences("aaaa", "aa"), 2);
    expect_eq(countOccurrences("aaaaa", "aa"), 2);
    expect_eq(countOccurrences("abc", ""), 0);

    expect_true(includesRepeated("ha-ha-ha", "ha", 3));
    expect_true(!includesRepeated("ha-ha-ha", "ha", 2));
    expect_true(includesRepeated("abc", "", 0));
});

// ============================================================================
// Test 4: Floating-point isCloseTo (Jest/Bun precision semantics)
// ============================================================================
test("expectMatch: isCloseTo handles 0.1 + 0.2, num_digits precision & NaN", () => {
    expect_true(isCloseTo(0.1 + 0.2, 0.3, 2));
    expect_true(isCloseTo(0.1 + 0.2, 0.3, 10));

    // Default num_digits=2 -> tolerance < 0.005
    expect_true(isCloseTo(1.004, 1.0, 2));
    expect_true(!isCloseTo(1.006, 1.0, 2));

    // Negative num_digits=-1 -> tolerance < 5.0
    expect_true(isCloseTo(104.0, 100.0, -1));
    expect_true(!isCloseTo(106.0, 100.0, -1));

    // NaN never matches
    const zero: f64 = 0.0;
    const nan_val: f64 = zero / zero;
    expect_true(!isCloseTo(nan_val, 0.0, 2));
    expect_true(!isCloseTo(0.0, nan_val, 2));
    expect_true(!isCloseTo(nan_val, nan_val, 2));
});

// ============================================================================
// Test 5: Half-open toBeWithin [start, end) & signed parity matchers
// ============================================================================
test("expectMatch: isWithinI64/F64 half-open bounds & isEven/isOdd parity", () => {
    expect_true(isWithinI64(10, 10, 20));
    expect_true(isWithinI64(19, 10, 20));
    expect_true(!isWithinI64(9, 10, 20));
    expect_true(!isWithinI64(20, 10, 20));

    expect_true(isWithinF64(0.5, 0.0, 1.0));
    expect_true(isWithinF64(0.0, 0.0, 1.0));
    expect_true(!isWithinF64(1.0, 0.0, 1.0));

    const zero: f64 = 0.0;
    const nan_val: f64 = zero / zero;
    expect_true(!isWithinF64(nan_val, 0.0, 1.0));

    expect_true(isEvenI64(0));
    expect_true(isEvenI64(-42));
    expect_true(!isEvenI64(-7));
    expect_true(isOddI64(13));
    expect_true(isOddI64(-13));
    expect_true(!isOddI64(8));
});

// ============================================================================
// Test 6: Stateful ExpectMatcher with .not negation & counters
// ============================================================================
test("expectMatch: ExpectMatcher tracks positive/negated assertions and failures", () => {
    let m = ExpectMatcher.init();
    expect_true(m.toEqualIgnoringWhitespace("a \n b", "ab"));
    expect_true(m.toStartWith("cap:test", "cap:"));
    expect_true(m.toEndWith("cap:test", ":test"));
    expect_true(m.toInclude("hello cp", "cp"));
    expect_true(m.toIncludeRepeated("x_x_x", "x", 3));
    expect_true(m.toBeCloseTo(0.1 + 0.2, 0.3, 5));
    expect_true(m.toBeWithinI64(5, 1, 10));
    expect_true(m.toBeEven(4));
    expect_true(m.toBeOdd(5));
    expect_eq(m.assertionCount(), 9);
    expect_eq(m.failureCount(), 0);

    // Negated mode (.not)
    let neg = ExpectMatcher.negatedMatcher();
    expect_true(neg.toEqualIgnoringWhitespace("abc", "xyz"));
    expect_true(!neg.toBeEven(10)); // 10 IS even, so `.not.toBeEven(10)` fails
    expect_eq(neg.assertionCount(), 2);
    expect_eq(neg.failureCount(), 1);
});
