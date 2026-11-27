import { test, expect_eq } from "cap:test";
import { BaseUnit, FormatSizeOptions, DECIMAL, BINARY, WINDOWS, formatSize, formatDecimal, formatBinary } from "../../packages/humansize/src/index.cp";

fn main() {
    test("formatDecimal handles bytes, kB, MB, GB, TB", () => {
        let buf: [64]u8 = undefined;
        let n = formatDecimal(0, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "0 B"), true);

        n = formatDecimal(999, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "999 B"), true);

        n = formatDecimal(1000, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 kB"), true);

        n = formatDecimal(1500000, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.50 MB"), true);

        n = formatDecimal(2500000000, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "2.50 GB"), true);
    });

    test("formatBinary handles IEC 1024 units", () => {
        let buf: [64]u8 = undefined;
        let n = formatBinary(1023, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1023 B"), true);

        n = formatBinary(1024, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 KiB"), true);

        n = formatBinary(1048576, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 MiB"), true);

        n = formatBinary(1572864, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.50 MiB"), true);
    });

    test("WINDOWS preset uses 1024 divisor with KB/MB labels", () => {
        let buf: [64]u8 = undefined;
        let n = formatSize(1024, WINDOWS, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 KB"), true);

        n = formatSize(1073741824, WINDOWS, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 GB"), true);
    });

    test("custom decimal_places and space_after_value options", () => {
        let buf: [64]u8 = undefined;
        const compact = FormatSizeOptions{
            base: BaseUnit.Decimal,
            decimal_places: 0,
            space_after_value: false,
            long_units: false,
        };
        let n = formatSize(1500, compact, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "2kB"), true);

        const precise = FormatSizeOptions{
            base: BaseUnit.Decimal,
            decimal_places: 3,
            space_after_value: true,
            long_units: false,
        };
        n = formatSize(1234567, precise, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.235 MB"), true);
    });

    test("long_units formats singular Byte, plural Bytes, and full prefixes", () => {
        let buf: [64]u8 = undefined;
        const dec_long = FormatSizeOptions{
            base: BaseUnit.Decimal,
            decimal_places: 1,
            space_after_value: true,
            long_units: true,
        };
        let n = formatSize(1, dec_long, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1 Byte"), true);

        n = formatSize(42, dec_long, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "42 Bytes"), true);

        n = formatSize(3500000, dec_long, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "3.5 Megabytes"), true);

        const bin_long = FormatSizeOptions{
            base: BaseUnit.Binary,
            decimal_places: 2,
            space_after_value: true,
            long_units: true,
        };
        n = formatSize(2048, bin_long, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "2.00 Kibibytes"), true);
    });

    test("rounding carry promotes unit and large u64 values stay in bounds", () => {
        let buf: [64]u8 = undefined;
        let n = formatDecimal(999999, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 MB"), true);

        // 10^18 = 1.00 EB
        n = formatDecimal(1000000000000000000, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "1.00 EB"), true);
    });
}
