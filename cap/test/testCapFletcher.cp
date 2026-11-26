import { test, expect_eq } from "cap:test";
import {
    Fletcher16, fletcher16, fletcher16Bytes,
    fletcher32, fletcher32Bytes, fletcher64
} from "../../packages/@cap/fletcher/src/index.cp";

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
}
