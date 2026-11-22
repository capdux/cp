// Origin: Pure-CP Stdlib Test Suite (lib/agent/tracer.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent hierarchical span tracer
// and GenAI JSON telemetry exporter: full nested AgentRun -> Retrieval / LlmCall / ToolExec /
// Guardrail span trees, accurate TraceRollup aggregation, RFC-8259 escaped JSON export,
// clock-skew clamping, and defensive invalid parent / double-end / pool-full / small-buffer guards.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    NO_PARENT_SPAN,
    AgentSpanKind,
    AgentSpanStatus,
    AgentSpan,
    TraceRollup,
    AgentTracer,
} from "cap:agent";

fn containsSubstr(haystack: []const u8, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn testFullHierarchicalSpanTreeAndRollup()! {
    let pool_arr: [8]AgentSpan = undefined;
    let pool: []AgentSpan = pool_arr[0..];
    let tr = AgentTracer.init(20261122, pool);

    // Root AgentRun: t=1000..1850
    const root_id = tr.startSpan(NO_PARENT_SPAN, AgentSpanKind.AgentRun, "rag_answer", "agent_v1", 1000) ?? 0;
    expect_eq(root_id > 0, true);

    // Child 1: Retrieval t=1010..1110
    const ret_id = tr.startSpan(root_id, AgentSpanKind.Retrieval, "vector_search", "hnsw_idx", 1010) ?? 0;
    expect_eq(tr.endSpan(ret_id, AgentSpanStatus.Ok, 0, 0, 50, 1110), true);

    // Child 2: LlmCall #1 t=1120..1420 (prompt=500, completion=120, cost=2500)
    const llm1_id = tr.startSpan(root_id, AgentSpanKind.LlmCall, "plan_tool", "claude-3.7", 1120) ?? 0;
    expect_eq(tr.endSpan(llm1_id, AgentSpanStatus.Ok, 500, 120, 2500, 1420), true);

    // Child 3: ToolExec (under llm1) t=1430..1580 -> Error
    const tool_id = tr.startSpan(llm1_id, AgentSpanKind.ToolExec, "sql_query", "postgres_ro", 1430) ?? 0;
    expect_eq(tr.endSpan(tool_id, AgentSpanStatus.Error, 0, 0, 10, 1580), true);

    // Child 4: LlmCall #2 t=1600..1780 (prompt=700, completion=180, cost=3400)
    const llm2_id = tr.startSpan(root_id, AgentSpanKind.LlmCall, "final_reply", "claude-3.7", 1600) ?? 0;
    expect_eq(tr.endSpan(llm2_id, AgentSpanStatus.Ok, 700, 180, 3400, 1780), true);

    // Child 5: Guardrail t=1790..1840
    const grd_id = tr.startSpan(root_id, AgentSpanKind.Guardrail, "pii_check", "policy_v2", 1790) ?? 0;
    expect_eq(tr.endSpan(grd_id, AgentSpanStatus.Ok, 0, 0, 0, 1840), true);

    // Finish root at t=1850
    expect_eq(tr.endSpan(root_id, AgentSpanStatus.Ok, 0, 0, 0, 1850), true);

    const r = tr.rollup();
    expect_eq(r.total_spans, 6);
    expect_eq(r.error_spans, 1);
    expect_eq(r.llm_calls, 2);
    expect_eq(r.tool_calls, 1);
    expect_eq(r.total_prompt_tokens, 1200);
    expect_eq(r.total_completion_tokens, 300);
    expect_eq(r.total_cost_micro_usd, 5960);
    expect_eq(r.total_duration_ms, 850);
}

fn testExportJsonEscapingAndFields()! {
    let pool_arr: [4]AgentSpan = undefined;
    let pool: []AgentSpan = pool_arr[0..];
    let tr = AgentTracer.init(424242, pool);

    // Include quotes, backslashes & newline to verify RFC-8259 string escaping
    const root_id = tr.startSpan(NO_PARENT_SPAN, AgentSpanKind.AgentRun, "run \"quoted\"\\ok", "core\nagent", 500) ?? 0;
    const child_id = tr.startSpan(root_id, AgentSpanKind.Handoff, "delegate", "specialist", 520) ?? 0;
    expect_eq(tr.endSpan(child_id, AgentSpanStatus.Ok, 40, 10, 300, 600), true);
    expect_eq(tr.endSpan(root_id, AgentSpanStatus.Ok, 0, 0, 0, 700), true);

    let json_arr: [2048]u8 = undefined;
    let json_buf: []u8 = json_arr[0..];
    if (n := tr.exportJson(json_buf)) {
        const out = json_buf[0..n];
        expect_eq(out[0], '{');
        expect_eq(out[n - 1], '}');
        expect_eq(containsSubstr(out, "\"trace_id\":424242"), true);
        expect_eq(containsSubstr(out, "\"spans\":["), true);
        expect_eq(containsSubstr(out, "\"parent_id\":null"), true);
        expect_eq(containsSubstr(out, "\"kind\":\"handoff\""), true);
        expect_eq(containsSubstr(out, "\"name\":\"run \\\"quoted\\\"\\\\ok\""), true);
        expect_eq(containsSubstr(out, "\"model_or_tool\":\"core\\nagent\""), true);
        expect_eq(containsSubstr(out, "\"total_duration_ms\":200"), true);
    } else {
        expect_eq(true, false);
    }
}

fn testClockSkewClampAndDoubleEndGuard()! {
    let pool_arr: [4]AgentSpan = undefined;
    let pool: []AgentSpan = pool_arr[0..];
    let tr = AgentTracer.init(77, pool);

    const sid = tr.startSpan(NO_PARENT_SPAN, AgentSpanKind.LlmCall, "skew_test", "m1", 2000) ?? 0;
    // Ending with Running status is rejected
    expect_eq(tr.endSpan(sid, AgentSpanStatus.Running, 10, 5, 100, 2100), false);
    // Backward clock timestamp (1500 < 2000) is clamped to start_ms (duration = 0)
    expect_eq(tr.endSpan(sid, AgentSpanStatus.Ok, 10, 5, 100, 1500), true);
    // Double-ending the same span is rejected
    expect_eq(tr.endSpan(sid, AgentSpanStatus.Error, 99, 99, 999, 3000), false);

    const r = tr.rollup();
    expect_eq(r.total_duration_ms, 0);
    expect_eq(r.error_spans, 0);
    expect_eq(r.total_prompt_tokens, 10);
}

fn testDefensiveGuardsInvalidParentPoolFullAndSmallJsonBuf()! {
    let pool_arr: [2]AgentSpan = undefined;
    let pool: []AgentSpan = pool_arr[0..];
    let tr = AgentTracer.init(88, pool);

    // 1. Empty tracer rollup
    const r0 = tr.rollup();
    expect_eq(r0.total_spans, 0);
    expect_eq(r0.total_duration_ms, 0);

    // 2. Non-existent parent span ID rejected
    expect_eq(tr.startSpan(999, AgentSpanKind.ToolExec, "bad", "t", 100) == null, true);

    // 3. Fill pool (capacity 2) and reject 3rd span
    const s1 = tr.startSpan(NO_PARENT_SPAN, AgentSpanKind.AgentRun, "r1", "a", 100) ?? 0;
    expect_eq(s1 > 0, true);
    const s2 = tr.startSpan(s1, AgentSpanKind.ToolExec, "t1", "b", 110) ?? 0;
    expect_eq(s2 > 0, true);
    expect_eq(tr.startSpan(s1, AgentSpanKind.ToolExec, "t2", "c", 120) == null, true);

    // 4. Unknown span_id in endSpan -> false
    expect_eq(tr.endSpan(777, AgentSpanStatus.Ok, 0, 0, 0, 200), false);

    // 5. Undersized JSON export buffer -> null
    let tiny_arr: [32]u8 = undefined;
    let tiny: []u8 = tiny_arr[0..];
    expect_eq(tr.exportJson(tiny) == null, true);
}

fn testCapAgentNamespaceIntegration()! {
    let pool_arr: [4]cap.agent.AgentSpan = undefined;
    let pool: []cap.agent.AgentSpan = pool_arr[0..];
    let tr = cap.agent.AgentTracer.init(1001, pool);
    if (sid := tr.startSpan(cap.agent.NO_PARENT_SPAN, cap.agent.AgentSpanKind.LlmCall, "ping", "m", 10)) {
        expect_eq(tr.endSpan(sid, cap.agent.AgentSpanStatus.Ok, 12, 8, 50, 60), true);
        const r = tr.rollup();
        expect_eq(r.total_spans, 1);
        expect_eq(r.total_duration_ms, 50);
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. agent/tracer: full hierarchical span tree and exact TraceRollup aggregation", () => {
        testFullHierarchicalSpanTreeAndRollup()?;
    });
    test("2. agent/tracer: RFC-8259 escaped exportJson with trace_id, rollup & spans", () => {
        testExportJsonEscapingAndFields()?;
    });
    test("3. agent/tracer: backward clock clamping and double-end / Running-status guards", () => {
        testClockSkewClampAndDoubleEndGuard()?;
    });
    test("4. agent/tracer: invalid parent ID, pool exhaustion & small JSON buffer guards", () => {
        testDefensiveGuardsInvalidParentPoolFullAndSmallJsonBuf()?;
    });
    test("5. agent/tracer: cap.agent namespace integration", () => {
        testCapAgentNamespaceIntegration()?;
    });
}
