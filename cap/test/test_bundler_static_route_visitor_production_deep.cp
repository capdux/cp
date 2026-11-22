// Production Deep Tests for cap:bundler Static Route Visitor (lib/bundler/static_route_visitor.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_STATIC_ROUTE_NODES,
    INVALID_ROUTE_SOURCE,
    StaticRouteError,
    StaticRouteGraph,
    StaticRouteVisitor,
    ServerComponentBoundaryTable,
} from "cap:bundler";

test("bundler static_route: pure server route and use_server boundary remain fully static", fn() {
    let graph = StaticRouteGraph.init(5) catch unreachable;
    graph.addImport(0, 1) catch unreachable;
    graph.addImport(1, 2) catch unreachable;
    graph.addImport(1, 3) catch unreachable;
    // External/invalid import record is safely ignored
    graph.addImport(2, INVALID_ROUTE_SOURCE) catch unreachable;

    let scb = ServerComponentBoundaryTable.init();
    // `"use server"` boundary does NOT make a route client-interactive
    * = scb.putSharedSsr(3, .use_server, 4) catch unreachable;

    let visitor = StaticRouteVisitor.init();
    expect_eq(visitor.hasTransitiveUseClient(&graph, &scb, 0) catch unreachable, false);
    expect_eq(visitor.isFullyStatic(&graph, &scb, 0) catch unreachable, true);
});

test("bundler static_route: detects deep transitive use_client boundary via reference_source_index", fn() {
    let graph = StaticRouteGraph.init(6) catch unreachable;
    // Route 0 -> layout 1 -> nav 2 -> client button reference 5
    graph.addImport(0, 1) catch unreachable;
    graph.addImport(1, 2) catch unreachable;
    graph.addImport(2, 5) catch unreachable;

    let scb = ServerComponentBoundaryTable.init();
    // Canonical client module 4 generates server-side reference module 5
    * = scb.putSharedSsr(4, .use_client, 5) catch unreachable;

    let visitor = StaticRouteVisitor.init();
    expect_eq(visitor.hasTransitiveUseClient(&graph, &scb, 0) catch unreachable, true);
    expect_eq(visitor.isFullyStatic(&graph, &scb, 0) catch unreachable, false);
});

test("bundler static_route: entry point itself is a client boundary", fn() {
    let graph = StaticRouteGraph.init(4) catch unreachable;
    let scb = ServerComponentBoundaryTable.init();
    * = scb.putSharedSsr(2, .use_client, 3) catch unreachable;

    let visitor = StaticRouteVisitor.init();
    expect_eq(visitor.isFullyStatic(&graph, &scb, 2) catch unreachable, false);
    expect_eq(visitor.nodes_visited, @as(u32, 0));
});

test("bundler static_route: cycle-safe iterative DFS for both static and interactive cycles", fn() {
    let graph = StaticRouteGraph.init(5) catch unreachable;
    // Cycle: 0 -> 1 -> 2 -> 0
    graph.addImport(0, 1) catch unreachable;
    graph.addImport(1, 2) catch unreachable;
    graph.addImport(2, 0) catch unreachable;

    let scb = ServerComponentBoundaryTable.init();
    let visitor = StaticRouteVisitor.init();
    expect_eq(visitor.isFullyStatic(&graph, &scb, 0) catch unreachable, true);

    // Now add an edge from node 2 to a client boundary 4
    graph.addImport(2, 4) catch unreachable;
    * = scb.putSharedSsr(3, .use_client, 4) catch unreachable;

    visitor.resetCache();
    expect_eq(visitor.isFullyStatic(&graph, &scb, 0) catch unreachable, false);
});

test("bundler static_route: memoization cache hit avoids redundant traversal", fn() {
    let graph = StaticRouteGraph.init(5) catch unreachable;
    // Two routes 0 and 1 share subtree 2 -> 3 -> 4
    graph.addImport(0, 2) catch unreachable;
    graph.addImport(1, 2) catch unreachable;
    graph.addImport(2, 3) catch unreachable;
    graph.addImport(3, 4) catch unreachable;

    let scb = ServerComponentBoundaryTable.init();
    let visitor = StaticRouteVisitor.init();

    // First query on route 0
    expect_eq(visitor.isFullyStatic(&graph, &scb, 0) catch unreachable, true);
    const visits_after_first = visitor.nodes_visited;
    expect_eq(visits_after_first, @as(u32, 4));

    // Repeated query on route 0 must hit cache with 0 extra visits
    expect_eq(visitor.isFullyStatic(&graph, &scb, 0) catch unreachable, true);
    expect_eq(visitor.cache_hits, @as(u32, 1));
    expect_eq(visitor.nodes_visited, visits_after_first);

    // resetCache clears stats
    visitor.resetCache();
    expect_eq(visitor.cache_hits, @as(u32, 0));
    expect_eq(visitor.nodes_visited, @as(u32, 0));
});

test("bundler static_route: out-of-range node index and capacity guards", fn() {
    let init_err = false;
    * = StaticRouteGraph.init(@as(u32, @intCast(MAX_STATIC_ROUTE_NODES)) + 1) catch |e| {
        init_err = (e == StaticRouteError.NodeIndexOutOfRange);
        StaticRouteGraph{}
    };
    expect_eq(init_err, true);

    let graph = StaticRouteGraph.init(3) catch unreachable;
    let edge_err = false;
    graph.addImport(3, 1) catch |e| {
        edge_err = (e == StaticRouteError.NodeIndexOutOfRange);
    };
    expect_eq(edge_err, true);

    let scb = ServerComponentBoundaryTable.init();
    let visitor = StaticRouteVisitor.init();
    let query_err = false;
    * = visitor.isFullyStatic(&graph, &scb, 5) catch |e| {
        query_err = (e == StaticRouteError.NodeIndexOutOfRange);
        false
    };
    expect_eq(query_err, true);
});
