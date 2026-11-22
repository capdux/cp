// Source: Absorbed from /data/dev/bun/src/bundler/Graph.cp | Synced: 2026-11-18T22:07:30Z
// Pure-CP zero-allocation Bundler Module Graph & Two-Phase Scan/Defer Coordinator
// Aligned with Bun src/bundler/Graph.cp & docs/bundler/plugins.mdx:
// - Deduplicates input files into stable dense `source_index` slots with loader & flag tracking
// - Maps entry points to original virtual/specifier names (`entry_point_original_names`)
// - Tracks server->HTML client-build pairs (`HtmlImports`), CSS file counts, file-loader estimates, and secondary paths
// - Coordinates asynchronous scan + `onLoad` `.defer()` counters (`pending_items` / `deferred_pending`):
//     * Enforces Bun's rule that `.defer()` can only be called once per `onLoad` callback
//     * Automatically wakes deferred plugin tasks when `pending_items` hits 0 and transitions to `ReadyToLink` once both hit 0.

/// 模块图最大输入文件槽位数
pub const GRAPH_MAX_INPUT_FILES: usize = 64;

/// 模块图最大入口点数
pub const GRAPH_MAX_ENTRY_POINTS: usize = 16;

/// 模块图最大服务端->HTML 导入对数
pub const GRAPH_MAX_HTML_IMPORTS: usize = 16;

/// 路径/虚拟入口名最大长度
pub const GRAPH_MAX_PATH_LEN: usize = 128;

/// Bundler 模块图与扫描协调器错误集
pub err GraphError {
    TooManyInputFiles,
    TooManyEntryPoints,
    TooManyHtmlImports,
    PathTooLong,
    EmptyPath,
    InvalidSourceIndex,
    DuplicateEntryPoint,
    PendingCounterUnderflow,
    PendingCounterOverflow,
    DeferAlreadyCalled,
    NoPendingTaskToDefer,
}

/// 输入模块 Loader 类型（用于预统计 CSS / File 数量）
pub enum GraphLoaderKind: u8 {
    Js = 0,
    Ts = 1,
    Jsx = 2,
    Tsx = 3,
    Css = 4,
    Html = 5,
    Json = 6,
    File = 7,
    Text = 8,
    Wasm = 9,
}

/// 扫描阶段计数器状态迁移结果
pub enum GraphScanTransition: u8 {
    /// 仍有活跃扫描/解析/插件任务在途
    Scanning = 0,
    /// `pending_items` 已归零但存在 `.defer()` 延迟插件任务，已自动唤醒进入新一轮扫描
    WakeDeferredPlugins = 1,
    /// `pending_items == 0 && deferred_pending == 0`，扫描阶段完成，可进入 Linker
    ReadyToLink = 2,
}

/// 输入文件标志位（对齐 Bun `InputFileFlags` + `.defer()` 单次调用守卫）
pub struct GraphInputFileFlags {
    is_plugin_file: bool = false,
    /// 当 barrel 文件通过 `export * from` 指向本文件时置位
    is_export_star_target: bool = false,
    /// 记录该模块的 `onLoad` 回调是否已调用过 `.defer()`（每回调仅允许一次）
    has_deferred_once: bool = false,
}

/// 单条输入文件元数据（对齐 Bun `Graph.InputFile`）
pub struct GraphInputFile {
    source_index: u32 = 0,
    path_buf: [GRAPH_MAX_PATH_LEN]u8 = [0; GRAPH_MAX_PATH_LEN],
    path_len: usize = 0,
    secondary_path_buf: [GRAPH_MAX_PATH_LEN]u8 = [0; GRAPH_MAX_PATH_LEN],
    secondary_path_len: usize = 0,
    loader: GraphLoaderKind = .Js,
    flags: GraphInputFileFlags = GraphInputFileFlags{},
    content_hash: u64 = 0,

    fn path(): []const u8 {
        return _.path_buf[0.._.path_len];
    }

    fn secondaryPath(): []const u8 {
        return _.secondary_path_buf[0.._.secondary_path_len];
    }
}

/// 入口点与原始说明符映射（对齐 Bun `entry_points` + `entry_point_original_names`）
pub struct GraphEntryPoint {
    source_index: u32 = 0,
    orig_name_buf: [GRAPH_MAX_PATH_LEN]u8 = [0; GRAPH_MAX_PATH_LEN],
    orig_name_len: usize = 0,

    fn originalName(): []const u8 {
        return _.orig_name_buf[0.._.orig_name_len];
    }
}

/// 服务端文件导入 HTML 文件映射对（对齐 Bun `HtmlImports`）
pub struct GraphHtmlImportPair {
    server_source_index: u32 = 0,
    html_source_index: u32 = 0,
}

/// 模块图扫描与预分配统计快照
pub struct GraphScanStats {
    input_file_count: usize = 0,
    entry_point_count: usize = 0,
    html_import_count: usize = 0,
    css_file_count: usize = 0,
    estimated_file_loader_count: usize = 0,
    has_any_secondary_paths: bool = false,
    pending_items: u32 = 0,
    deferred_pending: u32 = 0,
    deferred_rounds_completed: u32 = 0,
}

fn graphBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 零堆分配 Bundler 模块图与两阶段扫描/延迟插件协调器
pub struct BundlerGraph {
    input_files: [GRAPH_MAX_INPUT_FILES]GraphInputFile = [GraphInputFile{}; GRAPH_MAX_INPUT_FILES],
    input_file_count: usize = 0,
    entry_points: [GRAPH_MAX_ENTRY_POINTS]GraphEntryPoint = [GraphEntryPoint{}; GRAPH_MAX_ENTRY_POINTS],
    entry_point_count: usize = 0,
    html_imports: [GRAPH_MAX_HTML_IMPORTS]GraphHtmlImportPair = [GraphHtmlImportPair{}; GRAPH_MAX_HTML_IMPORTS],
    html_import_count: usize = 0,
    pending_items: u32 = 0,
    deferred_pending: u32 = 0,
    deferred_rounds_completed: u32 = 0,
    css_file_count: usize = 0,
    estimated_file_loader_count: usize = 0,
    has_any_secondary_paths: bool = false,
    kit_referenced_server_data: bool = false,
    kit_referenced_client_data: bool = false,

    static fn init(): BundlerGraph {
        return BundlerGraph{};
    }

    fn lookupSourceIndex(path: []const u8): u32? {
        let i: usize = 0;
        while (i < _.input_file_count) {
            if (graphBytesEql(_.input_files[i].path(), path)) {
                return _.input_files[i].source_index;
            }
            i += 1;
        }
        return null;
    }

    /// 添加或复用输入文件（同路径去重），自动维护 CSS/File 预统计与 secondary_path 标记
    fn addInputFile(
        *_,
        path: []const u8,
        secondary_path: []const u8,
        loader: GraphLoaderKind,
        is_plugin_file: bool,
        content_hash: u64,
    ): GraphError!u32 {
        if (path.len == 0) return err.EmptyPath;
        if (path.len > GRAPH_MAX_PATH_LEN || secondary_path.len > GRAPH_MAX_PATH_LEN) {
            return err.PathTooLong;
        }

        if (existing := _.lookupSourceIndex(path)) {
            return existing;
        }
        if (_.input_file_count >= GRAPH_MAX_INPUT_FILES) {
            return err.TooManyInputFiles;
        }

        const idx = _.input_file_count as u32;
        let f = GraphInputFile{};
        f.source_index = idx;
        f.path_len = path.len;
        let i: usize = 0;
        while (i < path.len) {
            f.path_buf[i] = path[i];
            i += 1;
        }
        f.secondary_path_len = secondary_path.len;
        i = 0;
        while (i < secondary_path.len) {
            f.secondary_path_buf[i] = secondary_path[i];
            i += 1;
        }
        f.loader = loader;
        f.flags.is_plugin_file = is_plugin_file;
        f.content_hash = content_hash;

        _.input_files[_.input_file_count] = f;
        _.input_file_count += 1;

        if (loader == .Css) {
            _.css_file_count += 1;
        } else if (loader == .File) {
            _.estimated_file_loader_count += 1;
        }
        if (secondary_path.len > 0) {
            _.has_any_secondary_paths = true;
        }
        return idx;
    }

    /// 标记目标模块为 barrel `export * from` 目标
    fn markExportStarTarget(*_, source_index: u32): GraphError!bool {
        if ((source_index as usize) >= _.input_file_count) return err.InvalidSourceIndex;
        _.input_files[source_index as usize].flags.is_export_star_target = true;
        return true;
    }

    /// 注册入口点及其原始说明符（用于虚拟插件入口映射）
    fn addEntryPoint(*_, source_index: u32, original_name: []const u8): GraphError!usize {
        if ((source_index as usize) >= _.input_file_count) return err.InvalidSourceIndex;
        if (original_name.len > GRAPH_MAX_PATH_LEN) return err.PathTooLong;

        let i: usize = 0;
        while (i < _.entry_point_count) {
            if (_.entry_points[i].source_index == source_index) return err.DuplicateEntryPoint;
            i += 1;
        }
        if (_.entry_point_count >= GRAPH_MAX_ENTRY_POINTS) return err.TooManyEntryPoints;

        let ep = GraphEntryPoint{};
        ep.source_index = source_index;
        ep.orig_name_len = original_name.len;
        i = 0;
        while (i < original_name.len) {
            ep.orig_name_buf[i] = original_name[i];
            i += 1;
        }
        _.entry_points[_.entry_point_count] = ep;
        _.entry_point_count += 1;
        return _.entry_point_count - 1;
    }

    /// 注册一条服务端模块导入客户端 HTML 入口的关联记录（对齐 Bun `HtmlImports`）
    fn addHtmlImport(*_, server_source_index: u32, html_source_index: u32): GraphError!usize {
        if ((server_source_index as usize) >= _.input_file_count ||
            (html_source_index as usize) >= _.input_file_count) {
            return err.InvalidSourceIndex;
        }
        if (_.html_import_count >= GRAPH_MAX_HTML_IMPORTS) return err.TooManyHtmlImports;

        _.html_imports[_.html_import_count] = GraphHtmlImportPair{
            server_source_index: server_source_index,
            html_source_index: html_source_index,
        };
        _.html_import_count += 1;
        return _.html_import_count - 1;
    }

    /// 递增活跃扫描任务计数（ParseTask / onResolve / onLoad）
    fn incrementScanCounter(*_): GraphError!u32 {
        if (_.pending_items >= 0xFFFF_FFFE) return err.PendingCounterOverflow;
        _.pending_items += 1;
        return _.pending_items;
    }

    /// 内部状态迁移：当 `pending_items == 0` 时自动唤醒延迟插件或宣告进入 Linker
    fn evaluateTransition(*_): GraphScanTransition {
        if (_.pending_items > 0) return .Scanning;
        if (_.deferred_pending > 0) {
            _.pending_items = _.deferred_pending;
            _.deferred_pending = 0;
            _.deferred_rounds_completed += 1;
            return .WakeDeferredPlugins;
        }
        return .ReadyToLink;
    }

    /// 插件 `onLoad` 调用 `.defer()`：每模块限调用一次，将计数从 `pending_items` 移入 `deferred_pending`
    fn deferTask(*_, source_index: u32): GraphError!GraphScanTransition {
        if ((source_index as usize) >= _.input_file_count) return err.InvalidSourceIndex;
        if (_.pending_items == 0) return err.NoPendingTaskToDefer;
        if (_.input_files[source_index as usize].flags.has_deferred_once) {
            return err.DeferAlreadyCalled;
        }

        _.input_files[source_index as usize].flags.has_deferred_once = true;
        _.pending_items -= 1;
        _.deferred_pending += 1;
        return _.evaluateTransition();
    }

    /// 递减活跃扫描任务计数，并在归零时返回准确的两阶段状态迁移
    fn decrementScanCounter(*_): GraphError!GraphScanTransition {
        if (_.pending_items == 0) return err.PendingCounterUnderflow;
        _.pending_items -= 1;
        return _.evaluateTransition();
    }

    fn getInputFile(source_index: u32): GraphInputFile? {
        if ((source_index as usize) >= _.input_file_count) return null;
        return _.input_files[source_index as usize];
    }

    fn getEntryOriginalName(source_index: u32): ([]const u8)? {
        let i: usize = 0;
        while (i < _.entry_point_count) {
            if (_.entry_points[i].source_index == source_index) {
                return _.entry_points[i].originalName();
            }
            i += 1;
        }
        return null;
    }

    fn getHtmlImport(idx: usize): GraphHtmlImportPair? {
        if (idx >= _.html_import_count) return null;
        return _.html_imports[idx];
    }

    fn stats(): GraphScanStats {
        return GraphScanStats{
            input_file_count: _.input_file_count,
            entry_point_count: _.entry_point_count,
            html_import_count: _.html_import_count,
            css_file_count: _.css_file_count,
            estimated_file_loader_count: _.estimated_file_loader_count,
            has_any_secondary_paths: _.has_any_secondary_paths,
            pending_items: _.pending_items,
            deferred_pending: _.deferred_pending,
            deferred_rounds_completed: _.deferred_rounds_completed,
        };
    }
}
