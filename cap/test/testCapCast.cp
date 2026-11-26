import { test, expect_eq } from "cap:test";
import {
    CastError,
    u8FromI64,
    u8FromU64,
    u16FromI64,
    u32FromI64,
    u32FromU64,
    i32FromU64,
    i32FromI64,
    u64FromI64,
    i64FromU64,
    usizeFromI64,
    u32FromF64,
    i32FromF64,
    u8FromF64,
    checkF64ToI32,
} from "../../packages/@cap/cast/src/index.cp";

fn main() {
    test("1. @cap/cast — u8 and u16 checked bounds", () => {
        expect_eq(u8FromI64(0) ?? 99, 0 as u8);
        expect_eq(u8FromI64(255) ?? 0, 255 as u8);
        expect_eq(u8FromI64(-1) == null, true);
        expect_eq(u8FromI64(256) == null, true);
        expect_eq(u8FromU64(256) == null, true);
        expect_eq(u16FromI64(65535) ?? 0, 65535 as u16);
        expect_eq(u16FromI64(65536) == null, true);
    });

    test("2. @cap/cast — u32 and i32 checked bounds", () => {
        expect_eq(u32FromI64(4294967295) ?? 0, 4294967295 as u32);
        expect_eq(u32FromI64(4294967296) == null, true);
        expect_eq(u32FromI64(-5) == null, true);
        expect_eq(u32FromU64(4294967296) == null, true);
        expect_eq(i32FromU64(2147483647) ?? 0, 2147483647 as i32);
        expect_eq(i32FromU64(2147483648) == null, true);
        expect_eq(i32FromI64(-2147483648) ?? 0, -2147483648 as i32);
        expect_eq(i32FromI64(-2147483649) == null, true);
    });

    test("3. @cap/cast — u64/i64/usize sign boundary checks", () => {
        expect_eq(u64FromI64(0) ?? 9, 0 as u64);
        expect_eq(u64FromI64(-1) == null, true);
        const hi: u64 = ((0x80000000 as u64) << 32);
        expect_eq(i64FromU64(hi) == null, true);
        expect_eq(i64FromU64(hi - 1) != null, true);
        expect_eq(usizeFromI64(-1) == null, true);
        expect_eq(usizeFromI64(42) ?? 0, 42 as usize);
    });

    test("4. @cap/cast — f64 to u32/i32/u8 rejects NaN and Inf", () => {
        const nan_v: f64 = std.math.nan(f64);
        const pos_inf: f64 = std.math.inf(f64);
        const neg_inf: f64 = -pos_inf;
        expect_eq(u32FromF64(nan_v) == null, true);
        expect_eq(u32FromF64(pos_inf) == null, true);
        expect_eq(i32FromF64(nan_v) == null, true);
        expect_eq(i32FromF64(neg_inf) == null, true);
        expect_eq(u8FromF64(255.0) ?? 0, 255 as u8);
        expect_eq(u8FromF64(-0.1) == null, true);
    });

    test("5. @cap/cast — f64 valid truncating casts within range", () => {
        expect_eq(u32FromF64(123.9) ?? 0, 123 as u32);
        expect_eq(i32FromF64(-42.7) ?? 0, -42 as i32);
        expect_eq(i32FromF64(2147483648.0) == null, true);
        expect_eq(i32FromF64(-2147483649.0) == null, true);
    });

    test("6. @cap/cast — checkF64ToI32 diagnoses exact CastError", () => {
        expect_eq(checkF64ToI32(100.0) == null, true);
        expect_eq((checkF64ToI32(std.math.nan(f64)) ?? .Overflow) == .NaN, true);
        expect_eq((checkF64ToI32(std.math.inf(f64)) ?? .NaN) == .Infinite, true);
        expect_eq((checkF64ToI32(1000000000000.0) ?? .NaN) == .Overflow, true);
        expect_eq((checkF64ToI32(-1000000000000.0) ?? .NaN) == .Underflow, true);
    });
}
