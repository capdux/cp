// cap/test/testCapHttpErrors.cp
// Comprehensive unit tests for @cap/http-errors

import { test, expect_eq } from "cap:test";
import {
    HttpError,
    createHttpError,
    badRequest,
    unauthorized,
    forbidden,
    notFound,
    methodNotAllowed,
    conflict,
    gone,
    payloadTooLarge,
    unsupportedMediaType,
    unprocessableEntity,
    tooManyRequests,
    internalServerError,
    notImplemented,
    badGateway,
    serviceUnavailable,
    gatewayTimeout,
    isClientError,
    isServerError,
    isRetryable,
    toProblemJson,
} from "../../packages/http-errors/src/index.cp";

fn main() {
    test("1. @cap/http-errors — 4xx constructors default expose=true & metadata", () => {
        const e400 = badRequest("missing field 'name'");
        expect_eq(e400.status, 400 as u16);
        expect_eq(e400.code, "BAD_REQUEST");
        expect_eq(e400.title, "Bad Request");
        expect_eq(e400.expose, true);
        expect_eq(isClientError(e400), true);
        expect_eq(isServerError(e400), false);

        expect_eq(unauthorized("").status, 401 as u16);
        expect_eq(forbidden("").status, 403 as u16);
        expect_eq(notFound("").status, 404 as u16);
        expect_eq(methodNotAllowed("").status, 405 as u16);
        expect_eq(conflict("").status, 409 as u16);
        expect_eq(gone("").status, 410 as u16);
        expect_eq(payloadTooLarge("").status, 413 as u16);
        expect_eq(unsupportedMediaType("").status, 415 as u16);
        expect_eq(unprocessableEntity("").status, 422 as u16);
        expect_eq(tooManyRequests("").status, 429 as u16);
    });

    test("2. @cap/http-errors — 5xx constructors default expose=false & server predicates", () => {
        const e500 = internalServerError("sql syntax error at line 42");
        expect_eq(e500.status, 500 as u16);
        expect_eq(e500.code, "INTERNAL_SERVER_ERROR");
        expect_eq(e500.title, "Internal Server Error");
        expect_eq(e500.expose, false);
        expect_eq(isServerError(e500), true);
        expect_eq(isClientError(e500), false);

        expect_eq(notImplemented("").status, 501 as u16);
        expect_eq(badGateway("").status, 502 as u16);
        expect_eq(serviceUnavailable("").status, 503 as u16);
        expect_eq(gatewayTimeout("").status, 504 as u16);
    });

    test("3. @cap/http-errors — isRetryable semantics", () => {
        expect_eq(isRetryable(tooManyRequests("slow down")), true);
        expect_eq(isRetryable(badGateway("upstream down")), true);
        expect_eq(isRetryable(serviceUnavailable("restarting")), true);
        expect_eq(isRetryable(gatewayTimeout("timed out")), true);
        expect_eq(isRetryable(badRequest("invalid")), false);
        expect_eq(isRetryable(notFound("gone")), false);
        expect_eq(isRetryable(internalServerError("bug")), false);
    });

    test("4. @cap/http-errors — RFC 9457 application/problem+json serialization", () => {
        let out: [512]u8 = undefined;
        const base = notFound("User 42 not found");
        const err = base.withInstance("/api/v1/users/42");
        const json = toProblemJson(err, false, out[0..512]);
        expect_eq(
            json,
            "{\"type\":\"about:blank\",\"title\":\"Not Found\",\"status\":404,\"code\":\"NOT_FOUND\",\"detail\":\"User 42 not found\",\"instance\":\"/api/v1/users/42\"}",
        );
    });

    test("5. @cap/http-errors — Hidden 5xx detail protection vs debug override", () => {
        let out: [512]u8 = undefined;
        const err = internalServerError("secret db password leaked");

        const prod_json = toProblemJson(err, false, out[0..512]);
        expect_eq(
            prod_json,
            "{\"type\":\"about:blank\",\"title\":\"Internal Server Error\",\"status\":500,\"code\":\"INTERNAL_SERVER_ERROR\"}",
        );

        const dev_json = toProblemJson(err, true, out[0..512]);
        expect_eq(
            dev_json,
            "{\"type\":\"about:blank\",\"title\":\"Internal Server Error\",\"status\":500,\"code\":\"INTERNAL_SERVER_ERROR\",\"detail\":\"secret db password leaked\"}",
        );
    });

    test("6. @cap/http-errors — JSON special char escaping & unknown status fallback", () => {
        let out: [512]u8 = undefined;
        const err = badRequest("bad \"quote\" \\ path\nline2");
        const json = toProblemJson(err, false, out[0..512]);
        expect_eq(
            json,
            "{\"type\":\"about:blank\",\"title\":\"Bad Request\",\"status\":400,\"code\":\"BAD_REQUEST\",\"detail\":\"bad \\\"quote\\\" \\\\ path\\nline2\"}",
        );

        // Non-error status (e.g. 200) safely falls back to 500
        const fallback = createHttpError(200, "unexpected");
        expect_eq(fallback.status, 500 as u16);
        expect_eq(fallback.expose, false);

        // Unknown 4xx status keeps status and uses generic Client Error title
        const unknown4xx = createHttpError(499, "client closed");
        expect_eq(unknown4xx.status, 499 as u16);
        expect_eq(unknown4xx.title, "Client Error");
        expect_eq(unknown4xx.expose, true);
    });
}
