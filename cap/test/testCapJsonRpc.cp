// cap/test/testCapJsonRpc.cp
// Comprehensive unit tests for @cap/json-rpc (JSON-RPC 2.0 specification)

import { test, expect_eq } from "cap:test";
import {
    ERR_PARSE,
    ERR_INVALID_REQUEST,
    ERR_METHOD_NOT_FOUND,
    ERR_INVALID_PARAMS,
    ERR_INTERNAL,
    errorMessage,
    isServerError,
    encodeRequest,
    encodeResult,
    encodeError,
    isValidEnvelope,
    isBatch,
} from "../../packages/jsonrpc-core/src/index.cp";

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
    test("1. @cap/json-rpc — encodeRequest with numeric/string id & params", () => {
        let buf: [256]u8 = undefined;
        const n1: usize = encodeRequest("1", "subtract", "[42,23]", buf[0..256]);
        expect_eq(
            sliceEq(buf[0..n1], "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"subtract\",\"params\":[42,23]}"),
            true,
        );

        const n2: usize = encodeRequest("\"req-9\"", "tools/call", "{\"name\":\"ping\"}", buf[0..256]);
        expect_eq(
            sliceEq(
                buf[0..n2],
                "{\"jsonrpc\":\"2.0\",\"id\":\"req-9\",\"method\":\"tools/call\",\"params\":{\"name\":\"ping\"}}",
            ),
            true,
        );
    });

    test("2. @cap/json-rpc — Notification omits id & optional params", () => {
        let buf: [256]u8 = undefined;
        const n: usize = encodeRequest("", "notifications/initialized", "", buf[0..256]);
        expect_eq(
            sliceEq(buf[0..n], "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}"),
            true,
        );
        expect_eq(isValidEnvelope(buf[0..n]), true);
    });

    test("3. @cap/json-rpc — encodeResult success response", () => {
        let buf: [256]u8 = undefined;
        const n: usize = encodeResult("42", "{\"ok\":true}", buf[0..256]);
        expect_eq(
            sliceEq(buf[0..n], "{\"jsonrpc\":\"2.0\",\"id\":42,\"result\":{\"ok\":true}}"),
            true,
        );
        expect_eq(isValidEnvelope(buf[0..n]), true);
    });

    test("4. @cap/json-rpc — encodeError with null id, optional data & string escaping", () => {
        let buf: [256]u8 = undefined;
        const n1: usize = encodeError("", ERR_PARSE, "Bad \"JSON\"\nline 1", "", buf[0..256]);
        expect_eq(
            sliceEq(
                buf[0..n1],
                "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32700,\"message\":\"Bad \\\"JSON\\\"\\nline 1\"}}",
            ),
            true,
        );

        const n2: usize = encodeError("7", ERR_INVALID_PARAMS, "Missing field", "{\"field\":\"x\"}", buf[0..256]);
        expect_eq(
            sliceEq(
                buf[0..n2],
                "{\"jsonrpc\":\"2.0\",\"id\":7,\"error\":{\"code\":-32602,\"message\":\"Missing field\",\"data\":{\"field\":\"x\"}}}",
            ),
            true,
        );
    });

    test("5. @cap/json-rpc — isValidEnvelope & isBatch detection", () => {
        expect_eq(isValidEnvelope("  { \"jsonrpc\" : \"2.0\", \"method\": \"ping\" }  "), true);
        expect_eq(isValidEnvelope("{\"jsonrpc\":\"1.0\",\"method\":\"ping\"}"), false);
        // Nested "jsonrpc":"2.0" inside params should NOT pass top-level envelope check
        expect_eq(isValidEnvelope("{\"params\":{\"jsonrpc\":\"2.0\"}}"), false);
        expect_eq(isValidEnvelope("[{\"jsonrpc\":\"2.0\"}]"), false);

        expect_eq(isBatch(" [{\"jsonrpc\":\"2.0\",\"method\":\"a\"}] "), true);
        expect_eq(isBatch("{\"jsonrpc\":\"2.0\"}"), false);
        expect_eq(isBatch(""), false);
    });

    test("6. @cap/json-rpc — errorMessage, isServerError & tiny buffer protection", () => {
        expect_eq(sliceEq(errorMessage(ERR_PARSE), "Parse error"), true);
        expect_eq(sliceEq(errorMessage(ERR_INVALID_REQUEST), "Invalid Request"), true);
        expect_eq(sliceEq(errorMessage(ERR_METHOD_NOT_FOUND), "Method not found"), true);
        expect_eq(sliceEq(errorMessage(ERR_INVALID_PARAMS), "Invalid params"), true);
        expect_eq(sliceEq(errorMessage(ERR_INTERNAL), "Internal error"), true);
        expect_eq(sliceEq(errorMessage(-32050), "Server error"), true);
        expect_eq(errorMessage(0).len, 0 as usize);

        expect_eq(isServerError(-32000), true);
        expect_eq(isServerError(-32099), true);
        expect_eq(isServerError(-32100), false);
        expect_eq(isServerError(-31999), false);

        let tiny: [10]u8 = undefined;
        expect_eq(encodeRequest("1", "ping", "", tiny[0..10]), 0 as usize);
        expect_eq(encodeResult("1", "true", tiny[0..10]), 0 as usize);
        expect_eq(encodeError("1", ERR_INTERNAL, "err", "", tiny[0..10]), 0 as usize);
    });
}
