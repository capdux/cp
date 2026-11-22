// Source: Absorbed from /data/dev/bun/src/install/npm.cp | Synced: 2026-11-22T11:53:58Z
// ============================================================================
// lib/pkg/npm.cp — NPM Registry Config (.npmrc), Auth & Packument Manifest Parser
// Absorbed from Bun src/install/npm.cp
// ============================================================================

const std = @import("std");
import { Base64, Hex } from "cap:encoding";
import { JSON, JsonValue } from "cap:json";
import { Version, SemverRange } from "cap:semver";
import { Integrity, IntegrityTag } from "../resolver/integrity.cp";
import { PkgError } from "./types.cp";
import { OperatingSystem, Architecture, Libc, BinLink } from "./resolution.cp";

pub const DEFAULT_NPM_REGISTRY_URL: []const u8 = "https://registry.npmjs.org/";

pub enum AuthKind: u8 {
    None = 0,
    Bearer = 1,
    Basic = 2,
}

/// Scoped NPM Registry endpoint & authentication configuration.
pub struct RegistryScope {
    scope_name: []const u8 = "",
    registry_url: []const u8 = DEFAULT_NPM_REGISTRY_URL,
    auth_kind: AuthKind = .None,
    token: []const u8 = "",
    username: []const u8 = "",
    password: []const u8 = "",
    basic_auth_b64: []const u8 = "",
    always_auth: bool = false,

    pub static fn extractScopeName(pkg_name: []const u8): []const u8 {
        if (pkg_name.len == 0 || pkg_name[0] != '@') return "";
        if (slash := std.mem.indexOfScalar(u8, pkg_name, '/')) {
            return pkg_name[0..slash];
        }
        return pkg_name;
    }

    pub fn hostPath(): []const u8 {
        let u = _.registry_url;
        if (std.mem.startsWith(u8, u, "https://")) {
            u = u[8..];
        } else if (std.mem.startsWith(u8, u, "http://")) {
            u = u[7..];
        }
        return std.mem.trimEnd(u8, u, "/");
    }

    pub fn hasAuth(): bool {
        return _.auth_kind != .None;
    }

    /// Formats HTTP Authorization header (`Bearer <token>` or `Basic <base64>`).
    pub fn authorizationHeaderAlloc(allocator: std.mem.Allocator): ([]u8)?! {
        if (_.auth_kind == .Bearer && _.token.len > 0) {
            return try std.fmt.allocPrint(allocator, "Bearer {s}", .{_.token});
        }
        if (_.auth_kind == .Basic) {
            if (_.basic_auth_b64.len > 0) {
                return try std.fmt.allocPrint(allocator, "Basic {s}", .{_.basic_auth_b64});
            }
            if (_.username.len > 0 || _.password.len > 0) {
                const raw = try std.fmt.allocPrint(allocator, "{s}:{s}", .{ _.username, _.password });
                defer allocator.free(raw);
                const b64_len = Base64.encodeLen(raw.len);
                let out = try allocator.alloc(u8, 6 + b64_len);
                @memcpy(out[0..6], "Basic ");
                const written = Base64.encode(out[6..out.len], raw);
                return out[0 .. 6 + written];
            }
        }
        return null;
    }

    /// Builds registry packument URL (`<registry>/<pkg>` or `<registry>/@scope%2Fname`).
    pub fn packageMetadataUrlAlloc(allocator: std.mem.Allocator, pkg_name: []const u8): []u8! {
        const base = std.mem.trimEnd(u8, _.registry_url, "/");
        if (pkg_name.len > 0 && pkg_name[0] == '@') {
            if (slash := std.mem.indexOfScalar(u8, pkg_name, '/')) {
                const scope_part = pkg_name[0..slash];
                const name_part = pkg_name[slash + 1 ..];
                return std.fmt.allocPrint(allocator, "{s}/{s}%2F{s}", .{ base, scope_part, name_part });
            }
        }
        return std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, pkg_name });
    }

    /// Builds default npm tarball URL (`<registry>/<pkg>/-/<unscoped>-<ver>.tgz`).
    pub fn tarballUrlAlloc(allocator: std.mem.Allocator, pkg_name: []const u8, version: []const u8): []u8! {
        const base = std.mem.trimEnd(u8, _.registry_url, "/");
        let short_name = pkg_name;
        if (short_name.len > 0 && short_name[0] == '@') {
            if (slash := std.mem.indexOfScalar(u8, short_name, '/')) {
                short_name = short_name[slash + 1 ..];
            }
        }
        return std.fmt.allocPrint(allocator, "{s}/{s}/-/{s}-{s}.tgz", .{ base, pkg_name, short_name, version });
    }
}

struct HostAuthEntry {
    host_key: []const u8 = "",
    auth_kind: AuthKind = .None,
    token: []const u8 = "",
    username: []const u8 = "",
    password: []const u8 = "",
    basic_auth_b64: []const u8 = "",
    always_auth: bool = false,
}

/// `.npmrc` registry routing & credential manager.
pub struct RegistryConfig {
    default_registry: RegistryScope = RegistryScope{},
    scopes: std.ArrayListUnmanaged(RegistryScope) = .empty,
    host_auths: std.ArrayListUnmanaged(HostAuthEntry) = .empty,

    pub static fn init(): RegistryConfig {
        return RegistryConfig{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.scopes.deinit(allocator);
        _.host_auths.deinit(allocator);
    }

    fn stripSurroundingQuotes(s: []const u8): []const u8 {
        if (s.len >= 2) {
            if ((s[0] == '"' && s[s.len - 1] == '"') || (s[0] == '\'' && s[s.len - 1] == '\'')) {
                return s[1 .. s.len - 1];
            }
        }
        return s;
    }

    fn normalizeHostKey(raw_key: []const u8): []const u8 {
        let k = raw_key;
        if (std.mem.startsWith(u8, k, "https://")) k = k[8..];
        if (std.mem.startsWith(u8, k, "http://")) k = k[7..];
        if (std.mem.startsWith(u8, k, "//")) k = k[2..];
        return std.mem.trimEnd(u8, k, "/");
    }

    fn getOrAddHostAuthIndex(*_, allocator: std.mem.Allocator, host_key: []const u8): usize! {
        let i: usize = 0;
        while (i < _.host_auths.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.host_auths.items[i].host_key, host_key)) {
                return i;
            }
        }
        try _.host_auths.append(allocator, HostAuthEntry{ host_key: host_key });
        return _.host_auths.items.len - 1;
    }

    fn setScopeRegistry(*_, allocator: std.mem.Allocator, scope_name: []const u8, url: []const u8): ! {
        for (*sc of _.scopes.items) {
            if (std.mem.eql(u8, sc.scope_name, scope_name)) {
                sc.registry_url = url;
                return;
            }
        }
        try _.scopes.append(allocator, RegistryScope{
            scope_name: scope_name,
            registry_url: url,
        });
    }

    fn applyHostAuthToScope(scope: *RegistryScope, host_auths: []const HostAuthEntry) {
        const hp = scope.hostPath();
        for (ha of host_auths) {
            if (std.mem.eql(u8, hp, ha.host_key) || std.mem.startsWith(u8, hp, ha.host_key)) {
                if (ha.auth_kind != .None) scope.auth_kind = ha.auth_kind;
                if (ha.token.len > 0) scope.token = ha.token;
                if (ha.username.len > 0) scope.username = ha.username;
                if (ha.password.len > 0) scope.password = ha.password;
                if (ha.basic_auth_b64.len > 0) scope.basic_auth_b64 = ha.basic_auth_b64;
                if (ha.always_auth) scope.always_auth = true;
            }
        }
    }

    /// Parses `.npmrc` lines (`registry=...`, `@scope:registry=...`, `//host/:_authToken=...`, etc.).
    pub fn parseNpmrc(*_, allocator: std.mem.Allocator, content: []const u8): ! {
        let lines = std.mem.splitScalar(u8, content, '\n');
        while (raw_line := lines.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0 || line[0] == '#' || line[0] == ';') continue;

            const eq_idx = std.mem.indexOfScalar(u8, line, '=') ?? continue;
            const key = std.mem.trim(u8, line[0..eq_idx], " \t");
            const val = RegistryConfig.stripSurroundingQuotes(std.mem.trim(u8, line[eq_idx + 1 ..], " \t"));

            if (std.mem.eql(u8, key, "registry")) {
                _.default_registry.registry_url = val;
                continue;
            }
            if (std.mem.endsWith(u8, key, ":registry") && key.len > 9) {
                const scope_name = key[0 .. key.len - 9];
                try _.setScopeRegistry(allocator, scope_name, val);
                continue;
            }
            if (std.mem.startsWith(u8, key, "//")) {
                if (colon_idx := std.mem.lastIndexOfScalar(u8, key, ':')) {
                    const host_key = RegistryConfig.normalizeHostKey(key[0..colon_idx]);
                    const auth_prop = key[colon_idx + 1 ..];
                    const idx = try _.getOrAddHostAuthIndex(allocator, host_key);

                    if (std.mem.eql(u8, auth_prop, "_authToken")) {
                        _.host_auths.items[idx].auth_kind = .Bearer;
                        _.host_auths.items[idx].token = val;
                    } else if (std.mem.eql(u8, auth_prop, "_auth")) {
                        _.host_auths.items[idx].auth_kind = .Basic;
                        _.host_auths.items[idx].basic_auth_b64 = val;
                    } else if (std.mem.eql(u8, auth_prop, "username")) {
                        _.host_auths.items[idx].username = val;
                        if (_.host_auths.items[idx].auth_kind == .None) _.host_auths.items[idx].auth_kind = .Basic;
                    } else if (std.mem.eql(u8, auth_prop, "_password")) {
                        _.host_auths.items[idx].password = val;
                        if (_.host_auths.items[idx].auth_kind == .None) _.host_auths.items[idx].auth_kind = .Basic;
                    } else if (std.mem.eql(u8, auth_prop, "always-auth")) {
                        _.host_auths.items[idx].always_auth = std.mem.eql(u8, val, "true");
                    }
                }
                continue;
            }
            if (std.mem.eql(u8, key, "_authToken")) {
                _.default_registry.auth_kind = .Bearer;
                _.default_registry.token = val;
            } else if (std.mem.eql(u8, key, "_auth")) {
                _.default_registry.auth_kind = .Basic;
                _.default_registry.basic_auth_b64 = val;
            } else if (std.mem.eql(u8, key, "always-auth")) {
                _.default_registry.always_auth = std.mem.eql(u8, val, "true");
            }
        }

        RegistryConfig.applyHostAuthToScope(&_.default_registry, _.host_auths.items);
        let si: usize = 0;
        while (si < _.scopes.items.len) : (si += 1) {
            RegistryConfig.applyHostAuthToScope(&_.scopes.items[si], _.host_auths.items);
        }
    }

    /// Resolves the matching `RegistryScope` for a package name (e.g. `@acme/foo` -> `@acme` scope).
    pub fn resolveForPackage(pkg_name: []const u8): RegistryScope {
        const scope_name = RegistryScope.extractScopeName(pkg_name);
        if (scope_name.len > 0) {
            for (sc of _.scopes.items) {
                if (std.mem.eql(u8, sc.scope_name, scope_name)) {
                    return sc;
                }
            }
        }
        return _.default_registry;
    }
}

pub struct NpmDepPair {
    name: []const u8 = "",
    spec: []const u8 = "",
    optional: bool = false,
}

pub struct NpmDistTag {
    tag: []const u8 = "",
    version: []const u8 = "",
}

pub struct NpmVersionEntry {
    version_str: []const u8 = "",
    version: Version = Version{},
    tarball_url: []const u8 = "",
    integrity: Integrity = Integrity{},
    shasum: []const u8 = "",
    dependencies: std.ArrayListUnmanaged(NpmDepPair) = .empty,
    optional_dependencies: std.ArrayListUnmanaged(NpmDepPair) = .empty,
    peer_dependencies: std.ArrayListUnmanaged(NpmDepPair) = .empty,
    os_mask: u16 = OperatingSystem.ALL,
    cpu_mask: u16 = Architecture.ALL,
    libc_mask: u8 = Libc.NONE,
    has_install_script: bool = false,
    bin: BinLink = BinLink{},
    unpacked_size: u64 = 0,
    file_count: u32 = 0,
    deprecated: bool = false,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.dependencies.deinit(allocator);
        _.optional_dependencies.deinit(allocator);
        _.peer_dependencies.deinit(allocator);
        _.bin.deinit(allocator);
    }

    pub fn isCompatibleWithCurrentPlatform(): bool {
        return OperatingSystem.isMatch(_.os_mask, OperatingSystem.current()) &&
            Architecture.isMatch(_.cpu_mask, Architecture.current()) &&
            Libc.isMatch(_.libc_mask, Libc.current());
    }
}

fn parseSha1HexIntoIntegrity(hex_str: []const u8): Integrity {
    if (hex_str.len != 40) return Integrity{};
    let out = Integrity{ tag: .Sha1 };
    let decoded: [20]u8 = undefined;
    const written = Hex.decode(hex_str, &decoded) catch return Integrity{};
    if (written != 20) return Integrity{};
    @memcpy(out.digest[0..20], decoded[0..20]);
    return out;
}

/// Parsed NPM Registry Packument (`GET /<package>`) with Semver best-match version selection.
pub struct NpmPackageManifest {
    name: []const u8 = "",
    etag: []const u8 = "",
    modified: []const u8 = "",
    dist_tags: std.ArrayListUnmanaged(NpmDistTag) = .empty,
    versions: std.ArrayListUnmanaged(NpmVersionEntry) = .empty,

    pub static fn init(): NpmPackageManifest {
        return NpmPackageManifest{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.dist_tags.deinit(allocator);
        for (*v of _.versions.items) {
            v.deinit(allocator);
        }
        _.versions.deinit(allocator);
    }

    fn appendDepMap(allocator: std.mem.Allocator, list: *std.ArrayListUnmanaged(NpmDepPair), obj: JsonValue?, default_optional: bool): ! {
        const map_val = obj ?? return;
        if (map_val.tag != .Object) return;
        for (f of map_val.obj_val) {
            if (spec := f.value.asString()) {
                try list.append(allocator, NpmDepPair{
                    name: f.key,
                    spec: spec,
                    optional: default_optional,
                });
            }
        }
    }

    /// Parses an NPM registry packument JSON payload into structured dist-tags & version entries.
    pub static fn parsePackument(allocator: std.mem.Allocator, json_bytes: []const u8): NpmPackageManifest! {
        const root = JSON.parse(allocator, json_bytes) catch return error.InvalidPackument;
        if (root.tag != .Object) return error.InvalidPackument;

        let manifest = NpmPackageManifest.init();
        errdefer manifest.deinit(allocator);

        if (v := root.get("name")) {
            if (s := v.asString()) manifest.name = s;
        }
        if (v := root.get("modified")) {
            if (s := v.asString()) manifest.modified = s;
        }

        if (dt := root.get("dist-tags")) {
            if (dt.tag == .Object) {
                for (f of dt.obj_val) {
                    if (ver_s := f.value.asString()) {
                        try manifest.dist_tags.append(allocator, NpmDistTag{
                            tag: f.key,
                            version: ver_s,
                        });
                    }
                }
            }
        }

        if (vers := root.get("versions")) {
            if (vers.tag == .Object) {
                for (vf of vers.obj_val) {
                    if (vf.value.tag != .Object) continue;
                    const parsed_ver = Version.parse(vf.key) catch continue;

                    let entry = NpmVersionEntry{
                        version_str: vf.key,
                        version: parsed_ver,
                    };
                    errdefer entry.deinit(allocator);

                    if (dep_v := vf.value.get("deprecated")) {
                        if (dep_v.tag == .String && dep_v.str_val.len > 0) {
                            entry.deprecated = true;
                        }
                    }
                    if (os_v := vf.value.get("os")) {
                        entry.os_mask = OperatingSystem.parseJsonArray(os_v);
                    }
                    if (cpu_v := vf.value.get("cpu")) {
                        entry.cpu_mask = Architecture.parseJsonArray(cpu_v);
                    }
                    if (libc_v := vf.value.get("libc")) {
                        entry.libc_mask = Libc.parseJsonArray(libc_v);
                    }
                    if (his := vf.value.get("hasInstallScript")) {
                        if (b := his.asBool()) {
                            entry.has_install_script = b;
                        }
                    }
                    if (!entry.has_install_script) {
                        if (sc := vf.value.get("scripts")) {
                            if (sc.tag == .Object) {
                                if (sc.get("preinstall") != null || sc.get("install") != null || sc.get("postinstall") != null) {
                                    entry.has_install_script = true;
                                }
                            }
                        }
                    }

                    const bin_val = vf.value.get("bin");
                    let dir_bin_val: JsonValue? = null;
                    if (dirs := vf.value.get("directories")) {
                        if (dirs.tag == .Object) {
                            dir_bin_val = dirs.get("bin");
                        }
                    }
                    entry.bin = try BinLink.fromJson(allocator, bin_val, dir_bin_val);

                    if (dist := vf.value.get("dist")) {
                        if (dist.tag == .Object) {
                            if (tb := dist.get("tarball")) {
                                if (s := tb.asString()) entry.tarball_url = s;
                            }
                            if (sz := dist.get("unpackedSize")) {
                                if (n := sz.asNumber()) {
                                    if (n >= 0.0) entry.unpacked_size = @intFromFloat(n);
                                }
                            }
                            if (fc := dist.get("fileCount")) {
                                if (n := fc.asNumber()) {
                                    if (n >= 0.0) entry.file_count = @intFromFloat(n);
                                }
                            }
                            if (sh := dist.get("shasum")) {
                                if (s := sh.asString()) entry.shasum = s;
                            }
                            if (integ := dist.get("integrity")) {
                                if (s := integ.asString()) {
                                    entry.integrity = Integrity.parseMultiple(s) catch Integrity{};
                                }
                            }
                            if (!entry.integrity.tag.isSupported() && entry.shasum.len > 0) {
                                entry.integrity = parseSha1HexIntoIntegrity(entry.shasum);
                            }
                        }
                    }

                    try NpmPackageManifest.appendDepMap(allocator, &entry.dependencies, vf.value.get("dependencies"), false);
                    try NpmPackageManifest.appendDepMap(allocator, &entry.optional_dependencies, vf.value.get("optionalDependencies"), true);
                    try NpmPackageManifest.appendDepMap(allocator, &entry.peer_dependencies, vf.value.get("peerDependencies"), false);

                    if (pmeta := vf.value.get("peerDependenciesMeta")) {
                        if (pmeta.tag == .Object) {
                            for (*pd of entry.peer_dependencies.items) {
                                if (m_entry := pmeta.get(pd.name)) {
                                    if (opt_v := m_entry.get("optional")) {
                                        if (opt_v.asBool() ?? false) {
                                            pd.optional = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    try manifest.versions.append(allocator, entry);
                }
            }
        }

        // Sort ascending by Semver order so highest matching version is at the end
        let items = manifest.versions.items;
        if (items.len > 1) {
            let i: usize = 1;
            while (i < items.len) : (i += 1) {
                const key = items[i];
                let j: usize = i;
                while (j > 0 && Version.order(items[j - 1].version, key.version) == .gt) {
                    items[j] = items[j - 1];
                    j -= 1;
                }
                items[j] = key;
            }
        }

        return manifest;
    }

    pub fn findByVersion(ver_str: []const u8): NpmVersionEntry? {
        const target = Version.parse(ver_str) catch return null;
        for (v of _.versions.items) {
            if (Version.order(v.version, target) == .eq) {
                return v;
            }
        }
        return null;
    }

    pub fn findByDistTag(tag: []const u8): NpmVersionEntry? {
        for (dt of _.dist_tags.items) {
            if (std.mem.eql(u8, dt.tag, tag)) {
                return _.findByVersion(dt.version);
            }
        }
        return null;
    }

    /// Resolves a Semver range or dist-tag (preferring `latest` if it satisfies the range, else highest match).
    pub fn resolveVersion(range_or_tag: []const u8, include_prerelease: bool = false): NpmVersionEntry? {
        const trimmed = std.mem.trim(u8, range_or_tag, " \t\r\n");
        if (trimmed.len == 0) {
            return _.findByDistTag("latest");
        }

        if (SemverRange.isValidRange(trimmed)) {
            if (latest := _.findByDistTag("latest")) {
                if (SemverRange.satisfiesWithOptions(latest.version, trimmed, include_prerelease)) {
                    return latest;
                }
            }
            let i = _.versions.items.len;
            while (i > 0) {
                i -= 1;
                const cand = _.versions.items[i];
                if (SemverRange.satisfiesWithOptions(cand.version, trimmed, include_prerelease)) {
                    return cand;
                }
            }
            return null;
        }

        return _.findByDistTag(trimmed);
    }
}
