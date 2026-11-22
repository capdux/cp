// cap/test/testJsImportTrim.cp
// Production-grade deep tests for cap:js importTrim.cp
// (ESM / TypeScript Unused Import Trimmer Policy & Decision Engine)

import { test, assert, assertEq } from "cap:test";
import {
    IMPORT_TRIM_MAX_ITEMS,
    TsImportsNotUsedAsValues,
    ImportTrimPolicy,
    ImportBindingUsage,
    ImportStmtInput,
    ImportStmtDecision,
    evaluateImportStmt,
} from "cap:js";

test("ImportTrimPolicy: shouldTrimUnusedImports across TS, Svelte/partial, bundler & verbatim modes", () => {
    const def = ImportTrimPolicy.tsDefault();
    assert(def.shouldTrimUnusedImports());

    // Partial module (Svelte): non-bundling + non-minifying + preserve -> keep unused imports
    const svelte = ImportTrimPolicy{
        is_typescript: true,
        is_bundling: false,
        minify_identifiers: false,
        verbatim_module_syntax: false,
        imports_not_used_as_values: TsImportsNotUsedAsValues.preserve,
    };
    assert(!svelte.shouldTrimUnusedImports());

    // Bundling overrides preserve since full module is visible
    const bundled_preserve = ImportTrimPolicy{
        is_typescript: true,
        is_bundling: true,
        minify_identifiers: false,
        verbatim_module_syntax: false,
        imports_not_used_as_values: TsImportsNotUsedAsValues.preserve,
    };
    assert(bundled_preserve.shouldTrimUnusedImports());

    // verbatimModuleSyntax disables implicit trimming
    const verbatim = ImportTrimPolicy{
        is_typescript: true,
        is_bundling: true,
        minify_identifiers: true,
        verbatim_module_syntax: true,
        imports_not_used_as_values: TsImportsNotUsedAsValues.remove,
    };
    assert(!verbatim.shouldTrimUnusedImports());

    // Plain JS does not trim imports implicitly
    const js_pol = ImportTrimPolicy{ is_typescript: false };
    assert(!js_pol.shouldTrimUnusedImports());
});

test("evaluateImportStmt: macro imports disabled & bare side-effect imports preserved", () => {
    const pol = ImportTrimPolicy.tsDefault();

    const mac = evaluateImportStmt(ImportStmtInput.macroImport(), pol);
    assert(mac.decision == ImportStmtDecision.disable_macro);
    assert(mac.mark_record_unused);

    const se = evaluateImportStmt(ImportStmtInput.sideEffectOnly(), pol);
    assert(se.decision == ImportStmtDecision.keep_side_effect_only);
    assert(!se.mark_record_unused);
});

test("evaluateImportStmt: mixed type-only, unused & runtime-used named bindings", () => {
    let stmt = ImportStmtInput.sideEffectOnly();
    stmt.setDefault(ImportBindingUsage.unused("UnusedDefault"));
    assert(stmt.addNamed(ImportBindingUsage.typeOnly("UserType")));
    assert(stmt.addNamed(ImportBindingUsage.runtimeUsed("fetchUser", 3)));
    assert(stmt.addNamed(ImportBindingUsage.unused("deadHelper")));

    const res = evaluateImportStmt(stmt, ImportTrimPolicy.tsDefault());
    assert(res.decision == ImportStmtDecision.keep_with_bindings);
    assert(!res.keep_default);
    assertEq(res.kept_named_count, @as(usize, 1));
    assertEq(res.kept_named_indices[0], @as(usize, 1)); // "fetchUser"
    assertEq(res.removed_binding_count, @as(usize, 3));
    assert(!res.mark_record_unused);
    assert(!res.is_unused_in_typescript);
});

test("evaluateImportStmt: all-unused TS import omitted vs downgraded under verbatim/preserve", () => {
    let stmt = ImportStmtInput.sideEffectOnly();
    assert(stmt.addNamed(ImportBindingUsage.typeOnly("OnlyType")));

    // Default TS removes statement completely
    const r_omit = evaluateImportStmt(stmt, ImportTrimPolicy.tsDefault());
    assert(r_omit.decision == ImportStmtDecision.omit_completely);
    assert(r_omit.mark_record_unused);
    assert(!r_omit.is_unused_in_typescript); // ts_use_count == 1

    // Under verbatimModuleSyntax, type-only named item is stripped and statement downgrades to side-effect
    const verbatim = ImportTrimPolicy{
        is_typescript: true,
        verbatim_module_syntax: true,
    };
    const r_se = evaluateImportStmt(stmt, verbatim);
    assert(r_se.decision == ImportStmtDecision.keep_side_effect_only);
    assert(!r_se.mark_record_unused);
});

test("evaluateImportStmt: namespace * as ns liveness via has_namespace_prop_access & max item guard", () => {
    assertEq(IMPORT_TRIM_MAX_ITEMS, @as(usize, 32));

    let stmt = ImportStmtInput.sideEffectOnly();
    stmt.setStar(ImportBindingUsage{
        name: "ns",
        runtime_use_count: 0,
        ts_use_count: 0,
        has_namespace_prop_access: true,
        is_type_only: false,
    });

    // Kept alive because bundler detected property access on namespace
    const res = evaluateImportStmt(stmt, ImportTrimPolicy.tsBundler());
    assert(res.decision == ImportStmtDecision.keep_with_bindings);
    assert(res.keep_star);

    // Without namespace prop access, star import is culled
    let stmt2 = ImportStmtInput.sideEffectOnly();
    stmt2.setStar(ImportBindingUsage.unused("ns"));
    const res2 = evaluateImportStmt(stmt2, ImportTrimPolicy.tsBundler());
    assert(res2.decision == ImportStmtDecision.omit_completely);
    assert(!res2.keep_star);

    // Max named items capacity guard
    let full = ImportStmtInput.sideEffectOnly();
    let k: usize = 0;
    while (k < IMPORT_TRIM_MAX_ITEMS) {
        assert(full.addNamed(ImportBindingUsage.runtimeUsed("x", 1)));
        k += 1;
    }
    assert(!full.addNamed(ImportBindingUsage.runtimeUsed("overflow", 1)));
});
