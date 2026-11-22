// Source: Absorbed from /data/dev/bun/src/js_parser/ast/BundledAst.cp | Synced: 2026-11-20T11:55:58Z
// ============================================================================
// lib/js/bundledAst.cp — Zero-Alloc BundledAst Module Flags, ExportsKind,
// Flat-Bundling / CJS-Unwrap Eligibility & Fixed-Point TLA Propagation
// ============================================================================
// Absorbed from Bun `js_parser/ast/BundledAst.cp`. Captures the slimmer
// bundler-phase AST metadata (`BundledAstFlags`, `ExportsKind`, `TlaCheck`,
// `BundledModuleTarget`), flat-bundling vs closure-wrapping decisions, and
// cycle-safe fixed-point Top-Level Await (TLA) propagation over caller-owned
// external module/edge slices with zero dynamic allocation.

pub const BAST_FLAG_NONE: u16 = 0x0000;
pub const BAST_FLAG_USES_EXPORTS_REF: u16 = 0x0001;
pub const BAST_FLAG_USES_MODULE_REF: u16 = 0x0002;
pub const BAST_FLAG_USES_EXPORT_KEYWORD: u16 = 0x0004;
pub const BAST_FLAG_HAS_CHAR_FREQ: u16 = 0x0008;
pub const BAST_FLAG_FORCE_CJS_TO_ESM: u16 = 0x0010;
pub const BAST_FLAG_HAS_LAZY_EXPORT: u16 = 0x0020;
pub const BAST_FLAG_CJS_EXPORTS_DEOPTIMIZED: u16 = 0x0040;
pub const BAST_FLAG_EXPLICIT_USE_STRICT: u16 = 0x0080;
pub const BAST_FLAG_HAS_IMPORT_META: u16 = 0x0100;

pub const BAST_NO_REDIRECT: u32 = 0xFFFFFFFF;

pub err JsBundledAstError {
    InvalidModuleIndex,
    BufferTooSmall,
}

/// Module export format classification (`ast.ExportsKind`).
pub enum JsExportsKind: u8 {
    none = 0,
    cjs = 1,
    esm = 2,
    esm_with_dynamic_fallback = 3,
    cjs_with_esm_interop = 4,

    fn isEsm(): bool {
        return _ == JsExportsKind.esm || _ == JsExportsKind.esm_with_dynamic_fallback;
    }

    fn isCjs(): bool {
        return _ == JsExportsKind.cjs || _ == JsExportsKind.cjs_with_esm_interop;
    }

    fn asStr(): []const u8 {
        if (_ == JsExportsKind.none) return "none";
        if (_ == JsExportsKind.cjs) return "cjs";
        if (_ == JsExportsKind.esm) return "esm";
        if (_ == JsExportsKind.esm_with_dynamic_fallback) return "esm_with_dynamic_fallback";
        return "cjs_with_esm_interop";
    }
}

/// Top-Level Await (TLA) status of a bundled module (`ast.TlaCheck`).
pub enum JsTlaStatus: u8 {
    none = 0,
    direct = 1,
    transitive = 2,

    fn isAsync(): bool {
        return _ != JsTlaStatus.none;
    }
}

/// Bundler target environment for a parsed module (`options.Target`).
pub enum JsBundledTarget: u8 {
    browser = 0,
    bun = 1,
    node = 2,
    bun_macro = 3,
    bake_server = 4,
}

/// Bundler wrapping mode selected for a module.
pub enum JsBundleWrapMode: u8 {
    flat_esm = 0,
    unwrapped_cjs = 1,
    commonjs_closure = 2,
    async_esm_wrapper = 3,
}

/// Slimmer bundler-phase AST metadata record (mirrors Bun `BundledAst`).
pub struct JsBundledAstMeta {
    source_index: u32 = 0,
    approximate_newline_count: u32 = 0,
    exports_kind: JsExportsKind = JsExportsKind.none,
    target: JsBundledTarget = JsBundledTarget.browser,
    flags: u16 = BAST_FLAG_NONE,
    tla_status: JsTlaStatus = JsTlaStatus.none,
    tla_source_edge: u32 = BAST_NO_REDIRECT,
    import_record_count: u32 = 0,
    named_export_count: u32 = 0,
    cjs_named_export_count: u32 = 0,
    redirect_import_record_index: u32 = BAST_NO_REDIRECT,

    static fn empty(source_index: u32): JsBundledAstMeta {
        return JsBundledAstMeta{ source_index: source_index };
    }

    fn hasFlag(mask: u16): bool {
        return (_.flags & mask) != 0;
    }

    fn setFlag(*_, mask: u16) {
        _.flags |= mask;
    }

    fn clearFlag(*_, mask: u16) {
        _.flags &= ~mask;
    }

    fn hasTopLevelAwait(): bool {
        return _.tla_status.isAsync();
    }

    fn hasRedirect(): bool {
        return _.redirect_import_record_index != BAST_NO_REDIRECT;
    }

    fn isStrictMode(): bool {
        return _.hasFlag(BAST_FLAG_EXPLICIT_USE_STRICT) || _.exports_kind.isEsm();
    }

    /// True if a CommonJS module can be safely unwrapped into flat ESM exports
    /// (mirrors Bun `unwrap_commonjs_to_esm` + `BundledAstFlags`).
    fn canUnwrapCjsToEsm(): bool {
        if (!_.exports_kind.isCjs()) return false;
        if (_.hasFlag(BAST_FLAG_FORCE_CJS_TO_ESM)) return true;
        if (_.hasFlag(BAST_FLAG_CJS_EXPORTS_DEOPTIMIZED)) return false;
        if (_.hasFlag(BAST_FLAG_USES_MODULE_REF)) return false;
        if (_.hasFlag(BAST_FLAG_HAS_LAZY_EXPORT)) return false;
        return _.cjs_named_export_count > 0;
    }

    /// True if this module is eligible for flat bundling without a `__commonJS` closure wrapper.
    fn canFlatBundle(): bool {
        if (_.hasFlag(BAST_FLAG_HAS_LAZY_EXPORT)) return false;
        if (_.exports_kind.isCjs()) {
            return _.canUnwrapCjsToEsm();
        }
        if (_.hasFlag(BAST_FLAG_USES_EXPORTS_REF) || _.hasFlag(BAST_FLAG_USES_MODULE_REF)) {
            return false;
        }
        if (_.hasFlag(BAST_FLAG_CJS_EXPORTS_DEOPTIMIZED)) {
            return false;
        }
        return true;
    }

    /// Selects the bundler output wrapper mode for this module.
    fn wrapMode(): JsBundleWrapMode {
        if (_.hasTopLevelAwait()) {
            return JsBundleWrapMode.async_esm_wrapper;
        }
        if (_.exports_kind.isCjs()) {
            if (_.canUnwrapCjsToEsm()) {
                return JsBundleWrapMode.unwrapped_cjs;
            }
            return JsBundleWrapMode.commonjs_closure;
        }
        if (!_.canFlatBundle()) {
            return JsBundleWrapMode.commonjs_closure;
        }
        return JsBundleWrapMode.flat_esm;
    }
}

/// Directed static import edge `importer -> imported` for TLA propagation.
pub struct JsBundledImportEdge {
    importer_idx: u32 = 0,
    imported_idx: u32 = 0,
    is_dynamic_import: bool = false,
}

/// Aggregate statistics over a slice of bundled modules.
pub struct JsBundledGraphStats {
    total_modules: usize = 0,
    flat_esm_count: usize = 0,
    unwrapped_cjs_count: usize = 0,
    cjs_closure_count: usize = 0,
    async_tla_count: usize = 0,
    import_meta_count: usize = 0,
}

/// Propagates Top-Level Await transitively along static import edges using
/// cycle-safe fixed-point iteration over caller-owned slices.
/// Dynamic `import()` edges do NOT propagate TLA to the importer.
/// Returns the number of modules newly marked `.transitive`.
pub fn propagateBundledTla(
    modules: []JsBundledAstMeta,
    edges: []const JsBundledImportEdge,
): JsBundledAstError!usize {
    // Validate all edge endpoints upfront
    let e_idx: usize = 0;
    while (e_idx < edges.len) {
        const edge = edges[e_idx];
        if (@as(usize, edge.importer_idx) >= modules.len || @as(usize, edge.imported_idx) >= modules.len) {
            return JsBundledAstError.InvalidModuleIndex;
        }
        e_idx += 1;
    }

    let newly_marked: usize = 0;
    let changed = true;
    while (changed) {
        changed = false;
        let i: usize = 0;
        while (i < edges.len) {
            const edge = edges[i];
            if (!edge.is_dynamic_import) {
                const importer = @as(usize, edge.importer_idx);
                const imported = @as(usize, edge.imported_idx);
                if (modules[imported].hasTopLevelAwait() && !modules[importer].hasTopLevelAwait()) {
                    let updated = modules[importer];
                    updated.tla_status = JsTlaStatus.transitive;
                    updated.tla_source_edge = edge.imported_idx;
                    modules[importer] = updated;
                    newly_marked += 1;
                    changed = true;
                }
            }
            i += 1;
        }
    }
    return newly_marked;
}

/// Summarizes wrapping modes and bundler feature counts across a module slice.
pub fn summarizeBundledGraph(modules: []const JsBundledAstMeta): JsBundledGraphStats {
    let stats = JsBundledGraphStats{ total_modules: modules.len };
    let i: usize = 0;
    while (i < modules.len) {
        const m = modules[i];
        const mode = m.wrapMode();
        if (mode == JsBundleWrapMode.flat_esm) {
            stats.flat_esm_count += 1;
        } else if (mode == JsBundleWrapMode.unwrapped_cjs) {
            stats.unwrapped_cjs_count += 1;
        } else if (mode == JsBundleWrapMode.commonjs_closure) {
            stats.cjs_closure_count += 1;
        } else {
            stats.async_tla_count += 1;
        }
        if (m.hasFlag(BAST_FLAG_HAS_IMPORT_META)) {
            stats.import_meta_count += 1;
        }
        i += 1;
    }
    return stats;
}
