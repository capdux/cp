// Origin: Pure-CP Stdlib (AI Agent Bounded Priority Mailbox with Backpressure & Key Coalescing) | Created: 2026-11-22
// Zero-heap-allocation BEAM-inspired bounded priority mailbox for high-concurrency AI Agents,
// supporting 4-tier priority FIFO dequeue, RejectNew / DropLowestOldest / CoalesceByKey backpressure.

/// 4-tier message priority for Agent mailbox scheduling.
pub enum MailPriority {
    Low,
    Normal,
    High,
    System,
}

fn priorityRank(p: MailPriority): u8 {
    return switch (p) {
        MailPriority.Low => 0,
        MailPriority.Normal => 1,
        MailPriority.High => 2,
        MailPriority.System => 3,
    };
}

/// Backpressure strategy applied when sending to a full mailbox (or matching an existing key).
pub enum BackpressurePolicy {
    RejectNew,
    DropLowestOldest,
    CoalesceByKey,
}

/// Fixed-size envelope stored in a bounded `AgentMailbox` slot.
pub struct MailEnvelope {
    msg_id: u64 = 0;
    sender_id: u32 = 0;
    priority: MailPriority = MailPriority.Normal;
    coalesce_key: u64 = 0;
    enqueue_seq: u64 = 0;
    payload_tag: u32 = 0;
    payload_u64: u64 = 0;
    occupied: bool = false;
}

/// Result returned by `AgentMailbox.send`.
pub enum SendOutcome {
    Enqueued,
    Coalesced,
    EvictedAndEnqueued,
    RejectedFull,
}

/// Operational telemetry snapshot returned by `AgentMailbox.stats`.
pub struct MailboxStats {
    len: usize = 0;
    capacity: usize = 0;
    enqueued_total: u64 = 0;
    dequeued_total: u64 = 0;
    coalesced_total: u64 = 0;
    dropped_total: u64 = 0;
    rejected_total: u64 = 0;
}

/// Fixed-capacity, zero-heap-allocation bounded priority mailbox with backpressure & key coalescing.
pub struct AgentMailbox {
    slots: []MailEnvelope;
    policy: BackpressurePolicy = BackpressurePolicy.RejectNew;
    count: usize = 0;
    next_msg_id: u64 = 1;
    next_seq: u64 = 1;
    enqueued_total: u64 = 0;
    dequeued_total: u64 = 0;
    coalesced_total: u64 = 0;
    dropped_total: u64 = 0;
    rejected_total: u64 = 0;

    /// Initializes a bounded mailbox backed by caller-owned `slots` and `policy`.
    static fn init(slots: *[]MailEnvelope, policy: BackpressurePolicy): AgentMailbox {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = MailEnvelope{};
        }
        return AgentMailbox{
            slots: slots,
            policy: policy,
            count: 0,
            next_msg_id: 1,
            next_seq: 1,
            enqueued_total: 0,
            dequeued_total: 0,
            coalesced_total: 0,
            dropped_total: 0,
            rejected_total: 0,
        };
    }

    /// Enqueues a message under the configured `BackpressurePolicy`:
    /// - Under `CoalesceByKey` with `coalesce_key != 0`, updates an existing matching entry in place
    ///   (preserving queue order and upgrading priority if higher) without consuming a new slot.
    /// - Under `DropLowestOldest` when full, evicts the oldest lowest-priority message ONLY if its
    ///   priority is `<= priority` (never evicts strictly higher-priority messages such as `System`).
    /// - Under `RejectNew` (or when no eligible victim exists), rejects with `RejectedFull`.
    fn send(
        *_,
        sender_id: u32,
        priority: MailPriority,
        coalesce_key: u64,
        payload_tag: u32,
        payload_u64: u64,
    ): SendOutcome {
        if (_.slots.len == 0) {
            _.rejected_total += 1;
            return SendOutcome.RejectedFull;
        }

        // 1. Key coalescing check (works even when mailbox is full).
        if (_.policy == BackpressurePolicy.CoalesceByKey && coalesce_key != 0) {
            let c: usize = 0;
            while (c < _.slots.len) : (c += 1) {
                if (_.slots[c].occupied && _.slots[c].coalesce_key == coalesce_key) {
                    const mid = _.next_msg_id;
                    _.next_msg_id += 1;
                    _.slots[c].msg_id = mid;
                    _.slots[c].sender_id = sender_id;
                    if (priorityRank(priority) > priorityRank(_.slots[c].priority)) {
                        _.slots[c].priority = priority;
                    }
                    _.slots[c].payload_tag = payload_tag;
                    _.slots[c].payload_u64 = payload_u64;
                    _.coalesced_total += 1;
                    return SendOutcome.Coalesced;
                }
            }
        }

        // 2. Find a free slot if capacity permits.
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                const mid = _.next_msg_id;
                const seq = _.next_seq;
                _.next_msg_id += 1;
                _.next_seq += 1;

                _.slots[i] = MailEnvelope{
                    msg_id: mid,
                    sender_id: sender_id,
                    priority: priority,
                    coalesce_key: coalesce_key,
                    enqueue_seq: seq,
                    payload_tag: payload_tag,
                    payload_u64: payload_u64,
                    occupied: true,
                };
                _.count += 1;
                _.enqueued_total += 1;
                return SendOutcome.Enqueued;
            }
        }

        // 3. Mailbox is full: apply backpressure policy.
        if (_.policy == BackpressurePolicy.DropLowestOldest) {
            let victim_idx: usize? = null;
            let victim_rank: u8 = 255;
            let victim_seq: u64 = 0;
            const incoming_rank = priorityRank(priority);

            let v: usize = 0;
            while (v < _.slots.len) : (v += 1) {
                if (!_.slots[v].occupied) continue;
                const r = priorityRank(_.slots[v].priority);
                // Never evict a strictly higher-priority message.
                if (r > incoming_rank) continue;

                const is_worse = (victim_idx == null) ||
                    (r < victim_rank) ||
                    (r == victim_rank && _.slots[v].enqueue_seq < victim_seq);
                if (is_worse) {
                    victim_idx = v;
                    victim_rank = r;
                    victim_seq = _.slots[v].enqueue_seq;
                }
            }

            if (slot := victim_idx) {
                const mid = _.next_msg_id;
                const seq = _.next_seq;
                _.next_msg_id += 1;
                _.next_seq += 1;

                _.slots[slot] = MailEnvelope{
                    msg_id: mid,
                    sender_id: sender_id,
                    priority: priority,
                    coalesce_key: coalesce_key,
                    enqueue_seq: seq,
                    payload_tag: payload_tag,
                    payload_u64: payload_u64,
                    occupied: true,
                };
                _.dropped_total += 1;
                _.enqueued_total += 1;
                return SendOutcome.EvictedAndEnqueued;
            }
        }

        _.rejected_total += 1;
        return SendOutcome.RejectedFull;
    }

    fn findNextIndex(): usize? {
        let best_idx: usize? = null;
        let best_rank: u8 = 0;
        let best_seq: u64 = 0;

        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            const r = priorityRank(_.slots[i].priority);
            const seq = _.slots[i].enqueue_seq;
            const better = (best_idx == null) ||
                (r > best_rank) ||
                (r == best_rank && seq < best_seq);
            if (better) {
                best_idx = i;
                best_rank = r;
                best_seq = seq;
            }
        }
        return best_idx;
    }

    /// Returns a copy of the highest-priority oldest message without removing it, or `null` if empty.
    fn peekNext(): MailEnvelope? {
        if (idx := _.findNextIndex()) {
            return _.slots[idx];
        }
        return null;
    }

    /// Dequeues the highest-priority message (FIFO within the same priority), or `null` if empty.
    fn recvNext(*_): MailEnvelope? {
        if (idx := _.findNextIndex()) {
            const msg = _.slots[idx];
            _.slots[idx] = MailEnvelope{};
            if (_.count > 0) {
                _.count -= 1;
            }
            _.dequeued_total += 1;
            return msg;
        }
        return null;
    }

    /// Returns the number of currently queued messages.
    fn len(): usize {
        return _.count;
    }

    /// Returns `true` when the mailbox holds no messages.
    fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Returns a read-only telemetry snapshot of mailbox occupancy and counters.
    fn stats(): MailboxStats {
        return MailboxStats{
            len: _.count,
            capacity: _.slots.len,
            enqueued_total: _.enqueued_total,
            dequeued_total: _.dequeued_total,
            coalesced_total: _.coalesced_total,
            dropped_total: _.dropped_total,
            rejected_total: _.rejected_total,
        };
    }

    /// Removes all queued messages while retaining monotonic IDs and cumulative statistics.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = MailEnvelope{};
        }
        _.count = 0;
    }
}
