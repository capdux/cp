// Source: Absorbed from /data/dev/bun/src/bundler/ParseTask.cp | Synced: 2026-11-23T15:28:17Z
// ============================================================================
// lib/bundler/parseScheduler.cp — Zero-Alloc SIMD Import Scanner & Parse Pool
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven parallel frontend parse scheduler,
// `cap:simd`-accelerated import specifier pre-scanner, and O(log L) source line
// diagnostic engine for `cap build`:
//   1. Zero-copy SIMD import specifier pre-scanner (`scanImportSpecifiersFast`)
//      extracting static `import ... from "..."`, side-effect `import "..."`,
//      `export ... from "..."`, `import("...")`, and `require("...")` while
//      skipping `//` comments, `/* */` block comments, and string literals;
//   2. Deduplicated, concurrency-throttled parse task pool (`ParseTaskScheduler`:
//      `Queued -> Parsing -> Parsed | CachedHit | Failed | Cancelled`) with
//      priority ordering and `FailFast` vs `CollectAllErrors` policies;
//   3. SIMD-accelerated line index (`SourceLineTable` using `Simd.indexOfByte`)
//      with O(log L) offset-to-`(line, col)` lookup and AI-friendly caret
//      diagnostic snippet renderer (`formatDiagnosticSnippet`);
//   4. Parse throughput & summary reporter (`formatParseSchedulerReport`).

import { Simd } from "cap:simd";

/// Default parallel parser worker limit.
pub const PARSE_SCHED_DEFAULT_WORKERS: u32 = 4;

/// Sentinel module ID indicating no module.
pub const PARSE_SCHED_INVALID_MODULE: u32 = 0xFFFFFFFF;

/// Parse scheduler, import scanner, and line table error set.
pub err ParseSchedulerError {
    InvalidTaskIndex,
    InvalidStateTransition,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte & Formatting Helpers
// ============================================================================

fn psBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn psIsIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

fn psIsIdentContinue(c: u8): bool {
    return psIsIdentStart(c) || (c >= '0' && c <= '9');
}

fn psIsWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn psAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn psAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn psAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        psAppendByte(out, pos, '0')?;
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
        psAppendByte(out, pos, tmp[n])?;
    }
}

fn psAppendRepeated(out: []u8, pos: *usize, ch: u8, count: usize)! {
    let i: usize = 0;
    while (i < count) {
        psAppendByte(out, pos, ch)?;
        i += 1;
    }
}

// ============================================================================
// Section 1: Zero-Copy SIMD Import Specifier Pre-Scanner
// ============================================================================

/// Kind of import/export/require reference discovered by `scanImportSpecifiersFast`.
pub enum ScannedImportKind: u8 {
    StaticImport = 0,
    StaticReExport = 1,
    DynamicImport = 2,
    CommonJsRequire = 3,
}

/// Zero-copy import specifier record pointing directly into the input source buffer.
pub struct ScannedImportEntry {
    kind: ScannedImportKind = ScannedImportKind.StaticImport,
    specifier: []const u8 = "",
    offset: usize = 0,
    is_type_only: bool = false,

    static fn empty(): ScannedImportEntry {
        return ScannedImportEntry{};
    }
}

/// Aggregate counters returned by `scanImportSpecifiersFast`.
pub struct ImportScanSummary {
    total_found: usize = 0,
    static_imports: u32 = 0,
    static_reexports: u32 = 0,
    dynamic_imports: u32 = 0,
    cjs_requires: u32 = 0,
    comments_skipped: u32 = 0,
}

fn skipWsAndComments(src: []const u8, pos: *usize, comments_skipped: *u32) {
    while (pos.* < src.len) {
        const c = src[pos.*];
        if (psIsWs(c)) {
            pos.* += 1;
            continue;
        }
        if (c == '/' && pos.* + 1 < src.len) {
            if (src[pos.* + 1] == '/') {
                comments_skipped.* += 1;
                pos.* += 2;
                if (nl := Simd.indexOfByte(src[pos.*..src.len], '\n')) {
                    pos.* += nl + 1;
                } else {
                    pos.* = src.len;
                }
                continue;
            }
            if (src[pos.* + 1] == '*') {
                comments_skipped.* += 1;
                pos.* += 2;
                while (pos.* < src.len) {
                    if (star := Simd.indexOfByte(src[pos.*..src.len], '*')) {
                        pos.* += star + 1;
                        if (pos.* < src.len && src[pos.*] == '/') {
                            pos.* += 1;
                            break;
                        }
                    } else {
                        pos.* = src.len;
                        break;
                    }
                }
                continue;
            }
        }
        break;
    }
}

/// Scans a single- or double-quoted string literal starting at `src[pos.*]`.
/// Returns the inner unquoted slice (zero-copy) and advances `pos.*` past closing quote.
fn scanQuotedLiteral(src: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= src.len) return null;
    const quote = src[pos.*];
    if (quote != '"' && quote != '\'') return null;
    const inner_start = pos.* + 1;
    pos.* += 1;
    while (pos.* < src.len) {
        const c = src[pos.*];
        if (c == '\\') {
            pos.* = if (pos.* + 2 <= src.len) pos.* + 2 else src.len;
        } else if (c == quote) {
            const inner = src[inner_start..pos.*];
            pos.* += 1;
            return inner;
        } else if (c == '\n') {
            return null;
        } else {
            pos.* += 1;
        }
    }
    return null;
}

fn skipTemplateLiteral(src: []const u8, pos: *usize) {
    if (pos.* >= src.len || src[pos.*] != '`') return;
    pos.* += 1;
    while (pos.* < src.len) {
        const c = src[pos.*];
        if (c == '\\') {
            pos.* = if (pos.* + 2 <= src.len) pos.* + 2 else src.len;
        } else if (c == '`') {
            pos.* += 1;
            return;
        } else {
            pos.* += 1;
        }
    }
}

fn recordScannedImport(
    out: []ScannedImportEntry,
    sum: *ImportScanSummary,
    kind: ScannedImportKind,
    spec: []const u8,
    offset: usize,
    is_type_only: bool,
)! {
    if (sum.total_found >= out.len) return err.SliceCapacityExceeded;
    out[sum.total_found] = ScannedImportEntry{
        kind: kind,
        specifier: spec,
        offset: offset,
        is_type_only: is_type_only,
    };
    sum.total_found += 1;
    switch (kind) {
        .StaticImport => { sum.static_imports += 1; },
        .StaticReExport => { sum.static_reexports += 1; },
        .DynamicImport => { sum.dynamic_imports += 1; },
        .CommonJsRequire => { sum.cjs_requires += 1; },
    }
}

/// Fast zero-copy pre-scanner extracting static imports, static re-exports,
/// dynamic `import("...")`, and `require("...")` while skipping comments and strings.
pub fn scanImportSpecifiersFast(
    source: []const u8,
    out: []ScannedImportEntry,
): ImportScanSummary! {
    let sum = ImportScanSummary{};
    let pos: usize = 0;
    let prev_non_ws: u8 = 0;

    while (pos < source.len) {
        const c = source[pos];

        if (psIsWs(c)) {
            pos += 1;
            continue;
        }

        // Skip single-line & block comments using SIMD byte search
        if (c == '/' && pos + 1 < source.len && (source[pos + 1] == '/' || source[pos + 1] == '*')) {
            skipWsAndComments(source, &pos, &sum.comments_skipped);
            continue;
        }

        // Skip string & template literals so embedded fake imports are ignored
        if (c == '"' || c == '\'') {
            * = scanQuotedLiteral(source, &pos);
            prev_non_ws = c;
            continue;
        }
        if (c == '`') {
            skipTemplateLiteral(source, &pos);
            prev_non_ws = '`';
            continue;
        }

        if (psIsIdentStart(c)) {
            const id_start = pos;
            pos += 1;
            while (pos < source.len && psIsIdentContinue(source[pos])) {
                pos += 1;
            }
            const ident = source[id_start..pos];

            // Ignore member property access like `obj.require("x")` or `obj.import("x")`
            if (prev_non_ws == '.') {
                prev_non_ws = 'a';
                continue;
            }

            if (psBytesEql(ident, "require")) {
                let look = pos;
                skipWsAndComments(source, &look, &sum.comments_skipped);
                if (look < source.len && source[look] == '(') {
                    look += 1;
                    skipWsAndComments(source, &look, &sum.comments_skipped);
                    const spec_off = look + 1;
                    if (spec := scanQuotedLiteral(source, &look)) {
                        skipWsAndComments(source, &look, &sum.comments_skipped);
                        if (look < source.len && source[look] == ')') {
                            recordScannedImport(out, &sum, ScannedImportKind.CommonJsRequire, spec, spec_off, false)?;
                            pos = look + 1;
                            prev_non_ws = ')';
                            continue;
                        }
                    }
                }
            } else if (psBytesEql(ident, "import")) {
                let look = pos;
                skipWsAndComments(source, &look, &sum.comments_skipped);

                // Case A: Dynamic `import("...")`
                if (look < source.len && source[look] == '(') {
                    look += 1;
                    skipWsAndComments(source, &look, &sum.comments_skipped);
                    const spec_off = look + 1;
                    if (spec := scanQuotedLiteral(source, &look)) {
                        recordScannedImport(out, &sum, ScannedImportKind.DynamicImport, spec, spec_off, false)?;
                        pos = look;
                        prev_non_ws = ')';
                        continue;
                    }
                }

                // Case B: Side-effect `import "mod"`
                if (look < source.len && (source[look] == '"' || source[look] == '\'')) {
                    const spec_off = look + 1;
                    if (spec := scanQuotedLiteral(source, &look)) {
                        recordScannedImport(out, &sum, ScannedImportKind.StaticImport, spec, spec_off, false)?;
                        pos = look;
                        prev_non_ws = '"';
                        continue;
                    }
                }

                // Case C: `import [type] ... from "mod"`
                let is_type = false;
                if (look + 4 <= source.len && psBytesEql(source[look..look + 4], "type") &&
                    (look + 4 == source.len || !psIsIdentContinue(source[look + 4]))) {
                    is_type = true;
                }

                // Scan forward within this statement up to `;` or `from "..."`
                let scan = look;
                let brace_depth: u32 = 0;
                while (scan < source.len) {
                    skipWsAndComments(source, &scan, &sum.comments_skipped);
                    if (scan >= source.len) break;
                    const ch = source[scan];
                    if (ch == '{') {
                        brace_depth += 1;
                        scan += 1;
                    } else if (ch == '}') {
                        if (brace_depth > 0) brace_depth -= 1;
                        scan += 1;
                    } else if (ch == ';' && brace_depth == 0) {
                        break;
                    } else if (brace_depth == 0 && psIsIdentStart(ch)) {
                        const kw_start = scan;
                        scan += 1;
                        while (scan < source.len && psIsIdentContinue(source[scan])) scan += 1;
                        if (psBytesEql(source[kw_start..scan], "from")) {
                            skipWsAndComments(source, &scan, &sum.comments_skipped);
                            const spec_off = scan + 1;
                            if (spec := scanQuotedLiteral(source, &scan)) {
                                recordScannedImport(out, &sum, ScannedImportKind.StaticImport, spec, spec_off, is_type)?;
                                pos = scan;
                            }
                            break;
                        }
                    } else if (ch == '"' || ch == '\'') {
                        * = scanQuotedLiteral(source, &scan);
                    } else {
                        scan += 1;
                    }
                }
            } else if (psBytesEql(ident, "export")) {
                let look = pos;
                skipWsAndComments(source, &look, &sum.comments_skipped);
                let is_type = false;
                if (look + 4 <= source.len && psBytesEql(source[look..look + 4], "type") &&
                    (look + 4 == source.len || !psIsIdentContinue(source[look + 4]))) {
                    is_type = true;
                    look += 4;
                    skipWsAndComments(source, &look, &sum.comments_skipped);
                }

                // Only re-export forms `export { ... } from "..."` or `export * [as ns] from "..."`
                if (look < source.len && (source[look] == '{' || source[look] == '*')) {
                    let scan = look;
                    let brace_depth: u32 = 0;
                    while (scan < source.len) {
                        skipWsAndComments(source, &scan, &sum.comments_skipped);
                        if (scan >= source.len) break;
                        const ch = source[scan];
                        if (ch == '{') {
                            brace_depth += 1;
                            scan += 1;
                        } else if (ch == '}') {
                            if (brace_depth > 0) brace_depth -= 1;
                            scan += 1;
                        } else if (ch == ';' && brace_depth == 0) {
                            break;
                        } else if (brace_depth == 0 && psIsIdentStart(ch)) {
                            const kw_start = scan;
                            scan += 1;
                            while (scan < source.len && psIsIdentContinue(source[scan])) scan += 1;
                            if (psBytesEql(source[kw_start..scan], "from")) {
                                skipWsAndComments(source, &scan, &sum.comments_skipped);
                                const spec_off = scan + 1;
                                if (spec := scanQuotedLiteral(source, &scan)) {
                                    recordScannedImport(out, &sum, ScannedImportKind.StaticReExport, spec, spec_off, is_type)?;
                                    pos = scan;
                                }
                                break;
                            }
                        } else if (ch == '"' || ch == '\'') {
                            * = scanQuotedLiteral(source, &scan);
                        } else {
                            scan += 1;
                        }
                    }
                }
            }

            prev_non_ws = 'a';
            continue;
        }

        prev_non_ws = c;
        pos += 1;
    }

    return sum;
}

// ============================================================================
// Section 2: Parallel Parse Task Pool Scheduler
// ============================================================================

/// Execution status of a module parse task in `ParseTaskScheduler`.
pub enum ParseTaskStatus: u8 {
    Queued = 0,
    Parsing = 1,
    Parsed = 2,
    CachedHit = 3,
    Failed = 4,
    Cancelled = 5,

    fn isTerminal(): bool {
        return switch (_) {
            .Parsed, .CachedHit, .Failed, .Cancelled => true,
            else => false,
        };
    }
}

/// Error-handling policy for `ParseTaskScheduler`.
pub enum ParseFailureMode: u8 {
    FailFast = 0,
    CollectAllErrors = 1,
}

/// Single module parse task stored in caller-owned backing slice.
pub struct ParseTaskEntry {
    module_id: u32 = PARSE_SCHED_INVALID_MODULE,
    path: []const u8 = "",
    byte_size: u64 = 0,
    priority: u16 = 0,
    status: ParseTaskStatus = ParseTaskStatus.Queued,
    started_at_ms: u64 = 0,
    finished_at_ms: u64 = 0,
    imports_discovered: u32 = 0,
    error_code: []const u8 = "",
    error_offset: usize = 0,
    error_message: []const u8 = "",

    static fn empty(): ParseTaskEntry {
        return ParseTaskEntry{};
    }
}

/// Worker dispatch payload returned by `ParseTaskScheduler.dispatchBatch`.
pub struct ParseDispatchItem {
    task_index: u32 = 0,
    module_id: u32 = 0,
    path: []const u8 = "",
    byte_size: u64 = 0,

    static fn empty(): ParseDispatchItem {
        return ParseDispatchItem{};
    }
}

/// Aggregate parse pool statistics.
pub struct ParseSchedulerStats {
    total_tasks: u32 = 0,
    queued: u32 = 0,
    parsing: u32 = 0,
    parsed: u32 = 0,
    cached_hits: u32 = 0,
    failed: u32 = 0,
    cancelled: u32 = 0,
    total_bytes_parsed: u64 = 0,
    total_imports_discovered: u32 = 0,
    total_parse_ms: u64 = 0,
    deduplicated_enqueues: u32 = 0,
}

/// Zero-allocation, deduplicated, worker-throttled frontend parse task pool.
pub struct ParseTaskScheduler {
    tasks: []ParseTaskEntry,
    task_count: usize = 0,
    max_workers: u32 = PARSE_SCHED_DEFAULT_WORKERS,
    failure_mode: ParseFailureMode = ParseFailureMode.CollectAllErrors,
    aborted: bool = false,
    deduplicated_enqueues: u32 = 0,

    static fn init(
        tasks: []ParseTaskEntry,
        max_workers: u32,
        failure_mode: ParseFailureMode,
    ): ParseTaskScheduler {
        const w = if (max_workers == 0) @as(u32, 1) else max_workers;
        return ParseTaskScheduler{
            tasks: tasks,
            max_workers: w,
            failure_mode: failure_mode,
        };
    }

    /// Enqueues a module parse task, deduplicating by `module_id` (and boosting priority if queued).
    fn enqueue(*_, module_id: u32, path: []const u8, byte_size: u64, priority: u16): u32! {
        let i: usize = 0;
        while (i < _.task_count) {
            if (_.tasks[i].module_id == module_id) {
                _.deduplicated_enqueues += 1;
                if (_.tasks[i].status == ParseTaskStatus.Queued && priority > _.tasks[i].priority) {
                    _.tasks[i].priority = priority;
                }
                return @as(u32, @intCast(i));
            }
            i += 1;
        }

        if (_.task_count >= _.tasks.len) return err.SliceCapacityExceeded;
        const idx = @as(u32, @intCast(_.task_count));
        _.tasks[_.task_count] = ParseTaskEntry{
            module_id: module_id,
            path: path,
            byte_size: byte_size,
            priority: priority,
            status: if (_.aborted) ParseTaskStatus.Cancelled else ParseTaskStatus.Queued,
        };
        _.task_count += 1;
        return idx;
    }

    /// Marks a queued task as satisfied from persistent AST cache without dispatching a worker.
    fn markCachedHit(*_, task_idx: u32, imports_discovered: u32)! {
        if (@as(usize, task_idx) >= _.task_count) return err.InvalidTaskIndex;
        const idx = @as(usize, task_idx);
        if (_.tasks[idx].status != ParseTaskStatus.Queued) return err.InvalidStateTransition;
        _.tasks[idx].status = ParseTaskStatus.CachedHit;
        _.tasks[idx].imports_discovered = imports_discovered;
    }

    fn activeWorkers(): u32 {
        let n: u32 = 0;
        let i: usize = 0;
        while (i < _.task_count) {
            if (_.tasks[i].status == ParseTaskStatus.Parsing) n += 1;
            i += 1;
        }
        return n;
    }

    /// Dispatches highest-priority `Queued` tasks up to `max_workers` concurrency limit.
    fn dispatchBatch(*_, now_ms: u64, out_batch: []ParseDispatchItem): usize {
        if (_.aborted) return 0;
        const active = _.activeWorkers();
        if (active >= _.max_workers) return 0;

        const avail = @as(usize, _.max_workers - active);
        const limit = if (avail < out_batch.len) avail else out_batch.len;
        let dispatched: usize = 0;

        while (dispatched < limit) {
            let best: usize = _.task_count;
            let best_prio: u16 = 0;
            let i: usize = 0;
            while (i < _.task_count) {
                if (_.tasks[i].status == ParseTaskStatus.Queued) {
                    if (best == _.task_count || _.tasks[i].priority > best_prio) {
                        best = i;
                        best_prio = _.tasks[i].priority;
                    }
                }
                i += 1;
            }
            if (best == _.task_count) break;

            _.tasks[best].status = ParseTaskStatus.Parsing;
            _.tasks[best].started_at_ms = now_ms;
            out_batch[dispatched] = ParseDispatchItem{
                task_index: @as(u32, @intCast(best)),
                module_id: _.tasks[best].module_id,
                path: _.tasks[best].path,
                byte_size: _.tasks[best].byte_size,
            };
            dispatched += 1;
        }

        return dispatched;
    }

    /// Records successful completion of a `Parsing` task.
    fn completeParsed(*_, task_idx: u32, now_ms: u64, imports_discovered: u32)! {
        if (@as(usize, task_idx) >= _.task_count) return err.InvalidTaskIndex;
        const idx = @as(usize, task_idx);
        if (_.tasks[idx].status != ParseTaskStatus.Parsing) return err.InvalidStateTransition;
        _.tasks[idx].status = ParseTaskStatus.Parsed;
        _.tasks[idx].finished_at_ms = now_ms;
        _.tasks[idx].imports_discovered = imports_discovered;
    }

    /// Records parse failure; under `FailFast`, cancels all remaining queued tasks.
    fn completeFailed(
        *_,
        task_idx: u32,
        now_ms: u64,
        error_code: []const u8,
        error_offset: usize,
        error_message: []const u8,
    )! {
        if (@as(usize, task_idx) >= _.task_count) return err.InvalidTaskIndex;
        const idx = @as(usize, task_idx);
        if (_.tasks[idx].status != ParseTaskStatus.Parsing) return err.InvalidStateTransition;
        _.tasks[idx].status = ParseTaskStatus.Failed;
        _.tasks[idx].finished_at_ms = now_ms;
        _.tasks[idx].error_code = error_code;
        _.tasks[idx].error_offset = error_offset;
        _.tasks[idx].error_message = error_message;

        if (_.failure_mode == ParseFailureMode.FailFast) {
            _.aborted = true;
            let i: usize = 0;
            while (i < _.task_count) {
                if (_.tasks[i].status == ParseTaskStatus.Queued) {
                    _.tasks[i].status = ParseTaskStatus.Cancelled;
                }
                i += 1;
            }
        }
    }

    fn isDrained(): bool {
        let i: usize = 0;
        while (i < _.task_count) {
            if (!_.tasks[i].status.isTerminal()) return false;
            i += 1;
        }
        return true;
    }

    fn stats(): ParseSchedulerStats {
        let st = ParseSchedulerStats{
            total_tasks: @as(u32, @intCast(_.task_count)),
            deduplicated_enqueues: _.deduplicated_enqueues,
        };
        let i: usize = 0;
        while (i < _.task_count) {
            const t = _.tasks[i];
            switch (t.status) {
                .Queued => { st.queued += 1; },
                .Parsing => { st.parsing += 1; },
                .Parsed => {
                    st.parsed += 1;
                    st.total_bytes_parsed += t.byte_size;
                    st.total_imports_discovered += t.imports_discovered;
                    if (t.finished_at_ms >= t.started_at_ms) {
                        st.total_parse_ms += t.finished_at_ms - t.started_at_ms;
                    }
                },
                .CachedHit => {
                    st.cached_hits += 1;
                    st.total_imports_discovered += t.imports_discovered;
                },
                .Failed => {
                    st.failed += 1;
                    if (t.finished_at_ms >= t.started_at_ms) {
                        st.total_parse_ms += t.finished_at_ms - t.started_at_ms;
                    }
                },
                .Cancelled => { st.cancelled += 1; },
            }
            i += 1;
        }
        return st;
    }
}

// ============================================================================
// Section 3: SIMD Source Line Table & AI-Friendly Diagnostic Formatter
// ============================================================================

/// 1-based line/column position and byte span of the enclosing source line.
pub struct SourceLineCol {
    line: u32 = 1,
    column: u32 = 1,
    line_start_offset: usize = 0,
    line_end_offset: usize = 0,
}

/// Zero-allocation line-start offset index built via `Simd.indexOfByte(..., '\n')`
/// for O(log L) byte-offset-to-`(line, column)` resolution.
pub struct SourceLineTable {
    line_starts: []usize,
    line_count: usize = 0,
    source_len: usize = 0,

    /// Builds a line offset table from `source` into caller-owned `line_starts_buf`.
    static fn build(source: []const u8, line_starts_buf: []usize): SourceLineTable! {
        if (line_starts_buf.len == 0) return err.SliceCapacityExceeded;
        line_starts_buf[0] = 0;
        let count: usize = 1;
        let pos: usize = 0;

        while (pos < source.len) {
            if (nl := Simd.indexOfByte(source[pos..source.len], '\n')) {
                const next_line_start = pos + nl + 1;
                if (next_line_start < source.len) {
                    if (count >= line_starts_buf.len) return err.SliceCapacityExceeded;
                    line_starts_buf[count] = next_line_start;
                    count += 1;
                }
                pos = next_line_start;
            } else {
                break;
            }
        }

        return SourceLineTable{
            line_starts: line_starts_buf,
            line_count: count,
            source_len: source.len,
        };
    }

    /// Resolves a byte offset to 1-based `(line, column)` via O(log L) binary search.
    fn resolveOffset(source: []const u8, byte_offset: usize): SourceLineCol {
        if (_.line_count == 0) return SourceLineCol{};
        const clamped = if (byte_offset > source.len) source.len else byte_offset;

        let lo: usize = 0;
        let hi: usize = _.line_count;
        while (lo + 1 < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.line_starts[mid] <= clamped) {
                lo = mid;
            } else {
                hi = mid;
            }
        }

        const start_off = _.line_starts[lo];
        let end_off = if (lo + 1 < _.line_count) _.line_starts[lo + 1] else source.len;
        if (end_off > start_off && source[end_off - 1] == '\n') end_off -= 1;
        if (end_off > start_off && source[end_off - 1] == '\r') end_off -= 1;

        return SourceLineCol{
            line: @as(u32, @intCast(lo + 1)),
            column: @as(u32, @intCast((clamped - start_off) + 1)),
            line_start_offset: start_off,
            line_end_offset: end_off,
        };
    }

    /// Renders an AI-friendly diagnostic snippet with `file:line:col`, source context,
    /// aligned `^^^` caret markers, and optional `= help:` guidance.
    fn formatDiagnosticSnippet(
        source: []const u8,
        file_path: []const u8,
        code: []const u8,
        message: []const u8,
        help: []const u8,
        byte_offset: usize,
        caret_len: usize,
        out: []u8,
    ): usize! {
        const loc = _.resolveOffset(source, byte_offset);
        let pos: usize = 0;

        psAppendBytes(out, &pos, "error[")?;
        psAppendBytes(out, &pos, code)?;
        psAppendBytes(out, &pos, "]: ")?;
        psAppendBytes(out, &pos, message)?;
        psAppendBytes(out, &pos, "\n  --> ")?;
        psAppendBytes(out, &pos, file_path)?;
        psAppendByte(out, &pos, ':')?;
        psAppendU64(out, &pos, @as(u64, loc.line))?;
        psAppendByte(out, &pos, ':')?;
        psAppendU64(out, &pos, @as(u64, loc.column))?;
        psAppendBytes(out, &pos, "\n   |\n ")?;
        psAppendU64(out, &pos, @as(u64, loc.line))?;
        psAppendBytes(out, &pos, " | ")?;
        psAppendBytes(out, &pos, source[loc.line_start_offset..loc.line_end_offset])?;
        psAppendBytes(out, &pos, "\n   | ")?;

        const col_pad = if (loc.column > 0) @as(usize, loc.column - 1) else 0;
        psAppendRepeated(out, &pos, ' ', col_pad)?;
        const carets = if (caret_len == 0) @as(usize, 1) else caret_len;
        psAppendRepeated(out, &pos, '^', carets)?;
        psAppendByte(out, &pos, '\n')?;

        if (help.len > 0) {
            psAppendBytes(out, &pos, "   = help: ")?;
            psAppendBytes(out, &pos, help)?;
            psAppendByte(out, &pos, '\n')?;
        }
        return pos;
    }
}

// ============================================================================
// Section 4: Parse Scheduler Throughput & Summary Report
// ============================================================================

/// Formats a concise parse pool summary including cache hits, imports, and throughput.
pub fn formatParseSchedulerReport(stats: ParseSchedulerStats, out: []u8): usize! {
    let pos: usize = 0;
    psAppendBytes(out, &pos, "Parse Summary: ")?;
    psAppendU64(out, &pos, @as(u64, stats.parsed))?;
    psAppendBytes(out, &pos, " parsed, ")?;
    psAppendU64(out, &pos, @as(u64, stats.cached_hits))?;
    psAppendBytes(out, &pos, " cached, ")?;
    psAppendU64(out, &pos, @as(u64, stats.failed))?;
    psAppendBytes(out, &pos, " failed")?;
    if (stats.cancelled > 0) {
        psAppendBytes(out, &pos, ", ")?;
        psAppendU64(out, &pos, @as(u64, stats.cancelled))?;
        psAppendBytes(out, &pos, " cancelled")?;
    }
    psAppendBytes(out, &pos, " (")?;
    psAppendU64(out, &pos, @as(u64, stats.total_imports_discovered))?;
    psAppendBytes(out, &pos, " imports, ")?;
    psAppendU64(out, &pos, @as(u64, stats.deduplicated_enqueues))?;
    psAppendBytes(out, &pos, " deduped)")?;

    if (stats.total_parse_ms > 0) {
        // Throughput in KB/s = (bytes * 1000) / (ms * 1024)
        const kb_per_sec = (stats.total_bytes_parsed * 1000) / (stats.total_parse_ms * 1024);
        psAppendBytes(out, &pos, " | ")?;
        psAppendU64(out, &pos, kb_per_sec)?;
        psAppendBytes(out, &pos, " KB/s\n")?;
    } else {
        psAppendByte(out, &pos, '\n')?;
    }
    return pos;
}
