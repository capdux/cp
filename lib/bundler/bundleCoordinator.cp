// Source: Absorbed from /data/dev/bun/src/bundler/bundle_v2.cp | Synced: 2026-11-22T12:08:55Z
// ============================================================================
// lib/bundler/bundleCoordinator.cp — Zero-Alloc Bundler Coordinator & VLQ Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven end-to-end bundling coordinator,
// SourceMap V3 Base64-VLQ codec, and virtual-module plugin hook pipeline:
//   1. SourceMap V3 Base64-VLQ codec (`encodeVlqI32`, `decodeVlqI32`) &
//      zero-heap V3 JSON encoder (`encodeSourceMapV3Json`) supporting full
//      relative column/source/line/name deltas across lines;
//   2. Virtual module & plugin lifecycle hook registry (`BundlePluginRegistry`)
//      with priority-ordered `onResolve` (prefix/suffix/exact/wildcard +
//      namespace filtering + cyclic redirect depth guard) and `onLoad` virtual
//      source injection;
//   3. End-to-end 6-stage bundler coordinator (`BundleCoordinator`:
//      ResolveEntries -> ScanGraph -> RunPlugins -> GenerateChunks ->
//      EmitSourceMaps -> WriteManifest), structured diagnostics (file/line/col/
//      code), chunk byte-budget enforcement (`max_chunk_bytes`), and
//      deterministic JSON Metafile emission (`formatMetafileJson`).

/// Default maximum chained `onResolve` redirect hops before flagging a cycle.
pub const BUNDLE_COORD_MAX_RESOLVE_DEPTH: u32 = 16;

/// SourceMap specification version emitted by `encodeSourceMapV3Json`.
pub const BUNDLE_COORD_SOURCEMAP_VERSION: u32 = 3;

/// Sentinel `name_idx` indicating a 4-field SourceMap segment without a symbol name.
pub const BUNDLE_COORD_NO_NAME: i32 = -1;

/// Bundler coordinator, VLQ codec, and plugin pipeline error set.
pub err BundleCoordinatorError {
    InvalidVlqDigit,
    VlqOverflow,
    TruncatedVlq,
    PluginResolveDepthExceeded,
    InvalidStageTransition,
    BuildAbortedOnError,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, Wildcard & JSON Escaping Helpers
// ============================================================================

const VLQ_BASE64_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

fn bcoBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn bcoStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return bcoBytesEql(s[0..prefix.len], prefix);
}

fn bcoEndsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    return bcoBytesEql(s[s.len - suffix.len..s.len], suffix);
}

fn bcoWildcardMatch(pattern: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = 0;
    let star_ti: usize = 0;
    let has_star = false;

    while (ti < text.len) {
        if (pi < pattern.len && pattern[pi] == '*') {
            has_star = true;
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (pi < pattern.len && pattern[pi] == text[ti]) {
            pi += 1;
            ti += 1;
        } else if (has_star) {
            star_ti += 1;
            ti = star_ti;
            pi = star_pi + 1;
        } else {
            return false;
        }
    }
    while (pi < pattern.len && pattern[pi] == '*') {
        pi += 1;
    }
    return pi == pattern.len;
}

fn bcoAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn bcoAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn bcoAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        bcoAppendByte(out, pos, '0')?;
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
        bcoAppendByte(out, pos, tmp[n])?;
    }
}

fn bcoAppendHex64(out: []u8, pos: *usize, val: u64)! {
    const hex = "0123456789abcdef";
    let shift: u6 = 60;
    while (true) {
        const nib: usize = @as(usize, @intCast((val >> shift) & 0xF));
        bcoAppendByte(out, pos, hex[nib])?;
        if (shift == 0) break;
        shift -= 4;
    }
}

fn bcoAppendJsonString(out: []u8, pos: *usize, s: []const u8)! {
    bcoAppendByte(out, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            bcoAppendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            bcoAppendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            bcoAppendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            bcoAppendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            bcoAppendBytes(out, pos, "\\t")?;
        } else {
            bcoAppendByte(out, pos, c)?;
        }
        i += 1;
    }
    bcoAppendByte(out, pos, '"')?;
}

// ============================================================================
// Section 1: SourceMap V3 Base64-VLQ Codec & Zero-Heap JSON Encoder
// ============================================================================

/// Result of decoding a single Base64-VLQ signed integer.
pub struct VlqDecodeResult {
    value: i32 = 0,
    consumed: usize = 0,
}

fn vlqBase64Value(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - @as(u8, 'A');
    if (c >= 'a' && c <= 'z') return (c - @as(u8, 'a')) + 26;
    if (c >= '0' && c <= '9') return (c - @as(u8, '0')) + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return null;
}

/// Encodes a signed 32-bit integer into SourceMap V3 Base64-VLQ format.
pub fn encodeVlqI32(value: i32, out: []u8, pos: *usize)! {
    let vlq: u64 = if (value < 0)
        (@as(u64, @intCast(-@as(i64, value))) << 1) | 1
    else
        @as(u64, @intCast(value)) << 1;

    while (true) {
        let digit: u8 = @as(u8, @intCast(vlq & 0x1F));
        vlq = vlq >> 5;
        if (vlq > 0) {
            digit = digit | 0x20;
        }
        bcoAppendByte(out, pos, VLQ_BASE64_ALPHABET[@as(usize, digit)])?;
        if (vlq == 0) break;
    }
}

/// Decodes a single SourceMap V3 Base64-VLQ signed 32-bit integer.
pub fn decodeVlqI32(encoded: []const u8): VlqDecodeResult! {
    if (encoded.len == 0) return err.TruncatedVlq;

    let result: u64 = 0;
    let shift: u6 = 0;
    let i: usize = 0;

    while (i < encoded.len) {
        const sextet = vlqBase64Value(encoded[i]) ?? return err.InvalidVlqDigit;
        i += 1;

        const cont = (sextet & 0x20) != 0;
        const payload: u64 = @as(u64, sextet & 0x1F);

        if (shift > 35) return err.VlqOverflow;
        result = result | (payload << shift);

        if (!cont) {
            const is_neg = (result & 1) != 0;
            const mag = result >> 1;
            if (is_neg) {
                if (mag > 2147483648) return err.VlqOverflow;
                const signed_val: i32 = @as(i32, @intCast(-@as(i64, @intCast(mag))));
                return VlqDecodeResult{ value: signed_val, consumed: i };
            }
            if (mag > 2147483647) return err.VlqOverflow;
            return VlqDecodeResult{ value: @as(i32, @intCast(mag)), consumed: i };
        }
        shift += 5;
    }

    return err.TruncatedVlq;
}

/// Single SourceMap V3 mapping segment (absolute coordinates; encoder computes VLQ deltas).
pub struct SourceMapSegment {
    gen_line: u32 = 0,
    gen_col: i32 = 0,
    src_idx: i32 = 0,
    src_line: i32 = 0,
    src_col: i32 = 0,
    name_idx: i32 = BUNDLE_COORD_NO_NAME,

    static fn empty(): SourceMapSegment {
        return SourceMapSegment{};
    }
}

/// Complete input specification for `encodeSourceMapV3Json`.
pub struct SourceMapV3Input {
    file: []const u8 = "",
    source_root: []const u8 = "",
    sources: []const []const u8 = [],
    sources_content: []const []const u8 = [],
    names: []const []const u8 = [],
    segments: []const SourceMapSegment = [],
}

/// Encodes a standard RFC 8259 SourceMap V3 JSON document into caller-owned `out`.
/// Expects `input.segments` to be ordered by ascending `gen_line`.
pub fn encodeSourceMapV3Json(input: SourceMapV3Input, out: []u8): usize! {
    let pos: usize = 0;
    bcoAppendBytes(out, &pos, "{\"version\":3")?;

    if (input.file.len > 0) {
        bcoAppendBytes(out, &pos, ",\"file\":")?;
        bcoAppendJsonString(out, &pos, input.file)?;
    }
    if (input.source_root.len > 0) {
        bcoAppendBytes(out, &pos, ",\"sourceRoot\":")?;
        bcoAppendJsonString(out, &pos, input.source_root)?;
    }

    // "sources": [...]
    bcoAppendBytes(out, &pos, ",\"sources\":[")?;
    let s: usize = 0;
    while (s < input.sources.len) {
        if (s > 0) bcoAppendByte(out, &pos, ',')?;
        bcoAppendJsonString(out, &pos, input.sources[s])?;
        s += 1;
    }
    bcoAppendByte(out, &pos, ']')?;

    // Optional "sourcesContent": [...]
    if (input.sources_content.len > 0) {
        bcoAppendBytes(out, &pos, ",\"sourcesContent\":[")?;
        let sc: usize = 0;
        while (sc < input.sources_content.len) {
            if (sc > 0) bcoAppendByte(out, &pos, ',')?;
            bcoAppendJsonString(out, &pos, input.sources_content[sc])?;
            sc += 1;
        }
        bcoAppendByte(out, &pos, ']')?;
    }

    // "names": [...]
    bcoAppendBytes(out, &pos, ",\"names\":[")?;
    let nm: usize = 0;
    while (nm < input.names.len) {
        if (nm > 0) bcoAppendByte(out, &pos, ',')?;
        bcoAppendJsonString(out, &pos, input.names[nm])?;
        nm += 1;
    }
    bcoAppendByte(out, &pos, ']')?;

    // "mappings": "..."
    bcoAppendBytes(out, &pos, ",\"mappings\":\"")?;
    let cur_line: u32 = 0;
    let prev_gen_col: i32 = 0;
    let prev_src_idx: i32 = 0;
    let prev_src_line: i32 = 0;
    let prev_src_col: i32 = 0;
    let prev_name_idx: i32 = 0;
    let seg_on_line = false;

    let i: usize = 0;
    while (i < input.segments.len) {
        const seg = input.segments[i];
        while (cur_line < seg.gen_line) {
            bcoAppendByte(out, &pos, ';')?;
            cur_line += 1;
            prev_gen_col = 0;
            seg_on_line = false;
        }
        if (seg_on_line) {
            bcoAppendByte(out, &pos, ',')?;
        }
        encodeVlqI32(seg.gen_col - prev_gen_col, out, &pos)?;
        prev_gen_col = seg.gen_col;

        if (seg.src_idx >= 0) {
            encodeVlqI32(seg.src_idx - prev_src_idx, out, &pos)?;
            prev_src_idx = seg.src_idx;

            encodeVlqI32(seg.src_line - prev_src_line, out, &pos)?;
            prev_src_line = seg.src_line;

            encodeVlqI32(seg.src_col - prev_src_col, out, &pos)?;
            prev_src_col = seg.src_col;

            if (seg.name_idx >= 0) {
                encodeVlqI32(seg.name_idx - prev_name_idx, out, &pos)?;
                prev_name_idx = seg.name_idx;
            }
        }
        seg_on_line = true;
        i += 1;
    }

    bcoAppendBytes(out, &pos, "\"}")?;
    return pos;
}

// ============================================================================
// Section 2: Virtual Module & Plugin Lifecycle Hook Registry
// ============================================================================

/// Matching strategy for `onResolve` and `onLoad` plugin hook filters.
pub enum PluginFilterKind: u8 {
    Prefix = 0,
    Suffix = 1,
    Exact = 2,
    Wildcard = 3,
}

fn matchesPluginFilter(kind: PluginFilterKind, pattern: []const u8, candidate: []const u8): bool {
    if (pattern.len == 0) return true;
    return switch (kind) {
        .Prefix => bcoStartsWith(candidate, pattern),
        .Suffix => bcoEndsWith(candidate, pattern),
        .Exact => bcoBytesEql(candidate, pattern),
        .Wildcard => bcoWildcardMatch(pattern, candidate),
    };
}

/// Declarative `onResolve` hook entry for virtual modules, aliases, or external marking.
pub struct PluginResolveHook {
    plugin_name: []const u8 = "",
    namespace_filter: []const u8 = "",
    filter_kind: PluginFilterKind = PluginFilterKind.Prefix,
    pattern: []const u8 = "",
    priority: i16 = 0,
    target_namespace: []const u8 = "virtual",
    resolved_override: []const u8 = "",
    redirect_specifier: []const u8 = "",
    mark_external: bool = false,

    static fn empty(): PluginResolveHook {
        return PluginResolveHook{};
    }
}

/// Declarative `onLoad` hook entry supplying virtual module source code and loader.
pub struct PluginLoadHook {
    plugin_name: []const u8 = "",
    namespace_filter: []const u8 = "virtual",
    filter_kind: PluginFilterKind = PluginFilterKind.Exact,
    pattern: []const u8 = "",
    priority: i16 = 0,
    contents: []const u8 = "",
    loader_name: []const u8 = "cp",

    static fn empty(): PluginLoadHook {
        return PluginLoadHook{};
    }
}

/// Result returned by `BundlePluginRegistry.runOnResolve`.
pub struct PluginResolveOutcome {
    matched: bool = false,
    plugin_name: []const u8 = "",
    resolved_path: []const u8 = "",
    namespace: []const u8 = "file",
    is_external: bool = false,
    hops: u32 = 0,
}

/// Result returned by `BundlePluginRegistry.runOnLoad`.
pub struct PluginLoadOutcome {
    matched: bool = false,
    plugin_name: []const u8 = "",
    contents: []const u8 = "",
    loader_name: []const u8 = "",
}

/// Zero-allocation priority-ordered `onResolve` / `onLoad` plugin hook registry.
pub struct BundlePluginRegistry {
    resolve_hooks: []PluginResolveHook,
    load_hooks: []PluginLoadHook,
    resolve_count: usize = 0,
    load_count: usize = 0,
    max_resolve_depth: u32 = BUNDLE_COORD_MAX_RESOLVE_DEPTH,

    static fn init(
        resolve_hooks: []PluginResolveHook,
        load_hooks: []PluginLoadHook,
        max_resolve_depth: u32,
    ): BundlePluginRegistry {
        const depth = if (max_resolve_depth == 0) BUNDLE_COORD_MAX_RESOLVE_DEPTH else max_resolve_depth;
        return BundlePluginRegistry{
            resolve_hooks: resolve_hooks,
            load_hooks: load_hooks,
            max_resolve_depth: depth,
        };
    }

    /// Registers an `onResolve` hook, keeping hooks sorted by descending priority.
    fn registerOnResolve(*_, hook: PluginResolveHook)! {
        if (_.resolve_count >= _.resolve_hooks.len) return err.SliceCapacityExceeded;
        let idx = _.resolve_count;
        _.resolve_hooks[idx] = hook;
        _.resolve_count += 1;

        // Stable insertion sort by descending priority
        while (idx > 0 && _.resolve_hooks[idx - 1].priority < _.resolve_hooks[idx].priority) {
            const tmp = _.resolve_hooks[idx - 1];
            _.resolve_hooks[idx - 1] = _.resolve_hooks[idx];
            _.resolve_hooks[idx] = tmp;
            idx -= 1;
        }
    }

    /// Registers an `onLoad` hook, keeping hooks sorted by descending priority.
    fn registerOnLoad(*_, hook: PluginLoadHook)! {
        if (_.load_count >= _.load_hooks.len) return err.SliceCapacityExceeded;
        let idx = _.load_count;
        _.load_hooks[idx] = hook;
        _.load_count += 1;

        while (idx > 0 && _.load_hooks[idx - 1].priority < _.load_hooks[idx].priority) {
            const tmp = _.load_hooks[idx - 1];
            _.load_hooks[idx - 1] = _.load_hooks[idx];
            _.load_hooks[idx] = tmp;
            idx -= 1;
        }
    }

    /// Evaluates registered `onResolve` hooks with redirect chaining and cycle depth guard.
    fn runOnResolve(specifier: []const u8, initial_namespace: []const u8): PluginResolveOutcome! {
        let cur_spec = specifier;
        let cur_ns = initial_namespace;
        let hops: u32 = 0;

        while (hops < _.max_resolve_depth) {
            let matched_hook = false;
            let i: usize = 0;
            while (i < _.resolve_count) {
                const h = _.resolve_hooks[i];
                const ns_ok = (h.namespace_filter.len == 0) || bcoBytesEql(h.namespace_filter, cur_ns);
                if (ns_ok && matchesPluginFilter(h.filter_kind, h.pattern, cur_spec)) {
                    matched_hook = true;
                    hops += 1;
                    if (h.redirect_specifier.len > 0) {
                        cur_spec = h.redirect_specifier;
                        if (h.target_namespace.len > 0) cur_ns = h.target_namespace;
                        break;
                    }
                    const out_path = if (h.resolved_override.len > 0) h.resolved_override else cur_spec;
                    const out_ns = if (h.target_namespace.len > 0) h.target_namespace else cur_ns;
                    return PluginResolveOutcome{
                        matched: true,
                        plugin_name: h.plugin_name,
                        resolved_path: out_path,
                        namespace: out_ns,
                        is_external: h.mark_external,
                        hops: hops,
                    };
                }
                i += 1;
            }
            if (!matched_hook) {
                return PluginResolveOutcome{
                    matched: hops > 0,
                    resolved_path: cur_spec,
                    namespace: cur_ns,
                    hops: hops,
                };
            }
        }

        return err.PluginResolveDepthExceeded;
    }

    /// Evaluates registered `onLoad` hooks in priority order for a resolved module.
    fn runOnLoad(resolved_path: []const u8, namespace: []const u8): PluginLoadOutcome {
        let i: usize = 0;
        while (i < _.load_count) {
            const h = _.load_hooks[i];
            const ns_ok = (h.namespace_filter.len == 0) || bcoBytesEql(h.namespace_filter, namespace);
            if (ns_ok && matchesPluginFilter(h.filter_kind, h.pattern, resolved_path)) {
                return PluginLoadOutcome{
                    matched: true,
                    plugin_name: h.plugin_name,
                    contents: h.contents,
                    loader_name: h.loader_name,
                };
            }
            i += 1;
        }
        return PluginLoadOutcome{};
    }
}

// ============================================================================
// Section 3: 6-Stage End-to-End Bundle Coordinator & Metafile Emitter
// ============================================================================

/// Six ordered pipeline stages coordinated by `BundleCoordinator`.
pub enum BundleCoordStage: u8 {
    Idle = 0,
    ResolveEntries = 1,
    ScanGraph = 2,
    RunPlugins = 3,
    GenerateChunks = 4,
    EmitSourceMaps = 5,
    WriteManifest = 6,
    Completed = 7,
    Failed = 8,

    fn asName(): []const u8 {
        return switch (_) {
            .Idle => "idle",
            .ResolveEntries => "resolve_entries",
            .ScanGraph => "scan_graph",
            .RunPlugins => "run_plugins",
            .GenerateChunks => "generate_chunks",
            .EmitSourceMaps => "emit_sourcemaps",
            .WriteManifest => "write_manifest",
            .Completed => "completed",
            .Failed => "failed",
        };
    }

    fn orderIndex(): u8 {
        return switch (_) {
            .Idle => 0,
            .ResolveEntries => 1,
            .ScanGraph => 2,
            .RunPlugins => 3,
            .GenerateChunks => 4,
            .EmitSourceMaps => 5,
            .WriteManifest => 6,
            .Completed => 7,
            .Failed => 8,
        };
    }
}

/// Severity level for bundler diagnostics.
pub enum BundleDiagSeverity: u8 {
    Info = 0,
    Warning = 1,
    Error = 2,

    fn asStr(): []const u8 {
        return switch (_) {
            .Info => "info",
            .Warning => "warning",
            .Error => "error",
        };
    }
}

/// Structured bundler diagnostic with source location and error code.
pub struct BundleDiagnostic {
    severity: BundleDiagSeverity = BundleDiagSeverity.Error,
    code: []const u8 = "",
    file_path: []const u8 = "",
    line: u32 = 0,
    column: u32 = 0,
    message: []const u8 = "",

    static fn empty(): BundleDiagnostic {
        return BundleDiagnostic{};
    }
}

/// Emitted output chunk descriptor tracked by `BundleCoordinator`.
pub struct BundleEmittedChunk {
    output_path: []const u8 = "",
    entry_point: []const u8 = "",
    byte_size: u64 = 0,
    input_count: u32 = 0,
    has_sourcemap: bool = false,
    content_hash64: u64 = 0,
    budget_exceeded: bool = false,

    static fn empty(): BundleEmittedChunk {
        return BundleEmittedChunk{};
    }
}

/// Configuration options for `BundleCoordinator`.
pub struct BundleCoordOptions {
    max_chunk_bytes: u64 = 0,
    fail_on_budget_exceeded: bool = false,
    emit_sourcemaps: bool = false,
    write_metafile: bool = true,

    static fn defaults(): BundleCoordOptions {
        return BundleCoordOptions{};
    }
}

/// Aggregate build summary produced by `BundleCoordinator.finish`.
pub struct BundleCoordSummary {
    stage: BundleCoordStage = BundleCoordStage.Idle,
    succeeded: bool = false,
    entry_count: u32 = 0,
    scanned_module_count: u32 = 0,
    plugin_hits: u32 = 0,
    chunk_count: u32 = 0,
    sourcemap_count: u32 = 0,
    total_output_bytes: u64 = 0,
    budget_warning_count: u32 = 0,
    warning_count: u32 = 0,
    error_count: u32 = 0,
    total_elapsed_ms: u64 = 0,
}

/// Zero-allocation 6-stage bundler coordinator tracking diagnostics, chunk budgets,
/// per-stage timings, and deterministic JSON Metafile generation.
pub struct BundleCoordinator {
    options: BundleCoordOptions,
    stage: BundleCoordStage = BundleCoordStage.Idle,
    diagnostics: []BundleDiagnostic,
    diag_count: usize = 0,
    chunks: []BundleEmittedChunk,
    chunk_count: usize = 0,
    stage_durations_ms: [6]u64 = [0; 6],
    entry_count: u32 = 0,
    scanned_module_count: u32 = 0,
    plugin_hits: u32 = 0,
    sourcemap_count: u32 = 0,
    budget_warning_count: u32 = 0,
    warning_count: u32 = 0,
    error_count: u32 = 0,

    static fn init(
        options: BundleCoordOptions,
        diagnostics: []BundleDiagnostic,
        chunks: []BundleEmittedChunk,
    ): BundleCoordinator {
        return BundleCoordinator{
            options: options,
            diagnostics: diagnostics,
            chunks: chunks,
        };
    }

    /// Transitions monotonically to the next pipeline stage and records the completed stage's duration.
    fn advanceStage(*_, next_stage: BundleCoordStage, completed_stage_ms: u64)! {
        if (_.stage == BundleCoordStage.Completed || _.stage == BundleCoordStage.Failed) {
            return err.InvalidStageTransition;
        }
        if (next_stage == BundleCoordStage.Failed) {
            _.stage = BundleCoordStage.Failed;
            return;
        }

        const cur_ord = _.stage.orderIndex();
        const next_ord = next_stage.orderIndex();
        if (next_ord <= cur_ord || next_ord > 7) {
            return err.InvalidStageTransition;
        }

        // Record duration of the stage we are leaving (1..6 -> index 0..5)
        if (cur_ord >= 1 && cur_ord <= 6) {
            _.stage_durations_ms[@as(usize, cur_ord - 1)] = completed_stage_ms;
        }

        if (_.error_count > 0 && next_stage == BundleCoordStage.Completed) {
            _.stage = BundleCoordStage.Failed;
            return err.BuildAbortedOnError;
        }

        _.stage = next_stage;
    }

    /// Appends a structured diagnostic and updates warning/error counters.
    fn addDiagnostic(*_, diag: BundleDiagnostic)! {
        if (_.diag_count >= _.diagnostics.len) return err.SliceCapacityExceeded;
        _.diagnostics[_.diag_count] = diag;
        _.diag_count += 1;
        switch (diag.severity) {
            .Warning => { _.warning_count += 1; },
            .Error => { _.error_count += 1; },
            .Info => {},
        }
    }

    fn recordEntriesResolved(*_, count: u32) {
        _.entry_count += count;
    }

    fn recordGraphScanned(*_, module_count: u32) {
        _.scanned_module_count += module_count;
    }

    fn recordPluginHit(*_) {
        _.plugin_hits += 1;
    }

    fn recordSourceMapEmitted(*_) {
        _.sourcemap_count += 1;
    }

    /// Records an emitted chunk and enforces `options.max_chunk_bytes` size budget.
    fn recordChunkEmitted(*_, chunk: BundleEmittedChunk)! {
        if (_.chunk_count >= _.chunks.len) return err.SliceCapacityExceeded;
        const idx = _.chunk_count;
        _.chunks[idx] = chunk;
        const over_budget = (_.options.max_chunk_bytes > 0) && (chunk.byte_size > _.options.max_chunk_bytes);
        _.chunks[idx].budget_exceeded = over_budget;
        _.chunk_count += 1;

        if (over_budget) {
            _.budget_warning_count += 1;
            const sev = if (_.options.fail_on_budget_exceeded)
                BundleDiagSeverity.Error
            else
                BundleDiagSeverity.Warning;
            _.addDiagnostic(BundleDiagnostic{
                severity: sev,
                code: "W_BUDGET_EXCEEDED",
                file_path: chunk.output_path,
                line: 0,
                column: 0,
                message: "emitted chunk exceeds configured max_chunk_bytes budget",
            })?;
        }
    }

    /// Finalizes the build session and returns the aggregate `BundleCoordSummary`.
    fn finish(*_, final_stage_ms: u64): BundleCoordSummary {
        const cur_ord = _.stage.orderIndex();
        if (cur_ord >= 1 && cur_ord <= 6) {
            _.stage_durations_ms[@as(usize, cur_ord - 1)] = final_stage_ms;
        }
        _.stage = if (_.error_count == 0) BundleCoordStage.Completed else BundleCoordStage.Failed;

        let total_bytes: u64 = 0;
        let i: usize = 0;
        while (i < _.chunk_count) {
            total_bytes += _.chunks[i].byte_size;
            i += 1;
        }

        let total_ms: u64 = 0;
        let s: usize = 0;
        while (s < 6) {
            total_ms += _.stage_durations_ms[s];
            s += 1;
        }

        return BundleCoordSummary{
            stage: _.stage,
            succeeded: _.error_count == 0,
            entry_count: _.entry_count,
            scanned_module_count: _.scanned_module_count,
            plugin_hits: _.plugin_hits,
            chunk_count: @as(u32, @intCast(_.chunk_count)),
            sourcemap_count: _.sourcemap_count,
            total_output_bytes: total_bytes,
            budget_warning_count: _.budget_warning_count,
            warning_count: _.warning_count,
            error_count: _.error_count,
            total_elapsed_ms: total_ms,
        };
    }

    /// Emits a deterministic RFC 8259 JSON Metafile summarizing outputs, timings, and diagnostics.
    fn formatMetafileJson(out: []u8): usize! {
        let pos: usize = 0;
        bcoAppendBytes(out, &pos, "{\"stage\":")?;
        bcoAppendJsonString(out, &pos, _.stage.asName())?;
        bcoAppendBytes(out, &pos, ",\"entries\":")?;
        bcoAppendU64(out, &pos, @as(u64, _.entry_count))?;
        bcoAppendBytes(out, &pos, ",\"modules\":")?;
        bcoAppendU64(out, &pos, @as(u64, _.scanned_module_count))?;
        bcoAppendBytes(out, &pos, ",\"pluginHits\":")?;
        bcoAppendU64(out, &pos, @as(u64, _.plugin_hits))?;
        bcoAppendBytes(out, &pos, ",\"sourcemaps\":")?;
        bcoAppendU64(out, &pos, @as(u64, _.sourcemap_count))?;

        bcoAppendBytes(out, &pos, ",\"outputs\":{")?;
        let i: usize = 0;
        while (i < _.chunk_count) {
            const c = _.chunks[i];
            if (i > 0) bcoAppendByte(out, &pos, ',')?;
            bcoAppendJsonString(out, &pos, c.output_path)?;
            bcoAppendBytes(out, &pos, ":{\"bytes\":")?;
            bcoAppendU64(out, &pos, c.byte_size)?;
            bcoAppendBytes(out, &pos, ",\"inputs\":")?;
            bcoAppendU64(out, &pos, @as(u64, c.input_count))?;
            bcoAppendBytes(out, &pos, ",\"hash\":\"")?;
            bcoAppendHex64(out, &pos, c.content_hash64)?;
            bcoAppendBytes(out, &pos, "\",\"budgetExceeded\":")?;
            bcoAppendBytes(out, &pos, if (c.budget_exceeded) "true" else "false")?;
            if (c.entry_point.len > 0) {
                bcoAppendBytes(out, &pos, ",\"entryPoint\":")?;
                bcoAppendJsonString(out, &pos, c.entry_point)?;
            }
            bcoAppendByte(out, &pos, '}')?;
            i += 1;
        }
        bcoAppendByte(out, &pos, '}')?;

        bcoAppendBytes(out, &pos, ",\"diagnostics\":[")?;
        let d: usize = 0;
        while (d < _.diag_count) {
            const dg = _.diagnostics[d];
            if (d > 0) bcoAppendByte(out, &pos, ',')?;
            bcoAppendBytes(out, &pos, "{\"severity\":")?;
            bcoAppendJsonString(out, &pos, dg.severity.asStr())?;
            bcoAppendBytes(out, &pos, ",\"code\":")?;
            bcoAppendJsonString(out, &pos, dg.code)?;
            bcoAppendBytes(out, &pos, ",\"file\":")?;
            bcoAppendJsonString(out, &pos, dg.file_path)?;
            bcoAppendBytes(out, &pos, ",\"line\":")?;
            bcoAppendU64(out, &pos, @as(u64, dg.line))?;
            bcoAppendBytes(out, &pos, ",\"column\":")?;
            bcoAppendU64(out, &pos, @as(u64, dg.column))?;
            bcoAppendBytes(out, &pos, ",\"message\":")?;
            bcoAppendJsonString(out, &pos, dg.message)?;
            bcoAppendByte(out, &pos, '}')?;
            d += 1;
        }
        bcoAppendBytes(out, &pos, "]}")?;
        return pos;
    }
}
