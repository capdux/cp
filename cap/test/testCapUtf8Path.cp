// cap/test/testCapUtf8Path.cp
// Comprehensive unit tests for @cap/utf8-path

import { test, expect_eq } from "cap:test";
import {
    isAbsolute,
    fileName,
    fileStem,
    extension,
    parentDir,
    normalizePath,
    joinPath,
    safeJoin,
    relativePath,
    withExtension,
} from "../../packages/camino/src/index.cp";

fn main() {
    test("1. @cap/utf8-path — isAbsolute, fileName, fileStem & extension (including dotfiles)", () => {
        expect_eq(isAbsolute("/usr/local/bin"), true);
        expect_eq(isAbsolute("C:\\Windows\\System32"), true);
        expect_eq(isAbsolute("D:/projects/cp"), true);
        expect_eq(isAbsolute("src/index.cp"), false);

        expect_eq(fileName("/a/b/archive.tar.gz") ?? "", "archive.tar.gz");
        expect_eq(fileStem("/a/b/archive.tar.gz") ?? "", "archive.tar");
        expect_eq(extension("/a/b/archive.tar.gz") ?? "", "gz");

        expect_eq(fileName("/repo/.gitignore") ?? "", ".gitignore");
        expect_eq(fileStem("/repo/.gitignore") ?? "", ".gitignore");
        expect_eq(extension("/repo/.gitignore") == null, true);

        expect_eq(fileName("/") == null, true);
        expect_eq(fileName("/a/b/..") == null, true);
    });

    test("2. @cap/utf8-path — parentDir edge cases", () => {
        expect_eq(parentDir("/a/b/c.cp") ?? "", "/a/b");
        expect_eq(parentDir("/a/b/") ?? "", "/a");
        expect_eq(parentDir("/only") ?? "", "/");
        expect_eq(parentDir("/") == null, true);
        expect_eq(parentDir("file.cp") ?? "X", "");
    });

    test("3. @cap/utf8-path — normalizePath & joinPath (POSIX & Windows separators)", () => {
        let b1: [256]u8 = undefined;
        expect_eq(normalizePath("/a//b/./c/../d/", b1[0..256]), "/a/b/d");

        let b2: [256]u8 = undefined;
        expect_eq(normalizePath("src\\sub\\..\\index.cp", b2[0..256]), "src/index.cp");

        let b3: [256]u8 = undefined;
        expect_eq(normalizePath("/../../etc/hosts", b3[0..256]), "/etc/hosts");

        let b4: [256]u8 = undefined;
        expect_eq(joinPath("/srv/app", "static/./js/../css/app.css", b4[0..256]), "/srv/app/static/css/app.css");
    });

    test("4. @cap/utf8-path — safeJoin prevents directory traversal & absolute override", () => {
        let b1: [256]u8 = undefined;
        expect_eq(safeJoin("/srv/uploads", "2026/a/../b.png", b1[0..256]) ?? "", "/srv/uploads/2026/b.png");

        let b2: [256]u8 = undefined;
        expect_eq(safeJoin("/srv/uploads", "../../etc/passwd", b2[0..256]) == null, true);

        let b3: [256]u8 = undefined;
        expect_eq(safeJoin("/srv/uploads", "a/../../secret.txt", b3[0..256]) == null, true);

        let b4: [256]u8 = undefined;
        expect_eq(safeJoin("/srv/uploads", "/etc/passwd", b4[0..256]) == null, true);

        let b5: [256]u8 = undefined;
        expect_eq(safeJoin("/srv/uploads", "C:\\Windows\\win.ini", b5[0..256]) == null, true);
    });

    test("5. @cap/utf8-path — relativePath minimal traversal calculation", () => {
        let b1: [256]u8 = undefined;
        expect_eq(relativePath("/a/b/c", "/a/d/e", b1[0..256]), "../../d/e");

        let b2: [256]u8 = undefined;
        expect_eq(relativePath("/a/b", "/a/b/c/d.cp", b2[0..256]), "c/d.cp");

        let b3: [256]u8 = undefined;
        expect_eq(relativePath("/a/b", "/a/b", b3[0..256]), ".");
    });

    test("6. @cap/utf8-path — withExtension replace, dot-prefix & remove", () => {
        let b1: [256]u8 = undefined;
        expect_eq(withExtension("/src/main.cp", "zig", b1[0..256]), "/src/main.zig");

        let b2: [256]u8 = undefined;
        expect_eq(withExtension("/src/main.cp", ".o", b2[0..256]), "/src/main.o");

        let b3: [256]u8 = undefined;
        expect_eq(withExtension("/src/main.cp", "", b3[0..256]), "/src/main");

        let b4: [256]u8 = undefined;
        expect_eq(withExtension("README", "md", b4[0..256]), "README.md");
    });
}
