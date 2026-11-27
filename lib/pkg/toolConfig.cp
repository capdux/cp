// Source: Absorbed from /data/dev/bun/src/cli/bunfig.cp | Synced: 2026-11-23T20:18:24Z
// ============================================================================
// lib/pkg/toolConfig.cp — Zero-Alloc Multi-Layer Toolchain Configuration Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven TOML configuration parser (capfig.toml /
// bunfig.toml), environment variable interpolator ($VAR / ${VAR}), 4-layer priority
// merger (System < User < Project < CLI), scoped registry router with secret token
// redaction, and strict/lenient configuration validator.

/// Default npm-compatible package registry URL.
pub const TOOL_CONFIG_DEFAULT_REGISTRY: []const u8 = "https://registry.npmjs.org";

/// Default dev server port when `[serve].port` is unset or 0.
pub const TOOL_CONFIG_DEFAULT_PORT: u16 = 3000;

/// Default console object inspection depth.
pub const TOOL_CONFIG_DEFAULT_CONSOLE_DEPTH: u32 = 2;

/// Default test timeout in milliseconds.
pub const TOOL_CONFIG_DEFAULT_TEST_TIMEOUT_MS: u32 = 5000;

/// Upper bound for concurrent lifecycle scripts.
pub const TOOL_CONFIG_MAX_CONCURRENT_SCRIPTS: u32 = 1024;

/// Configuration parsing, interpolation, merging, and validation error set.
pub err ToolConfigError {
    EmptyInput,
    InvalidSectionHeader,
    InvalidKeyValueSyntax,
    UnterminatedString,
    UnterminatedInlineTable,
    UnterminatedArray,
    InvalidBoolean,
    InvalidNumber,
    InvalidLogLevel,
    InvalidLinkerMode,
    InvalidShellMode,
    InvalidAutoInstallMode,
    InvalidRegistryUrl,
    InvalidTimeout,
    InvalidConcurrency,
    InvalidCoverageThreshold,
    DuplicateKey,
    UnknownSection,
    UnknownKey,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Configuration precedence layer (higher numeric value overrides lower).
pub enum ToolConfigLayer: u8 {
    System = 0,
    User = 1,
    Project = 2,
    CliOverride = 3,
}

/// Toolchain log verbosity level.
pub enum ToolConfigLogLevel: u8 {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
    Silent = 4,
}

/// Package linker strategy (`[install].linker`).
pub enum ToolConfigLinker: u8 {
    Hoisted = 0,
    Isolated = 1,
}

/// Automatic package installation mode (`[install].auto`).
pub enum ToolConfigAutoInstall: u8 {
    Auto = 0,
    Force = 1,
    Disable = 2,
    Fallback = 3,
}

/// Script runner shell selection (`[run].shell`).
pub enum ToolConfigShell: u8 {
    System = 0,
    Cap = 1,
}

/// Diagnostic severity level emitted during parsing or validation.
pub enum ToolConfigDiagSeverity: u8 {
    Warning = 0,
    Error = 1,
}

/// Caller-provided environment variable key/value binding for `$VAR`/`${VAR}` expansion.
pub struct ToolEnvVar {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// Scoped registry binding parsed from `[install.scopes]`.
pub struct ToolScopeRegistry {
    scope: []const u8 = "",
    url: []const u8 = "",
    token: []const u8 = "",
    username: []const u8 = "",
    password: []const u8 = "",
    email: []const u8 = "",
    layer: ToolConfigLayer = ToolConfigLayer.Project,

    static fn empty(): ToolScopeRegistry {
        return ToolScopeRegistry{};
    }
}

/// Generic key/value entry used for `[define]`, `[loader]`, and `preload` arrays.
pub struct ToolKeyValueEntry {
    key: []const u8 = "",
    value: []const u8 = "",
    layer: ToolConfigLayer = ToolConfigLayer.Project,

    static fn empty(): ToolKeyValueEntry {
        return ToolKeyValueEntry{};
    }
}

/// Structured parser/validator diagnostic entry.
pub struct ToolConfigDiagnostic {
    severity: ToolConfigDiagSeverity = ToolConfigDiagSeverity.Warning,
    line_no: u32 = 0,
    section: []const u8 = "",
    key: []const u8 = "",
    message: []const u8 = "",

    static fn empty(): ToolConfigDiagnostic {
        return ToolConfigDiagnostic{};
    }
}

/// `[install]`, `[install.cache]`, `[install.lockfile]`, and `[install.security]` settings.
/// Every field is optional (`T?`) to distinguish "unset" from explicit `false`/`0`.
pub struct ToolInstallConfig {
    registry_url: ([]const u8)? = null,
    registry_token: ([]const u8)? = null,
    registry_username: ([]const u8)? = null,
    registry_password: ([]const u8)? = null,
    registry_email: ([]const u8)? = null,
    auto_install: ToolConfigAutoInstall? = null,
    linker: ToolConfigLinker? = null,
    optional: bool? = null,
    dev: bool? = null,
    peer: bool? = null,
    production: bool? = null,
    exact: bool? = null,
    dry_run: bool? = null,
    frozen_lockfile: bool? = null,
    save_text_lockfile: bool? = null,
    ignore_scripts: bool? = null,
    link_workspace_packages: bool? = null,
    global_store: bool? = null,
    concurrent_scripts: u32? = null,
    minimum_release_age_secs: u64? = null,
    global_dir: ([]const u8)? = null,
    global_bin_dir: ([]const u8)? = null,
    cafile: ([]const u8)? = null,
    log_level: ToolConfigLogLevel? = null,
    cache_disable: bool? = null,
    cache_disable_manifest: bool? = null,
    cache_dir: ([]const u8)? = null,
    lockfile_save: bool? = null,
    lockfile_print: ([]const u8)? = null,
    lockfile_path: ([]const u8)? = null,
    security_scanner: ([]const u8)? = null,

    static fn empty(): ToolInstallConfig {
        return ToolInstallConfig{};
    }
}

/// `[test]` configuration section.
pub struct ToolTestConfig {
    root: ([]const u8)? = null,
    smol: bool? = null,
    coverage: bool? = null,
    only_failures: bool? = null,
    randomize: bool? = null,
    coverage_dir: ([]const u8)? = null,
    coverage_ignore_sourcemaps: bool? = null,
    coverage_skip_test_files: bool? = null,
    coverage_threshold_lines: f64? = null,
    coverage_threshold_functions: f64? = null,
    coverage_threshold_statements: f64? = null,
    timeout_ms: u32? = null,
    seed: u32? = null,
    rerun_each: u32? = null,
    retry: u32? = null,
    reporter_dots: bool? = null,
    reporter_junit: ([]const u8)? = null,

    static fn empty(): ToolTestConfig {
        return ToolTestConfig{};
    }
}

/// `[run]` configuration section.
pub struct ToolRunConfig {
    silent: bool? = null,
    elide_lines: u32? = null,
    shell: ToolConfigShell? = null,
    cap_alias: bool? = null,
    no_orphans: bool? = null,

    static fn empty(): ToolRunConfig {
        return ToolRunConfig{};
    }
}

/// `[debug]`, `[console]`, and `[serve]` configuration settings.
pub struct ToolDebugConfig {
    editor: ([]const u8)? = null,
    console_depth: u32? = null,
    serve_port: u16? = null,
    serve_hmr: bool? = null,
    minify_syntax: bool? = null,
    minify_whitespace: bool? = null,
    minify_identifiers: bool? = null,

    static fn empty(): ToolDebugConfig {
        return ToolDebugConfig{};
    }
}

/// Complete parsed or merged toolchain configuration.
pub struct ToolConfig {
    layer: ToolConfigLayer = ToolConfigLayer.Project,
    log_level: ToolConfigLogLevel? = null,
    smol: bool? = null,
    telemetry: bool? = null,
    env_disabled: bool? = null,
    origin: ([]const u8)? = null,
    install: ToolInstallConfig = ToolInstallConfig.empty(),
    test_cfg: ToolTestConfig = ToolTestConfig.empty(),
    run: ToolRunConfig = ToolRunConfig.empty(),
    debug: ToolDebugConfig = ToolDebugConfig.empty(),
    scopes_len: usize = 0,
    defines_len: usize = 0,
    loaders_len: usize = 0,
    preloads_len: usize = 0,
    diags_len: usize = 0,

    static fn empty(layer: ToolConfigLayer): ToolConfig {
        return ToolConfig{ layer: layer };
    }
}

/// Parser behavior flags.
pub struct ToolConfigParseOptions {
    layer: ToolConfigLayer = ToolConfigLayer.Project,
    strict_unknown: bool = false,
    reject_duplicate_keys: bool = true,
    keep_unresolved_env: bool = true,

    static fn defaults(): ToolConfigParseOptions {
        return ToolConfigParseOptions{};
    }

    static fn strict(layer: ToolConfigLayer): ToolConfigParseOptions {
        return ToolConfigParseOptions{
            layer: layer,
            strict_unknown: true,
            reject_duplicate_keys: true,
            keep_unresolved_env: true,
        };
    }
}

/// Caller-owned zero-heap backing slices passed to `parseToolConfig`.
pub struct ToolConfigStorage {
    scopes: []ToolScopeRegistry,
    defines: []ToolKeyValueEntry,
    loaders: []ToolKeyValueEntry,
    preloads: []ToolKeyValueEntry,
    diags: []ToolConfigDiagnostic,
    str_arena: []u8,
    str_pos: usize = 0,

    fn allocCopy(*_, data: []const u8): ([]const u8)! {
        if (_.str_pos + data.len > _.str_arena.len) {
            return ToolConfigError.BufferTooSmall;
        }
        const start = _.str_pos;
        let i: usize = 0;
        while (i < data.len) {
            _.str_arena[start + i] = data[i];
            i += 1;
        }
        _.str_pos += data.len;
        return _.str_arena[start .. start + data.len];
    }
}

/// Resolved registry endpoint and credentials returned by `resolveScopedRegistry`.
pub struct ResolvedRegistryRoute {
    url: []const u8 = TOOL_CONFIG_DEFAULT_REGISTRY,
    token: []const u8 = "",
    username: []const u8 = "",
    password: []const u8 = "",
    email: []const u8 = "",
    matched_scope: []const u8 = "",
    is_scoped: bool = false,
}

// ============================================================================
// Internal Byte & String Utilities (Zero-Alloc)
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
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

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn isEnvIdentChar(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

fn lookupEnv(env_vars: []const ToolEnvVar, name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < env_vars.len) {
        if (bytesEqual(env_vars[i].key, name)) {
            return env_vars[i].value;
        }
        i += 1;
    }
    return null;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) {
        return ToolConfigError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

/// Expands `$VAR`, `${VAR}`, and `$$` (literal `$`) using `env_vars` into `out_buf`.
pub fn expandEnvVars(
    raw: []const u8,
    env_vars: []const ToolEnvVar,
    keep_unresolved: bool,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '$') {
            if (i + 1 < raw.len && raw[i + 1] == '$') {
                appendBytes(out_buf, &pos, "$")?;
                i += 2;
                continue;
            }
            if (i + 1 < raw.len && raw[i + 1] == '{') {
                let close_idx = i + 2;
                while (close_idx < raw.len && raw[close_idx] != '}') {
                    close_idx += 1;
                }
                if (close_idx < raw.len && close_idx > i + 2) {
                    const var_name = raw[i + 2 .. close_idx];
                    if (val := lookupEnv(env_vars, var_name)) {
                        appendBytes(out_buf, &pos, val)?;
                    } else if (keep_unresolved) {
                        appendBytes(out_buf, &pos, raw[i .. close_idx + 1])?;
                    }
                    i = close_idx + 1;
                    continue;
                }
            } else if (i + 1 < raw.len && isEnvIdentChar(raw[i + 1])) {
                let end_idx = i + 1;
                while (end_idx < raw.len && isEnvIdentChar(raw[end_idx])) {
                    end_idx += 1;
                }
                const var_name = raw[i + 1 .. end_idx];
                if (val := lookupEnv(env_vars, var_name)) {
                    appendBytes(out_buf, &pos, val)?;
                } else if (keep_unresolved) {
                    appendBytes(out_buf, &pos, raw[i..end_idx])?;
                }
                i = end_idx;
                continue;
            }
        }
        if (pos >= out_buf.len) {
            return ToolConfigError.BufferTooSmall;
        }
        out_buf[pos] = raw[i];
        pos += 1;
        i += 1;
    }
    return pos;
}

/// Strips inline `# ...` comments outside of double/single quotes.
fn stripTomlComment(line: []const u8): []const u8 {
    let in_double = false;
    let in_single = false;
    let escaped = false;
    let i: usize = 0;
    while (i < line.len) {
        const c = line[i];
        if (in_double) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_double = false;
            }
        } else if (in_single) {
            if (c == '\'') {
                in_single = false;
            }
        } else {
            if (c == '"') {
                in_double = true;
            } else if (c == '\'') {
                in_single = true;
            } else if (c == '#') {
                return trimWs(line[0..i]);
            }
        }
        i += 1;
    }
    return trimWs(line);
}

/// Unescapes a TOML basic (`"..."`) or literal (`'...'`) string into `out_buf`.
pub fn unescapeTomlString(raw_token: []const u8, out_buf: []u8): usize! {
    const s = trimWs(raw_token);
    if (s.len < 2) {
        return ToolConfigError.UnterminatedString;
    }
    const quote = s[0];
    if ((quote != '"' && quote != '\'') || s[s.len - 1] != quote) {
        return ToolConfigError.UnterminatedString;
    }
    const inner = s[1 .. s.len - 1];
    if (quote == '\'') {
        if (inner.len > out_buf.len) return ToolConfigError.BufferTooSmall;
        let i: usize = 0;
        while (i < inner.len) {
            out_buf[i] = inner[i];
            i += 1;
        }
        return inner.len;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < inner.len) {
        const c = inner[i];
        if (c == '\\') {
            if (i + 1 >= inner.len) return ToolConfigError.UnterminatedString;
            const esc = inner[i + 1];
            let out_ch: u8 = esc;
            if (esc == 'n') {
                out_ch = @as(u8, 10);
            } else if (esc == 'r') {
                out_ch = @as(u8, 13);
            } else if (esc == 't') {
                out_ch = @as(u8, 9);
            } else if (esc == '"') {
                out_ch = @as(u8, 34);
            } else if (esc == '\\') {
                out_ch = @as(u8, 92);
            }
            if (pos >= out_buf.len) return ToolConfigError.BufferTooSmall;
            out_buf[pos] = out_ch;
            pos += 1;
            i += 2;
        } else {
            if (pos >= out_buf.len) return ToolConfigError.BufferTooSmall;
            out_buf[pos] = c;
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

fn parseBoolVal(raw: []const u8): bool! {
    const s = trimWs(raw);
    if (bytesEqual(s, "true")) return true;
    if (bytesEqual(s, "false")) return false;
    return ToolConfigError.InvalidBoolean;
}

fn parseU64Val(raw: []const u8): u64! {
    const s = trimWs(raw);
    if (s.len == 0) return ToolConfigError.InvalidNumber;
    if (s[0] == '-') return ToolConfigError.InvalidNumber;
    let val: u64 = 0;
    let has_digit = false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '_') {
            i += 1;
            continue;
        }
        if (c < '0' || c > '9') return ToolConfigError.InvalidNumber;
        has_digit = true;
        val = val * 10 + @as(u64, c - '0');
        i += 1;
    }
    if (!has_digit) return ToolConfigError.InvalidNumber;
    return val;
}

fn parseU32Val(raw: []const u8): u32! {
    const v = parseU64Val(raw)?;
    if (v > 4294967295) return ToolConfigError.InvalidNumber;
    return @intCast(v);
}

fn parseU16Val(raw: []const u8): u16! {
    const v = parseU64Val(raw)?;
    if (v > 65535) return ToolConfigError.InvalidNumber;
    return @intCast(v);
}

fn parseF64Val(raw: []const u8): f64! {
    const s = trimWs(raw);
    if (s.len == 0) return ToolConfigError.InvalidNumber;
    let neg = false;
    let i: usize = 0;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return ToolConfigError.InvalidNumber;

    let int_part: f64 = 0.0;
    let frac_part: f64 = 0.0;
    let frac_div: f64 = 1.0;
    let seen_dot = false;
    let has_digit = false;

    while (i < s.len) {
        const c = s[i];
        if (c == '_') {
            i += 1;
            continue;
        }
        if (c == '.') {
            if (seen_dot) return ToolConfigError.InvalidNumber;
            seen_dot = true;
            i += 1;
            continue;
        }
        if (c < '0' || c > '9') return ToolConfigError.InvalidNumber;
        has_digit = true;
        const d: f64 = @floatFromInt(c - '0');
        if (!seen_dot) {
            int_part = int_part * 10.0 + d;
        } else {
            frac_div = frac_div * 10.0;
            frac_part = frac_part + d / frac_div;
        }
        i += 1;
    }
    if (!has_digit) return ToolConfigError.InvalidNumber;
    const total = int_part + frac_part;
    return if (neg) -total else total;
}

fn parseStringExpanded(
    raw_token: []const u8,
    env_vars: []const ToolEnvVar,
    keep_unresolved: bool,
    storage: *ToolConfigStorage,
): ([]const u8)! {
    let tmp1: [1024]u8 = undefined;
    let tmp2: [1024]u8 = undefined;
    const unesc_len = unescapeTomlString(raw_token, tmp1[0..])?;
    const exp_len = expandEnvVars(tmp1[0..unesc_len], env_vars, keep_unresolved, tmp2[0..])?;
    return storage.allocCopy(tmp2[0..exp_len]);
}

fn parseLogLevelStr(s: []const u8): ToolConfigLogLevel! {
    if (bytesEqual(s, "debug")) return ToolConfigLogLevel.Debug;
    if (bytesEqual(s, "info")) return ToolConfigLogLevel.Info;
    if (bytesEqual(s, "warn")) return ToolConfigLogLevel.Warn;
    if (bytesEqual(s, "error")) return ToolConfigLogLevel.Error;
    if (bytesEqual(s, "silent")) return ToolConfigLogLevel.Silent;
    return ToolConfigError.InvalidLogLevel;
}

fn parseLinkerStr(s: []const u8): ToolConfigLinker! {
    if (bytesEqual(s, "hoisted")) return ToolConfigLinker.Hoisted;
    if (bytesEqual(s, "isolated")) return ToolConfigLinker.Isolated;
    return ToolConfigError.InvalidLinkerMode;
}

fn parseShellStr(s: []const u8): ToolConfigShell! {
    if (bytesEqual(s, "system")) return ToolConfigShell.System;
    if (bytesEqual(s, "cap") || bytesEqual(s, "bun")) return ToolConfigShell.Cap;
    return ToolConfigError.InvalidShellMode;
}

fn parseAutoInstallStr(s: []const u8): ToolConfigAutoInstall! {
    if (bytesEqual(s, "auto")) return ToolConfigAutoInstall.Auto;
    if (bytesEqual(s, "force")) return ToolConfigAutoInstall.Force;
    if (bytesEqual(s, "disable")) return ToolConfigAutoInstall.Disable;
    if (bytesEqual(s, "fallback")) return ToolConfigAutoInstall.Fallback;
    return ToolConfigError.InvalidAutoInstallMode;
}

/// Validates that a registry URL begins with `http://` or `https://` and has a non-empty host.
pub fn isValidRegistryUrl(url: []const u8): bool {
    let start: usize = 0;
    if (startsWith(url, "https://")) {
        start = 8;
    } else if (startsWith(url, "http://")) {
        start = 7;
    } else {
        return false;
    }
    if (start >= url.len) return false;
    const rest = url[start..];
    if (rest.len == 0) return false;
    if (rest[0] == '/' || rest[0] == ' ') return false;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == ' ' || rest[i] == '\r' || rest[i] == '\n' || rest[i] == '\t') {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses either a registry URL string (supporting `https://user:pass@host/` and
/// `https://:token@host/`) or an inline TOML table `{ url = "...", token = "..." }`.
fn parseRegistryValue(
    raw_val: []const u8,
    scope: []const u8,
    layer: ToolConfigLayer,
    env_vars: []const ToolEnvVar,
    keep_unresolved: bool,
    storage: *ToolConfigStorage,
): ToolScopeRegistry! {
    const s = trimWs(raw_val);
    let reg = ToolScopeRegistry.empty();
    reg.scope = scope;
    reg.layer = layer;

    if (s.len >= 2 && (s[0] == '"' || s[0] == '\'')) {
        const expanded = parseStringExpanded(s, env_vars, keep_unresolved, storage)?;
        // Check for embedded credentials in URL: `scheme://[user]:pass@host...`
        let scheme_len: usize = 0;
        if (startsWith(expanded, "https://")) {
            scheme_len = 8;
        } else if (startsWith(expanded, "http://")) {
            scheme_len = 7;
        } else {
            return ToolConfigError.InvalidRegistryUrl;
        }
        const after_scheme = expanded[scheme_len..];
        const slash_opt = indexOfByte(after_scheme, '/');
        const authority = if (slash_opt != null) after_scheme[0..slash_opt.?] else after_scheme;
        if (at_idx := indexOfByte(authority, '@')) {
            const userinfo = authority[0..at_idx];
            const host_and_path = after_scheme[at_idx + 1 ..];
            if (colon_idx := indexOfByte(userinfo, ':')) {
                const u = userinfo[0..colon_idx];
                const p = userinfo[colon_idx + 1 ..];
                if (u.len == 0 && p.len > 0) {
                    reg.token = p;
                } else {
                    reg.username = u;
                    reg.password = p;
                }
            } else if (userinfo.len > 0) {
                reg.token = userinfo;
            }
            // Reconstruct sanitized URL without embedded userinfo
            let url_buf: [512]u8 = undefined;
            let upos: usize = 0;
            appendBytes(url_buf[0..], &upos, expanded[0..scheme_len])?;
            appendBytes(url_buf[0..], &upos, host_and_path)?;
            reg.url = storage.allocCopy(url_buf[0..upos])?;
        } else {
            reg.url = expanded;
        }
        return reg;
    }

    if (s.len >= 2 && s[0] == '{') {
        if (s[s.len - 1] != '}') {
            return ToolConfigError.UnterminatedInlineTable;
        }
        const body = trimWs(s[1 .. s.len - 1]);
        let pos: usize = 0;
        while (pos < body.len) {
            // Skip whitespace and commas
            while (pos < body.len && (body[pos] == ' ' || body[pos] == '\t' || body[pos] == ',')) {
                pos += 1;
            }
            if (pos >= body.len) break;

            // Find '='
            let eq_idx = pos;
            while (eq_idx < body.len && body[eq_idx] != '=') {
                eq_idx += 1;
            }
            if (eq_idx >= body.len) return ToolConfigError.InvalidKeyValueSyntax;
            const k = trimWs(body[pos..eq_idx]);
            pos = eq_idx + 1;
            while (pos < body.len && (body[pos] == ' ' || body[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= body.len) return ToolConfigError.InvalidKeyValueSyntax;

            // Parse value token (quoted string or bare token until comma)
            const val_start = pos;
            if (body[pos] == '"' || body[pos] == '\'') {
                const q = body[pos];
                pos += 1;
                let esc = false;
                while (pos < body.len) {
                    if (q == '"' && esc) {
                        esc = false;
                    } else if (q == '"' && body[pos] == '\\') {
                        esc = true;
                    } else if (body[pos] == q) {
                        pos += 1;
                        break;
                    }
                    pos += 1;
                }
            } else {
                while (pos < body.len && body[pos] != ',') {
                    pos += 1;
                }
            }
            const raw_v = trimWs(body[val_start..pos]);
            const v_str = parseStringExpanded(raw_v, env_vars, keep_unresolved, storage)?;

            if (bytesEqual(k, "url")) {
                reg.url = v_str;
            } else if (bytesEqual(k, "token")) {
                reg.token = v_str;
            } else if (bytesEqual(k, "username")) {
                reg.username = v_str;
            } else if (bytesEqual(k, "password")) {
                reg.password = v_str;
            } else if (bytesEqual(k, "email")) {
                reg.email = v_str;
            } else {
                return ToolConfigError.UnknownKey;
            }
        }
        return reg;
    }

    return ToolConfigError.InvalidRegistryUrl;
}

/// Parses a TOML string array `["a", "b"]` or single string `"a"` into `out_entries`.
fn parseStringArrayInto(
    raw_val: []const u8,
    group_key: []const u8,
    layer: ToolConfigLayer,
    env_vars: []const ToolEnvVar,
    keep_unresolved: bool,
    storage: *ToolConfigStorage,
    entries_len: *usize,
)! {
    const s = trimWs(raw_val);
    if (s.len >= 2 && (s[0] == '"' || s[0] == '\'')) {
        const v = parseStringExpanded(s, env_vars, keep_unresolved, storage)?;
        if (entries_len.* >= storage.preloads.len) return ToolConfigError.SliceCapacityExceeded;
        storage.preloads[entries_len.*] = ToolKeyValueEntry{ key: group_key, value: v, layer: layer };
        entries_len.* += 1;
        return;
    }
    if (s.len < 2 || s[0] != '[' || s[s.len - 1] != ']') {
        return ToolConfigError.UnterminatedArray;
    }
    const body = trimWs(s[1 .. s.len - 1]);
    let pos: usize = 0;
    while (pos < body.len) {
        while (pos < body.len && (body[pos] == ' ' || body[pos] == '\t' || body[pos] == ',')) {
            pos += 1;
        }
        if (pos >= body.len) break;
        const start = pos;
        if (body[pos] != '"' && body[pos] != '\'') {
            return ToolConfigError.UnterminatedString;
        }
        const q = body[pos];
        pos += 1;
        let esc = false;
        let closed = false;
        while (pos < body.len) {
            if (q == '"' && esc) {
                esc = false;
            } else if (q == '"' && body[pos] == '\\') {
                esc = true;
            } else if (body[pos] == q) {
                pos += 1;
                closed = true;
                break;
            }
            pos += 1;
        }
        if (!closed) return ToolConfigError.UnterminatedString;
        const v = parseStringExpanded(body[start..pos], env_vars, keep_unresolved, storage)?;
        if (entries_len.* >= storage.preloads.len) return ToolConfigError.SliceCapacityExceeded;
        storage.preloads[entries_len.*] = ToolKeyValueEntry{ key: group_key, value: v, layer: layer };
        entries_len.* += 1;
    }
}

fn pushDiag(
    storage: *ToolConfigStorage,
    diags_len: *usize,
    sev: ToolConfigDiagSeverity,
    line_no: u32,
    section: []const u8,
    key: []const u8,
    msg: []const u8,
) {
    if (diags_len.* < storage.diags.len) {
        storage.diags[diags_len.*] = ToolConfigDiagnostic{
            severity: sev,
            line_no: line_no,
            section: section,
            key: key,
            message: msg,
        };
        diags_len.* += 1;
    }
}

fn findEqualsOutsideQuotes(line: []const u8): usize? {
    let in_double = false;
    let in_single = false;
    let i: usize = 0;
    while (i < line.len) {
        const c = line[i];
        if (in_double) {
            if (c == '"' && (i == 0 || line[i - 1] != '\\')) in_double = false;
        } else if (in_single) {
            if (c == '\'') in_single = false;
        } else {
            if (c == '"') in_double = true;
            else if (c == '\'') in_single = true;
            else if (c == '=') return i;
        }
        i += 1;
    }
    return null;
}

fn unquoteKey(raw_key: []const u8, storage: *ToolConfigStorage): ([]const u8)! {
    const k = trimWs(raw_key);
    if (k.len >= 2 && (k[0] == '"' || k[0] == '\'')) {
        let tmp: [256]u8 = undefined;
        const n = unescapeTomlString(k, tmp[0..])?;
        return storage.allocCopy(tmp[0..n]);
    }
    return k;
}

fn hasDuplicateEntry(entries: []const ToolKeyValueEntry, len: usize, key: []const u8): bool {
    let i: usize = 0;
    while (i < len) {
        if (bytesEqual(entries[i].key, key)) return true;
        i += 1;
    }
    return false;
}

fn hasDuplicateScope(scopes: []const ToolScopeRegistry, len: usize, scope: []const u8): bool {
    let i: usize = 0;
    while (i < len) {
        if (bytesEqual(scopes[i].scope, scope)) return true;
        i += 1;
    }
    return false;
}

// ============================================================================
// Core Multi-Section TOML Parser (`parseToolConfig`)
// ============================================================================

/// Parses a `capfig.toml` / `bunfig.toml` document into a `ToolConfig` using caller-owned
/// zero-heap slices in `storage`. Supports all major toolchain sections, inline tables,
/// arrays, environment variable interpolation, and strict/lenient unknown key handling.
pub fn parseToolConfig(
    toml_text: []const u8,
    env_vars: []const ToolEnvVar,
    opts: ToolConfigParseOptions,
    storage: *ToolConfigStorage,
): ToolConfig! {
    if (trimWs(toml_text).len == 0) {
        return ToolConfigError.EmptyInput;
    }

    let cfg = ToolConfig.empty(opts.layer);
    let cur_section: []const u8 = "";
    let pos: usize = 0;
    let line_no: u32 = 0;

    while (pos < toml_text.len) {
        const line_start = pos;
        while (pos < toml_text.len && toml_text[pos] != '\n') {
            pos += 1;
        }
        const raw_line = toml_text[line_start..pos];
        if (pos < toml_text.len && toml_text[pos] == '\n') {
            pos += 1;
        }
        line_no += 1;

        const line = stripTomlComment(raw_line);
        if (line.len == 0) continue;

        // Section header: `[install]`, `[install.cache]`, `[install.scopes]`, ...
        if (line[0] == '[') {
            if (line.len < 3 || line[line.len - 1] != ']') {
                return ToolConfigError.InvalidSectionHeader;
            }
            const sec = trimWs(line[1 .. line.len - 1]);
            if (sec.len == 0) {
                return ToolConfigError.InvalidSectionHeader;
            }
            const is_known =
                bytesEqual(sec, "install") ||
                bytesEqual(sec, "install.cache") ||
                bytesEqual(sec, "install.lockfile") ||
                bytesEqual(sec, "install.scopes") ||
                bytesEqual(sec, "install.security") ||
                bytesEqual(sec, "test") ||
                bytesEqual(sec, "run") ||
                bytesEqual(sec, "define") ||
                bytesEqual(sec, "loader") ||
                bytesEqual(sec, "debug") ||
                bytesEqual(sec, "console") ||
                bytesEqual(sec, "serve");
            if (!is_known) {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, sec, "", "unknown config section");
                if (opts.strict_unknown) {
                    return ToolConfigError.UnknownSection;
                }
            }
            cur_section = sec;
            continue;
        }

        // Key = Value
        const eq_opt = findEqualsOutsideQuotes(line);
        if (eq_opt == null) {
            return ToolConfigError.InvalidKeyValueSyntax;
        }
        const raw_key = trimWs(line[0..eq_opt.?]);
        const raw_val = trimWs(line[eq_opt.? + 1 ..]);
        if (raw_key.len == 0 || raw_val.len == 0) {
            return ToolConfigError.InvalidKeyValueSyntax;
        }
        const key = unquoteKey(raw_key, storage)?;

        // Dispatch by active section
        if (cur_section.len == 0) {
            // Top-level keys
            if (bytesEqual(key, "logLevel")) {
                const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
                cfg.log_level = parseLogLevelStr(v)?;
            } else if (bytesEqual(key, "smol")) {
                cfg.smol = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "telemetry")) {
                cfg.telemetry = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "env")) {
                const enabled = parseBoolVal(raw_val)?;
                cfg.env_disabled = !enabled;
            } else if (bytesEqual(key, "origin")) {
                cfg.origin = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "preload")) {
                parseStringArrayInto(raw_val, "preload", opts.layer, env_vars, opts.keep_unresolved_env, storage, &cfg.preloads_len)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown top-level key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "install")) {
            if (bytesEqual(key, "registry")) {
                const reg = parseRegistryValue(raw_val, "", opts.layer, env_vars, opts.keep_unresolved_env, storage)?;
                if (reg.url.len > 0) cfg.install.registry_url = reg.url;
                if (reg.token.len > 0) cfg.install.registry_token = reg.token;
                if (reg.username.len > 0) cfg.install.registry_username = reg.username;
                if (reg.password.len > 0) cfg.install.registry_password = reg.password;
                if (reg.email.len > 0) cfg.install.registry_email = reg.email;
            } else if (bytesEqual(key, "auto")) {
                const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
                cfg.install.auto_install = parseAutoInstallStr(v)?;
            } else if (bytesEqual(key, "linker")) {
                const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
                cfg.install.linker = parseLinkerStr(v)?;
            } else if (bytesEqual(key, "optional")) {
                cfg.install.optional = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "dev")) {
                cfg.install.dev = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "peer")) {
                cfg.install.peer = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "production")) {
                cfg.install.production = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "exact")) {
                cfg.install.exact = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "dryRun")) {
                cfg.install.dry_run = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "frozenLockfile")) {
                cfg.install.frozen_lockfile = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "saveTextLockfile")) {
                cfg.install.save_text_lockfile = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "ignoreScripts")) {
                cfg.install.ignore_scripts = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "linkWorkspacePackages")) {
                cfg.install.link_workspace_packages = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "globalStore")) {
                cfg.install.global_store = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "concurrentScripts")) {
                cfg.install.concurrent_scripts = parseU32Val(raw_val)?;
            } else if (bytesEqual(key, "minimumReleaseAge")) {
                cfg.install.minimum_release_age_secs = parseU64Val(raw_val)?;
            } else if (bytesEqual(key, "globalDir")) {
                cfg.install.global_dir = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "globalBinDir")) {
                cfg.install.global_bin_dir = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "cafile")) {
                cfg.install.cafile = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "logLevel")) {
                const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
                cfg.install.log_level = parseLogLevelStr(v)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [install] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "install.cache")) {
            if (bytesEqual(key, "disable")) {
                cfg.install.cache_disable = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "disableManifest")) {
                cfg.install.cache_disable_manifest = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "dir")) {
                cfg.install.cache_dir = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [install.cache] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "install.lockfile")) {
            if (bytesEqual(key, "save")) {
                cfg.install.lockfile_save = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "print")) {
                cfg.install.lockfile_print = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "path") || bytesEqual(key, "savePath")) {
                cfg.install.lockfile_path = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [install.lockfile] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "install.security")) {
            if (bytesEqual(key, "scanner")) {
                cfg.install.security_scanner = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [install.security] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "install.scopes")) {
            // Normalize scope name to always start with '@'
            let norm_scope = key;
            if (key.len > 0 && key[0] != '@') {
                let sbuf: [128]u8 = undefined;
                let spos: usize = 0;
                appendBytes(sbuf[0..], &spos, "@")?;
                appendBytes(sbuf[0..], &spos, key)?;
                norm_scope = storage.allocCopy(sbuf[0..spos])?;
            }
            if (opts.reject_duplicate_keys && hasDuplicateScope(storage.scopes, cfg.scopes_len, norm_scope)) {
                return ToolConfigError.DuplicateKey;
            }
            if (cfg.scopes_len >= storage.scopes.len) {
                return ToolConfigError.SliceCapacityExceeded;
            }
            const reg = parseRegistryValue(raw_val, norm_scope, opts.layer, env_vars, opts.keep_unresolved_env, storage)?;
            storage.scopes[cfg.scopes_len] = reg;
            cfg.scopes_len += 1;
        } else if (bytesEqual(cur_section, "test")) {
            if (bytesEqual(key, "root")) {
                cfg.test_cfg.root = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "smol")) {
                cfg.test_cfg.smol = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "coverage")) {
                cfg.test_cfg.coverage = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "onlyFailures")) {
                cfg.test_cfg.only_failures = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "randomize")) {
                cfg.test_cfg.randomize = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "coverageDir")) {
                cfg.test_cfg.coverage_dir = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else if (bytesEqual(key, "coverageIgnoreSourcemaps")) {
                cfg.test_cfg.coverage_ignore_sourcemaps = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "coverageSkipTestFiles")) {
                cfg.test_cfg.coverage_skip_test_files = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "coverageThreshold")) {
                const th = parseF64Val(raw_val)?;
                cfg.test_cfg.coverage_threshold_lines = th;
                cfg.test_cfg.coverage_threshold_functions = th;
                cfg.test_cfg.coverage_threshold_statements = th;
            } else if (bytesEqual(key, "timeout")) {
                cfg.test_cfg.timeout_ms = parseU32Val(raw_val)?;
            } else if (bytesEqual(key, "seed")) {
                cfg.test_cfg.seed = parseU32Val(raw_val)?;
            } else if (bytesEqual(key, "rerunEach")) {
                cfg.test_cfg.rerun_each = parseU32Val(raw_val)?;
            } else if (bytesEqual(key, "retry")) {
                cfg.test_cfg.retry = parseU32Val(raw_val)?;
            } else if (bytesEqual(key, "preload")) {
                parseStringArrayInto(raw_val, "test.preload", opts.layer, env_vars, opts.keep_unresolved_env, storage, &cfg.preloads_len)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [test] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "run")) {
            if (bytesEqual(key, "silent")) {
                cfg.run.silent = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "elide-lines") || bytesEqual(key, "elideLines")) {
                cfg.run.elide_lines = parseU32Val(raw_val)?;
            } else if (bytesEqual(key, "shell")) {
                const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
                cfg.run.shell = parseShellStr(v)?;
            } else if (bytesEqual(key, "bun") || bytesEqual(key, "cap")) {
                cfg.run.cap_alias = parseBoolVal(raw_val)?;
            } else if (bytesEqual(key, "noOrphans")) {
                cfg.run.no_orphans = parseBoolVal(raw_val)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [run] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "define")) {
            if (opts.reject_duplicate_keys && hasDuplicateEntry(storage.defines, cfg.defines_len, key)) {
                return ToolConfigError.DuplicateKey;
            }
            if (cfg.defines_len >= storage.defines.len) {
                return ToolConfigError.SliceCapacityExceeded;
            }
            const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            storage.defines[cfg.defines_len] = ToolKeyValueEntry{ key: key, value: v, layer: opts.layer };
            cfg.defines_len += 1;
        } else if (bytesEqual(cur_section, "loader")) {
            if (opts.reject_duplicate_keys && hasDuplicateEntry(storage.loaders, cfg.loaders_len, key)) {
                return ToolConfigError.DuplicateKey;
            }
            if (cfg.loaders_len >= storage.loaders.len) {
                return ToolConfigError.SliceCapacityExceeded;
            }
            const v = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            storage.loaders[cfg.loaders_len] = ToolKeyValueEntry{ key: key, value: v, layer: opts.layer };
            cfg.loaders_len += 1;
        } else if (bytesEqual(cur_section, "debug")) {
            if (bytesEqual(key, "editor")) {
                cfg.debug.editor = parseStringExpanded(raw_val, env_vars, opts.keep_unresolved_env, storage)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [debug] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "console")) {
            if (bytesEqual(key, "depth")) {
                cfg.debug.console_depth = parseU32Val(raw_val)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [console] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        } else if (bytesEqual(cur_section, "serve")) {
            if (bytesEqual(key, "port")) {
                cfg.debug.serve_port = parseU16Val(raw_val)?;
            } else if (bytesEqual(key, "hmr")) {
                cfg.debug.serve_hmr = parseBoolVal(raw_val)?;
            } else {
                pushDiag(storage, &cfg.diags_len, ToolConfigDiagSeverity.Warning, line_no, cur_section, key, "unknown [serve] key");
                if (opts.strict_unknown) return ToolConfigError.UnknownKey;
            }
        }
    }

    return cfg;
}

// ============================================================================
// Multi-Layer Priority Merger (`System < User < Project < CliOverride`)
// ============================================================================

/// Merges `overlay` onto `base` with field-level precision:
/// any non-null field in `overlay` (including explicit `false` or `0`) overrides `base`,
/// while `null` fields in `overlay` preserve `base`.
pub fn mergeToolConfig(base: ToolConfig, overlay: ToolConfig): ToolConfig {
    const out_layer = if (@intFromEnum(overlay.layer) >= @intFromEnum(base.layer)) overlay.layer else base.layer;
    return ToolConfig{
        layer: out_layer,
        log_level: overlay.log_level ?? base.log_level,
        smol: overlay.smol ?? base.smol,
        telemetry: overlay.telemetry ?? base.telemetry,
        env_disabled: overlay.env_disabled ?? base.env_disabled,
        origin: overlay.origin ?? base.origin,
        install: ToolInstallConfig{
            registry_url: overlay.install.registry_url ?? base.install.registry_url,
            registry_token: overlay.install.registry_token ?? base.install.registry_token,
            registry_username: overlay.install.registry_username ?? base.install.registry_username,
            registry_password: overlay.install.registry_password ?? base.install.registry_password,
            registry_email: overlay.install.registry_email ?? base.install.registry_email,
            auto_install: overlay.install.auto_install ?? base.install.auto_install,
            linker: overlay.install.linker ?? base.install.linker,
            optional: overlay.install.optional ?? base.install.optional,
            dev: overlay.install.dev ?? base.install.dev,
            peer: overlay.install.peer ?? base.install.peer,
            production: overlay.install.production ?? base.install.production,
            exact: overlay.install.exact ?? base.install.exact,
            dry_run: overlay.install.dry_run ?? base.install.dry_run,
            frozen_lockfile: overlay.install.frozen_lockfile ?? base.install.frozen_lockfile,
            save_text_lockfile: overlay.install.save_text_lockfile ?? base.install.save_text_lockfile,
            ignore_scripts: overlay.install.ignore_scripts ?? base.install.ignore_scripts,
            link_workspace_packages: overlay.install.link_workspace_packages ?? base.install.link_workspace_packages,
            global_store: overlay.install.global_store ?? base.install.global_store,
            concurrent_scripts: overlay.install.concurrent_scripts ?? base.install.concurrent_scripts,
            minimum_release_age_secs: overlay.install.minimum_release_age_secs ?? base.install.minimum_release_age_secs,
            global_dir: overlay.install.global_dir ?? base.install.global_dir,
            global_bin_dir: overlay.install.global_bin_dir ?? base.install.global_bin_dir,
            cafile: overlay.install.cafile ?? base.install.cafile,
            log_level: overlay.install.log_level ?? base.install.log_level,
            cache_disable: overlay.install.cache_disable ?? base.install.cache_disable,
            cache_disable_manifest: overlay.install.cache_disable_manifest ?? base.install.cache_disable_manifest,
            cache_dir: overlay.install.cache_dir ?? base.install.cache_dir,
            lockfile_save: overlay.install.lockfile_save ?? base.install.lockfile_save,
            lockfile_print: overlay.install.lockfile_print ?? base.install.lockfile_print,
            lockfile_path: overlay.install.lockfile_path ?? base.install.lockfile_path,
            security_scanner: overlay.install.security_scanner ?? base.install.security_scanner,
        },
        test_cfg: ToolTestConfig{
            root: overlay.test_cfg.root ?? base.test_cfg.root,
            smol: overlay.test_cfg.smol ?? base.test_cfg.smol,
            coverage: overlay.test_cfg.coverage ?? base.test_cfg.coverage,
            only_failures: overlay.test_cfg.only_failures ?? base.test_cfg.only_failures,
            randomize: overlay.test_cfg.randomize ?? base.test_cfg.randomize,
            coverage_dir: overlay.test_cfg.coverage_dir ?? base.test_cfg.coverage_dir,
            coverage_ignore_sourcemaps: overlay.test_cfg.coverage_ignore_sourcemaps ?? base.test_cfg.coverage_ignore_sourcemaps,
            coverage_skip_test_files: overlay.test_cfg.coverage_skip_test_files ?? base.test_cfg.coverage_skip_test_files,
            coverage_threshold_lines: overlay.test_cfg.coverage_threshold_lines ?? base.test_cfg.coverage_threshold_lines,
            coverage_threshold_functions: overlay.test_cfg.coverage_threshold_functions ?? base.test_cfg.coverage_threshold_functions,
            coverage_threshold_statements: overlay.test_cfg.coverage_threshold_statements ?? base.test_cfg.coverage_threshold_statements,
            timeout_ms: overlay.test_cfg.timeout_ms ?? base.test_cfg.timeout_ms,
            seed: overlay.test_cfg.seed ?? base.test_cfg.seed,
            rerun_each: overlay.test_cfg.rerun_each ?? base.test_cfg.rerun_each,
            retry: overlay.test_cfg.retry ?? base.test_cfg.retry,
            reporter_dots: overlay.test_cfg.reporter_dots ?? base.test_cfg.reporter_dots,
            reporter_junit: overlay.test_cfg.reporter_junit ?? base.test_cfg.reporter_junit,
        },
        run: ToolRunConfig{
            silent: overlay.run.silent ?? base.run.silent,
            elide_lines: overlay.run.elide_lines ?? base.run.elide_lines,
            shell: overlay.run.shell ?? base.run.shell,
            cap_alias: overlay.run.cap_alias ?? base.run.cap_alias,
            no_orphans: overlay.run.no_orphans ?? base.run.no_orphans,
        },
        debug: ToolDebugConfig{
            editor: overlay.debug.editor ?? base.debug.editor,
            console_depth: overlay.debug.console_depth ?? base.debug.console_depth,
            serve_port: overlay.debug.serve_port ?? base.debug.serve_port,
            serve_hmr: overlay.debug.serve_hmr ?? base.debug.serve_hmr,
            minify_syntax: overlay.debug.minify_syntax ?? base.debug.minify_syntax,
            minify_whitespace: overlay.debug.minify_whitespace ?? base.debug.minify_whitespace,
            minify_identifiers: overlay.debug.minify_identifiers ?? base.debug.minify_identifiers,
        },
        scopes_len: base.scopes_len,
        defines_len: base.defines_len,
        loaders_len: base.loaders_len,
        preloads_len: base.preloads_len,
        diags_len: base.diags_len,
    };
}

/// Merges scoped registry tables across layers: matching scopes in `overlay_scopes`
/// replace entries in `base_scopes`, preserving non-overlapping scopes.
pub fn mergeScopeRegistries(
    base_scopes: []const ToolScopeRegistry,
    overlay_scopes: []const ToolScopeRegistry,
    out_scopes: []ToolScopeRegistry,
): usize! {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < base_scopes.len) {
        if (out_len >= out_scopes.len) return ToolConfigError.SliceCapacityExceeded;
        out_scopes[out_len] = base_scopes[i];
        out_len += 1;
        i += 1;
    }

    let j: usize = 0;
    while (j < overlay_scopes.len) {
        const ov = overlay_scopes[j];
        let replaced = false;
        let k: usize = 0;
        while (k < out_len) {
            if (bytesEqual(out_scopes[k].scope, ov.scope)) {
                out_scopes[k] = ov;
                replaced = true;
                break;
            }
            k += 1;
        }
        if (!replaced) {
            if (out_len >= out_scopes.len) return ToolConfigError.SliceCapacityExceeded;
            out_scopes[out_len] = ov;
            out_len += 1;
        }
        j += 1;
    }
    return out_len;
}

/// Merges key/value slices (`[define]` / `[loader]`) across layers.
pub fn mergeKeyValueEntries(
    base_entries: []const ToolKeyValueEntry,
    overlay_entries: []const ToolKeyValueEntry,
    out_entries: []ToolKeyValueEntry,
): usize! {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < base_entries.len) {
        if (out_len >= out_entries.len) return ToolConfigError.SliceCapacityExceeded;
        out_entries[out_len] = base_entries[i];
        out_len += 1;
        i += 1;
    }
    let j: usize = 0;
    while (j < overlay_entries.len) {
        const ov = overlay_entries[j];
        let replaced = false;
        let k: usize = 0;
        while (k < out_len) {
            if (bytesEqual(out_entries[k].key, ov.key)) {
                out_entries[k] = ov;
                replaced = true;
                break;
            }
            k += 1;
        }
        if (!replaced) {
            if (out_len >= out_entries.len) return ToolConfigError.SliceCapacityExceeded;
            out_entries[out_len] = ov;
            out_len += 1;
        }
        j += 1;
    }
    return out_len;
}

// ============================================================================
// Scoped Registry Routing & Secret Token Redaction
// ============================================================================

/// Resolves the target registry and credentials for `package_name`.
/// Matches the longest applicable `@scope` prefix in `scopes`, falling back to
/// `[install].registry` or `TOOL_CONFIG_DEFAULT_REGISTRY`.
pub fn resolveScopedRegistry(
    cfg: ToolConfig,
    scopes: []const ToolScopeRegistry,
    package_name: []const u8,
): ResolvedRegistryRoute {
    let best_idx: usize? = null;
    let best_len: usize = 0;

    if (package_name.len > 0 && package_name[0] == '@') {
        let i: usize = 0;
        while (i < scopes.len) {
            const sc = scopes[i].scope;
            if (sc.len > 0 && startsWith(package_name, sc)) {
                const exact_or_boundary =
                    (package_name.len == sc.len) ||
                    (package_name[sc.len] == '/');
                if (exact_or_boundary && sc.len > best_len) {
                    best_len = sc.len;
                    best_idx = i;
                }
            }
            i += 1;
        }
    }

    if (idx := best_idx) {
        const sc_reg = scopes[idx];
        const fallback_url = cfg.install.registry_url ?? TOOL_CONFIG_DEFAULT_REGISTRY;
        return ResolvedRegistryRoute{
            url: if (sc_reg.url.len > 0) sc_reg.url else fallback_url,
            token: sc_reg.token,
            username: sc_reg.username,
            password: sc_reg.password,
            email: sc_reg.email,
            matched_scope: sc_reg.scope,
            is_scoped: true,
        };
    }

    return ResolvedRegistryRoute{
        url: cfg.install.registry_url ?? TOOL_CONFIG_DEFAULT_REGISTRY,
        token: cfg.install.registry_token ?? "",
        username: cfg.install.registry_username ?? "",
        password: cfg.install.registry_password ?? "",
        email: cfg.install.registry_email ?? "",
        matched_scope: "",
        is_scoped: false,
    };
}

/// Redacts a secret bearer token for safe logging:
/// tokens with length > 8 keep prefix(4) + "***" + suffix(4); shorter non-empty tokens become "***".
pub fn redactSecretToken(token: []const u8, out_buf: []u8): usize! {
    if (token.len == 0) return 0;
    let pos: usize = 0;
    if (token.len <= 8) {
        appendBytes(out_buf, &pos, "***")?;
        return pos;
    }
    appendBytes(out_buf, &pos, token[0..4])?;
    appendBytes(out_buf, &pos, "***")?;
    appendBytes(out_buf, &pos, token[token.len - 4 ..])?;
    return pos;
}

/// Formats a diagnostic-safe registry route summary with automatic secret redaction.
pub fn formatRedactedRegistryRoute(route: ResolvedRegistryRoute, out_buf: []u8): usize! {
    let pos: usize = 0;
    if (route.is_scoped) {
        appendBytes(out_buf, &pos, "scope=")?;
        appendBytes(out_buf, &pos, route.matched_scope)?;
        appendBytes(out_buf, &pos, " ")?;
    }
    appendBytes(out_buf, &pos, "url=")?;
    appendBytes(out_buf, &pos, route.url)?;
    if (route.token.len > 0) {
        appendBytes(out_buf, &pos, " token=")?;
        let red_buf: [64]u8 = undefined;
        const rlen = redactSecretToken(route.token, red_buf[0..])?;
        appendBytes(out_buf, &pos, red_buf[0..rlen])?;
    } else if (route.username.len > 0) {
        appendBytes(out_buf, &pos, " user=")?;
        appendBytes(out_buf, &pos, route.username)?;
        if (route.password.len > 0) {
            appendBytes(out_buf, &pos, " pass=***")?;
        }
    }
    return pos;
}

// ============================================================================
// Configuration Semantic Validator (`validateToolConfig`)
// ============================================================================

fn addValDiag(
    diags_out: []ToolConfigDiagnostic,
    count: *usize,
    sev: ToolConfigDiagSeverity,
    section: []const u8,
    key: []const u8,
    msg: []const u8,
) {
    if (count.* < diags_out.len) {
        diags_out[count.*] = ToolConfigDiagnostic{
            severity: sev,
            line_no: 0,
            section: section,
            key: key,
            message: msg,
        };
        count.* += 1;
    }
}

/// Validates semantic invariants across `cfg` and `scopes`, writing any issues into `diags_out`.
/// Returns the total number of validation diagnostics recorded (0 means fully valid).
pub fn validateToolConfig(
    cfg: ToolConfig,
    scopes: []const ToolScopeRegistry,
    diags_out: []ToolConfigDiagnostic,
): usize {
    let count: usize = 0;

    if (url := cfg.install.registry_url) {
        if (!isValidRegistryUrl(url)) {
            addValDiag(diags_out, &count, ToolConfigDiagSeverity.Error, "install", "registry", "registry URL must start with http:// or https://");
        }
    }

    if (jobs := cfg.install.concurrent_scripts) {
        if (jobs == 0 || jobs > TOOL_CONFIG_MAX_CONCURRENT_SCRIPTS) {
            addValDiag(diags_out, &count, ToolConfigDiagSeverity.Error, "install", "concurrentScripts", "concurrentScripts must be within 1..1024");
        }
    }

    if (timeout := cfg.test_cfg.timeout_ms) {
        if (timeout == 0) {
            addValDiag(diags_out, &count, ToolConfigDiagSeverity.Error, "test", "timeout", "test timeout must be greater than 0");
        }
    }

    if (th := cfg.test_cfg.coverage_threshold_lines) {
        if (th < 0.0 || th > 100.0) {
            addValDiag(diags_out, &count, ToolConfigDiagSeverity.Error, "test", "coverageThreshold", "coverage threshold must be in 0..100");
        }
    }

    let i: usize = 0;
    while (i < scopes.len) {
        const sc = scopes[i];
        if (sc.scope.len < 2 || sc.scope[0] != '@') {
            addValDiag(diags_out, &count, ToolConfigDiagSeverity.Error, "install.scopes", sc.scope, "scope name must start with '@'");
        }
        if (sc.url.len > 0 && !isValidRegistryUrl(sc.url)) {
            addValDiag(diags_out, &count, ToolConfigDiagSeverity.Error, "install.scopes", sc.scope, "scoped registry URL is invalid");
        }
        i += 1;
    }

    return count;
}
