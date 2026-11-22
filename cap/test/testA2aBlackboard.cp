// Origin: Pure-CP Stdlib Test Suite (lib/a2a/blackboard.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Multi-Agent Shared Blackboard & Optimistic CAS Engine:
//   1. Optimistic CAS create-if-absent, version conflict detection (returns latest version) & retry convergence
//   2. Exclusive edit leases: acquire, owner renewal, blocking other writers/deletes, release & auto-expiry
//   3. Entry TTL expiration, lazy eviction on get(), proactive sweepExpired() & slot reclamation
//   4. Monotonic change-log ring: prefix-filtered incremental pollChanges() & ring-wrap lag detection
//   5. Prefix snapshot listing, deleteKey() slot reuse & telemetry stats accuracy
//   6. 64-entry table capacity, invalid key/val/writer guards & cap.a2a namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    BLACKBOARD_MAX_ENTRIES,
    BLACKBOARD_MAX_KEY_LEN,
    BLACKBOARD_MAX_VAL_LEN,
    BLACKBOARD_EVENT_RING_CAP,
    BLACKBOARD_NO_OWNER,
    BLACKBOARD_NO_TTL,
    BlackboardOpStatus,
    BlackboardEventKind,
    BlackboardEntry,
    BlackboardWriteResult,
    BlackboardEvent,
    BlackboardPollResult,
    BlackboardStats,
    SharedBlackboard,
} from "cap:a2a";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("Blackboard: optimistic CAS create-if-absent, conflict feedback & retry convergence") {
    let bb = SharedBlackboard.init();

    // Agent 101 creates key with expected_version = 0 -> version 1
    const r1 = bb.compareAndSwap("task/spec", 0, "v1-draft", 101, BLACKBOARD_NO_TTL, 1000);
    expect_eq(r1.status, BlackboardOpStatus.ok);
    expect_eq(r1.version, 1 as u64);

    // Agent 102 races with expected_version = 0 -> version_conflict, receives latest version = 1
    const r2 = bb.compareAndSwap("task/spec", 0, "v1-other", 102, BLACKBOARD_NO_TTL, 1010);
    expect_eq(r2.status, BlackboardOpStatus.version_conflict);
    expect_eq(r2.version, 1 as u64);

    // Agent 102 retries with latest version 1 -> succeeds, advances to version 2
    const r3 = bb.compareAndSwap("task/spec", r2.version, "v2-reviewed", 102, BLACKBOARD_NO_TTL, 1020);
    expect_eq(r3.status, BlackboardOpStatus.ok);
    expect_eq(r3.version, 2 as u64);

    // Agent 101 with stale version 1 -> conflict, receives latest version 2
    const r4 = bb.compareAndSwap("task/spec", 1, "stale-write", 101, BLACKBOARD_NO_TTL, 1030);
    expect_eq(r4.status, BlackboardOpStatus.version_conflict);
    expect_eq(r4.version, 2 as u64);

    // CAS with non-zero expected_version on absent key -> conflict with version 0
    const r5 = bb.compareAndSwap("task/missing", 3, "x", 101, BLACKBOARD_NO_TTL, 1040);
    expect_eq(r5.status, BlackboardOpStatus.version_conflict);
    expect_eq(r5.version, 0 as u64);

    const entry = bb.get("task/spec", 1050) orelse { expect_true(false); return; };
    expect_eq(entry.version, 2 as u64);
    expect_eq(entry.writer_id, 102 as u64);
    expect_true(bytesEq(entry.valueSlice(), "v2-reviewed"));

    const st = bb.stats();
    expect_eq(st.cas_successes, 2 as u64);
    expect_eq(st.cas_conflicts, 3 as u64);
}

test("Blackboard: exclusive lease acquisition, blocking other agents, renewal & auto-expiry") {
    let bb = SharedBlackboard.init();
    * = bb.put("doc/readme", "init", 1, BLACKBOARD_NO_TTL, 1000);

    // Agent 1 acquires 500ms lease [1000, 1500)
    expect_eq(bb.acquireLease("doc/readme", 1, 500, 1000), BlackboardOpStatus.ok);
    // Agent 1 renews lease to [1200, 2000)
    expect_eq(bb.acquireLease("doc/readme", 1, 800, 1200), BlackboardOpStatus.ok);

    // Agent 2 is blocked from put, CAS, delete, acquireLease & releaseLease
    const b_put = bb.put("doc/readme", "hijack", 2, BLACKBOARD_NO_TTL, 1300);
    expect_eq(b_put.status, BlackboardOpStatus.lease_held_by_other);
    expect_eq(b_put.lease_owner_id, 1 as u64);

    const b_cas = bb.compareAndSwap("doc/readme", 1, "hijack", 2, BLACKBOARD_NO_TTL, 1310);
    expect_eq(b_cas.status, BlackboardOpStatus.lease_held_by_other);

    const b_del = bb.deleteKey("doc/readme", 2, 1320);
    expect_eq(b_del.status, BlackboardOpStatus.lease_held_by_other);

    expect_eq(bb.acquireLease("doc/readme", 2, 500, 1330), BlackboardOpStatus.lease_held_by_other);
    expect_eq(bb.releaseLease("doc/readme", 2, 1340), BlackboardOpStatus.lease_held_by_other);

    // Lease owner (Agent 1) writes freely
    const ok_put = bb.put("doc/readme", "edited-by-1", 1, BLACKBOARD_NO_TTL, 1400);
    expect_eq(ok_put.status, BlackboardOpStatus.ok);
    expect_eq(ok_put.version, 2 as u64);

    // At t=2000, lease has expired -> Agent 2 can now write & lease_expired event is logged!
    const after_exp = bb.put("doc/readme", "edited-by-2", 2, BLACKBOARD_NO_TTL, 2000);
    expect_eq(after_exp.status, BlackboardOpStatus.ok);
    expect_eq(after_exp.version, 3 as u64);

    // Explicit release flow
    expect_eq(bb.acquireLease("doc/readme", 2, 500, 2100), BlackboardOpStatus.ok);
    expect_eq(bb.releaseLease("doc/readme", 2, 2200), BlackboardOpStatus.ok);
    expect_eq(bb.put("doc/readme", "edited-by-3", 3, BLACKBOARD_NO_TTL, 2250).status, BlackboardOpStatus.ok);

    expect_eq(bb.stats().lease_denials, 5 as u64);
}

test("Blackboard: TTL expiration, lazy eviction & proactive sweepExpired reclamation") {
    let bb = SharedBlackboard.init();
    * = bb.put("ephemeral/a", "temp1", 10, 500, 1000); // expires at 1500
    * = bb.put("ephemeral/b", "temp2", 10, 1000, 1000); // expires at 2000
    * = bb.put("persist/c", "forever", 10, BLACKBOARD_NO_TTL, 1000);

    // Before expiry
    expect_true(bb.peek("ephemeral/a", 1499) != null);
    expect_true(bb.get("ephemeral/a", 1499) != null);

    // At t=1500, ephemeral/a expires lazily on get()
    expect_true(bb.get("ephemeral/a", 1500) == null);
    expect_eq(bb.stats().active_keys, 2 as usize);
    expect_eq(bb.stats().expired_swept, 1 as u64);

    // At t=2500, proactive sweepExpired evicts ephemeral/b while preserving persist/c
    const swept: usize = bb.sweepExpired(2500);
    expect_eq(swept, 1 as usize);
    expect_eq(bb.stats().active_keys, 1 as usize);
    expect_eq(bb.stats().expired_swept, 2 as u64);
    expect_true(bb.get("persist/c", 2500) != null);
}

test("Blackboard: prefix-filtered incremental pollChanges & ring-wrap lag detection") {
    let bb = SharedBlackboard.init();
    * = bb.put("plan/1", "a", 1, BLACKBOARD_NO_TTL, 100); // seq 1
    * = bb.put("code/1", "b", 1, BLACKBOARD_NO_TTL, 110); // seq 2
    * = bb.compareAndSwap("plan/1", 1, "a2", 2, BLACKBOARD_NO_TTL, 120); // seq 3
    * = bb.deleteKey("plan/1", 2, 130); // seq 4

    let evs: [8]BlackboardEvent = [BlackboardEvent{}; 8];
    let evs_s: []BlackboardEvent = evs[0..];

    // Poll only "plan/" prefix from seq 0
    const p1 = bb.pollChanges("plan/", 0, *evs_s);
    expect_eq(p1.count, 3 as usize);
    expect_eq(p1.lagged, false);
    expect_eq(p1.latest_seq, 4 as u64);
    expect_eq(evs[0].kind, BlackboardEventKind.put);
    expect_eq(evs[0].seq, 1 as u64);
    expect_eq(evs[1].kind, BlackboardEventKind.cas);
    expect_eq(evs[1].seq, 3 as u64);
    expect_eq(evs[2].kind, BlackboardEventKind.delete);
    expect_eq(evs[2].seq, 4 as u64);

    // Incremental poll from seq 3 yields only seq 4
    const p2 = bb.pollChanges("plan/", 3, *evs_s);
    expect_eq(p2.count, 1 as usize);
    expect_eq(evs[0].seq, 4 as u64);

    // Overflow the 128-event ring buffer -> stale cursor since_seq=0 triggers lagged=true
    let step: usize = 0;
    while (step < BLACKBOARD_EVENT_RING_CAP + 20) : (step += 1) {
        * = bb.put("tick/counter", "x", 1, BLACKBOARD_NO_TTL, (200 + step) as u64);
    }
    const p_lag = bb.pollChanges("tick/", 0, *evs_s);
    expect_true(p_lag.lagged);
    expect_true(p_lag.oldest_available_seq > 1);
    expect_eq(p_lag.count, 8 as usize); // capped by out_events.len
}

test("Blackboard: listKeysByPrefix snapshot, deleteKey & clear() reset") {
    let bb = SharedBlackboard.init();
    * = bb.put("agent/a/state", "idle", 1, BLACKBOARD_NO_TTL, 100);
    * = bb.put("agent/b/state", "busy", 2, BLACKBOARD_NO_TTL, 110);
    * = bb.put("sys/config", "v1", 3, BLACKBOARD_NO_TTL, 120);

    let list_buf: [8]BlackboardEntry = [BlackboardEntry{}; 8];
    let list_s: []BlackboardEntry = list_buf[0..];
    expect_eq(bb.listKeysByPrefix("agent/", 150, *list_s), 2 as usize);
    expect_eq(bb.listKeysByPrefix("", 150, *list_s), 3 as usize);

    expect_eq(bb.deleteKey("agent/a/state", 1, 200).status, BlackboardOpStatus.ok);
    expect_eq(bb.deleteKey("agent/a/state", 1, 210).status, BlackboardOpStatus.not_found);
    expect_eq(bb.listKeysByPrefix("agent/", 220, *list_s), 1 as usize);

    bb.clear();
    expect_eq(bb.stats().active_keys, 0 as usize);
    expect_eq(bb.stats().current_seq, 0 as u64);
}

test("Blackboard: 64-slot table limit, invalid inputs & cap.a2a facade parity") {
    let bb = SharedBlackboard.init();

    // Invalid writer / key / value guards
    expect_eq(bb.put("k", "v", 0, 0, 100).status, BlackboardOpStatus.invalid_writer);
    expect_eq(bb.put("", "v", 1, 0, 100).status, BlackboardOpStatus.invalid_key);
    let big_key: [49]u8 = [0x61; 49];
    expect_eq(bb.put(big_key[0..], "v", 1, 0, 100).status, BlackboardOpStatus.invalid_key);
    let big_val: [257]u8 = [0x62; 257];
    expect_eq(bb.put("k", big_val[0..], 1, 0, 100).status, BlackboardOpStatus.value_too_large);

    // Fill all 64 slots -> 65th returns table_full
    let i: usize = 0;
    while (i < BLACKBOARD_MAX_ENTRIES) : (i += 1) {
        let kbuf: [4]u8 = [0x6B as u8, (0x30 as u8) + ((i / 10) as u8), (0x30 as u8) + ((i % 10) as u8), 0x00 as u8];
        const res = bb.put(kbuf[0..3], "ok", 1, BLACKBOARD_NO_TTL, 100);
        expect_eq(res.status, BlackboardOpStatus.ok);
    }
    expect_eq(bb.stats().active_keys, BLACKBOARD_MAX_ENTRIES);
    expect_eq(bb.put("overflow_key", "x", 1, BLACKBOARD_NO_TTL, 100).status, BlackboardOpStatus.table_full);

    // Verify cap.a2a namespace facade parity
    expect_eq(cap.a2a.BLACKBOARD_MAX_ENTRIES, BLACKBOARD_MAX_ENTRIES);
    expect_eq(cap.a2a.BLACKBOARD_EVENT_RING_CAP, BLACKBOARD_EVENT_RING_CAP);
    expect_eq(cap.a2a.BLACKBOARD_NO_OWNER, BLACKBOARD_NO_OWNER);
}
