// Origin: Pure-CP Stdlib (Zero-Alloc AI Agent Context Compactor & Atomic Tool-Pair Guard) | Created: 2026-11-22
// Pure CP zero-heap-allocation conversation context compaction planner for multi-turn AI Agents:
//   - Models 6 conversation roles (`system | user | assistant | tool_call | tool_result | summary_anchor`)
//     with `token_count`, atomic `group_id`, `pinned` protection, and `importance_bps`
//   - Enforces strict atomic tool-pair integrity: never splits a `tool_call` from its matching `tool_result`
//     during compaction (`keep`, `summarize_drop`, or `drop` always applies to the entire `group_id` unit)
//   - Two-stage compaction (`planContextCompaction`): preserves `pinned` + last `min_recent_turns` turns,
//     then evicts lowest `(recency + importance)` middle-history units and emits exact summary span bounds
//   - Detects & flags orphan tool messages (`validateToolPairIntegrity` / `markOrphanToolMessages`)

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// Maximum conversation messages planned in a single zero-alloc compaction pass.
pub const COMPACTOR_MAX_MESSAGES: usize = 128;

/// Sentinel `group_id` indicating a standalone message not bound to a multi-message atomic group.
pub const COMPACTOR_NO_GROUP: u32 = 0;

/// Semantic role of a message inside an AI Agent conversation history.
pub enum CompactMessageRole {
    system,
    user,
    assistant,
    tool_call,
    tool_result,
    summary_anchor,
}

/// Per-message compaction disposition emitted by `planContextCompaction`.
pub enum CompactionAction {
    keep,
    summarize_drop,
    drop,
}

/// Aggregate outcome status of `planContextCompaction`.
pub enum CompactionStatus {
    within_budget,
    compacted,
    pinned_exceeds_budget,
    budget_tight_best_effort,
}

/// Specific tool-pair integrity violation detected by `validateToolPairIntegrity`.
pub enum ToolPairIssueKind {
    none,
    orphan_tool_call,
    orphan_tool_result,
    missing_group_id,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Lightweight descriptor for one conversation message in the compaction window.
pub struct CompactMessage {
    msg_id: u32 = 0;
    role: CompactMessageRole = CompactMessageRole.user;
    token_count: u32 = 0;
    group_id: u32 = COMPACTOR_NO_GROUP;
    pinned: bool = false;
    importance_bps: u16 = 5000;
}

/// Tuning parameters for two-stage conversation context compaction.
pub struct CompactionConfig {
    max_tokens: u32 = 32768;
    target_tokens: u32 = 24576;
    summary_reserve_tokens: u32 = 512;
    min_recent_turns: usize = 2;
    recency_weight_bps: u16 = 6000;
    importance_weight_bps: u16 = 4000;
    drop_instead_of_summarize_below_bps: u16 = 1500;
}

/// Diagnostic report produced by `validateToolPairIntegrity`.
pub struct ToolPairIntegrityReport {
    valid: bool = true;
    total_tool_calls: usize = 0;
    total_tool_results: usize = 0;
    orphan_tool_calls: usize = 0;
    orphan_tool_results: usize = 0;
    missing_group_id_count: usize = 0;
    first_issue_index: usize = 0;
    first_issue_kind: ToolPairIssueKind = ToolPairIssueKind.none;
}

/// Zero-alloc compaction execution plan returned by `planContextCompaction`.
pub struct CompactionPlan {
    status: CompactionStatus = CompactionStatus.within_budget;
    message_count: usize = 0;
    actions: [COMPACTOR_MAX_MESSAGES]CompactionAction = [CompactionAction.keep; COMPACTOR_MAX_MESSAGES];
    original_tokens: u32 = 0;
    kept_tokens: u32 = 0;
    saved_tokens: u32 = 0;
    kept_count: usize = 0;
    summarized_count: usize = 0;
    dropped_count: usize = 0;
    has_summary_span: bool = false;
    summary_start_idx: usize = 0;
    summary_end_idx: usize = 0;

    /// Returns the planned `CompactionAction` for message `idx`, or `null` if out of range.
    fn actionAt(idx: usize): CompactionAction? {
        if (idx >= _.message_count) {
            return null;
        }
        return _.actions[idx];
    }
}

// ============================================================================
// 3. Tool-Pair Integrity Validation & Orphan Detection
// ============================================================================

/// Validates that every `tool_call` has a matching subsequent `tool_result` with the same non-zero
/// `group_id`, and every `tool_result` has a preceding `tool_call` with the same non-zero `group_id`.
pub fn validateToolPairIntegrity(messages: []const CompactMessage): ToolPairIntegrityReport {
    const n: usize = if (messages.len <= COMPACTOR_MAX_MESSAGES) messages.len else COMPACTOR_MAX_MESSAGES;
    let rep = ToolPairIntegrityReport{ valid: true };

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const m = messages[i];
        if (m.role == CompactMessageRole.tool_call) {
            rep.total_tool_calls += 1;
            if (m.group_id == COMPACTOR_NO_GROUP) {
                rep.missing_group_id_count += 1;
                if (rep.valid) {
                    rep.valid = false;
                    rep.first_issue_index = i;
                    rep.first_issue_kind = ToolPairIssueKind.missing_group_id;
                }
                continue;
            }
            // Look ahead for at least one matching tool_result with the same group_id.
            let found_result: bool = false;
            let j: usize = i + 1;
            while (j < n) : (j += 1) {
                if (messages[j].role == CompactMessageRole.tool_result && messages[j].group_id == m.group_id) {
                    found_result = true;
                    break;
                }
            }
            if (!found_result) {
                rep.orphan_tool_calls += 1;
                if (rep.valid) {
                    rep.valid = false;
                    rep.first_issue_index = i;
                    rep.first_issue_kind = ToolPairIssueKind.orphan_tool_call;
                }
            }
        } else if (m.role == CompactMessageRole.tool_result) {
            rep.total_tool_results += 1;
            if (m.group_id == COMPACTOR_NO_GROUP) {
                rep.missing_group_id_count += 1;
                if (rep.valid) {
                    rep.valid = false;
                    rep.first_issue_index = i;
                    rep.first_issue_kind = ToolPairIssueKind.missing_group_id;
                }
                continue;
            }
            // Look behind for a preceding tool_call with the same group_id.
            let found_call: bool = false;
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (messages[j].role == CompactMessageRole.tool_call && messages[j].group_id == m.group_id) {
                    found_call = true;
                    break;
                }
            }
            if (!found_call) {
                rep.orphan_tool_results += 1;
                if (rep.valid) {
                    rep.valid = false;
                    rep.first_issue_index = i;
                    rep.first_issue_kind = ToolPairIssueKind.orphan_tool_result;
                }
            }
        }
    }

    return rep;
}

/// Marks orphan `tool_call` / `tool_result` messages (or those missing `group_id`) as `true`
/// in caller-owned `out_is_orphan`, returning the total number of flagged orphan messages.
pub fn markOrphanToolMessages(
    messages: []const CompactMessage,
    out_is_orphan: *[]bool,
): usize {
    let n: usize = messages.len;
    if (out_is_orphan.len < n) {
        n = out_is_orphan.len;
    }
    if (n > COMPACTOR_MAX_MESSAGES) {
        n = COMPACTOR_MAX_MESSAGES;
    }

    let flagged: usize = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_is_orphan[i] = false;
        const m = messages[i];
        if (m.role == CompactMessageRole.tool_call) {
            if (m.group_id == COMPACTOR_NO_GROUP) {
                out_is_orphan[i] = true;
                flagged += 1;
                continue;
            }
            let matched: bool = false;
            let j: usize = i + 1;
            while (j < n) : (j += 1) {
                if (messages[j].role == CompactMessageRole.tool_result && messages[j].group_id == m.group_id) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                out_is_orphan[i] = true;
                flagged += 1;
            }
        } else if (m.role == CompactMessageRole.tool_result) {
            if (m.group_id == COMPACTOR_NO_GROUP) {
                out_is_orphan[i] = true;
                flagged += 1;
                continue;
            }
            let matched: bool = false;
            let j: usize = 0;
            while (j < i) : (j += 1) {
                if (messages[j].role == CompactMessageRole.tool_call && messages[j].group_id == m.group_id) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                out_is_orphan[i] = true;
                flagged += 1;
            }
        }
    }
    return flagged;
}

// ============================================================================
// 4. Two-Stage Atomic-Group Context Compaction Planner
// ============================================================================

struct AtomicUnit {
    group_id: u32 = COMPACTOR_NO_GROUP;
    total_tokens: u32 = 0;
    is_pinned: bool = false;
    is_protected_recent: bool = false;
    max_idx: usize = 0;
    max_importance_bps: u16 = 0;
    score: u32 = 0;
    evicted: bool = false;
    evict_action: CompactionAction = CompactionAction.keep;
}

fn saturatingAddU32(a: u32, b: u32): u32 {
    const sum: u64 = (a as u64) + (b as u64);
    if (sum > 0xFFFFFFFF) {
        return 0xFFFFFFFF;
    }
    return sum as u32;
}

/// Plans a zero-allocation conversation compaction preserving `pinned` messages, the most recent
/// `min_recent_turns` turns, and atomic `group_id` tool-call/tool-result integrity.
pub fn planContextCompaction(
    messages: []const CompactMessage,
    cfg: CompactionConfig,
): CompactionPlan {
    const n: usize = if (messages.len <= COMPACTOR_MAX_MESSAGES) messages.len else COMPACTOR_MAX_MESSAGES;
    let plan = CompactionPlan{
        status: CompactionStatus.within_budget,
        message_count: n,
    };

    if (n == 0) {
        return plan;
    }

    // 1. Group messages into indivisible AtomicUnits keyed by non-zero `group_id`.
    let units: [COMPACTOR_MAX_MESSAGES]AtomicUnit = [AtomicUnit{}; COMPACTOR_MAX_MESSAGES];
    let msg_to_unit: [COMPACTOR_MAX_MESSAGES]usize = [0; COMPACTOR_MAX_MESSAGES];
    let unit_count: usize = 0;
    let original_tokens: u32 = 0;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const m = messages[i];
        original_tokens = saturatingAddU32(original_tokens, m.token_count);

        let found_unit: usize? = null;
        if (m.group_id != COMPACTOR_NO_GROUP) {
            let u: usize = 0;
            while (u < unit_count) : (u += 1) {
                if (units[u].group_id == m.group_id) {
                    found_unit = u;
                    break;
                }
            }
        }

        if (u_idx := found_unit) {
            msg_to_unit[i] = u_idx;
            units[u_idx].total_tokens = saturatingAddU32(units[u_idx].total_tokens, m.token_count);
            if (m.pinned) {
                units[u_idx].is_pinned = true;
            }
            if (i > units[u_idx].max_idx) {
                units[u_idx].max_idx = i;
            }
            if (m.importance_bps > units[u_idx].max_importance_bps) {
                units[u_idx].max_importance_bps = m.importance_bps;
            }
        } else {
            const u_idx: usize = unit_count;
            unit_count += 1;
            msg_to_unit[i] = u_idx;
            units[u_idx] = AtomicUnit{
                group_id: m.group_id,
                total_tokens: m.token_count,
                is_pinned: m.pinned,
                is_protected_recent: false,
                max_idx: i,
                max_importance_bps: m.importance_bps,
                score: 0,
                evicted: false,
                evict_action: CompactionAction.keep,
            };
        }
    }

    plan.original_tokens = original_tokens;

    // Fast path: already within hard max_tokens budget.
    if (original_tokens <= cfg.max_tokens) {
        plan.status = CompactionStatus.within_budget;
        plan.kept_tokens = original_tokens;
        plan.saved_tokens = 0;
        plan.kept_count = n;
        return plan;
    }

    // Compute total tokens locked by pinned atomic units.
    let pinned_tokens: u32 = 0;
    let u_p: usize = 0;
    while (u_p < unit_count) : (u_p += 1) {
        if (units[u_p].is_pinned) {
            pinned_tokens = saturatingAddU32(pinned_tokens, units[u_p].total_tokens);
        }
    }

    // 2. Stage 1 protection: mark the last `min_recent_turns` turns as protected
    //    (expanding to whole atomic units so tool_call/tool_result pairs are never split).
    if (cfg.min_recent_turns > 0) {
        let user_turns_seen: usize = 0;
        let cutoff_idx: usize = 0;
        let rev: usize = n;
        while (rev > 0) {
            rev -= 1;
            if (messages[rev].role == CompactMessageRole.user) {
                user_turns_seen += 1;
                if (user_turns_seen >= cfg.min_recent_turns) {
                    cutoff_idx = rev;
                    break;
                }
            }
        }
        let r: usize = cutoff_idx;
        while (r < n) : (r += 1) {
            units[msg_to_unit[r]].is_protected_recent = true;
        }
    }

    // 3. Compute recency + importance retention score for every atomic unit.
    const w_rec: u64 = cfg.recency_weight_bps as u64;
    const w_imp: u64 = cfg.importance_weight_bps as u64;
    const w_sum: u64 = if (w_rec + w_imp > 0) (w_rec + w_imp) else 1;

    let u_s: usize = 0;
    while (u_s < unit_count) : (u_s += 1) {
        const recency_bps: u64 = (((units[u_s].max_idx + 1) as u64) * 10000) / (n as u64);
        const imp_bps: u64 = units[u_s].max_importance_bps as u64;
        const blended: u64 = (recency_bps * w_rec + imp_bps * w_imp) / w_sum;
        units[u_s].score = blended as u32;
    }

    // 4. Stage 2 greedy eviction of lowest-score evictable middle-history units
    //    until `kept_tokens + summary_reserve <= eff_target`.
    const eff_target: u32 = if (cfg.target_tokens > 0 && cfg.target_tokens <= cfg.max_tokens) cfg.target_tokens else cfg.max_tokens;
    let cur_kept_tokens: u32 = original_tokens;
    let has_summarized_any: bool = false;

    while (true) {
        const reserve: u32 = if (has_summarized_any) cfg.summary_reserve_tokens else 0;
        if (saturatingAddU32(cur_kept_tokens, reserve) <= eff_target) {
            break;
        }

        // Find lowest-scoring non-evicted, non-pinned, non-recent unit (tie-break by earliest max_idx).
        let best_u: usize? = null;
        let u_k: usize = 0;
        while (u_k < unit_count) : (u_k += 1) {
            if (units[u_k].evicted || units[u_k].is_pinned || units[u_k].is_protected_recent) {
                continue;
            }
            if (prev_u := best_u) {
                if (units[u_k].score < units[prev_u].score ||
                    (units[u_k].score == units[prev_u].score && units[u_k].max_idx < units[prev_u].max_idx)) {
                    best_u = u_k;
                }
            } else {
                best_u = u_k;
            }
        }

        if (victim_u := best_u) {
            units[victim_u].evicted = true;
            if (units[victim_u].max_importance_bps < cfg.drop_instead_of_summarize_below_bps) {
                units[victim_u].evict_action = CompactionAction.drop;
            } else {
                units[victim_u].evict_action = CompactionAction.summarize_drop;
                has_summarized_any = true;
            }
            if (cur_kept_tokens >= units[victim_u].total_tokens) {
                cur_kept_tokens -= units[victim_u].total_tokens;
            } else {
                cur_kept_tokens = 0;
            }
        } else {
            break;
        }
    }

    // 5. Materialize per-message actions, summary span bounds, and final status.
    let m_idx: usize = 0;
    while (m_idx < n) : (m_idx += 1) {
        const u_id = msg_to_unit[m_idx];
        if (!units[u_id].evicted) {
            plan.actions[m_idx] = CompactionAction.keep;
            plan.kept_count += 1;
        } else {
            const act = units[u_id].evict_action;
            plan.actions[m_idx] = act;
            if (act == CompactionAction.summarize_drop) {
                plan.summarized_count += 1;
                if (!plan.has_summary_span) {
                    plan.has_summary_span = true;
                    plan.summary_start_idx = m_idx;
                    plan.summary_end_idx = m_idx + 1;
                } else {
                    plan.summary_end_idx = m_idx + 1;
                }
            } else {
                plan.dropped_count += 1;
            }
        }
    }

    plan.kept_tokens = cur_kept_tokens;
    plan.saved_tokens = if (original_tokens >= cur_kept_tokens) (original_tokens - cur_kept_tokens) else 0;

    if (pinned_tokens > cfg.max_tokens) {
        plan.status = CompactionStatus.pinned_exceeds_budget;
    } else if (cur_kept_tokens <= cfg.max_tokens) {
        plan.status = CompactionStatus.compacted;
    } else {
        plan.status = CompactionStatus.budget_tight_best_effort;
    }

    return plan;
}
