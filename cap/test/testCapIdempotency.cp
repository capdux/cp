// cap/test/testCapIdempotency.cp
// Comprehensive unit tests for @cap/idempotency

import { test, expect_eq } from "cap:test";
import {
    IDEMP_NEW,
    IDEMP_REPLAY,
    IDEMP_IN_FLIGHT,
    IDEMP_CONFLICT,
    parseKeyHeader,
    formatKeyHeader,
    isValidKey,
    isUuidV4Like,
    fingerprintRequest,
    evaluateState,
} from "../../packages/@cap/idempotency/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/idempotency — Quoted & bare Idempotency-Key parsing with unescaping", () => {
        let out1: [64]u8 = undefined;
        const n1 = parseKeyHeader("  \"key-123\"  ", out1[0..64]);
        expect_eq(sliceEq(out1[0..n1], "key-123"), true);

        let out2: [64]u8 = undefined;
        const n2 = parseKeyHeader("bare-key_456", out2[0..64]);
        expect_eq(sliceEq(out2[0..n2], "bare-key_456"), true);

        let out3: [64]u8 = undefined;
        const n3 = parseKeyHeader("\"a\\\"b\\\\c\"", out3[0..64]);
        expect_eq(sliceEq(out3[0..n3], "a\"b\\c"), true);

        // Unclosed quote or trailing garbage fails.
        expect_eq(parseKeyHeader("\"unclosed", out1[0..64]), 0 as usize);
        expect_eq(parseKeyHeader("\"ok\"extra", out1[0..64]), 0 as usize);
    });

    test("2. @cap/idempotency — formatKeyHeader RFC 8941 round-trip", () => {
        let fmtBuf: [64]u8 = undefined;
        const fn1 = formatKeyHeader("pay_01\"\\99", fmtBuf[0..64]);
        expect_eq(sliceEq(fmtBuf[0..fn1], "\"pay_01\\\"\\\\99\""), true);

        let parsedBuf: [64]u8 = undefined;
        const pn1 = parseKeyHeader(fmtBuf[0..fn1], parsedBuf[0..64]);
        expect_eq(sliceEq(parsedBuf[0..pn1], "pay_01\"\\99"), true);

        let tiny: [4]u8 = undefined;
        expect_eq(formatKeyHeader("pay_01", tiny[0..4]), 0 as usize);
    });

    test("3. @cap/idempotency — isValidKey rejects empty, >255, spaces & control chars", () => {
        expect_eq(isValidKey("ok-key_123!"), true);
        expect_eq(isValidKey(""), false);
        expect_eq(isValidKey("has space"), false);
        expect_eq(isValidKey("bad\nnewline"), false);
        expect_eq(isValidKey("bad\tkey"), false);

        let longKey: [256]u8 = undefined;
        let i: usize = 0;
        while (i < 256) {
            longKey[i] = 'a' as u8;
            i += 1;
        }
        expect_eq(isValidKey(longKey[0..255]), true);
        expect_eq(isValidKey(longKey[0..256]), false);
    });

    test("4. @cap/idempotency — isUuidV4Like canonical 8-4-4-4-12 hex validation", () => {
        expect_eq(isUuidV4Like("550e8400-e29b-41d4-a716-446655440000"), true);
        expect_eq(isUuidV4Like("550E8400-E29B-41D4-A716-446655440000"), true);
        expect_eq(isUuidV4Like("550e8400-e29b-41d4-a716-44665544000"), false);
        expect_eq(isUuidV4Like("550e8400_e29b-41d4-a716-446655440000"), false);
        expect_eq(isUuidV4Like("550e8400-e29b-41d4-a716-44665544000g"), false);
    });

    test("5. @cap/idempotency — fingerprintRequest determinism & sensitivity", () => {
        const fp1 = fingerprintRequest("POST", "/v1/charges", "{\"amount\":100}");
        const fp2 = fingerprintRequest("POST", "/v1/charges", "{\"amount\":100}");
        const fpDiffBody = fingerprintRequest("POST", "/v1/charges", "{\"amount\":200}");
        const fpDiffPath = fingerprintRequest("POST", "/v1/refunds", "{\"amount\":100}");
        const fpBoundary = fingerprintRequest("POST", "/v1/charges{\"amount\":100}", "");

        expect_eq(fp1, fp2);
        expect_eq(fp1 != fpDiffBody, true);
        expect_eq(fp1 != fpDiffPath, true);
        expect_eq(fp1 != fpBoundary, true);
    });

    test("6. @cap/idempotency — evaluateState 4-state transition matrix", () => {
        const fpA = fingerprintRequest("POST", "/v1/orders", "{\"id\":1}");
        const fpB = fingerprintRequest("POST", "/v1/orders", "{\"id\":2}");

        expect_eq(evaluateState(false, false, 0 as u64, fpA), IDEMP_NEW);
        expect_eq(evaluateState(true, false, fpA, fpA), IDEMP_REPLAY);
        expect_eq(evaluateState(true, true, fpA, fpA), IDEMP_IN_FLIGHT);
        expect_eq(evaluateState(true, false, fpA, fpB), IDEMP_CONFLICT);
        expect_eq(evaluateState(true, true, fpA, fpB), IDEMP_CONFLICT);
    });
}
