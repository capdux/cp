// Deep production regression tests for cap:bundler chunkStmts.cp
// Covers: 4-region wrapper placement, external import hoisting out of __esm/__commonJS,
// internal import dropping, barrel-optimized unused export * pruning, runtime __reExport injection
// with CJS entry double-write to module.exports, export keyword stripping, export default lowering,
// CJS unwrap-to-assignment lowering, preview formatting, and invalid-record / buffer-too-small error guards.

import { test, expect, expectEq } from "cap:test";
import {
    ChunkWrapKind,
    ChunkOutputFormat,
    ChunkStmtRegion,
    ChunkStmtInputKind,
    ChunkEmittedStmtKind,
    ChunkImportRecordMeta,
    ChunkStmtInput,
    ChunkConvertOptions,
    ChunkStmtConverter,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0 || haystack.len < needle.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEq(haystack[i .. i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("chunkStmts: hoists external ESM imports out of __esm wrapper and drops internal imports", fn () {
    let conv = ChunkStmtConverter.init();
    const ext_rec = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "node:path",
        namespace_name: "import_path",
        is_internal: false,
    }) catch unreachable;
    const int_rec = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "./utils.ts",
        namespace_name: "import_utils",
        is_internal: true,
    }) catch unreachable;

    * = conv.addStmt(ChunkStmtInput{ kind: .Import, import_record_idx: ext_rec }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .Import, import_record_idx: int_rec }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .FunctionDecl, is_export: true, name: "run" }) catch unreachable;

    const st = conv.convert(ChunkConvertOptions{
        wrap: .Esm,
        format: .Esm,
    }) catch unreachable;

    expectEq(st.outside_prefix_count, @as(usize, 1));
    expectEq(st.dropped_count, @as(usize, 1));
    expectEq(st.inside_suffix_count, @as(usize, 1));
    expectEq(st.stripped_export_count, @as(usize, 1));

    const out_pre = conv.regionSlice(.OutsideWrapperPrefix);
    expectEq(out_pre[0].kind, ChunkEmittedStmtKind.ImportStmt);
    expect(strEq(out_pre[0].target, "node:path"));

    let buf: [512]u8 = [0; 512];
    const n = conv.formatPreview(buf[0..], "init_main") catch unreachable;
    const preview = buf[0..n];
    expect(containsStr(preview, "import * as import_path from \"node:path\";\nvar init_main = __esm(() => {\nfunction run() {}\n});\n"));
});

test("chunkStmts: export * barrel pruning, ESM external __reExport, and internal init + reExport", fn () {
    let conv = ChunkStmtConverter.init();
    const unused_rec = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "./barrel_unused.ts",
        is_unused: true,
    }) catch unreachable;
    const ext_star = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "lodash-es",
        namespace_name: "import_lodash",
        is_internal: false,
        calls_runtime_re_export: true,
    }) catch unreachable;
    const int_star = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "./sub.ts",
        is_internal: true,
        target_wrap: .Esm,
        target_wrapper_name: "init_sub",
        target_exports_name: "exports_sub",
        target_is_esm_with_dynamic_fallback: true,
        calls_runtime_re_export: true,
    }) catch unreachable;

    * = conv.addStmt(ChunkStmtInput{ kind: .ExportStar, import_record_idx: unused_rec }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportStar, import_record_idx: ext_star }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportStar, import_record_idx: int_star }) catch unreachable;

    const st = conv.convert(ChunkConvertOptions{
        wrap: .Esm,
        format: .Esm,
        exports_name: "exports_entry",
    }) catch unreachable;

    expectEq(st.outside_prefix_count, @as(usize, 1));
    expectEq(st.re_export_calls, @as(usize, 2));
    expectEq(st.wrapper_init_calls, @as(usize, 1));
    expectEq(st.dropped_count, @as(usize, 2)); // unused barrel + internal export * removed

    const in_pre = conv.regionSlice(.InsideWrapperPrefix);
    expectEq(in_pre.len, @as(usize, 3));
    expectEq(in_pre[0].kind, ChunkEmittedStmtKind.RuntimeReExportCall);
    expect(strEq(in_pre[0].target, "import_lodash"));
    expectEq(in_pre[1].kind, ChunkEmittedStmtKind.WrapperInitCall);
    expect(strEq(in_pre[1].name, "init_sub"));
    expectEq(in_pre[2].kind, ChunkEmittedStmtKind.RuntimeReExportCall);
    expect(strEq(in_pre[2].target, "exports_sub"));
});

test("chunkStmts: CJS entry point double-writes __reExport with module.exports and unwraps CJS locals", fn () {
    let conv = ChunkStmtConverter.init();
    const cjs_dep = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "external-cjs",
        is_internal: false,
        calls_runtime_re_export: true,
    }) catch unreachable;

    * = conv.addStmt(ChunkStmtInput{ kind: .ExportStar, import_record_idx: cjs_dep }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{
        kind: .LocalDecl,
        was_cjs_export: true,
        has_initializer: true,
        name: "answer",
        code: "42",
    }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{
        kind: .LocalDecl,
        was_cjs_export: true,
        has_initializer: false,
        name: "empty_decl",
    }) catch unreachable;

    const st = conv.convert(ChunkConvertOptions{
        wrap: .Cjs,
        format: .Cjs,
        is_entry_point: true,
        unwrap_cjs_to_esm: true,
        exports_name: "exports_pkg",
    }) catch unreachable;

    expectEq(st.re_export_calls, @as(usize, 1));
    expectEq(st.inside_suffix_count, @as(usize, 1));
    expectEq(st.dropped_count, @as(usize, 2)); // export * + uninitialized cjs export

    const in_pre = conv.regionSlice(.InsideWrapperPrefix);
    expect(in_pre[0].uses_module_exports);
    const in_suf = conv.regionSlice(.InsideWrapperSuffix);
    expectEq(in_suf[0].kind, ChunkEmittedStmtKind.CjsExportAssign);

    let buf: [512]u8 = [0; 512];
    const n = conv.formatPreview(buf[0..], "") catch unreachable;
    expect(containsStr(buf[0..n], "__reExport(exports_pkg, external-cjs, module.exports);"));
    expect(containsStr(buf[0..n], "exports.answer = 42;"));
});

test("chunkStmts: lowers export default expr/func/class and strips export clause", fn () {
    let conv = ChunkStmtConverter.init();
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportDefaultExpr, default_name: "app_default", code: "{ ok: true }" }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportDefaultFunc, default_name: "fn_default" }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportDefaultClass, name: "Widget", default_name: "cls_default" }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportClause, name: "foo, bar" }) catch unreachable;

    const st = conv.convert(ChunkConvertOptions{
        wrap: .None,
        format: .Iife,
    }) catch unreachable;

    expectEq(st.dropped_count, @as(usize, 1));
    expectEq(st.stripped_export_count, @as(usize, 4));
    expectEq(st.inside_suffix_count, @as(usize, 3));

    const suf = conv.regionSlice(.InsideWrapperSuffix);
    expectEq(suf[0].kind, ChunkEmittedStmtKind.LocalDeclStmt);
    expect(strEq(suf[0].name, "app_default"));
    expectEq(suf[1].kind, ChunkEmittedStmtKind.FunctionStmt);
    expect(strEq(suf[1].name, "fn_default"));
    expectEq(suf[2].kind, ChunkEmittedStmtKind.ClassStmt);
    expect(strEq(suf[2].name, "Widget"));
});

test("chunkStmts: passthrough non-entry preserves export clause and export * as ns", fn () {
    let conv = ChunkStmtConverter.init();
    const ext = conv.addImportRecord(ChunkImportRecordMeta{
        specifier: "react",
        is_internal: false,
    }) catch unreachable;

    * = conv.addStmt(ChunkStmtInput{ kind: .ExportStarAs, import_record_idx: ext, alias_name: "ReactNS" }) catch unreachable;
    * = conv.addStmt(ChunkStmtInput{ kind: .ExportClause, name: "x" }) catch unreachable;

    const st = conv.convert(ChunkConvertOptions{
        wrap: .Esm,
        format: .Esm,
        is_entry_point: false,
        is_passthrough: true,
    }) catch unreachable;

    expectEq(st.stripped_export_count, @as(usize, 0));
    expectEq(st.outside_prefix_count, @as(usize, 2));
    const pre = conv.regionSlice(.OutsideWrapperPrefix);
    expectEq(pre[0].kind, ChunkEmittedStmtKind.ExportStarStmt);
    expectEq(pre[1].kind, ChunkEmittedStmtKind.ExportClauseStmt);
});

test("chunkStmts: guards against invalid import record index and tiny preview buffer", fn () {
    let conv = ChunkStmtConverter.init();
    * = conv.addStmt(ChunkStmtInput{ kind: .Import, import_record_idx: 99 }) catch unreachable;

    let failed_bad_idx = false;
    * = conv.convert(ChunkConvertOptions{}) catch {
        failed_bad_idx = true;
    };
    expect(failed_bad_idx);

    let conv2 = ChunkStmtConverter.init();
    * = conv2.addStmt(ChunkStmtInput{ kind: .FunctionDecl, name: "longFunctionName" }) catch unreachable;
    * = conv2.convert(ChunkConvertOptions{}) catch unreachable;

    let tiny: [8]u8 = [0; 8];
    let failed_buf = false;
    * = conv2.formatPreview(tiny[0..], "init_mod") catch {
        failed_buf = true;
    };
    expect(failed_buf);
});
