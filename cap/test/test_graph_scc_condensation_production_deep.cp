// Origin: Pure-CP Stdlib Test Suite (lib/graph/scc_condensation.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Iterative Tarjan SCC & Condensation DAG:
//   1. Pure acyclic DAG: N singleton SCCs, has_any_cycle == false & strict reverse-topo condensed edges
//   2. Single-node self-loop: scc_size == 1 yet isNodeInCycle == true & excluded from condensed_edges
//   3. Multi-cycle directed graph (3-cycle + bridge + 2-cycle + tail) exact SCC partition & sameScc
//   4. Condensation DAG deduplication of multiple inter-SCC edges & strict from_scc > to_scc invariant
//   5. 64-node full-depth chain & 64-node full cycle purely iterative with zero recursion
//   6. Parallel multi-edges & disconnected multi-component subgraphs
//   7. Invalid node counts (0, >64), out-of-bounds edges & OOB result query guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SCC_MAX_NODES,
    CondensedDagEdge,
    SccCondensationResult,
    IterativeTarjanGraph,
} from "cap:graph";

// ============================================================================
// Test 1: Pure Acyclic DAG -> Singleton SCCs & Reverse Topological Edges
// ============================================================================

fn testPureAcyclicDag()! {
    let g = cap.graph.IterativeTarjanGraph.init(5) ?? return error.InitFailed;
    // Diamond + tail: 0->1, 0->2, 1->3, 2->3, 3->4
    expect_eq(g.add_edge(0, 1), true);
    expect_eq(g.add_edge(0, 2), true);
    expect_eq(g.add_edge(1, 3), true);
    expect_eq(g.add_edge(2, 3), true);
    expect_eq(g.add_edge(3, 4), true);

    const res: SccCondensationResult = g.compute_scc_and_condensation();
    expect_eq(res.valid, true);
    expect_eq(res.scc_count, 5);
    expect_eq(res.cyclic_scc_count, 0);
    expect_eq(res.has_any_cycle, false);
    expect_eq(res.condensed_edge_count, 5);

    let u: usize = 0;
    while (u < 5) : (u += 1) {
        expect_eq(res.isNodeInCycle(u), false);
    }

    // Verify strict reverse-topological ordering: every condensed edge has from_scc > to_scc
    let e: usize = 0;
    while (e < res.condensed_edge_count) : (e += 1) {
        const edge: CondensedDagEdge = res.condensed_edges[e];
        expect_eq(edge.from_scc > edge.to_scc, true);
    }
}

// ============================================================================
// Test 2: Single-Node Self-Loop Cycle Detection
// ============================================================================

fn testSingleNodeSelfLoop()! {
    let g = IterativeTarjanGraph.init(3) ?? return error.InitFailed;
    expect_eq(g.add_edge(0, 1), true);
    expect_eq(g.add_edge(1, 1), true); // self-loop on node 1
    expect_eq(g.add_edge(1, 2), true);

    const res = g.compute_scc_and_condensation();
    expect_eq(res.scc_count, 3);
    expect_eq(res.cyclic_scc_count, 1);
    expect_eq(res.has_any_cycle, true);
    expect_eq(res.isNodeInCycle(0), false);
    expect_eq(res.isNodeInCycle(1), true);
    expect_eq(res.isNodeInCycle(2), false);
    // Self-loop is intra-SCC so only 2 inter-SCC condensed edges (0->1 and 1->2)
    expect_eq(res.condensed_edge_count, 2);
}

// ============================================================================
// Test 3: Classic Multi-Cycle Graph (3-Cycle + Bridge + 2-Cycle + Acyclic Tail)
// ============================================================================

fn testMultiCycleSccPartition()! {
    let g = IterativeTarjanGraph.init(6) ?? return error.InitFailed;
    // 3-cycle: 0 -> 1 -> 2 -> 0
    expect_eq(g.add_edge(0, 1), true);
    expect_eq(g.add_edge(1, 2), true);
    expect_eq(g.add_edge(2, 0), true);
    // Bridge: 2 -> 3
    expect_eq(g.add_edge(2, 3), true);
    // 2-cycle: 3 <-> 4
    expect_eq(g.add_edge(3, 4), true);
    expect_eq(g.add_edge(4, 3), true);
    // Acyclic tail: 4 -> 5
    expect_eq(g.add_edge(4, 5), true);

    const res = g.compute_scc_and_condensation();
    expect_eq(res.scc_count, 3);
    expect_eq(res.cyclic_scc_count, 2);
    expect_eq(res.has_any_cycle, true);
    expect_eq(res.condensed_edge_count, 2);

    // Exact component membership
    expect_eq(res.sameScc(0, 1), true);
    expect_eq(res.sameScc(1, 2), true);
    expect_eq(res.sameScc(3, 4), true);
    expect_eq(res.sameScc(2, 3), false);
    expect_eq(res.sameScc(4, 5), false);

    expect_eq(res.isNodeInCycle(0), true);
    expect_eq(res.isNodeInCycle(3), true);
    expect_eq(res.isNodeInCycle(5), false);

    // Reverse topological order: SCC({0,1,2}) > SCC({3,4}) > SCC({5})
    expect_eq(res.node_to_scc[0] > res.node_to_scc[3], true);
    expect_eq(res.node_to_scc[3] > res.node_to_scc[5], true);
}

// ============================================================================
// Test 4: Inter-SCC Condensation Edge Deduplication & Reverse-Topo Invariant
// ============================================================================

fn testCondensedDagEdgeDeduplication()! {
    let g = IterativeTarjanGraph.init(4) ?? return error.InitFailed;
    // SCC A = {0, 1}, SCC B = {2, 3}
    expect_eq(g.add_edge(0, 1), true);
    expect_eq(g.add_edge(1, 0), true);
    expect_eq(g.add_edge(2, 3), true);
    expect_eq(g.add_edge(3, 2), true);

    // 4 distinct cross-edges from SCC A -> SCC B must collapse into EXACTLY 1 condensed DAG edge!
    expect_eq(g.add_edge(0, 2), true);
    expect_eq(g.add_edge(0, 3), true);
    expect_eq(g.add_edge(1, 2), true);
    expect_eq(g.add_edge(1, 3), true);

    const res = g.compute_scc_and_condensation();
    expect_eq(res.scc_count, 2);
    expect_eq(res.condensed_edge_count, 1);
    expect_eq(res.condensed_edges[0].from_scc, res.node_to_scc[0]);
    expect_eq(res.condensed_edges[0].to_scc, res.node_to_scc[2]);
    expect_eq(res.condensed_edges[0].from_scc > res.condensed_edges[0].to_scc, true);
}

// ============================================================================
// Test 5: 64-Node Full-Depth Chain & 64-Node Cycle (Zero-Recursion Stress)
// ============================================================================

fn testMaxDepth64ChainAndCycle()! {
    // 5a. 64-node chain: 0 -> 1 -> ... -> 63
    let g_chain = IterativeTarjanGraph.init(SCC_MAX_NODES) ?? return error.InitFailed;
    let i: usize = 0;
    while (i + 1 < SCC_MAX_NODES) : (i += 1) {
        expect_eq(g_chain.add_edge(i, i + 1), true);
    }
    const r_chain = g_chain.compute_scc_and_condensation();
    expect_eq(r_chain.scc_count, SCC_MAX_NODES);
    expect_eq(r_chain.has_any_cycle, false);
    expect_eq(r_chain.condensed_edge_count, SCC_MAX_NODES - 1);

    // 5b. Closing the back-edge 63 -> 0 collapses all 64 nodes into 1 giant cyclic SCC!
    expect_eq(g_chain.add_edge(SCC_MAX_NODES - 1, 0), true);
    const r_cycle = g_chain.compute_scc_and_condensation();
    expect_eq(r_cycle.scc_count, 1);
    expect_eq(r_cycle.cyclic_scc_count, 1);
    expect_eq(r_cycle.has_any_cycle, true);
    expect_eq(r_cycle.scc_sizes[0], SCC_MAX_NODES as u16);
    expect_eq(r_cycle.condensed_edge_count, 0);
    expect_eq(r_cycle.sameScc(0, SCC_MAX_NODES - 1), true);
}

// ============================================================================
// Test 6: Parallel Multi-Edges & Disconnected Multi-Component Subgraphs
// ============================================================================

fn testParallelEdgesAndDisconnectedSubgraphs()! {
    // 5 nodes across 3 disconnected subgraphs:
    //   Subgraph 1: isolated node 0
    //   Subgraph 2: 2-cycle (1 <-> 2) with parallel duplicate edges
    //   Subgraph 3: acyclic edge (3 -> 4) duplicated twice
    let g = IterativeTarjanGraph.init(5) ?? return error.InitFailed;
    expect_eq(g.add_edge(1, 2), true);
    expect_eq(g.add_edge(1, 2), true);
    expect_eq(g.add_edge(2, 1), true);
    expect_eq(g.add_edge(3, 4), true);
    expect_eq(g.add_edge(3, 4), true);

    const res = g.compute_scc_and_condensation();
    expect_eq(res.scc_count, 4); // {0}, {1,2}, {3}, {4}
    expect_eq(res.cyclic_scc_count, 1);
    expect_eq(res.condensed_edge_count, 1); // deduplicated {3} -> {4}
    expect_eq(res.isNodeInCycle(0), false);
    expect_eq(res.isNodeInCycle(1), true);
    expect_eq(res.isNodeInCycle(3), false);
}

// ============================================================================
// Test 7: Invalid Node Count, Out-of-Bounds Edges & Query Guards
// ============================================================================

fn testInvalidBoundsGuards()! {
    expect_eq(IterativeTarjanGraph.init(0) == null, true);
    expect_eq(IterativeTarjanGraph.init(SCC_MAX_NODES + 1) == null, true);

    let g = IterativeTarjanGraph.init(3) ?? return error.InitFailed;
    expect_eq(g.add_edge(3, 0), false);
    expect_eq(g.add_edge(0, 3), false);
    expect_eq(g.add_edge(99, 100), false);

    const res = g.compute_scc_and_condensation();
    expect_eq(res.sameScc(0, 3), false);
    expect_eq(res.isNodeInCycle(3), false);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("graph scc_condensation: pure acyclic DAG yields singleton SCCs & reverse-topo edges", testPureAcyclicDag);
test("graph scc_condensation: single-node self-loop detected by isNodeInCycle & cyclic_scc_count", testSingleNodeSelfLoop);
test("graph scc_condensation: multi-cycle graph exact SCC partition, sameScc & reverse-topo order", testMultiCycleSccPartition);
test("graph scc_condensation: deduplicates parallel inter-SCC edges in condensed DAG", testCondensedDagEdgeDeduplication);
test("graph scc_condensation: 64-node full-depth chain & 64-node cycle run purely iteratively", testMaxDepth64ChainAndCycle);
test("graph scc_condensation: handles parallel multi-edges & disconnected subgraphs accurately", testParallelEdgesAndDisconnectedSubgraphs);
test("graph scc_condensation: guards invalid node count (0, >64), OOB edges & OOB queries", testInvalidBoundsGuards);
