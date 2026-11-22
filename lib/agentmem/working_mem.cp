// Origin: Pure-CP Stdlib (AI Agent Working Memory & Token-Budget Context Window Manager) | Created: 2026-11-22
// Pure CP zero-allocation working memory and sliding token-budget context window manager.
// Supports pinned prompt protection, FIFO / LRU / LowestScore eviction policies,
// and atomic oldest-slot compaction into Summary entries.

const std = @import("std");

/// Semantic role of a slot in the AI Agent working memory window.
pub enum MemSlotRole {
    System,
    User,
    Assistant,
    Tool,
    Summary,
}

/// Eviction strategy applied when token budget or slot capacity is exceeded.
pub enum EvictionPolicy {
    FifoOldest,
    Lru,
    LowestScore,
}

/// Fixed-layout working memory slot entry (zero heap ownership).
pub struct WorkingMemItem {
    id: u32 = 0,
    role: MemSlotRole = .User,
    tokens: u32 = 0,
    importance: f32 = 0.5,
    created_turn: u32 = 0,
    last_accessed_turn: u32 = 0,
    pinned: bool = false,
    content_hash: u64 = 0,
    occupied: bool = false,
}

/// Runtime telemetry snapshot for `WorkingMemory`.
pub struct WorkingMemStats {
    active_items: u32 = 0,
    pinned_items: u32 = 0,
    used_tokens: u32 = 0,
    budget_tokens: u32 = 0,
    evicted_total: u64 = 0,
    compacted_total: u64 = 0,
    utilization_pct: f32 = 0.0,
}

// ============================================================================
// Internal Scoring & Ordering Helpers
// ============================================================================

fn isValidImportance(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x) && x >= 0.0 && x <= 1.0;
}

fn maxU32(a: u32, b: u32): u32 {
    if (a >= b) {
        return a;
    }
    return b;
}

/// Computes composite retention score in `[0.0, 1.0]` combining importance and turn-based decay.
fn computeRetentionScore(item: WorkingMemItem, ref_turn: u32): f32 {
    let age_turns: u32 = 0;
    if (ref_turn > item.last_accessed_turn) {
        age_turns = ref_turn - item.last_accessed_turn;
    }
    const recency: f32 = 1.0 / (1.0 + 0.15 * (age_turns as f32));
    return 0.65 * item.importance + 0.35 * recency;
}

/// Returns `true` if `a` was created strictly earlier than `b` (tie-broken by ascending `id`).
fn isCreatedEarlier(a: WorkingMemItem, b: WorkingMemItem): bool {
    if (a.created_turn != b.created_turn) {
        return a.created_turn < b.created_turn;
    }
    return a.id < b.id;
}

/// Returns `true` if `candidate` should be evicted before `current` under `policy`.
fn isBetterEvictionVictim(
    candidate: WorkingMemItem,
    current: WorkingMemItem,
    policy: EvictionPolicy,
    ref_turn: u32,
): bool {
    switch (policy) {
        .FifoOldest => {
            return isCreatedEarlier(candidate, current);
        },
        .Lru => {
            if (candidate.last_accessed_turn != current.last_accessed_turn) {
                return candidate.last_accessed_turn < current.last_accessed_turn;
            }
            return isCreatedEarlier(candidate, current);
        },
        .LowestScore => {
            const cand_score: f32 = computeRetentionScore(candidate, ref_turn);
            const curr_score: f32 = computeRetentionScore(current, ref_turn);
            if (cand_score < curr_score) {
                return true;
            }
            if (cand_score > curr_score) {
                return false;
            }
            if (candidate.last_accessed_turn != current.last_accessed_turn) {
                return candidate.last_accessed_turn < current.last_accessed_turn;
            }
            return isCreatedEarlier(candidate, current);
        },
    }
}

fn isStrictlyAfterCursor(item: WorkingMemItem, cursor_turn: u32, cursor_id: u32): bool {
    if (item.created_turn != cursor_turn) {
        return item.created_turn > cursor_turn;
    }
    return item.id > cursor_id;
}

// ============================================================================
// WorkingMemory: Zero-Allocation Token-Budget Context Window Manager
// ============================================================================

/// Fixed-capacity working memory pool enforcing a hard `token_budget` with pinned-slot protection.
pub struct WorkingMemory {
    slots: []WorkingMemItem,
    token_budget: u32 = 0,
    used_tokens: u32 = 0,
    policy: EvictionPolicy = .Lru,
    next_id: u32 = 1,
    current_turn: u32 = 0,
    evicted_total: u64 = 0,
    compacted_total: u64 = 0,

    /// Initializes a zero-allocation `WorkingMemory` over caller-provided fixed `slots`.
    /// Resets all slots to unoccupied state so uninitialized stack buffers are safe.
    pub static fn init(
        slots: *[]WorkingMemItem,
        token_budget: u32,
        policy: EvictionPolicy,
    ): WorkingMemory {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = WorkingMemItem{};
        }
        return WorkingMemory{
            slots: slots,
            token_budget: token_budget,
            used_tokens: 0,
            policy: policy,
            next_id: 1,
            current_turn: 0,
            evicted_total: 0,
            compacted_total: 0,
        };
    }

    fn findSlotById(id: u32): usize? {
        if (id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].id == id) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                return i;
            }
        }
        return null;
    }

    fn selectVictimIndex(ref_turn: u32): usize? {
        let best_idx: usize? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const item = _.slots[i];
            if (!item.occupied || item.pinned) {
                continue;
            }
            if (cur_idx := best_idx) {
                if (isBetterEvictionVictim(item, _.slots[cur_idx], _.policy, ref_turn)) {
                    best_idx = i;
                }
            } else {
                best_idx = i;
            }
        }
        return best_idx;
    }

    /// Updates the eviction policy used for subsequent insertions.
    pub fn setPolicy(*_, policy: EvictionPolicy) {
        _.policy = policy;
    }

    /// Returns a copy of an active working memory item by `id`, or `null` if not found.
    pub fn get(id: u32): WorkingMemItem? {
        if (idx := _.findSlotById(id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Returns `true` if `id` is currently active in the working memory window.
    pub fn contains(id: u32): bool {
        return _.findSlotById(id) != null;
    }

    /// Inserts a new item into working memory, evicting non-pinned entries as needed
    /// to satisfy `token_budget` and slot capacity.
    ///
    /// Atomically rejects (returns `null` with zero side effects) if:
    /// - `slots` capacity is 0,
    /// - `importance` is `NaN`, `Inf`, or outside `[0.0, 1.0]`,
    /// - `tokens > token_budget`,
    /// - `pinned_tokens + tokens > token_budget`, or
    /// - all slots are occupied by `pinned` entries.
    pub fn push(
        *_,
        role: MemSlotRole,
        tokens: u32,
        importance: f32,
        turn: u32,
        pinned: bool,
        content_hash: u64,
    ): u32? {
        if (_.slots.len == 0) {
            return null;
        }
        if (!isValidImportance(importance)) {
            return null;
        }
        if (tokens > _.token_budget) {
            return null;
        }

        // Atomic feasibility pre-check: ensure pinned items never get evicted and
        // failed pushes never disturb existing entries.
        let pinned_tokens: u64 = 0;
        let pinned_count: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const item = _.slots[i];
            if (item.occupied && item.pinned) {
                pinned_tokens += item.tokens as u64;
                pinned_count += 1;
            }
        }
        if (pinned_tokens + (tokens as u64) > (_.token_budget as u64)) {
            return null;
        }
        if (pinned_count >= _.slots.len) {
            return null;
        }

        const ref_turn: u32 = maxU32(_.current_turn, turn);

        // Evict non-pinned items until both token budget and a free slot are available.
        while (((_.used_tokens as u64) + (tokens as u64) > (_.token_budget as u64)) || (_.findFreeSlot() == null)) {
            if (victim_idx := _.selectVictimIndex(ref_turn)) {
                _.used_tokens -= _.slots[victim_idx].tokens;
                _.slots[victim_idx] = WorkingMemItem{};
                _.evicted_total += 1;
            } else {
                return null;
            }
        }

        if (free_idx := _.findFreeSlot()) {
            const id: u32 = _.next_id;
            _.next_id += 1;
            _.current_turn = ref_turn;
            _.slots[free_idx] = WorkingMemItem{
                id: id,
                role: role,
                tokens: tokens,
                importance: importance,
                created_turn: turn,
                last_accessed_turn: turn,
                pinned: pinned,
                content_hash: content_hash,
                occupied: true,
            };
            _.used_tokens += tokens;
            return id;
        }
        return null;
    }

    /// Refreshes the `last_accessed_turn` of an active item. Returns `false` if not found.
    pub fn touch(*_, id: u32, turn: u32): bool {
        if (idx := _.findSlotById(id)) {
            _.slots[idx].last_accessed_turn = turn;
            _.current_turn = maxU32(_.current_turn, turn);
            return true;
        }
        return false;
    }

    /// Marks an active item as pinned (immune to automatic eviction and compaction).
    pub fn pin(*_, id: u32): bool {
        if (idx := _.findSlotById(id)) {
            _.slots[idx].pinned = true;
            return true;
        }
        return false;
    }

    /// Clears the pinned flag on an active item so it can participate in eviction/compaction.
    pub fn unpin(*_, id: u32): bool {
        if (idx := _.findSlotById(id)) {
            _.slots[idx].pinned = false;
            return true;
        }
        return false;
    }

    /// Explicitly removes an active item (including pinned items) and frees its token budget.
    pub fn remove(*_, id: u32): bool {
        if (idx := _.findSlotById(id)) {
            _.used_tokens -= _.slots[idx].tokens;
            _.slots[idx] = WorkingMemItem{};
            return true;
        }
        return false;
    }

    /// Atomically folds up to `count` oldest non-pinned items into a single `.Summary` slot.
    ///
    /// Returns the new summary item's `id`, or `null` with zero side effects if:
    /// - `count == 0`,
    /// - no non-pinned active items exist, or
    /// - replacing the selected items with `summary_tokens` would exceed `token_budget`.
    pub fn compactOldestIntoSummary(
        *_,
        count: u32,
        summary_tokens: u32,
        turn: u32,
        summary_hash: u64,
    ): u32? {
        if (count == 0 || _.slots.len == 0) {
            return null;
        }

        // Count eligible non-pinned items.
        let eligible: u32 = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && !_.slots[i].pinned) {
                eligible += 1;
            }
        }
        if (eligible == 0) {
            return null;
        }
        let target_count: u32 = count;
        if (target_count > eligible) {
            target_count = eligible;
        }

        // Pass 1 (read-only): sum tokens and track peak importance of the oldest `target_count`
        // non-pinned entries in strict `(created_turn, id)` ascending order.
        let freed_tokens: u64 = 0;
        let peak_importance: f32 = 0.7;
        let has_cursor: bool = false;
        let cursor_turn: u32 = 0;
        let cursor_id: u32 = 0;

        let step: u32 = 0;
        while (step < target_count) : (step += 1) {
            let pick_idx: usize? = null;
            let j: usize = 0;
            while (j < _.slots.len) : (j += 1) {
                const item = _.slots[j];
                if (!item.occupied || item.pinned) {
                    continue;
                }
                if (has_cursor && !isStrictlyAfterCursor(item, cursor_turn, cursor_id)) {
                    continue;
                }
                if (cur := pick_idx) {
                    if (isCreatedEarlier(item, _.slots[cur])) {
                        pick_idx = j;
                    }
                } else {
                    pick_idx = j;
                }
            }
            if (p := pick_idx) {
                const picked = _.slots[p];
                freed_tokens += picked.tokens as u64;
                if (picked.importance > peak_importance) {
                    peak_importance = picked.importance;
                }
                has_cursor = true;
                cursor_turn = picked.created_turn;
                cursor_id = picked.id;
            } else {
                break;
            }
        }

        // Atomic budget check before mutating any slot.
        const projected_tokens: u64 = (_.used_tokens as u64) - freed_tokens + (summary_tokens as u64);
        if (projected_tokens > (_.token_budget as u64)) {
            return null;
        }

        // Pass 2 (atomic commit): evict the exact `target_count` oldest non-pinned entries.
        let summary_slot: usize = 0;
        let has_summary_slot: bool = false;
        let committed: u32 = 0;
        while (committed < target_count) : (committed += 1) {
            let pick_idx: usize? = null;
            let j: usize = 0;
            while (j < _.slots.len) : (j += 1) {
                const item = _.slots[j];
                if (!item.occupied || item.pinned) {
                    continue;
                }
                if (cur := pick_idx) {
                    if (isCreatedEarlier(item, _.slots[cur])) {
                        pick_idx = j;
                    }
                } else {
                    pick_idx = j;
                }
            }
            if (p := pick_idx) {
                _.used_tokens -= _.slots[p].tokens;
                _.slots[p] = WorkingMemItem{};
                _.compacted_total += 1;
                if (!has_summary_slot) {
                    summary_slot = p;
                    has_summary_slot = true;
                }
            } else {
                break;
            }
        }

        if (!has_summary_slot) {
            return null;
        }

        const id: u32 = _.next_id;
        _.next_id += 1;
        _.current_turn = maxU32(_.current_turn, turn);
        _.slots[summary_slot] = WorkingMemItem{
            id: id,
            role: .Summary,
            tokens: summary_tokens,
            importance: peak_importance,
            created_turn: turn,
            last_accessed_turn: turn,
            pinned: false,
            content_hash: summary_hash,
            occupied: true,
        };
        _.used_tokens += summary_tokens;
        return id;
    }

    /// Returns a telemetry snapshot of slot occupancy, token utilization, and eviction counters.
    pub fn stats(): WorkingMemStats {
        let active: u32 = 0;
        let pinned: u32 = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const item = _.slots[i];
            if (item.occupied) {
                active += 1;
                if (item.pinned) {
                    pinned += 1;
                }
            }
        }
        let util: f32 = 0.0;
        if (_.token_budget > 0) {
            util = ((_.used_tokens as f32) * 100.0) / (_.token_budget as f32);
        }
        return WorkingMemStats{
            active_items: active,
            pinned_items: pinned,
            used_tokens: _.used_tokens,
            budget_tokens: _.token_budget,
            evicted_total: _.evicted_total,
            compacted_total: _.compacted_total,
            utilization_pct: util,
        };
    }
}
