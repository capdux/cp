// Source: cp/cap/test/test_graph_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:graph & cap.graph

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GraphError,
    GraphEdge,
    Graph,
    ShortestPathResult,
    SccResult,
    MstEdge,
    MstResult,
    PageRankEntry,
    GraphAlgo,
} from "cap:graph";

fn strEq(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn containsStr(haystack: []const string, needle: string): bool {
    for (item of haystack) {
        if (strEq(item, needle)) return true;
    }
    return false;
}

fn indexOfStr(haystack: []const string, needle: string): usize? {
    let i: usize = 0;
    while (i < haystack.len) : (i += 1) {
        if (strEq(haystack[i], needle)) return i;
    }
    return null;
}

fn approxEq(a: f64, b: f64, eps: f64 = 0.000001): bool {
    return @abs(a - b) <= eps;
}

fn testGraphBuildTraversalAndKHop(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Directed Graph: idempotent addNode, ownership-safe key/label copy, BFS/DFS & kHopSubgraph
    let kg = Graph.init(alloc, true);
    defer kg.deinit();

    expect_eq(kg.isDirected(), true);

    // Mutate caller buffers right after addNode/addEdge to prove ownership safety
    let id_buf: [8]u8 = undefined;
    @memcpy(id_buf[0..5], "Agent");
    const idx_agent = kg.addNode(id_buf[0..5])?;
    @memcpy(id_buf[0..5], "XXXXX");

    // Idempotent addNode returns the same index without creating duplicates
    const idx_agent_again = kg.addNode("Agent")?;
    expect_eq(idx_agent, idx_agent_again);
    expect_eq(kg.findNode("Agent").?, idx_agent);
    expect_eq(kg.findNode("Missing") == null, true);
    expect_eq(strEq(kg.nodeId(idx_agent).?, "Agent"), true);
    expect_eq(kg.nodeId(999) == null, true);

    * = kg.addNode("Memory")?;
    * = kg.addNode("GraphRAG")?;
    * = kg.addNode("VectorDB")?;
    * = kg.addNode("BM25")?;
    * = kg.addNode("PageRank")?;
    expect_eq(kg.nodeCount(), 6);

    let lbl_buf: [8]u8 = undefined;
    @memcpy(lbl_buf[0..4], "uses");
    kg.addEdge("Agent", "Memory", 1.0, lbl_buf[0..4])?;
    @memcpy(lbl_buf[0..4], "ZZZZ");
    kg.addEdge("Agent", "GraphRAG", 1.2, "queries")?;
    kg.addEdge("Memory", "VectorDB", 2.0, "backed_by")?;
    kg.addEdge("GraphRAG", "BM25", 1.0, "fuses")?;
    kg.addEdge("GraphRAG", "PageRank", 1.5, "ranks")?;
    expect_eq(kg.edgeCount(), 5);

    // Missing node guard on addEdge & neighbors
    let missing_edge_err = false;
    kg.addEdge("Agent", "Ghost", 1.0, "x") catch e => {
        if (e == GraphError.NodeNotFound) missing_edge_err = true;
    };
    expect_eq(missing_edge_err, true);

    let missing_nbr_err = false;
    * = kg.neighbors("Ghost") catch e => {
        if (e == GraphError.NodeNotFound) missing_nbr_err = true;
    };
    expect_eq(missing_nbr_err, true);

    const agent_nbrs = kg.neighbors("Agent")?;
    expect_eq(agent_nbrs.len, 2);
    expect_eq(strEq(agent_nbrs[0].label, "uses"), true);
    expect_eq(strEq(agent_nbrs[1].label, "queries"), true);

    // BFS traversal from "Agent": level 0 [Agent], level 1 [Memory, GraphRAG], level 2 [VectorDB, BM25, PageRank]
    const bfs_order = kg.bfs(alloc, "Agent")?;
    defer alloc.free(bfs_order);
    expect_eq(bfs_order.len, 6);
    expect_eq(strEq(bfs_order[0], "Agent"), true);
    expect_eq(strEq(bfs_order[1], "Memory"), true);
    expect_eq(strEq(bfs_order[2], "GraphRAG"), true);
    expect_eq(strEq(bfs_order[3], "VectorDB"), true);
    expect_eq(strEq(bfs_order[4], "BM25"), true);
    expect_eq(strEq(bfs_order[5], "PageRank"), true);

    // DFS traversal from "Agent": Agent -> Memory -> VectorDB -> GraphRAG -> BM25 -> PageRank
    const dfs_order = kg.dfs(alloc, "Agent")?;
    defer alloc.free(dfs_order);
    expect_eq(dfs_order.len, 6);
    expect_eq(strEq(dfs_order[0], "Agent"), true);
    expect_eq(strEq(dfs_order[1], "Memory"), true);
    expect_eq(strEq(dfs_order[2], "VectorDB"), true);
    expect_eq(strEq(dfs_order[3], "GraphRAG"), true);
    expect_eq(strEq(dfs_order[4], "BM25"), true);
    expect_eq(strEq(dfs_order[5], "PageRank"), true);

    // GraphRAG kHopSubgraph recall: 0-hop, 1-hop, 2-hop
    const hop0 = kg.kHopSubgraph(alloc, "Agent", 0)?;
    defer alloc.free(hop0);
    expect_eq(hop0.len, 1);
    expect_eq(strEq(hop0[0], "Agent"), true);

    const hop1 = kg.kHopSubgraph(alloc, "Agent", 1)?;
    defer alloc.free(hop1);
    expect_eq(hop1.len, 3);
    expect_eq(containsStr(hop1, "Agent"), true);
    expect_eq(containsStr(hop1, "Memory"), true);
    expect_eq(containsStr(hop1, "GraphRAG"), true);
    expect_eq(containsStr(hop1, "PageRank"), false);

    const hop2 = kg.kHopSubgraph(alloc, "Agent", 2)?;
    defer alloc.free(hop2);
    expect_eq(hop2.len, 6);

    // 2. Undirected Graph: symmetric edges automatically added
    let ug = Graph.init(alloc, false);
    defer ug.deinit();
    expect_eq(ug.isDirected(), false);
    * = ug.addNode("N1")?;
    * = ug.addNode("N2")?;
    * = ug.addNode("N3")?;
    ug.addEdge("N1", "N2", 3.5, "link")?;
    ug.addEdge("N2", "N3", 2.5, "link")?;
    expect_eq(ug.edgeCount(), 2);

    // Reverse lookup from N3 must reach N2 -> N1
    const ug_bfs = ug.bfs(alloc, "N3")?;
    defer alloc.free(ug_bfs);
    expect_eq(ug_bfs.len, 3);
    expect_eq(strEq(ug_bfs[0], "N3"), true);
    expect_eq(strEq(ug_bfs[1], "N2"), true);
    expect_eq(strEq(ug_bfs[2], "N1"), true);
}

fn testDijkstraShortestPathUnreachableAndNegativeGuard(): !void {
    const alloc = std.heap.page_allocator;

    let g = Graph.init(alloc, true);
    defer g.deinit();

    for (name of ["S", "A", "B", "C", "D", "Isolated"]) {
        * = g.addNode(name)?;
    }

    // S -> A (4), S -> B (1), B -> A (2), A -> C (1), B -> C (5), C -> D (3)
    // Optimal S -> D is S -> B -> A -> C -> D with cost 1 + 2 + 1 + 3 = 7
    g.addEdge("S", "A", 4.0, "e1")?;
    g.addEdge("S", "B", 1.0, "e2")?;
    g.addEdge("B", "A", 2.0, "e3")?;
    g.addEdge("A", "C", 1.0, "e4")?;
    g.addEdge("B", "C", 5.0, "e5")?;
    g.addEdge("C", "D", 3.0, "e6")?;

    let res = GraphAlgo.dijkstra(alloc, g, "S", "D")?;
    defer res.deinit();
    expect_eq(res.found, true);
    expect_eq(approxEq(res.total_cost, 7.0), true);
    expect_eq(res.path.len, 5);
    expect_eq(strEq(res.path[0], "S"), true);
    expect_eq(strEq(res.path[1], "B"), true);
    expect_eq(strEq(res.path[2], "A"), true);
    expect_eq(strEq(res.path[3], "C"), true);
    expect_eq(strEq(res.path[4], "D"), true);

    // Zero-cost self path
    let self_res = GraphAlgo.dijkstra(alloc, g, "A", "A")?;
    defer self_res.deinit();
    expect_eq(self_res.found, true);
    expect_eq(approxEq(self_res.total_cost, 0.0), true);
    expect_eq(self_res.path.len, 1);
    expect_eq(strEq(self_res.path[0], "A"), true);

    // Unreachable target
    let unreach = GraphAlgo.dijkstra(alloc, g, "S", "Isolated")?;
    defer unreach.deinit();
    expect_eq(unreach.found, false);
    expect_eq(unreach.path.len, 0);

    // Negative weight edge rejection
    g.addEdge("D", "Isolated", -0.5, "bad")?;
    let neg_caught = false;
    * = GraphAlgo.dijkstra(alloc, g, "S", "D") catch e => {
        if (e == GraphError.NegativeWeight) neg_caught = true;
    };
    expect_eq(neg_caught, true);
}

fn testTarjanSccAndTopologicalSort(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Acyclic DAG topological sort
    let dag = Graph.init(alloc, true);
    defer dag.deinit();

    for (name of ["compile", "link", "test", "package", "deploy"]) {
        * = dag.addNode(name)?;
    }
    dag.addEdge("compile", "link", 1.0, "dep")?;
    dag.addEdge("compile", "test", 1.0, "dep")?;
    dag.addEdge("link", "package", 1.0, "dep")?;
    dag.addEdge("test", "package", 1.0, "dep")?;
    dag.addEdge("package", "deploy", 1.0, "dep")?;

    const topo = GraphAlgo.topologicalSort(alloc, dag)?;
    defer alloc.free(topo);
    expect_eq(topo.len, 5);
    const p_compile = indexOfStr(topo, "compile").?;
    const p_link = indexOfStr(topo, "link").?;
    const p_test = indexOfStr(topo, "test").?;
    const p_package = indexOfStr(topo, "package").?;
    const p_deploy = indexOfStr(topo, "deploy").?;
    expect_eq(p_compile < p_link, true);
    expect_eq(p_compile < p_test, true);
    expect_eq(p_link < p_package, true);
    expect_eq(p_test < p_package, true);
    expect_eq(p_package < p_deploy, true);

    // 2. Graph with multiple SCCs: {A,B,C}, {D,E}, {F}
    let scc_g = Graph.init(alloc, true);
    defer scc_g.deinit();

    for (name of ["A", "B", "C", "D", "E", "F"]) {
        * = scc_g.addNode(name)?;
    }
    // Cycle 1: A -> B -> C -> A
    scc_g.addEdge("A", "B", 1.0, "")?;
    scc_g.addEdge("B", "C", 1.0, "")?;
    scc_g.addEdge("C", "A", 1.0, "")?;
    // Cross link to Cycle 2: C -> D
    scc_g.addEdge("C", "D", 1.0, "")?;
    // Cycle 2: D <-> E
    scc_g.addEdge("D", "E", 1.0, "")?;
    scc_g.addEdge("E", "D", 1.0, "")?;
    // Singleton sink F: E -> F
    scc_g.addEdge("E", "F", 1.0, "")?;

    // Topological sort must detect cycle
    let cycle_caught = false;
    * = GraphAlgo.topologicalSort(alloc, scc_g) catch e => {
        if (e == GraphError.CycleDetected) cycle_caught = true;
    };
    expect_eq(cycle_caught, true);

    // Tarjan SCC decomposition must produce exactly 3 components of sizes {3, 2, 1}
    let scc = GraphAlgo.tarjanScc(alloc, scc_g)?;
    defer scc.deinit();
    expect_eq(scc.count(), 3);

    let found_abc = false;
    let found_de = false;
    let found_f = false;
    for (comp of scc.components) {
        if (comp.len == 3 && containsStr(comp, "A") && containsStr(comp, "B") && containsStr(comp, "C")) {
            found_abc = true;
        } else if (comp.len == 2 && containsStr(comp, "D") && containsStr(comp, "E")) {
            found_de = true;
        } else if (comp.len == 1 && containsStr(comp, "F")) {
            found_f = true;
        }
    }
    expect_eq(found_abc, true);
    expect_eq(found_de, true);
    expect_eq(found_f, true);
}

fn testKruskalMstAndPageRank(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Undirected weighted graph for Kruskal MST (4 vertices, classic MST weight = 1 + 2 + 3 = 6)
    let ug = Graph.init(alloc, false);
    defer ug.deinit();
    for (name of ["V0", "V1", "V2", "V3"]) {
        * = ug.addNode(name)?;
    }
    ug.addEdge("V0", "V1", 10.0, "heavy")?;
    ug.addEdge("V0", "V2", 6.0, "mid")?;
    ug.addEdge("V0", "V3", 5.0, "e3")?;
    ug.addEdge("V1", "V3", 15.0, "too_heavy")?;
    ug.addEdge("V2", "V3", 4.0, "light")?;

    // Expected MST: (V2-V3: 4), (V0-V3: 5), (V0-V1: 10) -> total = 19.0, 3 edges
    let mst = GraphAlgo.kruskalMst(alloc, ug)?;
    defer mst.deinit();
    expect_eq(mst.edges.len, 3);
    expect_eq(approxEq(mst.total_weight, 19.0), true);

    // 2. PageRank centrality: Hub node pointed to by all spokes must rank #1
    let pr_g = Graph.init(alloc, true);
    defer pr_g.deinit();
    for (name of ["Hub", "Leaf1", "Leaf2", "Leaf3", "Leaf4", "Periphery"]) {
        * = pr_g.addNode(name)?;
    }
    pr_g.addEdge("Leaf1", "Hub", 1.0, "cites")?;
    pr_g.addEdge("Leaf2", "Hub", 1.0, "cites")?;
    pr_g.addEdge("Leaf3", "Hub", 1.0, "cites")?;
    pr_g.addEdge("Leaf4", "Hub", 1.0, "cites")?;
    pr_g.addEdge("Periphery", "Leaf1", 1.0, "cites")?;
    pr_g.addEdge("Hub", "Leaf1", 1.0, "refs")?;

    const ranks = GraphAlgo.pageRank(alloc, pr_g, 0.85, 30)?;
    defer alloc.free(ranks);
    expect_eq(ranks.len, 6);
    expect_eq(strEq(ranks[0].id, "Hub"), true);

    // Verify descending order and probability mass conservation (~1.0)
    let sum_score: f64 = 0.0;
    let i: usize = 0;
    while (i < ranks.len) : (i += 1) {
        sum_score += ranks[i].score;
        if (i + 1 < ranks.len) {
            expect_eq(ranks[i].score >= ranks[i + 1].score, true);
        }
    }
    expect_eq(approxEq(sum_score, 1.0, 0.00001), true);
    expect_eq(ranks[0].score > ranks[ranks.len - 1].score, true);
}

fn testCapGraphAutomaticLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.graph.*` dynamic lazy mounting
    let g = cap.graph.Graph.init(alloc, true);
    defer g.deinit();

    * = g.addNode("DocA")?;
    * = g.addNode("EntityX")?;
    * = g.addNode("EntityY")?;
    g.addEdge("DocA", "EntityX", 1.0, "mentions")?;
    g.addEdge("EntityX", "EntityY", 2.0, "related_to")?;

    const sub = g.kHopSubgraph(alloc, "DocA", 2)?;
    defer alloc.free(sub);
    expect_eq(sub.len, 3);

    let sp: cap.graph.ShortestPathResult = cap.graph.GraphAlgo.dijkstra(alloc, g, "DocA", "EntityY")?;
    defer sp.deinit();
    expect_eq(sp.found, true);
    expect_eq(approxEq(sp.total_cost, 3.0), true);

    const pr = cap.graph.GraphAlgo.pageRank(alloc, g, 0.85, 20)?;
    defer alloc.free(pr);
    expect_eq(pr.len, 3);
    expect_eq(strEq(pr[0].id, "EntityY"), true);
}

fn main() {
    test("1. Graph Directed/Undirected Build, Ownership Safety, BFS/DFS & kHopSubgraph", () => {
        testGraphBuildTraversalAndKHop()?;
    });
    test("2. Dijkstra Shortest Path Reconstruction, Unreachable & NegativeWeight Guard", () => {
        testDijkstraShortestPathUnreachableAndNegativeGuard()?;
    });
    test("3. Tarjan SCC Decomposition & Kahn Topological Sort (with CycleDetected)", () => {
        testTarjanSccAndTopologicalSort()?;
    });
    test("4. Kruskal MST (DSU Path Compression + Rank) & PageRank Centrality Ranking", () => {
        testKruskalMstAndPageRank()?;
    });
    test("5. cap.graph Automatic Dynamic Lazy Mounting Verification", () => {
        testCapGraphAutomaticLazyMount()?;
    });
}
