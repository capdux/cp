// cap/test/testCapQsParse.cp
// Comprehensive unit tests for @cap/qs-parse (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    QS_ARRAY_INDICES,
    QS_ARRAY_BRACKETS,
    QS_ARRAY_REPEAT,
    QS_ARRAY_COMMA,
    QsOptions,
    QsPair,
    parseKeyPathWithDots,
    parseQuery,
    stringifyPairs,
    stringifyArray,
    decodeQueryValue,
} from "../../packages/@cap/qs-parse/src/index.cp";

fn main() {
    test("1. @cap/qs-parse — parseQuery with leading '?', custom delimiter ';', and bare flags", () => {
        let pairs: [8]QsPair = undefined;
        const n1 = parseQuery("?foo=bar&baz=42&flag", QsOptions.defaultOpts(), pairs[0..8]);
        expect_eq(n1, 3 as usize);
        expect_eq(pairs[0].key, "foo");
        expect_eq(pairs[0].value, "bar");
        expect_eq(pairs[1].key, "baz");
        expect_eq(pairs[1].value, "42");
        expect_eq(pairs[2].key, "flag");
        expect_eq(pairs[2].value, "");

        const semiOpts = QsOptions {
            delimiter: 0x3B as u8, // ';'
            allow_dots: false,
            max_depth: 5,
            array_format: QS_ARRAY_INDICES,
            add_query_prefix: false,
        };
        const n2 = parseQuery("a=1;b=2;;c=3;", semiOpts, pairs[0..8]);
        expect_eq(n2, 3 as usize);
        expect_eq(pairs[0].key, "a");
        expect_eq(pairs[1].key, "b");
        expect_eq(pairs[2].key, "c");
        expect_eq(pairs[2].value, "3");
    });

    test("2. @cap/qs-parse — allow_dots key path parsing and max_depth abuse guard", () => {
        const p1 = parseKeyPathWithDots("user.profile[name]", true);
        expect_eq(p1.depth, 3 as usize);
        expect_eq(p1.segments[0], "user");
        expect_eq(p1.segments[1], "profile");
        expect_eq(p1.segments[2], "name");

        // With allow_dots=false, dots stay in single segment
        const p2 = parseKeyPathWithDots("a.b.c", false);
        expect_eq(p2.depth, 1 as usize);
        expect_eq(p2.segments[0], "a.b.c");

        // max_depth=2 allows "a.b" (depth 2) and "x[y]" (depth 2), rejects "a.b.c" (depth 3) & "k[1][2]" (depth 3)
        let pairs: [8]QsPair = undefined;
        const dotOpts = QsOptions {
            delimiter: 0x26 as u8,
            allow_dots: true,
            max_depth: 2,
            array_format: QS_ARRAY_INDICES,
            add_query_prefix: false,
        };
        const n = parseQuery("a.b=1&a.b.c=drop&x[y]=2&k[1][2]=drop2", dotOpts, pairs[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(pairs[0].key, "a.b");
        expect_eq(pairs[0].value, "1");
        expect_eq(pairs[1].key, "x[y]");
        expect_eq(pairs[1].value, "2");
    });

    test("3. @cap/qs-parse — stringifyPairs with add_query_prefix, custom delimiter & percent-encoding", () => {
        const pairs: [3]QsPair = [
            QsPair { key: "user[name]", value: "hello world" },
            QsPair { key: "lang", value: "CP/2026" },
            QsPair { key: "q", value: "a&b=c" },
        ];
        let out: [256]u8 = undefined;

        const opts1 = QsOptions {
            delimiter: 0x26 as u8,
            allow_dots: false,
            max_depth: 5,
            array_format: QS_ARRAY_INDICES,
            add_query_prefix: true,
        };
        const n1 = stringifyPairs(pairs[0..3], opts1, out[0..256]);
        expect_eq(out[0..n1], "?user[name]=hello%20world&lang=CP%2F2026&q=a%26b%3Dc");

        const opts2 = QsOptions {
            delimiter: 0x3B as u8, // ';'
            allow_dots: false,
            max_depth: 5,
            array_format: QS_ARRAY_INDICES,
            add_query_prefix: false,
        };
        const n2 = stringifyPairs(pairs[0..2], opts2, out[0..256]);
        expect_eq(out[0..n2], "user[name]=hello%20world;lang=CP%2F2026");
    });

    test("4. @cap/qs-parse — stringifyArray all 4 qs array formats (indices, brackets, repeat, comma)", () => {
        const vals: [3]string = ["x", "y z", "a/b"];
        let out: [256]u8 = undefined;

        const n0 = stringifyArray("a", vals[0..3], QS_ARRAY_INDICES, out[0..256]);
        expect_eq(out[0..n0], "a[0]=x&a[1]=y%20z&a[2]=a%2Fb");

        const n1 = stringifyArray("a", vals[0..3], QS_ARRAY_BRACKETS, out[0..256]);
        expect_eq(out[0..n1], "a[]=x&a[]=y%20z&a[]=a%2Fb");

        const n2 = stringifyArray("a", vals[0..3], QS_ARRAY_REPEAT, out[0..256]);
        expect_eq(out[0..n2], "a=x&a=y%20z&a=a%2Fb");

        const n3 = stringifyArray("a", vals[0..3], QS_ARRAY_COMMA, out[0..256]);
        expect_eq(out[0..n3], "a=x,y%20z,a%2Fb");
    });

    test("5. @cap/qs-parse — decodeQueryValue delegates to @cap/percent-encoding (+ and %XX)", () => {
        let out: [128]u8 = undefined;

        const n1 = decodeQueryValue("hello+world%21%20%E4%B8%AD%E6%96%87", out[0..128]);
        expect_eq(out[0..n1], "hello world! 中文");

        const n2 = decodeQueryValue("a%2Fb%3Dc%26d", out[0..128]);
        expect_eq(out[0..n2], "a/b=c&d");
    });

    test("6. @cap/qs-parse — empty inputs & small output buffer bounds safety", () => {
        let pairs: [2]QsPair = undefined;
        expect_eq(parseQuery("", QsOptions.defaultOpts(), pairs[0..2]), 0 as usize);
        expect_eq(parseQuery("?", QsOptions.defaultOpts(), pairs[0..2]), 0 as usize);

        // Truncates cleanly when out_pairs capacity is reached
        const n = parseQuery("a=1&b=2&c=3&d=4", QsOptions.defaultOpts(), pairs[0..2]);
        expect_eq(n, 2 as usize);
        expect_eq(pairs[0].key, "a");
        expect_eq(pairs[1].key, "b");

        // Tight output buffer never overflows
        const one_val: [2]string = ["alpha", "beta"];
        let tiny: [8]u8 = undefined;
        expect_eq(stringifyArray("k", one_val[0..0], QS_ARRAY_INDICES, tiny[0..8]), 0 as usize);

        const w = stringifyArray("items", one_val[0..2], QS_ARRAY_BRACKETS, tiny[0..8]);
        expect_eq(w <= 8, true);
    });
}
