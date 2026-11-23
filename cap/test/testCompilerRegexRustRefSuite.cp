// rust-lang/regex (MIT/Apache-2.0) Reference-Hardened PikeVM Regex Test Suite
// Covers:
//   - rust-lang/regex testdata vectors: flags.toml (?i/?m/?s/?U), word-boundary.toml (\b/\B),
//     empty.toml (empty alternations & zero-width findAll), anchored (\A/\z/^/$), lazy vs greedy,
//     exact quantifiers {m}/{m,}/{m,n}/{m,n}?, captures, replaceAll, split, char classes & \xHH
//   - Anti-ReDoS O(n*m) linear-time verification: a?^25 a^25, (a+)+$, (a|aa)+b
//   - SIMD prefilter (SingleByte / TwoBytes / FourBytes / LiteralPrefix / AnchoredStart)
//     vs pure PikeVM (findNoPrefilter) 100% semantic equivalence on 4KB haystacks
//   - 1,200-iteration deterministic PRNG malformed pattern compile + match fuzzing (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RegexError,
    PrefilterKind,
} from "../../lib/compiler/regexCompiler.cp";
import {
    Regex,
    MatchSpan,
    Captures,
} from "../../lib/compiler/regexVm.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn spansOptEqual(a: MatchSpan?, b: MatchSpan?): bool {
    if (a == null && b == null) return true;
    if (a == null || b == null) return false;
    return a.?.start == b.?.start && a.?.end == b.?.end;
}

fn main() {
    // ========================================================================
    // 1. rust-lang/regex testdata/flags.toml (?i, ?m, ?s, ?U, scoped flags)
    // ========================================================================
    test("regex_ref_01_inline_flags_i_m_s_U_and_scoped_flags", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // (?i)abc on "ABC" -> [0, 3]
        let r1 = Regex.compile(alloc, "(?i)abc") catch unreachable;
        defer r1.deinit();
        const m1 = r1.find("ABC");
        expect_eq(m1 != null, true);
        expect_eq(m1.?.start, 0);
        expect_eq(m1.?.end, 3);

        // (?i)a(?-i)bc on "Abc" -> [0, 3], on "ABC" -> null
        let r2 = Regex.compile(alloc, "(?i)a(?-i)bc") catch unreachable;
        defer r2.deinit();
        expect_eq(r2.isMatch("Abc"), true);
        expect_eq(r2.isMatch("ABC"), false);

        // (?is)a. on "A\n" -> [0, 2]
        let r3 = Regex.compile(alloc, "(?is)a.") catch unreachable;
        defer r3.deinit();
        const m3 = r3.find("A\n");
        expect_eq(m3 != null, true);
        expect_eq(m3.?.start, 0);
        expect_eq(m3.?.end, 2);

        // (?is)a.(?-is)a. on "A\nab" -> [0, 4], on "A\na\n" -> null
        let r4 = Regex.compile(alloc, "(?is)a.(?-is)a.") catch unreachable;
        defer r4.deinit();
        const m4 = r4.find("A\nab");
        expect_eq(m4 != null, true);
        expect_eq(m4.?.end, 4);
        expect_eq(r4.isMatch("A\na\n"), false);

        // Scoped flag group: (?is)a.(?-is:a.)? on "A\na\n" -> [0, 2]
        let r5 = Regex.compile(alloc, "(?is)a.(?-is:a.)?") catch unreachable;
        defer r5.deinit();
        const m5 = r5.find("A\na\n");
        expect_eq(m5 != null, true);
        expect_eq(m5.?.start, 0);
        expect_eq(m5.?.end, 2);

        // (?U)a+ on "aa" -> [0, 1]; (?U)a+? on "aa" -> [0, 2]; (?U)(?-U)a+ -> [0, 2]
        let r6 = Regex.compile(alloc, "(?U)a+") catch unreachable;
        defer r6.deinit();
        expect_eq(r6.find("aa").?.end, 1);

        let r7 = Regex.compile(alloc, "(?U)a+?") catch unreachable;
        defer r7.deinit();
        expect_eq(r7.find("aa").?.end, 2);

        let r8 = Regex.compile(alloc, "(?U)(?-U)a+") catch unreachable;
        defer r8.deinit();
        expect_eq(r8.find("aa").?.end, 2);

        // Multiline (?m)(?:^\d+$\n?)+ on "123\n456\n789" -> [0, 11]
        let r9 = Regex.compile(alloc, "(?m)(?:^\\d+$\\n?)+") catch unreachable;
        defer r9.deinit();
        const m9 = r9.find("123\n456\n789");
        expect_eq(m9 != null, true);
        expect_eq(m9.?.start, 0);
        expect_eq(m9.?.end, 11);

        // Case-insensitive char class (?i)[a-c]+
        let r10 = Regex.compile(alloc, "(?i)[a-c]+") catch unreachable;
        defer r10.deinit();
        const m10 = r10.find("xxAbCaBcZZ");
        expect_eq(m10 != null, true);
        expect_eq(m10.?.start, 2);
        expect_eq(m10.?.end, 8);
    });

    // ========================================================================
    // 2. rust-lang/regex testdata/word-boundary.toml (\b, \B, \A, \z)
    // ========================================================================
    test("regex_ref_02_word_boundaries_and_strict_anchors", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        let rb = Regex.compile(alloc, "\\b") catch unreachable;
        defer rb.deinit();
        const mb0 = rb.findAll(alloc, "") catch unreachable;
        defer alloc.free(mb0);
        expect_eq(mb0.len, 0);

        const mb1 = rb.findAll(alloc, "a") catch unreachable;
        defer alloc.free(mb1);
        expect_eq(mb1.len, 2);
        expect_eq(mb1[0].start, 0);
        expect_eq(mb1[1].start, 1);

        const mb2 = rb.findAll(alloc, "ab") catch unreachable;
        defer alloc.free(mb2);
        expect_eq(mb2.len, 2);
        expect_eq(mb2[0].start, 0);
        expect_eq(mb2[1].start, 2);

        // \bbar\b on "nobar bar foo bar" -> [6, 9], [14, 17]
        let rbar = Regex.compile(alloc, "\\bbar\\b") catch unreachable;
        defer rbar.deinit();
        const mbar = rbar.findAll(alloc, "nobar bar foo bar") catch unreachable;
        defer alloc.free(mbar);
        expect_eq(mbar.len, 2);
        expect_eq(mbar[0].start, 6);
        expect_eq(mbar[0].end, 9);
        expect_eq(mbar[1].start, 14);
        expect_eq(mbar[1].end, 17);

        // \Bbar\B on "xbarx bar" -> [1, 4]
        let rnb = Regex.compile(alloc, "\\Bbar\\B") catch unreachable;
        defer rnb.deinit();
        const mnb = rnb.find("xbarx bar");
        expect_eq(mnb != null, true);
        expect_eq(mnb.?.start, 1);
        expect_eq(mnb.?.end, 4);

        // \A and \z stay strict even under (?m)
        let rAz = Regex.compile(alloc, "(?m)\\Afoo\\z") catch unreachable;
        defer rAz.deinit();
        expect_eq(rAz.isMatch("foo"), true);
        expect_eq(rAz.isMatch("bar\nfoo"), false);
        expect_eq(rAz.isMatch("foo\nbar"), false);
    });

    // ========================================================================
    // 3. rust-lang/regex testdata/empty.toml & Zero-Width Loop Anti-Deadlock
    // ========================================================================
    test("regex_ref_03_empty_matches_and_zero_width_anti_deadlock", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // "b|" on "abc" -> [0,0], [1,2], [3,3]
        let r1 = Regex.compile(alloc, "b|") catch unreachable;
        defer r1.deinit();
        const m1 = r1.findAll(alloc, "abc") catch unreachable;
        defer alloc.free(m1);
        expect_eq(m1.len, 3);
        expect_eq(m1[0].start, 0);
        expect_eq(m1[0].end, 0);
        expect_eq(m1[1].start, 1);
        expect_eq(m1[1].end, 2);
        expect_eq(m1[2].start, 3);
        expect_eq(m1[2].end, 3);

        // "|b" on "abc" -> [0,0], [1,1], [2,2], [3,3]
        let r2 = Regex.compile(alloc, "|b") catch unreachable;
        defer r2.deinit();
        const m2 = r2.findAll(alloc, "abc") catch unreachable;
        defer alloc.free(m2);
        expect_eq(m2.len, 4);

        // Nested empty-matchable loops: (a*)*, ()*, (|a)* must never hang
        let r3 = Regex.compile(alloc, "(a*)*") catch unreachable;
        defer r3.deinit();
        const m3 = r3.find("aaab");
        expect_eq(m3 != null, true);
        expect_eq(m3.?.start, 0);
        expect_eq(m3.?.end, 3);

        let r4 = Regex.compile(alloc, "()*") catch unreachable;
        defer r4.deinit();
        const m4 = r4.findAll(alloc, "ab") catch unreachable;
        defer alloc.free(m4);
        expect_eq(m4.len, 3);

        let r5 = Regex.compile(alloc, "^") catch unreachable;
        defer r5.deinit();
        const m5 = r5.findAll(alloc, "hello") catch unreachable;
        defer alloc.free(m5);
        expect_eq(m5.len, 1);
        expect_eq(m5[0].start, 0);
        expect_eq(m5[0].end, 0);
    });

    // ========================================================================
    // 4. Quantifiers, Captures, Lazy vs Greedy, Escapes, ReplaceAll & Split
    // ========================================================================
    test("regex_ref_04_quantifiers_captures_escapes_replace_split", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Exact & lazy brace quantifiers
        let rq = Regex.compile(alloc, "a{2,4}?") catch unreachable;
        defer rq.deinit();
        const mq = rq.find("aaaaa");
        expect_eq(mq != null, true);
        expect_eq(mq.?.start, 0);
        expect_eq(mq.?.end, 2);

        let rqg = Regex.compile(alloc, "a{2,4}") catch unreachable;
        defer rqg.deinit();
        expect_eq(rqg.find("aaaaa").?.end, 4);

        // Hex escape \x41\x42 == "AB"
        let rhex = Regex.compile(alloc, "\\x41\\x42\\d+") catch unreachable;
        defer rhex.deinit();
        const mhex = rhex.find("zzAB99!");
        expect_eq(mhex != null, true);
        expect_eq(mhex.?.start, 2);
        expect_eq(mhex.?.end, 6);

        // Captures & replaceAll
        let rdate = Regex.compile(alloc, "(\\d{4})-(\\d{2})-(\\d{2})") catch unreachable;
        defer rdate.deinit();
        let caps_opt = rdate.captures(alloc, "date=2026-11-23!") catch unreachable;
        expect_eq(caps_opt != null, true);
        let caps = caps_opt.?;
        defer caps.deinit();
        expect_eq(caps.count(), 4);
        expect_eq(strEq(caps.group(1).?.text, "2026"), true);
        expect_eq(strEq(caps.group(2).?.text, "11"), true);
        expect_eq(strEq(caps.group(3).?.text, "23"), true);

        const replaced = rdate.replaceAll(alloc, "2026-11-23 and 2025-01-02", "$3/$2/$1") catch unreachable;
        defer alloc.free(replaced);
        expect_eq(strEq(replaced, "23/11/2026 and 02/01/2025"), true);

        // Split
        let rsep = Regex.compile(alloc, "\\s*,\\s*") catch unreachable;
        defer rsep.deinit();
        const parts = rsep.split(alloc, "a, b ,  c,d") catch unreachable;
        defer alloc.free(parts);
        expect_eq(parts.len, 4);
        expect_eq(strEq(parts[0], "a"), true);
        expect_eq(strEq(parts[1], "b"), true);
        expect_eq(strEq(parts[2], "c"), true);
        expect_eq(strEq(parts[3], "d"), true);
    });

    // ========================================================================
    // 5. Anti-ReDoS O(n*m) Linear-Time Immunity (a?^25 a^25 & (a+)+$)
    // ========================================================================
    test("regex_ref_05_anti_redos_linear_time_a25_and_nested_plus", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Classic Cox/RE2 benchmark: a?^25 a^25 on "a"*25 (exponential in backtracker, instant in PikeVM)
        const pat_a25 = "a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaaaaaaaaaaaa";
        let ra25 = Regex.compile(alloc, pat_a25) catch unreachable;
        defer ra25.deinit();
        expect_eq(ra25.isFullMatch("aaaaaaaaaaaaaaaaaaaaaaaaa"), true);
        expect_eq(ra25.isFullMatch("aaaaaaaaaaaaaaaaaaaaaaaa"), false);

        // Pathological (a+)+$ on 200 'a's + '!'
        let rnest = Regex.compile(alloc, "^(a+)+$") catch unreachable;
        defer rnest.deinit();
        let bad: [201]u8 = ['a'; 201];
        bad[200] = '!';
        expect_eq(rnest.isMatch(bad[0..]), false);
        expect_eq(rnest.isMatch(bad[0..200]), true);

        // (a|aa)+b on 120 'a's + 'c'
        let ralt = Regex.compile(alloc, "(a|aa)+b") catch unreachable;
        defer ralt.deinit();
        let bad2: [121]u8 = ['a'; 121];
        bad2[120] = 'c';
        expect_eq(ralt.isMatch(bad2[0..]), false);
        bad2[120] = 'b';
        expect_eq(ralt.isMatch(bad2[0..]), true);
    });

    // ========================================================================
    // 6. SIMD Prefilter vs Pure PikeVM 100% Equivalence (4KB Haystacks)
    // ========================================================================
    test("regex_ref_06_simd_prefilter_exact_equivalence_4kb", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Build 4096-byte haystack with needles near the tail
        let hay: [4096]u8 = ['.'; 4096];
        const tail = "ERROR_CODE_404 x999 Hello POST /api/v2";
        let ti: usize = 0;
        while (ti < tail.len) : (ti += 1) {
            hay[4000 + ti] = tail[ti];
        }

        // 1. LiteralPrefix
        let r_lit = Regex.compile(alloc, "ERROR_CODE_\\d+") catch unreachable;
        defer r_lit.deinit();
        expect_eq(r_lit.prefilterKind() == PrefilterKind.LiteralPrefix, true);
        const m_lit_s = r_lit.find(hay[0..]);
        const m_lit_p = r_lit.findNoPrefilter(hay[0..]);
        expect_eq(spansOptEqual(m_lit_s, m_lit_p), true);
        expect_eq(m_lit_s.?.start, 4000);
        expect_eq(m_lit_s.?.end, 4014);

        // 2. SingleByte
        let r_sb = Regex.compile(alloc, "x\\d+") catch unreachable;
        defer r_sb.deinit();
        expect_eq(r_sb.prefilterKind() == PrefilterKind.SingleByte, true);
        expect_eq(spansOptEqual(r_sb.find(hay[0..]), r_sb.findNoPrefilter(hay[0..])), true);
        expect_eq(r_sb.find(hay[0..]).?.start, 4015);

        // 3. TwoBytes ((?i)hello -> 'h'/'H')
        let r_tb = Regex.compile(alloc, "(?i)hello") catch unreachable;
        defer r_tb.deinit();
        expect_eq(r_tb.prefilterKind() == PrefilterKind.TwoBytes, true);
        expect_eq(spansOptEqual(r_tb.find(hay[0..]), r_tb.findNoPrefilter(hay[0..])), true);
        expect_eq(r_tb.find(hay[0..]).?.start, 4020);

        // 4. FourBytes ((GET|POST|PUT|HEAD) /\w+) -> 'G'/'P'/'H'
        let r_fb = Regex.compile(alloc, "(GET|POST|PUT|HEAD) /\\w+") catch unreachable;
        defer r_fb.deinit();
        expect_eq(r_fb.prefilterKind() == PrefilterKind.FourBytes, true);
        expect_eq(spansOptEqual(r_fb.find(hay[0..]), r_fb.findNoPrefilter(hay[0..])), true);
        expect_eq(r_fb.find(hay[0..]).?.start, 4026);

        // 5. AnchoredStart (^ERROR_CODE) -> fast reject on non-matching start
        let r_anc = Regex.compile(alloc, "^ERROR_CODE") catch unreachable;
        defer r_anc.deinit();
        expect_eq(r_anc.prefilterKind() == PrefilterKind.AnchoredStart, true);
        expect_eq(spansOptEqual(r_anc.find(hay[0..]), r_anc.findNoPrefilter(hay[0..])), true);
        expect_eq(r_anc.find(hay[0..]) == null, true);

        // 6. Miss case across entire 4KB buffer
        let r_miss = Regex.compile(alloc, "NOT_FOUND_TOKEN") catch unreachable;
        defer r_miss.deinit();
        expect_eq(r_miss.prefilterKind() == PrefilterKind.LiteralPrefix, true);
        expect_eq(r_miss.find(hay[0..]) == null, true);
        expect_eq(r_miss.findNoPrefilter(hay[0..]) == null, true);
    });

    // ========================================================================
    // 7. 1,200-Iteration Deterministic PRNG Malformed Pattern & Match Fuzzing
    // ========================================================================
    test("regex_ref_07_deterministic_1200_fuzz_compile_and_simd_equiv", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const alphabet = "ab01.*+?()[]{}|^$\\dwsiUm-:";
        const haystacks: [4]string = [
            "",
            "a",
            "aaabbb01\nab10",
            "foo_bar 123 AB ab01!",
        ];

        let state: u64 = 0x243F6A8885A308D3;
        let pat_buf: [32]u8 = [0; 32];
        let iter: usize = 0;
        let compiled_ok: usize = 0;
        let rejected_err: usize = 0;
        let hi: usize = 0;

        while (iter < 1200) : (iter += 1) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            const r = state *% 0x2545F4914F6CDD1D;
            const plen: usize = (1 as usize) + ((r % 20) as usize);

            let j: usize = 0;
            while (j < plen) : (j += 1) {
                state ^= state >> 12;
                state ^= state << 25;
                state ^= state >> 27;
                const aidx = (state % (alphabet.len as u64)) as usize;
                pat_buf[j] = alphabet[aidx];
            }

            let re = Regex.compile(alloc, pat_buf[0..plen]) catch {
                rejected_err += 1;
                continue;
            };
            defer re.deinit();
            compiled_ok += 1;

            // Verify SIMD prefilter == pure PikeVM on all 4 haystacks, and findAll never hangs
            hi = 0;
            while (hi < 4) : (hi += 1) {
                const h = haystacks[hi];
                const m_simd = re.find(h);
                const m_pure = re.findNoPrefilter(h);
                expect_eq(spansOptEqual(m_simd, m_pure), true);
                const all = re.findAll(alloc, h) catch unreachable;
                alloc.free(all);
            }
        }

        expect_eq(compiled_ok + rejected_err, 1200);
        expect_eq(compiled_ok > 50, true);
        expect_eq(rejected_err > 100, true);
    });
}
