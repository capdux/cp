// Wave 29 R9 (Lane 2B) Rust Reference Hardening Test Suite for lib/ds/ahoCorasick.cp
// Reference benchmark: BurntSushi/aho-corasick (MIT/Unlicense)
// Covers:
//   1. BurntSushi/aho-corasick src/tests.rs vectors: Standard vs LeftmostFirst vs LeftmostLongest
//   2. Overlapping, Prefix Chains, Empty Patterns & Duplicate Patterns (zero infinite loops)
//   3. ASCII Case-Insensitive Matching & Zero-Heap Single-Pass `replaceAllInto`
//   4. 4KB Long Text `cap:simd` Prefilter (1/2/3/4 start bytes) vs Pure Automaton 100% Equivalence
//   5. 1,300 Deterministic PRNG Differential Fuzz Cases vs Ground-Truth Naive Multi-Pattern Matcher

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MatchKind,
    AcError,
    AhoCorasickOptions,
    Match,
    AhoCorasick,
} from "../../lib/ds/ahoCorasick.cp";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

struct XorShift64 {
    state: u64 = 0x1E37_79B9_7F4A_7C15;

    fn next(*_): u64 {
        let x: u64 = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn range(*_, bound: usize): usize {
        if (bound == 0) return 0;
        return @as(usize, @intCast(_.next() % @as(u64, @intCast(bound))));
    }
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn asciiFold(b: u8, ci: bool): u8 {
    if (ci && b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn matchesAt(text: []const u8, pos: usize, pat: []const u8, ci: bool): bool {
    if (pos + pat.len > text.len) return false;
    let i: usize = 0;
    while (i < pat.len) : (i += 1) {
        if (asciiFold(text[pos + i], ci) != asciiFold(pat[i], ci)) return false;
    }
    return true;
}

// Ground-truth naive reference matcher for Standard, LeftmostFirst & LeftmostLongest
fn naiveFindNext(
    patterns: []const []const u8,
    text: []const u8,
    search_start: usize,
    allow_empty_at_start: bool,
    kind: MatchKind,
    ci: bool,
): Match? {
    if (search_start > text.len) return null;

    if (kind == MatchKind.Standard) {
        // Earliest end position `end >= search_start`; ties broken by lowest pattern_id
        let end: usize = search_start;
        while (end <= text.len) : (end += 1) {
            let best_found: bool = false;
            let best_m: Match = Match{ .pattern_id = 0, .start = 0, .end = 0 };
            let pid: usize = 0;
            while (pid < patterns.len) : (pid += 1) {
                const pat = patterns[pid];
                if (pat.len == 0 && end == search_start && !allow_empty_at_start) continue;
                if (end < pat.len) continue;
                const st: usize = end - pat.len;
                if (st < search_start) continue;
                if (matchesAt(text, st, pat, ci)) {
                    best_m = Match{ .pattern_id = pid, .start = st, .end = end };
                    best_found = true;
                    break; // lowest pid wins
                }
            }
            if (best_found) return best_m;
        }
        return null;
    }

    // LeftmostFirst or LeftmostLongest: earliest start position `st >= search_start`
    let st: usize = search_start;
    while (st <= text.len) : (st += 1) {
        let best_found: bool = false;
        let best_m: Match = Match{ .pattern_id = 0, .start = 0, .end = 0 };
        let pid: usize = 0;
        while (pid < patterns.len) : (pid += 1) {
            const pat = patterns[pid];
            if (pat.len == 0 && st == search_start && !allow_empty_at_start) continue;
            if (matchesAt(text, st, pat, ci)) {
                const cand = Match{ .pattern_id = pid, .start = st, .end = st + pat.len };
                if (!best_found) {
                    best_m = cand;
                    best_found = true;
                } else if (kind == MatchKind.LeftmostFirst) {
                    // Already iterating pid ascending, so first match at `st` wins
                } else {
                    if (cand.end > best_m.end) {
                        best_m = cand;
                    }
                }
            }
        }
        if (best_found) return best_m;
    }
    return null;
}

fn naiveFindAllNonOverlapping(
    patterns: []const []const u8,
    text: []const u8,
    kind: MatchKind,
    ci: bool,
    out: *[]Match,
): usize {
    let count: usize = 0;
    let pos: usize = 0;
    let allow_empty: bool = true;
    while (pos <= text.len && count < out.len) {
        const m_opt: Match? = naiveFindNext(patterns, text, pos, allow_empty, kind, ci);
        if (m_opt == null) break;
        if (m := m_opt) {
            out[count] = m;
            count += 1;
            if (m.end > m.start) {
                pos = m.end;
                allow_empty = false;
            } else {
                if (m.end >= text.len) break;
                pos = m.end + 1;
                allow_empty = true;
            }
        }
    }
    return count;
}

fn naiveCountOverlapping(
    patterns: []const []const u8,
    text: []const u8,
    ci: bool,
): usize {
    let total: usize = 0;
    let end: usize = 0;
    while (end <= text.len) : (end += 1) {
        let pid: usize = 0;
        while (pid < patterns.len) : (pid += 1) {
            const pat = patterns[pid];
            if (end >= pat.len && matchesAt(text, end - pat.len, pat, ci)) {
                total += 1;
            }
        }
    }
    return total;
}

fn assertMatchesEq(
    alloc: std.mem.Allocator,
    patterns: []const []const u8,
    text: []const u8,
    kind: MatchKind,
    expected: []const Match,
): void! {
    let ac = AhoCorasick.initWithOptions(alloc, patterns, AhoCorasickOptions{
        match_kind: kind,
        ascii_case_insensitive: false,
        enable_prefilter: true,
        ignore_empty_patterns: false,
    }) catch { expect_true(false)?; return; };
    defer ac.deinit(alloc);

    let out_buf: [64]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 64];
    let out_s: []Match = out_buf[0..];
    const n: usize = ac.findAllNonOverlapping(text, out_s);
    expect_eq(n, expected.len);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        expect_eq(out_buf[i].pattern_id, expected[i].pattern_id);
        expect_eq(out_buf[i].start, expected[i].start);
        expect_eq(out_buf[i].end, expected[i].end);
    }
}

// ============================================================================
// Test 1: BurntSushi/aho-corasick src/tests.rs (Standard / LeftmostFirst / LeftmostLongest)
// ============================================================================
fn runTestBurntSushiMatchKinds(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Standard vs LeftmostFirst vs LeftmostLongest on ["ab", "abcd"] / ["abcd", "ab"]
    const p_ab_abcd: [2][]const u8 = ["ab", "abcd"];
    const p_abcd_ab: [2][]const u8 = ["abcd", "ab"];
    const e_0_0_2: [1]Match = [Match{ .pattern_id = 0, .start = 0, .end = 2 }];
    const e_1_0_2: [1]Match = [Match{ .pattern_id = 1, .start = 0, .end = 2 }];
    const e_0_0_4: [1]Match = [Match{ .pattern_id = 0, .start = 0, .end = 4 }];
    const e_1_0_4: [1]Match = [Match{ .pattern_id = 1, .start = 0, .end = 4 }];

    assertMatchesEq(alloc, p_ab_abcd[0..], "abcd", MatchKind.Standard, e_0_0_2[0..])?;
    assertMatchesEq(alloc, p_abcd_ab[0..], "abcd", MatchKind.Standard, e_1_0_2[0..])?;
    assertMatchesEq(alloc, p_ab_abcd[0..], "abcd", MatchKind.LeftmostFirst, e_0_0_2[0..])?;
    assertMatchesEq(alloc, p_abcd_ab[0..], "abcd", MatchKind.LeftmostFirst, e_0_0_4[0..])?;
    assertMatchesEq(alloc, p_ab_abcd[0..], "abcd", MatchKind.LeftmostLongest, e_1_0_4[0..])?;
    assertMatchesEq(alloc, p_abcd_ab[0..], "abcd", MatchKind.LeftmostLongest, e_0_0_4[0..])?;

    // 2. standard400 vs leftlong010: ["abcd", "bcd", "cd", "b"] on "abcd"
    const p_400: [4][]const u8 = ["abcd", "bcd", "cd", "b"];
    const e_std400: [2]Match = [
        Match{ .pattern_id = 3, .start = 1, .end = 2 },
        Match{ .pattern_id = 2, .start = 2, .end = 4 },
    ];
    assertMatchesEq(alloc, p_400[0..], "abcd", MatchKind.Standard, e_std400[0..])?;
    assertMatchesEq(alloc, p_400[0..], "abcd", MatchKind.LeftmostFirst, e_0_0_4[0..])?;
    assertMatchesEq(alloc, p_400[0..], "abcd", MatchKind.LeftmostLongest, e_0_0_4[0..])?;

    // 3. leftfirst320 vs leftlong310: ["a", "abcdefghi", "hz", "abcdefgh"] on "abcdefghz"
    const p_320: [4][]const u8 = ["a", "abcdefghi", "hz", "abcdefgh"];
    const e_lf320: [2]Match = [
        Match{ .pattern_id = 0, .start = 0, .end = 1 },
        Match{ .pattern_id = 2, .start = 7, .end = 9 },
    ];
    const e_ll310: [1]Match = [Match{ .pattern_id = 3, .start = 0, .end = 8 }];
    assertMatchesEq(alloc, p_320[0..], "abcdefghz", MatchKind.LeftmostFirst, e_lf320[0..])?;
    assertMatchesEq(alloc, p_320[0..], "abcdefghz", MatchKind.LeftmostLongest, e_ll310[0..])?;

    // 4. leftfirst400: ["amwix", "samwise", "sam"] on "Zsamwix" -> (2, 1, 4)
    const p_sam: [3][]const u8 = ["amwix", "samwise", "sam"];
    const e_sam: [1]Match = [Match{ .pattern_id = 2, .start = 1, .end = 4 }];
    assertMatchesEq(alloc, p_sam[0..], "Zsamwix", MatchKind.LeftmostFirst, e_sam[0..])?;
    assertMatchesEq(alloc, p_sam[0..], "Zsamwix", MatchKind.LeftmostLongest, e_sam[0..])?;

    // 5. leftmost410: ["z", "abcdefghi", "hz", "abcdefgh"] on "abcdefghz" -> (3, 0, 8), (0, 8, 9)
    const p_410: [4][]const u8 = ["z", "abcdefghi", "hz", "abcdefgh"];
    const e_410: [2]Match = [
        Match{ .pattern_id = 3, .start = 0, .end = 8 },
        Match{ .pattern_id = 0, .start = 8, .end = 9 },
    ];
    assertMatchesEq(alloc, p_410[0..], "abcdefghz", MatchKind.LeftmostFirst, e_410[0..])?;
    assertMatchesEq(alloc, p_410[0..], "abcdefghz", MatchKind.LeftmostLongest, e_410[0..])?;

    // 6. leftfirst330 vs leftlong320: ["a", "abab"] on "abab"
    const p_abab: [2][]const u8 = ["a", "abab"];
    const e_lf_abab: [2]Match = [
        Match{ .pattern_id = 0, .start = 0, .end = 1 },
        Match{ .pattern_id = 0, .start = 2, .end = 3 },
    ];
    assertMatchesEq(alloc, p_abab[0..], "abab", MatchKind.LeftmostFirst, e_lf_abab[0..])?;
    assertMatchesEq(alloc, p_abab[0..], "abab", MatchKind.LeftmostLongest, e_1_0_4[0..])?;
}

// ============================================================================
// Test 2: Overlapping, Prefix Chains, Empty & Duplicate Patterns
// ============================================================================
fn runTestOverlappingEmptyDup(): void! {
    const alloc = std.heap.page_allocator;
    let out_buf: [64]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 64];
    let out_s: []Match = out_buf[0..];

    // 1. Classic ["he", "she", "his", "hers"] on "ushers"
    const p_ushers: [4][]const u8 = ["he", "she", "his", "hers"];
    let ac1 = AhoCorasick.init(alloc, p_ushers[0..]) catch { expect_true(false)?; return; };
    defer ac1.deinit(alloc);
    const n1: usize = ac1.findAllOverlappingInto("ushers", out_s);
    expect_eq(n1, 3);
    expect_eq(out_buf[0].pattern_id, 1); // she (1..4)
    expect_eq(out_buf[1].pattern_id, 0); // he (2..4)
    expect_eq(out_buf[2].pattern_id, 3); // hers (2..6)

    // 2. Prefix chain ["a", "ab", "abc", "bc", "c"] on "abc"
    const p_chain: [5][]const u8 = ["a", "ab", "abc", "bc", "c"];
    let ac2 = AhoCorasick.init(alloc, p_chain[0..]) catch { expect_true(false)?; return; };
    defer ac2.deinit(alloc);
    const n2: usize = ac2.findAllOverlappingInto("abc", out_s);
    expect_eq(n2, 5); // a(0..1), ab(0..2), abc(0..3), bc(1..3), c(2..3)

    // 3. Duplicate patterns over200: ["foo", "foo"] on "foobarfoo"
    const p_dup: [2][]const u8 = ["foo", "foo"];
    let ac3 = AhoCorasick.initWithOptions(alloc, p_dup[0..], AhoCorasickOptions{}) catch { expect_true(false)?; return; };
    defer ac3.deinit(alloc);
    const n3_over: usize = ac3.findAllOverlappingInto("foobarfoo", out_s);
    expect_eq(n3_over, 4); // (0,0,3),(1,0,3),(0,6,9),(1,6,9)
    expect_eq(out_buf[0].pattern_id, 0);
    expect_eq(out_buf[1].pattern_id, 1);
    const n3_non: usize = ac3.findAllNonOverlapping("foobarfoo", out_s);
    expect_eq(n3_non, 2); // (0,0,3),(0,6,9)

    // 4. Empty pattern boundaries (standard040, leftfirst010, leftfirst015, leftlong020, leftlong024)
    const p_a_emp: [2][]const u8 = ["a", ""];
    const p_emp_a: [2][]const u8 = ["", "a"];
    const e_std_emp: [2]Match = [
        Match{ .pattern_id = 1, .start = 0, .end = 0 },
        Match{ .pattern_id = 1, .start = 1, .end = 1 },
    ];
    const e_lf_emp_a: [2]Match = [
        Match{ .pattern_id = 0, .start = 0, .end = 0 },
        Match{ .pattern_id = 0, .start = 1, .end = 1 },
    ];
    const e_lf_a_emp_ab: [2]Match = [
        Match{ .pattern_id = 0, .start = 0, .end = 1 },
        Match{ .pattern_id = 1, .start = 2, .end = 2 },
    ];
    const e_ll_emp_a_ab: [2]Match = [
        Match{ .pattern_id = 1, .start = 0, .end = 1 },
        Match{ .pattern_id = 0, .start = 2, .end = 2 },
    ];
    assertMatchesEq(alloc, p_a_emp[0..], "a", MatchKind.Standard, e_std_emp[0..])?;
    assertMatchesEq(alloc, p_emp_a[0..], "a", MatchKind.LeftmostFirst, e_lf_emp_a[0..])?;
    assertMatchesEq(alloc, p_a_emp[0..], "ab", MatchKind.LeftmostFirst, e_lf_a_emp_ab[0..])?;
    assertMatchesEq(alloc, p_emp_a[0..], "ab", MatchKind.LeftmostLongest, e_ll_emp_a_ab[0..])?;
}

// ============================================================================
// Test 3: ASCII Case-Insensitive Matching & Zero-Heap Single-Pass replaceAllInto
// ============================================================================
fn runTestCaseInsensitiveAndReplace(): void! {
    const alloc = std.heap.page_allocator;

    const pats: [3][]const u8 = ["foo", "foobar", "bar"];
    let ac_ll = AhoCorasick.initWithOptions(alloc, pats[0..], AhoCorasickOptions{
        match_kind: MatchKind.LeftmostLongest,
        ascii_case_insensitive: true,
        enable_prefilter: true,
        ignore_empty_patterns: false,
    }) catch { expect_true(false)?; return; };
    defer ac_ll.deinit(alloc);

    // Start bytes: 'f','F','b','B' = 4 -> cap:simd prefilter MUST be active!
    expect_true(ac_ll.prefilterActive())?;
    expect_eq(ac_ll.prefilterByteCount(), 4);

    const reps: [3][]const u8 = ["[F]", "[FB]", "[B]"];
    let out_buf: [128]u8 = [0; 128];
    let out_s: []u8 = out_buf[0..];
    const written: usize = ac_ll.replaceAllInto("xx FoObAr and fOo and BAR!", reps[0..], out_s) catch {
        expect_true(false)?;
        return;
    };
    expect_true(eqBytes(out_buf[0..written], "xx [FB] and [F] and [B]!"))?;

    // LeftmostFirst prefers "foo" (pid 0) over "foobar" (pid 1) -> "[F][B]"
    let ac_lf = AhoCorasick.initWithOptions(alloc, pats[0..], AhoCorasickOptions{
        match_kind: MatchKind.LeftmostFirst,
        ascii_case_insensitive: true,
        enable_prefilter: true,
        ignore_empty_patterns: false,
    }) catch { expect_true(false)?; return; };
    defer ac_lf.deinit(alloc);
    const w2: usize = ac_lf.replaceAllInto("FoObAr", reps[0..], out_s) catch {
        expect_true(false)?;
        return;
    };
    expect_true(eqBytes(out_buf[0..w2], "[F][B]"))?;

    // Defensive error guards: BufferTooSmall & ReplacementCountMismatch
    let tiny: [3]u8 = [0; 3];
    let tiny_s: []u8 = tiny[0..];
    let got_small: bool = false;
    * = ac_lf.replaceAllInto("FoObAr", reps[0..], tiny_s) catch |e| {
        if (e == AcError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small)?;

    const short_reps: [1][]const u8 = ["X"];
    let got_mismatch: bool = false;
    * = ac_lf.replaceAllInto("FoObAr", short_reps[0..], out_s) catch |e| {
        if (e == AcError.ReplacementCountMismatch) got_mismatch = true;
    };
    expect_true(got_mismatch)?;
}

// ============================================================================
// Test 4: 4KB Long Text cap:simd Prefilter (1/2/3/4 bytes) vs Pure Automaton
// ============================================================================
fn runTestSimdPrefilter4KB(): void! {
    const alloc = std.heap.page_allocator;

    // Build a 4,096-byte haystack with scattered matches at non-aligned & SIMD-boundary offsets
    let hay: [4096]u8 = ['.'; 4096];
    const needles_pos: [6]usize = [13, 511, 1024, 2049, 3333, 4080];
    const stamps: [6][]const u8 = ["alpha", "beta", "gamma", "delta", "alpha", "delta"];
    let si: usize = 0;
    while (si < 6) : (si += 1) {
        const p: usize = needles_pos[si];
        const st: []const u8 = stamps[si];
        let j: usize = 0;
        while (j < st.len) : (j += 1) {
            hay[p + j] = st[j];
        }
    }

    // Test 1-byte, 2-byte, 3-byte, and 4-byte prefilter sets
    const p1: [1][]const u8 = ["alpha"];
    const p2: [2][]const u8 = ["alpha", "beta"];
    const p3: [3][]const u8 = ["alpha", "beta", "gamma"];
    const p4: [4][]const u8 = ["alpha", "beta", "gamma", "delta"];

    let m_simd: [32]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 32];
    let m_pure: [32]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 32];
    let ms_s: []Match = m_simd[0..];
    let mp_s: []Match = m_pure[0..];

    const sets: [4][]const []const u8 = [p1[0..], p2[0..], p3[0..], p4[0..]];
    const expected_counts: [4]usize = [2, 3, 4, 6];

    let s_idx: usize = 0;
    while (s_idx < 4) : (s_idx += 1) {
        const kinds: [3]MatchKind = [MatchKind.Standard, MatchKind.LeftmostFirst, MatchKind.LeftmostLongest];
        let k_idx: usize = 0;
        while (k_idx < 3) : (k_idx += 1) {
            let ac_simd = AhoCorasick.initWithOptions(alloc, sets[s_idx], AhoCorasickOptions{
                match_kind: kinds[k_idx],
                enable_prefilter: true,
            }) catch { expect_true(false)?; return; };
            defer ac_simd.deinit(alloc);
            expect_true(ac_simd.prefilterActive())?;
            expect_eq(ac_simd.prefilterByteCount(), s_idx + 1);

            let ac_pure = AhoCorasick.initWithOptions(alloc, sets[s_idx], AhoCorasickOptions{
                match_kind: kinds[k_idx],
                enable_prefilter: false,
            }) catch { expect_true(false)?; return; };
            defer ac_pure.deinit(alloc);
            expect_true(!ac_pure.prefilterActive())?;

            const n_s: usize = ac_simd.findAllNonOverlapping(hay[0..], ms_s);
            const n_p: usize = ac_pure.findAllNonOverlapping(hay[0..], mp_s);
            expect_eq(n_s, expected_counts[s_idx]);
            expect_eq(n_s, n_p);
            let mi: usize = 0;
            while (mi < n_s) : (mi += 1) {
                expect_eq(m_simd[mi].pattern_id, m_pure[mi].pattern_id);
                expect_eq(m_simd[mi].start, m_pure[mi].start);
                expect_eq(m_simd[mi].end, m_pure[mi].end);
            }

            const ov_s: usize = ac_simd.findAllOverlappingInto(hay[0..], ms_s);
            const ov_p: usize = ac_pure.findAllOverlappingInto(hay[0..], mp_s);
            expect_eq(ov_s, ov_p);
            expect_eq(ac_simd.containsAny(hay[0..]), ac_pure.containsAny(hay[0..]));
        }
    }
}

// ============================================================================
// Test 5: 1,300 Deterministic PRNG Differential Fuzz Cases vs Naive Reference
// ============================================================================
fn runTestDifferentialFuzz1300(): void! {
    const alloc = std.heap.page_allocator;
    let rng: XorShift64 = XorShift64{ state: 0x5A17_C0DE_9876_5432 };

    let pat_storage: [6][8]u8 = [[0; 8]; 6];
    let pat_slices: [6][]const u8 = [""; 6];
    let text_buf: [48]u8 = [0; 48];

    let ac_out: [128]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 128];
    let ac_nopf_out: [128]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 128];
    let ref_out: [128]Match = [Match{ .pattern_id = 0, .start = 0, .end = 0 }; 128];
    let ac_s: []Match = ac_out[0..];
    let nopf_s: []Match = ac_nopf_out[0..];
    let ref_s: []Match = ref_out[0..];

    let trial: usize = 0;
    while (trial < 1300) : (trial += 1) {
        const num_pats: usize = rng.range(6) + 1;
        const ci: bool = rng.range(4) == 0;
        const allow_empty_pat: bool = rng.range(6) == 0;

        let p: usize = 0;
        while (p < num_pats) : (p += 1) {
            const plen: usize = if (allow_empty_pat && p == 0) 0 else (rng.range(5) + 1);
            let j: usize = 0;
            while (j < plen) : (j += 1) {
                const base_ch: u8 = @as(u8, @intCast('a' + rng.range(4)));
                pat_storage[p][j] = if (ci && rng.range(2) == 0) (base_ch - 32) else base_ch;
            }
            pat_slices[p] = pat_storage[p][0..plen];
        }

        const tlen: usize = rng.range(40);
        let ti: usize = 0;
        while (ti < tlen) : (ti += 1) {
            const base_ch: u8 = @as(u8, @intCast('a' + rng.range(5)));
            text_buf[ti] = if (ci && rng.range(2) == 0) (base_ch - 32) else base_ch;
        }
        const text: []const u8 = text_buf[0..tlen];
        const pats: []const []const u8 = pat_slices[0..num_pats];

        const kind_pick: usize = rng.range(3);
        const kind: MatchKind = if (kind_pick == 0)
            MatchKind.Standard
        else if (kind_pick == 1)
            MatchKind.LeftmostFirst
        else
            MatchKind.LeftmostLongest;

        let ac_pf = AhoCorasick.initWithOptions(alloc, pats, AhoCorasickOptions{
            match_kind: kind,
            ascii_case_insensitive: ci,
            enable_prefilter: true,
            ignore_empty_patterns: false,
        }) catch { expect_true(false)?; return; };
        defer ac_pf.deinit(alloc);

        let ac_np = AhoCorasick.initWithOptions(alloc, pats, AhoCorasickOptions{
            match_kind: kind,
            ascii_case_insensitive: ci,
            enable_prefilter: false,
            ignore_empty_patterns: false,
        }) catch { expect_true(false)?; return; };
        defer ac_np.deinit(alloc);

        const n_ac: usize = ac_pf.findAllNonOverlapping(text, ac_s);
        const n_np: usize = ac_np.findAllNonOverlapping(text, nopf_s);
        const n_ref: usize = naiveFindAllNonOverlapping(pats, text, kind, ci, ref_s);

        expect_eq(n_ac, n_ref);
        expect_eq(n_np, n_ref);
        let mi: usize = 0;
        while (mi < n_ref) : (mi += 1) {
            expect_eq(ac_out[mi].pattern_id, ref_out[mi].pattern_id);
            expect_eq(ac_out[mi].start, ref_out[mi].start);
            expect_eq(ac_out[mi].end, ref_out[mi].end);
            expect_eq(ac_nopf_out[mi].pattern_id, ref_out[mi].pattern_id);
            expect_eq(ac_nopf_out[mi].start, ref_out[mi].start);
            expect_eq(ac_nopf_out[mi].end, ref_out[mi].end);
        }

        // Verify overlapping match count matches naive oracle (when <= 128)
        const ov_ref: usize = naiveCountOverlapping(pats, text, ci);
        if (ov_ref <= 128) {
            const ov_ac: usize = ac_pf.findAllOverlappingInto(text, ac_s);
            const ov_np: usize = ac_np.findAllOverlappingInto(text, nopf_s);
            expect_eq(ov_ac, ov_ref);
            expect_eq(ov_np, ov_ref);
        }
        expect_eq(ac_pf.containsAny(text), n_ref > 0);
    }
}

fn main() {
    test("BurntSushi/aho-corasick: Standard vs LeftmostFirst vs LeftmostLongest", () => {
        runTestBurntSushiMatchKinds()?;
    });
    test("AhoCorasick: overlapping, prefix chains, empty & duplicate patterns", () => {
        runTestOverlappingEmptyDup()?;
    });
    test("AhoCorasick: ASCII case-insensitive & zero-heap replaceAllInto", () => {
        runTestCaseInsensitiveAndReplace()?;
    });
    test("AhoCorasick: 4KB long text cap:simd prefilter (1/2/3/4 bytes) vs pure automaton", () => {
        runTestSimdPrefilter4KB()?;
    });
    test("AhoCorasick: 1,300 PRNG differential fuzz cases vs naive reference matcher", () => {
        runTestDifferentialFuzz1300()?;
    });
}
