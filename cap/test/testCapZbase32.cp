import { test, expect_eq } from "cap:test";
import {
    encode,
    decode,
    encodedLen,
    isValidZBase32,
} from "../../packages/@cap/zbase32/src/index.cp";

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
    test("1. @cap/zbase32 — Official test vectors [0x00] -> 'yy', [0xf0,0xbf,0xc7] -> '6n9hq' & 'hello'", () => {
        let out: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const z: [1]u8 = [0x00];
        const n0 = encode(z[0..1], out[0..32]);
        expect_eq(testEq(out[0..n0], "yy"), true);

        const v1: [3]u8 = [0xF0, 0xBF, 0xC7];
        const n1 = encode(v1[0..3], out[0..32]);
        expect_eq(testEq(out[0..n1], "6n9hq"), true);

        const nh = encode("hello", out[0..32]);
        expect_eq(nh, 8 as usize);
        const dh = decode(out[0..nh], dec_buf[0..32]);
        expect_eq(dh, 5 as i64);
        expect_eq(testEq(dec_buf[0..5], "hello"), true);
    });

    test("2. @cap/zbase32 — Full round-trip for every payload length 1..17 bytes", () => {
        let src: [17]u8 = undefined;
        let i: usize = 0;
        while (i < 17) {
            src[i] = ((i * 37 + 13) & 0xFF) as u8;
            i += 1;
        }

        let enc_buf: [64]u8 = undefined;
        let dec_buf: [32]u8 = undefined;
        let len: usize = 1;
        while (len <= 17) {
            const en = encode(src[0..len], enc_buf[0..64]);
            expect_eq(en, encodedLen(len));
            const dn = decode(enc_buf[0..en], dec_buf[0..32]);
            expect_eq(dn, len as i64);
            expect_eq(testEq(dec_buf[0..len], src[0..len]), true);
            len += 1;
        }
    });

    test("3. @cap/zbase32 — Case-insensitive decoding", () => {
        let out: [16]u8 = undefined;
        const n = decode("6N9HQ", out[0..16]);
        expect_eq(n, 3 as i64);
        expect_eq(out[0], 0xF0 as u8);
        expect_eq(out[1], 0xBF as u8);
        expect_eq(out[2], 0xC7 as u8);
    });

    test("4. @cap/zbase32 — encodedLen exact formula", () => {
        expect_eq(encodedLen(0), 0 as usize);
        expect_eq(encodedLen(1), 2 as usize);
        expect_eq(encodedLen(2), 4 as usize);
        expect_eq(encodedLen(3), 5 as usize);
        expect_eq(encodedLen(4), 7 as usize);
        expect_eq(encodedLen(5), 8 as usize);
    });

    test("5. @cap/zbase32 — isValidZBase32 rejects '0', 'l', 'v', '2' & invalid chars", () => {
        expect_eq(isValidZBase32("ybndrfg8ejkmcpqxot1uwisza345h769"), true);
        expect_eq(isValidZBase32("6N9HQ"), true);
        expect_eq(isValidZBase32("abc0def"), false);
        expect_eq(isValidZBase32("abcldef"), false);
        expect_eq(isValidZBase32("abcvdef"), false);
        expect_eq(isValidZBase32("abc2def"), false);
        expect_eq(isValidZBase32("abc="), false);
    });

    test("6. @cap/zbase32 — Empty input & undersized buffer protection", () => {
        let buf: [16]u8 = undefined;
        expect_eq(encode("", buf[0..16]), 0 as usize);
        expect_eq(decode("", buf[0..16]), 0 as i64);

        let tiny: [2]u8 = undefined;
        expect_eq(encode("hello", tiny[0..2]), 0 as usize);
        expect_eq(decode("6n9hq", tiny[0..2]), -1 as i64);
        expect_eq(decode("bad0char", buf[0..16]), -1 as i64);
    });
}
