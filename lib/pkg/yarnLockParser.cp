// Source: Absorbed from /data/dev/bun/src/install/yarn.cp | Synced: 2026-11-21T17:52:18Z
// ============================================================================
// lib/pkg/yarnLockParser.cp — Zero-Alloc Yarn v1 Lockfile Parser & Migrator
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven Yarn Lockfile v1 parser, SRI
// integrity validator, migration auditor, and bidirectional formatter for
// `cap install` automated Yarn-to-Cap project migration:
//   1. Version format detector (`detectYarnLockFormat`) distinguishing
//      `# yarn lockfile v1` from Yarn Berry (`__metadata:`) YAML v2+;
//   2. Multi-descriptor block lexer & parser (`parseYarnLockV1`) supporting
//      comma-separated aliases (`"@babel/core@^7.20.0", "@babel/core@^7.22.0":`),
//      double-quoted escape decoding, `version`, `resolved`, `integrity`,
//      `dependencies:`, and `optionalDependencies:` sub-blocks;
//   3. Package descriptor splitter (`splitYarnDescriptor`) for scoped packages,
//      `npm:` aliases, `workspace:` ranges, and tarball/git URLs;
//   4. SRI integrity parser (`parseYarnIntegrity`), hash-accelerated alias
//      lookup (`lookupYarnAlias`), dangling-dependency migration auditor
//      (`buildYarnMigrationReport`, `formatYarnMigrationSummary`), and
//      round-trip Yarn v1 serializer (`formatYarnLockV1`).

/// Canonical Yarn v1 lockfile header marker.
pub const YARN_V1_HEADER_MARKER: []const u8 = "# yarn lockfile v1";

/// Yarn Berry (v2+) YAML lockfile metadata header key.
pub const YARN_BERRY_METADATA_KEY: []const u8 = "__metadata:";

/// Sentinel index indicating an unresolved package reference.
pub const YARN_NO_PACKAGE_INDEX: u32 = 0xFFFFFFFF;

/// Yarn v1 lockfile parsing, SRI validation, and formatting error set.
pub err YarnLockParseError {
    UnsupportedYarnBerryFormat,
    MalformedHeaderLine,
    UnterminatedQuotedString,
    InvalidDescriptor,
    InvalidIntegrityFormat,
    UnexpectedIndentation,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, Hash & Line-Scanning Helpers
// ============================================================================

fn ylBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn ylStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return ylBytesEql(s[0..prefix.len], prefix);
}

fn ylIndexOf(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn ylTrimRight(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[0..end];
}

fn ylTrimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

/// Fast 8-byte unrolled newline scanner (accelerated line splitting without heap allocation).
fn ylFindNextNewline(src: []const u8, start: usize): usize {
    let i = start;
    while (i + 8 <= src.len) {
        if (src[i] == '\n') return i;
        if (src[i + 1] == '\n') return i + 1;
        if (src[i + 2] == '\n') return i + 2;
        if (src[i + 3] == '\n') return i + 3;
        if (src[i + 4] == '\n') return i + 4;
        if (src[i + 5] == '\n') return i + 5;
        if (src[i + 6] == '\n') return i + 6;
        if (src[i + 7] == '\n') return i + 7;
        i += 8;
    }
    while (i < src.len) {
        if (src[i] == '\n') return i;
        i += 1;
    }
    return src.len;
}

/// FNV-1a 64-bit hash over `(name, '@', range)` for fast alias lookup.
fn ylHashNameAndRange(name: []const u8, range: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < name.len) {
        h = (h ^ @as(u64, name[i])) *% 1099511628211;
        i += 1;
    }
    h = (h ^ @as(u64, '@')) *% 1099511628211;
    let j: usize = 0;
    while (j < range.len) {
        h = (h ^ @as(u64, range[j])) *% 1099511628211;
        j += 1;
    }
    return h;
}

fn ylAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ylAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ylAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ylAppendByte(out, pos, '0')?;
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
        ylAppendByte(out, pos, tmp[n])?;
    }
}

fn ylNeedsQuotes(s: []const u8): bool {
    if (s.len == 0) return true;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == ' ' || c == '"' || c == ',' || c == ':' || c == '@' || c == '^' || c == '~' || c == '*' || c == '>') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn ylAppendQuoted(out: []u8, pos: *usize, s: []const u8)! {
    ylAppendByte(out, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '"' || s[i] == '\\') {
            ylAppendByte(out, pos, '\\')?;
        }
        ylAppendByte(out, pos, s[i])?;
        i += 1;
    }
    ylAppendByte(out, pos, '"')?;
}

// ============================================================================
// Section 1: Format Detection & Package Descriptor Splitter
// ============================================================================

/// Detected Yarn lockfile format generation.
pub enum YarnLockFormatKind: u8 {
    YarnV1 = 0,
    YarnBerryV2Plus = 1,
    Unknown = 2,
}

/// Inspects the leading lines of a Yarn lockfile to classify v1 vs Berry (v2+).
pub fn detectYarnLockFormat(src: []const u8): YarnLockFormatKind {
    let pos: usize = 0;
    let lines_checked: u32 = 0;
    while (pos < src.len && lines_checked < 32) {
        const nl = ylFindNextNewline(src, pos);
        const line = ylTrimSpaces(src[pos..nl]);
        pos = if (nl < src.len) nl + 1 else src.len;
        if (line.len == 0) continue;
        lines_checked += 1;

        if (ylBytesEql(line, YARN_V1_HEADER_MARKER)) {
            return YarnLockFormatKind.YarnV1;
        }
        if (ylStartsWith(line, YARN_BERRY_METADATA_KEY)) {
            return YarnLockFormatKind.YarnBerryV2Plus;
        }
    }
    return YarnLockFormatKind.Unknown;
}

/// Decomposed Yarn package descriptor (`name@range`).
pub struct YarnDescriptorSpec {
    raw: []const u8 = "",
    name: []const u8 = "",
    range: []const u8 = "",
    is_scoped: bool = false,
    is_npm_alias: bool = false,
    is_workspace: bool = false,
    is_url: bool = false,
}

/// Splits a Yarn v1 descriptor (`@babel/core@^7.22.0`, `foo@npm:bar@^1.0.0`,
/// `pkg@https://...`, `pkg@workspace:^`) into its `(name, range)` components.
pub fn splitYarnDescriptor(raw_desc: []const u8): YarnDescriptorSpec! {
    const s = ylTrimSpaces(raw_desc);
    if (s.len < 3) return err.InvalidDescriptor;

    const is_scoped = (s[0] == '@');
    const search_start: usize = if (is_scoped) 1 else 0;
    const rel_at = ylIndexOf(s[search_start..s.len], '@') ?? return err.InvalidDescriptor;
    const at_idx = search_start + rel_at;

    if (at_idx == 0 || at_idx + 1 >= s.len) return err.InvalidDescriptor;
    const name = s[0..at_idx];
    const range = s[at_idx + 1..s.len];

    if (is_scoped && ylIndexOf(name, '/') == null) {
        return err.InvalidDescriptor;
    }

    return YarnDescriptorSpec{
        raw: s,
        name: name,
        range: range,
        is_scoped: is_scoped,
        is_npm_alias: ylStartsWith(range, "npm:"),
        is_workspace: ylStartsWith(range, "workspace:"),
        is_url: ylStartsWith(range, "https://") || ylStartsWith(range, "http://") || ylStartsWith(range, "git+"),
    };
}

// ============================================================================
// Section 2: Subresource Integrity (SRI) Parser
// ============================================================================

/// Supported SRI hash algorithms in Yarn lockfiles.
pub enum YarnIntegrityAlgo: u8 {
    None = 0,
    Sha1 = 1,
    Sha256 = 2,
    Sha384 = 3,
    Sha512 = 4,

    fn asPrefix(): []const u8 {
        return switch (_) {
            .None => "",
            .Sha1 => "sha1-",
            .Sha256 => "sha256-",
            .Sha384 => "sha384-",
            .Sha512 => "sha512-",
        };
    }
}

/// Parsed SRI integrity metadata.
pub struct YarnParsedIntegrity {
    algo: YarnIntegrityAlgo = YarnIntegrityAlgo.None,
    digest_b64: []const u8 = "",
    valid_base64: bool = false,
}

fn ylIsBase64Char(c: u8): bool {
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '+' || c == '/' || c == '=';
}

/// Parses and validates a Yarn `integrity` SRI string (`sha512-...`, `sha1-...`, etc.).
pub fn parseYarnIntegrity(raw: []const u8): YarnParsedIntegrity! {
    const s = ylTrimSpaces(raw);
    if (s.len == 0) {
        return YarnParsedIntegrity{ algo: YarnIntegrityAlgo.None, valid_base64: false };
    }

    let algo = YarnIntegrityAlgo.None;
    let prefix_len: usize = 0;

    if (ylStartsWith(s, "sha512-")) {
        algo = YarnIntegrityAlgo.Sha512;
        prefix_len = 7;
    } else if (ylStartsWith(s, "sha384-")) {
        algo = YarnIntegrityAlgo.Sha384;
        prefix_len = 7;
    } else if (ylStartsWith(s, "sha256-")) {
        algo = YarnIntegrityAlgo.Sha256;
        prefix_len = 7;
    } else if (ylStartsWith(s, "sha1-")) {
        algo = YarnIntegrityAlgo.Sha1;
        prefix_len = 5;
    } else {
        return err.InvalidIntegrityFormat;
    }

    const digest = s[prefix_len..s.len];
    if (digest.len < 8) return err.InvalidIntegrityFormat;

    let i: usize = 0;
    while (i < digest.len) {
        if (!ylIsBase64Char(digest[i])) return err.InvalidIntegrityFormat;
        i += 1;
    }

    return YarnParsedIntegrity{
        algo: algo,
        digest_b64: digest,
        valid_base64: true,
    };
}

// ============================================================================
// Section 3: Zero-Alloc Yarn Lockfile v1 Parser
// ============================================================================

/// Dependency section kind inside a Yarn v1 package block.
pub enum YarnV1DepKind: u8 {
    Runtime = 0,
    Optional = 1,
}

/// Single alias descriptor pointing to a resolved `YarnV1PackageRecord`.
pub struct YarnV1AliasEntry {
    descriptor: []const u8 = "",
    name: []const u8 = "",
    range: []const u8 = "",
    package_index: u32 = YARN_NO_PACKAGE_INDEX,
    desc_hash64: u64 = 0,

    static fn empty(): YarnV1AliasEntry {
        return YarnV1AliasEntry{};
    }
}

/// Single dependency edge declared under `dependencies:` or `optionalDependencies:`.
pub struct YarnV1DepEntry {
    owner_package_index: u32 = YARN_NO_PACKAGE_INDEX,
    kind: YarnV1DepKind = YarnV1DepKind.Runtime,
    name: []const u8 = "",
    range: []const u8 = "",

    static fn empty(): YarnV1DepEntry {
        return YarnV1DepEntry{};
    }
}

/// Resolved package block parsed from a Yarn v1 lockfile.
pub struct YarnV1PackageRecord {
    package_index: u32 = 0,
    name: []const u8 = "",
    version: []const u8 = "",
    resolved: []const u8 = "",
    integrity: []const u8 = "",
    integrity_algo: YarnIntegrityAlgo = YarnIntegrityAlgo.None,
    alias_start: u32 = 0,
    alias_count: u32 = 0,
    dep_start: u32 = 0,
    dep_count: u32 = 0,
    opt_dep_count: u32 = 0,

    static fn empty(): YarnV1PackageRecord {
        return YarnV1PackageRecord{};
    }
}

/// Caller-owned backing slices for zero-heap `parseYarnLockV1`.
pub struct YarnV1ParseBuffers {
    packages: []YarnV1PackageRecord,
    aliases: []YarnV1AliasEntry,
    deps: []YarnV1DepEntry,
    unescape_scratch: []u8,
}

/// Aggregate parse result from `parseYarnLockV1`.
pub struct YarnV1ParseResult {
    format: YarnLockFormatKind = YarnLockFormatKind.Unknown,
    package_count: usize = 0,
    alias_count: usize = 0,
    dep_count: usize = 0,
    lines_scanned: u32 = 0,
}

/// Decodes a Yarn v1 quoted or unquoted token, writing unescaped bytes into `scratch`
/// only when backslash escapes are actually present.
fn decodeYarnToken(raw: []const u8, scratch: []u8, scratch_pos: *usize): ([]const u8)! {
    const s = ylTrimSpaces(raw);
    if (s.len == 0) return "";
    if (s[0] != '"') return s;

    if (s.len < 2 || s[s.len - 1] != '"') {
        return err.UnterminatedQuotedString;
    }
    const inner = s[1..s.len - 1];

    // Fast path: no escape characters -> zero-copy slice directly into input!
    if (ylIndexOf(inner, '\\') == null) {
        return inner;
    }

    const start = scratch_pos.*;
    let i: usize = 0;
    while (i < inner.len) {
        if (inner[i] == '\\' && i + 1 < inner.len) {
            const nxt = inner[i + 1];
            const decoded: u8 = if (nxt == 'n') '\n'
                else if (nxt == 'r') '\r'
                else if (nxt == 't') '\t'
                else nxt;
            ylAppendByte(scratch, scratch_pos, decoded)?;
            i += 2;
        } else {
            ylAppendByte(scratch, scratch_pos, inner[i])?;
            i += 1;
        }
    }
    return scratch[start..scratch_pos.*];
}

/// Splits a Yarn key/value property line (`key value` or `"@scope/pkg" "^1.0.0"`).
struct YarnKvPair {
    key: []const u8 = "",
    value: []const u8 = "",
}

fn splitYarnKvLine(line: []const u8, scratch: []u8, scratch_pos: *usize): YarnKvPair! {
    const s = ylTrimSpaces(line);
    if (s.len == 0) return YarnKvPair{};

    let key_end: usize = 0;
    if (s[0] == '"') {
        let i: usize = 1;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
            } else if (s[i] == '"') {
                key_end = i + 1;
                break;
            } else {
                i += 1;
            }
        }
        if (key_end == 0) return err.UnterminatedQuotedString;
    } else {
        while (key_end < s.len && s[key_end] != ' ' && s[key_end] != '\t') {
            key_end += 1;
        }
    }

    const key = decodeYarnToken(s[0..key_end], scratch, scratch_pos)?;
    const rest = ylTrimSpaces(s[key_end..s.len]);
    const val = decodeYarnToken(rest, scratch, scratch_pos)?;
    return YarnKvPair{ key: key, value: val };
}

/// Parses comma-separated alias descriptors from a Yarn v1 header line (trailing `:` stripped).
fn parseHeaderAliases(
    header_body: []const u8,
    pkg_idx: u32,
    bufs: *YarnV1ParseBuffers,
    alias_count: *usize,
    scratch_pos: *usize,
): u32! {
    let added: u32 = 0;
    let i: usize = 0;

    while (i < header_body.len) {
        // Skip leading whitespace / commas
        while (i < header_body.len && (header_body[i] == ' ' || header_body[i] == '\t' || header_body[i] == ',')) {
            i += 1;
        }
        if (i >= header_body.len) break;

        const tok_start = i;
        let in_quotes = false;
        while (i < header_body.len) {
            const c = header_body[i];
            if (c == '\\' && in_quotes && i + 1 < header_body.len) {
                i += 2;
                continue;
            }
            if (c == '"') {
                in_quotes = !in_quotes;
            } else if (c == ',' && !in_quotes) {
                break;
            }
            i += 1;
        }
        if (in_quotes) return err.UnterminatedQuotedString;

        const decoded = decodeYarnToken(header_body[tok_start..i], bufs.unescape_scratch, scratch_pos)?;
        const split = splitYarnDescriptor(decoded)?;

        if (alias_count.* >= bufs.aliases.len) return err.SliceCapacityExceeded;
        bufs.aliases[alias_count.*] = YarnV1AliasEntry{
            descriptor: decoded,
            name: split.name,
            range: split.range,
            package_index: pkg_idx,
            desc_hash64: ylHashNameAndRange(split.name, split.range),
        };
        alias_count.* += 1;
        added += 1;
    }

    if (added == 0) return err.MalformedHeaderLine;
    return added;
}

/// Parses an entire Yarn Lockfile v1 document into caller-owned zero-allocation buffers.
pub fn parseYarnLockV1(
    src: []const u8,
    bufs: *YarnV1ParseBuffers,
): YarnV1ParseResult! {
    const fmt = detectYarnLockFormat(src);
    if (fmt == YarnLockFormatKind.YarnBerryV2Plus) {
        return err.UnsupportedYarnBerryFormat;
    }

    let pkg_count: usize = 0;
    let alias_count: usize = 0;
    let dep_count: usize = 0;
    let scratch_pos: usize = 0;
    let lines_scanned: u32 = 0;

    let has_current = false;
    let cur_pkg_idx: usize = 0;
    let in_sub_section: u8 = 0; // 0 = none, 1 = dependencies, 2 = optionalDependencies

    let pos: usize = 0;
    while (pos < src.len) {
        const nl = ylFindNextNewline(src, pos);
        const raw_line = ylTrimRight(src[pos..nl]);
        pos = if (nl < src.len) nl + 1 else src.len;
        lines_scanned += 1;

        if (raw_line.len == 0) {
            in_sub_section = 0;
            continue;
        }

        // Count leading spaces
        let indent: usize = 0;
        while (indent < raw_line.len && raw_line[indent] == ' ') {
            indent += 1;
        }
        const trimmed = raw_line[indent..raw_line.len];
        if (trimmed.len == 0 || trimmed[0] == '#') continue;

        if (indent == 0) {
            // New package block header: must end with ':'
            if (trimmed[trimmed.len - 1] != ':') {
                return err.MalformedHeaderLine;
            }
            if (pkg_count >= bufs.packages.len) return err.SliceCapacityExceeded;

            const p_idx_u32 = @as(u32, @intCast(pkg_count));
            const a_start = @as(u32, @intCast(alias_count));
            const a_added = parseHeaderAliases(
                trimmed[0..trimmed.len - 1],
                p_idx_u32,
                bufs,
                &alias_count,
                &scratch_pos,
            )?;

            bufs.packages[pkg_count] = YarnV1PackageRecord{
                package_index: p_idx_u32,
                name: bufs.aliases[@as(usize, a_start)].name,
                alias_start: a_start,
                alias_count: a_added,
                dep_start: @as(u32, @intCast(dep_count)),
                dep_count: 0,
                opt_dep_count: 0,
            };
            cur_pkg_idx = pkg_count;
            pkg_count += 1;
            has_current = true;
            in_sub_section = 0;
        } else if (indent == 2) {
            if (!has_current) return err.UnexpectedIndentation;
            if (ylBytesEql(trimmed, "dependencies:")) {
                in_sub_section = 1;
                continue;
            }
            if (ylBytesEql(trimmed, "optionalDependencies:")) {
                in_sub_section = 2;
                continue;
            }

            in_sub_section = 0;
            const kv = splitYarnKvLine(trimmed, bufs.unescape_scratch, &scratch_pos)?;
            if (ylBytesEql(kv.key, "version")) {
                bufs.packages[cur_pkg_idx].version = kv.value;
            } else if (ylBytesEql(kv.key, "resolved")) {
                bufs.packages[cur_pkg_idx].resolved = kv.value;
            } else if (ylBytesEql(kv.key, "integrity")) {
                bufs.packages[cur_pkg_idx].integrity = kv.value;
                const parsed_sri = parseYarnIntegrity(kv.value)?;
                bufs.packages[cur_pkg_idx].integrity_algo = parsed_sri.algo;
            }
        } else if (indent >= 4) {
            if (!has_current || in_sub_section == 0) return err.UnexpectedIndentation;
            const kv = splitYarnKvLine(trimmed, bufs.unescape_scratch, &scratch_pos)?;
            if (kv.key.len == 0) continue;

            if (dep_count >= bufs.deps.len) return err.SliceCapacityExceeded;
            const dep_kind = if (in_sub_section == 1) YarnV1DepKind.Runtime else YarnV1DepKind.Optional;
            bufs.deps[dep_count] = YarnV1DepEntry{
                owner_package_index: @as(u32, @intCast(cur_pkg_idx)),
                kind: dep_kind,
                name: kv.key,
                range: kv.value,
            };
            dep_count += 1;
            bufs.packages[cur_pkg_idx].dep_count += 1;
            if (dep_kind == YarnV1DepKind.Optional) {
                bufs.packages[cur_pkg_idx].opt_dep_count += 1;
            }
        } else {
            return err.UnexpectedIndentation;
        }
    }

    return YarnV1ParseResult{
        format: fmt,
        package_count: pkg_count,
        alias_count: alias_count,
        dep_count: dep_count,
        lines_scanned: lines_scanned,
    };
}

// ============================================================================
// Section 4: Fast Alias Lookup & Migration Integrity Auditor
// ============================================================================

/// Dangling dependency edge that could not be matched to any Yarn alias block.
pub struct YarnDanglingDep {
    owner_package_index: u32 = YARN_NO_PACKAGE_INDEX,
    dep_name: []const u8 = "",
    dep_range: []const u8 = "",
    is_optional: bool = false,

    static fn empty(): YarnDanglingDep {
        return YarnDanglingDep{};
    }
}

/// Structured migration audit summary for converting `yarn.lock` to `cap.lock`.
pub struct YarnMigrationReport {
    package_count: u32 = 0,
    alias_count: u32 = 0,
    total_dep_edges: u32 = 0,
    resolved_dep_edges: u32 = 0,
    dangling_required_count: u32 = 0,
    dangling_optional_count: u32 = 0,
    missing_integrity_count: u32 = 0,
    sha512_count: u32 = 0,
    sha1_count: u32 = 0,
}

/// Looks up the target `package_index` for `(name, range)` using hash pre-filtering.
pub fn lookupYarnAlias(
    aliases: []const YarnV1AliasEntry,
    name: []const u8,
    range: []const u8,
): u32? {
    const h = ylHashNameAndRange(name, range);
    let i: usize = 0;
    while (i < aliases.len) {
        if (aliases[i].desc_hash64 == h &&
            ylBytesEql(aliases[i].name, name) &&
            ylBytesEql(aliases[i].range, range)) {
            return aliases[i].package_index;
        }
        i += 1;
    }
    return null;
}

/// Audits parsed Yarn v1 records for SRI coverage and dangling dependency references.
pub fn buildYarnMigrationReport(
    packages: []const YarnV1PackageRecord,
    aliases: []const YarnV1AliasEntry,
    deps: []const YarnV1DepEntry,
    out_dangling: []YarnDanglingDep,
): YarnMigrationReport {
    let rep = YarnMigrationReport{
        package_count: @as(u32, @intCast(packages.len)),
        alias_count: @as(u32, @intCast(aliases.len)),
        total_dep_edges: @as(u32, @intCast(deps.len)),
    };

    let p: usize = 0;
    while (p < packages.len) {
        switch (packages[p].integrity_algo) {
            .None => { rep.missing_integrity_count += 1; },
            .Sha512 => { rep.sha512_count += 1; },
            .Sha1 => { rep.sha1_count += 1; },
            else => {},
        }
        p += 1;
    }

    let dang_pos: usize = 0;
    let d: usize = 0;
    while (d < deps.len) {
        const edge = deps[d];
        if (lookupYarnAlias(aliases, edge.name, edge.range) != null) {
            rep.resolved_dep_edges += 1;
        } else {
            const is_opt = (edge.kind == YarnV1DepKind.Optional);
            if (is_opt) {
                rep.dangling_optional_count += 1;
            } else {
                rep.dangling_required_count += 1;
            }
            if (dang_pos < out_dangling.len) {
                out_dangling[dang_pos] = YarnDanglingDep{
                    owner_package_index: edge.owner_package_index,
                    dep_name: edge.name,
                    dep_range: edge.range,
                    is_optional: is_opt,
                };
                dang_pos += 1;
            }
        }
        d += 1;
    }

    return rep;
}

/// Renders a concise terminal migration audit summary into caller-owned `out`.
pub fn formatYarnMigrationSummary(report: YarnMigrationReport, out: []u8): usize! {
    let pos: usize = 0;
    ylAppendBytes(out, &pos, "Yarn v1 Migration Audit: ")?;
    ylAppendU64(out, &pos, @as(u64, report.package_count))?;
    ylAppendBytes(out, &pos, " package(s), ")?;
    ylAppendU64(out, &pos, @as(u64, report.alias_count))?;
    ylAppendBytes(out, &pos, " alias(es), ")?;
    ylAppendU64(out, &pos, @as(u64, report.resolved_dep_edges))?;
    ylAppendByte(out, &pos, '/')?;
    ylAppendU64(out, &pos, @as(u64, report.total_dep_edges))?;
    ylAppendBytes(out, &pos, " dep edges resolved")?;
    if (report.missing_integrity_count > 0) {
        ylAppendBytes(out, &pos, " | warn: ")?;
        ylAppendU64(out, &pos, @as(u64, report.missing_integrity_count))?;
        ylAppendBytes(out, &pos, " missing integrity")?;
    }
    if (report.dangling_required_count > 0) {
        ylAppendBytes(out, &pos, " | error: ")?;
        ylAppendU64(out, &pos, @as(u64, report.dangling_required_count))?;
        ylAppendBytes(out, &pos, " dangling required dep(s)")?;
    }
    ylAppendByte(out, &pos, '\n')?;
    return pos;
}

// ============================================================================
// Section 5: Bidirectional Yarn Lockfile v1 Formatter (Round-Trip Serializer)
// ============================================================================

/// Serializes parsed Yarn v1 records back into canonical `# yarn lockfile v1` text.
pub fn formatYarnLockV1(
    packages: []const YarnV1PackageRecord,
    aliases: []const YarnV1AliasEntry,
    deps: []const YarnV1DepEntry,
    out: []u8,
): usize! {
    let pos: usize = 0;
    ylAppendBytes(out, &pos, "# THIS IS AN AUTOGENERATED FILE. DO NOT EDIT THIS FILE DIRECTLY.\n")?;
    ylAppendBytes(out, &pos, "# yarn lockfile v1\n\n")?;

    let p: usize = 0;
    while (p < packages.len) {
        const pkg = packages[p];
        if (p > 0) ylAppendByte(out, &pos, '\n')?;

        // Header line with comma-separated aliases
        let a: u32 = 0;
        while (a < pkg.alias_count) {
            if (a > 0) ylAppendBytes(out, &pos, ", ")?;
            const entry = aliases[@as(usize, pkg.alias_start + a)];
            if (ylNeedsQuotes(entry.descriptor)) {
                ylAppendQuoted(out, &pos, entry.descriptor)?;
            } else {
                ylAppendBytes(out, &pos, entry.descriptor)?;
            }
            a += 1;
        }
        ylAppendBytes(out, &pos, ":\n")?;

        if (pkg.version.len > 0) {
            ylAppendBytes(out, &pos, "  version ")?;
            ylAppendQuoted(out, &pos, pkg.version)?;
            ylAppendByte(out, &pos, '\n')?;
        }
        if (pkg.resolved.len > 0) {
            ylAppendBytes(out, &pos, "  resolved ")?;
            ylAppendQuoted(out, &pos, pkg.resolved)?;
            ylAppendByte(out, &pos, '\n')?;
        }
        if (pkg.integrity.len > 0) {
            ylAppendBytes(out, &pos, "  integrity ")?;
            ylAppendBytes(out, &pos, pkg.integrity)?;
            ylAppendByte(out, &pos, '\n')?;
        }

        // Runtime dependencies sub-block
        const runtime_deps = pkg.dep_count - pkg.opt_dep_count;
        if (runtime_deps > 0) {
            ylAppendBytes(out, &pos, "  dependencies:\n")?;
            let d: u32 = 0;
            while (d < pkg.dep_count) {
                const dep = deps[@as(usize, pkg.dep_start + d)];
                if (dep.kind == YarnV1DepKind.Runtime) {
                    ylAppendBytes(out, &pos, "    ")?;
                    if (ylNeedsQuotes(dep.name)) {
                        ylAppendQuoted(out, &pos, dep.name)?;
                    } else {
                        ylAppendBytes(out, &pos, dep.name)?;
                    }
                    ylAppendByte(out, &pos, ' ')?;
                    ylAppendQuoted(out, &pos, dep.range)?;
                    ylAppendByte(out, &pos, '\n')?;
                }
                d += 1;
            }
        }

        // Optional dependencies sub-block
        if (pkg.opt_dep_count > 0) {
            ylAppendBytes(out, &pos, "  optionalDependencies:\n")?;
            let d: u32 = 0;
            while (d < pkg.dep_count) {
                const dep = deps[@as(usize, pkg.dep_start + d)];
                if (dep.kind == YarnV1DepKind.Optional) {
                    ylAppendBytes(out, &pos, "    ")?;
                    if (ylNeedsQuotes(dep.name)) {
                        ylAppendQuoted(out, &pos, dep.name)?;
                    } else {
                        ylAppendBytes(out, &pos, dep.name)?;
                    }
                    ylAppendByte(out, &pos, ' ')?;
                    ylAppendQuoted(out, &pos, dep.range)?;
                    ylAppendByte(out, &pos, '\n')?;
                }
                d += 1;
            }
        }

        p += 1;
    }

    return pos;
}
