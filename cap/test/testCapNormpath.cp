// cap/test/testCapNormpath.cp
// Comprehensive unit tests for @cap/normpath

import { test, expect_eq } from "cap:test";
import {
    normalizePosix,
    normalizeWindows,
    joinNormalized,
    joinNormalizedWindows,
    hasParentTraversal,
    escapesRoot,
    isNormalized,
    isVerbatimWindows,
    stripVerbatimPrefix,
} from "../../packages/@cap/normpath/src/index.cp";

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
    test("1. @cap/normpath — POSIX absolute paths, collapsing slashes & root clamping", () => {
        let buf: [128]u8 = undefined;

        const n1 = normalizePosix("/", buf[0..]);
        expect_eq(strEq(buf[0..n1], "/"), true);

        const n2 = normalizePosix("///a///b//c///", buf[0..]);
        expect_eq(strEq(buf[0..n2], "/a/b/c"), true);

        const n3 = normalizePosix("/a/./b/../c/./d/..", buf[0..]);
        expect_eq(strEq(buf[0..n3], "/a/c"), true);

        const n4 = normalizePosix("/../../a/../../b", buf[0..]);
        expect_eq(strEq(buf[0..n4], "/b"), true);

        const n5 = normalizePosix("/../../../..", buf[0..]);
        expect_eq(strEq(buf[0..n5], "/"), true);
    });

    test("2. @cap/normpath — POSIX relative paths, leading .. & empty path", () => {
        let buf: [128]u8 = undefined;

        const n1 = normalizePosix("", buf[0..]);
        expect_eq(strEq(buf[0..n1], "."), true);

        const n2 = normalizePosix("././.", buf[0..]);
        expect_eq(strEq(buf[0..n2], "."), true);

        const n3 = normalizePosix("a/b/../..", buf[0..]);
        expect_eq(strEq(buf[0..n3], "."), true);

        const n4 = normalizePosix("../a/../b/c/..", buf[0..]);
        expect_eq(strEq(buf[0..n4], "../b"), true);

        const n5 = normalizePosix("a/../../b/../c", buf[0..]);
        expect_eq(strEq(buf[0..n5], "../c"), true);

        const n6 = normalizePosix("../../..", buf[0..]);
        expect_eq(strEq(buf[0..n6], "../../.."), true);
    });

    test("3. @cap/normpath — Windows drive roots, drive-relative & separator unification", () => {
        let buf: [128]u8 = undefined;

        const n1 = normalizeWindows("c:/Users/./dev/../admin//docs/", buf[0..]);
        expect_eq(strEq(buf[0..n1], "C:\\Users\\admin\\docs"), true);

        const n2 = normalizeWindows("D:\\..\\..\\foo\\bar\\..", buf[0..]);
        expect_eq(strEq(buf[0..n2], "D:\\foo"), true);

        const n3 = normalizeWindows("C:\\", buf[0..]);
        expect_eq(strEq(buf[0..n3], "C:\\"), true);

        const n4 = normalizeWindows("e:foo/bar/../../baz", buf[0..]);
        expect_eq(strEq(buf[0..n4], "E:baz"), true);

        const n5 = normalizeWindows("a/b\\..\\c", buf[0..]);
        expect_eq(strEq(buf[0..n5], "a\\c"), true);

        const n6 = normalizeWindows("", buf[0..]);
        expect_eq(strEq(buf[0..n6], "."), true);
    });

    test("4. @cap/normpath — Windows UNC & verbatim \\\\?\\ prefixes", () => {
        let buf: [128]u8 = undefined;

        const n1 = normalizeWindows("\\\\server\\share\\a\\..\\b\\.", buf[0..]);
        expect_eq(strEq(buf[0..n1], "\\\\server\\share\\b"), true);

        const n2 = normalizeWindows("//filesrv/public/../../secret", buf[0..]);
        expect_eq(strEq(buf[0..n2], "\\\\filesrv\\public\\secret"), true);

        const n3 = normalizeWindows("\\\\?\\c:\\foo\\bar\\..\\baz", buf[0..]);
        expect_eq(strEq(buf[0..n3], "\\\\?\\C:\\foo\\baz"), true);

        const n4 = normalizeWindows("\\\\?\\UNC\\srv\\shr\\x\\..\\y", buf[0..]);
        expect_eq(strEq(buf[0..n4], "\\\\?\\UNC\\srv\\shr\\y"), true);

        expect_eq(isVerbatimWindows("\\\\?\\C:\\test"), true);
        expect_eq(isVerbatimWindows("C:\\test"), false);
        expect_eq(strEq(stripVerbatimPrefix("\\\\?\\C:\\test"), "C:\\test"), true);
        expect_eq(strEq(stripVerbatimPrefix("C:\\test"), "C:\\test"), true);
    });

    test("5. @cap/normpath — joinNormalized & joinNormalizedWindows", () => {
        let buf: [128]u8 = undefined;

        const n1 = joinNormalized("/usr/local/lib", "../bin/./cap", buf[0..]);
        expect_eq(strEq(buf[0..n1], "/usr/local/bin/cap"), true);

        const n2 = joinNormalized("/usr/local", "/etc/hosts", buf[0..]);
        expect_eq(strEq(buf[0..n2], "/etc/hosts"), true);

        const n3 = joinNormalized("src/core", "../../tests/unit", buf[0..]);
        expect_eq(strEq(buf[0..n3], "tests/unit"), true);

        const n4 = joinNormalized("", "a/../b", buf[0..]);
        expect_eq(strEq(buf[0..n4], "b"), true);

        const n5 = joinNormalizedWindows("C:\\work\\proj", "..\\other\\file.cp", buf[0..]);
        expect_eq(strEq(buf[0..n5], "C:\\work\\other\\file.cp"), true);

        const n6 = joinNormalizedWindows("C:\\work", "D:\\abs\\path", buf[0..]);
        expect_eq(strEq(buf[0..n6], "D:\\abs\\path"), true);
    });

    test("6. @cap/normpath — hasParentTraversal, escapesRoot & isNormalized predicates", () => {
        expect_eq(hasParentTraversal("../etc/passwd"), true);
        expect_eq(hasParentTraversal("a/b/../c"), true);
        expect_eq(hasParentTraversal("a\\..\\b"), true);
        expect_eq(hasParentTraversal("..hidden/file..txt"), false);
        expect_eq(hasParentTraversal("/a/b/c"), false);

        expect_eq(escapesRoot("a/../../b"), true);
        expect_eq(escapesRoot("../x"), true);
        expect_eq(escapesRoot("a/b/.."), false);
        expect_eq(escapesRoot("/../../etc"), false);

        expect_eq(isNormalized("/"), true);
        expect_eq(isNormalized("."), true);
        expect_eq(isNormalized("/a/b/c"), true);
        expect_eq(isNormalized("../a/b"), true);
        expect_eq(isNormalized(""), false);
        expect_eq(isNormalized("/a//b"), false);
        expect_eq(isNormalized("/a/b/"), false);
        expect_eq(isNormalized("a/./b"), false);
        expect_eq(isNormalized("a/../b"), false);
        expect_eq(isNormalized("/../a"), false);
    });
}
