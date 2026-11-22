// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/findAllImportedPartsInJSOrder.cp | Synced: 2026-11-22T09:00:00Z
// Ported from Bun: /data/dev/bun/src/bundler/linker_context/findAllImportedPartsInJSOrder.cp & /data/dev/bun/docs/bundler/index.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation JS Chunk Part Ordering & Linearization Engine:
// - Sorts files inside a JS chunk by deterministic (distance_from_entry, stable_tie_breaker)
// - Runs cycle-safe DFS traversal from runtime + sorted chunk files (dependencies before dependents)
// - Coalesces adjacent live parts of the same module into compact PartRange spans
// - Places runtime parts and non-splittable CJS/ESM wrapper modules into parts_prefix before main part_ranges
// - Enforces code-splitting entry-bit equality vs non-splitting intersection and skips external dynamic import()
// - Records Server Component Boundary (SCB) owning chunk indices during traversal.

/// 单图支持的最大模块文件数量
pub const JS_ORDER_MAX_FILES: usize = 64;

/// 单文件支持的最大 AST 部件（Part）数量
pub const JS_ORDER_MAX_PARTS_PER_FILE: usize = 16;

/// 单部件支持的最大导入记录数量
pub const JS_ORDER_MAX_IMPORTS_PER_PART: usize = 8;

/// 单次 Chunk 线性化输出支持的最大 PartRange 区间数量
pub const JS_ORDER_MAX_RANGES: usize = 256;

/// 内置 Runtime 模块的保留源索引（对应 Bun `Index.runtime.value`）
pub const JS_ORDER_RUNTIME_SOURCE_INDEX: u32 = 0;

/// 无效源索引哨兵（对应 Bun `Index.invalid.value`）
pub const JS_ORDER_INVALID_SOURCE_INDEX: u32 = 0xFFFFFFFF;

/// 命名空间导出部件固定索引（对应 Bun `js_ast.namespace_export_part_index`）
pub const JS_ORDER_NAMESPACE_EXPORT_PART: u32 = 0;

/// JS Chunk 部件拓扑排序错误集
pub err JsOrderError {
    TooManyFiles,
    TooManyParts,
    TooManyImports,
    TooManyRanges,
    InvalidSourceIndex,
    InvalidPartIndex,
    DuplicateFile,
}

/// 模块包装模式（对应 Bun `JSMeta.Flags.wrap`）
pub enum JsPartWrapKind: u8 {
    /// 普通 ESM 模块，允许按 Part 粒度拆解合并
    None = 0,
    /// CommonJS 包装模块（`__commonJS`），不可拆分，所有 Part 必须连续置于 prefix
    Cjs = 1,
    /// ESM 包装模块（`__esm`），不可拆分，所有 Part 必须连续置于 prefix
    Esm = 2,
}

/// 部件级导入记录类别
pub enum JsOrderImportKind: u8 {
    /// 静态 `import` / `export from` 语句
    Stmt = 0,
    /// CommonJS `require()`
    Require = 1,
    /// 动态 `import()` 表达式
    DynamicImport = 2,
}

/// 单条部件级导入记录
pub struct JsOrderImportRecord {
    target_source_index: u32 = JS_ORDER_INVALID_SOURCE_INDEX,
    kind: JsOrderImportKind = .Stmt,
    /// 是否为外部动态导入（跨 chunk 或外置动态 import，DFS 不跟随展开）
    is_external_dynamic: bool = false,
}

/// 文件内的单个顶层语句部件（Part）
pub struct JsOrderPart {
    /// Tree-shaking 存活标记
    is_live: bool = true,
    /// 是否应当输出到最终 Chunk（对应 Bun `shouldIncludePart`）
    should_include: bool = true,
    imports: [JS_ORDER_MAX_IMPORTS_PER_PART]JsOrderImportRecord = [JsOrderImportRecord{}; JS_ORDER_MAX_IMPORTS_PER_PART],
    import_count: usize = 0,
}

/// 参与 JS Chunk 线性化排序的模块文件描述
pub struct JsOrderFile {
    source_index: u32 = JS_ORDER_INVALID_SOURCE_INDEX,
    distance_from_entry: u32 = 0,
    stable_tie_breaker: u32 = 0,
    /// 64 位入口可达位掩码（对应 Bun `AutoBitSet`）
    entry_bits: u64 = 0,
    wrap: JsPartWrapKind = .None,
    is_css: bool = false,
    /// 是否为 Server Component Boundary（SCB）边界模块
    is_scb: bool = false,
    /// 若为 SCB 模块，遍历期间自动回填其归属的 `chunk_index`
    entry_point_chunk_index: u32 = JS_ORDER_INVALID_SOURCE_INDEX,
    parts: [JS_ORDER_MAX_PARTS_PER_FILE]JsOrderPart = [JsOrderPart{}; JS_ORDER_MAX_PARTS_PER_FILE],
    part_count: usize = 0,
}

/// 连续部件区间 `[part_index_begin, part_index_end)`（精确对齐 Bun `PartRange`）
pub struct JsPartRange {
    source_index: u32 = 0,
    part_index_begin: u32 = 0,
    part_index_end: u32 = 0,

    fn len(): u32 {
        if (_.part_index_end <= _.part_index_begin) return 0;
        return _.part_index_end - _.part_index_begin;
    }

    fn canExtend(source_index: u32, part_index: u32): bool {
        return _.source_index == source_index && _.part_index_end == part_index;
    }
}

/// Chunk 内文件初始确定性排序键（精确对齐 Bun `Chunk.Order`）
pub struct JsChunkOrderEntry {
    source_index: u32 = 0,
    distance: u32 = 0,
    tie_breaker: u32 = 0,

    /// 严格弱序：先按距入口距离升序，再按稳定源索引平局破除
    static fn isBefore(a: JsChunkOrderEntry, b: JsChunkOrderEntry): bool {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }
        return a.tie_breaker < b.tie_breaker;
    }
}

/// JS Chunk 部件线性化结果（零动态堆分配）
pub struct JsOrderResult {
    files_in_chunk_order: [JS_ORDER_MAX_FILES]u32 = [0; JS_ORDER_MAX_FILES],
    file_count: usize = 0,
    /// 最终输出顺序：`parts_prefix[0..prefix_range_count]` 在前，常规 `part_ranges` 在后
    parts_in_chunk_in_order: [JS_ORDER_MAX_RANGES]JsPartRange = [JsPartRange{}; JS_ORDER_MAX_RANGES],
    part_range_count: usize = 0,
    prefix_range_count: usize = 0,

    fn filesSlice(): []const u32 {
        return _.files_in_chunk_order[0.._.file_count];
    }

    fn partRangesSlice(): []const JsPartRange {
        return _.parts_in_chunk_in_order[0.._.part_range_count];
    }

    fn prefixRangesSlice(): []const JsPartRange {
        return _.parts_in_chunk_in_order[0.._.prefix_range_count];
    }

    fn mainRangesSlice(): []const JsPartRange {
        return _.parts_in_chunk_in_order[_.prefix_range_count.._.part_range_count];
    }

    fn totalParts(): u32 {
        let total: u32 = 0;
        let i: usize = 0;
        while (i < _.part_range_count) {
            total += _.parts_in_chunk_in_order[i].len();
            i += 1;
        }
        return total;
    }
}

struct PartRangeBuffer {
    items: [JS_ORDER_MAX_RANGES]JsPartRange = [JsPartRange{}; JS_ORDER_MAX_RANGES],
    count: usize = 0,

    fn appendOrExtend(*_, source_index: u32, part_index: u32): ! {
        if (_.count > 0) {
            if (_.items[_.count - 1].canExtend(source_index, part_index)) {
                _.items[_.count - 1].part_index_end += 1;
                return;
            }
        }
        if (_.count >= JS_ORDER_MAX_RANGES) {
            return err.TooManyRanges;
        }
        _.items[_.count] = JsPartRange{
            source_index: source_index,
            part_index_begin: part_index,
            part_index_end: part_index + 1,
        };
        _.count += 1;
    }

    fn appendSpan(*_, source_index: u32, begin: u32, end: u32): ! {
        if (_.count >= JS_ORDER_MAX_RANGES) {
            return err.TooManyRanges;
        }
        _.items[_.count] = JsPartRange{
            source_index: source_index,
            part_index_begin: begin,
            part_index_end: end,
        };
        _.count += 1;
    }
}

/// 纯 CP 零堆分配 JS 模块部件依赖图与 Chunk 线性化排序器
pub struct JsOrderGraph {
    files: [JS_ORDER_MAX_FILES]JsOrderFile = [JsOrderFile{}; JS_ORDER_MAX_FILES],
    file_count: usize = 0,

    static fn init(): JsOrderGraph {
        return JsOrderGraph{};
    }

    /// 查找已注册 `source_index` 对应的内部槽位索引
    fn findSlotBySourceIndex(source_index: u32): usize? {
        if (source_index == JS_ORDER_INVALID_SOURCE_INDEX) return null;
        let i: usize = 0;
        while (i < _.file_count) {
            if (_.files[i].source_index == source_index) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 注册一个模块文件节点并返回其内部槽位索引
    fn addFile(
        *_,
        source_index: u32,
        distance_from_entry: u32,
        stable_tie_breaker: u32,
        entry_bits: u64,
        wrap: JsPartWrapKind,
    ): usize! {
        if (source_index == JS_ORDER_INVALID_SOURCE_INDEX) {
            return err.InvalidSourceIndex;
        }
        if (_.findSlotBySourceIndex(source_index) != null) {
            return err.DuplicateFile;
        }
        if (_.file_count >= JS_ORDER_MAX_FILES) {
            return err.TooManyFiles;
        }
        const slot = _.file_count;
        _.files[slot] = JsOrderFile{
            source_index: source_index,
            distance_from_entry: distance_from_entry,
            stable_tie_breaker: stable_tie_breaker,
            entry_bits: entry_bits,
            wrap: wrap,
            is_css: false,
            is_scb: false,
            entry_point_chunk_index: JS_ORDER_INVALID_SOURCE_INDEX,
            part_count: 0,
        };
        _.file_count += 1;
        return slot;
    }

    /// 配置模块附加标记（CSS 模块标记与 SCB 边界标记）
    fn setFileFlags(*_, slot: usize, is_css: bool, is_scb: bool): ! {
        if (slot >= _.file_count) {
            return err.InvalidSourceIndex;
        }
        _.files[slot].is_css = is_css;
        _.files[slot].is_scb = is_scb;
    }

    /// 为指定槽位文件追加一个 AST 部件（Part），返回其 `part_index`
    fn addPart(*_, slot: usize, is_live: bool, should_include: bool): u32! {
        if (slot >= _.file_count) {
            return err.InvalidSourceIndex;
        }
        if (_.files[slot].part_count >= JS_ORDER_MAX_PARTS_PER_FILE) {
            return err.TooManyParts;
        }
        const p_idx: u32 = @intCast(_.files[slot].part_count);
        _.files[slot].parts[_.files[slot].part_count] = JsOrderPart{
            is_live: is_live,
            should_include: should_include,
            import_count: 0,
        };
        _.files[slot].part_count += 1;
        return p_idx;
    }

    /// 为指定文件的指定部件追加一条导入记录
    fn addPartImport(
        *_,
        slot: usize,
        part_index: u32,
        target_source_index: u32,
        kind: JsOrderImportKind,
        is_external_dynamic: bool,
    ): ! {
        if (slot >= _.file_count) {
            return err.InvalidSourceIndex;
        }
        const p_usize: usize = @intCast(part_index);
        if (p_usize >= _.files[slot].part_count) {
            return err.InvalidPartIndex;
        }
        const ic = _.files[slot].parts[p_usize].import_count;
        if (ic >= JS_ORDER_MAX_IMPORTS_PER_PART) {
            return err.TooManyImports;
        }
        _.files[slot].parts[p_usize].imports[ic] = JsOrderImportRecord{
            target_source_index: target_source_index,
            kind: kind,
            is_external_dynamic: is_external_dynamic,
        };
        _.files[slot].parts[p_usize].import_count = ic + 1;
    }

    /// 查询 SCB 模块被回填的归属 Chunk 索引
    fn getScbChunkIndex(source_index: u32): u32? {
        if (slot := _.findSlotBySourceIndex(source_index)) {
            const idx = _.files[slot].entry_point_chunk_index;
            if (idx == JS_ORDER_INVALID_SOURCE_INDEX) return null;
            return idx;
        }
        return null;
    }

    fn visitSlot(
        *_,
        slot: usize,
        chunk_entry_bits: u64,
        chunk_index: u32,
        code_splitting: bool,
        with_scb: bool,
        visited: *[JS_ORDER_MAX_FILES]bool,
        files_out: *[JS_ORDER_MAX_FILES]u32,
        file_out_count: *usize,
        part_ranges: *PartRangeBuffer,
        parts_prefix: *PartRangeBuffer,
    ): ! {
        if (slot >= _.file_count) return;
        if (visited[slot]) return;
        visited[slot] = true;

        const source_index = _.files[slot].source_index;
        const file_entry_bits = _.files[slot].entry_bits;
        const is_css = _.files[slot].is_css;
        const can_be_split = _.files[slot].wrap == .None;
        const part_count = _.files[slot].part_count;

        let is_file_in_chunk: bool = false;
        if (code_splitting && !is_css) {
            // 代码分割模式下，仅当文件的所有入口位与当前 Chunk 完全一致时归属本 Chunk
            is_file_in_chunk = file_entry_bits == chunk_entry_bits;
        } else {
            // 非分割模式或 CSS 模块，只要入口位存在交集即归属本 Chunk
            is_file_in_chunk = (file_entry_bits & chunk_entry_bits) != 0;
        }

        // 若可拆分且 namespace_export_part (index 0) 存活，优先置于该文件常规区间首位
        if (can_be_split && is_file_in_chunk && part_count > 0 && _.files[slot].parts[JS_ORDER_NAMESPACE_EXPORT_PART].is_live) {
            part_ranges.appendOrExtend(source_index, JS_ORDER_NAMESPACE_EXPORT_PART)?;
        }

        let p_idx: usize = 0;
        while (p_idx < part_count) {
            const p_u32: u32 = @intCast(p_idx);
            const part_live = _.files[slot].parts[p_idx].is_live;
            const part_should_include = _.files[slot].parts[p_idx].should_include;
            const import_count = _.files[slot].parts[p_idx].import_count;
            const is_part_in_this_chunk = is_file_in_chunk && part_live;

            let r_idx: usize = 0;
            while (r_idx < import_count) {
                const rec = _.files[slot].parts[p_idx].imports[r_idx];
                if (rec.target_source_index != JS_ORDER_INVALID_SOURCE_INDEX &&
                    (rec.kind == .Stmt || is_part_in_this_chunk))
                {
                    // 外部动态 import() 不随静态依赖链展开
                    if (!rec.is_external_dynamic && rec.kind != .DynamicImport) {
                        if (target_slot := _.findSlotBySourceIndex(rec.target_source_index)) {
                            _.visitSlot(
                                target_slot,
                                chunk_entry_bits,
                                chunk_index,
                                code_splitting,
                                with_scb,
                                visited,
                                files_out,
                                file_out_count,
                                part_ranges,
                                parts_prefix,
                            )?;
                        }
                    }
                }
                r_idx += 1;
            }

            // 依赖模块递归完成后，再将当前部件加入输出区间
            if (is_part_in_this_chunk) {
                is_file_in_chunk = true;
                if (can_be_split && p_u32 != JS_ORDER_NAMESPACE_EXPORT_PART && part_should_include) {
                    if (source_index == JS_ORDER_RUNTIME_SOURCE_INDEX) {
                        parts_prefix.appendOrExtend(source_index, p_u32)?;
                    } else {
                        part_ranges.appendOrExtend(source_index, p_u32)?;
                    }
                }
            }

            p_idx += 1;
        }

        if (is_file_in_chunk) {
            if (with_scb && _.files[slot].is_scb) {
                _.files[slot].entry_point_chunk_index = chunk_index;
            }
            if (file_out_count.* < JS_ORDER_MAX_FILES) {
                files_out[file_out_count.*] = source_index;
                file_out_count.* += 1;
            }
            // CommonJS / ESM Wrapper 文件不可拆解，全部 Part 作为连续整体置于 prefix
            if (!can_be_split) {
                parts_prefix.appendSpan(source_index, 0, @intCast(part_count))?;
            }
        }
    }

    /// 对单个 JS Chunk 执行确定性部件与文件拓扑线性化（对齐 Bun `findImportedPartsInJSOrder`）
    fn computeChunkOrder(
        *_,
        chunk_entry_bits: u64,
        chunk_files: []const u32,
        chunk_index: u32,
        code_splitting: bool,
        with_scb: bool,
    ): JsOrderResult! {
        let order_list: [JS_ORDER_MAX_FILES]JsChunkOrderEntry = [JsChunkOrderEntry{}; JS_ORDER_MAX_FILES];
        let order_count: usize = 0;

        let i: usize = 0;
        while (i < chunk_files.len) {
            const src = chunk_files[i];
            if (slot := _.findSlotBySourceIndex(src)) {
                if (order_count >= JS_ORDER_MAX_FILES) {
                    return err.TooManyFiles;
                }
                order_list[order_count] = JsChunkOrderEntry{
                    source_index: src,
                    distance: _.files[slot].distance_from_entry,
                    tie_breaker: _.files[slot].stable_tie_breaker,
                };
                order_count += 1;
            } else {
                return err.InvalidSourceIndex;
            }
            i += 1;
        }

        // 稳定插入排序：按 (distance_from_entry, stable_tie_breaker) 升序
        let a: usize = 1;
        while (a < order_count) {
            const key = order_list[a];
            let b: usize = a;
            while (b > 0 && JsChunkOrderEntry.isBefore(key, order_list[b - 1])) {
                order_list[b] = order_list[b - 1];
                b -= 1;
            }
            order_list[b] = key;
            a += 1;
        }

        let visited: [JS_ORDER_MAX_FILES]bool = [false; JS_ORDER_MAX_FILES];
        let result = JsOrderResult{};
        let part_ranges = PartRangeBuffer{};
        let parts_prefix = PartRangeBuffer{};

        // 1. 优先访问内置 Runtime 模块（若已注册）
        if (rt_slot := _.findSlotBySourceIndex(JS_ORDER_RUNTIME_SOURCE_INDEX)) {
            _.visitSlot(
                rt_slot,
                chunk_entry_bits,
                chunk_index,
                code_splitting,
                with_scb,
                &visited,
                &result.files_in_chunk_order,
                &result.file_count,
                &part_ranges,
                &parts_prefix,
            )?;
        }

        // 2. 按确定性排序依次访问 Chunk 内各文件
        let k: usize = 0;
        while (k < order_count) {
            if (slot := _.findSlotBySourceIndex(order_list[k].source_index)) {
                _.visitSlot(
                    slot,
                    chunk_entry_bits,
                    chunk_index,
                    code_splitting,
                    with_scb,
                    &visited,
                    &result.files_in_chunk_order,
                    &result.file_count,
                    &part_ranges,
                    &parts_prefix,
                )?;
            }
            k += 1;
        }

        // 3. 合并 parts_prefix 与 part_ranges 到最终确定性输出列表
        if (parts_prefix.count + part_ranges.count > JS_ORDER_MAX_RANGES) {
            return err.TooManyRanges;
        }

        result.prefix_range_count = parts_prefix.count;
        let out_idx: usize = 0;
        let p: usize = 0;
        while (p < parts_prefix.count) {
            result.parts_in_chunk_in_order[out_idx] = parts_prefix.items[p];
            out_idx += 1;
            p += 1;
        }
        let m: usize = 0;
        while (m < part_ranges.count) {
            result.parts_in_chunk_in_order[out_idx] = part_ranges.items[m];
            out_idx += 1;
            m += 1;
        }
        result.part_range_count = out_idx;

        return result;
    }
}
