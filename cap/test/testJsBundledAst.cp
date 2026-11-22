import { test, assert, assertEq } from "cap:test";
import {
    BAST_FLAG_USES_EXPORTS_REF,
    BAST_FLAG_USES_MODULE_REF,
    BAST_FLAG_FORCE_CJS_TO_ESM,
    BAST_FLAG_HAS_LAZY_EXPORT,
    BAST_FLAG_CJS_EXPORTS_DEOPTIMIZED,
    BAST_FLAG_EXPLICIT_USE_STRICT,
    BAST_FLAG_HAS_IMPORT_META,
    JsBundledAstError,
    JsExportsKind,
    JsTlaStatus,
    JsBundleWrapMode,
    JsBundledAstMeta,
    JsBundledImportEdge,
    propagateBundledTla,
    summarizeBundledGraph,
} from "cap:js";

test("JsBundledAstMeta flat-bundling & strict mode decisions for ESM modules", () => {
    let m = JsBundledAstMeta.empty(0);
    m.exports_kind = JsExportsKind.esm;
    assert(m.isStrictMode());
    assert(m.canFlatBundle());
    assert(m.wrapMode() == JsBundleWrapMode.flat_esm);
    assert(!m.hasTopLevelAwait());
    assert(!m.hasRedirect());

    // ESM module that touches CJS `module` ref cannot be flat-bundled
    m.setFlag(BAST_FLAG_USES_MODULE_REF);
    assert(!m.canFlatBundle());
    assert(m.wrapMode() == JsBundleWrapMode.commonjs_closure);

    m.clearFlag(BAST_FLAG_USES_MODULE_REF);
    assert(m.canFlatBundle());

    // Lazy export disables flat-bundling
    m.setFlag(BAST_FLAG_HAS_LAZY_EXPORT);
    assert(!m.canFlatBundle());
});

test("JsBundledAstMeta CJS-to-ESM unwrapping eligibility and deoptimization guards", () => {
    // Pure named CJS exports (`exports.foo = 1`) -> eligible for unwrapped_cjs
    let cjs_clean = JsBundledAstMeta{
        source_index: 1,
        exports_kind: JsExportsKind.cjs,
        flags: BAST_FLAG_USES_EXPORTS_REF,
        cjs_named_export_count: 3,
    };
    assert(cjs_clean.canUnwrapCjsToEsm());
    assert(cjs_clean.canFlatBundle());
    assert(cjs_clean.wrapMode() == JsBundleWrapMode.unwrapped_cjs);
    assert(!cjs_clean.isStrictMode());

    // Explicit "use strict" enables strict mode on CJS
    cjs_clean.setFlag(BAST_FLAG_EXPLICIT_USE_STRICT);
    assert(cjs_clean.isStrictMode());

    // Deoptimized `module.exports = ...` forces closure wrapper
    let cjs_deopt = JsBundledAstMeta{
        source_index: 2,
        exports_kind: JsExportsKind.cjs,
        flags: BAST_FLAG_CJS_EXPORTS_DEOPTIMIZED,
        cjs_named_export_count: 2,
    };
    assert(!cjs_deopt.canUnwrapCjsToEsm());
    assert(!cjs_deopt.canFlatBundle());
    assert(cjs_deopt.wrapMode() == JsBundleWrapMode.commonjs_closure);

    // Explicit `force_cjs_to_esm` overrides deopt
    cjs_deopt.setFlag(BAST_FLAG_FORCE_CJS_TO_ESM);
    assert(cjs_deopt.canUnwrapCjsToEsm());
    assert(cjs_deopt.wrapMode() == JsBundleWrapMode.unwrapped_cjs);
});

test("propagateBundledTla fixed-point propagation along static edges and dynamic import barrier", () => {
    // Graph: 0 --static--> 1 --static--> 2 (direct TLA)
    //        3 --dynamic-> 2 (dynamic import does NOT propagate TLA)
    let mods: [4]JsBundledAstMeta = [
        JsBundledAstMeta{ source_index: 0, exports_kind: JsExportsKind.esm },
        JsBundledAstMeta{ source_index: 1, exports_kind: JsExportsKind.esm },
        JsBundledAstMeta{ source_index: 2, exports_kind: JsExportsKind.esm, tla_status: JsTlaStatus.direct },
        JsBundledAstMeta{ source_index: 3, exports_kind: JsExportsKind.esm },
    ];
    const edges: [3]JsBundledImportEdge = [
        JsBundledImportEdge{ importer_idx: 0, imported_idx: 1, is_dynamic_import: false },
        JsBundledImportEdge{ importer_idx: 1, imported_idx: 2, is_dynamic_import: false },
        JsBundledImportEdge{ importer_idx: 3, imported_idx: 2, is_dynamic_import: true },
    ];

    const newly = propagateBundledTla(mods[0..], edges[0..]) catch unreachable;
    assertEq(newly, 2);
    assert(mods[0].tla_status == JsTlaStatus.transitive);
    assertEq(mods[0].tla_source_edge, 1);
    assert(mods[1].tla_status == JsTlaStatus.transitive);
    assertEq(mods[1].tla_source_edge, 2);
    assert(mods[2].tla_status == JsTlaStatus.direct);
    assert(mods[3].tla_status == JsTlaStatus.none);
    assert(mods[0].wrapMode() == JsBundleWrapMode.async_esm_wrapper);
    assert(mods[3].wrapMode() == JsBundleWrapMode.flat_esm);
});

test("propagateBundledTla terminates cleanly on cyclic static import graphs", () => {
    // Cycle: 0 <-> 1, and 1 -> 2 (direct TLA)
    let mods: [3]JsBundledAstMeta = [
        JsBundledAstMeta{ source_index: 0, exports_kind: JsExportsKind.esm },
        JsBundledAstMeta{ source_index: 1, exports_kind: JsExportsKind.esm },
        JsBundledAstMeta{ source_index: 2, exports_kind: JsExportsKind.esm, tla_status: JsTlaStatus.direct },
    ];
    const edges: [3]JsBundledImportEdge = [
        JsBundledImportEdge{ importer_idx: 0, imported_idx: 1, is_dynamic_import: false },
        JsBundledImportEdge{ importer_idx: 1, imported_idx: 0, is_dynamic_import: false },
        JsBundledImportEdge{ importer_idx: 1, imported_idx: 2, is_dynamic_import: false },
    ];
    const newly = propagateBundledTla(mods[0..], edges[0..]) catch unreachable;
    assertEq(newly, 2);
    assert(mods[0].hasTopLevelAwait());
    assert(mods[1].hasTopLevelAwait());
});

test("summarizeBundledGraph counts and out-of-bounds edge guard", () => {
    const mods: [4]JsBundledAstMeta = [
        JsBundledAstMeta{ source_index: 0, exports_kind: JsExportsKind.esm, flags: BAST_FLAG_HAS_IMPORT_META },
        JsBundledAstMeta{ source_index: 1, exports_kind: JsExportsKind.cjs, cjs_named_export_count: 2 },
        JsBundledAstMeta{ source_index: 2, exports_kind: JsExportsKind.cjs, flags: BAST_FLAG_USES_MODULE_REF },
        JsBundledAstMeta{ source_index: 3, exports_kind: JsExportsKind.esm, tla_status: JsTlaStatus.direct, flags: BAST_FLAG_HAS_IMPORT_META },
    ];
    const st = summarizeBundledGraph(mods[0..]);
    assertEq(st.total_modules, 4);
    assertEq(st.flat_esm_count, 1);
    assertEq(st.unwrapped_cjs_count, 1);
    assertEq(st.cjs_closure_count, 1);
    assertEq(st.async_tla_count, 1);
    assertEq(st.import_meta_count, 2);

    // Out-of-bounds edge index rejected cleanly
    let mut_mods: [2]JsBundledAstMeta = [
        JsBundledAstMeta.empty(0),
        JsBundledAstMeta.empty(1),
    ];
    const bad_edges: [1]JsBundledImportEdge = [
        JsBundledImportEdge{ importer_idx: 0, imported_idx: 9, is_dynamic_import: false },
    ];
    let oob_err = false;
    * = propagateBundledTla(mut_mods[0..], bad_edges[0..]) catch |e| {
        oob_err = (e == JsBundledAstError.InvalidModuleIndex);
    };
    assert(oob_err);
});
