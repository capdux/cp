// cap/test/testCapDataEncoding.cp
// Comprehensive unit tests for @cap/data-encoding

import { test, expect_eq } from "cap:test";
import {
    encodeBase32,
    decodeBase32,
    encodeBase32Hex,
    decodeBase32Hex,
    encodeCrockford,
    decodeCrockford,
    encodeBase64UrlNoPad,
    decodeBase64UrlNoPad,
} from "../../packages/@cap/data-encoding/src/index.cp";

fn main() {
    test("1. @cap/data-encoding — RFC 4648 Base32 official test vectors (padded & unpadded)", () => {
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        expect_eq(encodeBase32("", true, enc[0..64]), "");
        expect_eq(encodeBase32("f", true, enc[0..64]), "MY======");
        expect_eq(encodeBase32("fo", true, enc[0..64]), "MZXQ====");
        expect_eq(encodeBase32("foo", true, enc[0..64]), "MZXW6===");
        expect_eq(encodeBase32("foob", true, enc[0..64]), "MZXW6YQ=");
        expect_eq(encodeBase32("fooba", true, enc[0..64]), "MZXW6YTB");
        expect_eq(encodeBase32("foobar", true, enc[0..64]), "MZXW6YTBOI======");

        expect_eq(encodeBase32("foobar", false, enc[0..64]), "MZXW6YTBOI");
        expect_eq(decodeBase32("MZXW6YTBOI======", dec_buf[0..64]) ?? "", "foobar");
        expect_eq(decodeBase32("MZXW6YTBOI", dec_buf[0..64]) ?? "", "foobar");
        expect_eq(decodeBase32("MY======", dec_buf[0..64]) ?? "", "f");
    });

    test("2. @cap/data-encoding — RFC 4648 Base32hex test vectors & round-trip", () => {
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        expect_eq(encodeBase32Hex("f", true, enc[0..64]), "CO======");
        expect_eq(encodeBase32Hex("fo", true, enc[0..64]), "CPNG====");
        expect_eq(encodeBase32Hex("foo", true, enc[0..64]), "CPNMU===");
        expect_eq(encodeBase32Hex("foobar", true, enc[0..64]), "CPNMUOJ1E8======");

        expect_eq(decodeBase32Hex("CPNMUOJ1E8======", dec_buf[0..64]) ?? "", "foobar");
        expect_eq(decodeBase32Hex("CPNMUOJ1E8", dec_buf[0..64]) ?? "", "foobar");
    });

    test("3. @cap/data-encoding — Crockford Base32 encoding, i/l/o tolerance & hyphens", () => {
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const c = encodeCrockford("Hello", enc[0..64]);
        expect_eq(c, "91JPRV3F");
        expect_eq(decodeCrockford(c, dec_buf[0..64]) ?? "", "Hello");

        // Tolerates lowercase, hyphens, i/l -> 1, o -> 0
        expect_eq(decodeCrockford("9i-jp-rv-3f", dec_buf[0..64]) ?? "", "Hello");
        expect_eq(decodeCrockford("9LJP-RV3F", dec_buf[0..64]) ?? "", "Hello");

        // 'U' is forbidden in Crockford Base32
        expect_eq(decodeCrockford("91JPRV3U", dec_buf[0..64]) == null, true);
    });

    test("4. @cap/data-encoding — Base64url (no-pad) with binary bytes containing 0xFB/0xFF", () => {
        let bin: [4]u8 = [0xFB, 0xFF, 0xBF, 0x00];
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const u = encodeBase64UrlNoPad(bin[0..4], enc[0..64]);
        // Standard base64 would use '+' and '/' and '=='; base64url uses '-' and '_' and no '='
        expect_eq(u, "-_-_AA");

        const back = decodeBase64UrlNoPad(u, dec_buf[0..64]);
        expect_eq(back != null, true);
        expect_eq(back.?.len, 4 as usize);
        expect_eq(back.?[0], 0xFB as u8);
        expect_eq(back.?[1], 0xFF as u8);
        expect_eq(back.?[2], 0xBF as u8);
        expect_eq(back.?[3], 0x00 as u8);
    });

    test("5. @cap/data-encoding — Arbitrary lengths 1..7 round-trip across all 4 codecs", () => {
        const msg = "Capdux!";
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;
        let len: usize = 1;
        while (len <= msg.len) {
            const sub = msg[0..len];
            expect_eq(decodeBase32(encodeBase32(sub, true, enc[0..64]), dec_buf[0..64]) ?? "", sub);
            expect_eq(decodeBase32Hex(encodeBase32Hex(sub, false, enc[0..64]), dec_buf[0..64]) ?? "", sub);
            expect_eq(decodeCrockford(encodeCrockford(sub, enc[0..64]), dec_buf[0..64]) ?? "", sub);
            expect_eq(decodeBase64UrlNoPad(encodeBase64UrlNoPad(sub, enc[0..64]), dec_buf[0..64]) ?? "", sub);
            len += 1;
        }
    });

    test("6. @cap/data-encoding — Strict rejection of invalid chars, padding & non-zero trailing bits", () => {
        let dec_buf: [64]u8 = undefined;

        // Invalid Base32: lowercase, bad char '1', non-canonical trailing bits ('MZ' has trailing 01 != 0)
        expect_eq(decodeBase32("my======", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase32("M1======", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase32("MZ======", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase32("M", dec_buf[0..64]) == null, true);

        // Invalid Base64url: rejects '+', '/', '=', bad length, non-zero trailing bits ('AB' -> low 4 bits = 1)
        expect_eq(decodeBase64UrlNoPad("abc=", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase64UrlNoPad("ab+d", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase64UrlNoPad("a", dec_buf[0..64]) == null, true);
        expect_eq(decodeBase64UrlNoPad("AB", dec_buf[0..64]) == null, true);
    });
}
