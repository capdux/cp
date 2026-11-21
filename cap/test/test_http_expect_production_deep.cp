import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ExpectIter,
    ExpectDecision,
    hasContinueExpectation,
    hasUnknownExpectation,
    evaluateExpectHeader,
} from "cap:http";

test("1. ExpectIter parses comma-separated items, quoted commas, and empty entries", () => {
    const hdr = " , 100-Continue , ext-a=42 , ext-b=\"hello, \\\"world\\\"\" , ";
    let it = ExpectIter.init(hdr);

    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.token, "100-Continue"), true);
    expect_eq(e0.value == null, true);
    expect_eq(e0.isContinue(), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.token, "ext-a"), true);
    expect_eq(std.mem.eql(u8, e1.value ?? unreachable, "42"), true);
    expect_eq(e1.isContinue(), false);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.token, "ext-b"), true);
    expect_eq(std.mem.eql(u8, e2.value ?? unreachable, "hello, \\\"world\\\""), true);
    expect_eq(e2.isContinue(), false);

    expect_eq(it.next() == null, true);
});

test("2. evaluateExpectHeader follows RFC 9110 §10.1.1 for HTTP/1.1+ vs HTTP/1.0 and body presence", () => {
    // HTTP/1.1+ with body and 100-continue -> send_100_continue
    expect_eq(evaluateExpectHeader("100-continue", true, true) == ExpectDecision.send_100_continue, true);
    expect_eq(evaluateExpectHeader("  100-CONTINUE , 100-continue ", true, true) == ExpectDecision.send_100_continue, true);

    // No request body -> none
    expect_eq(evaluateExpectHeader("100-continue", false, true) == ExpectDecision.none, true);

    // HTTP/1.0 -> ignore 100-continue, return none
    expect_eq(evaluateExpectHeader("100-continue", true, false) == ExpectDecision.none, true);

    // Empty or comma-only header -> none
    expect_eq(evaluateExpectHeader("", true, true) == ExpectDecision.none, true);
    expect_eq(evaluateExpectHeader(" , , \t ", true, true) == ExpectDecision.none, true);
});

test("3. evaluateExpectHeader returns 417 on any unknown or parameterized expectation", () => {
    expect_eq(evaluateExpectHeader("200-ok", true, true) == ExpectDecision.expectation_failed_417, true);
    expect_eq(
        evaluateExpectHeader("100-continue, custom-ext", true, true) == ExpectDecision.expectation_failed_417,
        true,
    );
    // 100-continue with unexpected value is non-standard -> 417
    expect_eq(
        evaluateExpectHeader("100-continue=1", true, true) == ExpectDecision.expectation_failed_417,
        true,
    );
    // Unknown expectation even on HTTP/1.0 without body -> 417
    expect_eq(
        evaluateExpectHeader("unknown-token", false, false) == ExpectDecision.expectation_failed_417,
        true,
    );
});

test("4. hasContinueExpectation and hasUnknownExpectation classify headers accurately", () => {
    expect_eq(hasContinueExpectation("100-continue"), true);
    expect_eq(hasUnknownExpectation("100-continue"), false);

    expect_eq(hasContinueExpectation("100-continue, x-foo=bar"), true);
    expect_eq(hasUnknownExpectation("100-continue, x-foo=bar"), true);

    expect_eq(hasContinueExpectation("x-foo=bar"), false);
    expect_eq(hasUnknownExpectation("x-foo=bar"), true);

    expect_eq(hasContinueExpectation(""), false);
    expect_eq(hasUnknownExpectation(""), false);
});

test("5. Malformed headers with unclosed quotes or empty token fail closed with 417", () => {
    expect_eq(
        evaluateExpectHeader("ext=\"unterminated", true, true) == ExpectDecision.expectation_failed_417,
        true,
    );
    expect_eq(
        evaluateExpectHeader("=value", true, true) == ExpectDecision.expectation_failed_417,
        true,
    );
    expect_eq(hasUnknownExpectation("ext=\"unterminated"), true);
});
