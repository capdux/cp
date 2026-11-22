// Origin: Pure-CP Stdlib Test Suite (lib/graph/articulation.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Iterative Tarjan Articulation Points, Bridges & SPOF Analyzer:
//   1. Classic topologies: chain, star, pure cycle & 2-ring dumbbell graph (SPOF + bridge + BCC)
//   2. Parallel multi-edges between nodes must NEVER be falsely reported as bridges
//   3. Disconnected multi-component forest (isolated nodes, bridge chain, resilient triangle)
//   4. Self-loops, out-of-bounds guards & empty/single-node boundary cases
//   5. Full 64-node deep chain stress test verifying iterative explicit stack (zero recursion)
//   6. `collectArticulations`/`collectBridges` truncation safety & `cap.graph` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    ARTIC_MAX_NODES,
    ARTIC_MAX_EDGES,
    ArticulationResult,
    ArticulationGraph,
} from "cap:graph";

test("Articulation: chain, star, pure cycle & double-ring dumbbell topology") {
    // 1. Chain 0-1-2-3: articulations={1,2}, bridges={0,1,2}
    let chain = ArticulationGraph.init(4) ?? return;
    const c0 = chain.addEdge(0, 1) ?? 0;
    const c1 = chain.addEdge(1, 2) ?? 0;
    const c2 = chain.addEdge(2, 3) ?? 0;
    const r_chain = chain.analyze();
    expect_eq(r_chain.connected_components, 1 as usize);
    expect_eq(r_chain.articulation_count, 2 as usize);
    expect_eq(r_chain.bridge_count, 3 as usize);
    expect_eq(r_chain.is_two_edge_connected, false);
    expect_eq(r_chain.isArticulation(0), false);
    expect_true(r_chain.isArticulation(1));
    expect_true(r_chain.isArticulation(2));
    expect_eq(r_chain.isArticulation(3), false);
    expect_true(r_chain.isBridge(c0) && r_chain.isBridge(c1) && r_chain.isBridge(c2));

    // 2. Star centered at 0 with 4 leaves: only node 0 is SPOF, all 4 edges are bridges
    let star = ArticulationGraph.init(5) ?? return;
    * = star.addEdge(0, 1);
    * = star.addEdge(0, 2);
    * = star.addEdge(0, 3);
    * = star.addEdge(0, 4);
    const r_star = star.analyze();
    expect_eq(r_star.articulation_count, 1 as usize);
    expect_true(r_star.isArticulation(0));
    expect_eq(r_star.bridge_count, 4 as usize);
    expect_eq(r_star.bcc_count, 4 as usize);
    expect_eq(r_star.max_bcc_edges, 1 as usize);

    // 3. Pure 4-cycle 0-1-2-3-0: zero articulations, zero bridges, 2-edge-connected & biconnected
    let cyc = ArticulationGraph.init(4) ?? return;
    const e0 = cyc.addEdge(0, 1) ?? 0;
    const e1 = cyc.addEdge(1, 2) ?? 0;
    const e2 = cyc.addEdge(2, 3) ?? 0;
    const e3 = cyc.addEdge(3, 0) ?? 0;
    const r_cyc = cyc.analyze();
    expect_eq(r_cyc.articulation_count, 0 as usize);
    expect_eq(r_cyc.bridge_count, 0 as usize);
    expect_eq(r_cyc.bcc_count, 1 as usize);
    expect_eq(r_cyc.max_bcc_edges, 4 as usize);
    expect_true(r_cyc.is_two_edge_connected);
    expect_true(r_cyc.is_biconnected);
    expect_eq(r_cyc.edgeBcc(e0) ?? 99, r_cyc.edgeBcc(e3) ?? 98);
    * = e1;
    * = e2;

    // 4. Double-ring dumbbell: (0-1-2-0) --bridge(2-3)-- (3-4-5-3)
    let dumb = ArticulationGraph.init(6) ?? return;
    * = dumb.addEdge(0, 1);
    * = dumb.addEdge(1, 2);
    * = dumb.addEdge(2, 0);
    const br = dumb.addEdge(2, 3) ?? 0;
    * = dumb.addEdge(3, 4);
    * = dumb.addEdge(4, 5);
    * = dumb.addEdge(5, 3);
    const r_dumb = dumb.analyze();
    expect_eq(r_dumb.articulation_count, 2 as usize);
    expect_true(r_dumb.isArticulation(2));
    expect_true(r_dumb.isArticulation(3));
    expect_eq(r_dumb.bridge_count, 1 as usize);
    expect_true(r_dumb.isBridge(br));
    expect_eq(r_dumb.bcc_count, 3 as usize);
    expect_eq(r_dumb.max_bcc_edges, 3 as usize);
}

test("Articulation: parallel multi-edges are never falsely reported as bridges") {
    // 0 ===(2 parallel edges)=== 1 ---(single bridge)--- 2 ===(2 parallel edges)=== 3
    let g = ArticulationGraph.init(4) ?? return;
    const p0 = g.addEdge(0, 1) ?? 0;
    const p1 = g.addEdge(0, 1) ?? 0; // parallel redundant link
    const mid = g.addEdge(1, 2) ?? 0; // single non-redundant bridge
    const p2 = g.addEdge(2, 3) ?? 0;
    const p3 = g.addEdge(3, 2) ?? 0; // parallel redundant link (reversed endpoints)

    const res = g.analyze();
    expect_eq(res.connected_components, 1 as usize);
    expect_eq(res.bridge_count, 1 as usize);
    expect_eq(res.isBridge(p0), false);
    expect_eq(res.isBridge(p1), false);
    expect_true(res.isBridge(mid));
    expect_eq(res.isBridge(p2), false);
    expect_eq(res.isBridge(p3), false);

    // Parallel edges share the same BCC
    expect_eq(res.edgeBcc(p0) ?? 99, res.edgeBcc(p1) ?? 98);
    expect_eq(res.edgeBcc(p2) ?? 99, res.edgeBcc(p3) ?? 98);

    // Pure 2-node parallel-edge graph is 2-edge-connected
    let g2 = ArticulationGraph.init(2) ?? return;
    * = g2.addEdge(0, 1);
    * = g2.addEdge(0, 1);
    const r2 = g2.analyze();
    expect_eq(r2.bridge_count, 0 as usize);
    expect_eq(r2.articulation_count, 0 as usize);
    expect_true(r2.is_two_edge_connected);
    expect_true(r2.is_biconnected);
}

test("Articulation: disconnected multi-component forest") {
    // Comp A: triangle (0-1-2-0) -> no SPOF, no bridges
    // Comp B: chain (3-4-5) -> SPOF={4}, bridges={(3,4),(4,5)}
    // Comp C: isolated node 6
    let g = ArticulationGraph.init(7) ?? return;
    * = g.addEdge(0, 1);
    * = g.addEdge(1, 2);
    * = g.addEdge(2, 0);
    const b1 = g.addEdge(3, 4) ?? 0;
    const b2 = g.addEdge(4, 5) ?? 0;

    const res = g.analyze();
    expect_eq(res.connected_components, 3 as usize);
    expect_eq(res.is_two_edge_connected, false);
    expect_eq(res.articulation_count, 1 as usize);
    expect_true(res.isArticulation(4));
    expect_eq(res.bridge_count, 2 as usize);
    expect_true(res.isBridge(b1) && res.isBridge(b2));

    // Nodes in different components have distinct component IDs
    const c_a = res.nodeComponent(0) ?? 99;
    const c_b = res.nodeComponent(3) ?? 99;
    const c_c = res.nodeComponent(6) ?? 99;
    expect_eq(c_a, res.nodeComponent(2) ?? 98);
    expect_eq(c_b, res.nodeComponent(5) ?? 98);
    expect_true(c_a != c_b && c_b != c_c && c_a != c_c);
}

test("Articulation: self-loops, empty/single-node & out-of-bounds guards") {
    expect_true(ArticulationGraph.init(ARTIC_MAX_NODES + 1) == null);

    // Empty graph
    let empty = ArticulationGraph.init(0) ?? return;
    const r_empty = empty.analyze();
    expect_eq(r_empty.connected_components, 0 as usize);
    expect_true(r_empty.is_two_edge_connected);

    // Single node with self-loop
    let single = ArticulationGraph.init(1) ?? return;
    const sl = single.addEdge(0, 0) ?? 0;
    expect_true(single.addEdge(0, 1) == null); // OOB v
    expect_true(single.addEdge(1, 0) == null); // OOB u
    const r_single = single.analyze();
    expect_eq(r_single.connected_components, 1 as usize);
    expect_eq(r_single.articulation_count, 0 as usize);
    expect_eq(r_single.bridge_count, 0 as usize);
    expect_eq(r_single.isBridge(sl), false);
    expect_true(r_single.edgeBcc(sl) == null);
    expect_true(r_single.is_two_edge_connected);

    // OOB queries on result are safe
    expect_eq(r_single.isArticulation(99), false);
    expect_eq(r_single.isBridge(99), false);
    expect_true(r_single.nodeComponent(99) == null);
}

test("Articulation: full 64-node deep chain stress test (100% iterative explicit stack)") {
    let g = ArticulationGraph.init(ARTIC_MAX_NODES) ?? return;
    let i: usize = 0;
    while (i < ARTIC_MAX_NODES - 1) : (i += 1) {
        expect_true(g.addEdge(i, i + 1) != null);
    }

    const res = g.analyze();
    expect_eq(res.connected_components, 1 as usize);
    expect_eq(res.articulation_count, ARTIC_MAX_NODES - 2);
    expect_eq(res.bridge_count, ARTIC_MAX_NODES - 1);
    expect_eq(res.isArticulation(0), false);
    expect_eq(res.isArticulation(ARTIC_MAX_NODES - 1), false);
    expect_true(res.isArticulation(1));
    expect_true(res.isArticulation(32));
    expect_true(res.isArticulation(ARTIC_MAX_NODES - 2));

    // Closing the chain into a 64-node ring eliminates every SPOF and every bridge
    expect_true(g.addEdge(ARTIC_MAX_NODES - 1, 0) != null);
    const r_ring = g.analyze();
    expect_eq(r_ring.articulation_count, 0 as usize);
    expect_eq(r_ring.bridge_count, 0 as usize);
    expect_eq(r_ring.bcc_count, 1 as usize);
    expect_eq(r_ring.max_bcc_edges, ARTIC_MAX_NODES);
    expect_true(r_ring.is_two_edge_connected);
    expect_true(r_ring.is_biconnected);
}

test("Articulation: collectArticulations/collectBridges truncation safety & cap.graph facade parity") {
    let g = cap.graph.ArticulationGraph.init(5) ?? return;
    // Bowtie + tail: (0-1-2-0) + (2-3) + (3-4) -> articulations={2,3}, bridges={(2,3),(3,4)}
    * = g.addEdge(0, 1);
    * = g.addEdge(1, 2);
    * = g.addEdge(2, 0);
    const b0 = g.addEdge(2, 3) ?? 0;
    const b1 = g.addEdge(3, 4) ?? 0;

    const res: ArticulationResult = g.analyze();
    let art_buf: [1]usize = [0; 1];
    let art_s: []usize = art_buf[0..];
    // Truncated to capacity 1 cleanly
    expect_eq(res.collectArticulations(*art_s), 1 as usize);
    expect_eq(art_buf[0], 2 as usize);

    let br_buf: [4]usize = [0; 4];
    let br_s: []usize = br_buf[0..];
    expect_eq(res.collectBridges(*br_s), 2 as usize);
    expect_eq(br_buf[0], b0);
    expect_eq(br_buf[1], b1);
    expect_eq(ARTIC_MAX_EDGES, 256 as usize);
}
