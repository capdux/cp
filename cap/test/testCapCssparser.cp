import { test, expect_eq } from "cap:test";
import {
    CssTokKind,
    CssLexer,
} from "../../packages/cssparser/src/index.cp";

fn main() {
    test("1. @cap/cssparser — tokenizes @media rule and block delimiters", () => {
        let lex = CssLexer.new("@media screen { color: #fff; }");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == CssTokKind.AtKeyword, true);
        expect_eq(t0.text, "media");

        const t1 = lex.nextToken();
        expect_eq(t1.kind == CssTokKind.Ident, true);
        expect_eq(t1.text, "screen");

        expect_eq(lex.nextToken().kind == CssTokKind.LBrace, true);

        const t3 = lex.nextToken();
        expect_eq(t3.kind == CssTokKind.Ident, true);
        expect_eq(t3.text, "color");

        expect_eq(lex.nextToken().kind == CssTokKind.Colon, true);

        const t5 = lex.nextToken();
        expect_eq(t5.kind == CssTokKind.Hash, true);
        expect_eq(t5.text, "fff");

        expect_eq(lex.nextToken().kind == CssTokKind.Semicolon, true);
        expect_eq(lex.nextToken().kind == CssTokKind.RBrace, true);
        expect_eq(lex.nextToken().kind == CssTokKind.Eof, true);
    });

    test("2. @cap/cssparser — distinguishes Number, Dimension, and Percentage", () => {
        let lex = CssLexer.new("0 12px -3.5rem 50%");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == CssTokKind.Number, true);
        expect_eq(t0.text, "0");

        const t1 = lex.nextToken();
        expect_eq(t1.kind == CssTokKind.Dimension, true);
        expect_eq(t1.text, "12");
        expect_eq(t1.unit, "px");

        const t2 = lex.nextToken();
        expect_eq(t2.kind == CssTokKind.Dimension, true);
        expect_eq(t2.text, "-3.5");
        expect_eq(t2.unit, "rem");

        const t3 = lex.nextToken();
        expect_eq(t3.kind == CssTokKind.Percentage, true);
        expect_eq(t3.text, "50");
        expect_eq(t3.unit, "%");
    });

    test("3. @cap/cssparser — tokenizes Function calls and arguments", () => {
        let lex = CssLexer.new("rgb(255, 128, 0)");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == CssTokKind.Function, true);
        expect_eq(t0.text, "rgb");

        expect_eq(lex.nextToken().text, "255");
        expect_eq(lex.nextToken().kind == CssTokKind.Comma, true);
        expect_eq(lex.nextToken().text, "128");
        expect_eq(lex.nextToken().kind == CssTokKind.Comma, true);
        expect_eq(lex.nextToken().text, "0");
        expect_eq(lex.nextToken().kind == CssTokKind.RParen, true);
    });

    test("4. @cap/cssparser — tokenizes single and double quoted strings", () => {
        let lex = CssLexer.new("\"Open Sans\" 'hello world'");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == CssTokKind.String, true);
        expect_eq(t0.text, "Open Sans");

        const t1 = lex.nextToken();
        expect_eq(t1.kind == CssTokKind.String, true);
        expect_eq(t1.text, "hello world");
    });

    test("5. @cap/cssparser — skips whitespace and block comments", () => {
        let lex = CssLexer.new("  /* header comment */  --accent /* inline */ : #1a2b3c ");
        const t0 = lex.nextToken();
        expect_eq(t0.kind == CssTokKind.Ident, true);
        expect_eq(t0.text, "--accent");

        expect_eq(lex.nextToken().kind == CssTokKind.Colon, true);

        const t2 = lex.nextToken();
        expect_eq(t2.kind == CssTokKind.Hash, true);
        expect_eq(t2.text, "1a2b3c");
        expect_eq(lex.nextToken().kind == CssTokKind.Eof, true);
    });

    test("6. @cap/cssparser — brackets, parens, and unknown delimiters", () => {
        let lex = CssLexer.new("a[href] > (b)");
        expect_eq(lex.nextToken().text, "a");
        expect_eq(lex.nextToken().kind == CssTokKind.LBracket, true);
        expect_eq(lex.nextToken().text, "href");
        expect_eq(lex.nextToken().kind == CssTokKind.RBracket, true);
        const gt = lex.nextToken();
        expect_eq(gt.kind == CssTokKind.Delim, true);
        expect_eq(gt.text, ">");
        expect_eq(lex.nextToken().kind == CssTokKind.LParen, true);
        expect_eq(lex.nextToken().text, "b");
        expect_eq(lex.nextToken().kind == CssTokKind.RParen, true);
    });
}
