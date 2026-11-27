// cap/test/testCapTerserMinify.cp
// Comprehensive unit tests for @cap/terser-minify (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    MinifyOptions,
    MinifyStats,
    minifyJs,
    minifyCss,
} from "../../packages/terser/src/index.cp";

fn main() {
    test("1. @cap/terser-minify — JS whitespace collapse while preserving keyword & identifier spacing", () => {
        const src = "function add( a , b ) {\n  const sum = a + b;\n  return sum;\n}";
        let out: [256]u8 = undefined;
        const st: MinifyStats = minifyJs(src, MinifyOptions.defaults(), out[0..256]);

        expect_eq(st.ok, true);
        expect_eq(st.comments_removed, 0 as usize);
        expect_eq(out[0..st.output_len], "function add(a,b){const sum=a+b;return sum;}");
        expect_eq(st.output_len < st.input_len, true);
    });

    test("2. @cap/terser-minify — strips // and /* */ comments while preserving /*! MIT */ legal comments", () => {
        const src = "/*! MIT License */\n// line comment\nlet x = /* inline */ 42;\n/* @license Apache-2.0 */\nreturn x;";
        let out: [256]u8 = undefined;
        const st = minifyJs(src, MinifyOptions.defaults(), out[0..256]);

        expect_eq(st.ok, true);
        expect_eq(st.comments_removed, 2 as usize);
        expect_eq(out[0..st.output_len], "/*! MIT License */let x=42;/* @license Apache-2.0 */return x;");

        // When preserve_legal_comments == false, legal comments are also stripped
        const stripAll = MinifyOptions {
            strip_comments: true,
            preserve_legal_comments: false,
            collapse_whitespace: true,
            keep_newlines: false,
        };
        const st2 = minifyJs(src, stripAll, out[0..256]);
        expect_eq(st2.ok, true);
        expect_eq(st2.comments_removed, 4 as usize);
        expect_eq(out[0..st2.output_len], "let x=42;return x;");
    });

    test("3. @cap/terser-minify — string literals and template strings preserved verbatim", () => {
        const src = "const u = \"http://a.b /* keep */ \\\"q\\\"\"; const t = `a  +  b // keep`;";
        let out: [256]u8 = undefined;
        const st = minifyJs(src, MinifyOptions.defaults(), out[0..256]);

        expect_eq(st.ok, true);
        expect_eq(st.comments_removed, 0 as usize);
        expect_eq(
            out[0..st.output_len],
            "const u=\"http://a.b /* keep */ \\\"q\\\"\";const t=`a  +  b // keep`;",
        );
    });

    test("4. @cap/terser-minify — prevents unsafe merging of `a + +b` and `a - -b`", () => {
        const src = "const r1 = a + +b; const r2 = a - -b;";
        let out: [256]u8 = undefined;
        const st = minifyJs(src, MinifyOptions.defaults(), out[0..256]);

        expect_eq(st.ok, true);
        expect_eq(out[0..st.output_len], "const r1=a+ +b;const r2=a- -b;");
    });

    test("5. @cap/terser-minify — minifyCss strips comments, collapses spaces & drops trailing semicolons", () => {
        const css = "/* header */ body , .card > p {\n  margin: 0px;\n  border: 1px solid red ;\n  content: \"a ; /* keep */\";\n}";
        let out: [256]u8 = undefined;
        const st = minifyCss(css, out[0..256]);

        expect_eq(st.ok, true);
        expect_eq(st.comments_removed, 1 as usize);
        expect_eq(
            out[0..st.output_len],
            "body,.card>p{margin:0px;border:1px solid red;content:\"a ; /* keep */\"}",
        );
    });

    test("6. @cap/terser-minify — empty input and small output buffer safety", () => {
        let out: [32]u8 = undefined;
        const emptyJs = minifyJs("", MinifyOptions.defaults(), out[0..32]);
        expect_eq(emptyJs.ok, true);
        expect_eq(emptyJs.output_len, 0 as usize);

        const emptyCss = minifyCss("", out[0..32]);
        expect_eq(emptyCss.ok, true);
        expect_eq(emptyCss.output_len, 0 as usize);

        // Buffer too small returns ok == false safely without overflow
        let tiny: [4]u8 = undefined;
        const smallJs = minifyJs("const alpha = 123;", MinifyOptions.defaults(), tiny[0..4]);
        expect_eq(smallJs.ok, false);
        expect_eq(smallJs.output_len <= 4, true);

        const smallCss = minifyCss("body { color: red; }", tiny[0..4]);
        expect_eq(smallCss.ok, false);
        expect_eq(smallCss.output_len <= 4, true);
    });
}
