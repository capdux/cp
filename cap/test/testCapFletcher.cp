import { test, expect_eq } from "cap:test";
import {
    Fletcher16, Fletcher32, Fletcher64,
    fletcher16, fletcher16Bytes, calcFletcher16,
    fletcher32, fletcher32Bytes, calcFletcher32,
    fletcher64, fletcher64Bytes, calcFletcher64
} from "../../packages/fletcher/src/index.cp";

fn main() {
    test("fletcher16 standard Wikipedia vectors", () => {
        expect_eq(fletcher16("abcde"), 0xC8F0 as u16);
        expect_eq(fletcher16("abcdef"), 0x2057 as u16);
        expect_eq(fletcher16("abcdefgh"), 0x0627 as u16);
    });

    test("fletcher16 streaming matches one-shot", () => {
        let h = Fletcher16.new();
        h.update("abc");
        h.update("def");
        h.update("gh");
        expect_eq(h.digest(), fletcher16("abcdefgh"));
    });

    test("fletcher16Bytes matches fletcher16", () => {
        let raw: [5]u8 = [97, 98, 99, 100, 101];
        expect_eq(fletcher16Bytes(raw[0..5]), 0xC8F0 as u16);
    });

    test("fletcher32 standard Wikipedia vectors", () => {
        expect_eq(fletcher32("abcde"), 0xF04FC729 as u32);
        expect_eq(fletcher32("abcdef"), 0x56502D2A as u32);
        expect_eq(fletcher32("abcdefgh"), 0xEBE19591 as u32);
    });

    test("fletcher32Bytes & order sensitivity", () => {
        let a: [4]u8 = [1, 2, 3, 4];
        let b: [4]u8 = [3, 4, 1, 2];
        expect_eq(fletcher32Bytes(a[0..4]) != fletcher32Bytes(b[0..4]), true);
    });

    test("fletcher64 deterministic & empty input", () => {
        expect_eq(fletcher64(""), 0 as u64);
        let h1 = fletcher64("abcdefgh");
        let h2 = fletcher64("abcdefgh");
        let h3 = fletcher64("hgfedcba");
        expect_eq(h1, h2);
        expect_eq(h1 != h3, true);
    });

    test("Fletcher16 init/value/reset/withInitialValues & calcFletcher16", () => {
        let h = Fletcher16.init();
        h.update("abcde");
        expect_eq(h.value(), 0xC8F0 as u16);
        h.reset();
        expect_eq(h.value(), 0 as u16);

        const raw: [5]u8 = [97, 98, 99, 100, 101];
        expect_eq(calcFletcher16(raw[0..5]), 0xC8F0 as u16);

        let h2 = Fletcher16.withInitialValues(1, 2);
        expect_eq(h2.value(), 0x0201 as u16);
    });

    test("Fletcher32 streaming across odd chunk boundaries & calcFletcher32", () => {
        let h = Fletcher32.init();
        h.update("a");
        h.update("bc");
        h.update("de");
        expect_eq(h.value(), fletcher32("abcde"));
        h.reset();
        h.update("abc");
        h.update("defgh");
        expect_eq(h.digest(), fletcher32("abcdefgh"));

        const words: [2]u16 = [0x6261, 0x6463]; // "abcd" little-endian
        expect_eq(calcFletcher32(words[0..2]), fletcher32("abcd"));
    });

    test("Fletcher64 streaming across unaligned chunks, fletcher64Bytes & calcFletcher64", () => {
        let h = Fletcher64.init();
        h.update("a");
        h.update("bc");
        h.update("def");
        h.update("gh");
        expect_eq(h.value(), fletcher64("abcdefgh"));

        const raw: [8]u8 = [97, 98, 99, 100, 101, 102, 103, 104];
        expect_eq(fletcher64Bytes(raw[0..8]), fletcher64("abcdefgh"));

        const words: [2]u32 = [0x64636261, 0x68676665]; // "abcdefgh" little-endian
        expect_eq(calcFletcher64(words[0..2]), fletcher64("abcdefgh"));
    });
}
