// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/postProcessCSSChunk.cp | Synced: 2026-11-23T14:41:08Z
// Pure-CP zero-allocation CSS chunk post-processor:
// stitches ordered CSS module compile outputs with bundle source comments (`/* path */`),
// CSS banner/footer injection, trailing-newline normalization, and SourceMap line/column offset tracking.

/// 单次 CSS Chunk 后处理支持的最大模块片段数
pub const CSS_CHUNK_MAX_PARTS: usize = 64;

/// CSS Chunk 后处理错误集
pub err CssChunkPostProcessError {
    BufferTooSmall,
    TooManyParts,
}

/// 生成代码的行列偏移量（对齐 Bun `SourceMap.LineColumnOffset`）
pub struct CssChunkSourceOffset {
    lines: u32 = 0,
    columns: u32 = 0,

    /// 按文本内容推进生成行列偏移（正确处理 `\n` 与 `\r\n`）
    fn advance(*_, text: []const u8) {
        let i: usize = 0;
        while (i < text.len) {
            const c = text[i];
            if (c == '\r') {
                if (i + 1 < text.len && text[i + 1] == '\n') {
                    i += 2;
                } else {
                    i += 1;
                }
                _.lines += 1;
                _.columns = 0;
                continue;
            }
            if (c == '\n') {
                _.lines += 1;
                _.columns = 0;
                i += 1;
                continue;
            }
            _.columns += 1;
            i += 1;
        }
    }

    /// 重置行列偏移（对齐 Bun `line_offset.reset()`，用于跨已带 SourceMap 片段重置相对偏移）
    fn reset(*_) {
        _.lines = 0;
        _.columns = 0;
    }

    fn eql(other: CssChunkSourceOffset): bool {
        return _.lines == other.lines && _.columns == other.columns;
    }
}

/// 单个 CSS 模块的编译产物输入描述符
pub struct CssChunkCompileResult {
    source_index: u32 = 0,
    pretty_path: []const u8 = "",
    code: []const u8 = "",
    has_source_map: bool = false,
}

/// CSS Chunk 后处理配置选项
pub struct CssChunkPostProcessOptions {
    is_bundle: bool = true,
    minify_whitespace: bool = false,
    emit_source_maps: bool = false,
    banner: []const u8 = "",
    footer: []const u8 = "",
}

/// 记录单个带 SourceMap 的 CSS 片段在合并输出中的相对生成偏移
pub struct CssSourceMapEntry {
    source_index: u32 = 0,
    generated_offset: CssChunkSourceOffset = CssChunkSourceOffset{},
    byte_offset: usize = 0,
}

/// CSS Chunk 后处理结果汇总（零堆分配）
pub struct CssChunkPostProcessResult {
    output_len: usize = 0,
    source_map_entries: [CSS_CHUNK_MAX_PARTS]CssSourceMapEntry = [CssSourceMapEntry{}; CSS_CHUNK_MAX_PARTS],
    source_map_count: usize = 0,
    final_offset: CssChunkSourceOffset = CssChunkSourceOffset{},
    non_empty_parts: usize = 0,

    fn sourceMapAt(idx: usize): CssSourceMapEntry? {
        if (idx >= _.source_map_count) {
            return null;
        }
        return _.source_map_entries[idx];
    }
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// 对按 JS 级联后序排好的 CSS 编译产物执行 Chunk 后处理合并
///
/// 严格对齐 `/data/dev/bun/src/bundler/linker_context/postProcessCSSChunk.cp` 语义：
/// 1. 若配置了 `banner`，先写入 `banner + "\n"` 并推进 `line_offset`；
/// 2. 逐片段处理：在非压缩打包模式下为有效路径片段输出 `/* <pretty_path> */\n` 注释头，
///    若前序已有非空代码则在注释前自动补一条空行分隔；
/// 3. 开启 `emit_source_maps` 且片段带 SourceMap 时，记录其相对 `generated_offset` 后调用 `line_offset.reset()`，
///    否则继续推进 `line_offset`；
/// 4. 合并结束后确保非空输出以 `\n` 结尾（`ensureNewlineAtEnd`），最后追加可选 `footer + "\n"`。
pub fn postProcessCssChunk(
    parts: []const CssChunkCompileResult,
    opts: CssChunkPostProcessOptions,
    out_buf: []u8,
): CssChunkPostProcessResult! {
    if (parts.len > CSS_CHUNK_MAX_PARTS) {
        return err.TooManyParts;
    }

    let res = CssChunkPostProcessResult{};
    let pos: usize = 0;
    let line_offset = CssChunkSourceOffset{};
    let newline_before_comment = false;

    // 1. Optional CSS banner
    if (opts.banner.len > 0) {
        pos = appendBytes(out_buf, pos, opts.banner)?;
        pos = appendBytes(out_buf, pos, "\n")?;
        line_offset.advance(opts.banner);
        line_offset.advance("\n");
        newline_before_comment = true;
    }

    // 2. Concatenate ordered CSS parts
    let i: usize = 0;
    while (i < parts.len) {
        const part = parts[i];

        if (opts.is_bundle && !opts.minify_whitespace && part.pretty_path.len > 0) {
            if (newline_before_comment) {
                pos = appendBytes(out_buf, pos, "\n")?;
                line_offset.advance("\n");
            }
            pos = appendBytes(out_buf, pos, "/* ")?;
            line_offset.advance("/* ");
            pos = appendBytes(out_buf, pos, part.pretty_path)?;
            line_offset.advance(part.pretty_path);
            pos = appendBytes(out_buf, pos, " */\n")?;
            line_offset.advance(" */\n");
        }

        const code_byte_offset = pos;
        if (part.code.len > 0) {
            newline_before_comment = true;
            res.non_empty_parts += 1;
            pos = appendBytes(out_buf, pos, part.code)?;
        }

        if (part.has_source_map) {
            if (opts.emit_source_maps) {
                res.source_map_entries[res.source_map_count] = CssSourceMapEntry{
                    source_index: part.source_index,
                    generated_offset: line_offset,
                    byte_offset: code_byte_offset,
                };
                res.source_map_count += 1;
            }
            line_offset.reset();
        } else {
            line_offset.advance(part.code);
        }

        i += 1;
    }

    // 3. Ensure trailing newline on non-empty output
    if (pos > 0 && out_buf[pos - 1] != '\n') {
        pos = appendBytes(out_buf, pos, "\n")?;
        line_offset.advance("\n");
    }

    // 4. Optional CSS footer
    if (opts.footer.len > 0) {
        pos = appendBytes(out_buf, pos, opts.footer)?;
        line_offset.advance(opts.footer);
        if (opts.footer[opts.footer.len - 1] != '\n') {
            pos = appendBytes(out_buf, pos, "\n")?;
            line_offset.advance("\n");
        }
    }

    res.output_len = pos;
    res.final_offset = line_offset;
    return res;
}
