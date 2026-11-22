import { test, expect } from "cap:test";
import {
    AST_BUILDER_MAX_SCOPES,
    AST_BUILDER_MAX_SYMBOLS,
    AstScopeKind,
    AstSymbolKind,
    AstSymRef,
    SyntheticAstBuilder,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("init pre-allocates Entry scope and module/hmr built-in symbols", () => {
    let ab = SyntheticAstBuilder.init(7, true);
    expect(ab.currentScopeKind() == .Entry);
    expect(ab.module_ref.is_valid);
    expect(ab.hmr_api_ref.is_valid);
    expect(ab.module_ref.source_index == 7);
    expect(ab.hmr_api_ref.source_index == 7);
    expect(!ab.module_ref.eql(ab.hmr_api_ref));

    const mod_sym = ab.getSymbol(ab.module_ref) catch unreachable;
    const hmr_sym = ab.getSymbol(ab.hmr_api_ref) catch unreachable;
    expect(strEq(mod_sym.original_name, "module"));
    expect(strEq(hmr_sym.original_name, "hmr"));
    expect(mod_sym.is_top_level);
    expect(hmr_sym.is_top_level);
});

test("Scope stack tracks top-level vs nested symbols and external must_not_be_renamed globals", () => {
    let ab = SyntheticAstBuilder.init(3, false);

    const err_ref = ab.newExternalSymbol("Error") catch unreachable;
    const err_sym = ab.getSymbol(err_ref) catch unreachable;
    expect(err_sym.must_not_be_renamed);
    expect(err_sym.is_top_level);

    const depth1 = ab.pushScope(.FunctionBody) catch unreachable;
    expect(depth1 == 1);
    expect(ab.currentScopeKind() == .FunctionBody);

    const local_ref = ab.newSymbol(.Constant, "tmp") catch unreachable;
    const local_sym = ab.getSymbol(local_ref) catch unreachable;
    expect(!local_sym.is_top_level);
    expect(!local_sym.must_not_be_renamed);

    const depth0 = ab.popScope() catch unreachable;
    expect(depth0 == 0);
    expect(ab.currentScopeKind() == .Entry);

    const summary = ab.finalize() catch unreachable;
    expect(summary.part_count == 2);
    expect(summary.symbol_count == 4); // module, hmr, Error, tmp
    expect(summary.top_level_symbol_count == 3);
    expect(summary.external_symbol_count == 1);

    // finalize marks all generated symbols as used (use_count == 1)
    const after_local = ab.getSymbol(local_ref) catch unreachable;
    expect(after_local.use_count == 1);
});

test("addImportStmt binds HMR namespace_alias only when hot_reloading is enabled and handles bare imports", () => {
    // 1. HMR enabled
    let ab_hmr = SyntheticAstBuilder.init(10, true);
    const items: [2][]const u8 = ["createElement", "Fragment"];
    const s_idx = ab_hmr.addImportStmt("react", "import_react", items[0..]) catch unreachable;

    const bare_items: [0][]const u8 = [];
    * = ab_hmr.addImportStmt("./side-effect.js", "main", bare_items[0..]) catch unreachable;

    expect(ab_hmr.import_records[1].was_originally_bare_import);
    expect(!ab_hmr.import_records[0].was_originally_bare_import);

    const sym0 = ab_hmr.getSymbol(ab_hmr.stmts[s_idx].item_refs[0]) catch unreachable;
    expect(sym0.kind == .Import);
    expect(sym0.has_namespace_alias);
    expect(strEq(sym0.namespace_alias.alias, "createElement"));
    expect(sym0.namespace_alias.import_record_index == 0);

    const sum_hmr = ab_hmr.finalize() catch unreachable;
    expect(sum_hmr.hmr_aliased_import_count == 2);
    expect(sum_hmr.import_record_count == 2);

    // 2. HMR disabled
    let ab_prod = SyntheticAstBuilder.init(11, false);
    * = ab_prod.addImportStmt("react", "import_react", items[0..]) catch unreachable;
    const sum_prod = ab_prod.finalize() catch unreachable;
    expect(sum_prod.hmr_aliased_import_count == 0);
});

test("formatJsPreview emits valid import, export const, and export default statements", () => {
    let ab = SyntheticAstBuilder.init(1, true);
    const items: [1][]const u8 = ["registerClientReference"];
    * = ab.addImportStmt("react-server-dom-webpack/server", "import_rsc", items[0..]) catch unreachable;
    const bare: [0][]const u8 = [];
    * = ab.addImportStmt("./polyfill.js", "poly", bare[0..]) catch unreachable;
    * = ab.addExportConstStmt("Button", "registerClientReference(fn, \"Button\")") catch unreachable;
    * = ab.addExportDefaultStmt("Button") catch unreachable;

    let buf: [512]u8 = [0; 512];
    const n = ab.formatJsPreview(buf[0..]) catch unreachable;
    const expected =
        "import { registerClientReference } from \"react-server-dom-webpack/server\";\n" ++
        "import \"./polyfill.js\";\n" ++
        "export const Button = registerClientReference(fn, \"Button\");\n" ++
        "export default Button;\n";
    expect(strEq(buf[0..n], expected));
});

test("finalize rejects unclosed scopes", () => {
    let ab = SyntheticAstBuilder.init(2, false);
    * = ab.pushScope(.Block) catch unreachable;
    let saw_unclosed = false;
    * = ab.finalize() catch {
        saw_unclosed = true;
    };
    expect(saw_unclosed);
});

test("Boundary guards: root pop, cross-source ref, empty ident/path, capacity, and tiny buffer", () => {
    let ab = SyntheticAstBuilder.init(5, false);

    // Cannot pop root Entry scope
    let saw_root_pop = false;
    * = ab.popScope() catch {
        saw_root_pop = true;
    };
    expect(saw_root_pop);

    // Cross-source symbol ref rejected
    let saw_bad_ref = false;
    * = ab.getSymbol(AstSymRef{ source_index: 999, inner_index: 0, is_valid: true }) catch {
        saw_bad_ref = true;
    };
    expect(saw_bad_ref);

    // Empty identifier / import path rejected
    let saw_empty_id = false;
    * = ab.newSymbol(.Other, "") catch {
        saw_empty_id = true;
    };
    expect(saw_empty_id);

    let saw_empty_path = false;
    * = ab.addImportRecord("", .Stmt) catch {
        saw_empty_path = true;
    };
    expect(saw_empty_path);

    // Scope stack capacity guard
    for (s of 1..AST_BUILDER_MAX_SCOPES) {
        * = s;
        * = ab.pushScope(.Block) catch unreachable;
    }
    let saw_scope_full = false;
    * = ab.pushScope(.Block) catch {
        saw_scope_full = true;
    };
    expect(saw_scope_full);
    for (s of 1..AST_BUILDER_MAX_SCOPES) {
        * = s;
        * = ab.popScope() catch unreachable;
    }

    // Symbol table capacity guard (2 already used by module + hmr)
    for (k of 2..AST_BUILDER_MAX_SYMBOLS) {
        * = k;
        * = ab.newSymbol(.Other, "x") catch unreachable;
    }
    let saw_sym_full = false;
    * = ab.newSymbol(.Other, "overflow") catch {
        saw_sym_full = true;
    };
    expect(saw_sym_full);

    // BufferTooSmall on preview
    let ab_small = SyntheticAstBuilder.init(6, false);
    * = ab_small.addExportDefaultStmt("123") catch unreachable;
    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = ab_small.formatJsPreview(tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);
});
