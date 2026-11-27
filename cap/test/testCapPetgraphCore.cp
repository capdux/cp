// cap/test/testCapPetgraphCore.cp
// Comprehensive unit tests for @cap/petgraph-core

import { test, expect_eq } from "cap:test";
import {
    PG_MAX_NODES,
    PG_MAX_EDGES,
    PG_INF,
    DiGraph,
} from "../../packages/petgraph/src/index.cp";

fn main() {
    test("1. @cap/petgraph-core — graph construction, degrees, BFS & iterative DFS", () => {
        let g = DiGraph.new();
        let n0 = g.addNode() ?? (99 as usize);
        let n1 = g.addNode() ?? (99 as usize);
        let n2 = g.addNode() ?? (99 as usize);
        let n3 = g.addNode() ?? (99 as usize);
        let n4 = g.addNode() ?? (99 as usize);

        expect_eq(g.nodeCount(), 5 as usize);
        expect_eq(g.addEdge(n0, n1, 1 as u64), true);
        expect_eq(g.addEdge(n0, n2, 1 as u64), true);
        expect_eq(g.addEdge(n1, n3, 1 as u64), true);
        expect_eq(g.addEdge(n2, n4, 1 as u64), true);
        expect_eq(g.edgeCount(), 4 as usize);

        expect_eq(g.hasEdge(n0, n1), true);
        expect_eq(g.hasEdge(n1, n0), false);
        expect_eq(g.outDegree(n0), 2 as usize);
        expect_eq(g.inDegree(n0), 0 as usize);
        expect_eq(g.inDegree(n3), 1 as usize);

        let bfs_out: [5]usize = undefined;
        expect_eq(g.bfs(n0, bfs_out[0..5]), 5 as usize);
        expect_eq(bfs_out[0], 0 as usize);
        expect_eq(bfs_out[1], 1 as usize);
        expect_eq(bfs_out[2], 2 as usize);
        expect_eq(bfs_out[3], 3 as usize);
        expect_eq(bfs_out[4], 4 as usize);

        let dfs_out: [5]usize = undefined;
        expect_eq(g.dfs(n0, dfs_out[0..5]), 5 as usize);
        expect_eq(dfs_out[0], 0 as usize);
        expect_eq(dfs_out[1], 1 as usize);
        expect_eq(dfs_out[2], 3 as usize);
        expect_eq(dfs_out[3], 2 as usize);
        expect_eq(dfs_out[4], 4 as usize);
    });

    test("2. @cap/petgraph-core — Kahn topological sort on a DAG", () => {
        let g = DiGraph.new();
        let a = g.addNode() ?? (0 as usize);
        let b = g.addNode() ?? (0 as usize);
        let c = g.addNode() ?? (0 as usize);
        let d = g.addNode() ?? (0 as usize);

        * = g.addEdge(a, b, 1 as u64);
        * = g.addEdge(a, c, 1 as u64);
        * = g.addEdge(b, d, 1 as u64);
        * = g.addEdge(c, d, 1 as u64);

        expect_eq(g.isCyclic(), false);
        let order: [4]usize = undefined;
        let cnt = g.topoSort(order[0..4]) ?? (0 as usize);
        expect_eq(cnt, 4 as usize);
        expect_eq(order[0], a);
        expect_eq(order[3], d);
    });

    test("3. @cap/petgraph-core — directed cycle detection & topoSort returns null", () => {
        let g = DiGraph.new();
        let n0 = g.addNode() ?? (0 as usize);
        let n1 = g.addNode() ?? (0 as usize);
        let n2 = g.addNode() ?? (0 as usize);

        * = g.addEdge(n0, n1, 1 as u64);
        * = g.addEdge(n1, n2, 1 as u64);
        expect_eq(g.isCyclic(), false);

        * = g.addEdge(n2, n0, 1 as u64);
        expect_eq(g.isCyclic(), true);
        let order: [3]usize = undefined;
        expect_eq(g.topoSort(order[0..3]) == null, true);
    });

    test("4. @cap/petgraph-core — weighted Dijkstra shortest paths & unreachable PG_INF", () => {
        let g = DiGraph.new();
        let n0 = g.addNode() ?? (0 as usize);
        let n1 = g.addNode() ?? (0 as usize);
        let n2 = g.addNode() ?? (0 as usize);
        let n3 = g.addNode() ?? (0 as usize);
        let n4 = g.addNode() ?? (0 as usize); // disconnected

        * = g.addEdge(n0, n1, 10 as u64);
        * = g.addEdge(n0, n2, 3 as u64);
        * = g.addEdge(n2, n1, 4 as u64);
        * = g.addEdge(n1, n3, 2 as u64);
        * = g.addEdge(n2, n3, 8 as u64);

        let dist: [5]u64 = undefined;
        expect_eq(g.dijkstra(n0, dist[0..5]), true);
        expect_eq(dist[0], 0 as u64);
        expect_eq(dist[1], 7 as u64);  // 0 -> 2 -> 1 (3 + 4)
        expect_eq(dist[2], 3 as u64);  // 0 -> 2
        expect_eq(dist[3], 9 as u64);  // 0 -> 2 -> 1 -> 3 (3 + 4 + 2)
        expect_eq(dist[4], PG_INF);    // unreachable
    });

    test("5. @cap/petgraph-core — undirected edges & reachability (hasPath)", () => {
        let g = DiGraph.new();
        let a = g.addNode() ?? (0 as usize);
        let b = g.addNode() ?? (0 as usize);
        let c = g.addNode() ?? (0 as usize);
        let island = g.addNode() ?? (0 as usize);

        expect_eq(g.addUndirectedEdge(a, b, 5 as u64), true);
        expect_eq(g.addUndirectedEdge(b, c, 6 as u64), true);
        expect_eq(g.edgeCount(), 4 as usize);
        expect_eq(g.hasEdge(a, b), true);
        expect_eq(g.hasEdge(b, a), true);

        expect_eq(g.hasPath(a, c), true);
        expect_eq(g.hasPath(c, a), true);
        expect_eq(g.hasPath(a, a), true);
        expect_eq(g.hasPath(a, island), false);
        expect_eq(g.hasPath(island, a), false);
    });

    test("6. @cap/petgraph-core — capacity limits & out-of-bounds protection", () => {
        let g = DiGraph.new();
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            expect_eq(g.addNode() == null, false);
            i += 1;
        }
        expect_eq(g.nodeCount(), PG_MAX_NODES);
        expect_eq(g.addNode() == null, true);

        // Out-of-bounds queries fail safely.
        expect_eq(g.addEdge(0 as usize, 64 as usize, 1 as u64), false);
        expect_eq(g.addUndirectedEdge(64 as usize, 0 as usize, 1 as u64), false);
        expect_eq(g.hasEdge(64 as usize, 0 as usize), false);
        expect_eq(g.outDegree(64 as usize), 0 as usize);
        expect_eq(g.inDegree(64 as usize), 0 as usize);
        expect_eq(g.hasPath(0 as usize, 64 as usize), false);

        // Fill edge pool to PG_MAX_EDGES (256).
        let e: usize = 0;
        while (e < PG_MAX_EDGES) {
            let from_n = e % PG_MAX_NODES;
            let to_n = (e + (1 as usize)) % PG_MAX_NODES;
            expect_eq(g.addEdge(from_n, to_n, 1 as u64), true);
            e += 1;
        }
        expect_eq(g.edgeCount(), PG_MAX_EDGES);
        expect_eq(g.addEdge(0 as usize, 1 as usize, 1 as u64), false);

        // Dijkstra rejects too-small output buffer.
        let small: [4]u64 = undefined;
        expect_eq(g.dijkstra(0 as usize, small[0..4]), false);
    });
}
