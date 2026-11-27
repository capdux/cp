import { test, expect_eq } from "cap:test";
import {
    formatU64, formatI64, formatU32, formatI32, formatUsize,
    formatHexU64, formatRadixU64
} from "../../packages/itoa/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/itoa — Unsigned decimal formatting (0, single, multi, u64 max)", () => {
        let buf: [32]u8 = undefined;
        expect_eq(eqBytes(buf[0..32], formatU64(0, buf[0..32]), "0"), true);
        expect_eq(eqBytes(buf[0..32], formatU64(7, buf[0..32]), "7"), true);
        expect_eq(eqBytes(buf[0..32], formatU64(42, buf[0..32]), "42"), true);
        expect_eq(eqBytes(buf[0..32], formatU64(1234567890, buf[0..32]), "1234567890"), true);
        let umax: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(eqBytes(buf[0..32], formatU64(umax, buf[0..32]), "18446744073709551615"), true);
    });

    test("2. @cap/itoa — Signed decimal formatting including i64::MIN and i64::MAX", () => {
        let buf: [32]u8 = undefined;
        expect_eq(eqBytes(buf[0..32], formatI64(0, buf[0..32]), "0"), true);
        expect_eq(eqBytes(buf[0..32], formatI64(-1, buf[0..32]), "-1"), true);
        expect_eq(eqBytes(buf[0..32], formatI64(-987654321, buf[0..32]), "-987654321"), true);
        let imax: i64 = 9223372036854775807;
        let imin: i64 = -9223372036854775807 - 1;
        expect_eq(eqBytes(buf[0..32], formatI64(imax, buf[0..32]), "9223372036854775807"), true);
        expect_eq(eqBytes(buf[0..32], formatI64(imin, buf[0..32]), "-9223372036854775808"), true);
    });

    test("3. @cap/itoa — 32-bit and usize wrappers", () => {
        let buf: [32]u8 = undefined;
        expect_eq(eqBytes(buf[0..32], formatU32(65535, buf[0..32]), "65535"), true);
        expect_eq(eqBytes(buf[0..32], formatI32(-2147483648, buf[0..32]), "-2147483648"), true);
        expect_eq(eqBytes(buf[0..32], formatUsize(1024, buf[0..32]), "1024"), true);
    });

    test("4. @cap/itoa — Hex formatting lower and upper", () => {
        let buf: [32]u8 = undefined;
        expect_eq(eqBytes(buf[0..32], formatHexU64(0, false, buf[0..32]), "0"), true);
        expect_eq(eqBytes(buf[0..32], formatHexU64(0xdeadbeef, false, buf[0..32]), "deadbeef"), true);
        expect_eq(eqBytes(buf[0..32], formatHexU64(0xCAFEBABE, true, buf[0..32]), "CAFEBABE"), true);
    });

    test("5. @cap/itoa — Arbitrary radix 2..36 formatting", () => {
        let buf: [64]u8 = undefined;
        expect_eq(eqBytes(buf[0..64], formatRadixU64(42, 2, buf[0..64]), "101010"), true);
        expect_eq(eqBytes(buf[0..64], formatRadixU64(255, 8, buf[0..64]), "377"), true);
        expect_eq(eqBytes(buf[0..64], formatRadixU64(1295, 36, buf[0..64]), "zz"), true);
        expect_eq(formatRadixU64(10, 1, buf[0..64]), 0);
        expect_eq(formatRadixU64(10, 37, buf[0..64]), 0);
    });

    test("6. @cap/itoa — Buffer-too-small returns 0 safely", () => {
        let tiny: [3]u8 = undefined;
        expect_eq(formatU64(1000, tiny[0..3]), 0);
        expect_eq(formatI64(-100, tiny[0..3]), 0);
        expect_eq(formatU64(999, tiny[0..3]), 3);
    });
}
