// cap/test/testJsScope.cp
// Production-grade deep tests for cap:js scope.cp
// (Zero-alloc JS/TS Lexical Scope Tree & Symbol Merge Engine)

import { test, assert, assertEq } from "cap:test";
import {
    JS_SCOPE_MAX_SCOPES,
    JS_SCOPE_MAX_MEMBERS,
    JsScopeError,
    JsScopeKind,
    JsStrictModeKind,
    JsSymbolKind,
    JsSymbolMergeResult,
    canMergeJsSymbols,
    JsScopeTree,
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

test("JsScopeKind: stopsHoisting boundary & strict mode predicates", () => {
    assert(!JsScopeKind.block.stopsHoisting());
    assert(!JsScopeKind.with_scope.stopsHoisting());
    assert(!JsScopeKind.class_body.stopsHoisting());
    assert(!JsScopeKind.catch_binding.stopsHoisting());

    assert(JsScopeKind.entry.stopsHoisting());
    assert(JsScopeKind.function_args.stopsHoisting());
    assert(JsScopeKind.function_body.stopsHoisting());
    assert(JsScopeKind.class_static_init.stopsHoisting());

    assert(eqStr(JsScopeKind.function_body.asStr(), "function_body"));
    assert(!JsStrictModeKind.sloppy_mode.isStrict());
    assert(JsStrictModeKind.explicit_strict_mode.isStrict());
    assert(JsStrictModeKind.implicit_strict_mode_class.isStrict());
});

test("canMergeJsSymbols: ECMAScript var, private getter/setter, catch & arguments rules", () => {
    // var + var and var + function in entry/function_body allowed
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.hoisted, JsSymbolKind.hoisted, false) == JsSymbolMergeResult.replace_with_new);
    assert(canMergeJsSymbols(JsScopeKind.function_body, JsSymbolKind.hoisted, JsSymbolKind.hoisted_function, false) == JsSymbolMergeResult.replace_with_new);

    // Block-scoped generator redeclaration is forbidden
    assert(canMergeJsSymbols(JsScopeKind.block, JsSymbolKind.generator_or_async_function, JsSymbolKind.generator_or_async_function, false) == JsSymbolMergeResult.forbidden);

    // Private getter + setter merges into pair (both instance and static)
    assert(canMergeJsSymbols(JsScopeKind.class_body, JsSymbolKind.private_get, JsSymbolKind.private_set, false) == JsSymbolMergeResult.become_private_get_set_pair);
    assert(canMergeJsSymbols(JsScopeKind.class_body, JsSymbolKind.private_static_set, JsSymbolKind.private_static_get, false) == JsSymbolMergeResult.become_private_static_get_set_pair);

    // catch (e) { var e } allowed
    assert(canMergeJsSymbols(JsScopeKind.catch_binding, JsSymbolKind.catch_identifier, JsSymbolKind.hoisted, false) == JsSymbolMergeResult.replace_with_new);

    // function() { var arguments } keeps existing; let arguments overwrites
    assert(canMergeJsSymbols(JsScopeKind.function_body, JsSymbolKind.arguments_sym, JsSymbolKind.hoisted, false) == JsSymbolMergeResult.keep_existing);
    assert(canMergeJsSymbols(JsScopeKind.function_body, JsSymbolKind.arguments_sym, JsSymbolKind.let_sym, false) == JsSymbolMergeResult.overwrite_with_new);

    // let + let and const + var forbidden
    assert(canMergeJsSymbols(JsScopeKind.block, JsSymbolKind.let_sym, JsSymbolKind.let_sym, false) == JsSymbolMergeResult.forbidden);
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.const_sym, JsSymbolKind.hoisted, false) == JsSymbolMergeResult.forbidden);
});

test("canMergeJsSymbols: TypeScript declaration merging vs plain JS", () => {
    // TS: type-only import can collide with class/enum
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.import_sym, JsSymbolKind.class_sym, true) == JsSymbolMergeResult.replace_with_new);
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.import_sym, JsSymbolKind.class_sym, false) == JsSymbolMergeResult.forbidden);

    // TS: enum + enum & namespace + enum
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.ts_enum, JsSymbolKind.ts_enum, true) == JsSymbolMergeResult.replace_with_new);
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.ts_namespace, JsSymbolKind.ts_enum, true) == JsSymbolMergeResult.replace_with_new);

    // TS: class + namespace keeps existing
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.class_sym, JsSymbolKind.ts_namespace, true) == JsSymbolMergeResult.keep_existing);
    assert(canMergeJsSymbols(JsScopeKind.entry, JsSymbolKind.hoisted_function, JsSymbolKind.ts_namespace, true) == JsSymbolMergeResult.keep_existing);
});

test("JsScopeTree: lexical resolution, hoisting target & class_body strict inheritance", () => {
    let tree = JsScopeTree.init(true);
    const entry = tree.pushScope(JsScopeKind.entry, null) catch unreachable;
    const fn_body = tree.pushScope(JsScopeKind.function_body, entry) catch unreachable;
    const inner_blk = tree.pushScope(JsScopeKind.block, fn_body) catch unreachable;
    const cls_body = tree.pushScope(JsScopeKind.class_body, inner_blk) catch unreachable;

    // Class body auto-enables strict mode & forbids arguments
    if (cls := tree.getScope(cls_body)) {
        assert(cls.strict_mode == JsStrictModeKind.implicit_strict_mode_class);
        assert(cls.forbid_arguments);
    } else {
        assert(false);
    }

    // Hoisting target from inner_blk stops at fn_body
    if (hoist := tree.findHoistTargetScope(inner_blk)) {
        assertEq(hoist, fn_body);
    } else {
        assert(false);
    }

    // Symbol declaration + shadowing + lexical resolution
    * = tree.declareSymbol(entry, "x", JsSymbolKind.const_sym, 10, 0) catch unreachable;
    * = tree.declareSymbol(fn_body, "y", JsSymbolKind.let_sym, 20, 12) catch unreachable;
    * = tree.declareSymbol(cls_body, "#secret", JsSymbolKind.private_get, 30, 24) catch unreachable;
    const pair_res = tree.declareSymbol(cls_body, "#secret", JsSymbolKind.private_set, 31, 40) catch unreachable;
    assert(pair_res == JsSymbolMergeResult.become_private_get_set_pair);

    if (m_x := tree.resolveSymbol(cls_body, "x")) {
        assertEq(m_x.ref_id, @as(u32, 10));
    } else {
        assert(false);
    }
    if (m_sec := tree.lookupInScope(cls_body, "#secret")) {
        assert(m_sec.sym_kind == JsSymbolKind.private_get_set_pair);
    } else {
        assert(false);
    }
});

test("JsScopeTree: markDirectEval propagation & recursiveSetStrictMode BFS", () => {
    let tree = JsScopeTree.init(false);
    const entry = tree.pushScope(JsScopeKind.entry, null) catch unreachable;
    const fn_a = tree.pushScope(JsScopeKind.function_body, entry) catch unreachable;
    const blk_a = tree.pushScope(JsScopeKind.block, fn_a) catch unreachable;
    const fn_b = tree.pushScope(JsScopeKind.function_body, entry) catch unreachable;

    assert(tree.canMinifyIdentifiersInScope(entry));
    assert(tree.canMinifyIdentifiersInScope(blk_a));
    assert(tree.canMinifyIdentifiersInScope(fn_b));

    // Direct eval in blk_a taints blk_a, fn_a, and entry, but NOT sibling fn_b
    const marked = tree.markDirectEval(blk_a) catch unreachable;
    assertEq(marked, @as(usize, 3));
    assert(!tree.canMinifyIdentifiersInScope(blk_a));
    assert(!tree.canMinifyIdentifiersInScope(fn_a));
    assert(!tree.canMinifyIdentifiersInScope(entry));
    assert(tree.canMinifyIdentifiersInScope(fn_b));

    // Propagate explicit strict mode from entry down to all sloppy descendants
    const updated = tree.recursiveSetStrictMode(entry, JsStrictModeKind.explicit_strict_mode) catch unreachable;
    assertEq(updated, @as(usize, 4));
    if (s_b := tree.getScope(fn_b)) {
        assert(s_b.strict_mode == JsStrictModeKind.explicit_strict_mode);
    } else {
        assert(false);
    }
    // Second propagation is a no-op since already strict
    const updated2 = tree.recursiveSetStrictMode(entry, JsStrictModeKind.implicit_strict_mode_export) catch unreachable;
    assertEq(updated2, @as(usize, 0));
});

test("JsScopeError: MergeForbidden, InvalidScopeId & capacity guards", () => {
    assertEq(JS_SCOPE_MAX_SCOPES, @as(usize, 64));
    assertEq(JS_SCOPE_MAX_MEMBERS, @as(usize, 256));

    let tree = JsScopeTree.init(false);
    const entry = tree.pushScope(JsScopeKind.entry, null) catch unreachable;
    * = tree.declareSymbol(entry, "dup", JsSymbolKind.let_sym, 1, 0) catch unreachable;

    let forbidden_err = false;
    const r1 = tree.declareSymbol(entry, "dup", JsSymbolKind.let_sym, 2, 10) catch |e| {
        if (e == JsScopeError.MergeForbidden) forbidden_err = true;
        JsSymbolMergeResult.forbidden
    };
    * = r1;
    assert(forbidden_err);

    let invalid_scope_err = false;
    const r2 = tree.pushScope(JsScopeKind.block, 999) catch |e| {
        if (e == JsScopeError.InvalidScopeId) invalid_scope_err = true;
        @as(u32, 0)
    };
    * = r2;
    assert(invalid_scope_err);

    // Fill remaining 63 scopes to hit ScopesFull
    let k: usize = 1;
    while (k < JS_SCOPE_MAX_SCOPES) {
        * = tree.pushScope(JsScopeKind.block, entry) catch unreachable;
        k += 1;
    }
    let full_err = false;
    const r3 = tree.pushScope(JsScopeKind.block, entry) catch |e| {
        if (e == JsScopeError.ScopesFull) full_err = true;
        @as(u32, 0)
    };
    * = r3;
    assert(full_err);
});
