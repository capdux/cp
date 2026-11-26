import { test, expect_eq } from "cap:test";
import {
    normalizePath,
    normalizePathDefault,
    hasBackslash,
    isUncPath,
} from "../../packages/@cap/normalize-path/src/index.cp";

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
    test("1. @cap/normalize-path — Windows backslashes convert to POSIX slashes", () => {
        let buf: [64]u8 = undefined;
        expect_eq(hasBackslash("foo\\bar\\baz"), true);
        expect_eq(hasBackslash("foo/bar/baz"), false);

        const n1 = normalizePathDefault("foo\\bar\\baz", &buf);
        expect_eq(strEq(buf[0..n1], "foo/bar/baz"), true);

        const n2 = normalizePathDefault("\\foo\\bar\\baz.txt", &buf);
        expect_eq(strEq(buf[0..n2], "/foo/bar/baz.txt"), true);

        const n3 = normalizePathDefault("C:\\Users\\dev\\app.cp", &buf);
        expect_eq(strEq(buf[0..n3], "C:/Users/dev/app.cp"), true);
    });

    test("2. @cap/normalize-path — Consecutive duplicate slashes collapse cleanly", () => {
        let buf: [64]u8 = undefined;
        const n1 = normalizePathDefault("a///b\\\\\\c//d", &buf);
        expect_eq(strEq(buf[0..n1], "a/b/c/d"), true);

        const n2 = normalizePathDefault("///foo//bar///", &buf);
        expect_eq(strEq(buf[0..n2], "/foo/bar"), true);

        const n3 = normalizePathDefault("\\\\\\foo\\\\bar", &buf);
        expect_eq(strEq(buf[0..n3], "/foo/bar"), true);
    });

    test("3. @cap/normalize-path — Trailing slash stripping vs strip_trailing=false", () => {
        let buf: [64]u8 = undefined;
        const n1 = normalizePath("foo/bar/baz/", true, &buf);
        expect_eq(strEq(buf[0..n1], "foo/bar/baz"), true);

        const n2 = normalizePath("foo/bar/baz///", false, &buf);
        expect_eq(strEq(buf[0..n2], "foo/bar/baz/"), true);

        const n3 = normalizePath("foo\\bar\\", false, &buf);
        expect_eq(strEq(buf[0..n3], "foo/bar/"), true);
    });

    test("4. @cap/normalize-path — Root '/' '\\' & drive root 'C:/' are preserved", () => {
        let buf: [64]u8 = undefined;
        const n1 = normalizePathDefault("/", &buf);
        expect_eq(strEq(buf[0..n1], "/"), true);

        const n2 = normalizePathDefault("\\", &buf);
        expect_eq(strEq(buf[0..n2], "/"), true);

        const n3 = normalizePathDefault("///", &buf);
        expect_eq(strEq(buf[0..n3], "/"), true);

        const n4 = normalizePathDefault("C:\\", &buf);
        expect_eq(strEq(buf[0..n4], "C:/"), true);

        const n5 = normalizePathDefault("C:\\\\", &buf);
        expect_eq(strEq(buf[0..n5], "C:/"), true);
    });

    test("5. @cap/normalize-path — UNC paths & Windows device prefix preserve leading '//'", () => {
        let buf: [64]u8 = undefined;
        expect_eq(isUncPath("\\\\server\\share\\dir\\"), true);
        expect_eq(isUncPath("//server/share"), true);
        expect_eq(isUncPath("\\\\?\\C:\\Users\\a\\"), true);
        expect_eq(isUncPath("///"), false);
        expect_eq(isUncPath("/normal/path"), false);

        const n1 = normalizePathDefault("\\\\server\\share\\dir\\", &buf);
        expect_eq(strEq(buf[0..n1], "//server/share/dir"), true);

        const n2 = normalizePath("\\\\server\\share\\dir\\", false, &buf);
        expect_eq(strEq(buf[0..n2], "//server/share/dir/"), true);

        const n3 = normalizePathDefault("\\\\?\\C:\\Users\\test\\", &buf);
        expect_eq(strEq(buf[0..n3], "//?/C:/Users/test"), true);
    });

    test("6. @cap/normalize-path — Empty string & small buffer truncation boundaries", () => {
        let buf: [16]u8 = undefined;
        expect_eq(normalizePathDefault("", &buf), 0);
        expect_eq(hasBackslash(""), false);
        expect_eq(isUncPath(""), false);

        const empty_out: []u8 = buf[0..0];
        expect_eq(normalizePathDefault("foo/bar", empty_out), 0);

        let small: [5]u8 = undefined;
        const n_trunc = normalizePathDefault("alpha\\beta\\gamma", &small);
        expect_eq(n_trunc, 5);
        expect_eq(strEq(small[0..n_trunc], "alpha"), true);

        let tiny: [1]u8 = undefined;
        const n_unc_tiny = normalizePathDefault("\\\\server\\share", &tiny);
        expect_eq(n_unc_tiny, 1);
        expect_eq(tiny[0], 47);
    });
}
