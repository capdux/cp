// Origin: Pure-CP Stdlib (AI Context Window Multi-Section Token Budget Allocator) | Created: 2026-11-22
// Zero-heap-allocation two-phase priority + weighted water-filling token budget allocator for
// assembling multi-source LLM prompts (system, tools, RAG docs, memory, history) within a hard window.

/// Token budget request for a single prompt context section.
pub struct ContextSectionRequest {
    section_id: u16 = 0;
    priority: u8 = 0;
    min_tokens: u32 = 0;
    desired_tokens: u32 = 0;
    weight: u32 = 1;
}

/// Resolved token allocation result for a single context section.
pub struct ContextSectionAlloc {
    section_id: u16 = 0;
    allocated_tokens: u32 = 0;
    truncated_tokens: u32 = 0;
    fully_satisfied: bool = false;
}

/// Overall budget accounting summary returned by `allocateContextBudget`.
pub struct ContextBudgetSummary {
    total_budget: u32 = 0;
    reserved_output: u32 = 0;
    usable_input_budget: u32 = 0;
    total_allocated: u32 = 0;
    total_truncated: u32 = 0;
    unallocated_slack: u32 = 0;
    min_floors_met: bool = true;
}

fn effectiveMin(req: ContextSectionRequest): u32 {
    return if (req.min_tokens < req.desired_tokens) req.min_tokens else req.desired_tokens;
}

fn effectiveWeight(req: ContextSectionRequest): u32 {
    return if (req.weight == 0) 1 else req.weight;
}

/// Finds the highest `priority` strictly below `upper_exclusive` (pass `256` for initial call).
fn findNextPriority(requests: []const ContextSectionRequest, upper_exclusive: u16): u16? {
    let found = false;
    let best: u16 = 0;
    let i: usize = 0;
    while (i < requests.len) : (i += 1) {
        const p: u16 = u16(requests[i].priority);
        if (p < upper_exclusive) {
            if (!found || p > best) {
                best = p;
                found = true;
            }
        }
    }
    if (!found) return null;
    return best;
}

/// Allocates `usable_input_budget = total_window - reserved_output` across `requests` in two phases:
///   1. Priority-descending guaranteed floor allocation (`min_tokens`, clamped to `desired_tokens`).
///   2. Priority-descending, intra-tier weighted water-filling up to `desired_tokens` with zero slack waste.
/// Returns `null` if `reserved_output > total_window` or `out_allocs.len < requests.len`.
pub fn allocateContextBudget(
    total_window: u32,
    reserved_output: u32,
    requests: []const ContextSectionRequest,
    out_allocs: *[]ContextSectionAlloc,
): ContextBudgetSummary? {
    if (reserved_output > total_window) return null;
    if (out_allocs.len < requests.len) return null;

    const usable: u32 = total_window - reserved_output;
    if (requests.len == 0) {
        return ContextBudgetSummary{
            total_budget: total_window,
            reserved_output: reserved_output,
            usable_input_budget: usable,
            total_allocated: 0,
            total_truncated: 0,
            unallocated_slack: usable,
            min_floors_met: true,
        };
    }

    let init_i: usize = 0;
    while (init_i < requests.len) : (init_i += 1) {
        out_allocs[init_i] = ContextSectionAlloc{
            section_id: requests[init_i].section_id,
            allocated_tokens: 0,
            truncated_tokens: 0,
            fully_satisfied: false,
        };
    }

    let remaining: u32 = usable;
    let min_floors_met = true;

    // Phase 1: Satisfy min_tokens floors in descending priority order.
    let p1_bound: u16 = 256;
    while (true) {
        if (next_p := findNextPriority(requests, p1_bound)) {
            const cur_p: u8 = u8(next_p);
            p1_bound = next_p;

            let i: usize = 0;
            while (i < requests.len) : (i += 1) {
                if (requests[i].priority != cur_p) continue;
                const need_min = effectiveMin(requests[i]);
                if (need_min == 0) continue;

                if (remaining >= need_min) {
                    out_allocs[i].allocated_tokens = need_min;
                    remaining -= need_min;
                } else {
                    out_allocs[i].allocated_tokens = remaining;
                    remaining = 0;
                    min_floors_met = false;
                }
            }
        } else {
            break;
        }
    }

    // Phase 2: Priority-descending weighted water-filling for remaining budget.
    let p2_bound: u16 = 256;
    while (remaining > 0) {
        if (next_p := findNextPriority(requests, p2_bound)) {
            const cur_p: u8 = u8(next_p);
            p2_bound = next_p;

            while (remaining > 0) {
                let total_need: u64 = 0;
                let total_weight: u64 = 0;
                let i: usize = 0;
                while (i < requests.len) : (i += 1) {
                    if (requests[i].priority != cur_p) continue;
                    const alloc_t = out_allocs[i].allocated_tokens;
                    const des_t = requests[i].desired_tokens;
                    if (alloc_t < des_t) {
                        total_need += u64(des_t - alloc_t);
                        total_weight += u64(effectiveWeight(requests[i]));
                    }
                }

                if (total_need == 0 || total_weight == 0) break;

                if (u64(remaining) >= total_need) {
                    let f: usize = 0;
                    while (f < requests.len) : (f += 1) {
                        if (requests[f].priority != cur_p) continue;
                        const alloc_t = out_allocs[f].allocated_tokens;
                        const des_t = requests[f].desired_tokens;
                        if (alloc_t < des_t) {
                            out_allocs[f].allocated_tokens = des_t;
                        }
                    }
                    remaining -= u32(total_need);
                    break;
                }

                const round_budget: u64 = u64(remaining);
                let distributed: u32 = 0;
                let j: usize = 0;
                while (j < requests.len) : (j += 1) {
                    if (requests[j].priority != cur_p) continue;
                    const alloc_t = out_allocs[j].allocated_tokens;
                    const des_t = requests[j].desired_tokens;
                    if (alloc_t >= des_t) continue;

                    const deficit: u32 = des_t - alloc_t;
                    const w: u64 = u64(effectiveWeight(requests[j]));
                    const share_u64: u64 = (round_budget * w) / total_weight;
                    const give: u32 = if (share_u64 >= u64(deficit)) deficit else u32(share_u64);
                    if (give > 0) {
                        out_allocs[j].allocated_tokens += give;
                        distributed += give;
                    }
                }

                remaining -= distributed;

                // Distribute integer-division remainder one token at a time by highest weight.
                if (distributed == 0 && remaining > 0) {
                    while (remaining > 0) {
                        let best_idx: usize? = null;
                        let best_w: u32 = 0;
                        let r: usize = 0;
                        while (r < requests.len) : (r += 1) {
                            if (requests[r].priority != cur_p) continue;
                            if (out_allocs[r].allocated_tokens >= requests[r].desired_tokens) continue;
                            const rw = effectiveWeight(requests[r]);
                            if (best_idx == null || rw > best_w) {
                                best_idx = r;
                                best_w = rw;
                            }
                        }
                        if (idx := best_idx) {
                            out_allocs[idx].allocated_tokens += 1;
                            remaining -= 1;
                        } else {
                            break;
                        }
                    }
                    break;
                }
            }
        } else {
            break;
        }
    }

    // Phase 3: Finalize per-section truncation flags and aggregate summary.
    let total_allocated: u64 = 0;
    let total_truncated: u64 = 0;
    let s: usize = 0;
    while (s < requests.len) : (s += 1) {
        const alloc_t = out_allocs[s].allocated_tokens;
        const des_t = requests[s].desired_tokens;
        const trunc_t: u32 = if (des_t > alloc_t) des_t - alloc_t else 0;
        out_allocs[s].truncated_tokens = trunc_t;
        out_allocs[s].fully_satisfied = (trunc_t == 0);
        total_allocated += u64(alloc_t);
        total_truncated += u64(trunc_t);
    }

    const total_alloc_u32: u32 = u32(total_allocated);
    const total_trunc_u32: u32 = if (total_truncated > 0xFFFFFFFF) 0xFFFFFFFF else u32(total_truncated);

    return ContextBudgetSummary{
        total_budget: total_window,
        reserved_output: reserved_output,
        usable_input_budget: usable,
        total_allocated: total_alloc_u32,
        total_truncated: total_trunc_u32,
        unallocated_slack: usable - total_alloc_u32,
        min_floors_met: min_floors_met,
    };
}
