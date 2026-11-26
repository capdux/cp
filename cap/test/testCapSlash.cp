// cap/test/testCapSlash.cp
// Comprehensive unit tests for @cap/slash

import { test, expect_eq } from "cap:test";
import {
    slash,
    isExtendedLengthPath,
    toBackslash,
    countSeparators,
} from "../../packages/@cap/slash/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/slash — Standard Windows backslash path conversion", () => {
        let buf: [64]u8 = undefined;
        const n = slash("C:\\Users\\dev\\src\\index.cp", buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "C:/Users/dev/src/index.cp"), true);
    });

    test("2. @cap/slash — Mixed separators normalize cleanly to POSIX '/'", () => {
        let buf: [64]u8 = undefined;
        const n = slash("foo\\bar/baz\\qux.txt", buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "foo/bar/baz/qux.txt"), true);
    });

    test("3. @cap/slash — Windows extended-length prefix \\\\?\\ preserved verbatim", () => {
        const extPath = "\\\\?\\C:\\foo\\bar\\baz.cp";
        expect_eq(isExtendedLengthPath(extPath), true);
        expect_eq(isExtendedLengthPath("C:\\foo\\bar"), false);

        let buf: [64]u8 = undefined;
        const n = slash(extPath, buf[0..64]);
        expect_eq(sliceEq(buf[0..n], extPath), true);
    });

    test("4. @cap/slash — Pure POSIX path passes through unchanged", () => {
        const posix = "/usr/local/bin/cpc";
        let buf: [64]u8 = undefined;
        const n = slash(posix, buf[0..64]);
        expect_eq(sliceEq(buf[0..n], posix), true);
    });

    test("5. @cap/slash — toBackslash reverse conversion & countSeparators", () => {
        let buf: [64]u8 = undefined;
        const n = toBackslash("a/b/c/d.cp", buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "a\\b\\c\\d.cp"), true);

        const extPath = "\\\\?\\C:\\a\\b";
        const ne = toBackslash(extPath, buf[0..64]);
        expect_eq(sliceEq(buf[0..ne], extPath), true);

        expect_eq(countSeparators("a\\b/c\\d/e"), 4 as usize);
        expect_eq(countSeparators("filename.cp"), 0 as usize);
    });

    test("6. @cap/slash — Empty string & small buffer truncation safety", () => {
        let buf: [16]u8 = undefined;
        expect_eq(slash("", buf[0..16]), 0 as usize);
        expect_eq(toBackslash("", buf[0..16]), 0 as usize);

        let tiny: [5]u8 = undefined;
        const n = slash("a\\b\\c\\d", tiny[0..5]);
        expect_eq(n, 5 as usize);
        expect_eq(sliceEq(tiny[0..5], "a/b/c"), true);
    });
}
