// Origin: Pure-CP Stdlib Test Suite (lib/a2a/taskGraph.cp) | Created: 2026-11-22
// Production Deep Verification Suite for A2A Multi-Agent Task DAG Scheduler:
//   1. Diamond DAG priority wave dispatch & completed_ok terminal rollup
//   2. Concurrency-bounded dispatch (`max_concurrency` accounts for active running tasks)
//   3. Iterative failure propagation cascade (`markFailed` -> `skipped_dep_failed`) & overall status
//   4. `allow_partial_fail = true` graceful degradation boundary unblocking downstream synthesis
//   5. `cancelSubtree` selective cancellation preserving completed nodes & sibling branches
//   6. Cycle detection (self-loop & multi-hop cycles) and duplicate edge/task guards
//   7. Strict state-machine transition guards & unknown task_id handling
//   8. Full-node (64) & full-edge (128) capacity_full guards + clear() recovery

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TASK_GRAPH_MAX_NODES,
    TASK_GRAPH_MAX_EDGES,
    TaskGraphNodeState,
    TaskGraphStatus,
    TaskGraphOpStatus,
    A2aTaskGraph,
} from "cap:a2a";

test("taskGraph: diamond DAG priority wave dispatch & completed_ok rollup", () => {
    let dag = A2aTaskGraph.init();
    expect_eq(dag.addTask(1, 10, 100, false), TaskGraphOpStatus.ok);
    expect_eq(dag.addTask(2, 20, 50, false), TaskGraphOpStatus.ok);
    expect_eq(dag.addTask(3, 30, 80, false), TaskGraphOpStatus.ok); // higher priority than 2
    expect_eq(dag.addTask(4, 40, 10, false), TaskGraphOpStatus.ok);

    expect_eq(dag.addDependency(1, 2), TaskGraphOpStatus.ok);
    expect_eq(dag.addDependency(1, 3), TaskGraphOpStatus.ok);
    expect_eq(dag.addDependency(2, 4), TaskGraphOpStatus.ok);
    expect_eq(dag.addDependency(3, 4), TaskGraphOpStatus.ok);

    let wave: [4]u64 = [0; 4];
    let wave_s: []u64 = wave[0..];

    // Wave 1: only task 1 is ready
    expect_eq(dag.collectReadyTasks(4, *wave_s), 1 as usize);
    expect_eq(wave[0], 1 as u64);
    expect_eq(dag.markRunning(1), TaskGraphOpStatus.ok);
    expect_eq(dag.markSucceeded(1), TaskGraphOpStatus.ok);

    // Wave 2: tasks 3 (prio 80) and 2 (prio 50) become ready in descending priority order
    expect_eq(dag.collectReadyTasks(4, *wave_s), 2 as usize);
    expect_eq(wave[0], 3 as u64);
    expect_eq(wave[1], 2 as u64);
    expect_eq(dag.markRunning(3), TaskGraphOpStatus.ok);
    expect_eq(dag.markRunning(2), TaskGraphOpStatus.ok);
    expect_eq(dag.markSucceeded(3), TaskGraphOpStatus.ok);
    // Task 4 must still be pending until task 2 also succeeds
    expect_eq((dag.getTask(4) ?? unreachable).state, TaskGraphNodeState.pending);
    expect_eq(dag.markSucceeded(2), TaskGraphOpStatus.ok);

    // Wave 3: task 4 is now ready
    expect_eq(dag.collectReadyTasks(4, *wave_s), 1 as usize);
    expect_eq(wave[0], 4 as u64);
    expect_eq(dag.markRunning(4), TaskGraphOpStatus.ok);
    expect_eq(dag.markSucceeded(4), TaskGraphOpStatus.ok);

    const sum = dag.summary();
    expect_eq(sum.total, 4 as usize);
    expect_eq(sum.succeeded, 4 as usize);
    expect_eq(sum.overall, TaskGraphStatus.completed_ok);
});

test("taskGraph: concurrency-bounded collectReadyTasks respects in-flight running count", () => {
    let dag = A2aTaskGraph.init();
    * = dag.addTask(10, 1, 10, false);
    * = dag.addTask(20, 2, 30, false);
    * = dag.addTask(30, 3, 20, false);

    let wave: [4]u64 = [0; 4];
    let wave_s: []u64 = wave[0..];

    // max_concurrency == 0 dispatches nothing
    expect_eq(dag.collectReadyTasks(0, *wave_s), 0 as usize);

    // max_concurrency == 2 picks top 2: [20, 30]
    expect_eq(dag.collectReadyTasks(2, *wave_s), 2 as usize);
    expect_eq(wave[0], 20 as u64);
    expect_eq(wave[1], 30 as u64);

    // Start task 20 -> 1 running, so max_concurrency=2 only allows 1 more ready task (30)
    expect_eq(dag.markRunning(20), TaskGraphOpStatus.ok);
    expect_eq(dag.collectReadyTasks(2, *wave_s), 1 as usize);
    expect_eq(wave[0], 30 as u64);

    // Start task 30 -> 2 running == max_concurrency -> 0 more dispatched
    expect_eq(dag.markRunning(30), TaskGraphOpStatus.ok);
    expect_eq(dag.collectReadyTasks(2, *wave_s), 0 as usize);
});

test("taskGraph: iterative failure propagation cascade & completed_partial / failed status", () => {
    // Chain: 1 -> 2 -> 3 -> 4
    let dag = A2aTaskGraph.init();
    * = dag.addTask(1, 1, 10, false);
    * = dag.addTask(2, 2, 10, false);
    * = dag.addTask(3, 3, 10, false);
    * = dag.addTask(4, 4, 10, false);
    * = dag.addDependency(1, 2);
    * = dag.addDependency(2, 3);
    * = dag.addDependency(3, 4);

    // Task 1 succeeds, Task 2 fails -> Tasks 3 and 4 cascade to skipped_dep_failed
    expect_eq(dag.markRunning(1), TaskGraphOpStatus.ok);
    expect_eq(dag.markSucceeded(1), TaskGraphOpStatus.ok);
    expect_eq(dag.markRunning(2), TaskGraphOpStatus.ok);
    expect_eq(dag.markFailed(2), TaskGraphOpStatus.ok);

    expect_eq((dag.getTask(3) ?? unreachable).state, TaskGraphNodeState.skipped_dep_failed);
    expect_eq((dag.getTask(4) ?? unreachable).state, TaskGraphNodeState.skipped_dep_failed);

    const sum = dag.summary();
    expect_eq(sum.succeeded, 1 as usize);
    expect_eq(sum.failed, 1 as usize);
    expect_eq(sum.skipped_dep_failed, 2 as usize);
    expect_eq(sum.overall, TaskGraphStatus.completed_partial);

    // Pure failure DAG (root fails -> 0 succeeded -> overall == failed)
    let dag2 = A2aTaskGraph.init();
    * = dag2.addTask(1, 1, 10, false);
    * = dag2.addTask(2, 2, 10, false);
    * = dag2.addDependency(1, 2);
    * = dag2.markRunning(1);
    * = dag2.markFailed(1);
    expect_eq(dag2.summary().overall, TaskGraphStatus.failed);
});

test("taskGraph: allow_partial_fail=true graceful degradation boundary", () => {
    // {1, 2} -> 3 (allow_partial_fail=true) -> 4 (strict)
    let dag = A2aTaskGraph.init();
    * = dag.addTask(1, 1, 10, false);
    * = dag.addTask(2, 2, 10, false);
    * = dag.addTask(3, 3, 10, true); // aggregator tolerates partial upstream failure
    * = dag.addTask(4, 4, 10, false);
    * = dag.addDependency(1, 3);
    * = dag.addDependency(2, 3);
    * = dag.addDependency(3, 4);

    // Task 1 fails while Task 2 is still running -> Task 3 stays pending (NOT skipped!)
    * = dag.markRunning(1);
    * = dag.markRunning(2);
    * = dag.markFailed(1);
    expect_eq((dag.getTask(3) ?? unreachable).state, TaskGraphNodeState.pending);

    // Task 2 succeeds -> all deps of 3 are terminal, so 3 transitions to ready!
    * = dag.markSucceeded(2);
    expect_eq((dag.getTask(3) ?? unreachable).state, TaskGraphNodeState.ready);

    // Task 3 runs and succeeds -> unblocks strict Task 4
    * = dag.markRunning(3);
    * = dag.markSucceeded(3);
    expect_eq((dag.getTask(4) ?? unreachable).state, TaskGraphNodeState.ready);
    * = dag.markRunning(4);
    * = dag.markSucceeded(4);

    const sum = dag.summary();
    expect_eq(sum.succeeded, 3 as usize);
    expect_eq(sum.failed, 1 as usize);
    expect_eq(sum.skipped_dep_failed, 0 as usize);
    expect_eq(sum.overall, TaskGraphStatus.completed_partial);
});

test("taskGraph: cancelSubtree selectively cancels reachable non-terminal nodes", () => {
    // Root 1 -> branch A (2 -> 3), branch B (4)
    let dag = A2aTaskGraph.init();
    * = dag.addTask(1, 1, 10, false);
    * = dag.addTask(2, 2, 10, false);
    * = dag.addTask(3, 3, 10, false);
    * = dag.addTask(4, 4, 10, false);
    * = dag.addDependency(1, 2);
    * = dag.addDependency(2, 3);
    * = dag.addDependency(1, 4);

    * = dag.markRunning(1);
    * = dag.markSucceeded(1);
    * = dag.markRunning(2);

    // Cancel subtree rooted at 2 -> cancels running 2 and pending 3; leaves 1 (succeeded) & 4 (ready) intact
    expect_eq(dag.cancelSubtree(2), 2 as usize);
    expect_eq((dag.getTask(1) ?? unreachable).state, TaskGraphNodeState.succeeded);
    expect_eq((dag.getTask(2) ?? unreachable).state, TaskGraphNodeState.cancelled);
    expect_eq((dag.getTask(3) ?? unreachable).state, TaskGraphNodeState.cancelled);
    expect_eq((dag.getTask(4) ?? unreachable).state, TaskGraphNodeState.ready);
    expect_eq(dag.cancelSubtree(999), 0 as usize);
});

test("taskGraph: cycle detection (self-loop & multi-hop) and duplicate guards", () => {
    let dag = A2aTaskGraph.init();
    expect_eq(dag.addTask(1, 1, 10, false), TaskGraphOpStatus.ok);
    expect_eq(dag.addTask(1, 2, 20, false), TaskGraphOpStatus.duplicate_task_id);
    expect_eq(dag.addTask(2, 2, 10, false), TaskGraphOpStatus.ok);
    expect_eq(dag.addTask(3, 3, 10, false), TaskGraphOpStatus.ok);

    // Self-loop rejected
    expect_eq(dag.addDependency(1, 1), TaskGraphOpStatus.self_loop);
    // Unknown task rejected
    expect_eq(dag.addDependency(1, 99), TaskGraphOpStatus.task_not_found);

    // 1 -> 2 -> 3
    expect_eq(dag.addDependency(1, 2), TaskGraphOpStatus.ok);
    expect_eq(dag.addDependency(1, 2), TaskGraphOpStatus.duplicate_edge);
    expect_eq(dag.addDependency(2, 3), TaskGraphOpStatus.ok);

    // Back-edges 3 -> 1 and 2 -> 1 must be rejected as cycle_detected
    expect_eq(dag.addDependency(3, 1), TaskGraphOpStatus.cycle_detected);
    expect_eq(dag.addDependency(2, 1), TaskGraphOpStatus.cycle_detected);
    expect_eq(dag.edgeCount(), 2 as usize);
});

test("taskGraph: strict state-machine transition guards", () => {
    let dag = A2aTaskGraph.init();
    * = dag.addTask(1, 1, 10, false);
    * = dag.addTask(2, 2, 10, false);
    * = dag.addDependency(1, 2);

    // Unknown task transitions
    expect_eq(dag.markRunning(99), TaskGraphOpStatus.task_not_found);
    expect_eq(dag.markSucceeded(99), TaskGraphOpStatus.task_not_found);
    expect_eq(dag.markFailed(99), TaskGraphOpStatus.task_not_found);

    // Cannot run pending task 2 or succeed/fail ready task 1 before running
    expect_eq(dag.markRunning(2), TaskGraphOpStatus.invalid_transition);
    expect_eq(dag.markSucceeded(1), TaskGraphOpStatus.invalid_transition);
    expect_eq(dag.markFailed(1), TaskGraphOpStatus.invalid_transition);

    // Cannot transition terminal task again
    expect_eq(dag.markRunning(1), TaskGraphOpStatus.ok);
    expect_eq(dag.markSucceeded(1), TaskGraphOpStatus.ok);
    expect_eq(dag.markFailed(1), TaskGraphOpStatus.invalid_transition);
    expect_eq(dag.markRunning(1), TaskGraphOpStatus.invalid_transition);
});

test("taskGraph: full-node (64) & full-edge (128) capacity_full guards + clear()", () => {
    let dag = A2aTaskGraph.init();
    let i: u64 = 0;
    while (i < TASK_GRAPH_MAX_NODES as u64) : (i += 1) {
        expect_eq(dag.addTask(i + 1, 1, 10, false), TaskGraphOpStatus.ok);
    }
    expect_eq(dag.addTask(999, 1, 10, false), TaskGraphOpStatus.capacity_full);

    // Add 128 acyclic edges (from lower IDs to higher IDs)
    let added: usize = 0;
    let src: u64 = 1;
    while (src <= 8 && added < TASK_GRAPH_MAX_EDGES) : (src += 1) {
        let dst: u64 = 9;
        while (dst <= 24 && added < TASK_GRAPH_MAX_EDGES) : (dst += 1) {
            expect_eq(dag.addDependency(src, dst), TaskGraphOpStatus.ok);
            added += 1;
        }
    }
    expect_eq(dag.edgeCount(), TASK_GRAPH_MAX_EDGES);
    expect_eq(dag.addDependency(1, 64), TaskGraphOpStatus.capacity_full);

    dag.clear();
    expect_eq(dag.nodeCount(), 0 as usize);
    expect_eq(dag.edgeCount(), 0 as usize);
    expect_eq(dag.summary().overall, TaskGraphStatus.completed_ok);
});
