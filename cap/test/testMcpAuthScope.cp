// Origin: Pure-CP Stdlib Test Suite (lib/mcp/authScope.cp) | Created: 2026-11-23
// Production Deep Verification Suite for MCP 2025-03-26 OAuth 2.1 Resource Server Auth Scope & RFC 9728 Engine:
//   1. 64-bit named scope registry, RFC 6749 space-delimited parsing, reverse bitmask formatting & invalid token guard
//   2. 64-rule policy table CRUD, upsert, per-method audience override, saturation & slot reuse after removal
//   3. OAuth 2.1 authorization matrix: allowed, missing_token, Confused-Deputy invalid_audience, unknown_policy, AND vs ANY scopes
//   4. exp/nbf validity window boundaries with configurable clock_skew_s, missing exp & negative skew clamping
//   5. RFC 9728 / RFC 6750 WWW-Authenticate zero-alloc formatting, CRLF/quote/backslash/NUL injection rejection & decision challenge
//   6. Global telemetry stats snapshot (allowed/401/403/audience/expired/nbf/scope/unknown), reset & cap.mcp facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_AUTH_MAX_POLICIES,
    MCP_AUTH_MAX_SCOPES,
    MCP_AUTH_DEFAULT_CLOCK_SKEW_S,
    McpAuthStatus,
    McpAuthOpStatus,
    McpTokenClaims,
    McpAuthDecision,
    McpAuthScopeStats,
    isSafeHeaderParamValue,
    formatWwwAuthenticateHeader,
    McpAuthScopeEngine,
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

test("AuthScope: 64-bit scope registry, RFC 6749 parsing & reverse bitmask formatting") {
    let eng = McpAuthScopeEngine.init(
        "https://mcp.cap.dev",
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        30,
    );

    const s_tools_read = eng.registerScope("mcp:tools.read") ?? 0;
    const s_tools_exec = eng.registerScope("mcp:tools.exec") ?? 0;
    const s_res_read = eng.registerScope("mcp:resources.read") ?? 0;

    expect_eq(s_tools_read, @as(u64, 1));
    expect_eq(s_tools_exec, @as(u64, 2));
    expect_eq(s_res_read, @as(u64, 4));

    // Idempotent re-registration returns identical bitmask without consuming a new slot
    expect_eq(eng.registerScope("mcp:tools.exec") ?? 0, s_tools_exec);
    expect_eq(eng.stats().registered_scopes, @as(usize, 3));

    // Reject malformed RFC 6749 scope tokens (empty, space, quote, backslash, control char)
    expect_true(eng.registerScope("") == null);
    expect_true(eng.registerScope("bad scope") == null);
    expect_true(eng.registerScope("bad\"quote") == null);
    expect_true(eng.registerScope("bad\\slash") == null);
    expect_true(eng.registerScope("bad\nline") == null);

    // Parse space-delimited scope string with extra spaces and unknown scopes ignored
    const mask = eng.parseScopesToMask("  mcp:resources.read   unknown:scope  mcp:tools.read  ");
    expect_eq(mask, s_tools_read | s_res_read);
    expect_eq(eng.parseScopesToMask("   "), @as(u64, 0));

    // Reverse format bitmask back to canonical space-delimited scope string
    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    const n = eng.formatScopeMask(s_tools_read | s_res_read, *out_s) ?? 0;
    expect_true(bytesEq(out[0..n], "mcp:tools.read mcp:resources.read"));

    // Buffer too small returns null safely
    let tiny: [10]u8 = [0; 10];
    let tiny_s: []u8 = tiny[0..];
    expect_true(eng.formatScopeMask(s_tools_read | s_res_read, *tiny_s) == null);
}

test("AuthScope: OAuth 2.1 authorization matrix, Confused-Deputy audience guard & AND/ANY scopes") {
    let eng = McpAuthScopeEngine.init(
        "https://mcp.cap.dev/api",
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        30,
    );

    const s_read = eng.registerScope("mcp:read") ?? 0;
    const s_write = eng.registerScope("mcp:write") ?? 0;
    const s_admin = eng.registerScope("mcp:admin") ?? 0;

    // AND policy: requires both read & write (uses default audience)
    expect_eq(
        eng.addPolicy("tools/call:deploy", s_read | s_write, true, ""),
        McpAuthOpStatus.ok,
    );
    // ANY policy: requires either write or admin (uses custom per-tool audience)
    expect_eq(
        eng.addPolicy("tools/call:audit", s_write | s_admin, false, "https://audit.cap.dev/mcp"),
        McpAuthOpStatus.ok,
    );
    // Zero-scope policy: authenticated valid token only
    expect_eq(
        eng.addPolicy("ping", 0, true, ""),
        McpAuthOpStatus.ok,
    );

    const now_s: i64 = 1_700_000_000;

    // 1. Unknown policy -> fail-closed 403
    const d_unknown = eng.authorizeCall(
        "tools/call:unregistered",
        McpTokenClaims.make("https://mcp.cap.dev/api", "mcp:read mcp:write", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_unknown.status, McpAuthStatus.unknown_policy);
    expect_true(!d_unknown.allowed);
    expect_eq(d_unknown.http_status, @as(u16, 403));

    // 2. Missing bearer token -> 401 with required scopes surfaced as missing_scopes
    const d_missing = eng.authorizeCall("tools/call:deploy", McpTokenClaims.missing(), now_s);
    expect_eq(d_missing.status, McpAuthStatus.missing_token);
    expect_true(!d_missing.allowed);
    expect_eq(d_missing.http_status, @as(u16, 401));
    expect_eq(d_missing.missing_scopes, s_read | s_write);

    // 3. Confused-Deputy protection: token minted for another MCP server audience is rejected (401)
    const d_aud = eng.authorizeCall(
        "tools/call:deploy",
        McpTokenClaims.make("https://other-server.example.com", "mcp:read mcp:write", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_aud.status, McpAuthStatus.invalid_audience);
    expect_eq(d_aud.http_status, @as(u16, 401));

    // Per-method custom audience rejects default audience token
    const d_aud_override = eng.authorizeCall(
        "tools/call:audit",
        McpTokenClaims.make("https://mcp.cap.dev/api", "mcp:admin", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_aud_override.status, McpAuthStatus.invalid_audience);

    // 4. AND mode: partial scope returns insufficient_scope (403) with exact missing bitmask
    const d_partial = eng.authorizeCall(
        "tools/call:deploy",
        McpTokenClaims.make("https://mcp.cap.dev/api", "mcp:read", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_partial.status, McpAuthStatus.insufficient_scope);
    expect_eq(d_partial.http_status, @as(u16, 403));
    expect_eq(d_partial.granted_scopes, s_read);
    expect_eq(d_partial.missing_scopes, s_write);

    // 5. AND mode: full scopes -> allowed (200)
    const d_ok_and = eng.authorizeCall(
        "tools/call:deploy",
        McpTokenClaims.make("https://mcp.cap.dev/api", "mcp:write mcp:read", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_ok_and.status, McpAuthStatus.allowed);
    expect_true(d_ok_and.allowed);
    expect_eq(d_ok_and.http_status, @as(u16, 200));
    expect_eq(d_ok_and.missing_scopes, @as(u64, 0));

    // 6. ANY mode: single matching scope with custom audience -> allowed
    const d_ok_any = eng.authorizeCall(
        "tools/call:audit",
        McpTokenClaims.make("https://audit.cap.dev/mcp", "mcp:admin", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_ok_any.status, McpAuthStatus.allowed);

    // ANY mode: zero overlap -> insufficient_scope with all candidate scopes in missing_scopes
    const d_fail_any = eng.authorizeCall(
        "tools/call:audit",
        McpTokenClaims.make("https://audit.cap.dev/mcp", "mcp:read", now_s + 300, now_s - 60),
        now_s,
    );
    expect_eq(d_fail_any.status, McpAuthStatus.insufficient_scope);
    expect_eq(d_fail_any.missing_scopes, s_write | s_admin);

    // 7. Zero-scope policy: valid token with empty scope string -> allowed
    const d_ping = eng.authorizeCall(
        "ping",
        McpTokenClaims.make("https://mcp.cap.dev/api", "", now_s + 60, 0),
        now_s,
    );
    expect_eq(d_ping.status, McpAuthStatus.allowed);
}

test("AuthScope: exp/nbf time window boundaries & configurable clock skew") {
    // Negative clock skew is clamped to 0
    let eng = McpAuthScopeEngine.init(
        "https://mcp.cap.dev",
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        -15,
    );
    const s_read = eng.registerScope("mcp:read") ?? 0;
    expect_eq(eng.addPolicy("resources/read", s_read, true, ""), McpAuthOpStatus.ok);

    // With skew == 0: now == exp is valid, now == exp + 1 is expired
    const tok = McpTokenClaims.make("https://mcp.cap.dev", "mcp:read", 1_000, 900);
    expect_eq(eng.authorizeCall("resources/read", tok, 1_000).status, McpAuthStatus.allowed);
    expect_eq(eng.authorizeCall("resources/read", tok, 1_001).status, McpAuthStatus.token_expired);
    expect_eq(eng.authorizeCall("resources/read", tok, 900).status, McpAuthStatus.allowed);
    expect_eq(eng.authorizeCall("resources/read", tok, 899).status, McpAuthStatus.token_not_yet_valid);

    // Enable 30s clock skew tolerance
    eng.setClockSkew(MCP_AUTH_DEFAULT_CLOCK_SKEW_S);
    expect_eq(eng.authorizeCall("resources/read", tok, 1_030).status, McpAuthStatus.allowed);
    expect_eq(eng.authorizeCall("resources/read", tok, 1_031).status, McpAuthStatus.token_expired);
    expect_eq(eng.authorizeCall("resources/read", tok, 870).status, McpAuthStatus.allowed);
    expect_eq(eng.authorizeCall("resources/read", tok, 869).status, McpAuthStatus.token_not_yet_valid);

    // Missing exp (exp <= 0) is always rejected as expired
    const no_exp = McpTokenClaims.make("https://mcp.cap.dev", "mcp:read", 0, 0);
    expect_eq(eng.authorizeCall("resources/read", no_exp, 500).status, McpAuthStatus.token_expired);
}

test("AuthScope: RFC 9728 / RFC 6750 WWW-Authenticate formatting & CRLF/quote injection guard") {
    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];

    const n1 = formatWwwAuthenticateHeader(
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        "insufficient_scope",
        "missing write permission",
        "mcp:tools.write",
        *buf_s,
    ) ?? 0;
    const expected = "Bearer resource_metadata=\"https://mcp.cap.dev/.well-known/oauth-protected-resource\", error=\"insufficient_scope\", error_description=\"missing write permission\", scope=\"mcp:tools.write\"";
    expect_true(bytesEq(buf[0..n1], expected));

    // Omitting optional fields yields clean header without trailing commas
    const n2 = formatWwwAuthenticateHeader(
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        "",
        "",
        "mcp:read",
        *buf_s,
    ) ?? 0;
    expect_true(
        bytesEq(
            buf[0..n2],
            "Bearer resource_metadata=\"https://mcp.cap.dev/.well-known/oauth-protected-resource\", scope=\"mcp:read\"",
        ),
    );

    // Injection protection: CRLF, quote breakout, backslash, NUL & control chars all rejected
    expect_true(!isSafeHeaderParamValue("ok\r\nX-Injected: 1"));
    expect_true(!isSafeHeaderParamValue("break\"out"));
    expect_true(!isSafeHeaderParamValue("back\\slash"));
    expect_true(
        formatWwwAuthenticateHeader(
            "https://mcp.cap.dev/meta\r\nSet-Cookie: a=1",
            "invalid_token",
            "",
            "",
            *buf_s,
        ) == null,
    );
    expect_true(
        formatWwwAuthenticateHeader(
            "https://mcp.cap.dev/meta",
            "invalid_token",
            "bad\"desc",
            "",
            *buf_s,
        ) == null,
    );
    expect_true(
        formatWwwAuthenticateHeader(
            "https://mcp.cap.dev/meta",
            "invalid token!",
            "",
            "",
            *buf_s,
        ) == null,
    );
    expect_true(
        formatWwwAuthenticateHeader(
            "https://mcp.cap.dev/meta",
            "insufficient_scope",
            "",
            "mcp:read\nX-Evil: 1",
            *buf_s,
        ) == null,
    );

    // Integrated decision challenge formatting via McpAuthScopeEngine
    let eng = McpAuthScopeEngine.init(
        "https://mcp.cap.dev",
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        30,
    );
    const s_r = eng.registerScope("mcp:read") ?? 0;
    const s_w = eng.registerScope("mcp:write") ?? 0;
    * = eng.addPolicy("tools/call:edit", s_r | s_w, true, "");

    const dec_insuff = eng.authorizeCall(
        "tools/call:edit",
        McpTokenClaims.make("https://mcp.cap.dev", "mcp:read", 2_000, 1_000),
        1_500,
    );
    const cn = eng.formatChallengeForDecision(dec_insuff, *buf_s) ?? 0;
    const hdr = buf[0..cn];
    expect_true(containsSubstr(hdr, "Bearer resource_metadata=\"https://mcp.cap.dev/.well-known/oauth-protected-resource\""));
    expect_true(containsSubstr(hdr, "error=\"insufficient_scope\""));
    expect_true(containsSubstr(hdr, "scope=\"mcp:write\""));

    // Allowed decision returns null (no WWW-Authenticate challenge needed)
    const dec_ok = eng.authorizeCall(
        "tools/call:edit",
        McpTokenClaims.make("https://mcp.cap.dev", "mcp:read mcp:write", 2_000, 1_000),
        1_500,
    );
    expect_true(eng.formatChallengeForDecision(dec_ok, *buf_s) == null);
}

test("AuthScope: 64-rule policy saturation, upsert, removal & slot reuse") {
    let eng = McpAuthScopeEngine.init(
        "https://mcp.cap.dev",
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        30,
    );
    const s_a = eng.registerScope("scope:a") ?? 0;
    const s_b = eng.registerScope("scope:b") ?? 0;

    // Fill all 64 policy slots
    let name_buf: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < MCP_AUTH_MAX_POLICIES) : (i += 1) {
        name_buf[0] = 't';
        name_buf[1] = '_';
        name_buf[2] = @as(u8, @intCast('0' + (i / 10)));
        name_buf[3] = @as(u8, @intCast('0' + (i % 10)));
        expect_eq(eng.addPolicy(name_buf[0..4], s_a, true, ""), McpAuthOpStatus.ok);
    }
    expect_eq(eng.stats().active_policies, MCP_AUTH_MAX_POLICIES);

    // Duplicate and overflow rejections
    expect_eq(eng.addPolicy("t_00", s_a, true, ""), McpAuthOpStatus.already_exists);
    expect_eq(eng.addPolicy("t_overflow", s_a, true, ""), McpAuthOpStatus.capacity_full);

    // Upsert updates existing rule in place even at 100% capacity
    expect_eq(eng.upsertPolicy("t_00", s_b, false, ""), McpAuthOpStatus.ok);
    const slot0 = eng.findPolicy("t_00") ?? 99;
    const p0 = eng.getPolicy(slot0);
    expect_true(p0 != null);
    if (pol := p0) {
        expect_eq(pol.required_scopes, s_b);
        expect_true(!pol.require_all);
    }

    // Remove one policy and reuse its freed slot
    expect_eq(eng.removePolicy("t_05"), McpAuthOpStatus.ok);
    expect_eq(eng.removePolicy("t_05"), McpAuthOpStatus.not_found);
    expect_eq(eng.stats().active_policies, MCP_AUTH_MAX_POLICIES - 1);
    expect_eq(eng.addPolicy("t_reused", s_a | s_b, true, ""), McpAuthOpStatus.ok);
    expect_eq(eng.stats().active_policies, MCP_AUTH_MAX_POLICIES);
}

test("AuthScope: zero-heap global telemetry stats, reset & cap.mcp facade parity") {
    let eng = cap.mcp.McpAuthScopeEngine.init(
        "https://mcp.cap.dev",
        "https://mcp.cap.dev/.well-known/oauth-protected-resource",
        10,
    );
    const s_exec = eng.registerScope("mcp:exec") ?? 0;
    * = eng.addPolicy("tools/call:run", s_exec, true, "");

    const now_s: i64 = 5_000;
    // 1. allowed
    * = eng.authorizeCall("tools/call:run", McpTokenClaims.makeWithSubject("agent-1", "https://mcp.cap.dev", "", s_exec, 6_000, 4_000), now_s);
    // 2. missing_token (401)
    * = eng.authorizeCall("tools/call:run", McpTokenClaims.missing(), now_s);
    // 3. invalid_audience (401)
    * = eng.authorizeCall("tools/call:run", McpTokenClaims.make("https://wrong.dev", "mcp:exec", 6_000, 4_000), now_s);
    // 4. token_expired (401)
    * = eng.authorizeCall("tools/call:run", McpTokenClaims.make("https://mcp.cap.dev", "mcp:exec", 4_900, 4_000), now_s);
    // 5. token_not_yet_valid (401)
    * = eng.authorizeCall("tools/call:run", McpTokenClaims.make("https://mcp.cap.dev", "mcp:exec", 6_000, 5_100), now_s);
    // 6. insufficient_scope (403)
    * = eng.authorizeCall("tools/call:run", McpTokenClaims.make("https://mcp.cap.dev", "", 6_000, 4_000), now_s);
    // 7. unknown_policy (403)
    * = eng.authorizeCall("tools/call:nope", McpTokenClaims.make("https://mcp.cap.dev", "mcp:exec", 6_000, 4_000), now_s);

    const st: McpAuthScopeStats = eng.stats();
    expect_eq(st.registered_scopes, @as(usize, 1));
    expect_eq(st.active_policies, @as(usize, 1));
    expect_eq(st.policy_capacity, MCP_AUTH_MAX_POLICIES);
    expect_eq(st.total_checks, @as(u64, 7));
    expect_eq(st.allowed_count, @as(u64, 1));
    expect_eq(st.unauthorized_401_count, @as(u64, 4));
    expect_eq(st.forbidden_403_count, @as(u64, 2));
    expect_eq(st.missing_token_count, @as(u64, 1));
    expect_eq(st.invalid_audience_count, @as(u64, 1));
    expect_eq(st.token_expired_count, @as(u64, 1));
    expect_eq(st.token_not_yet_valid_count, @as(u64, 1));
    expect_eq(st.insufficient_scope_count, @as(u64, 1));
    expect_eq(st.unknown_policy_count, @as(u64, 1));

    // Reset stats clears counters while keeping registered scopes & policies intact
    eng.resetStats();
    const st2 = eng.stats();
    expect_eq(st2.total_checks, @as(u64, 0));
    expect_eq(st2.allowed_count, @as(u64, 0));
    expect_eq(st2.registered_scopes, @as(usize, 1));
    expect_eq(st2.active_policies, @as(usize, 1));
    expect_eq(MCP_AUTH_MAX_SCOPES, @as(usize, 64));
}
