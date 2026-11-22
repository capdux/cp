// Source: Absorbed from /data/dev/bun/src/install/dependency.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pkg/dependency.cp — Full Dependency Specifier Parser & DependencyList
// Absorbed from Bun src/install/dependency.cp
// ============================================================================

const std = @import("std");
import { SemverRange } from "cap:semver";
import { HostedGitInfo } from "../resolver/hostedGit.cp";
import { JsonValue } from "cap:json";
import {
    Behavior,
    InstallFeatures,
    PackageNameHash,
    hashPackageName,
    PkgError,
} from "./types.cp";

pub enum DependencyURIKind: u8 {
    Local = 0,
    Remote = 1,
}

pub enum DependencyVersionTag: u8 {
    Uninit = 0,
    Npm = 1,
    DistTag = 2,
    Tarball = 3,
    Folder = 4,
    Symlink = 5,
    Workspace = 6,
    Git = 7,
    GitHub = 8,
    Catalog = 9,

    fn isNPM(): bool {
        return (_ as u8) == 1 || (_ as u8) == 2;
    }

    fn cmp(other: DependencyVersionTag): std.math.Order {
        return std.math.order(_ as u8, other as u8);
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Uninit => "uninitialized",
            .Npm => "npm",
            .DistTag => "dist_tag",
            .Tarball => "tarball",
            .Folder => "folder",
            .Symlink => "symlink",
            .Workspace => "workspace",
            .Git => "git",
            .GitHub => "github",
            .Catalog => "catalog",
        };
    }

    static fn fromStr(s: []const u8): DependencyVersionTag? {
        if (std.mem.eql(u8, s, "npm")) return .Npm;
        if (std.mem.eql(u8, s, "dist_tag") || std.mem.eql(u8, s, "dist-tag")) return .DistTag;
        if (std.mem.eql(u8, s, "tarball")) return .Tarball;
        if (std.mem.eql(u8, s, "folder")) return .Folder;
        if (std.mem.eql(u8, s, "symlink")) return .Symlink;
        if (std.mem.eql(u8, s, "workspace")) return .Workspace;
        if (std.mem.eql(u8, s, "git")) return .Git;
        if (std.mem.eql(u8, s, "github")) return .GitHub;
        if (std.mem.eql(u8, s, "catalog")) return .Catalog;
        return null;
    }

    static fn tryParseHostedGit(spec: []const u8): HostedGitInfo? {
        const info = HostedGitInfo.parse(spec) catch return null;
        return info;
    }

    static fn infer(raw_dependency: []const u8): DependencyVersionTag {
        const dep = std.mem.trim(u8, raw_dependency, " \t\r\n");
        if (dep.len == 0) return .DistTag;

        if (dep.len >= 3 && ((dep[0] >= 'a' && dep[0] <= 'z') || (dep[0] >= 'A' && dep[0] <= 'Z')) && dep[1] == ':' && (dep[2] == '/' || dep[2] == '\\')) {
            if (Dependency.isTarball(dep)) return .Tarball;
            return .Folder;
        }

        const first = dep[0];
        if (first == '=' || first == '>' || first == '<' || first == '^' || first == '*' || first == '|') {
            return .Npm;
        }
        if (first == '.') {
            if (Dependency.isTarball(dep)) return .Tarball;
            return .Folder;
        }
        if (first == '~') {
            if (dep.len > 1 && dep[1] == '/') {
                if (Dependency.isTarball(dep)) return .Tarball;
                return .Folder;
            }
            return .Npm;
        }
        if (first == '/') {
            if (Dependency.isTarball(dep)) return .Tarball;
            return .Folder;
        }
        if (first >= '0' && first <= '9') {
            if (Dependency.isTarball(dep)) return .Tarball;
            return .Npm;
        }
        if (std.mem.startsWith(u8, dep, "file:")) {
            if (Dependency.isTarball(dep)) return .Tarball;
            return .Folder;
        }
        if (std.mem.startsWith(u8, dep, "catalog:")) {
            return .Catalog;
        }
        if (std.mem.startsWith(u8, dep, "workspace:")) {
            return .Workspace;
        }
        if (std.mem.startsWith(u8, dep, "link:")) {
            return .Symlink;
        }
        if (std.mem.startsWith(u8, dep, "patch:")) {
            return .Npm;
        }
        if (std.mem.startsWith(u8, dep, "npm:") && dep.len > 4) {
            const after_prefix = dep[4..];
            const start_idx: usize = if (after_prefix.len > 0 && after_prefix[0] == '@') 1 else 0;
            if (at_idx := std.mem.indexOfScalarPos(u8, after_prefix, start_idx, '@')) {
                return DependencyVersionTag.infer(after_prefix[at_idx + 1 ..]);
            }
            return .Npm;
        }
        if (std.mem.startsWith(u8, dep, "github:")) {
            return .GitHub;
        }
        if (std.mem.startsWith(u8, dep, "git://") ||
            std.mem.startsWith(u8, dep, "git+ssh://") ||
            std.mem.startsWith(u8, dep, "git+file://") ||
            std.mem.startsWith(u8, dep, "git+http://") ||
            std.mem.startsWith(u8, dep, "git+https://") ||
            std.mem.startsWith(u8, dep, "ssh://"))
        {
            if (info := DependencyVersionTag.tryParseHostedGit(dep)) {
                if (info.host == .GitHub) return .GitHub;
            }
            return .Git;
        }
        if (std.mem.startsWith(u8, dep, "https://") || std.mem.startsWith(u8, dep, "http://")) {
            const after_scheme = if (std.mem.startsWith(u8, dep, "https://")) dep[8..] else dep[7..];
            if (std.mem.startsWith(u8, after_scheme, "github.com/")) {
                const gh_path = after_scheme[11..];
                if (Dependency.isGitHubTarballPath(gh_path)) return .Tarball;
                if (info := DependencyVersionTag.tryParseHostedGit(dep)) {
                    * = info;
                    return .GitHub;
                }
            }
            if (Dependency.isTarball(dep)) return .Tarball;
            if (std.mem.endsWith(u8, dep, ".git") || std.mem.indexOf(u8, dep, ".git#") != null) {
                return .Git;
            }
            return .Tarball;
        }
        if (first == 'v' && dep.len > 1 && dep[1] >= '0' && dep[1] <= '9') {
            if (Dependency.isTarball(dep)) return .Tarball;
            return .Npm;
        }
        if ((first == 'x' || first == 'X') && (dep.len == 1 || dep[1] == '.')) {
            return .Npm;
        }
        if (Dependency.isTarball(dep)) return .Tarball;
        if (info := DependencyVersionTag.tryParseHostedGit(dep)) {
            if (info.host == .GitHub && !Dependency.isSCPLikePath(dep) && !std.mem.startsWith(u8, dep, "git@")) {
                return .GitHub;
            }
            return .Git;
        }
        if (Dependency.isSCPLikePath(dep)) return .Git;
        if (std.mem.indexOfScalar(u8, dep, '|') != null) return .Npm;
        if (std.mem.indexOfScalar(u8, dep, ' ') != null) return .Npm;

        return .DistTag;
    }
}

pub struct DependencyVersion {
    tag: DependencyVersionTag = .Uninit,
    literal: []const u8 = "",
    package_name: []const u8 = "",
    is_alias: bool = false,
    npm_range: []const u8 = "",
    dist_tag: []const u8 = "",
    tarball_url: []const u8 = "",
    tarball_kind: DependencyURIKind = .Remote,
    path: []const u8 = "",
    workspace_spec: []const u8 = "",
    catalog_name: []const u8 = "",
    git_url: []const u8 = "",
    git_owner: []const u8 = "",
    git_repo: []const u8 = "",
    git_committish: []const u8 = "",
    git_host: []const u8 = "",

    pub static fn parse(alias_name: []const u8, raw_spec: []const u8): DependencyVersion! {
        const spec = std.mem.trim(u8, raw_spec, " \t\r\n");
        const tag = DependencyVersionTag.infer(spec);
        return DependencyVersion.parseWithTag(alias_name, spec, tag);
    }

    pub static fn parseWithTag(alias_name: []const u8, raw_spec: []const u8, tag: DependencyVersionTag): DependencyVersion! {
        const spec = std.mem.trim(u8, raw_spec, " \t\r\n");
        let res = DependencyVersion{
            tag: tag,
            literal: spec,
            package_name: alias_name,
        };

        switch (tag) {
            .Uninit => return error.InvalidDependency,
            .Npm => {
                let input = spec;
                if (std.mem.startsWith(u8, input, "npm:")) {
                    const rest = input[4..];
                    const search_start: usize = if (rest.len > 0 && rest[0] == '@') 1 else 0;
                    if (at_idx := std.mem.indexOfScalarPos(u8, rest, search_start, '@')) {
                        res.package_name = rest[0..at_idx];
                        res.is_alias = !std.mem.eql(u8, res.package_name, alias_name);
                        input = rest[at_idx + 1 ..];
                    } else {
                        res.package_name = rest;
                        res.is_alias = !std.mem.eql(u8, res.package_name, alias_name);
                        input = "*";
                    }
                }
                if (input.len > 1 && (input[0] == 'v' || input[0] == 'V') && input[1] >= '0' && input[1] <= '9') {
                    input = input[1..];
                }
                if (input.len == 0) input = "*";
                if (!SemverRange.isValidRange(input)) {
                    return error.InvalidVersion;
                }
                res.npm_range = input;
                return res;
            },
            .DistTag => {
                if (std.mem.startsWith(u8, spec, "npm:") && spec.len > 4) {
                    const rest = spec[4..];
                    const search_start: usize = if (rest.len > 0 && rest[0] == '@') 1 else 0;
                    if (at_idx := std.mem.indexOfScalarPos(u8, rest, search_start, '@')) {
                        res.package_name = rest[0..at_idx];
                        res.is_alias = !std.mem.eql(u8, res.package_name, alias_name);
                        const t = rest[at_idx + 1 ..];
                        res.dist_tag = if (t.len > 0) t else "latest";
                    } else {
                        res.package_name = rest;
                        res.is_alias = !std.mem.eql(u8, res.package_name, alias_name);
                        res.dist_tag = "latest";
                    }
                } else {
                    res.dist_tag = if (spec.len > 0) spec else "latest";
                }
                return res;
            },
            .Tarball => {
                if (Dependency.isRemoteTarball(spec)) {
                    res.tarball_url = spec;
                    res.tarball_kind = .Remote;
                } else if (std.mem.startsWith(u8, spec, "file://")) {
                    res.tarball_url = spec[7..];
                    res.tarball_kind = .Local;
                } else if (std.mem.startsWith(u8, spec, "file:")) {
                    res.tarball_url = spec[5..];
                    res.tarball_kind = .Local;
                } else {
                    res.tarball_url = spec;
                    res.tarball_kind = .Local;
                }
                return res;
            },
            .Folder => {
                if (std.mem.startsWith(u8, spec, "file://")) {
                    const after = spec[7..];
                    if (std.mem.startsWith(u8, after, "..")) {
                        res.path = after;
                    } else {
                        res.path = spec[5..];
                    }
                } else if (std.mem.startsWith(u8, spec, "file:")) {
                    res.path = spec[5..];
                } else {
                    res.path = spec;
                }
                return res;
            },
            .Symlink => {
                if (std.mem.startsWith(u8, spec, "link:")) {
                    res.path = spec[5..];
                } else {
                    res.path = spec;
                }
                return res;
            },
            .Workspace => {
                if (std.mem.startsWith(u8, spec, "workspace:")) {
                    res.workspace_spec = spec[10..];
                } else {
                    res.workspace_spec = spec;
                }
                if (res.workspace_spec.len == 0) {
                    res.workspace_spec = "*";
                }
                return res;
            },
            .Catalog => {
                if (std.mem.startsWith(u8, spec, "catalog:")) {
                    res.catalog_name = std.mem.trim(u8, spec[8..], " \t\r\n");
                }
                return res;
            },
            .GitHub => {
                const info = HostedGitInfo.parse(spec) catch return error.InvalidDependency;
                res.git_owner = info.user;
                res.git_repo = info.repo;
                if (c := info.committish) {
                    res.git_committish = c;
                }
                res.git_host = info.host.domain();
                res.git_url = spec;
                return res;
            },
            .Git => {
                let raw = spec;
                if (std.mem.startsWith(u8, raw, "git+")) {
                    raw = raw[4..];
                }
                if (info := DependencyVersionTag.tryParseHostedGit(spec)) {
                    res.git_owner = info.user;
                    res.git_repo = info.repo;
                    if (c := info.committish) {
                        res.git_committish = c;
                    }
                    res.git_host = info.host.domain();
                    res.git_url = raw;
                    return res;
                }
                if (hash_idx := std.mem.lastIndexOfScalar(u8, raw, '#')) {
                    res.git_url = raw[0..hash_idx];
                    res.git_committish = raw[hash_idx + 1 ..];
                } else {
                    res.git_url = raw;
                }
                return res;
            },
        }
    }

    pub fn isAliased(): bool {
        return _.is_alias;
    }

    pub fn effectivePackageName(fallback: []const u8): []const u8 {
        if (_.package_name.len > 0) return _.package_name;
        return fallback;
    }

    pub fn satisfiesVersion(version_str: []const u8): bool {
        if (_.tag != .Npm) return false;
        return SemverRange.satisfiesStr(version_str, _.npm_range);
    }

    pub fn eql(other: DependencyVersion): bool {
        if (_.tag != other.tag) return false;
        return std.mem.eql(u8, _.literal, other.literal);
    }
}

pub struct Dependency {
    name: []const u8 = "",
    name_hash: PackageNameHash = 0,
    version: DependencyVersion = DependencyVersion{},
    behavior: Behavior = Behavior{},

    pub static fn init(name: []const u8, spec: []const u8, behavior: Behavior): Dependency! {
        const parsed = try DependencyVersion.parse(name, spec);
        return Dependency{
            name: name,
            name_hash: hashPackageName(name),
            version: parsed,
            behavior: behavior,
        };
    }

    pub fn realName(): []const u8 {
        return _.version.effectivePackageName(_.name);
    }

    pub fn isAliased(): bool {
        return _.version.is_alias;
    }

    /// Canonical sort order: behavior (workspace -> dev -> optional -> prod -> peer), then name ASC.
    pub static fn isLessThan(lhs: Dependency, rhs: Dependency): bool {
        const b_cmp = lhs.behavior.cmp(rhs.behavior);
        if (b_cmp != .eq) return b_cmp == .lt;
        return std.mem.order(u8, lhs.name, rhs.name) == .lt;
    }

    pub static fn isTarball(spec: []const u8): bool {
        let base = spec;
        if (q_idx := std.mem.indexOfScalar(u8, base, '?')) {
            base = base[0..q_idx];
        }
        return std.mem.endsWith(u8, base, ".tgz") || std.mem.endsWith(u8, base, ".tar.gz");
    }

    pub static fn isRemoteTarball(spec: []const u8): bool {
        return std.mem.startsWith(u8, spec, "https://") || std.mem.startsWith(u8, spec, "http://");
    }

    pub static fn isGitHubTarballPath(gh_path: []const u8): bool {
        if (Dependency.isTarball(gh_path)) return true;
        let parts = std.mem.splitScalar(u8, gh_path, '/');
        let idx: usize = 0;
        while (part := parts.next()) {
            idx += 1;
            if (idx == 3) {
                return std.mem.eql(u8, part, "tarball");
            }
        }
        return false;
    }

    pub static fn isSCPLikePath(spec: []const u8): bool {
        if (spec.len < 3) return false;
        let at_idx: usize? = null;
        let i: usize = 0;
        while (i < spec.len) : (i += 1) {
            const c = spec[i];
            if (c == '@') {
                if (at_idx == null) at_idx = i;
            } else if (c == ':') {
                if (std.mem.startsWith(u8, spec[i..], "://")) return false;
                if (idx := at_idx) return i > idx + 1;
                return i > 0;
            } else if (c == '/') {
                if (idx := at_idx) return i > idx + 1;
                return false;
            }
        }
        return false;
    }

    pub static fn unscopedPackageName(name: []const u8): []const u8 {
        if (name.len == 0 || name[0] != '@') return name;
        if (slash := std.mem.indexOfScalar(u8, name[1..], '/')) {
            return name[1 + slash + 1 ..];
        }
        return name;
    }

    pub static fn isScopedPackageName(name: []const u8): bool! {
        if (name.len == 0) return error.InvalidPackageName;
        if (name[0] != '@') return false;
        if (slash := std.mem.indexOfScalar(u8, name, '/')) {
            if (slash > 1 && slash < name.len - 1) return true;
        }
        return error.InvalidPackageName;
    }

    pub struct SplitNameVersion {
        name: []const u8 = "",
        version: ([]const u8)? = null,
    }

    pub static fn splitNameAndMaybeVersion(input: []const u8): SplitNameVersion {
        if (at_idx := std.mem.indexOfScalar(u8, input, '@')) {
            if (at_idx != 0) {
                const ver = if (at_idx + 1 < input.len) input[at_idx + 1 ..] else null;
                return SplitNameVersion{ name: input[0..at_idx], version: ver };
            }
            if (second := std.mem.indexOfScalar(u8, input[1..], '@')) {
                const real_at = second + 1;
                const ver = if (real_at + 1 < input.len) input[real_at + 1 ..] else null;
                return SplitNameVersion{ name: input[0..real_at], version: ver };
            }
            return SplitNameVersion{ name: input, version: null };
        }
        return SplitNameVersion{ name: input, version: null };
    }
}

/// Ordered dependency collection with deduplication, behavior merging, and JSON extraction.
pub struct DependencyList {
    items: std.ArrayListUnmanaged(Dependency) = .empty,

    pub static fn init(): DependencyList {
        return DependencyList{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.items.deinit(allocator);
    }

    pub fn len(): usize {
        return _.items.items.len;
    }

    pub fn slice(): []const Dependency {
        return _.items.items;
    }

    pub fn findByName(name: []const u8): Dependency? {
        const h = hashPackageName(name);
        for (dep of _.items.items) {
            if (dep.name_hash == h && std.mem.eql(u8, dep.name, name)) {
                return dep;
            }
        }
        return null;
    }

    pub fn add(*_, allocator: std.mem.Allocator, dep: Dependency): ! {
        for (*existing of _.items.items) {
            if (existing.name_hash == dep.name_hash && std.mem.eql(u8, existing.name, dep.name)) {
                existing.behavior = existing.behavior.merge(dep.behavior);
                existing.version = dep.version;
                return;
            }
        }
        try _.items.append(allocator, dep);
    }

    pub fn addSpec(*_, allocator: std.mem.Allocator, name: []const u8, spec: []const u8, behavior: Behavior): ! {
        const dep = try Dependency.init(name, spec, behavior);
        try _.add(allocator, dep);
    }

    pub fn addFromJsonObject(*_, allocator: std.mem.Allocator, obj: JsonValue, behavior: Behavior): ! {
        if (obj.tag != .Object) return;
        for (field of obj.obj_val) {
            if (spec := field.value.asString()) {
                try _.addSpec(allocator, field.key, spec, behavior);
            }
        }
    }

    pub fn addFromPackageJsonRoot(*_, allocator: std.mem.Allocator, root: JsonValue, features: InstallFeatures): ! {
        if (root.tag != .Object) return;
        if (features.prod_dependencies) {
            if (v := root.get("dependencies")) {
                try _.addFromJsonObject(allocator, v, Behavior.normal());
            }
        }
        if (features.dev_dependencies) {
            if (v := root.get("devDependencies")) {
                try _.addFromJsonObject(allocator, v, Behavior.devOnly());
            }
        }
        if (features.optional_dependencies) {
            if (v := root.get("optionalDependencies")) {
                try _.addFromJsonObject(allocator, v, Behavior.optionalOnly());
            }
        }
        if (features.peer_dependencies) {
            if (v := root.get("peerDependencies")) {
                const peer_meta = root.get("peerDependenciesMeta");
                if (v.tag == .Object) {
                    for (field of v.obj_val) {
                        if (spec := field.value.asString()) {
                            let is_opt = false;
                            if (pm := peer_meta) {
                                if (entry := pm.get(field.key)) {
                                    if (opt_v := entry.get("optional")) {
                                        if (b := opt_v.asBool()) {
                                            is_opt = b;
                                        }
                                    }
                                }
                            }
                            const beh = if (is_opt) Behavior.optionalPeer() else Behavior.peerOnly();
                            try _.addSpec(allocator, field.key, spec, beh);
                        }
                    }
                }
            }
        }
    }

    pub fn sort(*_) {
        let arr = _.items.items;
        if (arr.len < 2) return;
        let i: usize = 1;
        while (i < arr.len) : (i += 1) {
            const key = arr[i];
            let j: usize = i;
            while (j > 0 && Dependency.isLessThan(key, arr[j - 1])) {
                arr[j] = arr[j - 1];
                j -= 1;
            }
            arr[j] = key;
        }
    }
}
