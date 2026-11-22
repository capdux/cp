// Origin: Pure-CP Stdlib Golden E2E Integration Suite (cap:agent + cap:vector + cap:agentmem + cap:prompt + cap:mcp) | Created: 2026-11-22
// End-to-End 5-Package Zero-Heap AI Agent Verification:
//   1. Full closed loop: Memory Recall -> Filtered SIMD RAG -> Budgeted Prompt Compose ->
//      Malformed LLM JSON Repair -> MCP Policy-Gated Tool Dispatch & Result Build ->
//      TurnRunner Accounting & Convergence -> CI Eval Gate Markdown Report
//   2. MCP Policy Denial (ReadOnlyBlocked / ApprovalRequired) -> TurnRunner Circuit Breaker -> Eval Gate Failure
//   3. AgentPromptComposer priority budget allocation, proportional truncation & low-priority drop
//   4. AgentPromptComposer invalid init, full buffer & output overflow guards
//   5. Zero-heap multi-session buffer reuse across Composer, RagStore & TurnRunner

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TurnRunnerConfig,
    TurnStopReason,
    TurnStepEntry,
    AgentTurnRunner,
    EvalGateThresholds,
    EvalCaseResult,
    EvalGateVerdict,
    evaluateAgentGate,
    renderEvalGateSummary,
} from "cap:agent";
import {
    RagDocMeta,
    RagSearchHit,
    InMemoryRagStore,
} from "cap:vector";
import {
    MemoryRetrievalConfig,
    MemoryCandidate,
    MemoryScoredHit,
    scoreAndRankMemoriesF32,
    packMemoriesForPrompt,
} from "cap:agentmem";
import {
    AgentPromptSection,
    AgentPromptBlock,
    AgentPromptBuildReport,
    AgentPromptComposer,
    extractJsonCandidate,
    repairLlmJson,
} from "cap:prompt";
import {
    McpToolAccess,
    McpToolSpec,
    McpToolCatalog,
    McpCallRequest,
    McpDispatchStatus,
    McpToolDispatcher,
    McpToolResultBuilder,
} from "cap:mcp";

// ============================================================================
// Test 1: Golden 5-Package Closed Loop (Memory -> RAG -> Prompt -> Repair -> MCP -> TurnRunner -> EvalGate)
// ============================================================================

fn testGoldenFivePackageAgentLoop()! {
    const query_emb: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // --- 1. cap:agentmem: Three-factor SIMD memory retrieval & prompt packing ---
    const mem_cands: [3]MemoryCandidate = [
        MemoryCandidate{ mem_id: 1, importance: 0.9, last_access_ms: 9500, token_count: 14, tag: "prefs", summary: "User prefers zero-alloc CP stdlib.", alive: true },
        MemoryCandidate{ mem_id: 2, importance: 0.4, last_access_ms: 1000, token_count: 14, tag: "misc", summary: "Old unrelated note.", alive: true },
        MemoryCandidate{ mem_id: 3, importance: 0.8, last_access_ms: 9000, token_count: 16, tag: "prefs", summary: "Always verify with cpc check.", alive: true },
    ];
    const mem_embs: [12]f32 = [
        0.98, 0.20, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.90, 0.43, 0.0, 0.0,
    ];
    let mem_hits: [2]MemoryScoredHit = [MemoryScoredHit{}; 2];
    let mem_hits_s: []MemoryScoredHit = mem_hits[0..];
    const n_mem = scoreAndRankMemoriesF32(
        query_emb[0..],
        mem_cands[0..],
        mem_embs[0..],
        4,
        10000,
        MemoryRetrievalConfig{ w_recency: 0.2, w_importance: 0.3, w_relevance: 0.5, decay_rate_per_hour: 0.01, min_score: 0.5 },
        2,
        mem_hits_s,
    ) ?? return error.MemSearchFailed;
    expect_eq(n_mem, 2);
    expect_eq(mem_hits[0].mem_id, 1);

    let mem_text_buf: [256]u8 = [0; 256];
    let mem_text_s: []u8 = mem_text_buf[0..];
    const mem_pack = packMemoriesForPrompt(mem_hits[0..n_mem], mem_cands[0..], 40, mem_text_s) ?? return error.MemPackFailed;
    const mem_prompt_str: string = mem_text_buf[0..mem_pack.bytes_written];

    // --- 2. cap:vector: Local zero-heap filtered SIMD RAG store ---
    let rag_metas: [4]RagDocMeta = [RagDocMeta{}; 4];
    let rag_vecs: [16]f32 = [0.0; 16];
    let rag_metas_s: []RagDocMeta = rag_metas[0..];
    let rag_vecs_s: []f32 = rag_vecs[0..];
    let rag = InMemoryRagStore.init(rag_metas_s, rag_vecs_s, 4) ?? return error.RagInitFailed;

    const e_doc1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const e_doc2: [4]f32 = [0.8, 0.6, 0.0, 0.0];
    const e_doc3: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(rag.upsert(101, 77, 0b01, 18, 1000, "cap:vector provides @dot SIMD cosine search.", e_doc1[0..]), true);
    expect_eq(rag.upsert(102, 77, 0b01, 16, 1001, "cap:mcp enforces ReadOnly and ApprovalRequired gates.", e_doc2[0..]), true);
    expect_eq(rag.upsert(103, 99, 0b10, 20, 1002, "Other namespace doc.", e_doc3[0..]), true);

    let rag_hits: [2]RagSearchHit = [RagSearchHit{}; 2];
    let rag_hits_s: []RagSearchHit = rag_hits[0..];
    const n_rag = rag.searchFiltered(query_emb[0..], 77, 0b01, 0.5, 2, rag_hits_s) ?? return error.RagSearchFailed;
    expect_eq(n_rag, 2);
    expect_eq(rag_hits[0].doc_id, 101);

    let rag_ctx_buf: [256]u8 = [0; 256];
    let rag_ctx_s: []u8 = rag_ctx_buf[0..];
    const rag_bytes = rag.packSearchContext(rag_hits[0..n_rag], 64, rag_ctx_s) ?? return error.RagPackFailed;
    const rag_prompt_str: string = rag_ctx_buf[0..rag_bytes];

    // --- 3. cap:prompt: Compose unified prompt under token budget ---
    let prompt_blocks: [8]AgentPromptBlock = [AgentPromptBlock{}; 8];
    let prompt_blocks_s: []AgentPromptBlock = prompt_blocks[0..];
    let composer = AgentPromptComposer.init(prompt_blocks_s, 256) ?? return error.ComposerInitFailed;
    expect_eq(composer.addSystemRole("You are a CP AI Systems Engineer.", 10), true);
    expect_eq(composer.addToolCatalog("lookup_symbol(name)", 8), true);
    expect_eq(composer.addMemoryContext(mem_prompt_str, mem_pack.used_tokens as u32), true);
    expect_eq(composer.addRagContext(rag_prompt_str, 34), true);
    expect_eq(composer.addUserQuery("Look up InMemoryRagStore.", 8), true);

    let final_prompt_buf: [1024]u8 = [0; 1024];
    let final_prompt_s: []u8 = final_prompt_buf[0..];
    const comp_rep: AgentPromptBuildReport = composer.build(final_prompt_s) ?? return error.ComposeFailed;
    const final_prompt = final_prompt_buf[0..comp_rep.bytes_written];
    expect_eq(comp_rep.included_sections, 5);
    expect_eq(comp_rep.dropped_sections, 0);
    expect_eq(std.mem.indexOf(u8, final_prompt, "## System\n") != null, true);
    expect_eq(std.mem.indexOf(u8, final_prompt, "## Tools\n") != null, true);
    expect_eq(std.mem.indexOf(u8, final_prompt, "## Memory\n") != null, true);
    expect_eq(std.mem.indexOf(u8, final_prompt, "## Context\n") != null, true);
    expect_eq(std.mem.indexOf(u8, final_prompt, "## User\n") != null, true);

    // --- 4. cap:agent + cap:prompt + cap:mcp: Turn loop with malformed JSON repair & MCP dispatch ---
    let tool_slots: [4]McpToolSpec = [McpToolSpec{}; 4];
    let tool_slots_s: []McpToolSpec = tool_slots[0..];
    let catalog = McpToolCatalog.init(tool_slots_s);
    expect_eq(catalog.register(1, "lookup_symbol", "Lookup CP symbol", "name", McpToolAccess.ReadOnly, 4), true);
    let dispatcher = McpToolDispatcher.init(true); // read-only mode enabled

    let step_buf: [16]TurnStepEntry = [TurnStepEntry{}; 16];
    let step_s: []TurnStepEntry = step_buf[0..];
    let runner = AgentTurnRunner.init(step_s, TurnRunnerConfig{
        max_turns: 4,
        max_tool_calls: 4,
        max_input_tokens: 4000,
        max_output_tokens: 2000,
        max_cost_micro_usd: 20000,
        max_consecutive_errors: 2,
    }) ?? return error.RunnerInitFailed;

    // Turn 1: LLM outputs malformed single-quoted JSON with trailing comma inside <think> + fence
    expect_eq(runner.beginTurn(1000) == null, true);
    expect_eq(runner.recordLlmResponse(320, 64, 900, true, "invoke lookup_symbol", 1080) == null, true);

    const raw_llm = "<think>check symbol</think>\n```json\n{'name': 'InMemoryRagStore',}\n```";
    const span = extractJsonCandidate(raw_llm) ?? return error.ExtractFailed;
    let repaired_buf: [128]u8 = [0; 128];
    let repaired_s: []u8 = repaired_buf[0..];
    const rep = repairLlmJson(raw_llm[span.start..span.end], repaired_s) ?? return error.RepairFailed;
    const clean_args: string = repaired_buf[0..rep.bytes_written];

    // MCP dispatch enforces ReadOnly policy and required `name` param
    const decision = dispatcher.dispatch(&catalog, McpCallRequest{
        tool_name: "lookup_symbol",
        arguments_json: clean_args,
        read_only_mode: true,
        approved: false,
    });
    expect_eq(decision.status == McpDispatchStatus.Allowed, true);

    // Build MCP CallToolResult JSON & finish call
    let tool_res_buf: [256]u8 = [0; 256];
    let tool_res_s: []u8 = tool_res_buf[0..];
    const res_len = McpToolResultBuilder.buildTextResult("found in lib/vector/rag_store.cp", false, tool_res_s) ?? return error.ResultBuildFailed;
    expect_eq(std.mem.indexOf(u8, tool_res_buf[0..res_len], "\"isError\":false") != null, true);
    expect_eq(dispatcher.finishCall(&catalog, decision.slot_index, false), true);
    expect_eq(runner.recordToolCall("lookup_symbol", true, "found in rag_store.cp", 1120) == null, true);

    // Turn 2: LLM converges with final answer
    expect_eq(runner.beginTurn(1200) == null, true);
    const stop_reason = runner.recordLlmResponse(380, 90, 1100, false, "final answer", 1320) ?? return error.ExpectedComplete;
    expect_eq(stop_reason == TurnStopReason.Completed, true);
    const outcome = runner.finish(stop_reason, 1350);
    expect_eq(outcome.isSuccess(), true);
    expect_eq(outcome.turns_executed, 2);
    expect_eq(outcome.tool_calls, 1);

    // --- 5. cap:agent: Feed outcome into CI Eval Gate & render Markdown summary ---
    const eval_cases: [1]EvalCaseResult = [
        EvalCaseResult{
            case_id: 1,
            passed: outcome.isSuccess(),
            latency_ms: outcome.elapsed_ms,
            cost_micro_usd: outcome.total_cost_micro_usd,
            tool_calls: outcome.tool_calls,
            tool_errors: outcome.tool_errors,
        },
    ];
    let lat_scratch: [2]u64 = [0; 2];
    let lat_s: []u64 = lat_scratch[0..];
    const gate = evaluateAgentGate(eval_cases[0..], EvalGateThresholds{
        min_pass_rate: 1.0,
        max_p95_latency_ms: 1000,
        max_avg_cost_micro_usd: 5000,
        max_tool_error_rate: 0.05,
    }, lat_s) ?? return error.GateFailed;
    expect_eq(gate.isPassed(), true);

    let md_buf: [512]u8 = [0; 512];
    let md_s: []u8 = md_buf[0..];
    const md_len = renderEvalGateSummary(gate, md_s) ?? return error.RenderFailed;
    expect_eq(std.mem.indexOf(u8, md_buf[0..md_len], "### Agent Eval Gate: PASSED") != null, true);
}

// ============================================================================
// Test 2: MCP Policy Denial -> TurnRunner Circuit Breaker -> Eval Gate Reject
// ============================================================================

fn testPolicyDenialCircuitBreakerAndEvalReject()! {
    let tool_slots: [2]McpToolSpec = [McpToolSpec{}; 2];
    let tool_slots_s: []McpToolSpec = tool_slots[0..];
    let catalog = McpToolCatalog.init(tool_slots_s);
    expect_eq(catalog.register(1, "drop_table", "Dangerous op", "table", McpToolAccess.ApprovalRequired, 1), true);
    let dispatcher = McpToolDispatcher.init(false);

    let step_buf: [12]TurnStepEntry = [TurnStepEntry{}; 12];
    let step_s: []TurnStepEntry = step_buf[0..];
    let runner = AgentTurnRunner.init(step_s, TurnRunnerConfig{
        max_turns: 4,
        max_tool_calls: 10,
        max_consecutive_errors: 2,
    }) ?? return error.RunnerInitFailed;

    expect_eq(runner.beginTurn(100) == null, true);
    expect_eq(runner.recordLlmResponse(100, 40, 200, true, "try drop", 120) == null, true);

    // Two unapproved attempts denied by MCP -> trips TurnRunner ConsecutiveErrorsExceeded
    const d1 = dispatcher.dispatch(&catalog, McpCallRequest{
        tool_name: "drop_table",
        arguments_json: "{\"table\":\"users\"}",
        read_only_mode: false,
        approved: false,
    });
    expect_eq(d1.status == McpDispatchStatus.ApprovalRequired, true);
    expect_eq(runner.recordToolCall("drop_table", false, "approval required", 140) == null, true);

    const d2 = dispatcher.dispatch(&catalog, McpCallRequest{
        tool_name: "drop_table",
        arguments_json: "{\"table\":\"users\"}",
        read_only_mode: false,
        approved: false,
    });
    expect_eq(d2.status == McpDispatchStatus.ApprovalRequired, true);
    const stop = runner.recordToolCall("drop_table", false, "approval required", 160) ?? return error.ExpectedCircuitBreak;
    expect_eq(stop == TurnStopReason.ConsecutiveErrorsExceeded, true);
    const outcome = runner.finish(stop, 180);

    const cases: [1]EvalCaseResult = [
        EvalCaseResult{
            case_id: 1,
            passed: outcome.isSuccess(),
            latency_ms: outcome.elapsed_ms,
            cost_micro_usd: outcome.total_cost_micro_usd,
            tool_calls: outcome.tool_calls,
            tool_errors: outcome.tool_errors,
        },
    ];
    let scratch: [1]u64 = [0; 1];
    let scratch_s: []u64 = scratch[0..];
    const gate = evaluateAgentGate(cases[0..], EvalGateThresholds{ min_pass_rate: 0.90 }, scratch_s) ?? return error.GateFailed;
    expect_eq(gate.verdict == EvalGateVerdict.PassRateBelowMin, true);
    expect_eq(gate.isPassed(), false);
}

// ============================================================================
// Test 3: AgentPromptComposer Priority Budget, Truncation & Drop
// ============================================================================

fn testAgentPromptComposerPriorityAndTruncation()! {
    let blocks: [6]AgentPromptBlock = [AgentPromptBlock{}; 6];
    let blocks_s: []AgentPromptBlock = blocks[0..];
    // Tight budget = 40 tokens:
    // SystemRole (prio 100, 15 tok) -> fits (rem 25)
    // UserQuery (prio 95, 15 tok) -> fits (rem 10)
    // RagContext (prio 70, 20 tok) -> proportionally truncated to 10 tok (rem 0)
    // MemoryContext (prio 60, 15 tok) -> dropped (rem 0)
    let composer = AgentPromptComposer.init(blocks_s, 40) ?? return error.InitFailed;
    expect_eq(composer.addMemoryContext("long term memory dropped", 15), true);
    expect_eq(composer.addRagContext("0123456789abcdefghij", 20), true);
    expect_eq(composer.addUserQuery("What is CP?", 15), true);
    expect_eq(composer.addSystemRole("System role.", 15), true);

    let out: [512]u8 = [0; 512];
    let out_s: []u8 = out[0..];
    const rep = composer.build(out_s) ?? return error.BuildFailed;
    const text = out[0..rep.bytes_written];

    expect_eq(rep.used_tokens, 40);
    expect_eq(rep.included_sections, 3);
    expect_eq(rep.truncated_sections, 1);
    expect_eq(rep.dropped_sections, 1);
    expect_eq(rep.system_blocks, 1);
    expect_eq(rep.rag_blocks, 1);
    expect_eq(rep.user_blocks, 1);
    expect_eq(rep.memory_blocks, 0);

    // Canonical order: System appears before Context, Context appears before User
    const idx_sys = std.mem.indexOf(u8, text, "## System\n") ?? return error.MissingSys;
    const idx_ctx = std.mem.indexOf(u8, text, "## Context\n") ?? return error.MissingCtx;
    const idx_usr = std.mem.indexOf(u8, text, "## User\n") ?? return error.MissingUsr;
    expect_eq(idx_sys < idx_ctx, true);
    expect_eq(idx_ctx < idx_usr, true);
    expect_eq(std.mem.indexOf(u8, text, "...[truncated]") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "## Memory\n") == null, true);
}

// ============================================================================
// Test 4: AgentPromptComposer Invalid Init, Full & Output Overflow Guards
// ============================================================================

fn testAgentPromptComposerBoundaryGuards()! {
    let buf: [2]AgentPromptBlock = [AgentPromptBlock{}; 2];
    let empty: []AgentPromptBlock = buf[0..0];
    let ok_s: []AgentPromptBlock = buf[0..];

    expect_eq(AgentPromptComposer.init(empty, 100) == null, true);
    expect_eq(AgentPromptComposer.init(ok_s, 0) == null, true);

    let composer = AgentPromptComposer.init(ok_s, 100) ?? return error.InitFailed;
    expect_eq(composer.addSystemRole("", 10), false); // empty content rejected
    expect_eq(composer.addSystemRole("sys", 0), true); // auto token estimation
    expect_eq(composer.addUserQuery("hi", 5), true);
    expect_eq(composer.addRagContext("overflow", 5), false); // full

    // Tiny output buffer -> null without overflow
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(composer.build(tiny_s) == null, true);

    // Reset clears staged sections
    composer.reset();
    expect_eq(composer.sectionCount(), 0);
}

// ============================================================================
// Test 5: Zero-Heap Multi-Session Buffer Reuse Across Packages
// ============================================================================

fn testZeroHeapMultiSessionReuse()! {
    let blocks: [4]AgentPromptBlock = [AgentPromptBlock{}; 4];
    let blocks_s: []AgentPromptBlock = blocks[0..];
    let composer = AgentPromptComposer.init(blocks_s, 100) ?? return error.InitFailed;

    let steps: [8]TurnStepEntry = [TurnStepEntry{}; 8];
    let steps_s: []TurnStepEntry = steps[0..];
    let runner = AgentTurnRunner.init(steps_s, TurnRunnerConfig{ max_turns: 3 }) ?? return error.InitFailed;

    let out: [256]u8 = [0; 256];
    let out_s: []u8 = out[0..];

    let session: usize = 0;
    while (session < 2) : (session += 1) {
        composer.reset();
        runner.reset();
        expect_eq(composer.addSystemRole("Agent", 5), true);
        expect_eq(composer.addUserQuery("Ping", 5), true);
        const rep = composer.build(out_s) ?? return error.BuildFailed;
        expect_eq(rep.included_sections, 2);

        expect_eq(runner.beginTurn(1000) == null, true);
        const stop = runner.recordLlmResponse(20, 10, 50, false, "Pong", 1020) ?? return error.ExpectedStop;
        const outcome = runner.finish(stop, 1030);
        expect_eq(outcome.isSuccess(), true);
        expect_eq(outcome.turns_executed, 1);
    }
}

test("ai_agent golden e2e: 5-package closed loop (memory->RAG->prompt->repair->MCP->turn->eval)", testGoldenFivePackageAgentLoop);
test("ai_agent golden e2e: MCP policy denial -> TurnRunner circuit breaker -> EvalGate reject", testPolicyDenialCircuitBreakerAndEvalReject);
test("ai_agent golden e2e: AgentPromptComposer priority budget, truncation & drop", testAgentPromptComposerPriorityAndTruncation);
test("ai_agent golden e2e: AgentPromptComposer invalid init, full & overflow guards", testAgentPromptComposerBoundaryGuards);
test("ai_agent golden e2e: zero-heap multi-session buffer reuse across packages", testZeroHeapMultiSessionReuse);
