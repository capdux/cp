import { test, expect_eq } from "cap:test";
import {
    METHOD_UNKNOWN,
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_TRACE,
    METHOD_PATCH,
    HTTP_UNKNOWN,
    HTTP_09,
    HTTP_10,
    HTTP_11,
    HTTP_2,
    HTTP_3,
    parseMethod,
    methodAsStr,
    isSafeMethod,
    isIdempotentMethod,
    StatusCode,
    parseHttpVersion,
    httpVersionAsStr,
    isValidHeaderName,
    headerNameEq,
} from "../../packages/http/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/http-types — parseMethod & methodAsStr round-trip", () => {
        expect_eq(parseMethod("GET"), METHOD_GET);
        expect_eq(parseMethod("HEAD"), METHOD_HEAD);
        expect_eq(parseMethod("POST"), METHOD_POST);
        expect_eq(parseMethod("PUT"), METHOD_PUT);
        expect_eq(parseMethod("DELETE"), METHOD_DELETE);
        expect_eq(parseMethod("CONNECT"), METHOD_CONNECT);
        expect_eq(parseMethod("OPTIONS"), METHOD_OPTIONS);
        expect_eq(parseMethod("TRACE"), METHOD_TRACE);
        expect_eq(parseMethod("PATCH"), METHOD_PATCH);
        expect_eq(parseMethod("get"), METHOD_UNKNOWN);
        expect_eq(parseMethod("FOO"), METHOD_UNKNOWN);

        expect_eq(eqStr(methodAsStr(METHOD_GET), "GET"), true);
        expect_eq(eqStr(methodAsStr(METHOD_TRACE), "TRACE"), true);
        expect_eq(eqStr(methodAsStr(METHOD_PATCH), "PATCH"), true);
        expect_eq(eqStr(methodAsStr(METHOD_UNKNOWN), ""), true);
    });

    test("2. @cap/http-types — isSafeMethod & isIdempotentMethod RFC 9110 semantics", () => {
        expect_eq(isSafeMethod(METHOD_GET), true);
        expect_eq(isSafeMethod(METHOD_HEAD), true);
        expect_eq(isSafeMethod(METHOD_OPTIONS), true);
        expect_eq(isSafeMethod(METHOD_TRACE), true);
        expect_eq(isSafeMethod(METHOD_POST), false);
        expect_eq(isSafeMethod(METHOD_PUT), false);
        expect_eq(isSafeMethod(METHOD_DELETE), false);

        expect_eq(isIdempotentMethod(METHOD_GET), true);
        expect_eq(isIdempotentMethod(METHOD_PUT), true);
        expect_eq(isIdempotentMethod(METHOD_DELETE), true);
        expect_eq(isIdempotentMethod(METHOD_POST), false);
        expect_eq(isIdempotentMethod(METHOD_PATCH), false);
        expect_eq(isIdempotentMethod(METHOD_CONNECT), false);
    });

    test("3. @cap/http-types — StatusCode validation & class predicates", () => {
        expect_eq(StatusCode.fromU16(99) == null, true);
        expect_eq(StatusCode.fromU16(600) == null, true);

        const s100 = StatusCode.fromU16(100) ?? StatusCode { code: 0 };
        expect_eq(s100.asU16(), 100 as u16);
        expect_eq(s100.isInformational(), true);
        expect_eq(s100.isSuccess(), false);

        const s200 = StatusCode.fromU16(200) ?? StatusCode { code: 0 };
        expect_eq(s200.isSuccess(), true);
        expect_eq(s200.isRedirection(), false);

        const s301 = StatusCode.fromU16(301) ?? StatusCode { code: 0 };
        expect_eq(s301.isRedirection(), true);

        const s404 = StatusCode.fromU16(404) ?? StatusCode { code: 0 };
        expect_eq(s404.isClientError(), true);
        expect_eq(s404.isServerError(), false);

        const s503 = StatusCode.fromU16(503) ?? StatusCode { code: 0 };
        expect_eq(s503.isServerError(), true);
    });

    test("4. @cap/http-types — StatusCode canonicalReason across 1xx-5xx", () => {
        const c101 = StatusCode.fromU16(101) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c101.canonicalReason(), "Switching Protocols"), true);

        const c204 = StatusCode.fromU16(204) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c204.canonicalReason(), "No Content"), true);

        const c308 = StatusCode.fromU16(308) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c308.canonicalReason(), "Permanent Redirect"), true);

        const c418 = StatusCode.fromU16(418) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c418.canonicalReason(), "I'm a teapot"), true);

        const c429 = StatusCode.fromU16(429) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c429.canonicalReason(), "Too Many Requests"), true);

        const c502 = StatusCode.fromU16(502) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c502.canonicalReason(), "Bad Gateway"), true);

        const c599 = StatusCode.fromU16(599) ?? StatusCode { code: 0 };
        expect_eq(eqStr(c599.canonicalReason(), ""), true);
    });

    test("5. @cap/http-types — parseHttpVersion & httpVersionAsStr (case-insensitive)", () => {
        expect_eq(parseHttpVersion("HTTP/0.9"), HTTP_09);
        expect_eq(parseHttpVersion("HTTP/1.0"), HTTP_10);
        expect_eq(parseHttpVersion("http/1.1"), HTTP_11);
        expect_eq(parseHttpVersion("HTTP/2"), HTTP_2);
        expect_eq(parseHttpVersion("HTTP/2.0"), HTTP_2);
        expect_eq(parseHttpVersion("http/3"), HTTP_3);
        expect_eq(parseHttpVersion("HTTP/4.0"), HTTP_UNKNOWN);

        expect_eq(eqStr(httpVersionAsStr(HTTP_09), "HTTP/0.9"), true);
        expect_eq(eqStr(httpVersionAsStr(HTTP_11), "HTTP/1.1"), true);
        expect_eq(eqStr(httpVersionAsStr(HTTP_2), "HTTP/2.0"), true);
        expect_eq(eqStr(httpVersionAsStr(HTTP_3), "HTTP/3.0"), true);
        expect_eq(eqStr(httpVersionAsStr(HTTP_UNKNOWN), ""), true);
    });

    test("6. @cap/http-types — isValidHeaderName & headerNameEq via @cap/unicase", () => {
        expect_eq(isValidHeaderName("Content-Type"), true);
        expect_eq(isValidHeaderName("X-Custom_Header.1"), true);
        expect_eq(isValidHeaderName(""), false);
        expect_eq(isValidHeaderName("Bad Header"), false);
        expect_eq(isValidHeaderName("Bad:Header"), false);
        expect_eq(isValidHeaderName("Bad\nHeader"), false);

        expect_eq(headerNameEq("Content-Type", "content-type"), true);
        expect_eq(headerNameEq("X-Request-ID", "x-request-id"), true);
        expect_eq(headerNameEq("Accept", "Accept-Encoding"), false);
    });
}
