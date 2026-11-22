// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/computeChunks.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/bundler/linker_context/computeChunks.cp & /data/dev/bun/docs/bundler/index.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Bundler Chunk Planning & Deterministic Ordering Engine:
// - Allocates JS / CSS / HTML entry chunks from entry points with u64 entry_bits reachability masks
// - Deduplicates CSS chunks by ordered CSS dependency hash (css_chunking=true) or entry_bits (css_chunking=false)
// - Buckets reachable live files into shared chunks under code_splitting=true, or fans out to every owning entry chunk under code_splitting=false
// - Propagates is_browser_chunk_from_server_build for server-side fullstack builds with HTML imports
// - Sorts JS chunks deterministically, interleaves paired HTML chunks, sorts CSS chunks by hash and remaps JS->CSS indices
// - Generates deterministic `<hex>C<08d>` chunk unique keys and selects entry/chunk path template kinds.

/// 单次规划支持的最大入口点数量（受 u64 位掩码与栈上零分配容量约束）
pub const CHUNK_PLAN_MAX_ENTRIES: usize = 32;

/// 单次规划支持的最大源文件数量
pub const CHUNK_PLAN_MAX_FILES: usize = 64;

/// 单次规划输出的最大 Chunk 总数（JS + CSS + HTML）
pub const CHUNK_PLAN_MAX_CHUNKS: usize = 64;

/// 单个 Chunk 内最多记录的源文件数量
pub const CHUNK_PLAN_MAX_FILES_PER_CHUNK: usize = 64;

/// 单个入口最多关联的有序 CSS 依赖数量
pub const CHUNK_PLAN_MAX_CSS_DEPS: usize = 16;

/// 无效索引哨兵值（对应 Bun `std.math.maxInt(u32)` / `Index.invalid`）
pub const CHUNK_PLAN_INVALID_INDEX: u32 = 0xFFFFFFFF;

/// Chunk 唯一键固定缓冲区容量（16 位 hex + 'C' + 10 位十进制足够容纳任意 u64 + u32）
pub const CHUNK_PLAN_KEY_CAP: usize = 32;

/// Chunk 规划器错误集
pub err ChunkPlanError {
    TooManyEntries,
    TooManyFiles,
    TooManyChunks,
    TooManyFilesInChunk,
    TooManyCssDeps,
    InvalidSourceIndex,
    DuplicateEntryPoint,
    DuplicateFile,
}

/// Chunk 输出内容类别
pub enum ChunkPlanContentKind: u8 {
    Javascript = 0,
    Css = 1,
    Html = 2,

    /// 返回对应的默认文件扩展名（不含前导点）
    fn ext(): []const u8 {
        return switch (_) {
            .Javascript => "js",
            .Css => "css",
            .Html => "html",
        };
    }
}

/// 输入文件的 Loader 类别
pub enum ChunkPlanLoaderKind: u8 {
    Js = 0,
    Css = 1,
    Html = 2,
}

/// 入口点来源类型（对应 Bun `EntryPoint.Kind`）
pub enum ChunkPlanEntryKind: u8 {
    /// 用户在 `entrypoints` 中显式指定的入口
    UserSpecified = 0,
    /// 通过动态 `import()` 自动分裂出的入口
    DynamicImport = 1,
    /// Server Component 边界入口
    ServerComponent = 2,
}

/// 打包执行目标（对应 Bun `options.Target`）
pub enum ChunkPlanTarget: u8 {
    Browser = 0,
    Bun = 1,
    Node = 2,
    BunMacro = 3,
    BakeSsr = 4,

    /// 是否为服务端执行目标
    fn isServerSide(): bool {
        return switch (_) {
            .Bun, .Node, .BunMacro, .BakeSsr => true,
            .Browser => false,
        };
    }

    /// 返回 `[target]` 模板占位符对应的规范标签
    fn asLabel(): []const u8 {
        return switch (_) {
            .Browser => "browser",
            .Bun => "bun",
            .Node => "node",
            .BunMacro => "macro",
            .BakeSsr => "ssr",
        };
    }
}

/// Chunk 路径命名模板类型（对应 Bun `PathTemplate.file` / `fileWithTarget` / `chunk` / `chunkWithTarget`）
pub enum ChunkPlanTemplateKind: u8 {
    EntryFile = 0,
    EntryFileWithTarget = 1,
    SharedChunk = 2,
    SharedChunkWithTarget = 3,

    /// 模板是否需要 `[target]` 占位符
    fn needsTarget(): bool {
        return _ == .EntryFileWithTarget || _ == .SharedChunkWithTarget;
    }
}

/// Chunk 规划配置选项
pub struct ChunkPlanOptions {
    /// 是否启用代码分割（`splitting: true`）
    code_splitting: bool = false,
    /// 是否按有序 CSS 导入列表哈希去重共享 CSS Chunk（`cssChunking`）
    css_chunking: bool = true,
    /// 顶层打包目标
    target: ChunkPlanTarget = .Browser,
    /// 图中是否包含 HTML 入口导入（用于服务端构建识别浏览器伴生 Chunk）
    has_html_imports: bool = false,
    /// 图中是否包含服务端 HTML 导入（决定默认是否附加 `[target]` 命名模板）
    has_server_html_imports: bool = false,
    /// 是否用户自定义了 `entry_naming`
    has_custom_entry_naming: bool = false,
    /// 是否用户自定义了 `chunk_naming`
    has_custom_chunk_naming: bool = false,
    /// 本次构建的唯一键种子（对应 Bun `unique_key: u64`）
    unique_key: u64 = 0,
}

/// 参与 Chunk 规划的单个源文件描述符
pub struct ChunkPlanFileInput {
    source_index: u32 = 0,
    loader: ChunkPlanLoaderKind = .Js,
    target: ChunkPlanTarget = .Browser,
    entry_kind: ChunkPlanEntryKind = .UserSpecified,
    is_live: bool = true,
    /// 该文件被哪些入口点可达（第 i 位对应 entry_point_id == i）
    entry_bits: u64 = 0,
    /// 该入口按 JS 顺序导入的 CSS 源文件列表（纯 CSS 入口亦可填入自身与递归 `@import` 列表）
    css_dep_count: usize = 0,
    css_deps: [CHUNK_PLAN_MAX_CSS_DEPS]u32 = [0; CHUNK_PLAN_MAX_CSS_DEPS],
    /// 入口输出文件基名（如 `"index"`、`"app"`）
    output_base_name: []const u8 = "",

    fn isCss(): bool {
        return _.loader == .Css;
    }

    fn isHtml(): bool {
        return _.loader == .Html;
    }

    fn cssDepsSlice(): []const u32 {
        return _.css_deps[0.._.css_dep_count];
    }

    fn addCssDep(*_, dep_source_index: u32): ChunkPlanError!bool {
        if (_.css_dep_count >= CHUNK_PLAN_MAX_CSS_DEPS) {
            return ChunkPlanError.TooManyCssDeps;
        }
        _.css_deps[_.css_dep_count] = dep_source_index;
        _.css_dep_count += 1;
        return true;
    }
}

/// 零堆分配 Chunk 唯一键（格式：`<hex_lower>C<08d>`）
pub struct ChunkPlanUniqueKey {
    buf: [CHUNK_PLAN_KEY_CAP]u8 = [0; CHUNK_PLAN_KEY_CAP],
    len: usize = 0,
    prefix_len: usize = 0,

    fn asSlice(): []const u8 {
        return _.buf[0.._.len];
    }

    fn prefix(): []const u8 {
        return _.buf[0.._.prefix_len];
    }

    fn eql(other: []const u8): bool {
        if (_.len != other.len) { return false; }
        let i: usize = 0;
        while (i < _.len) {
            if (_.buf[i] != other[i]) { return false; }
            i += 1;
        }
        return true;
    }
}

/// 规划输出的单个 Chunk 描述符
pub struct PlannedChunk {
    chunk_id: u32 = 0,
    content: ChunkPlanContentKind = .Javascript,
    entry_point_id: u32 = 0,
    source_index: u32 = 0,
    is_entry_point: bool = false,
    entry_bits: u64 = 0,
    has_html_chunk: bool = false,
    is_browser_chunk_from_server_build: bool = false,
    /// 若 JS Chunk 关联了 CSS Chunk，此处为排序重映射后的最终 CSS Chunk 索引；否则为 `CHUNK_PLAN_INVALID_INDEX`
    css_chunk_index: u32 = CHUNK_PLAN_INVALID_INDEX,
    /// CSS Chunk 的确定性去重键哈希（仅 `content == .Css` 时有意义）
    css_hash: u64 = 0,
    template_kind: ChunkPlanTemplateKind = .EntryFile,
    target_label: []const u8 = "browser",
    base_name: []const u8 = "",
    ext: []const u8 = "js",
    unique_key: ChunkPlanUniqueKey = ChunkPlanUniqueKey{},
    file_count: usize = 0,
    files: [CHUNK_PLAN_MAX_FILES_PER_CHUNK]u32 = [0; CHUNK_PLAN_MAX_FILES_PER_CHUNK],

    fn filesSlice(): []const u32 {
        return _.files[0.._.file_count];
    }

    fn containsFile(src_idx: u32): bool {
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i] == src_idx) { return true; }
            i += 1;
        }
        return false;
    }

    fn addFileUnique(*_, src_idx: u32): ChunkPlanError!bool {
        if (_.containsFile(src_idx)) { return false; }
        if (_.file_count >= CHUNK_PLAN_MAX_FILES_PER_CHUNK) {
            return ChunkPlanError.TooManyFilesInChunk;
        }
        _.files[_.file_count] = src_idx;
        _.file_count += 1;
        return true;
    }
}

/// 反向映射条目：入口源文件索引 -> 最终 Chunk 索引
pub struct EntryChunkMapping {
    source_index: u32 = 0,
    chunk_index: u32 = CHUNK_PLAN_INVALID_INDEX,
}

/// Chunk 规划完整结果（全栈上分配，零堆分配）
pub struct ChunkPlanResult {
    chunk_count: usize = 0,
    chunks: [CHUNK_PLAN_MAX_CHUNKS]PlannedChunk = [PlannedChunk{}; CHUNK_PLAN_MAX_CHUNKS],
    js_chunk_count: usize = 0,
    css_chunk_count: usize = 0,
    html_chunk_count: usize = 0,
    entry_map_count: usize = 0,
    entry_map: [CHUNK_PLAN_MAX_FILES]EntryChunkMapping = [EntryChunkMapping{}; CHUNK_PLAN_MAX_FILES],

    fn slice(): []const PlannedChunk {
        return _.chunks[0.._.chunk_count];
    }

    /// 根据入口 `source_index` 查询其对应的最终 Chunk 索引
    fn findChunkForEntrySource(src_idx: u32): u32? {
        let i: usize = 0;
        while (i < _.entry_map_count) {
            if (_.entry_map[i].source_index == src_idx) {
                return _.entry_map[i].chunk_index;
            }
            i += 1;
        }
        return null;
    }

    /// 根据 `unique_key` 精确查找 Chunk
    fn findByUniqueKey(key: []const u8): PlannedChunk? {
        let i: usize = 0;
        while (i < _.chunk_count) {
            if (_.chunks[i].unique_key.eql(key)) {
                return _.chunks[i];
            }
            i += 1;
        }
        return null;
    }
}

/// 计算有序 CSS 依赖列表的 64 位 FNV-1a 确定性哈希（对应 Bun `css_chunking` 去重键）
pub fn hashCssOrderKey(deps: []const u32): u64 {
    let h: u64 = 14695981039346656037;
    const prime: u64 = 1099511628211;

    // 混入长度防止前缀碰撞
    h = (h ^ @as(u64, deps.len)) *% prime;
    let i: usize = 0;
    while (i < deps.len) {
        const v = @as(u64, deps[i]);
        h = (h ^ (v & 0xFF)) *% prime;
        h = (h ^ ((v >> 8) & 0xFF)) *% prime;
        h = (h ^ ((v >> 16) & 0xFF)) *% prime;
        h = (h ^ ((v >> 24) & 0xFF)) *% prime;
        i += 1;
    }
    return h;
}

/// 格式化 Chunk 唯一键：`<hex_lower>C<08d>`（严格对齐 Bun `computeChunks.cp` 格式）
pub fn formatChunkUniqueKey(unique_key: u64, chunk_id: u32): ChunkPlanUniqueKey {
    const HEX = "0123456789abcdef";
    let out = ChunkPlanUniqueKey{};
    let pos: usize = 0;

    // 1. 小写十六进制编码 unique_key（无前导零，0 输出 "0"）
    let hex_tmp: [16]u8 = [0; 16];
    let hex_len: usize = 0;
    if (unique_key == 0) {
        hex_tmp[0] = '0';
        hex_len = 1;
    } else {
        let v = unique_key;
        while (v > 0 && hex_len < 16) {
            hex_tmp[hex_len] = HEX[@as(usize, @intCast(v & 0xF))];
            v = v >> 4;
            hex_len += 1;
        }
    }
    let hi: usize = hex_len;
    while (hi > 0) {
        hi -= 1;
        out.buf[pos] = hex_tmp[hi];
        pos += 1;
    }
    out.prefix_len = pos;

    // 2. 分隔符 'C'
    out.buf[pos] = 'C';
    pos += 1;

    // 3. 十进制零填充至至少 8 位
    let dec_tmp: [12]u8 = [0; 12];
    let dec_len: usize = 0;
    let cid = chunk_id;
    if (cid == 0) {
        dec_tmp[0] = '0';
        dec_len = 1;
    } else {
        while (cid > 0 && dec_len < 12) {
            dec_tmp[dec_len] = @as(u8, @intCast(cid % 10)) + '0';
            cid = cid / 10;
            dec_len += 1;
        }
    }
    let pad: usize = 0;
    if (dec_len < 8) {
        pad = @as(usize, 8) - dec_len;
    }
    let pi: usize = 0;
    while (pi < pad && pos < CHUNK_PLAN_KEY_CAP) {
        out.buf[pos] = '0';
        pos += 1;
        pi += 1;
    }
    let di: usize = dec_len;
    while (di > 0 && pos < CHUNK_PLAN_KEY_CAP) {
        di -= 1;
        out.buf[pos] = dec_tmp[di];
        pos += 1;
    }

    out.len = pos;
    return out;
}

/// 内部桶键（对应 Bun `JSChunkKeyFormatter`：non_splitting 含 has_html 前缀位，splitting 仅用 entry_bits）
struct JsBucketKey {
    non_html_flag: u8 = 1,
    entry_bits: u64 = 0,

    fn eql(other: JsBucketKey): bool {
        return _.non_html_flag == other.non_html_flag && _.entry_bits == other.entry_bits;
    }

    fn lessThan(other: JsBucketKey): bool {
        if (_.non_html_flag != other.non_html_flag) {
            return _.non_html_flag < other.non_html_flag;
        }
        return _.entry_bits < other.entry_bits;
    }
}

/// 零堆分配 Bundler Chunk 规划器
pub struct ChunkPlanner {
    file_count: usize = 0,
    files: [CHUNK_PLAN_MAX_FILES]ChunkPlanFileInput = [ChunkPlanFileInput{}; CHUNK_PLAN_MAX_FILES],
    entry_count: usize = 0,
    entry_file_slots: [CHUNK_PLAN_MAX_ENTRIES]usize = [0; CHUNK_PLAN_MAX_ENTRIES],

    static fn init(): ChunkPlanner {
        return ChunkPlanner{};
    }

    fn findFileSlot(src_idx: u32): usize? {
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i].source_index == src_idx) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 注册源文件节点，返回内部槽位索引
    fn addFile(*_, file: ChunkPlanFileInput): ChunkPlanError!usize {
        if (_.findFileSlot(file.source_index) != null) {
            return ChunkPlanError.DuplicateFile;
        }
        if (_.file_count >= CHUNK_PLAN_MAX_FILES) {
            return ChunkPlanError.TooManyFiles;
        }
        const slot = _.file_count;
        _.files[slot] = file;
        _.file_count += 1;
        return slot;
    }

    /// 将已注册文件声明为入口点，返回分配的 `entry_point_id`（0-based）
    fn addEntryPoint(*_, src_idx: u32): ChunkPlanError!u32 {
        const slot = _.findFileSlot(src_idx) ?? return ChunkPlanError.InvalidSourceIndex;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entry_file_slots[i] == slot) {
                return ChunkPlanError.DuplicateEntryPoint;
            }
            i += 1;
        }
        if (_.entry_count >= CHUNK_PLAN_MAX_ENTRIES) {
            return ChunkPlanError.TooManyEntries;
        }
        const entry_id = @as(u32, @intCast(_.entry_count));
        _.entry_file_slots[_.entry_count] = slot;
        _.entry_count += 1;
        return entry_id;
    }

    /// 为指定源文件追加一条有序 CSS 依赖记录
    fn attachCssDep(*_, src_idx: u32, css_src_idx: u32): ChunkPlanError!bool {
        const slot = _.findFileSlot(src_idx) ?? return ChunkPlanError.InvalidSourceIndex;
        return _.files[slot].addCssDep(css_src_idx);
    }

    /// 设置指定源文件的入口可达位掩码
    fn setFileEntryBits(*_, src_idx: u32, bits: u64): ChunkPlanError!bool {
        const slot = _.findFileSlot(src_idx) ?? return ChunkPlanError.InvalidSourceIndex;
        _.files[slot].entry_bits = bits;
        return true;
    }

    /// 执行完整的 Chunk 规划、分桶、确定性排序、CSS 索引重映射、唯一键生成与模板选择
    fn computeChunks(opts: ChunkPlanOptions): ChunkPlanError!ChunkPlanResult {
        let out = ChunkPlanResult{};
        if (_.entry_count == 0) {
            return out;
        }

        // 复制一份局部入口位掩码表，避免修改只读 planner
        let local_bits: [CHUNK_PLAN_MAX_FILES]u64 = [0; CHUNK_PLAN_MAX_FILES];
        let fi: usize = 0;
        while (fi < _.file_count) {
            local_bits[fi] = _.files[fi].entry_bits;
            fi += 1;
        }

        let js_keys: [CHUNK_PLAN_MAX_CHUNKS]JsBucketKey = [JsBucketKey{}; CHUNK_PLAN_MAX_CHUNKS];
        let js_chunks: [CHUNK_PLAN_MAX_CHUNKS]PlannedChunk = [PlannedChunk{}; CHUNK_PLAN_MAX_CHUNKS];
        let js_count: usize = 0;

        let css_hashes: [CHUNK_PLAN_MAX_CHUNKS]u64 = [0; CHUNK_PLAN_MAX_CHUNKS];
        let css_chunks: [CHUNK_PLAN_MAX_CHUNKS]PlannedChunk = [PlannedChunk{}; CHUNK_PLAN_MAX_CHUNKS];
        let css_count: usize = 0;

        let html_keys: [CHUNK_PLAN_MAX_CHUNKS]JsBucketKey = [JsBucketKey{}; CHUNK_PLAN_MAX_CHUNKS];
        let html_chunks: [CHUNK_PLAN_MAX_CHUNKS]PlannedChunk = [PlannedChunk{}; CHUNK_PLAN_MAX_CHUNKS];
        let html_consumed: [CHUNK_PLAN_MAX_CHUNKS]bool = [false; CHUNK_PLAN_MAX_CHUNKS];
        let html_count: usize = 0;

        let entry_to_js_idx: [CHUNK_PLAN_MAX_ENTRIES]u32 = [CHUNK_PLAN_INVALID_INDEX; CHUNK_PLAN_MAX_ENTRIES];

        const could_be_browser_from_server = opts.target.isServerSide() && opts.has_html_imports;

        // 阶段 1：为每个入口点创建 HTML / CSS / JS 入口 Chunk
        let eid: usize = 0;
        while (eid < _.entry_count) {
            const slot = _.entry_file_slots[eid];
            const f = _.files[slot];
            const entry_bit_mask: u64 = @as(u64, 1) << @as(u6, @intCast(eid));
            local_bits[slot] = local_bits[slot] | entry_bit_mask;

            const has_html = f.isHtml();
            const is_browser_from_srv = could_be_browser_from_server && f.target == .Browser;

            const bucket_key = if (opts.code_splitting)
                JsBucketKey{ non_html_flag: 0, entry_bits: entry_bit_mask }
            else
                JsBucketKey{
                    non_html_flag: if (has_html) 0 else 1,
                    entry_bits: local_bits[slot],
                };

            // 1a. HTML 入口 Chunk
            if (has_html) {
                let found_html = false;
                let hi: usize = 0;
                while (hi < html_count) {
                    if (html_keys[hi].eql(bucket_key)) {
                        found_html = true;
                        break;
                    }
                    hi += 1;
                }
                if (!found_html) {
                    if (js_count + css_count + html_count >= CHUNK_PLAN_MAX_CHUNKS) {
                        return ChunkPlanError.TooManyChunks;
                    }
                    html_keys[html_count] = bucket_key;
                    html_chunks[html_count] = PlannedChunk{
                        content: .Html,
                        entry_point_id: @as(u32, @intCast(eid)),
                        source_index: f.source_index,
                        is_entry_point: true,
                        entry_bits: entry_bit_mask,
                        is_browser_chunk_from_server_build: is_browser_from_srv,
                    };
                    html_count += 1;
                }
            }

            // 1b. 纯 CSS 入口 Chunk
            if (f.isCss()) {
                let css_key: u64 = 0;
                if (!opts.css_chunking) {
                    css_key = local_bits[slot];
                } else if (f.css_dep_count > 0) {
                    css_key = hashCssOrderKey(f.cssDepsSlice());
                } else {
                    const self_arr: [1]u32 = [f.source_index];
                    css_key = hashCssOrderKey(self_arr[0..]);
                }

                let found_css = false;
                let ci: usize = 0;
                while (ci < css_count) {
                    if (css_hashes[ci] == css_key) {
                        found_css = true;
                        break;
                    }
                    ci += 1;
                }
                if (!found_css) {
                    if (js_count + css_count + html_count >= CHUNK_PLAN_MAX_CHUNKS) {
                        return ChunkPlanError.TooManyChunks;
                    }
                    css_hashes[css_count] = css_key;
                    let c_chunk = PlannedChunk{
                        content: .Css,
                        entry_point_id: @as(u32, @intCast(eid)),
                        source_index: f.source_index,
                        is_entry_point: true,
                        entry_bits: entry_bit_mask,
                        has_html_chunk: has_html,
                        is_browser_chunk_from_server_build: is_browser_from_srv,
                        css_hash: css_key,
                    };
                    if (f.css_dep_count > 0) {
                        let di: usize = 0;
                        while (di < f.css_dep_count) {
                            * = c_chunk.addFileUnique(f.css_deps[di])?;
                            di += 1;
                        }
                    } else {
                        * = c_chunk.addFileUnique(f.source_index)?;
                    }
                    css_chunks[css_count] = c_chunk;
                    css_count += 1;
                }
                eid += 1;
                continue;
            }

            // 1c. JS 入口 Chunk
            let js_idx: usize = js_count;
            let ji: usize = 0;
            while (ji < js_count) {
                if (js_keys[ji].eql(bucket_key)) {
                    js_idx = ji;
                    break;
                }
                ji += 1;
            }
            if (js_idx == js_count) {
                if (js_count + css_count + html_count >= CHUNK_PLAN_MAX_CHUNKS) {
                    return ChunkPlanError.TooManyChunks;
                }
                js_keys[js_count] = bucket_key;
                js_count += 1;
            }
            entry_to_js_idx[eid] = @as(u32, @intCast(js_idx));
            js_chunks[js_idx] = PlannedChunk{
                content: .Javascript,
                entry_point_id: @as(u32, @intCast(eid)),
                source_index: f.source_index,
                is_entry_point: true,
                entry_bits: entry_bit_mask,
                has_html_chunk: has_html,
                is_browser_chunk_from_server_build: is_browser_from_srv,
            };

            // 1d. 若 JS/HTML 入口关联了 CSS 依赖，生成或复用伴生 CSS Chunk
            if (f.css_dep_count > 0) {
                const css_key = if (opts.css_chunking)
                    hashCssOrderKey(f.cssDepsSlice())
                else
                    local_bits[slot];

                let found_css_idx: usize = css_count;
                let ci: usize = 0;
                while (ci < css_count) {
                    if (css_hashes[ci] == css_key) {
                        found_css_idx = ci;
                        break;
                    }
                    ci += 1;
                }
                if (found_css_idx == css_count) {
                    if (js_count + css_count + html_count >= CHUNK_PLAN_MAX_CHUNKS) {
                        return ChunkPlanError.TooManyChunks;
                    }
                    css_hashes[css_count] = css_key;
                    let c_chunk = PlannedChunk{
                        content: .Css,
                        entry_point_id: @as(u32, @intCast(eid)),
                        source_index: f.source_index,
                        is_entry_point: true,
                        entry_bits: local_bits[slot],
                        has_html_chunk: has_html,
                        is_browser_chunk_from_server_build: is_browser_from_srv,
                        css_hash: css_key,
                    };
                    let di: usize = 0;
                    while (di < f.css_dep_count) {
                        * = c_chunk.addFileUnique(f.css_deps[di])?;
                        di += 1;
                    }
                    css_chunks[css_count] = c_chunk;
                    css_count += 1;
                }
                js_chunks[js_idx].css_chunk_index = @as(u32, @intCast(found_css_idx));
            }

            eid += 1;
        }

        // 阶段 2：将可达的活非 CSS 文件分桶至 JS Chunk（splitting 按精确 entry_bits 聚桶；non-splitting 扇出到各置位入口）
        if (js_count > 0) {
            let s_idx: usize = 0;
            while (s_idx < _.file_count) {
                const f = _.files[s_idx];
                const f_bits = local_bits[s_idx];
                if (f.is_live && !f.isCss() && f_bits != 0) {
                    if (opts.code_splitting) {
                        const key = JsBucketKey{ non_html_flag: 0, entry_bits: f_bits };
                        let found_idx: usize = js_count;
                        let ji: usize = 0;
                        while (ji < js_count) {
                            if (js_keys[ji].eql(key)) {
                                found_idx = ji;
                                break;
                            }
                            ji += 1;
                        }
                        if (found_idx == js_count) {
                            if (js_count + css_count + html_count >= CHUNK_PLAN_MAX_CHUNKS) {
                                return ChunkPlanError.TooManyChunks;
                            }
                            const is_browser_from_srv = could_be_browser_from_server && f.target == .Browser;
                            js_keys[js_count] = key;
                            js_chunks[js_count] = PlannedChunk{
                                content: .Javascript,
                                entry_point_id: 0,
                                source_index: f.source_index,
                                is_entry_point: false,
                                entry_bits: f_bits,
                                is_browser_chunk_from_server_build: is_browser_from_srv,
                            };
                            js_count += 1;
                        } else if (could_be_browser_from_server &&
                            !js_chunks[found_idx].is_entry_point &&
                            !js_chunks[found_idx].is_browser_chunk_from_server_build &&
                            f.target == .Browser)
                        {
                            js_chunks[found_idx].is_browser_chunk_from_server_build = true;
                        }
                        * = js_chunks[found_idx].addFileUnique(f.source_index)?;
                    } else {
                        let bit_i: usize = 0;
                        while (bit_i < _.entry_count) {
                            const mask: u64 = @as(u64, 1) << @as(u6, @intCast(bit_i));
                            if ((f_bits & mask) != 0) {
                                const target_js = entry_to_js_idx[bit_i];
                                if (target_js != CHUNK_PLAN_INVALID_INDEX) {
                                    * = js_chunks[@as(usize, target_js)].addFileUnique(f.source_index)?;
                                }
                            }
                            bit_i += 1;
                        }
                    }
                }
                s_idx += 1;
            }
        }

        // 阶段 3：确定性排序 JS Chunks（主入口 entry_point_id==0 优先，其余按 bucket key 升序）
        let js_order: [CHUNK_PLAN_MAX_CHUNKS]usize = [0; CHUNK_PLAN_MAX_CHUNKS];
        let oi: usize = 0;
        while (oi < js_count) {
            js_order[oi] = oi;
            oi += 1;
        }
        // 插入排序保证确定性与稳定性
        let i_sort: usize = 1;
        while (i_sort < js_count) {
            const cur = js_order[i_sort];
            let j_sort = i_sort;
            while (j_sort > 0) {
                const prev = js_order[j_sort - 1];
                const a_is_primary = js_chunks[cur].is_entry_point && js_chunks[cur].entry_point_id == 0;
                const b_is_primary = js_chunks[prev].is_entry_point && js_chunks[prev].entry_point_id == 0;
                let cur_before_prev = false;
                if (a_is_primary && !b_is_primary) {
                    cur_before_prev = true;
                } else if (!a_is_primary && b_is_primary) {
                    cur_before_prev = false;
                } else {
                    cur_before_prev = js_keys[cur].lessThan(js_keys[prev]);
                }
                if (!cur_before_prev) { break; }
                js_order[j_sort] = prev;
                j_sort -= 1;
            }
            js_order[j_sort] = cur;
            i_sort += 1;
        }

        // 输出排序后的 JS Chunks，并紧跟配对的 HTML Chunk
        let js_with_css_slots: [CHUNK_PLAN_MAX_CHUNKS]usize = [0; CHUNK_PLAN_MAX_CHUNKS];
        let js_with_css_count: usize = 0;

        let jo: usize = 0;
        while (jo < js_count) {
            const idx = js_order[jo];
            const chunk = js_chunks[idx];
            const out_pos = out.chunk_count;
            if (chunk.css_chunk_index != CHUNK_PLAN_INVALID_INDEX) {
                js_with_css_slots[js_with_css_count] = out_pos;
                js_with_css_count += 1;
            }
            out.chunks[out.chunk_count] = chunk;
            out.chunk_count += 1;
            out.js_chunk_count += 1;

            if (chunk.has_html_chunk) {
                let hi: usize = 0;
                while (hi < html_count) {
                    if (!html_consumed[hi] && html_keys[hi].eql(js_keys[idx])) {
                        out.chunks[out.chunk_count] = html_chunks[hi];
                        out.chunk_count += 1;
                        out.html_chunk_count += 1;
                        html_consumed[hi] = true;
                        break;
                    }
                    hi += 1;
                }
            }
            jo += 1;
        }

        // 阶段 4：CSS Chunks 按 css_hash 升序排序并追加，同时重映射 JS Chunk 的 css_chunk_index
        if (css_count > 0) {
            let css_order: [CHUNK_PLAN_MAX_CHUNKS]usize = [0; CHUNK_PLAN_MAX_CHUNKS];
            let ci: usize = 0;
            while (ci < css_count) {
                css_order[ci] = ci;
                ci += 1;
            }
            let cs: usize = 1;
            while (cs < css_count) {
                const cur = css_order[cs];
                let cj = cs;
                while (cj > 0 && css_hashes[cur] < css_hashes[css_order[cj - 1]]) {
                    css_order[cj] = css_order[cj - 1];
                    cj -= 1;
                }
                css_order[cj] = cur;
                cs += 1;
            }

            let remapped_css: [CHUNK_PLAN_MAX_CHUNKS]u32 = [CHUNK_PLAN_INVALID_INDEX; CHUNK_PLAN_MAX_CHUNKS];
            let co: usize = 0;
            while (co < css_count) {
                const orig_idx = css_order[co];
                const new_idx = @as(u32, @intCast(out.chunk_count));
                out.chunks[out.chunk_count] = css_chunks[orig_idx];
                out.chunk_count += 1;
                out.css_chunk_count += 1;
                remapped_css[orig_idx] = new_idx;
                co += 1;
            }

            let jc: usize = 0;
            while (jc < js_with_css_count) {
                const slot_pos = js_with_css_slots[jc];
                const old_css_idx = out.chunks[slot_pos].css_chunk_index;
                if (old_css_idx < @as(u32, @intCast(css_count))) {
                    out.chunks[slot_pos].css_chunk_index = remapped_css[@as(usize, old_css_idx)];
                }
                jc += 1;
            }
        }

        // 追加剩余未配对的 HTML Chunks
        let hr: usize = 0;
        while (hr < html_count) {
            if (!html_consumed[hr]) {
                out.chunks[out.chunk_count] = html_chunks[hr];
                out.chunk_count += 1;
                out.html_chunk_count += 1;
            }
            hr += 1;
        }

        // 阶段 5：建立入口源文件 -> 最终 Chunk 索引反向映射、生成唯一键与选择路径模板
        let cid: usize = 0;
        while (cid < out.chunk_count) {
            const c_id_u32 = @as(u32, @intCast(cid));
            out.chunks[cid].chunk_id = c_id_u32;
            out.chunks[cid].ext = out.chunks[cid].content.ext();
            out.chunks[cid].unique_key = formatChunkUniqueKey(opts.unique_key, c_id_u32);

            const src_idx = out.chunks[cid].source_index;
            let file_is_css = false;
            let file_entry_kind = ChunkPlanEntryKind.UserSpecified;
            let file_target = opts.target;
            let file_base_name: []const u8 = "";

            if (fslot := _.findFileSlot(src_idx)) {
                const f = _.files[fslot];
                file_is_css = f.isCss();
                file_entry_kind = f.entry_kind;
                file_target = f.target;
                file_base_name = f.output_base_name;
            }

            // 若当前为入口 Chunk，且不是非 CSS 入口的伴生 CSS Chunk，则写入反向映射
            if (out.chunks[cid].is_entry_point) {
                if (!(out.chunks[cid].content == .Css && !file_is_css)) {
                    let updated = false;
                    let mi: usize = 0;
                    while (mi < out.entry_map_count) {
                        if (out.entry_map[mi].source_index == src_idx) {
                            out.entry_map[mi].chunk_index = c_id_u32;
                            updated = true;
                            break;
                        }
                        mi += 1;
                    }
                    if (!updated && out.entry_map_count < CHUNK_PLAN_MAX_FILES) {
                        out.entry_map[out.entry_map_count] = EntryChunkMapping{
                            source_index: src_idx,
                            chunk_index: c_id_u32,
                        };
                        out.entry_map_count += 1;
                    }
                }
            }

            // 模板类型选择（对齐 Bun computeChunks.cp 第 425-449 行）
            const is_entry_tpl = out.chunks[cid].is_entry_point &&
                (out.chunks[cid].content == .Html ||
                    (file_entry_kind == .UserSpecified && !out.chunks[cid].has_html_chunk));
            if (is_entry_tpl) {
                if (opts.has_server_html_imports && !opts.has_custom_entry_naming) {
                    out.chunks[cid].template_kind = .EntryFileWithTarget;
                } else {
                    out.chunks[cid].template_kind = .EntryFile;
                }
            } else {
                if (opts.has_server_html_imports && !opts.has_custom_chunk_naming) {
                    out.chunks[cid].template_kind = .SharedChunkWithTarget;
                } else {
                    out.chunks[cid].template_kind = .SharedChunk;
                }
            }

            out.chunks[cid].target_label = file_target.asLabel();
            out.chunks[cid].base_name = file_base_name;
            cid += 1;
        }

        return out;
    }
}
