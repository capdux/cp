import { test, expect_eq } from "cap:test";
import { diffPaths } from "../../packages/@cap/pathdiff/src/index.cp";

fn main() {
    test("1. @cap/pathdiff — sibling & nested descendant paths", () => {
        let buf: [128]u8 = [0; 128];
        let n1 = diffPaths("/foo/bar/baz", "/foo/bar", buf[0..128]) ?? 0;
        expect_eq(buf[0..n1] as string, "baz");

        let n2 = diffPaths("/foo/bar/a/b", "/foo/bar", buf[0..128]) ?? 0;
        expect_eq(buf[0..n2] as string, "a/b");
    });

    test("2. @cap/pathdiff — upward traversal with '..' hops", () => {
        let buf: [128]u8 = [0; 128];
        let n = diffPaths("/foo/qux", "/foo/bar/baz", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "../../qux");
    });

    test("3. @cap/pathdiff — identical paths return '.'", () => {
        let buf: [128]u8 = [0; 128];
        let n = diffPaths("/a/b/c/", "/a/b/c", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, ".");
    });

    test("4. @cap/pathdiff — normalizes redundant '.' and '..' segments before diffing", () => {
        let buf: [128]u8 = [0; 128];
        let n = diffPaths("/a/b/../c/./d", "/a/c", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "d");
    });

    test("5. @cap/pathdiff — relative-to-relative diffing", () => {
        let buf: [128]u8 = [0; 128];
        let n = diffPaths("src/lib/mod.cp", "src/bin", buf[0..128]) ?? 0;
        expect_eq(buf[0..n] as string, "../lib/mod.cp");
    });

    test("6. @cap/pathdiff — returns null when mixing absolute and relative paths", () => {
        let buf: [128]u8 = [0; 128];
        expect_eq(diffPaths("/abs/path", "rel/base", buf[0..128]) == null, true);
        expect_eq(diffPaths("rel/path", "/abs/base", buf[0..128]) == null, true);
    });
}
