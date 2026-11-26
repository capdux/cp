// cap/test/testCapHex.cp
// Comprehensive unit tests for @cap/hex

import { test, expect_eq } from "cap:test";
import {
    encodeHex,
    encodeHexUpper,
    decodeHex,
    decodeHexPrefixed,
    isValidHex,
    encodeU32Hex,
    encodeU64Hex,
    hexDump,
} from "../../packages/@cap/hex/src/index.cp";

fn main() {
    test("1. @cap/hex — Lowercase & uppercase encoding and round-trip decode", () => {
        let enc_buf: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const lo = encodeHex("Hello, CP!", enc_buf[0..64]);
        expect_eq(lo, "48656c6c6f2c20435021");
        expect_eq(decodeHex(lo, dec_buf[0..64]) ?? "", "Hello, CP!");

        const hi = encodeHexUpper("Hello, CP!", enc_buf[0..64]);
        expect_eq(hi, "48656C6C6F2C20435021");
        expect_eq(decodeHex(hi, dec_buf[0..64]) ?? "", "Hello, CP!");

        expect_eq(encodeHex("", enc_buf[0..64]), "");
        expect_eq(decodeHex("", dec_buf[0..64]) ?? "err", "");
    });

    test("2. @cap/hex — Invalid hex & odd-length rejection + buffer overflow safety", () => {
        let small: [2]u8 = undefined;
        let out: [32]u8 = undefined;

        expect_eq(isValidHex("deadBEEF0123"), true);
        expect_eq(isValidHex(""), true);
        expect_eq(isValidHex("abc"), false);
        expect_eq(isValidHex("zz"), false);

        expect_eq(decodeHex("abc", out[0..32]) == null, true);
        expect_eq(decodeHex("486g", out[0..32]) == null, true);
        expect_eq(decodeHex("48656c", small[0..2]) == null, true);
    });

    test("3. @cap/hex — decodeHexPrefixed with 0x / 0X / plain", () => {
        let out: [32]u8 = undefined;
        expect_eq(decodeHexPrefixed("0x48656c6c6f", out[0..32]) ?? "", "Hello");
        expect_eq(decodeHexPrefixed("0X4350", out[0..32]) ?? "", "CP");
        expect_eq(decodeHexPrefixed("6162", out[0..32]) ?? "", "ab");
        expect_eq(decodeHexPrefixed("0x123", out[0..32]) == null, true);
    });

    test("4. @cap/hex — Fixed-width encodeU32Hex & encodeU64Hex", () => {
        let out: [32]u8 = undefined;
        expect_eq(encodeU32Hex(0, out[0..32]), "00000000");
        expect_eq(encodeU32Hex(0x00ff12ab, out[0..32]), "00ff12ab");
        expect_eq(encodeU32Hex(0xdeadbeef, out[0..32]), "deadbeef");

        const v64: u64 = ((0x01234567 as u64) << 32) | (0x89abcdef as u64);
        expect_eq(encodeU64Hex(v64, out[0..32]), "0123456789abcdef");
        expect_eq(encodeU64Hex(0, out[0..32]), "0000000000000000");
    });

    test("5. @cap/hex — Canonical 16-byte hexDump formatting (printable & control bytes)", () => {
        let out: [256]u8 = undefined;
        const dump = hexDump("Hello\n", out[0..256]);
        expect_eq(
            dump,
            "00000000  48 65 6c 6c 6f 0a                                 |Hello.|\n",
        );
        expect_eq(hexDump("", out[0..256]), "");
    });

    test("6. @cap/hex — Multi-line hexDump offset increment (20 bytes -> 2 lines)", () => {
        let out: [512]u8 = undefined;
        const dump = hexDump("0123456789abcdefABCD", out[0..512]);
        expect_eq(
            dump,
            "00000000  30 31 32 33 34 35 36 37  38 39 61 62 63 64 65 66  |0123456789abcdef|\n"
                ++ "00000010  41 42 43 44                                       |ABCD|\n",
        );
    });
}
