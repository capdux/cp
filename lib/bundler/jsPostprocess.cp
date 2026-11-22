// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/postProcessJSChunk.cp | Synced: 2026-11-22T12:51:02Z
// Pure-CP zero-allocation JS Chunk post-processing & assembly engine:
// ESM/CJS/IIFE/BakeDev wrapper assembly (shebang, banner, cross-chunk prefix/suffix, entry tail, footer),
// bytecode caching eligibility (`--bytecode` + `--format=esm` requires `--compile`),
// entry-point tail decision matrix, and SourceMap line/column offset tracking.

/// 单次 JS Chunk 拼装支持的最大模块段数
pub const JS_CHUNK_MAX_PARTS: usize = 64;

/// JS Chunk 后处理与拼装错误集
pub err JsPostprocessError {
    BytecodeRequiresCompileForEsm,
    InvalidShebang,
    BufferTooSmall,
    TooManyParts,
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
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

/// JS Chunk 输出格式（对齐 Bun `options.output_format`）
pub enum JsChunkOutputFormat {
    Esm,
    Cjs,
    Iife,
    InternalBakeDev,
}

/// 模块包装模式（对齐 `JSMeta.Flags.wrap`）
pub enum JsChunkWrapKind {
    None,
    Cjs,
    Esm,
}

/// 模块分隔注释风格（对齐 `postProcessJSChunk.CommentType`）
pub enum JsChunkCommentStyle {
    Multiline,
    Single,
}

/// 入口模块尾部执行语句决策（对齐 `generateEntryPointTailJS`）
pub enum JsEntryTailDecision {
    None,
    ExportDefaultRequire,
    AwaitInit,
    SyncInit,
    ModuleExports,
}

/// 判定当前构建是否需要生成 ESM 字节码 `ModuleInfo`（对齐 Bun `postProcessJSChunk.cp:68`）
pub fn shouldGenerateEsmBytecodeModuleInfo(
    generate_bytecode: bool,
    format: JsChunkOutputFormat,
    is_compile: bool,
): bool {
    return generate_bytecode && format == JsChunkOutputFormat.Esm && is_compile;
}

/// 校验 `--bytecode` 与输出格式的组合约束（参考 `/data/dev/bun/docs/bundler/bytecode.mdx`）：
/// ESM 字节码缓存需要嵌入模块静态导入导出元数据，必须搭配 `--compile` 单文件可执行构建。
pub fn validateBytecodeConfig(
    generate_bytecode: bool,
    format: JsChunkOutputFormat,
    is_compile: bool,
): bool! {
    if (generate_bytecode && format == JsChunkOutputFormat.Esm && !is_compile) {
        return err.BytecodeRequiresCompileForEsm;
    }
    return true;
}

/// 根据输出格式、入口包装类型与异步标记，决定入口尾部代码生成策略
pub fn decideEntryPointTail(
    format: JsChunkOutputFormat,
    wrap: JsChunkWrapKind,
    is_async: bool,
): JsEntryTailDecision {
    switch (format) {
        JsChunkOutputFormat.Esm => {
            switch (wrap) {
                JsChunkWrapKind.Cjs => return JsEntryTailDecision.ExportDefaultRequire,
                JsChunkWrapKind.Esm => {
                    if (is_async) return JsEntryTailDecision.AwaitInit;
                    return JsEntryTailDecision.SyncInit;
                },
                JsChunkWrapKind.None => return JsEntryTailDecision.None,
            }
        },
        JsChunkOutputFormat.Cjs => {
            if (wrap == JsChunkWrapKind.Cjs) {
                return JsEntryTailDecision.ModuleExports;
            }
            if (wrap == JsChunkWrapKind.Esm) {
                return JsEntryTailDecision.SyncInit;
            }
            return JsEntryTailDecision.None;
        },
        JsChunkOutputFormat.Iife => {
            if (wrap == JsChunkWrapKind.Esm) {
                return if (is_async) JsEntryTailDecision.AwaitInit else JsEntryTailDecision.SyncInit;
            }
            return JsEntryTailDecision.None;
        },
        JsChunkOutputFormat.InternalBakeDev => return JsEntryTailDecision.None,
    }
}

/// SourceMap 行列偏移推进器（对齐 `postProcessJSChunk` 中的 `line_offset`）
pub struct JsLineOffsetTracker {
    lines: u32 = 0,
    column: u32 = 0,

    static fn init(): JsLineOffsetTracker {
        return JsLineOffsetTracker{};
    }

    fn advance(*_, text: []const u8) {
        let i: usize = 0;
        while (i < text.len) {
            if (text[i] == '\n') {
                _.lines += 1;
                _.column = 0;
            } else {
                _.column += 1;
            }
            i += 1;
        }
    }

    fn reset(*_) {
        _.lines = 0;
        _.column = 0;
    }
}

/// 待拼装的单个 JS 编译结果段
pub struct JsChunkPartInput {
    label: []const u8 = "",
    code: []const u8 = "",
    is_runtime: bool = false,
}

/// JS Chunk 后处理拼装选项
pub struct JsChunkAssemblyOptions {
    format: JsChunkOutputFormat = JsChunkOutputFormat.Esm,
    is_bun_target: bool = false,
    shebang: []const u8 = "",
    banner: []const u8 = "",
    footer: []const u8 = "",
    cross_chunk_prefix: []const u8 = "",
    cross_chunk_suffix: []const u8 = "",
    entry_point_tail: []const u8 = "",
    emit_part_comments: bool = false,
    comment_style: JsChunkCommentStyle = JsChunkCommentStyle.Single,
}

/// JS Chunk 后处理拼装结果
pub struct JsChunkAssemblyResult {
    bytes_written: usize = 0,
    total_lines: u32 = 0,
    final_column: u32 = 0,
    parts_emitted: usize = 0,
    is_executable: bool = false,
}

fn pushTracked(out: []u8, pos: *usize, tracker: *JsLineOffsetTracker, s: []const u8): bool! {
    if (s.len == 0) {
        return true;
    }
    * = writeSlice(out, *pos, s)?;
    tracker.advance(s);
    return true;
}

fn ensureTrailingNewline(out: []u8, pos: *usize, tracker: *JsLineOffsetTracker): bool! {
    if (pos > 0 && out[pos - 1] != '\n') {
        * = writeByte(out, *pos, '\n')?;
        tracker.advance("\n");
    }
    return true;
}

/// 按 Bun `postProcessJSChunk` 严格顺序将多个已编译模块段拼装为最终 JS Chunk：
/// Shebang -> Banner -> Format Prefix -> Cross-Chunk Prefix -> Parts (with optional comments) ->
/// Entry Tail -> Cross-Chunk Suffix -> Format Suffix -> Trailing Newline -> Footer。
pub fn assembleJsChunk(
    opts: JsChunkAssemblyOptions,
    parts: []const JsChunkPartInput,
    out: []u8,
): JsChunkAssemblyResult! {
    if (parts.len > JS_CHUNK_MAX_PARTS) {
        return err.TooManyParts;
    }

    let pos: usize = 0;
    let tracker = JsLineOffsetTracker.init();
    let is_executable = false;

    // 1. Shebang 必须位于文件最首行（在 banner 与 wrapper 之前）
    if (opts.shebang.len > 0) {
        if (!startsWith(opts.shebang, "#!")) {
            return err.InvalidShebang;
        }
        * = pushTracked(out, *pos, *tracker, opts.shebang)?;
        * = ensureTrailingNewline(out, *pos, *tracker)?;
        is_executable = true;
    }

    // 2. Banner
    if (opts.banner.len > 0) {
        * = pushTracked(out, *pos, *tracker, opts.banner)?;
        * = ensureTrailingNewline(out, *pos, *tracker)?;
    }

    // 3. Format Wrapper Prefix
    if (opts.format == JsChunkOutputFormat.Iife) {
        * = pushTracked(out, *pos, *tracker, "(() => {\n")?;
    } else if (opts.format == JsChunkOutputFormat.Cjs && opts.is_bun_target) {
        * = pushTracked(out, *pos, *tracker, "(function(exports, require, module, __filename, __dirname) {\n")?;
    }

    // 4. Cross-chunk Prefix
    if (opts.cross_chunk_prefix.len > 0) {
        * = pushTracked(out, *pos, *tracker, opts.cross_chunk_prefix)?;
        * = ensureTrailingNewline(out, *pos, *tracker)?;
    }

    // 5. Compiled Module Parts
    let parts_emitted: usize = 0;
    let i: usize = 0;
    while (i < parts.len) {
        const part = parts[i];
        // InternalBakeDev 模式下跳过内嵌 runtime 段（对齐 postProcessJSChunk.cp:510）
        if (part.is_runtime && opts.format == JsChunkOutputFormat.InternalBakeDev) {
            i += 1;
            continue;
        }

        if (opts.emit_part_comments && part.label.len > 0) {
            * = ensureTrailingNewline(out, *pos, *tracker)?;
            switch (opts.comment_style) {
                JsChunkCommentStyle.Single => {
                    * = pushTracked(out, *pos, *tracker, "// ")?;
                    * = pushTracked(out, *pos, *tracker, part.label)?;
                    * = pushTracked(out, *pos, *tracker, "\n")?;
                },
                JsChunkCommentStyle.Multiline => {
                    * = pushTracked(out, *pos, *tracker, "/* ")?;
                    * = pushTracked(out, *pos, *tracker, part.label)?;
                    * = pushTracked(out, *pos, *tracker, " */\n")?;
                },
            }
        }

        if (part.code.len > 0) {
            * = pushTracked(out, *pos, *tracker, part.code)?;
            parts_emitted += 1;
        }
        i += 1;
    }

    // 6. Entry-point Tail
    if (opts.entry_point_tail.len > 0) {
        * = ensureTrailingNewline(out, *pos, *tracker)?;
        * = pushTracked(out, *pos, *tracker, opts.entry_point_tail)?;
    }

    // 7. Cross-chunk Suffix (placed inside IIFE)
    if (opts.cross_chunk_suffix.len > 0) {
        * = ensureTrailingNewline(out, *pos, *tracker)?;
        * = pushTracked(out, *pos, *tracker, opts.cross_chunk_suffix)?;
    }

    // 8. Format Wrapper Suffix
    if (opts.format == JsChunkOutputFormat.Iife) {
        * = ensureTrailingNewline(out, *pos, *tracker)?;
        * = pushTracked(out, *pos, *tracker, "})();\n")?;
    } else if (opts.format == JsChunkOutputFormat.Cjs && opts.is_bun_target) {
        * = ensureTrailingNewline(out, *pos, *tracker)?;
        * = pushTracked(out, *pos, *tracker, "})\n")?;
    }

    // 9. Ensure trailing newline
    * = ensureTrailingNewline(out, *pos, *tracker)?;

    // 10. Footer
    if (opts.footer.len > 0) {
        * = pushTracked(out, *pos, *tracker, opts.footer)?;
        * = ensureTrailingNewline(out, *pos, *tracker)?;
    }

    return JsChunkAssemblyResult{
        bytes_written: pos,
        total_lines: tracker.lines,
        final_column: tracker.column,
        parts_emitted: parts_emitted,
        is_executable: is_executable,
    };
}
