import { test, expect_eq } from "cap:test";
import {
    parsePosix,
    parseWin32,
} from "../../packages/path-parse/src/index.cp";

fn main() {
    test("1. @cap/path-parse — POSIX absolute file path", () => {
        const p = parsePosix("/home/user/dir/file.txt");
        expect_eq(p.root, "/");
        expect_eq(p.dir, "/home/user/dir");
        expect_eq(p.base, "file.txt");
        expect_eq(p.ext, ".txt");
        expect_eq(p.name, "file");
    });

    test("2. @cap/path-parse — POSIX root and trailing slashes", () => {
        const p1 = parsePosix("/");
        expect_eq(p1.root, "/");
        expect_eq(p1.dir, "/");
        expect_eq(p1.base, "");

        const p2 = parsePosix("/foo/bar/");
        expect_eq(p2.dir, "/foo");
        expect_eq(p2.base, "bar");
    });

    test("3. @cap/path-parse — dotfiles and multi-extension files", () => {
        const p1 = parsePosix("/app/.gitignore");
        expect_eq(p1.base, ".gitignore");
        expect_eq(p1.name, ".gitignore");
        expect_eq(p1.ext, "");

        const p2 = parsePosix("archive.tar.gz");
        expect_eq(p2.dir, "");
        expect_eq(p2.name, "archive.tar");
        expect_eq(p2.ext, ".gz");
    });

    test("4. @cap/path-parse — Win32 drive absolute path", () => {
        const p = parseWin32("C:\\path\\dir\\index.html");
        expect_eq(p.root, "C:\\");
        expect_eq(p.dir, "C:\\path\\dir");
        expect_eq(p.base, "index.html");
        expect_eq(p.ext, ".html");
        expect_eq(p.name, "index");
    });

    test("5. @cap/path-parse — Win32 drive-relative and root-only paths", () => {
        const p1 = parseWin32("C:file.txt");
        expect_eq(p1.root, "C:");
        expect_eq(p1.dir, "C:");
        expect_eq(p1.base, "file.txt");

        const p2 = parseWin32("D:\\");
        expect_eq(p2.root, "D:\\");
        expect_eq(p2.dir, "D:\\");
        expect_eq(p2.base, "");
    });

    test("6. @cap/path-parse — empty path and parent '..'", () => {
        const p1 = parsePosix("");
        expect_eq(p1.base, "");
        expect_eq(p1.dir, "");

        const p2 = parsePosix("..");
        expect_eq(p2.name, "..");
        expect_eq(p2.ext, "");
    });
}
