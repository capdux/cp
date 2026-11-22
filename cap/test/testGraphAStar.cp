// Origin: Pure-CP Stdlib Test Suite (lib/graph/aStar.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc A* / Weighted A* Shortest-Path Engine:
//   1. 2D 6x6 grid with vertical obstacle wall: optimal detour cost & path continuity
//   2. A* (`manhattan` / `euclidean`) vs `zero_dijkstra` significant expansion reduction
//   3. All 5 heuristic formulas (`computeAStarHeuristic`) & Weighted A* (`heuristic_weight > 1.0`)
//   4. Unreachable target (sealed wall & blocked goal/start) returns `.unreachable`
//   5. `max_expansions` budget guard returns `.expansion_limit_hit`
//   6. Invalid nodes, `start == goal`, negative weight guard & `cap.graph` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    AStarHeuristic,
    AStarStatus,
    AStarNodeCoord,
    AStarConfig,
    computeAStarHeuristic,
    AStarGraph,
} from "cap:graph";

fn buildGridGraph(width: usize, height: usize): AStarGraph {
    let g = AStarGraph.init();
    let y: usize = 0;
    while (y < height) : (y += 1) {
        let x: usize = 0;
        while (x < width) : (x += 1) {
            * = g.addNode(@as(f32, @floatFromInt(x)), @as(f32, @floatFromInt(y)));
        }
    }
    let gy: usize = 0;
    while (gy < height) : (gy += 1) {
        let gx: usize = 0;
        while (gx < width) : (gx += 1) {
            const u: u16 = @as(u16, @intCast(gy * width + gx));
            if (gx + 1 < width) {
                const r: u16 = @as(u16, @intCast(gy * width + (gx + 1)));
                * = g.addUndirectedEdge(u, r, 1.0);
            }
            if (gy + 1 < height) {
                const d: u16 = @as(u16, @intCast((gy + 1) * width + gx));
                * = g.addUndirectedEdge(u, d, 1.0);
            }
        }
    }
    return g;
}

test("AStarGraph: 6x6 grid optimal path routing around vertical obstacle wall") {
    let g = buildGridGraph(6, 6);
    // Block column x=2 for y=0..4 (leaving only gap at (2, 5))
    * = g.setNodeBlocked(2, true);
    * = g.setNodeBlocked(8, true);
    * = g.setNodeBlocked(14, true);
    * = g.setNodeBlocked(20, true);
    * = g.setNodeBlocked(26, true);

    const start: u16 = 0;  // (0, 0)
    const goal: u16 = 5;   // (5, 0)

    let cfg = AStarConfig.defaults();
    cfg.heuristic = AStarHeuristic.manhattan;
    const res = g.findPath(start, goal, cfg);

    // Optimal detour: down 5 (0,0)->(0,5), right 5 (0,5)->(5,5), up 5 (5,5)->(5,0) = 15 steps!
    expect_eq(res.status, AStarStatus.found);
    expect_true(res.total_cost > 14.99 && res.total_cost < 15.01);
    expect_eq(res.path_len, @as(usize, 16));
    expect_eq(res.path[0], start);
    expect_eq(res.path[res.path_len - 1], goal);

    // Verify no blocked node appears in the reconstructed path
    let i: usize = 0;
    while (i < res.path_len) : (i += 1) {
        const nid = res.path[i];
        expect_true(nid != 2 && nid != 8 && nid != 14 && nid != 20 && nid != 26);
    }
}

test("AStarGraph: A* expands significantly fewer nodes than zero_dijkstra on 8x8 grid") {
    // 8x8 grid = 64 nodes, 224 directed edges (< 512)
    let g = buildGridGraph(8, 8);
    const start: u16 = 27; // (3, 3) near center
    const goal: u16 = 63;  // (7, 7) bottom-right corner

    let cfg_dijk = AStarConfig.defaults();
    cfg_dijk.heuristic = AStarHeuristic.zero_dijkstra;
    const res_dijk = g.findPath(start, goal, cfg_dijk);

    let cfg_astar = AStarConfig.defaults();
    cfg_astar.heuristic = AStarHeuristic.manhattan;
    const res_astar = g.findPath(start, goal, cfg_astar);

    expect_eq(res_dijk.status, AStarStatus.found);
    expect_eq(res_astar.status, AStarStatus.found);
    // Both find the exact optimal distance = 8.0
    expect_true(res_dijk.total_cost > 7.99 && res_dijk.total_cost < 8.01);
    expect_true(res_astar.total_cost > 7.99 && res_astar.total_cost < 8.01);
    // A* with Manhattan + goal-biased tie-breaking expands dramatically fewer nodes than Dijkstra!
    expect_true(res_astar.nodes_expanded < res_dijk.nodes_expanded);
    expect_true(res_astar.nodes_expanded <= 12);
    expect_true(res_dijk.nodes_expanded >= 40);
}

test("AStarGraph: all 5 heuristic formulas & Weighted A* inflation") {
    const a = AStarNodeCoord{ x: 0.0, y: 0.0 };
    const b = AStarNodeCoord{ x: 3.0, y: 4.0 };

    expect_eq(computeAStarHeuristic(a, b, AStarHeuristic.zero_dijkstra), @as(f32, 0.0));
    expect_eq(computeAStarHeuristic(a, b, AStarHeuristic.manhattan), @as(f32, 7.0));
    const euc = computeAStarHeuristic(a, b, AStarHeuristic.euclidean);
    expect_true(euc > 4.999 && euc < 5.001);
    expect_eq(computeAStarHeuristic(a, b, AStarHeuristic.chebyshev), @as(f32, 4.0));
    const oct = computeAStarHeuristic(a, b, AStarHeuristic.octile);
    // 4 + (sqrt(2) - 1) * 3 = 5.24264
    expect_true(oct > 5.24 && oct < 5.25);

    // Weighted A* (w = 2.5) on 8x8 grid with partial obstacle
    let g = buildGridGraph(8, 8);
    * = g.setNodeBlocked(28, true);
    * = g.setNodeBlocked(36, true);
    let cfg_w = AStarConfig.defaults();
    cfg_w.heuristic = AStarHeuristic.euclidean;
    cfg_w.heuristic_weight = 2.5;
    const res_w = g.findPath(0, 63, cfg_w);
    expect_eq(res_w.status, AStarStatus.found);
    expect_eq(res_w.path[0], @as(u16, 0));
    expect_eq(res_w.path[res_w.path_len - 1], @as(u16, 63));
}

test("AStarGraph: sealed wall & blocked endpoints return unreachable") {
    let g = buildGridGraph(4, 4);
    // Completely seal column x=2 (nodes 2, 6, 10, 14)
    * = g.setNodeBlocked(2, true);
    * = g.setNodeBlocked(6, true);
    * = g.setNodeBlocked(10, true);
    * = g.setNodeBlocked(14, true);

    const res_wall = g.findPath(0, 15, AStarConfig.defaults());
    expect_eq(res_wall.status, AStarStatus.no_path);
    expect_eq(res_wall.path_len, @as(usize, 0));

    // Blocked start or blocked goal immediately returns no_path
    let g2 = buildGridGraph(3, 3);
    * = g2.setNodeBlocked(8, true);
    expect_eq(g2.findPath(0, 8, AStarConfig.defaults()).status, AStarStatus.no_path);
    expect_eq(g2.findPath(8, 0, AStarConfig.defaults()).status, AStarStatus.no_path);
}

test("AStarGraph: max_expansions budget cap triggers expansion_limit_hit") {
    let g = buildGridGraph(8, 8);
    let cfg = AStarConfig.defaults();
    cfg.heuristic = AStarHeuristic.zero_dijkstra;
    cfg.max_expansions = 5;

    const res = g.findPath(0, 63, cfg);
    expect_eq(res.status, AStarStatus.expansion_limit_hit);
    expect_eq(res.nodes_expanded, @as(usize, 5));
}

test("AStarGraph: invalid_node, start==goal, negative weight guard & cap.graph facade parity") {
    let g = cap.graph.AStarGraph.init();
    const n0 = g.addNode(0.0, 0.0) ?? 99;
    const n1 = g.addNode(1.0, 0.0) ?? 99;
    expect_eq(g.nodeCount(), @as(usize, 2));

    // Negative edge weight rejected
    expect_true(!g.addDirectedEdge(n0, n1, -1.0));
    expect_true(!g.addUndirectedEdge(n0, n1, -0.5));
    expect_eq(g.edgeCount(), @as(usize, 0));

    expect_true(g.addUndirectedEdge(n0, n1, 2.5));
    expect_eq(g.edgeCount(), @as(usize, 2));

    // Invalid node IDs
    expect_eq(g.findPath(0, 99, cap.graph.AStarConfig.defaults()).status, cap.graph.AStarStatus.invalid_node);
    expect_eq(g.findPath(99, 0, cap.graph.AStarConfig.defaults()).status, cap.graph.AStarStatus.invalid_node);

    // start == goal trivial path
    const res_same = g.findPath(n0, n0, cap.graph.AStarConfig.defaults());
    expect_eq(res_same.status, cap.graph.AStarStatus.found);
    expect_eq(res_same.total_cost, @as(f32, 0.0));
    expect_eq(res_same.path_len, @as(usize, 1));
    expect_eq(res_same.path[0], n0);
}
