// Production Deep Tests for cap:bundler Server Component Boundary Registry (lib/bundler/server_components.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_SCB_BOUNDARIES,
    MAX_SCB_BITSET_BITS,
    ScbError,
    UseDirective,
    ServerComponentBoundary,
    ScbBitSet,
    ServerComponentBoundaryTable,
} from "cap:bundler";

test("bundler scb: UseDirective parsing, quotes, semicolons, and rejections", fn() {
    expect_eq(UseDirective.parse("\"use client\"") == UseDirective.use_client, true);
    expect_eq(UseDirective.parse("  'use client';\n") == UseDirective.use_client, true);
    expect_eq(UseDirective.parse("use client") == UseDirective.use_client, true);
    expect_eq(UseDirective.parse("\"use server\";") == UseDirective.use_server, true);
    expect_eq(UseDirective.parse("'use server'") == UseDirective.use_server, true);

    expect_eq(UseDirective.parse("\"use strict\"") == UseDirective.none, true);
    expect_eq(UseDirective.parse("") == UseDirective.none, true);
    expect_eq(UseDirective.parse("\"use client \"") == UseDirective.none, true);

    expect_eq(UseDirective.use_client.asStr(), "use client");
    expect_eq(UseDirective.use_server.asStr(), "use server");
    expect_eq(UseDirective.none.asStr(), "");
    expect_eq(UseDirective.use_client.isBoundary(), true);
    expect_eq(UseDirective.none.isBoundary(), false);
});

test("bundler scb: register, forward/reverse lookup, and separate SSR graph", fn() {
    let tbl = ServerComponentBoundaryTable.init();
    expect_eq(tbl.isEmpty(), true);

    // Client component with separate SSR graph
    const i0 = tbl.put(10, .use_client, 100, 200) catch unreachable;
    expect_eq(i0, @as(usize, 0));

    // Server action with shared SSR reference
    const i1 = tbl.putSharedSsr(25, .use_server, 125) catch unreachable;
    expect_eq(i1, @as(usize, 1));

    expect_eq(tbl.getIndex(10) ?? 99, @as(usize, 0));
    expect_eq(tbl.getReferenceSourceIndex(10) ?? 0, @as(u32, 100));
    expect_eq(tbl.getSsrSourceIndex(10) ?? 0, @as(u32, 200));

    const b0 = tbl.get(10) ?? unreachable;
    expect_eq(b0.hasSeparateSsr(), true);

    const b1 = tbl.get(25) ?? unreachable;
    expect_eq(b1.hasSeparateSsr(), false);
    expect_eq(tbl.getSsrSourceIndex(25) ?? 0, @as(u32, 125));

    // Reverse lookup by reference_source_index
    const rev = tbl.getByReferenceIndex(100) ?? unreachable;
    expect_eq(rev.source_index, @as(u32, 10));
    expect_eq(tbl.getByReferenceIndex(999) == null, true);
    expect_eq(tbl.get(999) == null, true);
});

test("bundler scb: directive counting for mixed client and server boundaries", fn() {
    let tbl = ServerComponentBoundaryTable.init();
    * = tbl.putSharedSsr(1, .use_client, 101) catch unreachable;
    * = tbl.putSharedSsr(2, .use_client, 102) catch unreachable;
    * = tbl.putSharedSsr(3, .use_server, 103) catch unreachable;
    * = tbl.put(4, .use_client, 104, 204) catch unreachable;

    expect_eq(tbl.len, @as(usize, 4));
    expect_eq(tbl.countByDirective(.use_client), @as(u32, 3));
    expect_eq(tbl.countByDirective(.use_server), @as(u32, 1));
    expect_eq(tbl.countByDirective(.none), @as(u32, 0));
});

test("bundler scb: buildBitSet across 64-bit word boundaries", fn() {
    let tbl = ServerComponentBoundaryTable.init();
    * = tbl.putSharedSsr(0, .use_client, 500) catch unreachable;
    * = tbl.putSharedSsr(63, .use_client, 501) catch unreachable;
    * = tbl.putSharedSsr(64, .use_server, 502) catch unreachable;
    * = tbl.putSharedSsr(1023, .use_server, 503) catch unreachable;

    const bs = tbl.buildBitSet() catch unreachable;
    expect_eq(bs.count(), @as(u32, 4));
    expect_eq(bs.isSet(0), true);
    expect_eq(bs.isSet(63), true);
    expect_eq(bs.isSet(64), true);
    expect_eq(bs.isSet(1023), true);
    expect_eq(bs.isSet(1), false);
    expect_eq(bs.isSet(65), false);
    expect_eq(bs.isSet(1024), false);
});

test("bundler scb: duplicate source_index and bitset out-of-range guards", fn() {
    let tbl = ServerComponentBoundaryTable.init();
    * = tbl.putSharedSsr(42, .use_client, 142) catch unreachable;

    let dup_err = false;
    * = tbl.putSharedSsr(42, .use_server, 242) catch |e| {
        dup_err = (e == ScbError.DuplicateSourceIndex);
        @as(usize, 0)
    };
    expect_eq(dup_err, true);

    // Source index >= MAX_SCB_BITSET_BITS (1024) triggers SourceIndexOutOfRange in buildBitSet
    * = tbl.putSharedSsr(MAX_SCB_BITSET_BITS, .use_client, 2000) catch unreachable;
    let range_err = false;
    * = tbl.buildBitSet() catch |e| {
        range_err = (e == ScbError.SourceIndexOutOfRange);
        ScbBitSet.init()
    };
    expect_eq(range_err, true);
});

test("bundler scb: boundary table capacity limit guard", fn() {
    let tbl = ServerComponentBoundaryTable.init();
    let i: u32 = 0;
    while (i < MAX_SCB_BOUNDARIES) {
        * = tbl.putSharedSsr(i, .use_client, i + 1000) catch unreachable;
        i += 1;
    }
    expect_eq(tbl.len, MAX_SCB_BOUNDARIES);

    let cap_err = false;
    * = tbl.putSharedSsr(9999, .use_server, 19999) catch |e| {
        cap_err = (e == ScbError.BoundaryCapacityExceeded);
        @as(usize, 0)
    };
    expect_eq(cap_err, true);
    expect_eq(ServerComponentBoundary{}.hasSeparateSsr(), false);
});
