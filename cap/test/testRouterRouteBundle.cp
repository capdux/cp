// Origin: Pure-CP Stdlib Test Suite (lib/router/routeBundle.cp) | Created: 2026-11-22
// Production Deep Verification Suite for DevServer RouteBundle Lifecycle & Registry:
//   1. Framework & HTML route registration, findByPattern & initial_gen normalization
//   2. 6-state bundling lifecycle transitions & InvalidStateTransition guards
//   3. invalidateClientBundle generation rotation (including u32::MAX wrap), staleness check & HTML cache clear
//   4. addViewer / removeViewer HMR viewer ref-counting, activeRouteCount & ViewerUnderflow guard
//   5. sourceMapKey() derivation, formatClientBundleUrl() & BufferTooSmall guard
//   6. Boundary guards: InvalidRouteIndex, totalMemoryCost() & CapacityExceeded (128 routes)

import { test, expect_eq, expect_true } from "cap:test";
import {
    ROUTE_BUNDLE_MAX_COUNT,
    RouteBundleError,
    RouteBundleKind,
    RouteBundleState,
    RouteBundleRegistry,
} from "cap:router";

test("RouteBundleRegistry: registration, findByPattern & generation normalization", fn() {
    let reg = RouteBundleRegistry.init();
    expect_eq(reg.count(), 0);

    const id_fw = reg.registerFramework("/", 0) catch 999;
    const id_html = reg.registerHtml("/dashboard", 42) catch 999;
    expect_eq(id_fw, 0);
    expect_eq(id_html, 1);
    expect_eq(reg.count(), 2);

    const fw = reg.get(id_fw) catch unreachable;
    expect_eq(fw.kind, RouteBundleKind.Framework);
    expect_eq(fw.state, RouteBundleState.Unqueued);
    expect_eq(fw.client_script_generation, 1); // 0 normalized to 1
    expect_true(fw.state.isPending());

    const html = reg.get(id_html) catch unreachable;
    expect_eq(html.kind, RouteBundleKind.Html);
    expect_eq(html.client_script_generation, 42);

    if (found := reg.findByPattern("/dashboard")) {
        expect_eq(found, id_html);
    } else {
        expect_true(false);
    }
    expect_true(reg.findByPattern("/missing") == null);
});

test("RouteBundleRegistry: 6-state lifecycle transitions & InvalidStateTransition guards", fn() {
    let reg = RouteBundleRegistry.init();
    const id = reg.registerFramework("/api/home", 1) catch unreachable;

    // Cannot markLoaded directly from Unqueued
    let bad_load = false;
    reg.markLoaded(id, 100, 0, 0, null) catch |e| {
        if (e == RouteBundleError.InvalidStateTransition) bad_load = true;
    };
    expect_true(bad_load);

    // Unqueued -> Bundling
    expect_eq(reg.beginBundling(id) catch unreachable, RouteBundleState.Bundling);

    // Cannot re-enter Bundling while already Bundling
    let bad_reenter = false;
    reg.beginBundling(id) catch |e| {
        if (e == RouteBundleError.InvalidStateTransition) bad_reenter = true;
    };
    expect_true(bad_reenter);

    // Bundling -> Loaded
    expect_eq(reg.markLoaded(id, 2048, 0, 0, null) catch unreachable, RouteBundleState.Loaded);
    const loaded = reg.get(id) catch unreachable;
    expect_true(loaded.state.isReady());
    expect_true(!loaded.state.isFailed());

    // Failure & deferred states
    expect_eq(reg.markBundlingFailure(id) catch unreachable, RouteBundleState.PossibleBundlingFailures);
    expect_true((reg.get(id) catch unreachable).state.isFailed());
    expect_eq(reg.markEvaluationFailure(id) catch unreachable, RouteBundleState.EvaluationFailure);
    expect_eq((reg.get(id) catch unreachable).failure_count, 2);

    expect_eq(reg.deferToNextBundle(id) catch unreachable, RouteBundleState.DeferredToNextBundle);
    expect_true((reg.get(id) catch unreachable).state.isPending());
});

test("RouteBundleRegistry: invalidateClientBundle rotation, wrap-around & HTML cache clear", fn() {
    let reg = RouteBundleRegistry.init();
    const id = reg.registerHtml("/app", 0xFFFFFFFF) catch unreachable;

    * = reg.beginBundling(id) catch unreachable;
    * = reg.markLoaded(id, 4096, 1024, 1500, 128) catch unreachable;

    const before = reg.get(id) catch unreachable;
    expect_true(before.isGenerationCurrent(0xFFFFFFFF));
    expect_true(!before.isGenerationCurrent(1));
    expect_eq(before.memoryCost(), 4096 + 1024 + 1500);

    // Invalidate rotates 0xFFFFFFFF -> 1 (skipping 0), clears client_bundle & cached_response_bytes
    const next_gen = reg.invalidateClientBundle(id) catch 0;
    expect_eq(next_gen, 1);

    const after = reg.get(id) catch unreachable;
    expect_true(!after.has_client_bundle);
    expect_true(!after.isGenerationCurrent(0xFFFFFFFF));
    expect_true(!after.isGenerationCurrent(1)); // has_client_bundle is false until re-bundled
    expect_eq(after.client_bundle_bytes, 0);
    expect_eq(after.cached_response_bytes, 0);
    expect_eq(after.bundled_html_bytes, 1024); // HTML source text preserved until HTML file changes
    expect_eq(after.memoryCost(), 1024);
});

test("RouteBundleRegistry: HMR viewer ref-counting, activeRouteCount & ViewerUnderflow", fn() {
    let reg = RouteBundleRegistry.init();
    const r0 = reg.registerFramework("/", 1) catch unreachable;
    const r1 = reg.registerHtml("/settings", 1) catch unreachable;

    expect_eq(reg.activeRouteCount(), 0);
    expect_eq(reg.addViewer(r0) catch 0, 1);
    expect_eq(reg.addViewer(r0) catch 0, 2);
    expect_eq(reg.addViewer(r1) catch 0, 1);
    expect_eq(reg.activeRouteCount(), 2);
    expect_true((reg.get(r0) catch unreachable).hasActiveViewers());

    expect_eq(reg.removeViewer(r1) catch 99, 0);
    expect_eq(reg.activeRouteCount(), 1);
    expect_true(!(reg.get(r1) catch unreachable).hasActiveViewers());

    // Removing viewer when count == 0 -> ViewerUnderflow
    let underflow = false;
    reg.removeViewer(r1) catch |e| {
        if (e == RouteBundleError.ViewerUnderflow) underflow = true;
    };
    expect_true(underflow);
});

test("RouteBundleEntry: sourceMapKey, formatClientBundleUrl & BufferTooSmall", fn() {
    let reg = RouteBundleRegistry.init();
    const id = reg.registerHtml("/home", 0x00ab12cd) catch unreachable;
    const entry = reg.get(id) catch unreachable;

    const expected_key: u64 = (@as(u64, 0x00ab12cd) << 32) | @as(u64, id);
    expect_eq(entry.sourceMapKey(), expected_key);

    let buf: [64]u8 = [0; 64];
    const url = entry.formatClientBundleUrl(buf[0..]) catch "";
    expect_eq(url, "/_bun/client/0-00ab12cd.js");

    let tiny: [10]u8 = [0; 10];
    let got_small = false;
    entry.formatClientBundleUrl(tiny[0..]) catch |e| {
        if (e == RouteBundleError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);
});

test("RouteBundleRegistry: boundary guards (InvalidRouteIndex, totalMemoryCost & CapacityExceeded)", fn() {
    let reg = RouteBundleRegistry.init();

    // InvalidRouteIndex on empty registry
    let invalid_idx = false;
    reg.get(0) catch |e| {
        if (e == RouteBundleError.InvalidRouteIndex) invalid_idx = true;
    };
    expect_true(invalid_idx);

    // Fill all 128 slots and check totalMemoryCost
    let i: usize = 0;
    while (i < ROUTE_BUNDLE_MAX_COUNT) {
        const id = reg.registerFramework("/r", 1) catch 9999;
        expect_eq(id, @as(u32, i));
        * = reg.beginBundling(id) catch unreachable;
        * = reg.markLoaded(id, 100, 0, 0, null) catch unreachable;
        i += 1;
    }
    expect_eq(reg.count(), ROUTE_BUNDLE_MAX_COUNT);
    expect_eq(reg.totalMemoryCost(), ROUTE_BUNDLE_MAX_COUNT * 100);

    // 129th registration -> CapacityExceeded
    let cap_err = false;
    reg.registerHtml("/overflow", 1) catch |e| {
        if (e == RouteBundleError.CapacityExceeded) cap_err = true;
    };
    expect_true(cap_err);
});
