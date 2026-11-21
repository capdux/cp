import { test, expect_eq } from "cap:test";
import {
    DiffKind,
    DiffOptions,
    areLinesEqual,
    splitLines,
    diffLines,
    diffBytes,
    formatLineDiff,
} from "cap:myers_diff";

test("1. Identical & empty line slices produce zero edits", () => {
    const alloc = std.testing.allocator;
    const empty: [0]string = [];
    let r0 = diffLines(alloc, empty[0..], empty[0..], DiffOptions{}) catch unreachable;
    expect_eq(r0.len(), usize(0));
    expect_eq(r0.isIdentical(), true);

    const same: [3]string = ["one", "two", "three"];
    let r1 = diffLines(alloc, same[0..], same[0..], DiffOptions{}) catch unreachable;
    expect_eq(r1.len(), usize(3));
    expect_eq(r1.editDistance(), usize(0));
    expect_eq(r1.isIdentical(), true);
    expect_eq(r1.countKind(.equal), usize(3));
});

test("2. Line insertions, deletions, and replacements in chronological order", () => {
    const alloc = std.testing.allocator;
    const actual: [4]string = ["a", "b", "x", "d"];
    const expected: [3]string = ["a", "b", "c"];
    let res = diffLines(alloc, actual[0..], expected[0..], DiffOptions{}) catch unreachable;
    expect_eq(res.countKind(.equal), usize(2));
    expect_eq(res.countKind(.insert), usize(2));
    expect_eq(res.countKind(.delete), usize(1));
    expect_eq(res.editDistance(), usize(3));
    expect_eq(res.items[0].kind == .equal, true);
    expect_eq(std.mem.eql(u8, res.items[0].value, "a"), true);
    expect_eq(res.items[1].kind == .equal, true);
    expect_eq(std.mem.eql(u8, res.items[1].value, "b"), true);
});

test("3. Trailing comma disparity mode treats 'foo,' == 'foo'", () => {
    expect_eq(areLinesEqual("foo,", "foo", false), false);
    expect_eq(areLinesEqual("foo,", "foo", true), true);
    expect_eq(areLinesEqual("foo", "foo,", true), true);
    expect_eq(areLinesEqual("foo,,", "foo", true), false);

    const alloc = std.testing.allocator;
    const a: [2]string = ["  x: 1,", "  y: 2"];
    const b: [2]string = ["  x: 1", "  y: 2,"];
    let strict = diffLines(alloc, a[0..], b[0..], DiffOptions{ check_comma_disparity: false }) catch unreachable;
    expect_eq(strict.editDistance(), usize(4));

    let relaxed = diffLines(alloc, a[0..], b[0..], DiffOptions{ check_comma_disparity: true }) catch unreachable;
    expect_eq(relaxed.isIdentical(), true);
    expect_eq(relaxed.countKind(.equal), usize(2));
});

test("4. Byte-level Myers diff on classic ABCABBA vs CBABAC", () => {
    const alloc = std.testing.allocator;
    let res = diffBytes(alloc, "ABCABBA", "CBABAC") catch unreachable;
    // Classic Myers paper optimal edit distance = 5 (LCS length = 4)
    expect_eq(res.editDistance(), usize(5));
    expect_eq(res.isIdentical(), false);

    let same = diffBytes(alloc, "hello", "hello") catch unreachable;
    expect_eq(same.isIdentical(), true);
    expect_eq(same.len(), usize(5));
});

test("5. splitLines and formatLineDiff roundtrip with zero memory leaks", () => {
    const alloc = std.testing.allocator;
    const lines_a = splitLines(alloc, "head\nold\ntail") catch unreachable;
    defer alloc.free(lines_a);
    const lines_b = splitLines(alloc, "head\nnew\ntail") catch unreachable;
    defer alloc.free(lines_b);

    expect_eq(lines_a.len, usize(3));
    expect_eq(lines_b.len, usize(3));

    let res = diffLines(alloc, lines_a, lines_b, DiffOptions{}) catch unreachable;
    expect_eq(res.countKind(.equal), usize(2));
    expect_eq(res.editDistance(), usize(2));

    const rendered = formatLineDiff(alloc, res.items) catch unreachable;
    defer alloc.free(rendered);
    expect_eq(std.mem.indexOf(u8, rendered, "  head\n") != null, true);
    expect_eq(std.mem.indexOf(u8, rendered, "+ old\n") != null, true);
    expect_eq(std.mem.indexOf(u8, rendered, "- new\n") != null, true);
    expect_eq(std.mem.indexOf(u8, rendered, "  tail\n") != null, true);
});
