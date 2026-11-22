// Origin: Pure-CP Stdlib Test Suite (lib/agent/sandbox_policy.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Tool Sandbox & Capability Policy Engine:
//   1. Default-deny closed posture & highest-priority matching rule selection
//   2. Explicit deny unconditionally overrides higher-priority allow / require_approval
//   3. Path traversal (`..`), NUL byte & allowed_root sibling-prefix escape hard-block
//   4. Network subdomain wildcard (`*.example.com`, case-insensitive) & command prefix matching
//   5. Human approval gate (`require_approval` -> `allow` when `human_approved = true`)
//   6. Per-session call quota exhaustion (including weighted cost_units) & reset_session_quotas
//   7. Rule deduplication, 32-rule capacity boundary, audit_only & SandboxAuditStats accuracy

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_SANDBOX_RULES,
    CapabilityKind,
    RiskTier,
    PolicyAction,
    SandboxRequest,
    SandboxPolicyDecision,
    SandboxAuditStats,
    ToolSandboxPolicy,
} from "cap:agent";

// ============================================================================
// Test 1: Default-Deny Closed Posture & Priority Ordering
// ============================================================================

fn testDefaultDenyAndPriorityOrdering()! {
    let policy = cap.agent.ToolSandboxPolicy.init();

    // Empty policy must default-deny all requests
    const d0: SandboxPolicyDecision = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace/readme.md",
    });
    expect_eq(d0.action == PolicyAction.deny, true);
    expect_eq(d0.matched, false);
    expect_eq(d0.path_violation, false);

    // Register low-priority allow and high-priority require_approval for same scope
    expect_eq(policy.add_rule("low-allow", CapabilityKind.fs_write, "/workspace/*", RiskTier.low, PolicyAction.allow, 0, 10) != null, true);
    expect_eq(policy.add_rule("high-approve", CapabilityKind.fs_write, "/workspace/config/*", RiskTier.high, PolicyAction.require_approval, 0, 50) != null, true);

    // Matching only low-priority rule -> allow
    const d1 = policy.evaluate(SandboxRequest{
        tool_name: "write_file",
        capability: CapabilityKind.fs_write,
        target: "/workspace/src/main.cp",
    });
    expect_eq(d1.action == PolicyAction.allow, true);
    expect_eq(d1.matched, true);
    expect_eq(std.mem.eql(u8, d1.ruleIdSlice(), "low-allow"), true);

    // Matching both -> highest priority (50 > 10) wins -> require_approval
    const d2 = policy.evaluate(SandboxRequest{
        tool_name: "write_file",
        capability: CapabilityKind.fs_write,
        target: "/workspace/config/prod.json",
    });
    expect_eq(d2.action == PolicyAction.require_approval, true);
    expect_eq(std.mem.eql(u8, d2.ruleIdSlice(), "high-approve"), true);
}

// ============================================================================
// Test 2: Explicit Deny Unconditionally Overrides Allow
// ============================================================================

fn testExplicitDenyOverridesAllow()! {
    let policy = ToolSandboxPolicy.init();
    // High-priority allow (priority 100) vs lower-priority explicit deny (priority 1)
    expect_eq(policy.add_rule("allow-all-ws", CapabilityKind.fs_read, "/workspace/*", RiskTier.low, PolicyAction.allow, 0, 100) != null, true);
    expect_eq(policy.add_rule("deny-env", CapabilityKind.fs_read, "/workspace/.env*", RiskTier.critical, PolicyAction.deny, 0, 1) != null, true);

    // Explicit deny must win regardless of priority
    const d = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace/.env.local",
    });
    expect_eq(d.action == PolicyAction.deny, true);
    expect_eq(d.matched, true);
    expect_eq(d.risk_tier == RiskTier.critical, true);
    expect_eq(std.mem.eql(u8, d.ruleIdSlice(), "deny-env"), true);
}

// ============================================================================
// Test 3: Path Traversal, NUL Byte & Root-Escape Hard Block
// ============================================================================

fn testPathTraversalNulAndRootEscape()! {
    let policy = ToolSandboxPolicy.init();
    expect_eq(policy.set_allowed_root("/workspace"), true);
    // Reject invalid root containing ".."
    expect_eq(policy.set_allowed_root("/workspace/../etc"), false);

    // Even with wildcard allow rule, path violations must be hard-blocked!
    expect_eq(policy.add_rule("allow-any-read", CapabilityKind.fs_read, "*", RiskTier.low, PolicyAction.allow, 0, 100) != null, true);

    // 1. Parent traversal "../"
    const d_trav = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace/src/../../etc/passwd",
    });
    expect_eq(d_trav.action == PolicyAction.deny, true);
    expect_eq(d_trav.path_violation, true);

    // 2. Embedded NUL byte
    const nul_path: [16]u8 = [47, 119, 111, 114, 107, 115, 112, 97, 99, 101, 47, 97, 0, 46, 99, 112];
    const d_nul = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: nul_path[0..],
    });
    expect_eq(d_nul.action == PolicyAction.deny, true);
    expect_eq(d_nul.path_violation, true);

    // 3. Sibling prefix escape ("/workspace_evil/secret" must NOT match "/workspace")
    const d_sib = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace_evil/secret.txt",
    });
    expect_eq(d_sib.action == PolicyAction.deny, true);
    expect_eq(d_sib.path_violation, true);

    // 4. Clean normalized path with redundant slashes & "." inside root succeeds
    const d_ok = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "/workspace//src/./app.cp",
    });
    expect_eq(d_ok.action == PolicyAction.allow, true);
    expect_eq(d_ok.path_violation, false);
}

// ============================================================================
// Test 4: Network Subdomain Wildcard & Command Prefix Matching
// ============================================================================

fn testNetworkWildcardAndCommandPrefix()! {
    let policy = ToolSandboxPolicy.init();
    expect_eq(policy.add_rule("net-openai", CapabilityKind.net_egress, "*.openai.com", RiskTier.medium, PolicyAction.allow, 0, 10) != null, true);
    expect_eq(policy.add_rule("exec-git", CapabilityKind.exec_cmd, "git *", RiskTier.low, PolicyAction.allow, 0, 10) != null, true);
    expect_eq(policy.add_rule("deny-git-push", CapabilityKind.exec_cmd, "git push*", RiskTier.high, PolicyAction.deny, 0, 20) != null, true);

    // Case-insensitive valid subdomain allowed
    expect_eq(policy.evaluate(SandboxRequest{
        tool_name: "http_post",
        capability: CapabilityKind.net_egress,
        target: "API.OPENAI.COM",
    }).action == PolicyAction.allow, true);

    // Fake suffix domain ("evil-openai.com") must NOT match "*.openai.com"
    expect_eq(policy.evaluate(SandboxRequest{
        tool_name: "http_post",
        capability: CapabilityKind.net_egress,
        target: "evil-openai.com",
    }).action == PolicyAction.deny, true);

    // "git status" allowed, "git push origin main" denied
    expect_eq(policy.evaluate(SandboxRequest{
        tool_name: "shell",
        capability: CapabilityKind.exec_cmd,
        target: "git status",
    }).action == PolicyAction.allow, true);
    expect_eq(policy.evaluate(SandboxRequest{
        tool_name: "shell",
        capability: CapabilityKind.exec_cmd,
        target: "git push origin main",
    }).action == PolicyAction.deny, true);
}

// ============================================================================
// Test 5: Human Approval Gate Transition (Unapproved -> Approved)
// ============================================================================

fn testHumanApprovalGateTransition()! {
    let policy = ToolSandboxPolicy.init();
    expect_eq(policy.add_rule("approve-secret", CapabilityKind.secret_access, "vault/*", RiskTier.critical, PolicyAction.require_approval, 2, 10) != null, true);

    // Without approval: require_approval, quota untouched (remaining == 2)
    const d1 = policy.evaluate(SandboxRequest{
        tool_name: "get_secret",
        capability: CapabilityKind.secret_access,
        target: "vault/db_pass",
        cost_units: 1,
        human_approved: false,
    });
    expect_eq(d1.action == PolicyAction.require_approval, true);
    expect_eq(d1.has_quota, true);
    expect_eq(d1.remaining_quota, 2);

    // With human_approved = true: allowed and quota decremented to 1
    const d2 = policy.evaluate(SandboxRequest{
        tool_name: "get_secret",
        capability: CapabilityKind.secret_access,
        target: "vault/db_pass",
        cost_units: 1,
        human_approved: true,
    });
    expect_eq(d2.action == PolicyAction.allow, true);
    expect_eq(d2.remaining_quota, 1);
}

// ============================================================================
// Test 6: Session Quota Exhaustion & Reset
// ============================================================================

fn testSessionQuotaExhaustionAndReset()! {
    let policy = ToolSandboxPolicy.init();
    expect_eq(policy.add_rule("mcp-quota", CapabilityKind.mcp_tool, "db_query*", RiskTier.medium, PolicyAction.allow, 3, 10) != null, true);

    // Consume 2 units -> remaining 1
    const d1 = policy.evaluate(SandboxRequest{
        tool_name: "mcp",
        capability: CapabilityKind.mcp_tool,
        target: "db_query_users",
        cost_units: 2,
    });
    expect_eq(d1.action == PolicyAction.allow, true);
    expect_eq(d1.remaining_quota, 1);

    // Request 2 more units (> remaining 1) -> denied with quota_exceeded = true
    const d2 = policy.evaluate(SandboxRequest{
        tool_name: "mcp",
        capability: CapabilityKind.mcp_tool,
        target: "db_query_orders",
        cost_units: 2,
    });
    expect_eq(d2.action == PolicyAction.deny, true);
    expect_eq(d2.quota_exceeded, true);
    expect_eq(d2.remaining_quota, 1);

    // Request 1 unit -> succeeds, remaining 0
    const d3 = policy.evaluate(SandboxRequest{
        tool_name: "mcp",
        capability: CapabilityKind.mcp_tool,
        target: "db_query_ping",
        cost_units: 1,
    });
    expect_eq(d3.action == PolicyAction.allow, true);
    expect_eq(d3.remaining_quota, 0);

    // Reset session quotas restores full 3-unit allowance
    policy.reset_session_quotas();
    const d4 = policy.evaluate(SandboxRequest{
        tool_name: "mcp",
        capability: CapabilityKind.mcp_tool,
        target: "db_query_users",
        cost_units: 3,
    });
    expect_eq(d4.action == PolicyAction.allow, true);
    expect_eq(d4.remaining_quota, 0);
}

// ============================================================================
// Test 7: Rule Deduplication, Capacity Boundary, Audit-Only & Stats Accuracy
// ============================================================================

fn testRuleGuardsAuditOnlyAndStatsAccuracy()! {
    let policy = ToolSandboxPolicy.init();

    // Invalid empty rule_id or scope_pattern rejected
    expect_eq(policy.add_rule("", CapabilityKind.env_read, "*", RiskTier.low, PolicyAction.allow, 0, 1) == null, true);
    expect_eq(policy.add_rule("r0", CapabilityKind.env_read, "", RiskTier.low, PolicyAction.allow, 0, 1) == null, true);

    expect_eq(policy.add_rule("audit-env", CapabilityKind.env_read, "APP_*", RiskTier.low, PolicyAction.audit_only, 0, 10) != null, true);
    // Duplicate rule_id rejected
    expect_eq(policy.add_rule("audit-env", CapabilityKind.env_read, "*", RiskTier.low, PolicyAction.allow, 0, 10) == null, true);

    // audit_only action succeeds and increments allowed + audit_only_allowed
    const d_aud = policy.evaluate(SandboxRequest{
        tool_name: "getenv",
        capability: CapabilityKind.env_read,
        target: "APP_MODE",
    });
    expect_eq(d_aud.action == PolicyAction.audit_only, true);

    // Default deny increments denied
    * = policy.evaluate(SandboxRequest{
        tool_name: "getenv",
        capability: CapabilityKind.env_read,
        target: "AWS_SECRET_KEY",
    });

    // Path violation increments denied + path_violations
    * = policy.evaluate(SandboxRequest{
        tool_name: "read_file",
        capability: CapabilityKind.fs_read,
        target: "../etc/shadow",
    });

    const st: SandboxAuditStats = policy.stats();
    expect_eq(st.total_checks, 3);
    expect_eq(st.allowed, 1);
    expect_eq(st.audit_only_allowed, 1);
    expect_eq(st.denied, 2);
    expect_eq(st.path_violations, 1);
    expect_eq(st.quota_exceeded, 0);

    // Fill remaining slots up to MAX_SANDBOX_RULES (32) and verify 33rd is rejected
    let id_buf: [16]u8 = [0; 16];
    let i: usize = 1;
    while (i < MAX_SANDBOX_RULES) : (i += 1) {
        id_buf[0] = 114; // 'r'
        id_buf[1] = (48 + (i / 10)) as u8;
        id_buf[2] = (48 + (i % 10)) as u8;
        expect_eq(policy.add_rule(id_buf[0..3], CapabilityKind.mcp_tool, "t", RiskTier.low, PolicyAction.allow, 0, 1) != null, true);
    }
    expect_eq(policy.add_rule("overflow-33", CapabilityKind.mcp_tool, "t", RiskTier.low, PolicyAction.allow, 0, 1) == null, true);
}

test("agent sandbox_policy: default-deny closed posture & priority ordering", testDefaultDenyAndPriorityOrdering);
test("agent sandbox_policy: explicit deny unconditionally overrides allow", testExplicitDenyOverridesAllow);
test("agent sandbox_policy: path traversal, NUL byte & root-escape hard block", testPathTraversalNulAndRootEscape);
test("agent sandbox_policy: network subdomain wildcard & command prefix matching", testNetworkWildcardAndCommandPrefix);
test("agent sandbox_policy: human approval gate transition", testHumanApprovalGateTransition);
test("agent sandbox_policy: session quota exhaustion & reset", testSessionQuotaExhaustionAndReset);
test("agent sandbox_policy: rule guards, audit_only & SandboxAuditStats accuracy", testRuleGuardsAuditOnlyAndStatsAccuracy);
