// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/convertStmtsForChunk.cp | Synced: 2026-11-23T14:43:09Z
// Pure-CP zero-allocation Production Bundler Chunk Statement Placement & ESM/CJS Lowering Pass
// Aligned with Bun `convertStmtsForChunk.cp` & `docs/bundler/index.mdx`:
// - Routes statements into 4 wrapper regions (`OutsideWrapperPrefix`, `InsideWrapperPrefix`, `InsideWrapperSuffix`, `OutsideWrapperSuffix`)
// - Hoists external ESM imports out of `__esm`/`__commonJS` closures so top-level ES syntax remains valid
// - Drops barrel-optimized unused `export *` records and internal bundled imports
// - Rewrites `export *` / `export * as ns` / `export { ... } from` with `__reExport(exports, target[, module.exports])`
// - Strips `export` modifiers on functions/classes/locals and lowers `export default` into named local/fn/class declarations
// - Supports CJS entry-point double-write to `module.exports` and CJS unwrap-to-assignment lowering.

/// 单个 Chunk 转换器支持的最大输入语句数
pub const CHUNK_STMTS_MAX_INPUT: usize = 64;
/// 单个模块支持的最大导入记录数
pub const CHUNK_STMTS_MAX_IMPORTS: usize = 32;
/// 单个输出区域支持的最大语句数
pub const CHUNK_STMTS_MAX_REGION: usize = 32;

/// Chunk 语句转换器错误集
pub err ChunkStmtsError {
    TooManyStmts,
    TooManyImports,
    RegionOverflow,
    InvalidImportRecord,
    BufferTooSmall,
}

/// 模块包装模式（对齐 Bun `WrapKind`）
pub enum ChunkWrapKind: u8 {
    None = 0,
    Esm = 1,
    Cjs = 2,
}

/// 打包输出模块格式（对齐 Bun `Format`: `esm` / `cjs` / `iife`）
pub enum ChunkOutputFormat: u8 {
    Esm = 0,
    Cjs = 1,
    Iife = 2,

    /// 是否保留 ES6 `import`/`export` 顶层语法
    fn keepsEsmImportExportSyntax(): bool {
        return _ == .Esm;
    }
}

/// 包装器 4 大语句落位区域（对齐 Bun `StmtList`）
pub enum ChunkStmtRegion: u8 {
    OutsideWrapperPrefix = 0,
    InsideWrapperPrefix = 1,
    InsideWrapperSuffix = 2,
    OutsideWrapperSuffix = 3,
}

/// 待转换输入语句类型
pub enum ChunkStmtInputKind: u8 {
    Import = 0,
    ExportStar = 1,
    ExportStarAs = 2,
    ExportFrom = 3,
    ExportClause = 4,
    FunctionDecl = 5,
    ClassDecl = 6,
    LocalDecl = 7,
    ExportDefaultExpr = 8,
    ExportDefaultFunc = 9,
    ExportDefaultClass = 10,
    Other = 11,
}

/// 转换后生成的语句类型
pub enum ChunkEmittedStmtKind: u8 {
    ImportStmt = 0,
    ExportStarStmt = 1,
    ExportFromStmt = 2,
    ExportClauseStmt = 3,
    RuntimeReExportCall = 4,
    WrapperInitCall = 5,
    FunctionStmt = 6,
    ClassStmt = 7,
    LocalDeclStmt = 8,
    CjsExportAssign = 9,
    OtherStmt = 10,
}

/// 模块导入记录元数据（用于判断内部/外部、barrel 裁剪及动态 `__reExport`）
pub struct ChunkImportRecordMeta {
    specifier: []const u8 = "",
    namespace_name: []const u8 = "",
    /// 是否已解析为当前 Bundle 内部模块（非 external）
    is_internal: bool = false,
    /// Barrel 优化标记为未使用的 `export *` 记录
    is_unused: bool = false,
    target_wrap: ChunkWrapKind = .None,
    target_wrapper_name: []const u8 = "",
    target_exports_name: []const u8 = "",
    target_is_esm_with_dynamic_fallback: bool = false,
    calls_runtime_re_export: bool = false,
}

/// 输入语句描述符
pub struct ChunkStmtInput {
    kind: ChunkStmtInputKind = .Other,
    import_record_idx: u32 = 0,
    is_export: bool = false,
    was_cjs_export: bool = false,
    has_initializer: bool = true,
    name: []const u8 = "",
    alias_name: []const u8 = "",
    default_name: []const u8 = "default_export",
    code: []const u8 = "",
}

/// 输出语句描述符
pub struct ChunkEmittedStmt {
    region: ChunkStmtRegion = .InsideWrapperSuffix,
    kind: ChunkEmittedStmtKind = .OtherStmt,
    stripped_export: bool = false,
    uses_module_exports: bool = false,
    name: []const u8 = "",
    target: []const u8 = "",
    aux: []const u8 = "",
}

/// 转换选项（对齐 Bun `convertStmtsForChunk` 关键上下文参数）
pub struct ChunkConvertOptions {
    wrap: ChunkWrapKind = .None,
    format: ChunkOutputFormat = .Esm,
    is_entry_point: bool = false,
    is_passthrough: bool = false,
    unwrap_cjs_to_esm: bool = true,
    exports_name: []const u8 = "exports_mod",
}

/// 转换统计摘要
pub struct ChunkConvertStats {
    input_count: usize = 0,
    dropped_count: usize = 0,
    outside_prefix_count: usize = 0,
    inside_prefix_count: usize = 0,
    inside_suffix_count: usize = 0,
    outside_suffix_count: usize = 0,
    stripped_export_count: usize = 0,
    re_export_calls: usize = 0,
    wrapper_init_calls: usize = 0,
}

/// 零堆分配生产 Bundler Chunk 语句转换与区域落位引擎
pub struct ChunkStmtConverter {
    imports: [CHUNK_STMTS_MAX_IMPORTS]ChunkImportRecordMeta = [ChunkImportRecordMeta{}; CHUNK_STMTS_MAX_IMPORTS],
    import_count: usize = 0,
    inputs: [CHUNK_STMTS_MAX_INPUT]ChunkStmtInput = [ChunkStmtInput{}; CHUNK_STMTS_MAX_INPUT],
    input_count: usize = 0,

    outside_prefix: [CHUNK_STMTS_MAX_REGION]ChunkEmittedStmt = [ChunkEmittedStmt{}; CHUNK_STMTS_MAX_REGION],
    outside_prefix_len: usize = 0,
    inside_prefix: [CHUNK_STMTS_MAX_REGION]ChunkEmittedStmt = [ChunkEmittedStmt{}; CHUNK_STMTS_MAX_REGION],
    inside_prefix_len: usize = 0,
    inside_suffix: [CHUNK_STMTS_MAX_REGION]ChunkEmittedStmt = [ChunkEmittedStmt{}; CHUNK_STMTS_MAX_REGION],
    inside_suffix_len: usize = 0,
    outside_suffix: [CHUNK_STMTS_MAX_REGION]ChunkEmittedStmt = [ChunkEmittedStmt{}; CHUNK_STMTS_MAX_REGION],
    outside_suffix_len: usize = 0,

    static fn init(): ChunkStmtConverter {
        return ChunkStmtConverter{};
    }

    fn resetOutputs(*_) {
        _.outside_prefix_len = 0;
        _.inside_prefix_len = 0;
        _.inside_suffix_len = 0;
        _.outside_suffix_len = 0;
    }

    fn addImportRecord(*_, rec: ChunkImportRecordMeta): u32! {
        if (_.import_count >= CHUNK_STMTS_MAX_IMPORTS) {
            return err.TooManyImports;
        }
        const idx = @as(u32, @intCast(_.import_count));
        _.imports[_.import_count] = rec;
        _.import_count += 1;
        return idx;
    }

    fn addStmt(*_, stmt: ChunkStmtInput): usize! {
        if (_.input_count >= CHUNK_STMTS_MAX_INPUT) {
            return err.TooManyStmts;
        }
        const idx = _.input_count;
        _.inputs[_.input_count] = stmt;
        _.input_count += 1;
        return idx;
    }

    fn regionSlice(region: ChunkStmtRegion): []const ChunkEmittedStmt {
        return switch (region) {
            .OutsideWrapperPrefix => _.outside_prefix[0.._.outside_prefix_len],
            .InsideWrapperPrefix => _.inside_prefix[0.._.inside_prefix_len],
            .InsideWrapperSuffix => _.inside_suffix[0.._.inside_suffix_len],
            .OutsideWrapperSuffix => _.outside_suffix[0.._.outside_suffix_len],
        };
    }

    /// 执行完整的 Chunk 语句降级与 4 区域分流（对齐 Bun `convertStmtsForChunk`）
    fn convert(*_, opts: ChunkConvertOptions): ChunkConvertStats! {
        _.resetOutputs();
        let stats = ChunkConvertStats{ input_count: _.input_count };

        const extract_esm_for_wrap = opts.wrap != .None;
        const strip_exports = !opts.is_passthrough || opts.is_entry_point;
        const cjs_entry_double_write = opts.format == .Cjs && opts.is_entry_point;

        let i: usize = 0;
        while (i < _.input_count) {
            const st = _.inputs[i];
            i += 1;

            switch (st.kind) {
                .Import => {
                    const rec = _.getImportRecord(st.import_record_idx)?;
                    // 内部已打包模块在 bundle 中直接消除 ESM import 语句
                    if (rec.is_internal) {
                        stats.dropped_count += 1;
                        continue;
                    }
                    const emitted = ChunkEmittedStmt{
                        region: if (extract_esm_for_wrap) .OutsideWrapperPrefix else .InsideWrapperSuffix,
                        kind: .ImportStmt,
                        name: rec.namespace_name,
                        target: rec.specifier,
                        aux: st.code,
                    };
                    * = _.pushToRegion(emitted.region, emitted)?;
                },

                .ExportStarAs => {
                    const rec = _.getImportRecord(st.import_record_idx)?;
                    if (rec.is_internal) {
                        stats.dropped_count += 1;
                        continue;
                    }
                    const kind: ChunkEmittedStmtKind = if (strip_exports) .ImportStmt else .ExportStarStmt;
                    if (strip_exports) stats.stripped_export_count += 1;
                    const region: ChunkStmtRegion = if (extract_esm_for_wrap) .OutsideWrapperPrefix else .InsideWrapperSuffix;
                    * = _.pushToRegion(region, ChunkEmittedStmt{
                        region: region,
                        kind: kind,
                        stripped_export: strip_exports,
                        name: st.alias_name,
                        target: rec.specifier,
                    })?;
                },

                .ExportStar => {
                    const rec = _.getImportRecord(st.import_record_idx)?;
                    // Barrel 优化：未使用的 export * 直接裁剪
                    if (rec.is_unused) {
                        stats.dropped_count += 1;
                        continue;
                    }
                    if (!strip_exports) {
                        * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                            region: .InsideWrapperSuffix,
                            kind: .ExportStarStmt,
                            target: rec.specifier,
                        })?;
                        continue;
                    }

                    if (!rec.is_internal && opts.format.keepsEsmImportExportSyntax()) {
                        if (rec.calls_runtime_re_export) {
                            // 注入 __reExport(exports, ns[, module.exports]) 至 inside_wrapper_prefix
                            * = _.pushToRegion(.InsideWrapperPrefix, ChunkEmittedStmt{
                                region: .InsideWrapperPrefix,
                                kind: .RuntimeReExportCall,
                                uses_module_exports: cjs_entry_double_write,
                                name: opts.exports_name,
                                target: rec.namespace_name,
                            })?;
                            stats.re_export_calls += 1;

                            // 原语句转为 `import * as ns from "specifier"`
                            const region: ChunkStmtRegion = if (extract_esm_for_wrap) .OutsideWrapperPrefix else .InsideWrapperSuffix;
                            * = _.pushToRegion(region, ChunkEmittedStmt{
                                region: region,
                                kind: .ImportStmt,
                                stripped_export: true,
                                name: rec.namespace_name,
                                target: rec.specifier,
                            })?;
                            stats.stripped_export_count += 1;
                        } else {
                            * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                                region: .InsideWrapperSuffix,
                                kind: .ExportStarStmt,
                                target: rec.specifier,
                            })?;
                        }
                    } else {
                        // 内部已打包 ESM 模块且被包装：注入 `init_target()` 调用
                        if (rec.is_internal && rec.target_wrap == .Esm && rec.target_wrapper_name.len > 0) {
                            * = _.pushToRegion(.InsideWrapperPrefix, ChunkEmittedStmt{
                                region: .InsideWrapperPrefix,
                                kind: .WrapperInitCall,
                                name: rec.target_wrapper_name,
                            })?;
                            stats.wrapper_init_calls += 1;
                        }

                        if (rec.calls_runtime_re_export) {
                            const re_target = if (rec.is_internal && rec.target_is_esm_with_dynamic_fallback && rec.target_exports_name.len > 0)
                                rec.target_exports_name
                            else
                                rec.specifier;
                            * = _.pushToRegion(.InsideWrapperPrefix, ChunkEmittedStmt{
                                region: .InsideWrapperPrefix,
                                kind: .RuntimeReExportCall,
                                uses_module_exports: cjs_entry_double_write,
                                name: opts.exports_name,
                                target: re_target,
                                aux: if (rec.is_internal && rec.target_is_esm_with_dynamic_fallback) "ident" else "require",
                            })?;
                            stats.re_export_calls += 1;
                        }

                        // 移除原 export * 语句
                        stats.dropped_count += 1;
                        stats.stripped_export_count += 1;
                    }
                },

                .ExportFrom => {
                    const rec = _.getImportRecord(st.import_record_idx)?;
                    if (rec.is_internal) {
                        stats.dropped_count += 1;
                        continue;
                    }
                    const kind: ChunkEmittedStmtKind = if (strip_exports) .ImportStmt else .ExportFromStmt;
                    if (strip_exports) stats.stripped_export_count += 1;
                    const region: ChunkStmtRegion = if (extract_esm_for_wrap) .OutsideWrapperPrefix else .InsideWrapperSuffix;
                    * = _.pushToRegion(region, ChunkEmittedStmt{
                        region: region,
                        kind: kind,
                        stripped_export: strip_exports,
                        name: st.name,
                        target: rec.specifier,
                    })?;
                },

                .ExportClause => {
                    if (strip_exports) {
                        stats.dropped_count += 1;
                        stats.stripped_export_count += 1;
                        continue;
                    }
                    const region: ChunkStmtRegion = if (extract_esm_for_wrap) .OutsideWrapperPrefix else .InsideWrapperSuffix;
                    * = _.pushToRegion(region, ChunkEmittedStmt{
                        region: region,
                        kind: .ExportClauseStmt,
                        name: st.name,
                    })?;
                },

                .FunctionDecl => {
                    const stripped = strip_exports && st.is_export;
                    if (stripped) stats.stripped_export_count += 1;
                    * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                        region: .InsideWrapperSuffix,
                        kind: .FunctionStmt,
                        stripped_export: stripped,
                        name: st.name,
                        aux: st.code,
                    })?;
                },

                .ClassDecl => {
                    const stripped = strip_exports && st.is_export;
                    if (stripped) stats.stripped_export_count += 1;
                    * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                        region: .InsideWrapperSuffix,
                        kind: .ClassStmt,
                        stripped_export: stripped,
                        name: st.name,
                        aux: st.code,
                    })?;
                },

                .LocalDecl => {
                    if (strip_exports && st.is_export) {
                        stats.stripped_export_count += 1;
                        * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                            region: .InsideWrapperSuffix,
                            kind: .LocalDeclStmt,
                            stripped_export: true,
                            name: st.name,
                            aux: st.code,
                        })?;
                    } else if (opts.unwrap_cjs_to_esm && st.was_cjs_export && opts.wrap == .Cjs) {
                        if (!st.has_initializer) {
                            stats.dropped_count += 1;
                            continue;
                        }
                        * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                            region: .InsideWrapperSuffix,
                            kind: .CjsExportAssign,
                            name: st.name,
                            aux: st.code,
                        })?;
                    } else {
                        * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                            region: .InsideWrapperSuffix,
                            kind: .LocalDeclStmt,
                            name: st.name,
                            aux: st.code,
                        })?;
                    }
                },

                .ExportDefaultExpr => {
                    if (strip_exports) stats.stripped_export_count += 1;
                    * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                        region: .InsideWrapperSuffix,
                        kind: if (strip_exports) .LocalDeclStmt else .OtherStmt,
                        stripped_export: strip_exports,
                        name: st.default_name,
                        aux: st.code,
                    })?;
                },

                .ExportDefaultFunc => {
                    if (strip_exports) stats.stripped_export_count += 1;
                    * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                        region: .InsideWrapperSuffix,
                        kind: .FunctionStmt,
                        stripped_export: strip_exports,
                        name: if (st.name.len > 0) st.name else st.default_name,
                        aux: st.code,
                    })?;
                },

                .ExportDefaultClass => {
                    if (strip_exports) stats.stripped_export_count += 1;
                    * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                        region: .InsideWrapperSuffix,
                        kind: .ClassStmt,
                        stripped_export: strip_exports,
                        name: if (st.name.len > 0) st.name else st.default_name,
                        aux: st.code,
                    })?;
                },

                .Other => {
                    * = _.pushToRegion(.InsideWrapperSuffix, ChunkEmittedStmt{
                        region: .InsideWrapperSuffix,
                        kind: .OtherStmt,
                        aux: st.code,
                    })?;
                },
            }
        }

        stats.outside_prefix_count = _.outside_prefix_len;
        stats.inside_prefix_count = _.inside_prefix_len;
        stats.inside_suffix_count = _.inside_suffix_len;
        stats.outside_suffix_count = _.outside_suffix_len;
        return stats;
    }

    /// 将 4 区域结果渲染为可读 JS 预览（用于调试与快照验证）
    fn formatPreview(out: []u8, wrapper_name: []const u8): usize! {
        let pos: usize = 0;
        pos = appendRegionLines(out, pos, _.outside_prefix[0.._.outside_prefix_len])?;
        if (wrapper_name.len > 0) {
            pos = appendBytes(out, pos, "var ")?;
            pos = appendBytes(out, pos, wrapper_name)?;
            pos = appendBytes(out, pos, " = __esm(() => {\n")?;
        }
        pos = appendRegionLines(out, pos, _.inside_prefix[0.._.inside_prefix_len])?;
        pos = appendRegionLines(out, pos, _.inside_suffix[0.._.inside_suffix_len])?;
        if (wrapper_name.len > 0) {
            pos = appendBytes(out, pos, "});\n")?;
        }
        pos = appendRegionLines(out, pos, _.outside_suffix[0.._.outside_suffix_len])?;
        return pos;
    }

    fn getImportRecord(idx: u32): ChunkImportRecordMeta! {
        const u = @as(usize, idx);
        if (u >= _.import_count) {
            return err.InvalidImportRecord;
        }
        return _.imports[u];
    }

    fn pushToRegion(*_, region: ChunkStmtRegion, stmt: ChunkEmittedStmt): usize! {
        switch (region) {
            .OutsideWrapperPrefix => {
                if (_.outside_prefix_len >= CHUNK_STMTS_MAX_REGION) return err.RegionOverflow;
                _.outside_prefix[_.outside_prefix_len] = stmt;
                _.outside_prefix_len += 1;
                return _.outside_prefix_len;
            },
            .InsideWrapperPrefix => {
                if (_.inside_prefix_len >= CHUNK_STMTS_MAX_REGION) return err.RegionOverflow;
                _.inside_prefix[_.inside_prefix_len] = stmt;
                _.inside_prefix_len += 1;
                return _.inside_prefix_len;
            },
            .InsideWrapperSuffix => {
                if (_.inside_suffix_len >= CHUNK_STMTS_MAX_REGION) return err.RegionOverflow;
                _.inside_suffix[_.inside_suffix_len] = stmt;
                _.inside_suffix_len += 1;
                return _.inside_suffix_len;
            },
            .OutsideWrapperSuffix => {
                if (_.outside_suffix_len >= CHUNK_STMTS_MAX_REGION) return err.RegionOverflow;
                _.outside_suffix[_.outside_suffix_len] = stmt;
                _.outside_suffix_len += 1;
                return _.outside_suffix_len;
            },
        }
    }
}

// ----------------------------------------------------------------------------
// 内部零分配格式化辅助
// ----------------------------------------------------------------------------

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendRegionLines(out: []u8, start_pos: usize, items: []const ChunkEmittedStmt): usize! {
    let pos = start_pos;
    let i: usize = 0;
    while (i < items.len) {
        const it = items[i];
        switch (it.kind) {
            .ImportStmt => {
                pos = appendBytes(out, pos, "import * as ")?;
                pos = appendBytes(out, pos, if (it.name.len > 0) it.name else "ns")?;
                pos = appendBytes(out, pos, " from \"")?;
                pos = appendBytes(out, pos, it.target)?;
                pos = appendBytes(out, pos, "\";\n")?;
            },
            .RuntimeReExportCall => {
                pos = appendBytes(out, pos, "__reExport(")?;
                pos = appendBytes(out, pos, it.name)?;
                pos = appendBytes(out, pos, ", ")?;
                pos = appendBytes(out, pos, it.target)?;
                if (it.uses_module_exports) {
                    pos = appendBytes(out, pos, ", module.exports")?;
                }
                pos = appendBytes(out, pos, ");\n")?;
            },
            .WrapperInitCall => {
                pos = appendBytes(out, pos, it.name)?;
                pos = appendBytes(out, pos, "();\n")?;
            },
            .FunctionStmt => {
                pos = appendBytes(out, pos, "function ")?;
                pos = appendBytes(out, pos, it.name)?;
                pos = appendBytes(out, pos, "() {}\n")?;
            },
            .ClassStmt => {
                pos = appendBytes(out, pos, "class ")?;
                pos = appendBytes(out, pos, it.name)?;
                pos = appendBytes(out, pos, " {}\n")?;
            },
            .LocalDeclStmt => {
                pos = appendBytes(out, pos, "var ")?;
                pos = appendBytes(out, pos, it.name)?;
                if (it.aux.len > 0) {
                    pos = appendBytes(out, pos, " = ")?;
                    pos = appendBytes(out, pos, it.aux)?;
                }
                pos = appendBytes(out, pos, ";\n")?;
            },
            .CjsExportAssign => {
                pos = appendBytes(out, pos, "exports.")?;
                pos = appendBytes(out, pos, it.name)?;
                pos = appendBytes(out, pos, " = ")?;
                pos = appendBytes(out, pos, it.aux)?;
                pos = appendBytes(out, pos, ";\n")?;
            },
            else => {
                if (it.aux.len > 0) {
                    pos = appendBytes(out, pos, it.aux)?;
                    pos = appendBytes(out, pos, "\n")?;
                }
            },
        }
        i += 1;
    }
    return pos;
}
