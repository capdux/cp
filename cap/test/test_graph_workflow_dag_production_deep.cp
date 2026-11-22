// Origin: Pure-CP Stdlib Test Suite (lib/graph/workflow_dag.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Heap DAG Wave Planner, CPM & Cycle Extractor:
//   1. Diamond & multi-layer parallel wave partitioning + max_parallelism
//   2. Exact 6-node Critical Path Method (CPM) ES/EF/LS/LF/slack/makespan verification
//   3. Cycle detection (planParallelWaves) & 3-color DFS cycle extraction (3-cycle + self-loop)
//   4. Isolated nodes (0 edges) & acyclic findCycleNodes == 0
//   5. Duplicate edge resilience in both wave planning and CPM
//   6. Out-of-bounds node, undersized buffers & zero-node boundary guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DagNodeCost,
    DagEdgeSpec,
    DagWaveAssignment,
    CriticalPathMetrics,
    WorkflowPlanSummary,
    planParallelWaves,
    analyzeCriticalPath,
    findCycleNodes,
} from "cap:graph";

// ============================================================================
// Test 1: Diamond & Multi-Layer Parallel Wave Partitioning
// ============================================================================

fn testDiamondParallelWaves()! {
    // 0 -> {1, 2, 3} -> 4
    // Wave 0: [0]
    // Wave 1: [1, 2, 3] (max_parallelism = 3)
    // Wave 2: [4]
    const edges: [6]DagEdgeSpec = [
        DagEdgeSpec{ from: 0, to: 3 },
        DagEdgeSpec{ from: 0, to: 1 },
        DagEdgeSpec{ from: 0, to: 2 },
        DagEdgeSpec{ from: 2, to: 4 },
        DagEdgeSpec{ from: 1, to: 4 },
        DagEdgeSpec{ from: 3, to: 4 },
    ];
    let order: [5]u32 = [0; 5];
    let waves: [5]DagWaveAssignment = [DagWaveAssignment{}; 5];
    let in_deg: [5]u32 = [0; 5];
    let queue: [5]u32 = [0; 5];
    let order_s: []u32 = order[0..];
    let waves_s: []DagWaveAssignment = waves[0..];
    let in_deg_s: []u32 = in_deg[0..];
    let queue_s: []u32 = queue[0..];

    const plan: WorkflowPlanSummary = cap.graph.planParallelWaves(5, edges[0..], order_s, waves_s, in_deg_s, queue_s) ?? return error.PlanFailed;
    expect_eq(plan.has_cycle, false);
    expect_eq(plan.scheduled_count, 5);
    expect_eq(plan.wave_count, 3);
    expect_eq(plan.max_parallelism, 3);

    expect_eq(order[0], 0);
    expect_eq(order[1], 1);
    expect_eq(order[2], 2);
    expect_eq(order[3], 3);
    expect_eq(order[4], 4);
    expect_eq(waves[0].wave_index, 0);
    expect_eq(waves[1].wave_index, 1);
    expect_eq(waves[2].wave_index, 1);
    expect_eq(waves[3].wave_index, 1);
    expect_eq(waves[4].wave_index, 2);
}

// ============================================================================
// Test 2: Exact 6-Node Critical Path Method (CPM) Analysis
// ============================================================================

fn testCriticalPathMethodExact()! {
    // 0(10) -> 1(50) -> 3(20) -> 5(10)   [Critical path: 10+50+20+10 = 90ms]
    // 0(10) -> 2(15) -> 4(25) -> 5(10)   [Branch path:   10+15+25+10 = 60ms, slack = 30ms]
    // 2(15) -> 3(20)                     [Cross edge: EF(2)=25 < EF(1)=60, so 3 still starts at 60]
    const edges: [7]DagEdgeSpec = [
        DagEdgeSpec{ from: 0, to: 1 },
        DagEdgeSpec{ from: 0, to: 2 },
        DagEdgeSpec{ from: 1, to: 3 },
        DagEdgeSpec{ from: 2, to: 3 },
        DagEdgeSpec{ from: 2, to: 4 },
        DagEdgeSpec{ from: 3, to: 5 },
        DagEdgeSpec{ from: 4, to: 5 },
    ];
    const durs: [6]u64 = [10, 50, 15, 20, 25, 10];
    let order: [6]u32 = [0; 6];
    let waves: [6]DagWaveAssignment = [DagWaveAssignment{}; 6];
    let in_deg: [6]u32 = [0; 6];
    let queue: [6]u32 = [0; 6];
    let order_s: []u32 = order[0..];
    let waves_s: []DagWaveAssignment = waves[0..];
    let in_deg_s: []u32 = in_deg[0..];
    let queue_s: []u32 = queue[0..];

    const plan = planParallelWaves(6, edges[0..], order_s, waves_s, in_deg_s, queue_s) ?? return error.PlanFailed;
    expect_eq(plan.has_cycle, false);

    let metrics: [6]CriticalPathMetrics = [CriticalPathMetrics{}; 6];
    let metrics_s: []CriticalPathMetrics = metrics[0..];
    const makespan = analyzeCriticalPath(6, durs[0..], edges[0..], order[0..], metrics_s) ?? return error.CpmFailed;
    expect_eq(makespan, 90);

    // Critical nodes: 0, 1, 3, 5 (slack == 0)
    expect_eq(metrics[0].is_critical, true);
    expect_eq(metrics[1].is_critical, true);
    expect_eq(metrics[3].is_critical, true);
    expect_eq(metrics[5].is_critical, true);
    expect_eq(metrics[3].earliest_start, 60);
    expect_eq(metrics[3].earliest_finish, 80);
    expect_eq(metrics[5].earliest_start, 80);
    expect_eq(metrics[5].latest_finish, 90);

    // Non-critical nodes:
    // Node 4: ES=25, EF=50, LF=80, LS=55 -> slack = 30
    expect_eq(metrics[4].is_critical, false);
    expect_eq(metrics[4].earliest_start, 25);
    expect_eq(metrics[4].latest_start, 55);
    expect_eq(metrics[4].total_slack, 30);

    // Node 2: ES=10, EF=25, min(LS(3)=60, LS(4)=55)=55 -> LF=55, LS=40 -> slack = 30
    expect_eq(metrics[2].is_critical, false);
    expect_eq(metrics[2].latest_finish, 55);
    expect_eq(metrics[2].latest_start, 40);
    expect_eq(metrics[2].total_slack, 30);

    // Touch DagNodeCost to verify export
    const nc = DagNodeCost{ node_id: 0, duration_ms: 10 };
    expect_eq(nc.duration_ms, 10);
}

// ============================================================================
// Test 3: Cycle Detection & 3-Color DFS Cycle Extraction (3-Cycle + Self-Loop)
// ============================================================================

fn testCycleDetectionAndExtraction()! {
    // 0 -> 1 -> 2 -> 3 -> 1 (cycle is [1, 2, 3])
    const edges: [4]DagEdgeSpec = [
        DagEdgeSpec{ from: 0, to: 1 },
        DagEdgeSpec{ from: 1, to: 2 },
        DagEdgeSpec{ from: 2, to: 3 },
        DagEdgeSpec{ from: 3, to: 1 },
    ];
    let order: [4]u32 = [0; 4];
    let waves: [4]DagWaveAssignment = [DagWaveAssignment{}; 4];
    let in_deg: [4]u32 = [0; 4];
    let queue: [4]u32 = [0; 4];
    let order_s: []u32 = order[0..];
    let waves_s: []DagWaveAssignment = waves[0..];
    let in_deg_s: []u32 = in_deg[0..];
    let queue_s: []u32 = queue[0..];

    const plan = planParallelWaves(4, edges[0..], order_s, waves_s, in_deg_s, queue_s) ?? return error.PlanFailed;
    expect_eq(plan.has_cycle, true);
    expect_eq(plan.scheduled_count, 1); // only node 0 scheduled
    expect_eq(plan.first_cycle_node, 1);

    let cyc_nodes: [4]u32 = [0; 4];
    let color: [4]u8 = [0; 4];
    let parent: [4]u32 = [0; 4];
    let cyc_s: []u32 = cyc_nodes[0..];
    let color_s: []u8 = color[0..];
    let parent_s: []u32 = parent[0..];

    const cyc_len = findCycleNodes(4, edges[0..], cyc_s, color_s, parent_s);
    expect_eq(cyc_len, 3);
    expect_eq(cyc_nodes[0], 1);
    expect_eq(cyc_nodes[1], 2);
    expect_eq(cyc_nodes[2], 3);

    // Self-loop: 2 -> 2
    const self_edges: [1]DagEdgeSpec = [DagEdgeSpec{ from: 2, to: 2 }];
    const self_len = findCycleNodes(4, self_edges[0..], cyc_s, color_s, parent_s);
    expect_eq(self_len, 1);
    expect_eq(cyc_nodes[0], 2);
}

// ============================================================================
// Test 4: Isolated Nodes & Acyclic findCycleNodes == 0
// ============================================================================

fn testIsolatedNodesAndAcyclic()! {
    const empty_edges: [0]DagEdgeSpec = [];
    const durs: [3]u64 = [10, 40, 25];
    let order: [3]u32 = [0; 3];
    let waves: [3]DagWaveAssignment = [DagWaveAssignment{}; 3];
    let in_deg: [3]u32 = [0; 3];
    let queue: [3]u32 = [0; 3];
    let order_s: []u32 = order[0..];
    let waves_s: []DagWaveAssignment = waves[0..];
    let in_deg_s: []u32 = in_deg[0..];
    let queue_s: []u32 = queue[0..];

    const plan = planParallelWaves(3, empty_edges[0..], order_s, waves_s, in_deg_s, queue_s) ?? return error.PlanFailed;
    expect_eq(plan.has_cycle, false);
    expect_eq(plan.wave_count, 1);
    expect_eq(plan.max_parallelism, 3);

    let metrics: [3]CriticalPathMetrics = [CriticalPathMetrics{}; 3];
    let metrics_s: []CriticalPathMetrics = metrics[0..];
    const makespan = analyzeCriticalPath(3, durs[0..], empty_edges[0..], order[0..], metrics_s) ?? return error.CpmFailed;
    expect_eq(makespan, 40);
    expect_eq(metrics[0].total_slack, 30);
    expect_eq(metrics[1].is_critical, true);
    expect_eq(metrics[2].total_slack, 15);

    let cyc: [3]u32 = [0; 3];
    let color: [3]u8 = [0; 3];
    let parent: [3]u32 = [0; 3];
    let cyc_s: []u32 = cyc[0..];
    let color_s: []u8 = color[0..];
    let parent_s: []u32 = parent[0..];
    expect_eq(findCycleNodes(3, empty_edges[0..], cyc_s, color_s, parent_s), 0);
}

// ============================================================================
// Test 5: Duplicate Edge Resilience
// ============================================================================

fn testDuplicateEdgesResilience()! {
    const dup_edges: [3]DagEdgeSpec = [
        DagEdgeSpec{ from: 0, to: 1 },
        DagEdgeSpec{ from: 0, to: 1 },
        DagEdgeSpec{ from: 0, to: 1 },
    ];
    const durs: [2]u64 = [15, 25];
    let order: [2]u32 = [0; 2];
    let waves: [2]DagWaveAssignment = [DagWaveAssignment{}; 2];
    let in_deg: [2]u32 = [0; 2];
    let queue: [2]u32 = [0; 2];
    let order_s: []u32 = order[0..];
    let waves_s: []DagWaveAssignment = waves[0..];
    let in_deg_s: []u32 = in_deg[0..];
    let queue_s: []u32 = queue[0..];

    const plan = planParallelWaves(2, dup_edges[0..], order_s, waves_s, in_deg_s, queue_s) ?? return error.PlanFailed;
    expect_eq(plan.has_cycle, false);
    expect_eq(plan.wave_count, 2);

    let metrics: [2]CriticalPathMetrics = [CriticalPathMetrics{}; 2];
    let metrics_s: []CriticalPathMetrics = metrics[0..];
    const makespan = analyzeCriticalPath(2, durs[0..], dup_edges[0..], order[0..], metrics_s) ?? return error.CpmFailed;
    expect_eq(makespan, 40);
}

// ============================================================================
// Test 6: Out-of-Bounds Nodes, Undersized Slices & Zero-Node Guards
// ============================================================================

fn testWorkflowDagBoundaryGuards()! {
    const bad_edges: [1]DagEdgeSpec = [DagEdgeSpec{ from: 0, to: 5 }];
    const ok_edges: [1]DagEdgeSpec = [DagEdgeSpec{ from: 0, to: 1 }];
    const durs: [2]u64 = [10, 20];
    let order: [2]u32 = [0; 2];
    let waves: [2]DagWaveAssignment = [DagWaveAssignment{}; 2];
    let in_deg: [2]u32 = [0; 2];
    let queue: [2]u32 = [0; 2];
    let order_s: []u32 = order[0..];
    let waves_s: []DagWaveAssignment = waves[0..];
    let in_deg_s: []u32 = in_deg[0..];
    let queue_s: []u32 = queue[0..];
    let small_order: []u32 = order[0..1];

    // Out-of-bounds edge -> null
    expect_eq(planParallelWaves(2, bad_edges[0..], order_s, waves_s, in_deg_s, queue_s) == null, true);
    // Undersized output buffer -> null
    expect_eq(planParallelWaves(2, ok_edges[0..], small_order, waves_s, in_deg_s, queue_s) == null, true);

    // Zero nodes -> empty summary / 0 makespan
    const z_plan = planParallelWaves(0, bad_edges[0..0], small_order, waves_s, in_deg_s, queue_s) ?? return error.ZeroFailed;
    expect_eq(z_plan.node_count, 0);

    let metrics: [2]CriticalPathMetrics = [CriticalPathMetrics{}; 2];
    let metrics_s: []CriticalPathMetrics = metrics[0..];
    expect_eq(analyzeCriticalPath(0, durs[0..], ok_edges[0..0], order[0..], metrics_s) ?? 99, 0);
    // Bad edge in CPM -> null
    expect_eq(analyzeCriticalPath(2, durs[0..], bad_edges[0..], order[0..], metrics_s) == null, true);
}

test("graph workflow_dag: diamond & multi-layer parallel wave partitioning", testDiamondParallelWaves);
test("graph workflow_dag: exact 6-node Critical Path Method (CPM) metrics", testCriticalPathMethodExact);
test("graph workflow_dag: cycle detection & 3-color DFS cycle extraction", testCycleDetectionAndExtraction);
test("graph workflow_dag: isolated nodes & acyclic findCycleNodes == 0", testIsolatedNodesAndAcyclic);
test("graph workflow_dag: duplicate edge resilience", testDuplicateEdgesResilience);
test("graph workflow_dag: out-of-bounds, undersized & zero-node boundary guards", testWorkflowDagBoundaryGuards);
