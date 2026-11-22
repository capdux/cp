// Origin: Pure-CP Stdlib Test Suite (lib/mcp/toolSandbox.cp) | Created: 2026-11-23
// Production Deep Verification Suite for MCP Tool Execution Policy & Egress/Path Sandbox Gate:
//   1. Outbound host/IP boundary guard (`isBlockedEgressHost`): localhost, RFC1918, cloud metadata, ::1 vs public hosts
//   2. Outbound URL scheme & userinfo guard (`isAllowedEgressUrl`): http/https only, reject file://, gopher://, user@host
//   3. Path hygiene guard (`isSafeSandboxPath`): block `..` traversal & sensitive secret files/suffixes
//   4. Unified `evaluateToolCall` gate: unknown tool, invalid arg, session quota enforcement & reset
//   5. Privilege tier enforcement (non-egress tool blocked from network) & destructive confirmation token verification
//   6. Global audit stats (`SandboxStats`), 64-tool capacity boundary & `cap.mcp` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_SANDBOX_MAX_TOOLS,
    SandboxPermissionLevel,
    SandboxVerdict,
    SandboxCallRequest,
    isBlockedEgressHost,
    isAllowedEgressUrl,
    isSafeSandboxPath,
    formatSandboxPermission,
    formatSandboxVerdict,
    McpToolSandbox,
} from "cap:mcp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("McpToolSandbox: isBlockedEgressHost blocks loopback, RFC1918, metadata & ::1") {
    // Blocked loopback / localhost
    expect_true(isBlockedEgressHost("localhost"));
    expect_true(isBlockedEgressHost("LOCALHOST."));
    expect_true(isBlockedEgressHost("api.localhost"));
    expect_true(isBlockedEgressHost("127.0.0.1"));
    expect_true(isBlockedEgressHost("127.255.0.1:8080"));
    expect_true(isBlockedEgressHost("[::1]"));
    expect_true(isBlockedEgressHost("[::1]:443"));
    expect_true(isBlockedEgressHost("::1"));

    // Blocked RFC1918 private & cloud metadata ranges
    expect_true(isBlockedEgressHost("10.0.0.1"));
    expect_true(isBlockedEgressHost("172.16.0.1"));
    expect_true(isBlockedEgressHost("172.31.255.254"));
    expect_true(isBlockedEgressHost("192.168.1.100"));
    expect_true(isBlockedEgressHost("169.254.169.254"));
    expect_true(isBlockedEgressHost("metadata.google.internal"));
    expect_true(isBlockedEgressHost("[::ffff:127.0.0.1]"));
    expect_true(isBlockedEgressHost("[fe80::1]"));
    expect_true(isBlockedEgressHost("[fd00::1]"));

    // Allowed public destinations (including boundary non-private 172.15.x / 172.32.x / domain starting with fd)
    expect_true(!isBlockedEgressHost("api.github.com"));
    expect_true(!isBlockedEgressHost("8.8.8.8"));
    expect_true(!isBlockedEgressHost("172.15.0.1"));
    expect_true(!isBlockedEgressHost("172.32.0.1"));
    expect_true(!isBlockedEgressHost("fdcloud.example.com"));
}

test("McpToolSandbox: isAllowedEgressUrl enforces http/https schemes & blocks confusion") {
    // Public HTTP/HTTPS allowed
    expect_true(isAllowedEgressUrl("https://api.openai.com/v1/responses", false));
    expect_true(isAllowedEgressUrl("http://example.com:8080/health", false));

    // Non-HTTP schemes rejected even if allow_private_net is true
    expect_true(!isAllowedEgressUrl("file:///etc/passwd", true));
    expect_true(!isAllowedEgressUrl("gopher://127.0.0.1:6379/_INFO", true));
    expect_true(!isAllowedEgressUrl("ftp://files.example.com/a.bin", false));

    // Userinfo `@` confusion rejected
    expect_true(!isAllowedEgressUrl("https://trusted.com@127.0.0.1/admin", false));

    // Private network blocked by default, allowed when tool explicitly enables allow_private_net
    expect_true(!isAllowedEgressUrl("http://169.254.169.254/latest/meta-data/", false));
    expect_true(!isAllowedEgressUrl("http://10.0.0.5:9000/internal", false));
    expect_true(isAllowedEgressUrl("http://10.0.0.5:9000/internal", true));
}

test("McpToolSandbox: isSafeSandboxPath blocks traversal & sensitive credential files") {
    expect_true(isSafeSandboxPath("src/main.cp"));
    expect_true(isSafeSandboxPath("/workspace/data/report.json"));
    expect_true(isSafeSandboxPath("docs/v1..v2/notes.md")); // `v1..v2` is not a `..` segment

    // Traversal segments rejected
    expect_true(!isSafeSandboxPath("../secret.txt"));
    expect_true(!isSafeSandboxPath("workspace/../../etc/passwd"));
    expect_true(!isSafeSandboxPath("workspace\\..\\secrets"));

    // Sensitive secret files / suffixes rejected
    expect_true(!isSafeSandboxPath("/workspace/.env"));
    expect_true(!isSafeSandboxPath("/workspace/.env.production"));
    expect_true(!isSafeSandboxPath("/workspace/tls/server.pem"));
    expect_true(!isSafeSandboxPath("/workspace/tls/private.key"));
    expect_true(!isSafeSandboxPath("/home/user/.ssh/id_rsa"));
    expect_true(!isSafeSandboxPath("/etc/shadow"));
}

test("McpToolSandbox: evaluateToolCall quota enforcement, unknown tool & path/egress denial") {
    let sb = McpToolSandbox.init();
    expect_true(sb.registerTool("read_file", SandboxPermissionLevel.read_only, false, false, 2));
    expect_true(sb.registerTool("web_fetch", SandboxPermissionLevel.network_egress, false, false, 3));

    // Invalid empty tool name & unknown tool
    expect_eq(sb.evaluateToolCall(SandboxCallRequest{ tool_name: "" }).verdict, SandboxVerdict.denied_invalid_arg);
    expect_eq(sb.evaluateToolCall(SandboxCallRequest{ tool_name: "nonexistent" }).verdict, SandboxVerdict.denied_unknown_tool);

    // read_only tool attempting outbound egress is denied
    const d_egress_deny = sb.evaluateToolCall(SandboxCallRequest{
        tool_name: "read_file",
        egress_url: "https://example.com",
    });
    expect_eq(d_egress_deny.verdict, SandboxVerdict.denied_egress);

    // Path traversal denied without consuming quota
    const d_path_deny = sb.evaluateToolCall(SandboxCallRequest{
        tool_name: "read_file",
        target_path: "../.env",
    });
    expect_eq(d_path_deny.verdict, SandboxVerdict.denied_path);

    // 2 valid calls succeed, 3rd hits denied_quota
    const ok1 = sb.evaluateToolCall(SandboxCallRequest{ tool_name: "read_file", target_path: "src/a.cp" });
    expect_true(ok1.allowed);
    expect_eq(ok1.calls_used, @as(u32, 1));
    expect_eq(ok1.calls_remaining, @as(u32, 1));

    const ok2 = sb.evaluateToolCall(SandboxCallRequest{ tool_name: "read_file", target_path: "src/b.cp" });
    expect_true(ok2.allowed);
    expect_eq(ok2.calls_remaining, @as(u32, 0));

    const q_deny = sb.evaluateToolCall(SandboxCallRequest{ tool_name: "read_file", target_path: "src/c.cp" });
    expect_true(!q_deny.allowed);
    expect_eq(q_deny.verdict, SandboxVerdict.denied_quota);

    // Reset session quotas restores allowance
    sb.resetSessionQuotas();
    expect_true(sb.evaluateToolCall(SandboxCallRequest{ tool_name: "read_file", target_path: "src/c.cp" }).allowed);
}

test("McpToolSandbox: destructive & confirmation-gated tool token verification") {
    let sb = McpToolSandbox.init();
    expect_true(sb.setExpectedConfirmToken("human-ok-999"));
    expect_true(sb.registerTool("drop_table", SandboxPermissionLevel.destructive, false, false, 5));
    expect_true(sb.registerTool("write_cfg", SandboxPermissionLevel.workspace_write, true, false, 5));

    // Missing or wrong confirmation token -> require_confirmation (does NOT consume quota)
    const r_no_tok = sb.evaluateToolCall(SandboxCallRequest{ tool_name: "drop_table" });
    expect_eq(r_no_tok.verdict, SandboxVerdict.require_confirmation);
    expect_eq(r_no_tok.calls_used, @as(u32, 0));

    const r_bad_tok = sb.evaluateToolCall(SandboxCallRequest{
        tool_name: "drop_table",
        confirmation_token: "wrong-token",
    });
    expect_eq(r_bad_tok.verdict, SandboxVerdict.require_confirmation);

    // Correct token -> allowed
    const r_ok = sb.evaluateToolCall(SandboxCallRequest{
        tool_name: "drop_table",
        confirmation_token: "human-ok-999",
    });
    expect_true(r_ok.allowed);
    expect_eq(r_ok.verdict, SandboxVerdict.allowed);
    expect_eq(r_ok.calls_used, @as(u32, 1));

    // workspace_write with require_confirmation=true also gated
    expect_eq(sb.evaluateToolCall(SandboxCallRequest{
        tool_name: "write_cfg",
        target_path: "config/app.json",
    }).verdict, SandboxVerdict.require_confirmation);
    expect_true(sb.evaluateToolCall(SandboxCallRequest{
        tool_name: "write_cfg",
        target_path: "config/app.json",
        confirmation_token: "human-ok-999",
    }).allowed);
}

test("McpToolSandbox: global audit stats, 64-tool capacity & cap.mcp facade parity") {
    let sb = cap.mcp.McpToolSandbox.init();
    expect_true(sb.registerTool("fetch", cap.mcp.SandboxPermissionLevel.network_egress, false, false, 1));

    * = sb.evaluateToolCall(cap.mcp.SandboxCallRequest{ tool_name: "fetch", egress_url: "https://example.com" }); // allowed
    * = sb.evaluateToolCall(cap.mcp.SandboxCallRequest{ tool_name: "fetch", egress_url: "https://example.com" }); // denied_quota
    * = sb.evaluateToolCall(cap.mcp.SandboxCallRequest{ tool_name: "unknown" }); // denied_unknown_tool

    const st = sb.stats();
    expect_eq(st.tool_count, @as(usize, 1));
    expect_eq(st.capacity, MCP_SANDBOX_MAX_TOOLS);
    expect_eq(st.total_evaluated, @as(u64, 3));
    expect_eq(st.allowed_count, @as(u64, 1));
    expect_eq(st.denied_quota_count, @as(u64, 1));
    expect_eq(st.denied_unknown_tool_count, @as(u64, 1));

    expect_true(bytesEq(formatSandboxPermission(SandboxPermissionLevel.network_egress), "network_egress"));
    expect_true(bytesEq(formatSandboxVerdict(SandboxVerdict.require_confirmation), "require_confirmation"));

    // Verify 64-tool capacity boundary
    let i: usize = 1;
    while (i < MCP_SANDBOX_MAX_TOOLS) : (i += 1) {
        let name_buf: [8]u8 = ['t', '_', @as(u8, @intCast('0' + (i / 10))), @as(u8, @intCast('0' + (i % 10))), 0, 0, 0, 0];
        expect_true(sb.registerTool(name_buf[0..4], SandboxPermissionLevel.read_only, false, false, 0));
    }
    expect_eq(sb.stats().tool_count, MCP_SANDBOX_MAX_TOOLS);
    expect_true(!sb.registerTool("overflow_tool", SandboxPermissionLevel.read_only, false, false, 0));
    expect_true(sb.removeTool("fetch"));
    expect_true(sb.registerTool("overflow_tool", SandboxPermissionLevel.read_only, false, false, 0));
}
