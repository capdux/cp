// cap/test/testCapBase32Crockford.cp
// Comprehensive unit tests for @cap/base32-crockford

import { test, expect_eq } from "cap:test";
import {
    encodeU64,
    decodeU64,
    encodeBytes,
    decodeBytes,
    formatGrouped,
} from "../../packages/crockford-base32/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/base32-crockford — encodeU64/decodeU64 classic values (0, 32, 1234, large u64)", () => {
        let buf: [32]u8 = undefined;
        let outVal: u64 = 0 as u64;

        const n0 = encodeU64(0 as u64, false, buf[0..32]);
        expect_eq(sliceEq(buf[0..n0], "0"), true);
        expect_eq(decodeU64(buf[0..n0], false, &outVal), true);
        expect_eq(outVal, 0 as u64);

        const n32 = encodeU64(32 as u64, false, buf[0..32]);
        expect_eq(sliceEq(buf[0..n32], "10"), true);

        const n1234 = encodeU64(1234 as u64, false, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1234], "16J"), true);
        expect_eq(decodeU64("16J", false, &outVal), true);
        expect_eq(outVal, 1234 as u64);

        const big: u64 = ((0x12345678 as u64) << (32 as u6)) | (0x9ABCDEF0 as u64);
        const nBig = encodeU64(big, false, buf[0..32]);
        expect_eq(decodeU64(buf[0..nBig], false, &outVal), true);
        expect_eq(outVal, big);
    });

    test("2. @cap/base32-crockford — Mod-37 check symbol valid & tampered rejection", () => {
        let buf: [32]u8 = undefined;
        let outVal: u64 = 0 as u64;

        // 36 % 37 == 36 -> check symbol 'U'
        const n36 = encodeU64(36 as u64, true, buf[0..32]);
        expect_eq(sliceEq(buf[0..n36], "14U"), true);
        expect_eq(decodeU64("14U", true, &outVal), true);
        expect_eq(outVal, 36 as u64);

        // 32 % 37 == 32 -> check symbol '*'
        const n32 = encodeU64(32 as u64, true, buf[0..32]);
        expect_eq(sliceEq(buf[0..n32], "10*"), true);
        expect_eq(decodeU64("10*", true, &outVal), true);
        expect_eq(outVal, 32 as u64);

        // Tampered digit must fail checksum.
        expect_eq(decodeU64("15U", true, &outVal), false);
        expect_eq(decodeU64("10~", true, &outVal), false);
    });

    test("3. @cap/base32-crockford — Case-insensitivity, I/L/O & '-' tolerance", () => {
        let v1: u64 = 0 as u64;
        let v2: u64 = 0 as u64;
        expect_eq(decodeU64("1011-00", false, &v1), true);
        expect_eq(decodeU64("i-o-l-1-O-0", false, &v2), true);
        expect_eq(v1, v2);
    });

    test("4. @cap/base32-crockford — encodeBytes / decodeBytes arbitrary byte stream round-trip", () => {
        const msg = "Hello, Crockford Base32!";
        let enc: [64]u8 = undefined;
        const en = encodeBytes(msg, enc[0..64]);
        expect_eq(en > (0 as usize), true);

        let decBuf: [64]u8 = undefined;
        const dn = decodeBytes(enc[0..en], decBuf[0..64]);
        expect_eq(dn, msg.len as i64);
        expect_eq(sliceEq(decBuf[0..(dn as usize)], msg), true);

        // Empty input round-trip.
        expect_eq(encodeBytes("", enc[0..64]), 0 as usize);
        expect_eq(decodeBytes("", decBuf[0..64]), 0 as i64);
    });

    test("5. @cap/base32-crockford — formatGrouped 4-character hyphen grouping", () => {
        let out: [32]u8 = undefined;
        const n1 = formatGrouped("16J", 4, out[0..32]);
        expect_eq(sliceEq(out[0..n1], "16J"), true);

        const n2 = formatGrouped("ABCDEFGH", 4, out[0..32]);
        expect_eq(sliceEq(out[0..n2], "ABCD-EFGH"), true);

        const n3 = formatGrouped("123456789", 4, out[0..32]);
        expect_eq(sliceEq(out[0..n3], "1234-5678-9"), true);
    });

    test("6. @cap/base32-crockford — Reject invalid 'U' in data, overflow & tiny buffers", () => {
        let v: u64 = 0 as u64;
        expect_eq(decodeU64("16U", false, &v), false);
        expect_eq(decodeU64("", false, &v), false);
        // 14 'Z' chars overflow 64 bits (14 * 5 = 70 bits).
        expect_eq(decodeU64("ZZZZZZZZZZZZZZ", false, &v), false);

        let decBuf: [2]u8 = undefined;
        expect_eq(decodeBytes("16U", decBuf[0..2]), -1 as i64);
        expect_eq(decodeBytes("91JPRV3F", decBuf[0..2]), -1 as i64); // needs 5 bytes

        let tiny: [2]u8 = undefined;
        expect_eq(encodeU64(1234 as u64, false, tiny[0..2]), 0 as usize);
        expect_eq(encodeBytes("hello", tiny[0..2]), 0 as usize);
        expect_eq(formatGrouped("ABCDEFGH", 4, tiny[0..2]), 0 as usize);
    });
}
