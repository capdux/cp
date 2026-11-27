import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    HTTP_METHOD_COUNT,
    METHODS,
    METHODS_UPPER,
    findMethodIndex,
    isHttpMethod,
    toLowerMethod,
    methodToLower,
    toUpperMethod,
    methodToUpper,
    methodAt,
    methodUpperAt,
    isSafeMethod,
    isIdempotentMethod,
    isCacheableMethod,
    mayHaveRequestBody,
    isStandardRfc9110Method,
    isWebDavMethod,
    allowsResponseBody,
} from "../../packages/methods/src/index.cp";

fn main() {
    test("1. methods — Core HTTP methods case-insensitive lookup", () => {
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

    test("2. methods — Canonical lower/upper normalization & methodToUpper/methodToLower", () => {
        expect_eq(strEq(toLowerMethod("GET"), "get"), true);
        expect_eq(strEq(methodToLower("DeLeTe"), "delete"), true);
        expect_eq(strEq(toUpperMethod("patch"), "PATCH"), true);
        expect_eq(strEq(methodToUpper("propfind"), "PROPFIND"), true);
        expect_eq(strEq(toLowerMethod("NOPE"), ""), true);
        expect_eq(strEq(methodToUpper("NOPE"), ""), true);
    });

    test("3. methods — RFC 9110 safe and idempotent predicates", () => {
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

    test("4. methods — Cacheable and request-body predicates", () => {
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

    test("5. methods — Full 34-entry METHODS / METHODS_UPPER table consistency", () => {
        expect_eq(METHODS.len, HTTP_METHOD_COUNT);
        expect_eq(METHODS_UPPER.len, HTTP_METHOD_COUNT);
        let i: usize = 0;
        while (i < HTTP_METHOD_COUNT) {
            let name = methodAt(i);
            let upper = methodUpperAt(i);
            expect_eq(strEq(name, METHODS[i]), true);
            expect_eq(strEq(upper, METHODS_UPPER[i]), true);
            expect_eq(findMethodIndex(name), i as i32);
            expect_eq(findMethodIndex(upper), i as i32);
            i = i + 1;
        }
        expect_eq(strEq(methodAt(HTTP_METHOD_COUNT), ""), true);
        expect_eq(strEq(methodUpperAt(HTTP_METHOD_COUNT), ""), true);
    });

    test("6. methods — WebDAV methods recognized accurately", () => {
        expect_eq(isHttpMethod("MKCOL"), true);
        expect_eq(isHttpMethod("PROPFIND"), true);
        expect_eq(isHttpMethod("PROPPATCH"), true);
        expect_eq(isHttpMethod("LOCK"), true);
        expect_eq(isHttpMethod("UNLOCK"), true);
        expect_eq(isSafeMethod("PROPFIND"), true);
        expect_eq(isIdempotentMethod("MKCOL"), true);
        expect_eq(isWebDavMethod("PROPFIND"), true);
        expect_eq(isWebDavMethod("MKCALENDAR"), true);
        expect_eq(isWebDavMethod("GET"), false);
    });

    test("7. methods — isStandardRfc9110Method & allowsResponseBody", () => {
        expect_eq(isStandardRfc9110Method("GET"), true);
        expect_eq(isStandardRfc9110Method("POST"), true);
        expect_eq(isStandardRfc9110Method("PATCH"), true);
        expect_eq(isStandardRfc9110Method("CONNECT"), true);
        expect_eq(isStandardRfc9110Method("PROPFIND"), false);
        expect_eq(isStandardRfc9110Method("UNKNOWN"), false);

        expect_eq(allowsResponseBody("GET"), true);
        expect_eq(allowsResponseBody("POST"), true);
        expect_eq(allowsResponseBody("HEAD"), false);
        expect_eq(allowsResponseBody("NOPE"), false);
    });
}
