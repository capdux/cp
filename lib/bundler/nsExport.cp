// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/doStep5.cp | Synced: 2026-11-22T13:38:00Z
// Pure-CP zero-allocation Bundler Step 5 Namespace Exports planner & `__export(...)` code emitter.
// Faithfully models Bun's `doStep5` / `createExportsForFile` invariants:
// - Resolves transitive `imports_to_bind` chains with cycle-safe hop bounds
// - Filters ambiguous `export *` collisions (omits if distinct targets; keeps if all resolve to same symbol)
// - Silently omits unresolved TypeScript type-only re-exports (`probably_typescript_type`)
// - Deterministically sorts export aliases in descending lexicographical order (`strings.sortDesc`)
// - Emits `var <exports> = {};`, `__export(<exports>, { ... });`, and CJS entry `module.exports = __toCommonJS(<exports>);`

/// 单个模块支持的最大命名空间导出条目数
pub const NS_EXPORT_MAX_ENTRIES: usize = 64;
/// 单条导出支持的最大潜在歧义 `export *` 候选引用数
pub const NS_EXPORT_MAX_AMBIG_REFS: usize = 8;
/// 单个规划器支持的最大导入绑定映射数
pub const NS_EXPORT_MAX_BINDINGS: usize = 64;
/// 传递导入绑定解析的最大跳数（防环路死循环）
pub const NS_EXPORT_MAX_BIND_HOPS: usize = 16;

/// 命名空间导出规划器错误集
pub err NsExportError {
    EmptyAlias,
    EmptySymbolName,
    DuplicateAlias,
    InvalidExportIndex,
    CapacityExceeded,
    BufferTooSmall,
}

/// 模块内唯一符号引用（对齐 Bun `js_ast.Ref`）
pub struct NsSymbolRef {
    source_index: u32 = 0,
    inner_index: u32 = 0,

    fn eql(other: NsSymbolRef): bool {
        return _.source_index == other.source_index && _.inner_index == other.inner_index;
    }
}

/// 导入符号绑定记录（对齐 Bun `imports_to_bind`）
pub struct NsImportBinding {
    from_ref: NsSymbolRef = NsSymbolRef{},
    target_ref: NsSymbolRef = NsSymbolRef{},
    target_name: []const u8 = "",
    has_namespace_alias: bool = false,
}

/// 单个已解析的原始导出条目
pub struct NsResolvedExport {
    alias: []const u8 = "",
    sym: NsSymbolRef = NsSymbolRef{},
    local_name: []const u8 = "",
    probably_ts_type: bool = false,
    ambig_refs: [NS_EXPORT_MAX_AMBIG_REFS]NsSymbolRef = [NsSymbolRef{}; NS_EXPORT_MAX_AMBIG_REFS],
    ambig_count: usize = 0,
}

/// 导出 getter 体表达式类型（对齐 Bun `E.Identifier` vs `E.ImportIdentifier`）
pub enum NsExportExprKind: u8 {
    DirectIdent = 0,
    ImportIdent = 1,
}

/// 过滤并排序后的最终输出导出项
pub struct NsEmittedExport {
    alias: []const u8 = "",
    resolved_ref: NsSymbolRef = NsSymbolRef{},
    expr_name: []const u8 = "",
    expr_kind: NsExportExprKind = .DirectIdent,
}

/// 代码生成选项（对齐 Bun `createExportsForFile` 标志位）
pub struct NsExportEmitOptions {
    exports_var_name: []const u8 = "exports_mod",
    needs_exports_variable: bool = true,
    is_cjs_entry_point: bool = false,
    force_include_cjs_exports: bool = false,
}

/// Step 5 过滤与排序统计信息
pub struct NsExportStats {
    input_count: usize = 0,
    omitted_ambiguous: usize = 0,
    omitted_ts_type: usize = 0,
    emitted_count: usize = 0,
}

/// 零堆分配 Bundler Step 5 命名空间导出规划器
pub struct NsExportPlanner {
    bindings: [NS_EXPORT_MAX_BINDINGS]NsImportBinding = [NsImportBinding{}; NS_EXPORT_MAX_BINDINGS],
    binding_count: usize = 0,

    raw_exports: [NS_EXPORT_MAX_ENTRIES]NsResolvedExport = [NsResolvedExport{}; NS_EXPORT_MAX_ENTRIES],
    raw_count: usize = 0,

    emitted: [NS_EXPORT_MAX_ENTRIES]NsEmittedExport = [NsEmittedExport{}; NS_EXPORT_MAX_ENTRIES],
    emitted_count: usize = 0,

    static fn init(): NsExportPlanner {
        return NsExportPlanner{};
    }

    /// 注册一条导入符号绑定映射（`from_ref -> target_ref`）
    fn addBinding(
        *_,
        from_ref: NsSymbolRef,
        target_ref: NsSymbolRef,
        target_name: []const u8,
        has_namespace_alias: bool,
    ): usize! {
        if (target_name.len == 0) return err.EmptySymbolName;
        if (_.binding_count >= NS_EXPORT_MAX_BINDINGS) return err.CapacityExceeded;
        _.bindings[_.binding_count] = NsImportBinding{
            from_ref: from_ref,
            target_ref: target_ref,
            target_name: target_name,
            has_namespace_alias: has_namespace_alias,
        };
        _.binding_count += 1;
        return _.binding_count;
    }

    /// 注册一条模块的原始导出条目，返回索引供后续附加潜在歧义引用
    fn addExport(
        *_,
        alias: []const u8,
        sym: NsSymbolRef,
        local_name: []const u8,
        probably_ts_type: bool,
    ): usize! {
        if (alias.len == 0) return err.EmptyAlias;
        if (local_name.len == 0) return err.EmptySymbolName;
        for (i of 0.._.raw_count) {
            if (bytesEql(_.raw_exports[i].alias, alias)) {
                return err.DuplicateAlias;
            }
        }
        if (_.raw_count >= NS_EXPORT_MAX_ENTRIES) return err.CapacityExceeded;
        const idx = _.raw_count;
        _.raw_exports[idx] = NsResolvedExport{
            alias: alias,
            sym: sym,
            local_name: local_name,
            probably_ts_type: probably_ts_type,
            ambig_count: 0,
        };
        _.raw_count += 1;
        return idx;
    }

    /// 为指定导出条目追加一条来自 `export *` 的潜在歧义引用
    fn addAmbiguousRef(*_, export_idx: usize, ambig_ref: NsSymbolRef): usize! {
        if (export_idx >= _.raw_count) return err.InvalidExportIndex;
        const cnt = _.raw_exports[export_idx].ambig_count;
        if (cnt >= NS_EXPORT_MAX_AMBIG_REFS) return err.CapacityExceeded;
        _.raw_exports[export_idx].ambig_refs[cnt] = ambig_ref;
        _.raw_exports[export_idx].ambig_count = cnt + 1;
        return cnt + 1;
    }

    fn findBinding(sym: NsSymbolRef): NsImportBinding? {
        for (i of 0.._.binding_count) {
            if (_.bindings[i].from_ref.eql(sym)) {
                return _.bindings[i];
            }
        }
        return null;
    }

    fn resolveRef(sym: NsSymbolRef): NsSymbolRef {
        let cur = sym;
        for (_hop of 0..NS_EXPORT_MAX_BIND_HOPS) {
            const b = _.findBinding(cur) ?? break;
            if (b.target_ref.eql(cur)) break;
            cur = b.target_ref;
        }
        return cur;
    }

    fn resolveExportExpr(sym: NsSymbolRef, fallback_name: []const u8): NsEmittedExport {
        let cur_ref = sym;
        let cur_name = fallback_name;
        let kind = NsExportExprKind.DirectIdent;

        for (_hop of 0..NS_EXPORT_MAX_BIND_HOPS) {
            const b = _.findBinding(cur_ref) ?? break;
            cur_name = b.target_name;
            if (b.has_namespace_alias) {
                kind = .ImportIdent;
            }
            if (b.target_ref.eql(cur_ref)) break;
            cur_ref = b.target_ref;
        }

        return NsEmittedExport{
            resolved_ref: cur_ref,
            expr_name: cur_name,
            expr_kind: kind,
        };
    }

    /// 执行 Step 5 歧义裁决、TS type-only 静默过滤及别名降序排序（对齐 Bun `strings.sortDesc`）
    fn build(*_): NsExportStats {
        _.emitted_count = 0;
        let omitted_ambig: usize = 0;
        let omitted_ts: usize = 0;

        for (i of 0.._.raw_count) {
            const exp = _.raw_exports[i];

            if (exp.probably_ts_type) {
                omitted_ts += 1;
                continue;
            }

            const main_resolved = _.resolveRef(exp.sym);
            let is_ambiguous = false;
            for (ai of 0..exp.ambig_count) {
                const ambig_resolved = _.resolveRef(exp.ambig_refs[ai]);
                if (!main_resolved.eql(ambig_resolved)) {
                    is_ambiguous = true;
                    break;
                }
            }
            if (is_ambiguous) {
                omitted_ambig += 1;
                continue;
            }

            let emitted_item = _.resolveExportExpr(exp.sym, exp.local_name);
            emitted_item.alias = exp.alias;
            _.emitted[_.emitted_count] = emitted_item;
            _.emitted_count += 1;
        }

        // 确定性插入排序：按导出别名字典降序（对齐 Bun `strings.sortDesc`）
        let si: usize = 1;
        while (si < _.emitted_count) {
            const key = _.emitted[si];
            let j = si;
            while (j > 0 && bytesLessThan(_.emitted[j - 1].alias, key.alias)) {
                _.emitted[j] = _.emitted[j - 1];
                j -= 1;
            }
            _.emitted[j] = key;
            si += 1;
        }

        return NsExportStats{
            input_count: _.raw_count,
            omitted_ambiguous: omitted_ambig,
            omitted_ts_type: omitted_ts,
            emitted_count: _.emitted_count,
        };
    }

    /// 返回过滤排序后的只读导出切片
    fn emittedSlice(): []const NsEmittedExport {
        return _.emitted[0.._.emitted_count];
    }

    /// 格式化生成 Step 5 命名空间导出 JS 语句（`var exports_x = {}; __export(exports_x, { ... });`）
    fn formatJsCode(opts: NsExportEmitOptions, out: []u8): usize! {
        if (opts.exports_var_name.len == 0) return err.EmptySymbolName;
        let pos: usize = 0;

        if (opts.needs_exports_variable) {
            pos = copyInto(out, pos, "var ")?;
            pos = copyInto(out, pos, opts.exports_var_name)?;
            pos = copyInto(out, pos, " = {};\n")?;
        }

        if (_.emitted_count > 0) {
            pos = copyInto(out, pos, "__export(")?;
            pos = copyInto(out, pos, opts.exports_var_name)?;
            pos = copyInto(out, pos, ", { ")?;
            for (i of 0.._.emitted_count) {
                if (i > 0) {
                    pos = copyInto(out, pos, ", ")?;
                }
                const item = _.emitted[i];
                if (isSimpleJsIdent(item.alias)) {
                    pos = copyInto(out, pos, item.alias)?;
                } else {
                    pos = copyByte(out, pos, '"')?;
                    pos = copyInto(out, pos, item.alias)?;
                    pos = copyByte(out, pos, '"')?;
                }
                pos = copyInto(out, pos, ": () => ")?;
                pos = copyInto(out, pos, item.expr_name)?;
            }
            pos = copyInto(out, pos, " });\n")?;
        }

        if (opts.is_cjs_entry_point && opts.force_include_cjs_exports) {
            pos = copyInto(out, pos, "module.exports = __toCommonJS(")?;
            pos = copyInto(out, pos, opts.exports_var_name)?;
            pos = copyInto(out, pos, ");\n")?;
        }

        return pos;
    }
}

/// 判断别名是否可直接作为未加引号的 JS 对象属性名
pub fn isSimpleJsIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    const first = s[0];
    if (!((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || first == '_' || first == '$')) {
        return false;
    }
    for (i of 1..s.len) {
        const c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$')) {
            return false;
        }
    }
    return true;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn bytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    for (i of 0..min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
    }
    return a.len < b.len;
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
