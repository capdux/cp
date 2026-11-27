// cap/test/testCapDataEncoding.cp
// Comprehensive unit tests for @cap/data-encoding (RFC 4648 official test vectors + strict error checks)

import { test, expect_eq } from "cap:test";
import {
    DecodeResult,
    hexLowerEncode,
    hexUpperEncode,
    hexDecode,
    base32Encode,
    base32Decode,
    base32EncodeLen,
    base64Encode,
    base64Decode,
    base64EncodeLen,
} from "../../packages/data-encoding/src/index.cp";

fn bytesEqStr(buf: []const u8, n: usize, expected: string) -> bool {
    if (n != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/data-encoding — RFC 4648 Base16 / Hex encode & decode vectors", () => {
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        let n0 = hexUpperEncode("", enc[0..32]);
        expect_eq(n0, 0 as usize);

        let n1 = hexUpperEncode("foobar", enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n1, "666F6F626172"), true);

        let n2 = hexLowerEncode("foobar", enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n2, "666f6f626172"), true);

        // Mixed-case decode
        let r = hexDecode("666F6f626172", dec_buf[0..32]);
        expect_eq(r.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r.len, "foobar"), true);

        // Odd length & invalid hex char rejection
        let bad_odd = hexDecode("666", dec_buf[0..32]);
        expect_eq(bad_odd.ok, false);
        expect_eq(bad_odd.error_pos, 2 as usize);

        let bad_chr = hexDecode("66GG", dec_buf[0..32]);
        expect_eq(bad_chr.ok, false);
        expect_eq(bad_chr.error_pos, 2 as usize);
    });

    test("2. @cap/data-encoding — RFC 4648 Base32 official vectors (padded)", () => {
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        expect_eq(base32Encode("", true, enc[0..32]), 0 as usize);

        let n1 = base32Encode("f", true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n1, "MY======"), true);
        let r1 = base32Decode(enc[0..n1], dec_buf[0..32]);
        expect_eq(r1.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r1.len, "f"), true);

        let n2 = base32Encode("fo", true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n2, "MZXQ===="), true);
        let r2 = base32Decode(enc[0..n2], dec_buf[0..32]);
        expect_eq(r2.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r2.len, "fo"), true);

        let n3 = base32Encode("foo", true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n3, "MZXW6==="), true);
        let r3 = base32Decode(enc[0..n3], dec_buf[0..32]);
        expect_eq(r3.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r3.len, "foo"), true);

        let n4 = base32Encode("foob", true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n4, "MZXW6YQ="), true);
        let r4 = base32Decode(enc[0..n4], dec_buf[0..32]);
        expect_eq(r4.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r4.len, "foob"), true);

        let n5 = base32Encode("fooba", true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n5, "MZXW6YTB"), true);
        let r5 = base32Decode(enc[0..n5], dec_buf[0..32]);
        expect_eq(r5.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r5.len, "fooba"), true);

        let n6 = base32Encode("foobar", true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n6, "MZXW6YTBOI======"), true);
        let r6 = base32Decode(enc[0..n6], dec_buf[0..32]);
        expect_eq(r6.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r6.len, "foobar"), true);
    });

    test("3. @cap/data-encoding — RFC 4648 Base64 official vectors (padded)", () => {
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        expect_eq(base64Encode("", false, true, enc[0..32]), 0 as usize);

        let n1 = base64Encode("f", false, true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n1, "Zg=="), true);
        let r1 = base64Decode(enc[0..n1], false, dec_buf[0..32]);
        expect_eq(r1.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r1.len, "f"), true);

        let n2 = base64Encode("fo", false, true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n2, "Zm8="), true);
        let r2 = base64Decode(enc[0..n2], false, dec_buf[0..32]);
        expect_eq(r2.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r2.len, "fo"), true);

        let n3 = base64Encode("foo", false, true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n3, "Zm9v"), true);

        let n4 = base64Encode("foob", false, true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n4, "Zm9vYg=="), true);

        let n5 = base64Encode("fooba", false, true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n5, "Zm9vYmE="), true);

        let n6 = base64Encode("foobar", false, true, enc[0..32]);
        expect_eq(bytesEqStr(enc[0..32], n6, "Zm9vYmFy"), true);
        let r6 = base64Decode(enc[0..n6], false, dec_buf[0..32]);
        expect_eq(r6.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..32], r6.len, "foobar"), true);
    });

    test("4. @cap/data-encoding — Base64URL unpadded & Base32 unpadded round-trip", () => {
        let raw: [3]u8 = undefined;
        raw[0] = 0xFB as u8;
        raw[1] = 0xFF as u8;
        raw[2] = 0xBF as u8; // std: "+/+/", url: "-_-_"

        let enc: [16]u8 = undefined;
        let dec_buf: [16]u8 = undefined;

        let n_url = base64Encode(raw[0..3], true, false, enc[0..16]);
        expect_eq(bytesEqStr(enc[0..16], n_url, "-_-_"), true);
        let r_url = base64Decode(enc[0..n_url], true, dec_buf[0..16]);
        expect_eq(r_url.ok, true);
        expect_eq(r_url.len, 3 as usize);
        expect_eq(dec_buf[0], 0xFB as u8);
        expect_eq(dec_buf[1], 0xFF as u8);
        expect_eq(dec_buf[2], 0xBF as u8);

        // Unpadded Base64URL for "fo" -> "Zm8"
        let n_np = base64Encode("fo", true, false, enc[0..16]);
        expect_eq(bytesEqStr(enc[0..16], n_np, "Zm8"), true);
        let r_np = base64Decode("Zm8", true, dec_buf[0..16]);
        expect_eq(r_np.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..16], r_np.len, "fo"), true);

        // Unpadded Base32 for "foo" -> "MZXW6"
        let n_b32 = base32Encode("foo", false, enc[0..16]);
        expect_eq(bytesEqStr(enc[0..16], n_b32, "MZXW6"), true);
        let r_b32 = base32Decode("MZXW6", dec_buf[0..16]);
        expect_eq(r_b32.ok, true);
        expect_eq(bytesEqStr(dec_buf[0..16], r_b32.len, "foo"), true);
    });

    test("5. @cap/data-encoding — strict canonical non-zero trailing-bit & invalid char rejection", () => {
        let dec_buf: [16]u8 = undefined;

        // Base64: "Zg==" is canonical for 'f'; "Zh==" has non-zero trailing bits (must fail at pos 1).
        let b64_bad_trail = base64Decode("Zh==", false, dec_buf[0..16]);
        expect_eq(b64_bad_trail.ok, false);
        expect_eq(b64_bad_trail.error_pos, 1 as usize);

        // Base64: "Zm9=" has non-zero trailing bits on 3rd char (must fail at pos 2).
        let b64_bad_trail2 = base64Decode("Zm9=", false, dec_buf[0..16]);
        expect_eq(b64_bad_trail2.ok, false);
        expect_eq(b64_bad_trail2.error_pos, 2 as usize);

        // Base64: invalid length mod 4 == 1
        let b64_bad_len = base64Decode("A", false, dec_buf[0..16]);
        expect_eq(b64_bad_len.ok, false);

        // Base64URL rejects '+' from standard alphabet
        let b64_wrong_alpha = base64Decode("+/+/", true, dec_buf[0..16]);
        expect_eq(b64_wrong_alpha.ok, false);
        expect_eq(b64_wrong_alpha.error_pos, 0 as usize);

        // Base32: "MY======" is canonical for 'f'; "MZ======" has non-zero trailing bits (must fail at pos 1).
        let b32_bad_trail = base32Decode("MZ======", dec_buf[0..16]);
        expect_eq(b32_bad_trail.ok, false);
        expect_eq(b32_bad_trail.error_pos, 1 as usize);

        // Base32: invalid char '1'
        let b32_bad_chr = base32Decode("MZXW1===", dec_buf[0..16]);
        expect_eq(b32_bad_chr.ok, false);
        expect_eq(b32_bad_chr.error_pos, 4 as usize);
    });

    test("6. @cap/data-encoding — encode length estimators & small buffer protection", () => {
        expect_eq(base64EncodeLen(0 as usize, true), 0 as usize);
        expect_eq(base64EncodeLen(1 as usize, true), 4 as usize);
        expect_eq(base64EncodeLen(1 as usize, false), 2 as usize);
        expect_eq(base64EncodeLen(2 as usize, false), 3 as usize);
        expect_eq(base64EncodeLen(3 as usize, false), 4 as usize);

        expect_eq(base32EncodeLen(0 as usize, true), 0 as usize);
        expect_eq(base32EncodeLen(1 as usize, true), 8 as usize);
        expect_eq(base32EncodeLen(1 as usize, false), 2 as usize);
        expect_eq(base32EncodeLen(2 as usize, false), 4 as usize);
        expect_eq(base32EncodeLen(3 as usize, false), 5 as usize);
        expect_eq(base32EncodeLen(4 as usize, false), 7 as usize);
        expect_eq(base32EncodeLen(5 as usize, false), 8 as usize);

        // Buffer too small safely returns 0 / ok=false
        let tiny: [2]u8 = undefined;
        expect_eq(base64Encode("foobar", false, true, tiny[0..2]), 0 as usize);
        expect_eq(base32Encode("foobar", true, tiny[0..2]), 0 as usize);
        expect_eq(hexLowerEncode("foobar", tiny[0..2]), 0 as usize);
        expect_eq(base64Decode("Zm9vYmFy", false, tiny[0..2]).ok, false);
    });
}
