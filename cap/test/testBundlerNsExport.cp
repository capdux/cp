// Deep production regression tests for cap:bundler Step 5 Namespace Exports (lib/bundler/nsExport.cp)

import { test, expect, expectError } from "cap:test";
import {
    NsExportError,
    NsSymbolRef,
    NsExportExprKind,
    NsExportEmitOptions,
    NsExportPlanner,
    isSimpleJsIdent,
} from "cap:bundler";

test("nsExport: descending lexicographical sort and __export JS generation", () => {
    let p = NsExportPlanner.init();
    * = p.addExport("alpha", NsSymbolRef{ source_index: 1, inner_index: 1 }, "alpha_fn", false)?;
    * = p.addExport("zeta", NsSymbolRef{ source_index: 1, inner_index: 2 }, "zeta_val", false)?;
    * = p.addExport("default", NsSymbolRef{ source_index: 1, inner_index: 3 }, "mod_default", false)?;

    const st = p.build();
    expect(st.emitted_count).toBe(3 as usize);
    expect(p.emittedSlice()[0].alias).toEqual("zeta");
    expect(p.emittedSlice()[1].alias).toEqual("default");
    expect(p.emittedSlice()[2].alias).toEqual("alpha");

    let buf: [256]u8 = [0; 256];
    const n = p.formatJsCode(NsExportEmitOptions{
        exports_var_name: "exports_mod",
        needs_exports_variable: true,
    }, buf[0..])?;
    expect(buf[0..n]).toEqual(
        "var exports_mod = {};\n__export(exports_mod, { zeta: () => zeta_val, default: () => mod_default, alpha: () => alpha_fn });\n",
    );
});

test("nsExport: ambiguous export * collision vs diamond re-export resolution", () => {
    let p = NsExportPlanner.init();
    const shared_sym = NsSymbolRef{ source_index: 10, inner_index: 1 };
    const re_a = NsSymbolRef{ source_index: 2, inner_index: 1 };
    const re_b = NsSymbolRef{ source_index: 3, inner_index: 1 };
    const conflict_sym = NsSymbolRef{ source_index: 11, inner_index: 9 };

    // 菱形重导出：re_a 与 re_b 最终均绑定到同一个 shared_sym -> 不算歧义，保留
    * = p.addBinding(re_a, shared_sym, "sharedHelper", false)?;
    * = p.addBinding(re_b, shared_sym, "sharedHelper", false)?;

    const ok_idx = p.addExport("helper", re_a, "re_a_local", false)?;
    * = p.addAmbiguousRef(ok_idx, re_b)?;

    // 真冲突：同一别名指向两个不同最终符号 -> 按 ESM 规范静默剔除
    const bad_idx = p.addExport("clash", re_a, "re_a_local", false)?;
    * = p.addAmbiguousRef(bad_idx, conflict_sym)?;

    const st = p.build();
    expect(st.omitted_ambiguous).toBe(1 as usize);
    expect(st.emitted_count).toBe(1 as usize);
    expect(p.emittedSlice()[0].alias).toEqual("helper");
    expect(p.emittedSlice()[0].expr_name).toEqual("sharedHelper");
    expect(p.emittedSlice()[0].resolved_ref.eql(shared_sym)).toBe(true);
});

test("nsExport: omits unresolved TypeScript type-only re-exports silently", () => {
    let p = NsExportPlanner.init();
    * = p.addExport("UserType", NsSymbolRef{ source_index: 1, inner_index: 1 }, "UserType", true)?;
    * = p.addExport("createUser", NsSymbolRef{ source_index: 1, inner_index: 2 }, "createUser", false)?;

    const st = p.build();
    expect(st.omitted_ts_type).toBe(1 as usize);
    expect(st.emitted_count).toBe(1 as usize);
    expect(p.emittedSlice()[0].alias).toEqual("createUser");
});

test("nsExport: transitive binding chain marks ImportIdent when namespace_alias present", () => {
    let p = NsExportPlanner.init();
    const s1 = NsSymbolRef{ source_index: 1, inner_index: 1 };
    const s2 = NsSymbolRef{ source_index: 2, inner_index: 2 };
    const s3 = NsSymbolRef{ source_index: 3, inner_index: 3 };

    * = p.addBinding(s1, s2, "mid_sym", false)?;
    * = p.addBinding(s2, s3, "cjs_ns.foo", true)?;
    * = p.addExport("foo", s1, "s1_local", false)?;

    const st = p.build();
    expect(st.emitted_count).toBe(1 as usize);
    expect(p.emittedSlice()[0].resolved_ref.eql(s3)).toBe(true);
    expect(p.emittedSlice()[0].expr_name).toEqual("cjs_ns.foo");
    expect(p.emittedSlice()[0].expr_kind == .ImportIdent).toBe(true);
});

test("nsExport: CJS entry point wrapping and quoted non-identifier export aliases", () => {
    expect(isSimpleJsIdent("valid_1$")).toBe(true);
    expect(isSimpleJsIdent("kebab-export")).toBe(false);
    expect(isSimpleJsIdent("123bad")).toBe(false);

    let p = NsExportPlanner.init();
    * = p.addExport("kebab-export", NsSymbolRef{ source_index: 1, inner_index: 1 }, "kebab_val", false)?;
    * = p.build();

    let buf: [256]u8 = [0; 256];
    const n = p.formatJsCode(NsExportEmitOptions{
        exports_var_name: "exports_entry",
        needs_exports_variable: true,
        is_cjs_entry_point: true,
        force_include_cjs_exports: true,
    }, buf[0..])?;
    expect(buf[0..n]).toEqual(
        "var exports_entry = {};\n__export(exports_entry, { \"kebab-export\": () => kebab_val });\nmodule.exports = __toCommonJS(exports_entry);\n",
    );
});

test("nsExport: boundary guards for empty/duplicate alias, invalid index and small buffer", () => {
    let p = NsExportPlanner.init();
    const sym = NsSymbolRef{ source_index: 1, inner_index: 1 };

    expectError(p.addExport("", sym, "x", false), NsExportError.EmptyAlias);
    expectError(p.addExport("a", sym, "", false), NsExportError.EmptySymbolName);
    expectError(p.addBinding(sym, sym, "", false), NsExportError.EmptySymbolName);

    * = p.addExport("a", sym, "x", false)?;
    expectError(p.addExport("a", sym, "y", false), NsExportError.DuplicateAlias);
    expectError(p.addAmbiguousRef(99, sym), NsExportError.InvalidExportIndex);

    * = p.build();
    let tiny: [8]u8 = [0; 8];
    expectError(p.formatJsCode(NsExportEmitOptions{}, tiny[0..]), NsExportError.BufferTooSmall);
    expectError(
        p.formatJsCode(NsExportEmitOptions{ exports_var_name: "" }, tiny[0..]),
        NsExportError.EmptySymbolName,
    );
});
