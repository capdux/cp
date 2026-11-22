// Origin: Pure-CP Stdlib (A2A Event Stream Replay Ring & Last-Event-ID Resume Cursor) | Created: 2026-11-22
// Zero-heap-allocation Agent-to-Agent (A2A) task event stream ring buffer with monotonic event IDs,
// task-scoped replay filtering, and Last-Event-ID gap detection for reconnect snapshot fallback.

/// Semantic category of an A2A streaming task event.
pub enum A2aEventKind {
    StatusUpdate,
    ArtifactAppend,
    ArtifactComplete,
    InputRequired,
    Heartbeat,
    Error,
}

/// Fixed-size metadata record for a single A2A task stream event stored in the replay ring.
pub struct A2aStreamEvent {
    event_id: u64 = 0;
    task_id: u64 = 0;
    kind: A2aEventKind = A2aEventKind.StatusUpdate;
    timestamp_ms: u64 = 0;
    is_final: bool = false;
    payload_hash: u64 = 0;
    payload_len: u32 = 0;
}

/// Outcome of a `replayAfter` cursor query against `A2aEventStreamRing`.
pub struct ReplayResult {
    copied: usize = 0;
    total_available: usize = 0;
    gap_detected: bool = false;
    latest_event_id: u64 = 0;
}

/// Fixed-capacity, zero-heap-allocation ring buffer for A2A event replay and Last-Event-ID resume.
pub struct A2aEventStreamRing {
    slots: []A2aStreamEvent;
    head: usize = 0;
    len: usize = 0;
    next_event_id: u64 = 1;
    total_published: u64 = 0;
    total_overwritten: u64 = 0;

    /// Initializes a replay ring backed by caller-owned fixed `slots` buffer.
    static fn init(slots: *[]A2aStreamEvent): A2aEventStreamRing {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = A2aStreamEvent{};
        }
        return A2aEventStreamRing{
            slots: slots,
            head: 0,
            len: 0,
            next_event_id: 1,
            total_published: 0,
            total_overwritten: 0,
        };
    }

    /// Appends a new event with a strictly monotonic `event_id` (starting at 1), overwriting the
    /// oldest buffered slot when full. Returns the assigned `event_id`.
    fn publish(
        *_,
        task_id: u64,
        kind: A2aEventKind,
        timestamp_ms: u64,
        is_final: bool,
        payload_hash: u64,
        payload_len: u32,
    ): u64 {
        const eid = _.next_event_id;
        _.next_event_id += 1;
        _.total_published += 1;

        if (_.slots.len == 0) {
            _.total_overwritten += 1;
            return eid;
        }

        const ev = A2aStreamEvent{
            event_id: eid,
            task_id: task_id,
            kind: kind,
            timestamp_ms: timestamp_ms,
            is_final: is_final,
            payload_hash: payload_hash,
            payload_len: payload_len,
        };

        if (_.len < _.slots.len) {
            const idx = (_.head + _.len) % _.slots.len;
            _.slots[idx] = ev;
            _.len += 1;
        } else {
            _.slots[_.head] = ev;
            _.head = (_.head + 1) % _.slots.len;
            _.total_overwritten += 1;
        }
        return eid;
    }

    /// Returns the newest `event_id` currently retained in the ring, or `0` if empty.
    fn latestEventId(): u64 {
        if (_.len == 0 || _.slots.len == 0) return 0;
        const tail_idx = (_.head + _.len - 1) % _.slots.len;
        return _.slots[tail_idx].event_id;
    }

    /// Returns the oldest `event_id` currently retained in the ring, or `0` if empty.
    fn oldestEventId(): u64 {
        if (_.len == 0 || _.slots.len == 0) return 0;
        return _.slots[_.head].event_id;
    }

    /// Returns the number of events currently retained in the ring.
    fn count(): usize {
        return _.len;
    }

    /// Returns the fixed slot capacity of the backing buffer.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Clears all buffered events without resetting the monotonic `event_id` generator.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = A2aStreamEvent{};
        }
        _.head = 0;
        _.len = 0;
    }

    /// Replays buffered events strictly after `last_event_id` into `out_events` (truncation-safe).
    /// Pass `task_filter == 0` to replay all tasks, or a non-zero `task_id` to filter to one task.
    /// Sets `gap_detected = true` when events after `last_event_id` were overwritten or when
    /// `last_event_id` is ahead of `latest_event_id`, signaling the client to perform a full sync.
    fn replayAfter(
        last_event_id: u64,
        task_filter: u64,
        out_events: *[]A2aStreamEvent,
    ): ReplayResult {
        if (_.len == 0 || _.slots.len == 0) {
            const latest_published: u64 = if (_.next_event_id > 1) _.next_event_id - 1 else 0;
            const empty_gap = (_.total_overwritten > 0 && last_event_id < latest_published) ||
                (last_event_id > latest_published);
            return ReplayResult{
                copied: 0,
                total_available: 0,
                gap_detected: empty_gap,
                latest_event_id: 0,
            };
        }

        const oldest_id = _.oldestEventId();
        const latest_id = _.latestEventId();

        let gap = false;
        if (last_event_id > latest_id) {
            gap = true;
        } else if (oldest_id > 1 && last_event_id < oldest_id - 1) {
            gap = true;
        }

        let copied: usize = 0;
        let total_available: usize = 0;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const ev = _.slots[(_.head + i) % _.slots.len];
            if (ev.event_id <= last_event_id) continue;
            if (task_filter != 0 && ev.task_id != task_filter) continue;

            total_available += 1;
            if (copied < out_events.len) {
                out_events[copied] = ev;
                copied += 1;
            }
        }

        return ReplayResult{
            copied: copied,
            total_available: total_available,
            gap_detected: gap,
            latest_event_id: latest_id,
        };
    }
}
