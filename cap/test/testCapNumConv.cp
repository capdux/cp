import { test, expect_eq } from "cap:test";
import {
    extendU8ToU64, extendI8ToI64, extendI32ToI64,
    truncateU64ToU8, truncateU64ToU16, truncateU64ToU32,
    tryU64ToU8, tryU64ToU16, tryU64ToU32,
    tryI64ToI8, tryI64ToI16, tryI64ToI32, tryI64ToU8, tryI64ToU32,
    saturatingI64ToU8, saturatingI64ToI8, saturatingI64ToI16, saturatingU64ToU32
} from "../../packages/@cap/num-conv/src/index.cp";

fn main() {
    test("lossless extend conversions", () => {
        expect_eq(extendU8ToU64(255), 255 as u64);
        expect_eq(extendI8ToI64(-128), -128 as i64);
        expect_eq(extendI32ToI64(-100000), -100000 as i64);
    });

    test("explicit bit-preserving truncate", () => {
        expect_eq(truncateU64ToU8(0x1234), 0x34 as u8);
        expect_eq(truncateU64ToU16(0x123456), 0x3456 as u16);
        expect_eq(truncateU64ToU32(0x10000002a), 0x2a as u32);
    });

    test("checked unsigned narrowing", () => {
        let a = tryU64ToU8(255);
        expect_eq(a.ok, true);
        expect_eq(a.val, 255 as u8);
        expect_eq(tryU64ToU8(256).ok, false);
        expect_eq(tryU64ToU16(65535).ok, true);
        expect_eq(tryU64ToU16(65536).ok, false);
        expect_eq(tryU64ToU32(4294967295).ok, true);
        expect_eq(tryU64ToU32(4294967296).ok, false);
    });

    test("checked signed narrowing", () => {
        expect_eq(tryI64ToI8(-128).ok, true);
        expect_eq(tryI64ToI8(127).ok, true);
        expect_eq(tryI64ToI8(-129).ok, false);
        expect_eq(tryI64ToI8(128).ok, false);
        expect_eq(tryI64ToI16(32767).ok, true);
        expect_eq(tryI64ToI16(32768).ok, false);
        expect_eq(tryI64ToI32(-2147483648).ok, true);
        expect_eq(tryI64ToI32(2147483648).ok, false);
    });

    test("checked signed-to-unsigned rejects negatives", () => {
        expect_eq(tryI64ToU8(-1).ok, false);
        expect_eq(tryI64ToU8(200).ok, true);
        expect_eq(tryI64ToU32(-5).ok, false);
        expect_eq(tryI64ToU32(1000000).ok, true);
    });

    test("saturating narrowing clamps extremes", () => {
        expect_eq(saturatingI64ToU8(-50), 0 as u8);
        expect_eq(saturatingI64ToU8(500), 255 as u8);
        expect_eq(saturatingI64ToI8(-999), -128 as i8);
        expect_eq(saturatingI64ToI8(999), 127 as i8);
        expect_eq(saturatingI64ToI16(-99999), -32768 as i16);
        expect_eq(saturatingU64ToU32(99999999999), 4294967295 as u32);
    });
}
