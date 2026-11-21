// Source: cp/cap/test/test_dijkstra_production_deep.cp (Production deep tests for cap:dijkstra)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NO_PARENT,
    isReachable,
    Edge,
    WeightedCsrGraph,
    DijkstraWorkspace,
    DijkstraResult,
    BellmanFordResult,
    FloydWarshallResult,
    dijkstra,
    reconstructPath,
    bellmanFord,
    floydWarshall,
    reconstructFloydPath,
} from "cap:dijkstra";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

test("1. Classic 6-node weighted digraph: Dijkstra decrease-key & exact path reconstruction", () => {
    // 0 -> 1 (7), 0 -> 2 (9), 0 -> 5 (14)
    // 1 -> 2 (10), 1 -> 3 (15)
    // 2 -> 3 (11), 2 -> 5 (2)
    // 3 -> 4 (6)
    // 4 -> []
    // 5 -> 4 (9)
    // Shortest from 0:
    // dist[0]=0, dist[1]=7, dist[2]=9, dist[3]=20 (0->2->3), dist[5]=11 (0->2->5), dist[4]=20 (0->2->5->4)
    const row_ptr: [7]usize = [0, 3, 5, 7, 8, 8, 9];
    const col_idx: [9]usize = [1, 2, 5, 2, 3, 3, 5, 4, 4];
    const weights: [9]f64 = [7.0, 9.0, 14.0, 10.0, 15.0, 11.0, 2.0, 6.0, 9.0];
    const g = WeightedCsrGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };
    expect_eq(g.validate(), true);
    expect_eq(g.numEdges(), 9 as usize);
    expect_eq(g.outDegree(0), 3 as usize);
    expect_eq(g.neighbors(2).len, 2 as usize);
    expect_eq(g.edgeWeights(2)[1], 2.0);

    let dist: [6]f64 = std.mem.zeroes([6]f64);
    let parent: [6]i64 = std.mem.zeroes([6]i64);
    let hn: [6]usize = std.mem.zeroes([6]usize);
    let hp: [6]i64 = std.mem.zeroes([6]i64);
    let vis: [6]u8 = std.mem.zeroes([6]u8);
    let ws = DijkstraWorkspace {
        heap_nodes: hn[0..],
        heap_pos: hp[0..],
        visited: vis[0..],
    };

    const res = dijkstra(g, 0, dist[0..], parent[0..], &ws) ?? unreachable;
    expect_eq(res.settled_nodes, 6 as usize);
    expect_eq(res.relaxations >= 6, true);

    expect_eq(absDiff(dist[0], 0.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[1], 7.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[2], 9.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[3], 20.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[4], 20.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[5], 11.0) < 1.0e-12, true);
    expect_eq(parent[0], NO_PARENT);

    // Reconstruct path 0 -> 4: must be [0, 2, 5, 4] (total weight 9 + 2 + 9 = 20)
    let path: [6]usize = std.mem.zeroes([6]usize);
    const plen = reconstructPath(parent[0..], 0, 4, path[0..]) ?? unreachable;
    expect_eq(plen, 4 as usize);
    expect_eq(path[0], 0 as usize);
    expect_eq(path[1], 2 as usize);
    expect_eq(path[2], 5 as usize);
    expect_eq(path[3], 4 as usize);

    // Reconstruct trivial path 0 -> 0
    const plen0 = reconstructPath(parent[0..], 0, 0, path[0..]) ?? unreachable;
    expect_eq(plen0, 1 as usize);
    expect_eq(path[0], 0 as usize);
});

test("2. Equal-length paths, zero-weight edges & unreachable disconnected component", () => {
    // 0 -> 1 (2.5), 0 -> 2 (2.5), 1 -> 3 (0.0), 2 -> 3 (0.0); nodes 4, 5 form isolated component 4 -> 5 (1.0)
    const row_ptr: [7]usize = [0, 2, 3, 4, 4, 5, 5];
    const col_idx: [5]usize = [1, 2, 3, 3, 5];
    const weights: [5]f64 = [2.5, 2.5, 0.0, 0.0, 1.0];
    const g = WeightedCsrGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };

    let dist: [6]f64 = std.mem.zeroes([6]f64);
    let parent: [6]i64 = std.mem.zeroes([6]i64);
    let hn: [6]usize = std.mem.zeroes([6]usize);
    let hp: [6]i64 = std.mem.zeroes([6]i64);
    let vis: [6]u8 = std.mem.zeroes([6]u8);
    let ws = DijkstraWorkspace { heap_nodes: hn[0..], heap_pos: hp[0..], visited: vis[0..] };

    const res = dijkstra(g, 0, dist[0..], parent[0..], &ws) ?? unreachable;
    expect_eq(res.settled_nodes, 4 as usize);
    expect_eq(isReachable(dist[3]), true);
    expect_eq(absDiff(dist[3], 2.5) < 1.0e-12, true);
    expect_eq(isReachable(dist[4]), false);
    expect_eq(isReachable(dist[5]), false);
    expect_eq(parent[4], NO_PARENT);

    let path: [6]usize = std.mem.zeroes([6]usize);
    const plen = reconstructPath(parent[0..], 0, 3, path[0..]) ?? unreachable;
    expect_eq(plen, 3 as usize);
    expect_eq(path[0], 0 as usize);
    expect_eq(path[1] == 1 || path[1] == 2, true);
    expect_eq(path[2], 3 as usize);

    // Unreachable target returns null
    expect_eq(reconstructPath(parent[0..], 0, 5, path[0..]) == null, true);
});

test("3. Negative edge rejected by Dijkstra but solved accurately by Bellman-Ford", () => {
    // 0 -> 1 (4), 0 -> 2 (5), 1 -> 3 (6), 2 -> 1 (-3), 3 -> 4 (2), 2 -> 4 (8)
    // Shortest from 0:
    // dist[0]=0, dist[2]=5, dist[1]=5+(-3)=2, dist[3]=2+6=8, dist[4]=8+2=10 (via 0->2->1->3->4)
    const row_ptr: [6]usize = [0, 2, 3, 5, 6, 6];
    const col_idx: [6]usize = [1, 2, 3, 1, 4, 4];
    const weights: [6]f64 = [4.0, 5.0, 6.0, -3.0, 8.0, 2.0];
    const g = WeightedCsrGraph {
        num_nodes: 5,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };
    expect_eq(g.validate(), true);

    let dist: [5]f64 = std.mem.zeroes([5]f64);
    let parent: [5]i64 = std.mem.zeroes([5]i64);
    let hn: [5]usize = std.mem.zeroes([5]usize);
    let hp: [5]i64 = std.mem.zeroes([5]i64);
    let vis: [5]u8 = std.mem.zeroes([5]u8);
    let ws = DijkstraWorkspace { heap_nodes: hn[0..], heap_pos: hp[0..], visited: vis[0..] };

    // Dijkstra gracefully rejects negative edge weights
    expect_eq(dijkstra(g, 0, dist[0..], parent[0..], &ws) == null, true);

    // Bellman-Ford relaxes negative edges cleanly with no negative cycle
    const bf = bellmanFord(g, 0, dist[0..], parent[0..]) ?? unreachable;
    expect_eq(bf.has_negative_cycle, false);
    expect_eq(absDiff(dist[0], 0.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[1], 2.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[2], 5.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[3], 8.0) < 1.0e-12, true);
    expect_eq(absDiff(dist[4], 10.0) < 1.0e-12, true);

    let path: [5]usize = std.mem.zeroes([5]usize);
    const plen = reconstructPath(parent[0..], 0, 4, path[0..]) ?? unreachable;
    expect_eq(plen, 5 as usize);
    expect_eq(path[0], 0 as usize);
    expect_eq(path[1], 2 as usize);
    expect_eq(path[2], 1 as usize);
    expect_eq(path[3], 3 as usize);
    expect_eq(path[4], 4 as usize);
});

test("4. Negative cycle detection in Bellman-Ford and Floyd-Warshall", () => {
    // Reachable negative cycle: 0 -> 1 (1), 1 -> 2 (2), 2 -> 3 (-5), 3 -> 1 (1) => cycle sum = -2
    const row_ptr: [5]usize = [0, 1, 2, 3, 4];
    const col_idx: [4]usize = [1, 2, 3, 1];
    const weights: [4]f64 = [1.0, 2.0, -5.0, 1.0];
    const g = WeightedCsrGraph {
        num_nodes: 4,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };

    let dist: [4]f64 = std.mem.zeroes([4]f64);
    let parent: [4]i64 = std.mem.zeroes([4]i64);
    const bf = bellmanFord(g, 0, dist[0..], parent[0..]) ?? unreachable;
    expect_eq(bf.has_negative_cycle, true);

    const edges: [4]Edge = [
        Edge { from: 0, to: 1, weight: 1.0 },
        Edge { from: 1, to: 2, weight: 2.0 },
        Edge { from: 2, to: 3, weight: -5.0 },
        Edge { from: 3, to: 1, weight: 1.0 },
    ];
    let dmat: [16]f64 = std.mem.zeroes([16]f64);
    let nmat: [16]i64 = std.mem.zeroes([16]i64);
    const fw = floydWarshall(4, edges[0..], dmat[0..], nmat[0..]) ?? unreachable;
    expect_eq(fw.has_negative_cycle, true);
});

test("5. Floyd-Warshall all-pairs shortest paths cross-validated against Dijkstra for all 36 pairs", () => {
    // 6-node non-negative graph with cycles and multiple paths
    const edges: [11]Edge = [
        Edge { from: 0, to: 1, weight: 3.0 },
        Edge { from: 0, to: 2, weight: 8.0 },
        Edge { from: 1, to: 2, weight: 2.0 },
        Edge { from: 1, to: 3, weight: 5.0 },
        Edge { from: 2, to: 3, weight: 1.0 },
        Edge { from: 2, to: 4, weight: 4.0 },
        Edge { from: 3, to: 4, weight: 2.0 },
        Edge { from: 3, to: 5, weight: 6.0 },
        Edge { from: 4, to: 5, weight: 1.5 },
        Edge { from: 5, to: 0, weight: 7.0 },
        Edge { from: 4, to: 1, weight: 3.5 },
    ];
    // Equivalent CSR
    const row_ptr: [7]usize = [0, 2, 4, 6, 8, 10, 11];
    const col_idx: [11]usize = [1, 2, 2, 3, 3, 4, 4, 5, 5, 1, 0];
    const weights: [11]f64 = [3.0, 8.0, 2.0, 5.0, 1.0, 4.0, 2.0, 6.0, 1.5, 3.5, 7.0];
    const g = WeightedCsrGraph {
        num_nodes: 6,
        row_ptr: row_ptr[0..],
        col_idx: col_idx[0..],
        weights: weights[0..],
    };

    let dmat: [36]f64 = std.mem.zeroes([36]f64);
    let nmat: [36]i64 = std.mem.zeroes([36]i64);
    const fw = floydWarshall(6, edges[0..], dmat[0..], nmat[0..]) ?? unreachable;
    expect_eq(fw.has_negative_cycle, false);

    let dist: [6]f64 = std.mem.zeroes([6]f64);
    let parent: [6]i64 = std.mem.zeroes([6]i64);
    let hn: [6]usize = std.mem.zeroes([6]usize);
    let hp: [6]i64 = std.mem.zeroes([6]i64);
    let vis: [6]u8 = std.mem.zeroes([6]u8);
    let ws = DijkstraWorkspace { heap_nodes: hn[0..], heap_pos: hp[0..], visited: vis[0..] };

    let s: usize = 0;
    while (s < 6) {
        * = dijkstra(g, s, dist[0..], parent[0..], &ws) ?? unreachable;
        let t: usize = 0;
        while (t < 6) {
            const fw_d = dmat[s * 6 + t];
            expect_eq(isReachable(fw_d), isReachable(dist[t]));
            if (isReachable(fw_d)) {
                expect_eq(absDiff(fw_d, dist[t]) < 1.0e-12, true);
                let fpath: [6]usize = std.mem.zeroes([6]usize);
                const flen = reconstructFloydPath(nmat[0..], 6, s, t, fpath[0..]) ?? unreachable;
                expect_eq(fpath[0], s);
                expect_eq(fpath[flen - 1], t);
            }
            t += 1;
        }
        s += 1;
    }
});

test("6. Empty graph, out-of-bounds source, malformed CSR, undersized buffers & cycle-safe reconstructPath", () => {
    let dist: [4]f64 = std.mem.zeroes([4]f64);
    let parent: [4]i64 = std.mem.zeroes([4]i64);
    let small_d: [2]f64 = std.mem.zeroes([2]f64);
    let small_p: [2]i64 = std.mem.zeroes([2]i64);
    let hn: [4]usize = std.mem.zeroes([4]usize);
    let hp: [4]i64 = std.mem.zeroes([4]i64);
    let vis: [4]u8 = std.mem.zeroes([4]u8);
    let small_u: [2]usize = std.mem.zeroes([2]usize);
    let ws = DijkstraWorkspace { heap_nodes: hn[0..], heap_pos: hp[0..], visited: vis[0..] };
    let bad_ws = DijkstraWorkspace { heap_nodes: small_u[0..], heap_pos: hp[0..], visited: vis[0..] };

    // Empty graph
    const rp0: [1]usize = [0];
    const ci0: [0]usize = [];
    const w0: [0]f64 = [];
    const g0 = WeightedCsrGraph { num_nodes: 0, row_ptr: rp0[0..], col_idx: ci0[0..], weights: w0[0..] };
    expect_eq(dijkstra(g0, 0, dist[0..], parent[0..], &ws) == null, true);
    expect_eq(bellmanFord(g0, 0, dist[0..], parent[0..]) == null, true);

    // Malformed CSR: weights slice shorter than total_edges
    const rp3: [4]usize = [0, 1, 2, 2];
    const ci3: [2]usize = [1, 2];
    const w_short: [1]f64 = [1.0];
    const g_wshort = WeightedCsrGraph { num_nodes: 3, row_ptr: rp3[0..], col_idx: ci3[0..], weights: w_short[0..] };
    expect_eq(g_wshort.validate(), false);
    expect_eq(dijkstra(g_wshort, 0, dist[0..], parent[0..], &ws) == null, true);

    // Valid 3-node graph: reject OOB src & undersized buffers
    const w3: [2]f64 = [1.0, 2.0];
    const g3 = WeightedCsrGraph { num_nodes: 3, row_ptr: rp3[0..], col_idx: ci3[0..], weights: w3[0..] };
    expect_eq(g3.validate(), true);
    expect_eq(dijkstra(g3, 3, dist[0..], parent[0..], &ws) == null, true);
    expect_eq(dijkstra(g3, 0, small_d[0..], parent[0..], &ws) == null, true);
    expect_eq(dijkstra(g3, 0, dist[0..], small_p[0..], &ws) == null, true);
    expect_eq(dijkstra(g3, 0, dist[0..], parent[0..], &bad_ws) == null, true);
    expect_eq(bellmanFord(g3, 3, dist[0..], parent[0..]) == null, true);
    expect_eq(bellmanFord(g3, 0, small_d[0..], parent[0..]) == null, true);

    // Floyd-Warshall rejects n == 0, OOB edge, and undersized matrices
    const bad_edges: [1]Edge = [Edge { from: 0, to: 5, weight: 1.0 }];
    let dmat: [9]f64 = std.mem.zeroes([9]f64);
    let nmat: [9]i64 = std.mem.zeroes([9]i64);
    let dsmall: [4]f64 = std.mem.zeroes([4]f64);
    expect_eq(floydWarshall(0, bad_edges[0..0], dmat[0..], nmat[0..]) == null, true);
    expect_eq(floydWarshall(3, bad_edges[0..], dmat[0..], nmat[0..]) == null, true);
    expect_eq(floydWarshall(3, bad_edges[0..0], dsmall[0..], nmat[0..]) == null, true);

    // reconstructPath rejects malformed cycle in parent array & undersized path_out
    const cyc_parent: [3]i64 = [2, 0, 1];
    let pout: [3]usize = std.mem.zeroes([3]usize);
    expect_eq(reconstructPath(cyc_parent[0..], 0, 2, pout[0..]) == null, true);
    const ok_parent: [3]i64 = [-1, 0, 1];
    let pout_small: [2]usize = std.mem.zeroes([2]usize);
    expect_eq(reconstructPath(ok_parent[0..], 0, 2, pout_small[0..]) == null, true);
});
