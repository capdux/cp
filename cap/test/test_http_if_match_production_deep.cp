import {
    EntityTag,
    etagListMatches,
    PreconditionResult,
    ConditionalRequest,
    evaluatePreconditions,
} from "cap:http_if_match";
import { test, expect } from "cap:test";

test("EntityTag strong vs weak comparison and etagListMatches", () => {
    const s1 = EntityTag.parse("\"v1\"").?;
    const w1 = EntityTag.parse("W/\"v1\"").?;
    expect(!s1.strongEquals(w1));
    expect(s1.weakEquals(w1));
    expect(s1.strongEquals(EntityTag.parse("\"v1\"").?));

    // Strong mode (If-Match) rejects weak ETags
    expect(!etagListMatches("W/\"v1\"", "\"v1\"", true));
    expect(etagListMatches("\"v0\", \"v1\"", "\"v1\"", true));
    // Weak mode (If-None-Match) accepts weak ETags
    expect(etagListMatches("W/\"v1\"", "\"v1\"", false));
    // Wildcard '*' matches any existing resource, fails if resource absent
    expect(etagListMatches("*", "\"v1\"", true));
    expect(!etagListMatches("*", null, true));
});

test("evaluatePreconditions follows RFC 9110 §13.2.2 precedence", () => {
    // 1. GET with matching If-None-Match -> 304
    let r1 = ConditionalRequest.empty(true);
    r1.if_none_match = "W/\"abc\"";
    expect(evaluatePreconditions(r1, "\"abc\"", 1000) == PreconditionResult.not_modified_304);

    // 2. PUT with matching If-None-Match: * (create-if-absent) -> 412 when resource exists
    let r2 = ConditionalRequest.empty(false);
    r2.if_none_match = "*";
    expect(evaluatePreconditions(r2, "\"abc\"", 1000) == PreconditionResult.precondition_failed_412);
    expect(evaluatePreconditions(r2, null, null) == PreconditionResult.proceed);

    // 3. PUT with mismatched If-Match -> 412
    let r3 = ConditionalRequest.empty(false);
    r3.if_match = "\"old\"";
    expect(evaluatePreconditions(r3, "\"new\"", 1000) == PreconditionResult.precondition_failed_412);

    // 4. GET with If-Modified-Since -> 304 if last_modified <= ims, else proceed
    let r4 = ConditionalRequest.empty(true);
    r4.if_modified_since_sec = 2000;
    expect(evaluatePreconditions(r4, null, 1500) == PreconditionResult.not_modified_304);
    expect(evaluatePreconditions(r4, null, 2500) == PreconditionResult.proceed);

    // 5. If-None-Match overrides If-Modified-Since (RFC 9110 §13.1.3)
    let r5 = ConditionalRequest.empty(true);
    r5.if_none_match = "\"different\"";
    r5.if_modified_since_sec = 5000;
    expect(evaluatePreconditions(r5, "\"current\"", 1000) == PreconditionResult.proceed);
});
