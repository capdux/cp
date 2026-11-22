// Origin: Pure-CP Stdlib Test Suite (lib/graph/minCostFlow.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Min-Cost Max-Flow Solver:
//   1. Classic Min-Cost Max-Flow with reverse-edge cancellation & flow conservation check
//   2. Target max_flow_limit convexity, reached_limit flag & resetFlows() reuse
//   3. Negative-cost DAG edges via multi-source SPFA Johnson potential initialization
//   4. Strict negative-cost cycle detection returning McmfStatus.negative_cycle
//   5. Bipartite min-cost supply/demand transport solver (feasible & infeasible demand)
//   6. Unreachable sink, zero-capacity edges, bounds/overflow guards & cap.graph facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCMF_MAX_NODES,
    MCMF_MAX_EDGES,
    MCMF_NO_EDGE,
    MCMF_INF_COST,
    MCMF_INF_FLOW,
    McmfStatus,
    McmfEdge,
    McmfResult,
    McmfTransportResult,
    MinCostFlowNetwork,
    solveMinCostTransport,
} from "cap:graph";

test("MinCostFlow: classic network, reverse-edge augmentation & flow conservation") {
    // 4-node classic network:
    //   0 -> 1 (cap=2, cost=1)
    //   0 -> 2 (cap=2, cost=5)
    //   1 -> 2 (cap=1, cost=1)
    //   1 -> 3 (cap=1, cost=4)
    //   2 -> 3 (cap=3, cost=1)
    // Optimal flow = 4:
    //   1 unit via 0->1->2->3 (cost = 1+1+1 = 3)
    //   1 unit via 0->1->3    (cost = 1+4 = 5)
    //   2 units via 0->2->3   (cost = 2*(5+1) = 12)
    // Total min cost = 3 + 5 + 12 = 20.
    let net = MinCostFlowNetwork.init(4) orelse {
        expect_true(false);
        return;
    };
    const e01 = net.addEdge(0, 1, 2, 1) orelse { expect_true(false); return; };
    const e02 = net.addEdge(0, 2, 2, 5) orelse { expect_true(false); return; };
    const e12 = net.addEdge(1, 2, 1, 1) orelse { expect_true(false); return; };
    const e13 = net.addEdge(1, 3, 1, 4) orelse { expect_true(false); return; };
    const e23 = net.addEdge(2, 3, 3, 1) orelse { expect_true(false); return; };

    const res = net.solve(0, 3, MCMF_INF_FLOW);
    expect_eq(res.status, McmfStatus.ok);
    expect_eq(res.total_flow, 4 as i64);
    expect_eq(res.total_cost, 20 as i64);
    expect_eq(res.used_spfa_init, false);
    expect_eq(res.reached_limit, false);
    expect_true(res.augmentations >= 3);

    const f01 = net.edgeFlow(e01) orelse -1;
    const f02 = net.edgeFlow(e02) orelse -1;
    const f12 = net.edgeFlow(e12) orelse -1;
    const f13 = net.edgeFlow(e13) orelse -1;
    const f23 = net.edgeFlow(e23) orelse -1;

    expect_eq(f01, 2 as i64);
    expect_eq(f02, 2 as i64);
    expect_eq(f12, 1 as i64);
    expect_eq(f13, 1 as i64);
    expect_eq(f23, 3 as i64);

    // Flow conservation at intermediate nodes 1 and 2 + residual capacity identity
    expect_eq(f01, f12 + f13);
    expect_eq(f02 + f12, f23);
    expect_eq(f01 + (net.edgeResidualCap(e01) orelse -1), 2 as i64);
    expect_eq(f23 + (net.edgeResidualCap(e23) orelse -1), 3 as i64);
}

test("MinCostFlow: max_flow_limit convex cost progression & resetFlows() reuse") {
    let net = MinCostFlowNetwork.init(4) orelse {
        expect_true(false);
        return;
    };
    * = net.addEdge(0, 1, 2, 1);
    * = net.addEdge(0, 2, 2, 5);
    * = net.addEdge(1, 2, 1, 1);
    * = net.addEdge(1, 3, 1, 4);
    * = net.addEdge(2, 3, 3, 1);

    // Limit = 0 -> immediate 0 flow, 0 cost
    const r0 = net.solve(0, 3, 0);
    expect_eq(r0.status, McmfStatus.ok);
    expect_eq(r0.total_flow, 0 as i64);
    expect_eq(r0.total_cost, 0 as i64);
    expect_true(r0.reached_limit);

    // Limit = 1 -> takes cheapest path 0->1->2->3 (cost = 3)
    const r1 = net.solve(0, 3, 1);
    expect_eq(r1.status, McmfStatus.ok);
    expect_eq(r1.total_flow, 1 as i64);
    expect_eq(r1.total_cost, 3 as i64);
    expect_true(r1.reached_limit);

    // Reset and solve with Limit = 2 -> adds 0->1->3 (cost = 3 + 5 = 8)
    net.resetFlows();
    const r2 = net.solve(0, 3, 2);
    expect_eq(r2.status, McmfStatus.ok);
    expect_eq(r2.total_flow, 2 as i64);
    expect_eq(r2.total_cost, 8 as i64);
    expect_true(r2.reached_limit);

    // Reset and solve with Limit = 3 -> adds 1 unit of 0->2->3 (cost = 8 + 6 = 14)
    net.resetFlows();
    const r3 = net.solve(0, 3, 3);
    expect_eq(r3.status, McmfStatus.ok);
    expect_eq(r3.total_flow, 3 as i64);
    expect_eq(r3.total_cost, 14 as i64);
    expect_true(r3.reached_limit);
}

test("MinCostFlow: negative-cost DAG edges solved via SPFA Johnson potentials") {
    // Subsidized / profit-maximizing DAG with negative unit costs (no negative cycles):
    //   0 -> 1 (cap=3, cost=-10)
    //   1 -> 3 (cap=2, cost=2)   => path 0->1->3 unit cost = -8 (cap 2)
    //   1 -> 2 (cap=2, cost=-3)
    //   2 -> 3 (cap=2, cost=1)   => path 0->1->2->3 unit cost = -12 (cap 2)
    //   0 -> 2 (cap=2, cost=4)   => path 0->2->3 unit cost = +5 (cap 2)
    // Max flow from 0 to 3 is 4:
    //   2 units via 0->1->2->3 (cost = 2 * -12 = -24)
    //   1 unit  via 0->1->3    (cost = 1 * -8 = -8)
    //   Wait: 0->1 has cap 3, so 2 go to 2->3 (-24), 1 goes to 1->3 (-8); remaining 1 unit via 0->2?
    //   2->3 already saturated by 2 units! So max flow = 3, min cost = -32.
    let net = MinCostFlowNetwork.init(4) orelse {
        expect_true(false);
        return;
    };
    * = net.addEdge(0, 1, 3, -10);
    * = net.addEdge(1, 3, 2, 2);
    * = net.addEdge(1, 2, 2, -3);
    * = net.addEdge(2, 3, 2, 1);
    * = net.addEdge(0, 2, 2, 4);

    const res = net.solve(0, 3, MCMF_INF_FLOW);
    expect_eq(res.status, McmfStatus.ok);
    expect_true(res.used_spfa_init);
    expect_eq(res.total_flow, 3 as i64);
    expect_eq(res.total_cost, -32 as i64);
}

test("MinCostFlow: detects negative-cost cycle cleanly without infinite loop") {
    let net = MinCostFlowNetwork.init(5) orelse {
        expect_true(false);
        return;
    };
    * = net.addEdge(0, 1, 5, 1);
    // Negative cycle: 1 -> 2 (-4) -> 3 (1) -> 1 (1), total cycle cost = -2 < 0
    * = net.addEdge(1, 2, 5, -4);
    * = net.addEdge(2, 3, 5, 1);
    * = net.addEdge(3, 1, 5, 1);
    * = net.addEdge(2, 4, 5, 2);

    const res = net.solve(0, 4, MCMF_INF_FLOW);
    expect_eq(res.status, McmfStatus.negative_cycle);
    expect_true(res.used_spfa_init);
}

test("MinCostFlow: bipartite min-cost supply/demand transport (feasible & infeasible)") {
    // 2 warehouses: supply [30, 50] (total 80)
    // 3 customers:  demand [20, 35, 25] (total 80)
    // Costs matrix [2 x 3]:
    //   S0 -> [C0: 2, C1: 8, C2: 6]
    //   S1 -> [C0: 5, C1: 3, C2: 4]
    // Optimal plan:
    //   S0 -> C0: 20 (cost 40), S0 -> C2: 10 (cost 60)  [S0 used 30]
    //   S1 -> C1: 35 (cost 105), S1 -> C2: 15 (cost 60) [S1 used 50]
    // Total optimal cost = 40 + 60 + 105 + 60 = 265.
    const supplies: [2]i64 = [30, 50];
    const demands: [3]i64 = [20, 35, 25];
    const costs: [6]i64 = [
        2, 8, 6,
        5, 3, 4,
    ];
    let flows: [6]i64 = [0; 6];
    let flows_s: []i64 = flows[0..];

    const tr = solveMinCostTransport(supplies[0..], demands[0..], costs[0..], *flows_s);
    expect_eq(tr.status, McmfStatus.ok);
    expect_true(tr.fully_satisfied);
    expect_eq(tr.total_shipped, 80 as i64);
    expect_eq(tr.total_demand, 80 as i64);
    expect_eq(tr.total_cost, 265 as i64);

    // Check supply & demand balance
    expect_eq(flows[0] + flows[1] + flows[2], 30 as i64);
    expect_eq(flows[3] + flows[4] + flows[5], 50 as i64);
    expect_eq(flows[0] + flows[3], 20 as i64);
    expect_eq(flows[1] + flows[4], 35 as i64);
    expect_eq(flows[2] + flows[5], 25 as i64);

    // Infeasible demand (supply 40 < demand 60)
    const short_sup: [2]i64 = [15, 25];
    const big_dem: [2]i64 = [30, 30];
    const c2: [4]i64 = [1, 2, 3, 4];
    let f2: [4]i64 = [0; 4];
    let f2_s: []i64 = f2[0..];
    const tr_inf = solveMinCostTransport(short_sup[0..], big_dem[0..], c2[0..], *f2_s);
    expect_eq(tr_inf.status, McmfStatus.infeasible_demand);
    expect_eq(tr_inf.fully_satisfied, false);
    expect_eq(tr_inf.total_shipped, 40 as i64);
    expect_eq(tr_inf.total_demand, 60 as i64);
}

test("MinCostFlow: unreachable sink, zero-cap edges, invalid bounds & cap.graph facade") {
    expect_true(MinCostFlowNetwork.init(0) == null);
    expect_true(MinCostFlowNetwork.init(MCMF_MAX_NODES + 1) == null);

    let net = MinCostFlowNetwork.init(3) orelse {
        expect_true(false);
        return;
    };
    // Invalid edges rejected
    expect_true(net.addEdge(3, 0, 1, 1) == null);
    expect_true(net.addEdge(0, 3, 1, 1) == null);
    expect_true(net.addEdge(0, 1, -5, 1) == null);
    expect_true(net.edgeFlow(0) == null);

    // Zero-capacity edge leaves sink unreachable
    * = net.addEdge(0, 1, 0, 1);
    * = net.addEdge(1, 2, 10, 1);
    const r_unreach = net.solve(0, 2, MCMF_INF_FLOW);
    expect_eq(r_unreach.status, McmfStatus.ok);
    expect_eq(r_unreach.total_flow, 0 as i64);
    expect_eq(r_unreach.total_cost, 0 as i64);

    // Invalid source/sink/limit rejected
    expect_eq(net.solve(0, 0, 10).status, McmfStatus.invalid_source_sink);
    expect_eq(net.solve(0, 5, 10).status, McmfStatus.invalid_source_sink);
    expect_eq(net.solve(0, 2, -1).status, McmfStatus.invalid_source_sink);

    // Verify cap.graph namespace facade parity
    expect_eq(cap.graph.MCMF_MAX_NODES, MCMF_MAX_NODES);
    expect_eq(cap.graph.MCMF_MAX_EDGES, MCMF_MAX_EDGES);
    expect_eq(cap.graph.MCMF_INF_FLOW, MCMF_INF_FLOW);
}
