// Source: cp/cap/test/test_suffix_production_deep.cp (Production deep tests for cap:suffix)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SuffixWorkspace,
    buildSuffixArray,
    buildLcpArray,
    findPatternRange,
    countDistinctSubstrings,
    longestRepeatedSubstring,
} from "cap:ds";

fn bytesEqual(a: []const u8, b: []const u8): bool {
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

fn suffixStrictlyLess(text: []const u8, a: usize, b: usize): bool {
    let i: usize = 0;
    while (a + i < text.len && b + i < text.len) {
        if (text[a + i] != text[b + i]) {
            return text[a + i] < text[b + i];
        }
        i += 1;
    }
    return a > b;
}

fn bruteCountDistinct(text: []const u8): u64 {
    const n = text.len;
    let count: u64 = 0;
    let i: usize = 0;
    while (i < n) {
        let len: usize = 1;
        while (i + len <= n) {
            const sub = text[i .. i + len];
            // Count only first occurrence
            let seen_before = false;
            let p: usize = 0;
            while (p < i) {
                if (bytesEqual(text[p .. p + len], sub)) {
                    seen_before = true;
                    break;
                }
                p += 1;
            }
            if (!seen_before) {
                count += 1;
            }
            len += 1;
        }
        i += 1;
    }
    return count;
}

test("1. Classic 'banana' & 'mississippi' Suffix Array and Kasai LCP exact verification", () => {
    let rank_b: [16]usize = std.mem.zeroes([16]usize);
    let tmp_b: [16]usize = std.mem.zeroes([16]usize);
    let ord_b: [16]usize = std.mem.zeroes([16]usize);
    let cnt_b: [260]usize = std.mem.zeroes([260]usize);
    let ws = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_b[0..],
        order: ord_b[0..],
    };

    // "banana": SA = [5, 3, 1, 0, 4, 2], LCP = [0, 1, 3, 0, 0, 2]
    const banana: []const u8 = "banana";
    let sa1: [6]usize = std.mem.zeroes([6]usize);
    let lcp1: [6]usize = std.mem.zeroes([6]usize);
    let sa1_s: []usize = sa1[0..];
    let lcp1_s: []usize = lcp1[0..];
    let rk_s: []usize = rank_b[0..];

    expect_eq(buildSuffixArray(banana, &sa1_s, &ws), true);
    expect_eq(buildLcpArray(banana, sa1[0..], &lcp1_s, &rk_s), true);

    const exp_sa1: [6]usize = [5, 3, 1, 0, 4, 2];
    const exp_lcp1: [6]usize = [0, 1, 3, 0, 0, 2];
    let i: usize = 0;
    while (i < 6) {
        expect_eq(sa1[i], exp_sa1[i]);
        expect_eq(lcp1[i], exp_lcp1[i]);
        i += 1;
    }

    // "mississippi": SA = [10, 7, 4, 1, 0, 9, 8, 6, 3, 5, 2], LCP = [0, 1, 1, 4, 0, 0, 1, 0, 2, 1, 3]
    const miss: []const u8 = "mississippi";
    let sa2: [11]usize = std.mem.zeroes([11]usize);
    let lcp2: [11]usize = std.mem.zeroes([11]usize);
    let sa2_s: []usize = sa2[0..];
    let lcp2_s: []usize = lcp2[0..];

    expect_eq(buildSuffixArray(miss, &sa2_s, &ws), true);
    expect_eq(buildLcpArray(miss, sa2[0..], &lcp2_s, &rk_s), true);

    const exp_sa2: [11]usize = [10, 7, 4, 1, 0, 9, 8, 6, 3, 5, 2];
    const exp_lcp2: [11]usize = [0, 1, 1, 4, 0, 0, 1, 0, 2, 1, 3];
    i = 0;
    while (i < 11) {
        expect_eq(sa2[i], exp_sa2[i]);
        expect_eq(lcp2[i], exp_lcp2[i]);
        i += 1;
    }
});

test("2. findPatternRange: hits, misses, single-byte & empty pattern on 'mississippi'", () => {
    const miss: []const u8 = "mississippi";
    let rank_b: [16]usize = std.mem.zeroes([16]usize);
    let tmp_b: [16]usize = std.mem.zeroes([16]usize);
    let ord_b: [16]usize = std.mem.zeroes([16]usize);
    let cnt_b: [260]usize = std.mem.zeroes([260]usize);
    let ws = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_b[0..],
        order: ord_b[0..],
    };

    let sa: [11]usize = std.mem.zeroes([11]usize);
    let sa_s: []usize = sa[0..];
    expect_eq(buildSuffixArray(miss, &sa_s, &ws), true);

    // "issi" appears twice at indices 1 and 4
    const r_issi = findPatternRange(miss, sa[0..], "issi");
    expect_eq(r_issi.count, 2 as usize);
    expect_eq(r_issi.end - r_issi.start, 2 as usize);
    let k = r_issi.start;
    while (k < r_issi.end) {
        const pos = sa[k];
        expect_eq(bytesEqual(miss[pos .. pos + 4], "issi"), true);
        k += 1;
    }

    // "i" appears 4 times, "ss" appears 2 times, "xyz" appears 0 times, "" matches all 11
    expect_eq(findPatternRange(miss, sa[0..], "i").count, 4 as usize);
    expect_eq(findPatternRange(miss, sa[0..], "ss").count, 2 as usize);
    expect_eq(findPatternRange(miss, sa[0..], "xyz").count, 0 as usize);
    expect_eq(findPatternRange(miss, sa[0..], "").count, 11 as usize);
});

test("3. countDistinctSubstrings matches brute-force deduplication across diverse strings", () => {
    let rank_b: [16]usize = std.mem.zeroes([16]usize);
    let tmp_b: [16]usize = std.mem.zeroes([16]usize);
    let ord_b: [16]usize = std.mem.zeroes([16]usize);
    let cnt_b: [260]usize = std.mem.zeroes([260]usize);
    let ws = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_b[0..],
        order: ord_b[0..],
    };
    let sa: [16]usize = std.mem.zeroes([16]usize);
    let lcp: [16]usize = std.mem.zeroes([16]usize);
    let sa_s: []usize = sa[0..];
    let lcp_s: []usize = lcp[0..];
    let rk_s: []usize = rank_b[0..];

    const s1: []const u8 = "banana";
    expect_eq(buildSuffixArray(s1, &sa_s, &ws), true);
    expect_eq(buildLcpArray(s1, sa[0..s1.len], &lcp_s, &rk_s), true);
    expect_eq(countDistinctSubstrings(s1.len, lcp[0..s1.len]), 15 as u64);
    expect_eq(countDistinctSubstrings(s1.len, lcp[0..s1.len]), bruteCountDistinct(s1));

    const s2: []const u8 = "mississippi";
    expect_eq(buildSuffixArray(s2, &sa_s, &ws), true);
    expect_eq(buildLcpArray(s2, sa[0..s2.len], &lcp_s, &rk_s), true);
    expect_eq(countDistinctSubstrings(s2.len, lcp[0..s2.len]), 53 as u64);
    expect_eq(countDistinctSubstrings(s2.len, lcp[0..s2.len]), bruteCountDistinct(s2));

    const s3: []const u8 = "ababa";
    expect_eq(buildSuffixArray(s3, &sa_s, &ws), true);
    expect_eq(buildLcpArray(s3, sa[0..s3.len], &lcp_s, &rk_s), true);
    expect_eq(countDistinctSubstrings(s3.len, lcp[0..s3.len]), bruteCountDistinct(s3));
});

test("4. longestRepeatedSubstring on uniform 'aaaaaa', periodic 'abcabcabc' & unique 'abcdef'", () => {
    let rank_b: [16]usize = std.mem.zeroes([16]usize);
    let tmp_b: [16]usize = std.mem.zeroes([16]usize);
    let ord_b: [16]usize = std.mem.zeroes([16]usize);
    let cnt_b: [260]usize = std.mem.zeroes([260]usize);
    let ws = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_b[0..],
        order: ord_b[0..],
    };
    let sa: [16]usize = std.mem.zeroes([16]usize);
    let lcp: [16]usize = std.mem.zeroes([16]usize);
    let sa_s: []usize = sa[0..];
    let lcp_s: []usize = lcp[0..];
    let rk_s: []usize = rank_b[0..];

    // "aaaaaa" -> longest repeated substring has length 5 ("aaaaa")
    const uni: []const u8 = "aaaaaa";
    expect_eq(buildSuffixArray(uni, &sa_s, &ws), true);
    expect_eq(buildLcpArray(uni, sa[0..uni.len], &lcp_s, &rk_s), true);
    const r_uni = longestRepeatedSubstring(sa[0..uni.len], lcp[0..uni.len]);
    expect_eq(r_uni.len, 5 as usize);
    expect_eq(bytesEqual(uni[r_uni.pos .. r_uni.pos + r_uni.len], "aaaaa"), true);

    // "abcabcabc" -> longest repeated substring has length 6 ("abcabc")
    const per: []const u8 = "abcabcabc";
    expect_eq(buildSuffixArray(per, &sa_s, &ws), true);
    expect_eq(buildLcpArray(per, sa[0..per.len], &lcp_s, &rk_s), true);
    const r_per = longestRepeatedSubstring(sa[0..per.len], lcp[0..per.len]);
    expect_eq(r_per.len, 6 as usize);
    expect_eq(bytesEqual(per[r_per.pos .. r_per.pos + r_per.len], "abcabc"), true);

    // "abcdef" -> no repeated substring (len == 0)
    const uniq: []const u8 = "abcdef";
    expect_eq(buildSuffixArray(uniq, &sa_s, &ws), true);
    expect_eq(buildLcpArray(uniq, sa[0..uniq.len], &lcp_s, &rk_s), true);
    const r_uniq = longestRepeatedSubstring(sa[0..uniq.len], lcp[0..uniq.len]);
    expect_eq(r_uniq.len, 0 as usize);
});

test("5. Arbitrary binary byte strings containing 0x00 and 0xFF", () => {
    const bin_data: [10]u8 = [0x00, 0xFF, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x01, 0xFF, 0x00];
    let rank_b: [16]usize = std.mem.zeroes([16]usize);
    let tmp_b: [16]usize = std.mem.zeroes([16]usize);
    let ord_b: [16]usize = std.mem.zeroes([16]usize);
    let cnt_b: [260]usize = std.mem.zeroes([260]usize);
    let ws = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_b[0..],
        order: ord_b[0..],
    };

    let sa: [10]usize = std.mem.zeroes([10]usize);
    let lcp: [10]usize = std.mem.zeroes([10]usize);
    let sa_s: []usize = sa[0..];
    let lcp_s: []usize = lcp[0..];
    let rk_s: []usize = rank_b[0..];

    expect_eq(buildSuffixArray(bin_data[0..], &sa_s, &ws), true);
    expect_eq(buildLcpArray(bin_data[0..], sa[0..], &lcp_s, &rk_s), true);

    // Verify strict lexicographical ordering of all 10 binary suffixes
    let i: usize = 1;
    while (i < 10) {
        expect_eq(suffixStrictlyLess(bin_data[0..], sa[i - 1], sa[i]), true);
        i += 1;
    }

    // Pattern [0x00, 0xFF, 0x00, 0x01] occurs at indices 0 and 4
    const pat: [4]u8 = [0x00, 0xFF, 0x00, 0x01];
    const occ = findPatternRange(bin_data[0..], sa[0..], pat[0..]);
    expect_eq(occ.count, 2 as usize);

    // Longest repeated binary substring has length 4 ([0x00, 0xFF, 0x00, 0x01])
    const lrs = longestRepeatedSubstring(sa[0..], lcp[0..]);
    expect_eq(lrs.len, 4 as usize);
    expect_eq(countDistinctSubstrings(10, lcp[0..]), bruteCountDistinct(bin_data[0..]));
});

test("6. Defensive edge cases: empty text, single byte & undersized workspace rejection", () => {
    let rank_b: [4]usize = std.mem.zeroes([4]usize);
    let tmp_b: [4]usize = std.mem.zeroes([4]usize);
    let ord_b: [4]usize = std.mem.zeroes([4]usize);
    let cnt_short: [100]usize = std.mem.zeroes([100]usize);
    let cnt_ok: [257]usize = std.mem.zeroes([257]usize);

    let ws_bad = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_short[0..],
        order: ord_b[0..],
    };
    let ws_ok = SuffixWorkspace {
        rank: rank_b[0..],
        tmp_rank: tmp_b[0..],
        cnt: cnt_ok[0..],
        order: ord_b[0..],
    };

    let sa: [4]usize = std.mem.zeroes([4]usize);
    let lcp: [4]usize = std.mem.zeroes([4]usize);
    let sa_s: []usize = sa[0..];
    let lcp_s: []usize = lcp[0..];
    let rk_s: []usize = rank_b[0..];

    // Empty text succeeds cleanly
    const empty: []const u8 = "";
    expect_eq(buildSuffixArray(empty, &sa_s, &ws_ok), true);
    expect_eq(buildLcpArray(empty, sa[0..0], &lcp_s, &rk_s), true);
    expect_eq(findPatternRange(empty, sa[0..0], "a").count, 0 as usize);
    expect_eq(countDistinctSubstrings(0, lcp[0..0]), 0 as u64);

    // Single character "z"
    const single: []const u8 = "z";
    expect_eq(buildSuffixArray(single, &sa_s, &ws_ok), true);
    expect_eq(sa[0], 0 as usize);
    expect_eq(buildLcpArray(single, sa[0..1], &lcp_s, &rk_s), true);
    expect_eq(lcp[0], 0 as usize);
    expect_eq(countDistinctSubstrings(1, lcp[0..1]), 1 as u64);

    // Undersized cnt (< 257) rejected
    expect_eq(buildSuffixArray("abcd", &sa_s, &ws_bad), false);

    // Undersized sa_out rejected
    let short_sa: []usize = sa[0..2];
    expect_eq(buildSuffixArray("abcd", &short_sa, &ws_ok), false);

    // Invalid out-of-range sa index rejected by buildLcpArray
    const corrupt_sa: [4]usize = [0, 1, 99, 3];
    expect_eq(buildLcpArray("abcd", corrupt_sa[0..], &lcp_s, &rk_s), false);
});
