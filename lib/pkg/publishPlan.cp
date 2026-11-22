// Source: Absorbed from /data/dev/bun/src/cli/publish_command.cp | Synced: 2026-11-22T13:58:06Z
// ============================================================================
// lib/pkg/publishPlan.cp — Zero-Alloc Package Publish Planner & Registry Codec
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven `cap publish` planning engine:
// package name / SemVer / dist-tag / access validation, `workspace:*` & `catalog:`
// protocol rewriter, `publishConfig` override resolver, Bearer/Basic/OTP/OIDC
// header builder, standard Base64 encoder, NPM/Cap Registry PUT JSON payload
// generator, dry-run reporter, and 5-stage publish lifecycle state machine.

/// Default npm/cap distribution tag when none is specified.
pub const PUBLISH_DEFAULT_TAG: []const u8 = "latest";

/// Default registry base URL for publishing.
pub const PUBLISH_DEFAULT_REGISTRY: []const u8 = "https://registry.npmjs.org";

/// Standard MIME type for `_attachments.<tarball>.content_type`.
pub const PUBLISH_CONTENT_TYPE_TGZ: []const u8 = "application/octet-stream";

/// Publish planning, validation, rewriting, auth, and payload generation error set.
pub err PublishPlanError {
    EmptyPackageName,
    InvalidPackageName,
    EmptyVersion,
    InvalidSemver,
    PrivatePackageBlocked,
    InvalidDistTag,
    DistTagConflictsWithSemver,
    RestrictedUnscopedPackage,
    InvalidRegistryUrl,
    InvalidAccessValue,
    UnresolvedWorkspaceDep,
    UnresolvedCatalogDep,
    InvalidOtpCode,
    MissingAuthCredentials,
    InvalidIntegrityFormat,
    InvalidShasumFormat,
    InvalidStageTransition,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Package visibility policy (`--access` or `publishConfig.access`).
pub enum PublishAccessLevel: u8 {
    Public = 0,
    Restricted = 1,
}

/// Registry authentication mechanism.
pub enum PublishAuthMethod: u8 {
    None = 0,
    BearerToken = 1,
    BasicUserPass = 2,
    OidcProvenance = 3,
}

/// `npm-auth-type` HTTP header mode.
pub enum PublishAuthTypeHeader: u8 {
    Legacy = 0,
    Web = 1,
}

/// Dependency section kind inside a published manifest.
pub enum PublishDepKind: u8 {
    Dependencies = 0,
    OptionalDependencies = 1,
    PeerDependencies = 2,
}

/// Five-stage atomic publish lifecycle state machine.
pub enum PublishStage: u8 {
    Init = 0,
    Validated = 1,
    Packed = 2,
    Signed = 3,
    Uploaded = 4,
    Verified = 5,
    Failed = 6,
}

/// Single dependency entry to be rewritten prior to publishing.
pub struct PublishDepSpec {
    name: []const u8 = "",
    spec: []const u8 = "",
    kind: PublishDepKind = PublishDepKind.Dependencies,

    static fn empty(): PublishDepSpec {
        return PublishDepSpec{};
    }
}

/// Workspace member package descriptor for resolving `workspace:*` / `^` / `~`.
pub struct PublishWorkspaceTarget {
    name: []const u8 = "",
    version: []const u8 = "",
}

/// Catalog entry for resolving `catalog:` (default) or `catalog:<name>`.
pub struct PublishCatalogEntry {
    catalog_name: []const u8 = "",
    pkg_name: []const u8 = "",
    version: []const u8 = "",
}

/// `publishConfig` overrides from `package.json` / `cap.json`.
pub struct PublishConfigOverrides {
    main: ([]const u8)? = null,
    module: ([]const u8)? = null,
    exports: ([]const u8)? = null,
    bin: ([]const u8)? = null,
    registry: ([]const u8)? = null,
    access: PublishAccessLevel? = null,
    tag: ([]const u8)? = null,
    provenance: bool? = null,

    static fn empty(): PublishConfigOverrides {
        return PublishConfigOverrides{};
    }
}

/// Effective manifest entrypoints after applying `publishConfig` overrides.
pub struct EffectivePublishFields {
    main: []const u8 = "",
    module: []const u8 = "",
    exports: []const u8 = "",
    bin: []const u8 = "",
    registry: []const u8 = PUBLISH_DEFAULT_REGISTRY,
    tag: []const u8 = PUBLISH_DEFAULT_TAG,
    access: PublishAccessLevel = PublishAccessLevel.Public,
    provenance: bool = false,
}

/// Input package metadata for publish validation and payload construction.
pub struct PublishPackageInput {
    name: []const u8 = "",
    version: []const u8 = "",
    description: []const u8 = "",
    license: []const u8 = "",
    is_private: bool = false,
    main: []const u8 = "",
    module: []const u8 = "",
    exports: []const u8 = "",
    bin: []const u8 = "",
    publish_config: PublishConfigOverrides = PublishConfigOverrides.empty(),
}

/// CLI / toolchain options passed to `cap publish`.
pub struct PublishOptions {
    registry_url: ([]const u8)? = null,
    tag: ([]const u8)? = null,
    access: PublishAccessLevel? = null,
    dry_run: bool = false,
    otp: []const u8 = "",
    auth_type: PublishAuthTypeHeader = PublishAuthTypeHeader.Web,
    provenance: bool = false,

    static fn defaults(): PublishOptions {
        return PublishOptions{};
    }
}

/// Authentication & 2FA credentials for registry HTTP request header generation.
pub struct PublishAuthCredentials {
    method: PublishAuthMethod = PublishAuthMethod.None,
    token: []const u8 = "",
    username: []const u8 = "",
    password: []const u8 = "",
    otp: []const u8 = "",
    auth_type: PublishAuthTypeHeader = PublishAuthTypeHeader.Web,
    provenance: bool = false,
}

/// Zero-copy HTTP header pair emitted by `buildPublishAuthHeaders`.
pub struct PublishHttpHeader {
    name: []const u8 = "",
    value: []const u8 = "",

    static fn empty(): PublishHttpHeader {
        return PublishHttpHeader{};
    }
}

/// Packed tarball distribution metadata for Registry PUT JSON payload.
pub struct PublishTarballDist {
    tarball_filename: []const u8 = "",
    tarball_url: []const u8 = "",
    integrity: []const u8 = "",
    shasum: []const u8 = "",
    file_count: u32 = 0,
    unpacked_size: usize = 0,
    tarball_bytes: []const u8 = "",
}

/// Validated publish plan summary.
pub struct PublishPlanSummary {
    package_name: []const u8 = "",
    version: []const u8 = "",
    effective_tag: []const u8 = PUBLISH_DEFAULT_TAG,
    effective_registry: []const u8 = PUBLISH_DEFAULT_REGISTRY,
    effective_access: PublishAccessLevel = PublishAccessLevel.Public,
    is_scoped: bool = false,
    dry_run: bool = false,
    provenance_enabled: bool = false,
    rewritten_deps_count: u32 = 0,
    tarball_size: usize = 0,
    unpacked_size: usize = 0,
    file_count: u32 = 0,
}

/// Five-stage publish lifecycle state machine:
/// `Init -> Validated -> Packed -> Signed -> Uploaded -> Verified` (or `Failed`).
pub struct PublishLifecycleSession {
    stage: PublishStage = PublishStage.Init,
    package_name: []const u8 = "",
    version: []const u8 = "",
    tarball_size: usize = 0,
    dry_run: bool = false,
    last_error_code: u16 = 0,

    static fn init(): PublishLifecycleSession {
        return PublishLifecycleSession{};
    }

    fn markValidated(*_, summary: PublishPlanSummary)! {
        if (_.stage != PublishStage.Init) return PublishPlanError.InvalidStageTransition;
        _.package_name = summary.package_name;
        _.version = summary.version;
        _.dry_run = summary.dry_run;
        _.stage = PublishStage.Validated;
    }

    fn markPacked(*_, tarball_size: usize)! {
        if (_.stage != PublishStage.Validated) return PublishPlanError.InvalidStageTransition;
        _.tarball_size = tarball_size;
        _.stage = PublishStage.Packed;
    }

    fn markSigned(*_)! {
        if (_.stage != PublishStage.Packed) return PublishPlanError.InvalidStageTransition;
        _.stage = PublishStage.Signed;
    }

    fn markUploaded(*_)! {
        if (_.stage != PublishStage.Signed) return PublishPlanError.InvalidStageTransition;
        _.stage = PublishStage.Uploaded;
    }

    fn markVerified(*_)! {
        if (_.stage != PublishStage.Uploaded) return PublishPlanError.InvalidStageTransition;
        _.stage = PublishStage.Verified;
    }

    fn markFailed(*_, err_code: u16) {
        _.last_error_code = err_code;
        _.stage = PublishStage.Failed;
    }

    fn isComplete(): bool {
        return _.stage == PublishStage.Verified;
    }
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
    if (pos.* + data.len > out_buf.len) {
        return PublishPlanError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendByte(out_buf: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out_buf.len) {
        return PublishPlanError.BufferTooSmall;
    }
    out_buf[pos.*] = b;
    pos.* += 1;
}

fn appendUsizeDecimal(out_buf: []u8, pos: *usize, value: usize)! {
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
    if (pos.* + len > out_buf.len) {
        return PublishPlanError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
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
        } else if (c < 32) {
            // Control char -> \u00XX
            const hex: []const u8 = "0123456789abcdef";
            appendBytes(out_buf, pos, "\\u00")?;
            appendByte(out_buf, pos, hex[@as(usize, c >> 4)])?;
            appendByte(out_buf, pos, hex[@as(usize, c & 15)])?;
        } else {
            appendByte(out_buf, pos, c)?;
        }
        i += 1;
    }
    appendByte(out_buf, pos, @as(u8, 34))?; // '"'
}

/// Standard RFC 4648 Base64 encoder (zero-alloc, writes into caller's `out_buf`).
pub fn encodeBase64Standard(input: []const u8, out_buf: []u8): usize! {
    const alphabet: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const out_len = ((input.len + 2) / 3) * 4;
    if (out_len > out_buf.len) {
        return PublishPlanError.BufferTooSmall;
    }
    let i: usize = 0;
    let o: usize = 0;
    while (i < input.len) {
        const b0: u32 = @as(u32, input[i]);
        const has1 = (i + 1 < input.len);
        const has2 = (i + 2 < input.len);
        const b1: u32 = if (has1) @as(u32, input[i + 1]) else 0;
        const b2: u32 = if (has2) @as(u32, input[i + 2]) else 0;

        const triple = (b0 << 16) | (b1 << 8) | b2;
        out_buf[o] = alphabet[@as(usize, (triple >> 18) & 63)];
        out_buf[o + 1] = alphabet[@as(usize, (triple >> 12) & 63)];
        out_buf[o + 2] = if (has1) alphabet[@as(usize, (triple >> 6) & 63)] else @as(u8, 61); // '='
        out_buf[o + 3] = if (has2) alphabet[@as(usize, triple & 63)] else @as(u8, 61);
        i += 3;
        o += 4;
    }
    return o;
}

// ============================================================================
// Package Name, SemVer, Dist-Tag & Access Policy Validators
// ============================================================================

fn isValidPkgNameChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
}

fn isValidPkgSegment(seg: []const u8): bool {
    if (seg.len == 0 || seg.len > 214) return false;
    if (seg[0] == '.' || seg[0] == '_') return false;
    let i: usize = 0;
    while (i < seg.len) {
        if (!isValidPkgNameChar(seg[i])) return false;
        i += 1;
    }
    return true;
}

/// Validates an npm/cap package name (scoped `@scope/name` or unscoped `name`).
pub fn isValidPublishPkgName(name: []const u8): bool {
    if (name.len == 0 || name.len > 214) return false;
    if (name[0] == '@') {
        const slash_opt = indexOfByte(name, '/');
        if (slash_opt == null) return false;
        const s_idx = slash_opt.?;
        if (s_idx <= 1 || s_idx + 1 >= name.len) return false;
        // Reject second slash
        if (indexOfByte(name[s_idx + 1 ..], '/') != null) return false;
        return isValidPkgSegment(name[1..s_idx]) && isValidPkgSegment(name[s_idx + 1 ..]);
    }
    if (indexOfByte(name, '/') != null) return false;
    return isValidPkgSegment(name);
}

/// Returns true if `name` is a valid scoped package name (`@scope/pkg`).
pub fn isScopedPublishPkgName(name: []const u8): bool {
    return name.len > 0 && name[0] == '@' && isValidPublishPkgName(name);
}

/// URL-encodes a package name for registry endpoints (`@scope/pkg` -> `@scope%2fpkg`).
pub fn encodePackageNameForRegistry(name: []const u8, out_buf: []u8): usize! {
    if (!isValidPublishPkgName(name)) {
        return PublishPlanError.InvalidPackageName;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < name.len) {
        if (name[i] == '/') {
            appendBytes(out_buf, &pos, "%2f")?;
        } else {
            appendByte(out_buf, &pos, name[i])?;
        }
        i += 1;
    }
    return pos;
}

fn isValidNumericId(seg: []const u8): bool {
    if (seg.len == 0) return false;
    if (seg.len > 1 && seg[0] == '0') return false;
    let i: usize = 0;
    while (i < seg.len) {
        if (seg[i] < '0' || seg[i] > '9') return false;
        i += 1;
    }
    return true;
}

fn isValidIdentSeg(seg: []const u8): bool {
    if (seg.len == 0) return false;
    let i: usize = 0;
    while (i < seg.len) {
        const c = seg[i];
        const ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

/// Strictly validates a SemVer 2.0.0 version string (`MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`).
pub fn isValidPublishSemver(ver: []const u8): bool {
    if (ver.len == 0 || ver.len > 256) return false;

    // Strip optional +build metadata
    let core_and_pre = ver;
    if (plus_idx := indexOfByte(ver, '+')) {
        const build = ver[plus_idx + 1 ..];
        if (build.len == 0) return false;
        let b_start: usize = 0;
        let bi: usize = 0;
        while (bi <= build.len) {
            if (bi == build.len || build[bi] == '.') {
                if (!isValidIdentSeg(build[b_start..bi])) return false;
                b_start = bi + 1;
            }
            bi += 1;
        }
        core_and_pre = ver[0..plus_idx];
    }

    // Split optional -prerelease
    let core = core_and_pre;
    if (dash_idx := indexOfByte(core_and_pre, '-')) {
        const pre = core_and_pre[dash_idx + 1 ..];
        if (pre.len == 0) return false;
        let p_start: usize = 0;
        let pi: usize = 0;
        while (pi <= pre.len) {
            if (pi == pre.len || pre[pi] == '.') {
                const seg = pre[p_start..pi];
                if (!isValidIdentSeg(seg)) return false;
                p_start = pi + 1;
            }
            pi += 1;
        }
        core = core_and_pre[0..dash_idx];
    }

    // Parse MAJOR.MINOR.PATCH
    const dot1_opt = indexOfByte(core, '.');
    if (dot1_opt == null) return false;
    const dot1 = dot1_opt.?;
    const rest1 = core[dot1 + 1 ..];
    const dot2_opt = indexOfByte(rest1, '.');
    if (dot2_opt == null) return false;
    const dot2 = dot2_opt.?;
    const patch = rest1[dot2 + 1 ..];
    if (indexOfByte(patch, '.') != null) return false;

    return isValidNumericId(core[0..dot1]) && isValidNumericId(rest1[0..dot2]) && isValidNumericId(patch);
}

/// Validates a registry `dist-tag` (e.g. `"latest"`, `"next"`, `"canary"`).
/// Rejects empty tags, whitespace, and tags that parse as valid SemVer (with or without leading `v`).
pub fn validatePublishDistTag(tag: []const u8)! {
    if (tag.len == 0 || tag.len > 128) {
        return PublishPlanError.InvalidDistTag;
    }
    let i: usize = 0;
    while (i < tag.len) {
        const c = tag[i];
        if (c <= 32 || c == '/' || c == '\\' || c == '?' || c == '#') {
            return PublishPlanError.InvalidDistTag;
        }
        i += 1;
    }
    if (isValidPublishSemver(tag)) {
        return PublishPlanError.DistTagConflictsWithSemver;
    }
    if (tag.len > 1 && (tag[0] == 'v' || tag[0] == 'V') && isValidPublishSemver(tag[1..])) {
        return PublishPlanError.DistTagConflictsWithSemver;
    }
}

fn isValidHttpRegistryUrl(url: []const u8): bool {
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
    return true;
}

/// Resolves effective entrypoints & publish metadata after applying `publishConfig` and CLI overrides.
/// Priority: `CLI opts > package.json publishConfig > package.json top-level > defaults`.
pub fn applyPublishConfigOverrides(
    input: PublishPackageInput,
    opts: PublishOptions,
): EffectivePublishFields {
    const pc = input.publish_config;
    const is_scoped = isScopedPublishPkgName(input.name);
    const default_access = if (is_scoped) PublishAccessLevel.Restricted else PublishAccessLevel.Public;

    return EffectivePublishFields{
        main: pc.main ?? input.main,
        module: pc.module ?? input.module,
        exports: pc.exports ?? input.exports,
        bin: pc.bin ?? input.bin,
        registry: opts.registry_url ?? (pc.registry ?? PUBLISH_DEFAULT_REGISTRY),
        tag: opts.tag ?? (pc.tag ?? PUBLISH_DEFAULT_TAG),
        access: opts.access ?? (pc.access ?? default_access),
        provenance: opts.provenance || (pc.provenance ?? false),
    };
}

/// Validates all publish preconditions and produces a `PublishPlanSummary`.
pub fn validatePublishInput(
    input: PublishPackageInput,
    opts: PublishOptions,
): PublishPlanSummary! {
    if (input.name.len == 0) return PublishPlanError.EmptyPackageName;
    if (!isValidPublishPkgName(input.name)) return PublishPlanError.InvalidPackageName;
    if (input.version.len == 0) return PublishPlanError.EmptyVersion;
    if (!isValidPublishSemver(input.version)) return PublishPlanError.InvalidSemver;
    if (input.is_private) return PublishPlanError.PrivatePackageBlocked;

    const eff = applyPublishConfigOverrides(input, opts);
    const is_scoped = isScopedPublishPkgName(input.name);

    validatePublishDistTag(eff.tag)?;
    if (!isValidHttpRegistryUrl(eff.registry)) {
        return PublishPlanError.InvalidRegistryUrl;
    }
    if (eff.access == PublishAccessLevel.Restricted && !is_scoped) {
        return PublishPlanError.RestrictedUnscopedPackage;
    }

    return PublishPlanSummary{
        package_name: input.name,
        version: input.version,
        effective_tag: eff.tag,
        effective_registry: eff.registry,
        effective_access: eff.access,
        is_scoped: is_scoped,
        dry_run: opts.dry_run,
        provenance_enabled: eff.provenance,
    };
}

// ============================================================================
// `workspace:` & `catalog:` Pre-Publish Dependency Rewriter (Zero-Alloc)
// ============================================================================

fn findWorkspaceVersion(workspaces: []const PublishWorkspaceTarget, pkg_name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < workspaces.len) {
        if (bytesEqual(workspaces[i].name, pkg_name)) {
            return workspaces[i].version;
        }
        i += 1;
    }
    return null;
}

fn findCatalogVersion(
    catalogs: []const PublishCatalogEntry,
    catalog_name: []const u8,
    pkg_name: []const u8,
): ([]const u8)? {
    let i: usize = 0;
    while (i < catalogs.len) {
        if (bytesEqual(catalogs[i].catalog_name, catalog_name) && bytesEqual(catalogs[i].pkg_name, pkg_name)) {
            return catalogs[i].version;
        }
        i += 1;
    }
    return null;
}

/// Rewrites a single dependency specifier if it uses `workspace:` or `catalog:` protocol.
/// Non-protocol specifiers are returned zero-copy unchanged.
pub fn rewritePublishDepSpec(
    dep_name: []const u8,
    raw_spec: []const u8,
    workspaces: []const PublishWorkspaceTarget,
    catalogs: []const PublishCatalogEntry,
    str_arena: []u8,
    arena_pos: *usize,
): ([]const u8)! {
    if (startsWith(raw_spec, "workspace:")) {
        const rest = raw_spec[10..];
        const ws_ver_opt = findWorkspaceVersion(workspaces, dep_name);
        if (ws_ver_opt == null) {
            return PublishPlanError.UnresolvedWorkspaceDep;
        }
        const ws_ver = ws_ver_opt.?;
        if (bytesEqual(rest, "*")) {
            return ws_ver;
        }
        if (bytesEqual(rest, "^") || bytesEqual(rest, "~")) {
            const start = arena_pos.*;
            appendByte(str_arena, arena_pos, rest[0])?;
            appendBytes(str_arena, arena_pos, ws_ver)?;
            return str_arena[start..arena_pos.*];
        }
        if (rest.len > 0) {
            return rest;
        }
        return ws_ver;
    }

    if (startsWith(raw_spec, "catalog:")) {
        const cat_name = raw_spec[8..];
        const cat_ver_opt = findCatalogVersion(catalogs, cat_name, dep_name);
        if (cat_ver_opt == null) {
            return PublishPlanError.UnresolvedCatalogDep;
        }
        return cat_ver_opt.?;
    }

    return raw_spec;
}

/// Rewrites an entire slice of dependencies into `out_deps`, returning the number
/// of dependencies that were actively rewritten from `workspace:` or `catalog:`.
pub fn rewritePublishDependencies(
    deps: []const PublishDepSpec,
    workspaces: []const PublishWorkspaceTarget,
    catalogs: []const PublishCatalogEntry,
    out_deps: []PublishDepSpec,
    str_arena: []u8,
    arena_pos: *usize,
): u32! {
    if (deps.len > out_deps.len) {
        return PublishPlanError.SliceCapacityExceeded;
    }
    let rewritten_count: u32 = 0;
    let i: usize = 0;
    while (i < deps.len) {
        const d = deps[i];
        const new_spec = rewritePublishDepSpec(d.name, d.spec, workspaces, catalogs, str_arena, arena_pos)?;
        if (!bytesEqual(new_spec, d.spec)) {
            rewritten_count += 1;
        }
        out_deps[i] = PublishDepSpec{
            name: d.name,
            spec: new_spec,
            kind: d.kind,
        };
        i += 1;
    }
    return rewritten_count;
}

// ============================================================================
// Auth, OTP & Provenance HTTP Header Builder
// ============================================================================

fn isValidOtpCode(otp: []const u8): bool {
    if (otp.len < 6 || otp.len > 8) return false;
    let i: usize = 0;
    while (i < otp.len) {
        if (otp[i] < '0' || otp[i] > '9') return false;
        i += 1;
    }
    return true;
}

fn pushHeader(
    headers_out: []PublishHttpHeader,
    count: *usize,
    name: []const u8,
    value: []const u8,
)! {
    if (count.* >= headers_out.len) {
        return PublishPlanError.SliceCapacityExceeded;
    }
    headers_out[count.*] = PublishHttpHeader{ name: name, value: value };
    count.* += 1;
}

/// Builds HTTP headers for a Registry PUT publish request into `headers_out`,
/// backing dynamic values (Bearer/Basic auth) in caller-owned `arena`.
pub fn buildPublishAuthHeaders(
    creds: PublishAuthCredentials,
    headers_out: []PublishHttpHeader,
    arena: []u8,
    arena_pos: *usize,
): usize! {
    let count: usize = 0;

    if (creds.method == PublishAuthMethod.BearerToken || creds.method == PublishAuthMethod.OidcProvenance) {
        if (creds.token.len == 0) return PublishPlanError.MissingAuthCredentials;
        const start = arena_pos.*;
        appendBytes(arena, arena_pos, "Bearer ")?;
        appendBytes(arena, arena_pos, creds.token)?;
        pushHeader(headers_out, &count, "authorization", arena[start..arena_pos.*])?;
    } else if (creds.method == PublishAuthMethod.BasicUserPass) {
        if (creds.username.len == 0 || creds.password.len == 0) {
            return PublishPlanError.MissingAuthCredentials;
        }
        let raw_pair: [256]u8 = [0; 256];
        let raw_pos: usize = 0;
        appendBytes(raw_pair[0..], &raw_pos, creds.username)?;
        appendByte(raw_pair[0..], &raw_pos, @as(u8, 58))?; // ':'
        appendBytes(raw_pair[0..], &raw_pos, creds.password)?;

        let b64_buf: [384]u8 = [0; 384];
        const b64_len = encodeBase64Standard(raw_pair[0..raw_pos], b64_buf[0..])?;

        const start = arena_pos.*;
        appendBytes(arena, arena_pos, "Basic ")?;
        appendBytes(arena, arena_pos, b64_buf[0..b64_len])?;
        pushHeader(headers_out, &count, "authorization", arena[start..arena_pos.*])?;
    } else {
        return PublishPlanError.MissingAuthCredentials;
    }

    const auth_type_str: []const u8 = if (creds.auth_type == PublishAuthTypeHeader.Web) "web" else "legacy";
    pushHeader(headers_out, &count, "npm-auth-type", auth_type_str)?;

    if (creds.otp.len > 0) {
        if (!isValidOtpCode(creds.otp)) return PublishPlanError.InvalidOtpCode;
        pushHeader(headers_out, &count, "npm-otp", creds.otp)?;
    }

    if (creds.provenance || creds.method == PublishAuthMethod.OidcProvenance) {
        pushHeader(headers_out, &count, "npm-provenance", "true")?;
    }

    pushHeader(headers_out, &count, "content-type", "application/json")?;
    return count;
}

// ============================================================================
// Registry Endpoint URLs & PUT JSON Payload Generator (Zero-Alloc)
// ============================================================================

fn stripTrailingSlash(url: []const u8): []const u8 {
    let end = url.len;
    while (end > 0 && url[end - 1] == '/') {
        end -= 1;
    }
    return url[0..end];
}

/// Formats the Registry PUT endpoint URL: `<registry>/<encoded_pkg_name>`.
pub fn buildPublishPutUrl(registry_url: []const u8, pkg_name: []const u8, out_buf: []u8): usize! {
    if (!isValidHttpRegistryUrl(registry_url)) return PublishPlanError.InvalidRegistryUrl;
    let pos: usize = 0;
    appendBytes(out_buf, &pos, stripTrailingSlash(registry_url))?;
    appendByte(out_buf, &pos, @as(u8, 47))?; // '/'
    let enc_buf: [256]u8 = [0; 256];
    const enc_len = encodePackageNameForRegistry(pkg_name, enc_buf[0..])?;
    appendBytes(out_buf, &pos, enc_buf[0..enc_len])?;
    return pos;
}

/// Formats the standard tarball download URL: `<registry>/<pkg_name>/-/<unscoped>-<ver>.tgz`.
pub fn formatPublishTarballUrl(
    registry_url: []const u8,
    pkg_name: []const u8,
    version: []const u8,
    out_buf: []u8,
): usize! {
    if (!isValidHttpRegistryUrl(registry_url)) return PublishPlanError.InvalidRegistryUrl;
    if (!isValidPublishPkgName(pkg_name)) return PublishPlanError.InvalidPackageName;
    if (!isValidPublishSemver(version)) return PublishPlanError.InvalidSemver;

    let pos: usize = 0;
    appendBytes(out_buf, &pos, stripTrailingSlash(registry_url))?;
    appendByte(out_buf, &pos, @as(u8, 47))?; // '/'
    appendBytes(out_buf, &pos, pkg_name)?;
    appendBytes(out_buf, &pos, "/-/")?;

    // Unscoped basename after '/' if scoped
    let base_name = pkg_name;
    if (slash_idx := indexOfByte(pkg_name, '/')) {
        base_name = pkg_name[slash_idx + 1 ..];
    }
    appendBytes(out_buf, &pos, base_name)?;
    appendByte(out_buf, &pos, @as(u8, 45))?; // '-'
    appendBytes(out_buf, &pos, version)?;
    appendBytes(out_buf, &pos, ".tgz")?;
    return pos;
}

fn isValidSha1Hex(s: []const u8): bool {
    if (s.len != 40) return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        const ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!ok) return false;
        i += 1;
    }
    return true;
}

fn emitDepSectionJson(
    out_buf: []u8,
    pos: *usize,
    field_name: []const u8,
    deps: []const PublishDepSpec,
    kind: PublishDepKind,
)! {
    let count: usize = 0;
    let i: usize = 0;
    while (i < deps.len) {
        if (deps[i].kind == kind) count += 1;
        i += 1;
    }
    if (count == 0) return;

    appendBytes(out_buf, pos, ",")?;
    appendJsonEscapedString(out_buf, pos, field_name)?;
    appendBytes(out_buf, pos, ":{")?;
    let emitted: usize = 0;
    i = 0;
    while (i < deps.len) {
        if (deps[i].kind == kind) {
            if (emitted > 0) appendBytes(out_buf, pos, ",")?;
            appendJsonEscapedString(out_buf, pos, deps[i].name)?;
            appendBytes(out_buf, pos, ":")?;
            appendJsonEscapedString(out_buf, pos, deps[i].spec)?;
            emitted += 1;
        }
        i += 1;
    }
    appendBytes(out_buf, pos, "}")?;
}

/// Builds a complete standard NPM/Cap Registry PUT publish JSON payload into `out_buf`
/// with zero heap allocation. Validates `sha512-...` integrity and 40-hex `shasum`.
pub fn buildPublishRegistryPayload(
    summary: PublishPlanSummary,
    input: PublishPackageInput,
    rewritten_deps: []const PublishDepSpec,
    dist: PublishTarballDist,
    out_buf: []u8,
): usize! {
    if (!startsWith(dist.integrity, "sha512-") || dist.integrity.len <= 7) {
        return PublishPlanError.InvalidIntegrityFormat;
    }
    if (!isValidSha1Hex(dist.shasum)) {
        return PublishPlanError.InvalidShasumFormat;
    }

    const eff = applyPublishConfigOverrides(input, PublishOptions{
        registry_url: summary.effective_registry,
        tag: summary.effective_tag,
        access: summary.effective_access,
        dry_run: summary.dry_run,
        provenance: summary.provenance_enabled,
    });

    let pos: usize = 0;
    appendBytes(out_buf, &pos, "{\"_id\":")?;
    appendJsonEscapedString(out_buf, &pos, summary.package_name)?;
    appendBytes(out_buf, &pos, ",\"name\":")?;
    appendJsonEscapedString(out_buf, &pos, summary.package_name)?;

    if (input.description.len > 0) {
        appendBytes(out_buf, &pos, ",\"description\":")?;
        appendJsonEscapedString(out_buf, &pos, input.description)?;
    }

    // "dist-tags"
    appendBytes(out_buf, &pos, ",\"dist-tags\":{")?;
    appendJsonEscapedString(out_buf, &pos, summary.effective_tag)?;
    appendBytes(out_buf, &pos, ":")?;
    appendJsonEscapedString(out_buf, &pos, summary.version)?;
    appendBytes(out_buf, &pos, "}")?;

    // "versions"
    appendBytes(out_buf, &pos, ",\"versions\":{")?;
    appendJsonEscapedString(out_buf, &pos, summary.version)?;
    appendBytes(out_buf, &pos, ":{\"name\":")?;
    appendJsonEscapedString(out_buf, &pos, summary.package_name)?;
    appendBytes(out_buf, &pos, ",\"version\":")?;
    appendJsonEscapedString(out_buf, &pos, summary.version)?;

    if (input.description.len > 0) {
        appendBytes(out_buf, &pos, ",\"description\":")?;
        appendJsonEscapedString(out_buf, &pos, input.description)?;
    }
    if (input.license.len > 0) {
        appendBytes(out_buf, &pos, ",\"license\":")?;
        appendJsonEscapedString(out_buf, &pos, input.license)?;
    }
    if (eff.main.len > 0) {
        appendBytes(out_buf, &pos, ",\"main\":")?;
        appendJsonEscapedString(out_buf, &pos, eff.main)?;
    }
    if (eff.module.len > 0) {
        appendBytes(out_buf, &pos, ",\"module\":")?;
        appendJsonEscapedString(out_buf, &pos, eff.module)?;
    }

    emitDepSectionJson(out_buf, &pos, "dependencies", rewritten_deps, PublishDepKind.Dependencies)?;
    emitDepSectionJson(out_buf, &pos, "optionalDependencies", rewritten_deps, PublishDepKind.OptionalDependencies)?;
    emitDepSectionJson(out_buf, &pos, "peerDependencies", rewritten_deps, PublishDepKind.PeerDependencies)?;

    // "_id": "<name>@<version>"
    appendBytes(out_buf, &pos, ",\"_id\":")?;
    let pkg_id_buf: [320]u8 = [0; 320];
    let pid_pos: usize = 0;
    appendBytes(pkg_id_buf[0..], &pid_pos, summary.package_name)?;
    appendByte(pkg_id_buf[0..], &pid_pos, @as(u8, 64))?; // '@'
    appendBytes(pkg_id_buf[0..], &pid_pos, summary.version)?;
    appendJsonEscapedString(out_buf, &pos, pkg_id_buf[0..pid_pos])?;

    // "dist": { integrity, shasum, tarball, fileCount, unpackedSize }
    appendBytes(out_buf, &pos, ",\"dist\":{\"integrity\":")?;
    appendJsonEscapedString(out_buf, &pos, dist.integrity)?;
    appendBytes(out_buf, &pos, ",\"shasum\":")?;
    appendJsonEscapedString(out_buf, &pos, dist.shasum)?;
    appendBytes(out_buf, &pos, ",\"tarball\":")?;
    appendJsonEscapedString(out_buf, &pos, dist.tarball_url)?;
    appendBytes(out_buf, &pos, ",\"fileCount\":")?;
    appendUsizeDecimal(out_buf, &pos, @intCast(dist.file_count))?;
    appendBytes(out_buf, &pos, ",\"unpackedSize\":")?;
    appendUsizeDecimal(out_buf, &pos, dist.unpacked_size)?;
    appendBytes(out_buf, &pos, "}}}")?;

    // "access"
    const access_str: []const u8 = if (summary.effective_access == PublishAccessLevel.Restricted) "restricted" else "public";
    appendBytes(out_buf, &pos, ",\"access\":")?;
    appendJsonEscapedString(out_buf, &pos, access_str)?;

    // "_attachments"
    appendBytes(out_buf, &pos, ",\"_attachments\":{")?;
    appendJsonEscapedString(out_buf, &pos, dist.tarball_filename)?;
    appendBytes(out_buf, &pos, ":{\"content_type\":")?;
    appendJsonEscapedString(out_buf, &pos, PUBLISH_CONTENT_TYPE_TGZ)?;
    appendBytes(out_buf, &pos, ",\"data\":\"")?;

    const b64_needed = ((dist.tarball_bytes.len + 2) / 3) * 4;
    if (pos + b64_needed > out_buf.len) {
        return PublishPlanError.BufferTooSmall;
    }
    const b64_written = encodeBase64Standard(dist.tarball_bytes, out_buf[pos..])?;
    pos += b64_written;

    appendBytes(out_buf, &pos, "\",\"length\":")?;
    appendUsizeDecimal(out_buf, &pos, dist.tarball_bytes.len)?;
    appendBytes(out_buf, &pos, "}}}")?;

    return pos;
}

/// Formats a redacted, human-readable dry-run publish summary report.
pub fn formatPublishDryRunReport(
    summary: PublishPlanSummary,
    auth_method: PublishAuthMethod,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "publish ")?;
    appendBytes(out_buf, &pos, summary.package_name)?;
    appendByte(out_buf, &pos, @as(u8, 64))?; // '@'
    appendBytes(out_buf, &pos, summary.version)?;
    appendBytes(out_buf, &pos, " tag=")?;
    appendBytes(out_buf, &pos, summary.effective_tag)?;
    appendBytes(out_buf, &pos, " access=")?;
    const access_str: []const u8 = if (summary.effective_access == PublishAccessLevel.Restricted) "restricted" else "public";
    appendBytes(out_buf, &pos, access_str)?;
    appendBytes(out_buf, &pos, " registry=")?;
    appendBytes(out_buf, &pos, summary.effective_registry)?;
    appendBytes(out_buf, &pos, " auth=")?;
    if (auth_method == PublishAuthMethod.BearerToken) {
        appendBytes(out_buf, &pos, "bearer(redacted)")?;
    } else if (auth_method == PublishAuthMethod.BasicUserPass) {
        appendBytes(out_buf, &pos, "basic(redacted)")?;
    } else if (auth_method == PublishAuthMethod.OidcProvenance) {
        appendBytes(out_buf, &pos, "oidc-provenance")?;
    } else {
        appendBytes(out_buf, &pos, "none")?;
    }
    if (summary.dry_run) {
        appendBytes(out_buf, &pos, " [DRY-RUN]")?;
    }
    return pos;
}
