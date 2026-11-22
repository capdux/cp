import { test, expect } from "cap:test";
import {
    HMR_MAX_STMTS,
    HMR_MAX_IMPORTS,
    HMR_MAX_NAMED_ITEMS,
    HmrImportTag,
    HmrImportRecord,
    HmrStmtKind,
    HmrStmtInput,
    HmrPrefixDeclKind,
    HmrConvertOptions,
    HmrBuiltinEvent,
    parseHmrBuiltinEvent,
    hmrBuiltinEventName,
    isDirectImportMetaHotCall,
    HmrChunkStmtConverter,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ESM packed imports and multi-line updateImport formatting", () => {
    let conv = HmrChunkStmtConverter.init();

    const r0 = conv.addImportRecord(HmrImportRecord{ resolved_path: "module_1", tag: .UserModule }) catch unreachable;
    const r1 = conv.addImportRecord(HmrImportRecord{ resolved_path: "module_2", tag: .UserModule }) catch unreachable;
    const r2 = conv.addImportRecord(HmrImportRecord{ resolved_path: "module_3", tag: .UserModule }) catch unreachable;

    let s0 = HmrStmtInput{ kind: .Import, stmt_id: 1, import_record_index: r0, namespace_ident: "module_1" };
    * = s0.addNamedItem("add") catch unreachable;
    * = conv.addStmt(s0) catch unreachable;

    let s1 = HmrStmtInput{ kind: .Import, stmt_id: 2, import_record_index: r1, namespace_ident: "module_2" };
    * = s1.addNamedItem("mul") catch unreachable;
    * = s1.addNamedItem("div") catch unreachable;
    * = conv.addStmt(s1) catch unreachable;

    const s2 = HmrStmtInput{
        kind: .Import,
        stmt_id: 3,
        import_record_index: r2,
        namespace_ident: "module_3",
        has_star_name: true,
    };
    * = conv.addStmt(s2) catch unreachable;

    * = conv.addStmt(HmrStmtInput{
        kind: .Body,
        stmt_id: 4,
        body_code: "console.log(module_1.add(1, module_2.mul(2, 3)));",
    }) catch unreachable;

    const st = conv.convert(HmrConvertOptions{}) catch unreachable;
    expect(st.outside_prefix_count == 3);
    expect(st.inside_prefix_count == 2);
    expect(st.inside_suffix_count == 1);
    expect(st.esm_binding_count == 3);
    expect(st.bare_import_count == 0);

    // 3 slots -> multi-line updateImport (Bun rule: is_single_line = len <= 2)
    const decls = conv.insidePrefixDecls();
    expect(decls.len == 2);
    expect(decls[0].kind == .EsmImportsDestructure);
    expect(decls[1].kind == .EsmUpdateImportArray);
    expect(!decls[1].is_single_line);

    let packed_buf: [256]u8 = [0; 256];
    const packed_n = conv.formatPackedImports(packed_buf[0..]) catch unreachable;
    const expected_packed = "['module_1', 1, \"add\", 'module_2', 2, \"mul\", \"div\", 'module_3', 0]";
    expect(strEq(packed_buf[0..packed_n], expected_packed));

    let prefix_buf: [512]u8 = [0; 512];
    const prefix_n = conv.formatInsideWrapperPrefix(HmrConvertOptions{}, prefix_buf[0..]) catch unreachable;
    const expected_prefix =
        "var [module_1, module_2, module_3] = hmr.imports;\n" ++
        "hmr.updateImport = [\n" ++
        "  (module) => (module_1 = module),\n" ++
        "  (module) => (module_2 = module),\n" ++
        "  (module) => (module_3 = module),\n" ++
        "];\n";
    expect(strEq(prefix_buf[0..prefix_n], expected_prefix));

    let suffix_buf: [256]u8 = [0; 256];
    const suffix_n = conv.formatInsideWrapperSuffix(suffix_buf[0..]) catch unreachable;
    expect(strEq(suffix_buf[0..suffix_n], "console.log(module_1.add(1, module_2.mul(2, 3)));\n"));
});

test("Bare import holes, single-line updateImport, and CSS loader auto-disable", () => {
    let conv = HmrChunkStmtConverter.init();

    const r_css = conv.addImportRecord(HmrImportRecord{
        resolved_path: "./styles.css",
        tag: .UserModule,
        is_css_loader: true,
    }) catch unreachable;
    const r_side = conv.addImportRecord(HmrImportRecord{
        resolved_path: "./polyfill.js",
        tag: .UserModule,
    }) catch unreachable;
    const r_def = conv.addImportRecord(HmrImportRecord{
        resolved_path: "./app.js",
        tag: .UserModule,
    }) catch unreachable;

    * = conv.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r_css }) catch unreachable;
    * = conv.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r_side }) catch unreachable;
    * = conv.addStmt(HmrStmtInput{
        kind: .Import,
        import_record_index: r_def,
        namespace_ident: "app_ns",
        has_default_name: true,
    }) catch unreachable;

    const st = conv.convert(HmrConvertOptions{}) catch unreachable;
    expect(st.css_disabled_count == 1);
    expect(st.outside_prefix_count == 2);
    expect(st.esm_binding_count == 2);
    expect(st.bare_import_count == 1);

    if (css_rec := conv.getImportRecord(r_css)) {
        expect(css_rec.is_disabled);
    } else {
        expect(false);
    }

    // 2 slots -> single-line updateImport
    const decls = conv.insidePrefixDecls();
    expect(decls.len == 2);
    expect(decls[1].is_single_line);

    let packed_buf: [256]u8 = [0; 256];
    const packed_n = conv.formatPackedImports(packed_buf[0..]) catch unreachable;
    expect(strEq(packed_buf[0..packed_n], "['./polyfill.js', 0, './app.js', 1, \"default\"]"));

    let prefix_buf: [512]u8 = [0; 512];
    const prefix_n = conv.formatInsideWrapperPrefix(HmrConvertOptions{}, prefix_buf[0..]) catch unreachable;
    const expected_prefix =
        "var [, app_ns] = hmr.imports;\n" ++
        "hmr.updateImport = [() => {}, (module) => (app_ns = module)];\n";
    expect(strEq(prefix_buf[0..prefix_n], expected_prefix));
});

test("Barrel unused stubs and builtin/bun/runtime rewrites", () => {
    let conv = HmrChunkStmtConverter.init();

    const r_unused_named = conv.addImportRecord(HmrImportRecord{
        resolved_path: "./barrel/dead.js",
        tag: .UserModule,
        is_unused: true,
    }) catch unreachable;
    const r_unused_bare = conv.addImportRecord(HmrImportRecord{
        resolved_path: "./barrel/side.js",
        tag: .UserModule,
        is_unused: true,
    }) catch unreachable;
    const r_builtin = conv.addImportRecord(HmrImportRecord{
        resolved_path: "node:path",
        tag: .Builtin,
    }) catch unreachable;
    const r_bun = conv.addImportRecord(HmrImportRecord{
        resolved_path: "bun:sqlite",
        tag: .Bun,
    }) catch unreachable;
    const r_runtime = conv.addImportRecord(HmrImportRecord{
        resolved_path: "ignored_runtime_path",
        tag: .Runtime,
    }) catch unreachable;
    const r_builtin_bare = conv.addImportRecord(HmrImportRecord{
        resolved_path: "node:diagnostics_channel",
        tag: .Builtin,
    }) catch unreachable;

    let s_u1 = HmrStmtInput{ kind: .Import, import_record_index: r_unused_named, namespace_ident: "dead_ns" };
    * = s_u1.addNamedItem("neverUsed") catch unreachable;
    * = conv.addStmt(s_u1) catch unreachable;
    * = conv.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r_unused_bare }) catch unreachable;

    let s_b1 = HmrStmtInput{ kind: .Import, import_record_index: r_builtin, namespace_ident: "path_ns" };
    * = s_b1.addNamedItem("join") catch unreachable;
    * = conv.addStmt(s_b1) catch unreachable;

    * = conv.addStmt(HmrStmtInput{
        kind: .Import,
        import_record_index: r_bun,
        namespace_ident: "sqlite_ns",
        has_star_name: true,
    }) catch unreachable;

    let s_rt = HmrStmtInput{ kind: .Import, import_record_index: r_runtime, namespace_ident: "rt_ns" };
    * = s_rt.addNamedItem("__esm") catch unreachable;
    * = conv.addStmt(s_rt) catch unreachable;

    // Bare builtin import should be omitted cleanly
    * = conv.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r_builtin_bare }) catch unreachable;

    const st = conv.convert(HmrConvertOptions{ hmr_api_ident: "__hmr" }) catch unreachable;
    expect(st.outside_prefix_count == 0);
    expect(st.unused_stub_count == 1);
    expect(st.builtin_binding_count == 3);
    expect(st.inside_prefix_count == 4);

    let prefix_buf: [512]u8 = [0; 512];
    const prefix_n = conv.formatInsideWrapperPrefix(HmrConvertOptions{ hmr_api_ident: "__hmr" }, prefix_buf[0..]) catch unreachable;
    const expected =
        "var dead_ns = {};\n" ++
        "var path_ns = __hmr.builtin(\"node:path\");\n" ++
        "var sqlite_ns = __hmr.builtin(\"bun:sqlite\");\n" ++
        "var rt_ns = __hmr.require(\"bun:wrap\");\n";
    expect(strEq(prefix_buf[0..prefix_n], expected));
});

test("CommonJS passthrough mode and idempotent re-convert", () => {
    let conv = HmrChunkStmtConverter.init();
    * = conv.addStmt(HmrStmtInput{ kind: .Body, stmt_id: 10, body_code: "const x = require('./a');" }) catch unreachable;
    * = conv.addStmt(HmrStmtInput{ kind: .Body, stmt_id: 11, body_code: "module.exports = x + 1;" }) catch unreachable;

    const st1 = conv.convert(HmrConvertOptions{ is_commonjs: true }) catch unreachable;
    expect(st1.is_commonjs);
    expect(st1.outside_prefix_count == 0);
    expect(st1.inside_prefix_count == 0);
    expect(st1.inside_suffix_count == 2);

    // Second call resets buckets cleanly without duplicating entries
    const st2 = conv.convert(HmrConvertOptions{ is_commonjs: true }) catch unreachable;
    expect(st2.inside_suffix_count == 2);
    expect(conv.insideSuffixIndices().len == 2);

    let out: [256]u8 = [0; 256];
    const n = conv.formatInsideWrapperSuffix(out[0..]) catch unreachable;
    expect(strEq(out[0..n], "const x = require('./a');\nmodule.exports = x + 1;\n"));
});

test("Bun and Vite HMR built-in events and import.meta.hot direct-call contract", () => {
    // All 8 built-in events under both bun:* and vite:* prefixes
    const evs: [8]HmrBuiltinEvent = [
        .BeforeUpdate,
        .AfterUpdate,
        .BeforeFullReload,
        .BeforePrune,
        .Invalidate,
        .Error,
        .WsDisconnect,
        .WsConnect,
    ];
    for (ev of evs) {
        const bun_name = hmrBuiltinEventName(ev, false);
        const vite_name = hmrBuiltinEventName(ev, true);
        if (parsed_bun := parseHmrBuiltinEvent(bun_name)) {
            expect(parsed_bun == ev);
        } else {
            expect(false);
        }
        if (parsed_vite := parseHmrBuiltinEvent(vite_name)) {
            expect(parsed_vite == ev);
        } else {
            expect(false);
        }
    }

    expect(parseHmrBuiltinEvent("custom:beforeUpdate") == null);
    expect(parseHmrBuiltinEvent("bun:unknown") == null);
    expect(parseHmrBuiltinEvent("") == null);

    // Valid direct calls
    expect(isDirectImportMetaHotCall("import.meta.hot.accept()"));
    expect(isDirectImportMetaHotCall("import.meta.hot.dispose(() => {})"));
    expect(isDirectImportMetaHotCall("import.meta.hot.prune(() => {})"));
    expect(isDirectImportMetaHotCall("import.meta.hot.on(\"bun:beforeUpdate\", fn)"));
    expect(isDirectImportMetaHotCall("import.meta.hot.off(\"vite:error\", fn)"));
    expect(isDirectImportMetaHotCall("import.meta.hot.decline()"));
    expect(isDirectImportMetaHotCall("import.meta.hot.data"));
    expect(isDirectImportMetaHotCall("import.meta.hot.data.root"));

    // Invalid indirect or non-call forms
    expect(!isDirectImportMetaHotCall("hot.accept()"));
    expect(!isDirectImportMetaHotCall("meta.hot.accept()"));
    expect(!isDirectImportMetaHotCall("import.meta.hot.dispose"));
    expect(!isDirectImportMetaHotCall("import.meta.hot.unknown()"));
    expect(!isDirectImportMetaHotCall("import.meta.hot."));
});

test("Boundary guards: invalid record index, empty ident/path, buffer too small, capacity limits", () => {
    // 1. Invalid import_record_index
    let c1 = HmrChunkStmtConverter.init();
    * = c1.addStmt(HmrStmtInput{ kind: .Import, import_record_index: 99, namespace_ident: "m" }) catch unreachable;
    let saw_invalid_idx = false;
    * = c1.convert(HmrConvertOptions{}) catch {
        saw_invalid_idx = true;
    };
    expect(saw_invalid_idx);

    // 2. Empty namespace ident for non-bare import
    let c2 = HmrChunkStmtConverter.init();
    const r2 = c2.addImportRecord(HmrImportRecord{ resolved_path: "./x.js", tag: .UserModule }) catch unreachable;
    * = c2.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r2, has_default_name: true }) catch unreachable;
    let saw_empty_ns = false;
    * = c2.convert(HmrConvertOptions{}) catch {
        saw_empty_ns = true;
    };
    expect(saw_empty_ns);

    // 3. Empty resolved_path for user module
    let c3 = HmrChunkStmtConverter.init();
    const r3 = c3.addImportRecord(HmrImportRecord{ resolved_path: "", tag: .UserModule }) catch unreachable;
    * = c3.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r3 }) catch unreachable;
    let saw_empty_path = false;
    * = c3.convert(HmrConvertOptions{}) catch {
        saw_empty_path = true;
    };
    expect(saw_empty_path);

    // 4. BufferTooSmall on packed imports & quote escaping
    let c4 = HmrChunkStmtConverter.init();
    const r4 = c4.addImportRecord(HmrImportRecord{ resolved_path: "a'b\\c", tag: .UserModule }) catch unreachable;
    * = c4.addStmt(HmrStmtInput{ kind: .Import, import_record_index: r4, namespace_ident: "m", has_star_name: true }) catch unreachable;
    * = c4.convert(HmrConvertOptions{}) catch unreachable;

    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = c4.formatPackedImports(tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);

    let esc_buf: [64]u8 = [0; 64];
    const esc_n = c4.formatPackedImports(esc_buf[0..]) catch unreachable;
    expect(strEq(esc_buf[0..esc_n], "['a\\'b\\\\c', 0]"));

    // 5. Capacity limits
    let stmt_cap = HmrStmtInput{ kind: .Import };
    for (_ of 0..HMR_MAX_NAMED_ITEMS) {
        * = stmt_cap.addNamedItem("x") catch unreachable;
    }
    let saw_too_many_named = false;
    * = stmt_cap.addNamedItem("overflow") catch {
        saw_too_many_named = true;
    };
    expect(saw_too_many_named);

    let c5 = HmrChunkStmtConverter.init();
    for (_ of 0..HMR_MAX_IMPORTS) {
        * = c5.addImportRecord(HmrImportRecord{ resolved_path: "a" }) catch unreachable;
    }
    let saw_too_many_imp = false;
    * = c5.addImportRecord(HmrImportRecord{ resolved_path: "b" }) catch {
        saw_too_many_imp = true;
    };
    expect(saw_too_many_imp);

    for (_ of 0..HMR_MAX_STMTS) {
        * = c5.addStmt(HmrStmtInput{ kind: .Body }) catch unreachable;
    }
    let saw_too_many_stmts = false;
    * = c5.addStmt(HmrStmtInput{ kind: .Body }) catch {
        saw_too_many_stmts = true;
    };
    expect(saw_too_many_stmts);
});
