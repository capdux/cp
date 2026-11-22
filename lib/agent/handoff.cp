// Origin: Pure-CP Stdlib (Multi-Agent Collaboration: Handoff Router, Versioned Blackboard & Fan-Out Quorum Aggregator) | Created: 2026-11-22
// Pure CP zero-heap-allocation multi-agent orchestration primitives:
//   1. `HandoffRouter` / `HandoffRecord`: explicit agent handoff routing with max-hops cap, A->B->A cycle guard & full trace
//   2. `AgentBlackboard` / `BlackboardEntry`: fixed-slot versioned shared state with optimistic CAS (`putIfVersion`), writer audit & prefix/seq incremental reads
//   3. `FanOutAggregator` / `FanOutSlot`: parallel sub-agent task submission, quorum early-ready detection & score-ranked result collection

const std = @import("std");

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// 1. Multi-Agent Handoff Routing & Cycle Protection (`HandoffRouter`)
// ============================================================================

/// Outcome of a `HandoffRouter.handoff` routing attempt.
pub enum HandoffStatus {
    Accepted,
    SelfHandoff,
    CycleDetected,
    MaxHopsExceeded,
    HistoryFull,
}

/// Immutable trace record of a single agent-to-agent handoff transition.
pub struct HandoffRecord {
    hop_index: u32 = 0;
    from_agent: u32 = 0;
    to_agent: u32 = 0;
    timestamp_ms: i64 = 0;
    summary: string = "";
}

/// Zero-heap-allocation multi-agent handoff router with hop budget, cycle guard, and full trajectory audit.
pub struct HandoffRouter {
    records: []HandoffRecord;
    count: usize = 0;
    initial_agent: u32 = 0;
    current_agent: u32 = 0;
    max_hops: u32 = 0;
    forbid_cycles: bool = true;

    /// Initializes a handoff router backed by caller-provided `records` storage.
    /// Returns `null` if `max_hops == 0` or `records.len == 0`.
    static fn init(
        records: *[]HandoffRecord,
        initial_agent: u32,
        max_hops: u32,
        forbid_cycles: bool,
    ): HandoffRouter? {
        if (max_hops == 0 || records.len == 0) return null;

        let i: usize = 0;
        while (i < records.len) : (i += 1) {
            records[i] = HandoffRecord{};
        }

        return HandoffRouter{
            records: records,
            count: 0,
            initial_agent: initial_agent,
            current_agent: initial_agent,
            max_hops: max_hops,
            forbid_cycles: forbid_cycles,
        };
    }

    /// Returns `true` if `agent_id` is the initial agent or has already been visited in the active handoff chain.
    fn hasVisited(agent_id: u32): bool {
        if (agent_id == _.initial_agent) return true;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.records[i].to_agent == agent_id) return true;
        }
        return false;
    }

    /// Transfers active execution ownership from `current_agent` to `to_agent` with a context `summary`.
    /// Enforces self-handoff rejection, `max_hops` cap, optional `A -> B -> A` cycle detection,
    /// and backing history capacity without mutating state on rejection.
    fn handoff(*_, to_agent: u32, summary: string, timestamp_ms: i64): HandoffStatus {
        if (to_agent == _.current_agent) return HandoffStatus.SelfHandoff;
        if (_.count >= (_.max_hops as usize)) return HandoffStatus.MaxHopsExceeded;
        if (_.forbid_cycles && _.hasVisited(to_agent)) return HandoffStatus.CycleDetected;
        if (_.count >= _.records.len) return HandoffStatus.HistoryFull;

        const hop_num: u32 = (_.count + 1) as u32;
        _.records[_.count] = HandoffRecord{
            hop_index: hop_num,
            from_agent: _.current_agent,
            to_agent: to_agent,
            timestamp_ms: timestamp_ms,
            summary: summary,
        };
        _.count += 1;
        _.current_agent = to_agent;
        return HandoffStatus.Accepted;
    }

    /// Returns the currently active owner agent ID.
    fn currentAgent(): u32 {
        return _.current_agent;
    }

    /// Returns the number of completed handoff hops.
    fn hopCount(): usize {
        return _.count;
    }

    /// Returns remaining allowed hops before hitting `max_hops`.
    fn remainingHops(): usize {
        const cap: usize = _.max_hops as usize;
        if (_.count >= cap) return 0;
        return cap - _.count;
    }

    /// Returns the handoff record at 0-based `index`, or `null` if out of bounds.
    fn recordAt(index: usize): HandoffRecord? {
        if (index >= _.count) return null;
        return _.records[index];
    }

    /// Returns a read-only slice of all recorded handoffs in chronological order.
    fn history(): []const HandoffRecord {
        return _.records[0.._.count];
    }

    /// Resets routing trajectory to a fresh `initial_agent`.
    fn reset(*_, initial_agent: u32) {
        _.count = 0;
        _.initial_agent = initial_agent;
        _.current_agent = initial_agent;
    }
}

// ============================================================================
// 2. Versioned Shared Blackboard with Optimistic CAS (`AgentBlackboard`)
// ============================================================================

/// Outcome of a blackboard write or compare-and-swap (`putIfVersion`) operation.
pub enum BlackboardWriteStatus {
    Ok,
    VersionConflict,
    Full,
    InvalidKey,
}

/// Single versioned key-value slot on the multi-agent shared blackboard.
pub struct BlackboardEntry {
    occupied: bool = false;
    key: string = "";
    value: string = "";
    version: u64 = 0;
    global_seq: u64 = 0;
    writer_agent_id: u32 = 0;
    updated_ms: i64 = 0;
}

/// Zero-heap-allocation versioned shared blackboard for multi-agent state coordination.
/// Supports optimistic locking CAS (`putIfVersion`), writer `agent_id` audit trail,
/// key-prefix filtering (`listByPrefix`), and incremental delta reads (`listSinceSeq`).
pub struct AgentBlackboard {
    entries: []BlackboardEntry;
    active_count: usize = 0;
    global_seq: u64 = 0;

    /// Initializes an empty blackboard over caller-provided `entries` slots.
    static fn init(entries: *[]BlackboardEntry): AgentBlackboard {
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            entries[i] = BlackboardEntry{};
        }
        return AgentBlackboard{
            entries: entries,
            active_count: 0,
            global_seq: 0,
        };
    }

    /// Looks up an active entry by exact `key`, returning `null` if absent.
    fn get(key: string): BlackboardEntry? {
        if (key.len == 0) return null;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied && std.mem.eql(u8, _.entries[i].key, key)) {
                return _.entries[i];
            }
        }
        return null;
    }

    /// Unconditionally inserts or updates `key` with `value`, incrementing per-key `version`
    /// and monotonic `global_seq`, and recording `writer_agent_id` for auditability.
    fn put(*_, key: string, value: string, writer_agent_id: u32, now_ms: i64): BlackboardWriteStatus {
        if (key.len == 0) return BlackboardWriteStatus.InvalidKey;

        let free_slot: usize = _.entries.len;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied) {
                if (std.mem.eql(u8, _.entries[i].key, key)) {
                    _.global_seq += 1;
                    _.entries[i].value = value;
                    _.entries[i].version += 1;
                    _.entries[i].global_seq = _.global_seq;
                    _.entries[i].writer_agent_id = writer_agent_id;
                    _.entries[i].updated_ms = now_ms;
                    return BlackboardWriteStatus.Ok;
                }
            } else if (free_slot == _.entries.len) {
                free_slot = i;
            }
        }

        if (free_slot >= _.entries.len) return BlackboardWriteStatus.Full;

        _.global_seq += 1;
        _.entries[free_slot] = BlackboardEntry{
            occupied: true,
            key: key,
            value: value,
            version: 1,
            global_seq: _.global_seq,
            writer_agent_id: writer_agent_id,
            updated_ms: now_ms,
        };
        _.active_count += 1;
        return BlackboardWriteStatus.Ok;
    }

    /// Optimistic-concurrency compare-and-swap (CAS) write:
    /// - If `key` exists: succeeds only when `entry.version == expected_version`, incrementing `version` by 1.
    /// - If `key` does not exist: succeeds only when `expected_version == 0` (insert-if-absent), setting `version = 1`.
    /// - Returns `.VersionConflict` without mutating the slot on any version mismatch.
    fn putIfVersion(
        *_,
        key: string,
        value: string,
        expected_version: u64,
        writer_agent_id: u32,
        now_ms: i64,
    ): BlackboardWriteStatus {
        if (key.len == 0) return BlackboardWriteStatus.InvalidKey;

        let free_slot: usize = _.entries.len;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied) {
                if (std.mem.eql(u8, _.entries[i].key, key)) {
                    if (_.entries[i].version != expected_version) {
                        return BlackboardWriteStatus.VersionConflict;
                    }
                    _.global_seq += 1;
                    _.entries[i].value = value;
                    _.entries[i].version += 1;
                    _.entries[i].global_seq = _.global_seq;
                    _.entries[i].writer_agent_id = writer_agent_id;
                    _.entries[i].updated_ms = now_ms;
                    return BlackboardWriteStatus.Ok;
                }
            } else if (free_slot == _.entries.len) {
                free_slot = i;
            }
        }

        if (expected_version != 0) return BlackboardWriteStatus.VersionConflict;
        if (free_slot >= _.entries.len) return BlackboardWriteStatus.Full;

        _.global_seq += 1;
        _.entries[free_slot] = BlackboardEntry{
            occupied: true,
            key: key,
            value: value,
            version: 1,
            global_seq: _.global_seq,
            writer_agent_id: writer_agent_id,
            updated_ms: now_ms,
        };
        _.active_count += 1;
        return BlackboardWriteStatus.Ok;
    }

    /// Removes an active entry by exact `key`. Returns `true` if found and removed.
    fn remove(*_, key: string): bool {
        if (key.len == 0) return false;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied && std.mem.eql(u8, _.entries[i].key, key)) {
                _.entries[i] = BlackboardEntry{};
                if (_.active_count > 0) _.active_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Copies all occupied entries whose key starts with `prefix` into `out_entries` (up to `out_entries.len`).
    /// Returns the number of matching entries written.
    fn listByPrefix(prefix: string, out_entries: *[]BlackboardEntry): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len && written < out_entries.len) : (i += 1) {
            if (_.entries[i].occupied && std.mem.startsWith(u8, _.entries[i].key, prefix)) {
                out_entries[written] = _.entries[i];
                written += 1;
            }
        }
        return written;
    }

    /// Incremental delta read: copies all occupied entries updated after `after_seq`
    /// (`entry.global_seq > after_seq`) into `out_entries` (up to `out_entries.len`).
    /// Returns the number of entries written.
    fn listSinceSeq(after_seq: u64, out_entries: *[]BlackboardEntry): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len && written < out_entries.len) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].global_seq > after_seq) {
                out_entries[written] = _.entries[i];
                written += 1;
            }
        }
        return written;
    }

    /// Returns the number of active key-value entries currently stored.
    fn count(): usize {
        return _.active_count;
    }

    /// Returns the fixed slot capacity of this blackboard.
    fn capacity(): usize {
        return _.entries.len;
    }

    /// Returns the current global monotonic write sequence number.
    fn currentSeq(): u64 {
        return _.global_seq;
    }
}

// ============================================================================
// 3. Parallel Fan-Out / Fan-In Quorum Aggregator (`FanOutAggregator`)
// ============================================================================

/// Execution state of an individual fanned-out sub-agent task.
pub enum FanOutTaskState {
    Pending,
    Succeeded,
    Failed,
}

/// Overall quorum readiness status of a `FanOutAggregator`.
pub enum FanOutStatus {
    InProgress,
    QuorumReached,
    QuorumFailed,
}

/// Tracking slot for a single fanned-out sub-agent task and its result payload.
pub struct FanOutSlot {
    occupied: bool = false;
    task_id: u32 = 0;
    worker_agent_id: u32 = 0;
    state: FanOutTaskState = .Pending;
    score: f32 = 0.0;
    result_summary: string = "";
    completed_ms: i64 = 0;
}

/// Snapshot of fan-out task progress and quorum status.
pub struct FanOutSummary {
    submitted: usize = 0;
    pending: usize = 0;
    succeeded: usize = 0;
    failed: usize = 0;
    quorum: usize = 0;
    status: FanOutStatus = .InProgress;
}

fn fanOutSlotBetterDesc(a: FanOutSlot, b: FanOutSlot): bool {
    if (a.score != b.score) return a.score > b.score;
    return a.task_id < b.task_id;
}

fn sortFanOutSlotsDesc(slots: *[]FanOutSlot, count: usize) {
    if (count <= 1) return;
    let i: usize = 1;
    while (i < count) : (i += 1) {
        const key = slots[i];
        let j: usize = i;
        while (j > 0 && fanOutSlotBetterDesc(key, slots[j - 1])) : (j -= 1) {
            slots[j] = slots[j - 1];
        }
        slots[j] = key;
    }
}

/// Zero-heap-allocation parallel fan-out / fan-in quorum aggregator for multi-agent execution.
/// Tracks sub-agent task completion/failure, detects early quorum readiness (`succeeded >= quorum`)
/// or quorum impossibility, and extracts succeeded results ranked descending by score.
pub struct FanOutAggregator {
    slots: []FanOutSlot;
    submitted_count: usize = 0;
    succeeded_count: usize = 0;
    failed_count: usize = 0;
    quorum: usize = 1;

    /// Initializes a fan-out aggregator over caller-provided `slots` with required `quorum`.
    /// Returns `null` if `quorum == 0` or `slots.len < quorum`.
    static fn init(slots: *[]FanOutSlot, quorum: usize): FanOutAggregator? {
        if (quorum == 0 || slots.len < quorum) return null;

        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = FanOutSlot{};
        }

        return FanOutAggregator{
            slots: slots,
            submitted_count: 0,
            succeeded_count: 0,
            failed_count: 0,
            quorum: quorum,
        };
    }

    /// Registers a new parallel sub-agent task in `.Pending` state.
    /// Rejects duplicate `task_id` or full slot capacity (returns `false`).
    fn submit(*_, task_id: u32, worker_agent_id: u32): bool {
        if (_.submitted_count >= _.slots.len) return false;

        let i: usize = 0;
        while (i < _.submitted_count) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                return false;
            }
        }

        _.slots[_.submitted_count] = FanOutSlot{
            occupied: true,
            task_id: task_id,
            worker_agent_id: worker_agent_id,
            state: FanOutTaskState.Pending,
            score: 0.0,
            result_summary: "",
            completed_ms: 0,
        };
        _.submitted_count += 1;
        return true;
    }

    /// Marks a `.Pending` task as `.Succeeded` with its confidence/relevance `score` and `result_summary`.
    /// Rejects unknown `task_id`, already-terminal tasks, or non-finite `score` (returns `false`).
    fn recordSuccess(
        *_,
        task_id: u32,
        score: f32,
        result_summary: string,
        completed_ms: i64,
    ): bool {
        if (!isFiniteF32(score)) return false;

        let i: usize = 0;
        while (i < _.submitted_count) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                if (_.slots[i].state != FanOutTaskState.Pending) return false;
                _.slots[i].state = FanOutTaskState.Succeeded;
                _.slots[i].score = score;
                _.slots[i].result_summary = result_summary;
                _.slots[i].completed_ms = completed_ms;
                _.succeeded_count += 1;
                return true;
            }
        }
        return false;
    }

    /// Marks a `.Pending` task as `.Failed` with a failure `reason`.
    /// Rejects unknown `task_id` or already-terminal tasks (returns `false`).
    fn recordFailure(*_, task_id: u32, reason: string, completed_ms: i64): bool {
        let i: usize = 0;
        while (i < _.submitted_count) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                if (_.slots[i].state != FanOutTaskState.Pending) return false;
                _.slots[i].state = FanOutTaskState.Failed;
                _.slots[i].result_summary = reason;
                _.slots[i].completed_ms = completed_ms;
                _.failed_count += 1;
                return true;
            }
        }
        return false;
    }

    /// Returns `true` as soon as `succeeded_count >= quorum` (enabling early fan-in proceed).
    fn isQuorumReady(): bool {
        return _.succeeded_count >= _.quorum;
    }

    /// Computes overall quorum status:
    /// - `.QuorumReached`: `succeeded_count >= quorum`
    /// - `.QuorumFailed`: remaining capacity or submitted pending tasks cannot reach `quorum`
    /// - `.InProgress`: quorum not yet reached and still achievable
    fn status(): FanOutStatus {
        if (_.succeeded_count >= _.quorum) return FanOutStatus.QuorumReached;

        const pending: usize = _.submitted_count - _.succeeded_count - _.failed_count;
        const max_possible_in_slots: usize = _.slots.len - _.failed_count;
        if (max_possible_in_slots < _.quorum) return FanOutStatus.QuorumFailed;
        if (_.submitted_count >= _.quorum && (_.succeeded_count + pending) < _.quorum) {
            return FanOutStatus.QuorumFailed;
        }
        return FanOutStatus.InProgress;
    }

    /// Returns an immutable progress & quorum summary snapshot.
    fn summary(): FanOutSummary {
        const pending: usize = _.submitted_count - _.succeeded_count - _.failed_count;
        return FanOutSummary{
            submitted: _.submitted_count,
            pending: pending,
            succeeded: _.succeeded_count,
            failed: _.failed_count,
            quorum: _.quorum,
            status: _.status(),
        };
    }

    /// Extracts all `.Succeeded` task slots into `out_slots` (sorted descending by `score`,
    /// tie-broken deterministically by ascending `task_id`), returning the count written.
    fn collectSucceeded(out_slots: *[]FanOutSlot): usize {
        if (out_slots.len == 0) return 0;

        let collected: usize = 0;
        let i: usize = 0;
        while (i < _.submitted_count) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == FanOutTaskState.Succeeded) {
                if (collected < out_slots.len) {
                    out_slots[collected] = _.slots[i];
                    collected += 1;
                } else {
                    // Maintain top-K best-scoring succeeded results when out_slots is smaller than succeeded_count.
                    sortFanOutSlotsDesc(out_slots, collected);
                    if (fanOutSlotBetterDesc(_.slots[i], out_slots[collected - 1])) {
                        out_slots[collected - 1] = _.slots[i];
                    }
                }
            }
        }

        sortFanOutSlotsDesc(out_slots, collected);
        return collected;
    }
}
