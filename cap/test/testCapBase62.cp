import { test, expect_eq } from "cap:test";
import {
    encodeU64,
    encodeU64Padded,
    decodeU64,
    encodeBytes,
    decodeBytes,
    isValidBase62,
} from "../../packages/base62/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
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
    test("u64 encode and decode standard vectors including 0 and u64::MAX", () => {
        let buf: [16]u8 = undefined;
        let n0 = encodeU64(0, buf[0..16]);
        expect_eq(n0, 1);
        expect_eq(eqBytes(buf[0..n0], "0"), true);

        let n1 = encodeU64(61, buf[0..16]);
        expect_eq(eqBytes(buf[0..n1], "z"), true);

        let n2 = encodeU64(62, buf[0..16]);
        expect_eq(eqBytes(buf[0..n2], "10"), true);

        const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        let nm = encodeU64(max_u64, buf[0..16]);
        expect_eq(nm, 11);
        expect_eq(eqBytes(buf[0..nm], "LygHa16AHYF"), true);
        let dec_res = decodeU64(buf[0..nm]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.value, max_u64);
    });

    test("u64 overflow and invalid character rejection", () => {
        expect_eq(decodeU64("").ok, false);
        expect_eq(decodeU64("abc-123").ok, false);
        expect_eq(decodeU64("LygHa16AHYG").ok, false);
        expect_eq(decodeU64("zzzzzzzzzzz").ok, false);
        expect_eq(isValidBase62("09AZaz"), true);
        expect_eq(isValidBase62("09_az"), false);
    });

    test("padded u64 encoding preserves lexicographical order", () => {
        let a: [11]u8 = undefined;
        let b: [11]u8 = undefined;
        expect_eq(encodeU64Padded(42, 11, a[0..11]), 11);
        expect_eq(encodeU64Padded(999999, 11, b[0..11]), 11);
        expect_eq(decodeU64(a[0..11]).value, 42);
        expect_eq(decodeU64(b[0..11]).value, 999999);
        expect_eq(a[0], '0');
    });

    test("byte slice roundtrip preserves arbitrary binary and leading zeros", () => {
        const raw: [8]u8 = [0, 0, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x42];
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;
        let elen = encodeBytes(raw[0..8], enc[0..32]);
        expect_eq(elen > 0, true);
        expect_eq(enc[0], '0');
        expect_eq(enc[1], '0');
        let dlen = decodeBytes(enc[0..(elen as usize)], dec_buf[0..32]);
        expect_eq(dlen, 8);
        expect_eq(eqBytes(dec_buf[0..8], raw[0..8]), true);
    });

    test("UTF-8 string roundtrip via encodeBytes/decodeBytes", () => {
        const msg: []const u8 = "Hello, Capdux Base62!";
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;
        let elen = encodeBytes(msg, enc[0..64]);
        expect_eq(elen > 0, true);
        let dlen = decodeBytes(enc[0..(elen as usize)], dec_buf[0..64]);
        expect_eq(dlen, msg.len as i64);
        expect_eq(eqBytes(dec_buf[0..(dlen as usize)], msg), true);
    });

    test("empty and undersized buffer safety", () => {
        let small: [2]u8 = undefined;
        expect_eq(encodeU64(999999, small[0..2]), 0);
        expect_eq(encodeBytes("", small[0..2]), 0);
        expect_eq(decodeBytes("", small[0..2]), 0);
        expect_eq(decodeBytes("invalid!", small[0..2]), -1);
    });
}
