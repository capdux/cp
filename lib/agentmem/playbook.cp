// Origin: Pure-CP Stdlib (Procedural SOP / Playbook Memory Store with @popCount & Bayesian Reliability) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation procedural Playbook / SOP memory engine for autonomous AI Agents:
//   - Fixed-capacity 32-playbook registry with up to 12 structured tool steps per SOP
//   - Hardware `@popCount` intent-bitmask coverage matching & hard precondition bitmask gating
//   - Laplace-smoothed Bayesian reliability (`(s + 1) / (s + f + 2)` in bps) & EWMA latency tracking
//   - Automatic health state machine: `active -> degraded -> deprecated` on failure degradation
//     with automatic recovery back to `active` on consecutive successes

// ============================================================================
// 1. Constants & Enums
// ============================================================================

/// Fixed maximum number of playbooks stored inline in `PlaybookMemoryStore`.
pub const PLAYBOOK_MAX_ENTRIES: usize = 32;

/// Maximum number of ordered SOP steps per playbook.
pub const PLAYBOOK_MAX_STEPS: usize = 12;

/// Maximum byte length for playbook names and step tool names.
pub const PLAYBOOK_NAME_MAX_LEN: usize = 48;

/// Maximum byte length for step expected outcome descriptions.
pub const PLAYBOOK_STEP_MAX_LEN: usize = 64;

/// Lifecycle health status of a procedural playbook.
pub enum PlaybookStatus {
    active,
    degraded,
    deprecated,
    draft,
}

// ============================================================================
// 2. Data Models
// ============================================================================

fn copyTruncated(src: []const u8, dst: *[]u8): usize {
    const n: usize = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

/// Specification for a single ordered step inside a playbook SOP.
pub struct PlaybookStepSpec {
    tool_name_buf: [PLAYBOOK_NAME_MAX_LEN]u8 = [0; PLAYBOOK_NAME_MAX_LEN];
    tool_name_len: usize = 0;
    expected_outcome_buf: [PLAYBOOK_STEP_MAX_LEN]u8 = [0; PLAYBOOK_STEP_MAX_LEN];
    expected_outcome_len: usize = 0;
    is_optional: bool = false;
    timeout_ms: u32 = 0;

    /// Returns the tool name slice for this step.
    fn tool_name(): []const u8 {
        return tool_name_buf[0..tool_name_len];
    }

    /// Returns the expected outcome description slice for this step.
    fn expected_outcome(): []const u8 {
        return expected_outcome_buf[0..expected_outcome_len];
    }
}

/// Stored procedural playbook record with multi-step SOP, bitmasks, and Bayesian execution telemetry.
pub struct PlaybookEntry {
    active_slot: bool = false;
    playbook_id: u32 = 0;
    name_buf: [PLAYBOOK_NAME_MAX_LEN]u8 = [0; PLAYBOOK_NAME_MAX_LEN];
    name_len: usize = 0;
    intent_mask: u64 = 0;
    precondition_mask: u64 = 0;
    step_count: usize = 0;
    steps: [PLAYBOOK_MAX_STEPS]PlaybookStepSpec = [PlaybookStepSpec{}; PLAYBOOK_MAX_STEPS];
    success_count: u32 = 0;
    failure_count: u32 = 0;
    consecutive_successes: u32 = 0;
    consecutive_failures: u32 = 0;
    avg_latency_ms: u32 = 0;
    version: u16 = 1;
    status: PlaybookStatus = PlaybookStatus.active;
    last_used_ms: u64 = 0;

    /// Returns the playbook name slice.
    fn name(): []const u8 {
        return name_buf[0..name_len];
    }

    /// Returns the Laplace-smoothed Bayesian success rate in basis points (`0..=10000`):
    /// `((success_count + 1) * 10000) / (success_count + failure_count + 2)`.
    /// Unexecuted playbooks start at neutral `5000` bps (`50.00%`).
    fn successRateBps(): u16 {
        const num: u64 = (@as(u64, success_count) + 1) * 10000;
        const den: u64 = @as(u64, success_count) + @as(u64, failure_count) + 2;
        return @as(u16, @intCast(num / den));
    }
}

/// Query filter and gating parameters for `PlaybookMemoryStore.matchTopK`.
pub struct PlaybookQuery {
    /// Bitmask of desired intent capabilities (matched via `@popCount` overlap).
    required_intent_mask: u64 = 0;
    /// Bitmask of currently satisfied environment preconditions (hard filter).
    available_preconditions: u64 = 0;
    /// Optional latency ceiling in ms (`0` = no upper bound).
    max_latency_ms: u32 = 0;
    /// Minimum Laplace-smoothed reliability in bps (`0..=10000`).
    min_success_rate_bps: u16 = 0;
    /// Whether `PlaybookStatus.degraded` playbooks are eligible for recall.
    include_degraded: bool = false;
}

/// Ranked candidate match returned by `PlaybookMemoryStore.matchTopK`.
pub struct PlaybookMatch {
    playbook_id: u32 = 0;
    index: usize = 0;
    score: f32 = 0.0;
    intent_overlap: u32 = 0;
    success_rate_bps: u16 = 0;
}

/// Summary telemetry snapshot of `PlaybookMemoryStore`.
pub struct PlaybookStoreStats {
    total_playbooks: usize = 0;
    active_count: usize = 0;
    degraded_count: usize = 0;
    deprecated_count: usize = 0;
    total_executions: u64 = 0;
    total_successes: u64 = 0;
}

// ============================================================================
// 3. Core Engine: `PlaybookMemoryStore`
// ============================================================================

fn isBetterMatch(a: PlaybookMatch, b: PlaybookMatch): bool {
    if (a.score > b.score) return true;
    if (a.score < b.score) return false;
    if (a.intent_overlap > b.intent_overlap) return true;
    if (a.intent_overlap < b.intent_overlap) return false;
    return a.playbook_id < b.playbook_id;
}

/// Zero-heap-allocation procedural SOP / Playbook memory store with `@popCount` intent matching,
/// precondition gating, EWMA latency tracking, and Bayesian reliability reinforcement.
pub struct PlaybookMemoryStore {
    entries: [PLAYBOOK_MAX_ENTRIES]PlaybookEntry = [PlaybookEntry{}; PLAYBOOK_MAX_ENTRIES];
    next_id: u32 = 1;
    total_executions: u64 = 0;
    total_successes: u64 = 0;

    /// Initializes an empty `PlaybookMemoryStore`.
    static fn init(): PlaybookMemoryStore {
        return PlaybookMemoryStore{};
    }

    /// Registers a new active playbook and returns its monotonic `playbook_id`,
    /// or `null` if `name` is empty, `intent_mask == 0`, or the 32-slot store is full.
    fn registerPlaybook(
        *_,
        name: []const u8,
        intent_mask: u64,
        precondition_mask: u64,
        now_ms: u64,
    ): u32? {
        if (name.len == 0 || intent_mask == 0) return null;

        let slot: usize = PLAYBOOK_MAX_ENTRIES;
        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active_slot) {
                slot = i;
                break;
            }
        }
        if (slot == PLAYBOOK_MAX_ENTRIES) return null;

        const id: u32 = next_id;
        next_id += 1;

        let entry = PlaybookEntry{
            active_slot: true,
            playbook_id: id,
            intent_mask: intent_mask,
            precondition_mask: precondition_mask,
            version: 1,
            status: PlaybookStatus.active,
            last_used_ms: now_ms,
        };
        let name_dst: []u8 = entry.name_buf[0..];
        entry.name_len = copyTruncated(name, *name_dst);
        entries[slot] = entry;
        return id;
    }

    /// Appends an ordered step to `playbook_id`. Returns `false` if not found, `tool_name` is empty,
    /// or `PLAYBOOK_MAX_STEPS` (`12`) is already reached.
    fn addStep(
        *_,
        playbook_id: u32,
        tool_name: []const u8,
        expected_outcome: []const u8,
        is_optional: bool,
        timeout_ms: u32,
    ): bool {
        if (tool_name.len == 0) return false;

        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active_slot && entries[i].playbook_id == playbook_id) {
                if (entries[i].step_count >= PLAYBOOK_MAX_STEPS) return false;
                let step = PlaybookStepSpec{
                    is_optional: is_optional,
                    timeout_ms: timeout_ms,
                };
                let tn_dst: []u8 = step.tool_name_buf[0..];
                step.tool_name_len = copyTruncated(tool_name, *tn_dst);
                let out_dst: []u8 = step.expected_outcome_buf[0..];
                step.expected_outcome_len = copyTruncated(expected_outcome, *out_dst);

                const idx: usize = entries[i].step_count;
                entries[i].steps[idx] = step;
                entries[i].step_count += 1;
                entries[i].version += 1;
                return true;
            }
        }
        return false;
    }

    /// Records a playbook execution outcome, updating EWMA latency, Bayesian success/failure counts,
    /// and automatic health transitions (`active -> degraded -> deprecated`, or `degraded -> active`
    /// on consecutive successes). Returns the resulting `PlaybookStatus`, or `null` if not found.
    fn recordExecution(
        *_,
        playbook_id: u32,
        succeeded: bool,
        latency_ms: u32,
        now_ms: u64,
    ): PlaybookStatus? {
        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active_slot && entries[i].playbook_id == playbook_id) {
                const prev_trials: u32 = entries[i].success_count + entries[i].failure_count;
                if (prev_trials == 0) {
                    entries[i].avg_latency_ms = latency_ms;
                } else {
                    const ewma: u64 = (@as(u64, entries[i].avg_latency_ms) * 3 + @as(u64, latency_ms)) / 4;
                    entries[i].avg_latency_ms = @as(u32, @intCast(ewma));
                }

                entries[i].last_used_ms = now_ms;
                total_executions += 1;

                if (succeeded) {
                    entries[i].success_count += 1;
                    entries[i].consecutive_successes += 1;
                    entries[i].consecutive_failures = 0;
                    total_successes += 1;

                    // Automatic recovery from degraded -> active after >= 2 consecutive successes
                    // once Bayesian reliability is healthy (>= 5500 bps)
                    if (entries[i].status == PlaybookStatus.degraded) {
                        if (entries[i].consecutive_successes >= 2 && entries[i].successRateBps() >= 5500) {
                            entries[i].status = PlaybookStatus.active;
                        }
                    }
                } else {
                    entries[i].failure_count += 1;
                    entries[i].consecutive_failures += 1;
                    entries[i].consecutive_successes = 0;

                    const trials: u32 = entries[i].success_count + entries[i].failure_count;
                    const rate_bps: u16 = entries[i].successRateBps();
                    if (entries[i].status != PlaybookStatus.deprecated && trials >= 3) {
                        if (rate_bps < 2500 || entries[i].consecutive_failures >= 5) {
                            entries[i].status = PlaybookStatus.deprecated;
                        } else if (entries[i].status == PlaybookStatus.active && (rate_bps < 4500 || entries[i].consecutive_failures >= 3)) {
                            entries[i].status = PlaybookStatus.degraded;
                        }
                    }
                }
                return entries[i].status;
            }
        }
        return null;
    }

    /// Explicitly marks `playbook_id` as `PlaybookStatus.deprecated`.
    fn deprecate(*_, playbook_id: u32): bool {
        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active_slot && entries[i].playbook_id == playbook_id) {
                entries[i].status = PlaybookStatus.deprecated;
                return true;
            }
        }
        return false;
    }

    /// Returns a copy of the playbook entry for `playbook_id`, or `null` if absent.
    fn getPlaybook(playbook_id: u32): PlaybookEntry? {
        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active_slot && entries[i].playbook_id == playbook_id) {
                return entries[i];
            }
        }
        return null;
    }

    /// Retrieves Top-K matching playbooks into `out_matches`, sorted descending by composite score:
    /// - Hard-filters status (`deprecated`/`draft` excluded; `degraded` gated by `include_degraded`),
    ///   precondition bitmask (`(precondition_mask & ~available_preconditions) == 0`), latency cap,
    ///   and minimum Bayesian success rate.
    /// - Uses native `@popCount` to compute intent bitmask coverage and blends coverage,
    ///   Laplace-smoothed reliability, and latency penalty into a deterministic ranking score.
    fn matchTopK(query: PlaybookQuery, out_matches: *[]PlaybookMatch): usize {
        if (out_matches.len == 0) return 0;

        const req_bits: u32 = @popCount(query.required_intent_mask);
        let matched: usize = 0;

        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active_slot) continue;

            const st: PlaybookStatus = entries[i].status;
            if (st == PlaybookStatus.deprecated || st == PlaybookStatus.draft) continue;
            if (st == PlaybookStatus.degraded && !query.include_degraded) continue;

            // Hard precondition bitmask filter
            if ((entries[i].precondition_mask & (~query.available_preconditions)) != 0) continue;

            // Intent bitmask overlap via native @popCount
            const overlap: u32 = @popCount(entries[i].intent_mask & query.required_intent_mask);
            if (query.required_intent_mask != 0 && overlap == 0) continue;

            // Latency ceiling filter
            if (query.max_latency_ms > 0 && entries[i].avg_latency_ms > query.max_latency_ms) continue;

            // Bayesian reliability floor filter
            const rate_bps: u16 = entries[i].successRateBps();
            if (rate_bps < query.min_success_rate_bps) continue;

            const intent_cov: f32 = if (req_bits > 0) ((overlap as f32) / (req_bits as f32)) else 1.0;
            const rate_f: f32 = (rate_bps as f32) / 10000.0;

            let lat_pen: f32 = 0.0;
            if (query.max_latency_ms > 0) {
                lat_pen = ((entries[i].avg_latency_ms as f32) / (query.max_latency_ms as f32)) * 0.15;
            } else if (entries[i].avg_latency_ms > 0) {
                const l_f: f32 = entries[i].avg_latency_ms as f32;
                lat_pen = (l_f / (l_f + 1000.0)) * 0.10;
            }
            const deg_pen: f32 = if (st == PlaybookStatus.degraded) 0.10 else 0.0;
            const composite: f32 = intent_cov * 0.55 + rate_f * 0.45 - lat_pen - deg_pen;

            const cand = PlaybookMatch{
                playbook_id: entries[i].playbook_id,
                index: i,
                score: composite,
                intent_overlap: overlap,
                success_rate_bps: rate_bps,
            };

            // Insertion sort into top-K slice out_matches[0..matched]
            if (matched < out_matches.len) {
                let pos: usize = matched;
                while (pos > 0 && isBetterMatch(cand, out_matches[pos - 1])) : (pos -= 1) {
                    out_matches[pos] = out_matches[pos - 1];
                }
                out_matches[pos] = cand;
                matched += 1;
            } else if (isBetterMatch(cand, out_matches[matched - 1])) {
                let pos: usize = matched - 1;
                while (pos > 0 && isBetterMatch(cand, out_matches[pos - 1])) : (pos -= 1) {
                    out_matches[pos] = out_matches[pos - 1];
                }
                out_matches[pos] = cand;
            }
        }
        return matched;
    }

    /// Returns aggregate inventory and execution telemetry statistics.
    fn stats(): PlaybookStoreStats {
        let s = PlaybookStoreStats{
            total_executions: total_executions,
            total_successes: total_successes,
        };
        let i: usize = 0;
        while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active_slot) continue;
            s.total_playbooks += 1;
            if (entries[i].status == PlaybookStatus.active) {
                s.active_count += 1;
            } else if (entries[i].status == PlaybookStatus.degraded) {
                s.degraded_count += 1;
            } else if (entries[i].status == PlaybookStatus.deprecated) {
                s.deprecated_count += 1;
            }
        }
        return s;
    }
}
