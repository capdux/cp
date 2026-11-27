import { test, expect_eq } from "cap:test";
import {
    ALIGN_RIGHT,
    ALIGN_LEFT,
    ALIGN_CENTER,
    IntFmtSpec,
    formatU64,
    formatI64,
    formatDecPadded,
    formatHex,
    formatWithCommas,
} from "../../packages/powerfmt/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/powerfmt — Zero-padding vs space-padding negative sign alignment", () => {
        let buf: [32]u8 = undefined;
        const n1 = formatDecPadded(-42, 5, &buf);
        expect_eq(strEq(buf[0..n1], "-0042"), true);

        let sp = IntFmtSpec.defaultSpec();
        sp.min_width = 5;
        sp.pad_byte = 32; // ' '
        const n2 = formatI64(-42, sp, &buf);
        expect_eq(strEq(buf[0..n2], "  -42"), true);

        const n3 = formatDecPadded(7, 4, &buf);
        expect_eq(strEq(buf[0..n3], "0007"), true);
    });

    test("2. @cap/powerfmt — Left, right & center alignment", () => {
        let buf: [32]u8 = undefined;
        let sp = IntFmtSpec.defaultSpec();
        sp.min_width = 6;
        sp.pad_byte = 46; // '.'

        sp.alignment = ALIGN_RIGHT;
        const nr = formatI64(42, sp, &buf);
        expect_eq(strEq(buf[0..nr], "....42"), true);

        sp.alignment = ALIGN_LEFT;
        const nl = formatI64(42, sp, &buf);
        expect_eq(strEq(buf[0..nl], "42...."), true);

        sp.alignment = ALIGN_CENTER;
        const nc = formatI64(42, sp, &buf);
        expect_eq(strEq(buf[0..nc], "..42.."), true);

        sp.min_width = 7;
        const nc2 = formatI64(42, sp, &buf);
        expect_eq(strEq(buf[0..nc2], "..42..."), true);
    });

    test("3. @cap/powerfmt — Binary, octal & hex (lower/upper) radices", () => {
        let buf: [64]u8 = undefined;

        const nh_lo = formatHex(3735928559, false, &buf); // 0xdeadbeef
        expect_eq(strEq(buf[0..nh_lo], "deadbeef"), true);

        const nh_hi = formatHex(3735928559, true, &buf);
        expect_eq(strEq(buf[0..nh_hi], "DEADBEEF"), true);

        let sb = IntFmtSpec.defaultSpec();
        sb.radix = 2;
        const nb = formatU64(42, sb, &buf);
        expect_eq(strEq(buf[0..nb], "101010"), true);

        let so = IntFmtSpec.defaultSpec();
        so.radix = 8;
        const no = formatU64(493, so, &buf); // 0o755
        expect_eq(strEq(buf[0..no], "755"), true);
    });

    test("4. @cap/powerfmt — Thousands comma grouping for positive & negative numbers", () => {
        let buf: [32]u8 = undefined;

        const n1 = formatWithCommas(1234567, &buf);
        expect_eq(strEq(buf[0..n1], "1,234,567"), true);

        const n2 = formatWithCommas(-1000, &buf);
        expect_eq(strEq(buf[0..n2], "-1,000"), true);

        const n3 = formatWithCommas(999, &buf);
        expect_eq(strEq(buf[0..n3], "999"), true);

        const n4 = formatWithCommas(0, &buf);
        expect_eq(strEq(buf[0..n4], "0"), true);
    });

    test("5. @cap/powerfmt — force_sign '+42', zero & i64::MIN boundary", () => {
        let buf: [32]u8 = undefined;
        let sp = IntFmtSpec.defaultSpec();
        sp.force_sign = true;

        const n1 = formatI64(42, sp, &buf);
        expect_eq(strEq(buf[0..n1], "+42"), true);

        const n2 = formatI64(0, sp, &buf);
        expect_eq(strEq(buf[0..n2], "+0"), true);

        const n3 = formatU64(0, IntFmtSpec.defaultSpec(), &buf);
        expect_eq(strEq(buf[0..n3], "0"), true);

        // i64::MIN = -9223372036854775808
        const i64_min: i64 = -9223372036854775807 - 1;
        const n4 = formatI64(i64_min, IntFmtSpec.defaultSpec(), &buf);
        expect_eq(strEq(buf[0..n4], "-9223372036854775808"), true);
    });

    test("6. @cap/powerfmt — Small buffer truncation boundary safety", () => {
        let small: [4]u8 = undefined;
        const n1 = formatWithCommas(1234567, &small);
        expect_eq(n1, 4);
        expect_eq(strEq(small[0..n1], "1,23"), true);

        const empty_out: []u8 = small[0..0];
        expect_eq(formatI64(12345, IntFmtSpec.defaultSpec(), empty_out), 0);
        expect_eq(formatU64(12345, IntFmtSpec.defaultSpec(), empty_out), 0);
    });
}
