// Origin: Pure-CP Stdlib Test Suite (lib/graph/dominator.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Iterative Cooper-Harvey-Kennedy Dominator Tree & Dominance Frontiers:
//   1. Classic diamond CFG: idom, domDepth, dominates/strictlyDominates, dom children & Dominance Frontiers
//   2. Nested natural loops & self-loops: exact back-edge detection (`isBackEdge`, `collectBackEdges`) & loop DF
//   3. Irreducible two-entry loop: CHK convergence, mutual dominance frontiers & non-back-edge verification
//   4. Unreachable dead-code isolation (including edges from dead nodes into live join blocks)
//   5. 64-node deep chain iterative stress test + back-edge into entry node (`entry ∈ DF(63)`)
//   6. Invalid init/entry/OOB/capacity guards & truncation-safe collectors
//   7. `cap.graph` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    DOM_MAX_NODES,
    DOM_MAX_EDGES,
    DomEdge,
    DominatorGraph,
} from "cap:graph";

test("Dominator: classic diamond CFG idom, domDepth, children & Dominance Frontiers") {
    // 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3, 3 -> 4
    let g = DominatorGraph.init(5) ?? return;
    expect_true(g.addEdge(0, 1) != null);
    expect_true(g.addEdge(0, 2) != null);
    expect_true(g.addEdge(1, 3) != null);
    expect_true(g.addEdge(2, 3) != null);
    expect_true(g.addEdge(3, 4) != null);

    const dom = g.compute(0);
    expect_true(dom.valid);
    expect_eq(dom.reachable_count, 5 as usize);
    expect_true(dom.iterations >= 1);

    // Entry has no immediate dominator and depth 0
    expect_true(dom.idom(0) == null);
    expect_eq(dom.domDepth(0) ?? 99, 0 as usize);

    // 1, 2, 3 are all immediately dominated by 0; 4 is immediately dominated by 3
    expect_eq(dom.idom(1) ?? 99, 0 as usize);
    expect_eq(dom.idom(2) ?? 99, 0 as usize);
    expect_eq(dom.idom(3) ?? 99, 0 as usize);
    expect_eq(dom.idom(4) ?? 99, 3 as usize);

    expect_eq(dom.domDepth(1) ?? 99, 1 as usize);
    expect_eq(dom.domDepth(3) ?? 99, 1 as usize);
    expect_eq(dom.domDepth(4) ?? 99, 2 as usize);

    // Dominance & strict dominance
    expect_true(dom.dominates(0, 4));
    expect_true(dom.strictlyDominates(0, 4));
    expect_true(dom.dominates(3, 3));
    expect_true(!dom.strictlyDominates(3, 3));
    expect_true(dom.dominates(3, 4));
    expect_true(!dom.dominates(1, 3));
    expect_true(!dom.dominates(2, 4));

    // Dominator tree children of 0: [1, 2, 3]
    let children_buf: [8]usize = [0; 8];
    let children_slice: []usize = children_buf[0..];
    expect_eq(dom.collectDomChildren(0, children_slice), 3 as usize);
    expect_eq(children_buf[0], 1 as usize);
    expect_eq(children_buf[1], 2 as usize);
    expect_eq(children_buf[2], 3 as usize);

    // Dominance Frontiers: DF(1) = {3}, DF(2) = {3}, DF(0) = DF(3) = DF(4) = {}
    expect_true(dom.inFrontier(1, 3));
    expect_true(dom.inFrontier(2, 3));
    expect_eq(dom.frontierMask(0), 0 as u64);
    expect_eq(dom.frontierMask(3), 0 as u64);
    expect_eq(dom.frontierMask(4), 0 as u64);

    let df_buf: [4]usize = [0; 4];
    let df_slice: []usize = df_buf[0..];
    expect_eq(dom.collectFrontier(1, df_slice), 1 as usize);
    expect_eq(df_buf[0], 3 as usize);
}

test("Dominator: nested natural loop & self-loop back-edge detection + loop DF") {
    // 0 -> 1 -> 2 -> 3 -> 1 (outer loop), 2 -> 2 (self-loop), 1 -> 4 (exit)
    let g = DominatorGraph.init(5) ?? return;
    * = g.addEdge(0, 1);
    * = g.addEdge(1, 2);
    * = g.addEdge(2, 2);
    * = g.addEdge(2, 3);
    * = g.addEdge(3, 1);
    * = g.addEdge(1, 4);

    const dom = g.compute(0);
    expect_true(dom.valid);
    expect_eq(dom.idom(1) ?? 99, 0 as usize);
    expect_eq(dom.idom(2) ?? 99, 1 as usize);
    expect_eq(dom.idom(3) ?? 99, 2 as usize);
    expect_eq(dom.idom(4) ?? 99, 1 as usize);

    // Back-edges: (2 -> 2) and (3 -> 1)
    expect_true(dom.isBackEdge(2, 2));
    expect_true(dom.isBackEdge(3, 1));
    expect_true(!dom.isBackEdge(1, 2)); // forward tree edge
    expect_true(!dom.isBackEdge(4, 1)); // non-existent edge must return false

    let back_buf: [8]DomEdge = [DomEdge{}; 8];
    let back_slice: []DomEdge = back_buf[0..];
    expect_eq(dom.collectBackEdges(back_slice), 2 as usize);
    expect_eq(back_buf[0].u, 2 as u16);
    expect_eq(back_buf[0].v, 2 as u16);
    expect_eq(back_buf[1].u, 3 as u16);
    expect_eq(back_buf[1].v, 1 as u16);

    // Loop header 1 is in DF(1), DF(2), DF(3); self-loop 2 is in DF(2)
    expect_true(dom.inFrontier(1, 1));
    expect_true(dom.inFrontier(2, 2));
    expect_true(dom.inFrontier(2, 1));
    expect_true(dom.inFrontier(3, 1));
    expect_eq(dom.frontierMask(4), 0 as u64);
}

test("Dominator: irreducible two-entry loop convergence & mutual Dominance Frontiers") {
    // 0 -> 1, 0 -> 2, 1 -> 2, 2 -> 1, 1 -> 3, 2 -> 3
    let g = DominatorGraph.init(4) ?? return;
    * = g.addEdge(0, 1);
    * = g.addEdge(0, 2);
    * = g.addEdge(1, 2);
    * = g.addEdge(2, 1);
    * = g.addEdge(1, 3);
    * = g.addEdge(2, 3);

    const dom = g.compute(0);
    expect_true(dom.valid);
    expect_eq(dom.reachable_count, 4 as usize);

    // All of 1, 2, 3 are immediately dominated by 0
    expect_eq(dom.idom(1) ?? 99, 0 as usize);
    expect_eq(dom.idom(2) ?? 99, 0 as usize);
    expect_eq(dom.idom(3) ?? 99, 0 as usize);

    // Irreducible cycle edges are NOT natural back-edges (neither dominates the other)
    expect_true(!dom.dominates(1, 2));
    expect_true(!dom.dominates(2, 1));
    expect_true(!dom.isBackEdge(1, 2));
    expect_true(!dom.isBackEdge(2, 1));

    // Mutual dominance frontiers: DF(1) = {2, 3}, DF(2) = {1, 3}
    let df1_buf: [4]usize = [0; 4];
    let df1_slice: []usize = df1_buf[0..];
    expect_eq(dom.collectFrontier(1, df1_slice), 2 as usize);
    expect_eq(df1_buf[0], 2 as usize);
    expect_eq(df1_buf[1], 3 as usize);

    let df2_buf: [4]usize = [0; 4];
    let df2_slice: []usize = df2_buf[0..];
    expect_eq(dom.collectFrontier(2, df2_slice), 2 as usize);
    expect_eq(df2_buf[0], 1 as usize);
    expect_eq(df2_buf[1], 3 as usize);
}

test("Dominator: unreachable dead-code nodes are isolated from live idom & DF") {
    // Live path: 0 -> 1 -> 2; Dead subgraph: 3 -> 4, 4 -> 1, 4 -> 2
    let g = DominatorGraph.init(5) ?? return;
    * = g.addEdge(0, 1);
    * = g.addEdge(1, 2);
    * = g.addEdge(3, 4);
    * = g.addEdge(4, 1);
    * = g.addEdge(4, 2);

    const dom = g.compute(0);
    expect_true(dom.valid);
    expect_eq(dom.reachable_count, 3 as usize);

    expect_true(dom.isReachable(0));
    expect_true(dom.isReachable(1));
    expect_true(dom.isReachable(2));
    expect_true(!dom.isReachable(3));
    expect_true(!dom.isReachable(4));

    expect_true(dom.idom(3) == null);
    expect_true(dom.domDepth(4) == null);
    expect_true(!dom.dominates(0, 3));
    expect_true(!dom.dominates(3, 3));

    // Dead predecessors must NOT corrupt live idom or create spurious frontiers
    expect_eq(dom.idom(1) ?? 99, 0 as usize);
    expect_eq(dom.idom(2) ?? 99, 1 as usize);
    expect_eq(dom.frontierMask(0), 0 as u64);
    expect_eq(dom.frontierMask(1), 0 as u64);
    expect_eq(dom.frontierMask(2), 0 as u64);
}

test("Dominator: 64-node deep chain iterative stress & entry back-edge DF") {
    let g = DominatorGraph.init(DOM_MAX_NODES) ?? return;
    let i: usize = 0;
    while (i + 1 < DOM_MAX_NODES) : (i += 1) {
        expect_true(g.addEdge(i, i + 1) != null);
    }
    // Add back-edge from tail (63) directly to entry (0)
    expect_true(g.addEdge(DOM_MAX_NODES - 1, 0) != null);

    const dom = g.compute(0);
    expect_true(dom.valid);
    expect_eq(dom.reachable_count, DOM_MAX_NODES);
    expect_eq(dom.domDepth(DOM_MAX_NODES - 1) ?? 0, DOM_MAX_NODES - 1);
    expect_eq(dom.idom(DOM_MAX_NODES - 1) ?? 0, DOM_MAX_NODES - 2);
    expect_true(dom.dominates(0, DOM_MAX_NODES - 1));
    expect_true(dom.isBackEdge(DOM_MAX_NODES - 1, 0));

    // Back-edge 63 -> 0 places entry 0 in DF(63) (and every node on the spine 1..63)
    expect_true(dom.inFrontier(DOM_MAX_NODES - 1, 0));
    expect_true(dom.inFrontier(1, 0));
}

test("Dominator: invalid init, OOB entry/edge, edge capacity & collector truncation guards") {
    expect_true(DominatorGraph.init(0) == null);
    expect_true(DominatorGraph.init(DOM_MAX_NODES + 1) == null);

    let g = DominatorGraph.init(4) ?? return;
    expect_true(g.addEdge(0, 4) == null);
    expect_true(g.addEdge(4, 0) == null);

    // Out-of-bounds entry returns invalid result safely
    const bad = g.compute(4);
    expect_true(!bad.valid);
    expect_eq(bad.reachable_count, 0 as usize);
    expect_true(!bad.isReachable(0));
    expect_true(bad.idom(0) == null);
    expect_true(!bad.dominates(0, 0));

    // Truncation-safe collector with 1-slot buffer
    * = g.addEdge(0, 1);
    * = g.addEdge(0, 2);
    * = g.addEdge(0, 3);
    const dom = g.compute(0);
    let tiny: [1]usize = [0; 1];
    let tiny_slice: []usize = tiny[0..];
    expect_eq(dom.collectDomChildren(0, tiny_slice), 1 as usize);
    expect_eq(tiny[0], 1 as usize);

    // Fill edge capacity up to DOM_MAX_EDGES (already added 3 edges)
    let e: usize = 3;
    while (e < DOM_MAX_EDGES) : (e += 1) {
        expect_true(g.addEdge(1, 2) != null);
    }
    expect_true(g.addEdge(1, 2) == null);
}

test("Dominator: cap.graph namespace facade parity") {
    let g = cap.graph.DominatorGraph.init(6) ?? return;
    // Classic compiler CFG: 0 -> 1, 1 -> {2, 3}, 2 -> 4, 3 -> 4, 4 -> 5, 4 -> 1
    * = g.addEdge(0, 1);
    * = g.addEdge(1, 2);
    * = g.addEdge(1, 3);
    * = g.addEdge(2, 4);
    * = g.addEdge(3, 4);
    * = g.addEdge(4, 5);
    * = g.addEdge(4, 1);

    const dom = g.compute(0);
    expect_true(dom.valid);
    expect_eq(dom.reachable_count, 6 as usize);
    expect_eq(dom.idom(4) ?? 99, 1 as usize);
    expect_eq(dom.idom(5) ?? 99, 4 as usize);
    expect_true(dom.isBackEdge(4, 1));
    expect_true(dom.inFrontier(2, 4));
    expect_true(dom.inFrontier(3, 4));
    expect_true(dom.inFrontier(4, 1));
}
