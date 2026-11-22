// Origin: Pure-CP Stdlib Test Suite (lib/resolver/importRecord.cp) | Created: 2026-11-22
// Production Deep Verification Suite for ImportRecord, ImportKind & 15-bit Packed ImportRecordFlags:
//   1. ImportKind 12-variant label/errorLabel/fromLabel roundtrip & CJS/CSS/EntryPoint classifiers
//   2. ImportRecordFlags 15-bit toBits/fromBits bit-exact roundtrip & semantic predicates
//   3. ImportRecordTag specifier classification (bun, bun:/node:/cap:, macro:, tailwindcss) & isInternal
//   4. ImportRecord original_path preservation, namespace printing ("ns:path" vs "file") & BufferTooSmall guard
//   5. ImportRecordList tracking, countByKind, countActive, markUnused & countPrunable
//   6. Boundary guards: CapacityExceeded (256 slots) & IndexOutOfBounds

import { test, expect_eq, expect_true } from "cap:test";
import {
    IMPORT_RECORD_INVALID_SOURCE,
    IMPORT_RECORD_MAX_COUNT,
    ImportRecordError,
    ImportKind,
    ImportRecordFlags,
    ImportRecordTag,
    ImportRecordPrintMode,
    ImportRecord,
    ImportRecordList,
} from "cap:resolver";

test("ImportKind: 12-variant label/errorLabel roundtrip & domain classifiers", fn() {
    expect_eq(ImportKind.EntryPointRun.label(), "entry-point-run");
    expect_eq(ImportKind.EntryPointBuild.label(), "entry-point-build");
    expect_eq(ImportKind.Stmt.label(), "import-statement");
    expect_eq(ImportKind.Require.label(), "require-call");
    expect_eq(ImportKind.Dynamic.label(), "dynamic-import");
    expect_eq(ImportKind.RequireResolve.label(), "require-resolve");
    expect_eq(ImportKind.At.label(), "import-rule");
    expect_eq(ImportKind.AtConditional.label(), "import-rule-conditional");
    expect_eq(ImportKind.Url.label(), "url-token");
    expect_eq(ImportKind.Composes.label(), "composes");
    expect_eq(ImportKind.HtmlManifest.label(), "html_manifest");
    expect_eq(ImportKind.Internal.label(), "internal");

    expect_eq(ImportKind.Require.errorLabel(), "require()");
    expect_eq(ImportKind.Dynamic.errorLabel(), "import()");
    expect_eq(ImportKind.At.errorLabel(), "@import");
    expect_eq(ImportKind.HtmlManifest.errorLabel(), "HTML import");

    if (k1 := ImportKind.fromLabel("dynamic-import")) {
        expect_eq(k1, ImportKind.Dynamic);
    } else {
        expect_true(false);
    }
    if (k2 := ImportKind.fromLabel("composes")) {
        expect_eq(k2, ImportKind.Composes);
    } else {
        expect_true(false);
    }
    expect_true(ImportKind.fromLabel("unknown-kind") == null);

    expect_true(ImportKind.Require.isCommonJS());
    expect_true(ImportKind.RequireResolve.isCommonJS());
    expect_true(!ImportKind.Stmt.isCommonJS());

    expect_true(ImportKind.At.isFromCSS());
    expect_true(ImportKind.AtConditional.isFromCSS());
    expect_true(ImportKind.Url.isFromCSS());
    expect_true(ImportKind.Composes.isFromCSS());
    expect_true(!ImportKind.Dynamic.isFromCSS());

    expect_true(ImportKind.EntryPointRun.isEntryPoint());
    expect_true(ImportKind.EntryPointBuild.isEntryPoint());
    expect_true(!ImportKind.Internal.isEntryPoint());
});

test("ImportRecordFlags: 15-bit packed roundtrip & semantic helpers", fn() {
    const empty = ImportRecordFlags{};
    expect_eq(empty.toBits(), 0);
    expect_true(!empty.canSuppressResolveError());
    expect_true(!empty.canPruneWhenUnused());
    expect_true(!empty.needsModuleInteropWrap());

    // All 15 bits set -> 0x7FFF (32767)
    const all_bits: u16 = 0x7FFF;
    const full = ImportRecordFlags.fromBits(all_bits);
    expect_eq(full.toBits(), all_bits);
    expect_true(full.handles_import_errors);
    expect_true(full.is_internal);
    expect_true(full.is_unused);
    expect_true(full.contains_import_star);
    expect_true(full.contains_default_alias);
    expect_true(full.contains_es_module_alias);
    expect_true(full.calls_runtime_re_export_fn);
    expect_true(full.is_inside_try_body);
    expect_true(full.was_originally_bare_import);
    expect_true(full.was_originally_require);
    expect_true(full.was_injected_by_macro);
    expect_true(full.is_external_without_side_effects);
    expect_true(full.print_namespace_in_path);
    expect_true(full.wrap_with_to_esm);
    expect_true(full.wrap_with_to_commonjs);
    expect_true(full.canSuppressResolveError());
    expect_true(full.canPruneWhenUnused());
    expect_true(full.needsModuleInteropWrap());

    // Verify each individual bit 0..14 roundtrips cleanly without aliasing
    let bit: u4 = 0;
    while (bit < 15) {
        const mask: u16 = @as(u16, 1) << bit;
        const unpacked = ImportRecordFlags.fromBits(mask);
        expect_eq(unpacked.toBits(), mask);
        bit += 1;
    }

    // Semantic helper edge cases
    let try_only = ImportRecordFlags{ is_inside_try_body: true };
    expect_true(try_only.canSuppressResolveError());
    let unused_only = ImportRecordFlags{ is_unused: true };
    expect_true(!unused_only.canPruneWhenUnused());
    let wrap_esm = ImportRecordFlags{ wrap_with_to_esm: true };
    expect_true(wrap_esm.needsModuleInteropWrap());
});

test("ImportRecordTag: specifier classification & internal tag checks", fn() {
    expect_eq(ImportRecordTag.fromSpecifier("bun"), ImportRecordTag.Bun);
    expect_eq(ImportRecordTag.fromSpecifier("bun:sqlite"), ImportRecordTag.Builtin);
    expect_eq(ImportRecordTag.fromSpecifier("node:fs"), ImportRecordTag.Builtin);
    expect_eq(ImportRecordTag.fromSpecifier("cap:resolver"), ImportRecordTag.Builtin);
    expect_eq(ImportRecordTag.fromSpecifier("macro:build-info"), ImportRecordTag.Macro);
    expect_eq(ImportRecordTag.fromSpecifier("tailwindcss"), ImportRecordTag.Tailwind);
    expect_eq(ImportRecordTag.fromSpecifier("./local.ts"), ImportRecordTag.None);

    expect_true(ImportRecordTag.Runtime.isRuntime());
    expect_true(!ImportRecordTag.Builtin.isRuntime());

    expect_true(!ImportRecordTag.None.isInternal());
    expect_true(!ImportRecordTag.Bun.isInternal());
    expect_true(!ImportRecordTag.Builtin.isInternal());
    expect_true(ImportRecordTag.Runtime.isInternal());
    expect_true(ImportRecordTag.Macro.isInternal());
    expect_true(ImportRecordTag.BakeResolveToSsrGraph.isInternal());
    expect_true(ImportRecordTag.Tailwind.isInternal());
    expect_eq(@intFromEnum(ImportRecordPrintMode.Css), 2);
});

test("ImportRecord: resolveTo original_path preservation, namespace printing & BufferTooSmall", fn() {
    let rec = ImportRecord.init("./config.yaml", ImportKind.Stmt, 12, 13);
    expect_true(!rec.isResolved());
    expect_eq(rec.source_index, IMPORT_RECORD_INVALID_SOURCE);
    expect_eq(rec.original_path, "./config.yaml");
    expect_true(!rec.shouldPrintNamespace());

    // Resolve with default "file" namespace -> no namespace prefix printed
    rec.resolveTo("/project/config.yaml", "file", 3);
    expect_true(rec.isResolved());
    expect_eq(rec.original_path, "./config.yaml");
    expect_eq(rec.path, "/project/config.yaml");
    expect_true(!rec.shouldPrintNamespace());

    let buf: [64]u8 = [0; 64];
    const p1 = rec.formatPrintablePath(buf[0..]) catch "";
    expect_eq(p1, "/project/config.yaml");

    // Resolve with plugin namespace "yaml" -> prints "yaml:/project/config.yaml"
    rec.resolveTo("/project/config.yaml", "yaml", 4);
    expect_true(rec.shouldPrintNamespace());
    const p2 = rec.formatPrintablePath(buf[0..]) catch "";
    expect_eq(p2, "yaml:/project/config.yaml");

    // BufferTooSmall guard
    let tiny: [8]u8 = [0; 8];
    let got_small = false;
    rec.formatPrintablePath(tiny[0..]) catch |e| {
        if (e == ImportRecordError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);
});

test("ImportRecordList: kind counts, markUnused & prunable tracking", fn() {
    let list = ImportRecordList.init();
    expect_true(list.isEmpty());

    let r0 = ImportRecord.init("./a.ts", ImportKind.Stmt, 0, 6);
    r0.flags.is_external_without_side_effects = true;
    let r1 = ImportRecord.init("node:fs", ImportKind.Require, 10, 7);
    let r2 = ImportRecord.init("./style.css", ImportKind.At, 20, 11);

    const i0 = list.push(r0) catch 999;
    const i1 = list.push(r1) catch 999;
    const i2 = list.push(r2) catch 999;
    expect_eq(i0, 0);
    expect_eq(i1, 1);
    expect_eq(i2, 2);
    expect_eq(list.len(), 3);
    expect_eq(list.countByKind(.Stmt), 1);
    expect_eq(list.countByKind(.Require), 1);
    expect_eq(list.countByKind(.At), 1);
    expect_eq(list.countActive(), 3);
    expect_eq(list.countPrunable(), 0);

    // Marking r0 unused makes it prunable; r1 has side effects so not prunable
    expect_true(list.markUnused(0) catch false);
    expect_true(!(list.markUnused(1) catch true));
    expect_eq(list.countActive(), 1);
    expect_eq(list.countPrunable(), 1);

    const got1 = list.get(1) catch unreachable;
    expect_true(got1.flags.was_originally_require);
    expect_eq(got1.tag, ImportRecordTag.Builtin);

    list.clear();
    expect_true(list.isEmpty());
});

test("ImportRecordList: boundary guards (CapacityExceeded & IndexOutOfBounds)", fn() {
    let list = ImportRecordList.init();

    // Out-of-bounds read on empty list
    let oob_read = false;
    list.get(0) catch |e| {
        if (e == ImportRecordError.IndexOutOfBounds) oob_read = true;
    };
    expect_true(oob_read);

    // Fill to max capacity (256)
    let i: usize = 0;
    while (i < IMPORT_RECORD_MAX_COUNT) {
        const rec = ImportRecord.init("./mod.ts", ImportKind.Stmt, @as(i32, i), 8);
        const idx = list.push(rec) catch 9999;
        expect_eq(idx, i);
        i += 1;
    }
    expect_eq(list.len(), IMPORT_RECORD_MAX_COUNT);

    // 257th push must return CapacityExceeded
    let cap_err = false;
    list.push(ImportRecord.init("./overflow.ts", ImportKind.Stmt, 0, 13)) catch |e| {
        if (e == ImportRecordError.CapacityExceeded) cap_err = true;
    };
    expect_true(cap_err);
});
