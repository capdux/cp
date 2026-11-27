// cap/test/testCapJcs.cp
// Comprehensive unit tests for @cap/jcs (RFC 8785)

import { test, expect_eq } from "cap:test";
import {
    canonicalizeJson,
    areJsonEquivalent,
} from "../../packages/fast-json-stable-stringify/src/index.cp";

fn main() {
    test("1. @cap/jcs — deterministic object key sorting & whitespace stripping", () => {
        let buf: [512]u8 = undefined;
        const res = canonicalizeJson("  { \"z\" : 1 , \"a\" : 2 , \"m\" : 3 }  ", buf[0..512]);
        expect_eq(res.ok, true);
        expect_eq(res.output, "{\"a\":2,\"m\":3,\"z\":1}");
        expect_eq(res.len, res.output.len);
    });

    test("2. @cap/jcs — deeply nested objects & arrays preserving array order", () => {
        let buf: [1024]u8 = undefined;
        const raw = "{\n  \"items\": [ { \"b\": 2, \"a\": 1 }, { \"d\": true, \"c\": null } ],\n  \"meta\": { \"version\": 1, \"env\": \"prod\", \"flags\": [3, 1, 2] }\n}";
        const res = canonicalizeJson(raw, buf[0..1024]);
        expect_eq(res.ok, true);
        expect_eq(
            res.output,
            "{\"items\":[{\"a\":1,\"b\":2},{\"c\":null,\"d\":true}],\"meta\":{\"env\":\"prod\",\"flags\":[3,1,2],\"version\":1}}",
        );
    });

    test("3. @cap/jcs — string escapes, \\uXXXX normalization & UTF-16 key ordering", () => {
        let buf: [512]u8 = undefined;
        // "\u0061" decodes to "a", which sorts before "b"; "\/" normalizes to "/"
        const raw = "{\"b\":\"line1\\nline2\\t\\\"q\\\"\\\\/\",\"\\u0061\":\"\\u0041\\/\"}";
        const res = canonicalizeJson(raw, buf[0..512]);
        expect_eq(res.ok, true);
        expect_eq(res.output, "{\"a\":\"A/\",\"b\":\"line1\\nline2\\t\\\"q\\\"\\\\/\"}");
    });

    test("4. @cap/jcs — numeric normalization (-0 -> 0, trailing decimal zeros)", () => {
        let buf: [512]u8 = undefined;
        const raw = "{\"neg_zero\": -0, \"neg_zero_frac\": -0.00, \"whole\": 42.0, \"frac\": 3.1400, \"int\": -7}";
        const res = canonicalizeJson(raw, buf[0..512]);
        expect_eq(res.ok, true);
        expect_eq(res.output, "{\"frac\":3.14,\"int\":-7,\"neg_zero\":0,\"neg_zero_frac\":0,\"whole\":42}");
    });

    test("5. @cap/jcs — areJsonEquivalent semantic comparison", () => {
        expect_eq(
            areJsonEquivalent(
                "{ \"user\": { \"id\": 1, \"roles\": [\"admin\", \"dev\"] }, \"ok\": true }",
                "{\"ok\":true,\"user\":{\"roles\":[\"admin\",\"dev\"],\"id\":1.0}}",
            ),
            true,
        );
        // Array order matters!
        expect_eq(areJsonEquivalent("[1, 2]", "[2, 1]"), false);
        // Different values
        expect_eq(areJsonEquivalent("{\"a\":1}", "{\"a\":2}"), false);
        // Invalid JSON -> false
        expect_eq(areJsonEquivalent("{bad}", "{bad}"), false);
    });

    test("6. @cap/jcs — strict rejection of invalid JSON & trailing garbage", () => {
        let buf: [256]u8 = undefined;
        expect_eq(canonicalizeJson("", buf[0..256]).ok, false);
        expect_eq(canonicalizeJson("{a:1}", buf[0..256]).ok, false);
        expect_eq(canonicalizeJson("{\"a\":1,}", buf[0..256]).ok, false);
        expect_eq(canonicalizeJson("[1, 2,]", buf[0..256]).ok, false);
        expect_eq(canonicalizeJson("{\"a\": 01}", buf[0..256]).ok, false);
        expect_eq(canonicalizeJson("{\"a\": 1} extra", buf[0..256]).ok, false);
        expect_eq(canonicalizeJson("\"unterminated", buf[0..256]).ok, false);
    });
}
