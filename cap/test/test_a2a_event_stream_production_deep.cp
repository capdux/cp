// Origin: Pure-CP Stdlib Test Suite (A2A Event Stream Replay Ring & Last-Event-ID Resume Cursor) | Created: 2026-11-22
// Deep Production Verification Suite for lib/a2a/event_stream.cp
// Covers:
//   1. Strictly monotonic event_id publishing and task_id-filtered vs unfiltered replay
//   2. Reconnect resume via Last-Event-ID cursor (exact catch-up, boundary alignment & caught-up 0-copy)
//   3. Ring wrap-around overwrite triggering gap_detected == true & ahead-of-head cursor defense
//   4. Truncation-safe small output buffer replay with paginated multi-batch cursor resume
//   5. Empty ring, clear() retention of monotonic IDs, full replay from 0 & zero-capacity guard

import { test, expect_eq } from "cap:test";
import {
    A2aEventKind,
    A2aStreamEvent,
    ReplayResult,
    A2aEventStreamRing,
} from "../../lib/a2a/index.cp";

fn testMonotonicPublishAndTaskFilterReplay()! {
    let slots_buf: [8]A2aStreamEvent = [A2aStreamEvent{}; 8];
    let slots: []A2aStreamEvent = slots_buf[0..];
    let ring = A2aEventStreamRing.init(slots);

    const e1 = ring.publish(100, A2aEventKind.StatusUpdate, 1000, false, 0x1111, 32);
    const e2 = ring.publish(200, A2aEventKind.ArtifactAppend, 1010, false, 0x2222, 64);
    const e3 = ring.publish(100, A2aEventKind.ArtifactComplete, 1020, true, 0x3333, 128);
    const e4 = ring.publish(200, A2aEventKind.InputRequired, 1030, false, 0x4444, 16);
    const e5 = ring.publish(300, A2aEventKind.Heartbeat, 1040, false, 0x0, 0);

    expect_eq(e1, 1);
    expect_eq(e2, 2);
    expect_eq(e3, 3);
    expect_eq(e4, 4);
    expect_eq(e5, 5);
    expect_eq(ring.count(), 5);
    expect_eq(ring.oldestEventId(), 1);
    expect_eq(ring.latestEventId(), 5);

    // Unfiltered full replay (task_filter == 0)
    let out_buf: [8]A2aStreamEvent = [A2aStreamEvent{}; 8];
    let out: []A2aStreamEvent = out_buf[0..];
    const all_res = ring.replayAfter(0, 0, out);
    expect_eq(all_res.copied, 5);
    expect_eq(all_res.total_available, 5);
    expect_eq(all_res.gap_detected, false);
    expect_eq(all_res.latest_event_id, 5);
    expect_eq(out[0].event_id, 1);
    expect_eq(out[4].event_id, 5);
    expect_eq(out[2].is_final, true);
    expect_eq(out[2].payload_len, 128);

    // Task-filtered replay (task_id == 100)
    const t100_res = ring.replayAfter(0, 100, out);
    expect_eq(t100_res.copied, 2);
    expect_eq(t100_res.total_available, 2);
    expect_eq(t100_res.gap_detected, false);
    expect_eq(out[0].event_id, 1);
    expect_eq(out[0].kind == A2aEventKind.StatusUpdate, true);
    expect_eq(out[1].event_id, 3);
    expect_eq(out[1].kind == A2aEventKind.ArtifactComplete, true);

    // Task-filtered replay for non-existent task returns 0 without gap
    const none_res = ring.replayAfter(0, 999, out);
    expect_eq(none_res.copied, 0);
    expect_eq(none_res.total_available, 0);
    expect_eq(none_res.gap_detected, false);
    expect_eq(none_res.latest_event_id, 5);
}

fn testLastEventIdExactResume()! {
    let slots_buf: [6]A2aStreamEvent = [A2aStreamEvent{}; 6];
    let slots: []A2aStreamEvent = slots_buf[0..];
    let ring = A2aEventStreamRing.init(slots);

    * = ring.publish(42, A2aEventKind.StatusUpdate, 100, false, 0xA1, 10);
    * = ring.publish(42, A2aEventKind.ArtifactAppend, 200, false, 0xA2, 20);
    * = ring.publish(42, A2aEventKind.ArtifactAppend, 300, false, 0xA3, 30);
    * = ring.publish(42, A2aEventKind.ArtifactComplete, 400, true, 0xA4, 40);

    let out_buf: [6]A2aStreamEvent = [A2aStreamEvent{}; 6];
    let out: []A2aStreamEvent = out_buf[0..];

    // Client disconnected after event 2 -> resumes strictly at events 3 & 4
    const r1 = ring.replayAfter(2, 42, out);
    expect_eq(r1.copied, 2);
    expect_eq(r1.total_available, 2);
    expect_eq(r1.gap_detected, false);
    expect_eq(r1.latest_event_id, 4);
    expect_eq(out[0].event_id, 3);
    expect_eq(out[0].payload_hash, 0xA3);
    expect_eq(out[1].event_id, 4);
    expect_eq(out[1].is_final, true);

    // Client already caught up at event 4 -> 0 new events, no gap
    const r2 = ring.replayAfter(4, 42, out);
    expect_eq(r2.copied, 0);
    expect_eq(r2.total_available, 0);
    expect_eq(r2.gap_detected, false);
    expect_eq(r2.latest_event_id, 4);
}

fn testRingOverwriteGapDetection()! {
    let slots_buf: [4]A2aStreamEvent = [A2aStreamEvent{}; 4];
    let slots: []A2aStreamEvent = slots_buf[0..];
    let ring = A2aEventStreamRing.init(slots);

    // Publish 6 events into a 4-slot ring -> events 1 & 2 overwritten; ring retains 3, 4, 5, 6
    let i: u64 = 1;
    while (i <= 6) : (i += 1) {
        * = ring.publish(7, A2aEventKind.ArtifactAppend, i * 100, i == 6, i, u32(i * 10));
    }

    expect_eq(ring.count(), 4);
    expect_eq(ring.oldestEventId(), 3);
    expect_eq(ring.latestEventId(), 6);

    let out_buf: [6]A2aStreamEvent = [A2aStreamEvent{}; 6];
    let out: []A2aStreamEvent = out_buf[0..];

    // Client with last_event_id == 1 missed event 2 -> gap_detected MUST be true
    const g1 = ring.replayAfter(1, 0, out);
    expect_eq(g1.gap_detected, true);
    expect_eq(g1.copied, 4);
    expect_eq(g1.total_available, 4);
    expect_eq(g1.latest_event_id, 6);
    expect_eq(out[0].event_id, 3);
    expect_eq(out[3].event_id, 6);

    // Client with last_event_id == 0 on wrapped ring also detects gap (events 1..2 lost)
    const g0 = ring.replayAfter(0, 0, out);
    expect_eq(g0.gap_detected, true);
    expect_eq(g0.copied, 4);

    // Client with last_event_id == 2 aligns seamlessly with oldestEventId == 3 -> NO gap!
    const exact_align = ring.replayAfter(2, 0, out);
    expect_eq(exact_align.gap_detected, false);
    expect_eq(exact_align.copied, 4);
    expect_eq(exact_align.total_available, 4);
    expect_eq(out[0].event_id, 3);

    // Client presenting a stale/future cursor ahead of latest_event_id triggers gap fallback
    const ahead = ring.replayAfter(99, 0, out);
    expect_eq(ahead.gap_detected, true);
    expect_eq(ahead.copied, 0);
    expect_eq(ahead.total_available, 0);
    expect_eq(ahead.latest_event_id, 6);
}

fn testOutputBufferTruncationAndPagination()! {
    let slots_buf: [6]A2aStreamEvent = [A2aStreamEvent{}; 6];
    let slots: []A2aStreamEvent = slots_buf[0..];
    let ring = A2aEventStreamRing.init(slots);

    * = ring.publish(10, A2aEventKind.StatusUpdate, 10, false, 1, 8);
    * = ring.publish(10, A2aEventKind.ArtifactAppend, 20, false, 2, 16);
    * = ring.publish(10, A2aEventKind.ArtifactAppend, 30, false, 3, 24);
    * = ring.publish(10, A2aEventKind.ArtifactAppend, 40, false, 4, 32);
    * = ring.publish(10, A2aEventKind.Error, 50, true, 5, 40);

    // Small 2-slot caller buffer: batch 1 gets events 1..2 out of 5 available
    let page_buf: [2]A2aStreamEvent = [A2aStreamEvent{}; 2];
    let page: []A2aStreamEvent = page_buf[0..];

    const p1 = ring.replayAfter(0, 10, page);
    expect_eq(p1.copied, 2);
    expect_eq(p1.total_available, 5);
    expect_eq(p1.gap_detected, false);
    expect_eq(page[0].event_id, 1);
    expect_eq(page[1].event_id, 2);

    // Batch 2 resumes from page[1].event_id (2) -> events 3..4 out of 3 remaining
    const p2 = ring.replayAfter(page[1].event_id, 10, page);
    expect_eq(p2.copied, 2);
    expect_eq(p2.total_available, 3);
    expect_eq(p2.gap_detected, false);
    expect_eq(page[0].event_id, 3);
    expect_eq(page[1].event_id, 4);

    // Batch 3 resumes from 4 -> final event 5
    const p3 = ring.replayAfter(page[1].event_id, 10, page);
    expect_eq(p3.copied, 1);
    expect_eq(p3.total_available, 1);
    expect_eq(p3.gap_detected, false);
    expect_eq(page[0].event_id, 5);
    expect_eq(page[0].kind == A2aEventKind.Error, true);
    expect_eq(page[0].is_final, true);
}

fn testEmptyRingClearAndZeroCapacityGuards()! {
    let slots_buf: [4]A2aStreamEvent = [A2aStreamEvent{}; 4];
    let slots: []A2aStreamEvent = slots_buf[0..];
    let ring = A2aEventStreamRing.init(slots);

    let out_buf: [4]A2aStreamEvent = [A2aStreamEvent{}; 4];
    let out: []A2aStreamEvent = out_buf[0..];

    // Fresh empty ring
    expect_eq(ring.count(), 0);
    expect_eq(ring.capacity(), 4);
    expect_eq(ring.oldestEventId(), 0);
    expect_eq(ring.latestEventId(), 0);
    const empty_res = ring.replayAfter(0, 0, out);
    expect_eq(empty_res.copied, 0);
    expect_eq(empty_res.total_available, 0);
    expect_eq(empty_res.gap_detected, false);
    expect_eq(empty_res.latest_event_id, 0);

    // Publish, clear, and verify monotonic event_id continues without collision
    expect_eq(ring.publish(1, A2aEventKind.StatusUpdate, 10, false, 1, 4), 1);
    expect_eq(ring.publish(1, A2aEventKind.StatusUpdate, 20, false, 2, 4), 2);
    ring.clear();
    expect_eq(ring.count(), 0);
    expect_eq(ring.oldestEventId(), 0);
    expect_eq(ring.latestEventId(), 0);

    const e3 = ring.publish(1, A2aEventKind.StatusUpdate, 30, true, 3, 4);
    expect_eq(e3, 3);
    expect_eq(ring.count(), 1);
    expect_eq(ring.oldestEventId(), 3);
    expect_eq(ring.latestEventId(), 3);

    // Zero-capacity ring never panics on publish or replay
    let zero_buf: [0]A2aStreamEvent = [];
    let zero_slots: []A2aStreamEvent = zero_buf[0..];
    let zero_ring = A2aEventStreamRing.init(zero_slots);
    expect_eq(zero_ring.publish(1, A2aEventKind.Heartbeat, 100, false, 0, 0), 1);
    expect_eq(zero_ring.count(), 0);
    const zres = zero_ring.replayAfter(0, 0, out);
    expect_eq(zres.copied, 0);
    expect_eq(zres.gap_detected, true);
}

test("a2a event_stream: monotonic event_id publish and task_id filtered replay", testMonotonicPublishAndTaskFilterReplay);
test("a2a event_stream: Last-Event-ID exact reconnect resume and caught-up state", testLastEventIdExactResume);
test("a2a event_stream: ring wrap-around overwrite gap_detected alert and boundary alignment", testRingOverwriteGapDetection);
test("a2a event_stream: output buffer truncation safety and multi-batch cursor pagination", testOutputBufferTruncationAndPagination);
test("a2a event_stream: empty ring, clear monotonic continuity and zero-capacity guard", testEmptyRingClearAndZeroCapacityGuards);
