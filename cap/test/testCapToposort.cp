// cap/test/testCapToposort.cp
// Comprehensive unit tests for @cap/toposort (Kahn + cycle detection + DAG levels)

import { test, expect_eq } from "cap:test";
import { TopoGraph } from "../../packages/@cap/toposort/src/index.cp";

fn main() {
    test("1. @cap/toposort — empty graph & single node", () => {
        let empty = TopoGraph.init(0 as usize);
        const r0 = empty.sort();
        expect_eq(r0.ok, true);
        expect_eq(r0.count, 0 as usize);
        expect_eq(empty.has_cycle(), false);

        let single = TopoGraph.init(1 as usize);
        const r1 = single.sort();
        expect_eq(r1.ok, true);
        expect_eq(r1.count, 1 as usize);
        expect_eq(r1.order[0], 0 as u16);
        expect_eq(r1.levels[0], 0 as u16);
    });

    test("2. @cap/toposort — linear dependency chain (3 -> 2 -> 1 -> 0)", () => {
        let g = TopoGraph.init(4 as usize);
        expect_eq(g.add_edge(3 as usize, 2 as usize), true);
        expect_eq(g.add_edge(2 as usize, 1 as usize), true);
        expect_eq(g.add_edge(1 as usize, 0 as usize), true);

        expect_eq(g.in_degree(3 as usize), 0 as u16);
        expect_eq(g.out_degree(3 as usize), 1 as u16);
        expect_eq(g.in_degree(0 as usize), 1 as u16);
        expect_eq(g.out_degree(0 as usize), 0 as u16);

        const res = g.sort();
        expect_eq(res.ok, true);
        expect_eq(res.count, 4 as usize);
        expect_eq(res.order[0], 3 as u16);
        expect_eq(res.order[1], 2 as u16);
        expect_eq(res.order[2], 1 as u16);
        expect_eq(res.order[3], 0 as u16);
        expect_eq(res.levels[3], 0 as u16);
        expect_eq(res.levels[2], 1 as u16);
        expect_eq(res.levels[1], 2 as u16);
        expect_eq(res.levels[0], 3 as u16);
    });

    test("3. @cap/toposort — diamond DAG deterministic lexicographically smallest order", () => {
        // 0 -> 2, 0 -> 1, 2 -> 3, 1 -> 3
        // Even though edge 0->2 is added before 0->1, min-heap guarantees order [0, 1, 2, 3]
        let g = TopoGraph.init(4 as usize);
        * = g.add_edge(0 as usize, 2 as usize);
        * = g.add_edge(0 as usize, 1 as usize);
        * = g.add_edge(2 as usize, 3 as usize);
        * = g.add_edge(1 as usize, 3 as usize);

        const res = g.sort();
        expect_eq(res.ok, true);
        expect_eq(res.count, 4 as usize);
        expect_eq(res.order[0], 0 as u16);
        expect_eq(res.order[1], 1 as u16);
        expect_eq(res.order[2], 2 as u16);
        expect_eq(res.order[3], 3 as u16);
    });

    test("4. @cap/toposort — multi-path parallel DAG level stratification", () => {
        // 0 -> 1 -> 3 -> 4 (depth 3)
        // 0 -> 2 -> 4 (depth 2)
        // Longest path to 4 is 3, so levels[4] must be 3!
        let g = TopoGraph.init(5 as usize);
        * = g.add_edge(0 as usize, 1 as usize);
        * = g.add_edge(1 as usize, 3 as usize);
        * = g.add_edge(3 as usize, 4 as usize);
        * = g.add_edge(0 as usize, 2 as usize);
        * = g.add_edge(2 as usize, 4 as usize);

        const res = g.sort();
        expect_eq(res.ok, true);
        expect_eq(res.levels[0], 0 as u16);
        expect_eq(res.levels[1], 1 as u16);
        expect_eq(res.levels[2], 1 as u16);
        expect_eq(res.levels[3], 2 as u16);
        expect_eq(res.levels[4], 3 as u16);
    });

    test("5. @cap/toposort — self-loop & multi-node cycle detection", () => {
        // Self-loop: 1 -> 1
        let g1 = TopoGraph.init(2 as usize);
        * = g1.add_edge(1 as usize, 1 as usize);
        expect_eq(g1.has_cycle(), true);
        expect_eq(g1.sort().ok, false);

        // 3-node cycle: 0 -> 1 -> 2 -> 0
        let g2 = TopoGraph.init(4 as usize);
        * = g2.add_edge(0 as usize, 1 as usize);
        * = g2.add_edge(1 as usize, 2 as usize);
        * = g2.add_edge(2 as usize, 0 as usize);
        expect_eq(g2.has_cycle(), true);
        const r2 = g2.sort();
        expect_eq(r2.ok, false);
        // Isolated node 3 still popped, cycle nodes 0/1/2 stuck
        expect_eq(r2.count, 1 as usize);
        expect_eq(r2.order[0], 3 as u16);
    });

    test("6. @cap/toposort — isolated vertices, reset & out-of-bounds guards", () => {
        let g = TopoGraph.init(5 as usize);
        * = g.add_edge(4 as usize, 2 as usize);
        // Out-of-bounds edge rejected
        expect_eq(g.add_edge(0 as usize, 5 as usize), false);
        expect_eq(g.add_edge(5 as usize, 0 as usize), false);
        expect_eq(g.in_degree(99 as usize), 0 as u16);
        expect_eq(g.out_degree(99 as usize), 0 as u16);

        const res = g.sort();
        expect_eq(res.ok, true);
        expect_eq(res.count, 5 as usize);
        // Ready initially: 0, 1, 3, 4 -> after 4 unlocks 2 -> [0, 1, 3, 4, 2]
        expect_eq(res.order[0], 0 as u16);
        expect_eq(res.order[1], 1 as u16);
        expect_eq(res.order[2], 3 as u16);
        expect_eq(res.order[3], 4 as u16);
        expect_eq(res.order[4], 2 as u16);

        // Reset reuses graph cleanly
        g.reset(2 as usize);
        expect_eq(g.edge_count, 0 as usize);
        expect_eq(g.add_edge(1 as usize, 0 as usize), true);
        const r2 = g.sort();
        expect_eq(r2.ok, true);
        expect_eq(r2.order[0], 1 as u16);
        expect_eq(r2.order[1], 0 as u16);
    });
}
