// cap/test/testCapStatuses.cp
// Comprehensive unit tests for @cap/statuses (RFC 9110 / IANA HTTP status codes)

import { test, expect_eq } from "cap:test";
import {
    message,
    codeFromMessage,
    isValid,
    isInformational,
    isSuccess,
    isRedirect,
    isEmpty,
    isRetry,
    isClientError,
    isServerError,
} from "../../packages/statuses/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/statuses — 1xx/2xx/3xx/4xx/5xx message lookup", () => {
        expect_eq(sliceEq(message(100), "Continue"), true);
        expect_eq(sliceEq(message(101), "Switching Protocols"), true);
        expect_eq(sliceEq(message(200), "OK"), true);
        expect_eq(sliceEq(message(201), "Created"), true);
        expect_eq(sliceEq(message(204), "No Content"), true);
        expect_eq(sliceEq(message(301), "Moved Permanently"), true);
        expect_eq(sliceEq(message(304), "Not Modified"), true);
        expect_eq(sliceEq(message(404), "Not Found"), true);
        expect_eq(sliceEq(message(500), "Internal Server Error"), true);
        expect_eq(sliceEq(message(511), "Network Authentication Required"), true);
    });

    test("2. @cap/statuses — codeFromMessage case-insensitive reverse lookup", () => {
        expect_eq(codeFromMessage("ok"), 200 as u16);
        expect_eq(codeFromMessage("Not Found"), 404 as u16);
        expect_eq(codeFromMessage("TOO MANY REQUESTS"), 429 as u16);
        expect_eq(codeFromMessage("service unavailable"), 503 as u16);
        expect_eq(codeFromMessage("No Such Status"), 0 as u16);
        expect_eq(codeFromMessage(""), 0 as u16);
    });

    test("3. @cap/statuses — isRedirect, isEmpty & isRetry exact boundaries", () => {
        expect_eq(isRedirect(301), true);
        expect_eq(isRedirect(302), true);
        expect_eq(isRedirect(307), true);
        expect_eq(isRedirect(308), true);
        expect_eq(isRedirect(304), false);

        expect_eq(isEmpty(204), true);
        expect_eq(isEmpty(205), true);
        expect_eq(isEmpty(304), true);
        expect_eq(isEmpty(200), false);

        expect_eq(isRetry(408), true);
        expect_eq(isRetry(425), true);
        expect_eq(isRetry(429), true);
        expect_eq(isRetry(502), true);
        expect_eq(isRetry(503), true);
        expect_eq(isRetry(504), true);
        expect_eq(isRetry(404), false);
    });

    test("4. @cap/statuses — isInformational, isSuccess, isClientError, isServerError & isValid", () => {
        expect_eq(isInformational(103), true);
        expect_eq(isInformational(200), false);

        expect_eq(isSuccess(200), true);
        expect_eq(isSuccess(226), true);
        expect_eq(isSuccess(300), false);

        expect_eq(isClientError(400), true);
        expect_eq(isClientError(451), true);
        expect_eq(isClientError(500), false);

        expect_eq(isServerError(500), true);
        expect_eq(isServerError(511), true);
        expect_eq(isServerError(499), false);

        expect_eq(isValid(200), true);
        expect_eq(isValid(404), true);
        expect_eq(isValid(599), false);
    });

    test("5. @cap/statuses — unknown status codes return empty string / 0", () => {
        expect_eq(message(0).len, 0 as usize);
        expect_eq(message(299).len, 0 as usize);
        expect_eq(message(306).len, 0 as usize);
        expect_eq(message(419).len, 0 as usize);
        expect_eq(message(509).len, 0 as usize);
        expect_eq(message(999).len, 0 as usize);
        expect_eq(isValid(306), false);
        expect_eq(isValid(999), false);
    });

    test("6. @cap/statuses — modern standard codes 418, 429, 451, 503", () => {
        expect_eq(sliceEq(message(418), "I'm a Teapot"), true);
        expect_eq(sliceEq(message(429), "Too Many Requests"), true);
        expect_eq(sliceEq(message(451), "Unavailable For Legal Reasons"), true);
        expect_eq(sliceEq(message(503), "Service Unavailable"), true);
        expect_eq(codeFromMessage("I'm a Teapot"), 418 as u16);
        expect_eq(codeFromMessage("Unavailable For Legal Reasons"), 451 as u16);
    });
}
