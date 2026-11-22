// Origin: Pure-CP Stdlib (Zero-Alloc MCP Tool Execution Policy & Egress/Path Sandbox Gate) | Created: 2026-11-23
// Pure CP zero-heap-allocation MCP Tool Execution Policy & Egress/Path Sandbox Security Gate:
//   - Fixed 64-tool sandbox policy registry declaring `(permission: read_only | workspace_write |
//     network_egress | destructive, require_confirmation, allow_private_net, max_calls_per_session)`
//   - Outbound URL & IP boundary guard (`isBlockedEgressHost` / `isAllowedEgressUrl`):
//       * Blocks `localhost`, `127.0.0.0/8`, `10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`,
//         `169.254.0.0/16` (cloud metadata), `0.0.0.0/8`, `::1`, `fe80::/10`, `fc00::/7`, and
//         IPv4-mapped private IPv6 literals
//       * Permits only `http://` and `https://` schemes, rejecting `file://`, `gopher://`, etc.
//   - Path traversal (`..`) & sensitive credential file suffix guard (`isSafeSandboxPath`)
//   - Unified pre-execution gate (`McpToolSandbox.evaluateToolCall`) enforcing policy existence,
//     per-session quota, path hygiene, egress rules & confirmation tokens with zero-heap audit stats

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed capacity of the zero-heap MCP tool sandbox policy table.
pub const MCP_SANDBOX_MAX_TOOLS: usize = 64;

/// Maximum length in bytes of a registered tool name.
pub const MCP_SANDBOX_TOOL_NAME_MAX_LEN: usize = 48;

/// Maximum length in bytes of an expected human confirmation token.
pub const MCP_SANDBOX_CONFIRM_TOKEN_MAX_LEN: usize = 64;

/// Privilege tier assigned to a registered MCP tool.
pub enum SandboxPermissionLevel {
    read_only,
    workspace_write,
    network_egress,
    destructive,
}

/// Pre-execution verdict returned by `McpToolSandbox.evaluateToolCall`.
pub enum SandboxVerdict {
    allowed,
    require_confirmation,
    denied_unknown_tool,
    denied_quota,
    denied_egress,
    denied_path,
    denied_invalid_arg,
}

/// Registered sandbox policy entry for a single MCP tool.
pub struct SandboxToolRule {
    active: bool = false;
    tool_name_buf: [MCP_SANDBOX_TOOL_NAME_MAX_LEN]u8 = [0; MCP_SANDBOX_TOOL_NAME_MAX_LEN];
    tool_name_len: usize = 0;
    permission: SandboxPermissionLevel = SandboxPermissionLevel.read_only;
    require_confirmation: bool = false;
    allow_private_net: bool = false;
    max_calls_per_session: u32 = 0;
    calls_used: u32 = 0;

    /// Return the stored tool name slice.
    fn toolName(): []const u8 {
        return tool_name_buf[0..tool_name_len];
    }

    /// Return remaining session quota (`0xFFFFFFFF` if unlimited).
    fn remainingQuota(): u32 {
        if (max_calls_per_session == 0) return 0xFFFFFFFF;
        if (calls_used >= max_calls_per_session) return 0;
        return max_calls_per_session - calls_used;
    }
}

/// Tool call parameters submitted to `McpToolSandbox.evaluateToolCall`.
pub struct SandboxCallRequest {
    tool_name: []const u8 = "";
    target_path: []const u8 = "";
    egress_url: []const u8 = "";
    confirmation_token: []const u8 = "";
}

/// Detailed authorization verdict returned by `evaluateToolCall`.
pub struct SandboxDecision {
    verdict: SandboxVerdict = SandboxVerdict.denied_unknown_tool;
    allowed: bool = false;
    permission: SandboxPermissionLevel = SandboxPermissionLevel.read_only;
    calls_used: u32 = 0;
    calls_remaining: u32 = 0;
}

/// Zero-heap global audit telemetry snapshot for `McpToolSandbox`.
pub struct SandboxStats {
    tool_count: usize = 0;
    capacity: usize = MCP_SANDBOX_MAX_TOOLS;
    total_evaluated: u64 = 0;
    allowed_count: u64 = 0;
    require_confirmation_count: u64 = 0;
    denied_unknown_tool_count: u64 = 0;
    denied_quota_count: u64 = 0;
    denied_egress_count: u64 = 0;
    denied_path_count: u64 = 0;
    denied_invalid_arg_count: u64 = 0;
}

// ============================================================================
// 2. Pure Security Guards: Egress Host/URL, Path Hygiene & Formatting
// ============================================================================

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn hasCtlOrSpace(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] <= 0x20 || s[i] == 0x7F) return true;
    }
    return false;
}

fn hasCtlChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < 0x20 || s[i] == 0x7F) return true;
    }
    return false;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn constantTimeEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn asciiEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return asciiEqIgnoreCase(s[0..prefix.len], prefix);
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    return asciiEqIgnoreCase(s[s.len - suffix.len ..], suffix);
}

fn parseIpv4Octets(s: []const u8, out_octets: *[4]u8): bool {
    let part: usize = 0;
    let i: usize = 0;
    while (part < 4) : (part += 1) {
        if (i >= s.len) return false;
        let val: u32 = 0;
        let digits: usize = 0;
        while (i < s.len && s[i] != '.') : (i += 1) {
            const c = s[i];
            if (c < '0' || c > '9') return false;
            val = val * 10 + @as(u32, c - '0');
            digits += 1;
            if (digits > 3 || val > 255) return false;
        }
        if (digits == 0) return false;
        out_octets[part] = @as(u8, @intCast(val));
        if (part < 3) {
            if (i >= s.len || s[i] != '.') return false;
            i += 1;
        }
    }
    return i == s.len;
}

fn isPrivateIpv4Octets(o: [4]u8): bool {
    if (o[0] == 0) return true;                             // 0.0.0.0/8
    if (o[0] == 127) return true;                           // 127.0.0.0/8 loopback
    if (o[0] == 10) return true;                            // 10.0.0.0/8 RFC1918
    if (o[0] == 172 && o[1] >= 16 && o[1] <= 31) return true; // 172.16.0.0/12 RFC1918
    if (o[0] == 192 && o[1] == 168) return true;            // 192.168.0.0/16 RFC1918
    if (o[0] == 169 && o[1] == 254) return true;            // 169.254.0.0/16 link-local / cloud metadata
    if (o[0] == 100 && o[1] >= 64 && o[1] <= 127) return true; // 100.64.0.0/10 CGNAT
    return false;
}

/// Return true if `host` refers to a loopback, RFC1918 private, link-local, or cloud metadata
/// destination that must be blocked by default in outbound tool egress.
pub fn isBlockedEgressHost(raw_host: []const u8): bool {
    if (raw_host.len == 0 || hasCtlOrSpace(raw_host)) return true;

    let h = raw_host;
    // Strip optional brackets around IPv6 literal, e.g. `[::1]` or `[::1]:8080`.
    if (h[0] == '[') {
        let close_idx: usize = h.len;
        let i: usize = 1;
        while (i < h.len) : (i += 1) {
            if (h[i] == ']') {
                close_idx = i;
                break;
            }
        }
        if (close_idx == h.len) return true; // unclosed bracket
        h = h[1..close_idx];
    } else {
        // Strip optional `:port` only if there is a single colon (IPv4/hostname:port).
        let colon_count: usize = 0;
        let last_colon: usize = 0;
        let i: usize = 0;
        while (i < h.len) : (i += 1) {
            if (h[i] == ':') {
                colon_count += 1;
                last_colon = i;
            }
        }
        if (colon_count == 1) {
            h = h[0..last_colon];
        }
    }

    // Trim trailing DNS root dot(s), e.g. `localhost.`
    while (h.len > 0 && h[h.len - 1] == '.') {
        h = h[0 .. h.len - 1];
    }
    if (h.len == 0) return true;

    // 1. Localhost & cloud metadata hostnames
    if (asciiEqIgnoreCase(h, "localhost")) return true;
    if (endsWithIgnoreCase(h, ".localhost")) return true;
    if (asciiEqIgnoreCase(h, "localhost.localdomain")) return true;
    if (asciiEqIgnoreCase(h, "metadata.google.internal")) return true;
    if (asciiEqIgnoreCase(h, "metadata")) return true;

    // 2. IPv4 private / loopback / metadata ranges
    let octets: [4]u8 = [0; 4];
    if (parseIpv4Octets(h, *octets)) {
        return isPrivateIpv4Octets(octets);
    }

    // 3. IPv6 loopback (`::1`), unspecified (`::`), link-local (`fe80:`), ULA (`fc`/`fd`),
    //    and IPv4-mapped IPv6 (`::ffff:a.b.c.d`)
    if (asciiEqIgnoreCase(h, "::1") || asciiEqIgnoreCase(h, "::")) return true;
    if (asciiEqIgnoreCase(h, "0:0:0:0:0:0:0:1") || asciiEqIgnoreCase(h, "0:0:0:0:0:0:0:0")) return true;
    if (startsWithIgnoreCase(h, "fe80:") || startsWithIgnoreCase(h, "fe9") ||
        startsWithIgnoreCase(h, "fea") || startsWithIgnoreCase(h, "feb")) return true;
    if (startsWithIgnoreCase(h, "fc") || startsWithIgnoreCase(h, "fd")) {
        // Distinguish IPv6 ULA prefix (contains ':') from ordinary domain like `fcloud.com`
        let has_colon = false;
        let i: usize = 0;
        while (i < h.len) : (i += 1) {
            if (h[i] == ':') {
                has_colon = true;
                break;
            }
        }
        if (has_colon) return true;
    }
    if (startsWithIgnoreCase(h, "::ffff:")) {
        const mapped = h[7..];
        let m_octets: [4]u8 = [0; 4];
        if (parseIpv4Octets(mapped, *m_octets)) {
            return isPrivateIpv4Octets(m_octets);
        }
        return true;
    }

    return false;
}

/// Validate an outbound tool URL: permits only `http://` or `https://`, rejects userinfo `@`
/// confusion and control chars, and blocks private/loopback/metadata hosts unless `allow_private_net` is true.
pub fn isAllowedEgressUrl(url: []const u8, allow_private_net: bool): bool {
    if (url.len == 0 || hasCtlOrSpace(url)) return false;

    let rest: []const u8 = "";
    if (startsWithIgnoreCase(url, "https://")) {
        rest = url[8..];
    } else if (startsWithIgnoreCase(url, "http://")) {
        rest = url[7..];
    } else {
        return false; // Reject file://, gopher://, ftp://, data:, etc.
    }
    if (rest.len == 0) return false;

    // Extract authority slice up to '/', '?', or '#'
    let auth_end: usize = rest.len;
    let i: usize = 0;
    while (i < rest.len) : (i += 1) {
        const c = rest[i];
        if (c == '/' || c == '?' || c == '#') {
            auth_end = i;
            break;
        }
        // Reject userinfo (`user@host`) confusion in tool egress URLs
        if (c == '@') return false;
    }
    const authority = rest[0..auth_end];
    if (authority.len == 0) return false;

    if (!allow_private_net && isBlockedEgressHost(authority)) {
        return false;
    }
    return true;
}

/// Validate a tool target path against directory traversal (`..`) and sensitive secret files/suffixes.
pub fn isSafeSandboxPath(path: []const u8): bool {
    if (path.len == 0 || hasCtlChar(path)) return false;

    // Check for `..` traversal segments split by '/' or '\\'
    let start: usize = 0;
    let last_seg: []const u8 = "";
    while (start <= path.len) {
        let end = start;
        while (end < path.len && path[end] != '/' && path[end] != '\\') : (end += 1) {}
        const seg = path[start..end];
        if (bytesEq(seg, "..")) return false;
        if (seg.len > 0) last_seg = seg;
        if (end >= path.len) break;
        start = end + 1;
    }

    // Block common credential / key / shadow files
    if (asciiEqIgnoreCase(path, "/etc/shadow") || asciiEqIgnoreCase(path, "/etc/passwd")) return false;
    if (asciiEqIgnoreCase(last_seg, ".env") || startsWithIgnoreCase(last_seg, ".env.")) return false;
    if (asciiEqIgnoreCase(last_seg, "id_rsa") || asciiEqIgnoreCase(last_seg, "id_ed25519")) return false;
    if (asciiEqIgnoreCase(last_seg, "authorized_keys")) return false;
    if (endsWithIgnoreCase(last_seg, ".pem") ||
        endsWithIgnoreCase(last_seg, ".key") ||
        endsWithIgnoreCase(last_seg, ".p12") ||
        endsWithIgnoreCase(last_seg, ".pfx")) {
        return false;
    }

    return true;
}

/// Format a `SandboxPermissionLevel` enum variant as a canonical ASCII label.
pub fn formatSandboxPermission(perm: SandboxPermissionLevel): []const u8 {
    if (perm == SandboxPermissionLevel.read_only) return "read_only";
    if (perm == SandboxPermissionLevel.workspace_write) return "workspace_write";
    if (perm == SandboxPermissionLevel.network_egress) return "network_egress";
    return "destructive";
}

/// Format a `SandboxVerdict` enum variant as a canonical ASCII label.
pub fn formatSandboxVerdict(verdict: SandboxVerdict): []const u8 {
    if (verdict == SandboxVerdict.allowed) return "allowed";
    if (verdict == SandboxVerdict.require_confirmation) return "require_confirmation";
    if (verdict == SandboxVerdict.denied_unknown_tool) return "denied_unknown_tool";
    if (verdict == SandboxVerdict.denied_quota) return "denied_quota";
    if (verdict == SandboxVerdict.denied_egress) return "denied_egress";
    if (verdict == SandboxVerdict.denied_path) return "denied_path";
    return "denied_invalid_arg";
}

// ============================================================================
// 3. McpToolSandbox — Zero-Heap Policy & Egress/Path Sandbox Gate
// ============================================================================

/// Zero-heap-allocation MCP tool execution policy and egress/path sandbox security gate.
pub struct McpToolSandbox {
    rules: [MCP_SANDBOX_MAX_TOOLS]SandboxToolRule = [SandboxToolRule{}; MCP_SANDBOX_MAX_TOOLS];
    rule_count: usize = 0;
    confirm_token_buf: [MCP_SANDBOX_CONFIRM_TOKEN_MAX_LEN]u8 = [0; MCP_SANDBOX_CONFIRM_TOKEN_MAX_LEN];
    confirm_token_len: usize = 0;

    total_evaluated: u64 = 0;
    allowed_count: u64 = 0;
    require_confirmation_count: u64 = 0;
    denied_unknown_tool_count: u64 = 0;
    denied_quota_count: u64 = 0;
    denied_egress_count: u64 = 0;
    denied_path_count: u64 = 0;
    denied_invalid_arg_count: u64 = 0;

    /// Create an empty `McpToolSandbox`.
    static fn init(): McpToolSandbox {
        return McpToolSandbox{};
    }

    /// Configure an expected constant-time confirmation token for gated/destructive tool calls.
    /// Pass an empty slice to accept any non-empty confirmation token.
    fn setExpectedConfirmToken(*_, token: []const u8): bool {
        if (token.len > MCP_SANDBOX_CONFIRM_TOKEN_MAX_LEN || hasCtlChar(token)) return false;
        let i: usize = 0;
        while (i < token.len) : (i += 1) {
            confirm_token_buf[i] = token[i];
        }
        confirm_token_len = token.len;
        return true;
    }

    fn findToolSlot(tool_name: []const u8): usize? {
        if (tool_name.len == 0) return null;
        let i: usize = 0;
        while (i < MCP_SANDBOX_MAX_TOOLS) : (i += 1) {
            if (rules[i].active && bytesEq(rules[i].toolName(), tool_name)) {
                return i;
            }
        }
        return null;
    }

    /// Register or update a sandbox policy rule for `tool_name`.
    /// Returns `false` if `tool_name` is invalid or the 64-tool table is full.
    fn registerTool(
        *_,
        tool_name: []const u8,
        permission: SandboxPermissionLevel,
        require_confirmation: bool,
        allow_private_net: bool,
        max_calls_per_session: u32,
    ): bool {
        if (tool_name.len == 0 || tool_name.len > MCP_SANDBOX_TOOL_NAME_MAX_LEN) return false;
        if (hasCtlOrSpace(tool_name)) return false;

        let target_idx: usize = MCP_SANDBOX_MAX_TOOLS;
        let is_update = false;
        if (existing := findToolSlot(tool_name)) {
            target_idx = existing;
            is_update = true;
        } else {
            let i: usize = 0;
            while (i < MCP_SANDBOX_MAX_TOOLS) : (i += 1) {
                if (!rules[i].active) {
                    target_idx = i;
                    break;
                }
            }
        }
        if (target_idx == MCP_SANDBOX_MAX_TOOLS) return false;

        let r = SandboxToolRule{};
        r.active = true;
        let k: usize = 0;
        while (k < tool_name.len) : (k += 1) {
            r.tool_name_buf[k] = tool_name[k];
        }
        r.tool_name_len = tool_name.len;
        r.permission = permission;
        r.require_confirmation = require_confirmation;
        r.allow_private_net = allow_private_net;
        r.max_calls_per_session = max_calls_per_session;
        r.calls_used = if (is_update) rules[target_idx].calls_used else 0;

        rules[target_idx] = r;
        if (!is_update) {
            rule_count += 1;
        }
        return true;
    }

    /// Remove a tool rule from the sandbox policy table.
    fn removeTool(*_, tool_name: []const u8): bool {
        const idx = findToolSlot(tool_name) ?? return false;
        rules[idx] = SandboxToolRule{};
        if (rule_count > 0) rule_count -= 1;
        return true;
    }

    /// Retrieve a copy of a tool's sandbox rule by name.
    fn getToolRule(tool_name: []const u8): SandboxToolRule? {
        if (idx := findToolSlot(tool_name)) {
            return rules[idx];
        }
        return null;
    }

    /// Reset per-session call usage counters across all registered tools.
    fn resetSessionQuotas(*_) {
        let i: usize = 0;
        while (i < MCP_SANDBOX_MAX_TOOLS) : (i += 1) {
            if (rules[i].active) {
                rules[i].calls_used = 0;
            }
        }
    }

    /// Evaluate a tool invocation against registered policy, session quota, path hygiene,
    /// outbound egress boundaries, and human confirmation requirements.
    fn evaluateToolCall(*_, req: SandboxCallRequest): SandboxDecision {
        total_evaluated += 1;

        if (req.tool_name.len == 0 || hasCtlOrSpace(req.tool_name)) {
            denied_invalid_arg_count += 1;
            return SandboxDecision{
                verdict: SandboxVerdict.denied_invalid_arg,
                allowed: false,
            };
        }

        const idx_opt = findToolSlot(req.tool_name);
        if (idx_opt == null) {
            denied_unknown_tool_count += 1;
            return SandboxDecision{
                verdict: SandboxVerdict.denied_unknown_tool,
                allowed: false,
            };
        }
        const idx = idx_opt ?? 0;

        // 1. Per-session quota check
        if (rules[idx].max_calls_per_session > 0 && rules[idx].calls_used >= rules[idx].max_calls_per_session) {
            denied_quota_count += 1;
            return SandboxDecision{
                verdict: SandboxVerdict.denied_quota,
                allowed: false,
                permission: rules[idx].permission,
                calls_used: rules[idx].calls_used,
                calls_remaining: 0,
            };
        }

        // 2. Path traversal & sensitive file guard
        if (req.target_path.len > 0 && !isSafeSandboxPath(req.target_path)) {
            denied_path_count += 1;
            return SandboxDecision{
                verdict: SandboxVerdict.denied_path,
                allowed: false,
                permission: rules[idx].permission,
                calls_used: rules[idx].calls_used,
                calls_remaining: rules[idx].remainingQuota(),
            };
        }

        // 3. Outbound network egress guard
        if (req.egress_url.len > 0) {
            const can_egress = rules[idx].permission == SandboxPermissionLevel.network_egress ||
                rules[idx].permission == SandboxPermissionLevel.destructive;
            if (!can_egress || !isAllowedEgressUrl(req.egress_url, rules[idx].allow_private_net)) {
                denied_egress_count += 1;
                return SandboxDecision{
                    verdict: SandboxVerdict.denied_egress,
                    allowed: false,
                    permission: rules[idx].permission,
                    calls_used: rules[idx].calls_used,
                    calls_remaining: rules[idx].remainingQuota(),
                };
            }
        }

        // 4. Confirmation gate for `require_confirmation` or `destructive` tools
        const needs_confirm = rules[idx].require_confirmation ||
            rules[idx].permission == SandboxPermissionLevel.destructive;
        if (needs_confirm) {
            const confirmed: bool = if (confirm_token_len > 0)
                constantTimeEq(req.confirmation_token, confirm_token_buf[0..confirm_token_len])
            else
                req.confirmation_token.len > 0;
            if (!confirmed) {
                require_confirmation_count += 1;
                return SandboxDecision{
                    verdict: SandboxVerdict.require_confirmation,
                    allowed: false,
                    permission: rules[idx].permission,
                    calls_used: rules[idx].calls_used,
                    calls_remaining: rules[idx].remainingQuota(),
                };
            }
        }

        // 5. Permitted: charge session quota
        rules[idx].calls_used += 1;
        allowed_count += 1;
        return SandboxDecision{
            verdict: SandboxVerdict.allowed,
            allowed: true,
            permission: rules[idx].permission,
            calls_used: rules[idx].calls_used,
            calls_remaining: rules[idx].remainingQuota(),
        };
    }

    /// Return a zero-heap global audit telemetry snapshot.
    fn stats(): SandboxStats {
        return SandboxStats{
            tool_count: rule_count,
            capacity: MCP_SANDBOX_MAX_TOOLS,
            total_evaluated: total_evaluated,
            allowed_count: allowed_count,
            require_confirmation_count: require_confirmation_count,
            denied_unknown_tool_count: denied_unknown_tool_count,
            denied_quota_count: denied_quota_count,
            denied_egress_count: denied_egress_count,
            denied_path_count: denied_path_count,
            denied_invalid_arg_count: denied_invalid_arg_count,
        };
    }
}
