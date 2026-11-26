// cap/test/testCapAcornParse.cp
// Comprehensive unit tests for @cap/acorn-parse (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    TOK_EOF,
    TOK_IDENT,
    TOK_KEYWORD,
    TOK_NUMBER,
    TOK_STRING,
    TOK_TEMPLATE,
    TOK_PUNCT,
    TOK_COMMENT,
    JsLexer,
    analyzeJsModule,
    extractImportSpecifiers,
} from "../../packages/@cap/acorn-parse/src/index.cp";

fn main() {
    test("1. @cap/acorn-parse — JsLexer keywords, identifiers, strings, comments & line/col tracking", () => {
        const src = "// head\nconst msg = \"hi\\n\"; /* ok */";
        let lex = JsLexer.new(src);

        const t1 = lex.nextToken();
        expect_eq(t1.kind, TOK_COMMENT);
        expect_eq(t1.line, 1 as usize);
        expect_eq(t1.col, 1 as usize);
        expect_eq(src[t1.start..t1.end], "// head");

        const t2 = lex.nextToken();
        expect_eq(t2.kind, TOK_KEYWORD);
        expect_eq(t2.line, 2 as usize);
        expect_eq(t2.col, 1 as usize);
        expect_eq(src[t2.start..t2.end], "const");

        const t3 = lex.nextToken();
        expect_eq(t3.kind, TOK_IDENT);
        expect_eq(t3.line, 2 as usize);
        expect_eq(t3.col, 7 as usize);
        expect_eq(src[t3.start..t3.end], "msg");

        const t4 = lex.nextToken();
        expect_eq(t4.kind, TOK_PUNCT);
        expect_eq(src[t4.start..t4.end], "=");

        const t5 = lex.nextToken();
        expect_eq(t5.kind, TOK_STRING);
        expect_eq(src[t5.start..t5.end], "\"hi\\n\"");

        const t6 = lex.nextToken();
        expect_eq(t6.kind, TOK_PUNCT);
        expect_eq(src[t6.start..t6.end], ";");

        const t7 = lex.nextToken();
        expect_eq(t7.kind, TOK_COMMENT);
        expect_eq(src[t7.start..t7.end], "/* ok */");

        const t8 = lex.nextToken();
        expect_eq(t8.kind, TOK_EOF);
    });

    test("2. @cap/acorn-parse — numeric literals (hex, binary, float, scientific) & template literals", () => {
        const src = "0xFF 0b1010 3.14 1.5e-3 `hello ${a + 1}`";
        let lex = JsLexer.new(src);

        const t1 = lex.nextToken();
        expect_eq(t1.kind, TOK_NUMBER);
        expect_eq(src[t1.start..t1.end], "0xFF");

        const t2 = lex.nextToken();
        expect_eq(t2.kind, TOK_NUMBER);
        expect_eq(src[t2.start..t2.end], "0b1010");

        const t3 = lex.nextToken();
        expect_eq(t3.kind, TOK_NUMBER);
        expect_eq(src[t3.start..t3.end], "3.14");

        const t4 = lex.nextToken();
        expect_eq(t4.kind, TOK_NUMBER);
        expect_eq(src[t4.start..t4.end], "1.5e-3");

        const t5 = lex.nextToken();
        expect_eq(t5.kind, TOK_TEMPLATE);
        expect_eq(src[t5.start..t5.end], "`hello ${a + 1}`");
    });

    test("3. @cap/acorn-parse — multi-character ES2024 operators (===, !==, =>, ??, ?., ..., &&, ||)", () => {
        const src = "=== !== => ?? ?. ... && ||";
        let lex = JsLexer.new(src);

        const t1 = lex.nextToken();
        expect_eq(t1.kind, TOK_PUNCT);
        expect_eq(src[t1.start..t1.end], "===");

        const t2 = lex.nextToken();
        expect_eq(src[t2.start..t2.end], "!==");

        const t3 = lex.nextToken();
        expect_eq(src[t3.start..t3.end], "=>");

        const t4 = lex.nextToken();
        expect_eq(src[t4.start..t4.end], "??");

        const t5 = lex.nextToken();
        expect_eq(src[t5.start..t5.end], "?.");

        const t6 = lex.nextToken();
        expect_eq(src[t6.start..t6.end], "...");

        const t7 = lex.nextToken();
        expect_eq(src[t7.start..t7.end], "&&");

        const t8 = lex.nextToken();
        expect_eq(src[t8.start..t8.end], "||");
    });

    test("4. @cap/acorn-parse — analyzeJsModule valid module summary counts", () => {
        const src =
            "import fs from 'node:fs';\n" +
            "import { join } from 'node:path';\n" +
            "export class Engine {\n" +
            "  async run(args) { return [1, 2]; }\n" +
            "}\n" +
            "export function boot() { try { return new Engine(); } catch (e) { throw e; } }\n";

        const s = analyzeJsModule(src);
        expect_eq(s.ok, true);
        expect_eq(s.import_count, 2 as usize);
        expect_eq(s.export_count, 2 as usize);
        expect_eq(s.function_count, 1 as usize);
        expect_eq(s.class_count, 1 as usize);
        expect_eq(s.token_count > 30, true);
    });

    test("5. @cap/acorn-parse — analyzeJsModule detects mismatched and unclosed brackets", () => {
        const bad1 = "function f(a) { return [1, 2); }";
        const s1 = analyzeJsModule(bad1);
        expect_eq(s1.ok, false);
        expect_eq(bad1[s1.unmatched_bracket_pos], 41 as u8); // ')' mismatched with '['

        const bad2 = "const arr = [1, 2, { a: 3 };";
        const s2 = analyzeJsModule(bad2);
        expect_eq(s2.ok, false);
        expect_eq(bad2[s2.unmatched_bracket_pos], 91 as u8); // unclosed '['
    });

    test("6. @cap/acorn-parse — extractImportSpecifiers for static ESM imports (skipping dynamic import)", () => {
        const src =
            "import 'polyfill';\n" +
            "import React, { useState } from \"react\";\n" +
            "import * as path from 'node:path';\n" +
            "// import fake from 'ignored-comment';\n" +
            "const lazy = import('./dynamic.js');\n" +
            "import { helper } from './utils/helper.mjs';\n";

        let specs: [8]string = undefined;
        const n = extractImportSpecifiers(src, specs[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(specs[0], "polyfill");
        expect_eq(specs[1], "react");
        expect_eq(specs[2], "node:path");
        expect_eq(specs[3], "./utils/helper.mjs");
    });
}
