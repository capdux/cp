import { test, expect_eq } from "cap:test";
import {
    diffPaths,
    diffUtf8Paths,
    normalizePath,
    hasSameRoot,
} from "../../packages/pathdiff/src/index.cp";

fn main() {
    test("1. pathdiff — sibling & nested descendant paths", () => {
        let buf: [128]u8 = undefined;
        const n1 = diffPaths("/foo/bar/baz", "/foo/bar", buf[0..128]) ?? 0;
        expect_eq(buf[0..n1] as string, "baz");

        const n2 = diffPaths("/foo/bar/a/b", "/foo/bar", buf[0..128]) ?? 0;
        expect_eq(buf[0..n2] as string, "a/b");
    });

    test("2. pathdiff — upward traversal with '..' hops", () => {
        let buf: [128]u8 = undefined;
        const n = diffPaths("/foo/qux", "/foo/bar/baz", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "../../qux");
    });

    test("3. pathdiff — identical paths return '.'", () => {
        let buf: [128]u8 = undefined;
        const n = diffPaths("/a/b/c/", "/a/b/c", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, ".");

        const nRoot = diffPaths("/", "///", buf[0..128]) ?? 0;
        expect_eq(buf[0..nRoot] as string, ".");
    });

    test("4. pathdiff — normalizes redundant '.', '..' and multi-slashes before diffing", () => {
        let buf: [128]u8 = undefined;
        const n = diffPaths("/a/b/../c/./d///", "/a//c/", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "d");
    });

    test("5. pathdiff — relative-to-relative diffing", () => {
        let buf: [128]u8 = undefined;
        const n = diffPaths("src/lib/mod.cp", "src/bin", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "../lib/mod.cp");
    });

    test("6. pathdiff — returns null when mixing absolute and relative paths", () => {
        let buf: [128]u8 = undefined;
        expect_eq(diffPaths("/abs/path", "rel/base", buf[0..128]) == null, true);
        expect_eq(diffPaths("rel/path", "/abs/base", buf[0..128]) == null, true);
        expect_eq(hasSameRoot("/abs", "rel"), false);
        expect_eq(hasSameRoot("/a", "/b"), true);
    });

    test("7. pathdiff — Windows drive prefix matching & root mismatch", () => {
        let buf: [128]u8 = undefined;
        const n = diffPaths("C:\\Users\\alice\\docs", "c:/Users/bob", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "../alice/docs");

        expect_eq(diffPaths("C:/a/b", "D:/a/b", buf[0..128]) == null, true);
        expect_eq(hasSameRoot("C:/a", "c:\\b"), true);
        expect_eq(hasSameRoot("C:/a", "D:/a"), false);
    });

    test("8. pathdiff — diffUtf8Paths validates UTF-8 and handles Unicode paths", () => {
        let buf: [128]u8 = undefined;
        const n = diffUtf8Paths("/项目/源码/main.cp", "/项目/测试", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "../源码/main.cp");

        // Invalid UTF-8 byte sequence returns null
        const bad_bytes: [3]u8 = [0x2F, 0xFF, 0xFE];
        expect_eq(diffUtf8Paths(bad_bytes[0..3] as string, "/a", buf[0..128]) == null, true);
    });

    test("9. pathdiff — normalizePath and tiny buffer protection", () => {
        let buf: [128]u8 = undefined;
        const n = normalizePath("/a/b/../c//./d/", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "/a/c/d");

        const nEmpty = normalizePath("./", buf[0..128]) ?? 0;
        expect_eq(buf[0..nEmpty] as string, ".");

        let tiny: [2]u8 = undefined;
        expect_eq(diffPaths("/foo/bar/baz", "/foo", tiny[0..2]) == null, true);
    });
}
