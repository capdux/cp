// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc A2A Task Lifecycle State Machine & Streaming Artifact Router) | Created: 2026-11-22
// Pure CP zero-heap-allocation Google Agent2Agent (A2A) asynchronous task orchestration state machine
// and monotonic streaming artifact chunk router:
//   - Strict 8-phase `A2aTaskPhase` with terminal immutability & spec-compliant edge validation
//   - Fixed 64-task concurrency table + inline per-task `A2aArtifactMeta` slots (zero dynamic heap)
//   - Monotonic `seq` guard & `finalized` sealing for chunked streaming artifact delivery
//   - Cooperative cancellation (`request_cancel`) & expired terminal slot reclamation (`prune_terminal_before`)

const std = @import("std");

// ============================================================================
// 1. Capacity Constants
// ============================================================================

/// Maximum concurrent task slots in `A2aTaskLifecycleManager` (zero dynamic heap).
pub const A2A_LIFECYCLE_MAX_TASKS: usize = 64;
/// Maximum distinct streaming artifacts tracked per task.
pub const A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK: usize = 4;
/// Maximum byte length of an inline artifact identifier.
pub const A2A_ARTIFACT_ID_MAX_LEN: usize = 64;
/// Maximum byte length of an inline artifact MIME type string.
pub const A2A_ARTIFACT_MIME_MAX_LEN: usize = 64;

// ============================================================================
// 2. A2A Task Lifecycle Phase Enum
// ============================================================================

/// Google A2A protocol task execution phases.
pub enum A2aTaskPhase {
    submitted,
    working,
    input_required,
    auth_required,
    completed,
    failed,
    canceled,
    rejected,

    /// Returns `true` for immutable terminal phases (`completed`, `failed`, `canceled`, `rejected`).
    fn isTerminal(): bool {
        return switch (_) {
            A2aTaskPhase.completed,
            A2aTaskPhase.failed,
            A2aTaskPhase.canceled,
            A2aTaskPhase.rejected => true,
            else => false,
        };
    }

    /// Validates whether transitioning from this phase to `next` is legal under A2A lifecycle rules.
    /// Terminal phases and self-loops are always forbidden.
    fn canTransitionTo(next: A2aTaskPhase): bool {
        if (_.isTerminal() || _ == next) {
            return false;
        }
        return switch (_) {
            A2aTaskPhase.submitted => switch (next) {
                A2aTaskPhase.working,
                A2aTaskPhase.input_required,
                A2aTaskPhase.auth_required,
                A2aTaskPhase.rejected,
                A2aTaskPhase.canceled,
                A2aTaskPhase.failed => true,
                else => false,
            },
            A2aTaskPhase.working => switch (next) {
                A2aTaskPhase.input_required,
                A2aTaskPhase.auth_required,
                A2aTaskPhase.completed,
                A2aTaskPhase.failed,
                A2aTaskPhase.canceled => true,
                else => false,
            },
            A2aTaskPhase.input_required => switch (next) {
                A2aTaskPhase.working,
                A2aTaskPhase.auth_required,
                A2aTaskPhase.failed,
                A2aTaskPhase.canceled => true,
                else => false,
            },
            A2aTaskPhase.auth_required => switch (next) {
                A2aTaskPhase.working,
                A2aTaskPhase.input_required,
                A2aTaskPhase.failed,
                A2aTaskPhase.canceled,
                A2aTaskPhase.rejected => true,
                else => false,
            },
            else => false,
        };
    }
}

// ============================================================================
// 3. Data Models: Artifact Meta, Task Record, Transition Outcome & Stats
// ============================================================================

/// Inline metadata tracking a chunked streaming artifact attached to an A2A task.
pub struct A2aArtifactMeta {
    active: bool = false;
    artifact_id: [A2A_ARTIFACT_ID_MAX_LEN]u8 = [0; A2A_ARTIFACT_ID_MAX_LEN];
    artifact_id_len: usize = 0;
    mime_type: [A2A_ARTIFACT_MIME_MAX_LEN]u8 = [0; A2A_ARTIFACT_MIME_MAX_LEN];
    mime_type_len: usize = 0;
    chunks_received: u32 = 0;
    total_bytes: u64 = 0;
    finalized: bool = false;
    last_seq: u32 = 0;
}

/// Lifecycle state record for a single managed A2A task.
pub struct A2aTaskRecord {
    active: bool = false;
    task_id: u64 = 0;
    context_id: u64 = 0;
    phase: A2aTaskPhase = A2aTaskPhase.submitted;
    created_ms: u64 = 0;
    updated_ms: u64 = 0;
    transition_count: u32 = 0;
    artifact_count: usize = 0;
    cancel_requested: bool = false;
}

/// Structured result of an attempted task state transition.
pub struct A2aTransitionOutcome {
    ok: bool = false;
    prev_phase: A2aTaskPhase = A2aTaskPhase.submitted;
    next_phase: A2aTaskPhase = A2aTaskPhase.submitted;
    rejected_terminal: bool = false;
    invalid_edge: bool = false;
}

/// Aggregate phase and artifact statistics snapshot for `A2aTaskLifecycleManager`.
pub struct A2aLifecycleStats {
    total_tasks: usize = 0;
    active_tasks: usize = 0;
    completed_tasks: usize = 0;
    failed_tasks: usize = 0;
    canceled_tasks: usize = 0;
    rejected_tasks: usize = 0;
    total_artifacts: usize = 0;
}

// ============================================================================
// 4. Zero-Alloc A2A Task Lifecycle & Artifact Stream Manager
// ============================================================================

/// Fixed-capacity zero-heap-allocation A2A task lifecycle state machine and
/// streaming artifact chunk router.
pub struct A2aTaskLifecycleManager {
    tasks: [A2A_LIFECYCLE_MAX_TASKS]A2aTaskRecord = [A2aTaskRecord{}; A2A_LIFECYCLE_MAX_TASKS];
    artifacts: [A2A_LIFECYCLE_MAX_TASKS][A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK]A2aArtifactMeta =
        [[A2aArtifactMeta{}; A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK]; A2A_LIFECYCLE_MAX_TASKS];

    /// Initializes an empty 64-slot A2A task lifecycle manager.
    static fn init(): A2aTaskLifecycleManager {
        return A2aTaskLifecycleManager{
            tasks: [A2aTaskRecord{}; A2A_LIFECYCLE_MAX_TASKS],
            artifacts: [[A2aArtifactMeta{}; A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK]; A2A_LIFECYCLE_MAX_TASKS],
        };
    }

    fn findTaskSlot(task_id: u64): usize? {
        if (task_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < A2A_LIFECYCLE_MAX_TASKS) : (i += 1) {
            if (_.tasks[i].active && _.tasks[i].task_id == task_id) {
                return i;
            }
        }
        return null;
    }

    /// Submits a new task in `submitted` phase.
    /// Rejects `task_id == 0`, duplicate `task_id`, or full 64-slot table.
    fn submit_task(*_, task_id: u64, context_id: u64, now_ms: u64): bool {
        if (task_id == 0) {
            return false;
        }
        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < A2A_LIFECYCLE_MAX_TASKS) : (i += 1) {
            if (_.tasks[i].active) {
                if (_.tasks[i].task_id == task_id) {
                    return false;
                }
            } else if (free_idx == null) {
                free_idx = i;
            }
        }

        const slot = free_idx ?? return false;
        _.tasks[slot] = A2aTaskRecord{
            active: true,
            task_id: task_id,
            context_id: context_id,
            phase: A2aTaskPhase.submitted,
            created_ms: now_ms,
            updated_ms: now_ms,
            transition_count: 0,
            artifact_count: 0,
            cancel_requested: false,
        };
        let a: usize = 0;
        while (a < A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK) : (a += 1) {
            _.artifacts[slot][a] = A2aArtifactMeta{};
        }
        return true;
    }

    /// Attempts a spec-validated state transition on `task_id`.
    /// Terminal tasks always yield `rejected_terminal = true`; disallowed edges yield `invalid_edge = true`.
    fn transition(
        *_,
        task_id: u64,
        next_phase: A2aTaskPhase,
        now_ms: u64,
    ): A2aTransitionOutcome {
        const slot = _.findTaskSlot(task_id) ?? return A2aTransitionOutcome{
            ok: false,
            prev_phase: A2aTaskPhase.submitted,
            next_phase: next_phase,
            rejected_terminal: false,
            invalid_edge: true,
        };

        const prev = _.tasks[slot].phase;
        if (prev.isTerminal()) {
            return A2aTransitionOutcome{
                ok: false,
                prev_phase: prev,
                next_phase: next_phase,
                rejected_terminal: true,
                invalid_edge: false,
            };
        }
        if (!prev.canTransitionTo(next_phase)) {
            return A2aTransitionOutcome{
                ok: false,
                prev_phase: prev,
                next_phase: next_phase,
                rejected_terminal: false,
                invalid_edge: true,
            };
        }

        _.tasks[slot].phase = next_phase;
        if (now_ms >= _.tasks[slot].updated_ms) {
            _.tasks[slot].updated_ms = now_ms;
        }
        _.tasks[slot].transition_count += 1;
        if (next_phase == A2aTaskPhase.canceled) {
            _.tasks[slot].cancel_requested = true;
        }

        return A2aTransitionOutcome{
            ok: true,
            prev_phase: prev,
            next_phase: next_phase,
            rejected_terminal: false,
            invalid_edge: false,
        };
    }

    /// Requests cancellation of a non-terminal task and transitions it to `canceled`.
    /// Returns `false` if the task does not exist or is already in a terminal phase.
    fn request_cancel(*_, task_id: u64, now_ms: u64): bool {
        const slot = _.findTaskSlot(task_id) ?? return false;
        if (_.tasks[slot].phase.isTerminal()) {
            return false;
        }
        _.tasks[slot].cancel_requested = true;
        _.tasks[slot].phase = A2aTaskPhase.canceled;
        if (now_ms >= _.tasks[slot].updated_ms) {
            _.tasks[slot].updated_ms = now_ms;
        }
        _.tasks[slot].transition_count += 1;
        return true;
    }

    /// Appends a streaming artifact chunk to a task in `working` or `input_required` phase.
    ///
    /// Guards:
    /// - Rejects tasks not in `working` / `input_required`.
    /// - Rejects empty or oversized `artifact_id` / `mime_type`.
    /// - Enforces strictly monotonic increasing `seq` (`seq > last_seq`) on subsequent chunks.
    /// - Rejects further chunks once `is_last_chunk` has sealed (`finalized == true`) the artifact.
    fn append_artifact_chunk(
        *_,
        task_id: u64,
        artifact_id: []const u8,
        mime_type: []const u8,
        seq: u32,
        chunk_bytes: u64,
        is_last_chunk: bool,
    ): bool {
        if (artifact_id.len == 0 || artifact_id.len > A2A_ARTIFACT_ID_MAX_LEN) {
            return false;
        }
        if (mime_type.len == 0 || mime_type.len > A2A_ARTIFACT_MIME_MAX_LEN) {
            return false;
        }

        const slot = _.findTaskSlot(task_id) ?? return false;
        const phase = _.tasks[slot].phase;
        if (phase != A2aTaskPhase.working && phase != A2aTaskPhase.input_required) {
            return false;
        }

        let free_art: usize? = null;
        let a: usize = 0;
        while (a < A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK) : (a += 1) {
            if (_.artifacts[slot][a].active) {
                const cur_len = _.artifacts[slot][a].artifact_id_len;
                if (cur_len == artifact_id.len && std.mem.eql(u8, _.artifacts[slot][a].artifact_id[0..cur_len], artifact_id)) {
                    if (_.artifacts[slot][a].finalized) {
                        return false;
                    }
                    if (seq <= _.artifacts[slot][a].last_seq) {
                        return false;
                    }
                    if (chunk_bytes > 0xFFFFFFFFFFFFFFFF - _.artifacts[slot][a].total_bytes) {
                        return false;
                    }
                    _.artifacts[slot][a].chunks_received += 1;
                    _.artifacts[slot][a].total_bytes += chunk_bytes;
                    _.artifacts[slot][a].last_seq = seq;
                    if (is_last_chunk) {
                        _.artifacts[slot][a].finalized = true;
                    }
                    return true;
                }
            } else if (free_art == null) {
                free_art = a;
            }
        }

        const art_slot = free_art ?? return false;
        _.artifacts[slot][art_slot].active = true;
        _.artifacts[slot][art_slot].artifact_id_len = artifact_id.len;
        _.artifacts[slot][art_slot].mime_type_len = mime_type.len;
        _.artifacts[slot][art_slot].chunks_received = 1;
        _.artifacts[slot][art_slot].total_bytes = chunk_bytes;
        _.artifacts[slot][art_slot].finalized = is_last_chunk;
        _.artifacts[slot][art_slot].last_seq = seq;

        let i: usize = 0;
        while (i < artifact_id.len) : (i += 1) {
            _.artifacts[slot][art_slot].artifact_id[i] = artifact_id[i];
        }
        let j: usize = 0;
        while (j < mime_type.len) : (j += 1) {
            _.artifacts[slot][art_slot].mime_type[j] = mime_type[j];
        }
        _.tasks[slot].artifact_count += 1;
        return true;
    }

    /// Looks up a read-only copy of an artifact metadata entry for `(task_id, artifact_id)`.
    fn get_artifact(task_id: u64, artifact_id: []const u8): A2aArtifactMeta? {
        if (artifact_id.len == 0 || artifact_id.len > A2A_ARTIFACT_ID_MAX_LEN) {
            return null;
        }
        const slot = _.findTaskSlot(task_id) ?? return null;
        let a: usize = 0;
        while (a < A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK) : (a += 1) {
            if (_.artifacts[slot][a].active) {
                const cur_len = _.artifacts[slot][a].artifact_id_len;
                if (cur_len == artifact_id.len && std.mem.eql(u8, _.artifacts[slot][a].artifact_id[0..cur_len], artifact_id)) {
                    return _.artifacts[slot][a];
                }
            }
        }
        return null;
    }

    /// Reclaims terminal task slots whose `updated_ms < cutoff_ms` for immediate reuse.
    /// Returns the number of reclaimed task slots.
    fn prune_terminal_before(*_, cutoff_ms: u64): usize {
        let pruned: usize = 0;
        let i: usize = 0;
        while (i < A2A_LIFECYCLE_MAX_TASKS) : (i += 1) {
            if (_.tasks[i].active && _.tasks[i].phase.isTerminal() && _.tasks[i].updated_ms < cutoff_ms) {
                _.tasks[i] = A2aTaskRecord{};
                let a: usize = 0;
                while (a < A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK) : (a += 1) {
                    _.artifacts[i][a] = A2aArtifactMeta{};
                }
                pruned += 1;
            }
        }
        return pruned;
    }

    /// Returns a read-only copy of the `A2aTaskRecord` for `task_id`, or `null` if not found.
    fn get_task(task_id: u64): A2aTaskRecord? {
        const slot = _.findTaskSlot(task_id) ?? return null;
        return _.tasks[slot];
    }

    /// Computes a consistent snapshot of task phase counts and total tracked artifacts.
    fn stats(): A2aLifecycleStats {
        let s = A2aLifecycleStats{};
        let i: usize = 0;
        while (i < A2A_LIFECYCLE_MAX_TASKS) : (i += 1) {
            if (!_.tasks[i].active) {
                continue;
            }
            s.total_tasks += 1;
            s.total_artifacts += _.tasks[i].artifact_count;
            switch (_.tasks[i].phase) {
                A2aTaskPhase.completed => {
                    s.completed_tasks += 1;
                },
                A2aTaskPhase.failed => {
                    s.failed_tasks += 1;
                },
                A2aTaskPhase.canceled => {
                    s.canceled_tasks += 1;
                },
                A2aTaskPhase.rejected => {
                    s.rejected_tasks += 1;
                },
                else => {
                    s.active_tasks += 1;
                },
            }
        }
        return s;
    }
}
