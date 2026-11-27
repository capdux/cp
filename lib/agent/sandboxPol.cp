// Origin: Pure-CP Stdlib (Zero-Alloc AI Agent Tool Sandbox & Capability Policy Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation capability authorization, path-boundary enforcement,
// human-approval gate, and per-session quota policy engine for AI Agent tool calls:
//   - Capabilities: fs_read, fs_write, net_egress, exec_cmd, env_read, secret_access, mcp_tool
//   - Deterministic evaluation: path traversal hard-block -> explicit deny overrides allow ->
//     highest-priority matching rule -> default-deny closed posture
//   - Zero-alloc path normalization rejecting `..`, NUL bytes, and root-prefix escapes
//   - Per-rule session call quotas, human-approval gating, and zero-alloc audit statistics

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const MAX_SANDBOX_RULES: usize = 32;
pub const MAX_RULE_ID_LEN: usize = 48;
pub const MAX_SCOPE_PATTERN_LEN: usize = 128;
pub const MAX_NORMALIZED_PATH_LEN: usize = 256;

fn copyBytes(dst: *[]u8, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len && i < dst.len) : (i += 1) {
        dst[i] = src[i];
    }
    return i;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

fn startsWithBytes(s: []const u8, prefix: []const u8, ignore_case: bool): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (ignore_case) {
            if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) {
                return false;
            }
        } else {
            if (s[i] != prefix[i]) {
                return false;
            }
        }
    }
    return true;
}

fn endsWithBytes(s: []const u8, suffix: []const u8, ignore_case: bool): bool {
    if (s.len < suffix.len) {
        return false;
    }
    const off: usize = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (ignore_case) {
            if (toLowerAscii(s[off + i]) != toLowerAscii(suffix[i])) {
                return false;
            }
        } else {
            if (s[off + i] != suffix[i]) {
                return false;
            }
        }
    }
    return true;
}

fn matchScopePattern(pattern: []const u8, target: []const u8, cap: CapabilityKind): bool {
    if (pattern.len == 0) {
        return false;
    }
    // Wildcard matches everything
    if (pattern.len == 1 && pattern[0] == 42) { // '*'
        return true;
    }
    const ci: bool = cap == CapabilityKind.net_egress;

    // Leading wildcard subdomain pattern: "*.example.com"
    if (pattern.len >= 3 && pattern[0] == 42 && pattern[1] == 46) { // "*."
        const suffix = pattern[1..]; // ".example.com"
        return target.len > suffix.len && endsWithBytes(target, suffix, ci);
    }

    // Trailing wildcard prefix pattern: "/workspace/*" or "git *"
    if (pattern.len >= 2 && pattern[pattern.len - 1] == 42) { // ends with '*'
        const prefix = pattern[0 .. pattern.len - 1];
        if (startsWithBytes(target, prefix, ci)) {
            return true;
        }
        // Allow "/workspace/*" to also match exact directory "/workspace"
        if (prefix.len >= 2 && prefix[prefix.len - 1] == 47) { // ends with '/'
            const bare = prefix[0 .. prefix.len - 1];
            return if (ci) eqlIgnoreCaseAscii(target, bare) else std.mem.eql(u8, target, bare);
        }
        return false;
    }

    // Exact match
    return if (ci) eqlIgnoreCaseAscii(target, pattern) else std.mem.eql(u8, target, pattern);
}

// ============================================================================
// 1. Capability, Risk, Action, Rule, Request, Decision & Audit Models
// ============================================================================

/// Tool capability category governed by `ToolSandboxPolicy`.
pub enum CapabilityKind {
    fs_read,
    fs_write,
    net_egress,
    exec_cmd,
    env_read,
    secret_access,
    mcp_tool,
}

/// Risk classification tier for a sandbox rule.
pub enum RiskTier {
    low,
    medium,
    high,
    critical,
}

/// Policy enforcement outcome.
pub enum PolicyAction {
    allow,
    deny,
    require_approval,
    audit_only,
}

/// Fixed-size inline sandbox authorization rule with optional per-session call quota.
pub struct SandboxRule {
    rule_id_buf: [48]u8 = [0; 48];
    rule_id_len: usize = 0;
    capability: CapabilityKind = CapabilityKind.fs_read;
    scope_buf: [128]u8 = undefined;
    scope_len: usize = 0;
    risk_tier: RiskTier = RiskTier.low;
    action: PolicyAction = PolicyAction.deny;
    max_calls: u32 = 0;
    used_calls: u32 = 0;
    priority: u16 = 0;
    occupied: bool = false;

    /// Returns the rule ID as a borrowed byte slice.
    fn ruleIdSlice(): []const u8 {
        return _.rule_id_buf[0.._.rule_id_len];
    }

    /// Returns the scope pattern as a borrowed byte slice.
    fn scopePatternSlice(): []const u8 {
        return _.scope_buf[0.._.scope_len];
    }

    /// Returns remaining session quota (0 if unlimited or exhausted).
    fn remainingQuota(): u32 {
        if (_.max_calls == 0 || _.used_calls >= _.max_calls) {
            return 0;
        }
        return _.max_calls - _.used_calls;
    }
}

/// Incoming tool invocation request submitted to `ToolSandboxPolicy.evaluate`.
pub struct SandboxRequest {
    tool_name: []const u8 = "";
    capability: CapabilityKind = CapabilityKind.fs_read;
    target: []const u8 = "";
    cost_units: u32 = 1;
    human_approved: bool = false;
}

/// Deterministic evaluation verdict returned by `ToolSandboxPolicy.evaluate`.
pub struct SandboxPolicyDecision {
    action: PolicyAction = PolicyAction.deny;
    matched: bool = false;
    rule_id_buf: [48]u8 = [0; 48];
    rule_id_len: usize = 0;
    risk_tier: RiskTier = RiskTier.low;
    path_violation: bool = false;
    quota_exceeded: bool = false;
    has_quota: bool = false;
    remaining_quota: u32 = 0;

    /// Returns the matched rule ID as a borrowed byte slice (empty on default-deny or path violation).
    fn ruleIdSlice(): []const u8 {
        return _.rule_id_buf[0.._.rule_id_len];
    }
}

/// Zero-allocation audit telemetry counters for `ToolSandboxPolicy`.
pub struct SandboxAuditStats {
    total_checks: u64 = 0;
    allowed: u64 = 0;
    denied: u64 = 0;
    approval_required: u64 = 0;
    audit_only_allowed: u64 = 0;
    path_violations: u64 = 0;
    quota_exceeded: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc Tool Sandbox Policy Engine
// ============================================================================

/// Zero-heap-allocation AI Agent tool capability sandbox and approval gate engine.
/// Evaluates rules with strict security precedence:
///   1. Path traversal / NUL / root-escape hard block (for `fs_read` / `fs_write`)
///   2. Explicit `deny` overrides all `allow` / `require_approval` / `audit_only` rules
///   3. Highest-priority matching rule
///   4. Default-deny closed posture when no rule matches
pub struct ToolSandboxPolicy {
    rules: [32]SandboxRule = [SandboxRule{}; 32];
    rule_count: usize = 0;
    allowed_root_buf: [128]u8 = undefined;
    allowed_root_len: usize = 0;
    audit: SandboxAuditStats = SandboxAuditStats{};

    /// Initializes an empty default-deny sandbox policy with no global filesystem root restriction.
    static fn init(): ToolSandboxPolicy {
        return ToolSandboxPolicy{
            rules: [SandboxRule{}; 32],
            rule_count: 0,
            allowed_root_buf: undefined,
            allowed_root_len: 0,
            audit: SandboxAuditStats{},
        };
    }

    /// Configures a global authorized filesystem root prefix (e.g., `"/workspace"`).
    /// All `fs_read` / `fs_write` targets must reside inside this normalized root.
    /// Returns `false` if `root_path` is invalid, contains `..`/NUL, or exceeds buffer limits.
    fn set_allowed_root(*_, root_path: []const u8): bool {
        if (root_path.len == 0 || root_path.len > 128) {
            return false;
        }
        let norm_buf: [256]u8 = undefined;
        let norm_s: []u8 = norm_buf[0..];
        const norm_len = ToolSandboxPolicy.normalize_path_raw(root_path, norm_s) ?? return false;
        if (norm_len == 0 || norm_len > 128) {
            return false;
        }
        let dst: []u8 = _.allowed_root_buf[0..];
        _.allowed_root_len = copyBytes(dst, norm_buf[0..norm_len]);
        return true;
    }

    /// Pure zero-alloc path normalizer:
    /// - Rejects empty paths, NUL (`0x00`) bytes, and any `..` parent traversal segments
    /// - Collapses duplicate separators (`/` or `\`) and `.` segments into canonical `/`-delimited form
    /// Returns normalized length written into `out_norm`, or `null` on violation / overflow.
    static fn normalize_path_raw(path: []const u8, out_norm: *[]u8): usize? {
        if (path.len == 0 || out_norm.len == 0) {
            return null;
        }
        // Reject NUL bytes anywhere in path
        let k: usize = 0;
        while (k < path.len) : (k += 1) {
            if (path[k] == 0) {
                return null;
            }
        }

        const is_abs: bool = path[0] == 47 || path[0] == 92; // '/' or '\'
        let out_len: usize = 0;
        if (is_abs) {
            out_norm[0] = 47; // '/'
            out_len = 1;
        }

        let i: usize = 0;
        while (i < path.len) {
            while (i < path.len && (path[i] == 47 || path[i] == 92)) : (i += 1) {}
            if (i >= path.len) {
                break;
            }
            const seg_start: usize = i;
            while (i < path.len && path[i] != 47 && path[i] != 92) : (i += 1) {}
            const seg = path[seg_start..i];

            // Hard-reject parent directory traversal ("..")
            if (seg.len == 2 && seg[0] == 46 && seg[1] == 46) {
                return null;
            }
            // Skip current-directory no-op (".")
            if (seg.len == 1 && seg[0] == 46) {
                continue;
            }

            // Emit separator if needed
            if (out_len > 0 && out_norm[out_len - 1] != 47) {
                if (out_len >= out_norm.len) {
                    return null;
                }
                out_norm[out_len] = 47;
                out_len += 1;
            }

            if (seg.len > out_norm.len - out_len) {
                return null;
            }
            if (seg.len > 0) {
                @memcpy(out_norm[out_len .. out_len + seg.len], seg);
            }
            out_len += seg.len;
        }

        if (out_len == 0) {
            // Path consisted solely of "."
            if (out_norm.len < 1) {
                return null;
            }
            out_norm[0] = 46; // '.'
            out_len = 1;
        }
        return out_len;
    }

    /// Normalizes `path` into `out_norm` and enforces `allowed_root` containment if configured.
    /// Returns normalized byte length, or `null` if path is malformed, traverses `..`,
    /// contains NUL bytes, or escapes `allowed_root`.
    fn normalize_and_check_path(path: []const u8, out_norm: *[]u8): usize? {
        const n = ToolSandboxPolicy.normalize_path_raw(path, out_norm) ?? return null;
        if (_.allowed_root_len > 0) {
            const root = _.allowed_root_buf[0.._.allowed_root_len];
            const norm = out_norm[0..n];
            if (std.mem.eql(u8, root, "/")) {
                if (norm.len == 0 || norm[0] != 47) {
                    return null;
                }
            } else {
                if (norm.len < root.len || !startsWithBytes(norm, root, false)) {
                    return null;
                }
                // Guard against sibling prefix escape (e.g. "/workspace_evil" vs "/workspace")
                if (norm.len > root.len && norm[root.len] != 47) {
                    return null;
                }
            }
        }
        return n;
    }

    /// Finds an existing rule index by `rule_id`, or `null` if absent.
    fn find_rule(rule_id: []const u8): usize? {
        let i: usize = 0;
        while (i < _.rule_count) : (i += 1) {
            if (_.rules[i].occupied && std.mem.eql(u8, _.rules[i].ruleIdSlice(), rule_id)) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new sandbox rule with ID deduplication and fixed capacity (32) enforcement.
    /// Returns the assigned rule index, or `null` on invalid fields, duplicate ID, or full table.
    fn add_rule(
        *_,
        rule_id: []const u8,
        capability: CapabilityKind,
        scope_pattern: []const u8,
        risk_tier: RiskTier,
        action: PolicyAction,
        max_calls: u32,
        priority: u16,
    ): usize? {
        if (rule_id.len == 0 || rule_id.len > MAX_RULE_ID_LEN) {
            return null;
        }
        if (scope_pattern.len == 0 || scope_pattern.len > MAX_SCOPE_PATTERN_LEN) {
            return null;
        }
        if (_.rule_count >= MAX_SANDBOX_RULES) {
            return null;
        }
        if (_.find_rule(rule_id) != null) {
            return null;
        }

        let rule = SandboxRule{
            capability: capability,
            risk_tier: risk_tier,
            action: action,
            max_calls: max_calls,
            used_calls: 0,
            priority: priority,
            occupied: true,
        };
        let id_dst: []u8 = rule.rule_id_buf[0..];
        let sc_dst: []u8 = rule.scope_buf[0..];
        rule.rule_id_len = copyBytes(id_dst, rule_id);
        rule.scope_len = copyBytes(sc_dst, scope_pattern);

        const idx: usize = _.rule_count;
        _.rules[idx] = rule;
        _.rule_count += 1;
        return idx;
    }

    /// Evaluates `req` deterministically against registered rules, enforcing path safety,
    /// explicit-deny precedence, human-approval gating, session quotas, and audit telemetry.
    fn evaluate(*_, req: SandboxRequest): SandboxPolicyDecision {
        _.audit.total_checks += 1;

        let norm_buf: [256]u8 = undefined;
        let effective_target: []const u8 = req.target;

        // 1. Hard path boundary & traversal check for filesystem capabilities
        if (req.capability == CapabilityKind.fs_read || req.capability == CapabilityKind.fs_write) {
            let norm_s: []u8 = norm_buf[0..];
            if (n := _.normalize_and_check_path(req.target, norm_s)) {
                effective_target = norm_buf[0..n];
            } else {
                _.audit.denied += 1;
                _.audit.path_violations += 1;
                return SandboxPolicyDecision{
                    action: PolicyAction.deny,
                    matched: false,
                    risk_tier: RiskTier.critical,
                    path_violation: true,
                };
            }
        }

        // 2. Scan all matching rules: explicit deny ALWAYS overrides allow/approval/audit_only
        let best_deny_idx: usize? = null;
        let best_deny_prio: u16 = 0;
        let best_other_idx: usize? = null;
        let best_other_prio: u16 = 0;

        let i: usize = 0;
        while (i < _.rule_count) : (i += 1) {
            const r = _.rules[i];
            if (!r.occupied || r.capability != req.capability) {
                continue;
            }
            if (!matchScopePattern(r.scopePatternSlice(), effective_target, req.capability)) {
                continue;
            }
            if (r.action == PolicyAction.deny) {
                if (best_deny_idx == null || r.priority > best_deny_prio) {
                    best_deny_idx = i;
                    best_deny_prio = r.priority;
                }
            } else {
                if (best_other_idx == null || r.priority > best_other_prio) {
                    best_other_idx = i;
                    best_other_prio = r.priority;
                }
            }
        }

        // Explicit deny wins unconditionally
        if (d_idx := best_deny_idx) {
            _.audit.denied += 1;
            let verdict = SandboxPolicyDecision{
                action: PolicyAction.deny,
                matched: true,
                risk_tier: _.rules[d_idx].risk_tier,
                has_quota: _.rules[d_idx].max_calls > 0,
                remaining_quota: _.rules[d_idx].remainingQuota(),
            };
            let dst: []u8 = verdict.rule_id_buf[0..];
            verdict.rule_id_len = copyBytes(dst, _.rules[d_idx].ruleIdSlice());
            return verdict;
        }

        // 3. Default-deny closed posture when no rule matches
        let m_idx: usize = 0;
        if (idx := best_other_idx) {
            m_idx = idx;
        } else {
            _.audit.denied += 1;
            return SandboxPolicyDecision{
                action: PolicyAction.deny,
                matched: false,
                risk_tier: RiskTier.high,
            };
        }

        let verdict = SandboxPolicyDecision{
            action: _.rules[m_idx].action,
            matched: true,
            risk_tier: _.rules[m_idx].risk_tier,
            has_quota: _.rules[m_idx].max_calls > 0,
            remaining_quota: _.rules[m_idx].remainingQuota(),
        };
        let dst: []u8 = verdict.rule_id_buf[0..];
        verdict.rule_id_len = copyBytes(dst, _.rules[m_idx].ruleIdSlice());

        // 4. Human approval gate check (does not consume quota while pending approval)
        if (_.rules[m_idx].action == PolicyAction.require_approval && !req.human_approved) {
            _.audit.approval_required += 1;
            verdict.action = PolicyAction.require_approval;
            return verdict;
        }

        // 5. Per-session quota check & deduction
        if (_.rules[m_idx].max_calls > 0) {
            const cost: u32 = if (req.cost_units == 0) 1 else req.cost_units;
            const rem: u32 = _.rules[m_idx].remainingQuota();
            if (cost > rem) {
                _.audit.denied += 1;
                _.audit.quota_exceeded += 1;
                verdict.action = PolicyAction.deny;
                verdict.quota_exceeded = true;
                verdict.remaining_quota = rem;
                return verdict;
            }
            _.rules[m_idx].used_calls += cost;
            verdict.remaining_quota = _.rules[m_idx].remainingQuota();
        }

        // 6. Final allow / audit_only verdict
        if (_.rules[m_idx].action == PolicyAction.audit_only) {
            _.audit.allowed += 1;
            _.audit.audit_only_allowed += 1;
            verdict.action = PolicyAction.audit_only;
            return verdict;
        }

        _.audit.allowed += 1;
        verdict.action = PolicyAction.allow;
        return verdict;
    }

    /// Resets per-session `used_calls` counters across all rules without removing rules.
    fn reset_session_quotas(*_) {
        let i: usize = 0;
        while (i < _.rule_count) : (i += 1) {
            _.rules[i].used_calls = 0;
        }
    }

    /// Returns a snapshot of zero-allocation audit telemetry counters.
    fn stats(): SandboxAuditStats {
        return _.audit;
    }
}
