// Origin: Pure-CP Stdlib Test Suite (MCP Tool Registry, Capability ACL & Rate-Limited Router) | Created: 2026-11-22
// Deep Production Verification Suite for lib/mcp/registry.cp
// Covers:
//   1. Tool registration, findIndex, setEnabled, recordError, duplicate name & full-pool guards
//   2. ReadOnly / WriteLocal / Network / Destructive policy ACL & human-in-the-loop RequireApproval
//   3. Fixed 60s window per-minute rate limiting, negative/backward timestamp defense & cross-minute recovery
//   4. listAllowedTools dynamic policy filtering (tools/list) & output buffer truncation safety
//   5. Empty pool, empty name, unknown tool & negative initial timestamp boundary defenses

import { test, expect_eq } from "cap:test";
import {
    ToolRiskLevel,
    ToolPolicy,
    PolicyDecision,
    RegisteredTool,
    McpToolRegistry,
} from "../../lib/mcp/index.cp";

fn testRegistrationLookupEnableAndErrors()! {
    let slots_buf: [4]RegisteredTool = [RegisteredTool{}; 4];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    expect_eq(reg.len(), 0);
    expect_eq(reg.capacity(), 4);

    expect_eq(reg.register("read_file", "Read local file", ToolRiskLevel.ReadOnly, false, 0), 0);
    expect_eq(reg.register("write_file", "Write local file", ToolRiskLevel.WriteLocal, false, 10), 1);
    expect_eq(reg.register("web_fetch", "Fetch URL", ToolRiskLevel.Network, true, 5), 2);
    expect_eq(reg.register("drop_table", "Drop SQL table", ToolRiskLevel.Destructive, false, 1), 3);

    expect_eq(reg.len(), 4);
    expect_eq(reg.findIndex("read_file"), 0);
    expect_eq(reg.findIndex("drop_table"), 3);
    expect_eq(reg.findIndex("nonexistent") == null, true);

    // Duplicate name rejected & full pool rejected
    expect_eq(reg.register("read_file", "Dup", ToolRiskLevel.ReadOnly, false, 0) == null, true);
    expect_eq(reg.register("extra_tool", "Overflow", ToolRiskLevel.ReadOnly, false, 0) == null, true);
    expect_eq(reg.len(), 4);

    // Disable & re-enable
    const read_policy = ToolPolicy{
        max_risk: ToolRiskLevel.ReadOnly,
        require_approval_for_destructive: true,
        allow_network: false,
    };
    expect_eq(reg.setEnabled("read_file", false), true);
    expect_eq(reg.authorizeAndConsume("read_file", read_policy, 1000) == PolicyDecision.ToolDisabled, true);
    expect_eq(reg.setEnabled("read_file", true), true);
    expect_eq(reg.authorizeAndConsume("read_file", read_policy, 1000) == PolicyDecision.Allow, true);

    // Error accounting
    expect_eq(reg.recordError("read_file"), true);
    expect_eq(reg.recordError("read_file"), true);
    expect_eq(reg.recordError("ghost"), false);
    const rec = reg.get(0) ?? RegisteredTool{};
    expect_eq(rec.total_calls, 1);
    expect_eq(rec.total_errors, 2);
}

fn testPolicyAclAndDestructiveApproval()! {
    let slots_buf: [5]RegisteredTool = [RegisteredTool{}; 5];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    * = reg.register("read_file", "Read local file", ToolRiskLevel.ReadOnly, false, 0);
    * = reg.register("read_remote", "Read remote URL", ToolRiskLevel.ReadOnly, true, 0);
    * = reg.register("write_file", "Write local file", ToolRiskLevel.WriteLocal, false, 0);
    * = reg.register("web_fetch", "HTTP GET", ToolRiskLevel.Network, true, 0);
    * = reg.register("rm_rf", "Delete directory", ToolRiskLevel.Destructive, false, 0);

    // 1. Strict ReadOnly + no-network policy
    const ro_policy = ToolPolicy{
        max_risk: ToolRiskLevel.ReadOnly,
        require_approval_for_destructive: true,
        allow_network: false,
    };
    expect_eq(reg.authorizeAndConsume("read_file", ro_policy, 100) == PolicyDecision.Allow, true);
    expect_eq(reg.authorizeAndConsume("read_remote", ro_policy, 100) == PolicyDecision.DenyNetwork, true);
    expect_eq(reg.authorizeAndConsume("web_fetch", ro_policy, 100) == PolicyDecision.DenyNetwork, true);
    expect_eq(reg.authorizeAndConsume("write_file", ro_policy, 100) == PolicyDecision.DenyRisk, true);
    expect_eq(reg.authorizeAndConsume("rm_rf", ro_policy, 100) == PolicyDecision.DenyRisk, true);

    // Denied calls must not increment total_calls
    expect_eq((reg.get(2) ?? RegisteredTool{}).total_calls, 0);
    expect_eq((reg.get(4) ?? RegisteredTool{}).total_calls, 0);

    // 2. Destructive policy with human approval gate enabled -> RequireApproval
    const approval_policy = ToolPolicy{
        max_risk: ToolRiskLevel.Destructive,
        require_approval_for_destructive: true,
        allow_network: true,
    };
    expect_eq(reg.authorizeAndConsume("rm_rf", approval_policy, 200) == PolicyDecision.RequireApproval, true);
    expect_eq((reg.get(4) ?? RegisteredTool{}).total_calls, 0);
    expect_eq(reg.authorizeAndConsume("web_fetch", approval_policy, 200) == PolicyDecision.Allow, true);

    // 3. Destructive policy with approval gate disabled -> Allow
    const auto_policy = ToolPolicy{
        max_risk: ToolRiskLevel.Destructive,
        require_approval_for_destructive: false,
        allow_network: true,
    };
    expect_eq(reg.authorizeAndConsume("rm_rf", auto_policy, 300) == PolicyDecision.Allow, true);
    expect_eq((reg.get(4) ?? RegisteredTool{}).total_calls, 1);
}

fn testRateLimitWindowAndRecovery()! {
    let slots_buf: [2]RegisteredTool = [RegisteredTool{}; 2];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    * = reg.register("api_call", "Rate-limited API", ToolRiskLevel.Network, true, 2);
    const net_policy = ToolPolicy{
        max_risk: ToolRiskLevel.Network,
        require_approval_for_destructive: true,
        allow_network: true,
    };

    // First window starts at t = 1000
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 1000) == PolicyDecision.Allow, true);
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 30000) == PolicyDecision.Allow, true);
    // 3rd call within same 60s window (elapsed = 59999ms) -> DenyRateLimit
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 60999) == PolicyDecision.DenyRateLimit, true);
    // Negative / backward timestamp must NOT reset the active window
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, -5000) == PolicyDecision.DenyRateLimit, true);
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 500) == PolicyDecision.DenyRateLimit, true);
    expect_eq((reg.get(0) ?? RegisteredTool{}).total_calls, 2);

    // Cross-minute boundary: t = 61000 (elapsed == 60000ms) -> window resets and allows again
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 61000) == PolicyDecision.Allow, true);
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 61005) == PolicyDecision.Allow, true);
    expect_eq(reg.authorizeAndConsume("api_call", net_policy, 61010) == PolicyDecision.DenyRateLimit, true);

    const rec = reg.get(0) ?? RegisteredTool{};
    expect_eq(rec.total_calls, 4);
    expect_eq(rec.window_calls, 2);
    expect_eq(rec.window_start_ms, 61000);
}

fn testListAllowedToolsFiltering()! {
    let slots_buf: [5]RegisteredTool = [RegisteredTool{}; 5];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    * = reg.register("read_file", "Read", ToolRiskLevel.ReadOnly, false, 0);
    * = reg.register("write_file", "Write", ToolRiskLevel.WriteLocal, false, 0);
    * = reg.register("web_fetch", "Net", ToolRiskLevel.Network, true, 0);
    * = reg.register("drop_db", "Destruct", ToolRiskLevel.Destructive, false, 0);
    * = reg.register("disabled_read", "Disabled", ToolRiskLevel.ReadOnly, false, 0);
    expect_eq(reg.setEnabled("disabled_read", false), true);

    let out_buf: [5]usize = [99; 5];
    let out: []usize = out_buf[0..];

    // 1. ReadOnly, no network -> [0]
    const n_ro = reg.listAllowedTools(
        ToolPolicy{ max_risk: ToolRiskLevel.ReadOnly, require_approval_for_destructive: true, allow_network: false },
        out,
    );
    expect_eq(n_ro, 1);
    expect_eq(out_buf[0], 0);

    // 2. WriteLocal, no network -> [0, 1]
    const n_wl = reg.listAllowedTools(
        ToolPolicy{ max_risk: ToolRiskLevel.WriteLocal, require_approval_for_destructive: true, allow_network: false },
        out,
    );
    expect_eq(n_wl, 2);
    expect_eq(out_buf[0], 0);
    expect_eq(out_buf[1], 1);

    // 3. Network, allow_network=true -> [0, 1, 2]
    const n_net = reg.listAllowedTools(
        ToolPolicy{ max_risk: ToolRiskLevel.Network, require_approval_for_destructive: true, allow_network: true },
        out,
    );
    expect_eq(n_net, 3);
    expect_eq(out_buf[2], 2);

    // 4. Destructive, allow_network=true -> [0, 1, 2, 3] (disabled_read still excluded)
    const n_all = reg.listAllowedTools(
        ToolPolicy{ max_risk: ToolRiskLevel.Destructive, require_approval_for_destructive: true, allow_network: true },
        out,
    );
    expect_eq(n_all, 4);
    expect_eq(out_buf[3], 3);

    // 5. Small output slice truncates safely to out.len
    let small_buf: [2]usize = [99; 2];
    let small: []usize = small_buf[0..];
    const n_trunc = reg.listAllowedTools(
        ToolPolicy{ max_risk: ToolRiskLevel.Destructive, require_approval_for_destructive: true, allow_network: true },
        small,
    );
    expect_eq(n_trunc, 2);
    expect_eq(small_buf[0], 0);
    expect_eq(small_buf[1], 1);
}

fn testBoundaryAndMalformedInputDefenses()! {
    // 1. Empty 0-capacity backing pool
    let backing_one: [1]RegisteredTool = [RegisteredTool{}; 1];
    let empty_slots: []RegisteredTool = backing_one[0..0];
    let empty_reg = McpToolRegistry.init(empty_slots);

    const any_policy = ToolPolicy{
        max_risk: ToolRiskLevel.Destructive,
        require_approval_for_destructive: false,
        allow_network: true,
    };
    expect_eq(empty_reg.capacity(), 0);
    expect_eq(empty_reg.register("t1", "d", ToolRiskLevel.ReadOnly, false, 0) == null, true);
    expect_eq(empty_reg.findIndex("t1") == null, true);
    expect_eq(empty_reg.authorizeAndConsume("t1", any_policy, 0) == PolicyDecision.ToolNotFound, true);
    let idx_buf: [2]usize = [0; 2];
    let idx_s: []usize = idx_buf[0..];
    expect_eq(empty_reg.listAllowedTools(any_policy, idx_s), 0);

    // 2. Empty name, unknown tool & negative initial timestamp defense
    let slots_buf: [2]RegisteredTool = [RegisteredTool{}; 2];
    let slots: []RegisteredTool = slots_buf[0..];
    let reg = McpToolRegistry.init(slots);

    expect_eq(reg.register("", "Empty name", ToolRiskLevel.ReadOnly, false, 0) == null, true);
    expect_eq(reg.findIndex("") == null, true);
    expect_eq(reg.setEnabled("", true), false);
    expect_eq(reg.recordError(""), false);
    expect_eq(reg.authorizeAndConsume("", any_policy, 0) == PolicyDecision.ToolNotFound, true);
    expect_eq(reg.authorizeAndConsume("unknown", any_policy, 0) == PolicyDecision.ToolNotFound, true);

    // Negative initial timestamp clamps safely to 0 and enforces 60s window
    expect_eq(reg.register("limited", "1/min", ToolRiskLevel.ReadOnly, false, 1), 0);
    expect_eq(reg.authorizeAndConsume("limited", any_policy, -12345) == PolicyDecision.Allow, true);
    expect_eq(reg.authorizeAndConsume("limited", any_policy, -1) == PolicyDecision.DenyRateLimit, true);
    expect_eq(reg.authorizeAndConsume("limited", any_policy, 59999) == PolicyDecision.DenyRateLimit, true);
    expect_eq(reg.authorizeAndConsume("limited", any_policy, 60000) == PolicyDecision.Allow, true);
}

fn main() {
    test("1. mcp registry: register, findIndex, setEnabled, recordError & dup/full guards", () => {
        testRegistrationLookupEnableAndErrors()?;
    });
    test("2. mcp registry: capability risk ACL, network block & destructive RequireApproval", () => {
        testPolicyAclAndDestructiveApproval()?;
    });
    test("3. mcp registry: fixed 60s rate limit, negative-time defense & cross-minute recovery", () => {
        testRateLimitWindowAndRecovery()?;
    });
    test("4. mcp registry: listAllowedTools policy filtering & truncation safety", () => {
        testListAllowedToolsFiltering()?;
    });
    test("5. mcp registry: empty pool, empty name, unknown tool & negative-time defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
