// Source: Absorbed from /data/dev/bun/src/bundler/transpiler.cp | Synced: 2026-11-22T12:43:49Z
// ============================================================================
// lib/bundler/buildPipeline.cp — Zero-Alloc Build Pipeline & Incremental Cache
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven build pipeline engine for `cap build`:
// target & loader extension routing (`resolveBuildLoader`), external specifier
// matching (`cap:*`, exact, wildcard `pkg/*`), `--define` constant table and
// deterministic 64-bit build flags fingerprint, incremental cache invalidation
// with reverse-dependency BFS propagation (`IncrementalBuildCache`), 6-stage
// compile pipeline state machine, and zero-alloc build report formatter.

/// Schema version mixed into build flags fingerprints.
pub const BUILD_PIPELINE_FINGERPRINT_VERSION: u32 = 1;

/// Build pipeline configuration, caching, and state transition error set.
pub err BuildPipelineError {
    UnknownTarget,
    UnknownLoader,
    InvalidExternalPattern,
    InvalidUnitId,
    InvalidStageTransition,
    ScratchTooSmall,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Compilation output target platform/container kind.
pub enum BuildTargetKind: u8 {
    NativeExe = 0,
    NativeLib = 1,
    NestContainer = 2,
    Browser = 3,
    Node = 4,
    Wasm = 5,
}

/// Source asset loader classification.
pub enum BuildLoaderKind: u8 {
    Cp = 0,
    Zig = 1,
    C = 2,
    Cpp = 3,
    Js = 4,
    Ts = 5,
    Tsx = 6,
    Json = 7,
    Toml = 8,
    Css = 9,
    Text = 10,
    File = 11,
    DataUrl = 12,
    Wasm = 13,
}

/// Incremental cache status for an individual compilation unit.
pub enum BuildCacheStatus: u8 {
    FreshHit = 0,
    StaleSource = 1,
    StaleDep = 2,
    StaleFlags = 3,
    Missing = 4,
}

/// Six-stage build pipeline execution state machine.
pub enum BuildPipelineStage: u8 {
    Init = 0,
    Resolving = 1,
    CheckingCache = 2,
    Compiling = 3,
    Linking = 4,
    Emitting = 5,
    Done = 6,
    Failed = 7,
}

/// User-supplied extension-to-loader override rule (e.g. `".svg" -> Text`).
pub struct BuildLoaderOverride {
    extension: []const u8 = "",
    loader: BuildLoaderKind = BuildLoaderKind.File,
}

/// Compile-time `--define` key/value constant replacement entry.
pub struct BuildDefinePair {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// Build flags contributing to the deterministic build fingerprint.
pub struct BuildPipelineFlags {
    target: BuildTargetKind = BuildTargetKind.NativeExe,
    optimize_release: bool = false,
    minify: bool = false,
    sourcemap: bool = false,
    splitting: bool = false,
    tree_shaking: bool = true,
    target_triple: []const u8 = "x86_64-linux-gnu",

    static fn defaults(): BuildPipelineFlags {
        return BuildPipelineFlags{};
    }
}

/// Per-unit incremental build cache record stored in caller-owned slice.
pub struct BuildCacheUnit {
    unit_id: u32 = 0,
    path: []const u8 = "",
    loader: BuildLoaderKind = BuildLoaderKind.Cp,
    source_hash64: u64 = 0,
    flags_hash64: u64 = 0,
    deps_hash64: u64 = 0,
    artifact_hash64: u64 = 0,
    artifact_bytes: usize = 0,
    has_cached_artifact: bool = false,
    status: BuildCacheStatus = BuildCacheStatus.Missing,

    static fn empty(): BuildCacheUnit {
        return BuildCacheUnit{};
    }
}

/// Directed compile dependency edge (`from_unit` depends on `to_unit`).
pub struct BuildDepEdge {
    from_unit: u32 = 0,
    to_unit: u32 = 0,
}

/// Summary of cache evaluation and reverse-dependency invalidation.
pub struct InvalidationSummary {
    total_units: u32 = 0,
    fresh_hits: u32 = 0,
    stale_source: u32 = 0,
    stale_dep: u32 = 0,
    stale_flags: u32 = 0,
    missing_units: u32 = 0,
    units_to_rebuild: u32 = 0,
    hit_rate_permille: u32 = 0,
}

/// Aggregate build pipeline execution metrics.
pub struct BuildPipelineMetrics {
    total_units: u32 = 0,
    rebuilt_units: u32 = 0,
    cached_hits: u32 = 0,
    skipped_external: u32 = 0,
    total_output_bytes: usize = 0,
    elapsed_ms: u64 = 0,
    flags_hash64: u64 = 0,
}

// ============================================================================
// Internal Byte, Hash & Formatting Helpers (Zero-Alloc)
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn bytesEqualIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[off + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn indexOfByte(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn fnv1a64Update(hash: u64, data: []const u8): u64 {
    let h = hash;
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ @as(u64, data[i])) *% 1099511628211;
        i += 1;
    }
    return h;
}

fn fnv1a64Byte(hash: u64, b: u8): u64 {
    return (hash ^ @as(u64, b)) *% 1099511628211;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) return BuildPipelineError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendU64Decimal(out_buf: []u8, pos: *usize, value: u64)! {
    let digits: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = value;
    if (v == 0) {
        digits[0] = @as(u8, 48);
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @intCast(48 + (v % 10));
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out_buf.len) return BuildPipelineError.BufferTooSmall;
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
}

// ============================================================================
// Target & Loader Routing
// ============================================================================

/// Parses a CLI `--target` string into `BuildTargetKind`.
pub fn parseBuildTargetKind(name: []const u8): BuildTargetKind? {
    if (bytesEqualIgnoreCase(name, "native") || bytesEqualIgnoreCase(name, "exe") || bytesEqualIgnoreCase(name, "native-exe")) {
        return BuildTargetKind.NativeExe;
    }
    if (bytesEqualIgnoreCase(name, "lib") || bytesEqualIgnoreCase(name, "native-lib")) {
        return BuildTargetKind.NativeLib;
    }
    if (bytesEqualIgnoreCase(name, "nest") || bytesEqualIgnoreCase(name, "nest-container")) {
        return BuildTargetKind.NestContainer;
    }
    if (bytesEqualIgnoreCase(name, "browser")) return BuildTargetKind.Browser;
    if (bytesEqualIgnoreCase(name, "node")) return BuildTargetKind.Node;
    if (bytesEqualIgnoreCase(name, "wasm")) return BuildTargetKind.Wasm;
    return null;
}

/// Returns the canonical target name.
pub fn buildTargetName(t: BuildTargetKind): []const u8 {
    if (t == BuildTargetKind.NativeExe) return "native-exe";
    if (t == BuildTargetKind.NativeLib) return "native-lib";
    if (t == BuildTargetKind.NestContainer) return "nest";
    if (t == BuildTargetKind.Browser) return "browser";
    if (t == BuildTargetKind.Node) return "node";
    return "wasm";
}

/// Parses a loader name string (e.g. `"cp"`, `"tsx"`, `"dataurl"`) into `BuildLoaderKind`.
pub fn parseBuildLoaderKind(name: []const u8): BuildLoaderKind? {
    if (bytesEqualIgnoreCase(name, "cp")) return BuildLoaderKind.Cp;
    if (bytesEqualIgnoreCase(name, "zig")) return BuildLoaderKind.Zig;
    if (bytesEqualIgnoreCase(name, "c")) return BuildLoaderKind.C;
    if (bytesEqualIgnoreCase(name, "cpp") || bytesEqualIgnoreCase(name, "cxx")) return BuildLoaderKind.Cpp;
    if (bytesEqualIgnoreCase(name, "js")) return BuildLoaderKind.Js;
    if (bytesEqualIgnoreCase(name, "ts")) return BuildLoaderKind.Ts;
    if (bytesEqualIgnoreCase(name, "tsx") || bytesEqualIgnoreCase(name, "jsx")) return BuildLoaderKind.Tsx;
    if (bytesEqualIgnoreCase(name, "json")) return BuildLoaderKind.Json;
    if (bytesEqualIgnoreCase(name, "toml")) return BuildLoaderKind.Toml;
    if (bytesEqualIgnoreCase(name, "css")) return BuildLoaderKind.Css;
    if (bytesEqualIgnoreCase(name, "text")) return BuildLoaderKind.Text;
    if (bytesEqualIgnoreCase(name, "file")) return BuildLoaderKind.File;
    if (bytesEqualIgnoreCase(name, "dataurl")) return BuildLoaderKind.DataUrl;
    if (bytesEqualIgnoreCase(name, "wasm")) return BuildLoaderKind.Wasm;
    return null;
}

/// Returns the canonical loader name.
pub fn buildLoaderName(l: BuildLoaderKind): []const u8 {
    if (l == BuildLoaderKind.Cp) return "cp";
    if (l == BuildLoaderKind.Zig) return "zig";
    if (l == BuildLoaderKind.C) return "c";
    if (l == BuildLoaderKind.Cpp) return "cpp";
    if (l == BuildLoaderKind.Js) return "js";
    if (l == BuildLoaderKind.Ts) return "ts";
    if (l == BuildLoaderKind.Tsx) return "tsx";
    if (l == BuildLoaderKind.Json) return "json";
    if (l == BuildLoaderKind.Toml) return "toml";
    if (l == BuildLoaderKind.Css) return "css";
    if (l == BuildLoaderKind.Text) return "text";
    if (l == BuildLoaderKind.File) return "file";
    if (l == BuildLoaderKind.DataUrl) return "dataurl";
    return "wasm";
}

fn extractFileExtension(path: []const u8): []const u8 {
    let i = path.len;
    while (i > 0) {
        i -= 1;
        const c = path[i];
        if (c == '/' || c == '\\') return "";
        if (c == '.') {
            if (i == 0 || path[i - 1] == '/' || path[i - 1] == '\\') return "";
            return path[i..];
        }
    }
    return "";
}

/// Resolves the `BuildLoaderKind` for `path`, checking user `overrides` first
/// (case-insensitive extension match) before falling back to built-in rules.
pub fn resolveBuildLoader(path: []const u8, overrides: []const BuildLoaderOverride): BuildLoaderKind {
    const ext = extractFileExtension(path);
    if (ext.len == 0) return BuildLoaderKind.File;

    let i: usize = 0;
    while (i < overrides.len) {
        if (bytesEqualIgnoreCase(ext, overrides[i].extension)) {
            return overrides[i].loader;
        }
        i += 1;
    }

    if (bytesEqualIgnoreCase(ext, ".cp")) return BuildLoaderKind.Cp;
    if (bytesEqualIgnoreCase(ext, ".zig")) return BuildLoaderKind.Zig;
    if (bytesEqualIgnoreCase(ext, ".c") || bytesEqualIgnoreCase(ext, ".h")) return BuildLoaderKind.C;
    if (bytesEqualIgnoreCase(ext, ".cpp") || bytesEqualIgnoreCase(ext, ".cc") || bytesEqualIgnoreCase(ext, ".cxx") || bytesEqualIgnoreCase(ext, ".hpp")) {
        return BuildLoaderKind.Cpp;
    }
    if (bytesEqualIgnoreCase(ext, ".js") || bytesEqualIgnoreCase(ext, ".mjs") || bytesEqualIgnoreCase(ext, ".cjs")) {
        return BuildLoaderKind.Js;
    }
    if (bytesEqualIgnoreCase(ext, ".ts") || bytesEqualIgnoreCase(ext, ".mts") || bytesEqualIgnoreCase(ext, ".cts")) {
        return BuildLoaderKind.Ts;
    }
    if (bytesEqualIgnoreCase(ext, ".tsx") || bytesEqualIgnoreCase(ext, ".jsx")) return BuildLoaderKind.Tsx;
    if (bytesEqualIgnoreCase(ext, ".json") || bytesEqualIgnoreCase(ext, ".jsonc")) return BuildLoaderKind.Json;
    if (bytesEqualIgnoreCase(ext, ".toml")) return BuildLoaderKind.Toml;
    if (bytesEqualIgnoreCase(ext, ".css")) return BuildLoaderKind.Css;
    if (bytesEqualIgnoreCase(ext, ".txt") || bytesEqualIgnoreCase(ext, ".md")) return BuildLoaderKind.Text;
    if (bytesEqualIgnoreCase(ext, ".wasm")) return BuildLoaderKind.Wasm;
    return BuildLoaderKind.File;
}

// ============================================================================
// External Specifier Matching & `--define` Fingerprint Engine
// ============================================================================

/// Returns true if `spec` is a built-in `cap:*` standard library specifier.
pub fn isCapBuiltinSpecifier(spec: []const u8): bool {
    return startsWith(spec, "cap:") && spec.len > 4;
}

/// Matches an external pattern against an import `specifier`.
/// Supports `"*"`, `"prefix/*"`, `"prefix*suffix"`, exact match `"react"`,
/// and package subpath inheritance (`"react"` matches `"react/jsx-runtime"`).
pub fn matchExternalPattern(pattern: []const u8, specifier: []const u8): bool {
    if (pattern.len == 0) return false;
    if (bytesEqual(pattern, "*")) return true;

    if (star := indexOfByte(pattern, '*')) {
        const prefix = pattern[0..star];
        const suffix = pattern[star + 1 ..];
        if (specifier.len < prefix.len + suffix.len) return false;
        return startsWith(specifier, prefix) && endsWith(specifier, suffix);
    }

    if (bytesEqual(pattern, specifier)) return true;
    // Exact package name also externalizes subpaths (`react` -> `react/jsx-runtime`)
    if (specifier.len > pattern.len && startsWith(specifier, pattern) && specifier[pattern.len] == '/') {
        return true;
    }
    return false;
}

/// Returns true if `specifier` should be treated as external and excluded from bundling.
pub fn isExternalSpecifier(
    specifier: []const u8,
    externals: []const []const u8,
    treat_cap_builtin_as_external: bool,
): bool {
    if (treat_cap_builtin_as_external && isCapBuiltinSpecifier(specifier)) {
        return true;
    }
    let i: usize = 0;
    while (i < externals.len) {
        if (matchExternalPattern(externals[i], specifier)) return true;
        i += 1;
    }
    return false;
}

/// Looks up a compile-time `--define` replacement value by key (later entries override earlier).
pub fn lookupBuildDefine(defines: []const BuildDefinePair, key: []const u8): ([]const u8)? {
    let i = defines.len;
    while (i > 0) {
        i -= 1;
        if (bytesEqual(defines[i].key, key)) return defines[i].value;
    }
    return null;
}

/// Computes a deterministic 64-bit build flags fingerprint covering target, optimization
/// switches, target triple, `--define` pairs, and loader overrides.
pub fn computeBuildFlagsFingerprint(
    flags: BuildPipelineFlags,
    defines: []const BuildDefinePair,
    overrides: []const BuildLoaderOverride,
): u64 {
    let h: u64 = 14695981039346656037;
    h = fnv1a64Byte(h, @intCast(BUILD_PIPELINE_FINGERPRINT_VERSION & 0xFF));
    h = fnv1a64Byte(h, @intFromEnum(flags.target));
    h = fnv1a64Byte(h, if (flags.optimize_release) 1 else 0);
    h = fnv1a64Byte(h, if (flags.minify) 1 else 0);
    h = fnv1a64Byte(h, if (flags.sourcemap) 1 else 0);
    h = fnv1a64Byte(h, if (flags.splitting) 1 else 0);
    h = fnv1a64Byte(h, if (flags.tree_shaking) 1 else 0);
    h = fnv1a64Update(h, flags.target_triple);
    h = fnv1a64Byte(h, 0xFF);

    let i: usize = 0;
    while (i < defines.len) {
        h = fnv1a64Update(h, defines[i].key);
        h = fnv1a64Byte(h, @as(u8, 61)); // '='
        h = fnv1a64Update(h, defines[i].value);
        h = fnv1a64Byte(h, 0);
        i += 1;
    }

    i = 0;
    while (i < overrides.len) {
        h = fnv1a64Update(h, overrides[i].extension);
        h = fnv1a64Byte(h, @intFromEnum(overrides[i].loader));
        i += 1;
    }
    return h;
}

// ============================================================================
// Incremental Build Cache & Reverse-Dependency Invalidation Engine
// ============================================================================

/// Zero-heap incremental compilation cache backed by a caller-owned `[]BuildCacheUnit` slice.
pub struct IncrementalBuildCache {
    units: []BuildCacheUnit,
    units_len: usize = 0,

    static fn init(units_backing: []BuildCacheUnit): IncrementalBuildCache {
        return IncrementalBuildCache{
            units: units_backing,
            units_len: 0,
        };
    }

    fn count(): usize {
        return _.units_len;
    }

    /// Registers a compilation unit and its previous cached hashes (if any).
    fn registerUnit(
        *_,
        path: []const u8,
        loader: BuildLoaderKind,
        cached_source_hash: u64,
        cached_flags_hash: u64,
        cached_deps_hash: u64,
        cached_artifact_hash: u64,
        cached_artifact_bytes: usize,
        has_cached_artifact: bool,
    ): u32! {
        if (_.units_len >= _.units.len) return BuildPipelineError.SliceCapacityExceeded;
        const id: u32 = @intCast(_.units_len);
        _.units[_.units_len] = BuildCacheUnit{
            unit_id: id,
            path: path,
            loader: loader,
            source_hash64: cached_source_hash,
            flags_hash64: cached_flags_hash,
            deps_hash64: cached_deps_hash,
            artifact_hash64: cached_artifact_hash,
            artifact_bytes: cached_artifact_bytes,
            has_cached_artifact: has_cached_artifact,
            status: if (has_cached_artifact) BuildCacheStatus.FreshHit else BuildCacheStatus.Missing,
        };
        _.units_len += 1;
        return id;
    }

    /// Evaluates cache freshness against `current_source_hashes` & `current_flags_hash`,
    /// then propagates staleness along reverse dependency edges (`from` depends on `to`)
    /// using an external queue BFS so only the minimal affected closure is rebuilt.
    fn evaluateAndPropagate(
        *_,
        current_source_hashes: []const u64,
        current_flags_hash: u64,
        dep_edges: []const BuildDepEdge,
        queue_scratch: []u32,
    ): InvalidationSummary! {
        if (current_source_hashes.len < _.units_len || queue_scratch.len < _.units_len) {
            return BuildPipelineError.ScratchTooSmall;
        }
        // Validate edges
        let ei: usize = 0;
        while (ei < dep_edges.len) {
            if (@as(usize, dep_edges[ei].from_unit) >= _.units_len || @as(usize, dep_edges[ei].to_unit) >= _.units_len) {
                return BuildPipelineError.InvalidUnitId;
            }
            ei += 1;
        }

        let q_head: usize = 0;
        let q_tail: usize = 0;

        // Pass 1: direct local staleness check
        let i: usize = 0;
        while (i < _.units_len) {
            let u = _.units[i];
            if (!u.has_cached_artifact) {
                u.status = BuildCacheStatus.Missing;
            } else if (u.flags_hash64 != current_flags_hash) {
                u.status = BuildCacheStatus.StaleFlags;
            } else if (u.source_hash64 != current_source_hashes[i]) {
                u.status = BuildCacheStatus.StaleSource;
            } else {
                u.status = BuildCacheStatus.FreshHit;
            }
            _.units[i] = u;

            if (u.status != BuildCacheStatus.FreshHit) {
                queue_scratch[q_tail] = @intCast(i);
                q_tail += 1;
            }
            i += 1;
        }

        // Pass 2: reverse-dependency BFS propagation (if `to` is stale, `from` becomes StaleDep)
        while (q_head < q_tail) {
            const changed_id = queue_scratch[q_head];
            q_head += 1;

            ei = 0;
            while (ei < dep_edges.len) {
                const e = dep_edges[ei];
                if (e.to_unit == changed_id) {
                    const dep_idx: usize = @intCast(e.from_unit);
                    if (_.units[dep_idx].status == BuildCacheStatus.FreshHit) {
                        let du = _.units[dep_idx];
                        du.status = BuildCacheStatus.StaleDep;
                        _.units[dep_idx] = du;
                        queue_scratch[q_tail] = e.from_unit;
                        q_tail += 1;
                    }
                }
                ei += 1;
            }
        }

        // Pass 3: tally invalidation summary
        let sum = InvalidationSummary{ total_units: @intCast(_.units_len) };
        i = 0;
        while (i < _.units_len) {
            const st = _.units[i].status;
            if (st == BuildCacheStatus.FreshHit) {
                sum.fresh_hits += 1;
            } else {
                sum.units_to_rebuild += 1;
                if (st == BuildCacheStatus.StaleSource) sum.stale_source += 1;
                else if (st == BuildCacheStatus.StaleDep) sum.stale_dep += 1;
                else if (st == BuildCacheStatus.StaleFlags) sum.stale_flags += 1;
                else sum.missing_units += 1;
            }
            i += 1;
        }
        if (sum.total_units > 0) {
            sum.hit_rate_permille = (sum.fresh_hits * 1000) / sum.total_units;
        }
        return sum;
    }

    /// Commits a freshly compiled unit's updated hashes and artifact metadata into cache.
    fn commitCompiledUnit(
        *_,
        unit_id: u32,
        new_source_hash: u64,
        new_flags_hash: u64,
        new_deps_hash: u64,
        new_artifact_hash: u64,
        new_artifact_bytes: usize,
    )! {
        const idx: usize = @intCast(unit_id);
        if (idx >= _.units_len) return BuildPipelineError.InvalidUnitId;
        let u = _.units[idx];
        u.source_hash64 = new_source_hash;
        u.flags_hash64 = new_flags_hash;
        u.deps_hash64 = new_deps_hash;
        u.artifact_hash64 = new_artifact_hash;
        u.artifact_bytes = new_artifact_bytes;
        u.has_cached_artifact = true;
        u.status = BuildCacheStatus.FreshHit;
        _.units[idx] = u;
    }
}

// ============================================================================
// Multi-Stage Build Pipeline State Machine & Summary Reporter
// ============================================================================

/// Six-stage compilation pipeline session state machine.
pub struct BuildPipelineSession {
    stage: BuildPipelineStage = BuildPipelineStage.Init,
    target: BuildTargetKind = BuildTargetKind.NativeExe,
    metrics: BuildPipelineMetrics = BuildPipelineMetrics{},
    error_code: u16 = 0,

    static fn init(target: BuildTargetKind, flags_hash64: u64): BuildPipelineSession {
        return BuildPipelineSession{
            stage: BuildPipelineStage.Init,
            target: target,
            metrics: BuildPipelineMetrics{ flags_hash64: flags_hash64 },
        };
    }

    fn beginResolve(*_)! {
        if (_.stage != BuildPipelineStage.Init) return BuildPipelineError.InvalidStageTransition;
        _.stage = BuildPipelineStage.Resolving;
    }

    fn beginCacheCheck(*_, skipped_external: u32)! {
        if (_.stage != BuildPipelineStage.Resolving) return BuildPipelineError.InvalidStageTransition;
        _.metrics.skipped_external = skipped_external;
        _.stage = BuildPipelineStage.CheckingCache;
    }

    fn beginCompile(*_, inv: InvalidationSummary)! {
        if (_.stage != BuildPipelineStage.CheckingCache) return BuildPipelineError.InvalidStageTransition;
        _.metrics.total_units = inv.total_units;
        _.metrics.rebuilt_units = inv.units_to_rebuild;
        _.metrics.cached_hits = inv.fresh_hits;
        _.stage = BuildPipelineStage.Compiling;
    }

    fn beginLink(*_)! {
        if (_.stage != BuildPipelineStage.Compiling) return BuildPipelineError.InvalidStageTransition;
        _.stage = BuildPipelineStage.Linking;
    }

    fn beginEmit(*_, total_output_bytes: usize)! {
        if (_.stage != BuildPipelineStage.Linking) return BuildPipelineError.InvalidStageTransition;
        _.metrics.total_output_bytes = total_output_bytes;
        _.stage = BuildPipelineStage.Emitting;
    }

    fn markDone(*_, elapsed_ms: u64)! {
        if (_.stage != BuildPipelineStage.Emitting) return BuildPipelineError.InvalidStageTransition;
        _.metrics.elapsed_ms = elapsed_ms;
        _.stage = BuildPipelineStage.Done;
    }

    fn markFailed(*_, err_code: u16) {
        _.error_code = err_code;
        _.stage = BuildPipelineStage.Failed;
    }

    fn isDone(): bool {
        return _.stage == BuildPipelineStage.Done;
    }
}

/// Formats a concise human-readable build pipeline summary line into `out_buf`.
pub fn formatBuildPipelineReport(sess: BuildPipelineSession, out_buf: []u8): usize! {
    let pos: usize = 0;
    const hit_pct: u64 = if (sess.metrics.total_units > 0)
        (@as(u64, sess.metrics.cached_hits) * 100) / @as(u64, sess.metrics.total_units)
    else
        0;

    appendBytes(out_buf, &pos, "build target=")?;
    appendBytes(out_buf, &pos, buildTargetName(sess.target))?;
    appendBytes(out_buf, &pos, " units=")?;
    appendU64Decimal(out_buf, &pos, @as(u64, sess.metrics.total_units))?;
    appendBytes(out_buf, &pos, " rebuilt=")?;
    appendU64Decimal(out_buf, &pos, @as(u64, sess.metrics.rebuilt_units))?;
    appendBytes(out_buf, &pos, " cached=")?;
    appendU64Decimal(out_buf, &pos, @as(u64, sess.metrics.cached_hits))?;
    appendBytes(out_buf, &pos, " hit=")?;
    appendU64Decimal(out_buf, &pos, hit_pct)?;
    appendBytes(out_buf, &pos, "% out=")?;
    appendU64Decimal(out_buf, &pos, @as(u64, sess.metrics.total_output_bytes))?;
    appendBytes(out_buf, &pos, "B time=")?;
    appendU64Decimal(out_buf, &pos, sess.metrics.elapsed_ms)?;
    appendBytes(out_buf, &pos, "ms")?;
    return pos;
}
