import { test, expect_eq } from "cap:test";
import { cleanPath, cleanJoin, isClean } from "../../packages/path-clean/src/index.cp";

fn main() {
    test("empty, root, and single dot normalization", () => {
        let buf: [64]u8 = undefined;
        let n = cleanPath("", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "."), true);

        n = cleanPath(".", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "."), true);

        n = cleanPath("./", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "."), true);

        n = cleanPath("/", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/"), true);

        n = cleanPath("///", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/"), true);
    });

    test("collapse multiple slashes and strip trailing slashes", () => {
        let buf: [64]u8 = undefined;
        let n = cleanPath("a//b///c/", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "a/b/c"), true);

        n = cleanPath("//a//b/./c///", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/a/b/c"), true);

        n = cleanPath("foo/./bar/./", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "foo/bar"), true);
    });

    test("resolve parent references and clamp absolute root", () => {
        let buf: [64]u8 = undefined;
        let n = cleanPath("/a/b/../c", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/a/c"), true);

        n = cleanPath("/../..", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/"), true);

        n = cleanPath("/../../a/b/..", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/a"), true);

        n = cleanPath("a/b/../..", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "."), true);
    });

    test("preserve leading parent references for relative paths", () => {
        let buf: [64]u8 = undefined;
        let n = cleanPath("..", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], ".."), true);

        n = cleanPath("../../a/b/../c", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "../../a/c"), true);

        n = cleanPath("a/../../b", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "../b"), true);

        n = cleanPath("..a/.b/.../c", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "..a/.b/.../c"), true);
    });

    test("cleanJoin combines and normalizes two path segments", () => {
        let buf: [64]u8 = undefined;
        let n = cleanJoin("/usr/local", "../bin/./cp/", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "/usr/bin/cp"), true);

        n = cleanJoin("", "a/./b/", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "a/b"), true);

        n = cleanJoin("a/b/", "", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "a/b"), true);

        n = cleanJoin("", "", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "."), true);

        n = cleanJoin("a/b", "../../..", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], ".."), true);
    });

    test("isClean accurately identifies canonical vs dirty paths", () => {
        expect_eq(isClean(""), false);
        expect_eq(isClean("."), true);
        expect_eq(isClean("/"), true);
        expect_eq(isClean(".."), true);
        expect_eq(isClean("../../a/b"), true);
        expect_eq(isClean("/a/b/c"), true);
        expect_eq(isClean("..a/.b/..."), true);

        expect_eq(isClean("/a/b/"), false);
        expect_eq(isClean("a//b"), false);
        expect_eq(isClean("./a"), false);
        expect_eq(isClean("a/."), false);
        expect_eq(isClean("a/.."), false);
        expect_eq(isClean("/.."), false);
        expect_eq(isClean("../a/.."), false);
    });
}
