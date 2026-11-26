import { test, expect_eq } from "cap:test";
import {
    KIB,
    MIB,
    GIB,
    KB,
    MB,
    GB,
    parse,
    parseBytes,
    formatBytes,
    formatIec,
    formatSi,
} from "../../packages/@cap/bytes/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
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
    test("1. @cap/bytes — npm-compatible binary default parsing", () => {
        expect_eq(parse("0"), 0);
        expect_eq(parse("512B"), 512);
        expect_eq(parse("1KB"), KIB as i64);
        expect_eq(parse("1.5 MB"), (MIB + MIB / 2) as i64);
        expect_eq(parse("2gb"), (2 * GIB) as i64);
    });

    test("2. @cap/bytes — SI decimal vs explicit IEC binary units", () => {
        expect_eq(parseBytes("1KB", false), KB as i64);
        expect_eq(parseBytes("2.5 MB", false), (2 * MB + MB / 2) as i64);
        expect_eq(parseBytes("1GB", false), GB as i64);
        expect_eq(parseBytes("1KiB", false), KIB as i64);
        expect_eq(parseBytes("1MiB", false), MIB as i64);
    });

    test("3. @cap/bytes — formatIec formatting across scales", () => {
        let buf: [64]u8 = undefined;
        let n0: usize = formatIec(0, buf[0..]);
        expect_eq(strEq(buf[0..n0], "0 B"), true);

        let n1: usize = formatIec(1536, buf[0..]);
        expect_eq(strEq(buf[0..n1], "1.50 KiB"), true);

        let n2: usize = formatIec(MIB * 5, buf[0..]);
        expect_eq(strEq(buf[0..n2], "5.00 MiB"), true);

        let n3: usize = formatIec(GIB * 2 + MIB * 512, buf[0..]);
        expect_eq(strEq(buf[0..n3], "2.50 GiB"), true);
    });

    test("4. @cap/bytes — formatSi and custom decimal places", () => {
        let buf: [64]u8 = undefined;
        let n1: usize = formatSi(1500, buf[0..]);
        expect_eq(strEq(buf[0..n1], "1.50 KB"), true);

        let n2: usize = formatBytes(2500000, false, 1, buf[0..]);
        expect_eq(strEq(buf[0..n2], "2.5 MB"), true);

        let n3: usize = formatBytes(3000000000, false, 0, buf[0..]);
        expect_eq(strEq(buf[0..n3], "3 GB"), true);
    });

    test("5. @cap/bytes — invalid inputs rejected cleanly", () => {
        expect_eq(parse(""), -1);
        expect_eq(parse("   "), -1);
        expect_eq(parse("KB"), -1);
        expect_eq(parse("100 XB"), -1);
        expect_eq(parse("-50 MB"), -1);
    });

    test("6. @cap/bytes — small output buffer protection", () => {
        let tiny: [3]u8 = undefined;
        expect_eq(formatIec(1536, tiny[0..]), 0);
        expect_eq(formatSi(500, tiny[0..]), 0);
    });
}
