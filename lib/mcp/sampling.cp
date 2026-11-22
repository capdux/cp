// Origin: Pure-CP Stdlib (MCP Sampling Model Preference Router, Recursion Guard & Progress Tracker) | Created: 2026-11-22
// Zero-heap-allocation Model Context Protocol (MCP 2025-03-26) sampling/createMessage
// model preference scoring router, nested reverse-sampling recursion/token budget guard,
// and fixed-slot monotonic notifications/progress token tracker.

/// Returns `true` iff `v` is a finite floating-point weight within `[0.0, 1.0]`.
/// Rejects `NaN`, `-Inf`, `+Inf`, negative values, and values strictly greater than `1.0`.
pub fn isValidUnitWeight(v: f32): bool {
    return (v == v) && (v >= 0.0) && (v <= 1.0);
}

fn clampUnitScore(v: f32): f32 {
    if (v != v || v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

/// MCP 2025-03-26 `ModelPreferences` three-dimensional priority weights in `[0.0, 1.0]`.
pub struct ModelPreferences {
    cost_priority: f32 = 0.0;
    speed_priority: f32 = 0.0;
    intelligence_priority: f32 = 0.0;

    /// Returns equal-weight balanced preferences across cost, speed, and intelligence.
    static fn balanced(): ModelPreferences {
        return ModelPreferences{
            cost_priority: 0.5,
            speed_priority: 0.5,
            intelligence_priority: 0.5,
        };
    }

    /// Returns `true` iff all three priority weights are finite and within `[0.0, 1.0]`.
    fn isValid(): bool {
        return isValidUnitWeight(_.cost_priority)
            && isValidUnitWeight(_.speed_priority)
            && isValidUnitWeight(_.intelligence_priority);
    }

    /// Returns a sanitized copy with NaN/negative weights clamped to `0.0` and `> 1.0` clamped to `1.0`.
    fn clamped(): ModelPreferences {
        return ModelPreferences{
            cost_priority: clampUnitScore(_.cost_priority),
            speed_priority: clampUnitScore(_.speed_priority),
            intelligence_priority: clampUnitScore(_.intelligence_priority),
        };
    }
}

/// Candidate LLM descriptor evaluated by `selectSamplingModel`.
pub struct ModelCandidate {
    model_id: u16 = 0;
    cost_score: f32 = 0.0;
    speed_score: f32 = 0.0;
    intelligence_score: f32 = 0.0;
    max_tokens: u32 = 0;
    available: bool = false;
}

/// Computes the normalized weighted preference score in `[0.0, 1.0]` for a candidate model.
/// Returns `null` if `prefs` contains invalid weights (out of `[0, 1]`, NaN, or Inf),
/// or if any candidate score is non-finite (`NaN`/`Inf`).
/// When all three weights are `0.0`, falls back to the unweighted average across all three dimensions.
pub fn scoreSamplingCandidate(prefs: ModelPreferences, candidate: ModelCandidate): f32? {
    if (!prefs.isValid()) return null;
    if (!isValidUnitWeight(candidate.cost_score)
        || !isValidUnitWeight(candidate.speed_score)
        || !isValidUnitWeight(candidate.intelligence_score))
    {
        return null;
    }

    const w_sum: f32 = prefs.cost_priority + prefs.speed_priority + prefs.intelligence_priority;
    if (w_sum <= 0.0) {
        return (candidate.cost_score + candidate.speed_score + candidate.intelligence_score) / 3.0;
    }

    const weighted: f32 = (prefs.cost_priority * candidate.cost_score)
        + (prefs.speed_priority * candidate.speed_score)
        + (prefs.intelligence_priority * candidate.intelligence_score);
    return weighted / w_sum;
}

/// Selects the optimal `model_id` from `candidates` matching MCP `sampling/createMessage` preferences.
/// Filters out unavailable candidates, candidates with `max_tokens < min_tokens`, and candidates with
/// invalid (non-unit / NaN / Inf) scores. Rejects invalid `prefs` weights by returning `null`.
/// Deterministic tie-breaking preserves the earlier candidate in `candidates` slice order.
pub fn selectSamplingModel(
    prefs: ModelPreferences,
    min_tokens: u32,
    candidates: []const ModelCandidate,
): u16? {
    if (!prefs.isValid()) return null;
    if (candidates.len == 0) return null;

    let best_id: u16 = 0;
    let best_score: f32 = -1.0;
    let found: bool = false;

    let i: usize = 0;
    while (i < candidates.len) : (i += 1) {
        const cand = candidates[i];
        if (!cand.available) continue;
        if (cand.max_tokens < min_tokens) continue;

        if (score := scoreSamplingCandidate(prefs, cand)) {
            if (!found || score > best_score) {
                best_score = score;
                best_id = cand.model_id;
                found = true;
            }
        }
    }

    if (!found) return null;
    return best_id;
}

/// Nested reverse-sampling recursion depth and per-tree token budget guard.
/// Prevents MCP servers issuing nested `sampling/createMessage` callbacks from triggering
/// unbounded recursion loops or token budget exhaustion.
pub struct SamplingDepthGuard {
    max_depth: u8 = 0;
    max_tokens_per_tree: u32 = 0;
    current_depth: u8 = 0;
    spent_tokens: u32 = 0;

    /// Initializes a guard with hard limits on recursion depth and cumulative tree token budget.
    static fn init(max_depth: u8, max_tokens_per_tree: u32): SamplingDepthGuard {
        return SamplingDepthGuard{
            max_depth: max_depth,
            max_tokens_per_tree: max_tokens_per_tree,
            current_depth: 0,
            spent_tokens: 0,
        };
    }

    /// Returns remaining token budget in the current sampling call tree.
    fn remainingTokens(): u32 {
        if (_.spent_tokens >= _.max_tokens_per_tree) return 0;
        return _.max_tokens_per_tree - _.spent_tokens;
    }

    /// Returns remaining nesting levels available before `max_depth` is reached.
    fn remainingDepth(): u8 {
        if (_.current_depth >= _.max_depth) return 0;
        return _.max_depth - _.current_depth;
    }

    /// Attempts to enter one nested sampling frame reserving `requested_tokens`.
    /// Returns `false` without mutating state if `current_depth >= max_depth` or
    /// `requested_tokens > remainingTokens()` (overflow-safe).
    fn tryEnter(*_, requested_tokens: u32): bool {
        if (_.max_depth == 0 || _.current_depth >= _.max_depth) {
            return false;
        }
        const rem = _.remainingTokens();
        if (requested_tokens > rem) {
            return false;
        }
        _.current_depth += 1;
        _.spent_tokens += requested_tokens;
        return true;
    }

    /// Exits one nested sampling frame. Saturates safely at `0` on mismatched extra calls.
    fn exit(*_) {
        if (_.current_depth > 0) {
            _.current_depth -= 1;
        }
    }

    /// Refunds unused reserved tokens back to the tree budget (saturates at `0`).
    fn refundTokens(*_, unused_tokens: u32) {
        if (unused_tokens >= _.spent_tokens) {
            _.spent_tokens = 0;
        } else {
            _.spent_tokens -= unused_tokens;
        }
    }

    /// Resets depth and cumulative spent tokens for reuse on a new root request tree.
    fn reset(*_) {
        _.current_depth = 0;
        _.spent_tokens = 0;
    }
}

/// Fixed-slot state for a single MCP `notifications/progress` token.
pub struct ProgressEntry {
    token_id: u64 = 0;
    progress: u64 = 0;
    total: u64 = 0;
    completed: bool = false;
    occupied: bool = false;
}

/// Outcome of `McpProgressTracker.update`.
pub enum ProgressUpdateStatus {
    Updated,
    Completed,
    NotFound,
    AlreadyCompleted,
    NonMonotonic,
    ExceedsTotal,
}

/// Zero-heap-allocation fixed-slot tracker for MCP `notifications/progress` tokens.
/// Enforces strict monotonic progress advancement, completion tracking, percentage reporting,
/// and deterministic slot recycling.
pub struct McpProgressTracker {
    slots: []ProgressEntry;
    active_count: usize = 0;

    /// Initializes a tracker backed by caller-owned `slots` buffer and clears all slots.
    /// Safely supports zero-length slices (all registrations cleanly return `false`).
    static fn init(slots: *[]ProgressEntry): McpProgressTracker {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = ProgressEntry{};
        }
        return McpProgressTracker{
            slots: slots,
            active_count: 0,
        };
    }

    /// Returns total fixed slot capacity.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Returns number of currently occupied progress token slots.
    fn activeCount(): usize {
        return _.active_count;
    }

    /// Locates the slot index of an active `token_id`, or `null` if absent or `token_id == 0`.
    fn findIndex(token_id: u64): usize? {
        if (token_id == 0) return null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].token_id == token_id) {
                return i;
            }
        }
        return null;
    }

    /// Returns a read-only copy of the progress entry for `token_id`, or `null` if absent.
    fn get(token_id: u64): ProgressEntry? {
        if (idx := _.findIndex(token_id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Registers a new progress token (`total == 0` denotes indeterminate total).
    /// Rejects `token_id == 0`, duplicate active tokens, zero-capacity buffers, or full pools.
    fn register(*_, token_id: u64, total: u64): bool {
        if (token_id == 0 || _.slots.len == 0) return false;
        if (_.findIndex(token_id) != null) return false;

        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                _.slots[i] = ProgressEntry{
                    token_id: token_id,
                    progress: 0,
                    total: total,
                    completed: false,
                    occupied: true,
                };
                _.active_count += 1;
                return true;
            }
        }
        return false;
    }

    /// Advances progress monotonically for `token_id`.
    /// Rejects non-monotonic updates (`progress <= current_progress`), updates on already completed
    /// tokens, or values exceeding `effective_total` (when `effective_total > 0`).
    /// Automatically marks the entry completed when `effective_total > 0 && progress == effective_total`.
    fn update(*_, token_id: u64, progress: u64, total: u64?): ProgressUpdateStatus {
        const idx_opt = _.findIndex(token_id);
        if (idx_opt == null) return ProgressUpdateStatus.NotFound;
        const idx = idx_opt ?? 0;

        if (_.slots[idx].completed) {
            return ProgressUpdateStatus.AlreadyCompleted;
        }
        if (progress <= _.slots[idx].progress) {
            return ProgressUpdateStatus.NonMonotonic;
        }

        let effective_total = _.slots[idx].total;
        if (new_total := total) {
            if (new_total > 0) {
                effective_total = new_total;
            }
        }

        if (effective_total > 0 && progress > effective_total) {
            return ProgressUpdateStatus.ExceedsTotal;
        }

        _.slots[idx].progress = progress;
        _.slots[idx].total = effective_total;

        if (effective_total > 0 && progress == effective_total) {
            _.slots[idx].completed = true;
            return ProgressUpdateStatus.Completed;
        }
        return ProgressUpdateStatus.Updated;
    }

    /// Explicitly marks `token_id` as completed (useful for indeterminate `total == 0` operations).
    /// Returns `false` if `token_id` is not registered.
    fn complete(*_, token_id: u64): bool {
        if (idx := _.findIndex(token_id)) {
            if (_.slots[idx].total > 0 && _.slots[idx].progress < _.slots[idx].total) {
                _.slots[idx].progress = _.slots[idx].total;
            }
            _.slots[idx].completed = true;
            return true;
        }
        return false;
    }

    /// Computes completion percentage in `[0.0, 100.0]` for `token_id`, or `null` if not found.
    /// Returns `100.0` when `completed == true`, and `0.0` for incomplete indeterminate (`total == 0`) entries.
    fn percent(token_id: u64): f32? {
        if (idx := _.findIndex(token_id)) {
            const entry = _.slots[idx];
            if (entry.completed) return 100.0;
            if (entry.total == 0) return 0.0;
            const pct: f32 = ((entry.progress as f32) * 100.0) / (entry.total as f32);
            if (pct > 100.0) return 100.0;
            return pct;
        }
        return null;
    }

    /// Releases the slot occupied by `token_id` so it can be recycled by future registrations.
    /// Returns `false` if `token_id` is not currently tracked.
    fn remove(*_, token_id: u64): bool {
        if (idx := _.findIndex(token_id)) {
            _.slots[idx] = ProgressEntry{};
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
            return true;
        }
        return false;
    }

    /// Reclaims all slots whose entries are marked `completed == true` and returns the number reclaimed.
    fn purgeCompleted(*_): usize {
        let reclaimed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].completed) {
                _.slots[i] = ProgressEntry{};
                if (_.active_count > 0) {
                    _.active_count -= 1;
                }
                reclaimed += 1;
            }
        }
        return reclaimed;
    }
}
