// Origin: Pure-CP Stdlib Test Suite (lib/mcp/rateQuota.cp) | Created: 2026-11-23
// Production Deep Verification Suite for MCP Dual-Layer Rate Limiter & Cost Quota Gate:
//   1. Burst admission, token exhaustion denial & exact millisecond `retry_after_ms` recovery
//   2. Tool-level & session-level concurrency gating + `release()` slot restoration
//   3. Cost budget exhaustion (`denied_cost_budget`) & failed-call `refundCost()`
//   4. Strict zero-side-effect atomicity on every denial verdict
//   5. Unknown session guard, unregistered tool fallback, `stats()` & `cap.mcp` facade parity
//   6. JSON-RPC `-32029` error payload formatting (`formatQuotaErrorJson`) & buffer guard

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_QUOTA_MILLI_SCALE,
    McpQuotaVerdict,
    formatQuotaErrorJson,
    McpRateQuotaEngine,
} from "cap:mcp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("McpRateQuota: burst admission, token exhaustion & exact millisecond retry_after_ms recovery") {
    let eng = McpRateQuotaEngine.init();
    // Tool burst=2, refill=2/sec (1 token every 500ms), cost=100, max_conc=10
    * = eng.registerToolRule("search", 2, 2, 100, 10);
    // Session global burst=10, refill=10/sec, max_cost=10_000, max_conc=10
    * = eng.registerSession("s1", 10_000, 10, 10, 10, 1000);

    const d1 = eng.acquire("s1", "search", 1000);
    expect_eq(d1.verdict, McpQuotaVerdict.allowed);
    expect_eq(d1.remaining_tokens, @as(u32, 1));
    * = eng.release("s1", "search");

    const d2 = eng.acquire("s1", "search", 1000);
    expect_eq(d2.verdict, McpQuotaVerdict.allowed);
    expect_eq(d2.remaining_tokens, @as(u32, 0));
    * = eng.release("s1", "search");

    // 3rd call at t=1000 -> tool bucket empty -> denied_rate_limit with exact retry_after_ms = 500
    const d3 = eng.acquire("s1", "search", 1000);
    expect_eq(d3.verdict, McpQuotaVerdict.denied_rate_limit);
    expect_eq(d3.retry_after_ms, @as(u64, 500));
    expect_eq(d3.charged_cost_micros, @as(u32, 0));

    // At t=1200 (+200ms -> 400 milli-tokens) -> still denied, exact retry_after_ms = 300
    const d4 = eng.acquire("s1", "search", 1200);
    expect_eq(d4.verdict, McpQuotaVerdict.denied_rate_limit);
    expect_eq(d4.retry_after_ms, @as(u64, 300));

    // At t=1500 (+500ms -> 1000 milli-tokens) -> allowed!
    const d5 = eng.acquire("s1", "search", 1500);
    expect_eq(d5.verdict, McpQuotaVerdict.allowed);
    expect_eq(d5.charged_cost_micros, @as(u32, 100));
    * = eng.release("s1", "search");
}

test("McpRateQuota: tool-level and session-level concurrency gating & release") {
    let eng = McpRateQuotaEngine.init();
    * = eng.registerToolRule("heavy_tool", 10, 10, 50, 1); // tool max_concurrent = 1
    * = eng.registerToolRule("light_tool", 10, 10, 10, 5); // tool max_concurrent = 5
    * = eng.registerSession("s1", 10_000, 20, 20, 2, 1000); // session max_concurrent = 2

    // Acquire heavy_tool (1/1 tool, 1/2 session) -> allowed
    expect_eq(eng.acquire("s1", "heavy_tool", 1000).verdict, McpQuotaVerdict.allowed);

    // Second concurrent heavy_tool -> denied by tool max_concurrent=1
    expect_eq(eng.acquire("s1", "heavy_tool", 1000).verdict, McpQuotaVerdict.denied_concurrency);

    // Acquire light_tool (1/5 tool, 2/2 session) -> allowed
    expect_eq(eng.acquire("s1", "light_tool", 1000).verdict, McpQuotaVerdict.allowed);

    // Third call on light_tool -> denied by session max_concurrent=2
    expect_eq(eng.acquire("s1", "light_tool", 1000).verdict, McpQuotaVerdict.denied_concurrency);

    // Release heavy_tool -> both tool and session slots freed
    expect_true(eng.release("s1", "heavy_tool"));
    expect_eq(eng.acquire("s1", "heavy_tool", 1000).verdict, McpQuotaVerdict.allowed);
}

test("McpRateQuota: cost budget exhaustion & failed-call refundCost") {
    let eng = McpRateQuotaEngine.init();
    * = eng.registerToolRule("llm_call", 10, 10, 400, 10);
    * = eng.registerSession("s1", 1000, 10, 10, 10, 1000);

    // 1st call: 400 spent, 600 remaining
    const d1 = eng.acquire("s1", "llm_call", 1000);
    expect_eq(d1.verdict, McpQuotaVerdict.allowed);
    expect_eq(d1.remaining_cost_micros, @as(u64, 600));
    * = eng.release("s1", "llm_call");

    // 2nd call: 800 spent, 200 remaining
    const d2 = eng.acquire("s1", "llm_call", 1000);
    expect_eq(d2.verdict, McpQuotaVerdict.allowed);
    expect_eq(d2.remaining_cost_micros, @as(u64, 200));
    * = eng.release("s1", "llm_call");

    // 3rd call: 800 + 400 > 1000 -> denied_cost_budget
    const d3 = eng.acquire("s1", "llm_call", 1000);
    expect_eq(d3.verdict, McpQuotaVerdict.denied_cost_budget);
    expect_eq(d3.remaining_cost_micros, @as(u64, 200));

    // Refund 400 micros from failed 2nd call -> remaining becomes 600 -> 3rd call now succeeds!
    expect_true(eng.refundCost("s1", 400));
    const d4 = eng.acquire("s1", "llm_call", 1000);
    expect_eq(d4.verdict, McpQuotaVerdict.allowed);
    expect_eq(d4.remaining_cost_micros, @as(u64, 200));
}

test("McpRateQuota: zero side effects on denial (tokens, cost & inflight untouched)") {
    let eng = McpRateQuotaEngine.init();
    * = eng.registerToolRule("expensive", 5, 1, 900, 5);
    * = eng.registerSession("s1", 500, 5, 1, 5, 1000); // budget 500 < tool cost 900

    // Denied by cost budget: tokens must remain untouched at full 5000 milli-tokens!
    const d_cost = eng.acquire("s1", "expensive", 1000);
    expect_eq(d_cost.verdict, McpQuotaVerdict.denied_cost_budget);

    if (sess := eng.getSession("s1")) {
        expect_eq(sess.global_tokens_milli, @as(u64, 5) * MCP_QUOTA_MILLI_SCALE);
        expect_eq(sess.spent_cost_micros, @as(u64, 0));
        expect_eq(sess.active_calls, @as(u16, 0));
    } else {
        expect_true(false);
    }
    if (rule := eng.getToolRule("expensive")) {
        expect_eq(rule.tokens_milli, @as(u64, 5) * MCP_QUOTA_MILLI_SCALE);
        expect_eq(rule.active_calls, @as(u16, 0));
    } else {
        expect_true(false);
    }
}

test("McpRateQuota: unknown session, unregistered tool fallback, stats & cap.mcp facade parity") {
    let eng = cap.mcp.McpRateQuotaEngine.init();

    // Unknown session rejected cleanly
    const d_unk = eng.acquire("no-such-session", "any", 1000);
    expect_eq(d_unk.verdict, cap.mcp.McpQuotaVerdict.denied_unknown_session);
    expect_true(!eng.release("no-such-session", "any"));
    expect_true(!eng.refundCost("no-such-session", 100));

    // Unregistered tool uses session-global limits with 0 cost
    * = eng.registerSession("s-free", 0, 2, 1, 2, 1000);
    const d_ok = eng.acquire("s-free", "unregistered_tool", 1000);
    expect_eq(d_ok.verdict, cap.mcp.McpQuotaVerdict.allowed);
    expect_eq(d_ok.charged_cost_micros, @as(u32, 0));

    const st = eng.stats();
    expect_eq(st.total_checks, @as(u64, 2));
    expect_eq(st.allowed_count, @as(u64, 1));
    expect_eq(st.total_cost_micros_charged, @as(u64, 0));
}

test("McpRateQuota: JSON-RPC -32029 error payload formatting & small-buffer guard") {
    let eng = McpRateQuotaEngine.init();
    * = eng.registerToolRule("ping", 1, 2, 50, 4);
    * = eng.registerSession("s1", 1000, 10, 10, 4, 1000);

    * = eng.acquire("s1", "ping", 1000);
    * = eng.release("s1", "ping");
    const denied = eng.acquire("s1", "ping", 1000);
    expect_eq(denied.verdict, McpQuotaVerdict.denied_rate_limit);

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const n = formatQuotaErrorJson(denied, *buf_s) ?? 0;
    expect_true(n > 0);
    const json: []const u8 = buf[0..n];
    expect_true(containsSubstr(json, "\"code\":-32029"));
    expect_true(containsSubstr(json, "\"verdict\":\"denied_rate_limit\""));
    expect_true(containsSubstr(json, "\"retryAfterMs\":500"));
    expect_true(containsSubstr(json, "\"remainingCostMicros\":950"));

    // Small buffer returns null safely
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatQuotaErrorJson(denied, *tiny_s) == null);
}
