// cap/test/testCapConsistentHash.cp
// Comprehensive unit tests for @cap/consistent-hash

import { test, expect_eq } from "cap:test";
import {
    RingPoint,
    ReplicaList,
    HashRing,
} from "../../packages/@cap/consistent-hash/src/index.cp";

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/consistent-hash — Empty ring returns null & duplicate/missing node guards", () => {
        let ring = HashRing.init(8);
        expect_eq(ring.nodeCount(), 0 as usize);
        expect_eq(ring.vnodeCount(), 0 as usize);
        expect_eq(ring.getNode("any-key") == null, true);
        expect_eq(ring.getReplicas("any-key", 3).count, 0 as usize);

        expect_eq(ring.addNode("node-a"), true);
        expect_eq(ring.addNode("node-a"), false); // duplicate rejected
        expect_eq(ring.addNode(""), false);       // empty rejected
        expect_eq(ring.nodeCount(), 1 as usize);
        expect_eq(ring.vnodeCount(), 8 as usize);

        expect_eq(ring.removeNode("nonexistent"), false);
        expect_eq(ring.removeNode("node-a"), true);
        expect_eq(ring.nodeCount(), 0 as usize);
        expect_eq(ring.vnodeCount(), 0 as usize);
    });

    test("2. @cap/consistent-hash — Deterministic routing independent of insertion order", () => {
        let r1 = HashRing.init(16);
        r1.addNode("cache-1");
        r1.addNode("cache-2");
        r1.addNode("cache-3");

        let r2 = HashRing.init(16);
        r2.addNode("cache-3");
        r2.addNode("cache-1");
        r2.addNode("cache-2");

        expect_eq(r1.getNode("user:1001") ?? "", r2.getNode("user:1001") ?? "");
        expect_eq(r1.getNode("order:9999") ?? "", r2.getNode("order:9999") ?? "");
        expect_eq(r1.getNode("agent:alpha") ?? "", r2.getNode("agent:alpha") ?? "");
    });

    test("3. @cap/consistent-hash — Deduplicated multi-replica failover via getReplicas", () => {
        let ring = HashRing.init(16);
        ring.addNode("shard-0");
        ring.addNode("shard-1");
        ring.addNode("shard-2");

        const reps = ring.getReplicas("tenant:acme:doc:42", 3);
        expect_eq(reps.count, 3 as usize);
        const n0 = reps.nodeAt(0).?;
        const n1 = reps.nodeAt(1).?;
        const n2 = reps.nodeAt(2).?;
        // Primary replica matches getNode, and all 3 replicas are distinct physical nodes
        expect_eq(n0, ring.getNode("tenant:acme:doc:42").?);
        expect_eq(strEq(n0, n1), false);
        expect_eq(strEq(n0, n2), false);
        expect_eq(strEq(n1, n2), false);

        // Requesting more replicas than active nodes clamps cleanly to active_nodes
        const reps_over = ring.getReplicas("tenant:acme:doc:42", 8);
        expect_eq(reps_over.count, 3 as usize);
    });

    test("4. @cap/consistent-hash — Minimal key disruption when adding a 4th node", () => {
        let before = HashRing.init(16);
        before.addNode("n1");
        before.addNode("n2");
        before.addNode("n3");

        let after = HashRing.init(16);
        after.addNode("n1");
        after.addNode("n2");
        after.addNode("n3");
        after.addNode("n4");

        let moved: usize = 0;
        let buf: [16]u8 = ['k', 'e', 'y', '-', '0', '0', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let i: usize = 0;
        while (i < 200) {
            buf[4] = ('0' as usize + (i / 100)) as u8;
            buf[5] = ('0' as usize + ((i / 10) % 10)) as u8;
            buf[6] = ('0' as usize + (i % 10)) as u8;
            const k = buf[0..7];
            if (!strEq(before.getNode(k).?, after.getNode(k).?)) {
                moved += 1;
            }
            i += 1;
        }
        // Ideal migration for 3 -> 4 nodes is ~25% (~50/200); should be well under 50%
        expect_eq(moved > (10 as usize) && moved < (100 as usize), true);
    });

    test("5. @cap/consistent-hash — Node removal redistributes only departed node's keys", () => {
        let ring = HashRing.init(16);
        ring.addNode("n1");
        ring.addNode("n2");
        ring.addNode("n3");

        const k_target = "session:alpha";
        const orig = ring.getNode(k_target).?;
        const to_remove = if (strEq(orig, "n1")) "n2" else "n1";
        expect_eq(ring.removeNode(to_remove), true);
        // Key owned by a surviving node must stay on that exact same node!
        expect_eq(ring.getNode(k_target).?, orig);
    });

    test("6. @cap/consistent-hash — Balanced key distribution across 4 nodes", () => {
        let ring = HashRing.init(32);
        ring.addNode("w0");
        ring.addNode("w1");
        ring.addNode("w2");
        ring.addNode("w3");

        let counts: [4]usize = [0, 0, 0, 0];
        let buf: [8]u8 = ['p', 'k', 't', '-', '0', '0', '0', 0];
        let i: usize = 0;
        while (i < 400) {
            buf[4] = ('0' as usize + (i / 100)) as u8;
            buf[5] = ('0' as usize + ((i / 10) % 10)) as u8;
            buf[6] = ('0' as usize + (i % 10)) as u8;
            const owner = ring.getNode(buf[0..7]).?;
            if (strEq(owner, "w0")) counts[0] += 1;
            else if (strEq(owner, "w1")) counts[1] += 1;
            else if (strEq(owner, "w2")) counts[2] += 1;
            else counts[3] += 1;
            i += 1;
        }
        // Each of the 4 nodes should receive a healthy share (ideal 100 each)
        expect_eq(counts[0] > (40 as usize), true);
        expect_eq(counts[1] > (40 as usize), true);
        expect_eq(counts[2] > (40 as usize), true);
        expect_eq(counts[3] > (40 as usize), true);
    });
}
