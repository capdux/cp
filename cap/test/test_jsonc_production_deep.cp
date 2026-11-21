import {
    stripJsonc,
    stripJsoncPreserveLines,
    hasJsoncComments,
    countJsoncTrailingCommas,
} from "cap:jsonc";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn countNewlines(s: []const u8): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\n') n += 1;
        i += 1;
    }
    return n;
}

test("stripJsonc removes line comments, block comments, and trailing commas", () => {
    const src =
        "{\n" ++
        "  // single-line comment\n" ++
        "  \"name\": \"cp\", /* inline block */\n" ++
        "  \"items\": [1, 2, 3, /* trailing */ ],\n" ++
        "}\n";

    expect(hasJsoncComments(src));
    expect(countJsoncTrailingCommas(src) == 2);

    let buf: [256]u8 = [0; 256];
    const clean = stripJsonc(src, buf[0..]).?;
    const expected =
        "{\n" ++
        "  \n" ++
        "  \"name\": \"cp\", \n" ++
        "  \"items\": [1, 2, 3  ]\n" ++
        "}\n";
    expect(strEq(clean, expected));
    expect(!hasJsoncComments(clean));
    expect(countJsoncTrailingCommas(clean) == 0);
});

test("string literals protect //, /* */, escaped quotes, and commas inside strings", () => {
    const src = "{\"url\":\"https://cp-lang.dev/*docs*/\",\"esc\":\"a\\\"//b,}\",\"arr\":[\"x,]\"]}";

    expect(!hasJsoncComments(src));
    expect(countJsoncTrailingCommas(src) == 0);

    let buf: [256]u8 = [0; 256];
    const clean = stripJsonc(src, buf[0..]).?;
    expect(strEq(clean, src));

    // Escaped backslash right before closing quote: "\\" followed by real comment
    const esc_bs = "{\"path\":\"C:\\\\\" // comment\n}";
    expect(hasJsoncComments(esc_bs));
    const clean_bs = stripJsonc(esc_bs, buf[0..]).?;
    expect(strEq(clean_bs, "{\"path\":\"C:\\\\\" \n}"));
});

test("stripJsoncPreserveLines maintains exact line count across multi-line block comments", () => {
    const src =
        "{\n" ++
        "  /* line 2\n" ++
        "     line 3\n" ++
        "     line 4 */\n" ++
        "  \"target\": \"esnext\", // line 5\n" ++
        "}\n";

    let buf_compact: [256]u8 = [0; 256];
    let buf_lines: [256]u8 = [0; 256];

    const compact = stripJsonc(src, buf_compact[0..]).?;
    const preserved = stripJsoncPreserveLines(src, buf_lines[0..]).?;

    // Original has 6 newlines; preserved keeps all 6, compact drops 2 inside block comment
    expect(countNewlines(src) == 6);
    expect(countNewlines(preserved) == 6);
    expect(countNewlines(compact) == 4);
    expect(countJsoncTrailingCommas(preserved) == 0);
});

test("trailing comma detection works across interleaved comments and nested structures", () => {
    const src =
        "{\n" ++
        "  \"a\": [\n" ++
        "    1,\n" ++
        "    2, // comment after comma\n" ++
        "    /* block */\n" ++
        "  ], // outer trailing comma\n" ++
        "}";

    expect(countJsoncTrailingCommas(src) == 2);
    let buf: [256]u8 = [0; 256];
    const clean = stripJsonc(src, buf[0..]).?;
    expect(countJsoncTrailingCommas(clean) == 0);
    expect(strEq(clean, "{\n  \"a\": [\n    1,\n    2 \n    \n  ] \n}"));
});

test("error protection for unterminated comments, unterminated strings, and buffer overflow", () => {
    let buf: [64]u8 = [0; 64];

    // Unterminated block comment
    expect(stripJsonc("{\"a\": 1 /* unclosed", buf[0..]) == null);
    expect(stripJsoncPreserveLines("{\"a\": 1 /*\nunclosed", buf[0..]) == null);
    // Unterminated block comment after comma
    expect(stripJsonc("[1, /* unclosed", buf[0..]) == null);

    // Unterminated string literal and dangling escape
    expect(stripJsonc("{\"a\": \"unclosed", buf[0..]) == null);
    expect(stripJsonc("{\"a\": \"bad\\", buf[0..]) == null);

    // Exact buffer fit vs 1-byte overflow
    const simple = "[1,]"; // stripped -> "[1]" (3 bytes)
    expect(strEq(stripJsonc(simple, buf[0..3]).?, "[1]"));
    expect(stripJsonc(simple, buf[0..2]) == null);

    // Empty input
    expect(strEq(stripJsonc("", buf[0..]).?, ""));
    expect(!hasJsoncComments(""));
    expect(countJsoncTrailingCommas("") == 0);
});
