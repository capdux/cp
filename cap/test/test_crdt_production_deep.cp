// Source: cp/cap/test/test_crdt_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:crdt & cap.crdt

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CrdtErr,
    CausalOrder,
    VectorClock,
    GCounter,
    PnCounter,
    LwwRegister,
    OrSetTag,
    OrSet,
} from "cap:crdt";

fn testVectorClockCausalOrderingAndMerge(): !void {
    const alloc = std.heap.page_allocator;

    let vc_a = VectorClock.init(alloc);
    defer vc_a.deinit();
    let vc_b = VectorClock.init(alloc);
    defer vc_b.deinit();

    // Empty clocks are Equal
    expect_eq(vc_a.compare(vc_b) == CausalOrder.Equal, true);
    expect_eq(vc_a.compare(vc_b).isEqual(), true);
    expect_eq(vc_a.get("missing"), 0);

    // Ownership-safe node_id copy: mutate caller stack buffer right after increment
    let node_buf: [16]u8 = undefined;
    @memcpy(node_buf[0..7], "agent-a");
    vc_a.increment(node_buf[0..7])?;
    @memcpy(node_buf[0..7], "XXXXXXX");
    expect_eq(vc_a.get("agent-a"), 1);
    expect_eq(vc_a.get("XXXXXXX"), 0);

    // After vs Before
    expect_eq(vc_a.compare(vc_b) == CausalOrder.After, true);
    expect_eq(vc_a.compare(vc_b).isAfter(), true);
    expect_eq(vc_b.compare(vc_a) == CausalOrder.Before, true);
    expect_eq(vc_b.compare(vc_a).isBefore(), true);

    // Clone & advance agent-b => vc_a Before vc_c
    let vc_c = vc_a.clone(alloc)?;
    defer vc_c.deinit();
    vc_c.increment("agent-b")?;
    expect_eq(vc_a.compare(vc_c) == CausalOrder.Before, true);
    expect_eq(vc_c.compare(vc_a) == CausalOrder.After, true);

    // Concurrent: vc_a advances agent-a independently while vc_c advanced agent-b
    vc_a.increment("agent-a")?;
    expect_eq(vc_a.compare(vc_c) == CausalOrder.Concurrent, true);
    expect_eq(vc_a.compare(vc_c).isConcurrent(), true);
    expect_eq(vc_c.compare(vc_a) == CausalOrder.Concurrent, true);

    // Disjoint nodes are also Concurrent
    vc_b.increment("agent-z")?;
    expect_eq(vc_a.compare(vc_b) == CausalOrder.Concurrent, true);

    // Merge convergence & idempotence
    let vc_a_copy = vc_a.clone(alloc)?;
    defer vc_a_copy.deinit();

    vc_a.merge(vc_c)?;
    vc_c.merge(vc_a_copy)?;
    expect_eq(vc_a.compare(vc_c) == CausalOrder.Equal, true);
    expect_eq(vc_a.get("agent-a"), 2);
    expect_eq(vc_a.get("agent-b"), 1);

    // Idempotent self-merge
    vc_a.merge(vc_a)?;
    expect_eq(vc_a.get("agent-a"), 2);
    expect_eq(vc_a.get("agent-b"), 1);

    // Invalid empty node_id guard
    let err_empty = false;
    vc_a.increment("") catch {
        err_empty = true;
    };
    expect_eq(err_empty, true);
}

fn testCountersSemilatticeLaws(): !void {
    const alloc = std.heap.page_allocator;

    // 1. GCounter: 3 replicas with overlapping and disjoint updates
    let g1 = GCounter.init(alloc);
    defer g1.deinit();
    let g2 = GCounter.init(alloc);
    defer g2.deinit();
    let g3 = GCounter.init(alloc);
    defer g3.deinit();

    g1.increment("node-a", 5)?;
    g1.increment("node-b", 2)?;
    g2.increment("node-b", 7)?;
    g2.increment("node-c", 3)?;
    g3.increment("node-a", 1)?;
    g3.increment("node-d", 10)?;
    g3.increment("node-d", 0)?; // zero delta no-op

    expect_eq(g1.value(), 7);
    expect_eq(g2.value(), 10);
    expect_eq(g3.value(), 11);

    // Commutativity: g1 U g2 == g2 U g1
    let g_12 = g1.clone(alloc)?;
    defer g_12.deinit();
    g_12.merge(g2)?;

    let g_21 = g2.clone(alloc)?;
    defer g_21.deinit();
    g_21.merge(g1)?;

    expect_eq(g_12.eql(g_21), true);
    expect_eq(g_12.value(), 15); // 5 + 7 + 3

    // Associativity: (g1 U g2) U g3 == g1 U (g2 U g3)
    let g_left = g_12.clone(alloc)?;
    defer g_left.deinit();
    g_left.merge(g3)?;

    let g_23 = g2.clone(alloc)?;
    defer g_23.deinit();
    g_23.merge(g3)?;

    let g_right = g1.clone(alloc)?;
    defer g_right.deinit();
    g_right.merge(g_23)?;

    expect_eq(g_left.eql(g_right), true);
    expect_eq(g_left.value(), 25); // 5 + 7 + 3 + 10
    expect_eq(g_left.getNode("node-a"), 5);
    expect_eq(g_left.getNode("node-b"), 7);
    expect_eq(g_left.getNode("node-c"), 3);
    expect_eq(g_left.getNode("node-d"), 10);

    // Idempotence: (g1 U g2 U g3) U g2 == g1 U g2 U g3
    g_left.merge(g2)?;
    g_left.merge(g_left)?;
    expect_eq(g_left.eql(g_right), true);
    expect_eq(g_left.value(), 25);

    // 2. PnCounter: negative + positive convergence and algebraic laws
    let p1 = PnCounter.init(alloc);
    defer p1.deinit();
    let p2 = PnCounter.init(alloc);
    defer p2.deinit();
    let p3 = PnCounter.init(alloc);
    defer p3.deinit();

    p1.increment("node-a", 10)?;
    p1.decrement("node-a", 14)?; // -4
    p2.increment("node-b", 6)?;
    p2.decrement("node-c", 2)?; // +4
    p3.increment("node-a", 10)?; // duplicate observation of node-a pos
    p3.decrement("node-b", 5)?; // -5

    expect_eq(p1.value(), -4);
    expect_eq(p2.value(), 4);

    // Commutativity: p1 U p2 == p2 U p1
    let p_12 = p1.clone(alloc)?;
    defer p_12.deinit();
    p_12.merge(p2)?;

    let p_21 = p2.clone(alloc)?;
    defer p_21.deinit();
    p_21.merge(p1)?;
    expect_eq(p_12.eql(p_21), true);
    expect_eq(p_12.value(), 0); // (10 + 6) - (14 + 2) = 0

    // Associativity: (p1 U p2) U p3 == p1 U (p2 U p3)
    let p_left = p_12.clone(alloc)?;
    defer p_left.deinit();
    p_left.merge(p3)?;

    let p_23 = p2.clone(alloc)?;
    defer p_23.deinit();
    p_23.merge(p3)?;

    let p_right = p1.clone(alloc)?;
    defer p_right.deinit();
    p_right.merge(p_23)?;

    expect_eq(p_left.eql(p_right), true);
    expect_eq(p_left.value(), -5); // (10 + 6) - (14 + 2 + 5) = -5

    // Idempotence
    p_left.merge(p3)?;
    p_left.merge(p_left)?;
    expect_eq(p_left.eql(p_right), true);
    expect_eq(p_left.value(), -5);

    // Empty node_id guard
    let err_empty = false;
    g1.increment("", 1) catch {
        err_empty = true;
    };
    expect_eq(err_empty, true);
}

fn testLwwRegisterTieBreakAndConvergence(): !void {
    const alloc = std.heap.page_allocator;

    let r1 = LwwRegister.init(alloc);
    defer r1.deinit();
    expect_eq(r1.get() == null, true);
    expect_eq(r1.writerNode() == null, true);

    // Ownership-safe value & node_id copy
    let v_buf: [16]u8 = undefined;
    let n_buf: [16]u8 = undefined;
    @memcpy(v_buf[0..5], "alpha");
    @memcpy(n_buf[0..6], "node-a");
    r1.set(v_buf[0..5], 500, n_buf[0..6])?;
    @memcpy(v_buf[0..5], "XXXXX");
    @memcpy(n_buf[0..6], "YYYYYY");

    expect_eq(std.mem.eql(u8, r1.get().?, "alpha"), true);
    expect_eq(std.mem.eql(u8, r1.writerNode().?, "node-a"), true);
    expect_eq(r1.timestamp(), 500);

    // Stale write (ts < 500) is ignored
    r1.set("stale", 400, "node-z")?;
    expect_eq(std.mem.eql(u8, r1.get().?, "alpha"), true);

    // Same timestamp (500): "node-b" > "node-a" lexicographically => "node-b" wins
    let r2 = LwwRegister.init(alloc);
    defer r2.deinit();
    r2.set("beta-winner", 500, "node-b")?;

    let r1_clone = r1.clone(alloc)?;
    defer r1_clone.deinit();

    r1.merge(r2)?;
    r2.merge(r1_clone)?;

    expect_eq(std.mem.eql(u8, r1.get().?, "beta-winner"), true);
    expect_eq(std.mem.eql(u8, r2.get().?, "beta-winner"), true);
    expect_eq(std.mem.eql(u8, r1.writerNode().?, "node-b"), true);
    expect_eq(std.mem.eql(u8, r2.writerNode().?, "node-b"), true);

    // Same timestamp (500), lower node_id "node-0" < "node-b" must NOT override
    let r3 = LwwRegister.init(alloc);
    defer r3.deinit();
    r3.set("loser-0", 500, "node-0")?;
    r1.merge(r3)?;
    expect_eq(std.mem.eql(u8, r1.get().?, "beta-winner"), true);

    // Higher timestamp (900) with empty string "" value wins and preserves empty slice
    r3.set("", 900, "node-0")?;
    r1.merge(r3)?;
    r2.merge(r3)?;
    expect_eq(r1.get() != null, true);
    expect_eq(r1.get().?.len, 0);
    expect_eq(r2.get().?.len, 0);
    expect_eq(r1.timestamp(), 900);
}

fn testOrSetAddWinsAndThreeReplicaConvergence(): !void {
    const alloc = std.heap.page_allocator;

    let s1 = OrSet.init(alloc);
    defer s1.deinit();

    // Ownership-safe element & node_id copy
    let e_buf: [16]u8 = undefined;
    @memcpy(e_buf[0..5], "doc-1");
    s1.add(e_buf[0..5], "node-a")?;
    @memcpy(e_buf[0..5], "XXXXX");
    s1.add("doc-2", "node-a")?;

    expect_eq(s1.len(), 2);
    expect_eq(s1.contains("doc-1"), true);
    expect_eq(s1.contains("doc-2"), true);
    expect_eq(s1.contains("XXXXX"), false);

    // Sync baseline state to s2 and s3
    let s2 = OrSet.init(alloc);
    defer s2.deinit();
    s2.merge(s1)?;

    let s3 = OrSet.init(alloc);
    defer s3.deinit();
    s3.merge(s1)?;

    // Concurrent divergent operations across 3 partitioned replicas:
    // - s2 removes "doc-1" (only tombstones tag ("node-a", 1))
    expect_eq(s2.remove("doc-1"), true);
    expect_eq(s2.remove("doc-1"), false);
    expect_eq(s2.contains("doc-1"), false);

    // - s1 concurrently re-adds "doc-1" (fresh tag ("node-a", 3)) and adds "doc-3"
    s1.add("doc-1", "node-a")?;
    s1.add("doc-3", "node-a")?;

    // - s3 removes "doc-2" and adds "doc-4"
    expect_eq(s3.remove("doc-2"), true);
    s3.add("doc-4", "node-c")?;

    // Merge in three different permutations across clones of s1, s2, s3
    let conv_abc = s1.clone(alloc)?;
    defer conv_abc.deinit();
    conv_abc.merge(s2)?;
    conv_abc.merge(s3)?;

    let conv_cba = s3.clone(alloc)?;
    defer conv_cba.deinit();
    conv_cba.merge(s2)?;
    conv_cba.merge(s1)?;

    let conv_bca = s2.clone(alloc)?;
    defer conv_bca.deinit();
    conv_bca.merge(s3)?;
    conv_bca.merge(s1)?;

    // Verify add-wins for "doc-1", observed-remove for "doc-2", and presence of "doc-3", "doc-4"
    expect_eq(conv_abc.len(), 3);
    expect_eq(conv_cba.len(), 3);
    expect_eq(conv_bca.len(), 3);

    expect_eq(conv_abc.contains("doc-1"), true);
    expect_eq(conv_abc.contains("doc-2"), false);
    expect_eq(conv_abc.contains("doc-3"), true);
    expect_eq(conv_abc.contains("doc-4"), true);

    const elems_1 = conv_abc.elements(alloc)?;
    defer alloc.free(elems_1);
    const elems_2 = conv_cba.elements(alloc)?;
    defer alloc.free(elems_2);
    const elems_3 = conv_bca.elements(alloc)?;
    defer alloc.free(elems_3);

    expect_eq(elems_1.len, 3);
    expect_eq(elems_2.len, 3);
    expect_eq(elems_3.len, 3);
    expect_eq(std.mem.eql(u8, elems_1[0], "doc-1"), true);
    expect_eq(std.mem.eql(u8, elems_1[1], "doc-3"), true);
    expect_eq(std.mem.eql(u8, elems_1[2], "doc-4"), true);

    let k: usize = 0;
    while (k < 3) : (k += 1) {
        expect_eq(std.mem.eql(u8, elems_1[k], elems_2[k]), true);
        expect_eq(std.mem.eql(u8, elems_1[k], elems_3[k]), true);
    }

    // Idempotent re-merge
    conv_abc.merge(s2)?;
    conv_abc.merge(conv_abc)?;
    expect_eq(conv_abc.len(), 3);
    expect_eq(conv_abc.contains("doc-1"), true);
    expect_eq(conv_abc.contains("doc-2"), false);
}

fn testCapCrdtDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.crdt.*` automatic dynamic mounting
    let vc = cap.crdt.VectorClock.init(alloc);
    defer vc.deinit();
    vc.increment("hub-1")?;
    expect_eq(vc.get("hub-1"), 1);

    let gc = cap.crdt.GCounter.init(alloc);
    defer gc.deinit();
    gc.increment("hub-1", 42)?;
    expect_eq(gc.value(), 42);

    let pn = cap.crdt.PnCounter.init(alloc);
    defer pn.deinit();
    pn.increment("hub-1", 10)?;
    pn.decrement("hub-2", 15)?;
    expect_eq(pn.value(), -5);

    let reg = cap.crdt.LwwRegister.init(alloc);
    defer reg.deinit();
    reg.set("mounted", 1234, "hub-1")?;
    expect_eq(std.mem.eql(u8, reg.get().?, "mounted"), true);

    let set = cap.crdt.OrSet.init(alloc);
    defer set.deinit();
    set.add("capability-crdt", "hub-1")?;
    expect_eq(set.contains("capability-crdt"), true);
    expect_eq(set.len(), 1);
}

fn main() {
    test("1. VectorClock Causal Ordering (Before/After/Equal/Concurrent) & Merge", () => {
        testVectorClockCausalOrderingAndMerge()?;
    });
    test("2. GCounter & PnCounter Commutativity, Associativity & Idempotence Laws", () => {
        testCountersSemilatticeLaws()?;
    });
    test("3. LwwRegister Deterministic Timestamp + node_id Tie-Break & Convergence", () => {
        testLwwRegisterTieBreakAndConvergence()?;
    });
    test("4. OrSet Concurrent Add-Wins Semantics & Three-Replica Convergence", () => {
        testOrSetAddWinsAndThreeReplicaConvergence()?;
    });
    test("5. cap.crdt Automatic Dynamic Lazy Mounting Verification", () => {
        testCapCrdtDynamicMount()?;
    });
}
