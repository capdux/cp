// Origin: Pure-CP Stdlib Test Suite (lib/ds/orSet.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc OR-Set CRDT & LWW-Register64:
//   1. Concurrent add/remove Add-Wins semantics vs sequential causal remove
//   2. CRDT Commutativity `merge(A, B) == merge(B, A)` & Idempotency `merge(A, A) == A`
//   3. Three-replica CRDT Associativity `merge(merge(A, B), C) == merge(A, merge(B, C))`
//   4. Repeated `add` of same element with partial causal `remove` across replicas
//   5. `OrSetCausalContext` version vector ops, capacity/key/replica boundary guards & facade parity
//   6. `LwwRegister64` deterministic `(timestamp_ms, replica_id)` tie-breaking, staleness guard & CRDT laws

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    ORSET_MAX_REPLICAS,
    ORSET_MAX_ENTRIES,
    ORSET_KEY_MAX_LEN,
    OrSetDot,
    OrSetOpStatus,
    OrSetCausalContext,
    OrSetCrdt,
    LwwRegister64,
} from "cap:ds";

test("OrSetCrdt: concurrent add/remove Add-Wins semantics & sequential remove") {
    let a = OrSetCrdt.init(0);
    let b = OrSetCrdt.init(1);

    // 1. Replica A adds "alpha" and syncs to B
    expect_eq(a.add("alpha"), OrSetOpStatus.ok);
    expect_eq(b.merge(a), OrSetOpStatus.ok);
    expect_true(a.contains("alpha"));
    expect_true(b.contains("alpha"));

    // 2. Concurrent partition: A removes "alpha", B concurrently re-adds "alpha"
    expect_true(a.remove("alpha"));
    expect_true(!a.contains("alpha"));
    expect_eq(b.add("alpha"), OrSetOpStatus.ok);
    expect_true(b.contains("alpha"));

    // 3. Merge both ways -> Add-Wins preserves B's unobserved dot on both replicas
    expect_eq(a.merge(b), OrSetOpStatus.ok);
    expect_eq(b.merge(a), OrSetOpStatus.ok);
    expect_true(a.contains("alpha"));
    expect_true(b.contains("alpha"));
    expect_eq(a.dotCount(), @as(usize, 1));
    expect_eq(b.dotCount(), @as(usize, 1));
    expect_true(a.equalsElements(b));

    // 4. Sequential causal remove: now B has observed everything, B removes "alpha", merges into A
    expect_true(b.remove("alpha"));
    expect_true(!b.remove("alpha")); // second remove on absent element returns false
    expect_eq(a.merge(b), OrSetOpStatus.ok);
    expect_true(!a.contains("alpha"));
    expect_true(!b.contains("alpha"));
    expect_eq(a.distinctCount(), @as(usize, 0));
    expect_eq(a.dotCount(), @as(usize, 0));
}

test("OrSetCrdt: CRDT commutativity merge(A,B)==merge(B,A) and idempotency merge(A,A)==A") {
    let a = OrSetCrdt.init(0);
    let b = OrSetCrdt.init(1);

    // Shared prefix
    expect_eq(a.add("shared_keep"), OrSetOpStatus.ok);
    expect_eq(a.add("shared_del"), OrSetOpStatus.ok);
    expect_eq(b.merge(a), OrSetOpStatus.ok);

    // Divergent concurrent operations
    expect_eq(a.add("only_a"), OrSetOpStatus.ok);
    expect_true(a.remove("shared_del"));
    expect_eq(b.add("only_b"), OrSetOpStatus.ok);
    expect_eq(b.add("both_concurrent"), OrSetOpStatus.ok);
    expect_eq(a.add("both_concurrent"), OrSetOpStatus.ok);

    // Commutativity: ab = merge(A, B), ba = merge(B, A)
    let ab = a;
    let ba = b;
    expect_eq(ab.merge(b), OrSetOpStatus.ok);
    expect_eq(ba.merge(a), OrSetOpStatus.ok);

    expect_true(ab.equalsElements(ba));
    expect_eq(ab.distinctCount(), ba.distinctCount());
    expect_eq(ab.dotCount(), ba.dotCount());
    expect_true(ab.context.equals(ba.context));
    expect_true(ab.contains("shared_keep"));
    expect_true(!ab.contains("shared_del"));
    expect_true(ab.contains("only_a"));
    expect_true(ab.contains("only_b"));
    expect_true(ab.contains("both_concurrent"));
    expect_eq(ab.distinctCount(), @as(usize, 4));
    expect_eq(ab.dotCount(), @as(usize, 5)); // "both_concurrent" has 2 concurrent dots

    // Idempotency: merge(ab, ab) == ab
    const before_dots = ab.dotCount();
    const before_distinct = ab.distinctCount();
    const before_ctx = ab.context;
    expect_eq(ab.merge(ab), OrSetOpStatus.ok);
    expect_eq(ab.dotCount(), before_dots);
    expect_eq(ab.distinctCount(), before_distinct);
    expect_true(ab.context.equals(before_ctx));
    expect_true(ab.equalsElements(ba));
}

test("OrSetCrdt: three-replica CRDT associativity merge(merge(A,B),C)==merge(A,merge(B,C))") {
    let a = OrSetCrdt.init(0);
    let b = OrSetCrdt.init(1);
    let c = OrSetCrdt.init(2);

    // Seed shared state across A, B, C
    expect_eq(a.add("k1"), OrSetOpStatus.ok);
    expect_eq(a.add("k2"), OrSetOpStatus.ok);
    expect_eq(b.merge(a), OrSetOpStatus.ok);
    expect_eq(c.merge(a), OrSetOpStatus.ok);

    // Three-way concurrent edits:
    // - A removes k1, adds k3
    // - B removes k2, concurrently re-adds k1
    // - C adds k4, removes k1 (only sees A's k1, NOT B's concurrent k1!)
    expect_true(a.remove("k1"));
    expect_eq(a.add("k3"), OrSetOpStatus.ok);

    expect_true(b.remove("k2"));
    expect_eq(b.add("k1"), OrSetOpStatus.ok);

    expect_eq(c.add("k4"), OrSetOpStatus.ok);
    expect_true(c.remove("k1"));

    // LHS = merge(merge(A, B), C)
    let lhs = a;
    expect_eq(lhs.merge(b), OrSetOpStatus.ok);
    expect_eq(lhs.merge(c), OrSetOpStatus.ok);

    // RHS = merge(A, merge(B, C))
    let bc = b;
    expect_eq(bc.merge(c), OrSetOpStatus.ok);
    let rhs = a;
    expect_eq(rhs.merge(bc), OrSetOpStatus.ok);

    expect_true(lhs.equalsElements(rhs));
    expect_eq(lhs.dotCount(), rhs.dotCount());
    expect_eq(lhs.distinctCount(), rhs.distinctCount());
    expect_true(lhs.context.equals(rhs.context));

    // Expected final set: {k1 (B's concurrent add), k3, k4}; k2 was removed by B
    expect_true(lhs.contains("k1"));
    expect_true(!lhs.contains("k2"));
    expect_true(lhs.contains("k3"));
    expect_true(lhs.contains("k4"));
    expect_eq(lhs.distinctCount(), @as(usize, 3));
}

test("OrSetCrdt: repeated add of same element followed by partial causal remove") {
    let a = OrSetCrdt.init(0);
    let b = OrSetCrdt.init(1);

    // Local repeated add creates 2 dots for 1 distinct element
    expect_eq(a.add("item"), OrSetOpStatus.ok);
    expect_eq(b.merge(a), OrSetOpStatus.ok); // B observes only dot (0, 1)

    // A adds "item" a second time -> dot (0, 2); now A has 2 dots, 1 distinct element
    expect_eq(a.add("item"), OrSetOpStatus.ok);
    expect_eq(a.dotCount(), @as(usize, 2));
    expect_eq(a.distinctCount(), @as(usize, 1));

    // B removes "item" -> removes only observed dot (0, 1)
    expect_true(b.remove("item"));
    expect_true(!b.contains("item"));

    // Merge: (0, 1) is dropped because B observed and removed it, while (0, 2) survives!
    expect_eq(a.merge(b), OrSetOpStatus.ok);
    expect_eq(b.merge(a), OrSetOpStatus.ok);
    expect_true(a.contains("item"));
    expect_true(b.contains("item"));
    expect_eq(a.dotCount(), @as(usize, 1));
    expect_eq(b.dotCount(), @as(usize, 1));
    expect_eq(a.distinctCount(), @as(usize, 1));

    // Now local remove clears the surviving dot completely
    expect_true(a.remove("item"));
    expect_eq(a.dotCount(), @as(usize, 0));
    expect_eq(a.distinctCount(), @as(usize, 0));
}

test("OrSetCausalContext & OrSetCrdt: version vector, boundary guards, stats & facade parity") {
    let ctx1 = OrSetCausalContext.init();
    let ctx2 = OrSetCausalContext.init();

    // Out-of-range replica ID rejected
    expect_true(ctx1.nextDot(@as(u16, @intCast(ORSET_MAX_REPLICAS))) == null);

    const d1 = ctx1.nextDot(0) ?? OrSetDot{};
    const d2 = ctx2.nextDot(1) ?? OrSetDot{};
    expect_eq(d1.seq, @as(u64, 1));
    expect_true(ctx1.hasSeen(d1));
    expect_true(!ctx1.hasSeen(d2));
    expect_true(!ctx1.dominates(ctx2));

    ctx1.merge(ctx2);
    expect_true(ctx1.hasSeen(d2));
    expect_true(ctx1.dominates(ctx2));
    expect_eq(ctx1.trackedReplicas(), @as(usize, 2));

    // Unknown replica guard on OrSetCrdt
    let bad_rep = OrSetCrdt.init(@as(u16, @intCast(ORSET_MAX_REPLICAS)));
    expect_eq(bad_rep.add("x"), OrSetOpStatus.unknown_replica);

    // Key too long guard (> 48 bytes)
    let ok_rep = OrSetCrdt.init(3);
    let long_key: [ORSET_KEY_MAX_LEN + 5]u8 = ['K'; ORSET_KEY_MAX_LEN + 5];
    expect_eq(ok_rep.add(long_key[0..]), OrSetOpStatus.key_too_long);
    expect_eq(ok_rep.dotCount(), @as(usize, 0));

    // Capacity boundary (64 entries): 65th add returns capacity_full with zero side effects
    let i: usize = 0;
    while (i < ORSET_MAX_ENTRIES) : (i += 1) {
        expect_eq(ok_rep.add("e"), OrSetOpStatus.ok);
    }
    expect_eq(ok_rep.dotCount(), ORSET_MAX_ENTRIES);
    expect_eq(ok_rep.add("overflow"), OrSetOpStatus.capacity_full);
    expect_eq(ok_rep.dotCount(), ORSET_MAX_ENTRIES);
    expect_eq(ok_rep.context.clocks[3], @as(u64, ORSET_MAX_ENTRIES));

    // Merge overflow guard: merging 1 new remote dot into full 64-slot set returns capacity_full
    let remote = OrSetCrdt.init(4);
    expect_eq(remote.add("extra"), OrSetOpStatus.ok);
    expect_eq(ok_rep.merge(remote), OrSetOpStatus.capacity_full);
    expect_eq(ok_rep.stats().merge_count, @as(u64, 0));
    expect_eq(ok_rep.stats().replicas_tracked, @as(usize, 1));

    // Verify cap.ds facade parity
    let facade_set = cap.ds.OrSetCrdt.init(2);
    expect_eq(facade_set.add("facade_ok"), cap.ds.OrSetOpStatus.ok);
    expect_true(facade_set.contains("facade_ok"));
}

test("LwwRegister64: deterministic (timestamp_ms, replica_id) tie-breaking & CRDT laws") {
    let r1 = LwwRegister64.init(1);
    let r2 = LwwRegister64.init(2);
    let r3 = LwwRegister64.init(3);

    expect_true(r1.get() == null);

    // Local write and stale-timestamp rejection
    expect_true(r1.set(100, 1000));
    expect_eq(r1.get() ?? 0, @as(u64, 100));
    expect_true(!r1.set(50, 999)); // stale write rejected
    expect_eq(r1.get() ?? 0, @as(u64, 100));

    // Concurrent write at exact same timestamp (1000) from higher replica_id=2 wins deterministically
    expect_true(r2.set(200, 1000));
    let m12 = r1;
    let m21 = r2;
    expect_true(m12.merge(r2));
    expect_true(!m21.merge(r1));
    expect_eq(m12.get() ?? 0, @as(u64, 200));
    expect_eq(m21.get() ?? 0, @as(u64, 200));
    expect_eq(m12.replica_id, @as(u16, 2));
    expect_eq(m21.replica_id, @as(u16, 2));

    // Idempotency
    expect_true(!m12.merge(m12));
    expect_eq(m12.get() ?? 0, @as(u64, 200));

    // Higher timestamp (2000) from lower replica_id=1 beats older timestamp from replica_id=3
    expect_true(r3.set(300, 1500));
    expect_true(r1.set(999, 2000));

    // Associativity: merge(merge(r1, r2), r3) == merge(r1, merge(r2, r3))
    let lhs = r1;
    * = lhs.merge(r2);
    * = lhs.merge(r3);

    let r23 = r2;
    * = r23.merge(r3);
    let rhs = r1;
    * = rhs.merge(r23);

    expect_eq(lhs.get() ?? 0, @as(u64, 999));
    expect_eq(rhs.get() ?? 0, @as(u64, 999));
    expect_eq(lhs.timestamp_ms, rhs.timestamp_ms);
    expect_eq(lhs.replica_id, rhs.replica_id);
}
