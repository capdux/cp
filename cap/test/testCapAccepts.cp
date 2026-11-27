// cap/test/testCapAccepts.cp
// Comprehensive unit tests for @cap/accepts

import { test, expect_eq } from "cap:test";
import {
    parseQValue,
    negotiateEncoding,
    negotiateLanguage,
    negotiateMediaType,
} from "../../packages/accepts/src/index.cp";

fn main() {
    test("1. @cap/accepts — parseQValue boundaries & invalid rejection", () => {
        expect_eq(parseQValue("1"), 1000 as i32);
        expect_eq(parseQValue("1.0"), 1000 as i32);
        expect_eq(parseQValue("1.000"), 1000 as i32);
        expect_eq(parseQValue("0.85"), 850 as i32);
        expect_eq(parseQValue("0.9"), 900 as i32);
        expect_eq(parseQValue("0.005"), 5 as i32);
        expect_eq(parseQValue("0"), 0 as i32);
        expect_eq(parseQValue("0.000"), 0 as i32);

        expect_eq(parseQValue(""), -1 as i32);
        expect_eq(parseQValue("1.1"), -1 as i32);
        expect_eq(parseQValue("1.001"), -1 as i32);
        expect_eq(parseQValue("-0.5"), -1 as i32);
        expect_eq(parseQValue("0.1234"), -1 as i32);
        expect_eq(parseQValue("abc"), -1 as i32);
    });

    test("2. @cap/accepts — Chrome Accept-Encoding negotiation", () => {
        const hdr = "gzip;q=0.8, deflate;q=0.5, br;q=1.0, zstd;q=0.9";
        const avail: [3][]const u8 = ["deflate", "gzip", "br"];
        expect_eq(negotiateEncoding(hdr, avail[0..3]), 2 as i64);

        const avail2: [2][]const u8 = ["deflate", "zstd"];
        expect_eq(negotiateEncoding(hdr, avail2[0..2]), 1 as i64);
    });

    test("3. @cap/accepts — Explicit q=0 encoding rejection overrides wildcard *", () => {
        const hdr = "gzip;q=0, *;q=0.5";
        const onlyGzip: [1][]const u8 = ["gzip"];
        expect_eq(negotiateEncoding(hdr, onlyGzip[0..1]), -1 as i64);

        const both: [2][]const u8 = ["gzip", "br"];
        expect_eq(negotiateEncoding(hdr, both[0..2]), 1 as i64);
    });

    test("4. @cap/accepts — Accept-Language exact & BCP 47 prefix fallback", () => {
        const hdr = "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7";
        const avail1: [2][]const u8 = ["en", "zh-CN"];
        expect_eq(negotiateLanguage(hdr, avail1[0..2]), 1 as i64);

        // Prefix fallback: header has "zh-CN;q=1.0", candidate is "zh"
        const avail2: [2][]const u8 = ["fr", "zh"];
        expect_eq(negotiateLanguage("zh-CN, fr;q=0.5", avail2[0..2]), 1 as i64);

        // Exact match beats prefix match at equal q
        const avail3: [2][]const u8 = ["zh", "zh-CN"];
        expect_eq(negotiateLanguage("zh-CN", avail3[0..2]), 1 as i64);
    });

    test("5. @cap/accepts — Accept media type specificity (type/sub > type/* > */*) & q weights", () => {
        const hdr = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
        const avail1: [2][]const u8 = ["application/json", "text/html"];
        expect_eq(negotiateMediaType(hdr, avail1[0..2]), 1 as i64);

        const avail2: [2][]const u8 = ["application/json", "application/xml"];
        expect_eq(negotiateMediaType(hdr, avail2[0..2]), 1 as i64);

        // Specificity tie-break at equal q=1.0: text/plain > text/* > */*
        const avail3: [2][]const u8 = ["image/png", "text/plain"];
        expect_eq(negotiateMediaType("*/*, text/*", avail3[0..2]), 1 as i64);
    });

    test("6. @cap/accepts — Empty header selects first candidate & no-match returns -1", () => {
        const avail: [2][]const u8 = ["application/json", "text/plain"];
        expect_eq(negotiateMediaType("", avail[0..2]), 0 as i64);
        expect_eq(negotiateMediaType("   ", avail[0..2]), 0 as i64);
        expect_eq(negotiateMediaType("image/webp", avail[0..2]), -1 as i64);

        let emptyAvail: [0][]const u8 = undefined;
        expect_eq(negotiateEncoding("gzip", emptyAvail[0..0]), -1 as i64);
    });
}
