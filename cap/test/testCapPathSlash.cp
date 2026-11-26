// cap/test/testCapPathSlash.cp
// Comprehensive unit tests for @cap/path-slash

import { test, expect_eq } from "cap:test";
import {
    toSlash,
    toSlashLossy,
    fromSlash,
    hasBackslash,
    hasForwardSlash,
    trimTrailingSlashes,
    joinSlash,
} from "../../packages/@cap/path-slash/src/index.cp";

fn strEq(a: string, b: string): bool {
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
    test("1. @cap/path-slash — toSlash on relative, drive, UNC & verbatim paths", () => {
        let buf: [128]u8 = undefined;

        const n1 = toSlash("a\\b\\c.cp", buf[0..]);
        expect_eq(strEq(buf[0..n1], "a/b/c.cp"), true);

        const n2 = toSlash("C:\\Users\\dev\\proj", buf[0..]);
        expect_eq(strEq(buf[0..n2], "C:/Users/dev/proj"), true);

        const n3 = toSlash("\\\\server\\share\\dir\\file", buf[0..]);
        expect_eq(strEq(buf[0..n3], "//server/share/dir/file"), true);

        const n4 = toSlash("\\\\?\\C:\\long\\path", buf[0..]);
        expect_eq(strEq(buf[0..n4], "//?/C:/long/path"), true);

        const n5 = toSlash("", buf[0..]);
        expect_eq(n5, 0 as usize);
    });

    test("2. @cap/path-slash — fromSlash roundtrip & Windows conversion", () => {
        let buf: [128]u8 = undefined;

        const n1 = fromSlash("a/b/c.cp", buf[0..]);
        expect_eq(strEq(buf[0..n1], "a\\b\\c.cp"), true);

        const n2 = fromSlash("C:/Users/dev/proj", buf[0..]);
        expect_eq(strEq(buf[0..n2], "C:\\Users\\dev\\proj"), true);

        const n3 = fromSlash("//server/share/dir", buf[0..]);
        expect_eq(strEq(buf[0..n3], "\\\\server\\share\\dir"), true);

        const n4 = fromSlash("already\\win\\path", buf[0..]);
        expect_eq(strEq(buf[0..n4], "already\\win\\path"), true);
    });

    test("3. @cap/path-slash — toSlashLossy preserves valid UTF-8 & replaces invalid bytes", () => {
        let buf: [128]u8 = undefined;

        const n1 = toSlashLossy("目录\\αβ\\file.txt", buf[0..]);
        expect_eq(strEq(buf[0..n1], "目录/αβ/file.txt"), true);

        // "a\" + invalid 0xFF + "b" -> "a/" + U+FFFD (EF BF BD) + "b"
        let raw: [4]u8 = [ 0x61 as u8, 0x5C as u8, 0xFF as u8, 0x62 as u8 ];
        const n2 = toSlashLossy(raw[0..], buf[0..]);
        expect_eq(n2, 6 as usize);
        expect_eq(buf[0], 0x61 as u8);
        expect_eq(buf[1], 0x2F as u8);
        expect_eq(buf[2], 0xEF as u8);
        expect_eq(buf[3], 0xBF as u8);
        expect_eq(buf[4], 0xBD as u8);
        expect_eq(buf[5], 0x62 as u8);
    });

    test("4. @cap/path-slash — hasBackslash, hasForwardSlash & trimTrailingSlashes", () => {
        expect_eq(hasBackslash("a\\b"), true);
        expect_eq(hasBackslash("a/b/c"), false);
        expect_eq(hasBackslash(""), false);

        expect_eq(hasForwardSlash("a/b"), true);
        expect_eq(hasForwardSlash("a\\b"), false);

        expect_eq(strEq(trimTrailingSlashes("foo/bar///"), "foo/bar"), true);
        expect_eq(strEq(trimTrailingSlashes("foo\\bar\\\\"), "foo\\bar"), true);
        expect_eq(strEq(trimTrailingSlashes("/"), "/"), true);
        expect_eq(strEq(trimTrailingSlashes("C:\\"), "C:\\"), true);
    });

    test("5. @cap/path-slash — joinSlash across mixed separators, roots & empty inputs", () => {
        let buf: [128]u8 = undefined;

        const n1 = joinSlash("src\\core", "net\\tcp.cp", buf[0..]);
        expect_eq(strEq(buf[0..n1], "src/core/net/tcp.cp"), true);

        const n2 = joinSlash("src\\core\\", "net/tcp.cp", buf[0..]);
        expect_eq(strEq(buf[0..n2], "src/core/net/tcp.cp"), true);

        const n3 = joinSlash("/", "etc\\hosts", buf[0..]);
        expect_eq(strEq(buf[0..n3], "/etc/hosts"), true);

        const n4 = joinSlash("C:\\", "Windows\\System32", buf[0..]);
        expect_eq(strEq(buf[0..n4], "C:/Windows/System32"), true);

        const n5 = joinSlash("base/dir", "/abs/path", buf[0..]);
        expect_eq(strEq(buf[0..n5], "/abs/path"), true);

        const n6 = joinSlash("base/dir", "D:\\other", buf[0..]);
        expect_eq(strEq(buf[0..n6], "D:/other"), true);

        const n7 = joinSlash("", "rel\\path", buf[0..]);
        expect_eq(strEq(buf[0..n7], "rel/path"), true);
    });

    test("6. @cap/path-slash — Output buffer truncation safety", () => {
        let small: [5]u8 = undefined;
        const n1 = toSlash("a\\b\\c\\d\\e", small[0..]);
        expect_eq(n1, 5 as usize);
        expect_eq(strEq(small[0..n1], "a/b/c"), true);

        const n2 = fromSlash("1/2/3/4/5", small[0..]);
        expect_eq(n2, 5 as usize);
        expect_eq(strEq(small[0..n2], "1\\2\\3"), true);
    });
}
