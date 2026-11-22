// Origin: Pure-CP Stdlib Test Suite (lib/mcp/tool_dispatch.cp) | Created: 2026-11-22
// Production Deep Verification Suite for MCP Tool Catalog, Policy-Gated Dispatcher & CallToolResult Builder:
//   1. McpToolCatalog registration, duplicate name/id guards, unregister & slot reuse
//   2. McpToolCatalog.renderToolsListJson MCP JSON format, escaping & buffer overflow guard
//   3. McpToolDispatcher read-only & HITL approval policy gates + ToolNotFound
//   4. McpToolDispatcher required-param validation, nested-key isolation, concurrency cap & telemetry
//   5. McpToolResultBuilder multi-block text/resource emission, RFC 8259 escaping & isError
//   6. Buffer-too-small defensive guards across renderer & result builder

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    McpToolAccess,
    McpToolSpec,
    McpToolCatalog,
    McpCallRequest,
    McpDispatchStatus,
    McpDispatchDecision,
    McpToolDispatcher,
    McpToolResultBuilder,
} from "cap:mcp";

fn containsSlice(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

// ============================================================================
// Test 1: McpToolCatalog Registration, Duplicate Guards & Slot Reuse
// ============================================================================

fn testToolCatalogRegistrationAndGuards()! {
    let slots_buf: [2]McpToolSpec = [McpToolSpec{}; 2];
    let slots_s: []McpToolSpec = slots_buf[0..];
    let catalog = cap.mcp.McpToolCatalog.init(slots_s);

    expect_eq(catalog.count(), 0);
    expect_eq(catalog.capacity(), 2);

    // Empty name rejected
    expect_eq(catalog.register(1, "", "empty", "", McpToolAccess.ReadOnly, 0), false);

    // Valid registrations
    expect_eq(catalog.register(10, "read_file", "Read a file", "path", McpToolAccess.ReadOnly, 4), true);
    // Duplicate name or duplicate tool_id rejected
    expect_eq(catalog.register(11, "read_file", "Dup name", "path", McpToolAccess.ReadOnly, 4), false);
    expect_eq(catalog.register(10, "write_file", "Dup id", "path", McpToolAccess.Mutating, 1), false);

    expect_eq(catalog.register(20, "write_file", "Write a file", "path,content", McpToolAccess.Mutating, 1), true);
    expect_eq(catalog.count(), 2);

    // Full catalog rejects 3rd tool
    expect_eq(catalog.register(30, "exec", "Run cmd", "cmd", McpToolAccess.ApprovalRequired, 1), false);

    // Lookup & unregister frees slot for reuse
    const spec = catalog.findByName("write_file") ?? return error.NotFound;
    expect_eq(spec.tool_id, 20);
    expect_eq(spec.access == McpToolAccess.Mutating, true);

    expect_eq(catalog.unregister("write_file"), true);
    expect_eq(catalog.unregister("write_file"), false);
    expect_eq(catalog.count(), 1);
    expect_eq(catalog.register(30, "exec", "Run cmd", "cmd", McpToolAccess.ApprovalRequired, 1), true);
    expect_eq(catalog.count(), 2);
}

// ============================================================================
// Test 2: McpToolCatalog.renderToolsListJson Emission & Escaping
// ============================================================================

fn testRenderToolsListJson()! {
    let slots_buf: [2]McpToolSpec = [McpToolSpec{}; 2];
    let slots_s: []McpToolSpec = slots_buf[0..];
    let catalog = McpToolCatalog.init(slots_s);

    expect_eq(
        catalog.register(1, "search", "Search \"docs\"\nfast", "query, limit", McpToolAccess.ReadOnly, 0),
        true,
    );
    expect_eq(
        catalog.register(2, "ping", "Health check", "", McpToolAccess.ReadOnly, 0),
        true,
    );

    let out_buf: [512]u8 = [0; 512];
    let out_s: []u8 = out_buf[0..];
    const n = catalog.renderToolsListJson(out_s) ?? return error.RenderFailed;
    const json = out_buf[0..n];

    expect_eq(containsSlice(json, "{\"tools\":["), true);
    expect_eq(containsSlice(json, "\"name\":\"search\""), true);
    expect_eq(containsSlice(json, "\"description\":\"Search \\\"docs\\\"\\nfast\""), true);
    expect_eq(containsSlice(json, "\"required\":[\"query\",\"limit\"]"), true);
    expect_eq(containsSlice(json, "\"name\":\"ping\""), true);
    expect_eq(containsSlice(json, "\"required\":[]"), true);
}

// ============================================================================
// Test 3: McpToolDispatcher Read-Only & Approval Policy Gates
// ============================================================================

fn testDispatcherPolicyGates()! {
    let slots_buf: [3]McpToolSpec = [McpToolSpec{}; 3];
    let slots_s: []McpToolSpec = slots_buf[0..];
    let catalog = McpToolCatalog.init(slots_s);
    expect_eq(catalog.register(1, "read_file", "Read", "path", McpToolAccess.ReadOnly, 0), true);
    expect_eq(catalog.register(2, "write_file", "Write", "path,data", McpToolAccess.Mutating, 0), true);
    expect_eq(catalog.register(3, "deploy", "Deploy", "env", McpToolAccess.ApprovalRequired, 0), true);

    let disp = McpToolDispatcher.init(false);

    // Unknown tool -> ToolNotFound
    const d0 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "nonexistent",
        arguments_json: "{}",
    });
    expect_eq(d0.status == McpDispatchStatus.ToolNotFound, true);

    // Mutating blocked under request read_only_mode
    const d1 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "write_file",
        arguments_json: "{\"path\":\"a.txt\",\"data\":\"hi\"}",
        read_only_mode: true,
        approved: false,
    });
    expect_eq(d1.status == McpDispatchStatus.ReadOnlyBlocked, true);

    // ReadOnly allowed under read_only_mode
    const d2 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "read_file",
        arguments_json: "{\"path\":\"a.txt\"}",
        read_only_mode: true,
        approved: false,
    });
    expect_eq(d2.status == McpDispatchStatus.Allowed, true);
    expect_eq(d2.tool_id, 1);

    // ApprovalRequired blocked without approval, allowed with approval
    const d3 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "deploy",
        arguments_json: "{\"env\":\"prod\"}",
        read_only_mode: false,
        approved: false,
    });
    expect_eq(d3.status == McpDispatchStatus.ApprovalRequired, true);

    const d4 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "deploy",
        arguments_json: "{\"env\":\"prod\"}",
        read_only_mode: false,
        approved: true,
    });
    expect_eq(d4.status == McpDispatchStatus.Allowed, true);
    expect_eq(d4.tool_id, 3);

    // Server-wide default_read_only blocks Mutating even if req.read_only_mode == false
    let ro_disp = McpToolDispatcher.init(true);
    const d5 = ro_disp.dispatch(&catalog, McpCallRequest{
        tool_name: "write_file",
        arguments_json: "{\"path\":\"a.txt\",\"data\":\"hi\"}",
        read_only_mode: false,
        approved: true,
    });
    expect_eq(d5.status == McpDispatchStatus.ReadOnlyBlocked, true);
    expect_eq(disp.allowedCount(), 2);
    expect_eq(disp.deniedCount(), 3);
}

// ============================================================================
// Test 4: Required Params, Nested-Key Isolation, Concurrency Cap & Metrics
// ============================================================================

fn testParamsConcurrencyAndTelemetry()! {
    let slots_buf: [2]McpToolSpec = [McpToolSpec{}; 2];
    let slots_s: []McpToolSpec = slots_buf[0..];
    let catalog = McpToolCatalog.init(slots_s);
    expect_eq(catalog.register(7, "db_query", "Query DB", "sql, timeout", McpToolAccess.ReadOnly, 1), true);

    let disp = McpToolDispatcher.init(false);

    // Non-object arguments -> InvalidArguments
    const bad_args = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "db_query",
        arguments_json: "[\"not\", \"object\"]",
    });
    expect_eq(bad_args.status == McpDispatchStatus.InvalidArguments, true);

    // Nested "timeout" inside "meta" must NOT satisfy top-level required param "timeout"
    const miss = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "db_query",
        arguments_json: "{\"sql\": \"SELECT 1\", \"meta\": {\"timeout\": 5}}",
    });
    expect_eq(miss.status == McpDispatchStatus.MissingRequiredParam, true);
    expect_eq(std.mem.eql(u8, miss.missing_param, "timeout"), true);

    // Valid call acquires concurrency slot (max_concurrency = 1)
    const ok1 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "db_query",
        arguments_json: "{\"sql\": \"SELECT 1\", \"timeout\": 10}",
    });
    expect_eq(ok1.status == McpDispatchStatus.Allowed, true);

    // Second concurrent call blocked by ConcurrencyLimitReached
    const busy = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "db_query",
        arguments_json: "{\"sql\": \"SELECT 2\", \"timeout\": 10}",
    });
    expect_eq(busy.status == McpDispatchStatus.ConcurrencyLimitReached, true);

    // Finish first call with error -> releases slot and increments error_count
    expect_eq(disp.finishCall(&catalog, ok1.slot_index, true), true);
    const spec_after = catalog.findByName("db_query") ?? return error.NotFound;
    expect_eq(spec_after.inflight, 0);
    expect_eq(spec_after.call_count, 1);
    expect_eq(spec_after.error_count, 1);

    // Next call now succeeds
    const ok2 = disp.dispatch(&catalog, McpCallRequest{
        tool_name: "db_query",
        arguments_json: "{\"sql\": \"SELECT 2\", \"timeout\": 10}",
    });
    expect_eq(ok2.status == McpDispatchStatus.Allowed, true);
}

// ============================================================================
// Test 5: McpToolResultBuilder Multi-Block Emission & RFC 8259 Escaping
// ============================================================================

fn testToolResultBuilderAndEscaping()! {
    let out_buf: [512]u8 = [0; 512];
    let out_s: []u8 = out_buf[0..];

    // 1. Multi-block builder (text + embedded resource, isError: false)
    let builder = McpToolResultBuilder.init(out_s, false);
    expect_eq(builder.addText("line1\n\"quoted\"\t\\path"), true);
    expect_eq(builder.addResourceText("file:///a.cp", "text/plain", "fn main() {}"), true);
    const n1 = builder.finish() ?? return error.FinishFailed;
    const expected1 = "{\"content\":[{\"type\":\"text\",\"text\":\"line1\\n\\\"quoted\\\"\\t\\\\path\"},{\"type\":\"resource\",\"resource\":{\"uri\":\"file:///a.cp\",\"mimeType\":\"text/plain\",\"text\":\"fn main() {}\"}}],\"isError\":false}";
    expect_eq(std.mem.eql(u8, out_buf[0..n1], expected1), true);

    // 2. Convenience error result helper (isError: true)
    const n2 = McpToolResultBuilder.buildErrorResult("Permission denied: \"/etc/shadow\"", out_s) ?? return error.BuildFailed;
    const expected2 = "{\"content\":[{\"type\":\"text\",\"text\":\"Permission denied: \\\"/etc/shadow\\\"\"}],\"isError\":true}";
    expect_eq(std.mem.eql(u8, out_buf[0..n2], expected2), true);
}

// ============================================================================
// Test 6: Buffer-Too-Small Defensive Guards
// ============================================================================

fn testBufferOverflowGuards()! {
    let slots_buf: [1]McpToolSpec = [McpToolSpec{}; 1];
    let slots_s: []McpToolSpec = slots_buf[0..];
    let catalog = McpToolCatalog.init(slots_s);
    expect_eq(catalog.register(1, "long_tool_name", "A longer description", "param_a", McpToolAccess.ReadOnly, 0), true);

    let tiny_buf: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny_buf[0..];

    // renderToolsListJson returns null safely on undersized buffer
    expect_eq(catalog.renderToolsListJson(tiny_s) == null, true);

    // McpToolResultBuilder returns false/null safely on undersized buffer
    let b = McpToolResultBuilder.init(tiny_s, false);
    expect_eq(b.addText("this text exceeds sixteen bytes"), false);
    expect_eq(b.finish() == null, true);

    // One-shot helpers also return null on undersized buffer
    expect_eq(McpToolResultBuilder.buildTextResult("hello world", false, tiny_s) == null, true);
    expect_eq(McpToolResultBuilder.buildErrorResult("error", tiny_s) == null, true);
}

fn main() {
    test("1. mcp.tool_dispatch: catalog registration, duplicate guards & slot reuse", () => {
        testToolCatalogRegistrationAndGuards()?;
    });
    test("2. mcp.tool_dispatch: renderToolsListJson MCP format & escaping", () => {
        testRenderToolsListJson()?;
    });
    test("3. mcp.tool_dispatch: read-only & HITL approval policy gates", () => {
        testDispatcherPolicyGates()?;
    });
    test("4. mcp.tool_dispatch: required params, concurrency cap & telemetry", () => {
        testParamsConcurrencyAndTelemetry()?;
    });
    test("5. mcp.tool_dispatch: CallToolResult multi-block builder & escaping", () => {
        testToolResultBuilderAndEscaping()?;
    });
    test("6. mcp.tool_dispatch: buffer-too-small defensive guards", () => {
        testBufferOverflowGuards()?;
    });
}
