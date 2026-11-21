// Source: cp/cap/test/test_ron_production_deep.cp (Production deep tests for cap:ron)

import { test, expect_eq } from "cap:test";
import {
    RonTokenKind,
    RonToken,
    RonIter,
    validateRon,
    extractRonField,
} from "cap:ron";

fn main() {
    test("1. RonIter scans identifiers, bools, numbers, strings, raw strings and char literals", () => {
        const src =
            "Config(\n" ++
            "  enabled: true,\n" ++
            "  retries: 1_000,\n" ++
            "  mask: 0xff_0a,\n" ++
            "  ratio: -3.5e+2,\n" ++
            "  sep: '\\n',\n" ++
            "  raw: r#\"hello \"world\"\"#,\n" ++
            ")";

        let it = RonIter.init(src);
        const t0 = it.next().?;
        expect_eq(t0.kind, RonTokenKind.ident);
        expect_eq(t0.lexeme, "Config");

        expect_eq(it.next().?.kind, RonTokenKind.lparen);
        expect_eq(it.next().?.lexeme, "enabled");
        expect_eq(it.next().?.kind, RonTokenKind.colon);
        const t_bool = it.next().?;
        expect_eq(t_bool.kind, RonTokenKind.bool_lit);
        expect_eq(t_bool.lexeme, "true");
        expect_eq(it.next().?.kind, RonTokenKind.comma);

        // retries: 1_000,
        * = it.next();
        * = it.next();
        const t_num1 = it.next().?;
        expect_eq(t_num1.kind, RonTokenKind.number);
        expect_eq(t_num1.lexeme, "1_000");
        * = it.next();

        // mask: 0xff_0a,
        * = it.next();
        * = it.next();
        const t_hex = it.next().?;
        expect_eq(t_hex.kind, RonTokenKind.number);
        expect_eq(t_hex.lexeme, "0xff_0a");
        * = it.next();

        // ratio: -3.5e+2,
        * = it.next();
        * = it.next();
        const t_flt = it.next().?;
        expect_eq(t_flt.kind, RonTokenKind.number);
        expect_eq(t_flt.lexeme, "-3.5e+2");
        * = it.next();

        // sep: '\n',
        * = it.next();
        * = it.next();
        const t_ch = it.next().?;
        expect_eq(t_ch.kind, RonTokenKind.char_lit);
        expect_eq(t_ch.lexeme, "'\\n'");
        * = it.next();

        // raw: r#"hello "world""#,
        * = it.next();
        * = it.next();
        const t_raw = it.next().?;
        expect_eq(t_raw.kind, RonTokenKind.string);
        expect_eq(t_raw.lexeme, "r#\"hello \"world\"\"#");
    });

    test("2. RonIter skips line comments and nested block comments", () => {
        const src =
            "// top line comment\n" ++
            "/* outer /* nested */ still outer */\n" ++
            "(\n" ++
            "  a: /* inline */ 42,\n" ++
            ")";

        let it = RonIter.init(src);
        expect_eq(it.next().?.kind, RonTokenKind.lparen);
        expect_eq(it.next().?.lexeme, "a");
        expect_eq(it.next().?.kind, RonTokenKind.colon);
        expect_eq(it.next().?.lexeme, "42");
        expect_eq(it.next().?.kind, RonTokenKind.comma);
        expect_eq(it.next().?.kind, RonTokenKind.rparen);
        expect_eq(it.next() == null, true);
    });

    test("3. validateRon checks balanced brackets, comments/strings closure, and rejects malformed inputs", () => {
        expect_eq(validateRon("(a: [1, 2, { \"k\": (true, false) }])"), true);
        expect_eq(validateRon("Scene(/* nested /* ok */ */ items: [1, 2])"), true);

        // Empty or comment-only
        expect_eq(validateRon(""), false);
        expect_eq(validateRon("   // only comment"), false);

        // Mismatched or unclosed delimiters
        expect_eq(validateRon("(a: [1, 2)"), false);
        expect_eq(validateRon("([)]"), false);
        expect_eq(validateRon("(a: 1"), false);
        expect_eq(validateRon("a: 1)"), false);

        // Unclosed string / raw string / nested block comment
        expect_eq(validateRon("(a: \"unterminated)"), false);
        expect_eq(validateRon("(a: r#\"unterminated\")"), false);
        expect_eq(validateRon("/* outer /* inner */ (a: 1)"), false);
    });

    test("4. validateRon enforces 64-level maximum nesting depth protection", () => {
        // 64 nested parens -> valid
        let ok64: [129]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            ok64[i] = '(';
            ok64[128 - i] = ')';
            i += 1;
        }
        ok64[64] = '1';
        expect_eq(validateRon(&ok64), true);

        // 65 nested parens -> rejected
        let bad65: [131]u8 = undefined;
        let j: usize = 0;
        while (j < 65) {
            bad65[j] = '(';
            bad65[130 - j] = ')';
            j += 1;
        }
        bad65[65] = '1';
        expect_eq(validateRon(&bad65), false);
    });

    test("5. extractRonField extracts top-level values without being confused by nested structs or strings", () => {
        const cfg =
            "AppConfig(\n" ++
            "  note: \"port: 9999, host: fake\",\n" ++
            "  nested: (\n" ++
            "    port: 1111,\n" ++
            "    host: \"inner\",\n" ++
            "  ),\n" ++
            "  port: 8080,\n" ++
            "  window: (width: 1920, height: 1080),\n" ++
            "  tags: [\"alpha\", \"beta:gamma\"],\n" ++
            ")";

        expect_eq(extractRonField(cfg, "port").?, "8080");
        expect_eq(extractRonField(cfg, "note").?, "\"port: 9999, host: fake\"");
        expect_eq(extractRonField(cfg, "window").?, "(width: 1920, height: 1080)");
        expect_eq(extractRonField(cfg, "tags").?, "[\"alpha\", \"beta:gamma\"]");
        expect_eq(extractRonField(cfg, "host") == null, true);
        expect_eq(extractRonField(cfg, "missing") == null, true);
    });
}
