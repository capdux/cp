// Source: cp/cap/test/test_cluster_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:cluster & cap.cluster

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ClusterErr,
    RingVNode,
    ConsistentHashRing,
    MemberState,
    MemberInfo,
    MembershipTable,
    LeaseInfo,
    LeaderLease,
} from "cap:cluster";

fn formatIndexedKey(buf: []u8, prefix: []const u8, idx: usize): []const u8 {
    return std.fmt.bufPrint(buf, "{s}{d}", .{ prefix, idx }) catch unreachable;
}

fn nodeIndexOf(id: string): usize {
    if (std.mem.eql(u8, id, "node-0")) return 0;
    if (std.mem.eql(u8, id, "node-1")) return 1;
    if (std.mem.eql(u8, id, "node-2")) return 2;
    if (std.mem.eql(u8, id, "node-3")) return 3;
    if (std.mem.eql(u8, id, "node-4")) return 4;
    if (std.mem.eql(u8, id, "node-5")) return 5;
    return 999;
}

fn testConsistentHashRingDistributionMigrationAndReplicas(): !void {
    const alloc = std.heap.page_allocator;
    let ring = ConsistentHashRing.init(alloc);
    defer ring.deinit();

    // Ownership-safe node ID copy: mutate caller's stack buffer immediately after addNode
    let id_buf: [16]u8 = undefined;
    @memcpy(id_buf[0..6], "node-0");
    ring.addNode(id_buf[0..6], 128)?;
    @memcpy(id_buf[0..6], "XXXXXX");
    expect_eq(ring.hasNode("node-0"), true);
    expect_eq(ring.hasNode("XXXXXX"), false);

    ring.addNode("node-1", 128)?;
    ring.addNode("node-2", 128)?;
    ring.addNode("node-3", 128)?;
    ring.addNode("node-4", 128)?;
    expect_eq(ring.nodeCount(), 5);
    expect_eq(ring.vnodeCount(), 640);

    // Measure distribution across 2000 keys (expected mean = 400 per node)
    const num_keys: usize = 2000;
    let counts: [5]usize = [0; 5];
    let baseline = alloc.alloc(usize, num_keys) catch unreachable;
    defer alloc.free(baseline);

    let key_buf: [48]u8 = undefined;
    let i: usize = 0;
    while (i < num_keys) : (i += 1) {
        const key = formatIndexedKey(&key_buf, "agent:session:", i);
        const owner = ring.getNode(key).?;
        const idx = nodeIndexOf(owner);
        expect_eq(idx < 5, true);
        counts[idx] += 1;
        baseline[i] = idx;
    }

    // With 128 virtual nodes per physical node, each node should hold a balanced shard
    let n: usize = 0;
    while (n < 5) : (n += 1) {
        expect_eq(counts[n] >= 220, true);
        expect_eq(counts[n] <= 580, true);
    }

    // Scale out by adding node-5: only keys moving to node-5 should migrate (~1/6 = 16.7%)
    ring.addNode("node-5", 128)?;
    expect_eq(ring.nodeCount(), 6);
    expect_eq(ring.vnodeCount(), 768);

    let moved_on_add: usize = 0;
    let after_add = alloc.alloc(usize, num_keys) catch unreachable;
    defer alloc.free(after_add);

    i = 0;
    while (i < num_keys) : (i += 1) {
        const key = formatIndexedKey(&key_buf, "agent:session:", i);
        const owner = ring.getNode(key).?;
        const idx = nodeIndexOf(owner);
        expect_eq(idx < 6, true);
        after_add[i] = idx;
        if (idx != baseline[i]) {
            // Consistent hashing guarantee: existing keys only move TO the newly added node
            expect_eq(idx, 5);
            moved_on_add += 1;
        }
    }
    expect_eq(moved_on_add > 100, true);
    expect_eq(moved_on_add < 600, true);

    // Scale in by removing node-2: only keys previously owned by node-2 should migrate
    expect_eq(ring.removeNode("node-2"), true);
    expect_eq(ring.removeNode("node-2"), false);
    expect_eq(ring.nodeCount(), 5);
    expect_eq(ring.vnodeCount(), 640);
    expect_eq(ring.hasNode("node-2"), false);

    let moved_on_remove: usize = 0;
    i = 0;
    while (i < num_keys) : (i += 1) {
        const key = formatIndexedKey(&key_buf, "agent:session:", i);
        const owner = ring.getNode(key).?;
        const idx = nodeIndexOf(owner);
        expect_eq(idx != 2, true);
        if (idx != after_add[i]) {
            expect_eq(after_add[i], 2);
            moved_on_remove += 1;
        }
    }
    expect_eq(moved_on_remove > 100, true);

    // Replica deduplication across virtual nodes
    const primary = ring.getNode("shard-replica-probe-42").?;
    const replicas = ring.getReplicaNodes(alloc, "shard-replica-probe-42", 3)?;
    defer alloc.free(replicas);

    expect_eq(replicas.len, 3);
    expect_eq(std.mem.eql(u8, replicas[0], primary), true);
    expect_eq(std.mem.eql(u8, replicas[0], replicas[1]), false);
    expect_eq(std.mem.eql(u8, replicas[0], replicas[2]), false);
    expect_eq(std.mem.eql(u8, replicas[1], replicas[2]), false);
}

fn testMembershipSwimIncarnationAndTick(): !void {
    const alloc = std.heap.page_allocator;
    let table = MembershipTable.init(alloc);
    defer table.deinit();

    // Ownership-safe node_id and address copying
    let id_buf: [16]u8 = undefined;
    let addr_buf: [32]u8 = undefined;
    @memcpy(id_buf[0..6], "node-a");
    @memcpy(addr_buf[0..13], "10.0.0.1:7001");
    table.upsertSelf(id_buf[0..6], addr_buf[0..13], 1, 1000)?;
    @memcpy(id_buf[0..6], "ZZZZZZ");
    @memcpy(addr_buf[0..13], "99.9.9.9:9999");

    const init_a = table.get("node-a").?;
    expect_eq(init_a.state == MemberState.Alive, true);
    expect_eq(init_a.state.isAlive(), true);
    expect_eq(init_a.incarnation, 1);
    expect_eq(init_a.last_seen_ms, 1000);
    expect_eq(std.mem.eql(u8, init_a.address, "10.0.0.1:7001"), true);

    // Same incarnation: Suspect beats Alive
    table.mergeUpdate(MemberInfo{
        node_id: "node-a",
        address: "10.0.0.1:7001",
        incarnation: 1,
        state: MemberState.Suspect,
        last_seen_ms: 1050,
    })?;
    expect_eq(table.get("node-a").?.state == MemberState.Suspect, true);

    // Same incarnation: stale Alive CANNOT override Suspect
    table.mergeUpdate(MemberInfo{
        node_id: "node-a",
        address: "10.0.0.1:7001",
        incarnation: 1,
        state: MemberState.Alive,
        last_seen_ms: 1100,
    })?;
    expect_eq(table.get("node-a").?.state == MemberState.Suspect, true);

    // Same incarnation: Dead beats Suspect
    table.mergeUpdate(MemberInfo{
        node_id: "node-a",
        address: "10.0.0.1:7001",
        incarnation: 1,
        state: MemberState.Dead,
        last_seen_ms: 1150,
    })?;
    expect_eq(table.get("node-a").?.state == MemberState.Dead, true);

    // Higher incarnation (2) refutes Dead and updates address
    table.upsertSelf("node-a", "10.0.0.1:7002", 2, 1200)?;
    const refuted_a = table.get("node-a").?;
    expect_eq(refuted_a.state == MemberState.Alive, true);
    expect_eq(refuted_a.incarnation, 2);
    expect_eq(refuted_a.last_seen_ms, 1200);
    expect_eq(std.mem.eql(u8, refuted_a.address, "10.0.0.1:7002"), true);

    // Lower incarnation (1) gossip is strictly ignored
    table.mergeUpdate(MemberInfo{
        node_id: "node-a",
        address: "10.0.0.1:7001",
        incarnation: 1,
        state: MemberState.Dead,
        last_seen_ms: 9999,
    })?;
    expect_eq(table.get("node-a").?.state == MemberState.Alive, true);
    expect_eq(table.get("node-a").?.incarnation, 2);

    // Deterministic tick() progression: Alive -> Suspect -> Dead
    table.upsertSelf("node-b", "10.0.0.2:7001", 1, 2000)?;
    table.upsertSelf("node-c", "10.0.0.3:7001", 1, 2400)?;
    table.upsertSelf("node-a", "10.0.0.1:7002", 2, 2450)?;

    // At now_ms = 2499: node-b elapsed = 499 < 500 => all 3 Alive
    table.tick(2499, 500, 1500);
    expect_eq(table.countByState(MemberState.Alive), 3);
    expect_eq(table.countByState(MemberState.Suspect), 0);
    expect_eq(table.countByState(MemberState.Dead), 0);

    // At now_ms = 2500: node-b elapsed = 500 >= 500 => Suspect
    table.tick(2500, 500, 1500);
    expect_eq(table.get("node-b").?.state == MemberState.Suspect, true);
    expect_eq(table.get("node-c").?.state == MemberState.Alive, true);
    expect_eq(table.countByState(MemberState.Alive), 2);
    expect_eq(table.countByState(MemberState.Suspect), 1);

    // Heartbeat refresh for node-a at 3100 keeps node-a Alive; tick at 3500 marks node-b Dead and node-c Suspect
    table.upsertSelf("node-a", "10.0.0.1:7002", 2, 3100)?;
    table.tick(3500, 500, 1500);
    expect_eq(table.get("node-a").?.state == MemberState.Alive, true);
    expect_eq(table.get("node-b").?.state == MemberState.Dead, true);
    expect_eq(table.get("node-c").?.state == MemberState.Suspect, true);
    expect_eq(table.countByState(MemberState.Alive), 1);
    expect_eq(table.countByState(MemberState.Suspect), 1);
    expect_eq(table.countByState(MemberState.Dead), 1);

    const alive = table.aliveMembers(alloc)?;
    defer alloc.free(alive);
    expect_eq(alive.len, 1);
    expect_eq(std.mem.eql(u8, alive[0].node_id, "node-a"), true);
}

fn testLeaderLeaseAcquireRenewTakeoverAndFencing(): !void {
    const alloc = std.heap.page_allocator;
    let lease = LeaderLease.init(alloc);
    defer lease.deinit();

    // Free initially
    expect_eq(lease.currentLeader(1000) == null, true);
    expect_eq(lease.currentFencingToken(), 0);
    expect_eq(lease.validateToken(0), false);
    expect_eq(lease.validateToken(1), false);

    // Ownership-safe candidate_id copy
    let holder_buf: [16]u8 = undefined;
    @memcpy(holder_buf[0..8], "leader-1");
    const l1 = lease.tryAcquire(holder_buf[0..8], 500, 1000).?;
    @memcpy(holder_buf[0..8], "XXXXXXXX");

    expect_eq(std.mem.eql(u8, l1.holder_id, "leader-1"), true);
    expect_eq(l1.fencing_token, 1);
    expect_eq(l1.expires_at_ms, 1500);
    expect_eq(lease.validateToken(1), true);

    // Mutual exclusion while active
    expect_eq(lease.tryAcquire("leader-2", 500, 1200) == null, true);
    expect_eq(lease.tryAcquire("leader-1", 500, 1200) == null, true);
    expect_eq(lease.renew("leader-2", 500, 1200) == null, true);

    // Renew extends expiry while preserving fencing_token
    const l1_renewed = lease.renew("leader-1", 500, 1400).?;
    expect_eq(l1_renewed.fencing_token, 1);
    expect_eq(l1_renewed.expires_at_ms, 1900);
    expect_eq(std.mem.eql(u8, lease.currentLeader(1899).?.holder_id, "leader-1"), true);

    // Expiration boundary at now_ms = 1900
    expect_eq(lease.currentLeader(1900) == null, true);
    expect_eq(lease.renew("leader-1", 500, 1900) == null, true);

    // Takeover by leader-2 strictly increments fencing_token to 2
    const l2 = lease.tryAcquire("leader-2", 600, 1900).?;
    expect_eq(std.mem.eql(u8, l2.holder_id, "leader-2"), true);
    expect_eq(l2.fencing_token, 2);
    expect_eq(l2.expires_at_ms, 2500);

    // Split-brain fencing: stale leader-1 token (1) is rejected; leader-2 token (2) is accepted
    expect_eq(lease.validateToken(1), false);
    expect_eq(lease.validateToken(2), true);
    expect_eq(lease.validateToken(3), true);

    // Voluntary release & monotonic token 3 on next acquire
    expect_eq(lease.releaseLease("leader-1"), false);
    expect_eq(lease.releaseLease("leader-2"), true);
    expect_eq(lease.currentLeader(2000) == null, true);
    expect_eq(lease.validateToken(1), false);

    const l3 = lease.tryAcquire("leader-3", 400, 2000).?;
    expect_eq(l3.fencing_token, 3);
    expect_eq(lease.validateToken(2), false);
    expect_eq(lease.validateToken(3), true);
}

fn testClusterBoundaryGuards(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Empty ConsistentHashRing safety
    let ring = ConsistentHashRing.init(alloc);
    defer ring.deinit();

    expect_eq(ring.nodeCount(), 0);
    expect_eq(ring.vnodeCount(), 0);
    expect_eq(ring.getNode("any-key") == null, true);
    expect_eq(ring.removeNode("missing"), false);

    const empty_rep = ring.getReplicaNodes(alloc, "any-key", 3)?;
    defer alloc.free(empty_rep);
    expect_eq(empty_rep.len, 0);

    // 2. Invalid parameter guards
    let empty_id_err = false;
    ring.addNode("", 10) catch {
        empty_id_err = true;
    };
    expect_eq(empty_id_err, true);

    let zero_vn_err = false;
    ring.addNode("n1", 0) catch {
        zero_vn_err = true;
    };
    expect_eq(zero_vn_err, true);

    // 3. Duplicate node guard preserves original virtual nodes
    ring.addNode("n1", 16)?;
    let dup_err = false;
    ring.addNode("n1", 32) catch {
        dup_err = true;
    };
    expect_eq(dup_err, true);
    expect_eq(ring.nodeCount(), 1);
    expect_eq(ring.vnodeCount(), 16);

    // 4. Out-of-bounds replication_factor (0 and > nodeCount())
    ring.addNode("n2", 16)?;
    const zero_rep = ring.getReplicaNodes(alloc, "key-x", 0)?;
    defer alloc.free(zero_rep);
    expect_eq(zero_rep.len, 0);

    const capped_rep = ring.getReplicaNodes(alloc, "key-x", 100)?;
    defer alloc.free(capped_rep);
    expect_eq(capped_rep.len, 2);
    expect_eq(std.mem.eql(u8, capped_rep[0], capped_rep[1]), false);

    // 5. MembershipTable and LeaderLease boundary guards
    let table = MembershipTable.init(alloc);
    defer table.deinit();

    let empty_member_err = false;
    table.upsertSelf("", "127.0.0.1:8080", 1, 100) catch {
        empty_member_err = true;
    };
    expect_eq(empty_member_err, true);
    expect_eq(table.remove("missing"), false);

    const empty_alive = table.aliveMembers(alloc)?;
    defer alloc.free(empty_alive);
    expect_eq(empty_alive.len, 0);

    let lease = LeaderLease.init(alloc);
    defer lease.deinit();
    expect_eq(lease.tryAcquire("", 100, 100) == null, true);
    expect_eq(lease.tryAcquire("cand", 0, 100) == null, true);
    expect_eq(lease.tryAcquire("cand", -50, 100) == null, true);
    expect_eq(lease.releaseLease("cand"), false);
}

fn testCapClusterAutomaticLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.cluster.*` automatic dynamic mounting
    let hub_ring = cap.cluster.ConsistentHashRing.init(alloc);
    defer hub_ring.deinit();
    hub_ring.addNode("agent-east-1", 32)?;
    hub_ring.addNode("agent-west-1", 32)?;

    const owner = hub_ring.getNode("task:uuid:12345").?;
    const reps = hub_ring.getReplicaNodes(alloc, "task:uuid:12345", 2)?;
    defer alloc.free(reps);
    expect_eq(reps.len, 2);
    expect_eq(std.mem.eql(u8, reps[0], owner), true);

    let hub_table = cap.cluster.MembershipTable.init(alloc);
    defer hub_table.deinit();
    hub_table.upsertSelf("agent-east-1", "10.1.0.1:9000", 1, 500)?;
    hub_table.mergeUpdate(cap.cluster.MemberInfo{
        node_id: "agent-west-1",
        address: "10.2.0.1:9000",
        incarnation: 1,
        state: cap.cluster.MemberState.Alive,
        last_seen_ms: 500,
    })?;
    expect_eq(hub_table.countByState(cap.cluster.MemberState.Alive), 2);

    let hub_lease = cap.cluster.LeaderLease.init(alloc);
    defer hub_lease.deinit();
    const info: cap.cluster.LeaseInfo = hub_lease.tryAcquire(owner, 1000, 500).?;
    expect_eq(info.fencing_token, 1);
    expect_eq(hub_lease.validateToken(info.fencing_token), true);
}

fn main() {
    test("1. ConsistentHashRing Uniform Distribution, Minimal Migration & Replica Deduplication", () => {
        testConsistentHashRingDistributionMigrationAndReplicas()?;
    });
    test("2. MembershipTable SWIM Incarnation CRDT Convergence & Tick Timeouts", () => {
        testMembershipSwimIncarnationAndTick()?;
    });
    test("3. LeaderLease Acquire, Renew, Takeover, Monotonic Fencing Token & Split-Brain Guard", () => {
        testLeaderLeaseAcquireRenewTakeoverAndFencing()?;
    });
    test("4. Empty Ring, Duplicate Node & Out-of-Bounds Replication Factor Boundary Safety", () => {
        testClusterBoundaryGuards()?;
    });
    test("5. cap.cluster Automatic Dynamic Lazy Mounting Verification", () => {
        testCapClusterAutomaticLazyMount()?;
    });
}
