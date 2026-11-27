import { test, expect_eq } from "cap:test";
import {
    FormatSpec,
    parseFormatSpec,
    formatNumber,
} from "../../packages/d3-format/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn main() {
    test("1. @cap/d3-format — parseFormatSpec parses all D3/Python specifier parts", () => {
        const s1 = parseFormatSpec(",.2f");
        expect_eq(s1.valid, true);
        expect_eq(s1.comma, true);
        expect_eq(s1.precision, 2);
        expect_eq(s1.ty, 'f');

        const s2 = parseFormatSpec("+08d");
        expect_eq(s2.valid, true);
        expect_eq(s2.sign, '+');
        expect_eq(s2.zero, true);
        expect_eq(s2.width, 8);
        expect_eq(s2.ty, 'd');

        const s3 = parseFormatSpec("*^12,.1%");
        expect_eq(s3.valid, true);
        expect_eq(s3.fill, '*');
        expect_eq(s3.@"align", '^');
        expect_eq(s3.width, 12);
        expect_eq(s3.comma, true);
        expect_eq(s3.precision, 1);
        expect_eq(s3.ty, '%');

        const s4 = parseFormatSpec("#X");
        expect_eq(s4.valid, true);
        expect_eq(s4.symbol, '#');
        expect_eq(s4.ty, 'X');

        const bad1 = parseFormatSpec(".f");
        expect_eq(bad1.valid, false);
        const bad2 = parseFormatSpec("10z");
        expect_eq(bad2.valid, false);
    });

    test("2. @cap/d3-format — Fixed-point, thousands grouping & currency", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatNumber(",.2f", 1234567.891, buf[0..]);
        expect_eq(strEq(buf[0..n1], "1,234,567.89"), true);

        const n2 = formatNumber("$,.2f", -9876.5, buf[0..]);
        expect_eq(strEq(buf[0..n2], "-$9,876.50"), true);

        const n3 = formatNumber(".0f", 3.6, buf[0..]);
        expect_eq(strEq(buf[0..n3], "4"), true);

        const n4 = formatNumber(".3~f", 12.5, buf[0..]);
        expect_eq(strEq(buf[0..n4], "12.5"), true);
    });

    test("3. @cap/d3-format — Percentage & scientific notation", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatNumber(".1%", 0.125, buf[0..]);
        expect_eq(strEq(buf[0..n1], "12.5%"), true);

        const n2 = formatNumber("+.0%", 0.42, buf[0..]);
        expect_eq(strEq(buf[0..n2], "+42%"), true);

        const n3 = formatNumber(".2e", 1234.0, buf[0..]);
        expect_eq(strEq(buf[0..n3], "1.23e+03"), true);

        const n4 = formatNumber(".1E", 0.0052, buf[0..]);
        expect_eq(strEq(buf[0..n4], "5.2E-03"), true);
    });

    test("4. @cap/d3-format — SI prefix scaling (k/M/G/m/µ)", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatNumber(".2s", 1500.0, buf[0..]);
        expect_eq(strEq(buf[0..n1], "1.5k"), true);

        const n2 = formatNumber(".2s", 42000000.0, buf[0..]);
        expect_eq(strEq(buf[0..n2], "42M"), true);

        const n3 = formatNumber(".3s", 1250000000.0, buf[0..]);
        expect_eq(strEq(buf[0..n3], "1.25G"), true);

        const n4 = formatNumber(".3s", 0.00123, buf[0..]);
        expect_eq(strEq(buf[0..n4], "1.23m"), true);

        const n5 = formatNumber(".2s", 0.0000025, buf[0..]);
        expect_eq(strEq(buf[0..n5], "2.5µ"), true);
    });

    test("5. @cap/d3-format — Integer, hex/binary/octal & alternate '#' prefix", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatNumber(",d", 1000000.0, buf[0..]);
        expect_eq(strEq(buf[0..n1], "1,000,000"), true);

        const n2 = formatNumber("#x", 255.0, buf[0..]);
        expect_eq(strEq(buf[0..n2], "0xff"), true);

        const n3 = formatNumber("#X", 48879.0, buf[0..]);
        expect_eq(strEq(buf[0..n3], "0xBEEF"), false);
        expect_eq(strEq(buf[0..n3], "0XBEEF"), true);

        const n4 = formatNumber("#b", 10.0, buf[0..]);
        expect_eq(strEq(buf[0..n4], "0b1010"), true);

        const n5 = formatNumber("#o", 64.0, buf[0..]);
        expect_eq(strEq(buf[0..n5], "0o100"), true);
    });

    test("6. @cap/d3-format — Width alignment, zero-padding & NaN/Infinity", () => {
        let buf: [64]u8 = undefined;

        const n1 = formatNumber("+08d", 42.0, buf[0..]);
        expect_eq(strEq(buf[0..n1], "+0000042"), true);

        const n2 = formatNumber(">10.2f", 3.14, buf[0..]);
        expect_eq(strEq(buf[0..n2], "      3.14"), true);

        const n3 = formatNumber("_<8d", 7.0, buf[0..]);
        expect_eq(strEq(buf[0..n3], "7_______"), true);

        const n4 = formatNumber("*^7d", 99.0, buf[0..]);
        expect_eq(strEq(buf[0..n4], "**99***"), true);

        const n5 = formatNumber(".2f", std.math.nan(f64), buf[0..]);
        expect_eq(strEq(buf[0..n5], "NaN"), true);

        const n6 = formatNumber(".2f", -std.math.inf(f64), buf[0..]);
        expect_eq(strEq(buf[0..n6], "-Infinity"), true);
    });
}
