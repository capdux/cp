// Source: cp/cap/test/test_globset_production_deep.cp
// Deep Production Verification Suite for cap:globset & cap.globset
// Covers:
//   1. GlobMatcher: *, **, ?, [a-z], [!0-9], [^a-f], \*, a/**/b zero-or-more dirs, literal_separator
//   2. GlobSet: nested brace expansion {ts,tsx,{c,h}} and matchesWhich multi-rule hits
//   3. IgnoreMatcher: .gitignore rules (# comments, blank lines, ! negation, trailing / dir-only, leading / root-anchor)
//   4. Error interception (UnclosedBracket, UnclosedBrace, EmptyPattern, InvalidPattern) & case_insensitive
//   5. `import cap from "cap"; cap.globset` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GlobError,
    GlobOptions,
    GlobMatcher,
    GlobSet,
    GlobSetBuilder,
    IgnoreMatcher,
} from "cap:globset";

test("cap:globset - GlobMatcher *, **, ?, char classes, escapes, and a/**/b") {
    const alloc = std.testing.allocator;
    const opts = GlobOptions{};

    // Classic * with literal_separator = true
    let m1 = GlobMatcher.compile(alloc, "src/*.cp", opts) catch unreachable;
    defer m1.deinit();
    expect_eq(m1.matches("src/main.cp"), true);
    expect_eq(m1.matches("src/nested/main.cp"), false);
    expect_eq(m1.matches("main.cp"), false);

    // a/**/b zero or more intermediate directories
    let m2 = GlobMatcher.compile(alloc, "a/**/b", opts) catch unreachable;
    defer m2.deinit();
    expect_eq(m2.matches("a/b"), true);
    expect_eq(m2.matches("a/x/b"), true);
    expect_eq(m2.matches("a/x/y/z/b"), true);
    expect_eq(m2.matches("ab"), false);
    expect_eq(m2.matches("ax/b"), false);

    // Leading **/
    let m3 = GlobMatcher.compile(alloc, "**/index.cp", opts) catch unreachable;
    defer m3.deinit();
    expect_eq(m3.matches("index.cp"), true);
    expect_eq(m3.matches("lib/globset/index.cp"), true);
    expect_eq(m3.matches("my_index.cp"), false);

    // ? and character classes [a-z], [!0-9], [^a-f]
    let m4 = GlobMatcher.compile(alloc, "file_?[a-z][!0-9][^a-f].txt", opts) catch unreachable;
    defer m4.deinit();
    expect_eq(m4.matches("file_1azg.txt"), true);
    expect_eq(m4.matches("file_1a0g.txt"), false);
    expect_eq(m4.matches("file_1aza.txt"), false);
    expect_eq(m4.matches("file_/azg.txt"), false);
    expect_eq(m4.matches("file_1a/g.txt"), false);

    // Escaped wildcard \* and \?
    let m5 = GlobMatcher.compile(alloc, "literal\\*\\?.txt", opts) catch unreachable;
    defer m5.deinit();
    expect_eq(m5.matches("literal*?.txt"), true);
    expect_eq(m5.matches("literalab.txt"), false);

    // literal_separator = false allows * to cross /
    let m6 = GlobMatcher.compile(alloc, "src/*.cp", GlobOptions{
        case_insensitive: false,
        literal_separator: false,
    }) catch unreachable;
    defer m6.deinit();
    expect_eq(m6.matches("src/a/b/c.cp"), true);
}

test("cap:globset - GlobSet nested brace expansion {ts,tsx,{c,h}} and matchesWhich") {
    const alloc = std.testing.allocator;
    let builder = GlobSetBuilder.init(alloc);
    defer builder.deinit();

    builder.add("**/*.{ts,tsx,{c,h}}") catch unreachable; // index 0
    builder.add("src/**") catch unreachable;             // index 1
    builder.add("*.md") catch unreachable;               // index 2
    builder.add("src/core/*.{ts,cp}") catch unreachable; // index 3

    let set = builder.build() catch unreachable;
    defer set.deinit();

    expect_eq(set.len(), 4);
    expect_eq(set.isEmpty(), false);
    expect_eq(set.matches("src/core/mod.tsx"), true);
    expect_eq(set.matches("include/hdr.h"), true);
    expect_eq(set.matches("include/hdr.c"), true);
    expect_eq(set.matches("README.md"), true);
    expect_eq(set.matches("README.txt"), false);

    // Simultaneous hits for src/core/engine.ts -> indices [0, 1, 3]
    const hits = set.matchesWhich(alloc, "src/core/engine.ts") catch unreachable;
    defer alloc.free(hits);
    expect_eq(hits.len, 3);
    expect_eq(hits[0], 0);
    expect_eq(hits[1], 1);
    expect_eq(hits[2], 3);

    // Nested brace hit for src/core/ffi.h -> indices [0, 1]
    const hits2 = set.matchesWhich(alloc, "src/core/ffi.h") catch unreachable;
    defer alloc.free(hits2);
    expect_eq(hits2.len, 2);
    expect_eq(hits2[0], 0);
    expect_eq(hits2[1], 1);

    // No match -> empty slice
    const hits3 = set.matchesWhich(alloc, "unknown.bin") catch unreachable;
    defer alloc.free(hits3);
    expect_eq(hits3.len, 0);
}

test("cap:globset - IgnoreMatcher .gitignore comments, ! negation, trailing /, root /") {
    const alloc = std.testing.allocator;
    let ig = IgnoreMatcher.init(alloc);
    defer ig.deinit();

    const gitignore =
        "# Build artifacts and logs\n" ++
        "   \n" ++
        "\t\n" ++
        "*.log\n" ++
        "!important.log\n" ++
        "build/\n" ++
        "/root_only.txt\n" ++
        "docs/*.tmp\n";

    ig.parse(gitignore) catch unreachable;

    // Unanchored *.log matches at root and nested paths
    expect_eq(ig.isIgnored("app.log", false), true);
    expect_eq(ig.isIgnored("var/log/app.log", false), true);

    // !important.log overrides *.log (last-match-wins)
    expect_eq(ig.isIgnored("important.log", false), false);
    expect_eq(ig.isIgnored("sub/important.log", false), false);

    // Trailing / only matches directories
    expect_eq(ig.isIgnored("build", true), true);
    expect_eq(ig.isIgnored("packages/a/build", true), true);
    expect_eq(ig.isIgnored("build", false), false);

    // Leading / anchors to root only
    expect_eq(ig.isIgnored("root_only.txt", false), true);
    expect_eq(ig.isIgnored("sub/root_only.txt", false), false);

    // Pattern with internal / is also root-anchored
    expect_eq(ig.isIgnored("docs/draft.tmp", false), true);
    expect_eq(ig.isIgnored("sub/docs/draft.tmp", false), false);
}

test("cap:globset - GlobError validation (UnclosedBracket, UnclosedBrace, EmptyPattern) & case_insensitive") {
    const alloc = std.testing.allocator;
    const opts = GlobOptions{};

    // EmptyPattern
    let got_empty = false;
    const r_empty: GlobMatcher? = GlobMatcher.compile(alloc, "", opts) catch |e| blk: {
        got_empty = (e == GlobError.EmptyPattern);
        break :blk null;
    };
    expect_eq(r_empty == null, true);
    expect_eq(got_empty, true);

    // UnclosedBracket
    let got_bracket = false;
    const r_bracket: GlobMatcher? = GlobMatcher.compile(alloc, "src/[a-z.cp", opts) catch |e| blk: {
        got_bracket = (e == GlobError.UnclosedBracket);
        break :blk null;
    };
    expect_eq(r_bracket == null, true);
    expect_eq(got_bracket, true);

    // InvalidPattern (reversed range & trailing \)
    let got_invalid_range = false;
    const r_range: GlobMatcher? = GlobMatcher.compile(alloc, "[z-a]", opts) catch |e| blk: {
        got_invalid_range = (e == GlobError.InvalidPattern);
        break :blk null;
    };
    expect_eq(r_range == null, true);
    expect_eq(got_invalid_range, true);

    let got_trailing_bs = false;
    const r_bs: GlobMatcher? = GlobMatcher.compile(alloc, "abc\\", opts) catch |e| blk: {
        got_trailing_bs = (e == GlobError.InvalidPattern);
        break :blk null;
    };
    expect_eq(r_bs == null, true);
    expect_eq(got_trailing_bs, true);

    // UnclosedBrace in GlobSetBuilder
    let b = GlobSetBuilder.init(alloc);
    defer b.deinit();
    let got_brace = false;
    b.add("*.{ts,tsx") catch |e| {
        got_brace = (e == GlobError.UnclosedBrace);
    };
    expect_eq(got_brace, true);

    let got_nested_brace = false;
    b.add("{a,{b,c}") catch |e| {
        got_nested_brace = (e == GlobError.UnclosedBrace);
    };
    expect_eq(got_nested_brace, true);

    // case_insensitive = true
    let ci = GlobMatcher.compile(alloc, "SRC/[A-F]*.CP", GlobOptions{
        case_insensitive: true,
        literal_separator: true,
    }) catch unreachable;
    defer ci.deinit();
    expect_eq(ci.matches("src/alpha.cp"), true);
    expect_eq(ci.matches("SRC/BETA.CP"), true);
    expect_eq(ci.matches("src/zeta.cp"), false);
}

test("cap:globset - cap.globset dynamic mounting & std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;

    let m = cap.globset.GlobMatcher.compile(alloc, "lib/*/index.cp", cap.globset.GlobOptions{}) catch unreachable;
    defer m.deinit();
    expect_eq(m.matches("lib/globset/index.cp"), true);
    expect_eq(m.matches("lib/a/b/index.cp"), false);

    let builder = cap.globset.GlobSetBuilder.init(alloc);
    defer builder.deinit();
    builder.add("*.{cp,zig}") catch unreachable;
    let set = builder.build() catch unreachable;
    defer set.deinit();
    expect_eq(set.matches("glob.cp"), true);
    expect_eq(set.matches("main.zig"), true);

    let ig = cap.globset.IgnoreMatcher.init(alloc);
    defer ig.deinit();
    ig.addRule("*.o") catch unreachable;
    expect_eq(ig.isIgnored("out/main.o", false), true);
}
