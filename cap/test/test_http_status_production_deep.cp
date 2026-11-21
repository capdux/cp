import { test, expect_eq } from "cap:test";
import {
    StatusClass,
    statusLine,
    statusText,
    statusTextOrEmpty,
    classify,
    isInformational,
    isSuccess,
    isRedirect,
    isClientError,
    isServerError,
    isError,
    canHaveBody,
    isCacheableByDefault,
    isRetryable,
} from "cap:http";

test("1. statusText & statusLine across 1xx/2xx/3xx/4xx/5xx standard RFC codes", () => {
    // 1xx Informational
    expect_eq(std.mem.eql(u8, statusText(100) ?? "", "Continue"), true);
    expect_eq(std.mem.eql(u8, statusLine(100) ?? "", "100 Continue"), true);
    expect_eq(std.mem.eql(u8, statusText(101) ?? "", "Switching Protocols"), true);
    expect_eq(std.mem.eql(u8, statusLine(103) ?? "", "103 Early Hints"), true);

    // 2xx Success
    expect_eq(std.mem.eql(u8, statusText(200) ?? "", "OK"), true);
    expect_eq(std.mem.eql(u8, statusLine(200) ?? "", "200 OK"), true);
    expect_eq(std.mem.eql(u8, statusText(201) ?? "", "Created"), true);
    expect_eq(std.mem.eql(u8, statusText(204) ?? "", "No Content"), true);
    expect_eq(std.mem.eql(u8, statusText(206) ?? "", "Partial Content"), true);
    expect_eq(std.mem.eql(u8, statusLine(226) ?? "", "226 IM Used"), true);

    // 3xx Redirection
    expect_eq(std.mem.eql(u8, statusText(301) ?? "", "Moved Permanently"), true);
    expect_eq(std.mem.eql(u8, statusText(302) ?? "", "Found"), true);
    expect_eq(std.mem.eql(u8, statusText(304) ?? "", "Not Modified"), true);
    expect_eq(std.mem.eql(u8, statusText(307) ?? "", "Temporary Redirect"), true);
    expect_eq(std.mem.eql(u8, statusLine(308) ?? "", "308 Permanent Redirect"), true);

    // 4xx Client Error
    expect_eq(std.mem.eql(u8, statusText(400) ?? "", "Bad Request"), true);
    expect_eq(std.mem.eql(u8, statusText(401) ?? "", "Unauthorized"), true);
    expect_eq(std.mem.eql(u8, statusText(403) ?? "", "Forbidden"), true);
    expect_eq(std.mem.eql(u8, statusText(404) ?? "", "Not Found"), true);
    expect_eq(std.mem.eql(u8, statusLine(404) ?? "", "404 Not Found"), true);
    expect_eq(std.mem.eql(u8, statusText(418) ?? "", "I'm a Teapot"), true);
    expect_eq(std.mem.eql(u8, statusText(429) ?? "", "Too Many Requests"), true);
    expect_eq(std.mem.eql(u8, statusLine(451) ?? "", "451 Unavailable For Legal Reasons"), true);

    // 5xx Server Error
    expect_eq(std.mem.eql(u8, statusText(500) ?? "", "Internal Server Error"), true);
    expect_eq(std.mem.eql(u8, statusLine(502) ?? "", "502 Bad Gateway"), true);
    expect_eq(std.mem.eql(u8, statusText(503) ?? "", "Service Unavailable"), true);
    expect_eq(std.mem.eql(u8, statusText(504) ?? "", "Gateway Timeout"), true);
    expect_eq(std.mem.eql(u8, statusLine(511) ?? "", "511 Network Authentication Required"), true);

    // statusTextOrEmpty on known code
    expect_eq(std.mem.eql(u8, statusTextOrEmpty(200), "OK"), true);
    expect_eq(std.mem.eql(u8, statusTextOrEmpty(404), "Not Found"), true);
});

test("2. Unknown & out-of-range status code protection (0, 99, 299, 600, 999, 65535)", () => {
    const unknown_codes: [7]u16 = [0, 99, 299, 499, 600, 999, 65535];
    for (code of unknown_codes) {
        expect_eq(statusText(code) == null, true);
        expect_eq(statusLine(code) == null, true);
        expect_eq(std.mem.eql(u8, statusTextOrEmpty(code), ""), true);
    }

    expect_eq(classify(0) == StatusClass.Unknown, true);
    expect_eq(classify(99) == StatusClass.Unknown, true);
    expect_eq(classify(600) == StatusClass.Unknown, true);
    expect_eq(classify(999) == StatusClass.Unknown, true);
    expect_eq(classify(65535) == StatusClass.Unknown, true);

    expect_eq(isInformational(99), false);
    expect_eq(isSuccess(199), false);
    expect_eq(isRedirect(299), false);
    expect_eq(isClientError(399), false);
    expect_eq(isServerError(600), false);
    expect_eq(isError(399), false);
    expect_eq(isError(600), false);
});

test("3. classify & range predicates across exact boundaries", () => {
    expect_eq(classify(100) == StatusClass.Informational, true);
    expect_eq(classify(199) == StatusClass.Informational, true);
    expect_eq(isInformational(100), true);
    expect_eq(isInformational(199), true);
    expect_eq(isInformational(200), false);

    expect_eq(classify(200) == StatusClass.Success, true);
    expect_eq(classify(299) == StatusClass.Success, true);
    expect_eq(isSuccess(200), true);
    expect_eq(isSuccess(299), true);
    expect_eq(isSuccess(300), false);

    expect_eq(classify(300) == StatusClass.Redirection, true);
    expect_eq(classify(399) == StatusClass.Redirection, true);
    expect_eq(isRedirect(300), true);
    expect_eq(isRedirect(399), true);
    expect_eq(isRedirect(400), false);

    expect_eq(classify(400) == StatusClass.ClientError, true);
    expect_eq(classify(499) == StatusClass.ClientError, true);
    expect_eq(isClientError(400), true);
    expect_eq(isClientError(499), true);
    expect_eq(isClientError(500), false);

    expect_eq(classify(500) == StatusClass.ServerError, true);
    expect_eq(classify(599) == StatusClass.ServerError, true);
    expect_eq(isServerError(500), true);
    expect_eq(isServerError(599), true);
    expect_eq(isServerError(600), false);

    expect_eq(isError(400), true);
    expect_eq(isError(404), true);
    expect_eq(isError(500), true);
    expect_eq(isError(599), true);
    expect_eq(isError(200), false);
    expect_eq(isError(304), false);
});

test("4. canHaveBody enforces RFC 9110 §6.4.1 (1xx, 204, 304, out-of-range forbidden)", () => {
    // 1xx must not have body
    expect_eq(canHaveBody(100), false);
    expect_eq(canHaveBody(101), false);
    expect_eq(canHaveBody(103), false);
    expect_eq(canHaveBody(199), false);

    // 204 and 304 must not have body
    expect_eq(canHaveBody(204), false);
    expect_eq(canHaveBody(304), false);

    // Out-of-range codes cannot have body
    expect_eq(canHaveBody(0), false);
    expect_eq(canHaveBody(99), false);
    expect_eq(canHaveBody(600), false);
    expect_eq(canHaveBody(999), false);

    // Normal responses can have body
    expect_eq(canHaveBody(200), true);
    expect_eq(canHaveBody(201), true);
    expect_eq(canHaveBody(205), true);
    expect_eq(canHaveBody(301), true);
    expect_eq(canHaveBody(302), true);
    expect_eq(canHaveBody(404), true);
    expect_eq(canHaveBody(500), true);
    expect_eq(canHaveBody(503), true);
});

test("5. isCacheableByDefault (RFC 9110 §15.1) & isRetryable semantics", () => {
    // Cacheable by default per RFC 9110
    const cacheable: [12]u16 = [200, 203, 204, 206, 300, 301, 308, 404, 405, 410, 414, 501];
    for (code of cacheable) {
        expect_eq(isCacheableByDefault(code), true);
    }

    // Not cacheable by default
    const non_cacheable: [10]u16 = [100, 201, 202, 302, 307, 400, 401, 403, 500, 503];
    for (code of non_cacheable) {
        expect_eq(isCacheableByDefault(code), false);
    }

    // Retryable transient statuses
    const retryable: [7]u16 = [408, 425, 429, 500, 502, 503, 504];
    for (code of retryable) {
        expect_eq(isRetryable(code), true);
    }

    // Non-retryable statuses
    const non_retryable: [10]u16 = [0, 200, 204, 301, 400, 401, 403, 404, 422, 501];
    for (code of non_retryable) {
        expect_eq(isRetryable(code), false);
    }
});
