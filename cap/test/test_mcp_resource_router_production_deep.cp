// Origin: Pure-CP Stdlib Test Suite (MCP Resource URI Template Matcher & Subscription Dispatcher) | Created: 2026-11-22
// Deep Production Verification Suite for lib/mcp/resource_router.cp
// Covers:
//   1. RFC 6570 simple {var} single/multi-parameter URI template matching, zero-copy slicing & truncation safety
//   2. Exact static route precedence over parameterized templates & fallback after static route removal
//   3. Multi-client resource subscription, unsubscription, client disconnect cleanup & broadcast fan-out
//   4. Idempotent duplicate subscription deduplication & slot reuse after unsubscribe
//   5. Malformed templates/URIs, segment overflow, empty segments, full route/subscription pools & stats telemetry

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    UriParamCapture,
    McpResourceRoute,
    McpResourceMatch,
    McpSubscription,
    McpResourceRouterStats,
    hashResourceUri,
    isValidUriTemplate,
    isStaticUriTemplate,
    matchUriTemplate,
    McpResourceRouter,
} from "../../lib/mcp/index.cp";

fn testUriTemplateMatchingAndExtraction()! {
    let params_buf: [4]UriParamCapture = [UriParamCapture{}; 4];
    let params: []UriParamCapture = params_buf[0..];

    // 1. Multi-segment template: db://{schema}/{table}
    const c1 = matchUriTemplate("db://{schema}/{table}", "db://public/users", params);
    expect_eq(c1, 2);
    expect_eq(std.mem.eql(u8, params[0].name, "schema"), true);
    expect_eq(std.mem.eql(u8, params[0].value, "public"), true);
    expect_eq(std.mem.eql(u8, params[1].name, "table"), true);
    expect_eq(std.mem.eql(u8, params[1].value, "users"), true);

    // 2. Single-parameter template with literal suffix
    const c2 = matchUriTemplate("file:///logs/{name}.log", "file:///logs/gateway.log", params);
    expect_eq(c2, 1);
    expect_eq(std.mem.eql(u8, params[0].name, "name"), true);
    expect_eq(std.mem.eql(u8, params[0].value, "gateway"), true);

    // 3. Inline delimiter parameters within a single path segment
    const c3 = matchUriTemplate(
        "metrics://{region}/{year}-{month}.csv",
        "metrics://ap-southeast/2026-11.csv",
        params,
    );
    expect_eq(c3, 3);
    expect_eq(std.mem.eql(u8, params[0].name, "region"), true);
    expect_eq(std.mem.eql(u8, params[0].value, "ap-southeast"), true);
    expect_eq(std.mem.eql(u8, params[1].name, "year"), true);
    expect_eq(std.mem.eql(u8, params[1].value, "2026"), true);
    expect_eq(std.mem.eql(u8, params[2].name, "month"), true);
    expect_eq(std.mem.eql(u8, params[2].value, "11"), true);

    // 4. Static URI exact match yields 0 captured params
    const c4 = matchUriTemplate("config://app/settings", "config://app/settings", params);
    expect_eq(c4, 0);

    // 5. Truncation-safe capture when out_params buffer is smaller than parameter count
    let small_buf: [1]UriParamCapture = [UriParamCapture{}; 1];
    let small_params: []UriParamCapture = small_buf[0..];
    const c5 = matchUriTemplate("db://{schema}/{table}", "db://analytics/events", small_params);
    expect_eq(c5, 2);
    expect_eq(std.mem.eql(u8, small_params[0].name, "schema"), true);
    expect_eq(std.mem.eql(u8, small_params[0].value, "analytics"), true);
}

fn testStaticRoutePrecedenceOverTemplate()! {
    let route_buf: [4]McpResourceRoute = [McpResourceRoute{}; 4];
    let sub_buf: [4]McpSubscription = [McpSubscription{}; 4];
    let routes: []McpResourceRoute = route_buf[0..];
    let subs: []McpSubscription = sub_buf[0..];
    let router = McpResourceRouter.init(routes, subs);

    // Register parameterized template FIRST, then exact static route SECOND.
    expect_eq(router.registerTemplate(10, "db://{schema}/{table}", "application/json", true), 0);
    expect_eq(router.registerTemplate(20, "db://public/users", "application/vnd.mcp.users+json", false), 1);

    let params_buf: [4]UriParamCapture = [UriParamCapture{}; 4];
    let params: []UriParamCapture = params_buf[0..];

    // Static route must win over template registered earlier
    const m1 = router.resolveUri("db://public/users", params) ?? McpResourceMatch{};
    expect_eq(m1.route_id, 20);
    expect_eq(m1.slot_index, 1);
    expect_eq(m1.param_count, 0);
    expect_eq(m1.is_static, true);

    const static_rec = router.getRoute(m1.slot_index) ?? McpResourceRoute{};
    expect_eq(static_rec.read_only, false);
    expect_eq(std.mem.eql(u8, static_rec.mime_type, "application/vnd.mcp.users+json"), true);

    // Non-static sibling URI resolves to parameterized template
    const m2 = router.resolveUri("db://public/orders", params) ?? McpResourceMatch{};
    expect_eq(m2.route_id, 10);
    expect_eq(m2.slot_index, 0);
    expect_eq(m2.param_count, 2);
    expect_eq(m2.is_static, false);
    expect_eq(std.mem.eql(u8, params[0].value, "public"), true);
    expect_eq(std.mem.eql(u8, params[1].value, "orders"), true);

    // Removing the static route causes `db://public/users` to fall back to the template route
    expect_eq(router.unregisterRoute(20), true);
    expect_eq(router.unregisterRoute(20), false);
    expect_eq(router.getRoute(1) == null, true);

    const m3 = router.resolveUri("db://public/users", params) ?? McpResourceMatch{};
    expect_eq(m3.route_id, 10);
    expect_eq(m3.param_count, 2);
    expect_eq(m3.is_static, false);
    expect_eq(std.mem.eql(u8, params[1].value, "users"), true);
}

fn testMultiClientSubscriptionsAndFanout()! {
    let route_buf: [2]McpResourceRoute = [McpResourceRoute{}; 2];
    let sub_buf: [8]McpSubscription = [McpSubscription{}; 8];
    let routes: []McpResourceRoute = route_buf[0..];
    let subs: []McpSubscription = sub_buf[0..];
    let router = McpResourceRouter.init(routes, subs);

    expect_eq(router.subscribe(101, "db://public/users"), true);
    expect_eq(router.subscribe(102, "db://public/users"), true);
    expect_eq(router.subscribe(103, "db://public/users"), true);
    expect_eq(router.subscribe(101, "db://public/orders"), true);
    expect_eq(router.subscribe(104, "db://public/orders"), true);

    let out_buf: [8]u32 = [0; 8];
    let out_ids: []u32 = out_buf[0..];

    const n_users = router.collectSubscribers("db://public/users", out_ids);
    expect_eq(n_users, 3);
    expect_eq(out_ids[0], 101);
    expect_eq(out_ids[1], 102);
    expect_eq(out_ids[2], 103);

    const n_orders = router.collectSubscribers("db://public/orders", out_ids);
    expect_eq(n_orders, 2);
    expect_eq(out_ids[0], 101);
    expect_eq(out_ids[1], 104);

    // Output truncation safety when caller buffer is smaller than subscriber count
    let tiny_buf: [2]u32 = [0; 2];
    let tiny_ids: []u32 = tiny_buf[0..];
    const n_trunc = router.collectSubscribers("db://public/users", tiny_ids);
    expect_eq(n_trunc, 2);
    expect_eq(tiny_ids[0], 101);
    expect_eq(tiny_ids[1], 102);

    // Unsubscribe single client from single URI
    expect_eq(router.unsubscribe(102, "db://public/users"), true);
    expect_eq(router.unsubscribe(102, "db://public/users"), false);
    const n_after_unsub = router.collectSubscribers("db://public/users", out_ids);
    expect_eq(n_after_unsub, 2);
    expect_eq(out_ids[0], 101);
    expect_eq(out_ids[1], 103);

    // Disconnect cleanup removes all subscriptions for client 101 across all URIs
    expect_eq(router.unsubscribeClient(101), 2);
    expect_eq(router.unsubscribeClient(101), 0);
    expect_eq(router.collectSubscribers("db://public/users", out_ids), 1);
    expect_eq(out_ids[0], 103);
    expect_eq(router.collectSubscribers("db://public/orders", out_ids), 1);
    expect_eq(out_ids[0], 104);
}

fn testDuplicateSubscriptionDeduplication()! {
    let route_buf: [2]McpResourceRoute = [McpResourceRoute{}; 2];
    let sub_buf: [2]McpSubscription = [McpSubscription{}; 2];
    let routes: []McpResourceRoute = route_buf[0..];
    let subs: []McpSubscription = sub_buf[0..];
    let router = McpResourceRouter.init(routes, subs);

    // Repeated subscription by same client for same URI must not consume extra slots
    expect_eq(router.subscribe(7, "file:///workspace/README.md"), true);
    expect_eq(router.subscribe(7, "file:///workspace/README.md"), true);
    expect_eq(router.subscribe(7, "file:///workspace/README.md"), true);
    expect_eq(router.stats().subscription_count, 1);

    // Second distinct slot is still available
    expect_eq(router.subscribe(8, "file:///workspace/README.md"), true);
    expect_eq(router.stats().subscription_count, 2);

    // Duplicate on a full pool still succeeds idempotently
    expect_eq(router.subscribe(7, "file:///workspace/README.md"), true);
    expect_eq(router.stats().subscription_count, 2);

    // Fan-out contains each client exactly once
    let out_buf: [4]u32 = [0; 4];
    let out_ids: []u32 = out_buf[0..];
    expect_eq(router.collectSubscribers("file:///workspace/README.md", out_ids), 2);
    expect_eq(out_ids[0], 7);
    expect_eq(out_ids[1], 8);

    // Unsubscribe frees slot for a new subscriber
    expect_eq(router.unsubscribe(7, "file:///workspace/README.md"), true);
    expect_eq(router.stats().subscription_count, 1);
    expect_eq(router.subscribe(9, "file:///workspace/README.md"), true);
    expect_eq(router.stats().subscription_count, 2);
}

fn testMalformedInputsAndFullPoolGuards()! {
    // 1. Template syntax validation & hash determinism
    expect_eq(isValidUriTemplate(""), false);
    expect_eq(isValidUriTemplate("db://{schema"), false);
    expect_eq(isValidUriTemplate("db://{}"), false);
    expect_eq(isValidUriTemplate("db://{{schema}}"), false);
    expect_eq(isValidUriTemplate("db://{a/b}"), false);
    expect_eq(isValidUriTemplate("db://public}"), false);
    expect_eq(isValidUriTemplate("db://{a}{b}"), false);
    expect_eq(isValidUriTemplate("db://{a b}"), false);
    expect_eq(isValidUriTemplate("db://{schema}/{table}"), true);
    expect_eq(isStaticUriTemplate("db://public/users"), true);
    expect_eq(isStaticUriTemplate("db://{schema}/{table}"), false);
    expect_eq(hashResourceUri("db://public/users") == hashResourceUri("db://public/users"), true);
    expect_eq(hashResourceUri("db://public/users") != hashResourceUri("db://public/orders"), true);

    // 2. Non-matching & malformed URI checks
    let params_buf: [4]UriParamCapture = [UriParamCapture{}; 4];
    let params: []UriParamCapture = params_buf[0..];
    expect_eq(matchUriTemplate("db://{schema}/{table}", "", params) == null, true);
    expect_eq(matchUriTemplate("db://{schema}/{table}", "db:///users", params) == null, true);
    expect_eq(matchUriTemplate("db://{schema}/{table}", "db://public/", params) == null, true);
    expect_eq(matchUriTemplate("db://{schema}/{table}", "db://public/users/extra", params) == null, true);
    expect_eq(matchUriTemplate("db://{schema}/{table}", "http://public/users", params) == null, true);
    expect_eq(matchUriTemplate("db://{schema", "db://public", params) == null, true);
    expect_eq(matchUriTemplate("db://{a}{b}", "db://xy", params) == null, true);

    // 3. Full route & subscription pool guards + invalid IDs
    let route_buf: [2]McpResourceRoute = [McpResourceRoute{}; 2];
    let sub_buf: [2]McpSubscription = [McpSubscription{}; 2];
    let routes: []McpResourceRoute = route_buf[0..];
    let subs: []McpSubscription = sub_buf[0..];
    let router = McpResourceRouter.init(routes, subs);

    expect_eq(router.registerTemplate(0, "db://{s}/{t}", "text/plain", true) == null, true);
    expect_eq(router.registerTemplate(1, "db://{bad", "text/plain", true) == null, true);
    expect_eq(router.registerTemplate(1, "db://{s}/{t}", "text/plain", true), 0);
    // Duplicate route_id and duplicate template rejected
    expect_eq(router.registerTemplate(1, "file:///{p}", "text/plain", true) == null, true);
    expect_eq(router.registerTemplate(2, "db://{s}/{t}", "text/plain", true) == null, true);
    expect_eq(router.registerTemplate(2, "file:///{p}", "text/plain", true), 1);
    // Route pool full
    expect_eq(router.registerTemplate(3, "mem:///{k}", "text/plain", true) == null, true);

    // Subscription guards & full pool
    expect_eq(router.subscribe(0, "db://a/b"), false);
    expect_eq(router.subscribe(1, ""), false);
    expect_eq(router.subscribe(1, "db://a/b"), true);
    expect_eq(router.subscribe(2, "db://a/b"), true);
    expect_eq(router.subscribe(3, "db://a/b"), false);

    // Verify stats telemetry
    * = router.resolveUri("db://a/b", params);
    * = router.resolveUri("unknown://x", params);
    * = router.resolveUri("", params);
    let out_buf: [2]u32 = [0; 2];
    let out_ids: []u32 = out_buf[0..];
    * = router.collectSubscribers("db://a/b", out_ids);

    const st: McpResourceRouterStats = router.stats();
    expect_eq(st.route_count, 2);
    expect_eq(st.route_capacity, 2);
    expect_eq(st.subscription_count, 2);
    expect_eq(st.subscription_capacity, 2);
    expect_eq(st.total_resolves, 3);
    expect_eq(st.total_hits, 1);
    expect_eq(st.total_misses, 2);
    expect_eq(st.total_broadcasts, 1);
}

test("mcp resource_router: RFC 6570 URI template matching and zero-copy param extraction", testUriTemplateMatchingAndExtraction);
test("mcp resource_router: exact static route precedence over parameterized template", testStaticRoutePrecedenceOverTemplate);
test("mcp resource_router: multi-client subscribe, unsubscribe, disconnect cleanup and fan-out", testMultiClientSubscriptionsAndFanout);
test("mcp resource_router: idempotent duplicate subscription deduplication and slot reuse", testDuplicateSubscriptionDeduplication);
test("mcp resource_router: malformed templates/URIs, full pool guards and stats telemetry", testMalformedInputsAndFullPoolGuards);
