// Origin: Pure-CP Stdlib Test Suite (AI Agent Multi-Core Work-Stealing Scheduler & Load Balancer) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/work_steal.cp
// Covers:
//   1. Local LIFO cache-locality pop order & standalone WorkerQueue circular wrap-around
//   2. Idle worker stealing FIFO from the busiest peer worker's head
//   3. Multi-core skewed submission automatically balanced across all workers
//   4. Full queue rejection, empty pool null return & failed_steals / stats accounting
//   5. Invalid worker index, zero capacity, >16 workers & undersized flat buffer guards

import { test, expect_eq } from "cap:test";
import {
    MAX_WORK_STEAL_WORKERS,
    WorkTask,
    WorkerQueue,
    WorkStealStats,
    WorkStealPool,
} from "../../lib/agent/index.cp";

fn testLocalLifoAndStandaloneDequeWrapAround()! {
    // 1. Standalone WorkerQueue LIFO local vs FIFO steal with circular wrap-around
    let q_buf: [3]WorkTask = [WorkTask{}; 3];
    let q_slots: []WorkTask = q_buf[0..];
    let q = WorkerQueue.init(q_slots);

    expect_eq(q.isEmpty(), true);
    expect_eq(q.capacity(), 3);
    expect_eq(q.pushLocal(WorkTask{ task_id: 1, agent_id: 10, reductions_budget: 2000 }), true);
    expect_eq(q.pushLocal(WorkTask{ task_id: 2, agent_id: 11, reductions_budget: 2000 }), true);
    expect_eq(q.pushLocal(WorkTask{ task_id: 3, agent_id: 12, reductions_budget: 2000 }), true);
    // Full queue rejects 4th push
    expect_eq(q.pushLocal(WorkTask{ task_id: 4, agent_id: 13, reductions_budget: 2000 }), false);

    // Remote steal takes oldest from head (task 1)
    expect_eq((q.stealFromHead() ?? WorkTask{}).task_id, 1);
    // Wrap-around push into freed slot
    expect_eq(q.pushLocal(WorkTask{ task_id: 4, agent_id: 13, reductions_budget: 2000 }), true);
    // Local pop takes newest from tail (LIFO: 4 -> 3 -> 2)
    expect_eq((q.popLocal() ?? WorkTask{}).task_id, 4);
    expect_eq((q.popLocal() ?? WorkTask{}).task_id, 3);
    expect_eq((q.popLocal() ?? WorkTask{}).task_id, 2);
    expect_eq(q.popLocal() == null, true);

    // 2. Pool local LIFO locality
    let flat_buf: [8]WorkTask = [WorkTask{}; 8];
    let flat: []WorkTask = flat_buf[0..];
    let pool = WorkStealPool.init(2, flat, 4) ?? return;

    * = pool.submit(0, WorkTask{ task_id: 101, agent_id: 1, reductions_budget: 1000 });
    * = pool.submit(0, WorkTask{ task_id: 102, agent_id: 2, reductions_budget: 1000 });
    * = pool.submit(0, WorkTask{ task_id: 103, agent_id: 3, reductions_budget: 1000 });

    expect_eq((pool.nextTask(0) ?? WorkTask{}).task_id, 103);
    expect_eq((pool.nextTask(0) ?? WorkTask{}).task_id, 102);
    expect_eq((pool.nextTask(0) ?? WorkTask{}).task_id, 101);
    expect_eq(pool.stats().total_local_popped, 3);
    expect_eq(pool.stats().total_stolen, 0);
}

fn testIdleCoreStealsFifoFromBusiestVictim()! {
    let flat_buf: [12]WorkTask = [WorkTask{}; 12];
    let flat: []WorkTask = flat_buf[0..];
    let pool = WorkStealPool.init(3, flat, 4) ?? return;

    // Core 0 has 1 task; Core 2 has 3 tasks (busiest!); Core 1 is idle
    * = pool.submit(0, WorkTask{ task_id: 10, agent_id: 1, reductions_budget: 2000 });
    * = pool.submit(2, WorkTask{ task_id: 20, agent_id: 2, reductions_budget: 2000 });
    * = pool.submit(2, WorkTask{ task_id: 21, agent_id: 3, reductions_budget: 2000 });
    * = pool.submit(2, WorkTask{ task_id: 22, agent_id: 4, reductions_budget: 2000 });

    // Idle Core 1 must steal from busiest Core 2's HEAD (FIFO -> task 20)
    const stolen1 = pool.nextTask(1) ?? WorkTask{};
    expect_eq(stolen1.task_id, 20);
    expect_eq(pool.queueLen(2), 2);
    expect_eq(pool.queueLen(0), 1);

    // Idle Core 1 steals again -> Core 2 is still busiest (2 > 1) -> steals task 21
    const stolen2 = pool.nextTask(1) ?? WorkTask{};
    expect_eq(stolen2.task_id, 21);
    expect_eq(pool.queueLen(2), 1);

    // Core 2 locally pops its remaining task 22
    expect_eq((pool.nextTask(2) ?? WorkTask{}).task_id, 22);
    expect_eq(pool.stats().total_stolen, 2);
    expect_eq(pool.stats().total_local_popped, 1);
}

fn testMultiCoreSkewedLoadBalancing()! {
    let flat_buf: [32]WorkTask = [WorkTask{}; 32];
    let flat: []WorkTask = flat_buf[0..];
    let pool = WorkStealPool.init(4, flat, 8) ?? return;

    // Dump all 8 tasks onto Core 0 (skewed submission)
    let t: u64 = 1;
    while (t <= 8) : (t += 1) {
        expect_eq(pool.submit(0, WorkTask{ task_id: t, agent_id: u32(t), reductions_budget: 2000 }), true);
    }
    expect_eq(pool.stats().max_queue_len, 8);
    expect_eq(pool.stats().min_queue_len, 0);

    // All 4 cores pull 2 rounds -> Core 0 pops locally (2), Cores 1..3 steal (6), all 8 drained!
    let executed: usize = 0;
    let round: usize = 0;
    while (round < 2) : (round += 1) {
        let w: usize = 0;
        while (w < 4) : (w += 1) {
            if (task := pool.nextTask(w)) {
                * = task;
                executed += 1;
            }
        }
    }

    expect_eq(executed, 8);
    const st: WorkStealStats = pool.stats();
    expect_eq(st.total_pushed, 8);
    expect_eq(st.total_local_popped, 2);
    expect_eq(st.total_stolen, 6);
    expect_eq(st.max_queue_len, 0);
    expect_eq(st.min_queue_len, 0);
}

fn testFullQueueRejectionAndEmptyPoolStats()! {
    let flat_buf: [4]WorkTask = [WorkTask{}; 4];
    let flat: []WorkTask = flat_buf[0..];
    let pool = WorkStealPool.init(2, flat, 2) ?? return;

    // Empty pool returns null and increments failed_steals
    expect_eq(pool.nextTask(0) == null, true);
    expect_eq(pool.nextTask(1) == null, true);
    expect_eq(pool.stats().failed_steals, 2);

    // Fill worker 0 to capacity 2 -> 3rd submit rejected
    expect_eq(pool.submit(0, WorkTask{ task_id: 1, agent_id: 1, reductions_budget: 100 }), true);
    expect_eq(pool.submit(0, WorkTask{ task_id: 2, agent_id: 2, reductions_budget: 100 }), true);
    expect_eq(pool.submit(0, WorkTask{ task_id: 3, agent_id: 3, reductions_budget: 100 }), false);
    expect_eq(pool.queueLen(0), 2);
    expect_eq(pool.stats().total_pushed, 2);
}

fn testInvalidWorkerIndexAndInitGuards()! {
    let flat_buf: [16]WorkTask = [WorkTask{}; 16];
    let flat: []WorkTask = flat_buf[0..];

    // Invalid init parameters rejected
    expect_eq(WorkStealPool.init(0, flat, 4) == null, true);
    expect_eq(WorkStealPool.init(MAX_WORK_STEAL_WORKERS + 1, flat, 1) == null, true);
    expect_eq(WorkStealPool.init(4, flat, 0) == null, true);
    // Undersized flat buffer (4 * 5 = 20 > 16) rejected
    expect_eq(WorkStealPool.init(4, flat, 5) == null, true);

    // Out-of-bounds worker index guards on valid pool
    let pool = WorkStealPool.init(2, flat, 4) ?? return;
    expect_eq(pool.submit(2, WorkTask{ task_id: 1, agent_id: 1, reductions_budget: 100 }), false);
    expect_eq(pool.nextTask(99) == null, true);
    expect_eq(pool.queueLen(2), 0);

    // Zero-capacity standalone WorkerQueue never panics
    let zero_buf: [0]WorkTask = [];
    let zero_slots: []WorkTask = zero_buf[0..];
    let zq = WorkerQueue.init(zero_slots);
    expect_eq(zq.pushLocal(WorkTask{ task_id: 1, agent_id: 1, reductions_budget: 100 }), false);
    expect_eq(zq.popLocal() == null, true);
    expect_eq(zq.stealFromHead() == null, true);
}

test("agent work_steal: local LIFO cache locality and standalone deque wrap-around", testLocalLifoAndStandaloneDequeWrapAround);
test("agent work_steal: idle core steals FIFO from busiest peer head", testIdleCoreStealsFifoFromBusiestVictim);
test("agent work_steal: skewed multi-core submissions balanced automatically", testMultiCoreSkewedLoadBalancing);
test("agent work_steal: full queue rejection, empty pool null and failed_steals stats", testFullQueueRejectionAndEmptyPoolStats);
test("agent work_steal: invalid worker index, >16 workers and undersized buffer guards", testInvalidWorkerIndexAndInitGuards);
