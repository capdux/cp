// Source: Absorbed from /data/dev/bun/src/install/lockfile.cp | Synced: 2026-11-22T07:55:13Z
// ============================================================================
// lib/pkg/workspace.cp — Monorepo Workspaces, Catalogs & Member DAG Engine
// Absorbed from Bun src/install/{lockfile.cp,PnpmMatcher.cp}
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonTag } from "cap:json";
import { Matcher, GlobMatchOptions, GlobWalker, GlobWalkerOptions } from "cap:glob";
import { SemverRange } from "cap:semver";
import { DirEntry, DirIterator, DirEntryKind, exists, readFile } from "cap:sys";
import {
    Behavior,
    InstallFeatures,
    PkgError,
} from "./types.cp";
import { Dependency, DependencyList, DependencyVersionTag } from "./dependency.cp";
import { Resolution, PackageMeta } from "./resolution.cp";
import { Lockfile } from "./lockfile.cp";

pub err WorkspaceError {
    InvalidWorkspaceConfig,
    WorkspaceNotFound,
    WorkspaceVersionMismatch,
    CatalogEntryNotFound,
    CyclicWorkspaceDependency,
    OutOfMemory,
}

pub enum WorkspaceManifestKind: u8 {
    PackageJson = 0,
    CapJson = 1,
}

pub struct CatalogEntry {
    catalog_name: []const u8 = "",
    pkg_name: []const u8 = "",
    spec: []const u8 = "",
}

/// Default (`catalog:`) and named (`catalog:<name>`) dependency version catalogs.
pub struct CatalogMap {
    entries: std.ArrayListUnmanaged(CatalogEntry) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(): CatalogMap {
        return CatalogMap{};
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.entries.deinit(allocator);
        for (s of _.owned_strings.items) {
            allocator.free(s);
        }
        _.owned_strings.deinit(allocator);
    }

    fn internStr(*_, allocator: std.mem.Allocator, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try allocator.dupe(u8, s);
        errdefer allocator.free(copy);
        try _.owned_strings.append(allocator, copy);
        return copy;
    }

    pub fn count(): usize {
        return _.entries.items.len;
    }

    pub fn set(*_, allocator: std.mem.Allocator, catalog_name: []const u8, pkg_name: []const u8, spec: []const u8): ! {
        const owned_spec = try _.internStr(allocator, spec);
        for (*e of _.entries.items) {
            if (std.mem.eql(u8, e.catalog_name, catalog_name) && std.mem.eql(u8, e.pkg_name, pkg_name)) {
                e.spec = owned_spec;
                return;
            }
        }
        try _.entries.append(allocator, CatalogEntry{
            catalog_name: try _.internStr(allocator, catalog_name),
            pkg_name: try _.internStr(allocator, pkg_name),
            spec: owned_spec,
        });
    }

    pub fn get(catalog_name: []const u8, pkg_name: []const u8): ([]const u8)? {
        for (e of _.entries.items) {
            if (std.mem.eql(u8, e.catalog_name, catalog_name) && std.mem.eql(u8, e.pkg_name, pkg_name)) {
                return e.spec;
            }
        }
        return null;
    }

    /// Resolves `catalog:` or `catalog:<name>` for `pkg_name`.
    pub fn resolveSpec(pkg_name: []const u8, raw_spec: []const u8): ([]const u8)? {
        const trimmed = std.mem.trim(u8, raw_spec, " \t\r\n");
        if (!std.mem.startsWith(u8, trimmed, "catalog:")) return null;
        const cat_name = std.mem.trim(u8, trimmed[8..], " \t\r\n");
        if (cat_name.len == 0) {
            if (v := _.get("", pkg_name)) return v;
            return _.get("default", pkg_name);
        }
        if (v := _.get(cat_name, pkg_name)) return v;
        if (std.mem.eql(u8, cat_name, "default")) {
            return _.get("", pkg_name);
        }
        return null;
    }

    pub fn parseFromJson(*_, allocator: std.mem.Allocator, root: JsonValue): ! {
        if (root.tag != .Object) return;
        if (cat := root.get("catalog")) {
            if (cat.tag == .Object) {
                for (f of cat.obj_val) {
                    if (s := f.value.asString()) {
                        try _.set(allocator, "", f.key, s);
                    }
                }
            }
        }
        if (cats := root.get("catalogs")) {
            if (cats.tag == .Object) {
                for (cf of cats.obj_val) {
                    if (cf.value.tag == .Object) {
                        for (pf of cf.value.obj_val) {
                            if (s := pf.value.asString()) {
                                try _.set(allocator, cf.key, pf.key, s);
                            }
                        }
                    }
                }
            }
        }
    }
}

pub struct WorkspacePattern {
    raw: []const u8 = "",
    pattern: []const u8 = "",
    is_exclude: bool = false,

    pub static fn parse(raw_pat: []const u8): WorkspacePattern {
        let p = std.mem.trim(u8, raw_pat, " \t\r\n");
        let is_exc = false;
        while (p.len > 0 && p[0] == '!') {
            is_exc = !is_exc;
            p = p[1..];
        }
        while (std.mem.startsWith(u8, p, "./")) {
            p = p[2..];
        }
        p = std.mem.trimEnd(u8, p, "/");
        return WorkspacePattern{
            raw: raw_pat,
            pattern: p,
            is_exclude: is_exc,
        };
    }

    pub fn matchesPath(rel_path: []const u8): bool {
        if (_.pattern.len == 0) return false;
        return Matcher.matchWithOptions(_.pattern, rel_path, GlobMatchOptions{
            case_sensitive: true,
            dot: false,
            normalize: true,
            windows_paths: false,
        });
    }
}

pub struct WorkspaceDepEdge {
    name: []const u8 = "",
    raw_spec: []const u8 = "",
    resolved_spec: []const u8 = "",
    behavior: Behavior = Behavior{},
    resolved_member_idx: usize? = null,
    is_catalog: bool = false,
    is_workspace_protocol: bool = false,
}

pub struct WorkspaceMember {
    index: usize = 0,
    name: []const u8 = "",
    version: []const u8 = "0.0.0",
    rel_path: []const u8 = "",
    abs_path: []const u8 = "",
    manifest_kind: WorkspaceManifestKind = .PackageJson,
    dependencies: DependencyList = DependencyList.init(),
    edges: std.ArrayListUnmanaged(WorkspaceDepEdge) = .empty,
    has_scripts: bool = false,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.dependencies.deinit(allocator);
        _.edges.deinit(allocator);
    }
}

/// Complete Monorepo Workspaces discovery, protocol resolution (`workspace:*`/`^`/`~`, `catalog:`),
/// and DAG topological ordering engine.
pub struct WorkspaceGraph {
    root_dir: []const u8 = ".",
    root_name: []const u8 = "",
    root_version: []const u8 = "0.0.0",
    patterns: std.ArrayListUnmanaged(WorkspacePattern) = .empty,
    catalogs: CatalogMap = CatalogMap.init(),
    members: std.ArrayListUnmanaged(WorkspaceMember) = .empty,
    owned_strings: std.ArrayListUnmanaged([]u8) = .empty,

    pub static fn init(root_dir: []const u8): WorkspaceGraph {
        return WorkspaceGraph{
            root_dir: std.mem.trimEnd(u8, root_dir, "/"),
        };
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.patterns.deinit(allocator);
        _.catalogs.deinit(allocator);
        for (*m of _.members.items) {
            m.deinit(allocator);
        }
        _.members.deinit(allocator);
        for (s of _.owned_strings.items) {
            allocator.free(s);
        }
        _.owned_strings.deinit(allocator);
    }

    fn internStr(*_, allocator: std.mem.Allocator, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        const copy = try allocator.dupe(u8, s);
        errdefer allocator.free(copy);
        try _.owned_strings.append(allocator, copy);
        return copy;
    }

    /// Resolves `workspace:*`, `workspace:^`, `workspace:~`, or `workspace:<range>` against `member_version`.
    pub static fn resolveWorkspaceProtocol(
        allocator: std.mem.Allocator,
        raw_spec: []const u8,
        member_version: []const u8,
    ): ([]u8)! {
        const trimmed = std.mem.trim(u8, raw_spec, " \t\r\n");
        if (!std.mem.startsWith(u8, trimmed, "workspace:")) {
            return allocator.dupe(u8, trimmed);
        }
        const tail = std.mem.trim(u8, trimmed[10..], " \t\r\n");
        const ver = if (member_version.len > 0) member_version else "0.0.0";

        if (tail.len == 0 || std.mem.eql(u8, tail, "*")) {
            return allocator.dupe(u8, ver);
        }
        if (std.mem.eql(u8, tail, "^")) {
            return std.fmt.allocPrint(allocator, "^{s}", .{ver});
        }
        if (std.mem.eql(u8, tail, "~")) {
            return std.fmt.allocPrint(allocator, "~{s}", .{ver});
        }

        // Check if `workspace:<pkg>@<spec>` alias syntax
        let range_part = tail;
        if (tail.len > 0) {
            const search_from: usize = if (tail[0] == '@') 1 else 0;
            if (at_idx := std.mem.indexOfScalarPos(u8, tail, search_from, '@')) {
                const sub = tail[at_idx + 1 ..];
                if (std.mem.eql(u8, sub, "*")) return allocator.dupe(u8, ver);
                if (std.mem.eql(u8, sub, "^")) return std.fmt.allocPrint(allocator, "^{s}", .{ver});
                if (std.mem.eql(u8, sub, "~")) return std.fmt.allocPrint(allocator, "~{s}", .{ver});
                range_part = sub;
            }
        }

        if (SemverRange.isValidRange(range_part)) {
            if (!SemverRange.satisfiesStr(ver, range_part)) {
                return error.WorkspaceVersionMismatch;
            }
        }
        return allocator.dupe(u8, range_part);
    }

    pub fn matchesMemberPath(rel_path: []const u8): bool {
        let matched = false;
        for (pat of _.patterns.items) {
            if (!pat.is_exclude) {
                if (pat.matchesPath(rel_path)) {
                    matched = true;
                }
            }
        }
        if (!matched) return false;
        for (pat of _.patterns.items) {
            if (pat.is_exclude) {
                if (pat.matchesPath(rel_path)) {
                    return false;
                }
            }
        }
        return true;
    }

    fn parseWorkspacesField(*_, allocator: std.mem.Allocator, ws_val: JsonValue): ! {
        if (ws_val.tag == .Array) {
            for (elem of ws_val.arr_val) {
                if (s := elem.asString()) {
                    const owned = try _.internStr(allocator, s);
                    try _.patterns.append(allocator, WorkspacePattern.parse(owned));
                }
            }
            return;
        }
        if (ws_val.tag == .Object) {
            if (pkgs_arr := ws_val.get("packages")) {
                if (pkgs_arr.tag == .Array) {
                    for (elem of pkgs_arr.arr_val) {
                        if (s := elem.asString()) {
                            const owned = try _.internStr(allocator, s);
                            try _.patterns.append(allocator, WorkspacePattern.parse(owned));
                        }
                    }
                }
            }
            try _.catalogs.parseFromJson(allocator, ws_val);
        }
    }

    fn collectCandidateDirs(
        *_,
        allocator: std.mem.Allocator,
        cur_rel: []const u8,
        depth: usize,
        out_dirs: *std.ArrayListUnmanaged([]u8),
    ): ! {
        if (depth > 8) return;
        const abs_dir = if (cur_rel.len == 0)
            try allocator.dupe(u8, _.root_dir)
        else
            try std.fmt.allocPrint(allocator, "{s}/{s}", .{ _.root_dir, cur_rel });
        defer allocator.free(abs_dir);

        let iter = DirIterator.open(abs_dir) catch return;
        defer iter.close();

        while (entry := iter.next() catch null) {
            if (entry.kind != DirEntryKind.directory) continue;
            if (entry.name.len == 0 || entry.name[0] == '.') continue;
            if (std.mem.eql(u8, entry.name, "node_modules")) continue;

            const next_rel = if (cur_rel.len == 0)
                try allocator.dupe(u8, entry.name)
            else
                try std.fmt.allocPrint(allocator, "{s}/{s}", .{ cur_rel, entry.name });

            if (_.matchesMemberPath(next_rel)) {
                try out_dirs.append(allocator, next_rel);
            } else {
                // Continue descending if any include pattern could match deeper paths
                try _.collectCandidateDirs(allocator, next_rel, depth + 1, out_dirs);
                allocator.free(next_rel);
                continue;
            }
            // Also check deeper subdirectories in case patterns contain `**`
            try _.collectCandidateDirs(allocator, next_rel, depth + 1, out_dirs);
        }
    }

    fn scanWithGlobAndFs(*_, allocator: std.mem.Allocator, out_dirs: *std.ArrayListUnmanaged([]u8)): ! {
        // First use cap:glob GlobWalker for shallow/explicit patterns when available,
        // and fallback/complement with recursive directory traversal skipping node_modules/.git.
        for (pat of _.patterns.items) {
            if (pat.is_exclude || pat.pattern.len == 0) continue;
            if (!Matcher.hasMagic(pat.pattern)) {
                const candidate = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ _.root_dir, pat.pattern });
                defer allocator.free(candidate);
                if (exists(candidate) && _.matchesMemberPath(pat.pattern)) {
                    let dup = false;
                    for (existing of out_dirs.items) {
                        if (std.mem.eql(u8, existing, pat.pattern)) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) {
                        try out_dirs.append(allocator, try allocator.dupe(u8, pat.pattern));
                    }
                }
            }
        }

        // Use GlobWalker to scan `**/package.json` and `**/cap.json` or directory patterns
        let gw = GlobWalker.initWithOptions("*", _.root_dir, GlobWalkerOptions{
            only_files: false,
            dot: false,
            max_depth: 1,
        });
        const top_entries = gw.walk(allocator) catch [];
        for (e of top_entries) {
            allocator.free(e);
        }
        if (top_entries.len > 0) allocator.free(top_entries);

        let walked_dirs: std.ArrayListUnmanaged([]u8) = .empty;
        defer {
            for (d of walked_dirs.items) allocator.free(d);
            walked_dirs.deinit(allocator);
        }
        try _.collectCandidateDirs(allocator, "", 0, &walked_dirs);

        for (d of walked_dirs.items) {
            let dup = false;
            for (existing of out_dirs.items) {
                if (std.mem.eql(u8, existing, d)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                try out_dirs.append(allocator, try allocator.dupe(u8, d));
            }
        }

        // Deterministic lexicographical sort of candidate workspace directories
        let items = out_dirs.items;
        if (items.len > 1) {
            let i: usize = 1;
            while (i < items.len) : (i += 1) {
                const key = items[i];
                let j: usize = i;
                while (j > 0 && std.mem.order(u8, items[j - 1], key) == .gt) {
                    items[j] = items[j - 1];
                    j -= 1;
                }
                items[j] = key;
            }
        }
    }

    /// Discovers workspaces from `<root_dir>/package.json` or `<root_dir>/cap.json`,
    /// resolves `catalog:` and `workspace:` protocols, and builds the internal dependency DAG.
    pub static fn discover(allocator: std.mem.Allocator, root_dir: []const u8, features: InstallFeatures = InstallFeatures{}): WorkspaceGraph! {
        let graph = WorkspaceGraph.init(root_dir);
        errdefer graph.deinit(allocator);
        graph.root_dir = try graph.internStr(allocator, std.mem.trimEnd(u8, root_dir, "/"));

        const pj_path = try std.fmt.allocPrint(allocator, "{s}/package.json", .{graph.root_dir});
        defer allocator.free(pj_path);
        const cj_path = try std.fmt.allocPrint(allocator, "{s}/cap.json", .{graph.root_dir});
        defer allocator.free(cj_path);

        let loaded_any = false;
        if (exists(pj_path)) {
            const bytes = try readFile(allocator, pj_path);
            const owned_json = try graph.internStr(allocator, bytes);
            allocator.free(bytes);
            const root_json = JSON.parse(allocator, owned_json) catch return error.InvalidWorkspaceConfig;
            if (root_json.tag == .Object) {
                loaded_any = true;
                if (n := root_json.get("name")?.asString()) graph.root_name = n;
                if (v := root_json.get("version")?.asString()) graph.root_version = v;
                try graph.catalogs.parseFromJson(allocator, root_json);
                if (ws := root_json.get("workspaces")) {
                    try graph.parseWorkspacesField(allocator, ws);
                }
            }
        }
        if (exists(cj_path)) {
            const bytes = try readFile(allocator, cj_path);
            const owned_json = try graph.internStr(allocator, bytes);
            allocator.free(bytes);
            const cap_json = JSON.parse(allocator, owned_json) catch return error.InvalidWorkspaceConfig;
            if (cap_json.tag == .Object) {
                loaded_any = true;
                if (graph.root_name.len == 0) {
                    if (n := cap_json.get("name")?.asString()) graph.root_name = n;
                }
                if (std.mem.eql(u8, graph.root_version, "0.0.0")) {
                    if (v := cap_json.get("version")?.asString()) {
                        graph.root_version = v;
                    } else if (v2 := cap_json.get("ver")?.asString()) {
                        graph.root_version = v2;
                    }
                }
                try graph.catalogs.parseFromJson(allocator, cap_json);
                if (ws := cap_json.get("workspaces")) {
                    try graph.parseWorkspacesField(allocator, ws);
                }
            }
        }
        if (!loaded_any) return error.WorkspaceNotFound;

        let cand_dirs: std.ArrayListUnmanaged([]u8) = .empty;
        defer {
            for (d of cand_dirs.items) allocator.free(d);
            cand_dirs.deinit(allocator);
        }
        try graph.scanWithGlobAndFs(allocator, &cand_dirs);

        // Load each matched workspace member manifest
        for (rel_dir of cand_dirs.items) {
            const mem_abs = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ graph.root_dir, rel_dir });
            const owned_abs = try graph.internStr(allocator, mem_abs);
            allocator.free(mem_abs);
            const owned_rel = try graph.internStr(allocator, rel_dir);

            const m_pj = try std.fmt.allocPrint(allocator, "{s}/package.json", .{owned_abs});
            defer allocator.free(m_pj);
            const m_cj = try std.fmt.allocPrint(allocator, "{s}/cap.json", .{owned_abs});
            defer allocator.free(m_cj);

            let manifest_path: []const u8 = "";
            let manifest_kind = WorkspaceManifestKind.PackageJson;
            if (exists(m_pj)) {
                manifest_path = m_pj;
                manifest_kind = .PackageJson;
            } else if (exists(m_cj)) {
                manifest_path = m_cj;
                manifest_kind = .CapJson;
            } else {
                continue;
            }

            const m_bytes = try readFile(allocator, manifest_path);
            const m_owned = try graph.internStr(allocator, m_bytes);
            allocator.free(m_bytes);
            const m_json = JSON.parse(allocator, m_owned) catch continue;
            if (m_json.tag != .Object) continue;

            let m_name: []const u8 = owned_rel;
            if (nv := m_json.get("name")) {
                if (ns := nv.asString()) m_name = ns;
            }
            let m_ver: []const u8 = "";
            if (vv := m_json.get("version")) {
                if (vs := vv.asString()) m_ver = vs;
            }
            if (m_ver.len == 0) {
                if (vv2 := m_json.get("ver")) {
                    if (vs2 := vv2.asString()) m_ver = vs2;
                }
            }
            if (m_ver.len == 0) m_ver = "0.0.0";

            let member = WorkspaceMember{
                index: graph.members.items.len,
                name: m_name,
                version: m_ver,
                rel_path: owned_rel,
                abs_path: owned_abs,
                manifest_kind: manifest_kind,
            };
            if (sc := m_json.get("scripts")) {
                if (sc.tag == .Object && sc.obj_val.len > 0) {
                    member.has_scripts = true;
                }
            }
            try member.dependencies.addFromPackageJsonRoot(allocator, m_json, features);
            member.dependencies.sort();
            try graph.members.append(allocator, member);
        }

        // Second pass: resolve `catalog:` & `workspace:*`/`^`/`~` and build internal DAG edges
        for (*member of graph.members.items) {
            for (dep of member.dependencies.slice()) {
                const raw_spec = dep.version.literal;
                let resolved_spec = raw_spec;
                let is_cat = false;
                let is_ws = false;
                let target_idx: usize? = null;

                if (dep.version.tag == .Catalog) {
                    is_cat = true;
                    const cat_spec = graph.catalogs.resolveSpec(dep.name, raw_spec) ?? return error.CatalogEntryNotFound;
                    resolved_spec = cat_spec;
                }

                if (dep.version.tag == .Workspace || std.mem.startsWith(u8, resolved_spec, "workspace:")) {
                    is_ws = true;
                    const target_member = graph.findMemberByName(dep.name) ?? return error.WorkspaceNotFound;
                    target_idx = target_member.index;
                    const proto_res = try WorkspaceGraph.resolveWorkspaceProtocol(allocator, resolved_spec, target_member.version);
                    resolved_spec = try graph.internStr(allocator, proto_res);
                    allocator.free(proto_res);
                } else if (target_member := graph.findMemberByName(dep.name)) {
                    // Implicit workspace match if version satisfies local member
                    if (dep.version.tag == .Npm && SemverRange.satisfiesStr(target_member.version, resolved_spec)) {
                        target_idx = target_member.index;
                    }
                }

                try member.edges.append(allocator, WorkspaceDepEdge{
                    name: dep.name,
                    raw_spec: raw_spec,
                    resolved_spec: resolved_spec,
                    behavior: dep.behavior,
                    resolved_member_idx: target_idx,
                    is_catalog: is_cat,
                    is_workspace_protocol: is_ws,
                });
            }
        }

        return graph;
    }

    pub fn memberCount(): usize {
        return _.members.items.len;
    }

    pub fn findMemberByName(name: []const u8): WorkspaceMember? {
        for (m of _.members.items) {
            if (std.mem.eql(u8, m.name, name)) return m;
        }
        return null;
    }

    pub fn findMemberByPath(rel_path: []const u8): WorkspaceMember? {
        for (m of _.members.items) {
            if (std.mem.eql(u8, m.rel_path, rel_path)) return m;
        }
        return null;
    }

    fn dfsCycle(members: []const WorkspaceMember, idx: usize, state: []u8): bool {
        state[idx] = 1;
        for (edge of members[idx].edges.items) {
            if (t := edge.resolved_member_idx) {
                if (t >= members.len) continue;
                if (state[t] == 1) return true;
                if (state[t] == 0 && WorkspaceGraph.dfsCycle(members, t, state)) return true;
            }
        }
        state[idx] = 2;
        return false;
    }

    pub fn hasCycle(allocator: std.mem.Allocator): bool! {
        const n = _.members.items.len;
        if (n == 0) return false;
        const state = try allocator.alloc(u8, n);
        defer allocator.free(state);
        @memset(state, 0);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (state[i] == 0) {
                if (WorkspaceGraph.dfsCycle(_.members.items, i, state)) return true;
            }
        }
        return false;
    }

    fn dfsTopo(
        members: []const WorkspaceMember,
        idx: usize,
        state: []u8,
        out: *std.ArrayListUnmanaged(usize),
        allocator: std.mem.Allocator,
    ): ! {
        state[idx] = 1;
        for (edge of members[idx].edges.items) {
            if (t := edge.resolved_member_idx) {
                if (t >= members.len) continue;
                if (state[t] == 1) return error.CyclicWorkspaceDependency;
                if (state[t] == 0) {
                    try WorkspaceGraph.dfsTopo(members, t, state, out, allocator);
                }
            }
        }
        state[idx] = 2;
        try out.append(allocator, idx);
    }

    /// Post-order topological sort of workspace members (dependencies appear before dependents).
    pub fn topoSortMembers(allocator: std.mem.Allocator): ([]usize)! {
        const n = _.members.items.len;
        let out: std.ArrayListUnmanaged(usize) = .empty;
        errdefer out.deinit(allocator);
        if (n == 0) return out.toOwnedSlice(allocator);

        const state = try allocator.alloc(u8, n);
        defer allocator.free(state);
        @memset(state, 0);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (state[i] == 0) {
                try WorkspaceGraph.dfsTopo(_.members.items, i, state, &out, allocator);
            }
        }
        return out.toOwnedSlice(allocator);
    }

    /// Materializes discovered workspaces and internal edges into a `Lockfile`.
    pub fn applyToLockfile(allocator: std.mem.Allocator, lf: *Lockfile): ! {
        let member_pkg_ids: std.ArrayListUnmanaged(u32) = .empty;
        defer member_pkg_ids.deinit(allocator);

        for (m of _.members.items) {
            try lf.addWorkspace(allocator, m.name, m.rel_path, m.version);
            const pid = try lf.addPackage(
                allocator,
                m.name,
                m.version,
                Resolution.initWorkspace(m.rel_path),
                PackageMeta{ origin: .Workspace, has_install_script: m.has_scripts },
            );
            try member_pkg_ids.append(allocator, pid);
        }

        let mi: usize = 0;
        while (mi < _.members.items.len) : (mi += 1) {
            const m = _.members.items[mi];
            const from_id = member_pkg_ids.items[mi];
            for (edge of m.edges.items) {
                let target_id: u32 = std.math.maxInt(u32);
                if (t_idx := edge.resolved_member_idx) {
                    if (t_idx < member_pkg_ids.items.len) {
                        target_id = member_pkg_ids.items[t_idx];
                    }
                }
                try lf.addDependency(allocator, from_id, edge.name, edge.resolved_spec, edge.behavior, target_id);
            }
        }
    }
}
