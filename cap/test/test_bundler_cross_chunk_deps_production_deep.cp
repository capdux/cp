import { test, expect } from "cap:test";
import {
    CrossChunkAnalyzer,
    CrossChunkImportKind,
} from "cap:bundler";

test("bundler cross_chunk_deps: multi-entry shared chunk static imports, exports, and dedupe", () => {
    let a = CrossChunkAnalyzer.init();
    const entry_a = a.addChunk(true, true, 0, 1) catch 99;
    const entry_b = a.addChunk(true, true, 1, 2) catch 99;
    const shared = a.addChunk(true, false, 0, 3) catch 99; // shared by entry 0 & 1

    const sym_fn = a.addSymbol("formatDate", shared, false, false, null) catch 99;
    // entry_a uses sym_fn twice -> must deduplicate
    * = a.recordSymbolUse(entry_a, sym_fn) catch {};
    * = a.recordSymbolUse(entry_a, sym_fn) catch {};
    * = a.recordSymbolUse(entry_b, sym_fn) catch {};
    // self-use inside shared chunk -> must not create self-import
    * = a.recordSymbolUse(shared, sym_fn) catch {};

    const res = a.compute() catch {
        expect(false).toBe(true);
        return;
    };
    expect(res.import_count).toBe(2);
    expect(res.export_count).toBe(1);
    expect(res.hasImportEdge(entry_a, shared, CrossChunkImportKind.Stmt, sym_fn)).toBe(true);
    expect(res.hasImportEdge(entry_b, shared, CrossChunkImportKind.Stmt, sym_fn)).toBe(true);
    expect(res.isSymbolExportedByChunk(shared, sym_fn)).toBe(true);
});

test("bundler cross_chunk_deps: CJS namespace_alias indirection pulls namespace symbol", () => {
    let a = CrossChunkAnalyzer.init();
    const entry = a.addChunk(true, true, 0, 1) catch 99;
    const cjs_chunk = a.addChunk(true, false, 0, 1) catch 99;

    const ns_sym = a.addSymbol("import_lodash", cjs_chunk, false, false, null) catch 99;
    const named_sym = a.addSymbol("debounce", cjs_chunk, false, false, ns_sym) catch 99;

    * = a.recordSymbolUse(entry, named_sym) catch {};

    const res = a.compute() catch {
        expect(false).toBe(true);
        return;
    };
    expect(res.import_count).toBe(1);
    expect(res.hasImportEdge(entry, cjs_chunk, CrossChunkImportKind.Stmt, ns_sym)).toBe(true);
    expect(res.isSymbolExportedByChunk(cjs_chunk, ns_sym)).toBe(true);
    expect(res.isSymbolExportedByChunk(cjs_chunk, named_sym)).toBe(false);
});

test("bundler cross_chunk_deps: ignores unbound, missing_import, and non-JS CSS chunks", () => {
    let a = CrossChunkAnalyzer.init();
    const entry = a.addChunk(true, true, 0, 1) catch 99;
    const other_js = a.addChunk(true, false, 0, 0) catch 99;
    const css_chunk = a.addChunk(false, false, 0, 1) catch 99;

    const unbound_sym = a.addSymbol("window", other_js, true, false, null) catch 99;
    const missing_sym = a.addSymbol("missingExport", other_js, false, true, null) catch 99;
    const css_sym = a.addSymbol("cssToken", css_chunk, false, false, null) catch 99;

    * = a.recordSymbolUse(entry, unbound_sym) catch {};
    * = a.recordSymbolUse(entry, missing_sym) catch {};
    * = a.recordSymbolUse(entry, css_sym) catch {};
    * = a.recordSymbolUse(css_chunk, unbound_sym) catch {};

    const res = a.compute() catch {
        expect(false).toBe(true);
        return;
    };
    expect(res.import_count).toBe(0);
    expect(res.export_count).toBe(0);
});

test("bundler cross_chunk_deps: entry-point side-effect import injection when no symbol imported", () => {
    let a = CrossChunkAnalyzer.init();
    const entry_0 = a.addChunk(true, true, 0, 1) catch 99;
    const polyfill = a.addChunk(true, false, 0, 1) catch 99; // belongs to entry 0, no symbol uses
    const unrelated = a.addChunk(true, false, 0, 2) catch 99; // belongs to entry 1 only
    * = unrelated;

    const res = a.compute() catch {
        expect(false).toBe(true);
        return;
    };
    expect(res.import_count).toBe(1);
    expect(res.hasImportEdge(entry_0, polyfill, CrossChunkImportKind.SideEffect, null)).toBe(true);
    expect(res.hasAnyStaticImportBetween(entry_0, unrelated)).toBe(false);
});

test("bundler cross_chunk_deps: dynamic cross-chunk imports sorted deterministically and skip self", () => {
    let a = CrossChunkAnalyzer.init();
    const entry = a.addChunk(true, true, 0, 1) catch 99;
    const lazy_b = a.addChunk(true, false, 0, 0) catch 99;
    const lazy_a = a.addChunk(true, false, 0, 0) catch 99;

    // Record out-of-order + duplicate + self dynamic import
    * = a.recordDynamicImport(entry, lazy_a) catch {};
    * = a.recordDynamicImport(entry, lazy_b) catch {};
    * = a.recordDynamicImport(entry, lazy_b) catch {};
    * = a.recordDynamicImport(entry, entry) catch {};

    const res = a.compute() catch {
        expect(false).toBe(true);
        return;
    };
    expect(res.import_count).toBe(2);
    // Sorted by to_chunk ascending: lazy_b (1) before lazy_a (2)
    expect(res.imports[0].to_chunk).toBe(lazy_b);
    expect(res.imports[0].kind == CrossChunkImportKind.Dynamic).toBe(true);
    expect(res.imports[1].to_chunk).toBe(lazy_a);
    expect(res.imports[1].kind == CrossChunkImportKind.Dynamic).toBe(true);
});

test("bundler cross_chunk_deps: invalid chunk and symbol id guards", () => {
    let a = CrossChunkAnalyzer.init();
    const c0 = a.addChunk(true, true, 0, 1) catch 99;

    let bad_chunk_sym = false;
    * = a.addSymbol("x", 99, false, false, null) catch {
        bad_chunk_sym = true;
    };
    expect(bad_chunk_sym).toBe(true);

    let bad_ns_sym = false;
    * = a.addSymbol("x", c0, false, false, 42) catch {
        bad_ns_sym = true;
    };
    expect(bad_ns_sym).toBe(true);

    let bad_use = false;
    a.recordSymbolUse(c0, 5) catch {
        bad_use = true;
    };
    expect(bad_use).toBe(true);

    let bad_dyn = false;
    a.recordDynamicImport(c0, 9) catch {
        bad_dyn = true;
    };
    expect(bad_dyn).toBe(true);
});
