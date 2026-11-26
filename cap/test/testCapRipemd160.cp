import { test, expect_eq } from "cap:test";
import {
    RIPEMD160_DIGEST_LEN, RIPEMD160_BLOCK_LEN,
    Ripemd160, ripemd160Bytes, ripemd160Hex
} from "../../packages/@cap/ripemd160/src/index.cp";

fn main() {
    test("1. @cap/ripemd160 — official vector: empty string", () => {
        let hex: [40]u8 = [0; 40];
        let n = ripemd160Hex("", hex[0..40]);
        expect_eq(n, 40);
        let s: string = hex[0..40];
        expect_eq(s, "9c1185a5c5e9fc54612808977ee8f548b2258d31");
        expect_eq(RIPEMD160_DIGEST_LEN, 20);
        expect_eq(RIPEMD160_BLOCK_LEN, 64);
    });

    test("2. @cap/ripemd160 — official vectors: 'a' and 'abc'", () => {
        let hex: [40]u8 = [0; 40];
        * = ripemd160Hex("a", hex[0..40]);
        let s1: string = hex[0..40];
        expect_eq(s1, "0bdc9d2d256b3ee9daae347be6f4dc835a467ffe");

        * = ripemd160Hex("abc", hex[0..40]);
        let s2: string = hex[0..40];
        expect_eq(s2, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
    });

    test("3. @cap/ripemd160 — official vectors: 'message digest' and alphabet", () => {
        let hex: [40]u8 = [0; 40];
        * = ripemd160Hex("message digest", hex[0..40]);
        let s1: string = hex[0..40];
        expect_eq(s1, "5d0689ef49d2fae572b881b123a85ffa21595f36");

        * = ripemd160Hex("abcdefghijklmnopqrstuvwxyz", hex[0..40]);
        let s2: string = hex[0..40];
        expect_eq(s2, "f71c27109c692c1b56bbdceb5b9d2865b3708dbc");
    });

    test("4. @cap/ripemd160 — streaming multi-chunk update & reset match one-shot", () => {
        let h = Ripemd160.new();
        h.updateStr("message ");
        h.updateStr("digest");
        let hex: [40]u8 = [0; 40];
        * = h.finalizeHex(hex[0..40]);
        let s1: string = hex[0..40];
        expect_eq(s1, "5d0689ef49d2fae572b881b123a85ffa21595f36");

        h.reset();
        h.updateStr("ab");
        h.updateStr("c");
        * = h.finalizeHex(hex[0..40]);
        let s2: string = hex[0..40];
        expect_eq(s2, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
    });

    test("5. @cap/ripemd160 — multi-block (>64B) official vector: alphanumeric 62B & 80B digits", () => {
        let hex: [40]u8 = [0; 40];
        * = ripemd160Hex("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", hex[0..40]);
        let s1: string = hex[0..40];
        expect_eq(s1, "b0e20b6e3116640286ed3a87a5713079b21f5189");

        * = ripemd160Hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890", hex[0..40]);
        let s2: string = hex[0..40];
        expect_eq(s2, "9b752e45573d4b39f4dbd3323cab82bf63326bfb");
    });

    test("6. @cap/ripemd160 — ripemd160Bytes raw 20-byte output & undersized hex guard", () => {
        let raw: [3]u8 = [97, 98, 99]; // "abc"
        let out20: [20]u8 = [0; 20];
        ripemd160Bytes(raw[0..3], out20[0..20]);
        expect_eq(out20[0], 142); // 0x8e
        expect_eq(out20[1], 178); // 0xb2
        expect_eq(out20[18], 11); // 0x0b
        expect_eq(out20[19], 252); // 0xfc

        let short_hex: [10]u8 = [0; 10];
        expect_eq(ripemd160Hex("abc", short_hex[0..10]), 0);
    });
}
