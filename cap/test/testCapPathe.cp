// cap/test/testCapPathe.cp
// Comprehensive unit tests for @cap/pathe

import { test, expect_eq } from "cap:test";
import {
    normalize,
    join,
    joinPath,
    resolve,
    dirname,
    basename,
    extname,
    isAbsolute,
    relative,
} from "../../packages/pathe/src/index.cp";

fn main() {
    test("1. @cap/pathe — normalize POSIX, Windows backslashes, drive letters, and protocols", () => {
        let out: [256]u8 = undefined;

        const n1 = normalize("C:\\Users\\dev\\..\\app\\\\index.cp", out[0..]);
        expect_eq(out[0..n1], "C:/Users/app/index.cp");

        const n2 = normalize("/a//b/./c/../d/", out[0..]);
        expect_eq(out[0..n2], "/a/b/d/");

        const n3 = normalize("https://example.com/a/b/../c", out[0..]);
        expect_eq(out[0..n3], "https://example.com/a/c");

        const n4 = normalize("../../a/./b", out[0..]);
        expect_eq(out[0..n4], "../../a/b");

        const n5 = normalize("", out[0..]);
        expect_eq(out[0..n5], ".");
    });

    test("2. @cap/pathe — join combines and normalizes paths", () => {
        let out: [256]u8 = undefined;

        const n1 = joinPath("/usr/local", "bin/../lib/cp", out[0..]);
        expect_eq(out[0..n1], "/usr/local/lib/cp");

        const n2 = joinPath("C:\\work\\", "src\\main.cp", out[0..]);
        expect_eq(out[0..n2], "C:/work/src/main.cp");

        const n3 = joinPath("", "", out[0..]);
        expect_eq(out[0..n3], ".");
    });

    test("3. @cap/pathe — resolve handles relative and absolute overrides", () => {
        let out: [256]u8 = undefined;

        const n1 = resolve("/a/b/c", "../d/e/", out[0..]);
        expect_eq(out[0..n1], "/a/b/d/e");

        const n2 = resolve("/a/b/c", "/etc/hosts", out[0..]);
        expect_eq(out[0..n2], "/etc/hosts");

        const n3 = resolve("/a/b", "C:\\Windows\\System32\\", out[0..]);
        expect_eq(out[0..n3], "C:/Windows/System32");
    });

    test("4. @cap/pathe — dirname, basename, and extname", () => {
        expect_eq(dirname("/a/b/c.txt"), "/a/b");
        expect_eq(dirname("C:\\dir\\file.cp"), "C:\\dir");
        expect_eq(dirname("/only"), "/");
        expect_eq(dirname("plain.txt"), ".");

        expect_eq(basename("/a/b/c.txt"), "c.txt");
        expect_eq(basename("/a/b/dir/"), "dir");
        expect_eq(basename("C:\\a\\b.tar.gz"), "b.tar.gz");

        expect_eq(extname("/a/b/c.txt"), ".txt");
        expect_eq(extname("archive.tar.gz"), ".gz");
        expect_eq(extname(".gitignore"), "");
        expect_eq(extname("Makefile"), "");
    });

    test("5. @cap/pathe — isAbsolute across POSIX, Windows, and URLs", () => {
        expect_eq(isAbsolute("/usr/bin"), true);
        expect_eq(isAbsolute("\\Windows"), true);
        expect_eq(isAbsolute("C:/Users"), true);
        expect_eq(isAbsolute("D:\\Data"), true);
        expect_eq(isAbsolute("file:///tmp/a"), true);
        expect_eq(isAbsolute("src/index.cp"), false);
        expect_eq(isAbsolute("./a"), false);
        expect_eq(isAbsolute(""), false);
    });

    test("6. @cap/pathe — relative path computation", () => {
        let out: [256]u8 = undefined;

        const n1 = relative("/data/orandea/test/aaa", "/data/orandea/impl/bbb", out[0..]);
        expect_eq(out[0..n1], "../../impl/bbb");

        const n2 = relative("/a/b/c", "/a/b/c", out[0..]);
        expect_eq(n2, 0 as usize);

        const n3 = relative("C:\\a\\b", "C:/a/c/d.cp", out[0..]);
        expect_eq(out[0..n3], "../c/d.cp");
    });
}
