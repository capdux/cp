// cap/test/test_analytics_production_deep.cp
// Deep production test suite for cap:analytics & cap.analytics

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    REDACTED_VALUE,
    AnalyticsError,
    DropPolicy,
    PropKind,
    EventProp,
    AnalyticsEvent,
    CollectorConfig,
    AnalyticsCollector,
    isSensitiveKey,
    encodeBatchJson,
    encodeBatchNdjson,
} from "cap:analytics";

fn runCase1Redaction()! {
    const alloc = std.heap.page_allocator;
    let collector = try AnalyticsCollector.init(alloc, CollectorConfig{
        max_events: 8,
        drop_policy: .DropOldest,
    });
    defer collector.deinit();

    expect_eq(isSensitiveKey("password"), true);
    expect_eq(isSensitiveKey("X_API_KEY"), true);
    expect_eq(isSensitiveKey("user_route"), false);

    const props: []const EventProp = [
        EventProp.str("route", "/v1/auth"),
        EventProp.str("user_password", "super-secret-123"),
        EventProp.str("Authorization", "Bearer tok_abc"),
        EventProp.int("api_key", 999999),
        EventProp.int("latency_ms", 42),
        EventProp.float("cpu_ratio", 0.75),
        EventProp.boolean("ok", true),
    ];

    try collector.track("auth_attempt", "usr_01", "sess_01", 1700000001000, props);
    expect_eq(collector.pendingCount() as i64, 1);

    const batch = try collector.flushBatch(4);
    defer collector.freeBatch(batch);
    expect_eq(batch.len as i64, 1);
    expect_eq(batch[0].name, "auth_attempt");
    expect_eq(batch[0].distinct_id, "usr_01");
    expect_eq(batch[0].session_id, "sess_01");
    expect_eq(batch[0].timestamp_ms, 1700000001000);
    expect_eq(batch[0].props.len as i64, 7);

    expect_eq(batch[0].props[0].value.str_val, "/v1/auth");
    expect_eq(batch[0].props[1].value.kind == .String, true);
    expect_eq(batch[0].props[1].value.str_val, REDACTED_VALUE);
    expect_eq(batch[0].props[2].value.str_val, REDACTED_VALUE);
    expect_eq(batch[0].props[3].value.kind == .String, true);
    expect_eq(batch[0].props[3].value.str_val, REDACTED_VALUE);
    expect_eq(batch[0].props[4].value.int_val, 42);
    expect_eq(batch[0].props[5].value.float_val > 0.7, true);
    expect_eq(batch[0].props[6].value.bool_val, true);
}

fn runCase2Backpressure()! {
    const alloc = std.heap.page_allocator;
    const empty_props: []const EventProp = [];

    // Policy A: DropOldest
    let c_oldest = try AnalyticsCollector.init(alloc, CollectorConfig{
        max_events: 2,
        drop_policy: .DropOldest,
    });
    defer c_oldest.deinit();

    try c_oldest.track("ev1", "u1", "s1", 1, empty_props);
    try c_oldest.track("ev2", "u1", "s1", 2, empty_props);
    try c_oldest.track("ev3", "u1", "s1", 3, empty_props);
    expect_eq(c_oldest.pendingCount() as i64, 2);
    expect_eq(c_oldest.droppedCount() as i64, 1);

    const b_oldest = try c_oldest.flushBatch(10);
    defer c_oldest.freeBatch(b_oldest);
    expect_eq(b_oldest[0].name, "ev2");
    expect_eq(b_oldest[1].name, "ev3");

    // Policy B: DropNewest
    let c_newest = try AnalyticsCollector.init(alloc, CollectorConfig{
        max_events: 2,
        drop_policy: .DropNewest,
    });
    defer c_newest.deinit();

    try c_newest.track("ev1", "u1", "s1", 1, empty_props);
    try c_newest.track("ev2", "u1", "s1", 2, empty_props);
    try c_newest.track("ev3", "u1", "s1", 3, empty_props);
    expect_eq(c_newest.pendingCount() as i64, 2);
    expect_eq(c_newest.droppedCount() as i64, 1);

    const b_newest = try c_newest.flushBatch(10);
    defer c_newest.freeBatch(b_newest);
    expect_eq(b_newest[0].name, "ev1");
    expect_eq(b_newest[1].name, "ev2");

    // Policy C: RejectWhenFull
    let c_reject = try AnalyticsCollector.init(alloc, CollectorConfig{
        max_events: 2,
        drop_policy: .RejectWhenFull,
    });
    defer c_reject.deinit();

    try c_reject.track("ev1", "u1", "s1", 1, empty_props);
    try c_reject.track("ev2", "u1", "s1", 2, empty_props);
    let rejected = false;
    c_reject.track("ev3", "u1", "s1", 3, empty_props) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "BufferFull")) {
            rejected = true;
        }
    };
    expect_eq(rejected, true);
    expect_eq(c_reject.pendingCount() as i64, 2);
}

fn runCase3FlushBatch()! {
    const alloc = std.heap.page_allocator;
    const empty_props: []const EventProp = [];
    let collector = try AnalyticsCollector.init(alloc, CollectorConfig{
        max_events: 4,
        drop_policy: .DropOldest,
    });
    defer collector.deinit();

    try collector.track("e1", "u", "s", 10, empty_props);
    try collector.track("e2", "u", "s", 20, empty_props);
    try collector.track("e3", "u", "s", 30, empty_props);

    const b1 = try collector.flushBatch(2);
    expect_eq(b1.len as i64, 2);
    expect_eq(b1[0].name, "e1");
    expect_eq(b1[1].name, "e2");
    collector.freeBatch(b1);
    expect_eq(collector.pendingCount() as i64, 1);

    const b2 = try collector.flushBatch(5);
    expect_eq(b2.len as i64, 1);
    expect_eq(b2[0].name, "e3");
    collector.freeBatch(b2);
    expect_eq(collector.pendingCount() as i64, 0);

    const b_empty = try collector.flushBatch(5);
    expect_eq(b_empty.len as i64, 0);
    collector.freeBatch(b_empty);

    try collector.track("e4", "u", "s", 40, empty_props);
    expect_eq(collector.pendingCount() as i64, 1);
}

fn runCase4JsonAndGuards()! {
    const alloc = std.heap.page_allocator;
    let collector = try AnalyticsCollector.init(alloc, CollectorConfig{
        max_events: 8,
        drop_policy: .RejectWhenFull,
    });
    defer collector.deinit();

    const empty_props: []const EventProp = [];
    let empty_name_err = false;
    collector.track("", "u", "s", 1, empty_props) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "EmptyEventName")) empty_name_err = true;
    };
    expect_eq(empty_name_err, true);

    const long_key = "k_0123456789_0123456789_0123456789_0123456789_0123456789_0123456789";
    const bad_props: []const EventProp = [ EventProp.str(long_key, "v") ];
    let key_too_long_err = false;
    collector.track("valid", "u", "s", 1, bad_props) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "PropKeyTooLong")) key_too_long_err = true;
    };
    expect_eq(key_too_long_err, true);

    const esc_props: []const EventProp = [
        EventProp.str("msg", "line1\n\"quoted\"\\slash\t\x01"),
        EventProp.str("secret_token", "raw_secret"),
    ];
    try collector.track("log_event", "u1", "s1", 12345, esc_props);

    const batch = try collector.flushBatch(4);
    defer collector.freeBatch(batch);

    const json_bytes = try encodeBatchJson(alloc, batch);
    defer alloc.free(json_bytes);
    expect_eq(std.mem.indexOf(u8, json_bytes, "\\n\\\"quoted\\\"\\\\slash\\t\\u0001") != null, true);
    expect_eq(std.mem.indexOf(u8, json_bytes, "\"secret_token\":\"[REDACTED]\"") != null, true);

    const ndjson_bytes = try encodeBatchNdjson(alloc, batch);
    defer alloc.free(ndjson_bytes);
    expect_eq(ndjson_bytes.len > 0, true);
    expect_eq(ndjson_bytes[ndjson_bytes.len - 1] == '\n', true);
}

fn runCase5DynamicMountZeroLeak()! {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const alloc = gpa.allocator();
    {
        let collector = try cap.analytics.AnalyticsCollector.init(alloc, cap.analytics.CollectorConfig{
            max_events: 4,
            drop_policy: .DropOldest,
        });
        defer collector.deinit();

        const p0: EventProp = cap.analytics.EventProp.str("env", "prod");
        const p1: EventProp = cap.analytics.EventProp.int("retries", 2);
        const props: []const EventProp = [ p0, p1 ];
        try collector.track("boot", "node_1", "boot_0", 999, props);

        const batch = try collector.flushBatch(4);
        defer collector.freeBatch(batch);
        expect_eq(batch.len as i64, 1);

        const out = try cap.analytics.encodeBatchJson(alloc, batch);
        defer alloc.free(out);
        expect_eq(std.mem.indexOf(u8, out, "\"name\":\"boot\"") != null, true);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn main() {
    test("1. Event tracking & automatic sensitive property redaction", () => {
        try runCase1Redaction();
    });

    test("2. Backpressure drop policies: DropOldest, DropNewest, RejectWhenFull", () => {
        try runCase2Backpressure();
    });

    test("3. flushBatch partial draining & ring buffer state reset", () => {
        try runCase3FlushBatch();
    });

    test("4. encodeBatchJson / NDJSON escaping & validation guards", () => {
        try runCase4JsonAndGuards();
    });

    test("5. cap.analytics dynamic mount & zero-leak allocator verification", () => {
        try runCase5DynamicMountZeroLeak();
    });
}
