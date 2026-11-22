// Origin: Pure-CP Stdlib Test Suite (lib/net/consistent_ring.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Heap Consistent Hash Ring & Multi-Replica Router:
//   1. Deterministic order-independent routing, sorted ring invariant & weighted traffic distribution
//   2. Minimal key migration on node scale-out (~1/N) and exact 100% restoration on scale-in
//   3. Physical-host-deduplicated multi-replica routing across dense virtual nodes
//   4. Google Bounded-Load (`active_load < max_load`) hotspot spillover & unhealthy node failover
//   5. Node removal compaction, slot reuse & full RingStats telemetry
//   6. Empty ring, invalid init, duplicate node_id, vslot capacity & unknown node defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    hashRingKey,
    RingPhysicalNode,
    RingVirtualSlot,
    RingStats,
    ConsistentHashRing,
} from "cap:net";

fn formatTestKey(idx: usize, buf: *[]u8): string {
    // Deterministic ASCII key "shard-key-XXXX" without heap allocation
    const prefix: string = "shard-key-";
    let p: usize = 0;
    while (p < prefix.len) : (p += 1) {
        buf[p] = prefix[p];
    }
    let v: usize = idx;
    buf[p + 0] = ('0' as u8) + (((v / 1000) % 10) as u8);
    buf[p + 1] = ('0' as u8) + (((v / 100) % 10) as u8);
    buf[p + 2] = ('0' as u8) + (((v / 10) % 10) as u8);
    buf[p + 3] = ('0' as u8) + ((v % 10) as u8);
    return buf[0 .. p + 4];
}

// ============================================================================
// Test 1: Deterministic Order-Independent Routing & Weighted Distribution
// ============================================================================

fn testDeterministicAndWeightedRouting()! {
    let nodes_a: [4]RingPhysicalNode = [RingPhysicalNode{}; 4];
    let vslots_a: [256]RingVirtualSlot = [RingVirtualSlot{}; 256];
    let ring_a = cap.net.ConsistentHashRing.init(nodes_a[0..], vslots_a[0..], 32) ?? return error.InitFailed;

    let nodes_b: [4]RingPhysicalNode = [RingPhysicalNode{}; 4];
    let vslots_b: [256]RingVirtualSlot = [RingVirtualSlot{}; 256];
    let ring_b = ConsistentHashRing.init(nodes_b[0..], vslots_b[0..], 32) ?? return error.InitFailed;

    // Insert in opposite orders: Node 1 (weight 3 = 96 vnodes), Node 2 (weight 1 = 32 vnodes)
    expect_eq(ring_a.addNode(1, 3, 0), true);
    expect_eq(ring_a.addNode(2, 1, 0), true);

    expect_eq(ring_b.addNode(2, 1, 0), true);
    expect_eq(ring_b.addNode(1, 3, 0), true);

    expect_eq(ring_a.stats().virtual_slots, 128);
    expect_eq(ring_b.stats().virtual_slots, 128);

    // Verify ring virtual slots are strictly sorted by (token, node_id)
    let s: usize = 1;
    while (s < ring_a.vslot_count) : (s += 1) {
        const prev = vslots_a[s - 1];
        const cur = vslots_a[s];
        const ordered: bool = (prev.token < cur.token) || (prev.token == cur.token && prev.node_id <= cur.node_id);
        expect_eq(ordered, true);
    }

    // Route 400 keys: both rings must agree 100%, and weight-3 node must get significantly more keys
    let key_buf: [24]u8 = [0; 24];
    let hits_n1: usize = 0;
    let hits_n2: usize = 0;
    let i: usize = 0;
    while (i < 400) : (i += 1) {
        const k = formatTestKey(i, key_buf[0..]);
        const dst_a = ring_a.routeKey(k) ?? return error.RouteFailed;
        const dst_b = ring_b.routeKey(k) ?? return error.RouteFailed;
        expect_eq(dst_a, dst_b);
        if (dst_a == 1) {
            hits_n1 += 1;
        } else {
            hits_n2 += 1;
        }
    }

    // Expected ratio 75% vs 25% (300 vs 100); assert generous statistical band (hits_n1 > 220, hits_n2 > 40)
    expect_eq(hits_n1 + hits_n2, 400);
    expect_eq(hits_n1 > 220, true);
    expect_eq(hits_n2 > 40, true);
}

// ============================================================================
// Test 2: Low Key Migration on Scale-Out & Exact Restoration on Scale-In
// ============================================================================

fn testMinimalKeyMigrationOnScaleOutAndIn()! {
    let nodes: [8]RingPhysicalNode = [RingPhysicalNode{}; 8];
    let vslots: [512]RingVirtualSlot = [RingVirtualSlot{}; 512];
    let ring = ConsistentHashRing.init(nodes[0..], vslots[0..], 32) ?? return error.InitFailed;

    expect_eq(ring.addNode(10, 1, 0), true);
    expect_eq(ring.addNode(20, 1, 0), true);
    expect_eq(ring.addNode(30, 1, 0), true);
    expect_eq(ring.addNode(40, 1, 0), true);

    let baseline: [400]u32 = [0; 400];
    let key_buf: [24]u8 = [0; 24];
    let i: usize = 0;
    while (i < 400) : (i += 1) {
        const k = formatTestKey(i, key_buf[0..]);
        baseline[i] = ring.routeKey(k) ?? return error.RouteFailed;
    }

    // Scale out: add 5th node (50). Ideal migration is ~20% (80 keys), and ONLY to node 50!
    expect_eq(ring.addNode(50, 1, 0), true);

    let moved: usize = 0;
    i = 0;
    while (i < 400) : (i += 1) {
        const k = formatTestKey(i, key_buf[0..]);
        const after = ring.routeKey(k) ?? return error.RouteFailed;
        if (after != baseline[i]) {
            // Consistent hashing invariant: adding a node ONLY moves keys to the new node
            expect_eq(after, 50);
            moved += 1;
        }
    }
    expect_eq(moved > 30, true);
    expect_eq(moved < 140, true); // Strictly < 35% migration

    // Scale back in: remove node 50 -> 100% of keys must return to exact baseline node!
    expect_eq(ring.removeNode(50), true);
    i = 0;
    while (i < 400) : (i += 1) {
        const k = formatTestKey(i, key_buf[0..]);
        const restored = ring.routeKey(k) ?? return error.RouteFailed;
        expect_eq(restored, baseline[i]);
    }
}

// ============================================================================
// Test 3: Physical-Host-Deduplicated Multi-Replica Routing
// ============================================================================

fn testDeduplicatedMultiReplicaRouting()! {
    let nodes: [4]RingPhysicalNode = [RingPhysicalNode{}; 4];
    let vslots: [256]RingVirtualSlot = [RingVirtualSlot{}; 256];
    let ring = ConsistentHashRing.init(nodes[0..], vslots[0..], 32) ?? return error.InitFailed;

    expect_eq(ring.addNode(101, 2, 0), true); // 64 vnodes
    expect_eq(ring.addNode(102, 1, 0), true); // 32 vnodes
    expect_eq(ring.addNode(103, 1, 0), true); // 32 vnodes

    let key_buf: [24]u8 = [0; 24];
    let out: [4]u32 = [0; 4];

    let i: usize = 0;
    while (i < 100) : (i += 1) {
        const k = formatTestKey(i, key_buf[0..]);
        const primary = ring.routeKey(k) ?? return error.RouteFailed;

        // Ask for 3 replicas across 3 physical hosts (128 vnodes) -> must always yield 3 distinct hosts
        const n = ring.routeReplicas(k, 3, out[0..]);
        expect_eq(n, 3);
        expect_eq(out[0], primary);
        expect_eq(out[0] != out[1], true);
        expect_eq(out[0] != out[2], true);
        expect_eq(out[1] != out[2], true);

        // Asking for 5 replicas when only 3 physical hosts exist caps cleanly at 3
        const n_capped = ring.routeReplicas(k, 5, out[0..]);
        expect_eq(n_capped, 3);
    }

    // Mark node 102 unhealthy -> routeReplicas must skip 102 and return only 2 distinct hosts
    expect_eq(ring.setHealthy(102, false), true);
    const n_degraded = ring.routeReplicas("agent:workflow:99", 3, out[0..]);
    expect_eq(n_degraded, 2);
    expect_eq(out[0] != 102, true);
    expect_eq(out[1] != 102, true);
    expect_eq(out[0] != out[1], true);
}

// ============================================================================
// Test 4: Google Bounded-Load Hotspot Spillover & Unhealthy Failover
// ============================================================================

fn testBoundedLoadAndHealthFailover()! {
    let nodes: [4]RingPhysicalNode = [RingPhysicalNode{}; 4];
    let vslots: [128]RingVirtualSlot = [RingVirtualSlot{}; 128];
    let ring = ConsistentHashRing.init(nodes[0..], vslots[0..], 16) ?? return error.InitFailed;

    expect_eq(ring.addNode(1, 1, 10), true);
    expect_eq(ring.addNode(2, 1, 10), true);
    expect_eq(ring.addNode(3, 1, 10), true);

    const hot_key: string = "hot-tenant:llm-kv-prefix:alpha";
    let order: [3]u32 = [0; 3];
    expect_eq(ring.routeReplicas(hot_key, 3, order[0..]), 3);
    const n0 = order[0];
    const n1 = order[1];
    const n2 = order[2];

    expect_eq(ring.routeKey(hot_key) ?? 0, n0);

    // Load n0 to 9 (< max_load 10): still routes to n0
    expect_eq(ring.setLoad(n0, 9), true);
    expect_eq(ring.routeKey(hot_key) ?? 0, n0);

    // Load n0 to 10 (== max_load 10): Bounded-Load spills over clockwise to n1!
    expect_eq(ring.setLoad(n0, 10), true);
    expect_eq(ring.routeKey(hot_key) ?? 0, n1);

    // Mark n1 unhealthy: spills over clockwise to n2!
    expect_eq(ring.setHealthy(n1, false), true);
    expect_eq(ring.routeKey(hot_key) ?? 0, n2);

    // Overload n2 too: all nodes unavailable -> routeKey returns null, routeReplicas returns 0
    expect_eq(ring.setLoad(n2, 12), true);
    expect_eq(ring.routeKey(hot_key) == null, true);
    expect_eq(ring.routeReplicas(hot_key, 3, order[0..]), 0);

    const st = ring.stats();
    expect_eq(st.physical_nodes, 3);
    expect_eq(st.healthy_nodes, 2);
    expect_eq(st.overloaded_nodes, 2);
    expect_eq(st.total_load, 22);

    // Recover n0 load -> primary routing immediately snaps back to n0
    expect_eq(ring.setLoad(n0, 3), true);
    expect_eq(ring.routeKey(hot_key) ?? 0, n0);
}

// ============================================================================
// Test 5: Node Removal Compaction, Slot Reuse & Telemetry
// ============================================================================

fn testRemovalCompactionAndSlotReuse()! {
    let nodes: [2]RingPhysicalNode = [RingPhysicalNode{}; 2];
    let vslots: [32]RingVirtualSlot = [RingVirtualSlot{}; 32];
    let ring = ConsistentHashRing.init(nodes[0..], vslots[0..], 16) ?? return error.InitFailed;

    expect_eq(ring.addNode(1, 1, 50), true);
    expect_eq(ring.addNode(2, 1, 50), true);
    // Both physical & virtual buffers are now 100% full
    expect_eq(ring.addNode(3, 1, 50), false);
    expect_eq(ring.stats().virtual_slots, 32);

    // Remove node 1 -> frees 1 physical slot + 16 virtual slots
    expect_eq(ring.removeNode(1), true);
    expect_eq(ring.getNode(1) == null, true);
    expect_eq(ring.stats().physical_nodes, 1);
    expect_eq(ring.stats().virtual_slots, 16);

    // Now node 3 can be added reusing the reclaimed slots
    expect_eq(ring.addNode(3, 1, 50), true);
    expect_eq(ring.stats().physical_nodes, 2);
    expect_eq(ring.stats().virtual_slots, 32);
    expect_eq(ring.getNode(3) != null, true);

    // Verify routeKey works cleanly after compaction + reuse
    const dst = ring.routeKey("post-compaction-key") ?? return error.RouteFailed;
    expect_eq(dst == 2 || dst == 3, true);
}

// ============================================================================
// Test 6: Defensive Boundary & Invalid Input Guards
// ============================================================================

fn testDefensiveBoundaryGuards()! {
    let nodes: [2]RingPhysicalNode = [RingPhysicalNode{}; 2];
    let vslots: [16]RingVirtualSlot = [RingVirtualSlot{}; 16];
    let empty_nodes: []RingPhysicalNode = nodes[0..0];
    let empty_vslots: []RingVirtualSlot = vslots[0..0];

    // Invalid init parameters
    expect_eq(ConsistentHashRing.init(empty_nodes, vslots[0..], 8) == null, true);
    expect_eq(ConsistentHashRing.init(nodes[0..], empty_vslots, 8) == null, true);
    expect_eq(ConsistentHashRing.init(nodes[0..], vslots[0..], 0) == null, true);

    let ring = ConsistentHashRing.init(nodes[0..], vslots[0..], 8) ?? return error.InitFailed;

    // Empty ring routing returns null / 0
    expect_eq(ring.routeKey("any-key") == null, true);
    let out: [2]u32 = [0; 2];
    expect_eq(ring.routeReplicas("any-key", 2, out[0..]), 0);

    // Zero weight rejected
    expect_eq(ring.addNode(1, 0, 10), false);
    // Weight exceeding vslot capacity rejected (weight 3 * 8 = 24 > 16)
    expect_eq(ring.addNode(1, 3, 10), false);

    // Valid add + duplicate node_id rejected
    expect_eq(ring.addNode(1, 1, 10), true);
    expect_eq(ring.addNode(1, 1, 10), false);

    // Unknown node operations return false / null
    expect_eq(ring.removeNode(999), false);
    expect_eq(ring.setHealthy(999, false), false);
    expect_eq(ring.setLoad(999, 5), false);
    expect_eq(ring.getNode(999) == null, true);

    // Zero replica_count or empty out_node_ids safely returns 0
    expect_eq(ring.routeReplicas("k", 0, out[0..]), 0);
    let empty_out: []u32 = out[0..0];
    expect_eq(ring.routeReplicas("k", 2, empty_out), 0);

    // hashRingKey determinism sanity check
    expect_eq(hashRingKey("hello-cp"), hashRingKey("hello-cp"));
    expect_eq(hashRingKey("hello-cp") != hashRingKey("hello-cp-2"), true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("net/consistent_ring: deterministic order-independent routing & weighted distribution", testDeterministicAndWeightedRouting);
test("net/consistent_ring: minimal key migration on scale-out & exact restoration on scale-in", testMinimalKeyMigrationOnScaleOutAndIn);
test("net/consistent_ring: physical-host-deduplicated multi-replica routing", testDeduplicatedMultiReplicaRouting);
test("net/consistent_ring: Google Bounded-Load spillover & unhealthy failover", testBoundedLoadAndHealthFailover);
test("net/consistent_ring: node removal compaction, slot reuse & RingStats", testRemovalCompactionAndSlotReuse);
test("net/consistent_ring: empty ring, invalid init, duplicates & capacity guards", testDefensiveBoundaryGuards);
