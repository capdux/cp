// Source: Absorbed from /data/dev/bun/src/install/PackageManager/updatePackageJSONAndInstall.cp | Synced: 2026-11-24T06:02:27Z
// ============================================================================
// lib/pkg/manifestUpdater.cp — Fidelity-Preserving Zero-Alloc Manifest Updater
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven `cap.json` / `package.json`
// dependency rewriting and synchronization engine for `cap add` / `cap remove` /
// `cap update`:
//   1. Format fidelity detector (`detectManifestFormat`) capturing indentation
//      (2-space / 4-space / Tab), line endings (LF / CRLF), colon spacing, and
//      trailing EOF newline so unrelated fields remain untouched;
//   2. Version specifier classifier & normalizer (`classifyVersionSpec`,
//      `normalizeVersionSpec`) supporting `--exact`, `^` (Caret), `~` (Tilde),
//      `workspace:*` / `workspace:^` / `workspace:~`, `npm:<alias>@<ver>`, and
//      `catalog:` references;
//   3. Cross-group atomic mutation operations (`ManifestMutationOp`: `Add`,
//      `Upgrade`, `Remove`, `MoveGroup`) across `dependencies`,
//      `devDependencies`, `optionalDependencies`, and `peerDependencies` with
//      automatic cross-group deduplication;
//   4. Lexicographically sorted zero-heap RFC 8259 JSON rewriter
//      (`applyManifestMutations`) that preserves unrelated top-level properties
//      verbatim, creates missing dependency groups in canonical order, prunes
//      emptied groups, and produces structured diff records + readable terminal
//      summary (`formatManifestUpdateReport`).

/// Number of standard dependency groups in `cap.json` / `package.json`.
pub const MANIFEST_UPDATER_GROUP_COUNT: usize = 4;

/// Default indentation width when input JSON is empty or single-line.
pub const MANIFEST_UPDATER_DEFAULT_INDENT: u8 = 2;

/// Manifest parsing, mutation, normalization, and JSON rewriting error set.
pub err ManifestUpdaterError {
    InvalidJson,
    RootNotObject,
    UnterminatedString,
    UnexpectedEof,
    InvalidPackageName,
    InvalidVersionSpec,
    EmptyMutationName,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, JSON Escape & Lexicographical Helpers
// ============================================================================

fn muBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn muBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn muStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return muBytesEql(s[0..prefix.len], prefix);
}

fn muLastIndexOf(s: []const u8, ch: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) return i;
    }
    return null;
}

fn muIsWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn muIsDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn muHasControlChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] < 0x20 || s[i] == 0x7F) return true;
        i += 1;
    }
    return false;
}

fn muIsValidPkgName(name: []const u8): bool {
    if (name.len == 0 || name.len > 214) return false;
    if (muHasControlChar(name)) return false;
    let i: usize = 0;
    while (i < name.len) {
        if (name[i] == ' ' || name[i] == '"' || name[i] == '\\') return false;
        i += 1;
    }
    return true;
}

fn muAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn muAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn muAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        muAppendByte(out, pos, '0')?;
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
        muAppendByte(out, pos, tmp[n])?;
    }
}

/// Emits a valid RFC 8259 JSON string literal with proper escaping.
fn muAppendJsonString(out: []u8, pos: *usize, s: []const u8)! {
    muAppendByte(out, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            muAppendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            muAppendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            muAppendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            muAppendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            muAppendBytes(out, pos, "\\t")?;
        } else {
            muAppendByte(out, pos, c)?;
        }
        i += 1;
    }
    muAppendByte(out, pos, '"')?;
}

// ============================================================================
// Section 1: Format Fidelity Detector
// ============================================================================

/// Indentation character style detected from original manifest JSON.
pub enum ManifestIndentKind: u8 {
    Spaces = 0,
    Tabs = 1,
}

/// Line ending style detected from original manifest JSON.
pub enum ManifestNewlineKind: u8 {
    Lf = 0,
    CrLf = 1,
}

/// Formatting profile extracted from an existing `cap.json` / `package.json`.
pub struct ManifestFormatProfile {
    indent_kind: ManifestIndentKind = ManifestIndentKind.Spaces,
    indent_width: u8 = MANIFEST_UPDATER_DEFAULT_INDENT,
    newline_kind: ManifestNewlineKind = ManifestNewlineKind.Lf,
    has_trailing_newline: bool = true,
    space_after_colon: bool = true,
    is_single_line: bool = false,

    static fn defaults(): ManifestFormatProfile {
        return ManifestFormatProfile{};
    }

    fn newlineBytes(): []const u8 {
        return switch (_.newline_kind) {
            .Lf => "\n",
            .CrLf => "\r\n",
        };
    }

    fn writeIndent(level: usize, out: []u8, pos: *usize)! {
        const unit: u8 = switch (_.indent_kind) {
            .Spaces => ' ',
            .Tabs => '\t',
        };
        const total = level * @as(usize, _.indent_width);
        let i: usize = 0;
        while (i < total) {
            muAppendByte(out, pos, unit)?;
            i += 1;
        }
    }
}

/// Inspects raw JSON source to detect indentation, line endings, colon spacing,
/// and trailing EOF newline without allocating memory.
pub fn detectManifestFormat(json_src: []const u8): ManifestFormatProfile {
    let profile = ManifestFormatProfile.defaults();
    if (json_src.len == 0) return profile;

    profile.has_trailing_newline = (json_src[json_src.len - 1] == '\n');

    // Scan for first newline to detect LF vs CRLF and indentation of first property
    let i: usize = 0;
    let found_nl = false;
    while (i < json_src.len) {
        if (json_src[i] == '\n') {
            found_nl = true;
            if (i > 0 && json_src[i - 1] == '\r') {
                profile.newline_kind = ManifestNewlineKind.CrLf;
            } else {
                profile.newline_kind = ManifestNewlineKind.Lf;
            }
            break;
        }
        i += 1;
    }

    if (!found_nl) {
        profile.is_single_line = true;
        profile.has_trailing_newline = false;
        return profile;
    }

    // Look for the first line after opening '{' that begins with whitespace + '"'
    let p: usize = 0;
    while (p < json_src.len && json_src[p] != '{') {
        p += 1;
    }
    if (p < json_src.len) p += 1;

    let detected_indent = false;
    while (p < json_src.len) {
        if (json_src[p] == '\n') {
            p += 1;
            let tabs: u8 = 0;
            let spaces: u8 = 0;
            while (p < json_src.len && (json_src[p] == ' ' || json_src[p] == '\t')) {
                if (json_src[p] == '\t') {
                    if (tabs < 8) tabs += 1;
                } else {
                    if (spaces < 8) spaces += 1;
                }
                p += 1;
            }
            if (p < json_src.len && json_src[p] == '"') {
                if (tabs > 0) {
                    profile.indent_kind = ManifestIndentKind.Tabs;
                    profile.indent_width = tabs;
                    detected_indent = true;
                } else if (spaces > 0) {
                    profile.indent_kind = ManifestIndentKind.Spaces;
                    profile.indent_width = spaces;
                    detected_indent = true;
                }
                // Check colon spacing on this same property line
                p += 1;
                while (p < json_src.len && json_src[p] != '"') {
                    if (json_src[p] == '\\' && p + 1 < json_src.len) {
                        p += 2;
                    } else {
                        p += 1;
                    }
                }
                if (p < json_src.len) p += 1;
                while (p < json_src.len && (json_src[p] == ' ' || json_src[p] == '\t')) {
                    p += 1;
                }
                if (p < json_src.len && json_src[p] == ':') {
                    if (p + 1 < json_src.len && json_src[p + 1] == ' ') {
                        profile.space_after_colon = true;
                    } else {
                        profile.space_after_colon = false;
                    }
                }
                break;
            }
        } else {
            p += 1;
        }
    }

    * = detected_indent;
    return profile;
}

// ============================================================================
// Section 2: Version Specifier Classification & Normalization
// ============================================================================

/// Policy controlling how resolved or user-supplied versions are formatted.
pub enum ManifestVersionStyle: u8 {
    Caret = 0,
    Tilde = 1,
    Exact = 2,
    WorkspaceStar = 3,
    WorkspaceCaret = 4,
    WorkspaceTilde = 5,
    CatalogDefault = 6,
    Preserve = 7,
}

/// High-level protocol/syntax classification of a dependency version specifier.
pub enum ManifestVersionKind: u8 {
    Semver = 0,
    WorkspaceProtocol = 1,
    NpmAlias = 2,
    CatalogProtocol = 3,
    TagOrRange = 4,
}

fn muLooksLikeSemverCore(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    if (s[i] == '^' || s[i] == '~' || s[i] == '=' || s[i] == 'v' || s[i] == 'V') {
        i += 1;
    }
    if (i >= s.len || !muIsDigit(s[i])) return false;
    let dots: u8 = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '.') {
            dots += 1;
        } else if (c == '-' || c == '+') {
            break;
        } else if (!muIsDigit(c)) {
            return false;
        }
        i += 1;
    }
    return dots >= 1;
}

fn muStripSemverPrefix(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == '^' || s[start] == '~' || s[start] == '=' || s[start] == 'v' || s[start] == 'V')) {
        start += 1;
    }
    return s[start..s.len];
}

/// Classifies a dependency version specifier string.
pub fn classifyVersionSpec(spec: []const u8): ManifestVersionKind {
    if (muStartsWith(spec, "workspace:")) return ManifestVersionKind.WorkspaceProtocol;
    if (muStartsWith(spec, "npm:")) return ManifestVersionKind.NpmAlias;
    if (muStartsWith(spec, "catalog:")) return ManifestVersionKind.CatalogProtocol;
    if (muLooksLikeSemverCore(spec)) return ManifestVersionKind.Semver;
    return ManifestVersionKind.TagOrRange;
}

fn writeStyledSemver(raw: []const u8, style: ManifestVersionStyle, out: []u8, pos: *usize)! {
    if (style == ManifestVersionStyle.Preserve || !muLooksLikeSemverCore(raw)) {
        muAppendBytes(out, pos, raw)?;
        return;
    }
    const core = muStripSemverPrefix(raw);
    switch (style) {
        .Caret => {
            muAppendByte(out, pos, '^')?;
            muAppendBytes(out, pos, core)?;
        },
        .Tilde => {
            muAppendByte(out, pos, '~')?;
            muAppendBytes(out, pos, core)?;
        },
        .Exact => {
            muAppendBytes(out, pos, core)?;
        },
        else => {
            muAppendBytes(out, pos, raw)?;
        },
    }
}

/// Normalizes a version specifier into caller-provided `out` buffer according to
/// `style` (`--exact`, `^`, `~`, `workspace:*`, `workspace:^`, `workspace:~`,
/// `catalog:`, `npm:<alias>@<ver>`). Returns bytes written.
pub fn normalizeVersionSpec(
    raw_version: []const u8,
    style: ManifestVersionStyle,
    catalog_name: []const u8,
    npm_alias_target: []const u8,
    out: []u8,
): usize! {
    let pos: usize = 0;

    // 1. Explicit catalog protocol style
    if (style == ManifestVersionStyle.CatalogDefault) {
        if (muHasControlChar(catalog_name)) return err.InvalidVersionSpec;
        muAppendBytes(out, &pos, "catalog:")?;
        if (catalog_name.len > 0) {
            muAppendBytes(out, &pos, catalog_name)?;
        }
        return pos;
    }

    // 2. Explicit workspace protocol styles
    if (style == ManifestVersionStyle.WorkspaceStar) {
        muAppendBytes(out, &pos, "workspace:*")?;
        return pos;
    }
    if (style == ManifestVersionStyle.WorkspaceCaret) {
        muAppendBytes(out, &pos, "workspace:^")?;
        return pos;
    }
    if (style == ManifestVersionStyle.WorkspaceTilde) {
        muAppendBytes(out, &pos, "workspace:~")?;
        return pos;
    }

    if (raw_version.len == 0 || muHasControlChar(raw_version)) {
        return err.InvalidVersionSpec;
    }

    // 3. Explicit npm alias target (`npm:<alias>@<styled_ver>`)
    if (npm_alias_target.len > 0) {
        if (!muIsValidPkgName(npm_alias_target)) return err.InvalidPackageName;
        muAppendBytes(out, &pos, "npm:")?;
        muAppendBytes(out, &pos, npm_alias_target)?;
        muAppendByte(out, &pos, '@')?;
        writeStyledSemver(raw_version, style, out, &pos)?;
        return pos;
    }

    // 4. Existing `npm:<alias>@<ver>` specifier with style re-application
    if (muStartsWith(raw_version, "npm:")) {
        const rest = raw_version[4..raw_version.len];
        if (at_idx := muLastIndexOf(rest, '@')) {
            if (at_idx > 0 && at_idx + 1 < rest.len) {
                muAppendBytes(out, &pos, "npm:")?;
                muAppendBytes(out, &pos, rest[0..at_idx])?;
                muAppendByte(out, &pos, '@')?;
                writeStyledSemver(rest[at_idx + 1..rest.len], style, out, &pos)?;
                return pos;
            }
        }
        muAppendBytes(out, &pos, raw_version)?;
        return pos;
    }

    // 5. Existing `workspace:` / `catalog:` / URL / file / git protocols preserved verbatim
    if (muStartsWith(raw_version, "workspace:") ||
        muStartsWith(raw_version, "catalog:") ||
        muStartsWith(raw_version, "file:") ||
        muStartsWith(raw_version, "link:") ||
        muStartsWith(raw_version, "git+") ||
        muStartsWith(raw_version, "http://") ||
        muStartsWith(raw_version, "https://")) {
        muAppendBytes(out, &pos, raw_version)?;
        return pos;
    }

    // 6. Standard SemVer / tag styling
    writeStyledSemver(raw_version, style, out, &pos)?;
    return pos;
}

// ============================================================================
// Section 3: Cross-Group Atomic Operations & Diff Records
// ============================================================================

/// The four standard dependency groups in `cap.json` / `package.json`.
pub enum ManifestDepGroup: u8 {
    Dependencies = 0,
    DevDependencies = 1,
    OptionalDependencies = 2,
    PeerDependencies = 3,

    fn asJsonKey(): []const u8 {
        return switch (_) {
            .Dependencies => "dependencies",
            .DevDependencies => "devDependencies",
            .OptionalDependencies => "optionalDependencies",
            .PeerDependencies => "peerDependencies",
        };
    }

    fn index(): usize {
        return switch (_) {
            .Dependencies => 0,
            .DevDependencies => 1,
            .OptionalDependencies => 2,
            .PeerDependencies => 3,
        };
    }

    static fn fromIndex(idx: usize): ManifestDepGroup? {
        return switch (idx) {
            0 => ManifestDepGroup.Dependencies,
            1 => ManifestDepGroup.DevDependencies,
            2 => ManifestDepGroup.OptionalDependencies,
            3 => ManifestDepGroup.PeerDependencies,
            else => null,
        };
    }

    static fn fromJsonKey(raw_key: []const u8): ManifestDepGroup? {
        if (muBytesEql(raw_key, "dependencies")) return ManifestDepGroup.Dependencies;
        if (muBytesEql(raw_key, "devDependencies")) return ManifestDepGroup.DevDependencies;
        if (muBytesEql(raw_key, "optionalDependencies")) return ManifestDepGroup.OptionalDependencies;
        if (muBytesEql(raw_key, "peerDependencies")) return ManifestDepGroup.PeerDependencies;
        return null;
    }
}

/// Atomic mutation operation applied to a manifest dependency graph.
pub enum ManifestMutationKind: u8 {
    Add = 0,
    Upgrade = 1,
    Remove = 2,
    MoveGroup = 3,
}

/// Single caller-requested dependency mutation for `applyManifestMutations`.
pub struct ManifestMutationOp {
    kind: ManifestMutationKind = ManifestMutationKind.Add,
    package_name: []const u8 = "",
    version_spec: []const u8 = "",
    target_group: ManifestDepGroup = ManifestDepGroup.Dependencies,
    version_style: ManifestVersionStyle = ManifestVersionStyle.Preserve,
    catalog_name: []const u8 = "",
    npm_alias_target: []const u8 = "",

    static fn empty(): ManifestMutationOp {
        return ManifestMutationOp{};
    }
}

/// Classification of the actual outcome for a single mutation operation.
pub enum ManifestChangeKind: u8 {
    Added = 0,
    Updated = 1,
    Moved = 2,
    Removed = 3,
    Unchanged = 4,
    NotFound = 5,
}

/// Structured diff record emitted for each processed mutation.
pub struct ManifestChangeRecord {
    kind: ManifestChangeKind = ManifestChangeKind.Unchanged,
    package_name: []const u8 = "",
    from_group: ManifestDepGroup = ManifestDepGroup.Dependencies,
    to_group: ManifestDepGroup = ManifestDepGroup.Dependencies,
    old_version: []const u8 = "",
    new_version: []const u8 = "",

    static fn empty(): ManifestChangeRecord {
        return ManifestChangeRecord{};
    }
}

/// Scratch entry representing a single parsed or mutated dependency in memory.
pub struct ManifestDepEntry {
    group: ManifestDepGroup = ManifestDepGroup.Dependencies,
    package_name: []const u8 = "",
    version_spec: []const u8 = "",
    active: bool = false,

    static fn empty(): ManifestDepEntry {
        return ManifestDepEntry{};
    }
}

/// Scratch entry representing a top-level property span in the original JSON.
pub struct ManifestTopPropSpan {
    key_unquoted: []const u8 = "",
    raw_key: []const u8 = "",
    raw_value: []const u8 = "",
    is_dep_group: bool = false,
    dep_group: ManifestDepGroup = ManifestDepGroup.Dependencies,

    static fn empty(): ManifestTopPropSpan {
        return ManifestTopPropSpan{};
    }
}

/// Options controlling `applyManifestMutations`.
pub struct ManifestUpdateOptions {
    remove_empty_groups: bool = true,
    sort_dependencies: bool = true,

    static fn defaults(): ManifestUpdateOptions {
        return ManifestUpdateOptions{};
    }
}

/// Caller-owned scratch buffers for zero-heap `applyManifestMutations`.
pub struct ManifestUpdateBuffers {
    top_props: []ManifestTopPropSpan,
    dep_entries: []ManifestDepEntry,
    spec_scratch: []u8,
    out_changes: []ManifestChangeRecord,
}

/// Aggregate result returned by `applyManifestMutations`.
pub struct ManifestUpdateSummary {
    bytes_written: usize = 0,
    added_count: u32 = 0,
    updated_count: u32 = 0,
    moved_count: u32 = 0,
    removed_count: u32 = 0,
    unchanged_count: u32 = 0,
    not_found_count: u32 = 0,
    change_record_count: usize = 0,
    has_mutations: bool = false,
    format: ManifestFormatProfile = ManifestFormatProfile{},
}

// ============================================================================
// Section 4: Strict Zero-Alloc RFC 8259 JSON Scanner & Fidelity Rewriter
// ============================================================================

fn skipJsonWhitespace(src: []const u8, pos: *usize) {
    while (pos.* < src.len && muIsWhitespace(src[pos.*])) {
        pos.* += 1;
    }
}

/// Scans a JSON string starting at `src[pos.*] == '"'`; advances `pos.*` past closing `'"'`.
/// Returns the inner unquoted slice (without surrounding quotes).
fn scanJsonString(src: []const u8, pos: *usize): ([]const u8)! {
    if (pos.* >= src.len || src[pos.*] != '"') return err.InvalidJson;
    const content_start = pos.* + 1;
    pos.* += 1;
    while (pos.* < src.len) {
        const c = src[pos.*];
        if (c == '\\') {
            if (pos.* + 1 >= src.len) return err.UnterminatedString;
            pos.* += 2;
        } else if (c == '"') {
            const inner = src[content_start..pos.*];
            pos.* += 1;
            return inner;
        } else {
            pos.* += 1;
        }
    }
    return err.UnterminatedString;
}

/// Skips any valid JSON value (object, array, string, number, bool, null) and advances `pos.*`.
fn skipJsonValue(src: []const u8, pos: *usize)! {
    skipJsonWhitespace(src, pos);
    if (pos.* >= src.len) return err.UnexpectedEof;

    const c = src[pos.*];
    if (c == '"') {
        * = scanJsonString(src, pos)?;
        return;
    }
    if (c == '{' || c == '[') {
        let depth: usize = 0;
        while (pos.* < src.len) {
            const ch = src[pos.*];
            if (ch == '"') {
                * = scanJsonString(src, pos)?;
                continue;
            }
            if (ch == '{' || ch == '[') {
                depth += 1;
            } else if (ch == '}' || ch == ']') {
                if (depth == 0) return err.InvalidJson;
                depth -= 1;
                pos.* += 1;
                if (depth == 0) return;
                continue;
            }
            pos.* += 1;
        }
        return err.UnexpectedEof;
    }

    // Primitive: number, true, false, null
    const start = pos.*;
    while (pos.* < src.len) {
        const ch = src[pos.*];
        if (muIsWhitespace(ch) || ch == ',' || ch == '}' || ch == ']') break;
        pos.* += 1;
    }
    if (pos.* == start) return err.InvalidJson;
}

/// Parses a `{ "pkg": "ver", ... }` dependency object into `dep_entries`.
fn parseDepGroupObject(
    obj_src: []const u8,
    group: ManifestDepGroup,
    dep_entries: []ManifestDepEntry,
    dep_count: *usize,
)! {
    let pos: usize = 0;
    skipJsonWhitespace(obj_src, &pos);
    if (pos >= obj_src.len || obj_src[pos] != '{') return err.InvalidJson;
    pos += 1;

    while (true) {
        skipJsonWhitespace(obj_src, &pos);
        if (pos >= obj_src.len) return err.UnexpectedEof;
        if (obj_src[pos] == '}') {
            pos += 1;
            break;
        }

        const key = scanJsonString(obj_src, &pos)?;
        skipJsonWhitespace(obj_src, &pos);
        if (pos >= obj_src.len || obj_src[pos] != ':') return err.InvalidJson;
        pos += 1;
        skipJsonWhitespace(obj_src, &pos);
        const val = scanJsonString(obj_src, &pos)?;

        if (dep_count.* >= dep_entries.len) return err.SliceCapacityExceeded;
        dep_entries[dep_count.*] = ManifestDepEntry{
            group: group,
            package_name: key,
            version_spec: val,
            active: true,
        };
        dep_count.* += 1;

        skipJsonWhitespace(obj_src, &pos);
        if (pos >= obj_src.len) return err.UnexpectedEof;
        if (obj_src[pos] == ',') {
            pos += 1;
        } else if (obj_src[pos] == '}') {
            pos += 1;
            break;
        } else {
            return err.InvalidJson;
        }
    }
}

/// Scans top-level properties of a JSON manifest object, populating `top_props`
/// and parsing any existing dependency group objects into `dep_entries`.
fn parseTopLevelManifest(
    json_src: []const u8,
    top_props: []ManifestTopPropSpan,
    prop_count: *usize,
    dep_entries: []ManifestDepEntry,
    dep_count: *usize,
)! {
    let pos: usize = 0;
    skipJsonWhitespace(json_src, &pos);
    if (pos >= json_src.len) return err.UnexpectedEof;
    if (json_src[pos] != '{') return err.RootNotObject;
    pos += 1;

    while (true) {
        skipJsonWhitespace(json_src, &pos);
        if (pos >= json_src.len) return err.UnexpectedEof;
        if (json_src[pos] == '}') {
            pos += 1;
            break;
        }

        const key_raw_start = pos;
        const key_unquoted = scanJsonString(json_src, &pos)?;
        const key_raw_end = pos;

        skipJsonWhitespace(json_src, &pos);
        if (pos >= json_src.len || json_src[pos] != ':') return err.InvalidJson;
        pos += 1;

        skipJsonWhitespace(json_src, &pos);
        const val_start = pos;
        skipJsonValue(json_src, &pos)?;
        const val_end = pos;

        if (prop_count.* >= top_props.len) return err.SliceCapacityExceeded;
        let is_dep = false;
        let grp = ManifestDepGroup.Dependencies;
        if (g := ManifestDepGroup.fromJsonKey(key_unquoted)) {
            is_dep = true;
            grp = g;
            parseDepGroupObject(json_src[val_start..val_end], g, dep_entries, dep_count)?;
        }

        top_props[prop_count.*] = ManifestTopPropSpan{
            key_unquoted: key_unquoted,
            raw_key: json_src[key_raw_start..key_raw_end],
            raw_value: json_src[val_start..val_end],
            is_dep_group: is_dep,
            dep_group: grp,
        };
        prop_count.* += 1;

        skipJsonWhitespace(json_src, &pos);
        if (pos >= json_src.len) return err.UnexpectedEof;
        if (json_src[pos] == ',') {
            pos += 1;
        } else if (json_src[pos] == '}') {
            pos += 1;
            break;
        } else {
            return err.InvalidJson;
        }
    }
}

fn findActiveEntry(entries: []const ManifestDepEntry, count: usize, pkg_name: []const u8): usize? {
    let i: usize = 0;
    while (i < count) {
        if (entries[i].active && muBytesEql(entries[i].package_name, pkg_name)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn countActiveInGroup(entries: []const ManifestDepEntry, count: usize, group: ManifestDepGroup): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < count) {
        if (entries[i].active && entries[i].group == group) n += 1;
        i += 1;
    }
    return n;
}

/// Sorts all dependency entries by `(group, package_name)` in-place via insertion sort.
fn sortDepEntriesInPlace(entries: []ManifestDepEntry, count: usize) {
    let i: usize = 1;
    while (i < count) {
        const key = entries[i];
        let j = i;
        while (j > 0) {
            const prev = entries[j - 1];
            const prev_greater = if (prev.group.index() != key.group.index())
                prev.group.index() > key.group.index()
            else
                muBytesLessThan(key.package_name, prev.package_name);
            if (!prev_greater) break;
            entries[j] = prev;
            j -= 1;
        }
        entries[j] = key;
        i += 1;
    }
}

fn emitDepGroupJson(
    group: ManifestDepGroup,
    entries: []const ManifestDepEntry,
    dep_count: usize,
    fmt: ManifestFormatProfile,
    out: []u8,
    pos: *usize,
)! {
    const nl = fmt.newlineBytes();
    fmt.writeIndent(1, out, pos)?;
    muAppendJsonString(out, pos, group.asJsonKey())?;
    muAppendByte(out, pos, ':')?;
    if (fmt.space_after_colon) muAppendByte(out, pos, ' ')?;

    const active_n = countActiveInGroup(entries, dep_count, group);
    if (active_n == 0) {
        muAppendBytes(out, pos, "{}")?;
        return;
    }

    muAppendByte(out, pos, '{')?;
    muAppendBytes(out, pos, nl)?;

    let emitted: usize = 0;
    let i: usize = 0;
    while (i < dep_count) {
        if (entries[i].active && entries[i].group == group) {
            fmt.writeIndent(2, out, pos)?;
            muAppendJsonString(out, pos, entries[i].package_name)?;
            muAppendByte(out, pos, ':')?;
            if (fmt.space_after_colon) muAppendByte(out, pos, ' ')?;
            muAppendJsonString(out, pos, entries[i].version_spec)?;
            emitted += 1;
            if (emitted < active_n) {
                muAppendByte(out, pos, ',')?;
            }
            muAppendBytes(out, pos, nl)?;
        }
        i += 1;
    }

    fmt.writeIndent(1, out, pos)?;
    muAppendByte(out, pos, '}')?;
}

fn recordChange(
    out_changes: []ManifestChangeRecord,
    summary: *ManifestUpdateSummary,
    rec: ManifestChangeRecord,
)! {
    if (summary.change_record_count >= out_changes.len) {
        return err.SliceCapacityExceeded;
    }
    out_changes[summary.change_record_count] = rec;
    summary.change_record_count += 1;
    switch (rec.kind) {
        .Added => {
            summary.added_count += 1;
            summary.has_mutations = true;
        },
        .Updated => {
            summary.updated_count += 1;
            summary.has_mutations = true;
        },
        .Moved => {
            summary.moved_count += 1;
            summary.has_mutations = true;
        },
        .Removed => {
            summary.removed_count += 1;
            summary.has_mutations = true;
        },
        .Unchanged => {
            summary.unchanged_count += 1;
        },
        .NotFound => {
            summary.not_found_count += 1;
        },
    }
}

/// Applies a batch of atomic dependency mutations (`Add` / `Upgrade` / `Remove` /
/// `MoveGroup`) to `json_src` and rewrites valid RFC 8259 JSON into `out_json`
/// while preserving original indentation, line endings, trailing newline, and
/// all unrelated top-level properties verbatim.
pub fn applyManifestMutations(
    json_src: []const u8,
    mutations: []const ManifestMutationOp,
    opts: ManifestUpdateOptions,
    bufs: *ManifestUpdateBuffers,
    out_json: []u8,
): ManifestUpdateSummary! {
    const fmt = detectManifestFormat(json_src);
    let summary = ManifestUpdateSummary{ format: fmt };

    let prop_count: usize = 0;
    let dep_count: usize = 0;
    parseTopLevelManifest(
        json_src,
        bufs.top_props,
        &prop_count,
        bufs.dep_entries,
        &dep_count,
    )?;

    let spec_pos: usize = 0;

    // Apply all mutations in sequence
    let m: usize = 0;
    while (m < mutations.len) {
        const op = mutations[m];
        if (op.package_name.len == 0) return err.EmptyMutationName;
        if (!muIsValidPkgName(op.package_name)) return err.InvalidPackageName;

        switch (op.kind) {
            .Remove => {
                let removed_any = false;
                let removed_group = ManifestDepGroup.Dependencies;
                let removed_ver: []const u8 = "";
                let i: usize = 0;
                while (i < dep_count) {
                    if (bufs.dep_entries[i].active && muBytesEql(bufs.dep_entries[i].package_name, op.package_name)) {
                        bufs.dep_entries[i].active = false;
                        if (!removed_any) {
                            removed_group = bufs.dep_entries[i].group;
                            removed_ver = bufs.dep_entries[i].version_spec;
                            removed_any = true;
                        }
                    }
                    i += 1;
                }
                if (removed_any) {
                    recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                        kind: ManifestChangeKind.Removed,
                        package_name: op.package_name,
                        from_group: removed_group,
                        to_group: removed_group,
                        old_version: removed_ver,
                        new_version: "",
                    })?;
                } else {
                    recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                        kind: ManifestChangeKind.NotFound,
                        package_name: op.package_name,
                        from_group: op.target_group,
                        to_group: op.target_group,
                    })?;
                }
            },
            .Add => {
                const written_spec = normalizeVersionSpec(
                    op.version_spec,
                    op.version_style,
                    op.catalog_name,
                    op.npm_alias_target,
                    bufs.spec_scratch[spec_pos..bufs.spec_scratch.len],
                )?;
                const norm_spec = bufs.spec_scratch[spec_pos..spec_pos + written_spec];
                spec_pos += written_spec;

                if (existing_idx := findActiveEntry(bufs.dep_entries, dep_count, op.package_name)) {
                    const old_grp = bufs.dep_entries[existing_idx].group;
                    const old_ver = bufs.dep_entries[existing_idx].version_spec;
                    // Deactivate any duplicate occurrences in other groups
                    let j: usize = existing_idx + 1;
                    while (j < dep_count) {
                        if (bufs.dep_entries[j].active && muBytesEql(bufs.dep_entries[j].package_name, op.package_name)) {
                            bufs.dep_entries[j].active = false;
                        }
                        j += 1;
                    }

                    bufs.dep_entries[existing_idx].group = op.target_group;
                    bufs.dep_entries[existing_idx].version_spec = norm_spec;

                    if (old_grp != op.target_group) {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Moved,
                            package_name: op.package_name,
                            from_group: old_grp,
                            to_group: op.target_group,
                            old_version: old_ver,
                            new_version: norm_spec,
                        })?;
                    } else if (!muBytesEql(old_ver, norm_spec)) {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Updated,
                            package_name: op.package_name,
                            from_group: old_grp,
                            to_group: op.target_group,
                            old_version: old_ver,
                            new_version: norm_spec,
                        })?;
                    } else {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Unchanged,
                            package_name: op.package_name,
                            from_group: old_grp,
                            to_group: op.target_group,
                            old_version: old_ver,
                            new_version: norm_spec,
                        })?;
                    }
                } else {
                    if (dep_count >= bufs.dep_entries.len) return err.SliceCapacityExceeded;
                    bufs.dep_entries[dep_count] = ManifestDepEntry{
                        group: op.target_group,
                        package_name: op.package_name,
                        version_spec: norm_spec,
                        active: true,
                    };
                    dep_count += 1;
                    recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                        kind: ManifestChangeKind.Added,
                        package_name: op.package_name,
                        from_group: op.target_group,
                        to_group: op.target_group,
                        old_version: "",
                        new_version: norm_spec,
                    })?;
                }
            },
            .Upgrade => {
                const written_spec = normalizeVersionSpec(
                    op.version_spec,
                    op.version_style,
                    op.catalog_name,
                    op.npm_alias_target,
                    bufs.spec_scratch[spec_pos..bufs.spec_scratch.len],
                )?;
                const norm_spec = bufs.spec_scratch[spec_pos..spec_pos + written_spec];
                spec_pos += written_spec;

                if (existing_idx := findActiveEntry(bufs.dep_entries, dep_count, op.package_name)) {
                    const grp = bufs.dep_entries[existing_idx].group;
                    const old_ver = bufs.dep_entries[existing_idx].version_spec;
                    bufs.dep_entries[existing_idx].version_spec = norm_spec;

                    if (!muBytesEql(old_ver, norm_spec)) {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Updated,
                            package_name: op.package_name,
                            from_group: grp,
                            to_group: grp,
                            old_version: old_ver,
                            new_version: norm_spec,
                        })?;
                    } else {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Unchanged,
                            package_name: op.package_name,
                            from_group: grp,
                            to_group: grp,
                            old_version: old_ver,
                            new_version: norm_spec,
                        })?;
                    }
                } else {
                    if (dep_count >= bufs.dep_entries.len) return err.SliceCapacityExceeded;
                    bufs.dep_entries[dep_count] = ManifestDepEntry{
                        group: op.target_group,
                        package_name: op.package_name,
                        version_spec: norm_spec,
                        active: true,
                    };
                    dep_count += 1;
                    recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                        kind: ManifestChangeKind.Added,
                        package_name: op.package_name,
                        from_group: op.target_group,
                        to_group: op.target_group,
                        old_version: "",
                        new_version: norm_spec,
                    })?;
                }
            },
            .MoveGroup => {
                if (existing_idx := findActiveEntry(bufs.dep_entries, dep_count, op.package_name)) {
                    const old_grp = bufs.dep_entries[existing_idx].group;
                    const old_ver = bufs.dep_entries[existing_idx].version_spec;
                    let final_ver = old_ver;
                    if (op.version_spec.len > 0 ||
                        op.version_style == ManifestVersionStyle.WorkspaceStar ||
                        op.version_style == ManifestVersionStyle.WorkspaceCaret ||
                        op.version_style == ManifestVersionStyle.WorkspaceTilde ||
                        op.version_style == ManifestVersionStyle.CatalogDefault) {
                        const w = normalizeVersionSpec(
                            op.version_spec,
                            op.version_style,
                            op.catalog_name,
                            op.npm_alias_target,
                            bufs.spec_scratch[spec_pos..bufs.spec_scratch.len],
                        )?;
                        final_ver = bufs.spec_scratch[spec_pos..spec_pos + w];
                        spec_pos += w;
                    }

                    // Deduplicate any stray entry in other groups
                    let j: usize = existing_idx + 1;
                    while (j < dep_count) {
                        if (bufs.dep_entries[j].active && muBytesEql(bufs.dep_entries[j].package_name, op.package_name)) {
                            bufs.dep_entries[j].active = false;
                        }
                        j += 1;
                    }

                    bufs.dep_entries[existing_idx].group = op.target_group;
                    bufs.dep_entries[existing_idx].version_spec = final_ver;

                    if (old_grp != op.target_group) {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Moved,
                            package_name: op.package_name,
                            from_group: old_grp,
                            to_group: op.target_group,
                            old_version: old_ver,
                            new_version: final_ver,
                        })?;
                    } else if (!muBytesEql(old_ver, final_ver)) {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Updated,
                            package_name: op.package_name,
                            from_group: old_grp,
                            to_group: op.target_group,
                            old_version: old_ver,
                            new_version: final_ver,
                        })?;
                    } else {
                        recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                            kind: ManifestChangeKind.Unchanged,
                            package_name: op.package_name,
                            from_group: old_grp,
                            to_group: op.target_group,
                            old_version: old_ver,
                            new_version: final_ver,
                        })?;
                    }
                } else {
                    recordChange(bufs.out_changes, &summary, ManifestChangeRecord{
                        kind: ManifestChangeKind.NotFound,
                        package_name: op.package_name,
                        from_group: op.target_group,
                        to_group: op.target_group,
                    })?;
                }
            },
        }
        m += 1;
    }

    if (opts.sort_dependencies && dep_count > 1) {
        sortDepEntriesInPlace(bufs.dep_entries, dep_count);
    }

    // Track which dependency groups existed in the original JSON
    let existed: [4]bool = [false; 4];
    let pi: usize = 0;
    while (pi < prop_count) {
        if (bufs.top_props[pi].is_dep_group) {
            existed[bufs.top_props[pi].dep_group.index()] = true;
        }
        pi += 1;
    }

    // Count total top-level properties to emit (for accurate comma placement)
    let total_to_emit: usize = 0;
    let p1: usize = 0;
    while (p1 < prop_count) {
        if (bufs.top_props[p1].is_dep_group) {
            const g_active = countActiveInGroup(bufs.dep_entries, dep_count, bufs.top_props[p1].dep_group);
            if (g_active > 0 || !opts.remove_empty_groups) {
                total_to_emit += 1;
            }
        } else {
            total_to_emit += 1;
        }
        p1 += 1;
    }
    let g_idx: usize = 0;
    while (g_idx < MANIFEST_UPDATER_GROUP_COUNT) {
        if (!existed[g_idx]) {
            if (grp := ManifestDepGroup.fromIndex(g_idx)) {
                if (countActiveInGroup(bufs.dep_entries, dep_count, grp) > 0) {
                    total_to_emit += 1;
                }
            }
        }
        g_idx += 1;
    }

    // Rewrite JSON into `out_json`
    let out_pos: usize = 0;
    const nl = fmt.newlineBytes();

    if (total_to_emit == 0) {
        muAppendBytes(out_json, &out_pos, "{}")?;
        if (fmt.has_trailing_newline) muAppendBytes(out_json, &out_pos, nl)?;
        summary.bytes_written = out_pos;
        return summary;
    }

    muAppendByte(out_json, &out_pos, '{')?;
    muAppendBytes(out_json, &out_pos, nl)?;

    let emitted_props: usize = 0;
    let p2: usize = 0;
    while (p2 < prop_count) {
        const prop = bufs.top_props[p2];
        if (prop.is_dep_group) {
            const g_active = countActiveInGroup(bufs.dep_entries, dep_count, prop.dep_group);
            if (g_active > 0 || !opts.remove_empty_groups) {
                emitDepGroupJson(prop.dep_group, bufs.dep_entries, dep_count, fmt, out_json, &out_pos)?;
                emitted_props += 1;
                if (emitted_props < total_to_emit) muAppendByte(out_json, &out_pos, ',')?;
                muAppendBytes(out_json, &out_pos, nl)?;
            }
        } else {
            fmt.writeIndent(1, out_json, &out_pos)?;
            muAppendBytes(out_json, &out_pos, prop.raw_key)?;
            muAppendByte(out_json, &out_pos, ':')?;
            if (fmt.space_after_colon) muAppendByte(out_json, &out_pos, ' ')?;
            muAppendBytes(out_json, &out_pos, prop.raw_value)?;
            emitted_props += 1;
            if (emitted_props < total_to_emit) muAppendByte(out_json, &out_pos, ',')?;
            muAppendBytes(out_json, &out_pos, nl)?;
        }
        p2 += 1;
    }

    // Emit newly created dependency groups in canonical order
    let g2: usize = 0;
    while (g2 < MANIFEST_UPDATER_GROUP_COUNT) {
        if (!existed[g2]) {
            if (grp := ManifestDepGroup.fromIndex(g2)) {
                if (countActiveInGroup(bufs.dep_entries, dep_count, grp) > 0) {
                    emitDepGroupJson(grp, bufs.dep_entries, dep_count, fmt, out_json, &out_pos)?;
                    emitted_props += 1;
                    if (emitted_props < total_to_emit) muAppendByte(out_json, &out_pos, ',')?;
                    muAppendBytes(out_json, &out_pos, nl)?;
                }
            }
        }
        g2 += 1;
    }

    muAppendByte(out_json, &out_pos, '}')?;
    if (fmt.has_trailing_newline) {
        muAppendBytes(out_json, &out_pos, nl)?;
    }

    summary.bytes_written = out_pos;
    return summary;
}

// ============================================================================
// Section 5: Human-Readable Terminal Diff Summary Formatter
// ============================================================================

/// Formats a concise terminal diff summary report from `applyManifestMutations` output.
pub fn formatManifestUpdateReport(
    summary: ManifestUpdateSummary,
    records: []const ManifestChangeRecord,
    out: []u8,
): usize! {
    let pos: usize = 0;
    if (!summary.has_mutations) {
        muAppendBytes(out, &pos, "Manifest unchanged (0 mutations).\n")?;
        return pos;
    }

    muAppendBytes(out, &pos, "Manifest updated: +")?;
    muAppendU64(out, &pos, @as(u64, summary.added_count))?;
    muAppendBytes(out, &pos, " added, ~")?;
    muAppendU64(out, &pos, @as(u64, summary.updated_count))?;
    muAppendBytes(out, &pos, " updated, ->")?;
    muAppendU64(out, &pos, @as(u64, summary.moved_count))?;
    muAppendBytes(out, &pos, " moved, -")?;
    muAppendU64(out, &pos, @as(u64, summary.removed_count))?;
    muAppendBytes(out, &pos, " removed\n")?;

    let i: usize = 0;
    while (i < summary.change_record_count && i < records.len) {
        const r = records[i];
        switch (r.kind) {
            .Added => {
                muAppendBytes(out, &pos, "  + ")?;
                muAppendBytes(out, &pos, r.package_name)?;
                muAppendByte(out, &pos, '@')?;
                muAppendBytes(out, &pos, r.new_version)?;
                muAppendBytes(out, &pos, " [")?;
                muAppendBytes(out, &pos, r.to_group.asJsonKey())?;
                muAppendBytes(out, &pos, "]\n")?;
            },
            .Updated => {
                muAppendBytes(out, &pos, "  ~ ")?;
                muAppendBytes(out, &pos, r.package_name)?;
                muAppendByte(out, &pos, ' ')?;
                muAppendBytes(out, &pos, r.old_version)?;
                muAppendBytes(out, &pos, " -> ")?;
                muAppendBytes(out, &pos, r.new_version)?;
                muAppendBytes(out, &pos, " [")?;
                muAppendBytes(out, &pos, r.to_group.asJsonKey())?;
                muAppendBytes(out, &pos, "]\n")?;
            },
            .Moved => {
                muAppendBytes(out, &pos, "  -> ")?;
                muAppendBytes(out, &pos, r.package_name)?;
                muAppendByte(out, &pos, '@')?;
                muAppendBytes(out, &pos, r.new_version)?;
                muAppendBytes(out, &pos, " [")?;
                muAppendBytes(out, &pos, r.from_group.asJsonKey())?;
                muAppendBytes(out, &pos, " -> ")?;
                muAppendBytes(out, &pos, r.to_group.asJsonKey())?;
                muAppendBytes(out, &pos, "]\n")?;
            },
            .Removed => {
                muAppendBytes(out, &pos, "  - ")?;
                muAppendBytes(out, &pos, r.package_name)?;
                muAppendByte(out, &pos, '@')?;
                muAppendBytes(out, &pos, r.old_version)?;
                muAppendBytes(out, &pos, " [")?;
                muAppendBytes(out, &pos, r.from_group.asJsonKey())?;
                muAppendBytes(out, &pos, "]\n")?;
            },
            .Unchanged, .NotFound => {},
        }
        i += 1;
    }

    return pos;
}
