// Source: cp/cap/test/test_graph_production_deep.cp
// Deep Production Verification Suite for cap:graph & cap.graph
// Covers:
//   1. DirectedGraph build/transpose/error guards, Kahn topologicalSort, findCycle & transitiveClosure
//   2. Iterative Tarjan SCC on complex multi-cycle graph & deep chain stack-overflow immunity
//   3. Dijkstra shortest paths, reconstructPath, Disconnected & NegativeWeight guards
//   4. Bellman-Ford negative weights & negative-weight cycle detection
//   5. cap.graph dynamic mounting + Graph/GraphAlgo compatibility with std.testing.allocator zero-leak

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GraphError,
    WeightedEdge,
    DirectedGraph,
    NO_PREV,
    ReachabilityMatrix,
    TarjanSccResult,
    PathResult,
    topologicalSort,
    findCycle,
    reachableFrom,
    transitiveClosure,
    tarjanScc,
    dijkstra,
    bellmanFord,
    reconstructPath,
    Graph,
    GraphAlgo,
} from "cap:graph";

fn approxEq(a: f64, b: f64, eps: f64 = 0.000001): bool {
    return @abs(a - b) <= eps;
}

fn containsUsize(slice: []const usize, target: usize): bool {
    for (item of slice) {
        if (item == target) return true;
    }
    return false;
}

fn indexOfUsize(slice: []const usize, target: usize): usize? {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] == target) return i;
    }
    return null;
}

test("1. DirectedGraph, DAG topologicalSort, findCycle & transitiveClosure") {
    const alloc = std.testing.allocator;

    let dag = DirectedGraph.init(alloc, 5) catch unreachable;
    defer dag.deinit();

    const v5 = dag.addVertex() catch unreachable;
    expect_eq(v5, 5);
    expect_eq(dag.vertexCount(), 6);

    // DAG: 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3, 3 -> 4, 4 -> 5
    dag.addEdge(0, 1, 1.0) catch unreachable;
    dag.addEdge(0, 2, 1.0) catch unreachable;
    dag.addEdge(1, 3, 1.0) catch unreachable;
    dag.addEdge(2, 3, 1.0) catch unreachable;
    dag.addEdge(3, 4, 1.0) catch unreachable;
    dag.addEdge(4, 5, 1.0) catch unreachable;
    expect_eq(dag.edgeCount(), 6);

    expect_eq(dag.outDegree(0) catch unreachable, 2);
    expect_eq(dag.inDegree(3) catch unreachable, 2);
    expect_eq(dag.inDegree(0) catch unreachable, 0);

    const nbrs0 = dag.neighbors(0) catch unreachable;
    expect_eq(nbrs0.len, 2);
    expect_eq(nbrs0[0].to, 1);
    expect_eq(nbrs0[1].to, 2);

    // DuplicateEdge & InvalidVertex guards
    let dup_caught = false;
    dag.addEdge(0, 1, 2.0) catch |e| {
        if (e == GraphError.DuplicateEdge) dup_caught = true;
    };
    expect_eq(dup_caught, true);

    let inv_caught = false;
    dag.addEdge(0, 99, 1.0) catch |e| {
        if (e == GraphError.InvalidVertex) inv_caught = true;
    };
    expect_eq(inv_caught, true);

    // Transpose reverses edges
    let rev = dag.transpose(alloc) catch unreachable;
    defer rev.deinit();
    expect_eq(rev.vertexCount(), 6);
    expect_eq(rev.edgeCount(), 6);
    expect_eq(rev.outDegree(3) catch unreachable, 2);
    expect_eq(rev.inDegree(0) catch unreachable, 2);

    // Topological sort on valid DAG
    const order = topologicalSort(alloc, dag) catch unreachable;
    defer alloc.free(order);
    expect_eq(order.len, 6);
    expect_eq(indexOfUsize(order, 0).? < indexOfUsize(order, 1).?, true);
    expect_eq(indexOfUsize(order, 1).? < indexOfUsize(order, 3).?, true);
    expect_eq(indexOfUsize(order, 2).? < indexOfUsize(order, 3).?, true);
    expect_eq(indexOfUsize(order, 3).? < indexOfUsize(order, 4).?, true);
    expect_eq(indexOfUsize(order, 4).? < indexOfUsize(order, 5).?, true);

    // Acyclic check via findCycle -> null
    const no_cycle = findCycle(alloc, dag) catch unreachable;
    expect_eq(no_cycle == null, true);

    // Reachability & Transitive Closure
    const reach1 = reachableFrom(alloc, dag, 1) catch unreachable;
    defer alloc.free(reach1);
    expect_eq(reach1.len, 4); // {1, 3, 4, 5}
    expect_eq(containsUsize(reach1, 1), true);
    expect_eq(containsUsize(reach1, 5), true);
    expect_eq(containsUsize(reach1, 2), false);

    let tc = transitiveClosure(alloc, dag) catch unreachable;
    defer tc.deinit();
    expect_eq(tc.canReach(0, 5), true);
    expect_eq(tc.canReach(1, 2), false);
    expect_eq(tc.canReach(5, 0), false);

    // Introduce back-edge 4 -> 1 to create cycle 1 -> 3 -> 4 -> 1
    dag.addEdge(4, 1, 1.0) catch unreachable;

    let cyc_topo_caught = false;
    * = topologicalSort(alloc, dag) catch |e| {
        if (e == GraphError.CycleDetected) cyc_topo_caught = true;
    };
    expect_eq(cyc_topo_caught, true);

    const found_opt = findCycle(alloc, dag) catch unreachable;
    expect_eq(found_opt != null, true);
    const cyc = found_opt.?;
    defer alloc.free(cyc);
    expect_eq(cyc.len >= 4, true);
    expect_eq(cyc[0], cyc[cyc.len - 1]);
    expect_eq(containsUsize(cyc, 1), true);
    expect_eq(containsUsize(cyc, 3), true);
    expect_eq(containsUsize(cyc, 4), true);
}

test("2. Iterative Tarjan SCC on complex multi-cycle graph & deep chain") {
    const alloc = std.testing.allocator;

    // Complex graph with 4 SCCs:
    //   SCC A: {0, 1, 2} (0->1->2->0)
    //   SCC B: {3, 4}    (3<->4)
    //   SCC C: {5}       (self-loop 5->5)
    //   SCC D: {6}       (sink)
    //   Cross edges: 2->3, 4->5, 5->6
    let g = DirectedGraph.init(alloc, 7) catch unreachable;
    defer g.deinit();

    g.addEdge(0, 1, 1.0) catch unreachable;
    g.addEdge(1, 2, 1.0) catch unreachable;
    g.addEdge(2, 0, 1.0) catch unreachable;
    g.addEdge(2, 3, 1.0) catch unreachable;
    g.addEdge(3, 4, 1.0) catch unreachable;
    g.addEdge(4, 3, 1.0) catch unreachable;
    g.addEdge(4, 5, 1.0) catch unreachable;
    g.addEdge(5, 5, 1.0) catch unreachable;
    g.addEdge(5, 6, 1.0) catch unreachable;

    let scc = tarjanScc(alloc, g) catch unreachable;
    defer scc.deinit();

    expect_eq(scc.count(), 4);
    // Reverse topological order: sink {6} first, {5} second, {3, 4} third, {0, 1, 2} last
    expect_eq(scc.components[0].len, 1);
    expect_eq(scc.components[0][0], 6);

    expect_eq(scc.components[1].len, 1);
    expect_eq(scc.components[1][0], 5);

    expect_eq(scc.components[2].len, 2);
    expect_eq(containsUsize(scc.components[2], 3), true);
    expect_eq(containsUsize(scc.components[2], 4), true);

    expect_eq(scc.components[3].len, 3);
    expect_eq(containsUsize(scc.components[3], 0), true);
    expect_eq(containsUsize(scc.components[3], 1), true);
    expect_eq(containsUsize(scc.components[3], 2), true);

    // Deep chain (1500 vertices) to verify iterative Tarjan avoids stack overflow
    let deep = DirectedGraph.init(alloc, 1500) catch unreachable;
    defer deep.deinit();
    let idx: usize = 0;
    while (idx + 1 < 1500) : (idx += 1) {
        deep.addEdge(idx, idx + 1, 1.0) catch unreachable;
    }
    deep.addEdge(1499, 0, 1.0) catch unreachable;

    let deep_scc = tarjanScc(alloc, deep) catch unreachable;
    defer deep_scc.deinit();
    expect_eq(deep_scc.count(), 1);
    expect_eq(deep_scc.components[0].len, 1500);
}

test("3. Dijkstra shortest path, reconstructPath, Disconnected & NegativeWeight guards") {
    const alloc = std.testing.allocator;

    let g = DirectedGraph.init(alloc, 6) catch unreachable;
    defer g.deinit();

    // 0 -> 1 (4.0), 0 -> 2 (2.0), 2 -> 1 (1.0), 1 -> 3 (5.0), 2 -> 3 (8.0), 3 -> 4 (2.0)
    // Vertex 5 is isolated (unreachable from 0)
    g.addEdge(0, 1, 4.0) catch unreachable;
    g.addEdge(0, 2, 2.0) catch unreachable;
    g.addEdge(2, 1, 1.0) catch unreachable;
    g.addEdge(1, 3, 5.0) catch unreachable;
    g.addEdge(2, 3, 8.0) catch unreachable;
    g.addEdge(3, 4, 2.0) catch unreachable;

    let res = dijkstra(alloc, g, 0) catch unreachable;
    defer res.deinit();

    // Shortest 0 -> 4 is 0 -> 2 -> 1 -> 3 -> 4 = 2 + 1 + 5 + 2 = 10.0
    expect_eq(approxEq(res.distanceTo(4) catch unreachable, 10.0), true);
    expect_eq(approxEq(res.distanceTo(0) catch unreachable, 0.0), true);
    expect_eq(res.isReachable(4), true);
    expect_eq(res.isReachable(5), false);

    const path = reconstructPath(alloc, res.prev, 0, 4) catch unreachable;
    defer alloc.free(path);
    expect_eq(path.len, 5);
    expect_eq(path[0], 0);
    expect_eq(path[1], 2);
    expect_eq(path[2], 1);
    expect_eq(path[3], 3);
    expect_eq(path[4], 4);

    // Same-vertex path [0]
    const self_path = reconstructPath(alloc, res.prev, 0, 0) catch unreachable;
    defer alloc.free(self_path);
    expect_eq(self_path.len, 1);
    expect_eq(self_path[0], 0);

    // Disconnected guard on unreachable vertex 5
    let disc_caught = false;
    * = reconstructPath(alloc, res.prev, 0, 5) catch |e| {
        if (e == GraphError.Disconnected) disc_caught = true;
    };
    expect_eq(disc_caught, true);

    // NegativeWeight guard
    g.addEdge(4, 5, -0.5) catch unreachable;
    let neg_caught = false;
    * = dijkstra(alloc, g, 0) catch |e| {
        if (e == GraphError.NegativeWeight) neg_caught = true;
    };
    expect_eq(neg_caught, true);
}

test("4. Bellman-Ford negative edge relaxation & negative-weight cycle detection") {
    const alloc = std.testing.allocator;

    let g = DirectedGraph.init(alloc, 4) catch unreachable;
    defer g.deinit();

    // 0 -> 1 (4.0), 0 -> 2 (5.0), 1 -> 2 (-3.0), 2 -> 3 (2.0)
    // Shortest 0 -> 3 is 0 -> 1 -> 2 -> 3 = 4 + (-3) + 2 = 3.0
    g.addEdge(0, 1, 4.0) catch unreachable;
    g.addEdge(0, 2, 5.0) catch unreachable;
    g.addEdge(1, 2, -3.0) catch unreachable;
    g.addEdge(2, 3, 2.0) catch unreachable;

    let bf = bellmanFord(alloc, g, 0) catch unreachable;
    defer bf.deinit();

    expect_eq(approxEq(bf.distanceTo(2) catch unreachable, 1.0), true);
    expect_eq(approxEq(bf.distanceTo(3) catch unreachable, 3.0), true);

    const path = reconstructPath(alloc, bf.prev, 0, 3) catch unreachable;
    defer alloc.free(path);
    expect_eq(path.len, 4);
    expect_eq(path[0], 0);
    expect_eq(path[1], 1);
    expect_eq(path[2], 2);
    expect_eq(path[3], 3);

    // Add negative cycle: 2 -> 1 (weight 1.0) => cycle 1 -> 2 -> 1 has total weight -3 + 1 = -2
    g.addEdge(2, 1, 1.0) catch unreachable;
    let neg_cyc_caught = false;
    * = bellmanFord(alloc, g, 0) catch |e| {
        if (e == GraphError.CycleDetected) neg_cyc_caught = true;
    };
    expect_eq(neg_cyc_caught, true);
}

test("5. cap.graph dynamic mounting & std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;

    let g = cap.graph.DirectedGraph.init(alloc, 3) catch unreachable;
    defer g.deinit();

    g.addEdge(0, 1, 2.5) catch unreachable;
    g.addEdge(1, 2, 3.5) catch unreachable;

    const topo = cap.graph.topologicalSort(alloc, g) catch unreachable;
    defer alloc.free(topo);
    expect_eq(topo.len, 3);
    expect_eq(topo[0], 0);
    expect_eq(topo[1], 1);
    expect_eq(topo[2], 2);

    let sp = cap.graph.dijkstra(alloc, g, 0) catch unreachable;
    defer sp.deinit();
    expect_eq(approxEq(sp.distanceTo(2) catch unreachable, 6.0), true);

    const path = cap.graph.reconstructPath(alloc, sp.prev, 0, 2) catch unreachable;
    defer alloc.free(path);
    expect_eq(path.len, 3);

    // Compatibility check with Graph & GraphAlgo via cap.graph
    let kg = cap.graph.Graph.init(alloc, true);
    defer kg.deinit();
    * = kg.addNode("A") catch unreachable;
    * = kg.addNode("B") catch unreachable;
    kg.addEdge("A", "B", 1.0, "link") catch unreachable;

    let kg_sp = cap.graph.GraphAlgo.dijkstra(alloc, kg, "A", "B") catch unreachable;
    defer kg_sp.deinit();
    expect_eq(kg_sp.found, true);
    expect_eq(approxEq(kg_sp.total_cost, 1.0), true);
}
