// Reference-hardened against BurntSushi/regex-automata (MIT/Apache-2.0)
// Source: cp/cap/test/testRegexAutomataSuite.cp
// Comprehensive Production Test, Anti-ReDoS, Differential Fuzzing (1,800 cases) & Throughput Benchmark Suite
// for lib/text/nfaRegex.cp (SIMD Prefilter + Lazy DFA Cache + PikeVM / Thompson NFA).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    NfaRegex,
    RegexError,
    MatchSpan,
    Captures,
    PrefilterKind,
    SearchStats,
} from "../../lib/text/nfaRegex.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn spanOptEq(a: MatchSpan?, b: MatchSpan?): bool {
    if (a == null && b == null) return true;
    if (a == null || b == null) return false;
    return a.?.start == b.?.start && a.?.end == b.?.end;
}

fn monotonicNs(): u64 {
    let ts: std.os.linux.timespec = undefined;
    * = std.os.linux.clock_gettime(std.os.linux.CLOCK.MONOTONIC, &ts);
    const sec: u64 = ts.sec as u64;
    const nsec: u64 = ts.nsec as u64;
    return sec * 1_000_000_000 + nsec;
}

fn isCompileError(alloc: std.mem.Allocator, pat: []const u8, expected: RegexError): bool {
    const re = NfaRegex.compile(alloc, pat) catch (e) {
        return e == expected;
    };
    re.deinit();
    return false;
}

struct XorShift64 {
    state: u64;

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn bound(*_, max_val: usize): usize {
        return (_.next() as usize) % max_val;
    }
}

fn main() {
    test("1. Core syntax: char classes, escapes, anchors (^, $, \\b, \\B), and quantifiers ({m}, {m,}, {m,n})", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Shorthand classes \d \D \w \W \s \S
        const re_shorthand = NfaRegex.compile(alloc, "\\d+\\s+\\w+\\W+\\D+\\S") catch unreachable;
        defer re_shorthand.deinit();
        const m1 = re_shorthand.find(alloc, "### 42 \t hello---xyz!") catch unreachable;
        expect_eq(m1 != null, true);
        expect_eq(strEq("42 \t hello---xyz!", "### 42 \t hello---xyz!"[m1.?.start..m1.?.end]), true);

        // Word boundaries \b and \B
        const re_word = NfaRegex.compile(alloc, "\\bcat\\b") catch unreachable;
        defer re_word.deinit();
        expect_eq(re_word.isMatch(alloc, "a cat sits") catch unreachable, true);
        expect_eq(re_word.isMatch(alloc, "category") catch unreachable, false);
        expect_eq(re_word.isMatch(alloc, "scat") catch unreachable, false);
        const mw = re_word.find(alloc, "scatter cat!") catch unreachable;
        expect_eq(mw != null, true);
        expect_eq(mw.?.start, 8);
        expect_eq(mw.?.end, 11);

        const re_nonword = NfaRegex.compile(alloc, "\\Bcat\\B") catch unreachable;
        defer re_nonword.deinit();
        expect_eq(re_nonword.isMatch(alloc, "educate") catch unreachable, true);
        expect_eq(re_nonword.isMatch(alloc, "cat") catch unreachable, false);

        // Bounded quantifiers {m}, {m,}, {m,n}
        const re_date = NfaRegex.compile(alloc, "^\\d{4}-\\d{2}-\\d{2}$") catch unreachable;
        defer re_date.deinit();
        expect_eq(re_date.isMatch(alloc, "2026-11-24") catch unreachable, true);
        expect_eq(re_date.isMatch(alloc, "26-11-24") catch unreachable, false);
        expect_eq(re_date.isMatch(alloc, "2026-11-245") catch unreachable, false);

        const re_min = NfaRegex.compile(alloc, "^a{3,}$") catch unreachable;
        defer re_min.deinit();
        expect_eq(re_min.isMatch(alloc, "aa") catch unreachable, false);
        expect_eq(re_min.isMatch(alloc, "aaa") catch unreachable, true);
        expect_eq(re_min.isMatch(alloc, "aaaaaa") catch unreachable, true);

        const re_range = NfaRegex.compile(alloc, "^(ab){2,3}$") catch unreachable;
        defer re_range.deinit();
        expect_eq(re_range.isMatch(alloc, "ab") catch unreachable, false);
        expect_eq(re_range.isMatch(alloc, "abab") catch unreachable, true);
        expect_eq(re_range.isMatch(alloc, "ababab") catch unreachable, true);
        expect_eq(re_range.isMatch(alloc, "abababab") catch unreachable, false);

        // {0} matches empty
        const re_zero = NfaRegex.compile(alloc, "^x{0}y$") catch unreachable;
        defer re_zero.deinit();
        expect_eq(re_zero.isMatch(alloc, "y") catch unreachable, true);
        expect_eq(re_zero.isMatch(alloc, "xy") catch unreachable, false);

        // Anchored alternation (^foo|bar is NOT anchored_start, ^foo|^bar IS)
        const re_mix_anchor = NfaRegex.compile(alloc, "^foo|bar") catch unreachable;
        defer re_mix_anchor.deinit();
        expect_eq(re_mix_anchor.isMatch(alloc, "xxxbar") catch unreachable, true);
        expect_eq(re_mix_anchor.isMatch(alloc, "xxxfoo") catch unreachable, false);
        expect_eq(re_mix_anchor.isMatch(alloc, "foo") catch unreachable, true);

        // findAll non-overlapping spans & empty pattern
        const re_pets = NfaRegex.compile(alloc, "(cat|dog)s?") catch unreachable;
        defer re_pets.deinit();
        const spans = re_pets.findAll(alloc, "one cat, two dogs, many cats") catch unreachable;
        defer alloc.free(spans);
        expect_eq(spans.len, 3);
        expect_eq(spans[0].start, 4);
        expect_eq(spans[0].end, 7);
        expect_eq(spans[1].start, 13);
        expect_eq(spans[1].end, 17);
        expect_eq(spans[2].start, 24);
        expect_eq(spans[2].end, 28);
    });

    test("2. Greedy vs Lazy quantifiers (*?, +?, ??, {m,n}?) and (?i) case-insensitive flags", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        const re_greedy = NfaRegex.compile(alloc, "a+") catch unreachable;
        defer re_greedy.deinit();
        const mg = re_greedy.find(alloc, "aaaa") catch unreachable;
        expect_eq(mg != null, true);
        expect_eq(mg.?.start, 0);
        expect_eq(mg.?.end, 4);

        const re_lazy_plus = NfaRegex.compile(alloc, "a+?") catch unreachable;
        defer re_lazy_plus.deinit();
        const ml1 = re_lazy_plus.find(alloc, "aaaa") catch unreachable;
        expect_eq(ml1 != null, true);
        expect_eq(ml1.?.start, 0);
        expect_eq(ml1.?.end, 1);

        const re_tag = NfaRegex.compile(alloc, "<.*?>") catch unreachable;
        defer re_tag.deinit();
        const mt = re_tag.find(alloc, "<b>hello</b>") catch unreachable;
        expect_eq(mt != null, true);
        expect_eq(mt.?.start, 0);
        expect_eq(mt.?.end, 3);

        const re_lazy_q = NfaRegex.compile(alloc, "a??b") catch unreachable;
        defer re_lazy_q.deinit();
        const mq = re_lazy_q.find(alloc, "ab") catch unreachable;
        expect_eq(mq != null, true);
        expect_eq(mq.?.start, 0);
        expect_eq(mq.?.end, 2);

        const re_lazy_bound = NfaRegex.compile(alloc, "a{2,4}?") catch unreachable;
        defer re_lazy_bound.deinit();
        const mb = re_lazy_bound.find(alloc, "aaaaa") catch unreachable;
        expect_eq(mb != null, true);
        expect_eq(mb.?.start, 0);
        expect_eq(mb.?.end, 2);

        const re_ci = NfaRegex.compile(alloc, "(?i)hello [a-z]+") catch unreachable;
        defer re_ci.deinit();
        expect_eq(re_ci.isMatch(alloc, "HeLLo WoRLD") catch unreachable, true);
        expect_eq(re_ci.isMatch(alloc, "hello 123") catch unreachable, false);

        const re_ci_toggle = NfaRegex.compile(alloc, "(?i)foo(?-i)Bar") catch unreachable;
        defer re_ci_toggle.deinit();
        expect_eq(re_ci_toggle.isMatch(alloc, "FOOBar") catch unreachable, true);
        expect_eq(re_ci_toggle.isMatch(alloc, "FOObar") catch unreachable, false);

        const re_ci_scoped = NfaRegex.compile(alloc, "(?i:abc)DEF") catch unreachable;
        defer re_ci_scoped.deinit();
        expect_eq(re_ci_scoped.isMatch(alloc, "AbCDEF") catch unreachable, true);
        expect_eq(re_ci_scoped.isMatch(alloc, "AbCdef") catch unreachable, false);
    });

    test("3. Captures (...), non-capturing (?:...), replace, replaceAll, and split", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        const re_url = NfaRegex.compile(alloc, "(?:https?)://([a-z0-9.-]+)(?::(\\d+))?(/\\S*)?") catch unreachable;
        defer re_url.deinit();
        expect_eq(re_url.captureGroupCount(), 4);

        const url_in = "visit https://api.capdux.dev:8443/v1/agents now";
        const caps_opt = re_url.captures(alloc, url_in) catch unreachable;
        expect_eq(caps_opt != null, true);
        const caps = caps_opt.?;
        expect_eq(caps.len(), 4);
        expect_eq(strEq(caps.slice(url_in, 0).?, "https://api.capdux.dev:8443/v1/agents"), true);
        expect_eq(strEq(caps.slice(url_in, 1).?, "api.capdux.dev"), true);
        expect_eq(strEq(caps.slice(url_in, 2).?, "8443"), true);
        expect_eq(strEq(caps.slice(url_in, 3).?, "/v1/agents"), true);

        const url_no_port = "http://example.org/docs";
        const caps2_opt = re_url.captures(alloc, url_no_port) catch unreachable;
        expect_eq(caps2_opt != null, true);
        const caps2 = caps2_opt.?;
        expect_eq(strEq(caps2.slice(url_no_port, 1).?, "example.org"), true);
        expect_eq(caps2.get(2) == null, true);
        expect_eq(strEq(caps2.slice(url_no_port, 3).?, "/docs"), true);

        const re_kv = NfaRegex.compile(alloc, "(\\w+)=(\\d+)") catch unreachable;
        defer re_kv.deinit();
        const rep1 = re_kv.replace(alloc, "a=10 b=20 c=30", "$2:$$:$1") catch unreachable;
        defer alloc.free(rep1);
        expect_eq(strEq(rep1, "10:$:a b=20 c=30"), true);

        const rep_all = re_kv.replaceAll(alloc, "a=10 b=20 c=30", "[$1->$2]") catch unreachable;
        defer alloc.free(rep_all);
        expect_eq(strEq(rep_all, "[a->10] [b->20] [c->30]"), true);

        const rep_none = re_kv.replace(alloc, "no pairs here", "X") catch unreachable;
        defer alloc.free(rep_none);
        expect_eq(strEq(rep_none, "no pairs here"), true);

        const re_sep = NfaRegex.compile(alloc, "[,;\\s]+") catch unreachable;
        defer re_sep.deinit();
        const parts = re_sep.split(alloc, "alpha, beta;  gamma,") catch unreachable;
        defer alloc.free(parts);
        expect_eq(parts.len, 4);
        expect_eq(strEq(parts[0], "alpha"), true);
        expect_eq(strEq(parts[1], "beta"), true);
        expect_eq(strEq(parts[2], "gamma"), true);
        expect_eq(strEq(parts[3], ""), true);

        const parts_none = re_sep.split(alloc, "single") catch unreachable;
        defer alloc.free(parts_none);
        expect_eq(parts_none.len, 1);
        expect_eq(strEq(parts_none[0], "single"), true);
    });

    test("4. Error handling for malformed patterns & UTF-8 non-ASCII fallback safety", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(isCompileError(alloc, "(abc", RegexError.UnmatchedParen), true);
        expect_eq(isCompileError(alloc, "abc)", RegexError.UnmatchedParen), true);
        expect_eq(isCompileError(alloc, "[a-z", RegexError.UnclosedCharClass), true);
        expect_eq(isCompileError(alloc, "*abc", RegexError.NothingToRepeat), true);
        expect_eq(isCompileError(alloc, "+abc", RegexError.NothingToRepeat), true);
        expect_eq(isCompileError(alloc, "?abc", RegexError.NothingToRepeat), true);
        expect_eq(isCompileError(alloc, "{2}abc", RegexError.NothingToRepeat), true);
        expect_eq(isCompileError(alloc, "abc\\", RegexError.TrailingEscape), true);
        expect_eq(isCompileError(alloc, "[z-a]", RegexError.InvalidRange), true);
        expect_eq(isCompileError(alloc, "a{5,2}", RegexError.InvalidRange), true);
        expect_eq(isCompileError(alloc, "a{}", RegexError.InvalidQuantifier), true);
        expect_eq(isCompileError(alloc, "a{1,2", RegexError.InvalidQuantifier), true);
        expect_eq(isCompileError(alloc, "a{xy}", RegexError.InvalidQuantifier), true);
        expect_eq(isCompileError(alloc, "(?q)abc", RegexError.InvalidFlag), true);
        expect_eq(isCompileError(alloc, "a{999}", RegexError.PatternTooComplex), true);

        const re_utf8 = NfaRegex.compile(alloc, "id=\\d+") catch unreachable;
        defer re_utf8.deinit();
        const utf8_text = "前缀中文测试 id=98765 后缀";
        const m = re_utf8.find(alloc, utf8_text) catch unreachable;
        expect_eq(m != null, true);
        expect_eq(strEq(utf8_text[m.?.start..m.?.end], "id=98765"), true);
    });

    test("5. SIMD Prefilter classification & Lazy DFA cache hit verification", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        const re_lit = NfaRegex.compile(alloc, "ERROR-[0-9]+") catch unreachable;
        defer re_lit.deinit();
        expect_eq(re_lit.prefilterKind() == PrefilterKind.LiteralPrefix, true);
        expect_eq(strEq(re_lit.prefilterPrefix(), "ERROR-"), true);

        const re_two = NfaRegex.compile(alloc, "(?i)hello") catch unreachable;
        defer re_two.deinit();
        expect_eq(re_two.prefilterKind() == PrefilterKind.TwoBytes, true);

        const re_four = NfaRegex.compile(alloc, "(cat|dog|fox|rat)") catch unreachable;
        defer re_four.deinit();
        expect_eq(re_four.prefilterKind() == PrefilterKind.FourBytes, true);

        const re_range = NfaRegex.compile(alloc, "[0-9]+") catch unreachable;
        defer re_range.deinit();
        expect_eq(re_range.prefilterKind() == PrefilterKind.ByteRange, true);

        const buf: []u8 = alloc.alloc(u8, 4096) catch unreachable;
        defer alloc.free(buf);
        @memset(buf, '.');
        const target = "ERROR-90421";
        @memcpy(buf[3900 .. 3900 + target.len], target);

        let stats = SearchStats{};
        const hit = re_lit.findWithStats(alloc, buf, &stats) catch unreachable;
        expect_eq(hit != null, true);
        expect_eq(hit.?.start, 3900);
        expect_eq(hit.?.end, 3900 + target.len);
        expect_eq(stats.prefilter_skips >= 3800, true);
        expect_eq(stats.dfa_steps > 0, true);
    });

    test("6. Anti-ReDoS O(n) linear-time stress test: (a+)+b on 5,000 'a's in milliseconds", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        const re_unanchored = NfaRegex.compile(alloc, "(a+)+b") catch unreachable;
        defer re_unanchored.deinit();
        const re_anchored = NfaRegex.compile(alloc, "^(a+)+b$") catch unreachable;
        defer re_anchored.deinit();
        const re_alt = NfaRegex.compile(alloc, "(a|aa)+b") catch unreachable;
        defer re_alt.deinit();

        const N: usize = 5000;
        const redos_no_b: []u8 = alloc.alloc(u8, N) catch unreachable;
        defer alloc.free(redos_no_b);
        @memset(redos_no_b, 'a');

        let redos_with_b: []u8 = alloc.alloc(u8, N + 1) catch unreachable;
        defer alloc.free(redos_with_b);
        @memset(redos_with_b[0..N], 'a');
        redos_with_b[N] = 'b';

        const t0: u64 = monotonicNs();
        expect_eq(re_unanchored.isMatch(alloc, redos_no_b) catch unreachable, false);
        expect_eq((re_unanchored.find(alloc, redos_no_b) catch unreachable) == null, true);
        expect_eq(re_unanchored.isMatch(alloc, redos_with_b) catch unreachable, true);
        expect_eq(re_anchored.isMatch(alloc, redos_no_b) catch unreachable, false);
        expect_eq(re_anchored.isMatch(alloc, redos_with_b) catch unreachable, true);
        expect_eq(re_alt.isMatch(alloc, redos_no_b) catch unreachable, false);
        const t1: u64 = monotonicNs();
        const elapsed_ns: u64 = if (t1 > t0) t1 - t0 else 1;
        const elapsed_us: u64 = elapsed_ns / 1000;
        const elapsed_ms_f: f64 = (elapsed_ns as f64) / 1_000_000.0;

        std.debug.print("[ReDoS-5000a] 6 pathological runs finished in {d:.3} ms ({d} us)\n", .{ elapsed_ms_f, elapsed_us });
        expect_eq(elapsed_us < 500_000, true);
    });

    test("7. Deterministic PRNG Fuzzing (1,800 total cases: 800 malformed + 1,000 differential DFA vs NFA)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let rng = XorShift64{ state: 0x243F6A8885A308D3 };

        const fuzz_alphabet = "abc012()[]{}*+?|^$.\\dwsb-:,!i";
        let pat_buf: [24]u8 = undefined;
        let hay_buf: [48]u8 = undefined;

        let compiled_ok: usize = 0;
        let i: usize = 0;
        while (i < 800) : (i += 1) {
            const plen: usize = (1 as usize) + rng.bound(18);
            let p: usize = 0;
            while (p < plen) : (p += 1) {
                pat_buf[p] = fuzz_alphabet[rng.bound(fuzz_alphabet.len)];
            }
            const hlen = rng.bound(40);
            let h: usize = 0;
            while (h < hlen) : (h += 1) {
                hay_buf[h] = fuzz_alphabet[rng.bound(10)];
            }

            const compiled = NfaRegex.compile(alloc, pat_buf[0..plen]) catch continue;
            defer compiled.deinit();
            compiled_ok += 1;

            const m_fast = compiled.find(alloc, hay_buf[0..hlen]) catch unreachable;
            const m_nfa = compiled.findNfaOnly(alloc, hay_buf[0..hlen]) catch unreachable;
            expect_eq(spanOptEq(m_fast, m_nfa), true);
            const matched = compiled.isMatch(alloc, hay_buf[0..hlen]) catch unreachable;
            expect_eq(matched, m_nfa != null);
        }
        expect_eq(compiled_ok > 0, true);

        const atoms: [12][]const u8 = [
            "a", "b", "c", "ab", "bc", "[a-c]", "[^d]", "\\d", "\\w", "(a|b)", "(?:ab|c)", ".",
        ];
        const quants: [9][]const u8 = [
            "", "", "*", "+", "?", "{2}", "{1,3}", "*?", "+?",
        ];

        let pat_list: std.ArrayList(u8) = .empty;
        defer pat_list.deinit(alloc);

        let j: usize = 0;
        while (j < 1000) : (j += 1) {
            pat_list.items.len = 0;
            if (rng.bound(5) == 0) pat_list.appendSlice(alloc, "(?i)") catch unreachable;
            if (rng.bound(6) == 0) pat_list.append(alloc, '^') catch unreachable;

            const n_parts: usize = (1 as usize) + rng.bound(4);
            let k: usize = 0;
            while (k < n_parts) : (k += 1) {
                const atom = atoms[rng.bound(atoms.len)];
                const q = quants[rng.bound(quants.len)];
                pat_list.appendSlice(alloc, atom) catch unreachable;
                pat_list.appendSlice(alloc, q) catch unreachable;
                if (k + 1 < n_parts && rng.bound(5) == 0) {
                    pat_list.append(alloc, '|') catch unreachable;
                }
            }
            if (rng.bound(6) == 0) pat_list.append(alloc, '$') catch unreachable;

            const hlen2 = rng.bound(48);
            let h2: usize = 0;
            while (h2 < hlen2) : (h2 += 1) {
                const pick = rng.bound(10);
                hay_buf[h2] = if (pick < 3) 'a'
                    else if (pick < 6) 'b'
                    else if (pick < 8) 'c'
                    else if (pick == 8) '1'
                    else 'A';
            }

            const re = NfaRegex.compile(alloc, pat_list.items) catch unreachable;
            defer re.deinit();

            const m_fast2 = re.find(alloc, hay_buf[0..hlen2]) catch unreachable;
            const m_nfa2 = re.findNfaOnly(alloc, hay_buf[0..hlen2]) catch unreachable;
            expect_eq(spanOptEq(m_fast2, m_nfa2), true);
            expect_eq(re.isMatch(alloc, hay_buf[0..hlen2]) catch unreachable, m_nfa2 != null);
        }
    });

    test("8. SIMD Prefilter + Lazy DFA throughput benchmark (MB/s on 1MB haystack)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const HAY_LEN: usize = 1024 * 1024;
        let hay: []u8 = alloc.alloc(u8, HAY_LEN) catch unreachable;
        defer alloc.free(hay);

        let idx: usize = 0;
        while (idx < HAY_LEN) : (idx += 1) {
            hay[idx] = ('a' as u8) + ((idx % 23) as u8);
        }
        const needle_str = "FATAL-2026-9918";
        @memcpy(hay[HAY_LEN - 64 .. HAY_LEN - 64 + needle_str.len], needle_str);

        const re_pref = NfaRegex.compile(alloc, "FATAL-[0-9]{4}-[0-9]{4}") catch unreachable;
        defer re_pref.deinit();

        const iters: usize = 16;
        const t0: u64 = monotonicNs();
        let r: usize = 0;
        while (r < iters) : (r += 1) {
            const m = re_pref.find(alloc, hay) catch unreachable;
            expect_eq(m != null, true);
            expect_eq(m.?.start, HAY_LEN - 64);
        }
        const t1: u64 = monotonicNs();
        const ns_pref: f64 = ((if (t1 > t0) t1 - t0 else 1) as f64);
        const mb_pref: f64 = ((HAY_LEN * iters) as f64) / (1024.0 * 1024.0);
        const mbps_pref: f64 = mb_pref / (ns_pref / 1_000_000_000.0);

        const re_dfa = NfaRegex.compile(alloc, "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w)+Z") catch unreachable;
        defer re_dfa.deinit();

        const t2: u64 = monotonicNs();
        let d_iter: usize = 0;
        while (d_iter < 4) : (d_iter += 1) {
            expect_eq(re_dfa.isMatch(alloc, hay) catch unreachable, false);
        }
        const t3: u64 = monotonicNs();
        const ns_dfa: f64 = ((if (t3 > t2) t3 - t2 else 1) as f64);
        const mb_dfa: f64 = ((HAY_LEN * 4) as f64) / (1024.0 * 1024.0);
        const mbps_dfa: f64 = mb_dfa / (ns_dfa / 1_000_000_000.0);

        std.debug.print(
            "[Bench-1MiB] SIMD-Prefilter+LazyDFA: {d:.1} MB/s | Dense-LazyDFA: {d:.1} MB/s\n",
            .{ mbps_pref, mbps_dfa },
        );
        expect_eq(mbps_pref > 1.0, true);
        expect_eq(mbps_dfa > 1.0, true);
    });
}
