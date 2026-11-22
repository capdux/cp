// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc MCP In-Flight Concurrency Gate, Cancellation Registry & Timeout Sweeper) | Created: 2026-11-22
// Pure CP zero-heap-allocation Model Context Protocol (MCP) In-Flight Request Gatekeeper:
//   - Fixed inline 64-slot in-flight JSON-RPC request table (`MCP_GATE_MAX_INFLIGHT`) with 0 dynamic heap allocation
//   - Dual-level concurrency admission control: global total cap + per-method class caps (`tools/call`, `resources/read`, etc.)
//   - Active duplicate `req_id` rejection with post-completion ID reuse
//   - Idempotent single-request (`notifications/cancelled`) and batch cancellation (`cancel_batch`)
//   - Non-lossy batched deadline timeout sweeper (`sweep_expired`) reclaiming concurrency quotas cleanly

// ============================================================================
// 1. Constants, Enums & Public Models
// ============================================================================

/// Maximum concurrent in-flight MCP requests tracked inline in `McpInflightGate`.
pub const MCP_GATE_MAX_INFLIGHT: usize = 64;

/// MCP JSON-RPC method class used for per-method concurrency isolation.
pub enum McpMethodClass {
    tools_call,
    resources_read,
    prompts_get,
    sampling_create,
    other,
}

/// Lifecycle state of an in-flight MCP request slot.
pub enum McpInflightState {
    running,
    cancel_requested,
    completed,
    timed_out,
}

/// Global and per-method concurrency limits for `McpInflightGate`.
pub struct McpGateLimits {
    max_total_inflight: usize = 64;
    max_tools_call: usize = 16;
    max_resources_read: usize = 32;
    max_prompts_get: usize = 16;
    max_sampling_create: usize = 8;

    /// Returns production default limits (`total=64, tools=16, resources=32, prompts=16, sampling=8`).
    static fn defaults(): McpGateLimits {
        return McpGateLimits{
            max_total_inflight: 64,
            max_tools_call: 16,
            max_resources_read: 32,
            max_prompts_get: 16,
            max_sampling_create: 8,
        };
    }

    /// Validates that `1 <= max_total_inflight <= MCP_GATE_MAX_INFLIGHT` and no per-method cap exceeds total.
    fn isValid(): bool {
        return _.max_total_inflight >= 1 &&
            _.max_total_inflight <= MCP_GATE_MAX_INFLIGHT &&
            _.max_tools_call <= _.max_total_inflight &&
            _.max_resources_read <= _.max_total_inflight &&
            _.max_prompts_get <= _.max_total_inflight &&
            _.max_sampling_create <= _.max_total_inflight;
    }
}

/// Admission outcome returned by `McpInflightGate.admit_request`.
pub struct McpAdmitOutcome {
    admitted: bool = false;
    slot_idx: usize = 0;
    rejected_duplicate_id: bool = false;
    rejected_total_cap: bool = false;
    rejected_method_cap: bool = false;
}

/// Single in-flight request slot stored inline in `McpInflightGate`.
pub struct McpInflightSlot {
    req_id: u64 = 0;
    batch_id: u64 = 0;
    method_class: McpMethodClass = McpMethodClass.other;
    state: McpInflightState = McpInflightState.running;
    started_ms: u64 = 0;
    deadline_ms: u64 = 0;
    occupied: bool = false;
}

/// Telemetry & active concurrency snapshot returned by `McpInflightGate.stats()`.
pub struct McpGateStats {
    active_total: usize = 0;
    active_tools_call: usize = 0;
    active_cancel_pending: usize = 0;
    total_admitted: u64 = 0;
    total_completed: u64 = 0;
    total_canceled: u64 = 0;
    total_timed_out: u64 = 0;
    total_rejected: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc MCP In-Flight Concurrency Gate
// ============================================================================

/// Zero-heap-allocation MCP in-flight request gatekeeper with per-method concurrency limits,
/// cancellation token registry, and batched timeout sweeper.
pub struct McpInflightGate {
    limits: McpGateLimits = McpGateLimits{};
    slots: [MCP_GATE_MAX_INFLIGHT]McpInflightSlot = [McpInflightSlot{}; MCP_GATE_MAX_INFLIGHT];

    active_total: usize = 0;
    active_tools_call: usize = 0;
    active_resources_read: usize = 0;
    active_prompts_get: usize = 0;
    active_sampling_create: usize = 0;
    active_cancel_pending: usize = 0;

    total_admitted: u64 = 0;
    total_completed: u64 = 0;
    total_canceled: u64 = 0;
    total_timed_out: u64 = 0;
    total_rejected: u64 = 0;

    /// Initializes an `McpInflightGate` after validating `limits`. Returns `null` if invalid.
    static fn init(limits: McpGateLimits): McpInflightGate? {
        if (!limits.isValid()) return null;
        return McpInflightGate{
            limits: limits,
            slots: [McpInflightSlot{}; MCP_GATE_MAX_INFLIGHT],
            active_total: 0,
            active_tools_call: 0,
            active_resources_read: 0,
            active_prompts_get: 0,
            active_sampling_create: 0,
            active_cancel_pending: 0,
            total_admitted: 0,
            total_completed: 0,
            total_canceled: 0,
            total_timed_out: 0,
            total_rejected: 0,
        };
    }

    /// Internal helper: checks whether `method_class` has remaining per-method quota.
    fn methodQuotaAvailable(method_class: McpMethodClass): bool {
        if (method_class == McpMethodClass.tools_call) {
            return _.active_tools_call < _.limits.max_tools_call;
        }
        if (method_class == McpMethodClass.resources_read) {
            return _.active_resources_read < _.limits.max_resources_read;
        }
        if (method_class == McpMethodClass.prompts_get) {
            return _.active_prompts_get < _.limits.max_prompts_get;
        }
        if (method_class == McpMethodClass.sampling_create) {
            return _.active_sampling_create < _.limits.max_sampling_create;
        }
        return true;
    }

    /// Internal helper: increments per-method active counter.
    fn incMethodCount(*_, method_class: McpMethodClass) {
        if (method_class == McpMethodClass.tools_call) {
            _.active_tools_call += 1;
        } else if (method_class == McpMethodClass.resources_read) {
            _.active_resources_read += 1;
        } else if (method_class == McpMethodClass.prompts_get) {
            _.active_prompts_get += 1;
        } else if (method_class == McpMethodClass.sampling_create) {
            _.active_sampling_create += 1;
        }
    }

    /// Internal helper: releases an occupied slot and updates all active counters.
    fn releaseSlot(*_, idx: usize) {
        const mc = _.slots[idx].method_class;
        if (mc == McpMethodClass.tools_call && _.active_tools_call > 0) {
            _.active_tools_call -= 1;
        } else if (mc == McpMethodClass.resources_read && _.active_resources_read > 0) {
            _.active_resources_read -= 1;
        } else if (mc == McpMethodClass.prompts_get && _.active_prompts_get > 0) {
            _.active_prompts_get -= 1;
        } else if (mc == McpMethodClass.sampling_create && _.active_sampling_create > 0) {
            _.active_sampling_create -= 1;
        }

        if (_.slots[idx].state == McpInflightState.cancel_requested && _.active_cancel_pending > 0) {
            _.active_cancel_pending -= 1;
        }
        if (_.active_total > 0) {
            _.active_total -= 1;
        }
        _.slots[idx] = McpInflightSlot{};
    }

    /// Admits a new in-flight MCP request after checking:
    ///   1. `req_id != 0` and `timeout_ms > 0`
    ///   2. No duplicate active `req_id`
    ///   3. Global `max_total_inflight` capacity
    ///   4. Per-method class concurrency limit
    fn admit_request(
        *_,
        req_id: u64,
        batch_id: u64,
        method_class: McpMethodClass,
        timeout_ms: u64,
        now_ms: u64,
    ): McpAdmitOutcome {
        if (req_id == 0 || timeout_ms == 0) {
            _.total_rejected += 1;
            return McpAdmitOutcome{ admitted: false };
        }

        let free_idx: usize = MCP_GATE_MAX_INFLIGHT;
        let i: usize = 0;
        while (i < MCP_GATE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].req_id == req_id) {
                    _.total_rejected += 1;
                    return McpAdmitOutcome{
                        admitted: false,
                        rejected_duplicate_id: true,
                    };
                }
            } else if (free_idx == MCP_GATE_MAX_INFLIGHT) {
                free_idx = i;
            }
        }

        if (_.active_total >= _.limits.max_total_inflight || free_idx == MCP_GATE_MAX_INFLIGHT) {
            _.total_rejected += 1;
            return McpAdmitOutcome{
                admitted: false,
                rejected_total_cap: true,
            };
        }

        if (!_.methodQuotaAvailable(method_class)) {
            _.total_rejected += 1;
            return McpAdmitOutcome{
                admitted: false,
                rejected_method_cap: true,
            };
        }

        const deadline_ms: u64 = if (now_ms <= 0xFFFFFFFFFFFFFFFF - timeout_ms)
            (now_ms + timeout_ms)
        else
            0xFFFFFFFFFFFFFFFF;

        _.slots[free_idx] = McpInflightSlot{
            req_id: req_id,
            batch_id: batch_id,
            method_class: method_class,
            state: McpInflightState.running,
            started_ms: now_ms,
            deadline_ms: deadline_ms,
            occupied: true,
        };
        _.active_total += 1;
        _.incMethodCount(method_class);
        _.total_admitted += 1;

        return McpAdmitOutcome{
            admitted: true,
            slot_idx: free_idx,
        };
    }

    /// Marks an in-flight request as `cancel_requested` (handling `notifications/cancelled`).
    /// Idempotent on already-canceled requests; returns `false` if `req_id` is not active.
    fn request_cancel(*_, req_id: u64): bool {
        if (req_id == 0) return false;
        let i: usize = 0;
        while (i < MCP_GATE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].req_id == req_id) {
                if (_.slots[i].state == McpInflightState.running) {
                    _.slots[i].state = McpInflightState.cancel_requested;
                    _.active_cancel_pending += 1;
                    _.total_canceled += 1;
                }
                return true;
            }
        }
        return false;
    }

    /// Returns `true` iff `req_id` is currently active and has been marked `cancel_requested`.
    fn is_canceled(req_id: u64): bool {
        if (req_id == 0) return false;
        let i: usize = 0;
        while (i < MCP_GATE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].req_id == req_id) {
                return _.slots[i].state == McpInflightState.cancel_requested;
            }
        }
        return false;
    }

    /// Completes and releases an in-flight request slot, restoring its concurrency quota.
    /// Returns `false` if `req_id` is not currently active.
    fn complete_request(*_, req_id: u64): bool {
        if (req_id == 0) return false;
        let i: usize = 0;
        while (i < MCP_GATE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].req_id == req_id) {
                _.releaseSlot(i);
                _.total_completed += 1;
                return true;
            }
        }
        return false;
    }

    /// Marks all currently running requests belonging to `batch_id` as `cancel_requested`.
    /// Returns the number of newly canceled requests (ignores `batch_id == 0`).
    fn cancel_batch(*_, batch_id: u64): usize {
        if (batch_id == 0) return 0;
        let newly_canceled: usize = 0;
        let i: usize = 0;
        while (i < MCP_GATE_MAX_INFLIGHT) : (i += 1) {
            if (_.slots[i].occupied &&
                _.slots[i].batch_id == batch_id &&
                _.slots[i].state == McpInflightState.running)
            {
                _.slots[i].state = McpInflightState.cancel_requested;
                _.active_cancel_pending += 1;
                _.total_canceled += 1;
                newly_canceled += 1;
            }
        }
        return newly_canceled;
    }

    /// Sweeps expired requests (`now_ms >= deadline_ms`), releasing their slots and writing their
    /// `req_id` values into `out_expired_ids`. Stops cleanly when `out_expired_ids` is full so
    /// remaining expired requests are preserved for subsequent batched sweeps.
    fn sweep_expired(*_, now_ms: u64, out_expired_ids: *[]u64): usize {
        if (out_expired_ids.len == 0) return 0;

        let written: usize = 0;
        let i: usize = 0;
        while (i < MCP_GATE_MAX_INFLIGHT && written < out_expired_ids.len) : (i += 1) {
            if (_.slots[i].occupied && now_ms >= _.slots[i].deadline_ms) {
                out_expired_ids[written] = _.slots[i].req_id;
                written += 1;
                _.releaseSlot(i);
                _.total_timed_out += 1;
            }
        }
        return written;
    }

    /// Returns a cumulative telemetry & active concurrency snapshot.
    fn stats(): McpGateStats {
        return McpGateStats{
            active_total: _.active_total,
            active_tools_call: _.active_tools_call,
            active_cancel_pending: _.active_cancel_pending,
            total_admitted: _.total_admitted,
            total_completed: _.total_completed,
            total_canceled: _.total_canceled,
            total_timed_out: _.total_timed_out,
            total_rejected: _.total_rejected,
        };
    }
}
