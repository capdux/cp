import { test, expect_eq } from "cap:test";
import {
    HTTP_METHOD_COUNT, findMethodIndex, isHttpMethod, toLowerMethod, toUpperMethod,
    methodAt, isSafeMethod, isIdempotentMethod, isCacheableMethod, mayHaveRequestBody
} from "../../packages/methods/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/methods — Core HTTP methods case-insensitive lookup", () => {
        expect_eq(isHttpMethod("GET"), true);
        expect_eq(isHttpMethod("get"), true);
        expect_eq(isHttpMethod("PoSt"), true);
        expect_eq(isHttpMethod("DELETE"), true);
        expect_eq(isHttpMethod("PATCH"), true);
        expect_eq(isHttpMethod("OPTIONS"), true);
        expect_eq(isHttpMethod("M-SEARCH"), true);
        expect_eq(isHttpMethod("UNKNOWN"), false);
        expect_eq(isHttpMethod(""), false);
    });

    test("2. @cap/methods — Canonical lower/upper normalization", () => {
        expect_eq(strEq(toLowerMethod("GET"), "get"), true);
        expect_eq(strEq(toUpperMethod("patch"), "PATCH"), true);
        expect_eq(strEq(toUpperMethod("propfind"), "PROPFIND"), true);
        expect_eq(strEq(toLowerMethod("NOPE"), ""), true);
    });

    test("3. @cap/methods — RFC 9110 safe and idempotent predicates", () => {
        expect_eq(isSafeMethod("GET"), true);
        expect_eq(isSafeMethod("HEAD"), true);
        expect_eq(isSafeMethod("OPTIONS"), true);
        expect_eq(isSafeMethod("POST"), false);
        expect_eq(isSafeMethod("PUT"), false);
        expect_eq(isSafeMethod("DELETE"), false);

        expect_eq(isIdempotentMethod("GET"), true);
        expect_eq(isIdempotentMethod("PUT"), true);
        expect_eq(isIdempotentMethod("DELETE"), true);
        expect_eq(isIdempotentMethod("POST"), false);
        expect_eq(isIdempotentMethod("PATCH"), false);
    });

    test("4. @cap/methods — Cacheable and request-body predicates", () => {
        expect_eq(isCacheableMethod("GET"), true);
        expect_eq(isCacheableMethod("HEAD"), true);
        expect_eq(isCacheableMethod("POST"), true);
        expect_eq(isCacheableMethod("PUT"), false);

        expect_eq(mayHaveRequestBody("POST"), true);
        expect_eq(mayHaveRequestBody("PUT"), true);
        expect_eq(mayHaveRequestBody("PATCH"), true);
        expect_eq(mayHaveRequestBody("GET"), false);
        expect_eq(mayHaveRequestBody("HEAD"), false);
    });

    test("5. @cap/methods — Full 34-entry table binary-search self-consistency", () => {
        let i: usize = 0;
        while (i < HTTP_METHOD_COUNT) {
            let name = methodAt(i);
            expect_eq(name.len > 0, true);
            expect_eq(findMethodIndex(name), i as i32);
            i = i + 1;
        }
        expect_eq(strEq(methodAt(HTTP_METHOD_COUNT), ""), true);
    });

    test("6. @cap/methods — WebDAV methods recognized accurately", () => {
        expect_eq(isHttpMethod("MKCOL"), true);
        expect_eq(isHttpMethod("PROPFIND"), true);
        expect_eq(isHttpMethod("PROPPATCH"), true);
        expect_eq(isHttpMethod("LOCK"), true);
        expect_eq(isHttpMethod("UNLOCK"), true);
        expect_eq(isSafeMethod("PROPFIND"), true);
        expect_eq(isIdempotentMethod("MKCOL"), true);
    });
}
