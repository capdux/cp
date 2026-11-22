// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/generateCodeForLazyExport.cp | Synced: 2026-11-22T12:46:58Z
// Pure-CP zero-allocation Bundler Lazy Export planner & CSS Modules `composes` resolver:
// ESM tree-shakeable named export promotion vs CJS `module.exports` / N-API `__require` planning,
// default export identifier sanitization (`<stem>_default`), and cycle-safe recursive
// CSS Modules `composes` (local / cross-file imported / `from global`) class token expansion.

/// 单模块最多规划的懒导出顶层属性数
pub const LAZY_EXPORT_MAX_PROPS: usize = 64;

/// 单个 CSS Modules 组合图支持的最大类定义数
pub const LAZY_EXPORT_MAX_CSS_CLASSES: usize = 64;

/// 单个 CSS Modules 组合图支持的最大 `composes` 依赖边数
pub const LAZY_EXPORT_MAX_COMPOSES_EDGES: usize = 128;

/// 单次 `composes` 展开最多收集的类名 token 数
pub const LAZY_EXPORT_MAX_RESOLVED_TOKENS: usize = 32;

/// 懒导出规划与 CSS Modules 组合展开错误集
pub err LazyExportError {
    EmptyName,
    InvalidIdentifier,
    DuplicateClass,
    NonSingleClassCompose,
    MissingComposeTarget,
    TableFull,
    BufferTooSmall,
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

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentStart(c: u8): bool {
    return isAsciiAlpha(c) || c == '_' || c == '$';
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || isAsciiDigit(c);
}

fn isJsReservedWord(name: []const u8): bool {
    if (bytesEql(name, "default")) return true;
    if (bytesEql(name, "break")) return true;
    if (bytesEql(name, "case")) return true;
    if (bytesEql(name, "catch")) return true;
    if (bytesEql(name, "class")) return true;
    if (bytesEql(name, "const")) return true;
    if (bytesEql(name, "continue")) return true;
    if (bytesEql(name, "debugger")) return true;
    if (bytesEql(name, "delete")) return true;
    if (bytesEql(name, "do")) return true;
    if (bytesEql(name, "else")) return true;
    if (bytesEql(name, "enum")) return true;
    if (bytesEql(name, "export")) return true;
    if (bytesEql(name, "extends")) return true;
    if (bytesEql(name, "false")) return true;
    if (bytesEql(name, "finally")) return true;
    if (bytesEql(name, "for")) return true;
    if (bytesEql(name, "function")) return true;
    if (bytesEql(name, "if")) return true;
    if (bytesEql(name, "import")) return true;
    if (bytesEql(name, "in")) return true;
    if (bytesEql(name, "instanceof")) return true;
    if (bytesEql(name, "new")) return true;
    if (bytesEql(name, "null")) return true;
    if (bytesEql(name, "return")) return true;
    if (bytesEql(name, "super")) return true;
    if (bytesEql(name, "switch")) return true;
    if (bytesEql(name, "this")) return true;
    if (bytesEql(name, "throw")) return true;
    if (bytesEql(name, "true")) return true;
    if (bytesEql(name, "try")) return true;
    if (bytesEql(name, "typeof")) return true;
    if (bytesEql(name, "var")) return true;
    if (bytesEql(name, "void")) return true;
    if (bytesEql(name, "while")) return true;
    if (bytesEql(name, "with")) return true;
    return false;
}

/// 校验字符串是否为合法且非保留字的 ASCII JS 标识符（对齐 `bun.js_lexer.isIdentifier`）
pub fn isLazyExportJsIdentifier(name: []const u8): bool {
    if (name.len == 0 || !isIdentStart(name[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        if (!isIdentContinue(name[i])) {
            return false;
        }
        i += 1;
    }
    if (isJsReservedWord(name)) {
        return false;
    }
    return true;
}

/// 判定 JSON / TOML / YAML / CSS Modules 的顶层属性是否可提升为独立可 tree-shake 的 ESM 命名导出
/// （对齐 Bun：跳过 `"default"`、`"__esModule"` 以及非合法标识符键）
pub fn isPromotableLazyNamedExport(name: []const u8): bool {
    if (bytesEql(name, "default") || bytesEql(name, "__esModule")) {
        return false;
    }
    return isLazyExportJsIdentifier(name);
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    pos += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool! {
    if (pos + s.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos] = s[i];
        pos += 1;
        i += 1;
    }
    return true;
}

/// 将源文件主干名规范化为默认导出绑定标识符 `<sanitized>_default`（对齐 `"{f}_default"`）
pub fn formatLazyDefaultIdent(source_stem: []const u8, out: []u8): usize! {
    if (source_stem.len == 0) {
        return err.EmptyName;
    }
    let pos: usize = 0;
    if (isAsciiDigit(source_stem[0])) {
        * = writeByte(out, *pos, '_')?;
    }
    let i: usize = 0;
    while (i < source_stem.len) {
        const c = source_stem[i];
        if (isIdentContinue(c)) {
            * = writeByte(out, *pos, c)?;
        } else {
            * = writeByte(out, *pos, '_')?;
        }
        i += 1;
    }
    * = writeSlice(out, *pos, "_default")?;
    return pos;
}

/// 懒导出模块格式分类
pub enum LazyExportKind {
    Esm,
    Cjs,
    CjsNapiAddon,
}

/// 单个顶层属性的懒导出决策
pub enum LazyExportPropDecision {
    /// 提升为独立 ESM `export const <key> = ...` Part，可独立 tree-shake
    PromotedNamedExport,
    /// 保留在 `export default { ... }` 中（`"default"` / `"__esModule"` 或 CJS 模式）
    DefaultOnlyReserved,
    /// 保留在 `export default { ... }` 中（非合法 JS 标识符，如 `"foo-bar"`）
    DefaultOnlyNonIdent,
}

/// 单个顶层属性的规划记录
pub struct LazyExportPropRecord {
    key: []const u8 = "",
    decision: LazyExportPropDecision = LazyExportPropDecision.DefaultOnlyReserved,
    part_index: u32 = 0,
}

/// 懒导出规划汇总
pub struct LazyExportPlanSummary {
    kind: LazyExportKind = LazyExportKind.Esm,
    needs_runtime_require: bool = false,
    promoted_named_count: usize = 0,
    default_only_count: usize = 0,
    total_props: usize = 0,
}

/// 零堆分配 Bundler 懒导出规划器（对齐 `generateCodeForLazyExport` 的 ESM/CJS 分支）
pub struct LazyExportPlanner {
    kind: LazyExportKind = LazyExportKind.Esm,
    needs_runtime_require: bool = false,
    props: [LAZY_EXPORT_MAX_PROPS]LazyExportPropRecord = [LazyExportPropRecord{}; LAZY_EXPORT_MAX_PROPS],
    prop_len: usize = 0,
    next_part_index: u32 = 2,

    static fn init(kind: LazyExportKind, output_is_cjs: bool): LazyExportPlanner {
        const needs_req = (kind == LazyExportKind.CjsNapiAddon) && !output_is_cjs;
        return LazyExportPlanner{
            kind: kind,
            needs_runtime_require: needs_req,
        };
    }

    fn addProperty(*_, key: []const u8): LazyExportPropDecision! {
        if (key.len == 0) {
            return err.EmptyName;
        }
        if (_.prop_len >= LAZY_EXPORT_MAX_PROPS) {
            return err.TableFull;
        }

        let decision = LazyExportPropDecision.DefaultOnlyReserved;
        let part_idx: u32 = 1;

        if (_.kind == LazyExportKind.Esm) {
            if (bytesEql(key, "default") || bytesEql(key, "__esModule")) {
                decision = LazyExportPropDecision.DefaultOnlyReserved;
            } else if (isLazyExportJsIdentifier(key)) {
                decision = LazyExportPropDecision.PromotedNamedExport;
                part_idx = _.next_part_index;
                _.next_part_index += 1;
            } else {
                decision = LazyExportPropDecision.DefaultOnlyNonIdent;
            }
        }

        _.props[_.prop_len] = LazyExportPropRecord{
            key: key,
            decision: decision,
            part_index: part_idx,
        };
        _.prop_len += 1;
        return decision;
    }

    fn propAt(idx: usize): LazyExportPropRecord? {
        if (idx >= _.prop_len) {
            return null;
        }
        return _.props[idx];
    }

    fn summary(): LazyExportPlanSummary {
        let promoted: usize = 0;
        let default_only: usize = 0;
        let i: usize = 0;
        while (i < _.prop_len) {
            if (_.props[i].decision == LazyExportPropDecision.PromotedNamedExport) {
                promoted += 1;
            } else {
                default_only += 1;
            }
            i += 1;
        }
        return LazyExportPlanSummary{
            kind: _.kind,
            needs_runtime_require: _.needs_runtime_require,
            promoted_named_count: promoted,
            default_only_count: default_only,
            total_props: _.prop_len,
        };
    }
}

/// CSS Modules `composes` 目标类型
pub enum CssComposeTargetKind {
    LocalClass,
    ImportedClass,
    GlobalRaw,
}

/// CSS Modules 类定义条目
pub struct CssModuleClassDef {
    source_index: u32 = 0,
    local_name: []const u8 = "",
    mangled_name: []const u8 = "",
    is_single_class: bool = true,
}

/// CSS Modules `composes` 依赖边
pub struct CssComposeEdge {
    from_class_idx: usize = 0,
    target_kind: CssComposeTargetKind = CssComposeTargetKind.LocalClass,
    target_class_idx: usize = 0,
    global_name: []const u8 = "",
}

/// 零堆分配 CSS Modules `composes` 递归展开图（对齐 `generateCodeForLazyExport.Visitor`）：
/// 支持同文件类、跨文件导入类与 `from global` 原始类名，内置 `u64` 位掩码防环与去重。
pub struct CssComposeGraph {
    classes: [LAZY_EXPORT_MAX_CSS_CLASSES]CssModuleClassDef = [CssModuleClassDef{}; LAZY_EXPORT_MAX_CSS_CLASSES],
    class_len: usize = 0,
    edges: [LAZY_EXPORT_MAX_COMPOSES_EDGES]CssComposeEdge = [CssComposeEdge{}; LAZY_EXPORT_MAX_COMPOSES_EDGES],
    edge_len: usize = 0,

    static fn init(): CssComposeGraph {
        return CssComposeGraph{};
    }

    fn registerClass(
        *_,
        source_index: u32,
        local_name: []const u8,
        mangled_name: []const u8,
        is_single_class: bool,
    ): usize! {
        if (local_name.len == 0 || mangled_name.len == 0) {
            return err.EmptyName;
        }
        let i: usize = 0;
        while (i < _.class_len) {
            if (_.classes[i].source_index == source_index && bytesEql(_.classes[i].local_name, local_name)) {
                return err.DuplicateClass;
            }
            i += 1;
        }
        if (_.class_len >= LAZY_EXPORT_MAX_CSS_CLASSES) {
            return err.TableFull;
        }
        const idx = _.class_len;
        _.classes[idx] = CssModuleClassDef{
            source_index: source_index,
            local_name: local_name,
            mangled_name: mangled_name,
            is_single_class: is_single_class,
        };
        _.class_len += 1;
        return idx;
    }

    /// 添加本地或跨文件 `composes` 依赖；目标若非单类选择器返回 `err.NonSingleClassCompose`
    fn addComposeRef(*_, from_idx: usize, target_idx: usize): bool! {
        if (from_idx >= _.class_len || target_idx >= _.class_len) {
            return err.MissingComposeTarget;
        }
        if (!_.classes[from_idx].is_single_class || !_.classes[target_idx].is_single_class) {
            return err.NonSingleClassCompose;
        }
        if (_.edge_len >= LAZY_EXPORT_MAX_COMPOSES_EDGES) {
            return err.TableFull;
        }
        const kind = if (_.classes[from_idx].source_index == _.classes[target_idx].source_index)
            CssComposeTargetKind.LocalClass
        else
            CssComposeTargetKind.ImportedClass;
        _.edges[_.edge_len] = CssComposeEdge{
            from_class_idx: from_idx,
            target_kind: kind,
            target_class_idx: target_idx,
            global_name: "",
        };
        _.edge_len += 1;
        return true;
    }

    /// 添加 `composes: <global_name> from global` 依赖（不改写作用域前缀）
    fn addComposeGlobal(*_, from_idx: usize, global_name: []const u8): bool! {
        if (from_idx >= _.class_len) {
            return err.MissingComposeTarget;
        }
        if (global_name.len == 0) {
            return err.EmptyName;
        }
        if (!_.classes[from_idx].is_single_class) {
            return err.NonSingleClassCompose;
        }
        if (_.edge_len >= LAZY_EXPORT_MAX_COMPOSES_EDGES) {
            return err.TableFull;
        }
        _.edges[_.edge_len] = CssComposeEdge{
            from_class_idx: from_idx,
            target_kind: CssComposeTargetKind.GlobalRaw,
            target_class_idx: 0,
            global_name: global_name,
        };
        _.edge_len += 1;
        return true;
    }

    fn visitComposesDfs(
        class_idx: usize,
        visited_mask: *u64,
        out_tokens: [][]const u8,
        out_len: *usize,
    ): bool! {
        let e: usize = 0;
        while (e < _.edge_len) {
            const edge = _.edges[e];
            if (edge.from_class_idx == class_idx) {
                if (edge.target_kind == CssComposeTargetKind.GlobalRaw) {
                    if (out_len >= out_tokens.len) {
                        return err.BufferTooSmall;
                    }
                    out_tokens[out_len] = edge.global_name;
                    out_len += 1;
                } else {
                    const t_idx = edge.target_class_idx;
                    const bit = @as(u64, 1) << @as(u6, @intCast(t_idx));
                    if ((visited_mask & bit) == 0) {
                        visited_mask |= bit;
                        * = _.visitComposesDfs(t_idx, *visited_mask, out_tokens, *out_len)?;
                        if (out_len >= out_tokens.len) {
                            return err.BufferTooSmall;
                        }
                        out_tokens[out_len] = _.classes[t_idx].mangled_name;
                        out_len += 1;
                    }
                }
            }
            e += 1;
        }
        return true;
    }

    /// 递归展开指定类的全部 `composes` 依赖（防环去重），最后追加自身 `mangled_name`
    fn expandClassTokens(class_idx: usize, out_tokens: [][]const u8): usize! {
        if (class_idx >= _.class_len) {
            return err.MissingComposeTarget;
        }
        let visited_mask: u64 = @as(u64, 1) << @as(u6, @intCast(class_idx));
        let out_len: usize = 0;
        if (_.classes[class_idx].is_single_class) {
            * = _.visitComposesDfs(class_idx, *visited_mask, out_tokens, *out_len)?;
        }
        if (out_len >= out_tokens.len) {
            return err.BufferTooSmall;
        }
        out_tokens[out_len] = _.classes[class_idx].mangled_name;
        out_len += 1;
        return out_len;
    }

    /// 将展开后的类名列表按空格拼接为最终 CSS Modules 导出值字符串
    fn formatExpandedClassExpr(class_idx: usize, out: []u8): usize! {
        let tokens: [LAZY_EXPORT_MAX_RESOLVED_TOKENS][]const u8 = [""; LAZY_EXPORT_MAX_RESOLVED_TOKENS];
        const count = _.expandClassTokens(class_idx, tokens[0..])?;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < count) {
            if (i > 0) {
                * = writeByte(out, *pos, ' ')?;
            }
            * = writeSlice(out, *pos, tokens[i])?;
            i += 1;
        }
        return pos;
    }
}
