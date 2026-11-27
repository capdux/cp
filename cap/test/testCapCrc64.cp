// cap/test/testCapCrc64.cp
// Comprehensive unit tests for @cap/crc64 (ECMA-182, XZ, GO-ISO official vectors)

import { test, expect_eq } from "cap:test";
import {
    Crc64Hasher,
    crc64Ecma,
    crc64Xz,
    crc64Iso,
} from "../../packages/crc64fast/src/index.cp";

fn u64From(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn main() {
    test("1. @cap/crc64 — ECMA-182 official vector for \"123456789\"", () => {
        const expected = u64From(0x6C40DF5F as u32, 0x0B497347 as u32);
        expect_eq(crc64Ecma("123456789"), expected);
    });

    test("2. @cap/crc64 — CRC-64/XZ official vector for \"123456789\"", () => {
        const expected = u64From(0x995DC9BB as u32, 0xDF1939FA as u32);
        expect_eq(crc64Xz("123456789"), expected);
    });

    test("3. @cap/crc64 — CRC-64/GO-ISO official vector for \"123456789\"", () => {
        const expected = u64From(0xB90956C7 as u32, 0x75A41001 as u32);
        expect_eq(crc64Iso("123456789"), expected);
    });

    test("4. @cap/crc64 — empty input returns 0 across all three standards", () => {
        let empty: [1]u8 = [0];
        expect_eq(crc64Ecma(empty[0..0]), 0 as u64);
        expect_eq(crc64Xz(empty[0..0]), 0 as u64);
        expect_eq(crc64Iso(empty[0..0]), 0 as u64);
    });

    test("5. @cap/crc64 — streaming chunked updates match one-shot & reset reuses cleanly", () => {
        let h_ecma = Crc64Hasher.initEcma();
        h_ecma.update("123");
        h_ecma.update("456");
        h_ecma.update("789");
        expect_eq(h_ecma.finalize(), crc64Ecma("123456789"));

        let h_xz = Crc64Hasher.initXz();
        h_xz.update("12345");
        h_xz.update("6789");
        expect_eq(h_xz.finalize(), crc64Xz("123456789"));

        // Reset and compute a fresh payload
        h_xz.reset();
        h_xz.update("CapDux");
        expect_eq(h_xz.finalize(), crc64Xz("CapDux"));
    });

    test("6. @cap/crc64 — single-bit flip detection across all three standards", () => {
        const msg1 = "integrity-check-payload-0";
        const msg2 = "integrity-check-payload-1"; // 1-bit flip in last byte

        expect_eq(crc64Ecma(msg1) != crc64Ecma(msg2), true);
        expect_eq(crc64Xz(msg1) != crc64Xz(msg2), true);
        expect_eq(crc64Iso(msg1) != crc64Iso(msg2), true);
    });
}
