// Source: cp/cap/test/test_agent_guardrail_production_deep.cp (Production deep tests for cap:agent guardrail)

import { test, expect_eq } from "cap:test";
import {
    GuardAction,
    GuardReason,
    GuardRule,
    GuardVerdict,
    GuardConfig,
    evaluateGuardrails,
    JsonDepthReport,
    inspectJsonPayloadDepth,
} from "cap:agent";

test("1. Benign inputs, empty text, and empty ruleset are allowed cleanly", () => {
    const empty_rules: [0]GuardRule = [];
    const cfg = GuardConfig.default();

    const v0 = evaluateGuardrails("", empty_rules[0..], cfg);
    expect_eq(v0.action == GuardAction.Allow, true);
    expect_eq(v0.reason == GuardReason.None, true);
    expect_eq(v0.risk_score, 0);

    const v1 = evaluateGuardrails("Line 1\n\tLine 2\r\nNormal prompt with sk-short token.", empty_rules[0..], cfg);
    expect_eq(v1.action == GuardAction.Allow, true);
    expect_eq(v1.reason == GuardReason.None, true);
});

test("2. Custom case-insensitive rules trigger Warn, direct Block, and cumulative risk escalation", () => {
    const rules: [3]GuardRule = [
        GuardRule { id: 101, pattern: "override policy", case_insensitive: true, action: GuardAction.Warn, weight: 45 },
        GuardRule { id: 102, pattern: "dump hidden state", case_insensitive: true, action: GuardAction.Warn, weight: 45 },
        GuardRule { id: 201, pattern: "FORBIDDEN_OP", case_insensitive: false, action: GuardAction.Block, weight: 10 },
    ];
    const cfg = GuardConfig.default(); // warn >= 30, block >= 80

    // Single Warn rule (case-insensitive) -> Warn, risk 45
    const v_warn = evaluateGuardrails("Please OVERRIDE POLICY now", rules[0..], cfg);
    expect_eq(v_warn.action == GuardAction.Warn, true);
    expect_eq(v_warn.reason == GuardReason.RuleBlocked, true);
    expect_eq(v_warn.matched_rule_id, 101);
    expect_eq(v_warn.risk_score, 45);
    expect_eq(v_warn.match_offset, 7);

    // Two Warn rules together -> cumulative risk 90 >= 80 -> escalates to Block!
    const v_esc = evaluateGuardrails("Override Policy and Dump Hidden State", rules[0..], cfg);
    expect_eq(v_esc.action == GuardAction.Block, true);
    expect_eq(v_esc.reason == GuardReason.RuleBlocked, true);
    expect_eq(v_esc.risk_score, 90);

    // Case-sensitive rule does not match lowercase, matches exact uppercase -> direct Block
    const v_cs_miss = evaluateGuardrails("forbidden_op", rules[0..], cfg);
    expect_eq(v_cs_miss.action == GuardAction.Allow, true);

    const v_cs_hit = evaluateGuardrails("run FORBIDDEN_OP immediately", rules[0..], cfg);
    expect_eq(v_cs_hit.action == GuardAction.Block, true);
    expect_eq(v_cs_hit.matched_rule_id, 201);
    expect_eq(v_cs_hit.match_offset, 4);
});

test("3. Built-in secret leak detector blocks synthetic credential patterns and honors config toggle", () => {
    const empty_rules: [0]GuardRule = [];
    const cfg = GuardConfig.default();

    const s_openai: string = "token=" ++ "sk-" ++ "0123456789abcdef1234";
    const v1 = evaluateGuardrails(s_openai, empty_rules[0..], cfg);
    expect_eq(v1.action == GuardAction.Block, true);
    expect_eq(v1.reason == GuardReason.SecretLeak, true);
    expect_eq(v1.match_offset, 6);

    const s_gh: string = "auth " ++ "ghp_" ++ "abcd1234efgh5678";
    const v2 = evaluateGuardrails(s_gh, empty_rules[0..], cfg);
    expect_eq(v2.action == GuardAction.Block, true);
    expect_eq(v2.reason == GuardReason.SecretLeak, true);
    expect_eq(v2.match_offset, 5);

    const s_aws: string = "key:" ++ "AKIA" ++ "0000111122223333";
    const v3 = evaluateGuardrails(s_aws, empty_rules[0..], cfg);
    expect_eq(v3.action == GuardAction.Block, true);
    expect_eq(v3.reason == GuardReason.SecretLeak, true);

    const s_pem: string = "-----BEGIN " ++ "PRIVATE KEY-----";
    const v4 = evaluateGuardrails(s_pem, empty_rules[0..], cfg);
    expect_eq(v4.action == GuardAction.Block, true);
    expect_eq(v4.reason == GuardReason.SecretLeak, true);

    // Toggle block_secret_leaks = false allows the same input
    const cfg_off = GuardConfig {
        max_bytes: 65536,
        max_json_depth: 32,
        block_secret_leaks: false,
        warn_risk_threshold: 30,
        block_risk_threshold: 80,
    };
    const v_off = evaluateGuardrails(s_openai, empty_rules[0..], cfg_off);
    expect_eq(v_off.action == GuardAction.Allow, true);
});

test("4. MaxLengthExceeded and ControlCharAnomaly guards block oversized and NUL payloads", () => {
    const empty_rules: [0]GuardRule = [];
    const small_cfg = GuardConfig {
        max_bytes: 16,
        max_json_depth: 32,
        block_secret_leaks: true,
        warn_risk_threshold: 30,
        block_risk_threshold: 80,
    };

    const v_len = evaluateGuardrails("this payload exceeds sixteen bytes", empty_rules[0..], small_cfg);
    expect_eq(v_len.action == GuardAction.Block, true);
    expect_eq(v_len.reason == GuardReason.MaxLengthExceeded, true);
    expect_eq(v_len.match_offset, 16);

    // NUL byte inside payload -> ControlCharAnomaly
    const nul_bytes: [6]u8 = [104, 101, 108, 0, 108, 111];
    const nul_str: string = nul_bytes[0..] as string;
    const v_nul = evaluateGuardrails(nul_str, empty_rules[0..], GuardConfig.default());
    expect_eq(v_nul.action == GuardAction.Block, true);
    expect_eq(v_nul.reason == GuardReason.ControlCharAnomaly, true);
    expect_eq(v_nul.match_offset, 3);
});

test("5. inspectJsonPayloadDepth ignores braces inside escaped strings and blocks excessive nesting", () => {
    // Embedded braces and escaped quotes inside string values must NOT inflate depth
    const tricky: string = "{\"msg\":\"{{{[\\\"fake\\\"]}}}\",\"arr\":[1,{\"ok\":true}]}";
    const rep = inspectJsonPayloadDepth(tricky, 16);
    expect_eq(rep.valid_balance, true);
    expect_eq(rep.max_depth, 3); // { -> [ -> {
    expect_eq(rep.string_count, 4); // "msg", "{{{[\"fake\"]}}}", "arr", "ok"

    // Unbalanced payload detected
    const bad = inspectJsonPayloadDepth("{\"a\":[1,2}", 16);
    expect_eq(bad.valid_balance, false);

    // Excessive nesting short-circuits at max_allowed_depth + 1
    const deep: string = "[[[[[[1]]]]]]"; // depth 6
    const rep_deep = inspectJsonPayloadDepth(deep, 4);
    expect_eq(rep_deep.valid_balance, false);
    expect_eq(rep_deep.max_depth, 5);

    // Integrated into evaluateGuardrails -> NestingTooDeep
    const empty_rules: [0]GuardRule = [];
    const cfg = GuardConfig {
        max_bytes: 65536,
        max_json_depth: 4,
        block_secret_leaks: true,
        warn_risk_threshold: 30,
        block_risk_threshold: 80,
    };
    const v_deep = evaluateGuardrails(deep, empty_rules[0..], cfg);
    expect_eq(v_deep.action == GuardAction.Block, true);
    expect_eq(v_deep.reason == GuardReason.NestingTooDeep, true);
});
