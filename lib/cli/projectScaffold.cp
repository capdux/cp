// Source: Absorbed from /data/dev/bun/src/cli/init_command.cp | Synced: 2026-11-23T20:18:24Z
// ============================================================================
// lib/cli/projectScaffold.cp — Zero-Alloc Project Scaffold & Template Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven project scaffolding engine for
// `cap init` and `cap create`: kebab-case / scoped project name sanitizer,
// 6 built-in CP project templates (MinimalApp, StdLibrary, AiAgentService,
// HttpApiServer, RealtimeMediaApp, WorkspaceMonorepo), `{{key}}` + `{{#if}}`
// template interpolation engine with strict/lenient diagnostics, file conflict
// policy resolver, structured `cap.json` generator, and plan report formatter.

/// Default initial SemVer for newly scaffolded projects.
pub const SCAFFOLD_DEFAULT_VERSION: []const u8 = "0.1.0";

/// Default SPDX license identifier.
pub const SCAFFOLD_DEFAULT_LICENSE: []const u8 = "MIT";

/// Default copyright year for LICENSE rendering.
pub const SCAFFOLD_DEFAULT_YEAR: []const u8 = "2026";

/// Maximum legal package/project name length.
pub const SCAFFOLD_MAX_NAME_LEN: usize = 214;

/// Project scaffolding, template rendering, and conflict resolution error set.
pub err ScaffoldError {
    EmptyProjectName,
    InvalidProjectName,
    ReservedProjectName,
    UnsafePathTraversal,
    UnknownTemplate,
    UndefinedTemplateVar,
    UnclosedTemplateTag,
    UnmatchedIfBlock,
    ConflictDetected,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Built-in CP project scaffold template kinds.
pub enum ScaffoldTemplateKind: u8 {
    MinimalApp = 0,
    StdLibrary = 1,
    AiAgentService = 2,
    HttpApiServer = 3,
    RealtimeMediaApp = 4,
    WorkspaceMonorepo = 5,
}

/// Strategy for handling pre-existing files during scaffolding.
pub enum ScaffoldConflictPolicy: u8 {
    SkipExisting = 0,
    Overwrite = 1,
    FailOnConflict = 2,
    MergeJson = 3,
}

/// Resolved file action produced by conflict adjudication.
pub enum ScaffoldFileAction: u8 {
    Create = 0,
    Overwrite = 1,
    Skip = 2,
    Merge = 3,
    ConflictError = 4,
}

/// Template variable key/value binding.
pub struct ScaffoldVarBinding {
    key: []const u8 = "",
    value: []const u8 = "",
}

/// Options controlling `renderScaffoldTemplate`.
pub struct ScaffoldRenderOptions {
    strict_undefined_vars: bool = false,
    keep_undefined_placeholder: bool = false,

    static fn defaults(): ScaffoldRenderOptions {
        return ScaffoldRenderOptions{};
    }

    static fn strict(): ScaffoldRenderOptions {
        return ScaffoldRenderOptions{
            strict_undefined_vars: true,
            keep_undefined_placeholder: false,
        };
    }
}

/// Diagnostic summary populated by `renderScaffoldTemplate`.
pub struct ScaffoldRenderDiag {
    undefined_count: u32 = 0,
    first_undefined_key: []const u8 = "",

    static fn empty(): ScaffoldRenderDiag {
        return ScaffoldRenderDiag{};
    }
}

/// Dependency entry for structured `cap.json` generation.
pub struct CapManifestDep {
    name: []const u8 = "",
    spec: []const u8 = "",
}

/// Script entry for structured `cap.json` generation.
pub struct CapManifestScript {
    name: []const u8 = "",
    command: []const u8 = "",
}

/// Complete project scaffold configuration options.
pub struct ScaffoldProjectOptions {
    name: []const u8 = "my-cp-app",
    version: []const u8 = SCAFFOLD_DEFAULT_VERSION,
    description: []const u8 = "",
    author: []const u8 = "",
    license: []const u8 = SCAFFOLD_DEFAULT_LICENSE,
    entry: []const u8 = "src/main.cp",
    year: []const u8 = SCAFFOLD_DEFAULT_YEAR,
    template: ScaffoldTemplateKind = ScaffoldTemplateKind.MinimalApp,
    is_private: bool = false,
    include_tests: bool = true,
    include_readme: bool = true,
    include_license: bool = true,
    include_gitignore: bool = true,
    conflict_policy: ScaffoldConflictPolicy = ScaffoldConflictPolicy.SkipExisting,

    static fn defaults(): ScaffoldProjectOptions {
        return ScaffoldProjectOptions{};
    }
}

/// Raw unrendered template file descriptor.
pub struct ScaffoldTemplateFile {
    rel_path: []const u8 = "",
    template_body: []const u8 = "",
    is_json: bool = false,
    is_executable: bool = false,

    static fn empty(): ScaffoldTemplateFile {
        return ScaffoldTemplateFile{};
    }
}

/// Planned file write/skip/merge action with zero-copy rendered content in caller arena.
pub struct ScaffoldPlannedFile {
    rel_path: []const u8 = "",
    action: ScaffoldFileAction = ScaffoldFileAction.Create,
    is_json: bool = false,
    is_executable: bool = false,
    rendered_content: []const u8 = "",

    static fn empty(): ScaffoldPlannedFile {
        return ScaffoldPlannedFile{};
    }
}

/// Aggregate summary returned by `planProjectScaffold`.
pub struct ScaffoldPlanSummary {
    template: ScaffoldTemplateKind = ScaffoldTemplateKind.MinimalApp,
    project_name: []const u8 = "",
    total_files: u32 = 0,
    create_count: u32 = 0,
    overwrite_count: u32 = 0,
    skip_count: u32 = 0,
    merge_count: u32 = 0,
    conflict_count: u32 = 0,
    total_bytes: usize = 0,
}

// ============================================================================
// Internal Byte, Buffer & JSON Escaping Helpers (Zero-Alloc)
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

fn indexOfByte(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn indexOfSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (haystack.len < needle.len) return null;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return i;
        i += 1;
    }
    return null;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) return ScaffoldError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendByte(out_buf: []u8, pos: *usize, ch: u8)! {
    if (pos.* >= out_buf.len) return ScaffoldError.BufferTooSmall;
    out_buf[pos.*] = ch;
    pos.* += 1;
}

fn appendJsonEscapedString(out_buf: []u8, pos: *usize, s: []const u8)! {
    appendByte(out_buf, pos, @as(u8, 34))?; // '"'
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            appendBytes(out_buf, pos, "\\\"")?;
        } else if (c == '\\') {
            appendBytes(out_buf, pos, "\\\\")?;
        } else if (c == '\n') {
            appendBytes(out_buf, pos, "\\n")?;
        } else if (c == '\r') {
            appendBytes(out_buf, pos, "\\r")?;
        } else if (c == '\t') {
            appendBytes(out_buf, pos, "\\t")?;
        } else {
            appendByte(out_buf, pos, c)?;
        }
        i += 1;
    }
    appendByte(out_buf, pos, @as(u8, 34))?; // '"'
}

fn appendU32Decimal(out_buf: []u8, pos: *usize, value: u32)! {
    let digits: [16]u8 = [0; 16];
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
    if (pos.* + len > out_buf.len) return ScaffoldError.BufferTooSmall;
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
}

// ============================================================================
// Project Name Normalization & Security Validation
// ============================================================================

/// Returns true if `name` collides with a reserved package/filesystem name.
pub fn isReservedProjectName(name: []const u8): bool {
    return bytesEqualIgnoreCase(name, "node_modules") ||
        bytesEqualIgnoreCase(name, "cap_modules") ||
        bytesEqualIgnoreCase(name, "favicon.ico") ||
        bytesEqualIgnoreCase(name, "con") ||
        bytesEqualIgnoreCase(name, "prn") ||
        bytesEqualIgnoreCase(name, "aux") ||
        bytesEqualIgnoreCase(name, "nul") ||
        bytesEqualIgnoreCase(name, "std");
}

fn normalizeKebabSegment(raw: []const u8, out_buf: []u8, pos: *usize)! {
    let emitted_any = false;
    let prev_dash = false;
    let i: usize = 0;
    while (i < raw.len) {
        const c = toLowerAscii(raw[i]);
        const is_alnum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (is_alnum) {
            appendByte(out_buf, pos, c)?;
            emitted_any = true;
            prev_dash = false;
        } else if (c == '-' || c == '_' || c == ' ' || c == '.') {
            if (emitted_any && !prev_dash) {
                appendByte(out_buf, pos, @as(u8, 45))?; // '-'
                prev_dash = true;
            }
        }
        i += 1;
    }
    // Strip trailing '-'
    while (pos.* > 0 && out_buf[pos.* - 1] == '-') {
        pos.* -= 1;
    }
    if (!emitted_any) {
        return ScaffoldError.InvalidProjectName;
    }
}

/// Normalizes a raw directory or user-supplied name into a valid kebab-case or
/// scoped (`@scope/pkg`) CP project name. Rejects path traversal (`..`, `\`, NUL)
/// and reserved package names.
pub fn normalizeProjectName(raw: []const u8, out_buf: []u8): usize! {
    const trimmed = trimWs(raw);
    if (trimmed.len == 0) return ScaffoldError.EmptyProjectName;
    if (trimmed.len > SCAFFOLD_MAX_NAME_LEN) return ScaffoldError.InvalidProjectName;

    // Reject path traversal & illegal characters up front
    if (indexOfSubstr(trimmed, "..") != null ||
        indexOfByte(trimmed, '\\') != null ||
        indexOfByte(trimmed, 0) != null)
    {
        return ScaffoldError.UnsafePathTraversal;
    }

    let pos: usize = 0;
    if (trimmed[0] == '@') {
        const slash_opt = indexOfByte(trimmed, '/');
        if (slash_opt == null) return ScaffoldError.InvalidProjectName;
        const s_idx = slash_opt.?;
        if (s_idx <= 1 || s_idx + 1 >= trimmed.len) return ScaffoldError.InvalidProjectName;
        // Reject extra '/'
        if (indexOfByte(trimmed[s_idx + 1 ..], '/') != null) return ScaffoldError.UnsafePathTraversal;

        appendByte(out_buf, &pos, @as(u8, 64))?; // '@'
        normalizeKebabSegment(trimmed[1..s_idx], out_buf, &pos)?;
        appendByte(out_buf, &pos, @as(u8, 47))?; // '/'
        const pkg_start = pos;
        normalizeKebabSegment(trimmed[s_idx + 1 ..], out_buf, &pos)?;
        if (isReservedProjectName(out_buf[pkg_start..pos])) {
            return ScaffoldError.ReservedProjectName;
        }
        return pos;
    }

    if (indexOfByte(trimmed, '/') != null) {
        return ScaffoldError.UnsafePathTraversal;
    }

    normalizeKebabSegment(trimmed, out_buf, &pos)?;
    if (isReservedProjectName(out_buf[0..pos])) {
        return ScaffoldError.ReservedProjectName;
    }
    return pos;
}

// ============================================================================
// Template Kind Resolution & Metadata
// ============================================================================

/// Parses a CLI `--template` value into `ScaffoldTemplateKind`.
pub fn parseScaffoldTemplateKind(name: []const u8): ScaffoldTemplateKind? {
    if (bytesEqualIgnoreCase(name, "minimal") || bytesEqualIgnoreCase(name, "app") || bytesEqualIgnoreCase(name, "blank")) {
        return ScaffoldTemplateKind.MinimalApp;
    }
    if (bytesEqualIgnoreCase(name, "lib") || bytesEqualIgnoreCase(name, "library") || bytesEqualIgnoreCase(name, "std")) {
        return ScaffoldTemplateKind.StdLibrary;
    }
    if (bytesEqualIgnoreCase(name, "agent") || bytesEqualIgnoreCase(name, "ai") || bytesEqualIgnoreCase(name, "ai-agent")) {
        return ScaffoldTemplateKind.AiAgentService;
    }
    if (bytesEqualIgnoreCase(name, "http") || bytesEqualIgnoreCase(name, "api") || bytesEqualIgnoreCase(name, "server")) {
        return ScaffoldTemplateKind.HttpApiServer;
    }
    if (bytesEqualIgnoreCase(name, "media") || bytesEqualIgnoreCase(name, "realtime")) {
        return ScaffoldTemplateKind.RealtimeMediaApp;
    }
    if (bytesEqualIgnoreCase(name, "workspace") || bytesEqualIgnoreCase(name, "monorepo")) {
        return ScaffoldTemplateKind.WorkspaceMonorepo;
    }
    return null;
}

/// Returns the canonical template slug name.
pub fn scaffoldTemplateName(kind: ScaffoldTemplateKind): []const u8 {
    if (kind == ScaffoldTemplateKind.MinimalApp) return "minimal";
    if (kind == ScaffoldTemplateKind.StdLibrary) return "library";
    if (kind == ScaffoldTemplateKind.AiAgentService) return "ai-agent";
    if (kind == ScaffoldTemplateKind.HttpApiServer) return "http-api";
    if (kind == ScaffoldTemplateKind.RealtimeMediaApp) return "realtime-media";
    return "workspace";
}

/// Returns a concise human-readable description of a template.
pub fn scaffoldTemplateDescription(kind: ScaffoldTemplateKind): []const u8 {
    if (kind == ScaffoldTemplateKind.MinimalApp) return "Minimal standalone CP native application";
    if (kind == ScaffoldTemplateKind.StdLibrary) return "Reusable zero-alloc CP standard library package";
    if (kind == ScaffoldTemplateKind.AiAgentService) return "Concurrent Actor-based AI Agent service";
    if (kind == ScaffoldTemplateKind.HttpApiServer) return "High-performance cap:http REST API server";
    if (kind == ScaffoldTemplateKind.RealtimeMediaApp) return "Low-latency audio/video media pipeline application";
    return "Multi-package CP workspace monorepo";
}

// ============================================================================
// Zero-Alloc `{{key}}` & `{{#if cond}}...{{else}}...{{/if}}` Template Engine
// ============================================================================

fn lookupVar(vars: []const ScaffoldVarBinding, key: []const u8): ([]const u8)? {
    // Later bindings override earlier ones
    let i = vars.len;
    while (i > 0) {
        i -= 1;
        if (bytesEqual(vars[i].key, key)) {
            return vars[i].value;
        }
    }
    return null;
}

fn isTruthyVar(val_opt: ([]const u8)?): bool {
    if (v := val_opt) {
        if (v.len == 0) return false;
        if (bytesEqualIgnoreCase(v, "false") || bytesEqual(v, "0")) return false;
        return true;
    }
    return false;
}

/// Renders a template containing `{{key}}` placeholders and `{{#if cond}}...{{else}}...{{/if}}`
/// conditional blocks into `out_buf` with zero heap allocation.
pub fn renderScaffoldTemplate(
    tpl: []const u8,
    vars: []const ScaffoldVarBinding,
    opts: ScaffoldRenderOptions,
    out_buf: []u8,
    diag_out: *ScaffoldRenderDiag,
): usize! {
    diag_out.* = ScaffoldRenderDiag.empty();
    let out_pos: usize = 0;

    // Nested conditional block stack (up to 16 levels deep)
    let cond_active: [16]bool = [true; 16];
    let cond_branch_taken: [16]bool = [false; 16];
    let cond_depth: usize = 0;

    let i: usize = 0;
    while (i < tpl.len) {
        const emitting = if (cond_depth == 0) true else cond_active[cond_depth - 1];

        if (i + 1 < tpl.len && tpl[i] == '{' && tpl[i + 1] == '{') {
            const close_rel = indexOfSubstr(tpl[i + 2 ..], "}}");
            if (close_rel == null) {
                return ScaffoldError.UnclosedTemplateTag;
            }
            const tag_raw = tpl[i + 2 .. i + 2 + close_rel.?];
            const tag = trimWs(tag_raw);
            const next_i = i + 2 + close_rel.? + 2;

            if (startsWith(tag, "#if ")) {
                if (cond_depth >= 16) return ScaffoldError.SliceCapacityExceeded;
                const cond_key = trimWs(tag[4..]);
                const truthy = isTruthyVar(lookupVar(vars, cond_key));
                const parent_emit = emitting;
                cond_active[cond_depth] = parent_emit && truthy;
                cond_branch_taken[cond_depth] = truthy;
                cond_depth += 1;
                i = next_i;
                continue;
            }

            if (bytesEqual(tag, "else")) {
                if (cond_depth == 0) return ScaffoldError.UnmatchedIfBlock;
                const parent_emit = if (cond_depth == 1) true else cond_active[cond_depth - 2];
                cond_active[cond_depth - 1] = parent_emit && !cond_branch_taken[cond_depth - 1];
                i = next_i;
                continue;
            }

            if (bytesEqual(tag, "/if")) {
                if (cond_depth == 0) return ScaffoldError.UnmatchedIfBlock;
                cond_depth -= 1;
                i = next_i;
                continue;
            }

            if (emitting) {
                if (val := lookupVar(vars, tag)) {
                    appendBytes(out_buf, &out_pos, val)?;
                } else {
                    if (diag_out.undefined_count == 0) {
                        diag_out.first_undefined_key = tag;
                    }
                    diag_out.undefined_count += 1;
                    if (opts.strict_undefined_vars) {
                        return ScaffoldError.UndefinedTemplateVar;
                    }
                    if (opts.keep_undefined_placeholder) {
                        appendBytes(out_buf, &out_pos, tpl[i..next_i])?;
                    }
                }
            }
            i = next_i;
            continue;
        }

        if (emitting) {
            appendByte(out_buf, &out_pos, tpl[i])?;
        }
        i += 1;
    }

    if (cond_depth != 0) {
        return ScaffoldError.UnmatchedIfBlock;
    }
    return out_pos;
}

// ============================================================================
// Structured `cap.json` Generator
// ============================================================================

/// Generates a cleanly formatted, JSON-escaped `cap.json` manifest into `out_buf`.
pub fn formatCapManifestJson(
    opts: ScaffoldProjectOptions,
    deps: []const CapManifestDep,
    scripts: []const CapManifestScript,
    workspaces: []const []const u8,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "{\n  \"name\": ")?;
    appendJsonEscapedString(out_buf, &pos, opts.name)?;
    appendBytes(out_buf, &pos, ",\n  \"version\": ")?;
    appendJsonEscapedString(out_buf, &pos, opts.version)?;

    if (opts.description.len > 0) {
        appendBytes(out_buf, &pos, ",\n  \"description\": ")?;
        appendJsonEscapedString(out_buf, &pos, opts.description)?;
    }

    const pkg_type: []const u8 = if (opts.template == ScaffoldTemplateKind.StdLibrary)
        "library"
    else if (opts.template == ScaffoldTemplateKind.WorkspaceMonorepo)
        "workspace"
    else
        "app";
    appendBytes(out_buf, &pos, ",\n  \"type\": ")?;
    appendJsonEscapedString(out_buf, &pos, pkg_type)?;

    if (opts.is_private || opts.template == ScaffoldTemplateKind.WorkspaceMonorepo) {
        appendBytes(out_buf, &pos, ",\n  \"private\": true")?;
    }

    if (opts.entry.len > 0 && opts.template != ScaffoldTemplateKind.WorkspaceMonorepo) {
        appendBytes(out_buf, &pos, ",\n  \"entry\": ")?;
        appendJsonEscapedString(out_buf, &pos, opts.entry)?;
    }

    if (opts.author.len > 0) {
        appendBytes(out_buf, &pos, ",\n  \"author\": ")?;
        appendJsonEscapedString(out_buf, &pos, opts.author)?;
    }

    appendBytes(out_buf, &pos, ",\n  \"license\": ")?;
    appendJsonEscapedString(out_buf, &pos, opts.license)?;

    if (workspaces.len > 0) {
        appendBytes(out_buf, &pos, ",\n  \"workspaces\": [")?;
        let wi: usize = 0;
        while (wi < workspaces.len) {
            if (wi > 0) appendBytes(out_buf, &pos, ", ")?;
            appendJsonEscapedString(out_buf, &pos, workspaces[wi])?;
            wi += 1;
        }
        appendBytes(out_buf, &pos, "]")?;
    }

    if (scripts.len > 0) {
        appendBytes(out_buf, &pos, ",\n  \"scripts\": {\n")?;
        let si: usize = 0;
        while (si < scripts.len) {
            if (si > 0) appendBytes(out_buf, &pos, ",\n")?;
            appendBytes(out_buf, &pos, "    ")?;
            appendJsonEscapedString(out_buf, &pos, scripts[si].name)?;
            appendBytes(out_buf, &pos, ": ")?;
            appendJsonEscapedString(out_buf, &pos, scripts[si].command)?;
            si += 1;
        }
        appendBytes(out_buf, &pos, "\n  }")?;
    }

    if (deps.len > 0) {
        appendBytes(out_buf, &pos, ",\n  \"dependencies\": {\n")?;
        let di: usize = 0;
        while (di < deps.len) {
            if (di > 0) appendBytes(out_buf, &pos, ",\n")?;
            appendBytes(out_buf, &pos, "    ")?;
            appendJsonEscapedString(out_buf, &pos, deps[di].name)?;
            appendBytes(out_buf, &pos, ": ")?;
            appendJsonEscapedString(out_buf, &pos, deps[di].spec)?;
            di += 1;
        }
        appendBytes(out_buf, &pos, "\n  }")?;
    }

    appendBytes(out_buf, &pos, "\n}\n")?;
    return pos;
}

// ============================================================================
// Built-In Template File Library & Conflict Resolution Planner
// ============================================================================

const TPL_GITIGNORE: []const u8 =
    ".cap/\n" ++
    "bin/\n" ++
    "cap_modules/\n" ++
    "*.o\n" ++
    "*.a\n" ++
    ".DS_Store\n";

const TPL_README: []const u8 =
    "# {{name}}\n\n" ++
    "{{#if description}}{{description}}\n\n{{/if}}" ++
    "## Getting Started\n\n" ++
    "```bash\n" ++
    "cap test\n" ++
    "cap run {{entry}}\n" ++
    "```\n";

const TPL_LICENSE_MIT: []const u8 =
    "MIT License\n\n" ++
    "Copyright (c) {{year}} {{#if author}}{{author}}{{else}}{{name}} Contributors{{/if}}\n\n" ++
    "Permission is hereby granted, free of charge, to any person obtaining a copy\n" ++
    "of this software and associated documentation files.\n";

const TPL_MINIMAL_MAIN: []const u8 =
    "import { println } from \"cap:io\";\n\n" ++
    "pub fn main() {\n" ++
    "    println(\"Hello from {{name}} v{{version}}!\");\n" ++
    "}\n";

const TPL_STD_LIB_INDEX: []const u8 =
    "// {{name}} v{{version}}\n" ++
    "pub fn version(): []const u8 {\n" ++
    "    return \"{{version}}\";\n" ++
    "}\n";

const TPL_AGENT_MAIN: []const u8 =
    "import { println } from \"cap:io\";\n\n" ++
    "// Concurrent Actor service scaffold for {{name}}\n" ++
    "pub fn main() {\n" ++
    "    println(\"[agent] {{name}} ready\");\n" ++
    "}\n";

const TPL_HTTP_MAIN: []const u8 =
    "import { println } from \"cap:io\";\n\n" ++
    "// HTTP API server scaffold for {{name}}\n" ++
    "pub fn main() {\n" ++
    "    println(\"[http] {{name}} listening on :3000\");\n" ++
    "}\n";

const TPL_MEDIA_MAIN: []const u8 =
    "import { println } from \"cap:io\";\n\n" ++
    "// Realtime media pipeline scaffold for {{name}}\n" ++
    "pub fn main() {\n" ++
    "    println(\"[media] {{name}} pipeline initialized\");\n" ++
    "}\n";

const TPL_BASIC_TEST: []const u8 =
    "import { test, assert } from \"cap:test\";\n\n" ++
    "test(\"{{name}} smoke test\", () => {\n" ++
    "    assert(true);\n" ++
    "});\n";

fn pushTemplateFile(
    out: []ScaffoldTemplateFile,
    count: *usize,
    rel_path: []const u8,
    body: []const u8,
    is_json: bool,
)! {
    if (count.* >= out.len) return ScaffoldError.SliceCapacityExceeded;
    out[count.*] = ScaffoldTemplateFile{
        rel_path: rel_path,
        template_body: body,
        is_json: is_json,
        is_executable: false,
    };
    count.* += 1;
}

/// Populates `files_out` with the file entries required for `opts.template`.
pub fn collectTemplateFiles(
    opts: ScaffoldProjectOptions,
    files_out: []ScaffoldTemplateFile,
): usize! {
    let count: usize = 0;

    // 1. Primary source file(s) per template
    if (opts.template == ScaffoldTemplateKind.MinimalApp) {
        pushTemplateFile(files_out, &count, opts.entry, TPL_MINIMAL_MAIN, false)?;
    } else if (opts.template == ScaffoldTemplateKind.StdLibrary) {
        pushTemplateFile(files_out, &count, "src/index.cp", TPL_STD_LIB_INDEX, false)?;
    } else if (opts.template == ScaffoldTemplateKind.AiAgentService) {
        pushTemplateFile(files_out, &count, opts.entry, TPL_AGENT_MAIN, false)?;
    } else if (opts.template == ScaffoldTemplateKind.HttpApiServer) {
        pushTemplateFile(files_out, &count, opts.entry, TPL_HTTP_MAIN, false)?;
    } else if (opts.template == ScaffoldTemplateKind.RealtimeMediaApp) {
        pushTemplateFile(files_out, &count, opts.entry, TPL_MEDIA_MAIN, false)?;
    } else {
        // WorkspaceMonorepo
        pushTemplateFile(files_out, &count, "packages/core/src/index.cp", TPL_STD_LIB_INDEX, false)?;
    }

    // 2. Optional test, gitignore, README, LICENSE
    if (opts.include_tests) {
        pushTemplateFile(files_out, &count, "test/mainTest.cp", TPL_BASIC_TEST, false)?;
    }
    if (opts.include_gitignore) {
        pushTemplateFile(files_out, &count, ".gitignore", TPL_GITIGNORE, false)?;
    }
    if (opts.include_readme) {
        pushTemplateFile(files_out, &count, "README.md", TPL_README, false)?;
    }
    if (opts.include_license) {
        pushTemplateFile(files_out, &count, "LICENSE", TPL_LICENSE_MIT, false)?;
    }

    return count;
}

/// Resolves the action to take for a single file under `policy` given whether it already exists.
pub fn resolveScaffoldConflict(
    is_json: bool,
    exists: bool,
    policy: ScaffoldConflictPolicy,
): ScaffoldFileAction {
    if (!exists) return ScaffoldFileAction.Create;
    if (policy == ScaffoldConflictPolicy.Overwrite) return ScaffoldFileAction.Overwrite;
    if (policy == ScaffoldConflictPolicy.FailOnConflict) return ScaffoldFileAction.ConflictError;
    if (policy == ScaffoldConflictPolicy.MergeJson && is_json) return ScaffoldFileAction.Merge;
    return ScaffoldFileAction.Skip;
}

fn pathExistsInSlice(existing_files: []const []const u8, path: []const u8): bool {
    let i: usize = 0;
    while (i < existing_files.len) {
        if (bytesEqual(existing_files[i], path)) return true;
        i += 1;
    }
    return false;
}

/// Generates the full scaffold plan (including rendered `cap.json` and all template files)
/// into caller-owned slices with zero heap allocation.
pub fn planProjectScaffold(
    opts: ScaffoldProjectOptions,
    existing_files: []const []const u8,
    extra_vars: []const ScaffoldVarBinding,
    planned_out: []ScaffoldPlannedFile,
    content_arena: []u8,
    arena_pos: *usize,
): ScaffoldPlanSummary! {
    let norm_name_buf: [256]u8 = [0; 256];
    const norm_len = normalizeProjectName(opts.name, norm_name_buf[0..])?;
    // Copy normalized project name into arena so summary/cap.json reference stable memory
    const name_start = arena_pos.*;
    appendBytes(content_arena, arena_pos, norm_name_buf[0..norm_len])?;
    const norm_name = content_arena[name_start..arena_pos.*];

    const eff_opts = ScaffoldProjectOptions{
        name: norm_name,
        version: opts.version,
        description: opts.description,
        author: opts.author,
        license: opts.license,
        entry: opts.entry,
        year: opts.year,
        template: opts.template,
        is_private: opts.is_private,
        include_tests: opts.include_tests,
        include_readme: opts.include_readme,
        include_license: opts.include_license,
        include_gitignore: opts.include_gitignore,
        conflict_policy: opts.conflict_policy,
    };

    let summary = ScaffoldPlanSummary{
        template: eff_opts.template,
        project_name: norm_name,
    };

    // Assemble standard template variable bindings + caller extra_vars
    let var_buf: [32]ScaffoldVarBinding = [ScaffoldVarBinding{}; 32];
    var_buf[0] = ScaffoldVarBinding{ key: "name", value: norm_name };
    var_buf[1] = ScaffoldVarBinding{ key: "version", value: eff_opts.version };
    var_buf[2] = ScaffoldVarBinding{ key: "description", value: eff_opts.description };
    var_buf[3] = ScaffoldVarBinding{ key: "author", value: eff_opts.author };
    var_buf[4] = ScaffoldVarBinding{ key: "license", value: eff_opts.license };
    var_buf[5] = ScaffoldVarBinding{ key: "entry", value: eff_opts.entry };
    var_buf[6] = ScaffoldVarBinding{ key: "year", value: eff_opts.year };
    let var_count: usize = 7;

    let ei: usize = 0;
    while (ei < extra_vars.len && var_count < var_buf.len) {
        var_buf[var_count] = extra_vars[ei];
        var_count += 1;
        ei += 1;
    }

    let plan_idx: usize = 0;

    // 1. Plan `cap.json`
    const cap_json_exists = pathExistsInSlice(existing_files, "cap.json");
    const cap_json_action = resolveScaffoldConflict(true, cap_json_exists, eff_opts.conflict_policy);
    if (cap_json_action == ScaffoldFileAction.ConflictError) {
        return ScaffoldError.ConflictDetected;
    }

    const default_scripts: [2]CapManifestScript = [
        CapManifestScript{ name: "test", command: "cap test" },
        CapManifestScript{ name: "build", command: "cap build" },
    ];
    const ws_list: [1][]const u8 = ["packages/*"];
    const empty_ws: [0][]const u8 = [];
    const empty_deps: [0]CapManifestDep = [];

    const ws_slice = if (eff_opts.template == ScaffoldTemplateKind.WorkspaceMonorepo) ws_list[0..] else empty_ws[0..];
    const json_start = arena_pos.*;
    const json_len = formatCapManifestJson(
        eff_opts,
        empty_deps[0..],
        default_scripts[0..],
        ws_slice,
        content_arena[json_start..],
    )?;
    arena_pos.* += json_len;

    if (plan_idx >= planned_out.len) return ScaffoldError.SliceCapacityExceeded;
    planned_out[plan_idx] = ScaffoldPlannedFile{
        rel_path: "cap.json",
        action: cap_json_action,
        is_json: true,
        is_executable: false,
        rendered_content: content_arena[json_start .. json_start + json_len],
    };
    plan_idx += 1;
    recordActionSummary(&summary, cap_json_action, json_len);

    // 2. Plan template files
    let tpl_files: [16]ScaffoldTemplateFile = [ScaffoldTemplateFile.empty(); 16];
    const tpl_count = collectTemplateFiles(eff_opts, tpl_files[0..])?;

    let ti: usize = 0;
    while (ti < tpl_count) {
        const tf = tpl_files[ti];
        const exists = pathExistsInSlice(existing_files, tf.rel_path);
        const action = resolveScaffoldConflict(tf.is_json, exists, eff_opts.conflict_policy);
        if (action == ScaffoldFileAction.ConflictError) {
            return ScaffoldError.ConflictDetected;
        }

        const c_start = arena_pos.*;
        let diag = ScaffoldRenderDiag.empty();
        const c_len = renderScaffoldTemplate(
            tf.template_body,
            var_buf[0..var_count],
            ScaffoldRenderOptions.defaults(),
            content_arena[c_start..],
            &diag,
        )?;
        arena_pos.* += c_len;

        if (plan_idx >= planned_out.len) return ScaffoldError.SliceCapacityExceeded;
        planned_out[plan_idx] = ScaffoldPlannedFile{
            rel_path: tf.rel_path,
            action: action,
            is_json: tf.is_json,
            is_executable: tf.is_executable,
            rendered_content: content_arena[c_start .. c_start + c_len],
        };
        plan_idx += 1;
        recordActionSummary(&summary, action, c_len);
        ti += 1;
    }

    return summary;
}

fn recordActionSummary(summary: *ScaffoldPlanSummary, action: ScaffoldFileAction, byte_len: usize) {
    summary.total_files += 1;
    if (action == ScaffoldFileAction.Create) {
        summary.create_count += 1;
        summary.total_bytes += byte_len;
    } else if (action == ScaffoldFileAction.Overwrite) {
        summary.overwrite_count += 1;
        summary.total_bytes += byte_len;
    } else if (action == ScaffoldFileAction.Merge) {
        summary.merge_count += 1;
        summary.total_bytes += byte_len;
    } else if (action == ScaffoldFileAction.Skip) {
        summary.skip_count += 1;
    } else {
        summary.conflict_count += 1;
    }
}

/// Formats a human-readable scaffold plan report (`+ create`, `! overwrite`, `~ merge`, `- skip`).
pub fn formatScaffoldPlanReport(
    summary: ScaffoldPlanSummary,
    planned: []const ScaffoldPlannedFile,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "scaffold ")?;
    appendBytes(out_buf, &pos, summary.project_name)?;
    appendBytes(out_buf, &pos, " (template=")?;
    appendBytes(out_buf, &pos, scaffoldTemplateName(summary.template))?;
    appendBytes(out_buf, &pos, " files=")?;
    appendU32Decimal(out_buf, &pos, summary.total_files)?;
    appendBytes(out_buf, &pos, " create=")?;
    appendU32Decimal(out_buf, &pos, summary.create_count)?;
    appendBytes(out_buf, &pos, " skip=")?;
    appendU32Decimal(out_buf, &pos, summary.skip_count)?;
    appendBytes(out_buf, &pos, " merge=")?;
    appendU32Decimal(out_buf, &pos, summary.merge_count)?;
    appendBytes(out_buf, &pos, ")\n")?;

    let i: usize = 0;
    while (i < planned.len) {
        const pf = planned[i];
        if (pf.action == ScaffoldFileAction.Create) {
            appendBytes(out_buf, &pos, "  + create ")?;
        } else if (pf.action == ScaffoldFileAction.Overwrite) {
            appendBytes(out_buf, &pos, "  ! overwrite ")?;
        } else if (pf.action == ScaffoldFileAction.Merge) {
            appendBytes(out_buf, &pos, "  ~ merge ")?;
        } else {
            appendBytes(out_buf, &pos, "  - skip ")?;
        }
        appendBytes(out_buf, &pos, pf.rel_path)?;
        appendByte(out_buf, &pos, @as(u8, 10))?;
        i += 1;
    }
    return pos;
}
