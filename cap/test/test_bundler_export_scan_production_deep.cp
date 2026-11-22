// Production Deep Test for cap:bundler (exportScan.cp)
// Covers: Step 1 CommonJS/ESM wrap propagation (require, import */default on empty modules,
// export * -> CJS EsmWithDynamicFallback upgrade, top-level await ESM wrap),
// Step 2 export * recursive expansion ("default" exclusion, local shadowing, diamond deduplication,
// multi-source ambiguity detection, cycle safety), and error guards.

import { test, assert, assertEq } from "cap:test";
import {
    ScanExportsKind,
    ScanWrapKind,
    ScanImportKind,
    ScanModuleNode,
    ScanImportEdge,
    ExportScanner,
} from "cap:bundler";

test("exportScan: Step 1 CJS/ESM wrap propagation rules", () => {
    let sc = ExportScanner.init();

    const m_entry = sc.addModule(ScanModuleNode{
        source_index: 0,
        exports_kind: ScanExportsKind.Esm,
        is_entry_point: true,
    }) catch {
        assert(false);
        return;
    };
    const m_empty = sc.addModule(ScanModuleNode{
        source_index: 1,
        exports_kind: ScanExportsKind.None,
    }) catch {
        assert(false);
        return;
    };
    const m_req_target = sc.addModule(ScanModuleNode{
        source_index: 2,
        exports_kind: ScanExportsKind.Esm,
    }) catch {
        assert(false);
        return;
    };
    const m_cjs = sc.addModule(ScanModuleNode{
        source_index: 3,
        exports_kind: ScanExportsKind.Cjs,
    }) catch {
        assert(false);
        return;
    };
    const m_tla = sc.addModule(ScanModuleNode{
        source_index: 4,
        exports_kind: ScanExportsKind.Esm,
        has_top_level_await: true,
    }) catch {
        assert(false);
        return;
    };

    // import * as ns from './empty' promotes empty module to CJS wrap
    * = sc.addImportEdge(ScanImportEdge{
        importer_idx: m_entry,
        target_idx: m_empty,
        kind: ScanImportKind.Stmt,
        has_import_star: true,
    }) catch {
        assert(false);
        return;
    };
    // require('./esm-target') forces CJS wrap on target
    * = sc.addImportEdge(ScanImportEdge{
        importer_idx: m_entry,
        target_idx: m_req_target,
        kind: ScanImportKind.Require,
    }) catch {
        assert(false);
        return;
    };
    // export * from './cjs' upgrades entry to EsmWithDynamicFallback
    * = sc.addImportEdge(ScanImportEdge{
        importer_idx: m_entry,
        target_idx: m_cjs,
        kind: ScanImportKind.Stmt,
        is_export_star: true,
    }) catch {
        assert(false);
        return;
    };

    const wrapped = sc.resolveModuleWraps();
    assertEq(wrapped, 3); // m_empty (Cjs), m_req_target (Cjs), m_tla (Esm)

    if (n_empty := sc.moduleAt(m_empty)) {
        assert(n_empty.exports_kind == ScanExportsKind.Cjs);
        assert(n_empty.wrap == ScanWrapKind.Cjs);
    } else {
        assert(false);
    }
    if (n_req := sc.moduleAt(m_req_target)) {
        assert(n_req.wrap == ScanWrapKind.Cjs);
    } else {
        assert(false);
    }
    if (n_entry := sc.moduleAt(m_entry)) {
        assert(n_entry.exports_kind == ScanExportsKind.EsmWithDynamicFallback);
    } else {
        assert(false);
    }
    if (n_tla := sc.moduleAt(m_tla)) {
        assert(n_tla.wrap == ScanWrapKind.Esm);
    } else {
        assert(false);
    }
});

test("exportScan: export * excludes default and respects local shadowing", () => {
    let sc = ExportScanner.init();

    const m_barrel = sc.addModule(ScanModuleNode{ source_index: 0, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };
    const m_leaf = sc.addModule(ScanModuleNode{ source_index: 1, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };

    * = sc.addLocalExport(m_barrel, "shared") catch {
        assert(false);
        return;
    };
    * = sc.addLocalExport(m_leaf, "shared") catch {
        assert(false);
        return;
    };
    * = sc.addLocalExport(m_leaf, "helper") catch {
        assert(false);
        return;
    };
    * = sc.addLocalExport(m_leaf, "default") catch {
        assert(false);
        return;
    };
    * = sc.addImportEdge(ScanImportEdge{
        importer_idx: m_barrel,
        target_idx: m_leaf,
        kind: ScanImportKind.Stmt,
        is_export_star: true,
    }) catch {
        assert(false);
        return;
    };

    * = sc.resolveExportStars();

    // "default" must NOT be forwarded by export *
    assert(sc.lookupExport(m_barrel, "default") == null);

    // "helper" forwarded from m_leaf
    if (h := sc.lookupExport(m_barrel, "helper")) {
        assertEq(h.origin_module_idx, m_leaf);
        assert(!h.is_local);
        assert(!h.ambiguous);
    } else {
        assert(false);
    }

    // "shared" shadowed by m_barrel's own local export (not ambiguous!)
    if (s := sc.lookupExport(m_barrel, "shared")) {
        assertEq(s.origin_module_idx, m_barrel);
        assert(s.is_local);
        assert(!s.ambiguous);
    } else {
        assert(false);
    }
});

test("exportScan: diamond re-export deduplication vs multi-source ambiguity", () => {
    let sc = ExportScanner.init();

    const m_top = sc.addModule(ScanModuleNode{ source_index: 0, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };
    const m_left = sc.addModule(ScanModuleNode{ source_index: 1, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };
    const m_right = sc.addModule(ScanModuleNode{ source_index: 2, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };
    const m_base = sc.addModule(ScanModuleNode{ source_index: 3, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };

    // Diamond: m_top -> (m_left, m_right) -> m_base exports "common"
    * = sc.addLocalExport(m_base, "common") catch {
        assert(false);
        return;
    };
    // Conflict: both m_left and m_right locally define "clash"
    * = sc.addLocalExport(m_left, "clash") catch {
        assert(false);
        return;
    };
    * = sc.addLocalExport(m_right, "clash") catch {
        assert(false);
        return;
    };

    * = sc.addImportEdge(ScanImportEdge{ importer_idx: m_top, target_idx: m_left, is_export_star: true }) catch {
        assert(false);
        return;
    };
    * = sc.addImportEdge(ScanImportEdge{ importer_idx: m_top, target_idx: m_right, is_export_star: true }) catch {
        assert(false);
        return;
    };
    * = sc.addImportEdge(ScanImportEdge{ importer_idx: m_left, target_idx: m_base, is_export_star: true }) catch {
        assert(false);
        return;
    };
    * = sc.addImportEdge(ScanImportEdge{ importer_idx: m_right, target_idx: m_base, is_export_star: true }) catch {
        assert(false);
        return;
    };

    * = sc.resolveExportStars();

    // Diamond "common" from same origin m_base is unambiguous
    if (c := sc.lookupExport(m_top, "common")) {
        assertEq(c.origin_module_idx, m_base);
        assert(!c.ambiguous);
    } else {
        assert(false);
    }

    // "clash" from two distinct origins must be flagged ambiguous
    if (x := sc.lookupExport(m_top, "clash")) {
        assert(x.ambiguous);
    } else {
        assert(false);
    }
});

test("exportScan: cyclic export * termination safety", () => {
    let sc = ExportScanner.init();

    const m_a = sc.addModule(ScanModuleNode{ source_index: 10, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };
    const m_b = sc.addModule(ScanModuleNode{ source_index: 11, exports_kind: ScanExportsKind.Esm }) catch {
        assert(false);
        return;
    };

    * = sc.addLocalExport(m_a, "fromA") catch {
        assert(false);
        return;
    };
    * = sc.addLocalExport(m_b, "fromB") catch {
        assert(false);
        return;
    };
    * = sc.addImportEdge(ScanImportEdge{ importer_idx: m_a, target_idx: m_b, is_export_star: true }) catch {
        assert(false);
        return;
    };
    * = sc.addImportEdge(ScanImportEdge{ importer_idx: m_b, target_idx: m_a, is_export_star: true }) catch {
        assert(false);
        return;
    };

    * = sc.resolveExportStars();
    assert(sc.lookupExport(m_a, "fromB") != null);
    assert(sc.lookupExport(m_b, "fromA") != null);
});

test("exportScan: duplicate module, unknown module, and empty name guards", () => {
    let sc = ExportScanner.init();
    const m0 = sc.addModule(ScanModuleNode{ source_index: 5 }) catch {
        assert(false);
        return;
    };
    assertEq(m0, 0);

    const dup = sc.addModule(ScanModuleNode{ source_index: 5 }) catch 9999;
    assertEq(dup, 9999);

    assert(!(sc.addLocalExport(99, "x") catch false));
    assert(!(sc.addLocalExport(m0, "") catch false));
    assert(!(sc.addImportEdge(ScanImportEdge{ importer_idx: m0, target_idx: 99 }) catch false));
});
