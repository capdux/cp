// Source: cp/cap/test/test_nfa_regex_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { RegexError, NfaRegex } from "cap:text";

test("1. ReDoS-immune linear-time matching on pathological pattern (a+)+b") {
    const alloc = std.testing.allocator;
    const re = try NfaRegex.compile(alloc, "^(a+)+b$");
    defer re.deinit();

    // 200 'a's without trailing 'b' would hang backtracking engines, finishes instantly here
    let long_a: [200]u8 = undefined;
    @memset(&long_a, 'a');
    expect_eq(try re.isMatch(alloc, &long_a), false);

    let ok_buf: [201]u8 = undefined;
    @memset(ok_buf[0..200], 'a');
    ok_buf[200] = 'b';
    expect_eq(try re.isMatch(alloc, &ok_buf), true);
}

test("2. Character classes, negated classes, escapes and anchors") {
    const alloc = std.testing.allocator;
    const re_id = try NfaRegex.compile(alloc, "^[a-z_][a-z0-9_]*$");
    defer re_id.deinit();
    expect_eq(try re_id.isMatch(alloc, "user_42"), true);
    expect_eq(try re_id.isMatch(alloc, "42user"), false);

    const re_digits = try NfaRegex.compile(alloc, "\\d+\\s+[^0-9]+");
    defer re_digits.deinit();
    const m = try re_digits.findFirst(alloc, "id=123   abc!");
    try std.testing.expect(m != null);
    expect_eq(m.?.start, 3);
    expect_eq(m.?.end, 13);
}

test("3. Alternation, grouping, ?, +, * and findAll non-overlapping spans") {
    const alloc = std.testing.allocator;
    const re = try NfaRegex.compile(alloc, "(cat|dog)s?");
    defer re.deinit();

    const spans = try re.findAll(alloc, "one cat, two dogs, many cats");
    defer alloc.free(spans);
    expect_eq(spans.len, 3);
    expect_eq(spans[0].start, 4);
    expect_eq(spans[0].end, 7);
    expect_eq(spans[1].start, 13);
    expect_eq(spans[1].end, 17);
    expect_eq(spans[2].start, 24);
    expect_eq(spans[2].end, 28);
}

test("4. Malformed pattern syntax error protection") {
    const alloc = std.testing.allocator;
    try std.testing.expectError(RegexError.UnmatchedParen, NfaRegex.compile(alloc, "(abc"));
    try std.testing.expectError(RegexError.UnmatchedParen, NfaRegex.compile(alloc, "abc)"));
    try std.testing.expectError(RegexError.UnclosedCharClass, NfaRegex.compile(alloc, "[a-z"));
    try std.testing.expectError(RegexError.NothingToRepeat, NfaRegex.compile(alloc, "*abc"));
    try std.testing.expectError(RegexError.TrailingEscape, NfaRegex.compile(alloc, "abc\\"));
    try std.testing.expectError(RegexError.InvalidRange, NfaRegex.compile(alloc, "[z-a]"));
}

test("5. Dynamic cap.nfa_regex namespace and zero-leak check") {
    const alloc = std.testing.allocator;
    const re = try cap.text.NfaRegex.compile(alloc, "cap-[0-9]+");
    defer re.deinit();
    expect_eq(try re.isMatch(alloc, "hello cap-2026!"), true);
    expect_eq(try re.isMatch(alloc, "hello cap-abc!"), false);
}
