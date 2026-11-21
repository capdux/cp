// Source: cp/cap/test/test_kmp_production_deep.cp (Production deep tests for cap:kmp)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PalindromeSpan,
    computePrefixFunction,
    kmpFindFirst,
    kmpFindAll,
    kmpCount,
    shortestPeriod,
    computeZArray,
    manacherLongestPalindrome,
    countPalindromicSubstrings,
} from "cap:kmp";

fn bruteLcp(s: []const u8, i: usize): usize {
    let k: usize = 0;
    while (i + k < s.len && s[k] == s[i + k]) {
        k += 1;
    }
    return k;
}

fn isPalindrome(s: []const u8, l: usize, r: usize): bool {
    let lo = l;
    let hi = r;
    while (lo < hi) {
        if (s[lo] != s[hi]) {
            return false;
        }
        lo += 1;
        hi -= 1;
    }
    return true;
}

fn bruteCountPalindromes(s: []const u8): usize {
    let cnt: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        let j = i;
        while (j < s.len) {
            if (isPalindrome(s, i, j)) {
                cnt += 1;
            }
            j += 1;
        }
        i += 1;
    }
    return cnt;
}

test("1. KMP prefix function, overlapping matches, kmpCount & output buffer truncation safety", () => {
    // Prefix table for "ababcabab": [0, 0, 1, 2, 0, 1, 2, 3, 4]
    const p1 = "ababcabab";
    let pi1: [9]usize = std.mem.zeroes([9]usize);
    expect_eq(computePrefixFunction(p1, pi1[0..]), true);
    const expected_pi1: [9]usize = [0, 0, 1, 2, 0, 1, 2, 3, 4];
    let i: usize = 0;
    while (i < 9) {
        expect_eq(pi1[i], expected_pi1[i]);
        i += 1;
    }

    // Overlapping match: "aaa" in "aaaaa" -> [0, 1, 2]
    const t2 = "aaaaa";
    const p2 = "aaa";
    let pi2: [3]usize = std.mem.zeroes([3]usize);
    expect_eq(computePrefixFunction(p2, pi2[0..]), true);
    expect_eq(kmpFindFirst(t2, p2, pi2[0..]) ?? unreachable, 0 as usize);
    expect_eq(kmpCount(t2, p2, pi2[0..]), 3 as usize);

    let m2: [8]usize = std.mem.zeroes([8]usize);
    const n2 = kmpFindAll(t2, p2, pi2[0..], m2[0..]);
    expect_eq(n2, 3 as usize);
    expect_eq(m2[0], 0 as usize);
    expect_eq(m2[1], 1 as usize);
    expect_eq(m2[2], 2 as usize);

    // Truncated output buffer writes safely up to capacity
    let m2_small: [2]usize = std.mem.zeroes([2]usize);
    expect_eq(kmpFindAll(t2, p2, pi2[0..], m2_small[0..]), 2 as usize);
    expect_eq(m2_small[0], 0 as usize);
    expect_eq(m2_small[1], 1 as usize);

    // Overlapping match: "aba" in "abababa" -> [0, 2, 4]
    const t3 = "abababa";
    const p3 = "aba";
    let pi3: [3]usize = std.mem.zeroes([3]usize);
    expect_eq(computePrefixFunction(p3, pi3[0..]), true);
    expect_eq(kmpCount(t3, p3, pi3[0..]), 3 as usize);
    const n3 = kmpFindAll(t3, p3, pi3[0..], m2[0..]);
    expect_eq(n3, 3 as usize);
    expect_eq(m2[0], 0 as usize);
    expect_eq(m2[1], 2 as usize);
    expect_eq(m2[2], 4 as usize);
});

test("2. shortestPeriod on periodic, partially-periodic & aperiodic strings", () => {
    let pi: [16]usize = std.mem.zeroes([16]usize);

    const s1 = "abcabcabc";
    expect_eq(computePrefixFunction(s1, pi[0..]), true);
    expect_eq(shortestPeriod(s1, pi[0..]), 3 as usize);

    const s2 = "ababab";
    expect_eq(computePrefixFunction(s2, pi[0..]), true);
    expect_eq(shortestPeriod(s2, pi[0..]), 2 as usize);

    const s3 = "zzzzz";
    expect_eq(computePrefixFunction(s3, pi[0..]), true);
    expect_eq(shortestPeriod(s3, pi[0..]), 1 as usize);

    const s4 = "ababa";
    expect_eq(computePrefixFunction(s4, pi[0..]), true);
    expect_eq(shortestPeriod(s4, pi[0..]), 2 as usize);

    const s5 = "abcdefg";
    expect_eq(computePrefixFunction(s5, pi[0..]), true);
    expect_eq(shortestPeriod(s5, pi[0..]), 7 as usize);
});

test("3. Z-Algorithm verified element-by-element against brute-force LCP", () => {
    let z: [32]usize = std.mem.zeroes([32]usize);

    const s1 = "aabcaabxaaaz";
    expect_eq(computeZArray(s1, z[0..]), true);
    let i: usize = 0;
    while (i < s1.len) {
        expect_eq(z[i], bruteLcp(s1, i));
        i += 1;
    }

    const s2 = "abacabadabacaba";
    expect_eq(computeZArray(s2, z[0..]), true);
    i = 0;
    while (i < s2.len) {
        expect_eq(z[i], bruteLcp(s2, i));
        i += 1;
    }

    const s3 = "aaaaaaaaaa";
    expect_eq(computeZArray(s3, z[0..]), true);
    i = 0;
    while (i < s3.len) {
        expect_eq(z[i], bruteLcp(s3, i));
        i += 1;
    }
});

test("4. Manacher odd/even longest palindrome & exact palindromic substring counting", () => {
    let d1: [32]usize = std.mem.zeroes([32]usize);
    let d2: [32]usize = std.mem.zeroes([32]usize);

    // Odd palindrome: "xyzracecar12" -> "racecar" at [3..10] (len 7)
    const s_odd = "xyzracecar12";
    const sp_odd = manacherLongestPalindrome(s_odd, d1[0..], d2[0..]) ?? unreachable;
    expect_eq(sp_odd.start, 3 as usize);
    expect_eq(sp_odd.len, 7 as usize);
    expect_eq(countPalindromicSubstrings(s_odd, d1[0..], d2[0..]) ?? unreachable, bruteCountPalindromes(s_odd));

    // Even palindrome: "pqabccba99" -> "abccba" at [2..8] (len 6)
    const s_even = "pqabccba99";
    const sp_even = manacherLongestPalindrome(s_even, d1[0..], d2[0..]) ?? unreachable;
    expect_eq(sp_even.start, 2 as usize);
    expect_eq(sp_even.len, 6 as usize);
    expect_eq(countPalindromicSubstrings(s_even, d1[0..], d2[0..]) ?? unreachable, bruteCountPalindromes(s_even));

    // All-identical string of length 6 -> longest len 6, total palindromic substrings = 6*7/2 = 21
    const s_all = "aaaaaa";
    const sp_all = manacherLongestPalindrome(s_all, d1[0..], d2[0..]) ?? unreachable;
    expect_eq(sp_all.start, 0 as usize);
    expect_eq(sp_all.len, 6 as usize);
    expect_eq(countPalindromicSubstrings(s_all, d1[0..], d2[0..]) ?? unreachable, 21 as usize);

    // Mixed complex string cross-checked against brute-force count
    const s_mix = "abacabaabba";
    expect_eq(countPalindromicSubstrings(s_mix, d1[0..], d2[0..]) ?? unreachable, bruteCountPalindromes(s_mix));
});

test("5. Arbitrary binary byte slices containing 0x00 and 0xFF (sentinel-free safety)", () => {
    const bin: [10]u8 = [0x00, 0xFF, 0x00, 0x7F, 0xFF, 0xFF, 0x7F, 0x00, 0xFF, 0x01];
    let d1: [10]usize = std.mem.zeroes([10]usize);
    let d2: [10]usize = std.mem.zeroes([10]usize);

    // Longest palindrome is bin[1..9] = [0xFF, 0x00, 0x7F, 0xFF, 0xFF, 0x7F, 0x00, 0xFF] (len 8)
    const sp = manacherLongestPalindrome(bin[0..], d1[0..], d2[0..]) ?? unreachable;
    expect_eq(sp.start, 1 as usize);
    expect_eq(sp.len, 8 as usize);
    expect_eq(countPalindromicSubstrings(bin[0..], d1[0..], d2[0..]) ?? unreachable, bruteCountPalindromes(bin[0..]));

    // KMP on binary pattern [0x00, 0xFF] -> matches at indices 0 and 7
    const pat: [2]u8 = [0x00, 0xFF];
    let pi: [2]usize = std.mem.zeroes([2]usize);
    expect_eq(computePrefixFunction(pat[0..], pi[0..]), true);
    let matches: [4]usize = std.mem.zeroes([4]usize);
    const cnt = kmpFindAll(bin[0..], pat[0..], pi[0..], matches[0..]);
    expect_eq(cnt, 2 as usize);
    expect_eq(matches[0], 0 as usize);
    expect_eq(matches[1], 7 as usize);

    // Z-Array on binary slice
    let z: [10]usize = std.mem.zeroes([10]usize);
    expect_eq(computeZArray(bin[0..], z[0..]), true);
    let i: usize = 0;
    while (i < 10) {
        expect_eq(z[i], bruteLcp(bin[0..], i));
        i += 1;
    }
});

test("6. Boundary guards: empty strings, pattern longer than text, not found & undersized buffers", () => {
    const empty: [0]u8 = [];
    const text = "hello";
    const long_pat = "hello_world";
    let pi_small: [2]usize = std.mem.zeroes([2]usize);
    let pi_ok: [16]usize = std.mem.zeroes([16]usize);
    let matches: [4]usize = std.mem.zeroes([4]usize);

    // Empty pattern & empty text
    expect_eq(computePrefixFunction(empty[0..], pi_small[0..]), true);
    expect_eq(kmpFindFirst(text, empty[0..], pi_small[0..]) ?? unreachable, 0 as usize);
    expect_eq(kmpCount(text, empty[0..], pi_small[0..]), 0 as usize);
    expect_eq(shortestPeriod(empty[0..], pi_small[0..]), 0 as usize);

    // Pattern longer than text & not found
    expect_eq(computePrefixFunction(long_pat, pi_ok[0..]), true);
    expect_eq(kmpFindFirst(text, long_pat, pi_ok[0..]) == null, true);
    expect_eq(kmpFindAll(text, long_pat, pi_ok[0..], matches[0..]), 0 as usize);

    const missing = "xyz";
    expect_eq(computePrefixFunction(missing, pi_ok[0..]), true);
    expect_eq(kmpFindFirst(text, missing, pi_ok[0..]) == null, true);

    // Undersized buffers rejected
    expect_eq(computePrefixFunction(text, pi_small[0..]), false);
    expect_eq(kmpFindFirst(text, text, pi_small[0..]) == null, true);
    expect_eq(computeZArray(text, pi_small[0..]), false);
    expect_eq(computeZArray(empty[0..], pi_small[0..]), true);

    // Manacher empty & undersized scratch
    const sp_empty = manacherLongestPalindrome(empty[0..], pi_small[0..], pi_small[0..]) ?? unreachable;
    expect_eq(sp_empty.start, 0 as usize);
    expect_eq(sp_empty.len, 0 as usize);
    expect_eq(countPalindromicSubstrings(empty[0..], pi_small[0..], pi_small[0..]) ?? unreachable, 0 as usize);
    expect_eq(manacherLongestPalindrome(text, pi_small[0..], pi_ok[0..]) == null, true);
    expect_eq(manacherLongestPalindrome(text, pi_ok[0..], pi_small[0..]) == null, true);
    expect_eq(countPalindromicSubstrings(text, pi_small[0..], pi_ok[0..]) == null, true);
});
