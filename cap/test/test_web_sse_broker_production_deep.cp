// Origin: Pure-CP Stdlib Test Suite (lib/web/sse_broker.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc SSE Fanout Broker & Last-Event-ID Ring Buffer:
//   1. Multi-client independent fanout & exact topic_filter_hash routing
//   2. Last-Event-ID disconnection catch-up vs live-tail (last_event_id == 0) subscription
//   3. Circular ring wrap-around slow-consumer lagged_resynced detection & seamless catch-up
//   4. WHATWG formatEventWire multi-line data (\n & \r\n) & optional event/retry rendering
//   5. Header injection guard (\r, \n, \0 in event_type) & payload length limits
//   6. Unsubscribe slot reclamation, 32-client capacity, unknown client & small wire buffer guards
//   7. Coexistence check with existing cap:web::sse.cp symbols

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SseEvent,
    SSE_BROKER_MAX_HISTORY,
    SSE_BROKER_MAX_CLIENTS,
    SSE_EVENT_TYPE_MAX_LEN,
    SSE_DATA_MAX_LEN,
    SseStoredEvent,
    SseClientCursor,
    SsePollStatus,
    SsePollResult,
    SseBrokerStats,
    formatSseEventWire,
    SseFanoutBroker,
} from "cap:web";

// ============================================================================
// Test 1: Multi-Client Independent Consumption & Topic Filtering
// ============================================================================
test("sse_broker: multi-client independent fanout & topic_filter_hash filtering", () => {
    let broker = SseFanoutBroker.init();

    // Client 1: wildcard all topics (0); Client 2: topic 100; Client 3: topic 200
    expect_eq(broker.subscribe(1, 0, 0), true);
    expect_eq(broker.subscribe(2, 100, 0), true);
    expect_eq(broker.subscribe(3, 200, 0), true);

    expect_eq(broker.publish(100, "msg", "t100-a", 0) ?? 0, 1);
    expect_eq(broker.publish(200, "msg", "t200-a", 0) ?? 0, 2);
    expect_eq(broker.publish(100, "msg", "t100-b", 0) ?? 0, 3);

    // Client 1 receives all 3 in order
    expect_eq(broker.poll_next(1).event_id, 1);
    expect_eq(broker.poll_next(1).event_id, 2);
    expect_eq(broker.poll_next(1).event_id, 3);
    expect_eq(broker.poll_next(1).status == SsePollStatus.empty, true);

    // Client 2 receives only topic 100 events (1, 3)
    const c2_1 = broker.poll_next(2);
    expect_eq(c2_1.status == SsePollStatus.has_event, true);
    expect_eq(c2_1.event_id, 1);
    const c2_2 = broker.poll_next(2);
    expect_eq(c2_2.status == SsePollStatus.has_event, true);
    expect_eq(c2_2.event_id, 3);
    expect_eq(broker.poll_next(2).status == SsePollStatus.empty, true);

    // Client 3 receives only topic 200 event (2)
    const c3_1 = broker.poll_next(3);
    expect_eq(c3_1.status == SsePollStatus.has_event, true);
    expect_eq(c3_1.event_id, 2);
    expect_eq(broker.poll_next(3).status == SsePollStatus.empty, true);
});

// ============================================================================
// Test 2: Last-Event-ID Disconnection Resumption vs Tail Subscription
// ============================================================================
test("sse_broker: Last-Event-ID resumes exact unread events while 0 starts at tail", () => {
    let broker = SseFanoutBroker.init();
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        expect_eq(broker.publish(10, "tick", "payload", 0) != null, true);
    }

    // Live-tail subscriber (last_event_id == 0) sees empty immediately
    expect_eq(broker.subscribe(10, 0, 0), true);
    expect_eq(broker.poll_next(10).status == SsePollStatus.empty, true);

    // Reconnecting subscriber with Last-Event-ID = 2 catches up on 3, 4, 5
    expect_eq(broker.subscribe(20, 0, 2), true);
    const p3 = broker.poll_next(20);
    expect_eq(p3.status == SsePollStatus.has_event, true);
    expect_eq(p3.event_id, 3);
    expect_eq(broker.poll_next(20).event_id, 4);
    expect_eq(broker.poll_next(20).event_id, 5);
    expect_eq(broker.poll_next(20).status == SsePollStatus.empty, true);
});

// ============================================================================
// Test 3: Ring Wrap-Around & Slow Consumer lagged_resynced Recovery
// ============================================================================
test("sse_broker: circular ring wrap detects slow consumer lag & resyncs cleanly", () => {
    let broker = SseFanoutBroker.init();
    expect_eq(broker.subscribe(1, 0, 0), true);

    // Publish 2 events and consume both (client acked = 2)
    expect_eq(broker.publish(1, "ev", "e1", 0) ?? 0, 1);
    expect_eq(broker.publish(1, "ev", "e2", 0) ?? 0, 2);
    expect_eq(broker.poll_next(1).event_id, 1);
    expect_eq(broker.poll_next(1).event_id, 2);

    // Publish 70 more events (total = 72; ring retains [9..72], oldest = 9)
    let k: usize = 0;
    while (k < 70) : (k += 1) {
        expect_eq(broker.publish(1, "ev", "burst", 0) != null, true);
    }

    const st0 = broker.stats();
    expect_eq(st0.total_published, 72);
    expect_eq(st0.ring_count, 64);
    expect_eq(st0.oldest_event_id, 9);
    expect_eq(st0.latest_event_id, 72);

    // Client wanted ID 3, oldest is 9 -> skipped 6 events (3..8), resynced to ID 9
    const lag = broker.poll_next(1);
    expect_eq(lag.status == SsePollStatus.lagged_resynced, true);
    expect_eq(lag.skipped_events, 6);
    expect_eq(lag.event_id, 9);

    // Next poll resumes normal sequential delivery at ID 10
    const next_ev = broker.poll_next(1);
    expect_eq(next_ev.status == SsePollStatus.has_event, true);
    expect_eq(next_ev.skipped_events, 0);
    expect_eq(next_ev.event_id, 10);
    expect_eq(broker.stats().total_lag_drops, 6);
});

// ============================================================================
// Test 4: WHATWG formatEventWire Multi-Line Data & Optional Fields
// ============================================================================
test("sse_broker: formatEventWire splits CRLF/LF multi-line data & optional headers", () => {
    let broker = SseFanoutBroker.init();
    expect_eq(broker.subscribe(1, 0, 0), true);

    // Full event with event_type, retry_ms, and mixed \r\n & \n data
    expect_eq(broker.publish(1, "delta", "line1\r\nline2\nline3", 2500) ?? 0, 1);
    const p1 = broker.poll_next(1);
    const ev1 = broker.get_event(p1.event_idx) ?? return;

    let wire: [512]u8 = [0; 512];
    let wire_s: []u8 = wire[0..];
    const n1_opt = SseFanoutBroker.formatEventWire(ev1, wire_s);
    expect_eq(n1_opt != null, true);
    const n1 = n1_opt ?? return;
    const expected1 = "id: 1\nevent: delta\nretry: 2500\ndata: line1\ndata: line2\ndata: line3\n\n";
    expect_eq(std.mem.eql(u8, wire[0..n1], expected1), true);

    // Minimal event: no event_type, retry_ms == 0, empty data
    expect_eq(broker.publish(1, "", "", 0) ?? 0, 2);
    const p2 = broker.poll_next(1);
    const ev2 = broker.get_event(p2.event_idx) ?? return;
    const n2_opt = formatSseEventWire(ev2, wire_s);
    expect_eq(n2_opt != null, true);
    const n2 = n2_opt ?? return;
    const expected2 = "id: 2\ndata: \n\n";
    expect_eq(std.mem.eql(u8, wire[0..n2], expected2), true);
});

// ============================================================================
// Test 5: Header Injection Guard (\r, \n, \0 in event_type) & Size Limits
// ============================================================================
test("sse_broker: rejects CR/LF/NUL injection in event_type & oversized payloads", () => {
    let broker = SseFanoutBroker.init();
    expect_eq(SSE_EVENT_TYPE_MAX_LEN, 32);
    expect_eq(SSE_DATA_MAX_LEN, 256);

    // Header injection attempts blocked
    expect_eq(broker.publish(1, "msg\ndata: forged", "ok", 0) == null, true);
    expect_eq(broker.publish(1, "msg\r\nid: 999", "ok", 0) == null, true);
    const nul_type: [4]u8 = [97, 0, 98, 99];
    expect_eq(broker.publish(1, nul_type[0..], "ok", 0) == null, true);

    // Oversized event_type (33B) and oversized data (257B) blocked
    let big_type: [33]u8 = [97; 33];
    let big_data: [257]u8 = [98; 257];
    expect_eq(broker.publish(1, big_type[0..], "ok", 0) == null, true);
    expect_eq(broker.publish(1, "ok", big_data[0..], 0) == null, true);

    expect_eq(broker.stats().total_published, 0);
});

// ============================================================================
// Test 6: Unsubscribe Slot Reclamation, 32-Client Cap & Small Buffer Guards
// ============================================================================
test("sse_broker: unsubscribe slot reuse, 32-client cap & wire buffer overflow guard", () => {
    expect_eq(SSE_BROKER_MAX_HISTORY, 64);
    expect_eq(SSE_BROKER_MAX_CLIENTS, 32);

    let broker = SseFanoutBroker.init();
    expect_eq(broker.subscribe(0, 0, 0), false); // client_id == 0 rejected
    expect_eq(broker.poll_next(999).status == SsePollStatus.client_not_found, true);

    // Fill all 32 client slots
    let cid: u64 = 1;
    while (cid <= 32) : (cid += 1) {
        expect_eq(broker.subscribe(cid, 0, 0), true);
    }
    expect_eq(broker.stats().active_clients, 32);
    // Duplicate & 33rd client rejected
    expect_eq(broker.subscribe(1, 0, 0), false);
    expect_eq(broker.subscribe(33, 0, 0), false);

    // Unsubscribe slot 5 -> 33rd client succeeds
    expect_eq(broker.unsubscribe(5), true);
    expect_eq(broker.unsubscribe(5), false);
    expect_eq(broker.stats().active_clients, 31);
    expect_eq(broker.subscribe(33, 0, 0), true);
    expect_eq(broker.stats().active_clients, 32);

    // Small output buffer causes formatEventWire to return null safely without OOB
    expect_eq(broker.publish(1, "token", "hello world", 1000) != null, true);
    const ev = broker.get_event(0) ?? return;
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(SseFanoutBroker.formatEventWire(ev, tiny_s) == null, true);
});

// ============================================================================
// Test 7: Coexistence with Existing cap:web::sse.cp Symbols
// ============================================================================
test("sse_broker: zero symbol conflict with existing sse.cp SseEvent", () => {
    const ev = SseEvent{};
    let broker = SseFanoutBroker.init();
    * = ev;
    expect_eq(broker.stats().active_clients, 0);
});
