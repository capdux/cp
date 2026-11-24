// Source: Absorbed from /data/dev/bun/src/cli/create_command.cp | Synced: 2026-11-23T20:18:24Z
// ============================================================================
// lib/cli/createTemplate.cp — Zero-Alloc `cap create` Template & Workflow Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven template routing, destination hygiene,
// copy-filter/rename rules, and post-create workflow planner for `cap create`:
//   1. Multi-source template specifier parser (`parseCreateTemplateSpec`)
//      classifying `OfficialBuiltin` (`app`/`agent`/`media`/`web`/`lib`/`cli`/
//      `api`), `GitHubRepo` (`owner/repo[#ref]`, `owner/repo/subpath#ref`,
//      `github:owner/repo`, `https://github.com/owner/repo`), `NpmOrCapPackage`
//      (with automatic `create-` prefix normalization), `LocalPath`, and
//      `TarballUrl` (`https://.../*.tgz`), while blocking path traversal,
//      malformed hostnames, and shell metacharacter injection;
//   2. Destination directory hygiene & conflict evaluator
//      (`evaluateCreateDestination`) permitting safe ignored entries (`.git`,
//      `.DS_Store`, `LICENSE`, `README.md`), `--force` overrides, and safe
//      default rejection of non-empty directories;
//   3. Template file skip/rename rules (`shouldSkipTemplateFile`,
//      `resolveTemplateDestFilename`) and post-create workflow planner
//      (`planCreateWorkflow`);
//   4. Formatted next-steps onboarding banner (`formatCreateNextSteps`).

/// Default branch ref used for GitHub templates when no `#ref` is specified.
pub const CREATE_DEFAULT_GIT_REF: []const u8 = "main";

/// Maximum accepted template specifier length.
pub const CREATE_MAX_SPEC_LEN: usize = 512;

/// Template parsing, destination validation, and workflow planning error set.
pub err CreateTemplateError {
    EmptyTemplateSpec,
    SpecTooLong,
    UnsafeShellMetachar,
    PathTraversalDetected,
    InvalidGitHubSpec,
    InvalidTarballUrl,
    InvalidPackageSpec,
    DestinationNotEmpty,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, Path & Security Helpers
// ============================================================================

fn ctBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn ctAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn ctBytesEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (ctAsciiLower(a[i]) != ctAsciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn ctStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return ctBytesEql(s[0..prefix.len], prefix);
}

fn ctEndsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    return ctBytesEql(s[s.len - suffix.len..s.len], suffix);
}

fn ctIndexOf(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn ctLastIndexOf(s: []const u8, ch: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) return i;
    }
    return null;
}

fn ctIsAlphaNum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn ctAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ctAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ctAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ctAppendByte(out, pos, '0')?;
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
        ctAppendByte(out, pos, tmp[n])?;
    }
}

/// Returns true if `s` contains shell command-injection metacharacters or control bytes.
pub fn hasCreateShellMetachar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F) return true;
        if (c == ';' || c == '|' || c == '&' || c == '$' || c == '`' ||
            c == '<' || c == '>' || c == '"' || c == '\'' || c == '\\') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn ctContainsDotDotSegment(path: []const u8): bool {
    let i: usize = 0;
    while (i < path.len) {
        const at_start = (i == 0) || path[i - 1] == '/' || path[i - 1] == '\\';
        if (at_start && i + 1 < path.len && path[i] == '.' && path[i + 1] == '.') {
            const at_end = (i + 2 == path.len) || path[i + 2] == '/' || path[i + 2] == '\\';
            if (at_end) return true;
        }
        i += 1;
    }
    return false;
}

fn ctIsValidGitHubIdent(s: []const u8): bool {
    if (s.len == 0 || s.len > 100) return false;
    if (s[0] == '.' || s[s.len - 1] == '.' || s[0] == '-') return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!ctIsAlphaNum(c) && c != '-' && c != '_' && c != '.') return false;
        if (c == '.' && i + 1 < s.len && s[i + 1] == '.') return false;
        i += 1;
    }
    return true;
}

fn ctIsValidHostname(host: []const u8): bool {
    if (host.len == 0 || host.len > 253) return false;
    if (host[0] == '.' || host[host.len - 1] == '.' || host[0] == '-' || host[host.len - 1] == '-') {
        return false;
    }
    let has_dot = false;
    let i: usize = 0;
    while (i < host.len) {
        const c = host[i];
        if (c == '.') {
            has_dot = true;
            if (i + 1 < host.len && host[i + 1] == '.') return false;
        } else if (!ctIsAlphaNum(c) && c != '-' && c != ':') {
            return false;
        }
        i += 1;
    }
    return has_dot || ctBytesEqlIgnoreCase(host, "localhost");
}

fn ctBasename(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    if (slash := ctLastIndexOf(path[0..end], '/')) {
        return path[slash + 1..end];
    }
    return path[0..end];
}

// ============================================================================
// Section 1: Multi-Source Template Specifier Parser
// ============================================================================

/// The five supported `cap create` template source categories.
pub enum CreateSourceKind: u8 {
    OfficialBuiltin = 0,
    GitHubRepo = 1,
    NpmOrCapPackage = 2,
    LocalPath = 3,
    TarballUrl = 4,

    fn asName(): []const u8 {
        return switch (_) {
            .OfficialBuiltin => "builtin",
            .GitHubRepo => "github",
            .NpmOrCapPackage => "package",
            .LocalPath => "local",
            .TarballUrl => "tarball",
        };
    }
}

/// Normalized template specification returned by `parseCreateTemplateSpec`.
pub struct CreateTemplateSpec {
    kind: CreateSourceKind = CreateSourceKind.OfficialBuiltin,
    raw_input: []const u8 = "",
    builtin_name: []const u8 = "",
    github_owner: []const u8 = "",
    github_repo: []const u8 = "",
    github_ref: []const u8 = "",
    github_subpath: []const u8 = "",
    package_name: []const u8 = "",
    package_version: []const u8 = "",
    local_path: []const u8 = "",
    tarball_url: []const u8 = "",
    default_project_name: []const u8 = "",

    static fn empty(): CreateTemplateSpec {
        return CreateTemplateSpec{};
    }
}

/// Returns true if `name` is one of the official built-in `cap create` templates.
pub fn isOfficialBuiltinTemplate(name: []const u8): bool {
    return ctBytesEqlIgnoreCase(name, "app") ||
        ctBytesEqlIgnoreCase(name, "agent") ||
        ctBytesEqlIgnoreCase(name, "media") ||
        ctBytesEqlIgnoreCase(name, "web") ||
        ctBytesEqlIgnoreCase(name, "lib") ||
        ctBytesEqlIgnoreCase(name, "cli") ||
        ctBytesEqlIgnoreCase(name, "api");
}

/// Normalizes a package template name by inserting the `create-` prefix per npm/Bun rules:
///   - `vite` -> `create-vite`
///   - `create-vite` -> `create-vite`
///   - `@org` -> `@org/create`
///   - `@org/app` -> `@org/create-app`
///   - `@org/create-app` -> `@org/create-app`
pub fn normalizeCreatePackageName(raw: []const u8, out: []u8): usize! {
    if (raw.len == 0) return err.InvalidPackageSpec;
    if (hasCreateShellMetachar(raw) || ctContainsDotDotSegment(raw)) return err.InvalidPackageSpec;

    let pos: usize = 0;
    if (raw[0] == '@') {
        if (slash := ctIndexOf(raw, '/')) {
            const scope = raw[0..slash];
            const pkg = raw[slash + 1..raw.len];
            if (scope.len < 2 || pkg.len == 0) return err.InvalidPackageSpec;
            ctAppendBytes(out, &pos, scope)?;
            ctAppendByte(out, &pos, '/')?;
            if (ctStartsWith(pkg, "create-")) {
                ctAppendBytes(out, &pos, pkg)?;
            } else {
                ctAppendBytes(out, &pos, "create-")?;
                ctAppendBytes(out, &pos, pkg)?;
            }
            return pos;
        }
        // Bare `@org` -> `@org/create`
        if (raw.len < 2) return err.InvalidPackageSpec;
        ctAppendBytes(out, &pos, raw)?;
        ctAppendBytes(out, &pos, "/create")?;
        return pos;
    }

    if (ctStartsWith(raw, "create-")) {
        ctAppendBytes(out, &pos, raw)?;
    } else {
        ctAppendBytes(out, &pos, "create-")?;
        ctAppendBytes(out, &pos, raw)?;
    }
    return pos;
}

fn parseGitHubPathSpec(raw_input: []const u8, gh_body: []const u8): CreateTemplateSpec! {
    // Split optional `#ref`
    let path_part = gh_body;
    let git_ref: []const u8 = "";
    if (hash_idx := ctIndexOf(gh_body, '#')) {
        path_part = gh_body[0..hash_idx];
        git_ref = gh_body[hash_idx + 1..gh_body.len];
        if (git_ref.len == 0 || ctContainsDotDotSegment(git_ref)) return err.InvalidGitHubSpec;
    }

    // Strip optional trailing slash
    while (path_part.len > 0 && path_part[path_part.len - 1] == '/') {
        path_part = path_part[0..path_part.len - 1];
    }

    const s1 = ctIndexOf(path_part, '/') ?? return err.InvalidGitHubSpec;
    const owner = path_part[0..s1];
    const rest = path_part[s1 + 1..path_part.len];

    let repo = rest;
    let subpath: []const u8 = "";
    if (s2 := ctIndexOf(rest, '/')) {
        repo = rest[0..s2];
        subpath = rest[s2 + 1..rest.len];
    }

    // Strip `.git` suffix from repo if present
    if (ctEndsWith(repo, ".git") && repo.len > 4) {
        repo = repo[0..repo.len - 4];
    }

    // Handle `tree/<ref>/<subpath>` from web GitHub URLs
    if (ctStartsWith(subpath, "tree/")) {
        const after_tree = subpath[5..subpath.len];
        if (s3 := ctIndexOf(after_tree, '/')) {
            if (git_ref.len == 0) git_ref = after_tree[0..s3];
            subpath = after_tree[s3 + 1..after_tree.len];
        } else {
            if (git_ref.len == 0) git_ref = after_tree;
            subpath = "";
        }
    }

    if (!ctIsValidGitHubIdent(owner) || !ctIsValidGitHubIdent(repo)) {
        return err.InvalidGitHubSpec;
    }
    if (subpath.len > 0 && ctContainsDotDotSegment(subpath)) {
        return err.PathTraversalDetected;
    }

    const default_name = if (subpath.len > 0) ctBasename(subpath) else repo;
    return CreateTemplateSpec{
        kind: CreateSourceKind.GitHubRepo,
        raw_input: raw_input,
        github_owner: owner,
        github_repo: repo,
        github_ref: git_ref,
        github_subpath: subpath,
        default_project_name: default_name,
    };
}

/// Parses and validates a `cap create <template>` specifier into one of the 5
/// canonical source kinds, using caller-owned `norm_buf` for normalized package names.
pub fn parseCreateTemplateSpec(
    raw_spec: []const u8,
    norm_buf: []u8,
): CreateTemplateSpec! {
    if (raw_spec.len == 0) return err.EmptyTemplateSpec;
    if (raw_spec.len > CREATE_MAX_SPEC_LEN) return err.SpecTooLong;
    if (hasCreateShellMetachar(raw_spec)) return err.UnsafeShellMetachar;

    // 1. HTTPS / HTTP URLs -> GitHubRepo or TarballUrl
    if (ctStartsWith(raw_spec, "https://") || ctStartsWith(raw_spec, "http://")) {
        const is_https = ctStartsWith(raw_spec, "https://");
        const after_scheme = if (is_https) raw_spec[8..raw_spec.len] else raw_spec[7..raw_spec.len];
        const slash_idx = ctIndexOf(after_scheme, '/') ?? return err.InvalidTarballUrl;
        const host = after_scheme[0..slash_idx];
        const url_path = after_scheme[slash_idx + 1..after_scheme.len];

        if (!ctIsValidHostname(host)) return err.InvalidTarballUrl;
        if (ctContainsDotDotSegment(url_path)) return err.PathTraversalDetected;

        if (ctBytesEqlIgnoreCase(host, "github.com") && !ctEndsWith(url_path, ".tgz") && !ctEndsWith(url_path, ".tar.gz")) {
            return parseGitHubPathSpec(raw_spec, url_path);
        }

        if (!is_https) return err.InvalidTarballUrl;
        if (!ctEndsWith(url_path, ".tgz") && !ctEndsWith(url_path, ".tar.gz")) {
            return err.InvalidTarballUrl;
        }

        let base = ctBasename(url_path);
        if (ctEndsWith(base, ".tar.gz") && base.len > 7) {
            base = base[0..base.len - 7];
        } else if (ctEndsWith(base, ".tgz") && base.len > 4) {
            base = base[0..base.len - 4];
        }
        return CreateTemplateSpec{
            kind: CreateSourceKind.TarballUrl,
            raw_input: raw_spec,
            tarball_url: raw_spec,
            default_project_name: base,
        };
    }

    // 2. Explicit `github:owner/repo[#ref]`
    if (ctStartsWith(raw_spec, "github:")) {
        return parseGitHubPathSpec(raw_spec, raw_spec[7..raw_spec.len]);
    }

    // 3. Local filesystem template path (`./...`, `../...`, `/...`)
    if (ctStartsWith(raw_spec, "./") || ctStartsWith(raw_spec, "../") || raw_spec[0] == '/') {
        const base = ctBasename(raw_spec);
        return CreateTemplateSpec{
            kind: CreateSourceKind.LocalPath,
            raw_input: raw_spec,
            local_path: raw_spec,
            default_project_name: if (base.len > 0) base else "my-app",
        };
    }

    // 4. Official built-in templates (`app`, `agent`, `media`, `web`, `lib`, `cli`, `api`)
    if (isOfficialBuiltinTemplate(raw_spec)) {
        return CreateTemplateSpec{
            kind: CreateSourceKind.OfficialBuiltin,
            raw_input: raw_spec,
            builtin_name: raw_spec,
            default_project_name: raw_spec,
        };
    }

    // 5. Scoped npm/cap package (`@org/app[@ver]`, `@org/create-app`)
    if (raw_spec[0] == '@') {
        let name_part = raw_spec;
        let ver_part: []const u8 = "";
        if (at_idx := ctLastIndexOf(raw_spec[1..raw_spec.len], '@')) {
            const abs_at = at_idx + 1;
            name_part = raw_spec[0..abs_at];
            ver_part = raw_spec[abs_at + 1..raw_spec.len];
            if (ver_part.len == 0) return err.InvalidPackageSpec;
        }
        const nlen = normalizeCreatePackageName(name_part, norm_buf)?;
        const norm_name = norm_buf[0..nlen];
        return CreateTemplateSpec{
            kind: CreateSourceKind.NpmOrCapPackage,
            raw_input: raw_spec,
            package_name: norm_name,
            package_version: ver_part,
            default_project_name: ctBasename(name_part),
        };
    }

    // 6. GitHub shorthand `owner/repo[/subpath][#ref]`
    if (ctIndexOf(raw_spec, '/') != null) {
        return parseGitHubPathSpec(raw_spec, raw_spec);
    }

    // 7. Unscoped npm/cap package (`vite`, `create-vite@latest`)
    let name_part = raw_spec;
    let ver_part: []const u8 = "";
    if (at_idx := ctIndexOf(raw_spec, '@')) {
        name_part = raw_spec[0..at_idx];
        ver_part = raw_spec[at_idx + 1..raw_spec.len];
        if (name_part.len == 0 || ver_part.len == 0) return err.InvalidPackageSpec;
    }
    const nlen = normalizeCreatePackageName(name_part, norm_buf)?;
    return CreateTemplateSpec{
        kind: CreateSourceKind.NpmOrCapPackage,
        raw_input: raw_spec,
        package_name: norm_buf[0..nlen],
        package_version: ver_part,
        default_project_name: name_part,
    };
}

// ============================================================================
// Section 2: Destination Directory Hygiene & Conflict Guard
// ============================================================================

/// Outcome classification for `evaluateCreateDestination`.
pub enum CreateDestStatus: u8 {
    CleanEmpty = 0,
    CleanOnlyIgnored = 1,
    ForceOverwriteAllowed = 2,
    BlockedNonEmpty = 3,
    BlockedUnsafePath = 4,
}

/// Destination safety and conflict evaluation decision.
pub struct CreateDestDecision {
    allowed: bool = false,
    status: CreateDestStatus = CreateDestStatus.BlockedNonEmpty,
    ignored_entry_count: u32 = 0,
    conflict_entry_count: u32 = 0,
    first_conflict_entry: []const u8 = "",
}

/// Returns true if a destination directory entry is harmless and ignored during emptiness checks.
pub fn isSafeIgnoredDestEntry(entry_name: []const u8): bool {
    return ctBytesEql(entry_name, ".git") ||
        ctBytesEql(entry_name, ".gitignore") ||
        ctBytesEql(entry_name, ".DS_Store") ||
        ctBytesEql(entry_name, "Thumbs.db") ||
        ctBytesEql(entry_name, ".vscode") ||
        ctBytesEql(entry_name, ".idea") ||
        ctBytesEqlIgnoreCase(entry_name, "LICENSE") ||
        ctBytesEqlIgnoreCase(entry_name, "LICENSE.md") ||
        ctBytesEqlIgnoreCase(entry_name, "LICENSE.txt") ||
        ctBytesEqlIgnoreCase(entry_name, "README.md");
}

/// Returns true if `dest_path` is safe to create/write into (no `..` traversal or shell metachars).
pub fn isSafeCreateDestPath(dest_path: []const u8): bool {
    if (dest_path.len == 0) return false;
    if (hasCreateShellMetachar(dest_path)) return false;
    if (ctContainsDotDotSegment(dest_path)) return false;
    return true;
}

/// Evaluates whether `cap create` may safely materialize a template into `dest_path`.
pub fn evaluateCreateDestination(
    dest_path: []const u8,
    existing_entries: []const []const u8,
    force_overwrite: bool,
): CreateDestDecision {
    if (!isSafeCreateDestPath(dest_path)) {
        return CreateDestDecision{
            allowed: false,
            status: CreateDestStatus.BlockedUnsafePath,
        };
    }

    if (existing_entries.len == 0) {
        return CreateDestDecision{
            allowed: true,
            status: CreateDestStatus.CleanEmpty,
        };
    }

    let ignored: u32 = 0;
    let conflicts: u32 = 0;
    let first_conflict: []const u8 = "";

    let i: usize = 0;
    while (i < existing_entries.len) {
        if (isSafeIgnoredDestEntry(existing_entries[i])) {
            ignored += 1;
        } else {
            if (conflicts == 0) first_conflict = existing_entries[i];
            conflicts += 1;
        }
        i += 1;
    }

    if (conflicts == 0) {
        return CreateDestDecision{
            allowed: true,
            status: CreateDestStatus.CleanOnlyIgnored,
            ignored_entry_count: ignored,
        };
    }

    if (force_overwrite) {
        return CreateDestDecision{
            allowed: true,
            status: CreateDestStatus.ForceOverwriteAllowed,
            ignored_entry_count: ignored,
            conflict_entry_count: conflicts,
            first_conflict_entry: first_conflict,
        };
    }

    return CreateDestDecision{
        allowed: false,
        status: CreateDestStatus.BlockedNonEmpty,
        ignored_entry_count: ignored,
        conflict_entry_count: conflicts,
        first_conflict_entry: first_conflict,
    };
}

// ============================================================================
// Section 3: Template Copy Filters, Rename Rules & Post-Create Workflow Planner
// ============================================================================

/// Ordered workflow phase kinds produced by `planCreateWorkflow`.
pub enum CreateStepKind: u8 {
    FetchTemplate = 0,
    CopyAndTransformFiles = 1,
    InstallDependencies = 2,
    InitGitRepository = 3,
    RunPostCreateScript = 4,
}

/// Single planned step in a `cap create` execution workflow.
pub struct CreateWorkflowStep {
    kind: CreateStepKind = CreateStepKind.FetchTemplate,
    label: []const u8 = "",
    command: []const u8 = "",
    optional_step: bool = false,

    static fn empty(): CreateWorkflowStep {
        return CreateWorkflowStep{};
    }
}

/// Metadata extracted from a template's manifest (`post_create` hooks).
pub struct CreatePostMetadata {
    install_deps: bool = true,
    git_init: bool = true,
    post_create_script: []const u8 = "",
    dev_script_name: []const u8 = "dev",

    static fn defaults(): CreatePostMetadata {
        return CreatePostMetadata{};
    }
}

/// CLI flags controlling `planCreateWorkflow`.
pub struct CreateWorkflowOptions {
    no_install: bool = false,
    no_git: bool = false,
    force: bool = false,
    has_existing_git_dir: bool = false,

    static fn defaults(): CreateWorkflowOptions {
        return CreateWorkflowOptions{};
    }
}

/// Returns true if a template relative path should be skipped during copy
/// (`.git`, `node_modules`, `cap_modules`, lockfiles, `.DS_Store`).
pub fn shouldSkipTemplateFile(rel_path: []const u8): bool {
    if (rel_path.len == 0) return true;
    // Check leading or inner directory segments
    let start: usize = 0;
    while (start < rel_path.len) {
        let end = start;
        while (end < rel_path.len && rel_path[end] != '/' && rel_path[end] != '\\') {
            end += 1;
        }
        const seg = rel_path[start..end];
        if (ctBytesEql(seg, ".git") ||
            ctBytesEql(seg, "node_modules") ||
            ctBytesEql(seg, "cap_modules") ||
            ctBytesEql(seg, ".cap") ||
            ctBytesEql(seg, ".DS_Store")) {
            return true;
        }
        start = if (end < rel_path.len) end + 1 else rel_path.len;
    }

    const base = ctBasename(rel_path);
    if (ctBytesEql(base, "package-lock.json") ||
        ctBytesEql(base, "yarn.lock") ||
        ctBytesEql(base, "pnpm-lock.yaml") ||
        ctBytesEql(base, "bun.lockb") ||
        ctBytesEql(base, "bun.lock") ||
        ctBytesEql(base, "cap.lock")) {
        return true;
    }
    return false;
}

/// Transforms a template source relative path into its destination path:
///   - `gitignore` / `_gitignore` -> `.gitignore` (preserving parent directory)
///   - `_npmignore` -> `.npmignore`
///   - `_env.example` -> `.env.example`
///   - strips trailing `.tmpl` suffix (e.g. `cap.json.tmpl` -> `cap.json`)
pub fn resolveTemplateDestFilename(src_rel_path: []const u8, out: []u8): usize! {
    if (src_rel_path.len == 0) return err.EmptyTemplateSpec;
    if (ctContainsDotDotSegment(src_rel_path)) return err.PathTraversalDetected;

    let pos: usize = 0;
    let dir_prefix: []const u8 = "";
    let base = src_rel_path;

    if (slash := ctLastIndexOf(src_rel_path, '/')) {
        dir_prefix = src_rel_path[0..slash + 1];
        base = src_rel_path[slash + 1..src_rel_path.len];
    }

    if (dir_prefix.len > 0) {
        ctAppendBytes(out, &pos, dir_prefix)?;
    }

    if (ctBytesEql(base, "gitignore") || ctBytesEql(base, "_gitignore")) {
        ctAppendBytes(out, &pos, ".gitignore")?;
        return pos;
    }
    if (ctBytesEql(base, "_npmignore")) {
        ctAppendBytes(out, &pos, ".npmignore")?;
        return pos;
    }
    if (ctBytesEql(base, "_env.example")) {
        ctAppendBytes(out, &pos, ".env.example")?;
        return pos;
    }

    if (ctEndsWith(base, ".tmpl") && base.len > 5) {
        ctAppendBytes(out, &pos, base[0..base.len - 5])?;
        return pos;
    }

    ctAppendBytes(out, &pos, base)?;
    return pos;
}

/// Plans the ordered sequence of `cap create` workflow steps into caller-owned `out_steps`.
pub fn planCreateWorkflow(
    spec: CreateTemplateSpec,
    meta: CreatePostMetadata,
    opts: CreateWorkflowOptions,
    out_steps: []CreateWorkflowStep,
): usize! {
    let count: usize = 0;

    // 1. Fetch template (not required for OfficialBuiltin or LocalPath)
    if (spec.kind == CreateSourceKind.GitHubRepo ||
        spec.kind == CreateSourceKind.NpmOrCapPackage ||
        spec.kind == CreateSourceKind.TarballUrl) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        const lbl = switch (spec.kind) {
            .GitHubRepo => "Download GitHub template archive",
            .NpmOrCapPackage => "Resolve registry template package",
            .TarballUrl => "Download remote tarball template",
            else => "Fetch template",
        };
        out_steps[count] = CreateWorkflowStep{
            kind: CreateStepKind.FetchTemplate,
            label: lbl,
            command: spec.raw_input,
        };
        count += 1;
    }

    // 2. Copy & transform template files
    if (count >= out_steps.len) return err.SliceCapacityExceeded;
    out_steps[count] = CreateWorkflowStep{
        kind: CreateStepKind.CopyAndTransformFiles,
        label: "Materialize template files and rename dotfiles",
        command: "",
    };
    count += 1;

    // 3. Install dependencies
    if (meta.install_deps && !opts.no_install) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = CreateWorkflowStep{
            kind: CreateStepKind.InstallDependencies,
            label: "Install dependencies",
            command: "cap install",
        };
        count += 1;
    }

    // 4. Initialize git repository
    if (meta.git_init && !opts.no_git && !opts.has_existing_git_dir) {
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = CreateWorkflowStep{
            kind: CreateStepKind.InitGitRepository,
            label: "Initialize git repository",
            command: "git init",
            optional_step: true,
        };
        count += 1;
    }

    // 5. Optional post-create script hook
    if (meta.post_create_script.len > 0) {
        if (hasCreateShellMetachar(meta.post_create_script)) return err.UnsafeShellMetachar;
        if (count >= out_steps.len) return err.SliceCapacityExceeded;
        out_steps[count] = CreateWorkflowStep{
            kind: CreateStepKind.RunPostCreateScript,
            label: "Run template post-create hook",
            command: meta.post_create_script,
        };
        count += 1;
    }

    return count;
}

// ============================================================================
// Section 4: Onboarding Next-Steps Summary Renderer
// ============================================================================

/// Summary info rendered by `formatCreateNextSteps`.
pub struct CreateSummaryInfo {
    project_name: []const u8 = "",
    dest_dir: []const u8 = "",
    template_label: []const u8 = "",
    files_copied: u32 = 0,
    deps_installed: bool = false,
    git_initialized: bool = false,
    dev_script: []const u8 = "dev",
    is_current_dir: bool = false,
}

/// Renders a clean, actionable post-create onboarding guide into caller-owned `out`.
pub fn formatCreateNextSteps(
    info: CreateSummaryInfo,
    use_color: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;

    if (use_color) ctAppendBytes(out, &pos, "\x1b[32m")?;
    ctAppendBytes(out, &pos, "Created ")?;
    ctAppendBytes(out, &pos, info.project_name)?;
    if (use_color) ctAppendBytes(out, &pos, "\x1b[0m")?;

    if (info.template_label.len > 0) {
        ctAppendBytes(out, &pos, " from ")?;
        ctAppendBytes(out, &pos, info.template_label)?;
    }
    ctAppendBytes(out, &pos, " (")?;
    ctAppendU64(out, &pos, @as(u64, info.files_copied))?;
    ctAppendBytes(out, &pos, " file(s)")?;
    if (info.git_initialized) {
        ctAppendBytes(out, &pos, ", git initialized")?;
    }
    ctAppendBytes(out, &pos, ")\n\nNext steps:\n")?;

    if (!info.is_current_dir && info.dest_dir.len > 0 && !ctBytesEql(info.dest_dir, ".")) {
        ctAppendBytes(out, &pos, "  ")?;
        if (use_color) ctAppendBytes(out, &pos, "\x1b[36m")?;
        ctAppendBytes(out, &pos, "cd ")?;
        ctAppendBytes(out, &pos, info.dest_dir)?;
        if (use_color) ctAppendBytes(out, &pos, "\x1b[0m")?;
        ctAppendByte(out, &pos, '\n')?;
    }

    if (!info.deps_installed) {
        ctAppendBytes(out, &pos, "  ")?;
        if (use_color) ctAppendBytes(out, &pos, "\x1b[36m")?;
        ctAppendBytes(out, &pos, "cap install")?;
        if (use_color) ctAppendBytes(out, &pos, "\x1b[0m")?;
        ctAppendByte(out, &pos, '\n')?;
    }

    if (info.dev_script.len > 0) {
        ctAppendBytes(out, &pos, "  ")?;
        if (use_color) ctAppendBytes(out, &pos, "\x1b[36m")?;
        ctAppendBytes(out, &pos, "cap run ")?;
        ctAppendBytes(out, &pos, info.dev_script)?;
        if (use_color) ctAppendBytes(out, &pos, "\x1b[0m")?;
        ctAppendByte(out, &pos, '\n')?;
    }

    return pos;
}
