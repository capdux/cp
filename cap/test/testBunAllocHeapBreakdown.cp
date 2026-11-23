import { test, assert, assertEq } from "cap:test";
import {
    HEAP_ZONE_PREFIX,
    HeapBreakdownError,
    heapZoneBaseName,
    formatHeapZoneLabel,
    HeapZoneHandle,
    ZoneStats,
    HeapBreakdown,
} from "cap:mem";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("heapZoneBaseName & formatHeapZoneLabel", () => {
    assert(strEql(HEAP_ZONE_PREFIX, "Bun__"));
    assert(strEql(heapZoneBaseName("bun.ast.ExprNode"), "ExprNode"));
    assert(strEql(heapZoneBaseName("http::SocketBuf"), "SocketBuf"));
    assert(strEql(heapZoneBaseName("PlainZone"), "PlainZone"));

    let buf: [64]u8 = [0; 64];
    const n = formatHeapZoneLabel("ParserArena", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n], "Bun__ParserArena"));

    let empty_err = false;
    * = formatHeapZoneLabel("", buf[0..]) catch |e| {
        empty_err = (e == HeapBreakdownError.EmptyZoneName);
    };
    assert(empty_err);

    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = formatHeapZoneLabel("AST", tiny[0..]) catch |e| {
        buf_err = (e == HeapBreakdownError.BufferTooSmall);
    };
    assert(buf_err);
});

test("HeapBreakdown registration, idempotent lookup, and alloc/free/resize accounting", () => {
    let storage: [8]ZoneStats = [ZoneStats.empty(); 8];
    let hb = HeapBreakdown.init(storage[0..]);
    assertEq(hb.capacity(), 8);
    assertEq(hb.zoneCount(), 0);
    assert(!hb.hasLeaks());
    assert(hb.heaviestLiveZone() == null);

    const z_ast = hb.registerZone("AST") catch unreachable;
    const z_http = hb.getOrRegisterZone("HTTP") catch unreachable;
    const z_http2 = hb.getOrRegisterZone("HTTP") catch unreachable;
    assertEq(z_http.index, z_http2.index);
    assertEq(hb.zoneCount(), 2);

    // Record allocs & verify peak / max_single_alloc / avgAllocBytes
    hb.recordAlloc(z_ast, 1024) catch unreachable;
    hb.recordAlloc(z_ast, 3072) catch unreachable;
    const s1 = hb.stats(z_ast) catch unreachable;
    assertEq(s1.live_bytes, 4096);
    assertEq(s1.peak_bytes, 4096);
    assertEq(s1.alloc_count, 2);
    assertEq(s1.active_allocations, 2);
    assertEq(s1.max_single_alloc, 3072);
    assertEq(s1.avgAllocBytes(), 2048);
    assert(hb.hasLeaks());

    // Resize grow & shrink
    hb.recordResize(z_ast, 1024, 4096) catch unreachable;
    const s2 = hb.stats(z_ast) catch unreachable;
    assertEq(s2.live_bytes, 7168);
    assertEq(s2.peak_bytes, 7168);
    assertEq(s2.resize_count, 1);
    assertEq(s2.max_single_alloc, 4096);

    hb.recordResize(z_ast, 4096, 512) catch unreachable;
    const s3 = hb.stats(z_ast) catch unreachable;
    assertEq(s3.live_bytes, 3584);
    assertEq(s3.peak_bytes, 7168);
    assertEq(s3.resize_count, 2);

    // Free everything -> no leaks in AST zone
    hb.recordFree(z_ast, 512) catch unreachable;
    hb.recordFree(z_ast, 3072) catch unreachable;
    const s4 = hb.stats(z_ast) catch unreachable;
    assertEq(s4.live_bytes, 0);
    assertEq(s4.active_allocations, 0);
    assertEq(s4.free_count, 2);
    assert(!s4.hasLeaks());
    assert(!hb.hasLeaks());
});

test("HeapBreakdown aggregates, heaviestLiveZone, sorting, and formatReport", () => {
    let storage: [4]ZoneStats = [ZoneStats.empty(); 4];
    let hb = HeapBreakdown.init(storage[0..]);

    const z1 = hb.registerZone("Small") catch unreachable;
    const z2 = hb.registerZone("Large") catch unreachable;
    const z3 = hb.registerZone("Medium") catch unreachable;

    hb.recordAlloc(z1, 128) catch unreachable;
    hb.recordAlloc(z2, 8192) catch unreachable;
    hb.recordAlloc(z3, 1024) catch unreachable;

    assertEq(hb.totalLiveBytes(), 9344);
    assertEq(hb.totalPeakBytes(), 9344);
    assertEq(hb.totalAllocatedBytes(), 9344);
    assertEq(hb.totalActiveAllocations(), 3);

    if (top := hb.heaviestLiveZone()) {
        assertEq(top.index, z2.index);
    } else {
        assert(false);
    }

    hb.sortByLiveBytesDesc();
    const first = hb.stats(HeapZoneHandle{ index: 0 }) catch unreachable;
    const second = hb.stats(HeapZoneHandle{ index: 1 }) catch unreachable;
    const third = hb.stats(HeapZoneHandle{ index: 2 }) catch unreachable;
    assert(strEql(first.name, "Large"));
    assert(strEql(second.name, "Medium"));
    assert(strEql(third.name, "Small"));

    let report_buf: [512]u8 = [0; 512];
    const n = hb.formatReport(report_buf[0..]) catch unreachable;
    const expected = "Bun__Large: live=8192 peak=8192 allocs=1 active=1\nBun__Medium: live=1024 peak=1024 allocs=1 active=1\nBun__Small: live=128 peak=128 allocs=1 active=1\n";
    assert(strEql(report_buf[0..n], expected));
});

test("HeapBreakdown resetZone & resetAll preserve registered zone names", () => {
    let storage: [4]ZoneStats = [ZoneStats.empty(); 4];
    let hb = HeapBreakdown.init(storage[0..]);

    const z1 = hb.registerZone("Lexer") catch unreachable;
    const z2 = hb.registerZone("Codegen") catch unreachable;
    hb.recordAlloc(z1, 512) catch unreachable;
    hb.recordAlloc(z2, 2048) catch unreachable;

    hb.resetZone(z1) catch unreachable;
    assertEq((hb.stats(z1) catch unreachable).live_bytes, 0);
    assertEq((hb.stats(z2) catch unreachable).live_bytes, 2048);

    hb.resetAll();
    assertEq(hb.zoneCount(), 2);
    assertEq(hb.totalLiveBytes(), 0);
    assertEq(hb.totalPeakBytes(), 0);
    assert(!hb.hasLeaks());
    assert(hb.findZone("Lexer") != null);
    assert(hb.findZone("Codegen") != null);
});

test("HeapBreakdown error guards: empty, duplicate, full, invalid handle, underflow, tiny report", () => {
    let storage: [2]ZoneStats = [ZoneStats.empty(); 2];
    let hb = HeapBreakdown.init(storage[0..]);

    // Empty name
    let empty_err = false;
    * = hb.registerZone("") catch |e| { empty_err = (e == HeapBreakdownError.EmptyZoneName); };
    assert(empty_err);

    const z0 = hb.registerZone("Z0") catch unreachable;
    // Duplicate name
    let dup_err = false;
    * = hb.registerZone("Z0") catch |e| { dup_err = (e == HeapBreakdownError.DuplicateZone); };
    assert(dup_err);

    * = hb.registerZone("Z1") catch unreachable;
    // Table full
    let full_err = false;
    * = hb.registerZone("Z2") catch |e| { full_err = (e == HeapBreakdownError.ZoneTableFull); };
    assert(full_err);

    // Invalid handle
    let bad_h = false;
    * = hb.recordAlloc(HeapZoneHandle{ index: 99 }, 64) catch |e| {
        bad_h = (e == HeapBreakdownError.InvalidZoneHandle);
    };
    assert(bad_h);

    // Free underflow (active == 0)
    let underflow1 = false;
    * = hb.recordFree(z0, 16) catch |e| { underflow1 = (e == HeapBreakdownError.UnderflowFree); };
    assert(underflow1);

    // Free underflow (bytes > live_bytes)
    hb.recordAlloc(z0, 32) catch unreachable;
    let underflow2 = false;
    * = hb.recordFree(z0, 64) catch |e| { underflow2 = (e == HeapBreakdownError.UnderflowFree); };
    assert(underflow2);

    // Tiny report buffer
    let tiny: [10]u8 = [0; 10];
    let buf_err = false;
    * = hb.formatReport(tiny[0..]) catch |e| { buf_err = (e == HeapBreakdownError.BufferTooSmall); };
    assert(buf_err);
});
