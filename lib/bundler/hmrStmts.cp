// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/convertStmtsForChunkForDevServer.cp | Synced: 2026-11-22T14:00:00Z
// Pure-CP zero-allocation Bun Dev Server / Bake HMR Statement Converter,
// Packed Import Array Encoder & `import.meta.hot` Event/Call Contract Validator.
// Aligned with Bun `src/bundler/linker_context/convertStmtsForChunkForDevServer.cp`
// and `docs/bundler/hot-reloading.mdx`:
// - CommonJS chunks copy all statements directly to `inside_wrapper_suffix`.
// - ESM chunks disable CSS loader imports, turn barrel-optimized unused imports into
//   empty namespace stubs (`var <ns> = {};`), rewrite builtin/bun/runtime imports into
//   `hmr.builtin(...)` / `hmr.require("bun:wrap")`, and pack live user ESM imports into
//   `outside_wrapper_prefix` + `var [...] = hmr.imports;` + `hmr.updateImport = [...];`.
// - Bare imports preserve slot alignment via missing array holes and `() => {}` no-op callbacks.
// - Validates `bun:*` / `vite:*` HMR built-in events and direct `import.meta.hot.<api>` calls.

/// 单个 Dev Server Chunk 最大语句数
pub const HMR_MAX_STMTS: usize = 64;
/// 单个 Dev Server Chunk 最大导入记录数
pub const HMR_MAX_IMPORTS: usize = 32;
/// 单条 ESM import 语句支持的最大具名导入项数
pub const HMR_MAX_NAMED_ITEMS: usize = 16;

/// HMR 语句转换与序列化错误集
pub err HmrStmtsError {
    TooManyStmts,
    TooManyImports,
    TooManyNamedItems,
    InvalidImportRecordIndex,
    EmptyNamespaceIdent,
    EmptyResolvedPath,
    BufferTooSmall,
}

/// 导入记录来源分类（对齐 Bun `ImportRecord.tag`）
pub enum HmrImportTag: u8 {
    UserModule = 0,
    Builtin = 1,
    Bun = 2,
    Runtime = 3,
    Css = 4,
}

/// 模块导入记录（零堆分配视图）
pub struct HmrImportRecord {
    resolved_path: []const u8 = "",
    tag: HmrImportTag = .UserModule,
    is_disabled: bool = false,
    /// Barrel 优化标记：未使用的延迟子模块（不加入依赖数组，但保留空对象声明以防运行时抛错）
    is_unused: bool = false,
    /// 指向 `.css` loader 的导入记录在 JS dev chunk 中会被自动禁用
    is_css_loader: bool = false,
}

/// 输入语句类别
pub enum HmrStmtKind: u8 {
    Import = 0,
    Body = 1,
}

/// 待转换的单条语句输入描述符
pub struct HmrStmtInput {
    kind: HmrStmtKind = .Body,
    stmt_id: u32 = 0,
    import_record_index: usize = 0,
    namespace_ident: []const u8 = "",
    has_star_name: bool = false,
    has_default_name: bool = false,
    named_item_count: usize = 0,
    named_items: [HMR_MAX_NAMED_ITEMS][]const u8 = [""; HMR_MAX_NAMED_ITEMS],
    body_code: []const u8 = "",

    /// 判断是否为裸导入（无 `* as ns`、无 default、无具名导入项）
    fn isBareImport(): bool {
        return !_.has_star_name && _.named_item_count == 0 && !_.has_default_name;
    }

    /// 向 Import 语句追加一个具名导入符号
    fn addNamedItem(*_, name: []const u8): usize! {
        if (_.named_item_count >= HMR_MAX_NAMED_ITEMS) {
            return err.TooManyNamedItems;
        }
        const idx = _.named_item_count;
        _.named_items[idx] = name;
        _.named_item_count += 1;
        return idx;
    }
}

/// `inside_wrapper_prefix` 产出的声明类型
pub enum HmrPrefixDeclKind: u8 {
    /// Barrel 优化未用子模块：`var <ns> = {};`
    UnusedEmptyObj = 0,
    /// 内置模块导入：`var <ns> = <hmr>.builtin("<path>");`
    BuiltinImport = 1,
    /// 运行时辅助模块：`var <ns> = <hmr>.require("bun:wrap");`
    RuntimeRequire = 2,
    /// ESM 解构声明：`var [<ns1>, , <ns3>] = <hmr>.imports;`
    EsmImportsDestructure = 3,
    /// HMR 更新回调数组：`<hmr>.updateImport = [(module) => (<ns1> = module), () => {}, ...];`
    EsmUpdateImportArray = 4,
}

/// 单条 `inside_wrapper_prefix` 声明项
pub struct HmrPrefixDecl {
    kind: HmrPrefixDeclKind = .UnusedEmptyObj,
    namespace_ident: []const u8 = "",
    target_path: []const u8 = "",
    /// 对齐 Bun：当 `esm_callbacks.len <= 2` 时单行输出
    is_single_line: bool = true,
}

/// 单个 ESM 依赖槽位（对齐 `esm_decls` 与 `esm_callbacks`）
pub struct HmrEsmBindingSlot {
    is_bare: bool = false,
    namespace_ident: []const u8 = "",
    stmt_index: usize = 0,
    import_record_index: usize = 0,
}

/// Dev Server HMR 语句转换选项
pub struct HmrConvertOptions {
    /// CommonJS 模块：所有语句原样进入 `inside_wrapper_suffix`
    is_commonjs: bool = false,
    /// 是否为异步模块（顶层 await）
    is_async_module: bool = false,
    /// HMR API 包装参数名（默认 `"hmr"`）
    hmr_api_ident: []const u8 = "hmr",
    /// 更新回调参数名（默认 `"module"`）
    module_param_ident: []const u8 = "module",
}

/// 转换统计摘要
pub struct HmrConvertStats {
    outside_prefix_count: usize = 0,
    inside_prefix_count: usize = 0,
    inside_suffix_count: usize = 0,
    css_disabled_count: usize = 0,
    unused_stub_count: usize = 0,
    builtin_binding_count: usize = 0,
    esm_binding_count: usize = 0,
    bare_import_count: usize = 0,
    is_commonjs: bool = false,
    is_async_module: bool = false,
}

/// Bun / Vite 兼容的 HMR 内置事件枚举（对齐 `/data/dev/bun/docs/bundler/hot-reloading.mdx`）
pub enum HmrBuiltinEvent: u8 {
    BeforeUpdate = 0,
    AfterUpdate = 1,
    BeforeFullReload = 2,
    BeforePrune = 3,
    Invalidate = 4,
    Error = 5,
    WsDisconnect = 6,
    WsConnect = 7,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    for (i of 0..prefix.len) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
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

fn copyUsize(dst: []u8, offset: usize, val: usize): usize! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return copyByte(dst, offset, '0');
    }
    while (v > 0) {
        tmp[n] = ((v % 10) + 48) as u8;
        v = v / 10;
        n += 1;
    }
    let pos = offset;
    while (n > 0) {
        n -= 1;
        pos = copyByte(dst, pos, tmp[n])?;
    }
    return pos;
}

fn copyJsQuoted(dst: []u8, offset: usize, s: []const u8, quote: u8): usize! {
    let pos = copyByte(dst, offset, quote)?;
    for (i of 0..s.len) {
        const ch = s[i];
        if (ch == quote || ch == '\\') {
            pos = copyByte(dst, pos, '\\')?;
            pos = copyByte(dst, pos, ch)?;
        } else if (ch == '\n') {
            pos = copyInto(dst, pos, "\\n")?;
        } else if (ch == '\r') {
            pos = copyInto(dst, pos, "\\r")?;
        } else {
            pos = copyByte(dst, pos, ch)?;
        }
    }
    return copyByte(dst, pos, quote);
}

/// 解析 `bun:*` 或 `vite:*` 前缀的内置 HMR 事件名（对齐 Bun HMR 文档）
pub fn parseHmrBuiltinEvent(name: []const u8): HmrBuiltinEvent? {
    let suffix: []const u8 = "";
    if (startsWith(name, "bun:")) {
        suffix = name[4..];
    } else if (startsWith(name, "vite:")) {
        suffix = name[5..];
    } else {
        return null;
    }

    if (bytesEql(suffix, "beforeUpdate")) return .BeforeUpdate;
    if (bytesEql(suffix, "afterUpdate")) return .AfterUpdate;
    if (bytesEql(suffix, "beforeFullReload")) return .BeforeFullReload;
    if (bytesEql(suffix, "beforePrune")) return .BeforePrune;
    if (bytesEql(suffix, "invalidate")) return .Invalidate;
    if (bytesEql(suffix, "error")) return .Error;
    if (bytesEql(suffix, "ws:disconnect")) return .WsDisconnect;
    if (bytesEql(suffix, "ws:connect")) return .WsConnect;
    return null;
}

/// 返回标准内置 HMR 事件名（支持 `bun:*` 或 `vite:*` 兼容前缀）
pub fn hmrBuiltinEventName(ev: HmrBuiltinEvent, vite_prefix: bool): []const u8 {
    if (vite_prefix) {
        return switch (ev) {
            .BeforeUpdate => "vite:beforeUpdate",
            .AfterUpdate => "vite:afterUpdate",
            .BeforeFullReload => "vite:beforeFullReload",
            .BeforePrune => "vite:beforePrune",
            .Invalidate => "vite:invalidate",
            .Error => "vite:error",
            .WsDisconnect => "vite:ws:disconnect",
            .WsConnect => "vite:ws:connect",
        };
    }
    return switch (ev) {
        .BeforeUpdate => "bun:beforeUpdate",
        .AfterUpdate => "bun:afterUpdate",
        .BeforeFullReload => "bun:beforeFullReload",
        .BeforePrune => "bun:beforePrune",
        .Invalidate => "bun:invalidate",
        .Error => "bun:error",
        .WsDisconnect => "bun:ws:disconnect",
        .WsConnect => "bun:ws:connect",
    };
}

/// 校验表达式是否为满足 Bun 生产死代码消除约束的直调 `import.meta.hot.<API>` 形式
/// （Bun 禁止 `const hot = import.meta.hot; hot.accept()` 等间接引用调用）
pub fn isDirectImportMetaHotCall(expr: []const u8): bool {
    const prefix = "import.meta.hot.";
    if (!startsWith(expr, prefix)) return false;
    const rest = expr[prefix.len..];
    if (rest.len == 0) return false;

    let end: usize = 0;
    while (end < rest.len) {
        const c = rest[end];
        const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (!is_alpha) break;
        end += 1;
    }
    if (end == 0) return false;
    const member = rest[0..end];

    // `data` 允许作为属性访问/传递（如 `doSomething(import.meta.hot.data)`）
    if (bytesEql(member, "data")) {
        return end == rest.len || rest[end] == '.' || rest[end] == '[' || rest[end] == ' ';
    }

    // 其余 API 必须直接跟 `(` 调用
    const is_api = bytesEql(member, "accept") ||
        bytesEql(member, "dispose") ||
        bytesEql(member, "prune") ||
        bytesEql(member, "decline") ||
        bytesEql(member, "on") ||
        bytesEql(member, "off") ||
        bytesEql(member, "invalidate") ||
        bytesEql(member, "send");
    if (!is_api) return false;
    return end < rest.len && rest[end] == '(';
}

/// 零堆分配 Bun Dev Server / HMR Chunk 语句转换器与紧凑导入编码器
pub struct HmrChunkStmtConverter {
    imports: [HMR_MAX_IMPORTS]HmrImportRecord = [HmrImportRecord{}; HMR_MAX_IMPORTS],
    import_count: usize = 0,

    stmts: [HMR_MAX_STMTS]HmrStmtInput = [HmrStmtInput{}; HMR_MAX_STMTS],
    stmt_count: usize = 0,

    outside_prefix: [HMR_MAX_STMTS]usize = [0; HMR_MAX_STMTS],
    outside_prefix_count: usize = 0,

    inside_prefix: [HMR_MAX_STMTS]HmrPrefixDecl = [HmrPrefixDecl{}; HMR_MAX_STMTS],
    inside_prefix_count: usize = 0,

    inside_suffix: [HMR_MAX_STMTS]usize = [0; HMR_MAX_STMTS],
    inside_suffix_count: usize = 0,

    esm_slots: [HMR_MAX_IMPORTS]HmrEsmBindingSlot = [HmrEsmBindingSlot{}; HMR_MAX_IMPORTS],
    esm_slot_count: usize = 0,

    last_stats: HmrConvertStats = HmrConvertStats{},

    static fn init(): HmrChunkStmtConverter {
        return HmrChunkStmtConverter{};
    }

    /// 注册一条导入记录，返回其索引
    fn addImportRecord(*_, rec: HmrImportRecord): usize! {
        if (_.import_count >= HMR_MAX_IMPORTS) {
            return err.TooManyImports;
        }
        const idx = _.import_count;
        _.imports[idx] = rec;
        _.import_count += 1;
        return idx;
    }

    /// 追加一条待转换的语句，返回其索引
    fn addStmt(*_, stmt: HmrStmtInput): usize! {
        if (_.stmt_count >= HMR_MAX_STMTS) {
            return err.TooManyStmts;
        }
        const idx = _.stmt_count;
        _.stmts[idx] = stmt;
        _.stmt_count += 1;
        return idx;
    }

    /// 读取指定索引的导入记录（可用于验证 CSS loader 自动禁用状态）
    fn getImportRecord(idx: usize): HmrImportRecord? {
        if (idx >= _.import_count) return null;
        return _.imports[idx];
    }

    /// 返回 `outside_wrapper_prefix` 中的原始语句索引切片
    fn outsidePrefixIndices(): []const usize {
        return _.outside_prefix[0.._.outside_prefix_count];
    }

    /// 返回 `inside_wrapper_prefix` 生成的声明切片
    fn insidePrefixDecls(): []const HmrPrefixDecl {
        return _.inside_prefix[0.._.inside_prefix_count];
    }

    /// 返回 `inside_wrapper_suffix` 中的原始语句索引切片
    fn insideSuffixIndices(): []const usize {
        return _.inside_suffix[0.._.inside_suffix_count];
    }

    /// 返回 ESM 依赖槽位切片
    fn esmSlots(): []const HmrEsmBindingSlot {
        return _.esm_slots[0.._.esm_slot_count];
    }

    /// 执行 Dev Server HMR 语句拆分与重写（对齐 `convertStmtsForChunkForDevServer`）
    fn convert(*_, opts: HmrConvertOptions): HmrConvertStats! {
        _.outside_prefix_count = 0;
        _.inside_prefix_count = 0;
        _.inside_suffix_count = 0;
        _.esm_slot_count = 0;

        let stats = HmrConvertStats{
            is_commonjs: opts.is_commonjs,
            is_async_module: opts.is_async_module,
        };

        // 1. CommonJS 模块：所有语句直接进入 `inside_wrapper_suffix`
        if (opts.is_commonjs) {
            for (s_idx of 0.._.stmt_count) {
                _.inside_suffix[_.inside_suffix_count] = s_idx;
                _.inside_suffix_count += 1;
            }
            stats.inside_suffix_count = _.inside_suffix_count;
            _.last_stats = stats;
            return stats;
        }

        // 2. 预处理导入记录：在 Dev Server JS Chunk 中自动禁用 CSS loader 导入
        for (r_idx of 0.._.import_count) {
            if (!_.imports[r_idx].is_disabled) {
                if (_.imports[r_idx].is_css_loader || _.imports[r_idx].tag == .Css) {
                    _.imports[r_idx].is_disabled = true;
                    stats.css_disabled_count += 1;
                }
            }
        }

        // 3. 分类重写每条语句
        for (i of 0.._.stmt_count) {
            const stmt = _.stmts[i];
            if (stmt.kind != .Import) {
                _.inside_suffix[_.inside_suffix_count] = i;
                _.inside_suffix_count += 1;
                continue;
            }

            if (stmt.import_record_index >= _.import_count) {
                return err.InvalidImportRecordIndex;
            }
            const rec = _.imports[stmt.import_record_index];
            if (rec.is_disabled) {
                continue;
            }

            const is_bare = stmt.isBareImport();

            // Barrel 优化：延迟/未使用的子模块不加入依赖数组，但若有绑定则声明空对象防止运行时抛错
            if (rec.is_unused) {
                if (!is_bare) {
                    if (stmt.namespace_ident.len == 0) {
                        return err.EmptyNamespaceIdent;
                    }
                    if (_.inside_prefix_count >= HMR_MAX_STMTS) {
                        return err.TooManyStmts;
                    }
                    _.inside_prefix[_.inside_prefix_count] = HmrPrefixDecl{
                        kind: .UnusedEmptyObj,
                        namespace_ident: stmt.namespace_ident,
                    };
                    _.inside_prefix_count += 1;
                    stats.unused_stub_count += 1;
                }
                continue;
            }

            const is_builtin = rec.tag == .Builtin || rec.tag == .Bun || rec.tag == .Runtime;
            if (is_builtin) {
                if (!is_bare) {
                    if (stmt.namespace_ident.len == 0) {
                        return err.EmptyNamespaceIdent;
                    }
                    if (rec.tag != .Runtime && rec.resolved_path.len == 0) {
                        return err.EmptyResolvedPath;
                    }
                    if (_.inside_prefix_count >= HMR_MAX_STMTS) {
                        return err.TooManyStmts;
                    }
                    _.inside_prefix[_.inside_prefix_count] = HmrPrefixDecl{
                        kind: if (rec.tag == .Runtime) .RuntimeRequire else .BuiltinImport,
                        namespace_ident: stmt.namespace_ident,
                        target_path: if (rec.tag == .Runtime) "bun:wrap" else rec.resolved_path,
                    };
                    _.inside_prefix_count += 1;
                    stats.builtin_binding_count += 1;
                }
                continue;
            }

            // 普通 ESM 模块导入：加入 `esm_slots` 与 `outside_wrapper_prefix`
            if (_.esm_slot_count >= HMR_MAX_IMPORTS) {
                return err.TooManyImports;
            }
            if (!is_bare && stmt.namespace_ident.len == 0) {
                return err.EmptyNamespaceIdent;
            }
            if (rec.resolved_path.len == 0) {
                return err.EmptyResolvedPath;
            }

            _.esm_slots[_.esm_slot_count] = HmrEsmBindingSlot{
                is_bare: is_bare,
                namespace_ident: if (is_bare) "" else stmt.namespace_ident,
                stmt_index: i,
                import_record_index: stmt.import_record_index,
            };
            _.esm_slot_count += 1;
            if (is_bare) {
                stats.bare_import_count += 1;
            }

            _.outside_prefix[_.outside_prefix_count] = i;
            _.outside_prefix_count += 1;
        }

        // 4. 若存在 ESM 依赖槽位，追加解构声明与 `hmr.updateImport` 回调数组
        if (_.esm_slot_count > 0) {
            if (_.inside_prefix_count + 2 > HMR_MAX_STMTS) {
                return err.TooManyStmts;
            }
            _.inside_prefix[_.inside_prefix_count] = HmrPrefixDecl{
                kind: .EsmImportsDestructure,
                is_single_line: true,
            };
            _.inside_prefix_count += 1;

            _.inside_prefix[_.inside_prefix_count] = HmrPrefixDecl{
                kind: .EsmUpdateImportArray,
                is_single_line: _.esm_slot_count <= 2,
            };
            _.inside_prefix_count += 1;
        }

        stats.outside_prefix_count = _.outside_prefix_count;
        stats.inside_prefix_count = _.inside_prefix_count;
        stats.inside_suffix_count = _.inside_suffix_count;
        stats.esm_binding_count = _.esm_slot_count;
        _.last_stats = stats;
        return stats;
    }

    /// 序列化 Bun HMR 紧凑导入数组格式：
    /// `['module_1', 1, "add", 'module_2', 2, "mul", "div", 'module_3', 0]`
    /// （bare import 或 `import * as ns` 编码数量为 `0`）
    fn formatPackedImports(out: []u8): usize! {
        let pos: usize = 0;
        pos = copyByte(out, pos, '[')?;

        for (idx of 0.._.outside_prefix_count) {
            if (idx > 0) {
                pos = copyInto(out, pos, ", ")?;
            }
            const stmt = _.stmts[_.outside_prefix[idx]];
            const rec = _.imports[stmt.import_record_index];
            pos = copyJsQuoted(out, pos, rec.resolved_path, '\'')?;
            pos = copyInto(out, pos, ", ")?;

            if (stmt.isBareImport() || stmt.has_star_name) {
                pos = copyByte(out, pos, '0')?;
            } else {
                const total_items = stmt.named_item_count + (if (stmt.has_default_name) 1 else 0);
                pos = copyUsize(out, pos, total_items)?;
                if (stmt.has_default_name) {
                    pos = copyInto(out, pos, ", \"default\"")?;
                }
                for (k of 0..stmt.named_item_count) {
                    pos = copyInto(out, pos, ", ")?;
                    pos = copyJsQuoted(out, pos, stmt.named_items[k], '"')?;
                }
            }
        }

        pos = copyByte(out, pos, ']')?;
        return pos;
    }

    /// 序列化全部 `inside_wrapper_prefix` 生成代码（用于 HMR 包装函数头部）
    fn formatInsideWrapperPrefix(opts: HmrConvertOptions, out: []u8): usize! {
        const hmr_id = if (opts.hmr_api_ident.len > 0) opts.hmr_api_ident else "hmr";
        const mod_id = if (opts.module_param_ident.len > 0) opts.module_param_ident else "module";

        let pos: usize = 0;
        for (idx of 0.._.inside_prefix_count) {
            const decl = _.inside_prefix[idx];
            switch (decl.kind) {
                .UnusedEmptyObj => {
                    pos = copyInto(out, pos, "var ")?;
                    pos = copyInto(out, pos, decl.namespace_ident)?;
                    pos = copyInto(out, pos, " = {};\n")?;
                },
                .BuiltinImport => {
                    pos = copyInto(out, pos, "var ")?;
                    pos = copyInto(out, pos, decl.namespace_ident)?;
                    pos = copyInto(out, pos, " = ")?;
                    pos = copyInto(out, pos, hmr_id)?;
                    pos = copyInto(out, pos, ".builtin(")?;
                    pos = copyJsQuoted(out, pos, decl.target_path, '"')?;
                    pos = copyInto(out, pos, ");\n")?;
                },
                .RuntimeRequire => {
                    pos = copyInto(out, pos, "var ")?;
                    pos = copyInto(out, pos, decl.namespace_ident)?;
                    pos = copyInto(out, pos, " = ")?;
                    pos = copyInto(out, pos, hmr_id)?;
                    pos = copyInto(out, pos, ".require(")?;
                    pos = copyJsQuoted(out, pos, decl.target_path, '"')?;
                    pos = copyInto(out, pos, ");\n")?;
                },
                .EsmImportsDestructure => {
                    pos = copyInto(out, pos, "var [")?;
                    for (s of 0.._.esm_slot_count) {
                        if (s > 0) {
                            pos = copyInto(out, pos, ", ")?;
                        }
                        if (!_.esm_slots[s].is_bare) {
                            pos = copyInto(out, pos, _.esm_slots[s].namespace_ident)?;
                        }
                    }
                    pos = copyInto(out, pos, "] = ")?;
                    pos = copyInto(out, pos, hmr_id)?;
                    pos = copyInto(out, pos, ".imports;\n")?;
                },
                .EsmUpdateImportArray => {
                    pos = copyInto(out, pos, hmr_id)?;
                    pos = copyInto(out, pos, ".updateImport = [")?;
                    if (!decl.is_single_line) {
                        pos = copyByte(out, pos, '\n')?;
                    }
                    for (s of 0.._.esm_slot_count) {
                        if (!decl.is_single_line) {
                            pos = copyInto(out, pos, "  ")?;
                        } else if (s > 0) {
                            pos = copyInto(out, pos, ", ")?;
                        }

                        if (_.esm_slots[s].is_bare) {
                            pos = copyInto(out, pos, "() => {}")?;
                        } else {
                            pos = copyByte(out, pos, '(')?;
                            pos = copyInto(out, pos, mod_id)?;
                            pos = copyInto(out, pos, ") => (")?;
                            pos = copyInto(out, pos, _.esm_slots[s].namespace_ident)?;
                            pos = copyInto(out, pos, " = ")?;
                            pos = copyInto(out, pos, mod_id)?;
                            pos = copyByte(out, pos, ')')?;
                        }

                        if (!decl.is_single_line) {
                            pos = copyInto(out, pos, ",\n")?;
                        }
                    }
                    pos = copyInto(out, pos, "];\n")?;
                },
            }
        }
        return pos;
    }

    /// 序列化全部 `inside_wrapper_suffix` 主体语句
    fn formatInsideWrapperSuffix(out: []u8): usize! {
        let pos: usize = 0;
        for (idx of 0.._.inside_suffix_count) {
            const stmt = _.stmts[_.inside_suffix[idx]];
            if (stmt.body_code.len > 0) {
                pos = copyInto(out, pos, stmt.body_code)?;
                pos = copyByte(out, pos, '\n')?;
            }
        }
        return pos;
    }
}
