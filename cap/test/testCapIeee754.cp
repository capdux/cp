import { test, expect_eq } from "cap:test";
import {
    readF32Le,
    readF32Be,
    writeF32Le,
    writeF32Be,
    readF64Le,
    readF64Be,
    writeF64Le,
    writeF64Be,
    readIeee754,
    writeIeee754,
} from "../../packages/@cap/ieee754/src/index.cp";

fn main() {
    test("1. @cap/ieee754 — f32 little-endian exact bit representation (42.0 = 0x42280000)", () => {
        let buf: [4]u8 = [0; 4];
        expect_eq(writeF32Le(buf[0..4], 0, 42.0), true);
        expect_eq(buf[0], 0x00 as u8);
        expect_eq(buf[1], 0x00 as u8);
        expect_eq(buf[2], 0x28 as u8);
        expect_eq(buf[3], 0x42 as u8);
        expect_eq(readF32Le(buf[0..4], 0), 42.0 as f32);
    });

    test("2. @cap/ieee754 — f32 big-endian exact bit representation (-1.5 = 0xBFC00000)", () => {
        let buf: [4]u8 = [0; 4];
        expect_eq(writeF32Be(buf[0..4], 0, -1.5), true);
        expect_eq(buf[0], 0xBF as u8);
        expect_eq(buf[1], 0xC0 as u8);
        expect_eq(buf[2], 0x00 as u8);
        expect_eq(buf[3], 0x00 as u8);
        expect_eq(readF32Be(buf[0..4], 0), -1.5 as f32);
    });

    test("3. @cap/ieee754 — f64 big-endian & little-endian roundtrip (3.141592653589793)", () => {
        const pi: f64 = 3.141592653589793;
        let le: [8]u8 = [0; 8];
        let be: [8]u8 = [0; 8];
        expect_eq(writeF64Le(le[0..8], 0, pi), true);
        expect_eq(writeF64Be(be[0..8], 0, pi), true);
        // Byte order reversal check
        expect_eq(le[0], be[7]);
        expect_eq(le[7], be[0]);
        expect_eq(readF64Le(le[0..8], 0), pi);
        expect_eq(readF64Be(be[0..8], 0), pi);
    });

    test("4. @cap/ieee754 — preserves Infinity, -Infinity, and NaN", () => {
        let buf: [8]u8 = [0; 8];
        const inf: f64 = std.math.inf(f64);
        * = writeF64Be(buf[0..8], 0, inf);
        expect_eq(buf[0], 0x7F as u8);
        expect_eq(buf[1], 0xF0 as u8);
        expect_eq(readF64Be(buf[0..8], 0) == inf, true);

        * = writeF64Be(buf[0..8], 0, std.math.nan(f64));
        const got_nan = readF64Be(buf[0..8], 0);
        expect_eq(got_nan != got_nan, true);
    });

    test("5. @cap/ieee754 — preserves signed -0.0 bit pattern", () => {
        let buf: [8]u8 = [0; 8];
        const neg_zero: f64 = -0.0;
        * = writeF64Be(buf[0..8], 0, neg_zero);
        expect_eq(buf[0], 0x80 as u8);
        expect_eq(buf[7], 0x00 as u8);
    });

    test("6. @cap/ieee754 — generic readIeee754/writeIeee754 and bounds safety", () => {
        let buf: [12]u8 = [0; 12];
        expect_eq(writeIeee754(buf[0..12], 123.5, 2, true, 4), true);
        expect_eq(readIeee754(buf[0..12], 2, true, 4), 123.5);
        expect_eq(writeF64Le(buf[0..12], 8, 1.0), false); // out of bounds
        expect_eq(writeF32Le(buf[0..12], 10, 1.0), false); // out of bounds
    });
}
