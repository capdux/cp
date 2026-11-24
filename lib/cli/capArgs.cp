// Source: Absorbed from /data/dev/bun/src/cli/Arguments.cp | Synced: 2026-11-23T20:18:24Z
// ============================================================================
// lib/cli/capArgs.cp — Zero-Alloc Declarative CLI Router, Parser & Help Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven CLI argument parser for the `cap`
// toolchain: declarative `CliFlagSpec` (long/short/clustered/negated/counted/
// repeated/enum/int/uint/byte-size + default & env fallback), subcommand router
// with short aliases and `--` passthrough separation, Levenshtein spell-check
// suggestions ("did you mean ...?"), aligned help renderer, and Bash/Zsh
// completion script generator.

/// Maximum Levenshtein edit distance for spell-check suggestions.
pub const CAP_ARGS_MAX_SUGGEST_DIST: u32 = 3;

/// Number of built-in `cap` subcommands in the router table.
pub const CAP_SUBCOMMAND_COUNT: usize = 17;

/// CLI argument parsing, routing, and formatting error set.
pub err CapArgsError {
    UnknownSubcommand,
    UnknownFlag,
    MissingFlagValue,
    UnexpectedFlagValue,
    InvalidIntValue,
    InvalidUintValue,
    InvalidByteSize,
    InvalidEnumValue,
    MissingRequiredFlag,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Supported value kinds for declarative `CliFlagSpec`.
pub enum CliFlagValueKind: u8 {
    Bool = 0,
    Count = 1,
    String = 2,
    Int = 3,
    Uint = 4,
    ByteSize = 5,
    Enum = 6,
    StringList = 7,
}

/// Origin of a resolved flag value (`Cli > Env > Default > Unset`).
pub enum CliFlagValueSource: u8 {
    Unset = 0,
    Default = 1,
    Env = 2,
    Cli = 3,
}

/// Built-in `cap` toolchain subcommands.
pub enum CapSubcommand: u8 {
    None = 0,
    Run = 1,
    Build = 2,
    Test = 3,
    Install = 4,
    Add = 5,
    Remove = 6,
    Update = 7,
    Pack = 8,
    Publish = 9,
    Init = 10,
    Create = 11,
    Patch = 12,
    Check = 13,
    Fmt = 14,
    Exec = 15,
    Help = 16,
    Version = 17,
}

/// Target shell dialect for `formatShellCompletion`.
pub enum CapCompletionShell: u8 {
    Bash = 0,
    Zsh = 1,
}

/// Declarative flag specification.
pub struct CliFlagSpec {
    long_name: []const u8 = "",
    short_name: u8 = 0,
    kind: CliFlagValueKind = CliFlagValueKind.Bool,
    allow_negation: bool = false,
    required: bool = false,
    is_global: bool = false,
    default_val: ([]const u8)? = null,
    env_var: ([]const u8)? = null,
    allowed_values: []const u8 = "",
    value_placeholder: []const u8 = "",
    description: []const u8 = "",
}

/// Caller-provided environment variable entry for flag fallback.
pub struct CapEnvBinding {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// Resolved runtime value for a single `CliFlagSpec`.
pub struct CliParsedFlag {
    spec_index: usize = 0,
    source: CliFlagValueSource = CliFlagValueSource.Unset,
    bool_val: bool = false,
    count_val: u32 = 0,
    int_val: i64 = 0,
    uint_val: u64 = 0,
    str_val: []const u8 = "",
    list_start: usize = 0,
    list_len: usize = 0,

    static fn empty(): CliParsedFlag {
        return CliParsedFlag{};
    }

    fn isSet(): bool {
        return _.source != CliFlagValueSource.Unset;
    }
}

/// Structured parse error diagnostic with optional Levenshtein suggestion.
pub struct CapArgsDiagnostic {
    has_error: bool = false,
    code: CapArgsError = CapArgsError.UnknownFlag,
    offending_token: []const u8 = "",
    suggestion: []const u8 = "",
    has_suggestion: bool = false,

    static fn none(): CapArgsDiagnostic {
        return CapArgsDiagnostic{};
    }
}

/// Summary of a completed `parseCapArgs` invocation.
pub struct CapArgsResult {
    subcommand: CapSubcommand = CapSubcommand.None,
    subcommand_raw: []const u8 = "",
    positionals_len: usize = 0,
    passthrough_len: usize = 0,
    list_vals_len: usize = 0,
    help_requested: bool = false,
    version_requested: bool = false,
}

// ============================================================================
// Internal Byte, Buffer & Value Parsing Utilities (Zero-Alloc)
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

fn indexOfByte(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) return CapArgsError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendByte(out_buf: []u8, pos: *usize, ch: u8)! {
    if (pos.* >= out_buf.len) return CapArgsError.BufferTooSmall;
    out_buf[pos.*] = ch;
    pos.* += 1;
}

fn appendSpaces(out_buf: []u8, pos: *usize, count: usize)! {
    if (pos.* + count > out_buf.len) return CapArgsError.BufferTooSmall;
    let i: usize = 0;
    while (i < count) {
        out_buf[pos.* + i] = @as(u8, 32);
        i += 1;
    }
    pos.* += count;
}

fn lookupEnvVar(env_vars: []const CapEnvBinding, key: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < env_vars.len) {
        if (bytesEqual(env_vars[i].key, key)) {
            return env_vars[i].value;
        }
        i += 1;
    }
    return null;
}

/// Parses a signed 64-bit decimal integer (supports optional `+`/`-` and `_` separators).
pub fn parseCliInt(raw: []const u8): i64! {
    if (raw.len == 0) return CapArgsError.InvalidIntValue;
    let neg = false;
    let i: usize = 0;
    if (raw[0] == '-') {
        neg = true;
        i = 1;
    } else if (raw[0] == '+') {
        i = 1;
    }
    if (i >= raw.len) return CapArgsError.InvalidIntValue;
    let acc: u64 = 0;
    let has_digit = false;
    while (i < raw.len) {
        const c = raw[i];
        if (c == '_') {
            i += 1;
            continue;
        }
        if (c < '0' || c > '9') return CapArgsError.InvalidIntValue;
        has_digit = true;
        acc = acc * 10 + @as(u64, c - '0');
        if (acc > 9223372036854775807) return CapArgsError.InvalidIntValue;
        i += 1;
    }
    if (!has_digit) return CapArgsError.InvalidIntValue;
    const signed_val: i64 = @intCast(acc);
    return if (neg) -signed_val else signed_val;
}

/// Parses an unsigned 64-bit decimal integer.
pub fn parseCliUint(raw: []const u8): u64! {
    if (raw.len == 0) return CapArgsError.InvalidUintValue;
    let acc: u64 = 0;
    let has_digit = false;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == '_') {
            i += 1;
            continue;
        }
        if (c < '0' || c > '9') return CapArgsError.InvalidUintValue;
        has_digit = true;
        acc = acc * 10 + @as(u64, c - '0');
        i += 1;
    }
    if (!has_digit) return CapArgsError.InvalidUintValue;
    return acc;
}

/// Parses a human-friendly byte size such as `"512"`, `"64k"`, `"16MB"`, `"2g"`, `"1tib"`.
pub fn parseCliByteSize(raw: []const u8): u64! {
    if (raw.len == 0) return CapArgsError.InvalidByteSize;
    let num_end: usize = 0;
    while (num_end < raw.len && ((raw[num_end] >= '0' && raw[num_end] <= '9') || raw[num_end] == '_')) {
        num_end += 1;
    }
    if (num_end == 0) return CapArgsError.InvalidByteSize;
    const base = parseCliUint(raw[0..num_end]) catch return CapArgsError.InvalidByteSize;
    const suffix = raw[num_end..];
    if (suffix.len == 0 || bytesEqualIgnoreCase(suffix, "b")) {
        return base;
    }
    let mult: u64 = 1;
    if (bytesEqualIgnoreCase(suffix, "k") || bytesEqualIgnoreCase(suffix, "kb") || bytesEqualIgnoreCase(suffix, "kib")) {
        mult = 1024;
    } else if (bytesEqualIgnoreCase(suffix, "m") || bytesEqualIgnoreCase(suffix, "mb") || bytesEqualIgnoreCase(suffix, "mib")) {
        mult = 1048576;
    } else if (bytesEqualIgnoreCase(suffix, "g") || bytesEqualIgnoreCase(suffix, "gb") || bytesEqualIgnoreCase(suffix, "gib")) {
        mult = 1073741824;
    } else if (bytesEqualIgnoreCase(suffix, "t") || bytesEqualIgnoreCase(suffix, "tb") || bytesEqualIgnoreCase(suffix, "tib")) {
        mult = 1099511627776;
    } else {
        return CapArgsError.InvalidByteSize;
    }
    if (base > 18446744073709551615 / mult) return CapArgsError.InvalidByteSize;
    return base * mult;
}

/// Checks whether `val` appears in a comma-separated whitelist such as `"auto,fallback,force,disable"`.
pub fn isAllowedEnumValue(val: []const u8, csv_list: []const u8): bool {
    if (csv_list.len == 0) return true;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= csv_list.len) {
        if (i == csv_list.len || csv_list[i] == ',') {
            const item = csv_list[start..i];
            if (bytesEqual(val, item)) return true;
            start = i + 1;
        }
        i += 1;
    }
    return false;
}

fn parseBoolStr(s: []const u8): bool? {
    if (bytesEqualIgnoreCase(s, "true") || bytesEqual(s, "1") || bytesEqualIgnoreCase(s, "yes")) return true;
    if (bytesEqualIgnoreCase(s, "false") || bytesEqual(s, "0") || bytesEqualIgnoreCase(s, "no")) return false;
    return null;
}

// ============================================================================
// Subcommand Router Table & Canonical Metadata
// ============================================================================

/// Resolves a subcommand token (canonical name or short alias) to `CapSubcommand`.
pub fn resolveCapSubcommand(token: []const u8): CapSubcommand? {
    if (bytesEqual(token, "run") || bytesEqual(token, "r")) return CapSubcommand.Run;
    if (bytesEqual(token, "build") || bytesEqual(token, "b")) return CapSubcommand.Build;
    if (bytesEqual(token, "test") || bytesEqual(token, "t")) return CapSubcommand.Test;
    if (bytesEqual(token, "install") || bytesEqual(token, "i")) return CapSubcommand.Install;
    if (bytesEqual(token, "add") || bytesEqual(token, "a")) return CapSubcommand.Add;
    if (bytesEqual(token, "remove") || bytesEqual(token, "rm") || bytesEqual(token, "uninstall")) return CapSubcommand.Remove;
    if (bytesEqual(token, "update") || bytesEqual(token, "up")) return CapSubcommand.Update;
    if (bytesEqual(token, "pack")) return CapSubcommand.Pack;
    if (bytesEqual(token, "publish")) return CapSubcommand.Publish;
    if (bytesEqual(token, "init")) return CapSubcommand.Init;
    if (bytesEqual(token, "create") || bytesEqual(token, "c")) return CapSubcommand.Create;
    if (bytesEqual(token, "patch")) return CapSubcommand.Patch;
    if (bytesEqual(token, "check")) return CapSubcommand.Check;
    if (bytesEqual(token, "fmt")) return CapSubcommand.Fmt;
    if (bytesEqual(token, "exec") || bytesEqual(token, "x")) return CapSubcommand.Exec;
    if (bytesEqual(token, "help")) return CapSubcommand.Help;
    if (bytesEqual(token, "version")) return CapSubcommand.Version;
    return null;
}

/// Returns the canonical command name for a `CapSubcommand`.
pub fn capSubcommandName(cmd: CapSubcommand): []const u8 {
    if (cmd == CapSubcommand.Run) return "run";
    if (cmd == CapSubcommand.Build) return "build";
    if (cmd == CapSubcommand.Test) return "test";
    if (cmd == CapSubcommand.Install) return "install";
    if (cmd == CapSubcommand.Add) return "add";
    if (cmd == CapSubcommand.Remove) return "remove";
    if (cmd == CapSubcommand.Update) return "update";
    if (cmd == CapSubcommand.Pack) return "pack";
    if (cmd == CapSubcommand.Publish) return "publish";
    if (cmd == CapSubcommand.Init) return "init";
    if (cmd == CapSubcommand.Create) return "create";
    if (cmd == CapSubcommand.Patch) return "patch";
    if (cmd == CapSubcommand.Check) return "check";
    if (cmd == CapSubcommand.Fmt) return "fmt";
    if (cmd == CapSubcommand.Exec) return "exec";
    if (cmd == CapSubcommand.Help) return "help";
    if (cmd == CapSubcommand.Version) return "version";
    return "";
}

/// Returns a concise one-line description for help output.
pub fn capSubcommandSummary(cmd: CapSubcommand): []const u8 {
    if (cmd == CapSubcommand.Run) return "Execute a CP source file or package script";
    if (cmd == CapSubcommand.Build) return "Compile and bundle CP targets to native binaries";
    if (cmd == CapSubcommand.Test) return "Run the native CP test suite";
    if (cmd == CapSubcommand.Install) return "Install project dependencies from lockfile";
    if (cmd == CapSubcommand.Add) return "Add dependencies to cap.json";
    if (cmd == CapSubcommand.Remove) return "Remove dependencies from cap.json";
    if (cmd == CapSubcommand.Update) return "Update dependencies to latest compatible versions";
    if (cmd == CapSubcommand.Pack) return "Pack the current package into a .tgz tarball";
    if (cmd == CapSubcommand.Publish) return "Publish a package to the registry";
    if (cmd == CapSubcommand.Init) return "Initialize a new CP project in the current directory";
    if (cmd == CapSubcommand.Create) return "Scaffold a new project from a template";
    if (cmd == CapSubcommand.Patch) return "Prepare or commit a dependency patch";
    if (cmd == CapSubcommand.Check) return "Type-check and verify CP sources without emitting";
    if (cmd == CapSubcommand.Fmt) return "Format CP source files";
    if (cmd == CapSubcommand.Exec) return "Execute a package binary";
    if (cmd == CapSubcommand.Help) return "Display help information";
    if (cmd == CapSubcommand.Version) return "Print the cap toolchain version";
    return "";
}

// ============================================================================
// Zero-Alloc Levenshtein Edit Distance & Spell-Check Suggestions
// ============================================================================

/// Computes the Levenshtein edit distance between `a` and `b` using O(min(|b|, 128))
/// stack memory with zero heap allocation.
pub fn levenshteinDistance(a: []const u8, b: []const u8): u32 {
    if (a.len == 0) return @intCast(b.len);
    if (b.len == 0) return @intCast(a.len);
    const max_len: usize = 127;
    const alen = if (a.len > max_len) max_len else a.len;
    const blen = if (b.len > max_len) max_len else b.len;

    let prev: [128]u32 = [0; 128];
    let curr: [128]u32 = [0; 128];

    let j: usize = 0;
    while (j <= blen) {
        prev[j] = @intCast(j);
        j += 1;
    }

    let i: usize = 1;
    while (i <= alen) {
        curr[0] = @intCast(i);
        j = 1;
        while (j <= blen) {
            const cost: u32 = if (toLowerAscii(a[i - 1]) == toLowerAscii(b[j - 1])) 0 else 1;
            const del_c = prev[j] + 1;
            const ins_c = curr[j - 1] + 1;
            const sub_c = prev[j - 1] + cost;
            let best = if (del_c < ins_c) del_c else ins_c;
            if (sub_c < best) best = sub_c;
            curr[j] = best;
            j += 1;
        }
        j = 0;
        while (j <= blen) {
            prev[j] = curr[j];
            j += 1;
        }
        i += 1;
    }
    return prev[blen];
}

const ALL_SUBCOMMANDS: [17]CapSubcommand = [
    CapSubcommand.Run,
    CapSubcommand.Build,
    CapSubcommand.Test,
    CapSubcommand.Install,
    CapSubcommand.Add,
    CapSubcommand.Remove,
    CapSubcommand.Update,
    CapSubcommand.Pack,
    CapSubcommand.Publish,
    CapSubcommand.Init,
    CapSubcommand.Create,
    CapSubcommand.Patch,
    CapSubcommand.Check,
    CapSubcommand.Fmt,
    CapSubcommand.Exec,
    CapSubcommand.Help,
    CapSubcommand.Version,
];

/// Returns the closest canonical subcommand name within `CAP_ARGS_MAX_SUGGEST_DIST`.
pub fn suggestClosestSubcommand(input: []const u8): ([]const u8)? {
    let best_name: ([]const u8)? = null;
    let best_dist: u32 = CAP_ARGS_MAX_SUGGEST_DIST + 1;
    let i: usize = 0;
    while (i < ALL_SUBCOMMANDS.len) {
        const name = capSubcommandName(ALL_SUBCOMMANDS[i]);
        const d = levenshteinDistance(input, name);
        if (d < best_dist) {
            best_dist = d;
            best_name = name;
        }
        i += 1;
    }
    return best_name;
}

/// Returns the closest long flag name (without leading `--`) within `CAP_ARGS_MAX_SUGGEST_DIST`.
pub fn suggestClosestFlag(input_name: []const u8, specs: []const CliFlagSpec): ([]const u8)? {
    let best_name: ([]const u8)? = null;
    let best_dist: u32 = CAP_ARGS_MAX_SUGGEST_DIST + 1;
    let i: usize = 0;
    while (i < specs.len) {
        const name = specs[i].long_name;
        if (name.len > 0) {
            const d = levenshteinDistance(input_name, name);
            if (d < best_dist) {
                best_dist = d;
                best_name = name;
            }
        }
        i += 1;
    }
    return best_name;
}

/// Formats a developer-friendly diagnostic message including `"did you mean ...?"` when available.
pub fn formatCapArgsDiagnostic(diag: CapArgsDiagnostic, out_buf: []u8): usize! {
    let pos: usize = 0;
    if (diag.code == CapArgsError.UnknownSubcommand) {
        appendBytes(out_buf, &pos, "error: unknown subcommand '")?;
        appendBytes(out_buf, &pos, diag.offending_token)?;
        appendBytes(out_buf, &pos, "'")?;
        if (diag.has_suggestion) {
            appendBytes(out_buf, &pos, " — did you mean '")?;
            appendBytes(out_buf, &pos, diag.suggestion)?;
            appendBytes(out_buf, &pos, "'?")?;
        }
    } else if (diag.code == CapArgsError.UnknownFlag) {
        appendBytes(out_buf, &pos, "error: unknown flag '")?;
        appendBytes(out_buf, &pos, diag.offending_token)?;
        appendBytes(out_buf, &pos, "'")?;
        if (diag.has_suggestion) {
            appendBytes(out_buf, &pos, " — did you mean '--")?;
            appendBytes(out_buf, &pos, diag.suggestion)?;
            appendBytes(out_buf, &pos, "'?")?;
        }
    } else if (diag.code == CapArgsError.MissingFlagValue) {
        appendBytes(out_buf, &pos, "error: missing value for flag '")?;
        appendBytes(out_buf, &pos, diag.offending_token)?;
        appendBytes(out_buf, &pos, "'")?;
    } else if (diag.code == CapArgsError.MissingRequiredFlag) {
        appendBytes(out_buf, &pos, "error: missing required flag '--")?;
        appendBytes(out_buf, &pos, diag.offending_token)?;
        appendBytes(out_buf, &pos, "'")?;
    } else if (diag.code == CapArgsError.InvalidEnumValue) {
        appendBytes(out_buf, &pos, "error: invalid enum value for '")?;
        appendBytes(out_buf, &pos, diag.offending_token)?;
        appendBytes(out_buf, &pos, "'")?;
    } else {
        appendBytes(out_buf, &pos, "error: invalid argument '")?;
        appendBytes(out_buf, &pos, diag.offending_token)?;
        appendBytes(out_buf, &pos, "'")?;
    }
    return pos;
}

// ============================================================================
// Core Declarative Argument Parser & Subcommand Router (`parseCapArgs`)
// ============================================================================

fn findFlagByLong(specs: []const CliFlagSpec, name: []const u8): usize? {
    let i: usize = 0;
    while (i < specs.len) {
        if (bytesEqual(specs[i].long_name, name)) return i;
        i += 1;
    }
    return null;
}

fn findFlagByShort(specs: []const CliFlagSpec, ch: u8): usize? {
    if (ch == 0) return null;
    let i: usize = 0;
    while (i < specs.len) {
        if (specs[i].short_name == ch) return i;
        i += 1;
    }
    return null;
}

/// Looks up a parsed flag value by its canonical `long_name`.
pub fn getParsedFlag(
    specs: []const CliFlagSpec,
    parsed_flags: []const CliParsedFlag,
    long_name: []const u8,
): CliParsedFlag? {
    if (idx := findFlagByLong(specs, long_name)) {
        if (idx < parsed_flags.len) return parsed_flags[idx];
    }
    return null;
}

fn applyTypedValue(
    spec: CliFlagSpec,
    spec_idx: usize,
    raw_val: []const u8,
    src: CliFlagValueSource,
    parsed_flags: []CliParsedFlag,
    list_vals_out: [][]const u8,
    list_len: *usize,
    diag_out: *CapArgsDiagnostic,
)! {
    let entry = parsed_flags[spec_idx];
    entry.spec_index = spec_idx;
    entry.source = src;

    if (spec.kind == CliFlagValueKind.Bool) {
        if (b := parseBoolStr(raw_val)) {
            entry.bool_val = b;
        } else {
            diag_out.* = CapArgsDiagnostic{
                has_error: true,
                code: CapArgsError.UnexpectedFlagValue,
                offending_token: raw_val,
            };
            return CapArgsError.UnexpectedFlagValue;
        }
    } else if (spec.kind == CliFlagValueKind.Count) {
        const c = parseCliUint(raw_val) catch {
            diag_out.* = CapArgsDiagnostic{
                has_error: true,
                code: CapArgsError.InvalidUintValue,
                offending_token: raw_val,
            };
            return CapArgsError.InvalidUintValue;
        };
        entry.count_val = @intCast(c);
    } else if (spec.kind == CliFlagValueKind.String) {
        entry.str_val = raw_val;
    } else if (spec.kind == CliFlagValueKind.Int) {
        const iv = parseCliInt(raw_val) catch {
            diag_out.* = CapArgsDiagnostic{
                has_error: true,
                code: CapArgsError.InvalidIntValue,
                offending_token: raw_val,
            };
            return CapArgsError.InvalidIntValue;
        };
        entry.int_val = iv;
    } else if (spec.kind == CliFlagValueKind.Uint) {
        const uv = parseCliUint(raw_val) catch {
            diag_out.* = CapArgsDiagnostic{
                has_error: true,
                code: CapArgsError.InvalidUintValue,
                offending_token: raw_val,
            };
            return CapArgsError.InvalidUintValue;
        };
        entry.uint_val = uv;
    } else if (spec.kind == CliFlagValueKind.ByteSize) {
        const bz = parseCliByteSize(raw_val) catch {
            diag_out.* = CapArgsDiagnostic{
                has_error: true,
                code: CapArgsError.InvalidByteSize,
                offending_token: raw_val,
            };
            return CapArgsError.InvalidByteSize;
        };
        entry.uint_val = bz;
    } else if (spec.kind == CliFlagValueKind.Enum) {
        if (!isAllowedEnumValue(raw_val, spec.allowed_values)) {
            diag_out.* = CapArgsDiagnostic{
                has_error: true,
                code: CapArgsError.InvalidEnumValue,
                offending_token: spec.long_name,
            };
            return CapArgsError.InvalidEnumValue;
        }
        entry.str_val = raw_val;
    } else if (spec.kind == CliFlagValueKind.StringList) {
        if (list_len.* >= list_vals_out.len) return CapArgsError.SliceCapacityExceeded;
        if (entry.list_len == 0) {
            entry.list_start = list_len.*;
        }
        list_vals_out[list_len.*] = raw_val;
        list_len.* += 1;
        entry.list_len += 1;
        entry.str_val = raw_val;
    }

    parsed_flags[spec_idx] = entry;
}

/// Parses `argv` (excluding binary name at index 0) using declarative `specs`,
/// routing subcommands, handling global/local flags, `--no-*` negation, clustered
/// short flags (`-abc`), `--` passthrough args, env/default fallbacks, and spell-check hints.
pub fn parseCapArgs(
    argv: []const []const u8,
    specs: []const CliFlagSpec,
    env_vars: []const CapEnvBinding,
    require_known_subcommand: bool,
    parsed_flags_out: []CliParsedFlag,
    list_vals_out: [][]const u8,
    positionals_out: [][]const u8,
    passthrough_out: [][]const u8,
    diag_out: *CapArgsDiagnostic,
): CapArgsResult! {
    diag_out.* = CapArgsDiagnostic.none();
    if (specs.len > parsed_flags_out.len) {
        return CapArgsError.SliceCapacityExceeded;
    }

    // Initialize parsed_flags_out with defaults and environment fallbacks
    let list_len: usize = 0;
    let si: usize = 0;
    while (si < specs.len) {
        let init_entry = CliParsedFlag.empty();
        init_entry.spec_index = si;
        parsed_flags_out[si] = init_entry;

        if (def_v := specs[si].default_val) {
            applyTypedValue(specs[si], si, def_v, CliFlagValueSource.Default, parsed_flags_out, list_vals_out, &list_len, diag_out)?;
        }
        if (env_key := specs[si].env_var) {
            if (env_v := lookupEnvVar(env_vars, env_key)) {
                applyTypedValue(specs[si], si, env_v, CliFlagValueSource.Env, parsed_flags_out, list_vals_out, &list_len, diag_out)?;
            }
        }
        si += 1;
    }

    let res = CapArgsResult{};
    let pos_len: usize = 0;
    let pass_len: usize = 0;
    let seen_subcommand = false;

    let i: usize = 0;
    while (i < argv.len) {
        const tok = argv[i];

        // `--` stops all option parsing; remaining args go to passthrough_out
        if (bytesEqual(tok, "--")) {
            let p = i + 1;
            while (p < argv.len) {
                if (pass_len >= passthrough_out.len) return CapArgsError.SliceCapacityExceeded;
                passthrough_out[pass_len] = argv[p];
                pass_len += 1;
                p += 1;
            }
            break;
        }

        // Long flag: `--...`
        if (startsWith(tok, "--") && tok.len > 2) {
            const body = tok[2..];
            let name = body;
            let inline_val: ([]const u8)? = null;
            if (eq_idx := indexOfByte(body, '=')) {
                name = body[0..eq_idx];
                inline_val = body[eq_idx + 1 ..];
            }

            // Built-in --help / --version convenience
            if (bytesEqual(name, "help") && findFlagByLong(specs, "help") == null) {
                res.help_requested = true;
                i += 1;
                continue;
            }
            if (bytesEqual(name, "version") && findFlagByLong(specs, "version") == null) {
                res.version_requested = true;
                i += 1;
                continue;
            }

            // Check direct match or `--no-<flag>` negation
            let spec_idx_opt = findFlagByLong(specs, name);
            let negated = false;
            if (spec_idx_opt == null && startsWith(name, "no-") && name.len > 3) {
                if (neg_idx := findFlagByLong(specs, name[3..])) {
                    if (specs[neg_idx].allow_negation && specs[neg_idx].kind == CliFlagValueKind.Bool) {
                        spec_idx_opt = neg_idx;
                        negated = true;
                    }
                }
            }

            if (spec_idx_opt == null) {
                const sug = suggestClosestFlag(name, specs);
                diag_out.* = CapArgsDiagnostic{
                    has_error: true,
                    code: CapArgsError.UnknownFlag,
                    offending_token: tok,
                    suggestion: sug ?? "",
                    has_suggestion: (sug != null),
                };
                return CapArgsError.UnknownFlag;
            }

            const s_idx = spec_idx_opt.?;
            const sp = specs[s_idx];

            if (sp.kind == CliFlagValueKind.Bool) {
                if (iv := inline_val) {
                    applyTypedValue(sp, s_idx, iv, CliFlagValueSource.Cli, parsed_flags_out, list_vals_out, &list_len, diag_out)?;
                } else {
                    let e = parsed_flags_out[s_idx];
                    e.source = CliFlagValueSource.Cli;
                    e.bool_val = !negated;
                    parsed_flags_out[s_idx] = e;
                }
            } else if (sp.kind == CliFlagValueKind.Count) {
                if (iv := inline_val) {
                    applyTypedValue(sp, s_idx, iv, CliFlagValueSource.Cli, parsed_flags_out, list_vals_out, &list_len, diag_out)?;
                } else {
                    let e = parsed_flags_out[s_idx];
                    e.source = CliFlagValueSource.Cli;
                    e.count_val += 1;
                    parsed_flags_out[s_idx] = e;
                }
            } else {
                let val_str: []const u8 = "";
                if (iv := inline_val) {
                    val_str = iv;
                } else {
                    if (i + 1 >= argv.len) {
                        diag_out.* = CapArgsDiagnostic{
                            has_error: true,
                            code: CapArgsError.MissingFlagValue,
                            offending_token: tok,
                        };
                        return CapArgsError.MissingFlagValue;
                    }
                    i += 1;
                    val_str = argv[i];
                }
                applyTypedValue(sp, s_idx, val_str, CliFlagValueSource.Cli, parsed_flags_out, list_vals_out, &list_len, diag_out)?;
            }

            i += 1;
            continue;
        }

        // Short flag or clustered short flags: `-f`, `-abc`, `-vvv`
        if (tok.len >= 2 && tok[0] == '-' && tok[1] != '-') {
            let ci: usize = 1;
            while (ci < tok.len) {
                const ch = tok[ci];
                if (ch == 'h' && findFlagByShort(specs, 'h') == null) {
                    res.help_requested = true;
                    ci += 1;
                    continue;
                }
                if (ch == 'V' && findFlagByShort(specs, 'V') == null) {
                    res.version_requested = true;
                    ci += 1;
                    continue;
                }

                const s_opt = findFlagByShort(specs, ch);
                if (s_opt == null) {
                    diag_out.* = CapArgsDiagnostic{
                        has_error: true,
                        code: CapArgsError.UnknownFlag,
                        offending_token: tok,
                    };
                    return CapArgsError.UnknownFlag;
                }

                const s_idx = s_opt.?;
                const sp = specs[s_idx];

                if (sp.kind == CliFlagValueKind.Bool) {
                    let e = parsed_flags_out[s_idx];
                    e.source = CliFlagValueSource.Cli;
                    e.bool_val = true;
                    parsed_flags_out[s_idx] = e;
                    ci += 1;
                } else if (sp.kind == CliFlagValueKind.Count) {
                    let e = parsed_flags_out[s_idx];
                    e.source = CliFlagValueSource.Cli;
                    e.count_val += 1;
                    parsed_flags_out[s_idx] = e;
                    ci += 1;
                } else {
                    // Value-taking short flag: either remainder of token (`-p8080`) or next argv (`-p 8080`)
                    let val_str: []const u8 = "";
                    if (ci + 1 < tok.len) {
                        val_str = tok[ci + 1 ..];
                        ci = tok.len;
                    } else {
                        if (i + 1 >= argv.len) {
                            diag_out.* = CapArgsDiagnostic{
                                has_error: true,
                                code: CapArgsError.MissingFlagValue,
                                offending_token: tok,
                            };
                            return CapArgsError.MissingFlagValue;
                        }
                        i += 1;
                        val_str = argv[i];
                        ci = tok.len;
                    }
                    applyTypedValue(sp, s_idx, val_str, CliFlagValueSource.Cli, parsed_flags_out, list_vals_out, &list_len, diag_out)?;
                }
            }
            i += 1;
            continue;
        }

        // First non-flag token routes the subcommand
        if (!seen_subcommand) {
            seen_subcommand = true;
            if (cmd := resolveCapSubcommand(tok)) {
                res.subcommand = cmd;
                res.subcommand_raw = tok;
                i += 1;
                continue;
            } else if (require_known_subcommand) {
                const sug = suggestClosestSubcommand(tok);
                diag_out.* = CapArgsDiagnostic{
                    has_error: true,
                    code: CapArgsError.UnknownSubcommand,
                    offending_token: tok,
                    suggestion: sug ?? "",
                    has_suggestion: (sug != null),
                };
                return CapArgsError.UnknownSubcommand;
            }
        }

        // Positional argument
        if (pos_len >= positionals_out.len) return CapArgsError.SliceCapacityExceeded;
        positionals_out[pos_len] = tok;
        pos_len += 1;
        i += 1;
    }

    // Validate required flags (unless --help or --version was requested)
    if (!res.help_requested && !res.version_requested) {
        let ri: usize = 0;
        while (ri < specs.len) {
            if (specs[ri].required && parsed_flags_out[ri].source == CliFlagValueSource.Unset) {
                diag_out.* = CapArgsDiagnostic{
                    has_error: true,
                    code: CapArgsError.MissingRequiredFlag,
                    offending_token: specs[ri].long_name,
                };
                return CapArgsError.MissingRequiredFlag;
            }
            ri += 1;
        }
    }

    res.positionals_len = pos_len;
    res.passthrough_len = pass_len;
    res.list_vals_len = list_len;
    return res;
}

// ============================================================================
// Declarative Help Renderer & Shell Completion Generator (Zero-Alloc)
// ============================================================================

/// Renders an aligned Usage / Commands / Options help screen into `out_buf`.
pub fn formatCommandHelp(
    bin_name: []const u8,
    cmd_name: []const u8,
    summary: []const u8,
    specs: []const CliFlagSpec,
    include_subcommands: bool,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "Usage: ")?;
    appendBytes(out_buf, &pos, bin_name)?;
    if (cmd_name.len > 0) {
        appendByte(out_buf, &pos, @as(u8, 32))?;
        appendBytes(out_buf, &pos, cmd_name)?;
    } else if (include_subcommands) {
        appendBytes(out_buf, &pos, " <command>")?;
    }
    appendBytes(out_buf, &pos, " [options]\n")?;

    if (summary.len > 0) {
        appendByte(out_buf, &pos, @as(u8, 10))?;
        appendBytes(out_buf, &pos, summary)?;
        appendByte(out_buf, &pos, @as(u8, 10))?;
    }

    if (include_subcommands) {
        appendBytes(out_buf, &pos, "\nCommands:\n")?;
        let ci: usize = 0;
        while (ci < ALL_SUBCOMMANDS.len) {
            const c = ALL_SUBCOMMANDS[ci];
            const cname = capSubcommandName(c);
            const csum = capSubcommandSummary(c);
            appendBytes(out_buf, &pos, "  ")?;
            appendBytes(out_buf, &pos, cname)?;
            const pad = if (cname.len < 12) 12 - cname.len else 2;
            appendSpaces(out_buf, &pos, pad)?;
            appendBytes(out_buf, &pos, csum)?;
            appendByte(out_buf, &pos, @as(u8, 10))?;
            ci += 1;
        }
    }

    if (specs.len > 0) {
        appendBytes(out_buf, &pos, "\nOptions:\n")?;
        let i: usize = 0;
        while (i < specs.len) {
            const sp = specs[i];
            const line_start = pos;
            appendBytes(out_buf, &pos, "  ")?;
            if (sp.short_name != 0) {
                appendByte(out_buf, &pos, @as(u8, 45))?; // '-'
                appendByte(out_buf, &pos, sp.short_name)?;
                appendBytes(out_buf, &pos, ", ")?;
            } else {
                appendBytes(out_buf, &pos, "    ")?;
            }
            if (sp.long_name.len > 0) {
                appendBytes(out_buf, &pos, "--")?;
                appendBytes(out_buf, &pos, sp.long_name)?;
            }
            if (sp.value_placeholder.len > 0) {
                appendByte(out_buf, &pos, @as(u8, 32))?;
                appendBytes(out_buf, &pos, sp.value_placeholder)?;
            }
            const col_used = pos - line_start;
            const pad = if (col_used < 28) 28 - col_used else 2;
            appendSpaces(out_buf, &pos, pad)?;
            appendBytes(out_buf, &pos, sp.description)?;
            appendByte(out_buf, &pos, @as(u8, 10))?;
            i += 1;
        }
    }

    return pos;
}

/// Generates a Bash or Zsh shell completion script for `bin_name` and `specs`.
pub fn formatShellCompletion(
    bin_name: []const u8,
    shell: CapCompletionShell,
    specs: []const CliFlagSpec,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;

    if (shell == CapCompletionShell.Bash) {
        appendBytes(out_buf, &pos, "_")?;
        appendBytes(out_buf, &pos, bin_name)?;
        appendBytes(out_buf, &pos, "_completions() {\n  local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n  local words=\"")?;
        let ci: usize = 0;
        while (ci < ALL_SUBCOMMANDS.len) {
            if (ci > 0) appendByte(out_buf, &pos, @as(u8, 32))?;
            appendBytes(out_buf, &pos, capSubcommandName(ALL_SUBCOMMANDS[ci]))?;
            ci += 1;
        }
        let fi: usize = 0;
        while (fi < specs.len) {
            if (specs[fi].long_name.len > 0) {
                appendBytes(out_buf, &pos, " --")?;
                appendBytes(out_buf, &pos, specs[fi].long_name)?;
            }
            fi += 1;
        }
        appendBytes(out_buf, &pos, "\"\n  COMPREPLY=( $(compgen -W \"${words}\" -- \"${cur}\") )\n}\ncomplete -F _")?;
        appendBytes(out_buf, &pos, bin_name)?;
        appendBytes(out_buf, &pos, "_completions ")?;
        appendBytes(out_buf, &pos, bin_name)?;
        appendByte(out_buf, &pos, @as(u8, 10))?;
        return pos;
    }

    // Zsh completion script
    appendBytes(out_buf, &pos, "#compdef ")?;
    appendBytes(out_buf, &pos, bin_name)?;
    appendBytes(out_buf, &pos, "\n_")?;
    appendBytes(out_buf, &pos, bin_name)?;
    appendBytes(out_buf, &pos, "() {\n  local -a commands options\n  commands=(\n")?;
    let ci: usize = 0;
    while (ci < ALL_SUBCOMMANDS.len) {
        const c = ALL_SUBCOMMANDS[ci];
        appendBytes(out_buf, &pos, "    '")?;
        appendBytes(out_buf, &pos, capSubcommandName(c))?;
        appendByte(out_buf, &pos, @as(u8, 58))?; // ':'
        appendBytes(out_buf, &pos, capSubcommandSummary(c))?;
        appendBytes(out_buf, &pos, "'\n")?;
        ci += 1;
    }
    appendBytes(out_buf, &pos, "  )\n  options=(\n")?;
    let fi: usize = 0;
    while (fi < specs.len) {
        if (specs[fi].long_name.len > 0) {
            appendBytes(out_buf, &pos, "    '--")?;
            appendBytes(out_buf, &pos, specs[fi].long_name)?;
            appendBytes(out_buf, &pos, "[")?;
            appendBytes(out_buf, &pos, specs[fi].description)?;
            appendBytes(out_buf, &pos, "]'\n")?;
        }
        fi += 1;
    }
    appendBytes(out_buf, &pos, "  )\n  _describe 'command' commands\n  _arguments $options\n}\n")?;
    return pos;
}
