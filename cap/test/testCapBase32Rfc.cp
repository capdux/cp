import { test, expect_eq } from "cap:test";
import {
    encode,
    decode,
    encodeHex,
    decodeHex,
    encodedLen,
} from "../../packages/hi-base32/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/base32-rfc — RFC 4648 §10 all 7 standard Base32 vectors", () => {
        const inputs: [7]string = ["", "f", "fo", "foo", "foob", "fooba", "foobar"];
        const expected: [7]string = [
            "",
            "MY======",
            "MZXQ====",
            "MZXW6===",
            "MZXW6YQ=",
            "MZXW6YTB",
            "MZXW6YTBOI======",
        ];
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        let i: usize = 0;
        while (i < 7) {
            const en = encode(inputs[i], true, enc[0..32]);
            expect_eq(en, expected[i].len);
            expect_eq(testEq(enc[0..en], expected[i]), true);

            const dn = decode(enc[0..en], dec_buf[0..32]);
            expect_eq(dn, inputs[i].len as i64);
            expect_eq(testEq(dec_buf[0..dn as usize], inputs[i]), true);
            i += 1;
        }
    });

    test("2. @cap/base32-rfc — Unpadded pad=false encode & decode roundtrip", () => {
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const en = encode("foobar", false, enc[0..32]);
        expect_eq(testEq(enc[0..en], "MZXW6YTBOI"), true);

        const dn = decode(enc[0..en], dec_buf[0..32]);
        expect_eq(testEq(dec_buf[0..dn as usize], "foobar"), true);
    });

    test("3. @cap/base32-rfc — RFC 4648 §10 all 7 base32hex vectors", () => {
        const inputs: [7]string = ["", "f", "fo", "foo", "foob", "fooba", "foobar"];
        const expected: [7]string = [
            "",
            "CO======",
            "CPNG====",
            "CPNMU===",
            "CPNMUOG=",
            "CPNMUOJ1",
            "CPNMUOJ1E8======",
        ];
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        let i: usize = 0;
        while (i < 7) {
            const en = encodeHex(inputs[i], true, enc[0..32]);
            expect_eq(testEq(enc[0..en], expected[i]), true);

            const dn = decodeHex(enc[0..en], dec_buf[0..32]);
            expect_eq(testEq(dec_buf[0..dn as usize], inputs[i]), true);
            i += 1;
        }
    });

    test("4. @cap/base32-rfc — Case-insensitive lowercase decoding", () => {
        let dec_buf: [32]u8 = undefined;
        const n1 = decode("mzxw6ytboi======", dec_buf[0..32]);
        expect_eq(testEq(dec_buf[0..n1 as usize], "foobar"), true);

        const n2 = decodeHex("cpnmuoj1e8", dec_buf[0..32]);
        expect_eq(testEq(dec_buf[0..n2 as usize], "foobar"), true);
    });

    test("5. @cap/base32-rfc — encodedLen exact padded & unpadded values", () => {
        expect_eq(encodedLen(0, true), 0 as usize);
        expect_eq(encodedLen(1, true), 8 as usize);
        expect_eq(encodedLen(1, false), 2 as usize);
        expect_eq(encodedLen(3, false), 5 as usize);
        expect_eq(encodedLen(5, true), 8 as usize);
        expect_eq(encodedLen(6, true), 16 as usize);
        expect_eq(encodedLen(6, false), 10 as usize);
    });

    test("6. @cap/base32-rfc — Rejects invalid chars '0'/'1'/'8'/'9', bad padding & undersized buffers", () => {
        let buf: [32]u8 = undefined;
        expect_eq(decode("MZXW0===", buf[0..32]), -1 as i64);
        expect_eq(decode("MZXW1===", buf[0..32]), -1 as i64);
        expect_eq(decode("MZXW8===", buf[0..32]), -1 as i64);
        expect_eq(decode("MZXW9===", buf[0..32]), -1 as i64);
        expect_eq(decode("MY===", buf[0..32]), -1 as i64);    // wrong padded length
        expect_eq(decode("M", buf[0..32]), -1 as i64);        // rem == 1 invalid
        expect_eq(decodeHex("ZZZZZZZZ", buf[0..32]), -1 as i64);

        let tiny: [3]u8 = undefined;
        expect_eq(encode("foobar", true, tiny[0..3]), 0 as usize);
        expect_eq(decode("MZXW6YTBOI======", tiny[0..3]), -1 as i64);
    });
}
