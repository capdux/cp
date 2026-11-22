// Origin: Pure-CP Stdlib (Zero-Alloc MCP Long-Running Async Tool Task Manager & Polling State Machine) | Created: 2026-11-23
// Pure CP zero-heap-allocation MCP long-running asynchronous tool task lifecycle & cursor polling engine:
//   - Fixed 64-task table tracking `task_id`, `tool_name_hash`, 6-state lifecycle
//     (`queued | running | input_required | completed | failed | cancelled`), `progress_bps` (0..=10000),
//     `poll_interval_ms`, created/updated/TTL timestamps, error code & inline summary text
//   - Strict state transitions with irreversible terminal states, idempotent cancellation (`already_cancelled`),
//     and automatic TTL expiration reaping (`reapExpired`)
//   - Fixed 128-entry task event ring buffer with `since_seq` incremental cursor polling, optional `task_id`
//     filtering, overflow `lagged` detection & truncation-safe `has_more` pagination
//   - Zero-allocation RFC 8259 JSON-RPC task status formatter (`formatTaskStatusJson`) & global telemetry snapshot

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of concurrent async tool tasks tracked inline (zero dynamic heap).
pub const MCP_TASK_MAX_TASKS: usize = 64;

/// Capacity of the fixed incremental task event ring buffer.
pub const MCP_TASK_EVENT_RING_CAP: usize = 128;

/// Maximum inline byte length for task result/progress summaries.
pub const MCP_TASK_SUMMARY_MAX_LEN: usize = 128;

/// Default suggested client polling interval in milliseconds.
pub const MCP_TASK_DEFAULT_POLL_MS: u32 = 1000;

/// Maximum progress in basis points (10000 = 100.00%).
pub const MCP_TASK_MAX_PROGRESS_BPS: u16 = 10000;

/// Six-state lifecycle for long-running MCP asynchronous tool tasks.
pub enum McpAsyncTaskState {
    queued,
    running,
    input_required,
    completed,
    failed,
    cancelled,
}

/// Returns `true` if `state` is an irreversible terminal state (`completed | failed | cancelled`).
pub fn mcpTaskStateIsTerminal(state: McpAsyncTaskState): bool {
    return match (state) {
        McpAsyncTaskState.completed => true,
        McpAsyncTaskState.failed => true,
        McpAsyncTaskState.cancelled => true,
        _ => false,
    };
}

/// Returns the canonical wire string for a `McpAsyncTaskState`.
pub fn mcpTaskStateName(state: McpAsyncTaskState): []const u8 {
    return match (state) {
        McpAsyncTaskState.queued => "queued",
        McpAsyncTaskState.running => "running",
        McpAsyncTaskState.input_required => "input_required",
        McpAsyncTaskState.completed => "completed",
        McpAsyncTaskState.failed => "failed",
        McpAsyncTaskState.cancelled => "cancelled",
    };
}

/// Operation result status for async task state mutations.
pub enum McpAsyncTaskOpStatus {
    ok,
    already_cancelled,
    invalid_arg,
    duplicate_task_id,
    capacity_full,
    not_found,
    invalid_transition,
}

/// Event kinds recorded into the incremental task event ring.
pub enum McpAsyncTaskEventKind {
    submitted,
    started,
    progress,
    input_required,
    resumed,
    completed,
    failed,
    cancelled,
    expired,
}

/// Inline long-running MCP async tool task descriptor.
pub struct McpAsyncTask {
    occupied: bool = false,
    task_id: u64 = 0,
    tool_name_hash: u64 = 0,
    state: McpAsyncTaskState = McpAsyncTaskState.queued,
    progress_bps: u16 = 0,
    poll_interval_ms: u32 = MCP_TASK_DEFAULT_POLL_MS,
    created_ms: u64 = 0,
    updated_ms: u64 = 0,
    expires_at_ms: u64 = 0,
    error_code: i32 = 0,
    summary_buf: [MCP_TASK_SUMMARY_MAX_LEN]u8 = [0; MCP_TASK_SUMMARY_MAX_LEN],
    summary_len: u16 = 0,

    /// Returns the stored inline summary slice.
    pub fn summarySlice(): []const u8 {
        return _.summary_buf[0..(_.summary_len as usize)];
    }
}

/// Event entry stored in the bounded task event ring buffer.
pub struct McpAsyncTaskEvent {
    seq: u64 = 0,
    task_id: u64 = 0,
    kind: McpAsyncTaskEventKind = McpAsyncTaskEventKind.submitted,
    state: McpAsyncTaskState = McpAsyncTaskState.queued,
    progress_bps: u16 = 0,
    timestamp_ms: u64 = 0,
    summary_buf: [MCP_TASK_SUMMARY_MAX_LEN]u8 = [0; MCP_TASK_SUMMARY_MAX_LEN],
    summary_len: u16 = 0,

    /// Returns the event summary slice.
    pub fn summarySlice(): []const u8 {
        return _.summary_buf[0..(_.summary_len as usize)];
    }
}

/// Result returned by `McpAsyncTaskManager.pollEvents`.
pub struct McpAsyncTaskPollResult {
    count: usize = 0,
    next_seq: u64 = 0,
    latest_seq: u64 = 0,
    lagged: bool = false,
    has_more: bool = false,
}

/// Global telemetry snapshot for `McpAsyncTaskManager`.
pub struct McpAsyncTaskStats {
    active_slots: usize = 0,
    capacity: usize = MCP_TASK_MAX_TASKS,
    queued_count: usize = 0,
    running_count: usize = 0,
    input_required_count: usize = 0,
    completed_count: usize = 0,
    failed_count: usize = 0,
    cancelled_count: usize = 0,
    event_ring_count: usize = 0,
    latest_event_seq: u64 = 0,
    total_submitted: u64 = 0,
    total_completed: u64 = 0,
    total_failed: u64 = 0,
    total_cancelled: u64 = 0,
    total_reaped_expired: u64 = 0,
}

// ============================================================================
// 2. Internal Byte & JSON Formatting Helpers (Zero Allocation)
// ============================================================================

fn copyTruncated(dst: *[MCP_TASK_SUMMARY_MAX_LEN]u8, src: []const u8): u16 {
    const n = if (src.len < MCP_TASK_SUMMARY_MAX_LEN) src.len else MCP_TASK_SUMMARY_MAX_LEN;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n as u16;
}

fn writeLiteral(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn writeU64Dec(out: *[]u8, pos: *usize, val: u64): bool {
    let digits: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = ('0' as u8) + ((v % 10) as u8);
            v /= 10;
            len += 1;
        }
    }
    if (pos.* + len > out.len) return false;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        out[pos.* + i] = digits[len - 1 - i];
    }
    pos.* += len;
    return true;
}

fn writeI32Dec(out: *[]u8, pos: *usize, val: i32): bool {
    if (val < 0) {
        if (!writeLiteral(out, pos, "-")) return false;
        const mag: u64 = (-(val as i64)) as u64;
        return writeU64Dec(out, pos, mag);
    }
    return writeU64Dec(out, pos, val as u64);
}

fn hexNibble(n: u8): u8 {
    return if (n < 10) ('0' as u8) + n else ('a' as u8) + (n - 10);
}

fn writeJsonEscapedString(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (!writeLiteral(out, pos, "\"")) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '"') {
            if (!writeLiteral(out, pos, "\\\"")) return false;
        } else if (ch == '\\') {
            if (!writeLiteral(out, pos, "\\\\")) return false;
        } else if (ch == '\n') {
            if (!writeLiteral(out, pos, "\\n")) return false;
        } else if (ch == '\r') {
            if (!writeLiteral(out, pos, "\\r")) return false;
        } else if (ch == '\t') {
            if (!writeLiteral(out, pos, "\\t")) return false;
        } else if (ch < 0x20) {
            if (pos.* + 6 > out.len) return false;
            out[pos.*] = '\\';
            out[pos.* + 1] = 'u';
            out[pos.* + 2] = '0';
            out[pos.* + 3] = '0';
            out[pos.* + 4] = hexNibble((ch >> 4) & 0x0F);
            out[pos.* + 5] = hexNibble(ch & 0x0F);
            pos.* += 6;
        } else {
            if (pos.* + 1 > out.len) return false;
            out[pos.*] = ch;
            pos.* += 1;
        }
    }
    return writeLiteral(out, pos, "\"");
}

/// Zero-allocation RFC 8259 JSON-RPC task status view formatter.
/// Writes a compact JSON object into `out_buf` and returns bytes written, or `null` if buffer is too small.
pub fn formatTaskStatusJson(task: McpAsyncTask, out_buf: *[]u8): usize? {
    let pos: usize = 0;
    if (!writeLiteral(out_buf, *pos, "{\"taskId\":")) return null;
    if (!writeU64Dec(out_buf, *pos, task.task_id)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"status\":")) return null;
    if (!writeJsonEscapedString(out_buf, *pos, mcpTaskStateName(task.state))) return null;
    if (!writeLiteral(out_buf, *pos, ",\"progressBps\":")) return null;
    if (!writeU64Dec(out_buf, *pos, task.progress_bps as u64)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"pollIntervalMs\":")) return null;
    if (!writeU64Dec(out_buf, *pos, task.poll_interval_ms as u64)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"createdMs\":")) return null;
    if (!writeU64Dec(out_buf, *pos, task.created_ms)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"updatedMs\":")) return null;
    if (!writeU64Dec(out_buf, *pos, task.updated_ms)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"expiresAtMs\":")) return null;
    if (!writeU64Dec(out_buf, *pos, task.expires_at_ms)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"errorCode\":")) return null;
    if (!writeI32Dec(out_buf, *pos, task.error_code)) return null;
    if (!writeLiteral(out_buf, *pos, ",\"summary\":")) return null;
    if (!writeJsonEscapedString(out_buf, *pos, task.summarySlice())) return null;
    if (!writeLiteral(out_buf, *pos, "}")) return null;
    return pos;
}

// ============================================================================
// 3. Core Zero-Alloc MCP Async Tool Task Manager
// ============================================================================

/// Zero-heap-allocation MCP long-running async tool task handle & polling state machine.
pub struct McpAsyncTaskManager {
    tasks: [MCP_TASK_MAX_TASKS]McpAsyncTask = [McpAsyncTask{}; MCP_TASK_MAX_TASKS],
    active_slots: usize = 0,
    events: [MCP_TASK_EVENT_RING_CAP]McpAsyncTaskEvent = [McpAsyncTaskEvent{}; MCP_TASK_EVENT_RING_CAP],
    event_head: usize = 0,
    event_count: usize = 0,
    next_event_seq: u64 = 1,
    total_submitted: u64 = 0,
    total_completed: u64 = 0,
    total_failed: u64 = 0,
    total_cancelled: u64 = 0,
    total_reaped_expired: u64 = 0,

    /// Creates a new empty `McpAsyncTaskManager`.
    pub fn init(): McpAsyncTaskManager {
        return McpAsyncTaskManager{};
    }

    // ---- Internal Helpers ----

    fn findSlot(task_id: u64): usize? {
        if (task_id == 0) return null;
        let i: usize = 0;
        while (i < MCP_TASK_MAX_TASKS) : (i += 1) {
            if (_.tasks[i].occupied && _.tasks[i].task_id == task_id) return i;
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < MCP_TASK_MAX_TASKS) : (i += 1) {
            if (!_.tasks[i].occupied) return i;
        }
        return null;
    }

    fn pushEvent(
        *_,
        task_id: u64,
        kind: McpAsyncTaskEventKind,
        state: McpAsyncTaskState,
        progress_bps: u16,
        timestamp_ms: u64,
        summary: []const u8,
    ) {
        const seq = _.next_event_seq;
        _.next_event_seq += 1;

        let ev = McpAsyncTaskEvent{
            seq: seq,
            task_id: task_id,
            kind: kind,
            state: state,
            progress_bps: progress_bps,
            timestamp_ms: timestamp_ms,
        };
        ev.summary_len = copyTruncated(*ev.summary_buf, summary);

        let write_idx: usize = 0;
        if (_.event_count < MCP_TASK_EVENT_RING_CAP) {
            write_idx = (_.event_head + _.event_count) % MCP_TASK_EVENT_RING_CAP;
            _.event_count += 1;
        } else {
            write_idx = _.event_head;
            _.event_head = (_.event_head + 1) % MCP_TASK_EVENT_RING_CAP;
        }
        _.events[write_idx] = ev;
    }

    // ---- Lifecycle State Transitions ----

    /// Submits a new long-running tool task in `queued` state.
    /// `ttl_ms == 0` disables automatic expiration; `poll_interval_ms == 0` uses `MCP_TASK_DEFAULT_POLL_MS`.
    pub fn submitTask(
        *_,
        task_id: u64,
        tool_name_hash: u64,
        poll_interval_ms: u32,
        ttl_ms: u64,
        now_ms: u64,
    ): McpAsyncTaskOpStatus {
        if (task_id == 0) return McpAsyncTaskOpStatus.invalid_arg;
        if (_.findSlot(task_id) != null) return McpAsyncTaskOpStatus.duplicate_task_id;
        const slot = _.findFreeSlot() ?? return McpAsyncTaskOpStatus.capacity_full;

        const poll_ms = if (poll_interval_ms == 0) MCP_TASK_DEFAULT_POLL_MS else poll_interval_ms;
        const expires_at = if (ttl_ms == 0) 0 else now_ms +| ttl_ms;

        _.tasks[slot] = McpAsyncTask{
            occupied: true,
            task_id: task_id,
            tool_name_hash: tool_name_hash,
            state: McpAsyncTaskState.queued,
            progress_bps: 0,
            poll_interval_ms: poll_ms,
            created_ms: now_ms,
            updated_ms: now_ms,
            expires_at_ms: expires_at,
        };
        _.active_slots += 1;
        _.total_submitted += 1;

        _.pushEvent(task_id, McpAsyncTaskEventKind.submitted, McpAsyncTaskState.queued, 0, now_ms, "");
        return McpAsyncTaskOpStatus.ok;
    }

    /// Transitions a task from `queued` -> `running`.
    pub fn startTask(*_, task_id: u64, now_ms: u64): McpAsyncTaskOpStatus {
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        if (_.tasks[idx].state != McpAsyncTaskState.queued) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].state = McpAsyncTaskState.running;
        _.tasks[idx].updated_ms = now_ms;
        _.pushEvent(task_id, McpAsyncTaskEventKind.started, McpAsyncTaskState.running, _.tasks[idx].progress_bps, now_ms, "");
        return McpAsyncTaskOpStatus.ok;
    }

    /// Transitions a task from `running` -> `input_required` (e.g., waiting for user elicitation/approval).
    pub fn requireInput(*_, task_id: u64, prompt_summary: []const u8, now_ms: u64): McpAsyncTaskOpStatus {
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        if (_.tasks[idx].state != McpAsyncTaskState.running) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].state = McpAsyncTaskState.input_required;
        _.tasks[idx].updated_ms = now_ms;
        _.tasks[idx].summary_len = copyTruncated(*_.tasks[idx].summary_buf, prompt_summary);
        _.pushEvent(
            task_id,
            McpAsyncTaskEventKind.input_required,
            McpAsyncTaskState.input_required,
            _.tasks[idx].progress_bps,
            now_ms,
            prompt_summary,
        );
        return McpAsyncTaskOpStatus.ok;
    }

    /// Transitions a task from `input_required` -> `running` after client supplies input.
    pub fn resumeFromInput(*_, task_id: u64, note: []const u8, now_ms: u64): McpAsyncTaskOpStatus {
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        if (_.tasks[idx].state != McpAsyncTaskState.input_required) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].state = McpAsyncTaskState.running;
        _.tasks[idx].updated_ms = now_ms;
        _.tasks[idx].summary_len = copyTruncated(*_.tasks[idx].summary_buf, note);
        _.pushEvent(
            task_id,
            McpAsyncTaskEventKind.resumed,
            McpAsyncTaskState.running,
            _.tasks[idx].progress_bps,
            now_ms,
            note,
        );
        return McpAsyncTaskOpStatus.ok;
    }

    /// Updates progress (`0..=10000` bps), optional `poll_interval_ms` (`0` keeps current), and summary
    /// for a `running` task.
    pub fn updateProgress(
        *_,
        task_id: u64,
        progress_bps: u16,
        poll_interval_ms: u32,
        message: []const u8,
        now_ms: u64,
    ): McpAsyncTaskOpStatus {
        if (progress_bps > MCP_TASK_MAX_PROGRESS_BPS) return McpAsyncTaskOpStatus.invalid_arg;
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        if (_.tasks[idx].state != McpAsyncTaskState.running) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].progress_bps = progress_bps;
        if (poll_interval_ms > 0) {
            _.tasks[idx].poll_interval_ms = poll_interval_ms;
        }
        _.tasks[idx].updated_ms = now_ms;
        _.tasks[idx].summary_len = copyTruncated(*_.tasks[idx].summary_buf, message);

        _.pushEvent(
            task_id,
            McpAsyncTaskEventKind.progress,
            McpAsyncTaskState.running,
            progress_bps,
            now_ms,
            message,
        );
        return McpAsyncTaskOpStatus.ok;
    }

    /// Transitions a `running` or `input_required` task to terminal `completed` state (`progress_bps = 10000`).
    pub fn completeTask(*_, task_id: u64, result_summary: []const u8, now_ms: u64): McpAsyncTaskOpStatus {
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        const st = _.tasks[idx].state;
        if (st != McpAsyncTaskState.running && st != McpAsyncTaskState.input_required) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].state = McpAsyncTaskState.completed;
        _.tasks[idx].progress_bps = MCP_TASK_MAX_PROGRESS_BPS;
        _.tasks[idx].updated_ms = now_ms;
        _.tasks[idx].error_code = 0;
        _.tasks[idx].summary_len = copyTruncated(*_.tasks[idx].summary_buf, result_summary);
        _.total_completed += 1;

        _.pushEvent(
            task_id,
            McpAsyncTaskEventKind.completed,
            McpAsyncTaskState.completed,
            MCP_TASK_MAX_PROGRESS_BPS,
            now_ms,
            result_summary,
        );
        return McpAsyncTaskOpStatus.ok;
    }

    /// Transitions any non-terminal task (`queued | running | input_required`) to terminal `failed` state.
    pub fn failTask(
        *_,
        task_id: u64,
        error_code: i32,
        reason: []const u8,
        now_ms: u64,
    ): McpAsyncTaskOpStatus {
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        if (mcpTaskStateIsTerminal(_.tasks[idx].state)) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].state = McpAsyncTaskState.failed;
        _.tasks[idx].error_code = error_code;
        _.tasks[idx].updated_ms = now_ms;
        _.tasks[idx].summary_len = copyTruncated(*_.tasks[idx].summary_buf, reason);
        _.total_failed += 1;

        _.pushEvent(
            task_id,
            McpAsyncTaskEventKind.failed,
            McpAsyncTaskState.failed,
            _.tasks[idx].progress_bps,
            now_ms,
            reason,
        );
        return McpAsyncTaskOpStatus.ok;
    }

    /// Idempotently cancels a task:
    ///   - Non-terminal (`queued | running | input_required`) -> transitions to `cancelled` and returns `ok`.
    ///   - Already `cancelled` -> no-op, returns `already_cancelled`.
    ///   - `completed` or `failed` -> terminal states are irreversible, returns `invalid_transition`.
    pub fn cancelTask(*_, task_id: u64, reason: []const u8, now_ms: u64): McpAsyncTaskOpStatus {
        const idx = _.findSlot(task_id) ?? return McpAsyncTaskOpStatus.not_found;
        const st = _.tasks[idx].state;
        if (st == McpAsyncTaskState.cancelled) {
            return McpAsyncTaskOpStatus.already_cancelled;
        }
        if (mcpTaskStateIsTerminal(st)) {
            return McpAsyncTaskOpStatus.invalid_transition;
        }

        _.tasks[idx].state = McpAsyncTaskState.cancelled;
        _.tasks[idx].updated_ms = now_ms;
        _.tasks[idx].summary_len = copyTruncated(*_.tasks[idx].summary_buf, reason);
        _.total_cancelled += 1;

        _.pushEvent(
            task_id,
            McpAsyncTaskEventKind.cancelled,
            McpAsyncTaskState.cancelled,
            _.tasks[idx].progress_bps,
            now_ms,
            reason,
        );
        return McpAsyncTaskOpStatus.ok;
    }

    /// Reaps all tasks whose TTL has expired (`expires_at_ms > 0 && now_ms >= expires_at_ms`),
    /// emitting an `expired` event for each reclaimed slot. Returns the number of reaped tasks.
    pub fn reapExpired(*_, now_ms: u64): usize {
        let reaped: usize = 0;
        let i: usize = 0;
        while (i < MCP_TASK_MAX_TASKS) : (i += 1) {
            if (!_.tasks[i].occupied) continue;
            const exp = _.tasks[i].expires_at_ms;
            if (exp > 0 && now_ms >= exp) {
                const tid = _.tasks[i].task_id;
                const st = _.tasks[i].state;
                const prog = _.tasks[i].progress_bps;
                _.tasks[i] = McpAsyncTask{};
                if (_.active_slots > 0) _.active_slots -= 1;
                _.total_reaped_expired += 1;
                reaped += 1;
                _.pushEvent(tid, McpAsyncTaskEventKind.expired, st, prog, now_ms, "ttl_expired");
            }
        }
        return reaped;
    }

    // ---- Cursor Polling & Query API ----

    /// Incrementally polls task lifecycle/progress events with `seq > since_seq`.
    /// Set `task_filter == 0` for all tasks, or a specific `task_id` to filter.
    /// Sets `lagged = true` when the ring buffer overwrote events newer than `since_seq`.
    pub fn pollEvents(
        since_seq: u64,
        task_filter: u64,
        out_events: *[]McpAsyncTaskEvent,
    ): McpAsyncTaskPollResult {
        const latest: u64 = if (_.next_event_seq > 1) _.next_event_seq - 1 else 0;
        if (_.event_count == 0) {
            return McpAsyncTaskPollResult{
                count: 0,
                next_seq: since_seq,
                latest_seq: latest,
                lagged: false,
                has_more: false,
            };
        }

        const oldest_seq = _.events[_.event_head].seq;
        const lagged = (oldest_seq > 1) && (since_seq + 1 < oldest_seq);

        let written: usize = 0;
        let last_seq: u64 = since_seq;
        let has_more: bool = false;

        let i: usize = 0;
        while (i < _.event_count) : (i += 1) {
            const ev = _.events[(_.event_head + i) % MCP_TASK_EVENT_RING_CAP];
            if (ev.seq <= since_seq) continue;
            if (task_filter != 0 && ev.task_id != task_filter) continue;

            if (written < out_events.len) {
                out_events[written] = ev;
                written += 1;
                last_seq = ev.seq;
            } else {
                has_more = true;
                break;
            }
        }

        return McpAsyncTaskPollResult{
            count: written,
            next_seq: last_seq,
            latest_seq: latest,
            lagged: lagged,
            has_more: has_more,
        };
    }

    /// Looks up a task handle by `task_id`.
    pub fn getTask(task_id: u64): McpAsyncTask? {
        const idx = _.findSlot(task_id) ?? return null;
        return _.tasks[idx];
    }

    /// Returns a global telemetry snapshot of all active slots and counters.
    pub fn stats(): McpAsyncTaskStats {
        let queued: usize = 0;
        let running: usize = 0;
        let input_req: usize = 0;
        let completed: usize = 0;
        let failed: usize = 0;
        let cancelled: usize = 0;

        let i: usize = 0;
        while (i < MCP_TASK_MAX_TASKS) : (i += 1) {
            if (!_.tasks[i].occupied) continue;
            match (_.tasks[i].state) {
                McpAsyncTaskState.queued => { queued += 1; },
                McpAsyncTaskState.running => { running += 1; },
                McpAsyncTaskState.input_required => { input_req += 1; },
                McpAsyncTaskState.completed => { completed += 1; },
                McpAsyncTaskState.failed => { failed += 1; },
                McpAsyncTaskState.cancelled => { cancelled += 1; },
            }
        }

        return McpAsyncTaskStats{
            active_slots: _.active_slots,
            capacity: MCP_TASK_MAX_TASKS,
            queued_count: queued,
            running_count: running,
            input_required_count: input_req,
            completed_count: completed,
            failed_count: failed,
            cancelled_count: cancelled,
            event_ring_count: _.event_count,
            latest_event_seq: if (_.next_event_seq > 1) _.next_event_seq - 1 else 0,
            total_submitted: _.total_submitted,
            total_completed: _.total_completed,
            total_failed: _.total_failed,
            total_cancelled: _.total_cancelled,
            total_reaped_expired: _.total_reaped_expired,
        };
    }
}
