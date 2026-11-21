import {
    minCommonIndent,
    dedent,
    indentLines,
    stripCommonPrefix,
} from "cap:text";
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

test("minCommonIndent detects spaces, tabs, ignores blank lines, and handles empty input", () => {
    expect(minCommonIndent("") == 0);
    expect(minCommonIndent("   \n\t\t\n  ") == 0);
    expect(minCommonIndent("no indent\n    four") == 0);
    expect(minCommonIndent("    line1\n      line2\n   \n    line3") == 4);
    expect(minCommonIndent("\t\tline1\n\tline2\n\n\t\t\tline3") == 1);
    expect(minCommonIndent("  a\r\n    b\r\n   \r\n  c\r\n") == 2);
});

test("dedent strips common indent, normalizes blank lines, and preserves LF and CRLF", () => {
    let buf: [256]u8 = [0; 256];

    // LF block with whitespace-only line
    const lf_block = "    fn main() {\n        let x = 1;\n      \n    }\n";
    const expected_lf = "fn main() {\n    let x = 1;\n\n}\n";
    expect(strEq(dedent(lf_block, buf[0..]).?, expected_lf));

    // CRLF preservation
    const crlf_block = "  alpha\r\n    beta\r\n   \r\n  gamma";
    const expected_crlf = "alpha\r\n  beta\r\n\r\ngamma";
    expect(strEq(dedent(crlf_block, buf[0..]).?, expected_crlf));

    // Already zero-indented text still normalizes whitespace-only lines
    const zero_ind = "top\n   \n  nested";
    expect(strEq(dedent(zero_ind, buf[0..]).?, "top\n\n  nested"));
});

test("indentLines supports indent_empty true and false with LF/CRLF", () => {
    let buf: [256]u8 = [0; 256];

    const src = "line1\n\n   \nline2\n";
    // Skip empty & whitespace-only lines
    const skip_empty = indentLines(src, "  ", false, buf[0..]).?;
    expect(strEq(skip_empty, "  line1\n\n   \n  line2\n"));

    // Indent all lines including empty & whitespace-only
    const with_empty = indentLines(src, "> ", true, buf[0..]).?;
    expect(strEq(with_empty, "> line1\n> \n>    \n> line2\n"));

    // CRLF preserved
    const crlf = "a\r\n\r\nb";
    expect(strEq(indentLines(crlf, "\t", false, buf[0..]).?, "\ta\r\n\r\n\tb"));
});

test("stripCommonPrefix strips matching prefix per line and leaves non-matching lines intact", () => {
    let buf: [256]u8 = [0; 256];

    const quoted = "> first\n> second\nplain line\n> third\n";
    const unquoted = stripCommonPrefix(quoted, "> ", buf[0..]).?;
    expect(strEq(unquoted, "first\nsecond\nplain line\nthird\n"));

    // Comment prefix with CRLF and UTF-8
    const commented = "// 你好\r\n// world\r\nkeep\r\n";
    expect(strEq(stripCommonPrefix(commented, "// ", buf[0..]).?, "你好\r\nworld\r\nkeep\r\n"));

    // Empty prefix copies text unchanged
    expect(strEq(stripCommonPrefix("abc\ndef", "", buf[0..]).?, "abc\ndef"));
});

test("buffer capacity boundaries and empty input edge cases", () => {
    let buf: [32]u8 = [0; 32];

    // Empty input returns empty slice across all operations
    expect(strEq(dedent("", buf[0..]).?, ""));
    expect(strEq(indentLines("", "  ", true, buf[0..]).?, ""));
    expect(strEq(stripCommonPrefix("", "// ", buf[0..]).?, ""));

    // Exact buffer fit vs 1-byte overflow returns null
    const d_in = "  ab\n  cd"; // dedent -> "ab\ncd" (5 bytes)
    expect(strEq(dedent(d_in, buf[0..5]).?, "ab\ncd"));
    expect(dedent(d_in, buf[0..4]) == null);

    // indentLines("a\nb", ">>", false) -> ">>a\n>>b" (7 bytes)
    expect(strEq(indentLines("a\nb", ">>", false, buf[0..7]).?, ">>a\n>>b"));
    expect(indentLines("a\nb", ">>", false, buf[0..6]) == null);

    // stripCommonPrefix("//a\nb", "//") -> "a\nb" (3 bytes)
    expect(strEq(stripCommonPrefix("//a\nb", "//", buf[0..3]).?, "a\nb"));
    expect(stripCommonPrefix("//a\nb", "//", buf[0..2]) == null);
});
