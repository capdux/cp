// cap/test/testJsSymbolTable.cp
// Production-grade deep tests for cap:js symbolTable.cp
// (Zero-alloc JS/TS Symbol Link Table & Path-Compressing Union-Find Map)

import { test, assert, assertEq } from "cap:test";
import {
    JS_SYMBOL_MAX_ENTRIES,
    JsSymbolTableError,
    JsSlotNamespace,
    JsSymbolEntryKind,
    JsSymbolRef,
    JsSymbolTable,
} from "cap:js";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("JsSymbolEntryKind: isPrivate, isHoisted, isHoistedOrFunction & isFunction predicates", () => {
    assert(JsSymbolEntryKind.private_field.isPrivate());
    assert(JsSymbolEntryKind.private_method.isPrivate());
    assert(JsSymbolEntryKind.private_static_get_set_pair.isPrivate());
    assert(!JsSymbolEntryKind.class_sym.isPrivate());
    assert(!JsSymbolEntryKind.label.isPrivate());

    assert(JsSymbolEntryKind.hoisted.isHoisted());
    assert(JsSymbolEntryKind.hoisted_function.isHoisted());
    assert(!JsSymbolEntryKind.generator_or_async_function.isHoisted());

    assert(JsSymbolEntryKind.generator_or_async_function.isHoistedOrFunction());
    assert(JsSymbolEntryKind.generator_or_async_function.isFunction());
    assert(JsSymbolEntryKind.hoisted_function.isFunction());
    assert(!JsSymbolEntryKind.hoisted.isFunction());
});

test("JsSymbolTable: slotNamespace classification & countCanonicalByNamespace", () => {
    let tbl = JsSymbolTable.init();
    const s_unbound = tbl.addSymbol(0, "window", JsSymbolEntryKind.unbound) catch unreachable;
    const s_var = tbl.addSymbol(0, "count", JsSymbolEntryKind.hoisted) catch unreachable;
    const s_lbl = tbl.addSymbol(0, "outerLoop", JsSymbolEntryKind.label) catch unreachable;
    const s_priv = tbl.addSymbol(0, "#token", JsSymbolEntryKind.private_field) catch unreachable;
    const s_pinned = tbl.addSymbol(0, "evalTarget", JsSymbolEntryKind.other) catch unreachable;
    * = tbl.markMustNotBeRenamed(s_pinned) catch unreachable;

    assert((tbl.get(s_unbound) ?? unreachable).slotNamespace() == JsSlotNamespace.must_not_be_renamed);
    assert((tbl.get(s_var) ?? unreachable).slotNamespace() == JsSlotNamespace.default_ns);
    assert((tbl.get(s_lbl) ?? unreachable).slotNamespace() == JsSlotNamespace.label_ns);
    assert((tbl.get(s_priv) ?? unreachable).slotNamespace() == JsSlotNamespace.private_name);
    assert((tbl.get(s_pinned) ?? unreachable).slotNamespace() == JsSlotNamespace.must_not_be_renamed);

    const counts = tbl.countCanonicalByNamespace();
    assertEq(counts[@as(usize, @intFromEnum(JsSlotNamespace.must_not_be_renamed))], @as(u32, 2));
    assertEq(counts[@as(usize, @intFromEnum(JsSlotNamespace.default_ns))], @as(u32, 1));
    assertEq(counts[@as(usize, @intFromEnum(JsSlotNamespace.label_ns))], @as(u32, 1));
    assertEq(counts[@as(usize, @intFromEnum(JsSlotNamespace.private_name))], @as(u32, 1));
});

test("JsSymbolTable: merge aggregates use_count_estimate & preserves must_not_be_renamed", () => {
    let tbl = JsSymbolTable.init();
    const old_sym = tbl.addSymbol(0, "keepOriginalName", JsSymbolEntryKind.import_sym) catch unreachable;
    const new_sym = tbl.addSymbol(1, "internalAlias", JsSymbolEntryKind.hoisted_function) catch unreachable;

    * = tbl.addUseCount(old_sym, 7) catch unreachable;
    * = tbl.addUseCount(new_sym, 4) catch unreachable;
    * = tbl.markMustNotBeRenamed(old_sym) catch unreachable;

    const merged_root = tbl.merge(old_sym, new_sym) catch unreachable;
    assert(merged_root.eql(new_sym));

    // Resolved symbol carries summed use counts & inherited must_not_be_renamed + original_name
    if (rec := tbl.getResolved(old_sym)) {
        assertEq(rec.use_count_estimate, @as(u32, 11));
        assert(rec.must_not_be_renamed);
        assert(eqStr(rec.original_name, "keepOriginalName"));
        assert(rec.slotNamespace() == JsSlotNamespace.must_not_be_renamed);
    } else {
        assert(false);
    }

    // Self-merge is a safe no-op
    const same = tbl.merge(old_sym, new_sym) catch unreachable;
    assert(same.eql(new_sym));
});

test("JsSymbolTable: multi-hop chain merge, follow & followAll path compression", () => {
    let tbl = JsSymbolTable.init();
    const s0 = tbl.addSymbol(0, "a", JsSymbolEntryKind.import_sym) catch unreachable;
    const s1 = tbl.addSymbol(1, "b", JsSymbolEntryKind.import_sym) catch unreachable;
    const s2 = tbl.addSymbol(2, "c", JsSymbolEntryKind.import_sym) catch unreachable;
    const s3 = tbl.addSymbol(3, "d", JsSymbolEntryKind.hoisted_function) catch unreachable;

    * = tbl.addUseCount(s0, 1) catch unreachable;
    * = tbl.addUseCount(s1, 2) catch unreachable;
    * = tbl.addUseCount(s2, 3) catch unreachable;
    * = tbl.addUseCount(s3, 4) catch unreachable;

    // Build chain s0 -> s1 -> s2 -> s3
    * = tbl.merge(s0, s1) catch unreachable;
    * = tbl.merge(s1, s2) catch unreachable;
    * = tbl.merge(s2, s3) catch unreachable;

    // s0 still points to s1 until followAll compresses it directly to s3
    const compressed = tbl.followAll();
    assert(compressed >= 1);

    assert(tbl.follow(s0).eql(s3));
    assert(tbl.follow(s1).eql(s3));
    assert(tbl.follow(s2).eql(s3));

    if (root_rec := tbl.getResolved(s0)) {
        assertEq(root_rec.use_count_estimate, @as(u32, 10));
    } else {
        assert(false);
    }

    // Only 1 canonical unlinked root remains across all 4 entries
    const counts = tbl.countCanonicalByNamespace();
    assertEq(counts[@as(usize, @intFromEnum(JsSlotNamespace.default_ns))], @as(u32, 1));
});

test("JsSymbolTable: assignChunkIndex follows link to canonical root", () => {
    let tbl = JsSymbolTable.init();
    const imp = tbl.addSymbol(0, "x", JsSymbolEntryKind.import_sym) catch unreachable;
    const def = tbl.addSymbol(1, "x", JsSymbolEntryKind.constant) catch unreachable;
    * = tbl.merge(imp, def) catch unreachable;

    // Assigning chunk via imported ref updates canonical root
    * = tbl.assignChunkIndex(imp, 5) catch unreachable;
    if (rec := tbl.get(def)) {
        if (ci := rec.chunkIndex()) {
            assertEq(ci, @as(u32, 5));
        } else {
            assert(false);
        }
    } else {
        assert(false);
    }
});

test("JsSymbolTableError: InvalidRef & TableFull capacity guards", () => {
    assertEq(JS_SYMBOL_MAX_ENTRIES, @as(usize, 256));
    let tbl = JsSymbolTable.init();

    let invalid_err = false;
    const r1 = tbl.merge(JsSymbolRef.none(), JsSymbolRef.init(0, 999)) catch |e| {
        if (e == JsSymbolTableError.InvalidRef) invalid_err = true;
        JsSymbolRef.none()
    };
    * = r1;
    assert(invalid_err);

    let k: usize = 0;
    while (k < JS_SYMBOL_MAX_ENTRIES) {
        * = tbl.addSymbol(0, "s", JsSymbolEntryKind.other) catch unreachable;
        k += 1;
    }
    assertEq(tbl.len(), JS_SYMBOL_MAX_ENTRIES);

    let full_err = false;
    const r2 = tbl.addSymbol(0, "overflow", JsSymbolEntryKind.other) catch |e| {
        if (e == JsSymbolTableError.TableFull) full_err = true;
        JsSymbolRef.none()
    };
    * = r2;
    assert(full_err);
});
