// Source: Absorbed from /data/dev/bun/src/bundler/Chunk.cp | Synced: 2026-11-22T10:29:17Z
// Ported from Bun: /data/dev/bun/src/bundler/Chunk.cp & /data/dev/bun/docs/bundler/standalone-html.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation bundler Chunk OutputPiece assembler:
// deferred placeholder resolution (Chunk / Asset / SCB / HTMLImport),
// standalone HTML inline code & data: URI embedding with case-insensitive </script & </style escaping,
// cross-platform import prefix normalization, and //# debugId= comment emission.

/// 单个 Chunk 组装器支持的最大分段数
pub const CHUNK_OUTPUT_MAX_PIECES: usize = 64;

/// 单个查询表推荐上限（仅供调用方参考）
pub const CHUNK_OUTPUT_MAX_REFS: usize = 32;

/// Chunk 输出组装与转义错误集
pub err ChunkOutputError {
    TooManyPieces,
    InvalidReferenceIndex,
    InvalidCloseTag,
    BufferTooSmall,
    MissingHtmlImportManifest,
}

/// OutputPiece 延迟引用查询类型（严格对齐 Bun `ChunkOutputPieceQueryKind`）
pub enum ChunkPieceQueryKind: u8 {
    /// 末尾纯字面量段，无后续引用
    None = 0,
    /// 静态资源引用（普通模式输出路径，Standalone HTML 模式内联 `data:` URI）
    Asset = 1,
    /// 其他 Chunk 引用（普通模式输出相对路径，Standalone HTML 模式内联代码并转义闭合标签）
    Chunk = 2,
    /// Server Component Boundary 入口 Chunk 引用
    Scb = 3,
    /// HTML Import Manifest JSON 引用
    HtmlImport = 4,
}

/// 延迟引用查询描述符
pub struct ChunkPieceQuery {
    index: u32 = 0,
    kind: ChunkPieceQueryKind = ChunkPieceQueryKind.None,

    static fn none(): ChunkPieceQuery {
        return ChunkPieceQuery{ index: 0, kind: ChunkPieceQueryKind.None };
    }

    static fn init(kind: ChunkPieceQueryKind, index: u32): ChunkPieceQuery {
        return ChunkPieceQuery{ index: index, kind: kind };
    }
}

/// 单个中间输出分段：前置字面量 + 紧随其后的延迟引用查询
pub struct ChunkPiece {
    data: []const u8 = "",
    query: ChunkPieceQuery = ChunkPieceQuery{},
    prefix_comma: bool = false,
}

/// 内联内容类别（决定 Standalone HTML 转义闭合标签与文件扩展名）
pub enum ChunkInlineKind: u8 {
    JavaScript = 0,
    Css = 1,
    Html = 2,
}

/// 引用解析目标（用于 Chunk / Asset / SCB / HTMLImport 查询表）
pub struct ChunkRefTarget {
    rel_path: []const u8 = "",
    inline_content: ([]const u8)? = null,
    kind: ChunkInlineKind = ChunkInlineKind.JavaScript,
}

/// Chunk 输出组装选项
pub struct ChunkOutputAssembleOptions {
    import_prefix: []const u8 = "./",
    standalone_mode: bool = false,
    emit_debug_id: bool = false,
    debug_id_hash: u64 = 0,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return ChunkOutputError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): void! {
    if (pos.* + 1 > out.len) {
        return ChunkOutputError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

fn hexDigitLower(nibble: u8): u8 {
    if (nibble < 10) {
        return ('0' as u8) + nibble;
    }
    return ('a' as u8) + (nibble - 10);
}

/// 返回在 Standalone HTML 内联时需要转义的闭合标签前缀（对齐 Bun `Chunk.closingTagForContent`）
pub fn chunkClosingTagForKind(kind: ChunkInlineKind): []const u8 {
    return switch (kind) {
        ChunkInlineKind.JavaScript => "</script",
        ChunkInlineKind.Css => "</style",
        ChunkInlineKind.Html => "",
    };
}

/// 返回 Chunk 内容对应的标准文件扩展名（对齐 Bun `ChunkContent.ext`）
pub fn chunkExtensionForKind(kind: ChunkInlineKind): []const u8 {
    return switch (kind) {
        ChunkInlineKind.JavaScript => "js",
        ChunkInlineKind.Css => "css",
        ChunkInlineKind.Html => "html",
    };
}

/// 统计内容中大小写无关匹配 `close_tag`（如 `</script`、`</style`）的次数，
/// 用于精确预计算将 `</` 转义为 `<\/` 所需的额外字节数。
pub fn countClosingTagOccurrences(content: []const u8, close_tag: []const u8): usize {
    if (close_tag.len < 3 || close_tag[0] != '<' || close_tag[1] != '/') {
        return 0;
    }
    const tag_suffix = close_tag[2..];
    let count: usize = 0;
    let i: usize = 0;
    while (i + 2 <= content.len) {
        if (content[i] == '<' && content[i + 1] == '/') {
            const after = i + 2;
            if (after + tag_suffix.len <= content.len &&
                eqlIgnoreCaseAscii(content[after .. after + tag_suffix.len], tag_suffix))
            {
                count += 1;
                i = after + tag_suffix.len;
                continue;
            }
            i += 2;
            continue;
        }
        i += 1;
    }
    return count;
}

/// 将 `content` 拷贝至 `out`，并把所有大小写无关匹配的 `close_tag` 的 `</` 转义为 `<\/`，
/// 防止内联脚本或样式提前截断外层 `<script>` / `<style>` 标签。
/// 非匹配的闭合标签（如 `</div>`）原样保留。
pub fn escapeClosingTagsInto(content: []const u8, close_tag: []const u8, out: []u8): usize! {
    if (close_tag.len < 3 || close_tag[0] != '<' || close_tag[1] != '/') {
        return ChunkOutputError.InvalidCloseTag;
    }
    const extra = countClosingTagOccurrences(content, close_tag);
    if (content.len + extra > out.len) {
        return ChunkOutputError.BufferTooSmall;
    }
    const tag_suffix = close_tag[2..];
    let i: usize = 0;
    let dst: usize = 0;
    while (i < content.len) {
        if (i + 1 < content.len && content[i] == '<' && content[i + 1] == '/') {
            const after = i + 2;
            if (after + tag_suffix.len <= content.len &&
                eqlIgnoreCaseAscii(content[after .. after + tag_suffix.len], tag_suffix))
            {
                out[dst] = '<';
                out[dst + 1] = '\\';
                out[dst + 2] = '/';
                dst += 3;
                i += 2;
                continue;
            }
            out[dst] = '<';
            out[dst + 1] = '/';
            dst += 2;
            i += 2;
            continue;
        }
        out[dst] = content[i];
        dst += 1;
        i += 1;
    }
    return dst;
}

/// 计算 `normalizeChunkImportPath` 的精确输出字节长度
pub fn normalizedChunkImportPathLen(import_prefix: []const u8, raw_path: []const u8): usize {
    let p_start: usize = 0;
    // 若 raw_path 自身已带 "./" 或 ".\"，在存在前缀时去除重复的 "./"
    if (import_prefix.len > 0) {
        while (p_start + 2 <= raw_path.len &&
            raw_path[p_start] == '.' &&
            (raw_path[p_start + 1] == '/' || raw_path[p_start + 1] == '\\'))
        {
            p_start += 2;
        }
        // 若 prefix 末尾为 '/' 且 raw_path 开头为 '/' 或 '\'，避免生成双斜杠
        if (import_prefix[import_prefix.len - 1] == '/') {
            while (p_start < raw_path.len && (raw_path[p_start] == '/' || raw_path[p_start] == '\\')) {
                p_start += 1;
            }
        }
    }
    const rem_len = raw_path.len - p_start;
    let need_sep: usize = 0;
    if (import_prefix.len > 0 && rem_len > 0) {
        const last_p = import_prefix[import_prefix.len - 1];
        const first_r = raw_path[p_start];
        if (last_p != '/' && first_r != '/' && first_r != '\\') {
            need_sep = 1;
        }
    }
    return import_prefix.len + need_sep + rem_len;
}

/// 跨平台 Chunk 导入路径规范化（将 Windows `\` 转为 POSIX `/`，拼接 `import_prefix` 并消除重复 `./` 与双斜杠）
pub fn normalizeChunkImportPath(import_prefix: []const u8, raw_path: []const u8, out: []u8): usize! {
    const needed = normalizedChunkImportPathLen(import_prefix, raw_path);
    if (needed > out.len) {
        return ChunkOutputError.BufferTooSmall;
    }
    let p_start: usize = 0;
    if (import_prefix.len > 0) {
        while (p_start + 2 <= raw_path.len &&
            raw_path[p_start] == '.' &&
            (raw_path[p_start + 1] == '/' || raw_path[p_start + 1] == '\\'))
        {
            p_start += 2;
        }
        if (import_prefix[import_prefix.len - 1] == '/') {
            while (p_start < raw_path.len && (raw_path[p_start] == '/' || raw_path[p_start] == '\\')) {
                p_start += 1;
            }
        }
    }

    let pos: usize = 0;
    appendBytes(out, &pos, import_prefix)?;
    if (import_prefix.len > 0 && p_start < raw_path.len) {
        const last_p = import_prefix[import_prefix.len - 1];
        const first_r = raw_path[p_start];
        if (last_p != '/' && first_r != '/' && first_r != '\\') {
            appendByte(out, &pos, '/')?;
        }
    }

    let i: usize = p_start;
    while (i < raw_path.len) {
        const c = raw_path[i];
        out[pos] = if (c == '\\') '/' else c;
        pos += 1;
        i += 1;
    }
    return pos;
}

/// Debug ID 注释固定长度：`\n//# debugId=00000000-0000-0000-XXXX-XXXXXXXXXXXX\n` = 50 字节
pub const CHUNK_DEBUG_ID_COMMENT_LEN: usize = 50;

/// 生成确定性 SourceMap Debug ID 注释（位于 `//# sourceMappingURL` 之前）
pub fn formatChunkDebugIdComment(isolated_hash: u64, out: []u8): usize! {
    if (out.len < CHUNK_DEBUG_ID_COMMENT_LEN) {
        return ChunkOutputError.BufferTooSmall;
    }
    let pos: usize = 0;
    appendBytes(out, &pos, "\n//# debugId=00000000-0000-0000-")?;
    // 高 16 位 -> 4 个 hex 字符
    let shift: u6 = 60;
    let g1: usize = 0;
    while (g1 < 4) {
        const nib = ((isolated_hash >> shift) & 0xF) as u8;
        appendByte(out, &pos, hexDigitLower(nib))?;
        shift -= 4;
        g1 += 1;
    }
    appendByte(out, &pos, '-')?;
    // 低 48 位 -> 12 个 hex 字符
    let g2: usize = 0;
    while (g2 < 12) {
        const nib = ((isolated_hash >> shift) & 0xF) as u8;
        appendByte(out, &pos, hexDigitLower(nib))?;
        if (g2 < 11) {
            shift -= 4;
        }
        g2 += 1;
    }
    appendByte(out, &pos, '\n')?;
    return pos;
}

/// 零堆分配 Chunk OutputPiece 组装器（对齐 Bun `ChunkIntermediateOutput.codeWithSourceMapShifts`）
pub struct ChunkPieceAssembler {
    pieces: [CHUNK_OUTPUT_MAX_PIECES]ChunkPiece = [ChunkPiece{}; CHUNK_OUTPUT_MAX_PIECES],
    piece_count: usize = 0,

    static fn init(): ChunkPieceAssembler {
        return ChunkPieceAssembler{};
    }

    /// 追加一段纯字面量文本（无后续引用查询）
    fn addLiteral(*_, data: []const u8): void! {
        _.addPiece(data, ChunkPieceQuery.none())?;
    }

    /// 追加一段“字面量 + 延迟引用查询”组合分段
    fn addPiece(*_, data: []const u8, query: ChunkPieceQuery): void! {
        if (_.piece_count >= CHUNK_OUTPUT_MAX_PIECES) {
            return ChunkOutputError.TooManyPieces;
        }
        _.pieces[_.piece_count] = ChunkPiece{
            data: data,
            query: query,
            prefix_comma: false,
        };
        _.piece_count += 1;
    }

    /// 统计所有分段中字面量部分的总字节数（对齐 Bun `ChunkIntermediateOutput.getSize`）
    fn literalByteSize(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.piece_count) {
            total += _.pieces[i].data.len;
            i += 1;
        }
        return total;
    }

    /// 第一遍扫描：精确计算组装后输出的总字节数（包含闭合标签转义开销、规范化路径与可选 debugId）
    fn estimateOutputSize(
        chunks: []const ChunkRefTarget,
        assets: []const ChunkRefTarget,
        scbs: []const ChunkRefTarget,
        html_imports: []const ChunkRefTarget,
        opts: ChunkOutputAssembleOptions,
    ): usize! {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.piece_count) {
            const piece = _.pieces[i];
            total += piece.data.len;
            const idx = piece.query.index as usize;
            switch (piece.query.kind) {
                ChunkPieceQueryKind.None => {},
                ChunkPieceQueryKind.Chunk => {
                    if (idx >= chunks.len) {
                        return ChunkOutputError.InvalidReferenceIndex;
                    }
                    const target = chunks[idx];
                    if (opts.standalone_mode) {
                        if (inline_code := target.inline_content) {
                            const close_tag = chunkClosingTagForKind(target.kind);
                            const extra = countClosingTagOccurrences(inline_code, close_tag);
                            total += inline_code.len + extra;
                        } else {
                            total += normalizedChunkImportPathLen(opts.import_prefix, target.rel_path);
                        }
                    } else {
                        total += normalizedChunkImportPathLen(opts.import_prefix, target.rel_path);
                    }
                },
                ChunkPieceQueryKind.Asset => {
                    if (idx >= assets.len) {
                        return ChunkOutputError.InvalidReferenceIndex;
                    }
                    const target = assets[idx];
                    if (opts.standalone_mode) {
                        if (data_uri := target.inline_content) {
                            total += data_uri.len;
                        } else {
                            total += normalizedChunkImportPathLen(opts.import_prefix, target.rel_path);
                        }
                    } else {
                        total += normalizedChunkImportPathLen(opts.import_prefix, target.rel_path);
                    }
                },
                ChunkPieceQueryKind.Scb => {
                    if (idx >= scbs.len) {
                        return ChunkOutputError.InvalidReferenceIndex;
                    }
                    const target = scbs[idx];
                    total += normalizedChunkImportPathLen(opts.import_prefix, target.rel_path);
                },
                ChunkPieceQueryKind.HtmlImport => {
                    if (idx >= html_imports.len) {
                        return ChunkOutputError.InvalidReferenceIndex;
                    }
                    const target = html_imports[idx];
                    if (manifest_json := target.inline_content) {
                        total += manifest_json.len;
                    } else {
                        return ChunkOutputError.MissingHtmlImportManifest;
                    }
                },
            }
            i += 1;
        }
        if (opts.emit_debug_id) {
            total += CHUNK_DEBUG_ID_COMMENT_LEN;
        }
        return total;
    }

    /// 第二遍组装：将所有分段与引用解析写入 `out`，返回实际写入字节数（与 `estimateOutputSize` 严格相等）
    fn assemble(
        chunks: []const ChunkRefTarget,
        assets: []const ChunkRefTarget,
        scbs: []const ChunkRefTarget,
        html_imports: []const ChunkRefTarget,
        opts: ChunkOutputAssembleOptions,
        out: []u8,
    ): usize! {
        const needed = _.estimateOutputSize(chunks, assets, scbs, html_imports, opts)?;
        if (needed > out.len) {
            return ChunkOutputError.BufferTooSmall;
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.piece_count) {
            const piece = _.pieces[i];
            appendBytes(out, &pos, piece.data)?;
            const idx = piece.query.index as usize;
            switch (piece.query.kind) {
                ChunkPieceQueryKind.None => {},
                ChunkPieceQueryKind.Chunk => {
                    const target = chunks[idx];
                    let inlined = false;
                    if (opts.standalone_mode) {
                        if (inline_code := target.inline_content) {
                            const close_tag = chunkClosingTagForKind(target.kind);
                            if (close_tag.len >= 3) {
                                const wn = escapeClosingTagsInto(inline_code, close_tag, out[pos..])?;
                                pos += wn;
                            } else {
                                appendBytes(out, &pos, inline_code)?;
                            }
                            inlined = true;
                        }
                    }
                    if (!inlined) {
                        const pn = normalizeChunkImportPath(opts.import_prefix, target.rel_path, out[pos..])?;
                        pos += pn;
                    }
                },
                ChunkPieceQueryKind.Asset => {
                    const target = assets[idx];
                    let inlined = false;
                    if (opts.standalone_mode) {
                        if (data_uri := target.inline_content) {
                            appendBytes(out, &pos, data_uri)?;
                            inlined = true;
                        }
                    }
                    if (!inlined) {
                        const pn = normalizeChunkImportPath(opts.import_prefix, target.rel_path, out[pos..])?;
                        pos += pn;
                    }
                },
                ChunkPieceQueryKind.Scb => {
                    const target = scbs[idx];
                    const pn = normalizeChunkImportPath(opts.import_prefix, target.rel_path, out[pos..])?;
                    pos += pn;
                },
                ChunkPieceQueryKind.HtmlImport => {
                    const target = html_imports[idx];
                    if (manifest_json := target.inline_content) {
                        appendBytes(out, &pos, manifest_json)?;
                    } else {
                        return ChunkOutputError.MissingHtmlImportManifest;
                    }
                },
            }
            i += 1;
        }
        if (opts.emit_debug_id) {
            const dn = formatChunkDebugIdComment(opts.debug_id_hash, out[pos..])?;
            pos += dn;
        }
        return pos;
    }
}
