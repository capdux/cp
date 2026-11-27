import { test, expect_eq } from "cap:test";
import {
    BufWriter,
    writeStr,
    writeU64,
    writeI64,
    writeHexByte,
    parseU64,
    parseI64,
    parseU32,
    parseU16,
    parseHexU64,
} from "cap:fmt";

fn main() {
    test("1. cap:fmt — BufWriter basic text, repeat, and truncation safety", () => {
        let buf: [16]u8 = undefined;
        let w = BufWriter.new(buf[0..]);
        expect_eq(w.written(), 0);
        expect_eq(w.remaining(), 16);

        w.writeStr("hi");
        w.writeByte(58); // ':'
        w.writeRepeat(45, 3); // "---"
        expect_eq(w.written(), 6);
        expect_eq(w.remaining(), 10);
        expect_eq(w.asString(), "hi:---");

        // Overflow truncation must clamp safely at buffer capacity
        w.writeStr("0123456789ABCDEF");
        expect_eq(w.written(), 16);
        expect_eq(w.remaining(), 0);
        expect_eq(w.asString(), "hi:---0123456789");
    });

    test("2. cap:fmt — BufWriter integer and hex formatting including extremes", () => {
        let buf: [128]u8 = undefined;
        let w = BufWriter.new(buf[0..]);
        w.writeU64(0);
        w.writeByte(44);
        let max_u64 = (@as(u64, 0xFFFFFFFF) << 32) | @as(u64, 0xFFFFFFFF);
        w.writeU64(max_u64);
        w.writeByte(44);
        w.writeI64(-9223372036854775807 - 1);
        w.writeByte(44);
        w.writeHexU64(0, false);
        w.writeByte(44);
        w.writeHexU64(0xdeadbeef, false);
        w.writeByte(44);
        w.writeHexU64(0xCAFE, true);
        w.writeByte(44);
        w.writeHexByte(0x0a, false);
        w.writeByte(44);
        w.writeHexByte(0x0a, true);
        expect_eq(
            w.asString(),
            "0,18446744073709551615,-9223372036854775808,0,deadbeef,CAFE,0a,0A"
        );
    });

    test("3. cap:fmt — BufWriter writeFloatFixed rounding and special values", () => {
        let buf: [128]u8 = undefined;
        let w = BufWriter.new(buf[0..]);
        w.writeFloatFixed(3.14159, 2);
        w.writeByte(44);
        w.writeFloatFixed(1.996, 2);
        w.writeByte(44);
        w.writeFloatFixed(-0.05, 2);
        w.writeByte(44);
        w.writeFloatFixed(42.7, 0);
        w.writeByte(44);
        w.writeFloatFixed(std.math.nan(f64), 2);
        w.writeByte(44);
        w.writeFloatFixed(std.math.inf(f64), 2);
        w.writeByte(44);
        w.writeFloatFixed(-std.math.inf(f64), 2);
        expect_eq(w.asString(), "3.14,2.00,-0.05,43,NaN,Infinity,-Infinity");
    });

    test("4. cap:fmt — BufWriter writeJsonStr escaping", () => {
        let buf: [128]u8 = undefined;
        let w = BufWriter.new(buf[0..]);
        w.writeJsonStr("a\"b\\c\n\r\t\x01");
        expect_eq(w.asString(), "\"a\\\"b\\\\c\\n\\r\\t\\u0001\"");
    });

    test("5. cap:fmt — Free-function writeStr/writeU64/writeI64/writeHexByte", () => {
        let buf: [64]u8 = undefined;
        let p: usize = 0;
        p = writeStr("u=", buf[0..], p);
        p = writeU64(12345, buf[0..], p);
        p = writeStr(",i=", buf[0..], p);
        p = writeI64(-987, buf[0..], p);
        p = writeStr(",h=", buf[0..], p);
        p = writeHexByte(0x5f, true, buf[0..], p);
        expect_eq(buf[0..p], "u=12345,i=-987,h=5F");
    });

    test("6. cap:fmt — Strict zero-heap integer parsers (decimal, signed, hex, overflow)", () => {
        // parseU64
        expect_eq(parseU64("0") ?? 1, 0);
        expect_eq(parseU64("+42") ?? 0, 42);
        let max_u64 = (@as(u64, 0xFFFFFFFF) << 32) | @as(u64, 0xFFFFFFFF);
        expect_eq(parseU64("18446744073709551615") ?? 0, max_u64);
        expect_eq(parseU64("18446744073709551616") == null, true);
        expect_eq(parseU64("-1") == null, true);
        expect_eq(parseU64("") == null, true);
        expect_eq(parseU64("12a") == null, true);

        // parseI64
        expect_eq(parseI64("-9223372036854775808") ?? 0, -9223372036854775807 - 1);
        expect_eq(parseI64("9223372036854775807") ?? 0, 9223372036854775807);
        expect_eq(parseI64("+123") ?? 0, 123);
        expect_eq(parseI64("-0") ?? 1, 0);
        expect_eq(parseI64("-9223372036854775809") == null, true);
        expect_eq(parseI64("9223372036854775808") == null, true);
        expect_eq(parseI64("-") == null, true);

        // parseU32 & parseU16
        expect_eq(parseU32("4294967295") ?? 0, 4294967295);
        expect_eq(parseU32("4294967296") == null, true);
        expect_eq(parseU16("65535") ?? 0, 65535);
        expect_eq(parseU16("65536") == null, true);

        // parseHexU64
        expect_eq(parseHexU64("0") ?? 1, 0);
        expect_eq(parseHexU64("deadBEEF") ?? 0, 0xdeadbeef);
        expect_eq(parseHexU64("0xFFFFFFFFFFFFFFFF") ?? 0, max_u64);
        expect_eq(parseHexU64("0x10000000000000000") == null, true);
        expect_eq(parseHexU64("0x") == null, true);
        expect_eq(parseHexU64("0xZZ") == null, true);
    });
}
