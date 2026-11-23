// Reference-hardened test suite against BurntSushi/ripgrep (globset, MIT/Unlicense)
// Covers:
//   1. Core Glob syntax & ripgrep/globset vectors (*, ?, **, [a-z], [!0-9], [.-0], escapes, case-insensitive)
//   2. Brace alternation ({a,b{c,d}}), GlobSet matchesWhich, and malformed pattern errors
//   3. .gitignore IgnoreMatcher (comments, trailing spaces, ! negation, trailing / dir-only, leading / root anchor)
//   4. 4 classes of pathological wildcard / brace combinatorial-explosion guards (linear-time O(NM) & TooManyAlternates)
//   5. 16-lane @Vector(16, u8) SIMD scanners & equality 0-diff parity vs scalar reference
//   6. Deterministic PRNG Fuzz (>= 1,600 malformed glob patterns/paths + random SIMD parity)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    GLOB_MAX_ALTERNATES,
    GlobError,
    GlobOptions,
    GlobMatcher,
    simdIndexOfByte,
    simdIndexOfByteScalar,
    simdIndexOfAny4,
    simdIndexOfAny4Scalar,
    simdIndexOfGlobMagic,
    simdIndexOfGlobMagicScalar,
    simdBytesEqual,
    simdBytesEqualScalar,
} from "../../lib/text/glob.cp";
import {
    GlobSetBuilder,
    IgnoreMatcher,
} from "../../lib/text/globset.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn matchOne(alloc: std.mem.Allocator, pat: string, path: string, opts: GlobOptions): bool {
    let m = GlobMatcher.compile(alloc, pat, opts) catch return false;
    defer m.deinit();
    return m.matches(path);
}

fn compileFails(alloc: std.mem.Allocator, pat: string): bool {
    let m = GlobMatcher.compile(alloc, pat, GlobOptions{}) catch return true;
    m.deinit();
    return false;
}

fn fuzzOneMatcher(alloc: std.mem.Allocator, pat: string, path: string) {
    let m = GlobMatcher.compile(alloc, pat, GlobOptions{}) catch return;
    defer m.deinit();
    * = m.matches(path);
}

fn addExpectTooMany(alloc: std.mem.Allocator, pat: string): bool {
    let b = GlobSetBuilder.init(alloc);
    defer b.deinit();
    b.add(pat) catch (e) {
        return e == GlobError.TooManyAlternates;
    };
    return false;
}

fn main() {
    test("1. Core Glob syntax & BurntSushi/ripgrep globset test vectors", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const def = GlobOptions{};
        const ci = GlobOptions{ case_insensitive: true, literal_separator: true };
        const nolit = GlobOptions{ case_insensitive: false, literal_separator: false };

        // Literal fast-path (short & >16 bytes for SIMD path)
        expect_eq(matchOne(alloc, "src/runtime/core/engine.cp", "src/runtime/core/engine.cp", def), true);
        expect_eq(matchOne(alloc, "src/runtime/core/engine.cp", "src/runtime/core/engine.rs", def), false);

        // * and ? do not cross '/' under literal_separator = true
        expect_eq(matchOne(alloc, "a*b", "a_b", def), true);
        expect_eq(matchOne(alloc, "a*b", "a/b", def), false);
        expect_eq(matchOne(alloc, "a?b", "axb", def), true);
        expect_eq(matchOne(alloc, "a?b", "a/b", def), false);
        expect_eq(matchOne(alloc, "a*b", "a/b", nolit), true);

        // ** recursive: zero-level & multi-level directory, plus ripgrep anti-bleed vectors
        expect_eq(matchOne(alloc, "some/**/needle.txt", "some/needle.txt", def), true);
        expect_eq(matchOne(alloc, "some/**/needle.txt", "some/one/needle.txt", def), true);
        expect_eq(matchOne(alloc, "some/**/needle.txt", "some/one/two/needle.txt", def), true);
        expect_eq(matchOne(alloc, "some/**/**/needle.txt", "some/needle.txt", def), true);
        expect_eq(matchOne(alloc, "some/**/needle.txt", "some/xneedle.txt", def), false);
        expect_eq(matchOne(alloc, "**/foo", "foo", def), true);
        expect_eq(matchOne(alloc, "**/foo", "a/b/foo", def), true);
        expect_eq(matchOne(alloc, "**/foo", "foofoo", def), false);
        expect_eq(matchOne(alloc, "**/foo/bar", "foofoo/bar", def), false);
        expect_eq(matchOne(alloc, "/**/test", "/test", def), true);
        expect_eq(matchOne(alloc, "/**/test", "/one/two/test", def), true);
        expect_eq(matchOne(alloc, "/**/test", "test", def), false);
        expect_eq(matchOne(alloc, "test/**", "test/one/two", def), true);
        expect_eq(matchOne(alloc, "**", "a/b/c", def), true);

        // Character classes ([a-z], [!0-9], [^a-z], [-abc], [abc-], SLASHLIT exclusion)
        expect_eq(matchOne(alloc, "a[0-9]b", "a5b", def), true);
        expect_eq(matchOne(alloc, "a[0-9]b", "axb", def), false);
        expect_eq(matchOne(alloc, "a[!0-9]b", "axb", def), true);
        expect_eq(matchOne(alloc, "a[!0-9]b", "a5b", def), false);
        expect_eq(matchOne(alloc, "a[^0-9]b", "a_b", def), true);
        expect_eq(matchOne(alloc, "[-abc]", "-", def), true);
        expect_eq(matchOne(alloc, "[abc-]", "-", def), true);
        expect_eq(matchOne(alloc, "abc[/]def", "abc/def", def), false);
        expect_eq(matchOne(alloc, "**/[.-0]", "/", def), false);
        expect_eq(matchOne(alloc, "**/[!a]", "/", def), false);

        // Escapes & case-insensitive
        expect_eq(matchOne(alloc, "literal\\*.txt", "literal*.txt", def), true);
        expect_eq(matchOne(alloc, "literal\\*.txt", "literal_abc.txt", def), false);
        expect_eq(matchOne(alloc, "aBcDeFg", "abcdefg", ci), true);
        expect_eq(matchOne(alloc, "*.CP", "main.cp", ci), true);

        // Invalid patterns rejected
        expect_eq(compileFails(alloc, ""), true);
        expect_eq(compileFails(alloc, "[z-a]"), true);
        expect_eq(compileFails(alloc, "[unclosed"), true);
        expect_eq(compileFails(alloc, "trailing\\"), true);
    });

    test("2. Brace alternation ({a,b{c,d}}), GlobSet matchesWhich & error handling", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let builder = GlobSetBuilder.init(alloc);
        defer builder.deinit();

        builder.add("{src,lib}/**/*.{cp,zig}") catch unreachable;
        builder.add("{a,b{c,d}}.txt") catch unreachable;
        builder.add("lib/**/*.cp") catch unreachable;

        // Unclosed / unmatched braces must fail cleanly
        let bad1 = false;
        builder.add("{unclosed") catch { bad1 = true; };
        expect_eq(bad1, true);

        let bad2 = false;
        builder.add("unopened}") catch { bad2 = true; };
        expect_eq(bad2, true);

        let gs = builder.build() catch unreachable;
        defer gs.deinit();

        expect_eq(gs.len(), 3 as usize);
        expect_eq(gs.matches("src/core/main.cp"), true);
        expect_eq(gs.matches("lib/text/glob.zig"), true);
        expect_eq(gs.matches("bc.txt"), true);
        expect_eq(gs.matches("bd.txt"), true);
        expect_eq(gs.matches("a.txt"), true);
        expect_eq(gs.matches("docs/readme.md"), false);

        const which = gs.matchesWhich(alloc, "lib/text/glob.cp") catch unreachable;
        defer alloc.free(which);
        expect_eq(which.len, 2 as usize);
        expect_eq(which[0], 0 as usize);
        expect_eq(which[1], 2 as usize);
    });

    test("3. .gitignore IgnoreMatcher (comments, negation, trailing /, root /, trailing spaces)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let ig = IgnoreMatcher.init(alloc);
        defer ig.deinit();

        const rules =
            "# comment line\n" ++
            "   \n" ++
            "*.log   \n" ++
            "!important.log\n" ++
            "build/\n" ++
            "/root_only.txt\n" ++
            "docs/*.tmp\n";
        ig.parse(rules) catch unreachable;
        expect_eq(ig.ruleCount(), 5 as usize);

        expect_eq(ig.isIgnored("debug.log", false), true);
        expect_eq(ig.isIgnored("sub/app.log", false), true);
        expect_eq(ig.isIgnored("important.log", false), false);
        expect_eq(ig.isIgnored("sub/important.log", false), false);

        // Trailing / matches directories only
        expect_eq(ig.isIgnored("build", true), true);
        expect_eq(ig.isIgnored("sub/build", true), true);
        expect_eq(ig.isIgnored("build", false), false);

        // Leading / anchors to root
        expect_eq(ig.isIgnored("root_only.txt", false), true);
        expect_eq(ig.isIgnored("nested/root_only.txt", false), false);

        // Embedded / anchors to root
        expect_eq(ig.isIgnored("docs/a.tmp", false), true);
        expect_eq(ig.isIgnored("x/docs/a.tmp", false), false);
    });

    test("4. 4 pathological wildcard & brace combinatorial-explosion guards (linear O(NM))", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const def = GlobOptions{};

        // 4A. Catastrophic backtracking killer: *a*a*a*a*a*a*a*a*b vs 64 'a's
        const path_64a = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        expect_eq(matchOne(alloc, "*a*a*a*a*a*a*a*a*b", path_64a, def), false);
        expect_eq(matchOne(alloc, "a*a*a*a*a*a*a*a*a", path_64a, def), true);

        // 4B. Repeated **/**/**/**/**/x vs deep non-matching path
        const deep_path = "/a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t/u/v/w/y";
        expect_eq(matchOne(alloc, "/**/**/**/**/**/x", deep_path, def), false);
        expect_eq(matchOne(alloc, "/**/**/**/**/**/y", deep_path, def), true);

        // 4C. Brace combinatorial explosion: 2^7 = 128 > GLOB_MAX_ALTERNATES (64)
        expect_eq(GLOB_MAX_ALTERNATES, 64 as usize);
        expect_eq(addExpectTooMany(alloc, "{a,b}{a,b}{a,b}{a,b}{a,b}{a,b}{a,b}"), true);

        // 4D. Excessive brace nesting depth (> GLOB_MAX_BRACE_DEPTH = 16)
        expect_eq(addExpectTooMany(alloc, "{{{{{{{{{{{{{{{{{a}}}}}}}}}}}}}}}}}"), true);
    });

    test("5. 16-lane SIMD scanners & equality 0-diff parity vs scalar (alignment boundaries)", () => {
        const s1 = "0123456789abcdef/ghijklmnopqrstuvwxyz";
        const s2 = "0123456789abcdef*ghijklmnopqrstuvwxyz";
        const s3 = "no_magic_chars_anywhere_in_this_long_literal_path_segment_123456";
        const s4 = "no_magic_chars_anywhere_in_this_long_literal_path_segment_123457";

        expect_eq(simdIndexOfByte(s1, '/'), simdIndexOfByteScalar(s1, '/'));
        expect_eq(simdIndexOfByte(s3, '/'), simdIndexOfByteScalar(s3, '/'));
        expect_eq(simdIndexOfAny4(s2, '*', '?', '[', '{'), simdIndexOfAny4Scalar(s2, '*', '?', '[', '{'));
        expect_eq(simdIndexOfGlobMagic(s2), simdIndexOfGlobMagicScalar(s2));
        expect_eq(simdIndexOfGlobMagic(s3), simdIndexOfGlobMagicScalar(s3));
        expect_eq(simdBytesEqual(s3, s3), simdBytesEqualScalar(s3, s3));
        expect_eq(simdBytesEqual(s3, s4), simdBytesEqualScalar(s3, s4));

        // Exhaustive length boundaries 0..=40
        let buf: [48]u8 = undefined;
        let len: usize = 0;
        while (len <= 40) : (len += 1) {
            let k: usize = 0;
            while (k < len) : (k += 1) {
                buf[k] = 'a';
            }
            const slice = buf[0..len];
            expect_eq(simdIndexOfByte(slice, '/'), simdIndexOfByteScalar(slice, '/'));
            expect_eq(simdIndexOfGlobMagic(slice), simdIndexOfGlobMagicScalar(slice));
            if (len > 0) {
                buf[len - 1] = '?';
                expect_eq(simdIndexOfGlobMagic(slice), simdIndexOfGlobMagicScalar(slice));
                expect_eq(simdIndexOfAny4(slice, '*', '?', '[', '{'), simdIndexOfAny4Scalar(slice, '*', '?', '[', '{'));
            }
        }
    });

    test("6. Deterministic PRNG Fuzz (>= 1,600 malformed glob/path & random SIMD parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const alphabet: []const u8 = "abcXYZ019/*?[]{}!^-,\\. #\r\n";
        let seed: u64 = 0x5EED_610B_2026_0033;
        let pat_buf: [64]u8 = undefined;
        let path_buf: [64]u8 = undefined;

        // 6A. 1,000 malformed glob pattern + path fuzz iterations
        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const plen: usize = ((nextRng(&seed) % 32) + 1) as usize;
            const tlen: usize = ((nextRng(&seed) % 40) + 1) as usize;
            let k: usize = 0;
            while (k < plen) : (k += 1) {
                pat_buf[k] = alphabet[(nextRng(&seed) % (alphabet.len as u64)) as usize];
            }
            k = 0;
            while (k < tlen) : (k += 1) {
                path_buf[k] = alphabet[(nextRng(&seed) % (alphabet.len as u64)) as usize];
            }
            const pat = pat_buf[0..plen];
            const path = path_buf[0..tlen];

            fuzzOneMatcher(alloc, pat, path);

            let gb = GlobSetBuilder.init(alloc);
            gb.add(pat) catch {};
            gb.deinit();

            let ig = IgnoreMatcher.init(alloc);
            ig.addRule(pat) catch {};
            * = ig.isIgnored(path, (iter & 1) == 0);
            ig.deinit();
        }

        // 6B. 600 random byte-slice SIMD vs scalar parity checks
        let a_buf: [64]u8 = undefined;
        let b_buf: [64]u8 = undefined;
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            const n: usize = (nextRng(&seed) % 64) as usize;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                const c = alphabet[(nextRng(&seed) % (alphabet.len as u64)) as usize];
                a_buf[j] = c;
                b_buf[j] = if ((r_iter & 1) == 0) c else alphabet[(nextRng(&seed) % (alphabet.len as u64)) as usize];
            }
            const sa = a_buf[0..n];
            const sb = b_buf[0..n];
            expect_eq(simdIndexOfByte(sa, '/'), simdIndexOfByteScalar(sa, '/'));
            expect_eq(simdIndexOfAny4(sa, '*', '?', '[', '{'), simdIndexOfAny4Scalar(sa, '*', '?', '[', '{'));
            expect_eq(simdIndexOfGlobMagic(sa), simdIndexOfGlobMagicScalar(sa));
            expect_eq(simdBytesEqual(sa, sb), simdBytesEqualScalar(sa, sb));
        }
    });
}
