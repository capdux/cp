// cap/test/testCapBase36.cp
// Comprehensive unit tests for @cap/base36 (Base36 & radix 2..36 codec)

import { test, expect_eq } from "cap:test";
import {
    ParseU64Result,
    DecodeBytesResult,
    encodeRadix,
    decodeRadix,
    encodeBase36,
    decodeBase36,
    encodeU64,
    encodeU64Upper,
    decodeU64,
    encodedLenU64,
    encodeBytes,
    decodeBytes,
    isValidBase36,
    isValidBase36String,
} from "../../packages/base36/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/base36 — classic values 0, 35, 1234567890 & u64::MAX", () => {
        let buf: [32]u8 = undefined;
        expect_eq(sliceEq(buf[0..encodeBase36(0, buf[0..32])], "0"), true);
        expect_eq(sliceEq(buf[0..encodeBase36(35, buf[0..32])], "z"), true);
        expect_eq(sliceEq(buf[0..encodeBase36(1234567890, buf[0..32])], "kf12oi"), true);

        const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const n_max: usize = encodeBase36(max_u64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n_max], "3w5e11264sgsf"), true);
    });

    test("2. @cap/base36 — decodeBase36 case-insensitive round-trip", () => {
        const r1: ParseU64Result = decodeBase36("kf12oi");
        expect_eq(r1.valid, true);
        expect_eq(r1.value, 1234567890 as u64);

        const r2: ParseU64Result = decodeBase36("KF12OI");
        expect_eq(r2.valid, true);
        expect_eq(r2.value, 1234567890 as u64);

        const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const r3: ParseU64Result = decodeBase36("3W5E11264SGSF");
        expect_eq(r3.valid, true);
        expect_eq(r3.value, max_u64);
    });

    test("3. @cap/base36 — encodeRadix & decodeRadix across base 2, 8, 10, 16 & 36", () => {
        let buf: [64]u8 = undefined;

        const n2: usize = encodeRadix(42, 2, false, buf[0..64]);
        expect_eq(sliceEq(buf[0..n2], "101010"), true);
        expect_eq(decodeRadix("101010", 2).value, 42 as u64);

        const n8: usize = encodeRadix(255, 8, false, buf[0..64]);
        expect_eq(sliceEq(buf[0..n8], "377"), true);
        expect_eq(decodeRadix("377", 8).value, 255 as u64);

        const n10: usize = encodeRadix(98765, 10, false, buf[0..64]);
        expect_eq(sliceEq(buf[0..n10], "98765"), true);
        expect_eq(decodeRadix("98765", 10).value, 98765 as u64);

        const n16: usize = encodeRadix(0xDEAD, 16, true, buf[0..64]);
        expect_eq(sliceEq(buf[0..n16], "DEAD"), true);
        expect_eq(decodeRadix("dead", 16).value, 0xDEAD as u64);

        const n36: usize = encodeRadix(1234567890, 36, true, buf[0..64]);
        expect_eq(sliceEq(buf[0..n36], "KF12OI"), true);
    });

    test("4. @cap/base36 — rejects 64-bit overflow '3w5e11264sgsg'", () => {
        expect_eq(decodeBase36("3w5e11264sgsg").valid, false);
        expect_eq(decodeBase36("zzzzzzzzzzzzzz").valid, false);
        expect_eq(decodeRadix("10000000000000000", 16).valid, false);
    });

    test("5. @cap/base36 — isValidBase36 validation", () => {
        expect_eq(isValidBase36("0"), true);
        expect_eq(isValidBase36("HelloWorld99"), true);
        expect_eq(isValidBase36(""), false);
        expect_eq(isValidBase36("bad-char"), false);
        expect_eq(isValidBase36("3w5e11264sgsg"), false);
    });

    test("6. @cap/base36 — rejects invalid radix (1, 37), empty input & tiny buffers", () => {
        let buf: [16]u8 = undefined;
        expect_eq(encodeRadix(10, 1, false, buf[0..16]), 0 as usize);
        expect_eq(encodeRadix(10, 37, false, buf[0..16]), 0 as usize);
        expect_eq(decodeRadix("10", 1).valid, false);
        expect_eq(decodeRadix("10", 37).valid, false);
        expect_eq(decodeRadix("2", 2).valid, false);
        expect_eq(decodeRadix("", 10).valid, false);

        let tiny: [3]u8 = undefined;
        expect_eq(encodeBase36(1234567890, tiny[0..3]), 0 as usize);
    });

    test("7. @cap/base36 — encodeU64, encodeU64Upper, decodeU64 & encodedLenU64", () => {
        let buf: [32]u8 = undefined;
        const n1 = encodeU64(1234567890, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "kf12oi"), true);
        const n2 = encodeU64Upper(1234567890, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "KF12OI"), true);

        const d1 = decodeU64("kf12oi");
        expect_eq(d1.valid, true);
        expect_eq(d1.value, 1234567890 as u64);
        const d2 = decodeU64("Kf12Oi");
        expect_eq(d2.valid, true);
        expect_eq(d2.value, 1234567890 as u64);

        expect_eq(encodedLenU64(0, 36), 1 as usize);
        expect_eq(encodedLenU64(1234567890, 36), 6 as usize);
        expect_eq(encodedLenU64(42, 2), 6 as usize);
        expect_eq(encodedLenU64(42, 1), 0 as usize);
    });

    test("8. @cap/base36 — encodeBytes & decodeBytes round-trip with leading zeros and case-insensitivity", () => {
        let enc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const payload: [5]u8 = [0x00, 0x00, 0xDE, 0xAD, 0xBE];
        const elen = encodeBytes(payload[0..5], false, enc[0..64]);
        expect_eq(elen > 2, true);
        expect_eq(enc[0], '0' as u8);
        expect_eq(enc[1], '0' as u8);

        const res: DecodeBytesResult = decodeBytes(enc[0..elen] as string, dec_buf[0..64]);
        expect_eq(res.valid, true);
        expect_eq(res.len, 5 as usize);
        expect_eq(sliceEq(dec_buf[0..res.len], payload[0..5]), true);

        // Uppercase encode -> decodeBytes
        const ulen = encodeBytes(payload[0..5], true, enc[0..64]);
        const ures: DecodeBytesResult = decodeBytes(enc[0..ulen] as string, dec_buf[0..64]);
        expect_eq(ures.valid, true);
        expect_eq(sliceEq(dec_buf[0..ures.len], payload[0..5]), true);

        // All-zero bytes & invalid inputs
        const zeros: [3]u8 = [0, 0, 0];
        const zlen = encodeBytes(zeros[0..3], false, enc[0..64]);
        expect_eq(sliceEq(enc[0..zlen], "000"), true);
        const zdec = decodeBytes("000", dec_buf[0..64]);
        expect_eq(zdec.valid, true);
        expect_eq(zdec.len, 3 as usize);
        expect_eq(sliceEq(dec_buf[0..3], zeros[0..3]), true);

        expect_eq(decodeBytes("", dec_buf[0..64]).valid, false);
        expect_eq(decodeBytes("abc!def", dec_buf[0..64]).valid, false);
        expect_eq(isValidBase36String("3w5e11264sgsgZZZZ999"), true);
        expect_eq(isValidBase36String("bad_char"), false);
    });
}
