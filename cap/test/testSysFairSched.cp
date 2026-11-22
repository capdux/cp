// Origin: Pure-CP Stdlib Test Suite (lib/sys/fairSched.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Multi-Tenant Deficit Round Robin (DRR) Fair Scheduler:
//   1. Weighted 2:1 throughput proportionality across mixed task costs & Jain's Fairness Index (~10000 bps)
//   2. Multi-round deficit accumulation for large tasks (zero starvation) & deficit zeroing on queue drain
//   3. Single-tenant huge-cost task O(1) fast-forward without long loop stall
//   4. 32-slot FIFO queue-full backpressure, recovery after dequeue & invalid input rejection
//   5. Dynamic `setQuantum`, mid-ring `removeTenant` preservation & 32-tenant saturation + slot reuse
//   6. Per-tenant & global telemetry snapshots, `reset` & `cap.sys` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    FAIR_SCHED_MAX_TENANTS,
    FAIR_SCHED_QUEUE_CAP,
    FAIR_SCHED_BPS_SCALE,
    FairSchedOpStatus,
    FairTask,
    FairTenantStats,
    FairSchedStats,
    DeficitRoundRobinScheduler,
} from "cap:sys";

test("FairSched: 2:1 weighted throughput sharing & Jain's Fairness Index") {
    let sched = DeficitRoundRobinScheduler.init();
    expect_eq(sched.registerTenant(1, 200), FairSchedOpStatus.ok); // weight 2x
    expect_eq(sched.registerTenant(2, 100), FairSchedOpStatus.ok); // weight 1x

    // Enqueue 12 tasks of cost=100 for tenant 1 (total 1200) and 6 tasks of cost=100 for tenant 2 (total 600)
    let i: u64 = 0;
    while (i < 12) : (i += 1) {
        expect_eq(sched.enqueue(1, 1000 + i, 100, i), FairSchedOpStatus.ok);
    }
    i = 0;
    while (i < 6) : (i += 1) {
        expect_eq(sched.enqueue(2, 2000 + i, 100, i), FairSchedOpStatus.ok);
    }

    // Round 1: tenant 1 gets 2 tasks (200), tenant 2 gets 1 task (100)
    const t0 = sched.dequeue() ?? FairTask{};
    const t1 = sched.dequeue() ?? FairTask{};
    const t2 = sched.dequeue() ?? FairTask{};
    expect_eq(t0.tenant_id, @as(u32, 1));
    expect_eq(t1.tenant_id, @as(u32, 1));
    expect_eq(t2.tenant_id, @as(u32, 2));

    // Drain remaining 15 tasks
    let drained: usize = 3;
    while (sched.dequeue() != null) {
        drained += 1;
    }
    expect_eq(drained, @as(usize, 18));

    // Tenant 1 scheduled 1200 (1200/200 = 6.0), Tenant 2 scheduled 600 (600/100 = 6.0) -> perfect 10000 bps!
    expect_eq(sched.jainFairnessBps(), FAIR_SCHED_BPS_SCALE);
    if (st1 := sched.tenantStats(1)) {
        expect_eq(st1.scheduled_cost_total, @as(u64, 1200));
        expect_eq(st1.deficit, @as(u64, 0)); // drained queue zeroes deficit
        expect_true(!st1.in_active_ring);
    }
    if (st2 := sched.tenantStats(2)) {
        expect_eq(st2.scheduled_cost_total, @as(u64, 600));
        expect_eq(st2.deficit, @as(u64, 0));
    }
}

test("FairSched: multi-round deficit accumulation for large tasks & zero starvation") {
    let sched = DeficitRoundRobinScheduler.init();
    * = sched.registerTenant(1, 100);
    * = sched.registerTenant(2, 100);

    // Tenant 1 has one large task (cost=350) followed by one small task (cost=50)
    * = sched.enqueue(1, 101, 350, 1);
    * = sched.enqueue(1, 102, 50, 2);

    // Tenant 2 has 5 small tasks (cost=100 each)
    let i: u64 = 0;
    while (i < 5) : (i += 1) {
        * = sched.enqueue(2, 200 + i, 100, 10 + i);
    }

    // Rounds 1..3: tenant 1 accumulates deficit 100 -> 200 -> 300 while tenant 2 dispatches 200, 201, 202
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 200));
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 201));
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 202));

    // Round 4: tenant 1 reaches deficit 400 >= 350 -> dispatches 101 (leftover 50) AND immediately 102 (cost 50)!
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 101));
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 102));

    // Remaining tenant 2 tasks 203, 204 drain cleanly
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 203));
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 204));
    expect_true(sched.dequeue() == null);
}

test("FairSched: sole active tenant huge-cost task O(1) fast-forward") {
    let sched = DeficitRoundRobinScheduler.init();
    * = sched.registerTenant(1, 10);
    * = sched.enqueue(1, 999, 1_000_000, 42);

    const task_opt = sched.dequeue();
    expect_true(task_opt != null);
    if (t := task_opt) {
        expect_eq(t.task_id, @as(u64, 999));
        expect_eq(t.cost, @as(u64, 1_000_000));
    }
    expect_eq(sched.stats().pending_tasks, @as(usize, 0));
}

test("FairSched: 32-slot queue backpressure, recovery & invalid input guards") {
    let sched = DeficitRoundRobinScheduler.init();

    // Invalid inputs rejected cleanly
    expect_eq(sched.registerTenant(0, 100), FairSchedOpStatus.invalid_input);
    expect_eq(sched.registerTenant(1, 0), FairSchedOpStatus.invalid_input);
    expect_eq(sched.enqueue(1, 1, 10, 0), FairSchedOpStatus.tenant_not_found);

    expect_eq(sched.registerTenant(1, 100), FairSchedOpStatus.ok);
    expect_eq(sched.registerTenant(1, 100), FairSchedOpStatus.tenant_exists);
    expect_eq(sched.enqueue(1, 1, 0, 0), FairSchedOpStatus.invalid_input);

    // Fill all 32 slots
    let i: usize = 0;
    while (i < FAIR_SCHED_QUEUE_CAP) : (i += 1) {
        expect_eq(sched.enqueue(1, @as(u64, @intCast(i + 1)), 50, 0), FairSchedOpStatus.ok);
    }

    // 33rd enqueue triggers queue_full backpressure
    expect_eq(sched.enqueue(1, 999, 50, 0), FairSchedOpStatus.queue_full);
    expect_eq(sched.stats().total_dropped, @as(u64, 1));

    // Dequeue 1 task frees a slot for subsequent enqueue
    const first = sched.dequeue() ?? FairTask{};
    expect_eq(first.task_id, @as(u64, 1));
    expect_eq(sched.enqueue(1, 33, 50, 0), FairSchedOpStatus.ok);
    if (st := sched.tenantStats(1)) {
        expect_eq(st.queued_count, FAIR_SCHED_QUEUE_CAP);
        expect_eq(st.dropped_total, @as(u64, 1));
    }
}

test("FairSched: dynamic setQuantum, mid-ring removeTenant & 32-tenant saturation") {
    let sched = DeficitRoundRobinScheduler.init();
    * = sched.registerTenant(1, 100);
    * = sched.registerTenant(2, 100);
    * = sched.registerTenant(3, 100);

    * = sched.enqueue(1, 11, 100, 0);
    * = sched.enqueue(2, 21, 100, 0);
    * = sched.enqueue(3, 31, 100, 0);
    * = sched.enqueue(3, 32, 100, 0);

    // Dynamically boost tenant 3 quantum to 200 and remove tenant 2 from middle of active ring
    expect_eq(sched.setQuantum(3, 0), FairSchedOpStatus.invalid_input);
    expect_eq(sched.setQuantum(99, 200), FairSchedOpStatus.tenant_not_found);
    expect_eq(sched.setQuantum(3, 200), FairSchedOpStatus.ok);
    expect_eq(sched.removeTenant(2), FairSchedOpStatus.ok);
    expect_eq(sched.removeTenant(2), FairSchedOpStatus.tenant_not_found);
    expect_eq(sched.stats().active_tenants, @as(usize, 2));
    expect_eq(sched.stats().pending_tasks, @as(usize, 3));

    // Dequeue order: tenant 1 (11), then tenant 3 (31, 32)
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 11));
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 31));
    expect_eq((sched.dequeue() ?? FairTask{}).task_id, @as(u64, 32));
    expect_true(sched.dequeue() == null);

    // Fill all 32 tenant slots and test saturation + slot reuse
    sched.reset();
    let tid: u32 = 1;
    while (tid <= @as(u32, @intCast(FAIR_SCHED_MAX_TENANTS))) : (tid += 1) {
        expect_eq(sched.registerTenant(tid, 100), FairSchedOpStatus.ok);
    }
    expect_eq(sched.registerTenant(999, 100), FairSchedOpStatus.tenant_table_full);
    expect_eq(sched.removeTenant(5), FairSchedOpStatus.ok);
    expect_eq(sched.registerTenant(999, 100), FairSchedOpStatus.ok);
}

test("FairSched: global telemetry snapshot, reset & cap.sys facade parity") {
    let sched = cap.sys.DeficitRoundRobinScheduler.init();
    * = sched.registerTenant(10, 50);
    * = sched.registerTenant(20, 50);
    * = sched.enqueue(10, 1, 50, 100);
    * = sched.enqueue(20, 2, 50, 101);

    * = sched.dequeue();
    const st: FairSchedStats = sched.stats();
    expect_eq(st.registered_tenants, @as(usize, 2));
    expect_eq(st.tenant_capacity, FAIR_SCHED_MAX_TENANTS);
    expect_eq(st.active_tenants, @as(usize, 1));
    expect_eq(st.pending_tasks, @as(usize, 1));
    expect_eq(st.total_enqueued, @as(u64, 2));
    expect_eq(st.total_dequeued, @as(u64, 1));
    expect_eq(st.total_scheduled_cost, @as(u64, 50));

    sched.reset();
    expect_eq(sched.stats().registered_tenants, @as(usize, 0));
    expect_eq(sched.stats().total_dequeued, @as(u64, 0));
    expect_true(sched.tenantStats(10) == null);
}
