import { test, expect_eq } from "cap:test";
import {
    RonTokenKind,
    RonLexer,
    ronGetField,
} from "../../packages/@cap/ron/src/index.cp";

fn main() {
    test("1. @cap/ron — lexes struct punctuation, identifiers, booleans, ints, floats, and strings", () => {
        let lex = RonLexer.new("(a: 42, b: -3.14, c: true, d: \"hi\")");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == RonTokenKind.LParen, true);
        const t1 = lex.nextToken();
        expect_eq(t1.kind == RonTokenKind.Ident, true);
        expect_eq(t1.text, "a");
        const t2 = lex.nextToken();
        expect_eq(t2.kind == RonTokenKind.Colon, true);
        const t3 = lex.nextToken();
        expect_eq(t3.kind == RonTokenKind.Int, true);
        expect_eq(t3.text, "42");
        const t4 = lex.nextToken();
        expect_eq(t4.kind == RonTokenKind.Comma, true);
        const t5 = lex.nextToken();
        expect_eq(t5.text, "b");
        const t6 = lex.nextToken();
        expect_eq(t6.kind == RonTokenKind.Colon, true);
        const t7 = lex.nextToken();
        expect_eq(t7.kind == RonTokenKind.Float, true);
        expect_eq(t7.text, "-3.14");
        const t8 = lex.nextToken();
        expect_eq(t8.kind == RonTokenKind.Comma, true);
        const t9 = lex.nextToken();
        expect_eq(t9.text, "c");
        const t10 = lex.nextToken();
        expect_eq(t10.kind == RonTokenKind.Colon, true);
        const t11 = lex.nextToken();
        expect_eq(t11.kind == RonTokenKind.Bool, true);
        expect_eq(t11.text, "true");
        const t12 = lex.nextToken();
        expect_eq(t12.kind == RonTokenKind.Comma, true);
        const t13 = lex.nextToken();
        expect_eq(t13.text, "d");
        const t14 = lex.nextToken();
        expect_eq(t14.kind == RonTokenKind.Colon, true);
        const t15 = lex.nextToken();
        expect_eq(t15.kind == RonTokenKind.String, true);
        expect_eq(t15.text, "\"hi\"");
        const t16 = lex.nextToken();
        expect_eq(t16.kind == RonTokenKind.RParen, true);
        const t17 = lex.nextToken();
        expect_eq(t17.kind == RonTokenKind.Eof, true);
    });

    test("2. @cap/ron — skips line comments and nested block comments", () => {
        let lex = RonLexer.new("// header\n/* outer /* inner */ still comment */ [1, false]");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == RonTokenKind.LBracket, true);
        const t1 = lex.nextToken();
        expect_eq(t1.kind == RonTokenKind.Int, true);
        expect_eq(t1.text, "1");
        const t2 = lex.nextToken();
        expect_eq(t2.kind == RonTokenKind.Comma, true);
        const t3 = lex.nextToken();
        expect_eq(t3.kind == RonTokenKind.Bool, true);
        expect_eq(t3.text, "false");
        const t4 = lex.nextToken();
        expect_eq(t4.kind == RonTokenKind.RBracket, true);
    });

    test("3. @cap/ron — handles escaped quotes inside strings and scientific floats", () => {
        let lex = RonLexer.new("{\"k\": \"say \\\"hi\\\"\", \"n\": 1.5e-2}");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == RonTokenKind.LBrace, true);
        const t1 = lex.nextToken();
        expect_eq(t1.kind == RonTokenKind.String, true);
        const t2 = lex.nextToken();
        expect_eq(t2.kind == RonTokenKind.Colon, true);
        const t3 = lex.nextToken();
        expect_eq(t3.kind == RonTokenKind.String, true);
        expect_eq(t3.text, "\"say \\\"hi\\\"\"");
        const t4 = lex.nextToken();
        expect_eq(t4.kind == RonTokenKind.Comma, true);
        const t5 = lex.nextToken();
        expect_eq(t5.kind == RonTokenKind.String, true);
        const t6 = lex.nextToken();
        expect_eq(t6.kind == RonTokenKind.Colon, true);
        const t7 = lex.nextToken();
        expect_eq(t7.kind == RonTokenKind.Float, true);
        expect_eq(t7.text, "1.5e-2");
        const t8 = lex.nextToken();
        expect_eq(t8.kind == RonTokenKind.RBrace, true);
    });

    test("4. @cap/ron — ronGetField extracts scalar fields from top-level struct", () => {
        const src: string = "(a: 1, b: \"hi\", c: true)";
        expect_eq(ronGetField(src, "a") ?? "", "1");
        expect_eq(ronGetField(src, "b") ?? "", "\"hi\"");
        expect_eq(ronGetField(src, "c") ?? "", "true");
        expect_eq(ronGetField(src, "missing") == null, true);
    });

    test("5. @cap/ron — ronGetField skips nested structs, arrays, and maps cleanly", () => {
        const src: string = "Config(\n  // nested block\n  nested: (a: 99, b: [1, 2, 3]),\n  tags: [\"alpha\", \"beta\"],\n  enabled: false,\n)";
        expect_eq(ronGetField(src, "nested") ?? "", "(a: 99, b: [1, 2, 3])");
        expect_eq(ronGetField(src, "tags") ?? "", "[\"alpha\", \"beta\"]");
        expect_eq(ronGetField(src, "enabled") ?? "", "false");
        expect_eq(ronGetField(src, "a") == null, true);
    });

    test("6. @cap/ron — handles invalid tokens and unclosed strings gracefully", () => {
        let lex = RonLexer.new("@ \"unterminated");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == RonTokenKind.Invalid, true);
        expect_eq(t0.text, "@");
        const t1 = lex.nextToken();
        expect_eq(t1.kind == RonTokenKind.Invalid, true);
        expect_eq(ronGetField("not a struct", "a") == null, true);
    });
}
