// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/OutputFileListBuilder.cp | Synced: 2026-11-22T04:28:11Z
// Pure-CP zero-allocation 3-segment bundler OutputFileList layout builder:
//   Segment 1: [0 .. chunk_count)                           -> Chunks (O(1) invariant: outputIndexForChunk(i) == i)
//   Segment 2: [chunk_count .. additional_start)            -> External sourcemaps, bytecode caches, ESM module_info
//   Segment 3: [additional_start .. total_len)              -> Additional static output files

pub const MAX_OUTPUT_FILES: usize = 128;

/// 三段式输出产物列表构建错误集
pub err OutputFileListError {
    CapacityExceeded,
    ChunkRegionFull,
    NoSourceMapsOrBytecode,
    SupplementaryRegionFull,
    AdditionalCountMismatch,
    IncompleteOutputList,
}

/// Chunk 内容类型（用于决定是否产出外部 sourcemap / bytecode / module_info）
pub enum ChunkContentKind: u8 {
    javascript = 0,
    css = 1,
    html = 2,
    asset = 3,
}

/// 用于预计算容量布局的 Chunk 元数据描述符
pub struct ChunkDescriptor {
    content_kind: ChunkContentKind = .javascript,
    has_external_sourcemap: bool = false,
    is_js_like_loader: bool = true,
}

/// Bundler 输出布局选项（对齐 Bun `LinkerContext.options` 容量计算分支）
pub struct OutputLayoutOptions {
    external_source_maps: bool = false,
    generate_bytecode_cache: bool = false,
    is_esm_output: bool = true,
    is_compile_mode: bool = false,
    compile_to_standalone_html: bool = false,
    additional_output_files_count: u32 = 0,
}

/// 预计算的三段式输出容量明细
pub struct OutputFileCapacity {
    total_len: u32,
    chunk_count: u32,
    sourcemap_count: u32,
    bytecode_count: u32,
    module_info_count: u32,
    supplementary_count: u32,
    additional_count: u32,
}

/// 输出产物槽位分类
pub enum OutputFileSlotKind: u8 {
    chunk = 0,
    sourcemap = 1,
    bytecode = 2,
    module_info = 3,
    additional = 4,
}

/// 单条构建输出产物元数据记录（零堆分配）
pub struct OutputFileRecord {
    path: string = "",
    slot_kind: OutputFileSlotKind = .chunk,
    byte_size: usize = 0,
    source_chunk_index: u32? = null,
}

/// 预计算三段式输出文件列表的总容量与各分区长度（对齐 Bun `calculateOutputFileListCapacity`）
pub fn calculateOutputFileListCapacity(
    opts: OutputLayoutOptions,
    chunks: []const ChunkDescriptor,
): OutputFileCapacity {
    let sourcemap_count: u32 = 0;
    if (opts.external_source_maps) {
        let i: usize = 0;
        while (i < chunks.len) {
            if (chunks[i].has_external_sourcemap) {
                sourcemap_count += 1;
            }
            i += 1;
        }
    }

    let bytecode_count: u32 = 0;
    if (opts.generate_bytecode_cache) {
        let i: usize = 0;
        while (i < chunks.len) {
            if (chunks[i].content_kind == .javascript && chunks[i].is_js_like_loader) {
                bytecode_count += 1;
            }
            i += 1;
        }
    }

    // ESM bytecode under `--compile` emits a companion `module_info` file per bytecode chunk
    const module_info_count: u32 = if (opts.generate_bytecode_cache && opts.is_esm_output && opts.is_compile_mode)
        bytecode_count
    else
        0;

    // Standalone HTML bundles inline additional assets, so additional output file count is 0
    const additional_count: u32 = if (opts.compile_to_standalone_html) 0 else opts.additional_output_files_count;
    const chunk_count: u32 = chunks.len as u32;
    const supplementary_count: u32 = sourcemap_count + bytecode_count + module_info_count;
    const total_len: u32 = chunk_count + supplementary_count + additional_count;

    return OutputFileCapacity{
        total_len: total_len,
        chunk_count: chunk_count,
        sourcemap_count: sourcemap_count,
        bytecode_count: bytecode_count,
        module_info_count: module_info_count,
        supplementary_count: supplementary_count,
        additional_count: additional_count,
    };
}

/// 零堆分配三段式 `OutputFile` 列表构建器（保持 `outputIndexForChunk(chunk[i]) == i` 核心不变量）
pub struct OutputFileListBuilder {
    files: [MAX_OUTPUT_FILES]OutputFileRecord = undefined,
    total_len: u32 = 0,
    chunk_count: u32 = 0,
    index_for_chunk: u32 = 0,
    index_for_sourcemaps_and_bytecode: u32? = null,
    additional_output_files_start: u32 = 0,
    total_insertions: u32 = 0,
    additional_inserted: bool = false,

    static fn init(cap: OutputFileCapacity): OutputFileListBuilder! {
        if ((cap.total_len as usize) > MAX_OUTPUT_FILES) {
            return err.CapacityExceeded;
        }
        let builder = OutputFileListBuilder{
            files: undefined,
            total_len: cap.total_len,
            chunk_count: cap.chunk_count,
            index_for_chunk: 0,
            index_for_sourcemaps_and_bytecode: if (cap.supplementary_count == 0) null else cap.chunk_count,
            additional_output_files_start: cap.chunk_count + cap.supplementary_count,
            total_insertions: 0,
            additional_inserted: false,
        };
        let i: usize = 0;
        while (i < (cap.total_len as usize)) {
            builder.files[i] = OutputFileRecord{};
            i += 1;
        }
        return builder;
    }

    static fn initFromLayout(
        opts: OutputLayoutOptions,
        chunks: []const ChunkDescriptor,
    ): OutputFileListBuilder! {
        const cap = calculateOutputFileListCapacity(opts, chunks);
        return OutputFileListBuilder.init(cap);
    }

    /// 向分区 1（Chunks）按顺序插入主产物，返回其索引（恒等于 chunk 索引 `i`）
    fn insertForChunk(*_, file: OutputFileRecord): u32! {
        if (_.index_for_chunk >= _.chunk_count) {
            return err.ChunkRegionFull;
        }
        const index = _.index_for_chunk;
        _.index_for_chunk += 1;
        _.files[index as usize] = file;
        _.total_insertions += 1;
        return index;
    }

    /// 向分区 2（SourceMaps / Bytecode / ModuleInfo）插入附属产物
    fn insertForSourcemapOrBytecode(*_, file: OutputFileRecord): u32! {
        const current = _.index_for_sourcemaps_and_bytecode ?? return err.NoSourceMapsOrBytecode;
        if (current >= _.additional_output_files_start) {
            return err.SupplementaryRegionFull;
        }
        _.index_for_sourcemaps_and_bytecode = current + 1;
        _.files[current as usize] = file;
        _.total_insertions += 1;
        return current;
    }

    /// 向分区 3（Additional Output Files）批量写入静态额外产物
    fn insertAdditionalOutputFiles(*_, additional_files: []const OutputFileRecord)! {
        const expected = (_.total_len - _.additional_output_files_start) as usize;
        if (_.additional_inserted || additional_files.len != expected) {
            return err.AdditionalCountMismatch;
        }
        const start = _.additional_output_files_start as usize;
        let i: usize = 0;
        while (i < additional_files.len) {
            _.files[start + i] = additional_files[i];
            i += 1;
        }
        _.total_insertions += additional_files.len as u32;
        _.additional_inserted = true;
    }

    /// 检查三个分区是否全部填充完毕
    fn isComplete(): bool {
        return _.total_insertions == _.total_len;
    }

    /// 校验全部槽位已填充后返回完整不可变切片（对齐 Bun `OutputFileList.take()` 完整性守卫）
    fn finish(): ([]const OutputFileRecord)! {
        if (_.total_insertions != _.total_len) {
            return err.IncompleteOutputList;
        }
        return _.files[0 .. _.total_len as usize];
    }

    /// O(1) 按 chunk 索引直接读取对应 `OutputFileRecord`（核心索引不变量）
    fn getForChunk(chunk_index: u32): OutputFileRecord? {
        if (chunk_index >= _.index_for_chunk) {
            return null;
        }
        return _.files[chunk_index as usize];
    }

    fn chunksSlice(): []const OutputFileRecord {
        return _.files[0 .. _.chunk_count as usize];
    }

    fn supplementarySlice(): []const OutputFileRecord {
        return _.files[_.chunk_count as usize .. _.additional_output_files_start as usize];
    }

    fn additionalSlice(): []const OutputFileRecord {
        return _.files[_.additional_output_files_start as usize .. _.total_len as usize];
    }
}
