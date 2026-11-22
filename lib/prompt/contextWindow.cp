// Origin: Pure-CP Stdlib (Zero-Alloc Tool-Pair-Safe Context Window Compactor & Prefix-Cache Planner) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-turn LLM context window governance engine:
//   - Atomic `tool_call` / `tool_result` group integrity (`group_id` binding & `validateToolPairIntegrity`)
//     preventing orphaned tool messages (HTTP 400 API errors) during context eviction
//   - Three budget eviction strategies (`sliding_recent`, `preserve_prefix_and_tail`,
//     `drop_tool_outputs_first`) with unconditional `system` and `pinned` turn protection
//   - Exact contiguous KV-cache / Prompt Cache prefix retention accounting (`preserved_prefix_tokens`
//     and `prefix_cache_ratio_bps` in `0..=10000`)
//   - Stateful zero-heap `ContextWindowTracker` supporting threshold-triggered compaction and
//     atomic in-place `summary` turn injection at the exact eviction boundary

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of conversation turns tracked inline in a single compaction window.
pub const CTX_WIN_MAX_TURNS: usize = 64;

/// Maximum number of atomic turn groups tracked during compaction planning.
pub const CTX_WIN_MAX_GROUPS: usize = 64;

/// Basis-point scale (10000 = 100.00%) used for utilization and prefix cache ratios.
pub const CTX_WIN_BPS_SCALE: u32 = 10_000;

/// Role of a single turn in a multi-turn AI Agent conversation.
pub enum CtxTurnRole {
    system,
    user,
    assistant,
    tool_call,
    tool_result,
    summary,
}

/// Eviction ordering policy applied when total input tokens exceed the effective budget.
pub enum CtxEvictStrategy {
    sliding_recent,
    preserve_prefix_and_tail,
    drop_tool_outputs_first,
}

/// Metadata describing a single conversation turn in the context window.
/// Turns sharing a non-zero `group_id` (e.g. a `tool_call` and its matching `tool_result`s)
/// form an indivisible atomic group that is always kept or evicted together.
pub struct CtxTurnMeta {
    turn_id: u32 = 0;
    role: CtxTurnRole = CtxTurnRole.user;
    group_id: u32 = 0;
    tokens: u32 = 0;
    pinned: bool = false;
    priority: u8 = 0;
    content_hash: u64 = 0;
}

/// Configuration governing input token budget, protected prefix/tail anchors, and eviction strategy.
pub struct CtxWindowConfig {
    max_input_tokens: u32 = 8192;
    reserve_output_tokens: u32 = 1024;
    min_recent_turns: usize = 4;
    prefix_anchor_turns: usize = 2;
    strategy: CtxEvictStrategy = CtxEvictStrategy.preserve_prefix_and_tail;

    /// Return balanced default window configuration.
    static fn defaults(): CtxWindowConfig {
        return CtxWindowConfig{};
    }

    /// Return the effective token budget available for input turns (`max_input_tokens - reserve_output_tokens`).
    fn effectiveInputBudget(): u32 {
        if (max_input_tokens > reserve_output_tokens) {
            return max_input_tokens - reserve_output_tokens;
        }
        return 0;
    }
}

/// Deterministic compaction plan produced by `planContextCompaction`.
pub struct CtxCompactionPlan {
    kept_count: usize = 0;
    evicted_count: usize = 0;
    kept_indices: [CTX_WIN_MAX_TURNS]usize = [0; CTX_WIN_MAX_TURNS];
    kept_tokens: u32 = 0;
    evicted_tokens: u32 = 0;
    preserved_prefix_tokens: u32 = 0;
    prefix_cache_ratio_bps: u16 = 0;
    atomic_groups_evicted: usize = 0;
    needs_summary: bool = false;
}

// ============================================================================
// 2. Internal Group Descriptor
// ============================================================================

struct AtomicGroupInfo {
    group_id: u32 = 0;
    first_idx: usize = 0;
    last_idx: usize = 0;
    tokens: u32 = 0;
    member_count: usize = 0;
    is_protected: bool = false;
    is_tool_group: bool = false;
    in_prefix_anchor: bool = false;
    in_recent_tail: bool = false;
    kept: bool = true;
}

// ============================================================================
// 3. Core Pure Functions
// ============================================================================

/// Strictly validate that kept turns preserve atomic `tool_call` / `tool_result` pair integrity:
///   1. Every kept index is in bounds (`< turns.len`).
///   2. Every kept `tool_call` has a non-zero `group_id` and at least one kept `tool_result`
///      with the same `group_id` (and vice versa for every kept `tool_result`).
///   3. No non-zero `group_id` is partially split: if any member of a group in `turns` is kept,
///      all members of that group in `turns` must also be present in `kept_indices[0..kept_count]`.
pub fn validateToolPairIntegrity(
    turns: []const CtxTurnMeta,
    kept_indices: []const usize,
    kept_count: usize,
): bool {
    if (kept_count > kept_indices.len) return false;
    const n: usize = if (turns.len > CTX_WIN_MAX_TURNS) CTX_WIN_MAX_TURNS else turns.len;

    let is_kept: [CTX_WIN_MAX_TURNS]bool = [false; CTX_WIN_MAX_TURNS];
    let i: usize = 0;
    while (i < kept_count) : (i += 1) {
        const idx = kept_indices[i];
        if (idx >= n) return false;
        is_kept[idx] = true;
    }

    let idx: usize = 0;
    while (idx < n) : (idx += 1) {
        if (!is_kept[idx]) continue;
        const t = turns[idx];

        // If this kept turn belongs to an atomic group, all members in `turns` must be kept.
        if (t.group_id != 0) {
            let j: usize = 0;
            while (j < n) : (j += 1) {
                if (turns[j].group_id == t.group_id && !is_kept[j]) {
                    return false;
                }
            }
        }

        // Every kept tool_call must have a matching kept tool_result with the same non-zero group_id.
        if (t.role == CtxTurnRole.tool_call) {
            if (t.group_id == 0) return false;
            let has_result = false;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                if (is_kept[j] && turns[j].role == CtxTurnRole.tool_result && turns[j].group_id == t.group_id) {
                    has_result = true;
                    break;
                }
            }
            if (!has_result) return false;
        }

        // Every kept tool_result must have a matching kept tool_call with the same non-zero group_id.
        if (t.role == CtxTurnRole.tool_result) {
            if (t.group_id == 0) return false;
            let has_call = false;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                if (is_kept[j] && turns[j].role == CtxTurnRole.tool_call && turns[j].group_id == t.group_id) {
                    has_call = true;
                    break;
                }
            }
            if (!has_call) return false;
        }
    }

    return true;
}

fn selectVictimGroup(
    groups: []const AtomicGroupInfo,
    group_count: usize,
    strategy: CtxEvictStrategy,
): usize? {
    let found = false;
    let best_g: usize = 0;
    let best_tier: u8 = 255;
    let best_key: usize = 0;

    let g: usize = 0;
    while (g < group_count) : (g += 1) {
        const info = groups[g];
        if (!info.kept || info.is_protected) continue;

        let tier: u8 = 0;
        let prefer_larger_idx = false;

        if (strategy == CtxEvictStrategy.sliding_recent) {
            tier = if (!info.in_recent_tail) 0 else 1;
        } else if (strategy == CtxEvictStrategy.preserve_prefix_and_tail) {
            if (!info.in_prefix_anchor && !info.in_recent_tail) {
                tier = 0; // Middle turns evicted first -> preserves prefix KV cache + recent tail
            } else if (!info.in_prefix_anchor) {
                tier = 1; // Recent tail evicted next if budget is still tight
            } else {
                tier = 2; // Prefix anchor evicted last, from newest to oldest to keep longest prefix
                prefer_larger_idx = true;
            }
        } else {
            // drop_tool_outputs_first
            if (info.is_tool_group && !info.in_recent_tail) {
                tier = 0;
            } else if (info.is_tool_group) {
                tier = 1;
            } else if (!info.in_prefix_anchor && !info.in_recent_tail) {
                tier = 2;
            } else if (!info.in_prefix_anchor) {
                tier = 3;
            } else {
                tier = 4;
                prefer_larger_idx = true;
            }
        }

        const key: usize = if (prefer_larger_idx) info.last_idx else info.first_idx;
        if (!found) {
            found = true;
            best_g = g;
            best_tier = tier;
            best_key = key;
        } else if (tier < best_tier) {
            best_g = g;
            best_tier = tier;
            best_key = key;
        } else if (tier == best_tier) {
            if (prefer_larger_idx) {
                if (key > best_key) {
                    best_g = g;
                    best_key = key;
                }
            } else {
                if (key < best_key) {
                    best_g = g;
                    best_key = key;
                }
            }
        }
    }

    if (!found) return null;
    return best_g;
}

/// Compute a deterministic, atomic-group-safe compaction plan for `turns` under `cfg`.
///   - Never splits turns sharing a non-zero `group_id` (e.g. `tool_call` + `tool_result`)
///   - Always protects `system` and `pinned` turns (and any atomic group containing them)
///   - Outputs `kept_indices[0..kept_count]` in strict ascending chronological order
///   - Computes exact contiguous KV-cache prefix retention (`preserved_prefix_tokens`, `prefix_cache_ratio_bps`)
pub fn planContextCompaction(turns: []const CtxTurnMeta, cfg: CtxWindowConfig): CtxCompactionPlan {
    let plan = CtxCompactionPlan{};
    const n: usize = if (turns.len > CTX_WIN_MAX_TURNS) CTX_WIN_MAX_TURNS else turns.len;
    if (n == 0) {
        plan.prefix_cache_ratio_bps = @as(u16, @intCast(CTX_WIN_BPS_SCALE));
        return plan;
    }

    let groups: [CTX_WIN_MAX_GROUPS]AtomicGroupInfo = [AtomicGroupInfo{}; CTX_WIN_MAX_GROUPS];
    let turn_group: [CTX_WIN_MAX_TURNS]usize = [0; CTX_WIN_MAX_TURNS];
    let group_count: usize = 0;

    const tail_start: usize = if (n > cfg.min_recent_turns) n - cfg.min_recent_turns else 0;
    let total_tokens: u32 = 0;

    // 1. Partition turns into indivisible atomic groups.
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const t = turns[i];
        total_tokens += t.tokens;

        let g_idx: usize = group_count;
        if (t.group_id != 0) {
            let k: usize = 0;
            while (k < group_count) : (k += 1) {
                if (groups[k].group_id == t.group_id) {
                    g_idx = k;
                    break;
                }
            }
        }

        if (g_idx == group_count) {
            let info = AtomicGroupInfo{};
            info.group_id = t.group_id;
            info.first_idx = i;
            info.last_idx = i;
            info.tokens = t.tokens;
            info.member_count = 1;
            info.is_protected = (t.role == CtxTurnRole.system || t.pinned);
            info.is_tool_group = (t.role == CtxTurnRole.tool_call || t.role == CtxTurnRole.tool_result);
            info.in_prefix_anchor = (i < cfg.prefix_anchor_turns);
            info.in_recent_tail = (i >= tail_start);
            info.kept = true;
            groups[group_count] = info;
            group_count += 1;
        } else {
            groups[g_idx].last_idx = i;
            groups[g_idx].tokens += t.tokens;
            groups[g_idx].member_count += 1;
            if (t.role == CtxTurnRole.system || t.pinned) {
                groups[g_idx].is_protected = true;
            }
            if (t.role == CtxTurnRole.tool_call || t.role == CtxTurnRole.tool_result) {
                groups[g_idx].is_tool_group = true;
            }
            if (i < cfg.prefix_anchor_turns) {
                groups[g_idx].in_prefix_anchor = true;
            }
            if (i >= tail_start) {
                groups[g_idx].in_recent_tail = true;
            }
        }
        turn_group[i] = g_idx;
    }

    // 2. Evict lowest-priority non-protected atomic groups until within effective budget.
    const budget: u32 = cfg.effectiveInputBudget();
    let current_tokens: u32 = total_tokens;
    while (current_tokens > budget) {
        const victim = selectVictimGroup(groups[0..group_count], group_count, cfg.strategy) ?? break;
        groups[victim].kept = false;
        if (current_tokens > groups[victim].tokens) {
            current_tokens -= groups[victim].tokens;
        } else {
            current_tokens = 0;
        }
        plan.atomic_groups_evicted += 1;
    }

    // 3. Emit kept indices in strict ascending chronological order and compute token metrics.
    let prefix_intact = true;
    let idx: usize = 0;
    while (idx < n) : (idx += 1) {
        const g_idx = turn_group[idx];
        if (groups[g_idx].kept) {
            plan.kept_indices[plan.kept_count] = idx;
            plan.kept_count += 1;
            plan.kept_tokens += turns[idx].tokens;
            if (prefix_intact) {
                plan.preserved_prefix_tokens += turns[idx].tokens;
            }
        } else {
            prefix_intact = false;
            plan.evicted_count += 1;
            plan.evicted_tokens += turns[idx].tokens;
        }
    }

    if (total_tokens == 0) {
        plan.prefix_cache_ratio_bps = @as(u16, @intCast(CTX_WIN_BPS_SCALE));
    } else {
        const bps: u64 = (@as(u64, plan.preserved_prefix_tokens) * @as(u64, CTX_WIN_BPS_SCALE)) / @as(u64, total_tokens);
        plan.prefix_cache_ratio_bps = @as(u16, @intCast(if (bps > @as(u64, CTX_WIN_BPS_SCALE)) @as(u64, CTX_WIN_BPS_SCALE) else bps));
    }
    plan.needs_summary = plan.evicted_count > 0;
    return plan;
}

// ============================================================================
// 4. ContextWindowTracker — Stateful Zero-Heap Multi-Turn Window Governor
// ============================================================================

/// Stateful zero-heap multi-turn context window tracker with threshold-triggered
/// atomic compaction and in-place summary turn replacement.
pub struct ContextWindowTracker {
    cfg: CtxWindowConfig = CtxWindowConfig{},
    turns: [CTX_WIN_MAX_TURNS]CtxTurnMeta = [CtxTurnMeta{}; CTX_WIN_MAX_TURNS];
    turn_count: usize = 0;
    next_turn_id: u32 = 1;
    compaction_count: u64 = 0;
    total_evicted_turns: u64 = 0;

    /// Initialize a new `ContextWindowTracker` with the given window configuration.
    static fn init(cfg: CtxWindowConfig): ContextWindowTracker {
        let tr = ContextWindowTracker{};
        tr.cfg = cfg;
        return tr;
    }

    /// Return the current number of active turns in the tracker.
    fn len(): usize {
        return turn_count;
    }

    /// Look up a turn by 0-based chronological index.
    fn getTurn(idx: usize): CtxTurnMeta? {
        if (idx >= turn_count) return null;
        return turns[idx];
    }

    /// Append a new turn to the context window.
    /// Returns the assigned monotonic `turn_id`, or `null` if all 64 slots are full.
    fn pushTurn(
        *_,
        role: CtxTurnRole,
        group_id: u32,
        tokens: u32,
        pinned: bool,
        priority: u8,
        content_hash: u64,
    ): u32? {
        if (turn_count >= CTX_WIN_MAX_TURNS) return null;
        const tid = next_turn_id;
        turns[turn_count] = CtxTurnMeta{
            turn_id: tid,
            role: role,
            group_id: group_id,
            tokens: tokens,
            pinned: pinned,
            priority: priority,
            content_hash: content_hash,
        };
        turn_count += 1;
        next_turn_id += 1;
        return tid;
    }

    /// Return the sum of tokens across all active turns.
    fn totalTokens(): u32 {
        let sum: u32 = 0;
        let i: usize = 0;
        while (i < turn_count) : (i += 1) {
            sum += turns[i].tokens;
        }
        return sum;
    }

    /// Return current budget utilization in basis points (`0..=10000`).
    fn utilizationBps(): u16 {
        const total = totalTokens();
        const budget = cfg.effectiveInputBudget();
        if (budget == 0) {
            return if (total > 0) @as(u16, @intCast(CTX_WIN_BPS_SCALE)) else 0;
        }
        const bps: u64 = (@as(u64, total) * @as(u64, CTX_WIN_BPS_SCALE)) / @as(u64, budget);
        if (bps > @as(u64, CTX_WIN_BPS_SCALE)) {
            return @as(u16, @intCast(CTX_WIN_BPS_SCALE));
        }
        return @as(u16, @intCast(bps));
    }

    /// Return true if current utilization meets/exceeds `threshold_bps` or exceeds the effective budget.
    fn shouldCompact(threshold_bps: u16): bool {
        if (totalTokens() > cfg.effectiveInputBudget()) return true;
        return utilizationBps() >= threshold_bps;
    }

    /// Preview a compaction plan without modifying the tracker's active turns.
    fn planCompaction(): CtxCompactionPlan {
        return planContextCompaction(turns[0..turn_count], cfg);
    }

    /// Execute compaction in-place:
    ///   - Evicts all turns excluded by `planContextCompaction`
    ///   - Atomically injects a single `.summary` turn at the exact chronological eviction boundary
    ///     (preserving the unbroken KV-cache prefix ahead of it)
    ///   - Updates `compaction_count` and `total_evicted_turns`
    /// Returns the executed `CtxCompactionPlan`.
    fn applyCompaction(*_, summary_tokens: u32, summary_hash: u64): CtxCompactionPlan {
        const plan = planContextCompaction(turns[0..turn_count], cfg);
        if (plan.evicted_count == 0) return plan;

        let is_kept: [CTX_WIN_MAX_TURNS]bool = [false; CTX_WIN_MAX_TURNS];
        let k: usize = 0;
        while (k < plan.kept_count) : (k += 1) {
            is_kept[plan.kept_indices[k]] = true;
        }

        // Locate the first evicted index so the summary turn lands exactly at the eviction gap.
        let first_evicted: usize = turn_count;
        let idx: usize = 0;
        while (idx < turn_count) : (idx += 1) {
            if (!is_kept[idx]) {
                first_evicted = idx;
                break;
            }
        }

        let next_turns: [CTX_WIN_MAX_TURNS]CtxTurnMeta = [CtxTurnMeta{}; CTX_WIN_MAX_TURNS];
        let next_count: usize = 0;
        let summary_inserted = false;

        let i: usize = 0;
        while (i < turn_count) : (i += 1) {
            if (!summary_inserted && i == first_evicted && next_count < CTX_WIN_MAX_TURNS) {
                next_turns[next_count] = CtxTurnMeta{
                    turn_id: next_turn_id,
                    role: CtxTurnRole.summary,
                    group_id: 0,
                    tokens: summary_tokens,
                    pinned: false,
                    priority: 1,
                    content_hash: summary_hash,
                };
                next_turn_id += 1;
                next_count += 1;
                summary_inserted = true;
            }
            if (is_kept[i] && next_count < CTX_WIN_MAX_TURNS) {
                next_turns[next_count] = turns[i];
                next_count += 1;
            }
        }

        let w: usize = 0;
        while (w < next_count) : (w += 1) {
            turns[w] = next_turns[w];
        }
        while (w < turn_count) : (w += 1) {
            turns[w] = CtxTurnMeta{};
        }
        turn_count = next_count;
        compaction_count += 1;
        total_evicted_turns += @as(u64, plan.evicted_count);

        return plan;
    }
}
