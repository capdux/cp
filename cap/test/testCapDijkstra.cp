// cap/test/testCapDijkstra.cp
// Comprehensive unit tests for @cap/dijkstra

import { test, expect_eq } from "cap:test";
import {
    WeightedGraph,
} from "../../packages/@cap/dijkstra/src/index.cp";

fn main() {
    test("1. @cap/dijkstra — Classic directed weighted graph & path reconstruction", () => {
        let g = WeightedGraph.init(5 as usize);
        expect_eq(g.addEdge(0 as usize, 1 as usize, 4 as i64), true);
        expect_eq(g.addEdge(0 as usize, 2 as usize, 1 as i64), true);
        expect_eq(g.addEdge(2 as usize, 1 as usize, 2 as i64), true);
        expect_eq(g.addEdge(1 as usize, 3 as usize, 1 as i64), true);
        expect_eq(g.addEdge(2 as usize, 3 as usize, 5 as i64), true);
        expect_eq(g.addEdge(3 as usize, 4 as usize, 3 as i64), true);
        expect_eq(g.nodeCount(), 5 as usize);
        expect_eq(g.edgeCount(), 6 as usize);

        const sp = g.dijkstra(0 as usize);
        expect_eq(sp.ok, true);
        expect_eq(sp.distanceTo(0 as usize), 0 as i64);
        expect_eq(sp.distanceTo(2 as usize), 1 as i64);
        expect_eq(sp.distanceTo(1 as usize), 3 as i64);
        expect_eq(sp.distanceTo(3 as usize), 4 as i64);
        expect_eq(sp.distanceTo(4 as usize), 7 as i64);

        let path: [8]u16 = undefined;
        const plen = sp.reconstructPath(4 as usize, path[0..8]);
        // Path: 0 -> 2 -> 1 -> 3 -> 4
        expect_eq(plen, 5 as usize);
        expect_eq(path[0], 0 as u16);
        expect_eq(path[1], 2 as u16);
        expect_eq(path[2], 1 as u16);
        expect_eq(path[3], 3 as u16);
        expect_eq(path[4], 4 as u16);
    });

    test("2. @cap/dijkstra — Undirected graph symmetric distances", () => {
        let g = WeightedGraph.init(4 as usize);
        expect_eq(g.addUndirectedEdge(0 as usize, 1 as usize, 5 as i64), true);
        expect_eq(g.addUndirectedEdge(1 as usize, 2 as usize, 3 as i64), true);
        expect_eq(g.addUndirectedEdge(2 as usize, 3 as usize, 2 as i64), true);
        expect_eq(g.addUndirectedEdge(0 as usize, 3 as usize, 20 as i64), true);

        const from0 = g.dijkstra(0 as usize);
        const from3 = g.dijkstra(3 as usize);
        expect_eq(from0.distanceTo(3 as usize), 10 as i64);
        expect_eq(from3.distanceTo(0 as usize), 10 as i64);
    });

    test("3. @cap/dijkstra — Unreachable isolated nodes return -1 & path len 0", () => {
        let g = WeightedGraph.init(4 as usize);
        * = g.addEdge(0 as usize, 1 as usize, 7 as i64);
        // Nodes 2 and 3 are isolated
        const sp = g.dijkstra(0 as usize);
        expect_eq(sp.isReachable(1 as usize), true);
        expect_eq(sp.isReachable(2 as usize), false);
        expect_eq(sp.distanceTo(2 as usize), -1 as i64);
        expect_eq(sp.distanceTo(99 as usize), -1 as i64);

        let path: [8]u16 = undefined;
        expect_eq(sp.reconstructPath(2 as usize, path[0..8]), 0 as usize);
    });

    test("4. @cap/dijkstra — Bellman-Ford handles negative edge weights accurately", () => {
        let g = WeightedGraph.init(4 as usize);
        * = g.addEdge(0 as usize, 1 as usize, 4 as i64);
        * = g.addEdge(0 as usize, 2 as usize, 5 as i64);
        * = g.addEdge(2 as usize, 1 as usize, -3 as i64);
        * = g.addEdge(1 as usize, 3 as usize, 2 as i64);

        const sp = g.bellmanFord(0 as usize);
        expect_eq(sp.ok, true);
        // 0 -> 2 (5) -> 1 (2) -> 3 (4)
        expect_eq(sp.distanceTo(1 as usize), 2 as i64);
        expect_eq(sp.distanceTo(3 as usize), 4 as i64);

        let path: [8]u16 = undefined;
        const plen = sp.reconstructPath(3 as usize, path[0..8]);
        expect_eq(plen, 4 as usize);
        expect_eq(path[0], 0 as u16);
        expect_eq(path[1], 2 as u16);
        expect_eq(path[2], 1 as u16);
        expect_eq(path[3], 3 as u16);
    });

    test("5. @cap/dijkstra — Bellman-Ford detects reachable negative cycle (ok == false)", () => {
        let g = WeightedGraph.init(4 as usize);
        * = g.addEdge(0 as usize, 1 as usize, 1 as i64);
        * = g.addEdge(1 as usize, 2 as usize, 2 as i64);
        * = g.addEdge(2 as usize, 3 as usize, -5 as i64);
        * = g.addEdge(3 as usize, 1 as usize, 1 as i64); // Cycle 1->2->3->1 total = -2

        const sp = g.bellmanFord(0 as usize);
        expect_eq(sp.ok, false);
        expect_eq(sp.isReachable(1 as usize), false);
    });

    test("6. @cap/dijkstra — Zero-weight edges & reset graph reuse", () => {
        let g = WeightedGraph.init(3 as usize);
        * = g.addEdge(0 as usize, 1 as usize, 0 as i64);
        * = g.addEdge(1 as usize, 2 as usize, 0 as i64);
        const sp1 = g.dijkstra(0 as usize);
        expect_eq(sp1.distanceTo(2 as usize), 0 as i64);

        g.reset(2 as usize);
        expect_eq(g.nodeCount(), 2 as usize);
        expect_eq(g.edgeCount(), 0 as usize);
        * = g.addEdge(0 as usize, 1 as usize, 12 as i64);
        const sp2 = g.dijkstra(0 as usize);
        expect_eq(sp2.distanceTo(1 as usize), 12 as i64);
    });
}
