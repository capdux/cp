// cap/test/testSourceMapPackedMap.cp
// Production-grade deep tests for cap:sourcemap packedMap.cp
// (Zero-alloc DevServer PackedMapArena & SharedPackedMapRef)

import { test, assert, assertEq } from "cap:test";
import {
    PACKED_MAP_MAX_SLOTS,
    PACKED_MAP_ARENA_BYTES,
    PACKED_MAP_ENTRY_OVERHEAD,
    PackedMapError,
    PackedMapEndState,
    SharedPackedMapKind,
    SharedPackedMapRef,
    PackedMapArena,
} from "cap:sourcemap";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SharedPackedMapRef: zero-alloc none, line_count, and take() move semantics", () => {
    const n = SharedPackedMapRef.none();
    assert(n.isNone());
    assert(!n.isLineCount());
    assert(!n.hasMap());
    if (s := n.slotIndex()) {
        * = s;
        assert(false);
    }

    let lc = SharedPackedMapRef.fromLineCount(128);
    assert(lc.isLineCount());
    assert(!lc.hasMap());
    if (lines := lc.lineCount()) {
        assertEq(lines, @as(u32, 128));
    } else {
        assert(false);
    }

    const moved = lc.take();
    assert(lc.isNone());
    assert(moved.isLineCount());
    if (lines2 := moved.lineCount()) {
        assertEq(lines2, @as(u32, 128));
    } else {
        assert(false);
    }
});

test("PackedMapArena: allocPacked stores VLQ, pre-escaped source & EndState", () => {
    let arena = PackedMapArena.init();
    assertEq(arena.totalActiveCount(), @as(usize, 0));
    assertEq(arena.usedArenaBytes(), @as(usize, 0));

    const end = PackedMapEndState{ original_line: 42, original_column: 7 };
    const h = arena.allocPacked("AAAA;AACA,IAAI", "export const a = 1;\\n", end) catch unreachable;
    assert(h.hasMap());
    assertEq(arena.totalActiveCount(), @as(usize, 1));
    assertEq(arena.strongCount(h), @as(u32, 1));
    assertEq(arena.usedArenaBytes(), @as(usize, 14 + 21));

    if (vlq := arena.vlqSlice(h)) {
        assert(eqStr(vlq, "AAAA;AACA,IAAI"));
    } else {
        assert(false);
    }
    if (esc := arena.escapedSourceSlice(h)) {
        assert(eqStr(esc, "export const a = 1;\\n"));
    } else {
        assert(false);
    }
    if (st := arena.endState(h)) {
        assert(st.eql(end));
    } else {
        assert(false);
    }
});

test("PackedMapArena: cloneRef/releaseRef strong counting & amortizedMemoryCost", () => {
    let arena = PackedMapArena.init();

    // 10 bytes VLQ + 30 bytes escaped + 40 overhead = 80 bytes total
    const h1 = arena.allocPacked("0123456789", "012345678901234567890123456789", PackedMapEndState{}) catch unreachable;
    assertEq(arena.memoryCost(h1), @as(usize, 10 + 30 + PACKED_MAP_ENTRY_OVERHEAD));
    assertEq(arena.amortizedMemoryCost(h1), @as(usize, 80));

    const h2 = arena.cloneRef(h1) catch unreachable;
    assertEq(arena.strongCount(h1), @as(u32, 2));
    assertEq(arena.amortizedMemoryCost(h1), @as(usize, 40)); // 80 / 2

    const h3 = arena.cloneRef(h2) catch unreachable;
    const h4 = arena.cloneRef(h3) catch unreachable;
    assertEq(arena.strongCount(h1), @as(u32, 4));
    assertEq(arena.amortizedMemoryCost(h1), @as(usize, 20)); // 80 / 4

    // Release 3 refs -> still alive
    assert(!(arena.releaseRef(h4) catch unreachable));
    assert(!(arena.releaseRef(h3) catch unreachable));
    assert(!(arena.releaseRef(h2) catch unreachable));
    assertEq(arena.strongCount(h1), @as(u32, 1));
    assertEq(arena.totalActiveCount(), @as(usize, 1));

    // Final release frees slot
    assert(arena.releaseRef(h1) catch unreachable);
    assertEq(arena.totalActiveCount(), @as(usize, 0));
    assertEq(arena.strongCount(h1), @as(u32, 0));
    assertEq(arena.memoryCost(h1), @as(usize, 0));

    // Non-map handles have zero memory cost and pass-through clone/release
    const lc = SharedPackedMapRef.fromLineCount(55);
    assertEq(arena.memoryCost(lc), @as(usize, 0));
    const lc2 = arena.cloneRef(lc) catch unreachable;
    assert(lc2.isLineCount());
    assert(!(arena.releaseRef(lc2) catch unreachable));
});

test("PackedMapArena: writeSourcesContentItem lazy JSON formatting", () => {
    let arena = PackedMapArena.init();
    let buf: [64]u8 = [0; 64];

    // .none and .line_count emit JSON null
    const n_none = arena.writeSourcesContentItem(SharedPackedMapRef.none(), buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_none], "null"));
    const n_lc = arena.writeSourcesContentItem(SharedPackedMapRef.fromLineCount(9), buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_lc], "null"));

    // .some emits quoted pre-escaped source
    const h = arena.allocPacked("AAAA", "let x = \\\"ok\\\";", PackedMapEndState{}) catch unreachable;
    const n_some = arena.writeSourcesContentItem(h, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_some], "\"let x = \\\"ok\\\";\""));
});

test("PackedMapArena: slot reuse after release", () => {
    let arena = PackedMapArena.init();
    const h1 = arena.allocPacked("AAAA", "a", PackedMapEndState{ original_line: 1, original_column: 0 }) catch unreachable;
    const slot1 = h1.slotIndex() ?? 999;
    assertEq(slot1, @as(u32, 0));

    assert(arena.releaseRef(h1) catch unreachable);
    assertEq(arena.totalActiveCount(), @as(usize, 0));

    // Next allocation reuses freed slot 0
    const h2 = arena.allocPacked("BBBB", "b", PackedMapEndState{ original_line: 2, original_column: 1 }) catch unreachable;
    const slot2 = h2.slotIndex() ?? 999;
    assertEq(slot2, @as(u32, 0));
    assertEq(arena.totalActiveCount(), @as(usize, 1));
    if (vlq := arena.vlqSlice(h2)) {
        assert(eqStr(vlq, "BBBB"));
    } else {
        assert(false);
    }
});

test("PackedMapError: EmptyVlq, InvalidHandle, BufferTooSmall & ArenaFull guards", () => {
    assertEq(PACKED_MAP_MAX_SLOTS, @as(usize, 64));
    assertEq(PACKED_MAP_ARENA_BYTES, @as(usize, 8192));

    let arena = PackedMapArena.init();

    // Empty VLQ rejected (aligned with Bun newNonEmpty)
    let empty_err = false;
    const r1 = arena.allocPacked("", "src", PackedMapEndState{}) catch |e| {
        if (e == PackedMapError.EmptyVlq) empty_err = true;
        SharedPackedMapRef.none()
    };
    * = r1;
    assert(empty_err);

    // Double-release rejected with InvalidHandle
    const h = arena.allocPacked("AAAA", "src", PackedMapEndState{}) catch unreachable;
    assert(arena.releaseRef(h) catch unreachable);
    let invalid_err = false;
    const r2 = arena.releaseRef(h) catch |e| {
        if (e == PackedMapError.InvalidHandle) invalid_err = true;
        false
    };
    * = r2;
    assert(invalid_err);

    // BufferTooSmall on writeSourcesContentItem
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    const r3 = arena.writeSourcesContentItem(SharedPackedMapRef.none(), tiny[0..]) catch |e| {
        if (e == PackedMapError.BufferTooSmall) buf_err = true;
        @as(usize, 0)
    };
    * = r3;
    assert(buf_err);

    // ArenaFull guard when exceeding 8192 bytes
    let big: [1024]u8 = ['A'; 1024];
    let k: usize = 0;
    while (k < 7) {
        * = arena.allocPacked(big[0..], "", PackedMapEndState{}) catch unreachable;
        k += 1;
    }
    let full_err = false;
    const r4 = arena.allocPacked(big[0..], big[0..], PackedMapEndState{}) catch |e| {
        if (e == PackedMapError.ArenaFull) full_err = true;
        SharedPackedMapRef.none()
    };
    * = r4;
    assert(full_err);
});
