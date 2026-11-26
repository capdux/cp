import { test, expect_eq } from "cap:test";
import {
    CssEscOptions,
    defaultCssEscOptions,
    cssescBuf,
    cssesc,
} from "../../packages/@cap/cssesc/src/index.cp";

fn main() {
    test("1. @cap/cssesc — Identifier escaping: leading digits, -digit & lone dash", () => {
        const id_opts = CssEscOptions{
            is_identifier: true,
            quotes: .Single,
            wrap: false,
            escape_everything: false,
        };
        expect_eq(std.mem.eql(u8, cssesc("123a", id_opts), "\\31 23a"), true);
        expect_eq(std.mem.eql(u8, cssesc("-123a", id_opts), "-\\31 23a"), true);
        expect_eq(std.mem.eql(u8, cssesc("-", id_opts), "\\-"), true);
        expect_eq(std.mem.eql(u8, cssesc("--custom-prop", id_opts), "--custom-prop"), true);
        expect_eq(std.mem.eql(u8, cssesc("a.b#c:hover", id_opts), "a\\.b\\#c\\:hover"), true);
    });

    test("2. @cap/cssesc — String mode with single/double quotes & wrap", () => {
        const s_opts = CssEscOptions{
            is_identifier: false,
            quotes: .Single,
            wrap: true,
            escape_everything: false,
        };
        const d_opts = CssEscOptions{
            is_identifier: false,
            quotes: .Double,
            wrap: true,
            escape_everything: false,
        };
        const raw = "Lorem ipsum \"dolor\" sit 'amet'";
        expect_eq(std.mem.eql(u8, cssesc(raw, s_opts), "'Lorem ipsum \"dolor\" sit \\'amet\\''"), true);
        expect_eq(std.mem.eql(u8, cssesc(raw, d_opts), "\"Lorem ipsum \\\"dolor\\\" sit 'amet'\""), true);
    });

    test("3. @cap/cssesc — Non-ASCII & astral plane UTF-8 hex escapes", () => {
        const def = defaultCssEscOptions();
        // © is U+00A9 (UTF-8: C2 A9)
        expect_eq(std.mem.eql(u8, cssesc("foo\xC2\xA9bar", def), "foo\\a9 bar"), true);
        expect_eq(std.mem.eql(u8, cssesc("\xC2\xA9", def), "\\a9"), true);

        // 𝌆 is U+1D306 (UTF-8: F0 9D 8C 86)
        const id_opts = CssEscOptions{
            is_identifier: true,
            quotes: .Single,
            wrap: false,
            escape_everything: false,
        };
        expect_eq(std.mem.eql(u8, cssesc("foo \xF0\x9D\x8C\x86 bar", id_opts), "foo\\ \\1d306 \\ bar"), true);
    });

    test("4. @cap/cssesc — Control characters, NUL & backslashes", () => {
        const def = defaultCssEscOptions();
        expect_eq(std.mem.eql(u8, cssesc("a\x01b", def), "a\\1 b"), true);
        expect_eq(std.mem.eql(u8, cssesc("a\x7F", def), "a\\7f"), true);
        expect_eq(std.mem.eql(u8, cssesc("a\\b", def), "a\\\\b"), true);
        // NUL -> U+FFFD (EF BF BD)
        expect_eq(std.mem.eql(u8, cssesc("\x00", def), "\xEF\xBF\xBD"), true);
    });

    test("5. @cap/cssesc — escape_everything mode", () => {
        const all_opts = CssEscOptions{
            is_identifier: false,
            quotes: .Single,
            wrap: false,
            escape_everything: true,
        };
        expect_eq(std.mem.eql(u8, cssesc("ab", all_opts), "\\61 \\62"), true);
    });

    test("6. @cap/cssesc — cssescBuf overflow guard & empty string", () => {
        const def = defaultCssEscOptions();
        let tiny: [2]u8 = undefined;
        expect_eq(cssescBuf("abcdef", def, tiny[0..2]), 0 as usize);
        expect_eq(std.mem.eql(u8, cssesc("", def), ""), true);
    });
}
