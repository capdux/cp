// Origin: Pure-CP Stdlib (AI Agent Parallel Tool-Call Conflict Analyzer & Wave Scheduler) | Created: 2026-11-22
// Zero-heap-allocation LLM batch tool-call read/write lock conflict analyzer and order-preserving
// parallel execution wave scheduler with GlobalBarrier isolation and critical-path speedup estimation.

/// Concurrency lock semantics required by a single tool invocation.
pub enum ResourceLockMode {
    SharedRead,
    ExclusiveWrite,
    GlobalBarrier,
}

/// Descriptor for a single pending LLM tool call in program order.
pub struct ToolCallRequest {
    call_id: u32 = 0;
    resource_hash: u64 = 0;
    lock_mode: ResourceLockMode = ResourceLockMode.SharedRead;
    estimated_ms: u32 = 0;
}

/// Scheduled parallel execution wave assignment for a tool call.
pub struct WaveAssignment {
    call_id: u32 = 0;
    wave_index: u16 = 0;
}

/// Summary metrics for a planned parallel tool execution schedule.
pub struct ParallelPlanStats {
    total_calls: usize = 0;
    total_waves: u16 = 0;
    max_wave_width: usize = 0;
    serial_ms: u64 = 0;
    critical_path_ms: u64 = 0;
    speedup_x100: u32 = 100;
}

/// Returns `true` if two tool calls cannot safely execute in the same parallel wave.
/// - Any `GlobalBarrier` conflicts with every other call.
/// - Calls targeting different `resource_hash` values never conflict (unless barrier).
/// - Calls targeting the same `resource_hash` conflict unless both are `SharedRead`.
pub fn callsConflict(a: ToolCallRequest, b: ToolCallRequest): bool {
    if (a.lock_mode == ResourceLockMode.GlobalBarrier || b.lock_mode == ResourceLockMode.GlobalBarrier) {
        return true;
    }
    if (a.resource_hash != b.resource_hash) {
        return false;
    }
    if (a.lock_mode == ResourceLockMode.SharedRead && b.lock_mode == ResourceLockMode.SharedRead) {
        return false;
    }
    return true;
}

/// Schedules `calls` into the earliest conflict-free parallel execution waves while strictly
/// preserving program-order hazard dependencies (RAW/WAR/WAW and GlobalBarrier) and respecting
/// `max_parallelism` per wave.
/// Returns `null` if `max_parallelism == 0`, `out_assignments.len < calls.len`, or `calls.len > 65535`.
pub fn planToolWaves(
    calls: []const ToolCallRequest,
    max_parallelism: usize,
    out_assignments: *[]WaveAssignment,
): ParallelPlanStats? {
    if (max_parallelism == 0) return null;
    if (out_assignments.len < calls.len) return null;
    if (calls.len > 65535) return null;

    if (calls.len == 0) {
        return ParallelPlanStats{
            total_calls: 0,
            total_waves: 0,
            max_wave_width: 0,
            serial_ms: 0,
            critical_path_ms: 0,
            speedup_x100: 100,
        };
    }

    let max_wave_used: u16 = 0;
    let serial_ms: u64 = 0;

    // Pass 1: assign each call to its earliest legal wave with available concurrency capacity.
    let i: usize = 0;
    while (i < calls.len) : (i += 1) {
        serial_ms += u64(calls[i].estimated_ms);

        let min_wave: u16 = 0;
        let j: usize = 0;
        while (j < i) : (j += 1) {
            if (callsConflict(calls[j], calls[i])) {
                const req_wave: u16 = out_assignments[j].wave_index + 1;
                if (req_wave > min_wave) {
                    min_wave = req_wave;
                }
            }
        }

        let chosen_wave: u16 = min_wave;
        while (true) {
            let width_in_wave: usize = 0;
            let k: usize = 0;
            while (k < i) : (k += 1) {
                if (out_assignments[k].wave_index == chosen_wave) {
                    width_in_wave += 1;
                }
            }
            if (width_in_wave < max_parallelism) {
                break;
            }
            chosen_wave += 1;
        }

        out_assignments[i] = WaveAssignment{
            call_id: calls[i].call_id,
            wave_index: chosen_wave,
        };
        if (chosen_wave > max_wave_used) {
            max_wave_used = chosen_wave;
        }
    }

    const total_waves: u16 = max_wave_used + 1;

    // Pass 2: compute max_wave_width and wave-summed critical path latency (zero scratch heap).
    let max_wave_width: usize = 0;
    let critical_path_ms: u64 = 0;
    let w: u16 = 0;
    while (w < total_waves) : (w += 1) {
        let width: usize = 0;
        let wave_max_ms: u64 = 0;
        let idx: usize = 0;
        while (idx < calls.len) : (idx += 1) {
            if (out_assignments[idx].wave_index == w) {
                width += 1;
                const est = u64(calls[idx].estimated_ms);
                if (est > wave_max_ms) {
                    wave_max_ms = est;
                }
            }
        }
        if (width > max_wave_width) {
            max_wave_width = width;
        }
        critical_path_ms += wave_max_ms;
    }

    let speedup_x100: u32 = 100;
    if (critical_path_ms > 0) {
        const scaled = (serial_ms * 100 + critical_path_ms / 2) / critical_path_ms;
        speedup_x100 = u32(scaled);
    }

    return ParallelPlanStats{
        total_calls: calls.len,
        total_waves: total_waves,
        max_wave_width: max_wave_width,
        serial_ms: serial_ms,
        critical_path_ms: critical_path_ms,
        speedup_x100: speedup_x100,
    };
}
