// Source: cp/cap/test/test_mvcc_production_deep.cp (Deep production tests for cap:mvcc & cap.mvcc)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { MvccStore, MvccError, TxnState } from "cap:mvcc";
import cap from "cap";

test("mvcc snapshot isolation repeatable read and read-your-own-writes") {
    let store = MvccStore.init(std.testing.allocator);
    defer store.deinit();

    // Seed initial committed state at ts=1
    let t0 = try store.beginTxn(false);
    defer t0.deinit();
    try t0.put("k1", "v1");
    const ts1 = try t0.commit();
    expect_eq(ts1, 1);

    // Start long-running snapshot reader t1 at read_ts=1
    let t1 = try store.beginTxn(true);
    defer t1.deinit();
    expect_eq(t1.read_ts, 1);

    // Concurrent writer t2 modifies k1 and adds k2, uncommitted yet
    let t2 = try store.beginTxn(false);
    defer t2.deinit();
    try t2.put("k1", "v2");
    try t2.put("k2", "new_val");

    // Read-Your-Own-Writes for t2
    expect_eq(std.mem.eql(u8, t2.get("k1").?, "v2"), true);
    expect_eq(std.mem.eql(u8, t2.get("k2").?, "new_val"), true);

    // Uncommitted writes of t2 are invisible to t1
    expect_eq(std.mem.eql(u8, t1.get("k1").?, "v1"), true);
    expect_eq(t1.get("k2") == null, true);

    // Commit t2 (ts=2) -> t1 still sees its snapshot at read_ts=1 (repeatable read)
    const ts2 = try t2.commit();
    expect_eq(ts2, 2);
    expect_eq(std.mem.eql(u8, t1.get("k1").?, "v1"), true);
    expect_eq(t1.get("k2") == null, true);
    * = try t1.commit();

    // New transaction t3 at read_ts=2 sees committed writes from t2
    let t3 = try store.beginTxn(true);
    defer t3.deinit();
    expect_eq(std.mem.eql(u8, t3.get("k1").?, "v2"), true);
    expect_eq(std.mem.eql(u8, t3.get("k2").?, "new_val"), true);
    * = try t3.commit();
}

test("mvcc first-committer-wins write-write conflict detection") {
    let store = MvccStore.init(std.testing.allocator);
    defer store.deinit();

    let seed = try store.beginTxn(false);
    defer seed.deinit();
    try seed.put("balance", "100");
    * = try seed.commit();

    // Two concurrent transactions read at the same snapshot
    let t1 = try store.beginTxn(false);
    defer t1.deinit();
    let t2 = try store.beginTxn(false);
    defer t2.deinit();

    try t1.put("balance", "150");
    try t2.put("balance", "200");

    // t1 commits first -> succeeds
    const ts_t1 = try t1.commit();
    expect_eq(ts_t1, 2);

    // t2 commits second -> WriteConflict and auto-rollback
    let conflict_detected = false;
    t2.commit() catch |e| {
        if (e == MvccError.WriteConflict) {
            conflict_detected = true;
        }
    };
    expect_eq(conflict_detected, true);
    expect_eq(t2.state == TxnState.Aborted, true);

    // Store still holds t1's committed value
    let reader = try store.beginTxn(true);
    defer reader.deinit();
    expect_eq(std.mem.eql(u8, reader.get("balance").?, "150"), true);
    * = try reader.commit();
}

test("mvcc delete tombstone readonly interception and explicit rollback") {
    let store = MvccStore.init(std.testing.allocator);
    defer store.deinit();

    let t0 = try store.beginTxn(false);
    defer t0.deinit();
    try t0.put("item", "active");
    * = try t0.commit();

    // Read-only transaction cannot put or delete
    let ro = try store.beginTxn(true);
    defer ro.deinit();
    let ro_put_err = false;
    ro.put("item", "hack") catch |e| {
        if (e == MvccError.ReadOnlyTxn) {
            ro_put_err = true;
        }
    };
    expect_eq(ro_put_err, true);

    let ro_del_err = false;
    ro.delete("item") catch |e| {
        if (e == MvccError.ReadOnlyTxn) {
            ro_del_err = true;
        }
    };
    expect_eq(ro_del_err, true);
    * = try ro.commit();

    // Explicit rollback discards write_set
    let t_rb = try store.beginTxn(false);
    defer t_rb.deinit();
    try t_rb.put("item", "rolled_back");
    expect_eq(std.mem.eql(u8, t_rb.get("item").?, "rolled_back"), true);
    t_rb.rollback();
    expect_eq(t_rb.state == TxnState.Aborted, true);

    // Delete tombstone semantics
    let t_del = try store.beginTxn(false);
    defer t_del.deinit();
    try t_del.delete("item");
    expect_eq(t_del.get("item") == null, true);
    * = try t_del.commit();

    let t_verify = try store.beginTxn(true);
    defer t_verify.deinit();
    expect_eq(t_verify.get("item") == null, true);
    * = try t_verify.commit();
}

test("mvcc minActiveReadTs and gcVersions safe version pruning") {
    let store = MvccStore.init(std.testing.allocator);
    defer store.deinit();

    // Create v1 (ts=1) and v2 (ts=2)
    let t1 = try store.beginTxn(false);
    defer t1.deinit();
    try t1.put("doc", "v1");
    * = try t1.commit();

    let t2 = try store.beginTxn(false);
    defer t2.deinit();
    try t2.put("doc", "v2");
    * = try t2.commit();

    // Long-running reader pins snapshot at read_ts=2
    let long_reader = try store.beginTxn(true);
    defer long_reader.deinit();
    expect_eq(store.minActiveReadTs(), 2);

    // Create v3 (ts=3) and v4 (ts=4)
    let t3 = try store.beginTxn(false);
    defer t3.deinit();
    try t3.put("doc", "v3");
    * = try t3.commit();

    let t4 = try store.beginTxn(false);
    defer t4.deinit();
    try t4.put("doc", "v4");
    * = try t4.commit();

    expect_eq(store.versionCount("doc"), 4);

    // GC at safe watermark = minActiveReadTs() = 2:
    // v1 (ts=1 < 2) is pruned; v2 (baseline <= 2), v3, v4 are preserved
    const pruned1 = store.gcVersions(store.minActiveReadTs());
    expect_eq(pruned1, 1);
    expect_eq(store.versionCount("doc"), 3);

    // Long-running reader still reads v2 cleanly
    expect_eq(std.mem.eql(u8, long_reader.get("doc").?, "v2"), true);
    * = try long_reader.commit();

    // Now no active transactions: minActiveReadTs() == current_ts == 4
    expect_eq(store.minActiveReadTs(), 4);
    const pruned2 = store.gcVersions(store.minActiveReadTs());
    expect_eq(pruned2, 2);
    expect_eq(store.versionCount("doc"), 1);

    let final_reader = try store.beginTxn(true);
    defer final_reader.deinit();
    expect_eq(std.mem.eql(u8, final_reader.get("doc").?, "v4"), true);
    * = try final_reader.commit();
}

test("mvcc cap.mvcc dynamic mount and zero-leak check") {
    let store = cap.mvcc.MvccStore.init(std.testing.allocator);
    defer store.deinit();

    let txn = try store.beginTxn(false);
    defer txn.deinit();
    try txn.put("mount_key", "mount_val");
    const ts = try txn.commit();
    expect_eq(ts, 1);

    let r = try store.beginTxn(true);
    defer r.deinit();
    expect_eq(std.mem.eql(u8, r.get("mount_key").?, "mount_val"), true);
    * = try r.commit();
}
