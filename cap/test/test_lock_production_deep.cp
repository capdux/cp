// Source: cp/cap/test/test_lock_production_deep.cp
// Deep Production Test Suite for cap:lock & cap.lock
// Covers:
//   1. NamedLockTable concurrent Shared readers, Exclusive mutual exclusion, reentrancy & NotOwner guard
//   2. Classic AB-BA deadlock detection (`DeadlockDetected`) with wait-edge rollback
//   3. LeaseLockManager TTL expiration preemption & strictly monotonic fencing_token
//   4. renewLease, releaseLease & stale fencing_token rejection via validateToken (split-brain guard)
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.lock`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LockError,
    LockMode,
    WaitForGraph,
    NamedLockTable,
    LeaseToken,
    LeaseLockManager,
} from "cap:db";
import cap from "cap";

fn strEq(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn testNamedLockTableRwReentrancyAndNotOwner(): !void {
    const alloc = std.heap.page_allocator;
    let table = NamedLockTable.init(alloc);
    defer table.deinit();

    // Multiple concurrent Shared readers on the same resource
    expect_eq(table.tryAcquire("res:doc", "reader-1", .Shared)?, true);
    expect_eq(table.tryAcquire("res:doc", "reader-2", .Shared)?, true);
    expect_eq(table.isResourceLocked("res:doc"), true);
    expect_eq(table.holderCount("res:doc"), 2);

    // Exclusive writer is blocked while readers hold the lock
    expect_eq(table.tryAcquire("res:doc", "writer-1", .Exclusive)?, false);

    // Same-owner reentrancy for reader-1
    expect_eq(table.tryAcquire("res:doc", "reader-1", .Shared)?, true);
    expect_eq(table.holderCount("res:doc"), 2);

    // Non-owner unlock must return LockError.NotOwner
    let got_not_owner = false;
    table.unlock("res:doc", "stranger") catch |e| {
        if (e == LockError.NotOwner) {
            got_not_owner = true;
        }
    };
    expect_eq(got_not_owner, true);

    // Release reader-1 twice (reentrant count = 2) and reader-2 once
    table.unlock("res:doc", "reader-1")?;
    expect_eq(table.holderCount("res:doc"), 2);
    table.unlock("res:doc", "reader-1")?;
    expect_eq(table.holderCount("res:doc"), 1);
    table.unlock("res:doc", "reader-2")?;
    expect_eq(table.isResourceLocked("res:doc"), false);
    expect_eq(table.holderCount("res:doc"), 0);

    // Now writer-1 can acquire Exclusive; another reader/writer is blocked
    expect_eq(table.tryAcquire("res:doc", "writer-1", .Exclusive)?, true);
    expect_eq(table.holderCount("res:doc"), 1);
    expect_eq(table.tryAcquire("res:doc", "reader-1", .Shared)?, false);
    expect_eq(table.tryAcquire("res:doc", "writer-2", .Exclusive)?, false);

    // Exclusive reentrancy for writer-1
    expect_eq(table.tryAcquire("res:doc", "writer-1", .Exclusive)?, true);
    table.unlock("res:doc", "writer-1")?;
    expect_eq(table.isResourceLocked("res:doc"), true);
    table.unlock("res:doc", "writer-1")?;
    expect_eq(table.isResourceLocked("res:doc"), false);
}

fn testAbBaDeadlockDetection(): !void {
    const alloc = std.heap.page_allocator;
    let table = NamedLockTable.init(alloc);
    defer table.deinit();

    // AgentA acquires res1; AgentB acquires res2
    expect_eq(table.tryAcquire("res1", "AgentA", .Exclusive)?, true);
    expect_eq(table.tryAcquire("res2", "AgentB", .Exclusive)?, true);

    // AgentA requests res2 -> blocked by AgentB (wait edge AgentA -> AgentB)
    expect_eq(table.tryAcquire("res2", "AgentA", .Exclusive)?, false);
    expect_eq(table.detectCycle(alloc)?, false);

    // AgentB requests res1 -> would create cycle AgentB -> AgentA -> AgentB
    let deadlock_hit = false;
    * = table.tryAcquire("res1", "AgentB", .Exclusive) catch |e| {
        if (e == LockError.DeadlockDetected) {
            deadlock_hit = true;
        }
        false
    };
    expect_eq(deadlock_hit, true);

    // Verify rolled-back wait edge leaves graph cycle-free
    expect_eq(table.detectCycle(alloc)?, false);

    // AgentB releases res2, then AgentA can acquire res2 cleanly
    table.unlock("res2", "AgentB")?;
    expect_eq(table.tryAcquire("res2", "AgentA", .Exclusive)?, true);

    table.unlock("res2", "AgentA")?;
    table.unlock("res1", "AgentA")?;
}

fn testLeaseTtlPreemptionAndMonotonicFencing(): !void {
    const alloc = std.heap.page_allocator;
    let mgr = LeaseLockManager.init(alloc);
    defer mgr.deinit();

    // Acquire initial lease at t=1000, ttl=500 -> expires at 1500
    const tok1 = mgr.acquireLease("shard:0", "node-a", 500, 1000)?;
    expect_eq(strEq(tok1.resource, "shard:0"), true);
    expect_eq(strEq(tok1.owner, "node-a"), true);
    expect_eq(tok1.fencing_token >= 1, true);
    expect_eq(tok1.expires_at_ms, 1500);

    // Preemption before expiration (t=1499) must fail with LockBusy
    let busy = false;
    * = mgr.acquireLease("shard:0", "node-b", 500, 1499) catch |e| {
        if (e == LockError.LockBusy) {
            busy = true;
        }
        tok1
    };
    expect_eq(busy, true);

    // Preemption at exact expiration (t=1500) succeeds with strictly higher fencing_token
    const tok2 = mgr.acquireLease("shard:0", "node-b", 1000, 1500)?;
    expect_eq(strEq(tok2.owner, "node-b"), true);
    expect_eq(tok2.fencing_token > tok1.fencing_token, true);
    expect_eq(tok2.expires_at_ms, 2500);

    // Old token is rejected by validateToken even while tok2 is active
    expect_eq(mgr.validateToken("shard:0", tok1.fencing_token, 1600), false);
    expect_eq(mgr.validateToken("shard:0", tok2.fencing_token, 1600), true);
}

fn testLeaseRenewReleaseAndSplitBrainGuard(): !void {
    const alloc = std.heap.page_allocator;
    let mgr = LeaseLockManager.init(alloc);
    defer mgr.deinit();

    const tok1 = mgr.acquireLease("leader:raft", "primary-1", 1000, 100)?;
    expect_eq(mgr.validateToken("leader:raft", tok1.fencing_token, 500), true);

    // Renew with valid owner + fencing_token before expiry (t=800, ttl=1000 -> 1800)
    const renewed = mgr.renewLease("leader:raft", "primary-1", tok1.fencing_token, 1000, 800)?;
    expect_eq(renewed.fencing_token, tok1.fencing_token);
    expect_eq(renewed.expires_at_ms, 1800);
    expect_eq(mgr.validateToken("leader:raft", tok1.fencing_token, 1500), true);

    // Wrong owner renew must fail with NotOwner
    let wrong_owner_renew = false;
    * = mgr.renewLease("leader:raft", "rogue-node", tok1.fencing_token, 500, 1500) catch |e| {
        if (e == LockError.NotOwner) {
            wrong_owner_renew = true;
        }
        tok1
    };
    expect_eq(wrong_owner_renew, true);

    // Release by wrong owner fails with NotOwner
    let wrong_owner_release = false;
    mgr.releaseLease("leader:raft", "rogue-node", tok1.fencing_token) catch |e| {
        if (e == LockError.NotOwner) {
            wrong_owner_release = true;
        }
    };
    expect_eq(wrong_owner_release, true);

    // Release by valid owner succeeds and invalidates token
    mgr.releaseLease("leader:raft", "primary-1", tok1.fencing_token)?;
    expect_eq(mgr.validateToken("leader:raft", tok1.fencing_token, 1500), false);

    // Re-acquire by primary-2 produces higher fencing_token; stale tok1 renew is rejected
    const tok2 = mgr.acquireLease("leader:raft", "primary-2", 1000, 1600)?;
    expect_eq(tok2.fencing_token > tok1.fencing_token, true);
    expect_eq(mgr.validateToken("leader:raft", tok1.fencing_token, 1700), false);
    expect_eq(mgr.validateToken("leader:raft", tok2.fencing_token, 1700), true);

    let stale_renew = false;
    * = mgr.renewLease("leader:raft", "primary-1", tok1.fencing_token, 500, 1700) catch |e| {
        if (e == LockError.NotOwner || e == LockError.LeaseExpired) {
            stale_renew = true;
        }
        tok2
    };
    expect_eq(stale_renew, true);
}

fn testCapLockDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    let wg = cap.db.WaitForGraph.init(alloc);
    defer wg.deinit();
    wg.addEdge("t1", "t2")?;
    wg.addEdge("t2", "t3")?;
    expect_eq(wg.hasCycle(alloc)?, false);
    wg.addEdge("t3", "t1")?;
    expect_eq(wg.hasCycle(alloc)?, true);

    let table = cap.db.NamedLockTable.init(alloc);
    defer table.deinit();
    expect_eq(table.tryAcquire("tool:fs", "agent-x", cap.db.LockMode.Exclusive)?, true);
    expect_eq(table.isResourceLocked("tool:fs"), true);
    table.unlock("tool:fs", "agent-x")?;
    expect_eq(table.isResourceLocked("tool:fs"), false);

    let mgr = cap.db.LeaseLockManager.init(alloc);
    defer mgr.deinit();
    const tok = mgr.acquireLease("kv:slot1", "agent-x", 2000, 5000)?;
    expect_eq(mgr.validateToken("kv:slot1", tok.fencing_token, 6999), true);
    expect_eq(mgr.validateToken("kv:slot1", tok.fencing_token, 7000), false);
}

fn main() {
    test("1. NamedLockTable Shared Concurrency, Exclusive Mutual Exclusion, Reentrancy & NotOwner", () => {
        testNamedLockTableRwReentrancyAndNotOwner()?;
    });

    test("2. NamedLockTable Classic AB-BA Deadlock Detection & Rollback", () => {
        testAbBaDeadlockDetection()?;
    });

    test("3. LeaseLockManager TTL Expiration Preemption & Monotonic Fencing Token", () => {
        testLeaseTtlPreemptionAndMonotonicFencing()?;
    });

    test("4. LeaseLockManager renewLease, releaseLease & Split-Brain validateToken Guard", () => {
        testLeaseRenewReleaseAndSplitBrainGuard()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.lock`", () => {
        testCapLockDynamicMount()?;
    });
}
