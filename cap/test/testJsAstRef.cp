import { test, assert, assertEq } from "cap:test";
import {
    JS_SOURCE_INDEX_INVALID,
    JS_SOURCE_INDEX_RUNTIME,
    JS_AST_REF_MAX_INDEX,
    JsAstRefError,
    JsSourceIndex,
    JsAstRefTag,
    JsAstRef,
    JsSourceTableSpan,
    JsMergedSymbolMap,
} from "cap:js";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("JsSourceIndex lifecycle, sentinel values, and predicates", () => {
    const inv = JsSourceIndex.invalid();
    assert(inv.isInvalid());
    assert(!inv.isValid());
    assertEq(inv.get(), JS_SOURCE_INDEX_INVALID);

    const rt = JsSourceIndex.runtime();
    assert(rt.isValid());
    assert(rt.isRuntime());
    assertEq(rt.get(), JS_SOURCE_INDEX_RUNTIME);

    let idx = JsSourceIndex.fromU32(7);
    assert(idx.isValid());
    assert(!idx.isRuntime());
    assertEq(idx.get(), 7);
    idx.set(42);
    assert(idx.eql(JsSourceIndex.fromU32(42)));
});

test("JsAstRef 64-bit bit-packing, roundtrip, tags, and NONE == 0 invariant", () => {
    const none = JsAstRef.none();
    assert(none.isEmpty());
    assert(!none.isValid());
    assertEq(none.asU64(), 0);
    assert(none.tag() == JsAstRefTag.invalid);

    const r1 = JsAstRef.init(12345, 67, true) catch unreachable;
    assert(r1.isValid());
    assert(!r1.isEmpty());
    assert(r1.isSourceContentsSlice());
    assert(!r1.isAllocatedName());
    assertEq(r1.innerIndex(), 12345);
    assertEq(r1.sourceIndex(), 67);

    // Roundtrip through u64 bits
    const rt = JsAstRef.fromU64(r1.asU64());
    assert(rt.eql(r1));
    assertEq(rt.innerIndex(), 12345);
    assertEq(rt.sourceIndex(), 67);

    // Max u31 boundary values
    const r_max = JsAstRef.initSymbol(JS_AST_REF_MAX_INDEX, JS_AST_REF_MAX_INDEX) catch unreachable;
    assert(r_max.isSymbol());
    assert(r_max.isSourceIndexNull());
    assertEq(r_max.innerIndex(), JS_AST_REF_MAX_INDEX);
    assertEq(r_max.sourceIndex(), JS_AST_REF_MAX_INDEX);
});

test("JsAstRef hashing, deterministic ordering, and Bun-aligned formatting", () => {
    const a = JsAstRef.initSymbol(10, 1) catch unreachable;
    const b = JsAstRef.initSymbol(2, 2) catch unreachable;
    const c = JsAstRef.init(10, 1, false) catch unreachable;

    // Ordering: (src=1, inner=10, tag=allocated_name) < (src=1, inner=10, tag=symbol) < (src=2, inner=2, tag=symbol)
    assertEq(c.compare(a), -1);
    assertEq(a.compare(b), -1);
    assertEq(b.compare(a), 1);
    assertEq(a.compare(a), 0);

    // SplitMix64 hash stability & collision avoidance
    assertEq(a.hash64(), a.hash64());
    assert(a.hash64() != b.hash64());
    assert(a.hash32() != c.hash32());

    // Canonical Ref.format & Ref.jsonStringify
    let buf: [64]u8 = [0; 64];
    const n1 = a.format(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "Ref[inner=10, src=1, .symbol]"));

    const n2 = a.formatJsonPair(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], "[1,10]"));
});

test("JsMergedSymbolMap O(1) 2D parallel-parser symbol resolution", () => {
    let spans: [4]JsSourceTableSpan = [JsSourceTableSpan.empty(); 4];
    let entries: [16]u32 = [0; 16];
    let map = JsMergedSymbolMap.init(spans[0..], entries[0..]);

    // Source 0 has 3 symbols, Source 2 has 5 symbols (sparse registration supported)
    map.registerSource(0, 3, 100) catch unreachable;
    map.registerSource(2, 5, 200) catch unreachable;
    assertEq(map.totalSymbols(), 8);

    const r0 = JsAstRef.initSymbol(2, 0) catch unreachable;
    const r2 = JsAstRef.initSymbol(4, 2) catch unreachable;
    assertEq(map.get(r0) catch unreachable, 100);
    assertEq(map.get(r2) catch unreachable, 200);

    map.set(r0, 999) catch unreachable;
    map.set(r2, 777) catch unreachable;
    assertEq(map.get(r0) catch unreachable, 999);
    assertEq(map.get(r2) catch unreachable, 777);
});

test("JsAstRef & JsMergedSymbolMap boundary and error guards", () => {
    // Out-of-range (> u31 max) rejected
    let range_err = false;
    * = JsAstRef.initSymbol(0x80000000, 0) catch |e| { range_err = (e == JsAstRefError.IndexOutOfRange); };
    assert(range_err);

    let src_range_err = false;
    * = JsAstRef.init(0, 0x80000000, false) catch |e| { src_range_err = (e == JsAstRefError.IndexOutOfRange); };
    assert(src_range_err);

    // Tiny format buffer rejected
    const r = JsAstRef.initSymbol(1, 2) catch unreachable;
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = r.format(tiny[0..]) catch |e| { buf_err = (e == JsAstRefError.BufferTooSmall); };
    assert(buf_err);

    // Merged map guards: invalid ref, unregistered source, inner index overflow, table full
    let spans: [2]JsSourceTableSpan = [JsSourceTableSpan.empty(); 2];
    let entries: [4]u32 = [0; 4];
    let map = JsMergedSymbolMap.init(spans[0..], entries[0..]);
    map.registerSource(0, 2, 0) catch unreachable;

    let inv_err = false;
    * = map.get(JsAstRef.none()) catch |e| { inv_err = (e == JsAstRefError.InvalidRef); };
    assert(inv_err);

    let unreg_err = false;
    * = map.get(JsAstRef.initSymbol(0, 1) catch unreachable) catch |e| { unreg_err = (e == JsAstRefError.SourceOutOfBounds); };
    assert(unreg_err);

    let inner_oob = false;
    * = map.get(JsAstRef.initSymbol(2, 0) catch unreachable) catch |e| { inner_oob = (e == JsAstRefError.IndexOutOfRange); };
    assert(inner_oob);

    let full_err = false;
    * = map.registerSource(1, 5, 0) catch |e| { full_err = (e == JsAstRefError.TableFull); };
    assert(full_err);
});
