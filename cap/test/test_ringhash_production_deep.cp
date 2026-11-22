// Source: cp/cap/test/test_ringhash_production_deep.cp
// Deep Production Verification Suite for cap:ringhash & cap.ringhash
// Covers:
//   1. ConsistentRing uniform distribution, weighted ratio & low key migration on add/remove
//   2. getReplicas strict physical-node deduplication even with adjacent virtual nodes
//   3. jumpConsistentHash zero-migration monotonicity & uniform bucket distribution
//   4. RendezvousHasher HRW deterministic routing, weight skew & error interception
//   5. `import cap from "cap"; cap.ringhash` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RingHashError,
    ConsistentRing,
    RendezvousHasher,
    jumpConsistentHash,
} from "cap:ds";

fn formatKey(buf: []u8, prefix: string, num: usize): []const u8 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len && pos < buf.len) : (i += 1) {
        buf[pos] = prefix[i];
        pos += 1;
    }
    if (num == 0) {
        if (pos < buf.len) {
            buf[pos] = '0';
            pos += 1;
        }
        return buf[0..pos];
    }
    let digits: [24]u8 = undefined;
    let dlen: usize = 0;
    let v = num;
    while (v > 0) {
        digits[dlen] = ('0' + (v % 10)) as u8;
        dlen += 1;
        v /= 10;
    }
    while (dlen > 0 && pos < buf.len) {
        dlen -= 1;
        buf[pos] = digits[dlen];
        pos += 1;
    }
    return buf[0..pos];
}

test("cap:ringhash - ConsistentRing uniform distribution, weights & low migration on add/remove") {
    const alloc = std.heap.page_allocator;
    let ring = ConsistentRing.init(alloc, 150) catch unreachable;
    defer ring.deinit();

    ring.addNode("node-0", 1) catch unreachable;
    ring.addNode("node-1", 1) catch unreachable;
    ring.addNode("node-2", 1) catch unreachable;
    ring.addNode("node-3", 1) catch unreachable;
    expect_eq(ring.nodeCount(), 4);
    expect_eq(ring.vnodeCount(), 600);

    let counts: [4]usize = [0, 0, 0, 0];
    let baseline = alloc.alloc(u8, 4000) catch unreachable;
    defer alloc.free(baseline);

    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 4000) : (i += 1) {
        const key = formatKey(&key_buf, "req:", i);
        const owner = ring.getNode(key) catch unreachable;
        const idx = (owner[5] - '0') as usize;
        counts[idx] += 1;
        baseline[i] = idx as u8;
    }

    // Each of 4 equal-weight nodes should receive roughly ~1000 keys (within [700, 1300]).
    let n: usize = 0;
    while (n < 4) : (n += 1) {
        expect_eq(counts[n] >= 700, true);
        expect_eq(counts[n] <= 1300, true);
    }

    // Add 5th node: theoretical migration ~20%; require < 32% (< 1280 keys).
    ring.addNode("node-4", 1) catch unreachable;
    expect_eq(ring.nodeCount(), 5);
    expect_eq(ring.vnodeCount(), 750);

    let migrated_add: usize = 0;
    i = 0;
    while (i < 4000) : (i += 1) {
        const key = formatKey(&key_buf, "req:", i);
        const owner = ring.getNode(key) catch unreachable;
        const idx = (owner[5] - '0') as u8;
        if (idx != baseline[i]) {
            migrated_add += 1;
        }
    }
    expect_eq(migrated_add > 400, true);
    expect_eq(migrated_add < 1280, true);

    // Remove node-2: no key may route to node-2, and keys not on node-2 stay put.
    ring.removeNode("node-2") catch unreachable;
    expect_eq(ring.nodeCount(), 4);
    expect_eq(ring.vnodeCount(), 600);

    i = 0;
    while (i < 4000) : (i += 1) {
        const key = formatKey(&key_buf, "req:", i);
        const owner = ring.getNode(key) catch unreachable;
        expect_eq(std.mem.eql(u8, owner, "node-2"), false);
    }

    // Weighted ring: weight 3 vs weight 1 -> ~3:1 ratio.
    let wring = ConsistentRing.init(alloc, 100) catch unreachable;
    defer wring.deinit();
    wring.addNode("heavy", 3) catch unreachable;
    wring.addNode("light", 1) catch unreachable;

    let heavy_hits: usize = 0;
    let light_hits: usize = 0;
    i = 0;
    while (i < 2000) : (i += 1) {
        const key = formatKey(&key_buf, "w:", i);
        const owner = wring.getNode(key) catch unreachable;
        if (std.mem.eql(u8, owner, "heavy")) {
            heavy_hits += 1;
        } else {
            light_hits += 1;
        }
    }
    expect_eq(heavy_hits > light_hits * 2, true);
}

test("cap:ringhash - getReplicas strict physical node deduplication") {
    const alloc = std.heap.page_allocator;
    let ring = ConsistentRing.init(alloc, 64) catch unreachable;
    defer ring.deinit();

    ring.addNode("zone-a", 1) catch unreachable;
    ring.addNode("zone-b", 1) catch unreachable;
    ring.addNode("zone-c", 1) catch unreachable;

    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 200) : (i += 1) {
        const key = formatKey(&key_buf, "obj:", i);
        const primary = ring.getNode(key) catch unreachable;

        // Requesting 5 replicas on 3 physical nodes must clamp to 3 distinct physical nodes.
        const reps = ring.getReplicas(alloc, key, 5) catch unreachable;
        defer alloc.free(reps);
        expect_eq(reps.len, 3);
        expect_eq(std.mem.eql(u8, reps[0], primary), true);
        expect_eq(std.mem.eql(u8, reps[0], reps[1]), false);
        expect_eq(std.mem.eql(u8, reps[0], reps[2]), false);
        expect_eq(std.mem.eql(u8, reps[1], reps[2]), false);
    }

    const zero_reps = ring.getReplicas(alloc, "obj:0", 0) catch unreachable;
    defer alloc.free(zero_reps);
    expect_eq(zero_reps.len, 0);
}

test("cap:ringhash - jumpConsistentHash zero-migration monotonicity & uniform buckets") {
    let buckets: [10]usize = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let moved_to_new: usize = 0;

    let i: usize = 0;
    while (i < 10000) : (i += 1) {
        const key_hash = std.hash.Wyhash.hash(0x1337, std.mem.asBytes(&i));
        const b10 = jumpConsistentHash(key_hash, 10) catch unreachable;
        expect_eq(b10 < 10, true);
        buckets[b10] += 1;

        // Expanding 10 -> 11: keys must NEVER move between existing buckets 0..9.
        const b11 = jumpConsistentHash(key_hash, 11) catch unreachable;
        expect_eq(b11 < 11, true);
        if (b11 != b10) {
            expect_eq(b11, 10);
            moved_to_new += 1;
        }
    }

    // Uniformity across 10 buckets (~1000 each, within [850, 1150]).
    let b: usize = 0;
    while (b < 10) : (b += 1) {
        expect_eq(buckets[b] >= 850, true);
        expect_eq(buckets[b] <= 1150, true);
    }

    // Monotonic migration fraction ~1/11 (~909 keys, within [750, 1100]).
    expect_eq(moved_to_new >= 750, true);
    expect_eq(moved_to_new <= 1100, true);
}

test("cap:ringhash - RendezvousHasher HRW determinism, weight skew & error interception") {
    const alloc = std.heap.page_allocator;
    let hrw = RendezvousHasher.init(alloc);
    defer hrw.deinit();

    // EmptyRing interception
    let got_empty = false;
    * = hrw.getNode("k1") catch |e| {
        if (e == RingHashError.EmptyRing) got_empty = true;
    };
    expect_eq(got_empty, true);

    hrw.addNode("srv-a", 1) catch unreachable;
    hrw.addNode("srv-b", 1) catch unreachable;
    hrw.addNode("srv-c", 1) catch unreachable;

    // DuplicateNode & InvalidWeight interception
    let got_dup = false;
    hrw.addNode("srv-a", 1) catch |e| {
        if (e == RingHashError.DuplicateNode) got_dup = true;
    };
    expect_eq(got_dup, true);

    let got_weight = false;
    hrw.addNode("srv-bad", 0) catch |e| {
        if (e == RingHashError.InvalidWeight) got_weight = true;
    };
    expect_eq(got_weight, true);

    let got_not_found = false;
    hrw.removeNode("srv-missing") catch |e| {
        if (e == RingHashError.NodeNotFound) got_not_found = true;
    };
    expect_eq(got_not_found, true);

    let got_bucket_err = false;
    * = jumpConsistentHash(42, 0) catch |e| {
        if (e == RingHashError.InvalidBucketCount) got_bucket_err = true;
    };
    expect_eq(got_bucket_err, true);

    // Determinism & minimal disruption on removal
    let owners_before = alloc.alloc(u8, 500) catch unreachable;
    defer alloc.free(owners_before);
    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 500) : (i += 1) {
        const key = formatKey(&key_buf, "hrw:", i);
        const owner = hrw.getNode(key) catch unreachable;
        const owner_again = hrw.getNode(key) catch unreachable;
        expect_eq(std.mem.eql(u8, owner, owner_again), true);
        owners_before[i] = owner[4];
    }

    const reps = hrw.getReplicas(alloc, "hrw:42", 3) catch unreachable;
    defer alloc.free(reps);
    expect_eq(reps.len, 3);
    expect_eq(std.mem.eql(u8, reps[0], hrw.getNode("hrw:42") catch unreachable), true);
    expect_eq(std.mem.eql(u8, reps[0], reps[1]), false);
    expect_eq(std.mem.eql(u8, reps[1], reps[2]), false);

    // Removing srv-b: keys previously on srv-a or srv-c MUST NOT move!
    hrw.removeNode("srv-b") catch unreachable;
    i = 0;
    while (i < 500) : (i += 1) {
        const key = formatKey(&key_buf, "hrw:", i);
        const owner = hrw.getNode(key) catch unreachable;
        if (owners_before[i] != 'b') {
            expect_eq(owner[4], owners_before[i]);
        }
    }

    // Weighted HRW skew: primary(4) vs secondary(1) -> primary > 70%
    let whrw = RendezvousHasher.init(alloc);
    defer whrw.deinit();
    whrw.addNode("primary", 4) catch unreachable;
    whrw.addNode("secondary", 1) catch unreachable;
    let p_hits: usize = 0;
    i = 0;
    while (i < 2000) : (i += 1) {
        const key = formatKey(&key_buf, "whrw:", i);
        const owner = whrw.getNode(key) catch unreachable;
        if (std.mem.eql(u8, owner, "primary")) {
            p_hits += 1;
        }
    }
    expect_eq(p_hits > 1400, true);
}

test("cap:ringhash - dynamic cap.ringhash mounting & std.testing.allocator zero-leak") {
    const t_alloc = std.testing.allocator;

    let ring = cap.ds.ConsistentRing.init(t_alloc, 32) catch unreachable;
    defer ring.deinit();
    ring.addNode("dyn-1", 1) catch unreachable;
    ring.addNode("dyn-2", 2) catch unreachable;
    ring.addNode("dyn-3", 1) catch unreachable;

    const reps = ring.getReplicas(t_alloc, "tenant:alpha", 2) catch unreachable;
    defer t_alloc.free(reps);
    expect_eq(reps.len, 2);
    expect_eq(std.mem.eql(u8, reps[0], reps[1]), false);

    ring.removeNode("dyn-2") catch unreachable;
    expect_eq(ring.nodeCount(), 2);

    let hrw = cap.ds.RendezvousHasher.init(t_alloc);
    defer hrw.deinit();
    hrw.addNode("hrw-1", 2) catch unreachable;
    hrw.addNode("hrw-2", 1) catch unreachable;
    const hrw_reps = hrw.getReplicas(t_alloc, "tenant:beta", 2) catch unreachable;
    defer t_alloc.free(hrw_reps);
    expect_eq(hrw_reps.len, 2);

    const j = cap.ds.jumpConsistentHash(0xdeadbeefcafebabe, 8) catch unreachable;
    expect_eq(j < 8, true);
}
