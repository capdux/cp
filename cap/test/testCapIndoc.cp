// cap/test/testCapIndoc.cp
// Comprehensive unit tests for @cap/indoc

import { test, expect_eq } from "cap:test";
import {
    minCommonIndent,
    dedent,
    indent,
    stripMargin,
    normalizeNewlines,
    trimBlankLines,
} from "../../packages/indoc/src/index.cp";

fn main() {
    test("1. @cap/indoc — Standard multiline dedent with outer blank lines & nested indent", () => {
        let out: [512]u8 = undefined;
        const sql = "\n    SELECT id, name\n      FROM users\n    WHERE active = 1\n    ";
        expect_eq(minCommonIndent(sql), 4 as usize);
        expect_eq(
            dedent(sql, out[0..512]),
            "SELECT id, name\n  FROM users\nWHERE active = 1",
        );
    });

    test("2. @cap/indoc — Blank & whitespace-only interior lines cleared cleanly", () => {
        let out: [512]u8 = undefined;
        const doc = "      line1\n   \n        nested\n      line3";
        expect_eq(minCommonIndent(doc), 6 as usize);
        expect_eq(
            dedent(doc, out[0..512]),
            "line1\n\n  nested\nline3",
        );
    });

    test("3. @cap/indoc — indent with skip_empty=true vs skip_empty=false", () => {
        let out: [512]u8 = undefined;
        const src = "alpha\n\nbeta";
        expect_eq(indent(src, "> ", true, out[0..512]), "> alpha\n\n> beta");
        expect_eq(indent(src, "> ", false, out[0..512]), "> alpha\n> \n> beta");
    });

    test("4. @cap/indoc — Scala/Kotlin stripMargin('|') for embedded blocks", () => {
        let out: [512]u8 = undefined;
        const tpl = "\n        |fn main() {\n        |    print(\"hi\");\n        |}\n        ";
        expect_eq(
            stripMargin(tpl, '|', out[0..512]),
            "fn main() {\n    print(\"hi\");\n}",
        );
    });

    test("5. @cap/indoc — normalizeNewlines (CRLF & CR -> LF)", () => {
        let out: [256]u8 = undefined;
        expect_eq(normalizeNewlines("a\r\nb\rc\nd", out[0..256]), "a\nb\nc\nd");
        expect_eq(normalizeNewlines("", out[0..256]), "");
    });

    test("6. @cap/indoc — trimBlankLines & empty/whitespace-only boundaries", () => {
        let out: [128]u8 = undefined;
        expect_eq(trimBlankLines("\n  \n  hello\nworld\n\n  "), "  hello\nworld");
        expect_eq(dedent("", out[0..128]), "");
        expect_eq(minCommonIndent("   \n\t\n"), 0 as usize);
    });
}
