// Deep production regression tests for cap:css cssString.cp
// Covers CSSOM double/single quote serialization, NUL -> U+FFFD replacement,
// control-byte hex escaping, CSS Syntax L3 Unicode hex escapes (1..6 digits + trailing ws),
// escaped line continuations, and malformed/buffer-size error guards.

import { test, expect, expect_eq } from "cap:test";
import {
    CssStringError,
    CssQuoteStyle,
    CssString,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn expectStrErr(res: CssStringError!usize, want: CssStringError) {
    let got_err = false;
    * = res catch |e| cb: {
        expect_eq(e, want);
        got_err = true;
        break :cb 0;
    };
    expect(got_err);
}

test("cssString: serialize & parse round-trip for plain, quoted and backslash strings", () => {
    let enc: [128]u8 = [0; 128];
    let out_dec: [128]u8 = [0; 128];

    expect(!CssString.needsEscaping("Inter, sans-serif", .Double));
    expect(CssString.needsEscaping("a\"b", .Double));
    expect(!CssString.needsEscaping("a\"b", .Single));
    expect(CssString.needsEscaping("a'b", .Single));

    const n1 = CssString.serialize("Fira Code", enc[0..]) catch unreachable;
    expect(strEq(enc[0..n1], "\"Fira Code\""));
    const d1 = CssString.parse(enc[0..n1], out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d1], "Fira Code"));

    // Double quotes and backslashes escaped properly
    const raw = "say \"hi\" \\ path";
    const n2 = CssString.serialize(raw, enc[0..]) catch unreachable;
    expect(strEq(enc[0..n2], "\"say \\\"hi\\\" \\\\ path\""));
    const d2 = CssString.parse(enc[0..n2], out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d2], raw));

    // Single-quote serialization leaves double quotes unescaped
    const n3 = CssString.serializeWithQuote("it's \"ok\"", .Single, enc[0..]) catch unreachable;
    expect(strEq(enc[0..n3], "'it\\'s \"ok\"'"));
    const d3 = CssString.parse(enc[0..n3], out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d3], "it's \"ok\""));
});

test("cssString: control bytes hex escaping and NUL -> U+FFFD replacement", () => {
    let enc: [64]u8 = [0; 64];
    let out_dec: [64]u8 = [0; 64];

    // LF (0x0A) -> \a , DEL (0x7F) -> \7f
    let ctrl: [4]u8 = ['A', '\n', 0x7F, 'B'];
    const n1 = CssString.serialize(ctrl[0..], enc[0..]) catch unreachable;
    expect(strEq(enc[0..n1], "\"A\\a \\7f B\""));
    const d1 = CssString.parse(enc[0..n1], out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d1], ctrl[0..]));

    // NUL byte (0x00) serialized as UTF-8 U+FFFD (EF BF BD)
    let nul_in: [3]u8 = ['x', 0, 'y'];
    const n2 = CssString.serialize(nul_in[0..], enc[0..]) catch unreachable;
    expect(n2 == 7); // '"' + 'x' + 3 bytes U+FFFD + 'y' + '"'
    expect(enc[2] == 0xEF && enc[3] == 0xBF && enc[4] == 0xBD);
});

test("cssString: CSS Syntax L3 Unicode hex escapes (1..6 digits, trailing space, UTF-8)", () => {
    let out_dec: [64]u8 = [0; 64];

    // \41 + single space consumed -> "A", followed by "B"
    const d1 = CssString.parse("\"\\41 B\"", out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d1], "AB"));

    // 6-digit hex escape consumes up to 6 digits + optional space: \01f680 -> U+1F680 (🚀: F0 9F 9A 80)
    const d2 = CssString.parse("\"\\01f680!\"", out_dec[0..]) catch unreachable;
    expect(d2 == 5);
    expect(out_dec[0] == 0xF0 && out_dec[1] == 0x9F && out_dec[2] == 0x9A && out_dec[3] == 0x80 && out_dec[4] == '!');

    // Invalid codepoints (\0, surrogate \d800, overflow \110000) sanitize to U+FFFD
    const d3 = CssString.parse("\"\\0\"", out_dec[0..]) catch unreachable;
    expect(d3 == 3 && out_dec[0] == 0xEF && out_dec[1] == 0xBF && out_dec[2] == 0xBD);

    const d4 = CssString.parse("\"\\d800\"", out_dec[0..]) catch unreachable;
    expect(d4 == 3 && out_dec[0] == 0xEF && out_dec[1] == 0xBF && out_dec[2] == 0xBD);

    const d5 = CssString.parse("\"\\110000\"", out_dec[0..]) catch unreachable;
    expect(d5 == 3 && out_dec[0] == 0xEF && out_dec[1] == 0xBF && out_dec[2] == 0xBD);
});

test("cssString: escaped newline continuations (LF, CRLF, CR) collapse cleanly", () => {
    let out_dec: [64]u8 = [0; 64];

    const d1 = CssString.parse("\"hello\\\nworld\"", out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d1], "helloworld"));

    const d2 = CssString.parse("\"foo\\\r\nbar\"", out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d2], "foobar"));

    const d3 = CssString.parse("'one\\\rtwo'", out_dec[0..]) catch unreachable;
    expect(strEq(out_dec[0..d3], "onetwo"));
});

test("cssString: malformed quoted strings and isValidQuoted guards", () => {
    let out_dec: [32]u8 = [0; 32];

    expect(CssString.isValidQuoted("\"\""));
    expect(CssString.isValidQuoted("'hello \\'world\\''"));
    expect(!CssString.isValidQuoted(""));
    expect(!CssString.isValidQuoted("\""));
    expect(!CssString.isValidQuoted("\"unterminated"));
    expect(!CssString.isValidQuoted("\"bare\nnewline\""));
    expect(!CssString.isValidQuoted("\"trailing\"extra"));

    expectStrErr(CssString.parse("", out_dec[0..]), CssStringError.EmptyInput);
    expectStrErr(CssString.parse("\"", out_dec[0..]), CssStringError.EmptyInput);
    expectStrErr(CssString.parse("abc", out_dec[0..]), CssStringError.UnterminatedString);
    expectStrErr(CssString.parse("\"abc", out_dec[0..]), CssStringError.UnterminatedString);
    expectStrErr(CssString.parse("\"a\nb\"", out_dec[0..]), CssStringError.UnescapedNewline);
    expectStrErr(CssString.parse("\"abc\\", out_dec[0..]), CssStringError.InvalidEscape);
    expectStrErr(CssString.parse("\"a\"b", out_dec[0..]), CssStringError.UnterminatedString);
});

test("cssString: BufferTooSmall protection on serialize and parse", () => {
    let tiny: [3]u8 = [0; 3];
    expectStrErr(CssString.serialize("abcd", tiny[0..]), CssStringError.BufferTooSmall);
    expectStrErr(CssString.parse("\"abcd\"", tiny[0..]), CssStringError.BufferTooSmall);

    // Exact-size buffer succeeds
    let exact: [4]u8 = [0; 4];
    const n = CssString.serialize("ab", exact[0..]) catch unreachable;
    expect(n == 4 && strEq(exact[0..n], "\"ab\""));
});
