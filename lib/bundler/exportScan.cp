// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/scanImportsAndExports.cp | Synced: 2026-11-23T14:41:08Z
// Pure-CP zero-allocation Bundler Linker import/export scanner:
// Step 1 CommonJS/ESM wrap propagation (`require()`, `import *`, `import default` on empty modules,
// top-level await ESM wrapper propagation), and Step 2 cycle-safe `export * from` recursive expansion
// with ES-spec `"default"` exclusion, local-export shadowing, diamond de-duplication, and ambiguity detection.

/// 单个扫描器支持的最大模块节点数
pub const EXPORT_SCAN_MAX_MODULES: usize = 64;

/// 单个扫描器支持的最大导出条目数（含展开后的 `export *` 条目）
pub const EXPORT_SCAN_MAX_EXPORTS: usize = 128;

/// 单个扫描器支持的最大导入/重导出边数
pub const EXPORT_SCAN_MAX_RECORDS: usize = 128;

/// 导入导出扫描错误集
pub err ExportScanError {
    DuplicateModule,
    UnknownModule,
    EmptyName,
    TableFull,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 模块导出语法分类（对齐 Bun `js_ast.ExportsKind`）
pub enum ScanExportsKind {
    None,
    Esm,
    Cjs,
    EsmWithDynamicFallback,
}

/// 模块链接期包装模式（对齐 Bun `JSMeta.Flags.wrap`）
pub enum ScanWrapKind {
    None,
    Cjs,
    Esm,
}

/// 导入记录类型（对齐 Bun `ImportRecord.Kind`）
pub enum ScanImportKind {
    Stmt,
    Require,
    DynamicImport,
    RequireResolve,
}

/// 模块图节点元数据
pub struct ScanModuleNode {
    source_index: u32 = 0,
    exports_kind: ScanExportsKind = ScanExportsKind.None,
    wrap: ScanWrapKind = ScanWrapKind.None,
    has_lazy_export: bool = false,
    force_cjs_to_esm: bool = false,
    is_entry_point: bool = false,
    has_top_level_await: bool = false,
}

/// 模块间导入/重导出边
pub struct ScanImportEdge {
    importer_idx: usize = 0,
    target_idx: usize = 0,
    kind: ScanImportKind = ScanImportKind.Stmt,
    has_import_star: bool = false,
    has_default_alias: bool = false,
    is_export_star: bool = false,
}

/// 模块导出表条目（支持追踪原始声明模块与 `export *` 冲突歧义）
pub struct ScanExportDecl {
    module_idx: usize = 0,
    export_name: []const u8 = "",
    origin_module_idx: usize = 0,
    is_local: bool = true,
    ambiguous: bool = false,
}

/// 零堆分配 Bundler 链接期导入导出扫描与 `export *` 展开引擎
pub struct ExportScanner {
    modules: [EXPORT_SCAN_MAX_MODULES]ScanModuleNode = [ScanModuleNode{}; EXPORT_SCAN_MAX_MODULES],
    module_len: usize = 0,
    edges: [EXPORT_SCAN_MAX_RECORDS]ScanImportEdge = [ScanImportEdge{}; EXPORT_SCAN_MAX_RECORDS],
    edge_len: usize = 0,
    exports: [EXPORT_SCAN_MAX_EXPORTS]ScanExportDecl = [ScanExportDecl{}; EXPORT_SCAN_MAX_EXPORTS],
    export_len: usize = 0,

    static fn init(): ExportScanner {
        return ExportScanner{};
    }

    fn addModule(*_, node: ScanModuleNode): usize! {
        let i: usize = 0;
        while (i < _.module_len) {
            if (_.modules[i].source_index == node.source_index) {
                return err.DuplicateModule;
            }
            i += 1;
        }
        if (_.module_len >= EXPORT_SCAN_MAX_MODULES) {
            return err.TableFull;
        }
        const idx = _.module_len;
        _.modules[idx] = node;
        _.module_len += 1;
        return idx;
    }

    fn addLocalExport(*_, module_idx: usize, export_name: []const u8): bool! {
        if (module_idx >= _.module_len) {
            return err.UnknownModule;
        }
        if (export_name.len == 0) {
            return err.EmptyName;
        }
        let i: usize = 0;
        while (i < _.export_len) {
            if (_.exports[i].module_idx == module_idx && bytesEql(_.exports[i].export_name, export_name)) {
                _.exports[i].origin_module_idx = module_idx;
                _.exports[i].is_local = true;
                _.exports[i].ambiguous = false;
                return false;
            }
            i += 1;
        }
        if (_.export_len >= EXPORT_SCAN_MAX_EXPORTS) {
            return err.TableFull;
        }
        _.exports[_.export_len] = ScanExportDecl{
            module_idx: module_idx,
            export_name: export_name,
            origin_module_idx: module_idx,
            is_local: true,
            ambiguous: false,
        };
        _.export_len += 1;
        return true;
    }

    fn addImportEdge(*_, edge: ScanImportEdge): bool! {
        if (edge.importer_idx >= _.module_len || edge.target_idx >= _.module_len) {
            return err.UnknownModule;
        }
        if (_.edge_len >= EXPORT_SCAN_MAX_RECORDS) {
            return err.TableFull;
        }
        _.edges[_.edge_len] = edge;
        _.edge_len += 1;
        return true;
    }

    /// Step 1：按 Bun `scanImportsAndExports` 规则计算 CJS/ESM 模块包装策略：
    /// - `import * as ns` / `import def` 指向 `None` 空模块（非 lazy/force_cjs_to_esm） -> 转为 `Cjs` 并 `wrap = Cjs`；
    /// - `import def` 指向 `force_cjs_to_esm` -> 转为 `Cjs` 并 `wrap = Cjs`；
    /// - `require()` 指向任意目标 -> 目标标记 `wrap = Cjs`（若原本为 `None` 同时置 `exports_kind = Cjs`）；
    /// - `export *` 指向 `Cjs` 模块 -> 宿主升级为 `EsmWithDynamicFallback`；
    /// - `has_top_level_await` 模块标记 `wrap = Esm`（若未强制 `Cjs`）。
    /// 返回需要包装（`wrap != None`）的模块总数。
    fn resolveModuleWraps(*_): usize {
        let e: usize = 0;
        while (e < _.edge_len) {
            const edge = _.edges[e];
            const target = _.modules[edge.target_idx];

            if (edge.kind == ScanImportKind.Stmt) {
                if ((edge.has_import_star || edge.has_default_alias) &&
                    !target.has_lazy_export &&
                    !target.force_cjs_to_esm &&
                    target.exports_kind == ScanExportsKind.None)
                {
                    _.modules[edge.target_idx].exports_kind = ScanExportsKind.Cjs;
                    _.modules[edge.target_idx].wrap = ScanWrapKind.Cjs;
                }

                if (edge.has_default_alias && target.force_cjs_to_esm) {
                    _.modules[edge.target_idx].exports_kind = ScanExportsKind.Cjs;
                    _.modules[edge.target_idx].wrap = ScanWrapKind.Cjs;
                }

                if (edge.is_export_star && _.modules[edge.target_idx].exports_kind == ScanExportsKind.Cjs) {
                    if (_.modules[edge.importer_idx].exports_kind == ScanExportsKind.Esm) {
                        _.modules[edge.importer_idx].exports_kind = ScanExportsKind.EsmWithDynamicFallback;
                    }
                }
            } else if (edge.kind == ScanImportKind.Require) {
                if (target.exports_kind == ScanExportsKind.None) {
                    _.modules[edge.target_idx].exports_kind = ScanExportsKind.Cjs;
                }
                _.modules[edge.target_idx].wrap = ScanWrapKind.Cjs;
            }
            e += 1;
        }

        // Top-level await 需要 ESM 异步包装器
        let m: usize = 0;
        while (m < _.module_len) {
            if (_.modules[m].has_top_level_await && _.modules[m].wrap == ScanWrapKind.None) {
                _.modules[m].wrap = ScanWrapKind.Esm;
            }
            m += 1;
        }

        let wrapped: usize = 0;
        m = 0;
        while (m < _.module_len) {
            if (_.modules[m].wrap != ScanWrapKind.None) {
                wrapped += 1;
            }
            m += 1;
        }
        return wrapped;
    }

    fn findExportIndex(module_idx: usize, export_name: []const u8): usize? {
        let i: usize = 0;
        while (i < _.export_len) {
            if (_.exports[i].module_idx == module_idx && bytesEql(_.exports[i].export_name, export_name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn collectStarExportsDfs(*_, root_idx: usize, cur_idx: usize, visited_mask: *u64) {
        let e: usize = 0;
        while (e < _.edge_len) {
            const edge = _.edges[e];
            if (edge.importer_idx == cur_idx && edge.is_export_star) {
                const t_idx = edge.target_idx;
                const bit = @as(u64, 1) << @as(u6, @intCast(t_idx));
                if ((visited_mask & bit) == 0) {
                    visited_mask |= bit;
                    // 复制目标模块的本地导出（排除 ES 规范禁止转发的 "default"）
                    const snap_len = _.export_len;
                    let xi: usize = 0;
                    while (xi < snap_len) {
                        const decl = _.exports[xi];
                        if (decl.module_idx == t_idx && decl.is_local && !bytesEql(decl.export_name, "default")) {
                            if (existing := _.findExportIndex(root_idx, decl.export_name)) {
                                // 本地声明优先覆盖 export *；若同为重导出且来源不同则标记歧义
                                if (!_.exports[existing].is_local && _.exports[existing].origin_module_idx != decl.origin_module_idx) {
                                    _.exports[existing].ambiguous = true;
                                }
                            } else if (_.export_len < EXPORT_SCAN_MAX_EXPORTS) {
                                _.exports[_.export_len] = ScanExportDecl{
                                    module_idx: root_idx,
                                    export_name: decl.export_name,
                                    origin_module_idx: decl.origin_module_idx,
                                    is_local: false,
                                    ambiguous: false,
                                };
                                _.export_len += 1;
                            }
                        }
                        xi += 1;
                    }
                    _.collectStarExportsDfs(root_idx, t_idx, *visited_mask);
                }
            }
            e += 1;
        }
    }

    /// Step 2：防环递归展开全部 `export * from` 链路，执行 `"default"` 排除、本地遮蔽、菱形去重与歧义标记。
    /// 返回展开后的总导出条目数。
    fn resolveExportStars(*_): usize {
        let m: usize = 0;
        while (m < _.module_len) {
            let visited: u64 = @as(u64, 1) << @as(u6, @intCast(m));
            _.collectStarExportsDfs(m, m, *visited);
            m += 1;
        }
        return _.export_len;
    }

    fn lookupExport(module_idx: usize, export_name: []const u8): ScanExportDecl? {
        const idx = _.findExportIndex(module_idx, export_name) ?? return null;
        return _.exports[idx];
    }

    fn moduleAt(idx: usize): ScanModuleNode? {
        if (idx >= _.module_len) {
            return null;
        }
        return _.modules[idx];
    }
}
