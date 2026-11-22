// Origin: Pure-CP Stdlib Test Suite (AI GraphRAG Spreading Activation & Personalized PageRank) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agentmem/activation.cp
// Covers:
//   1. 2-hop & 3-hop chain spreading activation decay precision and min_threshold noise pruning
//   2. Strong vs weak branch edge-weight ranking and multi-path DAG convergence Top-K ordering
//   3. Bidirectional vs unidirectional edge propagation semantics
//   4. Personalized PageRank (PPR) probability conservation (sum == 1.0), convergence & entity re-ranking
//   5. Dangling-node mass recovery, empty graph, out-of-bounds/negative edges & invalid buffer/config guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MemGraphEdge,
    ActivatedNode,
    SpreadingConfig,
    spreadActivation,
    personalizedPageRank,
} from "../../lib/agentmem/index.cp";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const d = if (a > b) a - b else b - a;
    return d <= eps;
}

fn testChainSpreadingDecayAndThreshold()! {
    // 0 --(1.0)--> 1 --(1.0)--> 2 --(1.0)--> 3
    const edges: [3]MemGraphEdge = [
        MemGraphEdge{ src: 0, dst: 1, weight: 1.0 },
        MemGraphEdge{ src: 1, dst: 2, weight: 1.0 },
        MemGraphEdge{ src: 2, dst: 3, weight: 1.0 },
    ];
    const seeds: [1]u32 = [0];
    const seed_w: [1]f32 = [1.0];

    let buf_a: [4]f32 = [0.0; 4];
    let buf_b: [4]f32 = [0.0; 4];
    let top_buf: [4]ActivatedNode = [ActivatedNode{}; 4];
    let sa: []f32 = buf_a[0..];
    let sb: []f32 = buf_b[0..];
    let top: []ActivatedNode = top_buf[0..];

    // 3 hops with decay=0.5 -> exact scores: node0=1.0, node1=0.5, node2=0.25, node3=0.125
    const cfg3 = SpreadingConfig{
        decay: 0.5,
        hops: 3,
        min_threshold: 0.05,
        bidirectional: false,
    };
    const n3 = spreadActivation(4, edges[0..], seeds[0..], seed_w[0..], cfg3, sa, sb, top) ?? 0;
    expect_eq(n3, 4);
    expect_eq(top[0].node_id, 0);
    expect_eq(approxEq(top[0].score, 1.0, 0.0001), true);
    expect_eq(top[1].node_id, 1);
    expect_eq(approxEq(top[1].score, 0.5, 0.0001), true);
    expect_eq(top[2].node_id, 2);
    expect_eq(approxEq(top[2].score, 0.25, 0.0001), true);
    expect_eq(top[3].node_id, 3);
    expect_eq(approxEq(top[3].score, 0.125, 0.0001), true);

    // 2 hops -> node 3 is unreachable
    const cfg2 = SpreadingConfig{
        decay: 0.5,
        hops: 2,
        min_threshold: 0.05,
        bidirectional: false,
    };
    const n2 = spreadActivation(4, edges[0..], seeds[0..], seed_w[0..], cfg2, sa, sb, top) ?? 0;
    expect_eq(n2, 3);
    expect_eq(top[2].node_id, 2);

    // Threshold pruning: min_threshold=0.2 prunes node 3 (0.125 < 0.2) even with 3 hops
    const cfg_prune = SpreadingConfig{
        decay: 0.5,
        hops: 3,
        min_threshold: 0.2,
        bidirectional: false,
    };
    const n_pruned = spreadActivation(4, edges[0..], seeds[0..], seed_w[0..], cfg_prune, sa, sb, top) ?? 0;
    expect_eq(n_pruned, 3);
    expect_eq(top[2].node_id, 2);
}

fn testBranchWeightOrderingAndTopK()! {
    // Seed 0 fans out to strong branch 1 (w=0.9), weak branch 2 (w=0.2), medium branch 3 (w=0.6)
    // Plus diamond convergence: 1 -> 4 (w=0.8) and 3 -> 4 (w=0.8)
    const edges: [5]MemGraphEdge = [
        MemGraphEdge{ src: 0, dst: 1, weight: 0.9 },
        MemGraphEdge{ src: 0, dst: 2, weight: 0.2 },
        MemGraphEdge{ src: 0, dst: 3, weight: 0.6 },
        MemGraphEdge{ src: 1, dst: 4, weight: 0.8 },
        MemGraphEdge{ src: 3, dst: 4, weight: 0.8 },
    ];
    const seeds: [1]u32 = [0];
    const seed_w: [1]f32 = [1.0];

    let buf_a: [5]f32 = [0.0; 5];
    let buf_b: [5]f32 = [0.0; 5];
    let top_buf: [3]ActivatedNode = [ActivatedNode{}; 3];
    let sa: []f32 = buf_a[0..];
    let sb: []f32 = buf_b[0..];
    let top: []ActivatedNode = top_buf[0..];

    const cfg = SpreadingConfig{
        decay: 0.5,
        hops: 2,
        min_threshold: 0.01,
        bidirectional: false,
    };
    // Expected scores after 2 hops:
    // node 0: 1.0
    // node 1: 0.9 * 0.5 = 0.45
    // node 3: 0.6 * 0.5 = 0.30
    // node 4: (0.45 * 0.8 + 0.30 * 0.8) * 0.5 = (0.36 + 0.24) * 0.5 = 0.30 (tie with 3 -> node 3 wins by lower id)
    // node 2: 0.2 * 0.5 = 0.10
    const count = spreadActivation(5, edges[0..], seeds[0..], seed_w[0..], cfg, sa, sb, top) ?? 0;
    expect_eq(count, 3);
    expect_eq(top[0].node_id, 0);
    expect_eq(top[1].node_id, 1);
    expect_eq(approxEq(top[1].score, 0.45, 0.0001), true);
    expect_eq(top[2].node_id, 3);
    expect_eq(approxEq(top[2].score, 0.30, 0.0001), true);
    expect_eq(approxEq(sa[4], 0.30, 0.0001), true);
    expect_eq(approxEq(sa[2], 0.10, 0.0001), true);
}

fn testBidirectionalVsUnidirectionalPropagation()! {
    // Directed edge points 1 -> 0 and 2 -> 1 (incoming into seed 0)
    const edges: [2]MemGraphEdge = [
        MemGraphEdge{ src: 1, dst: 0, weight: 0.8 },
        MemGraphEdge{ src: 2, dst: 1, weight: 0.5 },
    ];
    const seeds: [1]u32 = [0];
    const seed_w: [1]f32 = [1.0];

    let buf_a: [3]f32 = [0.0; 3];
    let buf_b: [3]f32 = [0.0; 3];
    let top_buf: [3]ActivatedNode = [ActivatedNode{}; 3];
    let sa: []f32 = buf_a[0..];
    let sb: []f32 = buf_b[0..];
    let top: []ActivatedNode = top_buf[0..];

    // Unidirectional: seed 0 has no outgoing edges -> only node 0 activated
    const uni_cfg = SpreadingConfig{
        decay: 0.5,
        hops: 2,
        min_threshold: 0.01,
        bidirectional: false,
    };
    const n_uni = spreadActivation(3, edges[0..], seeds[0..], seed_w[0..], uni_cfg, sa, sb, top) ?? 0;
    expect_eq(n_uni, 1);
    expect_eq(top[0].node_id, 0);

    // Bidirectional: propagates backwards 0 -> 1 -> 2
    const bi_cfg = SpreadingConfig{
        decay: 0.5,
        hops: 2,
        min_threshold: 0.01,
        bidirectional: true,
    };
    const n_bi = spreadActivation(3, edges[0..], seeds[0..], seed_w[0..], bi_cfg, sa, sb, top) ?? 0;
    expect_eq(n_bi, 3);
    expect_eq(top[0].node_id, 0);
    expect_eq(top[1].node_id, 1);
    expect_eq(approxEq(top[1].score, 0.4, 0.0001), true);
    expect_eq(top[2].node_id, 2);
    expect_eq(approxEq(top[2].score, 0.1, 0.0001), true);
}

fn testPersonalizedPageRankConservationAndConvergence()! {
    // 4-node graph:
    // 0 -> 1 (w=3.0), 0 -> 2 (w=1.0), 1 -> 3 (w=1.0), 2 -> 3 (w=1.0), 3 -> 0 (w=1.0)
    const edges: [5]MemGraphEdge = [
        MemGraphEdge{ src: 0, dst: 1, weight: 3.0 },
        MemGraphEdge{ src: 0, dst: 2, weight: 1.0 },
        MemGraphEdge{ src: 1, dst: 3, weight: 1.0 },
        MemGraphEdge{ src: 2, dst: 3, weight: 1.0 },
        MemGraphEdge{ src: 3, dst: 0, weight: 1.0 },
    ];
    const seeds: [1]u32 = [0];
    const seed_w: [1]f32 = [1.0];

    let cur_buf: [4]f32 = [0.0; 4];
    let nxt_buf: [4]f32 = [0.0; 4];
    let deg_buf: [4]f32 = [0.0; 4];
    let top_buf: [4]ActivatedNode = [ActivatedNode{}; 4];
    let cur: []f32 = cur_buf[0..];
    let nxt: []f32 = nxt_buf[0..];
    let deg: []f32 = deg_buf[0..];
    let top: []ActivatedNode = top_buf[0..];

    const k = personalizedPageRank(
        4,
        edges[0..],
        seeds[0..],
        seed_w[0..],
        0.85,
        100,
        0.00001,
        cur,
        nxt,
        deg,
        top,
    ) ?? 0;

    expect_eq(k, 4);

    // Probability mass must be conserved: sum(cur) == 1.0
    const total_mass = cur[0] + cur[1] + cur[2] + cur[3];
    expect_eq(approxEq(total_mass, 1.0, 0.0005), true);

    // Seed node 0 has highest rank; strong neighbor 1 ranks strictly higher than weak neighbor 2
    expect_eq(top[0].node_id, 0);
    expect_eq(cur[1] > cur[2], true);
    // Exact ratio between node 1 and node 2 is 3:1
    expect_eq(approxEq(cur[1] / cur[2], 3.0, 0.005), true);
}

fn testDanglingNodesEmptyGraphAndInvalidGuards()! {
    // 1. Dangling sink node (0 -> 1, node 1 has out_deg == 0): mass must be recovered to seed 0
    const sink_edges: [3]MemGraphEdge = [
        MemGraphEdge{ src: 0, dst: 1, weight: 1.0 },
        // Out-of-bounds & negative weight edges must be safely ignored
        MemGraphEdge{ src: 0, dst: 99, weight: 5.0 },
        MemGraphEdge{ src: 1, dst: 0, weight: -2.0 },
    ];
    const seeds: [1]u32 = [0];
    const seed_w: [1]f32 = [2.0];

    let cur_buf: [2]f32 = [0.0; 2];
    let nxt_buf: [2]f32 = [0.0; 2];
    let deg_buf: [2]f32 = [0.0; 2];
    let top_buf: [2]ActivatedNode = [ActivatedNode{}; 2];
    let cur: []f32 = cur_buf[0..];
    let nxt: []f32 = nxt_buf[0..];
    let deg: []f32 = deg_buf[0..];
    let top: []ActivatedNode = top_buf[0..];

    const k_sink = personalizedPageRank(
        2,
        sink_edges[0..],
        seeds[0..],
        seed_w[0..],
        0.8,
        50,
        0.00001,
        cur,
        nxt,
        deg,
        top,
    ) ?? 0;
    expect_eq(k_sink, 2);
    // Analytical steady state for 0 -> 1 (dangling) with d=0.8:
    // r1 = 0.8 * r0, r0 + r1 = 1 => r0 = 1/1.8 = 0.5556, r1 = 0.8/1.8 = 0.4444
    expect_eq(approxEq(cur[0] + cur[1], 1.0, 0.0005), true);
    expect_eq(approxEq(cur[0], 1.0 / 1.8, 0.002), true);
    expect_eq(approxEq(cur[1], 0.8 / 1.8, 0.002), true);

    // 2. Empty graph (num_nodes == 0) returns 0 without error
    const empty_edges: [0]MemGraphEdge = [];
    const empty_k1 = spreadActivation(0, empty_edges[0..], seeds[0..], seed_w[0..], SpreadingConfig{}, cur, nxt, top);
    expect_eq(empty_k1, 0);
    const empty_k2 = personalizedPageRank(0, empty_edges[0..], seeds[0..], seed_w[0..], 0.85, 10, 0.001, cur, nxt, deg, top);
    expect_eq(empty_k2, 0);

    // 3. Invalid config / mismatched seed arrays / undersized scratch buffers return null
    const bad_cfg = SpreadingConfig{ decay: 1.5, hops: 2, min_threshold: 0.01, bidirectional: false };
    expect_eq(spreadActivation(2, sink_edges[0..], seeds[0..], seed_w[0..], bad_cfg, cur, nxt, top) == null, true);

    const short_w: [0]f32 = [];
    expect_eq(spreadActivation(2, sink_edges[0..], seeds[0..], short_w[0..], SpreadingConfig{}, cur, nxt, top) == null, true);

    // Undersized scratch buffer (num_nodes=5 > cur.len=2)
    expect_eq(spreadActivation(5, sink_edges[0..], seeds[0..], seed_w[0..], SpreadingConfig{}, cur, nxt, top) == null, true);
    expect_eq(personalizedPageRank(5, sink_edges[0..], seeds[0..], seed_w[0..], 0.85, 10, 0.001, cur, nxt, deg, top) == null, true);

    // Invalid damping factor (>= 1.0 or <= 0.0)
    expect_eq(personalizedPageRank(2, sink_edges[0..], seeds[0..], seed_w[0..], 1.0, 10, 0.001, cur, nxt, deg, top) == null, true);
    expect_eq(personalizedPageRank(2, sink_edges[0..], seeds[0..], seed_w[0..], 0.0, 10, 0.001, cur, nxt, deg, top) == null, true);
}

test("agentmem activation: 2-hop & 3-hop chain spreading decay precision and threshold pruning", testChainSpreadingDecayAndThreshold);
test("agentmem activation: branch edge-weight ranking and multi-path DAG Top-K ordering", testBranchWeightOrderingAndTopK);
test("agentmem activation: bidirectional vs unidirectional propagation semantics", testBidirectionalVsUnidirectionalPropagation);
test("agentmem activation: Personalized PageRank probability conservation and convergence", testPersonalizedPageRankConservationAndConvergence);
test("agentmem activation: dangling-node recovery, empty graph, OOB edges and invalid guards", testDanglingNodesEmptyGraphAndInvalidGuards);
