// Source: Absorbed from /data/dev/bun/src/install/migration.cp | Synced: 2026-11-23T20:24:05Z
// ============================================================================
// lib/pkg/migration.cp — Multi-Ecosystem Lockfile Migrator & Dependency Analyzer
// Absorbed from Bun src/install/{migration.cp,yarn.cp,pnpm.cp}
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonTag } from "cap:json";
import { ConfValue, ConfTag, ConfField } from "../conf/value.cp";
import { Integrity, IntegrityTag } from "../resolver/integrity.cp";
import { exists, readFile } from "cap:sys";
import {
    PackageID,
    INVALID_PACKAGE_ID,
    Behavior,
    PkgError,
} from "./types.cp";
import { Dependency } from "./dependency.cp";
import { Resolution, PackageMeta, Origin, BinLink } from "./resolution.cp";
import { Lockfile } from "./lockfile.cp";

pub err MigrationError {
    InvalidNPMLockfile,
    UnsupportedNPMLockfileVersion,
    InvalidYarnLockfile,
    InvalidPnpmLockfile,
    UnsupportedPnpmLockfileVersion,
    NoLockfileFound,
    OutOfMemory,
}

pub enum ExternalLockfileFormat: u8 {
    None = 0,
    CapLock = 1,
    PackageLockJson = 2,
    YarnLock = 3,
    PnpmLockYaml = 4,

    pub fn filename(): []const u8 {
        return switch (_) {
            .None => "",
            .CapLock => "cap.lock",
            .PackageLockJson => "package-lock.json",
            .YarnLock => "yarn.lock",
            .PnpmLockYaml => "pnpm-lock.yaml",
        };
    }
}

struct NpmPkgEntryMeta {
    pkg_path: []const u8 = "",
    pkg_name: []const u8 = "",
    pkg_id: PackageID = INVALID_PACKAGE_ID,
    json_val: JsonValue = .{},
    is_link: bool = false,
    resolved_target: []const u8 = "",
}

struct YarnSpecBind {
    spec_key: []const u8 = "",
    pkg_id: PackageID = INVALID_PACKAGE_ID,
}

struct YarnParsedEntry {
    specs: std.ArrayListUnmanaged(string) = .empty,
    version: []const u8 = "",
    resolved: []const u8 = "",
    integrity: []const u8 = "",
    deps: std.ArrayListUnmanaged(YarnDepPair) = .empty,

    fn deinit(*_, allocator: std.mem.Allocator) {
        _.specs.deinit(allocator);
        _.deps.deinit(allocator);
    }
}

struct YarnDepPair {
    name: []const u8 = "",
    spec: []const u8 = "",
    behavior: Behavior = Behavior.normal(),
}

struct PnpmPkgMapEntry {
    key: []const u8 = "",
    name: []const u8 = "",
    version: []const u8 = "",
    pkg_id: PackageID = INVALID_PACKAGE_ID,
}

struct KeyValPair {
    key: []const u8 = "",
    val: []const u8 = "",
}

struct NameVersionPair {
    name: []const u8 = "",
    version: []const u8 = "",
}

struct YamlLine {
    indent: usize = 0,
    content: []const u8 = "",
}

/// Migrates npm `package-lock.json` (v2/v3), Yarn v1 `yarn.lock`, and pnpm `pnpm-lock.yaml` (via `cap:conf` YAML 1.2)
/// into native CP `Lockfile` (`cap.lock`) graphs.
pub struct LockfileMigrator {
    pub static fn detectFormat(project_dir: []const u8): ExternalLockfileFormat {
        const clean = std.mem.trimEnd(u8, project_dir, "/");
        let buf: [4096]u8 = undefined;

        const c_lock = std.fmt.bufPrint(&buf, "{s}/cap.lock", .{clean}) catch "";
        if (c_lock.len > 0 && exists(c_lock)) return .CapLock;

        const n_lock = std.fmt.bufPrint(&buf, "{s}/package-lock.json", .{clean}) catch "";
        if (n_lock.len > 0 && exists(n_lock)) return .PackageLockJson;

        const y_lock = std.fmt.bufPrint(&buf, "{s}/yarn.lock", .{clean}) catch "";
        if (y_lock.len > 0 && exists(y_lock)) return .YarnLock;

        const p_lock = std.fmt.bufPrint(&buf, "{s}/pnpm-lock.yaml", .{clean}) catch "";
        if (p_lock.len > 0 && exists(p_lock)) return .PnpmLockYaml;

        return .None;
    }

    pub static fn migrateFromDir(allocator: std.mem.Allocator, project_dir: []const u8): Lockfile! {
        const fmt = LockfileMigrator.detectFormat(project_dir);
        const clean = std.mem.trimEnd(u8, project_dir, "/");

        let root_pj_bytes: ([]u8)? = null;
        defer {
            if (b := root_pj_bytes) allocator.free(b);
        }
        const pj_path = try std.fmt.allocPrint(allocator, "{s}/package.json", .{clean});
        defer allocator.free(pj_path);
        if (exists(pj_path)) {
            root_pj_bytes = readFile(allocator, pj_path) catch null;
        }
        let pj_opt: ([]const u8)? = null;
        if (b := root_pj_bytes) pj_opt = b;

        switch (fmt) {
            .CapLock => {
                const p = try std.fmt.allocPrint(allocator, "{s}/cap.lock", .{clean});
                defer allocator.free(p);
                return Lockfile.loadTextFile(allocator, p);
            },
            .PackageLockJson => {
                const p = try std.fmt.allocPrint(allocator, "{s}/package-lock.json", .{clean});
                defer allocator.free(p);
                const bytes = try readFile(allocator, p);
                defer allocator.free(bytes);
                return LockfileMigrator.fromNpmPackageLockJson(allocator, bytes);
            },
            .YarnLock => {
                const p = try std.fmt.allocPrint(allocator, "{s}/yarn.lock", .{clean});
                defer allocator.free(p);
                const bytes = try readFile(allocator, p);
                defer allocator.free(bytes);
                return LockfileMigrator.fromYarnLock(allocator, bytes, pj_opt);
            },
            .PnpmLockYaml => {
                const p = try std.fmt.allocPrint(allocator, "{s}/pnpm-lock.yaml", .{clean});
                defer allocator.free(p);
                const bytes = try readFile(allocator, p);
                defer allocator.free(bytes);
                return LockfileMigrator.fromPnpmLockYaml(allocator, bytes, pj_opt);
            },
            .None => return error.NoLockfileFound,
        }
    }

    // ------------------------------------------------------------------------
    // 1. npm package-lock.json (lockfileVersion 2 & 3)
    // ------------------------------------------------------------------------

    static fn extractNpmNameFromModulesPath(pkg_path: []const u8): []const u8 {
        if (pkg_path.len == 0) return "";
        if (idx := std.mem.lastIndexOf(u8, pkg_path, "node_modules/")) {
            return pkg_path[idx + 13 ..];
        }
        return pkg_path;
    }

    static fn findNpmScopedPackageId(
        entries: []const NpmPkgEntryMeta,
        parent_path: []const u8,
        dep_name: []const u8,
        allocator: std.mem.Allocator,
    ): PackageID {
        // Walk from innermost `<parent_path>/node_modules/<dep_name>` upward to hoisted `node_modules/<dep_name>`
        let cur = parent_path;
        while (true) {
            const probe = if (cur.len == 0)
                std.fmt.allocPrint(allocator, "node_modules/{s}", .{dep_name}) catch return INVALID_PACKAGE_ID
            else
                std.fmt.allocPrint(allocator, "{s}/node_modules/{s}", .{ cur, dep_name }) catch return INVALID_PACKAGE_ID;
            defer allocator.free(probe);

            for (e of entries) {
                if (std.mem.eql(u8, e.pkg_path, probe) && e.pkg_id != INVALID_PACKAGE_ID) {
                    return e.pkg_id;
                }
            }
            if (cur.len == 0) break;
            if (nm_idx := std.mem.lastIndexOf(u8, cur, "/node_modules/")) {
                cur = cur[0..nm_idx];
            } else {
                cur = "";
            }
        }
        // Fallback by package name
        for (e of entries) {
            if (!e.is_link && std.mem.eql(u8, e.pkg_name, dep_name) && e.pkg_id != INVALID_PACKAGE_ID) {
                return e.pkg_id;
            }
        }
        return INVALID_PACKAGE_ID;
    }

    static fn addNpmDepEdges(
        allocator: std.mem.Allocator,
        lf: *Lockfile,
        from_id: PackageID,
        from_path: []const u8,
        pkg_obj: JsonValue,
        entries: []const NpmPkgEntryMeta,
    ): ! {
        const dep_keys: [4][]const u8 = ["dependencies", "devDependencies", "optionalDependencies", "peerDependencies"];
        const dep_behs: [4]Behavior = [
            Behavior.normal(),
            Behavior.devOnly(),
            Behavior.optionalOnly(),
            Behavior.peerOnly(),
        ];
        let ki: usize = 0;
        while (ki < 4) : (ki += 1) {
            if (dep_obj := pkg_obj.get(dep_keys[ki])) {
                if (dep_obj.tag == .Object) {
                    for (f of dep_obj.obj_val) {
                        if (spec := f.value.asString()) {
                            const tid = LockfileMigrator.findNpmScopedPackageId(entries, from_path, f.key, allocator);
                            try lf.addDependency(allocator, from_id, f.key, spec, dep_behs[ki], tid);
                        }
                    }
                }
            }
        }
    }

    pub static fn fromNpmPackageLockJson(allocator: std.mem.Allocator, json_bytes: []const u8): Lockfile! {
        const root = JSON.parse(allocator, json_bytes) catch return error.InvalidNPMLockfile;
        if (root.tag != .Object) return error.InvalidNPMLockfile;

        const lv_num = root.get("lockfileVersion")?.asNumber() ?? return error.InvalidNPMLockfile;
        if (lv_num < 2.0 || lv_num > 3.0) return error.UnsupportedNPMLockfileVersion;

        const pkgs_obj = root.get("packages") ?? return error.InvalidNPMLockfile;
        if (pkgs_obj.tag != .Object || pkgs_obj.obj_val.len == 0) return error.InvalidNPMLockfile;

        let lf = Lockfile.init();
        errdefer lf.deinit(allocator);

        const root_name_top = (root.get("name")?.asString()) ?? "";
        const root_ver_top = (root.get("version")?.asString()) ?? "";

        let metas: std.ArrayListUnmanaged(NpmPkgEntryMeta) = .empty;
        defer metas.deinit(allocator);

        // Pass 1: create packages (root `""` first, then workspaces / node_modules packages)
        let root_json_val = JsonValue{};
        let has_root = false;
        for (f of pkgs_obj.obj_val) {
            if (f.value.tag != .Object) continue;
            if (f.key.len == 0) {
                has_root = true;
                root_json_val = f.value;
                const r_name = (f.value.get("name")?.asString()) ?? (if (root_name_top.len > 0) root_name_top else "root");
                const r_ver = (f.value.get("version")?.asString()) ?? (if (root_ver_top.len > 0) root_ver_top else "0.0.0");
                const rid = try lf.addPackage(allocator, r_name, r_ver, Resolution.initRoot(), PackageMeta{ origin: .Workspace });
                try metas.append(allocator, NpmPkgEntryMeta{
                    pkg_path: "",
                    pkg_name: r_name,
                    pkg_id: rid,
                    json_val: f.value,
                });
                break;
            }
        }
        if (!has_root) return error.InvalidNPMLockfile;

        for (f of pkgs_obj.obj_val) {
            if (f.key.len == 0 || f.value.tag != .Object) continue;
            const is_link = (f.value.get("link")?.asBool()) ?? false;
            const resolved_s = (f.value.get("resolved")?.asString()) ?? "";
            if (is_link) {
                try metas.append(allocator, NpmPkgEntryMeta{
                    pkg_path: f.key,
                    pkg_name: LockfileMigrator.extractNpmNameFromModulesPath(f.key),
                    pkg_id: INVALID_PACKAGE_ID,
                    json_val: f.value,
                    is_link: true,
                    resolved_target: resolved_s,
                });
                continue;
            }

            const is_ws = !std.mem.startsWith(u8, f.key, "node_modules/") && std.mem.indexOf(u8, f.key, "/node_modules/") == null;
            const p_name = (f.value.get("name")?.asString()) ?? LockfileMigrator.extractNpmNameFromModulesPath(f.key);
            const p_ver = (f.value.get("version")?.asString()) ?? "0.0.0";
            const integ_s = (f.value.get("integrity")?.asString()) ?? "";
            const has_script = (f.value.get("hasInstallScript")?.asBool()) ?? false;

            let res = if (is_ws)
                Resolution.initWorkspace(f.key)
            else if (resolved_s.len > 0 && (std.mem.startsWith(u8, resolved_s, "git+") || std.mem.startsWith(u8, resolved_s, "github:")))
                Resolution.fromTextLockfile(resolved_s) catch Resolution.initNpm(p_ver, resolved_s)
            else
                Resolution.initNpm(p_ver, resolved_s);

            let meta = PackageMeta{
                origin: if (is_ws) Origin.Workspace else Origin.Npm,
                has_install_script: has_script,
                tarball_url: resolved_s,
            };
            if (integ_s.len > 0) {
                meta.integrity = Integrity.parseMultiple(integ_s) catch Integrity{};
            }

            const pid = try lf.addPackage(allocator, p_name, p_ver, res, meta);
            if (is_ws) {
                try lf.addWorkspace(allocator, p_name, f.key, p_ver);
            }
            if (bv := f.value.get("bin")) {
                let blink = try BinLink.fromJson(allocator, bv, null);
                defer blink.deinit(allocator);
                if (!blink.isEmpty()) try lf.setPackageBin(allocator, pid, blink);
            }

            try metas.append(allocator, NpmPkgEntryMeta{
                pkg_path: f.key,
                pkg_name: p_name,
                pkg_id: pid,
                json_val: f.value,
            });
        }

        // Resolve link entries to their target workspace package IDs
        for (*m of metas.items) {
            if (m.is_link && m.resolved_target.len > 0) {
                for (target of metas.items) {
                    if (!target.is_link && std.mem.eql(u8, target.pkg_path, m.resolved_target)) {
                        m.pkg_id = target.pkg_id;
                        break;
                    }
                }
            }
        }

        // Pass 2: wire dependency edges
        for (m of metas.items) {
            if (m.is_link || m.pkg_id == INVALID_PACKAGE_ID) continue;
            try LockfileMigrator.addNpmDepEdges(allocator, &lf, m.pkg_id, m.pkg_path, m.json_val, metas.items);
        }

        lf.computeMetaHash();
        return lf;
    }

    // ------------------------------------------------------------------------
    // 2. Yarn v1 yarn.lock (indented key-value blocks)
    // ------------------------------------------------------------------------

    static fn extractYarnSpecName(spec: []const u8): []const u8 {
        const s = std.mem.trim(u8, spec, " \t\r\n\"'");
        if (s.len == 0) return "";
        const search_from: usize = if (s[0] == '@') 1 else 0;
        if (at_idx := std.mem.indexOfScalarPos(u8, s, search_from, '@')) {
            return s[0..at_idx];
        }
        return s;
    }

    static fn stripQuotes(s: []const u8): []const u8 {
        const t = std.mem.trim(u8, s, " \t\r\n");
        if (t.len >= 2 && ((t[0] == '"' && t[t.len - 1] == '"') || (t[0] == '\'' && t[t.len - 1] == '\''))) {
            return t[1 .. t.len - 1];
        }
        return t;
    }

    static fn tryParseJson(allocator: std.mem.Allocator, bytes: []const u8): JsonValue? {
        const v = JSON.parse(allocator, bytes) catch return null;
        return v;
    }

    static fn splitYarnKeyVal(trimmed: []const u8): KeyValPair {
        let i: usize = 0;
        let in_quote = false;
        while (i < trimmed.len) : (i += 1) {
            const c = trimmed[i];
            if (c == '"') {
                in_quote = !in_quote;
            } else if (!in_quote && (c == ' ' || c == '\t')) {
                const k = LockfileMigrator.stripQuotes(trimmed[0..i]);
                const v = LockfileMigrator.stripQuotes(trimmed[i + 1 ..]);
                return KeyValPair{ key: k, val: v };
            }
        }
        return KeyValPair{ key: LockfileMigrator.stripQuotes(trimmed), val: "" };
    }

    pub static fn fromYarnLock(
        allocator: std.mem.Allocator,
        yarn_text: []const u8,
        root_pkg_json_opt: ([]const u8)? = null,
    ): Lockfile! {
        let entries: std.ArrayListUnmanaged(YarnParsedEntry) = .empty;
        defer {
            for (*e of entries.items) e.deinit(allocator);
            entries.deinit(allocator);
        }

        let current: YarnParsedEntry? = null;
        let cur_section: Behavior? = null;

        let lines = std.mem.splitScalar(u8, yarn_text, '\n');
        while (raw_line := lines.next()) {
            const line = std.mem.trimEnd(u8, raw_line, " \t\r");
            if (line.len == 0) continue;
            const trimmed_lead = std.mem.trimStart(u8, line, " \t");
            if (trimmed_lead.len == 0 || trimmed_lead[0] == '#') continue;

            const indent = line.len - trimmed_lead.len;
            if (indent == 0 && std.mem.endsWith(u8, trimmed_lead, ":")) {
                if (cur := current) {
                    try entries.append(allocator, cur);
                }
                let next_e = YarnParsedEntry{};
                const header_body = trimmed_lead[0 .. trimmed_lead.len - 1];
                let spec_iter = std.mem.splitScalar(u8, header_body, ',');
                while (sp := spec_iter.next()) {
                    const clean_sp = LockfileMigrator.stripQuotes(sp);
                    if (clean_sp.len > 0) {
                        try next_e.specs.append(allocator, clean_sp);
                    }
                }
                current = next_e;
                cur_section = null;
                continue;
            }

            if (current == null) continue;

            if (std.mem.eql(u8, trimmed_lead, "dependencies:")) {
                cur_section = Behavior.normal();
                continue;
            } else if (std.mem.eql(u8, trimmed_lead, "optionalDependencies:")) {
                cur_section = Behavior.optionalOnly();
                continue;
            } else if (std.mem.eql(u8, trimmed_lead, "peerDependencies:")) {
                cur_section = Behavior.peerOnly();
                continue;
            } else if (std.mem.eql(u8, trimmed_lead, "devDependencies:")) {
                cur_section = Behavior.devOnly();
                continue;
            }

            const kv = LockfileMigrator.splitYarnKeyVal(trimmed_lead);
            if (kv.key.len == 0) continue;

            if (indent <= 2) {
                cur_section = null;
                if (std.mem.eql(u8, kv.key, "version")) {
                    current.?.version = kv.val;
                } else if (std.mem.eql(u8, kv.key, "resolved")) {
                    current.?.resolved = kv.val;
                } else if (std.mem.eql(u8, kv.key, "integrity")) {
                    current.?.integrity = kv.val;
                }
            } else if (beh := cur_section) {
                try current.?.deps.append(allocator, YarnDepPair{
                    name: kv.key,
                    spec: kv.val,
                    behavior: beh,
                });
            }
        }
        if (cur := current) {
            try entries.append(allocator, cur);
        }
        if (entries.items.len == 0) return error.InvalidYarnLockfile;

        let lf = Lockfile.init();
        errdefer lf.deinit(allocator);

        let root_name: []const u8 = "root";
        let root_ver: []const u8 = "0.0.0";
        let root_json_opt: JsonValue? = null;
        if (pj_bytes := root_pkg_json_opt) {
            if (pj := LockfileMigrator.tryParseJson(allocator, pj_bytes)) {
                if (pj.tag == .Object) {
                    root_json_opt = pj;
                    if (n := pj.get("name")?.asString()) root_name = n;
                    if (v := pj.get("version")?.asString()) root_ver = v;
                }
            }
        }
        const root_id = try lf.addPackage(allocator, root_name, root_ver, Resolution.initRoot(), PackageMeta{ origin: .Workspace });

        let spec_binds: std.ArrayListUnmanaged(YarnSpecBind) = .empty;
        defer spec_binds.deinit(allocator);

        for (e of entries.items) {
            const pkg_name = if (e.specs.items.len > 0) LockfileMigrator.extractYarnSpecName(e.specs.items[0]) else "";
            if (pkg_name.len == 0) continue;
            const ver = if (e.version.len > 0) e.version else "0.0.0";
            let meta = PackageMeta{
                origin: .Npm,
                tarball_url: e.resolved,
            };
            if (e.integrity.len > 0) {
                meta.integrity = Integrity.parseMultiple(e.integrity) catch Integrity{};
            }
            const pid = try lf.addPackage(allocator, pkg_name, ver, Resolution.initNpm(ver, e.resolved), meta);
            for (sp of e.specs.items) {
                try spec_binds.append(allocator, YarnSpecBind{ spec_key: sp, pkg_id: pid });
            }
        }

        // Wire root edges if package.json provided
        if (rj := root_json_opt) {
            const dep_keys: [4][]const u8 = ["dependencies", "devDependencies", "optionalDependencies", "peerDependencies"];
            const dep_behs: [4]Behavior = [Behavior.normal(), Behavior.devOnly(), Behavior.optionalOnly(), Behavior.peerOnly()];
            let ki: usize = 0;
            while (ki < 4) : (ki += 1) {
                if (dobj := rj.get(dep_keys[ki])) {
                    if (dobj.tag == .Object) {
                        for (f of dobj.obj_val) {
                            if (sp := f.value.asString()) {
                                const lookup_key = try std.fmt.allocPrint(allocator, "{s}@{s}", .{ f.key, sp });
                                defer allocator.free(lookup_key);
                                let tid: PackageID = INVALID_PACKAGE_ID;
                                for (sb of spec_binds.items) {
                                    if (std.mem.eql(u8, sb.spec_key, lookup_key)) {
                                        tid = sb.pkg_id;
                                        break;
                                    }
                                }
                                if (tid == INVALID_PACKAGE_ID) {
                                    if (fid := lf.findPackageByName(f.key)) tid = fid;
                                }
                                try lf.addDependency(allocator, root_id, f.key, sp, dep_behs[ki], tid);
                            }
                        }
                    }
                }
            }
        }

        // Wire package -> package edges
        let ei: usize = 0;
        for (e of entries.items) {
            const from_id: PackageID = @intCast(ei + 1);
            if ((from_id as usize) >= lf.packages.items.len) break;
            for (dep of e.deps.items) {
                const lookup_key = try std.fmt.allocPrint(allocator, "{s}@{s}", .{ dep.name, dep.spec });
                defer allocator.free(lookup_key);
                let tid: PackageID = INVALID_PACKAGE_ID;
                for (sb of spec_binds.items) {
                    if (std.mem.eql(u8, sb.spec_key, lookup_key)) {
                        tid = sb.pkg_id;
                        break;
                    }
                }
                if (tid == INVALID_PACKAGE_ID) {
                    if (fid := lf.findPackageByName(dep.name)) tid = fid;
                }
                try lf.addDependency(allocator, from_id, dep.name, dep.spec, dep.behavior, tid);
            }
            ei += 1;
        }

        lf.computeMetaHash();
        return lf;
    }

    // ------------------------------------------------------------------------
    // 3. pnpm pnpm-lock.yaml (powered by cap:conf YAML 1.2 parser)
    // ------------------------------------------------------------------------

    /// Strips peer-dependency / patch suffixes from pnpm keys: e.g. `/lodash@4.17.21(foo@1.0.0)` -> `lodash@4.17.21`.
    static fn stripPnpmKeySuffix(raw_key: []const u8): []const u8 {
        let k = std.mem.trimStart(u8, raw_key, "/");
        if (paren := std.mem.indexOfScalar(u8, k, '(')) {
            k = k[0..paren];
        }
        return k;
    }

    static fn splitPnpmNameAndVersion(clean_key: []const u8): NameVersionPair {
        const search_from: usize = if (clean_key.len > 0 && clean_key[0] == '@') 1 else 0;
        if (at_idx := std.mem.indexOfScalarPos(u8, clean_key, search_from, '@')) {
            return NameVersionPair{ name: clean_key[0..at_idx], version: clean_key[at_idx + 1 ..] };
        }
        if (slash_idx := std.mem.lastIndexOfScalar(u8, clean_key, '/')) {
            return NameVersionPair{ name: clean_key[0..slash_idx], version: clean_key[slash_idx + 1 ..] };
        }
        return NameVersionPair{ name: clean_key, version: "0.0.0" };
    }

    static fn addPnpmImporterEdges(
        allocator: std.mem.Allocator,
        lf: *Lockfile,
        from_id: PackageID,
        importer_obj: ConfValue,
        pkg_entries: []const PnpmPkgMapEntry,
    ): ! {
        const dep_keys: [3][]const u8 = ["dependencies", "devDependencies", "optionalDependencies"];
        const dep_behs: [3]Behavior = [Behavior.normal(), Behavior.devOnly(), Behavior.optionalOnly()];

        let ki: usize = 0;
        while (ki < 3) : (ki += 1) {
            const group = importer_obj.get(dep_keys[ki]) ?? continue;
            if (group.tag != ConfTag.Object) continue;
            for (f of group.obj_val) {
                let spec_s: []const u8 = "";
                let ver_s: []const u8 = "";
                if (f.value.tag == ConfTag.Object) {
                    if (sv := f.value.get("specifier")) {
                        if (ss := sv.asString()) spec_s = ss;
                    }
                    if (vv := f.value.get("version")) {
                        if (vs := vv.asString()) ver_s = vs;
                    }
                } else if (s := f.value.asString()) {
                    ver_s = s;
                    spec_s = s;
                }
                const clean_ver_raw = LockfileMigrator.stripPnpmKeySuffix(ver_s);
                let tid: PackageID = INVALID_PACKAGE_ID;
                for (pe of pkg_entries) {
                    if (std.mem.eql(u8, pe.name, f.key) && std.mem.eql(u8, pe.version, clean_ver_raw)) {
                        tid = pe.pkg_id;
                        break;
                    }
                }
                if (tid == INVALID_PACKAGE_ID) {
                    if (fid := lf.findPackageByName(f.key)) tid = fid;
                }
                try lf.addDependency(allocator, from_id, f.key, if (spec_s.len > 0) spec_s else clean_ver_raw, dep_behs[ki], tid);
            }
        }
    }

    static fn stripYamlComment(line: []const u8): []const u8 {
        let in_sq = false;
        let in_dq = false;
        let i: usize = 0;
        while (i < line.len) : (i += 1) {
            const c = line[i];
            if (c == '\'' && !in_dq) {
                in_sq = !in_sq;
            } else if (c == '"' && !in_sq) {
                if (i == 0 || line[i - 1] != '\\') in_dq = !in_dq;
            } else if (c == '#' && !in_sq && !in_dq) {
                if (i == 0 || line[i - 1] == ' ' || line[i - 1] == '\t') {
                    return std.mem.trimEnd(u8, line[0..i], " \t");
                }
            }
        }
        return std.mem.trimEnd(u8, line, " \t");
    }

    static fn unquoteYamlScalar(raw: []const u8): []const u8 {
        const t = std.mem.trim(u8, raw, " \t\r");
        if (t.len >= 2 && ((t[0] == '\'' && t[t.len - 1] == '\'') || (t[0] == '"' && t[t.len - 1] == '"'))) {
            return t[1 .. t.len - 1];
        }
        return t;
    }

    static fn parseYamlScalar(allocator: std.mem.Allocator, raw: []const u8): ConfValue! {
        const t = std.mem.trim(u8, raw, " \t\r");
        if (t.len == 0 || std.mem.eql(u8, t, "null") || std.mem.eql(u8, t, "~")) {
            return ConfValue.makeNull();
        }
        if (std.mem.eql(u8, t, "{}")) {
            return ConfValue.makeObject(try allocator.alloc(ConfField, 0));
        }
        if (std.mem.eql(u8, t, "[]")) {
            return ConfValue.makeArray(try allocator.alloc(ConfValue, 0));
        }
        if (t.len >= 2 && t[0] == '[' && t[t.len - 1] == ']') {
            let items: std.ArrayListUnmanaged(ConfValue) = .empty;
            errdefer items.deinit(allocator);
            const inner = std.mem.trim(u8, t[1 .. t.len - 1], " \t");
            if (inner.len > 0) {
                let it = std.mem.splitScalar(u8, inner, ',');
                while (part := it.next()) {
                    try items.append(allocator, try LockfileMigrator.parseYamlScalar(allocator, part));
                }
            }
            return ConfValue.makeArray(try items.toOwnedSlice(allocator));
        }
        if (t.len >= 2 && ((t[0] == '\'' && t[t.len - 1] == '\'') || (t[0] == '"' && t[t.len - 1] == '"'))) {
            return ConfValue.makeString(t[1 .. t.len - 1]);
        }
        if (std.mem.eql(u8, t, "true")) return ConfValue.makeBool(true);
        if (std.mem.eql(u8, t, "false")) return ConfValue.makeBool(false);
        if (iv := std.fmt.parseInt(i64, t, 10) catch null) {
            return ConfValue.makeInt(iv);
        }
        if (fv := std.fmt.parseFloat(f64, t) catch null) {
            return ConfValue.makeFloat(fv);
        }
        return ConfValue.makeString(t);
    }

    static fn findYamlKeyColon(s: []const u8): usize? {
        let in_sq = false;
        let in_dq = false;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '\'' && !in_dq) {
                in_sq = !in_sq;
            } else if (c == '"' && !in_sq) {
                if (i == 0 || s[i - 1] != '\\') in_dq = !in_dq;
            } else if (c == ':' && !in_sq && !in_dq) {
                if (i + 1 == s.len || s[i + 1] == ' ' || s[i + 1] == '\t') {
                    return i;
                }
            }
        }
        return null;
    }

    static fn parseYamlBlock(allocator: std.mem.Allocator, lines: []const YamlLine, idx: *usize, min_indent: usize): ConfValue! {
        if (idx.* >= lines.len || lines[idx.*].indent < min_indent) {
            return ConfValue.makeObject(try allocator.alloc(ConfField, 0));
        }

        const block_indent = lines[idx.*].indent;
        if (std.mem.startsWith(u8, lines[idx.*].content, "- ") || std.mem.eql(u8, lines[idx.*].content, "-")) {
            let arr: std.ArrayListUnmanaged(ConfValue) = .empty;
            errdefer arr.deinit(allocator);
            while (idx.* < lines.len) {
                const ln = lines[idx.*];
                if (ln.indent < block_indent) break;
                if (ln.indent > block_indent) break;
                if (!std.mem.startsWith(u8, ln.content, "- ") && !std.mem.eql(u8, ln.content, "-")) break;
                const rest = if (ln.content.len > 2) std.mem.trim(u8, ln.content[2..], " \t") else "";
                idx.* += 1;
                if (rest.len > 0) {
                    try arr.append(allocator, try LockfileMigrator.parseYamlScalar(allocator, rest));
                } else if (idx.* < lines.len && lines[idx.*].indent > block_indent) {
                    try arr.append(allocator, try LockfileMigrator.parseYamlBlock(allocator, lines, idx, lines[idx.*].indent));
                } else {
                    try arr.append(allocator, ConfValue.makeNull());
                }
            }
            return ConfValue.makeArray(try arr.toOwnedSlice(allocator));
        }

        let fields: std.ArrayListUnmanaged(ConfField) = .empty;
        errdefer fields.deinit(allocator);
        while (idx.* < lines.len) {
            const ln = lines[idx.*];
            if (ln.indent < block_indent) break;
            if (ln.indent > block_indent) {
                idx.* += 1;
                continue;
            }
            const colon_opt = LockfileMigrator.findYamlKeyColon(ln.content);
            if (colon_opt == null) {
                idx.* += 1;
                continue;
            }
            const colon = colon_opt.?;
            const key = LockfileMigrator.unquoteYamlScalar(ln.content[0..colon]);
            const after = if (colon + 1 < ln.content.len) std.mem.trim(u8, ln.content[colon + 1 ..], " \t") else "";
            idx.* += 1;

            let val = ConfValue.makeNull();
            if (after.len > 0) {
                val = try LockfileMigrator.parseYamlScalar(allocator, after);
            } else if (idx.* < lines.len && lines[idx.*].indent > block_indent) {
                val = try LockfileMigrator.parseYamlBlock(allocator, lines, idx, lines[idx.*].indent);
            } else {
                val = ConfValue.makeObject(try allocator.alloc(ConfField, 0));
            }
            try fields.append(allocator, ConfField{ key: key, value: val });
        }
        return ConfValue.makeObject(try fields.toOwnedSlice(allocator));
    }

    /// Parses YAML 1.2 lockfile text into a `cap:conf` `ConfValue` AST.
    pub static fn parseYamlToConfValue(allocator: std.mem.Allocator, yaml_bytes: []const u8): ConfValue! {
        let lines_list: std.ArrayListUnmanaged(YamlLine) = .empty;
        defer lines_list.deinit(allocator);

        let it = std.mem.splitScalar(u8, yaml_bytes, '\n');
        while (raw_line := it.next()) {
            const no_cr = std.mem.trimEnd(u8, raw_line, "\r");
            const no_comment = LockfileMigrator.stripYamlComment(no_cr);
            const trimmed = std.mem.trimStart(u8, no_comment, " ");
            if (trimmed.len == 0) continue;
            if (std.mem.eql(u8, trimmed, "---") || std.mem.eql(u8, trimmed, "...")) continue;
            const indent = no_comment.len - trimmed.len;
            try lines_list.append(allocator, YamlLine{ indent: indent, content: trimmed });
        }

        if (lines_list.items.len == 0) return ConfValue.makeNull();
        let idx: usize = 0;
        return LockfileMigrator.parseYamlBlock(allocator, lines_list.items, &idx, lines_list.items[0].indent);
    }

    pub static fn fromPnpmLockYaml(
        allocator: std.mem.Allocator,
        yaml_bytes: []const u8,
        root_pkg_json_opt: ([]const u8)? = null,
    ): Lockfile! {
        const doc = LockfileMigrator.parseYamlToConfValue(allocator, yaml_bytes) catch return error.InvalidPnpmLockfile;
        if (doc.tag != ConfTag.Object) return error.InvalidPnpmLockfile;

        const lv_val = doc.get("lockfileVersion") ?? return error.InvalidPnpmLockfile;
        let lv_num: f64 = 0.0;
        if (n := lv_val.asNumber()) {
            lv_num = n;
        } else if (s := lv_val.asString()) {
            lv_num = std.fmt.parseFloat(f64, std.mem.trim(u8, s, " '\"")) catch return error.InvalidPnpmLockfile;
        } else {
            return error.InvalidPnpmLockfile;
        }
        if (lv_num < 6.0) return error.UnsupportedPnpmLockfileVersion;

        let lf = Lockfile.init();
        errdefer lf.deinit(allocator);

        if (ov := doc.get("overrides")) {
            if (ov.tag == ConfTag.Object) {
                for (f of ov.obj_val) {
                    if (s := f.value.asString()) try lf.addOverride(allocator, f.key, s);
                }
            }
        }
        if (pd := doc.get("patchedDependencies")) {
            if (pd.tag == ConfTag.Object) {
                for (f of pd.obj_val) {
                    if (f.value.tag == ConfTag.Object) {
                        const pp = (f.value.get("path")?.asString()) ?? "";
                        try lf.addPatchedDependency(allocator, f.key, pp, 0);
                    }
                }
            }
        }

        let root_name: []const u8 = "root";
        let root_ver: []const u8 = "0.0.0";
        if (pj_bytes := root_pkg_json_opt) {
            if (pj := LockfileMigrator.tryParseJson(allocator, pj_bytes)) {
                if (pj.tag == .Object) {
                    if (n := pj.get("name")?.asString()) root_name = n;
                    if (v := pj.get("version")?.asString()) root_ver = v;
                }
            }
        }
        const root_id = try lf.addPackage(allocator, root_name, root_ver, Resolution.initRoot(), PackageMeta{ origin: .Workspace });

        // Register workspace importers != "."
        let importer_ids: std.ArrayListUnmanaged(PackageID) = .empty;
        defer importer_ids.deinit(allocator);
        let importer_vals: std.ArrayListUnmanaged(ConfValue) = .empty;
        defer importer_vals.deinit(allocator);

        let root_importer: ConfValue? = null;
        if (imps := doc.get("importers")) {
            if (imps.tag == ConfTag.Object) {
                for (f of imps.obj_val) {
                    if (f.value.tag != ConfTag.Object) continue;
                    if (std.mem.eql(u8, f.key, ".")) {
                        root_importer = f.value;
                    } else {
                        const w_ver = (f.value.get("version")?.asString()) ?? "0.0.0";
                        try lf.addWorkspace(allocator, f.key, f.key, w_ver);
                        const wid = try lf.addPackage(allocator, f.key, w_ver, Resolution.initWorkspace(f.key), PackageMeta{ origin: .Workspace });
                        try importer_ids.append(allocator, wid);
                        try importer_vals.append(allocator, f.value);
                    }
                }
            }
        }

        // Parse `packages` (tarball/integrity/platform metadata) & optional `snapshots` (pnpm v9)
        let pkg_entries: std.ArrayListUnmanaged(PnpmPkgMapEntry) = .empty;
        defer pkg_entries.deinit(allocator);
        let pkg_objs: std.ArrayListUnmanaged(ConfValue) = .empty;
        defer pkg_objs.deinit(allocator);

        if (pkgs := doc.get("packages")) {
            if (pkgs.tag == ConfTag.Object) {
                for (f of pkgs.obj_val) {
                    if (f.value.tag != ConfTag.Object) continue;
                    const clean_key = LockfileMigrator.stripPnpmKeySuffix(f.key);
                    const nv = LockfileMigrator.splitPnpmNameAndVersion(clean_key);
                    let meta = PackageMeta{ origin: .Npm };
                    let tb_url: []const u8 = "";

                    if (res_obj := f.value.get("resolution")) {
                        if (res_obj.tag == ConfTag.Object) {
                            if (integ_s := res_obj.get("integrity")?.asString()) {
                                meta.integrity = Integrity.parseMultiple(integ_s) catch Integrity{};
                            }
                            if (tb := res_obj.get("tarball")?.asString()) {
                                tb_url = tb;
                                meta.tarball_url = tb;
                            }
                        }
                    }
                    const pid = try lf.addPackage(allocator, nv.name, nv.version, Resolution.initNpm(nv.version, tb_url), meta);
                    try pkg_entries.append(allocator, PnpmPkgMapEntry{
                        key: clean_key,
                        name: nv.name,
                        version: nv.version,
                        pkg_id: pid,
                    });
                    try pkg_objs.append(allocator, f.value);
                }
            }
        }

        // Wire importer edges
        if (ri := root_importer) {
            try LockfileMigrator.addPnpmImporterEdges(allocator, &lf, root_id, ri, pkg_entries.items);
        }
        let ii: usize = 0;
        while (ii < importer_ids.items.len) : (ii += 1) {
            try LockfileMigrator.addPnpmImporterEdges(allocator, &lf, importer_ids.items[ii], importer_vals.items[ii], pkg_entries.items);
        }

        // Wire package edges from `snapshots` (v9) or directly from `packages` (v6/v7/v8)
        let wired_from_snapshots = false;
        if (snaps := doc.get("snapshots")) {
            if (snaps.tag == ConfTag.Object && snaps.obj_val.len > 0) {
                wired_from_snapshots = true;
                for (sf of snaps.obj_val) {
                    if (sf.value.tag != ConfTag.Object) continue;
                    const clean_skey = LockfileMigrator.stripPnpmKeySuffix(sf.key);
                    let from_id: PackageID = INVALID_PACKAGE_ID;
                    for (pe of pkg_entries.items) {
                        if (std.mem.eql(u8, pe.key, clean_skey)) {
                            from_id = pe.pkg_id;
                            break;
                        }
                    }
                    if (from_id == INVALID_PACKAGE_ID) continue;
                    try LockfileMigrator.addPnpmImporterEdges(allocator, &lf, from_id, sf.value, pkg_entries.items);
                }
            }
        }
        if (!wired_from_snapshots) {
            let pi: usize = 0;
            while (pi < pkg_entries.items.len) : (pi += 1) {
                try LockfileMigrator.addPnpmImporterEdges(allocator, &lf, pkg_entries.items[pi].pkg_id, pkg_objs.items[pi], pkg_entries.items);
            }
        }

        lf.computeMetaHash();
        return lf;
    }
}

// ============================================================================
// Dependency Graph Diagnostic Analyzer (`why`, `findDuplicates`, `auditIntegrity`)
// ============================================================================

pub struct WhyStep {
    pkg_id: PackageID = INVALID_PACKAGE_ID,
    name: []const u8 = "",
    version: []const u8 = "",
    spec: []const u8 = "",
    behavior: Behavior = Behavior{},
}

pub struct WhyPath {
    steps: std.ArrayListUnmanaged(WhyStep) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.steps.deinit(allocator);
    }

    pub fn formatAlloc(allocator: std.mem.Allocator): ([]u8)! {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);
        let i: usize = 0;
        while (i < _.steps.items.len) : (i += 1) {
            const st = _.steps.items[i];
            if (i > 0) try out.appendSlice(allocator, " -> ");
            const seg = if (st.spec.len > 0)
                try std.fmt.allocPrint(allocator, "{s}@{s} ({s})", .{ st.name, st.version, st.spec })
            else
                try std.fmt.allocPrint(allocator, "{s}@{s}", .{ st.name, st.version });
            defer allocator.free(seg);
            try out.appendSlice(allocator, seg);
        }
        return out.toOwnedSlice(allocator);
    }
}

pub struct WhyReport {
    target_name: []const u8 = "",
    paths: std.ArrayListUnmanaged(WhyPath) = .empty,
    matched_versions: std.ArrayListUnmanaged(string) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (*p of _.paths.items) p.deinit(allocator);
        _.paths.deinit(allocator);
        _.matched_versions.deinit(allocator);
    }

    pub fn isInstalled(): bool {
        return _.paths.items.len > 0;
    }
}

pub struct DuplicateVersionInfo {
    pkg_id: PackageID = INVALID_PACKAGE_ID,
    version: []const u8 = "",
    dependent_count: usize = 0,
}

pub struct DuplicatePackageGroup {
    name: []const u8 = "",
    versions: std.ArrayListUnmanaged(DuplicateVersionInfo) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.versions.deinit(allocator);
    }
}

pub enum IntegrityIssueKind: u8 {
    Missing = 0,
    WeakSha1 = 1,
}

pub struct IntegrityFinding {
    pkg_id: PackageID = INVALID_PACKAGE_ID,
    name: []const u8 = "",
    version: []const u8 = "",
    issue: IntegrityIssueKind = .Missing,
    algorithm: IntegrityTag = .Unknown,
}

pub struct IntegrityAuditReport {
    total_checked: usize = 0,
    missing_count: usize = 0,
    weak_sha1_count: usize = 0,
    strong_count: usize = 0,
    findings: std.ArrayListUnmanaged(IntegrityFinding) = .empty,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.findings.deinit(allocator);
    }

    pub fn isClean(): bool {
        return _.missing_count == 0 && _.weak_sha1_count == 0;
    }
}

pub struct DependencyAnalyzer {
    static fn dfsWhy(
        allocator: std.mem.Allocator,
        lf: &Lockfile,
        cur_id: PackageID,
        target_name: []const u8,
        stack: *std.ArrayListUnmanaged(WhyStep),
        on_stack: []bool,
        report: *WhyReport,
    ): ! {
        if ((cur_id as usize) >= lf.packages.items.len) return;
        const pkg = lf.packages.items[cur_id as usize];

        if (std.mem.eql(u8, pkg.name, target_name) && stack.items.len > 1) {
            let p = WhyPath{};
            errdefer p.deinit(allocator);
            for (st of stack.items) {
                try p.steps.append(allocator, st);
            }
            try report.paths.append(allocator, p);

            let seen_ver = false;
            for (v of report.matched_versions.items) {
                if (std.mem.eql(u8, v, pkg.version)) {
                    seen_ver = true;
                    break;
                }
            }
            if (!seen_ver) {
                try report.matched_versions.append(allocator, pkg.version);
            }
            return;
        }

        for (edge of pkg.dependencies.items) {
            if (edge.resolved_pkg_id == INVALID_PACKAGE_ID) continue;
            const nid = edge.resolved_pkg_id as usize;
            if (nid >= lf.packages.items.len || on_stack[nid]) continue;

            const next_pkg = lf.packages.items[nid];
            on_stack[nid] = true;
            try stack.append(allocator, WhyStep{
                pkg_id: next_pkg.id,
                name: next_pkg.name,
                version: next_pkg.version,
                spec: edge.spec,
                behavior: edge.behavior,
            });
            try DependencyAnalyzer.dfsWhy(allocator, lf, next_pkg.id, target_name, stack, on_stack, report);
            * = stack.pop();
            on_stack[nid] = false;
        }
    }

    /// Reverse/forward dependency path tracer (`cap why <pkg>`): finds all dependency chains from root to `target_name`.
    pub static fn why(allocator: std.mem.Allocator, lf: &Lockfile, target_name: []const u8): WhyReport! {
        let report = WhyReport{ target_name: target_name };
        errdefer report.deinit(allocator);
        if (lf.packages.items.len == 0) return report;

        let on_stack = try allocator.alloc(bool, lf.packages.items.len);
        defer allocator.free(on_stack);
        @memset(on_stack, false);

        let stack: std.ArrayListUnmanaged(WhyStep) = .empty;
        defer stack.deinit(allocator);

        const root_pkg = lf.packages.items[0];
        on_stack[0] = true;
        try stack.append(allocator, WhyStep{
            pkg_id: root_pkg.id,
            name: root_pkg.name,
            version: root_pkg.version,
        });
        try DependencyAnalyzer.dfsWhy(allocator, lf, root_pkg.id, target_name, &stack, on_stack, &report);
        return report;
    }

    /// Detects duplicate package installations (same name installed with multiple versions/IDs) for `dedupe`.
    pub static fn findDuplicates(allocator: std.mem.Allocator, lf: &Lockfile): (std.ArrayListUnmanaged(DuplicatePackageGroup))! {
        let groups: std.ArrayListUnmanaged(DuplicatePackageGroup) = .empty;
        errdefer {
            for (*g of groups.items) g.deinit(allocator);
            groups.deinit(allocator);
        }

        // Precompute incoming reference counts per PackageID
        let in_deg = try allocator.alloc(usize, lf.packages.items.len);
        defer allocator.free(in_deg);
        @memset(in_deg, 0);
        for (pkg of lf.packages.items) {
            for (edge of pkg.dependencies.items) {
                if (edge.resolved_pkg_id != INVALID_PACKAGE_ID && (edge.resolved_pkg_id as usize) < in_deg.len) {
                    in_deg[edge.resolved_pkg_id as usize] += 1;
                }
            }
        }

        let all_groups: std.ArrayListUnmanaged(DuplicatePackageGroup) = .empty;
        defer {
            for (*g of all_groups.items) g.deinit(allocator);
            all_groups.deinit(allocator);
        }

        for (pkg of lf.packages.items) {
            if (pkg.resolution.tag == .Root) continue;
            let found_idx: usize? = null;
            let gi: usize = 0;
            while (gi < all_groups.items.len) : (gi += 1) {
                if (std.mem.eql(u8, all_groups.items[gi].name, pkg.name)) {
                    found_idx = gi;
                    break;
                }
            }
            const deg = if ((pkg.id as usize) < in_deg.len) in_deg[pkg.id as usize] else 0;
            const info = DuplicateVersionInfo{
                pkg_id: pkg.id,
                version: pkg.version,
                dependent_count: deg,
            };
            if (idx := found_idx) {
                try all_groups.items[idx].versions.append(allocator, info);
            } else {
                let grp = DuplicatePackageGroup{ name: pkg.name };
                try grp.versions.append(allocator, info);
                try all_groups.append(allocator, grp);
            }
        }

        for (*g of all_groups.items) {
            if (g.versions.items.len > 1) {
                let out_grp = DuplicatePackageGroup{ name: g.name };
                for (v of g.versions.items) {
                    try out_grp.versions.append(allocator, v);
                }
                try groups.append(allocator, out_grp);
            }
        }
        return groups;
    }

    /// Scans the lockfile for packages missing W3C SRI integrity hashes or using weak `sha1`.
    pub static fn auditIntegrity(allocator: std.mem.Allocator, lf: &Lockfile): IntegrityAuditReport! {
        let report = IntegrityAuditReport{};
        errdefer report.deinit(allocator);

        for (pkg of lf.packages.items) {
            if (pkg.resolution.tag == .Root || pkg.resolution.tag == .Workspace || pkg.resolution.tag == .Symlink) {
                continue;
            }
            report.total_checked += 1;
            const tag = pkg.meta.integrity.tag;
            if (tag == .Unknown) {
                report.missing_count += 1;
                try report.findings.append(allocator, IntegrityFinding{
                    pkg_id: pkg.id,
                    name: pkg.name,
                    version: pkg.version,
                    issue: .Missing,
                    algorithm: tag,
                });
            } else if (tag == .Sha1) {
                report.weak_sha1_count += 1;
                try report.findings.append(allocator, IntegrityFinding{
                    pkg_id: pkg.id,
                    name: pkg.name,
                    version: pkg.version,
                    issue: .WeakSha1,
                    algorithm: tag,
                });
            } else {
                report.strong_count += 1;
            }
        }
        return report;
    }
}
