// cap/test/testCapHttpStatus.cp
// Comprehensive unit tests for @cap/http-status

import { test, expect_eq } from "cap:test";
import {
    CONTINUE,
    EARLY_HINTS,
    OK,
    CREATED,
    ACCEPTED,
    NO_CONTENT,
    PARTIAL_CONTENT,
    MULTI_STATUS,
    MOVED_PERMANENTLY,
    FOUND,
    SEE_OTHER,
    NOT_MODIFIED,
    TEMPORARY_REDIRECT,
    PERMANENT_REDIRECT,
    BAD_REQUEST,
    UNAUTHORIZED,
    PAYMENT_REQUIRED,
    FORBIDDEN,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    CONFLICT,
    GONE,
    PAYLOAD_TOO_LARGE,
    UNSUPPORTED_MEDIA_TYPE,
    IM_A_TEAPOT,
    UNPROCESSABLE_ENTITY,
    TOO_MANY_REQUESTS,
    INTERNAL_SERVER_ERROR,
    NOT_IMPLEMENTED,
    BAD_GATEWAY,
    SERVICE_UNAVAILABLE,
    GATEWAY_TIMEOUT,
    CONNECTION_TIMED_OUT,
    StatusClass,
    reasonPhrase,
    codeFromPhrase,
    statusClass,
    isValidStatus,
    isKnownStatus,
    isInformational,
    isSuccess,
    isRedirect,
    isClientError,
    isServerError,
    isEmptyBody,
    isRetryable,
    isCacheableByDefault,
} from "../../packages/http-status-codes/src/index.cp";

fn main() {
    test("1. @cap/http-status — 1xx..5xx & Cloudflare reasonPhrase lookup", () => {
        expect_eq(reasonPhrase(CONTINUE), "Continue");
        expect_eq(reasonPhrase(EARLY_HINTS), "Early Hints");
        expect_eq(reasonPhrase(OK), "OK");
        expect_eq(reasonPhrase(CREATED), "Created");
        expect_eq(reasonPhrase(ACCEPTED), "Accepted");
        expect_eq(reasonPhrase(NO_CONTENT), "No Content");
        expect_eq(reasonPhrase(PARTIAL_CONTENT), "Partial Content");
        expect_eq(reasonPhrase(MULTI_STATUS), "Multi-Status");
        expect_eq(reasonPhrase(MOVED_PERMANENTLY), "Moved Permanently");
        expect_eq(reasonPhrase(FOUND), "Found");
        expect_eq(reasonPhrase(SEE_OTHER), "See Other");
        expect_eq(reasonPhrase(NOT_MODIFIED), "Not Modified");
        expect_eq(reasonPhrase(TEMPORARY_REDIRECT), "Temporary Redirect");
        expect_eq(reasonPhrase(PERMANENT_REDIRECT), "Permanent Redirect");
        expect_eq(reasonPhrase(BAD_REQUEST), "Bad Request");
        expect_eq(reasonPhrase(UNAUTHORIZED), "Unauthorized");
        expect_eq(reasonPhrase(PAYMENT_REQUIRED), "Payment Required");
        expect_eq(reasonPhrase(FORBIDDEN), "Forbidden");
        expect_eq(reasonPhrase(NOT_FOUND), "Not Found");
        expect_eq(reasonPhrase(METHOD_NOT_ALLOWED), "Method Not Allowed");
        expect_eq(reasonPhrase(CONFLICT), "Conflict");
        expect_eq(reasonPhrase(GONE), "Gone");
        expect_eq(reasonPhrase(PAYLOAD_TOO_LARGE), "Payload Too Large");
        expect_eq(reasonPhrase(UNSUPPORTED_MEDIA_TYPE), "Unsupported Media Type");
        expect_eq(reasonPhrase(IM_A_TEAPOT), "I'm a Teapot");
        expect_eq(reasonPhrase(UNPROCESSABLE_ENTITY), "Unprocessable Entity");
        expect_eq(reasonPhrase(TOO_MANY_REQUESTS), "Too Many Requests");
        expect_eq(reasonPhrase(INTERNAL_SERVER_ERROR), "Internal Server Error");
        expect_eq(reasonPhrase(NOT_IMPLEMENTED), "Not Implemented");
        expect_eq(reasonPhrase(BAD_GATEWAY), "Bad Gateway");
        expect_eq(reasonPhrase(SERVICE_UNAVAILABLE), "Service Unavailable");
        expect_eq(reasonPhrase(GATEWAY_TIMEOUT), "Gateway Timeout");
        expect_eq(reasonPhrase(CONNECTION_TIMED_OUT), "Connection Timed Out");
        expect_eq(reasonPhrase(599), "");
    });

    test("2. @cap/http-status — Reverse codeFromPhrase (case/space/underscore/hyphen/alias)", () => {
        expect_eq(codeFromPhrase("Not Found") ?? 0, 404 as u16);
        expect_eq(codeFromPhrase("not_found") ?? 0, 404 as u16);
        expect_eq(codeFromPhrase("TOO-MANY-REQUESTS") ?? 0, 429 as u16);
        expect_eq(codeFromPhrase("im_a_teapot") ?? 0, 418 as u16);
        expect_eq(codeFromPhrase("I'm a teapot") ?? 0, 418 as u16);
        expect_eq(codeFromPhrase("Content Too Large") ?? 0, 413 as u16);
        expect_eq(codeFromPhrase("unprocessable_content") ?? 0, 422 as u16);
        expect_eq(codeFromPhrase("  service unavailable  ") ?? 0, 503 as u16);
        expect_eq(codeFromPhrase("Non-Authoritative Information") ?? 0, 203 as u16);
        expect_eq(codeFromPhrase("non_authoritative_information") ?? 0, 203 as u16);
        expect_eq(codeFromPhrase("Unknown Status") == null, true);
        expect_eq(codeFromPhrase("") == null, true);
    });

    test("3. @cap/http-status — statusClass & range classification predicates", () => {
        expect_eq(statusClass(100) == StatusClass.Informational, true);
        expect_eq(statusClass(204) == StatusClass.Success, true);
        expect_eq(statusClass(307) == StatusClass.Redirection, true);
        expect_eq(statusClass(429) == StatusClass.ClientError, true);
        expect_eq(statusClass(502) == StatusClass.ServerError, true);
        expect_eq(statusClass(99) == StatusClass.Invalid, true);
        expect_eq(statusClass(600) == StatusClass.Invalid, true);

        expect_eq(isInformational(103), true);
        expect_eq(isInformational(200), false);
        expect_eq(isSuccess(201), true);
        expect_eq(isRedirect(308), true);
        expect_eq(isClientError(418), true);
        expect_eq(isServerError(511), true);
    });

    test("4. @cap/http-status — isEmptyBody semantics (1xx, 204, 205, 304)", () => {
        expect_eq(isEmptyBody(100), true);
        expect_eq(isEmptyBody(101), true);
        expect_eq(isEmptyBody(204), true);
        expect_eq(isEmptyBody(205), true);
        expect_eq(isEmptyBody(304), true);
        expect_eq(isEmptyBody(200), false);
        expect_eq(isEmptyBody(302), false);
        expect_eq(isEmptyBody(404), false);
        expect_eq(isEmptyBody(500), false);
    });

    test("5. @cap/http-status — isRetryable & isCacheableByDefault semantics", () => {
        // Retryable
        expect_eq(isRetryable(408), true);
        expect_eq(isRetryable(425), true);
        expect_eq(isRetryable(429), true);
        expect_eq(isRetryable(500), true);
        expect_eq(isRetryable(502), true);
        expect_eq(isRetryable(503), true);
        expect_eq(isRetryable(504), true);
        expect_eq(isRetryable(400), false);
        expect_eq(isRetryable(404), false);
        expect_eq(isRetryable(200), false);

        // Cacheable by default (RFC 9110)
        expect_eq(isCacheableByDefault(200), true);
        expect_eq(isCacheableByDefault(203), true);
        expect_eq(isCacheableByDefault(204), true);
        expect_eq(isCacheableByDefault(206), true);
        expect_eq(isCacheableByDefault(300), true);
        expect_eq(isCacheableByDefault(301), true);
        expect_eq(isCacheableByDefault(404), true);
        expect_eq(isCacheableByDefault(405), true);
        expect_eq(isCacheableByDefault(410), true);
        expect_eq(isCacheableByDefault(414), true);
        expect_eq(isCacheableByDefault(501), true);
        expect_eq(isCacheableByDefault(201), false);
        expect_eq(isCacheableByDefault(302), false);
        expect_eq(isCacheableByDefault(500), false);
    });

    test("6. @cap/http-status — Boundary & invalid code checks", () => {
        expect_eq(isValidStatus(0), false);
        expect_eq(isValidStatus(99), false);
        expect_eq(isValidStatus(100), true);
        expect_eq(isValidStatus(599), true);
        expect_eq(isValidStatus(600), false);
        expect_eq(isValidStatus(999), false);

        expect_eq(isKnownStatus(200), true);
        expect_eq(isKnownStatus(526), true);
        expect_eq(isKnownStatus(299), false);
        expect_eq(isKnownStatus(99), false);
        expect_eq(reasonPhrase(0), "");
        expect_eq(reasonPhrase(600), "");
    });
}
