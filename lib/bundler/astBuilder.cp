// Source: Absorbed from /data/dev/bun/src/bundler/AstBuilder.cp | Synced: 2026-11-22T14:20:00Z
// Pure-CP zero-allocation Synthetic JavaScript AST Builder for Bun Bundler
// (used for RSC boundaries, virtual plugin modules, and generated wrappers).
// Aligned with Bun `src/bundler/AstBuilder.cp` and `docs/bundler/plugins.mdx`:
// - Pre-allocates top-level `.Entry` scope plus built-in `module_ref` (`"module"`) and
//   `hmr_api_ref` (`"hmr"`) symbols on `init()`.
// - Tracks lexical scope stack (`pushScope`/`popScope`), `is_top_level` symbol declarations,
//   `must_not_be_renamed` external globals (`newExternalSymbol`), and import records.
// - Automatically binds HMR `namespace_alias` on imported symbols when `hot_reloading == true`.
// - `finalize()` enforces zero unclosed scopes, marks all generated symbols as used
//   (`use_count = 1`), and builds the 2-part bundled AST summary (Part 0 empty, Part 1 body).

/// 单个合成模块支持的最大符号数
pub const AST_BUILDER_MAX_SYMBOLS: usize = 64;
/// 单个合成模块支持的最大作用域嵌套深度
pub const AST_BUILDER_MAX_SCOPES: usize = 16;
/// 单个合成模块支持的最大导入记录数
pub const AST_BUILDER_MAX_IMPORTS: usize = 32;
/// 单条合成 Import 语句支持的最大导入项数
pub const AST_BUILDER_MAX_IMPORT_ITEMS: usize = 16;
/// 单个合成模块支持的最大语句数
pub const AST_BUILDER_MAX_STMTS: usize = 64;

/// 合成 AST 构建器错误集
pub err AstBuilderError {
    TooManySymbols,
    ScopeStackFull,
    CannotPopRootScope,
    UnclosedScope,
    TooManyImports,
    TooManyImportItems,
    TooManyStmts,
    InvalidSymbolRef,
    EmptyIdentifier,
    EmptyImportPath,
    BufferTooSmall,
}

/// 作用域类别（对齐 Bun `js_ast.Scope.Kind`）
pub enum AstScopeKind: u8 {
    Entry = 0,
    Block = 1,
    FunctionArgs = 2,
    FunctionBody = 3,
    CatchIdentifier = 4,
}

/// 符号类别（对齐 Bun `js_ast.Symbol.Kind`）
pub enum AstSymbolKind: u8 {
    Other = 0,
    Import = 1,
    Unbound = 2,
    Hoisted = 3,
    Constant = 4,
}

/// 导入记录来源类别（对齐 Bun `ImportKind`）
pub enum AstImportKind: u8 {
    Stmt = 0,
    Require = 1,
    Dynamic = 2,
}

/// 合成语句类别
pub enum AstStmtKind: u8 {
    Import = 0,
    ExportConst = 1,
    ExportDefault = 2,
    RawStmt = 3,
}

/// 唯一符号引用（对齐 Bun `js_ast.Ref`）
pub struct AstSymRef {
    source_index: u32 = 0,
    inner_index: u32 = 0,
    is_valid: bool = false,

    fn eql(other: AstSymRef): bool {
        return _.is_valid == other.is_valid &&
            _.source_index == other.source_index &&
            _.inner_index == other.inner_index;
    }
}

/// HMR 命名空间别名绑定（`hot_reloading == true` 时附加在 `.Import` 符号上）
pub struct AstNamespaceAlias {
    namespace_ref: AstSymRef = AstSymRef{},
    alias: []const u8 = "",
    import_record_index: u32 = 0,
}

/// 符号表条目（对齐 Bun `js_ast.Symbol` + `DeclaredSymbol`）
pub struct AstSymbolEntry {
    kind: AstSymbolKind = .Other,
    original_name: []const u8 = "",
    is_top_level: bool = true,
    must_not_be_renamed: bool = false,
    use_count: u32 = 0,
    has_namespace_alias: bool = false,
    namespace_alias: AstNamespaceAlias = AstNamespaceAlias{},
}

/// 模块导入记录
pub struct AstImportRecord {
    path: []const u8 = "",
    kind: AstImportKind = .Stmt,
    was_originally_bare_import: bool = false,
}

/// 单条合成语句节点
pub struct AstSyntheticStmt {
    kind: AstStmtKind = .RawStmt,
    import_record_index: u32 = 0,
    namespace_ref: AstSymRef = AstSymRef{},
    item_count: usize = 0,
    item_names: [AST_BUILDER_MAX_IMPORT_ITEMS][]const u8 = [""; AST_BUILDER_MAX_IMPORT_ITEMS],
    item_refs: [AST_BUILDER_MAX_IMPORT_ITEMS]AstSymRef = [AstSymRef{}; AST_BUILDER_MAX_IMPORT_ITEMS],
    is_single_line: bool = true,
    export_name: []const u8 = "",
    expr_code: []const u8 = "",
}

/// `finalize()` 返回的 2-Part 合成 AST 摘要（对齐 Bun `AstBuilder.toBundledAst`）
pub struct AstBuildSummary {
    source_index: u32 = 0,
    /// 对齐 Bun：`parts.len == 2`（Part 0 空占位，Part 1 主体语句）
    part_count: u32 = 2,
    symbol_count: usize = 0,
    top_level_symbol_count: usize = 0,
    external_symbol_count: usize = 0,
    import_record_count: usize = 0,
    stmt_count: usize = 0,
    hmr_aliased_import_count: usize = 0,
    hot_reloading: bool = false,
    module_ref: AstSymRef = AstSymRef{},
    hmr_api_ref: AstSymRef = AstSymRef{},
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

fn copyJsQuoted(dst: []u8, offset: usize, s: []const u8): usize! {
    let pos = copyByte(dst, offset, '"')?;
    for (i of 0..s.len) {
        const ch = s[i];
        if (ch == '"' || ch == '\\') {
            pos = copyByte(dst, pos, '\\')?;
            pos = copyByte(dst, pos, ch)?;
        } else {
            pos = copyByte(dst, pos, ch)?;
        }
    }
    return copyByte(dst, pos, '"');
}

/// 零堆分配 Bundler 合成 JS AST 构建器（对齐 Bun `AstBuilder`）
pub struct SyntheticAstBuilder {
    source_index: u32 = 0,
    hot_reloading: bool = false,

    scope_stack: [AST_BUILDER_MAX_SCOPES]AstScopeKind = [.Entry; AST_BUILDER_MAX_SCOPES],
    scope_depth: usize = 0,

    symbols: [AST_BUILDER_MAX_SYMBOLS]AstSymbolEntry = [AstSymbolEntry{}; AST_BUILDER_MAX_SYMBOLS],
    symbol_count: usize = 0,

    import_records: [AST_BUILDER_MAX_IMPORTS]AstImportRecord = [AstImportRecord{}; AST_BUILDER_MAX_IMPORTS],
    import_count: usize = 0,

    stmts: [AST_BUILDER_MAX_STMTS]AstSyntheticStmt = [AstSyntheticStmt{}; AST_BUILDER_MAX_STMTS],
    stmt_count: usize = 0,

    module_ref: AstSymRef = AstSymRef{},
    hmr_api_ref: AstSymRef = AstSymRef{},

    /// 初始化构建器，预创建 `.Entry` 作用域与 `module`/`hmr` 两个内置顶层符号
    static fn init(source_index: u32, hot_reloading: bool): SyntheticAstBuilder {
        let ab = SyntheticAstBuilder{
            source_index: source_index,
            hot_reloading: hot_reloading,
            scope_depth: 0,
        };
        ab.scope_stack[0] = .Entry;
        ab.module_ref = ab.newSymbol(.Other, "module") catch unreachable;
        ab.hmr_api_ref = ab.newSymbol(.Other, "hmr") catch unreachable;
        return ab;
    }

    /// 压入新的子作用域，返回新的嵌套深度
    fn pushScope(*_, kind: AstScopeKind): usize! {
        if (_.scope_depth + 1 >= AST_BUILDER_MAX_SCOPES) {
            return err.ScopeStackFull;
        }
        _.scope_depth += 1;
        _.scope_stack[_.scope_depth] = kind;
        return _.scope_depth;
    }

    /// 弹出当前子作用域（严禁弹出根 `.Entry` 作用域）
    fn popScope(*_): usize! {
        if (_.scope_depth == 0) {
            return err.CannotPopRootScope;
        }
        _.scope_depth -= 1;
        return _.scope_depth;
    }

    /// 返回当前作用域类别
    fn currentScopeKind(): AstScopeKind {
        return _.scope_stack[_.scope_depth];
    }

    /// 在当前作用域中创建新符号并记录 `is_top_level`
    fn newSymbol(*_, kind: AstSymbolKind, identifier: []const u8): AstSymRef! {
        if (identifier.len == 0) return err.EmptyIdentifier;
        if (_.symbol_count >= AST_BUILDER_MAX_SYMBOLS) {
            return err.TooManySymbols;
        }
        const idx = _.symbol_count;
        _.symbols[idx] = AstSymbolEntry{
            kind: kind,
            original_name: identifier,
            is_top_level: _.scope_depth == 0,
            must_not_be_renamed: false,
            use_count: 0,
            has_namespace_alias: false,
        };
        _.symbol_count += 1;
        return AstSymRef{
            source_index: _.source_index,
            inner_index: idx as u32,
            is_valid: true,
        };
    }

    /// 创建外部全局符号（标记 `must_not_be_renamed = true`，对齐 `AstBuilder.newExternalSymbol`）
    fn newExternalSymbol(*_, name: []const u8): AstSymRef! {
        const r = _.newSymbol(.Other, name)?;
        _.symbols[r.inner_index as usize].must_not_be_renamed = true;
        return r;
    }

    /// 根据引用读取符号条目（校验 `source_index` 与边界）
    fn getSymbol(sym_ref: AstSymRef): AstSymbolEntry! {
        if (!sym_ref.is_valid || sym_ref.source_index != _.source_index) {
            return err.InvalidSymbolRef;
        }
        const idx = sym_ref.inner_index as usize;
        if (idx >= _.symbol_count) {
            return err.InvalidSymbolRef;
        }
        return _.symbols[idx];
    }

    /// 追加导入记录，返回记录索引
    fn addImportRecord(*_, path: []const u8, kind: AstImportKind): u32! {
        if (path.len == 0) return err.EmptyImportPath;
        if (_.import_count >= AST_BUILDER_MAX_IMPORTS) {
            return err.TooManyImports;
        }
        const idx = _.import_count;
        _.import_records[idx] = AstImportRecord{
            path: path,
            kind: kind,
            was_originally_bare_import: false,
        };
        _.import_count += 1;
        return idx as u32;
    }

    /// 生成合成 `import { ... } from "<path>";` 语句（或裸导入 `import "<path>";`）；
    /// 在 `hot_reloading == true` 时自动为每个导入符号绑定 `namespace_alias`
    fn addImportStmt(*_, path: []const u8, namespace_ident: []const u8, items: []const []const u8): usize! {
        if (items.len > AST_BUILDER_MAX_IMPORT_ITEMS) {
            return err.TooManyImportItems;
        }
        if (_.stmt_count >= AST_BUILDER_MAX_STMTS) {
            return err.TooManyStmts;
        }
        const rec_idx = _.addImportRecord(path, .Stmt)?;
        const ns_name = if (namespace_ident.len > 0) namespace_ident else "import_ns";
        const ns_ref = _.newSymbol(.Other, ns_name)?;

        let stmt = AstSyntheticStmt{
            kind: .Import,
            import_record_index: rec_idx,
            namespace_ref: ns_ref,
            item_count: items.len,
            is_single_line: items.len == 0,
        };

        if (items.len == 0) {
            _.import_records[rec_idx as usize].was_originally_bare_import = true;
        } else {
            for (i of 0..items.len) {
                const sym_ref = _.newSymbol(.Import, items[i])?;
                if (_.hot_reloading) {
                    _.symbols[sym_ref.inner_index as usize].has_namespace_alias = true;
                    _.symbols[sym_ref.inner_index as usize].namespace_alias = AstNamespaceAlias{
                        namespace_ref: ns_ref,
                        alias: items[i],
                        import_record_index: rec_idx,
                    };
                }
                stmt.item_names[i] = items[i];
                stmt.item_refs[i] = sym_ref;
            }
        }

        const s_idx = _.stmt_count;
        _.stmts[s_idx] = stmt;
        _.stmt_count += 1;
        return s_idx;
    }

    /// 追加 `export const <name> = <expr>;` 合成语句
    fn addExportConstStmt(*_, export_name: []const u8, expr_code: []const u8): usize! {
        if (export_name.len == 0) return err.EmptyIdentifier;
        if (_.stmt_count >= AST_BUILDER_MAX_STMTS) {
            return err.TooManyStmts;
        }
        * = _.newSymbol(.Constant, export_name)?;
        const s_idx = _.stmt_count;
        _.stmts[s_idx] = AstSyntheticStmt{
            kind: .ExportConst,
            export_name: export_name,
            expr_code: expr_code,
        };
        _.stmt_count += 1;
        return s_idx;
    }

    /// 追加 `export default <expr>;` 合成语句
    fn addExportDefaultStmt(*_, expr_code: []const u8): usize! {
        if (_.stmt_count >= AST_BUILDER_MAX_STMTS) {
            return err.TooManyStmts;
        }
        * = _.newSymbol(.Other, "default")?;
        const s_idx = _.stmt_count;
        _.stmts[s_idx] = AstSyntheticStmt{
            kind: .ExportDefault,
            export_name: "default",
            expr_code: expr_code,
        };
        _.stmt_count += 1;
        return s_idx;
    }

    /// 结束构建并生成 2-Part `AstBuildSummary`（要求所有子作用域已弹出，并将全部符号标记 `use_count = 1`）
    fn finalize(*_): AstBuildSummary! {
        if (_.scope_depth != 0) {
            return err.UnclosedScope;
        }
        let top_count: usize = 0;
        let ext_count: usize = 0;
        let hmr_alias_count: usize = 0;

        for (i of 0.._.symbol_count) {
            _.symbols[i].use_count = 1;
            if (_.symbols[i].is_top_level) top_count += 1;
            if (_.symbols[i].must_not_be_renamed) ext_count += 1;
            if (_.symbols[i].has_namespace_alias) hmr_alias_count += 1;
        }

        return AstBuildSummary{
            source_index: _.source_index,
            part_count: 2,
            symbol_count: _.symbol_count,
            top_level_symbol_count: top_count,
            external_symbol_count: ext_count,
            import_record_count: _.import_count,
            stmt_count: _.stmt_count,
            hmr_aliased_import_count: hmr_alias_count,
            hot_reloading: _.hot_reloading,
            module_ref: _.module_ref,
            hmr_api_ref: _.hmr_api_ref,
        };
    }

    /// 序列化所有合成语句为可读 JS 源码预览
    fn formatJsPreview(out: []u8): usize! {
        let pos: usize = 0;
        for (i of 0.._.stmt_count) {
            const st = _.stmts[i];
            switch (st.kind) {
                .Import => {
                    const rec = _.import_records[st.import_record_index as usize];
                    if (st.item_count == 0) {
                        pos = copyInto(out, pos, "import ")?;
                        pos = copyJsQuoted(out, pos, rec.path)?;
                        pos = copyInto(out, pos, ";\n")?;
                    } else {
                        pos = copyInto(out, pos, "import { ")?;
                        for (k of 0..st.item_count) {
                            if (k > 0) {
                                pos = copyInto(out, pos, ", ")?;
                            }
                            pos = copyInto(out, pos, st.item_names[k])?;
                        }
                        pos = copyInto(out, pos, " } from ")?;
                        pos = copyJsQuoted(out, pos, rec.path)?;
                        pos = copyInto(out, pos, ";\n")?;
                    }
                },
                .ExportConst => {
                    pos = copyInto(out, pos, "export const ")?;
                    pos = copyInto(out, pos, st.export_name)?;
                    pos = copyInto(out, pos, " = ")?;
                    pos = copyInto(out, pos, st.expr_code)?;
                    pos = copyInto(out, pos, ";\n")?;
                },
                .ExportDefault => {
                    pos = copyInto(out, pos, "export default ")?;
                    pos = copyInto(out, pos, st.expr_code)?;
                    pos = copyInto(out, pos, ";\n")?;
                },
                .RawStmt => {
                    if (st.expr_code.len > 0) {
                        pos = copyInto(out, pos, st.expr_code)?;
                        pos = copyByte(out, pos, '\n')?;
                    }
                },
            }
        }
        return pos;
    }
}
