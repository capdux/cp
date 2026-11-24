// Source: Absorbed from /data/dev/bun/src/cli/build_command.cp | Synced: 2026-11-23T20:18:24Z
// ============================================================================
// lib/bundler/buildCommand.cp — Zero-Alloc `cap build` CLI, Matrix & Table Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven build command frontend for `cap build`:
//   1. Complete CLI configuration model & parser (`BuildCommandConfig`,
//      `parseBuildCommandArgs`) supporting multi-entry slices, `--outdir` /
//      `--outfile`, `--target`, `--format`, granular `--minify*`, `--sourcemap`,
//      `--splitting`, `--compile`, `--bytecode`, `--nest` (`-n`), `--external`,
//      `--define`, `--loader`, `--public-path`, naming templates & `--watch`;
//   2. Semantic conflict & security validator (`validateBuildConfig`) catching
//      mutually exclusive flags, invalid placeholder templates, reserved
//      standalone binary names, and path traversal attempts;
//   3. Cross-compile target matrix parser & artifact path planner
//      (`parseCrossCompileTarget`, `formatCrossArtifactPath`) supporting
//      `cap-linux-x86_64`, `cap-macos-aarch64`, `cap-windows-x86_64`, `wasm`,
//      and `nest` with automatic `.exe` / `.nest` / `.wasm` suffixing;
//   4. Column-aligned terminal build artifact table renderer
//      (`formatBuildArtifactTable`, `formatHumanByteSize`) with type badges,
//      human-readable sizes (`B`/`KB`/`MB`/`GB`), and timing/cache summary.

/// Default entry-point naming template.
pub const BUILD_CMD_DEFAULT_ENTRY_NAMING: []const u8 = "[dir]/[name].[ext]";

/// Default code-split chunk naming template.
pub const BUILD_CMD_DEFAULT_CHUNK_NAMING: []const u8 = "./[name]-[hash].[ext]";

/// Default static asset naming template.
pub const BUILD_CMD_DEFAULT_ASSET_NAMING: []const u8 = "./[name]-[hash].[ext]";

/// Build command parsing, validation, target resolution, and formatting error set.
pub err BuildCommandError {
    MissingEntryPoint,
    MissingFlagValue,
    UnknownFlag,
    InvalidTarget,
    InvalidFormat,
    InvalidSourceMapMode,
    InvalidDefineSyntax,
    InvalidLoaderSyntax,
    InvalidNamingTemplate,
    PathTraversalDetected,
    ConflictingOptions,
    ReservedOutputName,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, Path & Formatting Helpers
// ============================================================================

fn bcBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn bcAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn bcBytesEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (bcAsciiLower(a[i]) != bcAsciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn bcStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return bcBytesEql(s[0..prefix.len], prefix);
}

fn bcEndsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    return bcBytesEql(s[s.len - suffix.len..s.len], suffix);
}

fn bcIndexOf(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn bcBasename(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    let i = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\') {
            return path[i + 1..end];
        }
    }
    return path[0..end];
}

fn bcStripTrailingSlash(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    return path[0..end];
}

fn bcAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn bcAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn bcAppendSpaces(out: []u8, pos: *usize, count: usize)! {
    let i: usize = 0;
    while (i < count) {
        bcAppendByte(out, pos, ' ')?;
        i += 1;
    }
}

fn bcAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        bcAppendByte(out, pos, '0')?;
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
        bcAppendByte(out, pos, tmp[n])?;
    }
}

/// Formats byte counts into human-readable units (`512 B`, `14.20 KB`, `2.35 MB`, `1.10 GB`).
pub fn formatHumanByteSize(bytes: u64, out: []u8, pos: *usize)! {
    if (bytes < 1024) {
        bcAppendU64(out, pos, bytes)?;
        bcAppendBytes(out, pos, " B")?;
        return;
    }
    const kb_unit: u64 = 1024;
    const mb_unit: u64 = 1024 * 1024;
    const gb_unit: u64 = 1024 * 1024 * 1024;

    if (bytes < mb_unit) {
        const whole = bytes / kb_unit;
        const frac = ((bytes % kb_unit) * 100) / kb_unit;
        bcAppendU64(out, pos, whole)?;
        bcAppendByte(out, pos, '.')?;
        if (frac < 10) bcAppendByte(out, pos, '0')?;
        bcAppendU64(out, pos, frac)?;
        bcAppendBytes(out, pos, " KB")?;
        return;
    }
    if (bytes < gb_unit) {
        const whole = bytes / mb_unit;
        const frac = ((bytes % mb_unit) * 100) / mb_unit;
        bcAppendU64(out, pos, whole)?;
        bcAppendByte(out, pos, '.')?;
        if (frac < 10) bcAppendByte(out, pos, '0')?;
        bcAppendU64(out, pos, frac)?;
        bcAppendBytes(out, pos, " MB")?;
        return;
    }
    const whole = bytes / gb_unit;
    const frac = ((bytes % gb_unit) * 100) / gb_unit;
    bcAppendU64(out, pos, whole)?;
    bcAppendByte(out, pos, '.')?;
    if (frac < 10) bcAppendByte(out, pos, '0')?;
    bcAppendU64(out, pos, frac)?;
    bcAppendBytes(out, pos, " GB")?;
}

fn bcFormatDurationMs(ms: u64, out: []u8, pos: *usize)! {
    if (ms < 1000) {
        bcAppendU64(out, pos, ms)?;
        bcAppendBytes(out, pos, "ms")?;
        return;
    }
    const secs = ms / 1000;
    const centis = (ms % 1000) / 10;
    bcAppendU64(out, pos, secs)?;
    bcAppendByte(out, pos, '.')?;
    if (centis < 10) bcAppendByte(out, pos, '0')?;
    bcAppendU64(out, pos, centis)?;
    bcAppendByte(out, pos, 's')?;
}

// ============================================================================
// Section 1: Build Command Configuration Model & CLI Parser
// ============================================================================

/// High-level `--target` environment / platform selection for `cap build`.
pub enum BuildCmdTarget: u8 {
    Native = 0,
    Nest = 1,
    LinuxX64 = 2,
    LinuxArm64 = 3,
    MacosArm64 = 4,
    MacosX64 = 5,
    WindowsX64 = 6,
    Wasm = 7,
    Browser = 8,
    Node = 9,

    fn asStr(): []const u8 {
        return switch (_) {
            .Native => "native",
            .Nest => "nest",
            .LinuxX64 => "linux-x64",
            .LinuxArm64 => "linux-arm64",
            .MacosArm64 => "macos-arm64",
            .MacosX64 => "macos-x64",
            .WindowsX64 => "windows-x64",
            .Wasm => "wasm",
            .Browser => "browser",
            .Node => "node",
        };
    }

    fn isWindows(): bool {
        return _ == BuildCmdTarget.WindowsX64;
    }

    fn isNest(): bool {
        return _ == BuildCmdTarget.Nest;
    }

    static fn fromStr(raw: []const u8): BuildCmdTarget? {
        if (bcBytesEqlIgnoreCase(raw, "native")) return BuildCmdTarget.Native;
        if (bcBytesEqlIgnoreCase(raw, "nest")) return BuildCmdTarget.Nest;
        if (bcBytesEqlIgnoreCase(raw, "linux-x64") || bcBytesEqlIgnoreCase(raw, "linux-x86_64") || bcBytesEqlIgnoreCase(raw, "cap-linux-x86_64")) return BuildCmdTarget.LinuxX64;
        if (bcBytesEqlIgnoreCase(raw, "linux-arm64") || bcBytesEqlIgnoreCase(raw, "linux-aarch64") || bcBytesEqlIgnoreCase(raw, "cap-linux-aarch64")) return BuildCmdTarget.LinuxArm64;
        if (bcBytesEqlIgnoreCase(raw, "macos-arm64") || bcBytesEqlIgnoreCase(raw, "macos-aarch64") || bcBytesEqlIgnoreCase(raw, "cap-macos-aarch64") || bcBytesEqlIgnoreCase(raw, "darwin-arm64")) return BuildCmdTarget.MacosArm64;
        if (bcBytesEqlIgnoreCase(raw, "macos-x64") || bcBytesEqlIgnoreCase(raw, "macos-x86_64") || bcBytesEqlIgnoreCase(raw, "cap-macos-x86_64") || bcBytesEqlIgnoreCase(raw, "darwin-x64")) return BuildCmdTarget.MacosX64;
        if (bcBytesEqlIgnoreCase(raw, "windows-x64") || bcBytesEqlIgnoreCase(raw, "windows-x86_64") || bcBytesEqlIgnoreCase(raw, "cap-windows-x86_64") || bcBytesEqlIgnoreCase(raw, "win-x64")) return BuildCmdTarget.WindowsX64;
        if (bcBytesEqlIgnoreCase(raw, "wasm") || bcBytesEqlIgnoreCase(raw, "wasm32")) return BuildCmdTarget.Wasm;
        if (bcBytesEqlIgnoreCase(raw, "browser")) return BuildCmdTarget.Browser;
        if (bcBytesEqlIgnoreCase(raw, "node")) return BuildCmdTarget.Node;
        return null;
    }
}

/// Output module / binary format selected via `--format`.
pub enum BuildCmdFormat: u8 {
    Exe = 0,
    Lib = 1,
    Nest = 2,
    Esm = 3,
    Cjs = 4,

    fn asStr(): []const u8 {
        return switch (_) {
            .Exe => "exe",
            .Lib => "lib",
            .Nest => "nest",
            .Esm => "esm",
            .Cjs => "cjs",
        };
    }

    static fn fromStr(raw: []const u8): BuildCmdFormat? {
        if (bcBytesEqlIgnoreCase(raw, "exe")) return BuildCmdFormat.Exe;
        if (bcBytesEqlIgnoreCase(raw, "lib")) return BuildCmdFormat.Lib;
        if (bcBytesEqlIgnoreCase(raw, "nest")) return BuildCmdFormat.Nest;
        if (bcBytesEqlIgnoreCase(raw, "esm")) return BuildCmdFormat.Esm;
        if (bcBytesEqlIgnoreCase(raw, "cjs")) return BuildCmdFormat.Cjs;
        return null;
    }
}

/// Source map emission mode selected via `--sourcemap`.
pub enum BuildCmdSourceMap: u8 {
    None = 0,
    Inline = 1,
    Linked = 2,
    External = 3,

    fn asStr(): []const u8 {
        return switch (_) {
            .None => "none",
            .Inline => "inline",
            .Linked => "linked",
            .External => "external",
        };
    }

    static fn fromStr(raw: []const u8): BuildCmdSourceMap? {
        if (bcBytesEqlIgnoreCase(raw, "none") || bcBytesEqlIgnoreCase(raw, "false")) return BuildCmdSourceMap.None;
        if (bcBytesEqlIgnoreCase(raw, "inline")) return BuildCmdSourceMap.Inline;
        if (bcBytesEqlIgnoreCase(raw, "linked") || bcBytesEqlIgnoreCase(raw, "true")) return BuildCmdSourceMap.Linked;
        if (bcBytesEqlIgnoreCase(raw, "external")) return BuildCmdSourceMap.External;
        return null;
    }
}

/// Key/value pair used for `--define KEY=VALUE` and `--loader .ext:loader`.
pub struct BuildCmdKeyValue {
    key: []const u8 = "",
    value: []const u8 = "",

    static fn empty(): BuildCmdKeyValue {
        return BuildCmdKeyValue{};
    }
}

/// Complete parsed configuration for a `cap build` invocation.
pub struct BuildCommandConfig {
    entry_points: []const []const u8 = [],
    outdir: []const u8 = "",
    outfile: []const u8 = "",
    target: BuildCmdTarget = BuildCmdTarget.Native,
    format: BuildCmdFormat = BuildCmdFormat.Exe,
    sourcemap: BuildCmdSourceMap = BuildCmdSourceMap.None,
    minify_whitespace: bool = false,
    minify_syntax: bool = false,
    minify_identifiers: bool = false,
    splitting: bool = false,
    compile: bool = false,
    bytecode: bool = false,
    nest_mode: bool = false,
    watch: bool = false,
    production: bool = false,
    externals: []const []const u8 = [],
    defines: []const BuildCmdKeyValue = [],
    loaders: []const BuildCmdKeyValue = [],
    public_path: []const u8 = "",
    entry_naming: []const u8 = BUILD_CMD_DEFAULT_ENTRY_NAMING,
    chunk_naming: []const u8 = BUILD_CMD_DEFAULT_CHUNK_NAMING,
    asset_naming: []const u8 = BUILD_CMD_DEFAULT_ASSET_NAMING,

    static fn defaults(): BuildCommandConfig {
        return BuildCommandConfig{};
    }

    fn isMinified(): bool {
        return _.minify_whitespace || _.minify_syntax || _.minify_identifiers;
    }

    fn outputsToStdout(): bool {
        return !_.compile && _.outfile.len == 0 && _.outdir.len == 0;
    }

    fn supportsMultipleOutputs(): bool {
        return _.outdir.len > 0 && _.outfile.len == 0 && !_.compile;
    }
}

/// Caller-owned slice buffers backing `parseBuildCommandArgs`.
pub struct BuildCmdParseBuffers {
    entry_points: [][]const u8,
    externals: [][]const u8,
    defines: []BuildCmdKeyValue,
    loaders: []BuildCmdKeyValue,
}

fn splitFlagEq(arg: []const u8): BuildCmdKeyValue {
    if (eq_pos := bcIndexOf(arg, '=')) {
        return BuildCmdKeyValue{
            key: arg[0..eq_pos],
            value: arg[eq_pos + 1..arg.len],
        };
    }
    return BuildCmdKeyValue{ key: arg, value: "" };
}

fn hasInlineEq(arg: []const u8): bool {
    return bcIndexOf(arg, '=') != null;
}

fn consumeFlagValue(
    args: []const []const u8,
    idx: *usize,
    inline_has_eq: bool,
    inline_val: []const u8,
): ([]const u8)! {
    if (inline_has_eq) {
        if (inline_val.len == 0) return err.MissingFlagValue;
        return inline_val;
    }
    if (idx.* + 1 >= args.len) return err.MissingFlagValue;
    idx.* += 1;
    return args[idx.*];
}

/// Parses raw `cap build` CLI arguments into a zero-allocation `BuildCommandConfig`.
pub fn parseBuildCommandArgs(
    args: []const []const u8,
    bufs: *BuildCmdParseBuffers,
): BuildCommandConfig! {
    let cfg = BuildCommandConfig.defaults();
    let entry_count: usize = 0;
    let ext_count: usize = 0;
    let def_count: usize = 0;
    let ldr_count: usize = 0;
    let positional_only = false;

    let i: usize = 0;
    while (i < args.len) {
        const arg = args[i];
        if (!positional_only && bcBytesEql(arg, "--")) {
            positional_only = true;
            i += 1;
            continue;
        }

        if (positional_only || arg.len == 0 || arg[0] != '-') {
            if (entry_count >= bufs.entry_points.len) return err.SliceCapacityExceeded;
            bufs.entry_points[entry_count] = arg;
            entry_count += 1;
            i += 1;
            continue;
        }

        const has_eq = hasInlineEq(arg);
        const kv = splitFlagEq(arg);
        const flag = kv.key;

        if (bcBytesEql(flag, "--outdir") || bcBytesEql(flag, "-d")) {
            cfg.outdir = consumeFlagValue(args, &i, has_eq, kv.value)?;
        } else if (bcBytesEql(flag, "--outfile") || bcBytesEql(flag, "-o")) {
            cfg.outfile = consumeFlagValue(args, &i, has_eq, kv.value)?;
        } else if (bcBytesEql(flag, "--target") || bcBytesEql(flag, "-t")) {
            const val = consumeFlagValue(args, &i, has_eq, kv.value)?;
            if (t := BuildCmdTarget.fromStr(val)) {
                cfg.target = t;
                if (t == BuildCmdTarget.Nest) cfg.nest_mode = true;
            } else {
                return err.InvalidTarget;
            }
        } else if (bcBytesEql(flag, "--format")) {
            const val = consumeFlagValue(args, &i, has_eq, kv.value)?;
            if (f := BuildCmdFormat.fromStr(val)) {
                cfg.format = f;
                if (f == BuildCmdFormat.Nest) cfg.nest_mode = true;
            } else {
                return err.InvalidFormat;
            }
        } else if (bcBytesEql(flag, "--minify")) {
            cfg.minify_whitespace = true;
            cfg.minify_syntax = true;
            cfg.minify_identifiers = true;
        } else if (bcBytesEql(flag, "--minify-whitespace")) {
            cfg.minify_whitespace = true;
        } else if (bcBytesEql(flag, "--minify-syntax")) {
            cfg.minify_syntax = true;
        } else if (bcBytesEql(flag, "--minify-identifiers")) {
            cfg.minify_identifiers = true;
        } else if (bcBytesEql(flag, "--sourcemap")) {
            if (has_eq) {
                if (sm := BuildCmdSourceMap.fromStr(kv.value)) {
                    cfg.sourcemap = sm;
                } else {
                    return err.InvalidSourceMapMode;
                }
            } else {
                // Bare `--sourcemap` defaults to `linked` (matches Bun CLI)
                cfg.sourcemap = BuildCmdSourceMap.Linked;
            }
        } else if (bcBytesEql(flag, "--splitting")) {
            cfg.splitting = true;
        } else if (bcBytesEql(flag, "--compile")) {
            cfg.compile = true;
        } else if (bcBytesEql(flag, "--bytecode")) {
            cfg.bytecode = true;
        } else if (bcBytesEql(flag, "-n") || bcBytesEql(flag, "--nest")) {
            cfg.nest_mode = true;
            cfg.format = BuildCmdFormat.Nest;
        } else if (bcBytesEql(flag, "--watch") || bcBytesEql(flag, "-w")) {
            cfg.watch = true;
        } else if (bcBytesEql(flag, "--production")) {
            cfg.production = true;
        } else if (bcBytesEql(flag, "--external") || bcBytesEql(flag, "-e")) {
            const val = consumeFlagValue(args, &i, has_eq, kv.value)?;
            if (ext_count >= bufs.externals.len) return err.SliceCapacityExceeded;
            bufs.externals[ext_count] = val;
            ext_count += 1;
        } else if (bcBytesEql(flag, "--define")) {
            const val = consumeFlagValue(args, &i, has_eq, kv.value)?;
            if (eq_idx := bcIndexOf(val, '=')) {
                if (eq_idx == 0) return err.InvalidDefineSyntax;
                if (def_count >= bufs.defines.len) return err.SliceCapacityExceeded;
                bufs.defines[def_count] = BuildCmdKeyValue{
                    key: val[0..eq_idx],
                    value: val[eq_idx + 1..val.len],
                };
                def_count += 1;
            } else {
                return err.InvalidDefineSyntax;
            }
        } else if (bcBytesEql(flag, "--loader")) {
            const val = consumeFlagValue(args, &i, has_eq, kv.value)?;
            if (col_idx := bcIndexOf(val, ':')) {
                if (col_idx == 0 || col_idx + 1 >= val.len || val[0] != '.') {
                    return err.InvalidLoaderSyntax;
                }
                if (ldr_count >= bufs.loaders.len) return err.SliceCapacityExceeded;
                bufs.loaders[ldr_count] = BuildCmdKeyValue{
                    key: val[0..col_idx],
                    value: val[col_idx + 1..val.len],
                };
                ldr_count += 1;
            } else {
                return err.InvalidLoaderSyntax;
            }
        } else if (bcBytesEql(flag, "--public-path")) {
            cfg.public_path = consumeFlagValue(args, &i, has_eq, kv.value)?;
        } else if (bcBytesEql(flag, "--entry-naming")) {
            cfg.entry_naming = consumeFlagValue(args, &i, has_eq, kv.value)?;
        } else if (bcBytesEql(flag, "--chunk-naming")) {
            cfg.chunk_naming = consumeFlagValue(args, &i, has_eq, kv.value)?;
        } else if (bcBytesEql(flag, "--asset-naming")) {
            cfg.asset_naming = consumeFlagValue(args, &i, has_eq, kv.value)?;
        } else {
            return err.UnknownFlag;
        }

        i += 1;
    }

    cfg.entry_points = bufs.entry_points[0..entry_count];
    cfg.externals = bufs.externals[0..ext_count];
    cfg.defines = bufs.defines[0..def_count];
    cfg.loaders = bufs.loaders[0..ldr_count];
    return cfg;
}

// ============================================================================
// Section 2: Semantic Conflict & Security Validator
// ============================================================================

/// Diagnostic issue code produced by `validateBuildConfig`.
pub enum BuildValidationIssueKind: u8 {
    None = 0,
    NoEntryPoints = 1,
    OutdirAndOutfileConflict = 2,
    MultiEntryRequiresOutdir = 3,
    SplittingRequiresOutdir = 4,
    SplittingIncompatibleWithCjs = 5,
    CompileIncompatibleWithSplitting = 6,
    CompileIncompatibleWithOutdir = 7,
    ExternalSourcemapRequiresOutdir = 8,
    InvalidNamingPlaceholder = 9,
    PathTraversalInOutput = 10,
    ReservedStandaloneBinaryName = 11,
}

/// Structured validation result from `validateBuildConfig`.
pub struct BuildValidationResult {
    valid: bool = true,
    issue: BuildValidationIssueKind = BuildValidationIssueKind.None,
    message: []const u8 = "",
}

/// Returns false if `path` contains null bytes or `..` parent-directory traversal segments.
pub fn isSafeBuildOutputPath(path: []const u8): bool {
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == 0) return false;
        // Check segment boundaries for ".."
        const at_seg_start = (i == 0) || path[i - 1] == '/' || path[i - 1] == '\\';
        if (at_seg_start && i + 1 < path.len && path[i] == '.' && path[i + 1] == '.') {
            const at_seg_end = (i + 2 == path.len) || path[i + 2] == '/' || path[i + 2] == '\\';
            if (at_seg_end) return false;
        }
        i += 1;
    }
    return true;
}

/// Validates a naming template (e.g. `[dir]/[name]-[hash].[ext]`), ensuring balanced
/// brackets, only recognized placeholders (`dir`, `name`, `hash`, `ext`), and no `..` traversal.
pub fn validateBuildNamingTemplate(tpl: []const u8): bool {
    if (tpl.len == 0) return false;
    if (!isSafeBuildOutputPath(tpl)) return false;

    let i: usize = 0;
    while (i < tpl.len) {
        const c = tpl[i];
        if (c == ']') return false;
        if (c == '[') {
            const start = i + 1;
            let close = start;
            while (close < tpl.len && tpl[close] != ']') {
                if (tpl[close] == '[') return false;
                close += 1;
            }
            if (close >= tpl.len) return false;
            const token = tpl[start..close];
            const ok = bcBytesEql(token, "dir") ||
                bcBytesEql(token, "name") ||
                bcBytesEql(token, "hash") ||
                bcBytesEql(token, "ext");
            if (!ok) return false;
            i = close + 1;
            continue;
        }
        i += 1;
    }
    return true;
}

/// Validates semantic invariants and security boundaries of a `BuildCommandConfig`.
pub fn validateBuildConfig(cfg: BuildCommandConfig): BuildValidationResult {
    if (cfg.entry_points.len == 0) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.NoEntryPoints,
            message: "at least one entry point is required",
        };
    }

    if (cfg.outdir.len > 0 && cfg.outfile.len > 0) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.OutdirAndOutfileConflict,
            message: "cannot specify both --outdir and --outfile",
        };
    }

    if (cfg.compile && cfg.splitting) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.CompileIncompatibleWithSplitting,
            message: "cannot use --compile with --splitting",
        };
    }

    if (cfg.compile && cfg.outdir.len > 0) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.CompileIncompatibleWithOutdir,
            message: "cannot use --compile with --outdir; use --outfile instead",
        };
    }

    if (cfg.splitting && cfg.format == BuildCmdFormat.Cjs) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.SplittingIncompatibleWithCjs,
            message: "code --splitting is not supported with --format=cjs",
        };
    }

    if (!cfg.compile && cfg.outdir.len == 0) {
        if (cfg.entry_points.len > 1) {
            return BuildValidationResult{
                valid: false,
                issue: BuildValidationIssueKind.MultiEntryRequiresOutdir,
                message: "must specify --outdir when building multiple entry points",
            };
        }
        if (cfg.splitting) {
            return BuildValidationResult{
                valid: false,
                issue: BuildValidationIssueKind.SplittingRequiresOutdir,
                message: "must specify --outdir when --splitting is enabled",
            };
        }
        if (cfg.sourcemap == BuildCmdSourceMap.External) {
            return BuildValidationResult{
                valid: false,
                issue: BuildValidationIssueKind.ExternalSourcemapRequiresOutdir,
                message: "cannot use --sourcemap=external without --outdir",
            };
        }
    }

    if (cfg.compile && cfg.outfile.len > 0) {
        const base = bcBasename(cfg.outfile);
        if (bcBytesEqlIgnoreCase(base, "cap") || bcBytesEqlIgnoreCase(base, "bun") || bcBytesEqlIgnoreCase(base, "bunx")) {
            return BuildValidationResult{
                valid: false,
                issue: BuildValidationIssueKind.ReservedStandaloneBinaryName,
                message: "standalone --outfile cannot be named 'cap', 'bun', or 'bunx'",
            };
        }
    }

    if ((cfg.outdir.len > 0 && !isSafeBuildOutputPath(cfg.outdir)) ||
        (cfg.outfile.len > 0 && !isSafeBuildOutputPath(cfg.outfile))) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.PathTraversalInOutput,
            message: "output path contains forbidden parent-directory traversal ('..')",
        };
    }

    if (!validateBuildNamingTemplate(cfg.entry_naming) ||
        !validateBuildNamingTemplate(cfg.chunk_naming) ||
        !validateBuildNamingTemplate(cfg.asset_naming)) {
        return BuildValidationResult{
            valid: false,
            issue: BuildValidationIssueKind.InvalidNamingPlaceholder,
            message: "invalid placeholder or unsafe path in naming template",
        };
    }

    return BuildValidationResult{ valid: true, issue: BuildValidationIssueKind.None, message: "" };
}

// ============================================================================
// Section 3: Cross-Compile Target Matrix Parser & Artifact Path Planner
// ============================================================================

/// Cross-compilation operating system family.
pub enum CrossOsKind: u8 {
    Linux = 0,
    Macos = 1,
    Windows = 2,
    Wasm = 3,
    Nest = 4,

    fn asStr(): []const u8 {
        return switch (_) {
            .Linux => "linux",
            .Macos => "macos",
            .Windows => "windows",
            .Wasm => "wasm",
            .Nest => "nest",
        };
    }
}

/// Cross-compilation CPU architecture family.
pub enum CrossArchKind: u8 {
    X86_64 = 0,
    Aarch64 = 1,
    Wasm32 = 2,
    Any = 3,

    fn asStr(): []const u8 {
        return switch (_) {
            .X86_64 => "x86_64",
            .Aarch64 => "aarch64",
            .Wasm32 => "wasm32",
            .Any => "any",
        };
    }
}

/// Cross-compilation libc / runtime ABI variant.
pub enum CrossAbiKind: u8 {
    Gnu = 0,
    Musl = 1,
    Darwin = 2,
    Msvc = 3,
    Wasi = 4,
    None = 5,
}

/// Normalized cross-compilation target triple.
pub struct CrossTargetSpec {
    os: CrossOsKind = CrossOsKind.Linux,
    arch: CrossArchKind = CrossArchKind.X86_64,
    abi: CrossAbiKind = CrossAbiKind.Gnu,
    is_baseline: bool = false,
    is_modern: bool = false,

    /// Returns the platform-appropriate binary extension (`.exe`, `.wasm`, `.nest`, or `""`).
    fn defaultExtension(): []const u8 {
        return switch (_.os) {
            .Windows => ".exe",
            .Wasm => ".wasm",
            .Nest => ".nest",
            else => "",
        };
    }

    /// Formats the canonical `cap` distribution directory name (e.g. `cap-macos-aarch64`).
    fn canonicalDirName(out: []u8): usize! {
        let pos: usize = 0;
        bcAppendBytes(out, &pos, "cap-")?;
        bcAppendBytes(out, &pos, _.os.asStr())?;
        if (_.os != CrossOsKind.Nest && _.os != CrossOsKind.Wasm) {
            bcAppendByte(out, &pos, '-')?;
            bcAppendBytes(out, &pos, _.arch.asStr())?;
        }
        if (_.abi == CrossAbiKind.Musl) {
            bcAppendBytes(out, &pos, "-musl")?;
        }
        if (_.is_baseline) {
            bcAppendBytes(out, &pos, "-baseline")?;
        }
        return pos;
    }
}

/// Parses a cross-compilation target string such as `cap-linux-x86_64`,
/// `cap-macos-aarch64`, `cap-windows-x86_64`, `linux-arm64-musl`, `wasm`, or `nest`.
pub fn parseCrossCompileTarget(raw: []const u8): CrossTargetSpec! {
    if (raw.len == 0) return err.InvalidTarget;

    // Strip optional `cap-` or `bun-` prefix
    let s = raw;
    if (s.len > 4 && (bcBytesEqlIgnoreCase(s[0..4], "cap-") || bcBytesEqlIgnoreCase(s[0..4], "bun-"))) {
        s = s[4..s.len];
    }

    if (bcBytesEqlIgnoreCase(s, "nest")) {
        return CrossTargetSpec{
            os: CrossOsKind.Nest,
            arch: CrossArchKind.Any,
            abi: CrossAbiKind.None,
        };
    }
    if (bcBytesEqlIgnoreCase(s, "wasm") || bcBytesEqlIgnoreCase(s, "wasm32") || bcBytesEqlIgnoreCase(s, "wasm32-wasi")) {
        return CrossTargetSpec{
            os: CrossOsKind.Wasm,
            arch: CrossArchKind.Wasm32,
            abi: CrossAbiKind.Wasi,
        };
    }

    // Split on '-' tokens (up to 4 parts: os-arch[-abi][-cpu])
    let has_os = false;
    let has_arch = false;
    let spec = CrossTargetSpec{};

    let start: usize = 0;
    while (start < s.len) {
        let end = start;
        while (end < s.len && s[end] != '-') {
            end += 1;
        }
        const tok = s[start..end];
        if (tok.len > 0) {
            if (bcBytesEqlIgnoreCase(tok, "linux")) {
                spec.os = CrossOsKind.Linux;
                spec.abi = CrossAbiKind.Gnu;
                has_os = true;
            } else if (bcBytesEqlIgnoreCase(tok, "macos") || bcBytesEqlIgnoreCase(tok, "darwin")) {
                spec.os = CrossOsKind.Macos;
                spec.abi = CrossAbiKind.Darwin;
                has_os = true;
            } else if (bcBytesEqlIgnoreCase(tok, "windows") || bcBytesEqlIgnoreCase(tok, "win")) {
                spec.os = CrossOsKind.Windows;
                spec.abi = CrossAbiKind.Msvc;
                has_os = true;
            } else if (bcBytesEqlIgnoreCase(tok, "x64") || bcBytesEqlIgnoreCase(tok, "x86_64") || bcBytesEqlIgnoreCase(tok, "amd64")) {
                spec.arch = CrossArchKind.X86_64;
                has_arch = true;
            } else if (bcBytesEqlIgnoreCase(tok, "arm64") || bcBytesEqlIgnoreCase(tok, "aarch64")) {
                spec.arch = CrossArchKind.Aarch64;
                has_arch = true;
            } else if (bcBytesEqlIgnoreCase(tok, "musl")) {
                spec.abi = CrossAbiKind.Musl;
            } else if (bcBytesEqlIgnoreCase(tok, "gnu")) {
                spec.abi = CrossAbiKind.Gnu;
            } else if (bcBytesEqlIgnoreCase(tok, "baseline")) {
                spec.is_baseline = true;
            } else if (bcBytesEqlIgnoreCase(tok, "modern")) {
                spec.is_modern = true;
            } else {
                return err.InvalidTarget;
            }
        }
        start = if (end < s.len) end + 1 else s.len;
    }

    if (!has_os || !has_arch) return err.InvalidTarget;
    return spec;
}

/// Formats a normalized cross-platform output artifact path:
/// `<out_dir>[/<cap-os-arch>]/<base_name>[.exe|.nest|.wasm]`.
/// Automatically appends `.exe` on Windows and `.nest` when `nest_mode` is true,
/// while preventing duplicate extensions.
pub fn formatCrossArtifactPath(
    out_dir: []const u8,
    base_name: []const u8,
    spec: CrossTargetSpec,
    nest_mode: bool,
    include_target_subdir: bool,
    out: []u8,
): usize! {
    if (base_name.len == 0) return err.MissingEntryPoint;
    if (!isSafeBuildOutputPath(out_dir) || !isSafeBuildOutputPath(base_name)) {
        return err.PathTraversalDetected;
    }

    let pos: usize = 0;
    if (out_dir.len > 0) {
        const clean_dir = bcStripTrailingSlash(out_dir);
        bcAppendBytes(out, &pos, clean_dir)?;
        bcAppendByte(out, &pos, '/')?;
    }

    if (include_target_subdir) {
        let dir_buf: [64]u8 = [0; 64];
        const dir_len = spec.canonicalDirName(dir_buf[0..])?;
        bcAppendBytes(out, &pos, dir_buf[0..dir_len])?;
        bcAppendByte(out, &pos, '/')?;
    }

    bcAppendBytes(out, &pos, base_name)?;

    const ext = if (nest_mode) ".nest" else spec.defaultExtension();
    if (ext.len > 0 && !bcEndsWith(base_name, ext)) {
        bcAppendBytes(out, &pos, ext)?;
    }
    return pos;
}

// ============================================================================
// Section 4: Column-Aligned Terminal Build Artifact Table Renderer
// ============================================================================

/// Artifact category tag displayed in the build output summary table.
pub enum BuildArtifactKind: u8 {
    Entry = 0,
    Chunk = 1,
    Asset = 2,
    SourceMap = 3,
    Native = 4,
    Nest = 5,

    fn asTag(): []const u8 {
        return switch (_) {
            .Entry => "entry",
            .Chunk => "chunk",
            .Asset => "asset",
            .SourceMap => "sourcemap",
            .Native => "native",
            .Nest => "nest",
        };
    }
}

/// Single emitted build artifact row for `formatBuildArtifactTable`.
pub struct BuildArtifactEntry {
    path: []const u8 = "",
    kind: BuildArtifactKind = BuildArtifactKind.Entry,
    byte_size: u64 = 0,
    content_hash32: u32 = 0,

    static fn empty(): BuildArtifactEntry {
        return BuildArtifactEntry{};
    }
}

/// Build completion metrics shown beneath the artifact table.
pub struct BuildSummaryMetrics {
    elapsed_ms: u64 = 0,
    input_module_count: u32 = 0,
    cache_hits: u32 = 0,
    warnings_count: u32 = 0,
}

/// Renders a column-aligned terminal table of build artifacts with type badges,
/// human-readable byte sizes, and total build metrics into caller-owned `out`.
pub fn formatBuildArtifactTable(
    artifacts: []const BuildArtifactEntry,
    metrics: BuildSummaryMetrics,
    use_color: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;

    // Compute dynamic path column width (minimum 16) and size column width (minimum 9)
    let max_path_w: usize = 16;
    let max_tag_w: usize = 9; // "sourcemap"
    let total_bytes: u64 = 0;

    let i: usize = 0;
    while (i < artifacts.len) {
        if (artifacts[i].path.len > max_path_w) {
            max_path_w = artifacts[i].path.len;
        }
        const tlen = artifacts[i].kind.asTag().len;
        if (tlen > max_tag_w) max_tag_w = tlen;
        total_bytes += artifacts[i].byte_size;
        i += 1;
    }

    // Header row
    bcAppendBytes(out, &pos, "Artifact")?;
    bcAppendSpaces(out, &pos, max_path_w - 8 + 2)?;
    bcAppendBytes(out, &pos, "Kind")?;
    bcAppendSpaces(out, &pos, max_tag_w - 4 + 2)?;
    bcAppendBytes(out, &pos, "     Size\n")?;

    // Divider line
    let div_len = max_path_w + 2 + max_tag_w + 2 + 9;
    let d: usize = 0;
    while (d < div_len) {
        bcAppendByte(out, &pos, '-')?;
        d += 1;
    }
    bcAppendByte(out, &pos, '\n')?;

    // Artifact rows
    let r: usize = 0;
    while (r < artifacts.len) {
        const a = artifacts[r];
        if (use_color) bcAppendBytes(out, &pos, "\x1b[36m")?;
        bcAppendBytes(out, &pos, a.path)?;
        if (use_color) bcAppendBytes(out, &pos, "\x1b[0m")?;
        bcAppendSpaces(out, &pos, max_path_w - a.path.len + 2)?;

        const tag = a.kind.asTag();
        if (use_color) {
            const ccode = switch (a.kind) {
                .Entry, .Native, .Nest => "\x1b[32m",
                .Chunk => "\x1b[33m",
                .Asset => "\x1b[35m",
                .SourceMap => "\x1b[90m",
            };
            bcAppendBytes(out, &pos, ccode)?;
        }
        bcAppendBytes(out, &pos, tag)?;
        if (use_color) bcAppendBytes(out, &pos, "\x1b[0m")?;
        bcAppendSpaces(out, &pos, max_tag_w - tag.len + 2)?;

        let sz_buf: [24]u8 = [0; 24];
        let sz_len: usize = 0;
        formatHumanByteSize(a.byte_size, sz_buf[0..], &sz_len)?;
        if (sz_len < 9) {
            bcAppendSpaces(out, &pos, 9 - sz_len)?;
        }
        bcAppendBytes(out, &pos, sz_buf[0..sz_len])?;
        bcAppendByte(out, &pos, '\n')?;
        r += 1;
    }

    // Footer summary
    let d2: usize = 0;
    while (d2 < div_len) {
        bcAppendByte(out, &pos, '-')?;
        d2 += 1;
    }
    bcAppendByte(out, &pos, '\n')?;

    if (use_color) bcAppendBytes(out, &pos, "\x1b[32m")?;
    bcAppendBytes(out, &pos, "Built ")?;
    bcAppendU64(out, &pos, @as(u64, artifacts.len))?;
    bcAppendBytes(out, &pos, " artifact(s) (")?;
    formatHumanByteSize(total_bytes, out, &pos)?;
    bcAppendBytes(out, &pos, ") in ")?;
    bcFormatDurationMs(metrics.elapsed_ms, out, &pos)?;
    if (use_color) bcAppendBytes(out, &pos, "\x1b[0m")?;

    if (metrics.input_module_count > 0) {
        bcAppendBytes(out, &pos, " | ")?;
        bcAppendU64(out, &pos, @as(u64, metrics.input_module_count))?;
        bcAppendBytes(out, &pos, " module(s)")?;
        if (metrics.cache_hits > 0) {
            bcAppendBytes(out, &pos, ", ")?;
            bcAppendU64(out, &pos, @as(u64, metrics.cache_hits))?;
            bcAppendBytes(out, &pos, " cached")?;
        }
    }
    if (metrics.warnings_count > 0) {
        bcAppendBytes(out, &pos, " | ")?;
        bcAppendU64(out, &pos, @as(u64, metrics.warnings_count))?;
        bcAppendBytes(out, &pos, " warning(s)")?;
    }
    bcAppendByte(out, &pos, '\n')?;

    return pos;
}
