// Source: native-cp-stdlib (State-Based CRDTs: GCounter, PNCounter, GSet, LWW)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { GCounter, PNCounter, LWWRegister, GSet } from "cap:ds";
import cap from "cap";

test("GCounter: multi-node concurrent increments and semilattice merge properties") {
    const alloc = std.testing.allocator;

    let a = GCounter.init(alloc);
    defer a.deinit();
    let b = GCounter.init(alloc);
    defer b.deinit();
    let c = GCounter.init(alloc);
    defer c.deinit();

    try a.increment(1, 5);
    try a.increment(2, 3);
    try b.increment(2, 7);
    try b.increment(3, 4);
    try c.increment(1, 2);
    try c.increment(4, 10);

    // Commutativity: (a merge b) == (b merge a)
    let ab = GCounter.init(alloc);
    defer ab.deinit();
    try ab.merge(a);
    try ab.merge(b);

    let ba = GCounter.init(alloc);
    defer ba.deinit();
    try ba.merge(b);
    try ba.merge(a);

    try expect_eq(ab.value(), 16); // node1:5 + node2:7 + node3:4
    try expect_eq(ab.value(), ba.value());

    // Idempotency: merge(ab, ab) == ab
    try ab.merge(ab);
    try expect_eq(ab.value(), 16);

    // Associativity: ((a merge b) merge c) == (a merge (b merge c))
    let abc_left = GCounter.init(alloc);
    defer abc_left.deinit();
    try abc_left.merge(ab);
    try abc_left.merge(c);

    let bc = GCounter.init(alloc);
    defer bc.deinit();
    try bc.merge(b);
    try bc.merge(c);

    let abc_right = GCounter.init(alloc);
    defer abc_right.deinit();
    try abc_right.merge(a);
    try abc_right.merge(bc);

    try expect_eq(abc_left.value(), 26); // 5 + 7 + 4 + 10
    try expect_eq(abc_left.value(), abc_right.value());
}

test("PNCounter: increments, decrements, and cross-partition convergence") {
    const alloc = std.testing.allocator;

    let p1 = PNCounter.init(alloc);
    defer p1.deinit();
    let p2 = PNCounter.init(alloc);
    defer p2.deinit();

    try p1.increment(1, 20);
    try p1.decrement(1, 5);
    try p2.increment(2, 10);
    try p2.decrement(2, 30);

    try expect_eq(p1.value(), 15);
    try expect_eq(p2.value(), -20);

    // Cross-partition merge in both directions
    try p1.merge(p2);
    try p2.merge(p1);

    try expect_eq(p1.value(), -5);
    try expect_eq(p2.value(), -5);
}

test("LWWRegister: total order tie-breaking by (timestamp, node_id)") {
    let r1 = LWWRegister(i64).init(10, 100, 1);
    let r2 = LWWRegister(i64).init(20, 100, 2);
    let r3 = LWWRegister(i64).init(30, 99, 99);

    // Same timestamp: higher node_id wins
    r1.merge(r2);
    try expect_eq(r1.get(), 20);

    // Stale timestamp with higher node_id is ignored
    r1.merge(r3);
    try expect_eq(r1.get(), 20);

    // Newer timestamp wins regardless of node_id
    r1.set(42, 101, 0);
    try expect_eq(r1.get(), 42);
}

test("GSet: deduplicated union convergence") {
    const alloc = std.testing.allocator;

    let s1 = GSet(u64).init(alloc);
    defer s1.deinit();
    let s2 = GSet(u64).init(alloc);
    defer s2.deinit();

    try s1.add(10);
    try s1.add(20);
    try s1.add(10); // duplicate
    try expect_eq(s1.len(), 2);

    try s2.add(20);
    try s2.add(30);
    try s2.add(40);

    try s1.merge(s2);
    try expect_eq(s1.len(), 4);
    try expect_eq(s1.contains(10), true);
    try expect_eq(s1.contains(20), true);
    try expect_eq(s1.contains(30), true);
    try expect_eq(s1.contains(40), true);
    try expect_eq(s1.contains(99), false);
}

test("cap.crdt_pn dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;

    let gc = cap.ds.GCounter.init(alloc);
    defer gc.deinit();
    try gc.increment(7, 12);
    try expect_eq(gc.value(), 12);

    let pn = cap.ds.PNCounter.init(alloc);
    defer pn.deinit();
    try pn.increment(1, 8);
    try pn.decrement(1, 3);
    try expect_eq(pn.value(), 5);

    let gs = cap.ds.GSet(i32).init(alloc);
    defer gs.deinit();
    try gs.add(-1);
    try gs.add(1);
    try expect_eq(gs.len(), 2);

    let reg = cap.ds.LWWRegister(i32).init(5, 1, 1);
    reg.set(9, 2, 1);
    try expect_eq(reg.get(), 9);
}
