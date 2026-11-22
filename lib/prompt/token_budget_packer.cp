// Origin: Pure-CP Stdlib (AI Agent Zero-Alloc Priority Prompt Token Budget Packer & Middle-Out Truncator) | Created: 2026-11-22
// Pure CP zero-heap-allocation Prompt Token Budget Planner & Middle-Out Truncator for Long-Context AI Agents:
//   - Fixed inline stack capacity (`BUDGET_PACKER_MAX_ITEMS = 64`), 0 dynamic heap allocation
//   - Two-phase packing: Phase 1 locks `must_keep` items (with `pinned_overflow` detection without exceeding budget);
//     Phase 2 ranks remaining items deterministically by `(priority desc, recency_seq desc, input_idx asc)`
//   - Supports `must_keep`, `drop_whole`, `keep_head`, `keep_tail`, and `keep_middle_out` (head/tail split with
//     exact `ellipsis_overhead_tokens` accounting to mitigate "Lost in the Middle" context degradation)
//   - Writes `out_decisions` in original input order for direct zero-reorder rendering

/// Maximum number of prompt segments supported per `packPromptWithinBudget` call (zero dynamic heap).
pub const BUDGET_PACKER_MAX_ITEMS: usize = 64;

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

// ============================================================================
// 1. Public Enums, Configuration & Packing Models
// ============================================================================

/// Semantic category of a prompt segment in an AI Agent context window.
pub enum BudgetItemKind {
    system_pinned,
    tool_schema,
    rag_context,
    history_turn,
    user_query,
}

/// Truncation or eviction policy when a segment does not fit completely in remaining token budget.
pub enum BudgetTruncatePolicy {
    must_keep,
    drop_whole,
    keep_head,
    keep_tail,
    keep_middle_out,
}

/// Token budget configuration for `packPromptWithinBudget`.
pub struct BudgetPackConfig {
    max_context_tokens: u32 = 8192;
    reserved_output_tokens: u32 = 1024;
    ellipsis_overhead_tokens: u32 = 4;

    /// Returns available input token budget (`max_context_tokens - reserved_output_tokens`, or `0` if invalid).
    fn effectiveBudget(): u32 {
        if (_.max_context_tokens <= _.reserved_output_tokens) {
            return 0;
        }
        return _.max_context_tokens - _.reserved_output_tokens;
    }

    /// Validates that `max_context_tokens > 0` and `reserved_output_tokens < max_context_tokens`.
    fn isValid(): bool {
        return _.max_context_tokens > 0 && _.reserved_output_tokens < _.max_context_tokens;
    }
}

/// Input descriptor for a single prompt segment competing for context window tokens.
pub struct BudgetedPromptItem {
    item_id: u32 = 0;
    kind: BudgetItemKind = BudgetItemKind.history_turn;
    /// Higher value means higher priority.
    priority: u16 = 0;
    token_count: u32 = 0;
    min_tokens: u32 = 0;
    policy: BudgetTruncatePolicy = BudgetTruncatePolicy.drop_whole;
    /// Monotonic recency sequence number (higher = more recent).
    recency_seq: u32 = 0;
}

/// Per-item packing outcome written in original input order to `out_decisions`.
pub struct PackedSlotDecision {
    item_id: u32 = 0;
    included: bool = false;
    allocated_tokens: u32 = 0;
    truncated: bool = false;
    head_tokens: u32 = 0;
    tail_tokens: u32 = 0;
}

/// Aggregated budget utilization and eviction summary returned by `packPromptWithinBudget`.
pub struct BudgetPackReport {
    effective_budget: u32 = 0;
    allocated_total: u32 = 0;
    remaining_tokens: u32 = 0;
    included_count: usize = 0;
    truncated_count: usize = 0;
    dropped_count: usize = 0;
    pinned_overflow: bool = false;
}

// ============================================================================
// 2. Deterministic Priority Comparator & Two-Phase Zero-Alloc Packer
// ============================================================================

/// Returns `true` iff `items[idx_a]` should be ranked strictly ahead of `items[idx_b]`:
/// `(priority desc, recency_seq desc, input_idx asc)`.
fn itemRanksBefore(items: []const BudgetedPromptItem, idx_a: usize, idx_b: usize): bool {
    const a = items[idx_a];
    const b = items[idx_b];
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }
    if (a.recency_seq != b.recency_seq) {
        return a.recency_seq > b.recency_seq;
    }
    return idx_a < idx_b;
}

/// Packs prompt segments within `cfg.effectiveBudget()` using zero dynamic heap allocation.
/// Writes one `PackedSlotDecision` per input item in original input order into `out_decisions[0..items.len]`.
/// Strictly guarantees `report.allocated_total <= report.effective_budget`.
pub fn packPromptWithinBudget(
    items: []const BudgetedPromptItem,
    cfg: BudgetPackConfig,
    out_decisions: *[]PackedSlotDecision,
): BudgetPackReport {
    const n: usize = items.len;
    if (!cfg.isValid() || n == 0 || n > BUDGET_PACKER_MAX_ITEMS || out_decisions.len < n) {
        const eff: u32 = if (cfg.isValid()) cfg.effectiveBudget() else 0;
        return BudgetPackReport{
            effective_budget: eff,
            allocated_total: 0,
            remaining_tokens: eff,
            included_count: 0,
            truncated_count: 0,
            dropped_count: n,
            pinned_overflow: false,
        };
    }

    const budget: u32 = cfg.effectiveBudget();
    let remaining: u32 = budget;
    let allocated_total: u32 = 0;
    let pinned_overflow: bool = false;

    // Initialize output decisions in original input order and build index array
    let order: [BUDGET_PACKER_MAX_ITEMS]usize = [0; BUDGET_PACKER_MAX_ITEMS];
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_decisions[i] = PackedSlotDecision{
            item_id: items[i].item_id,
            included: false,
            allocated_tokens: 0,
            truncated: false,
            head_tokens: 0,
            tail_tokens: 0,
        };
        order[i] = i;
    }

    // Deterministic insertion sort on `order[0..n]` by (priority desc, recency_seq desc, input_idx asc)
    let s: usize = 1;
    while (s < n) : (s += 1) {
        const key_idx: usize = order[s];
        let j: usize = s;
        while (j > 0 && itemRanksBefore(items, key_idx, order[j - 1])) {
            order[j] = order[j - 1];
            j -= 1;
        }
        order[j] = key_idx;
    }

    // Phase 1: Allocate all `must_keep` items first in priority order without exceeding budget
    i = 0;
    while (i < n) : (i += 1) {
        const idx: usize = order[i];
        const it = items[idx];
        if (it.policy != BudgetTruncatePolicy.must_keep) {
            continue;
        }
        if (it.token_count <= remaining) {
            out_decisions[idx] = PackedSlotDecision{
                item_id: it.item_id,
                included: true,
                allocated_tokens: it.token_count,
                truncated: false,
                head_tokens: it.token_count,
                tail_tokens: 0,
            };
            allocated_total += it.token_count;
            remaining -= it.token_count;
        } else {
            pinned_overflow = true;
        }
    }

    // Phase 2: Pack remaining non-`must_keep` items in priority/recency order
    i = 0;
    while (i < n) : (i += 1) {
        const idx: usize = order[i];
        const it = items[idx];
        if (it.policy == BudgetTruncatePolicy.must_keep) {
            continue;
        }

        if (it.token_count <= remaining) {
            out_decisions[idx] = PackedSlotDecision{
                item_id: it.item_id,
                included: true,
                allocated_tokens: it.token_count,
                truncated: false,
                head_tokens: it.token_count,
                tail_tokens: 0,
            };
            allocated_total += it.token_count;
            remaining -= it.token_count;
        } else if (it.policy != BudgetTruncatePolicy.drop_whole) {
            // Attempt policy-driven truncation if remaining >= max(1, min_tokens) + ellipsis_overhead_tokens
            const eff_min: u32 = if (it.min_tokens == 0) 1 else it.min_tokens;
            const min_needed: u32 = satAddU32(eff_min, cfg.ellipsis_overhead_tokens);
            if (remaining >= min_needed && remaining > cfg.ellipsis_overhead_tokens) {
                const content_budget: u32 = remaining - cfg.ellipsis_overhead_tokens;
                let head_t: u32 = 0;
                let tail_t: u32 = 0;

                if (it.policy == BudgetTruncatePolicy.keep_head) {
                    head_t = content_budget;
                    tail_t = 0;
                } else if (it.policy == BudgetTruncatePolicy.keep_tail) {
                    head_t = 0;
                    tail_t = content_budget;
                } else {
                    // BudgetTruncatePolicy.keep_middle_out: split content budget evenly between head & tail
                    head_t = (content_budget + 1) / 2;
                    tail_t = content_budget - head_t;
                }

                const alloc: u32 = content_budget + cfg.ellipsis_overhead_tokens;
                out_decisions[idx] = PackedSlotDecision{
                    item_id: it.item_id,
                    included: true,
                    allocated_tokens: alloc,
                    truncated: true,
                    head_tokens: head_t,
                    tail_tokens: tail_t,
                };
                allocated_total += alloc;
                remaining -= alloc;
            }
        }
    }

    // Aggregate summary counts
    let included_cnt: usize = 0;
    let truncated_cnt: usize = 0;
    let dropped_cnt: usize = 0;
    i = 0;
    while (i < n) : (i += 1) {
        if (out_decisions[i].included) {
            included_cnt += 1;
            if (out_decisions[i].truncated) {
                truncated_cnt += 1;
            }
        } else {
            dropped_cnt += 1;
        }
    }

    return BudgetPackReport{
        effective_budget: budget,
        allocated_total: allocated_total,
        remaining_tokens: remaining,
        included_count: included_cnt,
        truncated_count: truncated_cnt,
        dropped_count: dropped_cnt,
        pinned_overflow: pinned_overflow,
    };
}
