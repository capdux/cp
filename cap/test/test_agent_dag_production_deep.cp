// Source: cp/cap/test/test_agent_dag_production_deep.cp (Production deep tests for cap:agent DAG scheduler)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TaskState,
    DagTask,
    DagEdge,
    DagSummary,
    AgentDag,
} from "cap:agent";

test("1. Diamond Fan-Out/Fan-In parallel batch claiming and strict join barrier", () => {
    let tasks_arr: [8]DagTask = std.mem.zeroes([8]DagTask);
    let edges_arr: [16]DagEdge = std.mem.zeroes([16]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    let dag = AgentDag.init(&tasks_s, &edges_s);

    const plan = dag.addTask(100, 0, false) ?? unreachable;
    const search = dag.addTask(20, 0, false) ?? unreachable;
    const analyze = dag.addTask(80, 0, false) ?? unreachable;
    const lint = dag.addTask(50, 0, false) ?? unreachable;
    const synth = dag.addTask(10, 0, false) ?? unreachable;
    const publish = dag.addTask(5, 0, false) ?? unreachable;

    expect_eq(dag.addDependency(plan, search), true);
    expect_eq(dag.addDependency(plan, analyze), true);
    expect_eq(dag.addDependency(plan, lint), true);
    expect_eq(dag.addDependency(search, synth), true);
    expect_eq(dag.addDependency(analyze, synth), true);
    expect_eq(dag.addDependency(lint, synth), true);
    expect_eq(dag.addDependency(synth, publish), true);

    let indeg_arr: [8]u16 = std.mem.zeroes([8]u16);
    let queue_arr: [8]u32 = std.mem.zeroes([8]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    expect_eq(dag.validateAcyclic(&indeg_s, &queue_s), true);

    const s0 = dag.summary();
    expect_eq(s0.ready, 1);
    expect_eq(s0.pending, 5);
    expect_eq(s0.is_done, false);

    // Only plan is ready initially
    const first = dag.claimNextReady() ?? unreachable;
    expect_eq(first, plan);
    expect_eq(dag.claimNextReady() == null, true);
    expect_eq(dag.completeTask(plan), true);

    // Parallel Fan-Out batch claim: must return [analyze(80), lint(50), search(20)]
    let batch_arr: [4]u32 = std.mem.zeroes([4]u32);
    let batch_s: []u32 = batch_arr[0..];
    const claimed = dag.claimReadyBatch(&batch_s);
    expect_eq(claimed, 3);
    expect_eq(batch_s[0], analyze);
    expect_eq(batch_s[1], lint);
    expect_eq(batch_s[2], search);

    // Strict Fan-In barrier: synth stays Pending until ALL 3 upstream tasks finish
    expect_eq(dag.completeTask(analyze), true);
    expect_eq((dag.taskState(synth) ?? unreachable) == TaskState.Pending, true);
    expect_eq(dag.claimNextReady() == null, true);

    expect_eq(dag.completeTask(lint), true);
    expect_eq((dag.taskState(synth) ?? unreachable) == TaskState.Pending, true);
    expect_eq(dag.claimNextReady() == null, true);

    expect_eq(dag.completeTask(search), true);
    expect_eq((dag.taskState(synth) ?? unreachable) == TaskState.Ready, true);

    const join_id = dag.claimNextReady() ?? unreachable;
    expect_eq(join_id, synth);
    expect_eq(dag.completeTask(synth), true);

    const pub_id = dag.claimNextReady() ?? unreachable;
    expect_eq(pub_id, publish);
    expect_eq(dag.completeTask(publish), true);

    const done_snap = dag.summary();
    expect_eq(done_snap.completed, 6);
    expect_eq(done_snap.pending, 0);
    expect_eq(done_snap.ready, 0);
    expect_eq(done_snap.running, 0);
    expect_eq(done_snap.is_done, true);
});

test("2. Deterministic priority ordering with negative priorities and lowest-id tie breaking", () => {
    let tasks_arr: [8]DagTask = std.mem.zeroes([8]DagTask);
    let edges_arr: [4]DagEdge = std.mem.zeroes([4]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    let dag = AgentDag.init(&tasks_s, &edges_s);

    const t0 = dag.addTask(-10, 0, false) ?? unreachable; // id 0, prio -10
    const t1 = dag.addTask(50, 0, false) ?? unreachable;  // id 1, prio 50
    const t2 = dag.addTask(50, 0, false) ?? unreachable;  // id 2, prio 50
    const t3 = dag.addTask(100, 0, false) ?? unreachable; // id 3, prio 100
    const t4 = dag.addTask(0, 0, false) ?? unreachable;   // id 4, prio 0
    const t5 = dag.addTask(50, 0, false) ?? unreachable;  // id 5, prio 50

    let indeg_arr: [8]u16 = std.mem.zeroes([8]u16);
    let queue_arr: [8]u32 = std.mem.zeroes([8]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    expect_eq(dag.validateAcyclic(&indeg_s, &queue_s), true);

    // Claim in two batches of 3 to verify determinism across batch boundaries
    let b1_arr: [3]u32 = std.mem.zeroes([3]u32);
    let b1_s: []u32 = b1_arr[0..];
    expect_eq(dag.claimReadyBatch(&b1_s), 3);
    expect_eq(b1_s[0], t3); // 100
    expect_eq(b1_s[1], t1); // 50, id 1
    expect_eq(b1_s[2], t2); // 50, id 2

    let b2_arr: [4]u32 = std.mem.zeroes([4]u32);
    let b2_s: []u32 = b2_arr[0..];
    expect_eq(dag.claimReadyBatch(&b2_s), 3);
    expect_eq(b2_s[0], t5); // 50, id 5
    expect_eq(b2_s[1], t4); // 0
    expect_eq(b2_s[2], t0); // -10
});

test("3. Transient failure retries back to Ready and succeeds within max_retries", () => {
    let tasks_arr: [4]DagTask = std.mem.zeroes([4]DagTask);
    let edges_arr: [4]DagEdge = std.mem.zeroes([4]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    let dag = AgentDag.init(&tasks_s, &edges_s);
    const flaky = dag.addTask(10, 2, false) ?? unreachable; // up to 2 retries (3 total attempts)
    const next_task = dag.addTask(5, 0, false) ?? unreachable;
    expect_eq(dag.addDependency(flaky, next_task), true);

    let indeg_arr: [4]u16 = std.mem.zeroes([4]u16);
    let queue_arr: [4]u32 = std.mem.zeroes([4]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    expect_eq(dag.validateAcyclic(&indeg_s, &queue_s), true);

    // Attempt 1 -> fail -> Ready
    expect_eq((dag.claimNextReady() ?? unreachable), flaky);
    expect_eq((dag.failTask(flaky) ?? unreachable) == TaskState.Ready, true);
    expect_eq((dag.taskAt(flaky) ?? unreachable).attempts, 1);
    expect_eq((dag.taskState(next_task) ?? unreachable) == TaskState.Pending, true);

    // Attempt 2 -> fail -> Ready
    expect_eq((dag.claimNextReady() ?? unreachable), flaky);
    expect_eq((dag.failTask(flaky) ?? unreachable) == TaskState.Ready, true);
    expect_eq((dag.taskAt(flaky) ?? unreachable).attempts, 2);

    // Attempt 3 -> complete -> unlocks next_task
    expect_eq((dag.claimNextReady() ?? unreachable), flaky);
    expect_eq(dag.completeTask(flaky), true);
    expect_eq((dag.taskAt(flaky) ?? unreachable).attempts, 3);
    expect_eq((dag.taskState(next_task) ?? unreachable) == TaskState.Ready, true);

    expect_eq((dag.claimNextReady() ?? unreachable), next_task);
    expect_eq(dag.completeTask(next_task), true);
    expect_eq(dag.summary().is_done, true);
});

test("4. Failure cascade Skipped when !allow_failure vs downstream continuation when allow_failure", () => {
    let tasks_arr: [8]DagTask = std.mem.zeroes([8]DagTask);
    let edges_arr: [12]DagEdge = std.mem.zeroes([12]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    let dag = AgentDag.init(&tasks_s, &edges_s);

    // Critical chain: crit -> mid1 -> leaf, crit -> mid2 -> leaf
    const crit = dag.addTask(100, 1, false) ?? unreachable;
    const mid1 = dag.addTask(50, 0, false) ?? unreachable;
    const mid2 = dag.addTask(50, 0, false) ?? unreachable;
    const leaf = dag.addTask(10, 0, false) ?? unreachable;

    // Optional branch: opt (allow_failure=true) -> after_opt
    const opt = dag.addTask(90, 0, true) ?? unreachable;
    const after_opt = dag.addTask(20, 0, false) ?? unreachable;

    // Independent task: indep
    const indep = dag.addTask(1, 0, false) ?? unreachable;

    expect_eq(dag.addDependency(crit, mid1), true);
    expect_eq(dag.addDependency(crit, mid2), true);
    expect_eq(dag.addDependency(mid1, leaf), true);
    expect_eq(dag.addDependency(mid2, leaf), true);
    expect_eq(dag.addDependency(opt, after_opt), true);

    let indeg_arr: [8]u16 = std.mem.zeroes([8]u16);
    let queue_arr: [8]u32 = std.mem.zeroes([8]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    expect_eq(dag.validateAcyclic(&indeg_s, &queue_s), true);

    // 1. Fail crit twice (max_retries = 1): second failure exhausts retries and cascades Skipped
    expect_eq((dag.claimNextReady() ?? unreachable), crit);
    expect_eq((dag.failTask(crit) ?? unreachable) == TaskState.Ready, true);
    expect_eq((dag.claimNextReady() ?? unreachable), crit);
    expect_eq((dag.failTask(crit) ?? unreachable) == TaskState.Failed, true);

    expect_eq((dag.taskState(mid1) ?? unreachable) == TaskState.Skipped, true);
    expect_eq((dag.taskState(mid2) ?? unreachable) == TaskState.Skipped, true);
    expect_eq((dag.taskState(leaf) ?? unreachable) == TaskState.Skipped, true);

    // 2. Fail opt (allow_failure = true, max_retries = 0): transitions to Failed, unlocks after_opt!
    expect_eq((dag.claimNextReady() ?? unreachable), opt);
    expect_eq((dag.failTask(opt) ?? unreachable) == TaskState.Failed, true);
    expect_eq((dag.taskState(after_opt) ?? unreachable) == TaskState.Ready, true);

    expect_eq((dag.claimNextReady() ?? unreachable), after_opt);
    expect_eq(dag.completeTask(after_opt), true);

    // 3. Independent task unaffected
    expect_eq((dag.claimNextReady() ?? unreachable), indep);
    expect_eq(dag.completeTask(indep), true);

    const snap = dag.summary();
    expect_eq(snap.completed, 2);
    expect_eq(snap.failed, 2);
    expect_eq(snap.skipped, 3);
    expect_eq(snap.is_done, true);
});

test("5. Cycle detection rejects 2-node, 3-node, and downstream cycles and checks scratch size", () => {
    let tasks_arr: [4]DagTask = std.mem.zeroes([4]DagTask);
    let edges_arr: [8]DagEdge = std.mem.zeroes([8]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    // root -> a -> b -> c -> a (cycle attached to valid root)
    let dag = AgentDag.init(&tasks_s, &edges_s);
    const root = dag.addTask(10, 0, false) ?? unreachable;
    const a = dag.addTask(10, 0, false) ?? unreachable;
    const b = dag.addTask(10, 0, false) ?? unreachable;
    const c = dag.addTask(10, 0, false) ?? unreachable;

    expect_eq(dag.addDependency(root, a), true);
    expect_eq(dag.addDependency(a, b), true);
    expect_eq(dag.addDependency(b, c), true);
    expect_eq(dag.addDependency(c, a), true);

    // Too-small scratch buffer rejected
    let tiny_indeg: [2]u16 = std.mem.zeroes([2]u16);
    let tiny_queue: [2]u32 = std.mem.zeroes([2]u32);
    let tiny_indeg_s: []u16 = tiny_indeg[0..];
    let tiny_queue_s: []u32 = tiny_queue[0..];
    expect_eq(dag.validateAcyclic(&tiny_indeg_s, &tiny_queue_s), false);

    // Full scratch buffer detects cycle and refuses to promote root
    let indeg_arr: [4]u16 = std.mem.zeroes([4]u16);
    let queue_arr: [4]u32 = std.mem.zeroes([4]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    expect_eq(dag.validateAcyclic(&indeg_s, &queue_s), false);
    expect_eq((dag.taskState(root) ?? unreachable) == TaskState.Pending, true);
    expect_eq(dag.claimNextReady() == null, true);
});

test("6. Capacity limits, duplicate/self-loop guards, illegal transitions, and cancelAllPending", () => {
    let tasks_arr: [3]DagTask = std.mem.zeroes([3]DagTask);
    let edges_arr: [2]DagEdge = std.mem.zeroes([2]DagEdge);
    let tasks_s: []DagTask = tasks_arr[0..];
    let edges_s: []DagEdge = edges_arr[0..];

    let dag = AgentDag.init(&tasks_s, &edges_s);
    const t0 = dag.addTask(10, 0, false) ?? unreachable;
    const t1 = dag.addTask(5, 0, false) ?? unreachable;
    const t2 = dag.addTask(1, 0, false) ?? unreachable;
    // Task pool full
    expect_eq(dag.addTask(1, 0, false) == null, true);

    // Self-loop and out-of-bounds rejected
    expect_eq(dag.addDependency(t0, t0), false);
    expect_eq(dag.addDependency(t0, 99), false);
    expect_eq(dag.addDependency(99, t0), false);

    // Valid edge + duplicate rejected
    expect_eq(dag.addDependency(t0, t1), true);
    expect_eq(dag.addDependency(t0, t1), false);
    expect_eq(dag.addDependency(t1, t2), true);
    // Edge pool full (capacity 2)
    expect_eq(dag.addDependency(t0, t2), false);

    // Claim before validation returns null
    expect_eq(dag.claimNextReady() == null, true);

    let indeg_arr: [3]u16 = std.mem.zeroes([3]u16);
    let queue_arr: [3]u32 = std.mem.zeroes([3]u32);
    let indeg_s: []u16 = indeg_arr[0..];
    let queue_s: []u32 = queue_arr[0..];
    expect_eq(dag.validateAcyclic(&indeg_s, &queue_s), true);

    // Illegal transitions on Ready / Pending / out-of-bounds
    expect_eq(dag.completeTask(t0), false); // Ready, not Running
    expect_eq(dag.failTask(t0) == null, true);
    expect_eq(dag.completeTask(t1), false); // Pending
    expect_eq(dag.completeTask(99), false);
    expect_eq(dag.failTask(99) == null, true);
    expect_eq(dag.taskState(99) == null, true);

    // Claim t0 (Running), leave t1/t2 Pending, then cancelAllPending
    expect_eq((dag.claimNextReady() ?? unreachable), t0);
    const cancelled = dag.cancelAllPending();
    expect_eq(cancelled, 3);
    expect_eq(dag.completeTask(t0), false); // Already Cancelled
    expect_eq(dag.failTask(t0) == null, true);

    const snap = dag.summary();
    expect_eq(snap.cancelled, 3);
    expect_eq(snap.is_done, true);
});
