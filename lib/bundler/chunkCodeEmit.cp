// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/generateCodeForFileInChunkJS.cp | Synced: 2026-11-20T16:10:08Z
// ============================================================================
// lib/bundler/chunkCodeEmit.cp — Chunk Module Wrapper, Line Tracking & Safe Minifier
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven chunk code emission engine for `cap build`:
//   1. Four module wrapper emitters (`emitWrappedModuleInto`): `FlatEsm`,
//      `CommonJsClosure` (`__commonJS((exports, module) => { ... })`),
//      `LazyEsmInit` (`__esm(() => { ... })`), and `AsyncEsmInit` (top-level await);
//   2. Chunk-level zero-heap assembler (`ChunkCodeAssembler`) stitching Banner,
//      Runtime Helpers, wrapped modules, Export Tail, and Footer while tracking
//      exact 0-based `(start_line, start_col) -> (end_line, end_col)` spans via
//      `cap:simd` (`Simd.countByte`) for SourceMap V3 concatenation;
//   3. Lexer-aware comment stripper & safe whitespace minifier
//      (`minifyJsWhitespaceSafe`) preserving single/double/template strings,
//      regex literals, `/*! ... */` legal comments, and ASI/identifier boundaries
//      with `cap:simd` fast-path scanning;
//   4. Diagnostic span table reporter (`formatChunkEmitSpanTable`).

import { Simd } from "cap:simd";

/// Standard CommonJS closure runtime helper injected when a chunk wraps CJS modules.
pub const CHUNK_CODE_COMMONJS_HELPER: []const u8 =
    "var __commonJS = (cb, mod) => () => (mod || cb((mod = { exports: {} }).exports, mod), mod.exports);\n";

/// Standard lazy ESM initializer helper injected for wrapped ESM modules.
pub const CHUNK_CODE_ESM_HELPER: []const u8 =
    "var __esm = (fn, res) => () => (fn && (res = fn(fn = 0)), res);\n";

/// Standard async ESM initializer helper injected for Top-Level Await modules.
pub const CHUNK_CODE_ASYNC_ESM_HELPER: []const u8 =
    "var __asyncEsm = (fn, res) => () => (res || (res = fn()));\n";

/// Error set for module wrapping, chunk assembly, and whitespace minification.
pub err ChunkCodeEmitError {
    InvalidWrapperName,
    SliceCapacityExceeded,
    BufferTooSmall,
    AlreadyFinalized,
}

// ============================================================================
// Internal Zero-Alloc Byte, Identifier, and Formatting Helpers
// ============================================================================

fn ccIsIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

fn ccIsIdentContinue(c: u8): bool {
    return ccIsIdentStart(c) || (c >= '0' && c <= '9');
}

fn ccIsValidJsIdent(name: []const u8): bool {
    if (name.len == 0) return false;
    if (!ccIsIdentStart(name[0])) return false;
    let i: usize = 1;
    while (i < name.len) {
        if (!ccIsIdentContinue(name[i])) return false;
        i += 1;
    }
    return true;
}

fn ccAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ccAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ccAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ccAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        ccAppendByte(out, pos, tmp[n])?;
    }
}

fn ccLastNewlineOffset(slice: []const u8): usize? {
    let i = slice.len;
    while (i > 0) {
        i -= 1;
        if (slice[i] == '\n') return i;
    }
    return null;
}

// ============================================================================
// Section 1: Lexer-Aware Comment Stripper & Safe Whitespace Minifier
// ============================================================================

/// Configuration for `minifyJsWhitespaceSafe`.
pub struct MinifyWhitespaceOptions {
    preserve_legal_comments: bool = true,
    collapse_newlines: bool = true,

    static fn defaults(): MinifyWhitespaceOptions {
        return MinifyWhitespaceOptions{};
    }
}

/// Telemetry returned by `minifyJsWhitespaceSafe`.
pub struct MinifyWhitespaceStats {
    input_bytes: usize = 0,
    output_bytes: usize = 0,
    line_comments_stripped: u32 = 0,
    block_comments_stripped: u32 = 0,
    legal_comments_preserved: u32 = 0,
    strings_preserved: u32 = 0,
    regexes_preserved: u32 = 0,
}

fn ccCanStartRegexAfter(prev_byte: u8): bool {
    if (prev_byte == 0) return true;
    return prev_byte == '(' || prev_byte == '[' || prev_byte == '{' ||
        prev_byte == ',' || prev_byte == ';' || prev_byte == ':' ||
        prev_byte == '=' || prev_byte == '!' || prev_byte == '?' ||
        prev_byte == '&' || prev_byte == '|' || prev_byte == '+' ||
        prev_byte == '-' || prev_byte == '*' || prev_byte == '%' ||
        prev_byte == '^' || prev_byte == '~' || prev_byte == '<' ||
        prev_byte == '>' || prev_byte == '\n';
}

fn ccNeedsSpaceBetween(prev_b: u8, next_b: u8): bool {
    if (prev_b == 0 || next_b == 0) return false;
    // Prevent merging adjacent identifiers/numbers (e.g. `const a` -> `consta`).
    if (ccIsIdentContinue(prev_b) && ccIsIdentContinue(next_b)) return true;
    // Prevent turning `+ +` into `++` or `- -` into `--`.
    if (prev_b == '+' && next_b == '+') return true;
    if (prev_b == '-' && next_b == '-') return true;
    // Prevent `/ /` from becoming `//` or `/ *` becoming `/*`.
    if (prev_b == '/' && (next_b == '/' || next_b == '*')) return true;
    return false;
}

/// Strips non-legal `//` and `/* ... */` comments and safely collapses redundant
/// whitespace while strictly preserving:
///   - Single-quoted `'...'`, double-quoted `"..."`, and template `` `...` `` strings;
///   - Regex literals `/pattern/flags` (including `[...]` character classes);
///   - `/*! ... */` license comments and `//# ` sourceMappingURL directives;
///   - Required separators between adjacent identifiers, numbers, and `++`/`--` tokens.
pub fn minifyJsWhitespaceSafe(
    src: []const u8,
    opts: MinifyWhitespaceOptions,
    out_buf: []u8,
): ChunkCodeEmitError!MinifyWhitespaceStats {
    let stats = MinifyWhitespaceStats{ input_bytes: src.len };
    let out_pos: usize = 0;
    let i: usize = 0;
    let last_emitted: u8 = 0;
    let pending_space = false;
    let pending_newline = false;

    while (i < src.len) {
        const c = src[i];

        // 1. Whitespace run (SIMD-accelerated skip)
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            let saw_nl = (c == '\n' || c == '\r');
            const ws_skip = Simd.skipWhitespace(src[i..]);
            const ws_end = i + ws_skip;
            if (!saw_nl) {
                if (Simd.indexOfAny2(src[i..ws_end], '\n', '\r') != null) {
                    saw_nl = true;
                }
            }
            if (saw_nl) {
                pending_newline = true;
            } else {
                pending_space = true;
            }
            i = ws_end;
            continue;
        }

        // 2. Comments or Regex literal starting with '/'
        if (c == '/' && i + 1 < src.len) {
            const nxt = src[i + 1];

            // Single-line comment `// ...`
            if (nxt == '/') {
                const is_directive = opts.preserve_legal_comments &&
                    (i + 2 < src.len && src[i + 2] == '#');
                let line_end = src.len;
                if (nl_rel := Simd.indexOfByte(src[i + 2..], '\n')) {
                    line_end = i + 2 + nl_rel;
                }

                if (is_directive) {
                    if (out_pos > 0 && last_emitted != '\n') {
                        ccAppendByte(out_buf, &out_pos, '\n')?;
                    }
                    ccAppendBytes(out_buf, &out_pos, src[i..line_end])?;
                    ccAppendByte(out_buf, &out_pos, '\n')?;
                    last_emitted = '\n';
                    pending_space = false;
                    pending_newline = false;
                    stats.legal_comments_preserved += 1;
                } else {
                    stats.line_comments_stripped += 1;
                    pending_newline = true;
                }
                i = if (line_end < src.len) line_end + 1 else src.len;
                continue;
            }

            // Block comment `/* ... */`
            if (nxt == '*') {
                const is_legal = opts.preserve_legal_comments &&
                    (i + 2 < src.len && src[i + 2] == '!');
                let scan = i + 2;
                let closed = false;
                while (scan + 1 < src.len) {
                    if (src[scan] == '*' && src[scan + 1] == '/') {
                        scan += 2;
                        closed = true;
                        break;
                    }
                    scan += 1;
                }
                if (!closed) {
                    scan = src.len;
                }

                if (is_legal) {
                    if (pending_newline && out_pos > 0 && last_emitted != '\n') {
                        ccAppendByte(out_buf, &out_pos, '\n')?;
                        last_emitted = '\n';
                    } else if (pending_space && ccNeedsSpaceBetween(last_emitted, '/')) {
                        ccAppendByte(out_buf, &out_pos, ' ')?;
                        last_emitted = ' ';
                    }
                    ccAppendBytes(out_buf, &out_pos, src[i..scan])?;
                    last_emitted = if (scan > i) src[scan - 1] else '/';
                    pending_space = false;
                    pending_newline = false;
                    stats.legal_comments_preserved += 1;
                } else {
                    stats.block_comments_stripped += 1;
                    pending_space = true;
                }
                i = scan;
                continue;
            }

            // Potential Regex literal `/pattern/flags`
            if (ccCanStartRegexAfter(last_emitted)) {
                let scan = i + 1;
                let in_class = false;
                let closed = false;
                while (scan < src.len) {
                    const rc = src[scan];
                    if (rc == '\n' || rc == '\r') break;
                    if (rc == '\\' && scan + 1 < src.len) {
                        scan += 2;
                        continue;
                    }
                    if (rc == '[') {
                        in_class = true;
                    } else if (rc == ']') {
                        in_class = false;
                    } else if (rc == '/' && !in_class) {
                        scan += 1;
                        while (scan < src.len && ((src[scan] >= 'a' && src[scan] <= 'z') || (src[scan] >= 'A' && src[scan] <= 'Z'))) {
                            scan += 1;
                        }
                        closed = true;
                        break;
                    }
                    scan += 1;
                }

                if (closed) {
                    if (pending_newline && !opts.collapse_newlines && out_pos > 0 && last_emitted != '\n') {
                        ccAppendByte(out_buf, &out_pos, '\n')?;
                        last_emitted = '\n';
                    } else if ((pending_space || pending_newline) && ccNeedsSpaceBetween(last_emitted, '/')) {
                        ccAppendByte(out_buf, &out_pos, ' ')?;
                        last_emitted = ' ';
                    }
                    ccAppendBytes(out_buf, &out_pos, src[i..scan])?;
                    last_emitted = src[scan - 1];
                    pending_space = false;
                    pending_newline = false;
                    stats.regexes_preserved += 1;
                    i = scan;
                    continue;
                }
            }
        }

        // Flush pending separator before emitting next real token
        if (out_pos > 0) {
            if (pending_newline) {
                if (!opts.collapse_newlines) {
                    if (last_emitted != '\n') {
                        ccAppendByte(out_buf, &out_pos, '\n')?;
                        last_emitted = '\n';
                    }
                } else {
                    // Preserve newline when ASI could be affected (between non-punctuation tokens)
                    const asi_safe_prev = last_emitted == ';' || last_emitted == '{' || last_emitted == ',' || last_emitted == '(' || last_emitted == '[';
                    if (!asi_safe_prev && c != '}' && c != ')' && c != ']' && c != ';') {
                        ccAppendByte(out_buf, &out_pos, '\n')?;
                        last_emitted = '\n';
                    } else if (ccNeedsSpaceBetween(last_emitted, c)) {
                        ccAppendByte(out_buf, &out_pos, ' ')?;
                        last_emitted = ' ';
                    }
                }
            } else if (pending_space && ccNeedsSpaceBetween(last_emitted, c)) {
                ccAppendByte(out_buf, &out_pos, ' ')?;
                last_emitted = ' ';
            }
        }
        pending_space = false;
        pending_newline = false;

        // 3. String or template literal (`'...'`, `"..."`, `` `...` ``)
        if (c == '\'' || c == '"' || c == '`') {
            const quote = c;
            let scan = i + 1;
            while (scan < src.len) {
                const sc = src[scan];
                if (sc == '\\' && scan + 1 < src.len) {
                    scan += 2;
                    continue;
                }
                if (sc == quote) {
                    scan += 1;
                    break;
                }
                if (quote != '`' && (sc == '\n' || sc == '\r')) {
                    break;
                }
                scan += 1;
            }
            ccAppendBytes(out_buf, &out_pos, src[i..scan])?;
            last_emitted = src[scan - 1];
            stats.strings_preserved += 1;
            i = scan;
            continue;
        }

        // 4. Regular code byte
        ccAppendByte(out_buf, &out_pos, c)?;
        last_emitted = c;
        i += 1;
    }

    stats.output_bytes = out_pos;
    return stats;
}

// ============================================================================
// Section 2: Four Module Wrapper Emitters (`emitWrappedModuleInto`)
// ============================================================================

/// Wrapping strategy applied to a single module inside a JS chunk.
pub enum ModuleWrapMode: u8 {
    FlatEsm = 0,
    CommonJsClosure = 1,
    LazyEsmInit = 2,
    AsyncEsmInit = 3,
}

/// Returns the canonical name for `mode`.
pub fn moduleWrapModeName(mode: ModuleWrapMode): []const u8 {
    if (mode == ModuleWrapMode.FlatEsm) return "flat_esm";
    if (mode == ModuleWrapMode.CommonJsClosure) return "commonjs_closure";
    if (mode == ModuleWrapMode.LazyEsmInit) return "lazy_esm_init";
    return "async_esm_init";
}

/// Input descriptor for emitting a single module into a chunk buffer.
pub struct ModuleEmitInput {
    source_index: u32 = 0,
    source_path: []const u8 = "",
    wrapper_name: []const u8 = "",
    wrap_mode: ModuleWrapMode = ModuleWrapMode.FlatEsm,
    emit_use_strict: bool = false,
    hoisted_imports: []const u8 = "",
    body_code: []const u8 = "",
    trailing_exports: []const u8 = "",

    static fn empty(): ModuleEmitInput {
        return ModuleEmitInput{};
    }
}

/// Formatting and minification options for module and chunk code emission.
pub struct ModuleEmitOptions {
    emit_source_comments: bool = true,
    minify_whitespace: bool = false,
    preserve_legal_comments: bool = true,

    static fn defaults(): ModuleEmitOptions {
        return ModuleEmitOptions{};
    }
}

/// Byte and line metrics returned by `emitWrappedModuleInto`.
pub struct ModuleEmitResult {
    bytes_written: usize = 0,
    lines_written: u32 = 0,
    body_start_offset: usize = 0,
    body_end_offset: usize = 0,
}

/// Emits a single module into `out_buf` using one of the 4 wrapping modes:
///   - `FlatEsm`: top-level statements emitted directly;
///   - `CommonJsClosure`: `var <wrapper> = __commonJS((exports, module) => { ... });`;
///   - `LazyEsmInit`: `var <wrapper> = __esm(() => { ... });`;
///   - `AsyncEsmInit`: `var <wrapper> = __asyncEsm(async () => { ... });`.
///
/// Automatically injects source-path header comments, `"use strict";` prologue when
/// requested, and optionally runs `minifyJsWhitespaceSafe` on the module body.
pub fn emitWrappedModuleInto(
    input: ModuleEmitInput,
    opts: ModuleEmitOptions,
    out_buf: []u8,
): ChunkCodeEmitError!ModuleEmitResult {
    if (input.wrap_mode != ModuleWrapMode.FlatEsm) {
        if (!ccIsValidJsIdent(input.wrapper_name)) {
            return err.InvalidWrapperName;
        }
    }

    let pos: usize = 0;

    // Source path header comment (mirrors Bun's `// src/foo.ts`)
    if (opts.emit_source_comments && !opts.minify_whitespace && input.source_path.len > 0) {
        ccAppendBytes(out_buf, &pos, "// ")?;
        ccAppendBytes(out_buf, &pos, input.source_path)?;
        ccAppendByte(out_buf, &pos, '\n')?;
    }

    // Hoisted imports (emitted outside wrapper for ESM semantics)
    if (input.hoisted_imports.len > 0) {
        ccAppendBytes(out_buf, &pos, input.hoisted_imports)?;
        if (input.hoisted_imports[input.hoisted_imports.len - 1] != '\n') {
            ccAppendByte(out_buf, &pos, '\n')?;
        }
    }

    // Opening wrapper header
    if (input.wrap_mode == ModuleWrapMode.CommonJsClosure) {
        ccAppendBytes(out_buf, &pos, "var ")?;
        ccAppendBytes(out_buf, &pos, input.wrapper_name)?;
        ccAppendBytes(out_buf, &pos, " = __commonJS((exports, module) => {\n")?;
    } else if (input.wrap_mode == ModuleWrapMode.LazyEsmInit) {
        ccAppendBytes(out_buf, &pos, "var ")?;
        ccAppendBytes(out_buf, &pos, input.wrapper_name)?;
        ccAppendBytes(out_buf, &pos, " = __esm(() => {\n")?;
    } else if (input.wrap_mode == ModuleWrapMode.AsyncEsmInit) {
        ccAppendBytes(out_buf, &pos, "var ")?;
        ccAppendBytes(out_buf, &pos, input.wrapper_name)?;
        ccAppendBytes(out_buf, &pos, " = __asyncEsm(async () => {\n")?;
    }

    if (input.emit_use_strict) {
        ccAppendBytes(out_buf, &pos, "\"use strict\";\n")?;
    }

    const body_start = pos;
    if (input.body_code.len > 0) {
        if (opts.minify_whitespace) {
            const mstats = minifyJsWhitespaceSafe(
                input.body_code,
                MinifyWhitespaceOptions{
                    preserve_legal_comments: opts.preserve_legal_comments,
                    collapse_newlines: true,
                },
                out_buf[pos..],
            )?;
            pos += mstats.output_bytes;
        } else {
            ccAppendBytes(out_buf, &pos, input.body_code)?;
        }
        if (pos > body_start && out_buf[pos - 1] != '\n') {
            ccAppendByte(out_buf, &pos, '\n')?;
        }
    }
    const body_end = pos;

    // Closing wrapper footer
    if (input.wrap_mode != ModuleWrapMode.FlatEsm) {
        ccAppendBytes(out_buf, &pos, "});\n")?;
    }

    // Optional trailing export declarations
    if (input.trailing_exports.len > 0) {
        ccAppendBytes(out_buf, &pos, input.trailing_exports)?;
        if (input.trailing_exports[input.trailing_exports.len - 1] != '\n') {
            ccAppendByte(out_buf, &pos, '\n')?;
        }
    }

    const lines = @as(u32, @intCast(Simd.countByte(out_buf[0..pos], '\n')));
    return ModuleEmitResult{
        bytes_written: pos,
        lines_written: lines,
        body_start_offset: body_start,
        body_end_offset: body_end,
    };
}

// ============================================================================
// Section 3: Chunk-Level Assembler & SIMD SourceMap Line/Column Span Tracker
// ============================================================================

/// Exact byte and 0-based line/column span for one module emitted into a chunk.
/// Directly consumable by SourceMap V3 concatenation pipelines.
pub struct ModuleEmitSpan {
    source_index: u32 = 0,
    source_path: []const u8 = "",
    wrap_mode: ModuleWrapMode = ModuleWrapMode.FlatEsm,
    byte_start: usize = 0,
    byte_end: usize = 0,
    start_line: u32 = 0,
    start_col: u32 = 0,
    end_line: u32 = 0,
    end_col: u32 = 0,

    static fn empty(): ModuleEmitSpan {
        return ModuleEmitSpan{};
    }
}

/// Aggregate chunk assembly statistics.
pub struct ChunkAssembleStats {
    total_bytes: usize = 0,
    total_lines: u32 = 0,
    modules_emitted: usize = 0,
    flat_esm_emitted: u32 = 0,
    commonjs_wrapped: u32 = 0,
    lazy_esm_wrapped: u32 = 0,
    async_esm_wrapped: u32 = 0,
    helpers_injected: u32 = 0,
}

/// Zero-heap, caller-slice-backed JS chunk assembler stitching Banner, Runtime
/// Helpers, wrapped modules, Export Tail, and Footer while tracking exact 0-based
/// `(line, col)` spans via `cap:simd` (`Simd.countByte`).
pub struct ChunkCodeAssembler {
    out_buf: []u8,
    pos: usize = 0,
    cur_line: u32 = 0,
    cur_col: u32 = 0,
    spans: []ModuleEmitSpan,
    span_count: usize = 0,
    opts: ModuleEmitOptions = ModuleEmitOptions{},
    finalized: bool = false,
    flat_esm_emitted: u32 = 0,
    commonjs_wrapped: u32 = 0,
    lazy_esm_wrapped: u32 = 0,
    async_esm_wrapped: u32 = 0,
    helpers_injected: u32 = 0,

    static fn init(
        out_buf: []u8,
        spans_buf: []ModuleEmitSpan,
        opts: ModuleEmitOptions,
    ): ChunkCodeAssembler {
        return ChunkCodeAssembler{
            out_buf: out_buf,
            spans: spans_buf,
            opts: opts,
        };
    }

    fn advanceCursor(*_, written: []const u8) {
        if (written.len == 0) return;
        const nl_count = Simd.countByte(written, '\n');
        if (nl_count == 0) {
            _.cur_col += @as(u32, @intCast(written.len));
        } else {
            _.cur_line += @as(u32, @intCast(nl_count));
            if (last_nl := ccLastNewlineOffset(written)) {
                _.cur_col = @as(u32, @intCast(written.len - last_nl - 1));
            } else {
                _.cur_col = 0;
            }
        }
    }

    fn appendRaw(*_, data: []const u8)! {
        if (_.finalized) return err.AlreadyFinalized;
        if (data.len == 0) return;
        const start = _.pos;
        ccAppendBytes(_.out_buf, &_.pos, data)?;
        _.advanceCursor(_.out_buf[start.._.pos]);
    }

    /// Emits an optional chunk banner (ensuring trailing newline).
    fn emitBanner(*_, banner: []const u8)! {
        if (banner.len == 0) return;
        _.appendRaw(banner)?;
        if (banner[banner.len - 1] != '\n') {
            _.appendRaw("\n")?;
        }
    }

    /// Injects required runtime helper declarations (`__commonJS`, `__esm`, `__asyncEsm`).
    fn emitRuntimeHelpers(*_, need_cjs: bool, need_esm: bool, need_async_esm: bool)! {
        if (need_cjs) {
            _.appendRaw(CHUNK_CODE_COMMONJS_HELPER)?;
            _.helpers_injected += 1;
        }
        if (need_esm) {
            _.appendRaw(CHUNK_CODE_ESM_HELPER)?;
            _.helpers_injected += 1;
        }
        if (need_async_esm) {
            _.appendRaw(CHUNK_CODE_ASYNC_ESM_HELPER)?;
            _.helpers_injected += 1;
        }
    }

    /// Emits a single wrapped module and records its exact byte and line/col span.
    fn emitModule(*_, input: ModuleEmitInput): ChunkCodeEmitError!ModuleEmitSpan {
        if (_.finalized) return err.AlreadyFinalized;
        if (_.span_count >= _.spans.len) return err.SliceCapacityExceeded;

        const byte_start = _.pos;
        const start_line = _.cur_line;
        const start_col = _.cur_col;

        const res = emitWrappedModuleInto(input, _.opts, _.out_buf[_.pos..])?;
        _.pos += res.bytes_written;
        _.advanceCursor(_.out_buf[byte_start.._.pos]);

        const span = ModuleEmitSpan{
            source_index: input.source_index,
            source_path: input.source_path,
            wrap_mode: input.wrap_mode,
            byte_start: byte_start,
            byte_end: _.pos,
            start_line: start_line,
            start_col: start_col,
            end_line: _.cur_line,
            end_col: _.cur_col,
        };
        _.spans[_.span_count] = span;
        _.span_count += 1;

        if (input.wrap_mode == ModuleWrapMode.FlatEsm) {
            _.flat_esm_emitted += 1;
        } else if (input.wrap_mode == ModuleWrapMode.CommonJsClosure) {
            _.commonjs_wrapped += 1;
        } else if (input.wrap_mode == ModuleWrapMode.LazyEsmInit) {
            _.lazy_esm_wrapped += 1;
        } else {
            _.async_esm_wrapped += 1;
        }

        return span;
    }

    /// Emits chunk-level export tail (e.g. `export { foo, bar };`).
    fn emitExportTail(*_, tail: []const u8)! {
        if (tail.len == 0) return;
        _.appendRaw(tail)?;
        if (tail[tail.len - 1] != '\n') {
            _.appendRaw("\n")?;
        }
    }

    /// Emits an optional chunk footer (e.g. `//# sourceMappingURL=...`).
    fn emitFooter(*_, footer: []const u8)! {
        if (footer.len == 0) return;
        _.appendRaw(footer)?;
        if (footer[footer.len - 1] != '\n') {
            _.appendRaw("\n")?;
        }
    }

    /// Finalizes the chunk and returns the emitted byte slice.
    fn finalize(*_): []const u8 {
        _.finalized = true;
        return _.out_buf[0.._.pos];
    }

    /// Returns the slice of recorded `ModuleEmitSpan` entries.
    fn emittedSpans(): []const ModuleEmitSpan {
        return _.spans[0.._.span_count];
    }

    /// Returns aggregate assembly telemetry.
    fn stats(): ChunkAssembleStats {
        return ChunkAssembleStats{
            total_bytes: _.pos,
            total_lines: _.cur_line,
            modules_emitted: _.span_count,
            flat_esm_emitted: _.flat_esm_emitted,
            commonjs_wrapped: _.commonjs_wrapped,
            lazy_esm_wrapped: _.lazy_esm_wrapped,
            async_esm_wrapped: _.async_esm_wrapped,
            helpers_injected: _.helpers_injected,
        };
    }
}

// ============================================================================
// Section 4: Diagnostic Emit Span Table Formatter
// ============================================================================

/// Formats a human-readable line-offset span table for SourceMap diagnostics.
pub fn formatChunkEmitSpanTable(
    spans: []const ModuleEmitSpan,
    out_buf: []u8,
): ChunkCodeEmitError!usize {
    let pos: usize = 0;
    ccAppendBytes(out_buf, &pos, "Chunk Module Emit Spans\n")?;
    let i: usize = 0;
    while (i < spans.len) {
        const sp = spans[i];
        ccAppendBytes(out_buf, &pos, "  [")?;
        ccAppendU64(out_buf, &pos, @as(u64, sp.source_index))?;
        ccAppendBytes(out_buf, &pos, "] ")?;
        ccAppendBytes(out_buf, &pos, if (sp.source_path.len > 0) sp.source_path else "<anon>")?;
        ccAppendBytes(out_buf, &pos, " (")?;
        ccAppendBytes(out_buf, &pos, moduleWrapModeName(sp.wrap_mode))?;
        ccAppendBytes(out_buf, &pos, ") L")?;
        ccAppendU64(out_buf, &pos, @as(u64, sp.start_line))?;
        ccAppendByte(out_buf, &pos, ':')?;
        ccAppendU64(out_buf, &pos, @as(u64, sp.start_col))?;
        ccAppendBytes(out_buf, &pos, "..L")?;
        ccAppendU64(out_buf, &pos, @as(u64, sp.end_line))?;
        ccAppendByte(out_buf, &pos, ':')?;
        ccAppendU64(out_buf, &pos, @as(u64, sp.end_col))?;
        ccAppendBytes(out_buf, &pos, " bytes=")?;
        ccAppendU64(out_buf, &pos, @as(u64, sp.byte_end - sp.byte_start))?;
        ccAppendByte(out_buf, &pos, '\n')?;
        i += 1;
    }
    return pos;
}

/// Resolved module ownership for a 0-based generated chunk line number.
pub struct GeneratedLineOwner {
    span_index: usize = 0,
    source_index: u32 = 0,
    source_path: []const u8 = "",
    module_relative_line: u32 = 0,
}

/// Performs O(log M) binary search over sorted `spans` to map a 0-based generated
/// chunk line number back to the originating `ModuleEmitSpan` and relative line.
pub fn resolveGeneratedLineToModule(
    spans: []const ModuleEmitSpan,
    generated_line: u32,
): GeneratedLineOwner? {
    if (spans.len == 0) return null;
    let lo: usize = 0;
    let hi: usize = spans.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        const sp = spans[mid];
        if (generated_line < sp.start_line) {
            hi = mid;
        } else if (generated_line >= sp.end_line) {
            lo = mid + 1;
        } else {
            return GeneratedLineOwner{
                span_index: mid,
                source_index: sp.source_index,
                source_path: sp.source_path,
                module_relative_line: generated_line - sp.start_line,
            };
        }
    }
    return null;
}

/// One-shot batch chunk emission pipeline: inspects `modules` to inject only the
/// needed runtime helpers (`__commonJS`, `__esm`, `__asyncEsm`), emits banner,
/// helpers, all wrapped modules, export tail, and footer into `out_buf`, recording
/// exact line/column spans into `spans_buf`.
pub fn emitModuleBatchIntoChunk(
    banner: []const u8,
    modules: []const ModuleEmitInput,
    export_tail: []const u8,
    footer: []const u8,
    opts: ModuleEmitOptions,
    out_buf: []u8,
    spans_buf: []ModuleEmitSpan,
): ChunkCodeEmitError!ChunkAssembleStats {
    let need_cjs = false;
    let need_esm = false;
    let need_async_esm = false;
    let i: usize = 0;
    while (i < modules.len) {
        const wm = modules[i].wrap_mode;
        if (wm == ModuleWrapMode.CommonJsClosure) need_cjs = true;
        if (wm == ModuleWrapMode.LazyEsmInit) need_esm = true;
        if (wm == ModuleWrapMode.AsyncEsmInit) need_async_esm = true;
        i += 1;
    }

    let assembler = ChunkCodeAssembler.init(out_buf, spans_buf, opts);
    assembler.emitBanner(banner)?;
    assembler.emitRuntimeHelpers(need_cjs, need_esm, need_async_esm)?;

    let m: usize = 0;
    while (m < modules.len) {
        * = assembler.emitModule(modules[m])?;
        m += 1;
    }

    assembler.emitExportTail(export_tail)?;
    assembler.emitFooter(footer)?;
    * = assembler.finalize();
    return assembler.stats();
}
