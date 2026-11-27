// Source: cp/lib/analytics/collector.cp (Bounded Ring-Buffer Telemetry Collector with Backpressure)
// Supports DropOldest, DropNewest, and RejectWhenFull policies with zero-leak batch flushing.

const std = @import("std");
import {
    AnalyticsError,
    AnalyticsEvent,
    DropPolicy,
    EventProp,
    MAX_PROPS_PER_EVENT,
    cloneEventWithSanitization,
    freeEvent,
    freeEventBatch,
    validateEventName,
    validatePropKey,
} from "./types.cp";

pub struct CollectorConfig {
    max_events: usize = 256,
    drop_policy: DropPolicy = .DropOldest,
}

pub struct AnalyticsCollector {
    alloc: std.mem.Allocator,
    ring: []AnalyticsEvent,
    head: usize = 0,
    len: usize = 0,
    cap: usize = 0,
    policy: DropPolicy = .DropOldest,
    dropped_count: u64 = 0,

    pub static fn init(alloc: std.mem.Allocator, config: CollectorConfig): AnalyticsCollector! {
        if (config.max_events == 0) {
            return AnalyticsError.InvalidConfig;
        }
        let ring = alloc.alloc(AnalyticsEvent, config.max_events) catch return AnalyticsError.OutOfMemory;
        let i: usize = 0;
        while (i < ring.len) : (i += 1) {
            ring[i] = AnalyticsEvent{};
        }
        return AnalyticsCollector{
            alloc: alloc,
            ring: ring,
            head: 0,
            len: 0,
            cap: config.max_events,
            policy: config.drop_policy,
            dropped_count: 0,
        };
    }

    pub fn deinit(*_) {
        _.clear();
        if (_.ring.len > 0) {
            _.alloc.free(_.ring);
        }
        _.ring = [];
        _.cap = 0;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const idx = (_.head + i) % _.cap;
            freeEvent(_.alloc, &_.ring[idx]);
        }
        _.head = 0;
        _.len = 0;
    }

    pub fn pendingCount(): usize {
        return _.len;
    }

    pub fn droppedCount(): u64 {
        return _.dropped_count;
    }

    pub fn capacity(): usize {
        return _.cap;
    }

    pub fn track(
        *_,
        name: []const u8,
        distinct_id: []const u8,
        session_id: []const u8,
        timestamp_ms: i64,
        props: []const EventProp,
    ): void! {
        try validateEventName(name);
        if (props.len > MAX_PROPS_PER_EVENT) {
            return AnalyticsError.TooManyProps;
        }
        let k: usize = 0;
        while (k < props.len) : (k += 1) {
            try validatePropKey(props[k].key);
        }

        if (_.len == _.cap) {
            if (_.policy == .RejectWhenFull) {
                return AnalyticsError.BufferFull;
            }
            if (_.policy == .DropNewest) {
                _.dropped_count += 1;
                return;
            }
        }

        const ev = try cloneEventWithSanitization(
            _.alloc,
            name,
            distinct_id,
            session_id,
            timestamp_ms,
            props,
        );

        if (_.len == _.cap) {
            // DropOldest: free slot at head and advance head
            freeEvent(_.alloc, &_.ring[_.head]);
            _.ring[_.head] = ev;
            _.head = (_.head + 1) % _.cap;
            _.dropped_count += 1;
            return;
        }

        const tail = (_.head + _.len) % _.cap;
        _.ring[tail] = ev;
        _.len += 1;
    }

    /// Drains up to `max_count` events in FIFO order into an owned slice.
    /// Caller must release the returned slice with `freeBatch` / `freeEventBatch`.
    pub fn flushBatch(*_, max_count: usize): ([]AnalyticsEvent)! {
        if (_.len == 0 || max_count == 0) {
            return [];
        }
        const count = if (max_count < _.len) max_count else _.len;
        let batch = _.alloc.alloc(AnalyticsEvent, count) catch return AnalyticsError.OutOfMemory;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const idx = (_.head + i) % _.cap;
            batch[i] = _.ring[idx];
            _.ring[idx] = AnalyticsEvent{};
        }
        _.head = (_.head + count) % _.cap;
        _.len -= count;
        if (_.len == 0) {
            _.head = 0;
        }
        return batch;
    }

    pub fn freeBatch(*_, batch: []AnalyticsEvent) {
        freeEventBatch(_.alloc, batch);
    }
}
