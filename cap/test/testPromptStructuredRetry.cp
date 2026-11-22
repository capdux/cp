// Origin: Pure-CP Stdlib Test Suite (lib/prompt/structuredRetry.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Structured Output Self-Healing Retry Controller:
//   1. First-attempt success (`accept`), attempt record inspection & `reset()`
//   2. Syntax/schema failure stepped temperature escalation & targeted `formatFeedbackPrompt`
//   3. `truncated_output` automatic `max_tokens` boost (`token_boost_factor_pct`) capped at `max_tokens_cap`
//   4. Consecutive identical error hash circuit breaker (`abort_identical_loop`)
//   5. Escalation to `retry_fallback_model` after `fallback_after_attempt` failures
//   6. `model_refusal` immediate `abort_refusal` & budget exhaustion (`abort_exhausted`)
//   7. Undersized feedback buffer safety, `StructuredRetryStats` telemetry & `cap.prompt` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    STRUCT_RETRY_MAX_ATTEMPTS,
    STRUCT_RETRY_MSG_MAX_LEN,
    RetryFailureKind,
    RetryDirectiveAction,
    StructuredRetryConfig,
    StructuredRetryController,
} from "cap:prompt";

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (haystack[i] != prefix[i]) return false;
    }
    return true;
}

fn endsWith(haystack: []const u8, suffix: []const u8): bool {
    if (haystack.len < suffix.len) return false;
    const off: usize = haystack.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (haystack[off + i] != suffix[i]) return false;
    }
    return true;
}

test("StructuredRetry: first-attempt success & reset") {
    let ctrl = StructuredRetryController.init(StructuredRetryConfig.defaults());
    expect_true(ctrl.lastAttempt() == null);

    const decision = ctrl.recordSuccess();
    expect_true(decision.action == RetryDirectiveAction.accept);
    expect_eq(decision.next_attempt, @as(u8, 1));
    expect_eq(decision.next_temperature_milli, @as(u16, 200));
    expect_eq(decision.next_max_tokens, @as(u32, 1024));
    expect_true(!decision.use_fallback_model);

    const st = ctrl.stats();
    expect_eq(st.total_attempts, @as(u32, 1));
    expect_true(st.succeeded);

    ctrl.reset();
    expect_eq(ctrl.stats().total_attempts, @as(u32, 0));
    expect_true(!ctrl.stats().succeeded);
    expect_true(ctrl.lastAttempt() == null);
}

test("StructuredRetry: stepped temperature escalation & targeted feedback prompt") {
    let cfg = StructuredRetryConfig.defaults();
    cfg.initial_temp_milli = 200;
    cfg.temp_step_milli = 250;
    cfg.max_temp_milli = 600;
    cfg.fallback_after_attempt = 0; // disable fallback to isolate temperature stepping
    let ctrl = StructuredRetryController.init(cfg);

    // Attempt 1: json_syntax -> temp 200 + 250 = 450
    const d1 = ctrl.recordFailure(RetryFailureKind.json_syntax, "unexpected trailing comma at col 14");
    expect_true(d1.action == RetryDirectiveAction.retry_with_feedback);
    expect_eq(d1.next_attempt, @as(u8, 2));
    expect_eq(d1.next_temperature_milli, @as(u16, 450));

    // Attempt 2: missing_field -> temp 450 + 250 -> capped at 600
    const d2 = ctrl.recordFailure(RetryFailureKind.missing_field, "summary");
    expect_true(d2.action == RetryDirectiveAction.retry_with_feedback);
    expect_eq(d2.next_attempt, @as(u8, 3));
    expect_eq(d2.next_temperature_milli, @as(u16, 600));

    // Format targeted feedback prompts
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const n1 = ctrl.formatFeedbackPrompt("summary", RetryFailureKind.missing_field, *buf_s) ?? 0;
    expect_true(n1 > 0);
    expect_true(startsWith(buf[0..n1], "[Fix Missing Field]"));
    expect_true(endsWith(buf[0..n1], "summary"));

    // Attempt 3 succeeds
    const d3 = ctrl.recordSuccess();
    expect_true(d3.action == RetryDirectiveAction.accept);
    const st = ctrl.stats();
    expect_eq(st.total_attempts, @as(u32, 3));
    expect_eq(st.syntax_failures, @as(u32, 1));
    expect_eq(st.schema_failures, @as(u32, 1));
    expect_true(st.succeeded);
}

test("StructuredRetry: truncated_output auto-boosts max_tokens up to cap") {
    let cfg = StructuredRetryConfig.defaults();
    cfg.initial_max_tokens = 1000;
    cfg.token_boost_factor_pct = 150;
    cfg.max_tokens_cap = 2000;
    cfg.max_identical_errors = 0; // allow distinct/repeated truncations
    let ctrl = StructuredRetryController.init(cfg);

    // 1000 * 1.5 = 1500
    const d1 = ctrl.recordFailure(RetryFailureKind.truncated_output, "hit stop_reason=length at token 1000");
    expect_true(d1.action == RetryDirectiveAction.retry_increase_max_tokens);
    expect_eq(d1.next_max_tokens, @as(u32, 1500));

    // 1500 * 1.5 = 2250 -> capped at 2000
    const d2 = ctrl.recordFailure(RetryFailureKind.truncated_output, "hit stop_reason=length at token 1500");
    expect_true(d2.action == RetryDirectiveAction.retry_increase_max_tokens);
    expect_eq(d2.next_max_tokens, @as(u32, 2000));
    expect_eq(ctrl.stats().truncations, @as(u32, 2));
}

test("StructuredRetry: identical error hash triggers abort_identical_loop") {
    let cfg = StructuredRetryConfig.defaults();
    cfg.max_identical_errors = 2;
    cfg.max_attempts = 5;
    let ctrl = StructuredRetryController.init(cfg);

    const d1 = ctrl.recordFailure(RetryFailureKind.type_mismatch, "field 'age' expected integer, got string");
    expect_true(d1.action == RetryDirectiveAction.retry_with_feedback);
    expect_eq(d1.consecutive_same_error, @as(u8, 1));

    // Exact same error detail & kind -> 2nd consecutive hit trips circuit breaker
    const d2 = ctrl.recordFailure(RetryFailureKind.type_mismatch, "field 'age' expected integer, got string");
    expect_true(d2.action == RetryDirectiveAction.abort_identical_loop);
    expect_eq(d2.consecutive_same_error, @as(u8, 2));
    expect_eq(ctrl.stats().identical_loop_aborts, @as(u32, 1));
}

test("StructuredRetry: fallback model escalation after threshold") {
    let cfg = StructuredRetryConfig.defaults();
    cfg.max_attempts = 5;
    cfg.fallback_after_attempt = 2;
    let ctrl = StructuredRetryController.init(cfg);

    const d1 = ctrl.recordFailure(RetryFailureKind.json_syntax, "missing closing brace");
    expect_true(d1.action == RetryDirectiveAction.retry_with_feedback);
    expect_true(!d1.use_fallback_model);

    // 2nd distinct failure reaches fallback_after_attempt=2 -> escalates to fallback model
    const d2 = ctrl.recordFailure(RetryFailureKind.constraint_violation, "confidence must be in [0, 1]");
    expect_true(d2.action == RetryDirectiveAction.retry_fallback_model);
    expect_true(d2.use_fallback_model);

    // Verify lastAttempt stored detail
    if (rec := ctrl.lastAttempt()) {
        expect_eq(rec.attempt_no, @as(u8, 2));
        expect_true(startsWith(rec.detail(), "confidence must be"));
    } else {
        expect_true(false);
    }
}

test("StructuredRetry: model_refusal immediate abort & max_attempts exhaustion") {
    // 1. Immediate abort on refusal
    let ctrl_ref = StructuredRetryController.init(StructuredRetryConfig.defaults());
    const d_ref = ctrl_ref.recordFailure(RetryFailureKind.model_refusal, "policy declined request");
    expect_true(d_ref.action == RetryDirectiveAction.abort_refusal);
    expect_eq(ctrl_ref.stats().total_attempts, @as(u32, 1));

    // 2. Exhaustion when reaching max_attempts = 2
    let cfg = StructuredRetryConfig.defaults();
    cfg.max_attempts = 2;
    cfg.fallback_after_attempt = 0;
    let ctrl_ex = StructuredRetryController.init(cfg);
    const e1 = ctrl_ex.recordFailure(RetryFailureKind.json_syntax, "err 1");
    expect_true(e1.action == RetryDirectiveAction.retry_with_feedback);
    const e2 = ctrl_ex.recordFailure(RetryFailureKind.missing_field, "err 2");
    expect_true(e2.action == RetryDirectiveAction.abort_exhausted);
    expect_eq(ctrl_ex.stats().total_attempts, @as(u32, 2));
}

test("StructuredRetry: tiny feedback buffer safety & cap.prompt facade parity") {
    const ctrl = StructuredRetryController.init(StructuredRetryConfig.defaults());
    let tiny: [12]u8 = [0; 12];
    let tiny_s: []u8 = tiny[0..];
    expect_true(ctrl.formatFeedbackPrompt("unexpected EOF", RetryFailureKind.json_syntax, *tiny_s) == null);

    // `cap.prompt` facade parity
    expect_eq(cap.prompt.STRUCT_RETRY_MAX_ATTEMPTS, @as(usize, 8));
    expect_eq(cap.prompt.STRUCT_RETRY_MSG_MAX_LEN, @as(usize, 160));
    expect_eq(STRUCT_RETRY_MAX_ATTEMPTS, @as(usize, 8));
    expect_eq(STRUCT_RETRY_MSG_MAX_LEN, @as(usize, 160));
}
