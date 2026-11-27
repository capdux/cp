import { test, expect_eq } from "cap:test";
import {
    isValid,
    escapeToken,
    unescapeToken,
    countSegments,
    hasSegment,
    getSegment,
    parseArrayIndex,
    appendSegment,
    parentPointer,
    PointerIter,
} from "../../packages/jsonpointer/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/json-pointer — Root '' & validation rules", () => {
        expect_eq(isValid(""), true);
        expect_eq(countSegments(""), 0 as usize);
        expect_eq(isValid("/"), true);
        expect_eq(countSegments("/"), 1 as usize);
        expect_eq(isValid("/foo/bar/0"), true);
        expect_eq(isValid("/a~1b/m~0n"), true);

        // Invalid pointers: missing leading '/', bad '~' escape
        expect_eq(isValid("foo/bar"), false);
        expect_eq(isValid("/bad~2escape"), false);
        expect_eq(isValid("/trailing~"), false);
    });

    test("2. @cap/json-pointer — Multi-level path segment counting & extraction", () => {
        const ptr = "/users/42/profile/email";
        expect_eq(countSegments(ptr), 4 as usize);
        expect_eq(hasSegment(ptr, 0), true);
        expect_eq(hasSegment(ptr, 3), true);
        expect_eq(hasSegment(ptr, 4), false);

        let buf: [64]u8 = undefined;
        const n0 = getSegment(ptr, 0, buf[0..64]);
        expect_eq(testEq(buf[0..n0], "users"), true);
        const n1 = getSegment(ptr, 1, buf[0..64]);
        expect_eq(testEq(buf[0..n1], "42"), true);
        const n3 = getSegment(ptr, 3, buf[0..64]);
        expect_eq(testEq(buf[0..n3], "email"), true);
        expect_eq(getSegment(ptr, 9, buf[0..64]), 0 as usize);
    });

    test("3. @cap/json-pointer — RFC 6901 ~0 / ~1 escaping & ~01 non-double-unescaping", () => {
        let esc: [64]u8 = undefined;
        let unesc: [64]u8 = undefined;

        const n1 = escapeToken("a/b~c/d~1", esc[0..64]);
        expect_eq(testEq(esc[0..n1], "a~1b~0c~1d~01"), true);

        const n2 = unescapeToken(esc[0..n1], unesc[0..64]);
        expect_eq(testEq(unesc[0..n2], "a/b~c/d~1"), true);

        // Crucial RFC 6901 edge case: "~01" must decode to "~1", NOT "/"
        const n3 = unescapeToken("~01", unesc[0..64]);
        expect_eq(testEq(unesc[0..n3], "~1"), true);
    });

    test("4. @cap/json-pointer — Strict RFC 6901 array index & '-' append token", () => {
        expect_eq(parseArrayIndex("0"), 0 as i64);
        expect_eq(parseArrayIndex("1"), 1 as i64);
        expect_eq(parseArrayIndex("42"), 42 as i64);
        expect_eq(parseArrayIndex("1024"), 1024 as i64);
        expect_eq(parseArrayIndex("-"), -2 as i64);

        // Forbidden leading zeros & non-digits
        expect_eq(parseArrayIndex("00"), -1 as i64);
        expect_eq(parseArrayIndex("01"), -1 as i64);
        expect_eq(parseArrayIndex("-1"), -1 as i64);
        expect_eq(parseArrayIndex("abc"), -1 as i64);
        expect_eq(parseArrayIndex(""), -1 as i64);
    });

    test("5. @cap/json-pointer — parentPointer & appendSegment with auto-escaping", () => {
        expect_eq(testEq(parentPointer("/a/b/c"), "/a/b"), true);
        expect_eq(testEq(parentPointer("/a"), ""), true);
        expect_eq(testEq(parentPointer(""), ""), true);

        let buf: [64]u8 = undefined;
        const n1 = appendSegment("", "a/b", buf[0..64]);
        expect_eq(testEq(buf[0..n1], "/a~1b"), true);

        let buf2: [64]u8 = undefined;
        const n2 = appendSegment(buf[0..n1], "m~n", buf2[0..64]);
        expect_eq(testEq(buf2[0..n2], "/a~1b/m~0n"), true);
    });

    test("6. @cap/json-pointer — PointerIter zero-alloc streaming traversal", () => {
        let it = PointerIter.init("/foo/a~1b/m~0n/");
        let seg: [64]u8 = undefined;

        const l0 = it.next(seg[0..64]);
        expect_eq(l0, 3 as i64);
        expect_eq(testEq(seg[0..l0 as usize], "foo"), true);

        const l1 = it.next(seg[0..64]);
        expect_eq(l1, 3 as i64);
        expect_eq(testEq(seg[0..l1 as usize], "a/b"), true);

        const l2 = it.next(seg[0..64]);
        expect_eq(l2, 3 as i64);
        expect_eq(testEq(seg[0..l2 as usize], "m~n"), true);

        // Trailing '/' represents empty key "" (length 0)
        const l3 = it.next(seg[0..64]);
        expect_eq(l3, 0 as i64);

        // End of iteration
        expect_eq(it.next(seg[0..64]), -1 as i64);
    });
}
