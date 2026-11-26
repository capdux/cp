// cap/test/testCapSyntaxHighlight.cp
// Comprehensive unit tests for @cap/syntax-highlight

import { test, expect_eq } from "cap:test";
import {
    TOK_PLAIN,
    TOK_KEYWORD,
    TOK_STRING,
    TOK_NUMBER,
    TOK_COMMENT,
    TOK_OPERATOR,
    TOK_TYPE,
    TOK_PUNCT,
    LANG_CODE,
    LANG_JSON,
    LANG_SQL,
    Token,
    tokenize,
    highlightAnsi,
    highlightHtml,
} from "../../packages/@cap/syntax-highlight/src/index.cp";

fn containsSlice(hay: []const u8, needle: string) -> bool {
    if (needle.len == (0 as usize)) { return true; }
    if (needle.len > hay.len) { return false; }
    let i: usize = 0;
    const limit = hay.len - needle.len;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return true; }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/syntax-highlight — LANG_CODE keywords, types, numbers, strings & comments", () => {
        const src = "const x: i32 = 0x2A; // hi\n\"ok\"";
        let toks: [32]Token = undefined;
        const n = tokenize(src, LANG_CODE, toks[0..32]);
        expect_eq(n > (8 as usize), true);

        // Verify contiguous non-overlapping coverage
        let covered: usize = 0;
        let hasKw = false;
        let hasType = false;
        let hasNum = false;
        let hasCmt = false;
        let hasStr = false;
        let i: usize = 0;
        while (i < n) {
            expect_eq(toks[i].start, covered);
            covered += toks[i].len;
            if (toks[i].kind == TOK_KEYWORD) { hasKw = true; }
            if (toks[i].kind == TOK_TYPE) { hasType = true; }
            if (toks[i].kind == TOK_NUMBER) { hasNum = true; }
            if (toks[i].kind == TOK_COMMENT) { hasCmt = true; }
            if (toks[i].kind == TOK_STRING) { hasStr = true; }
            i += 1;
        }
        expect_eq(covered, src.len);
        expect_eq(hasKw, true);
        expect_eq(hasType, true);
        expect_eq(hasNum, true);
        expect_eq(hasCmt, true);
        expect_eq(hasStr, true);
    });

    test("2. @cap/syntax-highlight — LANG_JSON strings, numbers, booleans & punctuation", () => {
        const src = "{\"ok\": true, \"n\": 3.14}";
        let toks: [32]Token = undefined;
        const n = tokenize(src, LANG_JSON, toks[0..32]);

        let strCount: usize = 0;
        let kwCount: usize = 0;
        let numCount: usize = 0;
        let puncCount: usize = 0;
        let i: usize = 0;
        while (i < n) {
            if (toks[i].kind == TOK_STRING) { strCount += 1; }
            if (toks[i].kind == TOK_KEYWORD) { kwCount += 1; }
            if (toks[i].kind == TOK_NUMBER) { numCount += 1; }
            if (toks[i].kind == TOK_PUNCT) { puncCount += 1; }
            i += 1;
        }
        expect_eq(strCount, 2 as usize);
        expect_eq(kwCount, 1 as usize);
        expect_eq(numCount, 1 as usize);
        expect_eq(puncCount, 5 as usize);
    });

    test("3. @cap/syntax-highlight — LANG_SQL case-insensitive keywords & -- comments", () => {
        const src = "select id from users where age >= 18 -- filter";
        let toks: [32]Token = undefined;
        const n = tokenize(src, LANG_SQL, toks[0..32]);

        let kwCount: usize = 0;
        let cmtCount: usize = 0;
        let opCount: usize = 0;
        let i: usize = 0;
        while (i < n) {
            if (toks[i].kind == TOK_KEYWORD) { kwCount += 1; }
            if (toks[i].kind == TOK_COMMENT) { cmtCount += 1; }
            if (toks[i].kind == TOK_OPERATOR) { opCount += 1; }
            i += 1;
        }
        // select, from, where => 3 keywords; >= => 1 op; -- filter => 1 comment
        expect_eq(kwCount, 3 as usize);
        expect_eq(opCount, 1 as usize);
        expect_eq(cmtCount, 1 as usize);
    });

    test("4. @cap/syntax-highlight — highlightAnsi emits ANSI color & reset sequences", () => {
        const src = "return 42;";
        let out: [256]u8 = undefined;
        const n = highlightAnsi(src, LANG_CODE, out[0..256]);
        expect_eq(n > (0 as i64), true);
        const slice = out[0..(n as usize)];
        expect_eq(containsSlice(slice, "[35mreturn"), true);
        expect_eq(containsSlice(slice, "[33m42"), true);
        expect_eq(containsSlice(slice, "[0m"), true);
    });

    test("5. @cap/syntax-highlight — highlightHtml escapes XSS chars (< > & \") and wraps spans", () => {
        const src = "if (a < b && s == \"<script>\")";
        let out: [512]u8 = undefined;
        const n = highlightHtml(src, LANG_CODE, out[0..512]);
        expect_eq(n > (0 as i64), true);
        const slice = out[0..(n as usize)];

        expect_eq(containsSlice(slice, "<span class=\"hl-kw\">if</span>"), true);
        expect_eq(containsSlice(slice, "&lt;script&gt;"), true);
        expect_eq(containsSlice(slice, "&amp;&amp;"), true);
        expect_eq(containsSlice(slice, "&quot;"), true);
        // Raw unescaped <script> must NOT exist
        expect_eq(containsSlice(slice, "<script>"), false);
    });

    test("6. @cap/syntax-highlight — Empty input & insufficient buffer returns -1", () => {
        let toks: [4]Token = undefined;
        expect_eq(tokenize("", LANG_CODE, toks[0..4]), 0 as usize);

        let out: [64]u8 = undefined;
        expect_eq(highlightAnsi("", LANG_CODE, out[0..64]), 0 as i64);
        expect_eq(highlightHtml("", LANG_CODE, out[0..64]), 0 as i64);

        let tiny: [5]u8 = undefined;
        expect_eq(highlightAnsi("const x = 1;", LANG_CODE, tiny[0..5]), -1 as i64);
        expect_eq(highlightHtml("const x = 1;", LANG_CODE, tiny[0..5]), -1 as i64);
    });
}
