import { test, expect_eq } from "cap:test";
import {
    JsTokenKind,
    JsToken,
    nextJsToken,
    tokenizeJs,
    countJsTokens,
} from "../../packages/@cap/js-tokens/src/index.cp";

fn main() {
    test("1. @cap/js-tokens — tokenizes const arrow declaration with operators", () => {
        const src = "const add = (a, b) => a + b;";
        let toks: [32]JsToken = undefined;
        let n = tokenizeJs(src, toks[0..32]);
        expect_eq(n, 21 as usize);
        expect_eq(toks[0].kind == .IdentifierName, true);
        expect_eq(toks[0].text(src), "const");
        expect_eq(toks[13].kind == .Punctuator, true);
        expect_eq(toks[13].text(src), "=>");
    });

    test("2. @cap/js-tokens — recognizes hex, binary, float, scientific, and BigInt numbers", () => {
        const src = "0xFF 0b1010 3.14e-2 42n";
        let toks: [16]JsToken = undefined;
        let n = tokenizeJs(src, toks[0..16]);
        expect_eq(n, 7 as usize);
        expect_eq(toks[0].kind == .NumericLiteral, true);
        expect_eq(toks[0].text(src), "0xFF");
        expect_eq(toks[2].text(src), "0b1010");
        expect_eq(toks[4].text(src), "3.14e-2");
        expect_eq(toks[6].text(src), "42n");
    });

    test("3. @cap/js-tokens — handles escaped strings and unclosed string detection", () => {
        const src = "\"he\\\"llo\" 'unclosed";
        let toks: [8]JsToken = undefined;
        let n = tokenizeJs(src, toks[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(toks[0].kind == .StringLiteral, true);
        expect_eq(toks[0].closed, true);
        expect_eq(toks[2].kind == .StringLiteral, true);
        expect_eq(toks[2].closed, false);
    });

    test("4. @cap/js-tokens — scans single-line and multi-line comments", () => {
        const src = "// hi\n/* block */";
        let toks: [8]JsToken = undefined;
        let n = tokenizeJs(src, toks[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(toks[0].kind == .SingleLineComment, true);
        expect_eq(toks[1].kind == .LineTerminatorSequence, true);
        expect_eq(toks[2].kind == .MultiLineComment, true);
        expect_eq(toks[2].closed, true);
    });

    test("5. @cap/js-tokens — scans template literals and template heads", () => {
        const src = "`plain` `hi ${x}`";
        let toks: [8]JsToken = undefined;
        let n = tokenizeJs(src, toks[0..8]);
        expect_eq(toks[0].kind == .NoSubstitutionTemplate, true);
        expect_eq(toks[0].text(src), "`plain`");
        expect_eq(toks[2].kind == .TemplateHead, true);
        expect_eq(toks[2].text(src), "`hi ${");
    });

    test("6. @cap/js-tokens — longest-match punctuators and countJsTokens", () => {
        const src = "a >>>= b ??= c?.d ...e";
        expect_eq(countJsTokens(src), 14 as usize);
        let toks: [16]JsToken = undefined;
        * = tokenizeJs(src, toks[0..16]);
        expect_eq(toks[2].text(src), ">>>=");
        expect_eq(toks[6].text(src), "??=");
        expect_eq(toks[9].text(src), "?.");
        expect_eq(toks[12].text(src), "...");
    });
}
