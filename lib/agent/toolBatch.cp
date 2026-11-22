// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Agent Tool Call Batcher, Read Coalescer & Hazard Wave Planner) | Created: 2026-11-22
// Pure CP zero-heap-allocation parallel tool-calling batch planner for single-turn multi-tool LLM execution:
//   - Program-order RAW / WAR / WAW per-resource hazard detection (`pure_read` / `resource_write`)
//   - Exclusive `global_barrier` isolation (pre- and post-barrier waves for shell/global mutations)
//   - Same-wave identical `pure_read` deduplication/coalescing (`is_coalesced = true`, zero concurrency slot cost)
//   - Bounded concurrency per wave (`max_parallel_per_wave`) and critical-path speedup estimation (`speedup_x100`)

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum number of tool calls planned in a single zero-alloc batch.
pub const TOOL_BATCH_MAX_CALLS: usize = 64;

const COALESCED_NONE: u16 = 0xFFFF;
const U32_MAX: u64 = 0xFFFFFFFF;

/// Resource access semantics for a single tool invocation.
pub enum ToolAccessKind {
    pure_read,
    resource_write,
    global_barrier,
}

/// Input descriptor for a single tool call in a parallel tool-calling turn.
pub struct ToolBatchItem {
    call_id: u64 = 0;
    tool_hash: u64 = 0;
    args_hash: u64 = 0;
    resource_hash: u64 = 0;
    access: ToolAccessKind = ToolAccessKind.pure_read;
    estimated_ms: u32 = 0;
}

/// Output schedule assignment for a single tool call (1-to-1 with input index).
pub struct ToolBatchScheduleEntry {
    call_id: u64 = 0;
    wave_index: u16 = 0;
    coalesced_into_idx: u16 = COALESCED_NONE;
    is_coalesced: bool = false;
}

/// Aggregate execution plan summary returned by `planToolCallBatch`.
pub struct ToolBatchPlanSummary {
    total_calls: usize = 0;
    unique_executions: usize = 0;
    coalesced_duplicates: usize = 0;
    wave_count: usize = 0;
    estimated_serial_ms: u64 = 0;
    estimated_parallel_ms: u64 = 0;
    speedup_x100: u32 = 0;
}

// ============================================================================
// 2. Zero-Alloc Hazard-Aware Tool Batch & Coalescing Wave Planner
// ============================================================================

/// Plans an execution wave schedule for `items` in strict program order with zero dynamic heap allocation:
/// - Safely caps planned count to `min(items.len, out_schedule.len, TOOL_BATCH_MAX_CALLS)`.
/// - Coalesces identical `pure_read` calls (matching `tool_hash`, `args_hash`, `resource_hash`) into the
///   same-wave primary read call whenever no intervening write to that resource or `global_barrier` exists;
///   coalesced calls consume zero concurrency slots.
/// - Enforces RAW, WAR, and WAW ordering on identical `resource_hash`, and isolates `global_barrier` calls
///   in exclusive waves.
/// - Caps actual executions per wave to `max_parallel_per_wave` (`0` means unlimited up to `TOOL_BATCH_MAX_CALLS`).
pub fn planToolCallBatch(
    items: []const ToolBatchItem,
    max_parallel_per_wave: usize,
    out_schedule: *[]ToolBatchScheduleEntry,
): ToolBatchPlanSummary {
    let n: usize = items.len;
    if (out_schedule.len < n) {
        n = out_schedule.len;
    }
    if (n > TOOL_BATCH_MAX_CALLS) {
        n = TOOL_BATCH_MAX_CALLS;
    }
    if (n == 0) {
        return ToolBatchPlanSummary{};
    }

    const eff_limit: usize = if (max_parallel_per_wave == 0) TOOL_BATCH_MAX_CALLS else max_parallel_per_wave;

    let wave_exec_count: [TOOL_BATCH_MAX_CALLS]usize = [0; TOOL_BATCH_MAX_CALLS];
    let wave_max_ms: [TOOL_BATCH_MAX_CALLS]u64 = [0; TOOL_BATCH_MAX_CALLS];

    let unique_executions: usize = 0;
    let coalesced_duplicates: usize = 0;
    let max_wave_used: usize = 0;
    let estimated_serial_ms: u64 = 0;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const cur = items[i];
        estimated_serial_ms += cur.estimated_ms as u64;

        // 1. Compute earliest legal wave honoring program-order RAW/WAR/WAW & global_barrier constraints.
        let min_wave: usize = 0;
        let j: usize = 0;
        while (j < i) : (j += 1) {
            const prev = items[j];
            const prev_wave: usize = out_schedule[j].wave_index as usize;
            let must_follow: bool = false;

            if (cur.access == ToolAccessKind.global_barrier || prev.access == ToolAccessKind.global_barrier) {
                must_follow = true;
            } else if (cur.resource_hash == prev.resource_hash) {
                if (cur.access == ToolAccessKind.resource_write || prev.access == ToolAccessKind.resource_write) {
                    // RAW, WAR, or WAW hazard on the same resource
                    must_follow = true;
                }
            }

            if (must_follow && prev_wave + 1 > min_wave) {
                min_wave = prev_wave + 1;
            }
        }

        // 2. Try to coalesce identical pure_read calls with an eligible earlier primary read
        //    in wave >= min_wave (guaranteed no intervening write or global_barrier).
        let coalesced_idx: usize = n;
        if (cur.access == ToolAccessKind.pure_read) {
            let k: usize = 0;
            while (k < i) : (k += 1) {
                if (out_schedule[k].is_coalesced) continue;
                const cand = items[k];
                if (cand.access != ToolAccessKind.pure_read) continue;
                if ((out_schedule[k].wave_index as usize) < min_wave) continue;
                if (cand.tool_hash == cur.tool_hash &&
                    cand.args_hash == cur.args_hash &&
                    cand.resource_hash == cur.resource_hash)
                {
                    coalesced_idx = k;
                    break;
                }
            }
        }

        if (coalesced_idx < n) {
            const target_wave: u16 = out_schedule[coalesced_idx].wave_index;
            out_schedule[i] = ToolBatchScheduleEntry{
                call_id: cur.call_id,
                wave_index: target_wave,
                coalesced_into_idx: @intCast(coalesced_idx),
                is_coalesced: true,
            };
            coalesced_duplicates += 1;
            continue;
        }

        // 3. Assign earliest wave >= min_wave with available concurrency slot.
        let assigned_wave: usize = min_wave;
        while (assigned_wave < TOOL_BATCH_MAX_CALLS && wave_exec_count[assigned_wave] >= eff_limit) {
            assigned_wave += 1;
        }
        if (assigned_wave >= TOOL_BATCH_MAX_CALLS) {
            assigned_wave = TOOL_BATCH_MAX_CALLS - 1;
        }

        out_schedule[i] = ToolBatchScheduleEntry{
            call_id: cur.call_id,
            wave_index: @intCast(assigned_wave),
            coalesced_into_idx: COALESCED_NONE,
            is_coalesced: false,
        };
        wave_exec_count[assigned_wave] += 1;
        if ((cur.estimated_ms as u64) > wave_max_ms[assigned_wave]) {
            wave_max_ms[assigned_wave] = cur.estimated_ms as u64;
        }
        if (assigned_wave > max_wave_used) {
            max_wave_used = assigned_wave;
        }
        unique_executions += 1;
    }

    const wave_count: usize = max_wave_used + 1;
    let estimated_parallel_ms: u64 = 0;
    let w: usize = 0;
    while (w < wave_count) : (w += 1) {
        estimated_parallel_ms += wave_max_ms[w];
    }

    let speedup_x100: u32 = 100;
    if (estimated_parallel_ms > 0) {
        const raw_speedup: u64 = (estimated_serial_ms * 100) / estimated_parallel_ms;
        speedup_x100 = if (raw_speedup > U32_MAX) @intCast(U32_MAX) else @intCast(raw_speedup);
    }

    return ToolBatchPlanSummary{
        total_calls: n,
        unique_executions: unique_executions,
        coalesced_duplicates: coalesced_duplicates,
        wave_count: wave_count,
        estimated_serial_ms: estimated_serial_ms,
        estimated_parallel_ms: estimated_parallel_ms,
        speedup_x100: speedup_x100,
    };
}
