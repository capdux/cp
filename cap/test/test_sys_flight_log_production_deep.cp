// Origin: Pure-CP Stdlib Test Suite (lib/sys/flightLog.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Black-Box Binary Flight Recorder:
//   1. Basic recording, monotonic seq_id, empty payload & ordered full snapshot replay
//   2. Ring-buffer wrap-around overwrite, exact dropped_count accounting & zero tear verification
//   3. Incremental after_seq_id & recent-N (max_recent) snapshot filtering
//   4. Crash freeze/unfreeze latch protecting forensic state from overwrite and clear()
//   5. CRC32 binary dump export, verifyBinaryDump & offline parseBinaryDump across wrap-around
//   6. Tamper, truncation, bad magic & bad version dump rejection guards
//   7. Oversized payload (>256B) rejection, exact 256B fit, empty buffer & undersized dump buffer guards
//   8. clear() reset preserving monotonic next_seq_id

import { test, expect_eq } from "cap:test";
import {
    FLIGHT_LOG_RING_BYTES,
    FLIGHT_LOG_MAX_PAYLOAD,
    FLIGHT_LOG_RECORD_HEADER_SIZE,
    FLIGHT_LOG_DUMP_MAGIC,
    FLIGHT_LOG_DUMP_HEADER_SIZE,
    FLIGHT_LOG_DUMP_TRAILER_SIZE,
    FlightSeverity,
    FlightRecordStatus,
    FlightDumpStatus,
    FlightEvent,
    FlightDumpMeta,
    FlightLogStats,
    FlightRecorder,
    verifyBinaryDump,
    parseBinaryDump,
} from "cap:sys";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Basic Recording, Monotonic seq_id & Ordered Replay
// ============================================================================

fn testBasicRecordAndOrderedReplay()! {
    let rec = FlightRecorder.init();

    expect_eq(rec.recordEvent(1000, 10, FlightSeverity.info as u8, "boot:ok") == FlightRecordStatus.ok, true);
    expect_eq(rec.recordEvent(2000, 20, FlightSeverity.warn as u8, "") == FlightRecordStatus.ok, true);
    expect_eq(rec.recordEvent(3000, 30, FlightSeverity.err as u8, "io:timeout") == FlightRecordStatus.ok, true);

    let out: [8]FlightEvent = [FlightEvent{}; 8];
    let out_s: []FlightEvent = out[0..];
    const n = rec.snapshotEvents(0, 0, *out_s);
    expect_eq(n, 3);

    expect_eq(out[0].seq_id, 1);
    expect_eq(out[0].timestamp_ns, 1000);
    expect_eq(out[0].event_kind, 10);
    expect_eq(out[0].severity, FlightSeverity.info as u8);
    expect_eq(bytesEq(out[0].payloadSlice(), "boot:ok"), true);

    expect_eq(out[1].seq_id, 2);
    expect_eq(out[1].payload_len, 0);

    expect_eq(out[2].seq_id, 3);
    expect_eq(out[2].timestamp_ns, 3000);
    expect_eq(out[2].event_kind, 30);
    expect_eq(out[2].severity, FlightSeverity.err as u8);
    expect_eq(bytesEq(out[2].payloadSlice(), "io:timeout"), true);
}

// ============================================================================
// Test 2: Ring-Buffer Wrap-Around Overwrite & Exact dropped_count
// ============================================================================

fn testRingWrapAroundAndExactDroppedCount()! {
    let rec = FlightRecorder.init();

    // Write 120 events * (22 + 100 = 122B) = 14640B > 8192B -> guaranteed wrap-around
    let i: usize = 1;
    while (i <= 120) : (i += 1) {
        let payload: [100]u8 = [0; 100];
        let j: usize = 0;
        while (j < 100) : (j += 1) {
            payload[j] = ((i + j) & 0xff) as u8;
        }
        expect_eq(rec.recordEvent((i as u64) * 100, (i & 0xffff) as u16, FlightSeverity.debug as u8, payload[0..]) == FlightRecordStatus.ok, true);
    }

    const st: FlightLogStats = rec.stats();
    expect_eq(st.total_recorded, 120);
    expect_eq(st.used_bytes <= FLIGHT_LOG_RING_BYTES, true);
    expect_eq(st.dropped_count > 0, true);
    expect_eq((st.live_events as u64) + st.dropped_count, 120);

    // Replay all surviving events and verify unbroken seq_id & zero payload corruption
    let out: [80]FlightEvent = [FlightEvent{}; 80];
    let out_s: []FlightEvent = out[0..];
    const n = rec.snapshotEvents(0, 0, *out_s);
    expect_eq(n, st.live_events);

    const first_expected_seq: u64 = st.dropped_count + 1;
    let k: usize = 0;
    while (k < n) : (k += 1) {
        const expected_seq: u64 = first_expected_seq + (k as u64);
        expect_eq(out[k].seq_id, expected_seq);
        expect_eq(out[k].payload_len, 100);
        const p = out[k].payloadSlice();
        expect_eq(p[0], (expected_seq & 0xff) as u8);
        expect_eq(p[99], ((expected_seq + 99) & 0xff) as u8);
    }
}

// ============================================================================
// Test 3: Incremental after_seq_id & Recent-N Snapshot Filtering
// ============================================================================

fn testIncrementalAndRecentNSnapshots()! {
    let rec = FlightRecorder.init();
    let i: u64 = 1;
    while (i <= 10) : (i += 1) {
        * = rec.recordEvent(i * 1000, 1, FlightSeverity.info as u8, "ev");
    }

    let out: [10]FlightEvent = [FlightEvent{}; 10];
    let out_s: []FlightEvent = out[0..];

    // Incremental after_seq_id = 6 -> [7, 8, 9, 10]
    const n1 = rec.snapshotEvents(6, 0, *out_s);
    expect_eq(n1, 4);
    expect_eq(out[0].seq_id, 7);
    expect_eq(out[3].seq_id, 10);

    // Recent 3 -> [8, 9, 10]
    const n2 = rec.snapshotEvents(0, 3, *out_s);
    expect_eq(n2, 3);
    expect_eq(out[0].seq_id, 8);
    expect_eq(out[1].seq_id, 9);
    expect_eq(out[2].seq_id, 10);

    // Combined after_seq_id = 5, max_recent = 2 -> [9, 10]
    const n3 = rec.snapshotEvents(5, 2, *out_s);
    expect_eq(n3, 2);
    expect_eq(out[0].seq_id, 9);
    expect_eq(out[1].seq_id, 10);

    // Beyond latest seq_id -> 0
    expect_eq(rec.snapshotEvents(10, 0, *out_s), 0);
}

// ============================================================================
// Test 4: Crash Freeze / Unfreeze Latch Protecting Forensic State
// ============================================================================

fn testFreezeUnfreezeForensicLatch()! {
    let rec = FlightRecorder.init();
    * = rec.recordEvent(10, 1, FlightSeverity.info as u8, "before_crash");
    * = rec.recordEvent(20, 2, FlightSeverity.fatal as u8, "panic:oom");

    rec.freeze();
    expect_eq(rec.isFrozen(), true);
    expect_eq(rec.recordEvent(30, 3, FlightSeverity.info as u8, "noise") == FlightRecordStatus.frozen, true);
    expect_eq(rec.clear(), false);
    expect_eq(rec.stats().live_events, 2);
    expect_eq(rec.stats().frozen_rejects, 1);

    rec.unfreeze();
    expect_eq(rec.isFrozen(), false);
    expect_eq(rec.recordEvent(40, 4, FlightSeverity.info as u8, "recovered") == FlightRecordStatus.ok, true);
    expect_eq(rec.stats().live_events, 3);
    expect_eq(rec.stats().next_seq_id, 4);
}

// ============================================================================
// Test 5: CRC32 Binary Dump Export, Verify & Offline Parse Across Wrap-Around
// ============================================================================

fn testBinaryDumpExportVerifyAndOfflineParse()! {
    let rec = FlightRecorder.init();

    // Force wrap-around before exporting dump
    let i: usize = 1;
    while (i <= 90) : (i += 1) {
        let p: [110]u8 = [(i & 0xff) as u8; 110];
        * = rec.recordEvent((i as u64) * 50, (i & 0xff) as u16, FlightSeverity.info as u8, p[0..]);
    }
    rec.freeze();

    let dump_buf: [9000]u8 = [0; 9000];
    let dump_s: []u8 = dump_buf[0..];
    const dump_len = rec.exportBinaryDump(*dump_s) ?? 0;
    expect_eq(dump_len > FLIGHT_LOG_DUMP_HEADER_SIZE + FLIGHT_LOG_DUMP_TRAILER_SIZE, true);

    const meta: FlightDumpMeta = verifyBinaryDump(dump_buf[0..dump_len]);
    expect_eq(meta.status == FlightDumpStatus.ok, true);
    expect_eq(meta.frozen, true);
    expect_eq(meta.record_count as usize, rec.stats().live_events);
    expect_eq(meta.dropped_count, rec.stats().dropped_count);
    expect_eq(meta.next_seq_id, rec.stats().next_seq_id);

    let parsed: [80]FlightEvent = [FlightEvent{}; 80];
    let parsed_s: []FlightEvent = parsed[0..];
    const n_parsed = parseBinaryDump(dump_buf[0..dump_len], *parsed_s) ?? 0;
    expect_eq(n_parsed, rec.stats().live_events);

    let online: [80]FlightEvent = [FlightEvent{}; 80];
    let online_s: []FlightEvent = online[0..];
    const n_online = rec.snapshotEvents(0, 0, *online_s);
    expect_eq(n_parsed, n_online);

    let k: usize = 0;
    while (k < n_parsed) : (k += 1) {
        expect_eq(parsed[k].seq_id, online[k].seq_id);
        expect_eq(parsed[k].timestamp_ns, online[k].timestamp_ns);
        expect_eq(bytesEq(parsed[k].payloadSlice(), online[k].payloadSlice()), true);
    }
}

// ============================================================================
// Test 6: Tamper, Truncation, Bad Magic & Bad Version Dump Guards
// ============================================================================

fn testBinaryDumpCorruptionGuards()! {
    let rec = FlightRecorder.init();
    * = rec.recordEvent(100, 1, FlightSeverity.info as u8, "forensic_data");

    let dump_buf: [256]u8 = [0; 256];
    let dump_s: []u8 = dump_buf[0..];
    const dump_len = rec.exportBinaryDump(*dump_s) ?? 0;
    expect_eq(verifyBinaryDump(dump_buf[0..dump_len]).status == FlightDumpStatus.ok, true);

    // 1) Flip 1 bit in body -> checksum_mismatch & parseBinaryDump returns null
    dump_buf[FLIGHT_LOG_DUMP_HEADER_SIZE + 1] = dump_buf[FLIGHT_LOG_DUMP_HEADER_SIZE + 1] ^ 0x01;
    expect_eq(verifyBinaryDump(dump_buf[0..dump_len]).status == FlightDumpStatus.checksum_mismatch, true);
    let evs: [4]FlightEvent = [FlightEvent{}; 4];
    let evs_s: []FlightEvent = evs[0..];
    expect_eq(parseBinaryDump(dump_buf[0..dump_len], *evs_s) == null, true);
    dump_buf[FLIGHT_LOG_DUMP_HEADER_SIZE + 1] = dump_buf[FLIGHT_LOG_DUMP_HEADER_SIZE + 1] ^ 0x01;

    // 2) Truncated dump -> truncated
    expect_eq(verifyBinaryDump(dump_buf[0..dump_len - 1]).status == FlightDumpStatus.truncated, true);
    expect_eq(verifyBinaryDump(dump_buf[0..10]).status == FlightDumpStatus.truncated, true);

    // 3) Bad version -> invalid_version
    dump_buf[4] = 99;
    expect_eq(verifyBinaryDump(dump_buf[0..dump_len]).status == FlightDumpStatus.invalid_version, true);
    dump_buf[4] = 1;

    // 4) Bad magic -> invalid_magic
    dump_buf[0] = 0x00;
    expect_eq(verifyBinaryDump(dump_buf[0..dump_len]).status == FlightDumpStatus.invalid_magic, true);
}

// ============================================================================
// Test 7: Oversized Payload, Exact 256B Fit, Empty Buffer & Small Dump Buffer
// ============================================================================

fn testOversizedPayloadAndEmptyGuards()! {
    let rec = FlightRecorder.init();

    // Empty recorder snapshot & empty dump
    let evs: [4]FlightEvent = [FlightEvent{}; 4];
    let evs_s: []FlightEvent = evs[0..];
    expect_eq(rec.snapshotEvents(0, 0, *evs_s), 0);

    let tiny_dump: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny_dump[0..];
    expect_eq(rec.exportBinaryDump(*tiny_s) == null, true);

    let ok_empty_dump: [64]u8 = [0; 64];
    let ok_empty_s: []u8 = ok_empty_dump[0..];
    const elen = rec.exportBinaryDump(*ok_empty_s) ?? 0;
    expect_eq(elen, FLIGHT_LOG_DUMP_HEADER_SIZE + FLIGHT_LOG_DUMP_TRAILER_SIZE);
    expect_eq(verifyBinaryDump(ok_empty_dump[0..elen]).status == FlightDumpStatus.ok, true);

    // Oversized payload (257B > 256B) rejected
    const big257: [257]u8 = [0xAA; 257];
    expect_eq(rec.recordEvent(1, 1, 0, big257[0..]) == FlightRecordStatus.payload_too_large, true);
    expect_eq(rec.stats().oversized_rejects, 1);

    // Exact 256B payload succeeds
    const exact256: [256]u8 = [0x55; 256];
    expect_eq(rec.recordEvent(2, 1, 0, exact256[0..]) == FlightRecordStatus.ok, true);
    expect_eq(rec.snapshotEvents(0, 0, *evs_s), 1);
    expect_eq(evs[0].payload_len, 256);

    expect_eq(FLIGHT_LOG_RING_BYTES, 8192);
    expect_eq(FLIGHT_LOG_MAX_PAYLOAD, 256);
    expect_eq(FLIGHT_LOG_RECORD_HEADER_SIZE, 22);
    expect_eq(FLIGHT_LOG_DUMP_MAGIC, 0x314C4643);
}

// ============================================================================
// Test 8: clear() Reset Preserving Monotonic next_seq_id
// ============================================================================

fn testClearPreservesMonotonicSeqId()! {
    let rec = FlightRecorder.init();
    * = rec.recordEvent(1, 1, 0, "a");
    * = rec.recordEvent(2, 1, 0, "b");
    * = rec.recordEvent(3, 1, 0, "c");

    expect_eq(rec.clear(), true);
    expect_eq(rec.stats().live_events, 0);
    expect_eq(rec.stats().used_bytes, 0);
    expect_eq(rec.stats().next_seq_id, 4);

    * = rec.recordEvent(4, 1, 0, "after_clear");
    let evs: [4]FlightEvent = [FlightEvent{}; 4];
    let evs_s: []FlightEvent = evs[0..];
    expect_eq(rec.snapshotEvents(0, 0, *evs_s), 1);
    expect_eq(evs[0].seq_id, 4);
    expect_eq(bytesEq(evs[0].payloadSlice(), "after_clear"), true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("sys.flightLog: basic record, monotonic seq_id, empty payload & ordered replay", testBasicRecordAndOrderedReplay);
test("sys.flightLog: ring-buffer wrap-around overwrite & exact dropped_count", testRingWrapAroundAndExactDroppedCount);
test("sys.flightLog: incremental after_seq_id & recent-N snapshot filtering", testIncrementalAndRecentNSnapshots);
test("sys.flightLog: crash freeze/unfreeze latch protecting forensic state", testFreezeUnfreezeForensicLatch);
test("sys.flightLog: CRC32 binary dump export, verify & offline parse across wrap-around", testBinaryDumpExportVerifyAndOfflineParse);
test("sys.flightLog: tamper, truncation, bad magic & bad version dump guards", testBinaryDumpCorruptionGuards);
test("sys.flightLog: oversized payload (>256B), exact 256B fit & empty/small buffer guards", testOversizedPayloadAndEmptyGuards);
test("sys.flightLog: clear() reset preserving monotonic next_seq_id", testClearPreservesMonotonicSeqId);
