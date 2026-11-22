// Source: cp/lib/pubsub/broker.cp (In-Process Wildcard Pub/Sub Broker with Backpressure & Retained Messages)
// Pure CP in-process wildcard pub/sub broker for AI Agent fan-out messaging with
// bounded backpressure queues (`DropOldest` / `DropNewest`), retained message replay,
// and O(1) subscription lookup.

const std = @import("std");
import { TopicStyle, TopicMatcher } from "./pubsub_topic.cp";

/// Error set returned by `PubSubBroker` operations.
pub err PubSubError {
    InvalidPattern,
    InvalidTopic,
    SubscriptionNotFound,
    OutOfMemory,
}

/// Backpressure policy applied when a subscriber's bounded queue reaches `max_queue`.
pub enum DropPolicy: u8 {
    DropOldest = 0,
    DropNewest = 1,
}

/// Owned message delivered to a subscriber via `PubSubBroker.poll`.
/// Callers must call `deinit()` when finished to release owned `topic` and `payload` buffers.
pub struct BrokerMessage {
    pub seq: u64 = 0;
    pub topic: string = "";
    pub payload: string = "";
    pub retained: bool = false;
    priv allocator: std.mem.Allocator? = null;

    /// Releases owned `topic` and `payload` string allocations, idempotently.
    pub fn deinit(*_) {
        if (alloc := _.allocator) {
            if (_.topic.len > 0) {
                alloc.free(_.topic);
            }
            if (_.payload.len > 0) {
                alloc.free(_.payload);
            }
            _.topic = "";
            _.payload = "";
            _.allocator = null;
        }
    }
}

/// Aggregate operational telemetry for `PubSubBroker`.
pub struct BrokerStats {
    pub published: u64 = 0;
    pub delivered: u64 = 0;
    pub dropped: u64 = 0;
    pub active_subscriptions: usize = 0;
    pub retained_topics: usize = 0;
}

/// Internal FIFO bounded queue for a single subscriber.
struct MsgQueue {
    items: std.ArrayList(BrokerMessage) = .empty,
    head: usize = 0,
    max_queue: usize = 0,
    drop_policy: DropPolicy = .DropOldest,

    fn count(): usize {
        return _.items.items.len - _.head;
    }

    fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = _.head;
        while (i < _.items.items.len) : (i += 1) {
            _.items.items[i].deinit();
        }
        _.items.deinit(alloc);
        _.head = 0;
    }

    fn popFront(*_): BrokerMessage? {
        if (_.head >= _.items.items.len) {
            return null;
        }
        const msg = _.items.items[_.head];
        _.head += 1;
        if (_.head == _.items.items.len) {
            _.items.items.len = 0;
            _.head = 0;
        } else if (_.head >= 32 && _.head * 2 >= _.items.items.len) {
            const rem = _.items.items.len - _.head;
            std.mem.copyForwards(BrokerMessage, _.items.items[0..rem], _.items.items[_.head..]);
            _.items.items.len = rem;
            _.head = 0;
        }
        return msg;
    }
}

struct Subscription {
    id: u64 = 0;
    pattern: string = "";
    queue: MsgQueue = MsgQueue{},

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.pattern.len > 0) {
            alloc.free(_.pattern);
            _.pattern = "";
        }
        _.queue.deinit(alloc);
    }
}

struct RetainedEntry {
    topic_buf: []u8,
    payload_buf: []u8,
    seq: u64 = 0,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.topic_buf.len > 0) {
            alloc.free(_.topic_buf);
        }
        if (_.payload_buf.len > 0) {
            alloc.free(_.payload_buf);
        }
    }
}

/// In-process wildcard pub/sub broker supporting NATS/MQTT topic matching,
/// bounded per-subscriber queues with configurable drop policies, and retained messages.
pub struct PubSubBroker {
    priv allocator: std.mem.Allocator;
    priv style: TopicStyle;
    priv next_sub_id: u64 = 1;
    priv next_seq: u64 = 1;
    priv subscriptions: std.ArrayList(Subscription) = .empty;
    priv sub_index: std.AutoHashMapUnmanaged(u64, usize) = .empty;
    priv retained: std.StringHashMapUnmanaged(RetainedEntry) = .empty;
    priv stats_published: u64 = 0;
    priv stats_delivered: u64 = 0;
    priv stats_dropped: u64 = 0;

    /// Initializes a `PubSubBroker` using `alloc` and `style` (`.Nats` or `.Mqtt`).
    pub static fn init(alloc: std.mem.Allocator, style: TopicStyle): PubSubBroker {
        return PubSubBroker{
            allocator: alloc,
            style: style,
            next_sub_id: 1,
            next_seq: 1,
            subscriptions: .empty,
            sub_index: .empty,
            retained: .empty,
            stats_published: 0,
            stats_delivered: 0,
            stats_dropped: 0,
        };
    }

    /// Releases all subscriptions, queued messages, retained topics, and internal maps.
    pub fn deinit(*_) {
        for (*sub of _.subscriptions.items) {
            sub.deinit(_.allocator);
        }
        _.subscriptions.deinit(_.allocator);
        _.sub_index.deinit(_.allocator);

        let iter = _.retained.iterator();
        while (entry := iter.next()) {
            entry.value_ptr.deinit(_.allocator);
        }
        _.retained.deinit(_.allocator);
    }

    fn dupeOwned(alloc: std.mem.Allocator, src: string): string ! PubSubError {
        if (src.len == 0) {
            return "";
        }
        return alloc.dupe(u8, src) catch return PubSubError.OutOfMemory;
    }

    fn freeOwned(alloc: std.mem.Allocator, s: string) {
        if (s.len > 0) {
            alloc.free(s);
        }
    }

    fn makeMessage(alloc: std.mem.Allocator, seq: u64, topic: string, payload: string, retained: bool): BrokerMessage ! PubSubError {
        const owned_topic = PubSubBroker.dupeOwned(alloc, topic)?;
        errdefer PubSubBroker.freeOwned(alloc, owned_topic);
        const owned_payload = PubSubBroker.dupeOwned(alloc, payload)?;
        return BrokerMessage{
            seq: seq,
            topic: owned_topic,
            payload: owned_payload,
            retained: retained,
            allocator: alloc,
        };
    }

    fn enqueueToSubIdx(*_, sub_idx: usize, seq: u64, topic: string, payload: string, retained: bool): bool ! PubSubError {
        if (_.subscriptions.items[sub_idx].queue.max_queue == 0) {
            _.stats_dropped += 1;
            return false;
        }
        const cur_len = _.subscriptions.items[sub_idx].queue.count();
        if (cur_len >= _.subscriptions.items[sub_idx].queue.max_queue && _.subscriptions.items[sub_idx].queue.drop_policy == .DropNewest) {
            _.stats_dropped += 1;
            return false;
        }

        let msg = PubSubBroker.makeMessage(_.allocator, seq, topic, payload, retained)?;
        errdefer msg.deinit();

        if (cur_len >= _.subscriptions.items[sub_idx].queue.max_queue) {
            // DropOldest: allocate new message first so OOM leaves queue intact.
            if (oldest := _.subscriptions.items[sub_idx].queue.popFront()) {
                let drop_old = oldest;
                drop_old.deinit();
                _.stats_dropped += 1;
            }
        }

        _.subscriptions.items[sub_idx].queue.items.append(_.allocator, msg) catch return PubSubError.OutOfMemory;
        _.stats_delivered += 1;
        return true;
    }

    /// Registers a wildcard subscription with bounded capacity `max_queue` and `drop_policy`.
    /// Immediately replays any matching retained messages (with `retained = true`) into the new queue.
    pub fn subscribe(*_, pattern: string, max_queue: usize, drop_policy: DropPolicy): u64 ! PubSubError {
        if (!TopicMatcher.isValidPattern(_.style, pattern)) {
            return PubSubError.InvalidPattern;
        }

        const owned_pattern = PubSubBroker.dupeOwned(_.allocator, pattern)?;
        let pattern_moved = false;
        errdefer {
            if (!pattern_moved) {
                PubSubBroker.freeOwned(_.allocator, owned_pattern);
            }
        }

        const sub_id = _.next_sub_id;
        const idx = _.subscriptions.items.len;

        _.subscriptions.append(_.allocator, Subscription{
            id: sub_id,
            pattern: owned_pattern,
            queue: MsgQueue{
                items: .empty,
                head: 0,
                max_queue: max_queue,
                drop_policy: drop_policy,
            },
        }) catch return PubSubError.OutOfMemory;
        pattern_moved = true;

        let indexed = false;
        errdefer {
            if (indexed) {
                * = _.sub_index.remove(sub_id);
            }
            let popped = _.subscriptions.pop().?;
            popped.deinit(_.allocator);
        }

        _.sub_index.put(_.allocator, sub_id, idx) catch return PubSubError.OutOfMemory;
        indexed = true;
        _.next_sub_id += 1;

        // Replay matching retained messages to the newly registered subscriber.
        let iter = _.retained.iterator();
        while (entry := iter.next()) {
            const ret = entry.value_ptr.*;
            if (TopicMatcher.matches(_.style, owned_pattern, ret.topic_buf)) {
                * = _.enqueueToSubIdx(idx, ret.seq, ret.topic_buf, ret.payload_buf, true)?;
            }
        }

        return sub_id;
    }

    /// Removes a subscription by `sub_id` and frees all pending messages in its queue.
    /// Returns `true` if the subscription existed and was removed.
    pub fn unsubscribe(*_, sub_id: u64): bool {
        const kv = _.sub_index.fetchRemove(sub_id) ?? return false;
        const idx = kv.value;
        let removed = _.subscriptions.swapRemove(idx);
        removed.deinit(_.allocator);

        if (idx < _.subscriptions.items.len) {
            const moved_id = _.subscriptions.items[idx].id;
            _.sub_index.putAssumeCapacity(moved_id, idx);
        }
        return true;
    }

    fn deliverLive(*_, seq: u64, topic: string, payload: string): usize ! PubSubError {
        let delivered_now: usize = 0;
        let i: usize = 0;
        while (i < _.subscriptions.items.len) : (i += 1) {
            if (TopicMatcher.matches(_.style, _.subscriptions.items[i].pattern, topic)) {
                const accepted = _.enqueueToSubIdx(i, seq, topic, payload, false)?;
                if (accepted) {
                    delivered_now += 1;
                }
            }
        }
        return delivered_now;
    }

    /// Publishes a non-retained message to all matching active subscribers.
    /// Returns the number of subscriber queues that accepted the message.
    pub fn publish(*_, topic: string, payload: string): usize ! PubSubError {
        if (!TopicMatcher.isValidPublishTopic(_.style, topic)) {
            return PubSubError.InvalidTopic;
        }
        const seq = _.next_seq;
        _.next_seq += 1;
        _.stats_published += 1;
        return _.deliverLive(seq, topic, payload);
    }

    /// Publishes a retained message and delivers it live to current matching subscribers.
    /// Following MQTT semantics, an empty `payload` clears any retained message for `topic`
    /// while still delivering the live message to current subscribers.
    pub fn publishRetained(*_, topic: string, payload: string): usize ! PubSubError {
        if (!TopicMatcher.isValidPublishTopic(_.style, topic)) {
            return PubSubError.InvalidTopic;
        }
        const seq = _.next_seq;
        _.next_seq += 1;
        _.stats_published += 1;

        if (payload.len == 0) {
            if (kv := _.retained.fetchRemove(topic)) {
                let old_entry = kv.value;
                old_entry.deinit(_.allocator);
            }
        } else {
            const new_payload = _.allocator.dupe(u8, payload) catch return PubSubError.OutOfMemory;
            if (existing := _.retained.getPtr(topic)) {
                if (existing.payload_buf.len > 0) {
                    _.allocator.free(existing.payload_buf);
                }
                existing.payload_buf = new_payload;
                existing.seq = seq;
            } else {
                errdefer _.allocator.free(new_payload);
                const new_topic = _.allocator.dupe(u8, topic) catch return PubSubError.OutOfMemory;
                errdefer _.allocator.free(new_topic);
                const entry = RetainedEntry{
                    topic_buf: new_topic,
                    payload_buf: new_payload,
                    seq: seq,
                };
                _.retained.put(_.allocator, new_topic, entry) catch return PubSubError.OutOfMemory;
            }
        }

        return _.deliverLive(seq, topic, payload);
    }

    /// Dequeues the next pending message for `sub_id`, or returns `null` if empty/unknown.
    /// Caller owns the returned `BrokerMessage` and must call `.deinit()`.
    pub fn poll(*_, sub_id: u64): BrokerMessage? {
        const idx = _.sub_index.get(sub_id) ?? return null;
        return _.subscriptions.items[idx].queue.popFront();
    }

    /// Returns the number of queued messages waiting for `sub_id` (`0` if unknown).
    pub fn pendingCount(sub_id: u64): usize {
        const idx = _.sub_index.get(sub_id) ?? return 0;
        return _.subscriptions.items[idx].queue.count();
    }

    /// Returns a snapshot of broker statistics.
    pub fn stats(): BrokerStats {
        return BrokerStats{
            published: _.stats_published,
            delivered: _.stats_delivered,
            dropped: _.stats_dropped,
            active_subscriptions: _.subscriptions.items.len,
            retained_topics: _.retained.count(),
        };
    }
}
