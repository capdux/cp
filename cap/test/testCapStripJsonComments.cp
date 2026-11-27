// cap/test/testCapStripJsonComments.cp
// Comprehensive unit tests for @cap/strip-json-comments

import { test, expect_eq } from "cap:test";
import {
    StripOptions,
    stripComments,
    stripJsonc,
    stripCompact,
} from "../../packages/strip-json-comments/src/index.cp";

fn main() {
    test("1. @cap/strip-json-comments — Single-line // & multi-line /* */ comments", () => {
        const src = "{\n  // header comment\n  \"a\": 1 /* inline */\n}";
        let out: [256]u8 = undefined;
        const n = stripCompact(src, out[0..256]);
        expect_eq(n > (0 as i64), true);
        expect_eq(out[0..(n as usize)], "{\n  \n  \"a\": 1 \n}");
    });

    test("2. @cap/strip-json-comments — preserve_whitespace keeps exact byte length & newlines", () => {
        const src = "{\n  /* line1\n     line2 */\n  \"k\": 2 // end\n}";
        let out: [256]u8 = undefined;
        const n = stripJsonc(src, out[0..256]);
        expect_eq(n, src.len as i64);
        // Every newline must remain at the exact same byte offset
        let i: usize = 0;
        while (i < src.len) {
            if (src[i] == '\n') {
                expect_eq(out[i], '\n');
            }
            i += 1;
        }
    });

    test("3. @cap/strip-json-comments — Preserves //, /* */ and \\\" inside quoted strings", () => {
        const src = "{\"url\":\"https://example.com/*not*/\",\"q\":\"a\\\"//b\"}";
        let out: [256]u8 = undefined;
        const n = stripCompact(src, out[0..256]);
        expect_eq(n, src.len as i64);
        expect_eq(out[0..(n as usize)], src);
    });

    test("4. @cap/strip-json-comments — Trailing comma removal across comments", () => {
        const src = "{\"a\":[1,2, /* end */ ],\"b\":true, // trailing\n}";
        let out: [256]u8 = undefined;
        const n = stripCompact(src, out[0..256]);
        expect_eq(n > (0 as i64), true);
        expect_eq(out[0..(n as usize)], "{\"a\":[1,2  ],\"b\":true \n}");

        // With strip_trailing_commas = false, comma stays intact
        const keepOpts = StripOptions{
            preserve_whitespace: false,
            strip_trailing_commas: false,
        };
        let out2: [256]u8 = undefined;
        const n2 = stripComments("{\"a\":1,}", out2[0..256], keepOpts);
        expect_eq(out2[0..(n2 as usize)], "{\"a\":1,}");
    });

    test("5. @cap/strip-json-comments — stripCompact tight output", () => {
        const src = "[/*c1*/1,/*c2*/2,/*c3*/]";
        let out: [128]u8 = undefined;
        const n = stripCompact(src, out[0..128]);
        expect_eq(out[0..(n as usize)], "[1,2]");
    });

    test("6. @cap/strip-json-comments — Empty input, unclosed block comment & small buffer -1", () => {
        let out: [64]u8 = undefined;
        expect_eq(stripJsonc("", out[0..64]), 0 as i64);

        const unclosed = "{\"a\":1} /* unclosed";
        const nUnclosed = stripCompact(unclosed, out[0..64]);
        expect_eq(out[0..(nUnclosed as usize)], "{\"a\":1} ");

        let tiny: [3]u8 = undefined;
        expect_eq(stripJsonc("{\"a\":1}", tiny[0..3]), -1 as i64);
        expect_eq(stripCompact("{\"a\":1}", tiny[0..3]), -1 as i64);
    });
}
