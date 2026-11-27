// cap/test/testCapBase85.cp
// Comprehensive unit tests for @cap/base85 (Z85 RFC 32 & Adobe Ascii85)

import { test, expect_eq } from "cap:test";
import {
    encodeZ85,
    decodeZ85,
    encodeAscii85,
    decodeAscii85,
} from "../../packages/z85/src/index.cp";

fn main() {
    test("1. @cap/base85 — Z85 RFC 32 official test vector (HelloWorld)", () => {
        let raw: [8]u8 = [0x86, 0x4F, 0xD2, 0x6F, 0xB5, 0x59, 0xF7, 0x5B];
        let enc_buf: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const enc = encodeZ85(raw[0..8], enc_buf[0..32]).?;
        expect_eq(enc, "HelloWorld");

        const decoded = decodeZ85(enc, dec_buf[0..32]).?;
        expect_eq(decoded.len, 8 as usize);
        expect_eq(decoded[0], 0x86 as u8);
        expect_eq(decoded[3], 0x6F as u8);
        expect_eq(decoded[4], 0xB5 as u8);
        expect_eq(decoded[7], 0x5B as u8);
    });

    test("2. @cap/base85 — Z85 rejects non-4/5 multiples, invalid chars & u32 overflow", () => {
        let buf: [32]u8 = undefined;
        expect_eq(encodeZ85("abc", buf[0..32]) == null, true);
        expect_eq(decodeZ85("abcd", buf[0..32]) == null, true);
        // Double quote is forbidden in Z85 alphabet
        expect_eq(decodeZ85("0000\"", buf[0..32]) == null, true);
        // "#####" = 84*85^4 + ... > u32::MAX (4294967295)
        expect_eq(decodeZ85("#####", buf[0..32]) == null, true);

        // Empty input works cleanly
        expect_eq(encodeZ85("", buf[0..32]).?, "");
        expect_eq(decodeZ85("", buf[0..32]).?, "");
    });

    test("3. @cap/base85 — Ascii85 arbitrary lengths (1, 2, 3, 4, 5, 11 bytes) round-trip", () => {
        let enc_buf: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const s1 = "A";
        expect_eq(decodeAscii85(encodeAscii85(s1, enc_buf[0..64]).?, dec_buf[0..64]).?, s1);

        const s2 = "AB";
        expect_eq(decodeAscii85(encodeAscii85(s2, enc_buf[0..64]).?, dec_buf[0..64]).?, s2);

        const s3 = "ABC";
        expect_eq(decodeAscii85(encodeAscii85(s3, enc_buf[0..64]).?, dec_buf[0..64]).?, s3);

        const s4 = "Man ";
        const e4 = encodeAscii85(s4, enc_buf[0..64]).?;
        expect_eq(e4, "9jqo^");
        expect_eq(decodeAscii85(e4, dec_buf[0..64]).?, s4);

        const s11 = "Hello World";
        expect_eq(decodeAscii85(encodeAscii85(s11, enc_buf[0..64]).?, dec_buf[0..64]).?, s11);
    });

    test("4. @cap/base85 — Ascii85 'z' all-zero 4-byte compression & expansion", () => {
        let zeros: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
        let enc_buf: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const enc = encodeAscii85(zeros[0..8], enc_buf[0..32]).?;
        expect_eq(enc, "zz");

        const decoded = decodeAscii85("zz", dec_buf[0..32]).?;
        expect_eq(decoded.len, 8 as usize);
        expect_eq(decoded[0], 0 as u8);
        expect_eq(decoded[7], 0 as u8);
    });

    test("5. @cap/base85 — Ascii85 <~...~> framing & whitespace tolerance", () => {
        let dec_buf: [64]u8 = undefined;
        const framed = "  <~ 9jqo^ \n z ~>  ";
        const decoded = decodeAscii85(framed, dec_buf[0..64]).?;
        expect_eq(decoded.len, 8 as usize);
        expect_eq(decoded[0..4], "Man ");
        expect_eq(decoded[4], 0 as u8);
        expect_eq(decoded[7], 0 as u8);
    });

    test("6. @cap/base85 — Ascii85 rejects invalid chars, mid-block 'z', lone char & overflow", () => {
        let buf: [32]u8 = undefined;
        // 'z' inside a partial group is forbidden
        expect_eq(decodeAscii85("!z", buf[0..32]) == null, true);
        // Single trailing char is invalid
        expect_eq(decodeAscii85("!", buf[0..32]) == null, true);
        // Character > 'u' (117) like 'v' is invalid
        expect_eq(decodeAscii85("vvvvv", buf[0..32]) == null, true);
        // "uuuuu" overflows u32::MAX
        expect_eq(decodeAscii85("uuuuu", buf[0..32]) == null, true);
    });
}
