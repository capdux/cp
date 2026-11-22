import { test, assert, assertEq } from "cap:test";
import {
    JS_RESOLVE_NONE,
    JsSymbolResolveError,
    JsResolveScopeKind,
    JsResolveSymKind,
    hashJsSymbolName,
    JsResolveSymbol,
    JsResolveScope,
    JsResolveMember,
    JsTsNsExport,
    JsSymbolResolver,
} from "cap:js";

test("JsSymbolResolver lexical scope shadowing, use_count, and unbound deduplication", () => {
    let scopes: [8]JsResolveScope = [JsResolveScope.empty(); 8];
    let members: [16]JsResolveMember = [JsResolveMember.empty(); 16];
    let syms: [16]JsResolveSymbol = [JsResolveSymbol.empty(); 16];
    let ts_exp: [4]JsTsNsExport = [JsTsNsExport.empty(); 4];
    let r = JsSymbolResolver.init(scopes[0..], members[0..], syms[0..], ts_exp[0..]) catch unreachable;

    const root = r.moduleScope();
    const outer_x = r.declareSymbol(root, "x", 10) catch unreachable;
    const fn_sc = r.pushScope(root, JsResolveScopeKind.function_scope) catch unreachable;
    const inner_x = r.declareSymbol(fn_sc, "x", 25) catch unreachable;
    const blk_sc = r.pushScope(fn_sc, JsResolveScopeKind.block) catch unreachable;

    // Inner scope resolves shadowed inner_x
    const res1 = r.findSymbol(blk_sc, 40, "x") catch unreachable;
    assertEq(res1.sym_id, inner_x);
    assertEq(res1.declare_loc, 25);
    assert(!res1.is_inside_with_scope);
    assert(!res1.created_unbound);

    // Module scope resolves outer_x
    const res2 = r.findSymbol(root, 50, "x") catch unreachable;
    assertEq(res2.sym_id, outer_x);
    assertEq((r.getSymbol(inner_x) catch unreachable).use_count, 1);
    assertEq((r.getSymbol(outer_x) catch unreachable).use_count, 1);

    // Probe of unknown global does not mutate symbol table
    const before_cnt = r.symbolCount();
    const probe = r.probeSymbol(blk_sc, 60, "console") catch unreachable;
    assertEq(probe.sym_id, JS_RESOLVE_NONE);
    assertEq(r.symbolCount(), before_cnt);

    // findSymbol materializes unbound global once and deduplicates subsequent lookups
    const u1 = r.findSymbol(blk_sc, 60, "console") catch unreachable;
    assert(u1.created_unbound);
    assert((r.getSymbol(u1.sym_id) catch unreachable).kind == JsResolveSymKind.unbound);

    const u2 = r.findSymbol(root, 75, "console") catch unreachable;
    assert(!u2.created_unbound);
    assertEq(u2.sym_id, u1.sym_id);
    assertEq((r.getSymbol(u1.sym_id) catch unreachable).use_count, 2);
});

test("JsSymbolResolver with-scope traversal marks symbols must_not_be_renamed", () => {
    let scopes: [8]JsResolveScope = [JsResolveScope.empty(); 8];
    let members: [16]JsResolveMember = [JsResolveMember.empty(); 16];
    let syms: [16]JsResolveSymbol = [JsResolveSymbol.empty(); 16];
    let ts_exp: [4]JsTsNsExport = [JsTsNsExport.empty(); 4];
    let r = JsSymbolResolver.init(scopes[0..], members[0..], syms[0..], ts_exp[0..]) catch unreachable;

    const root = r.moduleScope();
    const val_id = r.declareSymbol(root, "value", 5) catch unreachable;
    const with_sc = r.pushScope(root, JsResolveScopeKind.with_scope) catch unreachable;
    const inner_blk = r.pushScope(with_sc, JsResolveScopeKind.block) catch unreachable;

    assert(!(r.getSymbol(val_id) catch unreachable).must_not_be_renamed);

    const res = r.findSymbol(inner_blk, 30, "value") catch unreachable;
    assertEq(res.sym_id, val_id);
    assert(res.is_inside_with_scope);
    assert((r.getSymbol(val_id) catch unreachable).must_not_be_renamed);
});

test("JsSymbolResolver class_body forbids referencing arguments", () => {
    let scopes: [8]JsResolveScope = [JsResolveScope.empty(); 8];
    let members: [16]JsResolveMember = [JsResolveMember.empty(); 16];
    let syms: [16]JsResolveSymbol = [JsResolveSymbol.empty(); 16];
    let ts_exp: [4]JsTsNsExport = [JsTsNsExport.empty(); 4];
    let r = JsSymbolResolver.init(scopes[0..], members[0..], syms[0..], ts_exp[0..]) catch unreachable;

    const root = r.moduleScope();
    const cls_sc = r.pushScope(root, JsResolveScopeKind.class_body) catch unreachable;
    const res = r.findSymbol(cls_sc, 100, "arguments") catch unreachable;
    assert(res.forbid_arguments_error);
    assert(res.created_unbound);

    // Non-arguments identifier in class_body does not trigger forbid_arguments_error
    const ok_res = r.probeSymbol(cls_sc, 110, "helper") catch unreachable;
    assert(!ok_res.forbid_arguments_error);
});

test("JsSymbolResolver TypeScript namespace & enum lazy property-access aliasing", () => {
    let scopes: [8]JsResolveScope = [JsResolveScope.empty(); 8];
    let members: [16]JsResolveMember = [JsResolveMember.empty(); 16];
    let syms: [16]JsResolveSymbol = [JsResolveSymbol.empty(); 16];
    let ts_exp: [8]JsTsNsExport = [JsTsNsExport.empty(); 8];
    let r = JsSymbolResolver.init(scopes[0..], members[0..], syms[0..], ts_exp[0..]) catch unreachable;

    const root = r.moduleScope();
    const ns_arg = r.declareSymbol(root, "MyNS", 0) catch unreachable;
    const ns_sc = r.pushScope(root, JsResolveScopeKind.ts_namespace) catch unreachable;
    r.attachTsNamespace(ns_sc, ns_arg, false) catch unreachable;
    r.addTsNamespaceExport(ns_sc, "exportedFn", 12, false) catch unreachable;
    // Enum member export should NOT match inside a non-enum namespace scope
    r.addTsNamespaceExport(ns_sc, "EnumOnly", 20, true) catch unreachable;

    const hit1 = r.findSymbol(ns_sc, 50, "exportedFn") catch unreachable;
    assert(hit1.resolved_via_ts_namespace);
    assertEq(hit1.declare_loc, 12);
    const alias1 = r.getSymbol(hit1.sym_id) catch unreachable;
    assert(alias1.kind == JsResolveSymKind.ts_namespace_alias);
    assertEq(alias1.namespace_ref, ns_arg);
    assertEq(alias1.use_count, 1);

    // Second lookup reuses cached alias symbol instead of allocating a new one
    const sym_cnt_after_first = r.symbolCount();
    const hit2 = r.findSymbol(ns_sc, 60, "exportedFn") catch unreachable;
    assertEq(hit2.sym_id, hit1.sym_id);
    assertEq(r.symbolCount(), sym_cnt_after_first);
    assertEq((r.getSymbol(hit1.sym_id) catch unreachable).use_count, 2);

    // EnumOnly is not matched in non-enum namespace scope -> probe returns NONE
    const miss = r.probeSymbol(ns_sc, 70, "EnumOnly") catch unreachable;
    assertEq(miss.sym_id, JS_RESOLVE_NONE);
});

test("JsSymbolResolver error guards: empty name, invalid scope/symbol, table full, hash stability", () => {
    assertEq(hashJsSymbolName("foo"), hashJsSymbolName("foo"));
    assert(hashJsSymbolName("foo") != hashJsSymbolName("bar"));

    // Zero-capacity scopes rejected
    let zero_sc: [0]JsResolveScope = [];
    let members: [2]JsResolveMember = [JsResolveMember.empty(); 2];
    let syms: [2]JsResolveSymbol = [JsResolveSymbol.empty(); 2];
    let ts_exp: [1]JsTsNsExport = [JsTsNsExport.empty(); 1];
    let init_full = false;
    * = JsSymbolResolver.init(zero_sc[0..], members[0..], syms[0..], ts_exp[0..]) catch |e| {
        init_full = (e == JsSymbolResolveError.TableFull);
    };
    assert(init_full);

    let scopes: [1]JsResolveScope = [JsResolveScope.empty(); 1];
    let r = JsSymbolResolver.init(scopes[0..], members[0..], syms[0..], ts_exp[0..]) catch unreachable;

    // Empty name rejected
    let empty_err = false;
    * = r.declareSymbol(r.moduleScope(), "", 0) catch |e| { empty_err = (e == JsSymbolResolveError.EmptyName); };
    assert(empty_err);

    // Invalid scope rejected
    let bad_sc = false;
    * = r.findSymbol(99, 0, "x") catch |e| { bad_sc = (e == JsSymbolResolveError.InvalidScope); };
    assert(bad_sc);

    // Invalid symbol rejected
    let bad_sym = false;
    * = r.getSymbol(99) catch |e| { bad_sym = (e == JsSymbolResolveError.InvalidSymbol); };
    assert(bad_sym);

    // Scope table full rejected
    let sc_full = false;
    * = r.pushScope(r.moduleScope(), JsResolveScopeKind.block) catch |e| {
        sc_full = (e == JsSymbolResolveError.TableFull);
    };
    assert(sc_full);
});
