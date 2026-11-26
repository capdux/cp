import { test, expect_eq } from "cap:test";
import {
    isPathInside,
    isPathInsideOrEqual,
} from "../../packages/@cap/is-path-inside/src/index.cp";

fn main() {
    test("1. @cap/is-path-inside — detects nested child paths", () => {
        expect_eq(isPathInside("/a/b/c", "/a/b"), true);
        expect_eq(isPathInside("/a/b/c/d.txt", "/a/b/"), true);
        expect_eq(isPathInside("a/b/c", "a/b"), true);
        expect_eq(isPathInside("/foo", "/"), true);
    });

    test("2. @cap/is-path-inside — rejects equal paths and sibling prefix collisions", () => {
        expect_eq(isPathInside("/a/b", "/a/b"), false);
        expect_eq(isPathInside("/a/b/", "/a/b"), false);
        expect_eq(isPathInside("/a/bc", "/a/b"), false);
        expect_eq(isPathInside("/a/b-extra/file", "/a/b"), false);
    });

    test("3. @cap/is-path-inside — blocks '..' path traversal escapes", () => {
        expect_eq(isPathInside("/a/b/../c", "/a/b"), false);
        expect_eq(isPathInside("/a/b/c/../../etc/passwd", "/a/b"), false);
        expect_eq(isPathInside("/a/b/./sub/../ok.txt", "/a/b"), true);
    });

    test("4. @cap/is-path-inside — normalizes Windows backslashes", () => {
        expect_eq(isPathInside("\\srv\\www\\index.html", "\\srv\\www"), true);
        expect_eq(isPathInside("\\srv\\www\\..\\secret", "\\srv\\www"), false);
    });

    test("5. @cap/is-path-inside — rejects empty or mismatched abs/rel paths", () => {
        expect_eq(isPathInside("", "/a"), false);
        expect_eq(isPathInside("/a/b", ""), false);
        expect_eq(isPathInside("a/b", "/a"), false);
        expect_eq(isPathInside("/a/b", "a"), false);
    });

    test("6. @cap/is-path-inside — isPathInsideOrEqual accepts identical normalized paths", () => {
        expect_eq(isPathInsideOrEqual("/a/b/.", "/a/b/"), true);
        expect_eq(isPathInsideOrEqual("/a/b/c", "/a/b"), true);
        expect_eq(isPathInsideOrEqual("/a/bc", "/a/b"), false);
    });
}
