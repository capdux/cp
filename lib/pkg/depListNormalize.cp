// Source: Absorbed from /data/dev/bun/src/install/PackageManager/processDependencyList.cp | Synced: 2026-11-23T20:22:19Z
// ============================================================================
// lib/pkg/depListNormalize.cp — Zero-Alloc All-Protocol Dep Normalizer & Overrides
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven dependency specifier normalizer,
// group-priority deduplicator, and overrides/resolutions injector for
// `cap install` / `cap add`:
//   1. All-protocol specifier parser (`parseDependencyProtocolSpec`) supporting
//      all 10 npm/cap protocols (`SemverRange`, `DistTag`, `NpmAlias`,
//      `WorkspaceProtocol`, `CatalogProtocol`, `FileProtocol`, `LinkProtocol`,
//      `GitHubShorthand`, `GitUrl`, `TarballUrl`) with unsafe scheme & path
//      traversal guards;
//   2. Group-priority merge, deduplication & lexicographical sorter
//      (`normalizeDependencyList`) enforcing `dependencies > optionalDependencies
//      > devDependencies > peerDependencies`;
//   3. Overrides / resolutions injector (`applyDependencyOverrides`) supporting
//      top-level exact (`"lodash"`), version-scoped (`"lodash@^4.0.0"`), and
//      parent-child path (`"parent>child"`) rules with full audit trails;
//   4. Zero-heap summary & override audit reporter (`formatNormalizedDepReport`).

/// Maximum allowed length for a single dependency specifier string.
pub const DEP_NORM_MAX_SPEC_LEN: usize = 512;

/// Dependency protocol parsing, normalization, and override injection error set.
pub err DepListNormalizeError {
    EmptySpec,
    SpecTooLong,
    InvalidPackageName,
    UnsafeProtocol,
    PathTraversalDetected,
    InvalidAliasSpec,
    InvalidGitHubSpec,
    InvalidTarballUrl,
    InvalidOverrideRule,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, Path & Security Helpers
// ============================================================================

fn dnBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn dnBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn dnStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    return dnBytesEql(s[0..prefix.len], prefix);
}

fn dnEndsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) return false;
    return dnBytesEql(s[s.len - suffix.len..s.len], suffix);
}

fn dnIndexOf(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn dnLastIndexOf(s: []const u8, ch: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) return i;
    }
    return null;
}

fn dnTrimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn dnHasControlOrUnsafeChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F || c == ';' || c == '`' || c == '$') return true;
        i += 1;
    }
    return false;
}

fn dnHasDotDotSegment(path: []const u8): bool {
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

fn dnIsDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn dnIsAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn dnLooksLikeSemverRange(s: []const u8): bool {
    if (s.len == 0) return false;
    const c = s[0];
    if (dnIsDigit(c) || c == '^' || c == '~' || c == '>' || c == '<' || c == '=' || c == '*') {
        return true;
    }
    if ((c == 'v' || c == 'V') && s.len > 1 && dnIsDigit(s[1])) return true;
    return false;
}

fn dnAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn dnAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn dnAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        dnAppendByte(out, pos, '0')?;
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
        dnAppendByte(out, pos, tmp[n])?;
    }
}

// ============================================================================
// Section 1: All-Protocol Dependency Specifier Parser
// ============================================================================

/// All 10 supported dependency specifier protocol families.
pub enum DepProtocolKind: u8 {
    SemverRange = 0,
    DistTag = 1,
    NpmAlias = 2,
    WorkspaceProtocol = 3,
    CatalogProtocol = 4,
    FileProtocol = 5,
    LinkProtocol = 6,
    GitHubShorthand = 7,
    GitUrl = 8,
    TarballUrl = 9,

    fn asName(): []const u8 {
        return switch (_) {
            .SemverRange => "semver",
            .DistTag => "tag",
            .NpmAlias => "npm-alias",
            .WorkspaceProtocol => "workspace",
            .CatalogProtocol => "catalog",
            .FileProtocol => "file",
            .LinkProtocol => "link",
            .GitHubShorthand => "github",
            .GitUrl => "git",
            .TarballUrl => "tarball",
        };
    }
}

/// Decomposed dependency specifier returned by `parseDependencyProtocolSpec`.
pub struct ParsedDepProtocolSpec {
    kind: DepProtocolKind = DepProtocolKind.SemverRange,
    raw_spec: []const u8 = "",
    target_name: []const u8 = "",
    version_or_ref: []const u8 = "",
    local_or_url_path: []const u8 = "",
}

/// Parses and validates a dependency specifier across all 10 supported protocols,
/// rejecting unsafe schemes (`javascript:`, `data:`, `ftp:`, non-HTTPS tarballs)
/// and path traversal attempts.
pub fn parseDependencyProtocolSpec(raw_spec: []const u8): ParsedDepProtocolSpec! {
    const s = dnTrimSpaces(raw_spec);
    if (s.len == 0) return err.EmptySpec;
    if (s.len > DEP_NORM_MAX_SPEC_LEN) return err.SpecTooLong;
    if (dnHasControlOrUnsafeChar(s)) return err.UnsafeProtocol;

    // Block explicitly unsafe schemes
    if (dnStartsWith(s, "javascript:") || dnStartsWith(s, "data:") || dnStartsWith(s, "ftp://")) {
        return err.UnsafeProtocol;
    }

    // 1. npm:<alias>[@<ver>]
    if (dnStartsWith(s, "npm:")) {
        const body = s[4..s.len];
        if (body.len == 0 || dnHasDotDotSegment(body)) return err.InvalidAliasSpec;
        const search_from: usize = if (body[0] == '@') 1 else 0;
        if (rel_at := dnIndexOf(body[search_from..body.len], '@')) {
            const at_idx = search_from + rel_at;
            if (at_idx == 0 || at_idx + 1 >= body.len) return err.InvalidAliasSpec;
            return ParsedDepProtocolSpec{
                kind: DepProtocolKind.NpmAlias,
                raw_spec: s,
                target_name: body[0..at_idx],
                version_or_ref: body[at_idx + 1..body.len],
            };
        }
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.NpmAlias,
            raw_spec: s,
            target_name: body,
            version_or_ref: "*",
        };
    }

    // 2. workspace:* / workspace:^ / workspace:~ / workspace:<ver>
    if (dnStartsWith(s, "workspace:")) {
        const tail = s[10..s.len];
        if (tail.len == 0 || dnHasDotDotSegment(tail)) return err.PathTraversalDetected;
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.WorkspaceProtocol,
            raw_spec: s,
            version_or_ref: tail,
        };
    }

    // 3. catalog: / catalog:<name>
    if (dnStartsWith(s, "catalog:")) {
        const cat = s[8..s.len];
        if (cat.len > 0 && dnHasDotDotSegment(cat)) return err.PathTraversalDetected;
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.CatalogProtocol,
            raw_spec: s,
            version_or_ref: cat,
        };
    }

    // 4. file:<path>
    if (dnStartsWith(s, "file:")) {
        const p = s[5..s.len];
        if (p.len == 0) return err.EmptySpec;
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.FileProtocol,
            raw_spec: s,
            local_or_url_path: p,
        };
    }

    // 5. link:<path>
    if (dnStartsWith(s, "link:")) {
        const p = s[5..s.len];
        if (p.len == 0) return err.EmptySpec;
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.LinkProtocol,
            raw_spec: s,
            local_or_url_path: p,
        };
    }

    // 6. git+https:// / git+ssh:// / git://
    if (dnStartsWith(s, "git+https://") || dnStartsWith(s, "git+ssh://") || dnStartsWith(s, "git://")) {
        let url_part = s;
        let ref_part: []const u8 = "";
        if (hash := dnIndexOf(s, '#')) {
            url_part = s[0..hash];
            ref_part = s[hash + 1..s.len];
        }
        if (dnHasDotDotSegment(url_part) || dnHasDotDotSegment(ref_part)) {
            return err.PathTraversalDetected;
        }
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.GitUrl,
            raw_spec: s,
            local_or_url_path: url_part,
            version_or_ref: ref_part,
        };
    }

    // 7. HTTPS Tarball URL (reject plain HTTP tarballs as unsafe)
    if (dnStartsWith(s, "http://")) {
        return err.UnsafeProtocol;
    }
    if (dnStartsWith(s, "https://")) {
        const after = s[8..s.len];
        if (dnHasDotDotSegment(after)) return err.PathTraversalDetected;
        if (!dnEndsWith(after, ".tgz") && !dnEndsWith(after, ".tar.gz")) {
            return err.InvalidTarballUrl;
        }
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.TarballUrl,
            raw_spec: s,
            local_or_url_path: s,
        };
    }

    // 8. github:owner/repo[#ref]
    if (dnStartsWith(s, "github:")) {
        const body = s[7..s.len];
        let repo_part = body;
        let ref_part: []const u8 = "";
        if (hash := dnIndexOf(body, '#')) {
            repo_part = body[0..hash];
            ref_part = body[hash + 1..body.len];
        }
        if (dnHasDotDotSegment(repo_part) || dnHasDotDotSegment(ref_part)) {
            return err.PathTraversalDetected;
        }
        if (dnIndexOf(repo_part, '/') == null) return err.InvalidGitHubSpec;
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.GitHubShorthand,
            raw_spec: s,
            target_name: repo_part,
            version_or_ref: ref_part,
        };
    }

    // 9. SemVer range
    if (dnLooksLikeSemverRange(s)) {
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.SemverRange,
            raw_spec: s,
            version_or_ref: s,
        };
    }

    // 10. Bare GitHub shorthand `owner/repo[#ref]`
    if (dnIndexOf(s, '/') != null && s[0] != '@' && s[0] != '.') {
        let repo_part = s;
        let ref_part: []const u8 = "";
        if (hash := dnIndexOf(s, '#')) {
            repo_part = s[0..hash];
            ref_part = s[hash + 1..s.len];
        }
        if (dnHasDotDotSegment(repo_part) || dnHasDotDotSegment(ref_part)) {
            return err.PathTraversalDetected;
        }
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.GitHubShorthand,
            raw_spec: s,
            target_name: repo_part,
            version_or_ref: ref_part,
        };
    }

    // 11. Dist-tag (`latest`, `next`, `beta`, `rc-1`)
    if (dnIsAlpha(s[0]) && dnIndexOf(s, ':') == null && dnIndexOf(s, ' ') == null) {
        return ParsedDepProtocolSpec{
            kind: DepProtocolKind.DistTag,
            raw_spec: s,
            version_or_ref: s,
        };
    }

    return err.UnsafeProtocol;
}

// ============================================================================
// Section 2: Dependency Group Priority Merge, Deduplication & Sorting
// ============================================================================

/// Dependency section ordered by merge precedence:
/// `Dependencies (0) > OptionalDependencies (1) > DevDependencies (2) > PeerDependencies (3)`.
pub enum DepNormGroup: u8 {
    Dependencies = 0,
    OptionalDependencies = 1,
    DevDependencies = 2,
    PeerDependencies = 3,

    fn priorityRank(): u8 {
        return switch (_) {
            .Dependencies => 0,
            .OptionalDependencies => 1,
            .DevDependencies => 2,
            .PeerDependencies => 3,
        };
    }

    fn asJsonKey(): []const u8 {
        return switch (_) {
            .Dependencies => "dependencies",
            .OptionalDependencies => "optionalDependencies",
            .DevDependencies => "devDependencies",
            .PeerDependencies => "peerDependencies",
        };
    }
}

/// Raw dependency input item passed to `normalizeDependencyList`.
pub struct RawDepItem {
    name: []const u8 = "",
    spec: []const u8 = "",
    group: DepNormGroup = DepNormGroup.Dependencies,
    parent_name: []const u8 = "",

    static fn empty(): RawDepItem {
        return RawDepItem{};
    }
}

/// Normalized, deduplicated dependency entry produced by `normalizeDependencyList`.
pub struct NormalizedDepItem {
    name: []const u8 = "",
    spec: []const u8 = "",
    original_spec: []const u8 = "",
    group: DepNormGroup = DepNormGroup.Dependencies,
    protocol: DepProtocolKind = DepProtocolKind.SemverRange,
    parent_name: []const u8 = "",
    was_deduplicated: bool = false,
    was_overridden: bool = false,
    override_rule_key: []const u8 = "",

    static fn empty(): NormalizedDepItem {
        return NormalizedDepItem{};
    }
}

/// Aggregate normalization metrics returned by `normalizeDependencyList`.
pub struct DepNormalizeSummary {
    input_count: u32 = 0,
    output_count: u32 = 0,
    deduplicated_count: u32 = 0,
    conflict_resolved_count: u32 = 0,
}

/// Validates protocols, deduplicates同名 dependencies by canonical group priority
/// (`dependencies > optionalDependencies > devDependencies > peerDependencies`),
/// and sorts deterministically in lexicographical order into caller-owned `out_items`.
pub fn normalizeDependencyList(
    raw_items: []const RawDepItem,
    out_items: []NormalizedDepItem,
): DepNormalizeSummary! {
    let sum = DepNormalizeSummary{
        input_count: @as(u32, @intCast(raw_items.len)),
    };
    let count: usize = 0;

    let i: usize = 0;
    while (i < raw_items.len) {
        const item = raw_items[i];
        if (item.name.len == 0 || dnHasControlOrUnsafeChar(item.name)) {
            return err.InvalidPackageName;
        }
        const parsed = parseDependencyProtocolSpec(item.spec)?;

        // Search for existing entry with same (name, parent_name)
        let existing_idx: usize = count;
        let j: usize = 0;
        while (j < count) {
            if (dnBytesEql(out_items[j].name, item.name) && dnBytesEql(out_items[j].parent_name, item.parent_name)) {
                existing_idx = j;
                break;
            }
            j += 1;
        }

        if (existing_idx < count) {
            sum.deduplicated_count += 1;
            const cur = out_items[existing_idx];
            const spec_differs = !dnBytesEql(cur.spec, parsed.raw_spec);
            if (spec_differs) sum.conflict_resolved_count += 1;

            if (item.group.priorityRank() < cur.group.priorityRank()) {
                out_items[existing_idx] = NormalizedDepItem{
                    name: item.name,
                    spec: parsed.raw_spec,
                    original_spec: parsed.raw_spec,
                    group: item.group,
                    protocol: parsed.kind,
                    parent_name: item.parent_name,
                    was_deduplicated: true,
                };
            } else {
                out_items[existing_idx] = NormalizedDepItem{
                    name: cur.name,
                    spec: cur.spec,
                    original_spec: cur.original_spec,
                    group: cur.group,
                    protocol: cur.protocol,
                    parent_name: cur.parent_name,
                    was_deduplicated: true,
                };
            }
        } else {
            if (count >= out_items.len) return err.SliceCapacityExceeded;
            out_items[count] = NormalizedDepItem{
                name: item.name,
                spec: parsed.raw_spec,
                original_spec: parsed.raw_spec,
                group: item.group,
                protocol: parsed.kind,
                parent_name: item.parent_name,
            };
            count += 1;
        }
        i += 1;
    }

    // Deterministic lexicographical insertion sort by (name, parent_name)
    let a: usize = 1;
    while (a < count) {
        const key = out_items[a];
        let b = a;
        while (b > 0) {
            const prev = out_items[b - 1];
            const prev_greater = if (!dnBytesEql(prev.name, key.name))
                dnBytesLessThan(key.name, prev.name)
            else
                dnBytesLessThan(key.parent_name, prev.parent_name);
            if (!prev_greater) break;
            out_items[b] = prev;
            b -= 1;
        }
        out_items[b] = key;
        a += 1;
    }

    sum.output_count = @as(u32, @intCast(count));
    return sum;
}

// ============================================================================
// Section 3: Overrides / Resolutions Injector
// ============================================================================

/// Specificity tier of a dependency override rule.
pub enum DepOverrideRuleKind: u8 {
    ExactName = 0,
    VersionScoped = 1,
    ParentChildPath = 2,
}

/// Raw override/resolution rule (`"lodash": "4.17.21"`, `"lodash@^4.0.0": "4.17.21"`,
/// or `"parent>child": "^2.0.0"`).
pub struct DepOverrideRule {
    raw_key: []const u8 = "",
    replacement_spec: []const u8 = "",

    static fn empty(): DepOverrideRule {
        return DepOverrideRule{};
    }
}

/// Structured audit entry emitted whenever an override rule rewrites a dependency.
pub struct DepOverrideAuditEntry {
    package_name: []const u8 = "",
    parent_name: []const u8 = "",
    rule_key: []const u8 = "",
    rule_kind: DepOverrideRuleKind = DepOverrideRuleKind.ExactName,
    old_spec: []const u8 = "",
    new_spec: []const u8 = "",

    static fn empty(): DepOverrideAuditEntry {
        return DepOverrideAuditEntry{};
    }
}

/// Aggregate override injection statistics.
pub struct DepOverrideSummary {
    rules_evaluated: u32 = 0,
    applied_count: u32 = 0,
    parent_path_applied: u32 = 0,
    version_scoped_applied: u32 = 0,
    exact_name_applied: u32 = 0,
}

struct ParsedOverrideRule {
    kind: DepOverrideRuleKind = DepOverrideRuleKind.ExactName,
    parent_name: []const u8 = "",
    pkg_name: []const u8 = "",
    version_filter: []const u8 = "",
}

fn parseOverrideKey(raw_key: []const u8): ParsedOverrideRule! {
    const k = dnTrimSpaces(raw_key);
    if (k.len == 0) return err.InvalidOverrideRule;

    // Parent > Child rule: `"parent>child"` (optional spaces around `>`)
    if (gt := dnIndexOf(k, '>')) {
        const p = dnTrimSpaces(k[0..gt]);
        const c = dnTrimSpaces(k[gt + 1..k.len]);
        if (p.len == 0 || c.len == 0) return err.InvalidOverrideRule;
        return ParsedOverrideRule{
            kind: DepOverrideRuleKind.ParentChildPath,
            parent_name: p,
            pkg_name: c,
        };
    }

    // Version-scoped rule: `"lodash@^4.0.0"` or `"@scope/pkg@^1.0.0"`
    const search_from: usize = if (k[0] == '@') 1 else 0;
    if (rel_at := dnIndexOf(k[search_from..k.len], '@')) {
        const at_idx = search_from + rel_at;
        const pkg = k[0..at_idx];
        const ver = k[at_idx + 1..k.len];
        if (pkg.len == 0 || ver.len == 0) return err.InvalidOverrideRule;
        return ParsedOverrideRule{
            kind: DepOverrideRuleKind.VersionScoped,
            pkg_name: pkg,
            version_filter: ver,
        };
    }

    return ParsedOverrideRule{
        kind: DepOverrideRuleKind.ExactName,
        pkg_name: k,
    };
}

/// Applies override rules to `items` in precedence order (`ParentChildPath` >
/// `VersionScoped` > `ExactName`), updating specs in-place and recording an audit trail.
pub fn applyDependencyOverrides(
    items: []NormalizedDepItem,
    rules: []const DepOverrideRule,
    out_audit: []DepOverrideAuditEntry,
): DepOverrideSummary! {
    let sum = DepOverrideSummary{
        rules_evaluated: @as(u32, @intCast(rules.len)),
    };
    let audit_pos: usize = 0;

    // Validate all rules upfront
    let v: usize = 0;
    while (v < rules.len) {
        * = parseOverrideKey(rules[v].raw_key)?;
        * = parseDependencyProtocolSpec(rules[v].replacement_spec)?;
        v += 1;
    }

    let i: usize = 0;
    while (i < items.len) {
        const cur = items[i];
        let matched = false;
        let best_rank: u8 = 0; // 1 = Exact, 2 = VersionScoped, 3 = ParentChildPath
        let best_rule_idx: usize = 0;
        let best_kind = DepOverrideRuleKind.ExactName;

        let r: usize = 0;
        while (r < rules.len) {
            const pr = parseOverrideKey(rules[r].raw_key)?;
            let is_match = false;
            let rank: u8 = 0;

            switch (pr.kind) {
                .ParentChildPath => {
                    if (dnBytesEql(cur.parent_name, pr.parent_name) && dnBytesEql(cur.name, pr.pkg_name)) {
                        is_match = true;
                        rank = 3;
                    }
                },
                .VersionScoped => {
                    if (dnBytesEql(cur.name, pr.pkg_name) && dnBytesEql(cur.spec, pr.version_filter)) {
                        is_match = true;
                        rank = 2;
                    }
                },
                .ExactName => {
                    if (dnBytesEql(cur.name, pr.pkg_name)) {
                        is_match = true;
                        rank = 1;
                    }
                },
            }

            if (is_match && rank > best_rank) {
                matched = true;
                best_rank = rank;
                best_rule_idx = r;
                best_kind = pr.kind;
            }
            r += 1;
        }

        if (matched) {
            const rule = rules[best_rule_idx];
            const parsed_rep = parseDependencyProtocolSpec(rule.replacement_spec)?;
            items[i] = NormalizedDepItem{
                name: cur.name,
                spec: parsed_rep.raw_spec,
                original_spec: cur.original_spec,
                group: cur.group,
                protocol: parsed_rep.kind,
                parent_name: cur.parent_name,
                was_deduplicated: cur.was_deduplicated,
                was_overridden: true,
                override_rule_key: rule.raw_key,
            };

            sum.applied_count += 1;
            switch (best_kind) {
                .ParentChildPath => { sum.parent_path_applied += 1; },
                .VersionScoped => { sum.version_scoped_applied += 1; },
                .ExactName => { sum.exact_name_applied += 1; },
            }

            if (audit_pos >= out_audit.len) return err.SliceCapacityExceeded;
            out_audit[audit_pos] = DepOverrideAuditEntry{
                package_name: cur.name,
                parent_name: cur.parent_name,
                rule_key: rule.raw_key,
                rule_kind: best_kind,
                old_spec: cur.spec,
                new_spec: parsed_rep.raw_spec,
            };
            audit_pos += 1;
        }
        i += 1;
    }

    return sum;
}

// ============================================================================
// Section 4: Zero-Heap Dependency & Override Audit Report Formatter
// ============================================================================

/// Renders a readable dependency normalization and override audit report into `out`.
pub fn formatNormalizedDepReport(
    items: []const NormalizedDepItem,
    norm_sum: DepNormalizeSummary,
    ov_sum: DepOverrideSummary,
    out: []u8,
): usize! {
    let pos: usize = 0;
    dnAppendBytes(out, &pos, "Normalized Dependencies: ")?;
    dnAppendU64(out, &pos, @as(u64, norm_sum.output_count))?;
    dnAppendBytes(out, &pos, " unique (from ")?;
    dnAppendU64(out, &pos, @as(u64, norm_sum.input_count))?;
    dnAppendBytes(out, &pos, ", ")?;
    dnAppendU64(out, &pos, @as(u64, norm_sum.deduplicated_count))?;
    dnAppendBytes(out, &pos, " deduped, ")?;
    dnAppendU64(out, &pos, @as(u64, ov_sum.applied_count))?;
    dnAppendBytes(out, &pos, " overridden)\n")?;

    let i: usize = 0;
    while (i < items.len) {
        const it = items[i];
        dnAppendBytes(out, &pos, "  - ")?;
        if (it.parent_name.len > 0) {
            dnAppendBytes(out, &pos, it.parent_name)?;
            dnAppendByte(out, &pos, '>')?;
        }
        dnAppendBytes(out, &pos, it.name)?;
        dnAppendByte(out, &pos, '@')?;
        dnAppendBytes(out, &pos, it.spec)?;
        dnAppendBytes(out, &pos, " [")?;
        dnAppendBytes(out, &pos, it.group.asJsonKey())?;
        dnAppendByte(out, &pos, '/')?;
        dnAppendBytes(out, &pos, it.protocol.asName())?;
        dnAppendByte(out, &pos, ']')?;
        if (it.was_overridden) {
            dnAppendBytes(out, &pos, " (override: ")?;
            dnAppendBytes(out, &pos, it.original_spec)?;
            dnAppendBytes(out, &pos, " -> ")?;
            dnAppendBytes(out, &pos, it.spec)?;
            dnAppendByte(out, &pos, ')')?;
        }
        dnAppendByte(out, &pos, '\n')?;
        i += 1;
    }

    return pos;
}

/// Per-protocol distribution counters for normalized dependency lists.
pub struct DepProtocolCounts {
    semver_count: u32 = 0,
    dist_tag_count: u32 = 0,
    npm_alias_count: u32 = 0,
    workspace_count: u32 = 0,
    catalog_count: u32 = 0,
    file_or_link_count: u32 = 0,
    git_or_github_count: u32 = 0,
    tarball_count: u32 = 0,
}

/// Counts normalized dependencies grouped by their parsed protocol family.
pub fn summarizeDepProtocols(items: []const NormalizedDepItem): DepProtocolCounts {
    let counts = DepProtocolCounts{};
    let i: usize = 0;
    while (i < items.len) {
        switch (items[i].protocol) {
            .SemverRange => { counts.semver_count += 1; },
            .DistTag => { counts.dist_tag_count += 1; },
            .NpmAlias => { counts.npm_alias_count += 1; },
            .WorkspaceProtocol => { counts.workspace_count += 1; },
            .CatalogProtocol => { counts.catalog_count += 1; },
            .FileProtocol, .LinkProtocol => { counts.file_or_link_count += 1; },
            .GitHubShorthand, .GitUrl => { counts.git_or_github_count += 1; },
            .TarballUrl => { counts.tarball_count += 1; },
        }
        i += 1;
    }
    return counts;
}

/// Performs O(log N) binary search for `(name, parent_name)` in a sorted `NormalizedDepItem` slice.
pub fn findNormalizedDep(
    sorted_items: []const NormalizedDepItem,
    name: []const u8,
    parent_name: []const u8,
): usize? {
    let lo: usize = 0;
    let hi: usize = sorted_items.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        const cur = sorted_items[mid];
        if (dnBytesEql(cur.name, name) && dnBytesEql(cur.parent_name, parent_name)) {
            return mid;
        }
        const cur_less = if (!dnBytesEql(cur.name, name))
            dnBytesLessThan(cur.name, name)
        else
            dnBytesLessThan(cur.parent_name, parent_name);
        if (cur_less) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return null;
}
