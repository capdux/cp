// cap/test/testCapPercentEncoding.cp
// Comprehensive unit tests for @cap/percent-encoding

import { test, expect_eq } from "cap:test";
import {
    AsciiSet,
    controlsSet,
    fragmentSet,
    querySet,
    pathSegmentSet,
    userinfoSet,
    nonAlphanumericSet,
    rfc3986UnreservedSet,
    percentEncode,
    percentDecode,
    percentDecodeStrict,
    formUrlEncode,
    formUrlDecode,
} from "../../packages/@cap/percent-encoding/src/index.cp";

fn main() {
    test("1. @cap/percent-encoding — WHATWG character set hierarchy differences", () => {
        let b1: [128]u8 = undefined;
        expect_eq(percentEncode("a b#c?d/e@f", fragmentSet(), b1[0..128]), "a%20b#c?d/e@f");

        let b2: [128]u8 = undefined;
        expect_eq(percentEncode("a b#c?d/e@f", querySet(), b2[0..128]), "a%20b%23c?d/e@f");

        let b3: [128]u8 = undefined;
        expect_eq(percentEncode("a b#c?d/e@f", pathSegmentSet(), b3[0..128]), "a%20b%23c%3Fd%2Fe@f");

        let b4: [128]u8 = undefined;
        expect_eq(percentEncode("a b#c?d/e@f", userinfoSet(), b4[0..128]), "a%20b%23c%3Fd%2Fe%40f");
    });

    test("2. @cap/percent-encoding — controlsSet, nonAlphanumericSet & rfc3986UnreservedSet", () => {
        const ctrl = controlsSet();
        expect_eq(ctrl.contains(0x00 as u8), true);
        expect_eq(ctrl.contains(0x1F as u8), true);
        expect_eq(ctrl.contains(0x7F as u8), true);
        expect_eq(ctrl.contains(' '), false);

        let b1: [128]u8 = undefined;
        expect_eq(percentEncode("A-b.c_d~e!", rfc3986UnreservedSet(), b1[0..128]), "A-b.c_d~e%21");

        let b2: [128]u8 = undefined;
        expect_eq(percentEncode("A-b.c_d~", nonAlphanumericSet(), b2[0..128]), "A%2Db%2Ec%5Fd%7E");
    });

    test("3. @cap/percent-encoding — UTF-8 Chinese round-trip encoding & decoding", () => {
        let encBuf: [128]u8 = undefined;
        const enc = percentEncode("你好 CP!", rfc3986UnreservedSet(), encBuf[0..128]);
        expect_eq(enc, "%E4%BD%A0%E5%A5%BD%20CP%21");

        let decBuf: [128]u8 = undefined;
        const decoded = percentDecodeStrict(enc, decBuf[0..128]);
        expect_eq(decoded ?? "", "你好 CP!");
    });

    test("4. @cap/percent-encoding — Lenient percentDecode vs strict percentDecodeStrict", () => {
        let out1: [128]u8 = undefined;
        expect_eq(percentDecode("100%25%ZZ%2", out1[0..128]), "100%%ZZ%2");

        let out2: [128]u8 = undefined;
        expect_eq(percentDecodeStrict("hello%20world%21", out2[0..128]) ?? "", "hello world!");

        let out3: [128]u8 = undefined;
        expect_eq(percentDecodeStrict("bad%ZZhex", out3[0..128]) == null, true);

        let out4: [128]u8 = undefined;
        expect_eq(percentDecodeStrict("truncated%A", out4[0..128]) == null, true);

        let out5: [128]u8 = undefined;
        expect_eq(percentDecodeStrict("trailing%", out5[0..128]) == null, true);
    });

    test("5. @cap/percent-encoding — formUrlEncode & formUrlDecode (+ <-> space)", () => {
        let eBuf: [128]u8 = undefined;
        const enc = formUrlEncode("a + b = c & 100%", eBuf[0..128]);
        expect_eq(enc, "a+%2B+b+%3D+c+%26+100%25");

        let dBuf: [128]u8 = undefined;
        expect_eq(formUrlDecode(enc, dBuf[0..128]), "a + b = c & 100%");
    });

    test("6. @cap/percent-encoding — Custom AsciiSet.add & AsciiSet.remove", () => {
        let custom = AsciiSet.empty();
        custom.add(':');
        custom.add('/');
        expect_eq(custom.contains(':'), true);
        expect_eq(custom.contains('/'), true);
        expect_eq(custom.contains('a'), false);
        expect_eq(custom.contains(0x80 as u8), true);

        custom.remove('/');
        expect_eq(custom.contains('/'), false);

        let out: [128]u8 = undefined;
        expect_eq(percentEncode("http://a:80/x", custom, out[0..128]), "http%3A//a%3A80/x");
    });
}
