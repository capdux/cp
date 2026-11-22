// Origin: Pure-CP Stdlib (MCP Tool Registry, Capability ACL & Rate-Limited Router) | Created: 2026-11-22
// Zero-heap-allocation Model Context Protocol (MCP) tool registry with capability risk tiers,
// network ACL, human-in-the-loop approval gating, and fixed-window per-minute rate limiting.

const std = @import("std");

const RATE_WINDOW_MS: i64 = 60000;

/// Capability risk classification for MCP tools.
pub enum ToolRiskLevel {
    ReadOnly,
    WriteLocal,
    Network,
    Destructive,
}

/// Agent-scoped security policy governing tool visibility and execution.
pub struct ToolPolicy {
    max_risk: ToolRiskLevel = ToolRiskLevel.ReadOnly;
    require_approval_for_destructive: bool = true;
    allow_network: bool = false;
}

/// Deterministic authorization outcome returned by `McpToolRegistry.authorizeAndConsume`.
pub enum PolicyDecision {
    Allow,
    RequireApproval,
    DenyRisk,
    DenyNetwork,
    DenyRateLimit,
    ToolNotFound,
    ToolDisabled,
}

/// Fixed-slot descriptor and telemetry record for a registered MCP tool.
pub struct RegisteredTool {
    name: string = "";
    description: string = "";
    risk: ToolRiskLevel = ToolRiskLevel.ReadOnly;
    requires_network: bool = false;
    enabled: bool = false;
    rate_limit_per_min: u32 = 0;
    window_start_ms: i64 = 0;
    window_calls: u32 = 0;
    total_calls: u64 = 0;
    total_errors: u64 = 0;
}

fn riskLevelRank(r: ToolRiskLevel): u8 {
    return switch (r) {
        ToolRiskLevel.ReadOnly => 0,
        ToolRiskLevel.WriteLocal => 1,
        ToolRiskLevel.Network => 2,
        ToolRiskLevel.Destructive => 3,
    };
}

/// Fixed-capacity, zero-heap-allocation MCP tool registry and policy router.
pub struct McpToolRegistry {
    slots: []RegisteredTool;
    count: usize = 0;

    /// Initializes a registry backed by caller-owned `slots` buffer.
    static fn init(slots: *[]RegisteredTool): McpToolRegistry {
        return McpToolRegistry{
            slots: slots,
            count: 0,
        };
    }

    /// Returns the number of registered tools.
    fn len(): usize {
        return _.count;
    }

    /// Returns the maximum capacity of the backing slot pool.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Finds the slot index of a registered tool by exact `name`, or `null` if absent.
    fn findIndex(name: string): usize? {
        if (name.len == 0) return null;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (std.mem.eql(u8, _.slots[i].name, name)) {
                return i;
            }
        }
        return null;
    }

    /// Returns a copy of the registered tool record at `index`, or `null` if out of bounds.
    fn get(index: usize): RegisteredTool? {
        if (index >= _.count) return null;
        return _.slots[index];
    }

    /// Registers a new MCP tool into the fixed pool.
    /// Rejects empty `name`, duplicate `name`, or full pool by returning `null`.
    /// `rate_limit_per_min == 0` disables rate limiting for this tool.
    fn register(
        *_,
        name: string,
        description: string,
        risk: ToolRiskLevel,
        requires_network: bool,
        rate_limit_per_min: u32,
    ): usize? {
        if (name.len == 0) return null;
        if (_.count >= _.slots.len) return null;
        if (_.findIndex(name) != null) return null;

        const idx = _.count;
        _.slots[idx] = RegisteredTool{
            name: name,
            description: description,
            risk: risk,
            requires_network: requires_network,
            enabled: true,
            rate_limit_per_min: rate_limit_per_min,
            window_start_ms: -1,
            window_calls: 0,
            total_calls: 0,
            total_errors: 0,
        };
        _.count += 1;
        return idx;
    }

    /// Enables or disables a registered tool by `name`. Returns `false` if not found.
    fn setEnabled(*_, name: string, enabled: bool): bool {
        if (idx := _.findIndex(name)) {
            _.slots[idx].enabled = enabled;
            return true;
        }
        return false;
    }

    /// Evaluates existence, enabled flag, network ACL, risk tier, destructive approval gate,
    /// and fixed 60-second rate-limit window. Atomically increments call counters only on `Allow`.
    /// Negative timestamps are clamped to `0` and backward clock skew never resets an active window.
    fn authorizeAndConsume(*_, name: string, policy: ToolPolicy, now_ms: i64): PolicyDecision {
        const idx_opt = _.findIndex(name);
        if (idx_opt == null) return PolicyDecision.ToolNotFound;
        const idx = idx_opt ?? 0;

        if (!_.slots[idx].enabled) {
            return PolicyDecision.ToolDisabled;
        }

        if ((_.slots[idx].requires_network || _.slots[idx].risk == ToolRiskLevel.Network) && !policy.allow_network) {
            return PolicyDecision.DenyNetwork;
        }

        if (riskLevelRank(_.slots[idx].risk) > riskLevelRank(policy.max_risk)) {
            return PolicyDecision.DenyRisk;
        }

        if (_.slots[idx].risk == ToolRiskLevel.Destructive && policy.require_approval_for_destructive) {
            return PolicyDecision.RequireApproval;
        }

        if (_.slots[idx].rate_limit_per_min > 0) {
            const safe_now: i64 = if (now_ms < 0) 0 else now_ms;
            if (_.slots[idx].window_start_ms < 0) {
                _.slots[idx].window_start_ms = safe_now;
                _.slots[idx].window_calls = 0;
            } else if (safe_now >= _.slots[idx].window_start_ms) {
                const elapsed = safe_now - _.slots[idx].window_start_ms;
                if (elapsed >= RATE_WINDOW_MS) {
                    _.slots[idx].window_start_ms = safe_now;
                    _.slots[idx].window_calls = 0;
                }
            }

            if (_.slots[idx].window_calls >= _.slots[idx].rate_limit_per_min) {
                return PolicyDecision.DenyRateLimit;
            }
            _.slots[idx].window_calls += 1;
        }

        _.slots[idx].total_calls += 1;
        return PolicyDecision.Allow;
    }

    /// Increments the execution error counter for a registered tool. Returns `false` if not found.
    fn recordError(*_, name: string): bool {
        if (idx := _.findIndex(name)) {
            _.slots[idx].total_errors += 1;
            return true;
        }
        return false;
    }

    /// Writes indices of all enabled tools permitted by `policy` into `out_indices`
    /// (for dynamic `tools/list` filtering) and returns the number of indices written.
    fn listAllowedTools(policy: ToolPolicy, out_indices: *[]usize): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const tool = _.slots[i];
            if (!tool.enabled) continue;
            if ((tool.requires_network || tool.risk == ToolRiskLevel.Network) && !policy.allow_network) {
                continue;
            }
            if (riskLevelRank(tool.risk) > riskLevelRank(policy.max_risk)) {
                continue;
            }
            if (written < out_indices.len) {
                out_indices[written] = i;
                written += 1;
            }
        }
        return written;
    }
}
