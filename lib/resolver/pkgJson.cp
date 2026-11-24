// Source: Absorbed from /data/dev/bun/src/resolver/package_json.cp | Synced: 2026-11-23T20:38:00Z
// ============================================================================
// lib/resolver/package_json.cp — package.json Structured Parser & Exports/Imports Router
// ============================================================================

const std = @import("std");
import { JSON, JsonValue, JsonTag } from "cap:json";

pub enum ModuleType: u8 {
    Unknown = 0,
    CommonJs = 1,
    Module = 2,
}

pub enum ExportResolveStatus: u8 {
    Matched = 0,
    NotFound = 1,
    Disabled = 2,
    NoConditionsMatch = 3,
    InvalidSpecifier = 4,
    InvalidConfig = 5,
}

pub struct ExportResolveResult {
    status: ExportResolveStatus = .NotFound,
    path: []const u8 = "",
}

fn containsCondition(conditions: []const []const u8, key: []const u8): bool {
    for (c of conditions) {
        if (std.mem.eql(u8, c, key)) return true;
    }
    return false;
}

fn isDotSubpathKey(key: []const u8): bool {
    return key.len > 0 && key[0] == '.';
}

fn hasDotKeys(val: JsonValue): bool {
    if (val.tag != .Object) return false;
    for (f of val.obj_val) {
        if (isDotSubpathKey(f.key)) return true;
    }
    return false;
}

fn hasNonDotKeys(val: JsonValue): bool {
    if (val.tag != .Object) return false;
    for (f of val.obj_val) {
        if (!isDotSubpathKey(f.key)) return true;
    }
    return false;
}

fn hasPathTraversal(s: []const u8): bool {
    if (std.mem.indexOf(u8, s, "/../") != null) return true;
    if (std.mem.indexOf(u8, s, "\\..\\") != null) return true;
    if (std.mem.startsWith(u8, s, "../") || std.mem.startsWith(u8, s, "..\\")) return true;
    if (std.mem.endsWith(u8, s, "/..") || std.mem.endsWith(u8, s, "\\..")) return true;
    if (std.mem.eql(u8, s, "..")) return true;
    return false;
}

fn replaceAllStar(allocator: std.mem.Allocator, template: []const u8, replacement: []const u8): ([]const u8)! {
    const star_opt = std.mem.indexOfScalar(u8, template, '*');
    if (star_opt == null) return template;

    // Count occurrences of '*'
    let count: usize = 0;
    for (ch of template) {
        if (ch == '*') count += 1;
    }
    const out_len = template.len - count + (count * replacement.len);
    let out = allocator.alloc(u8, out_len)?;
    let pos: usize = 0;
    for (ch of template) {
        if (ch == '*') {
            @memcpy(out[pos .. pos + replacement.len], replacement);
            pos += replacement.len;
        } else {
            out[pos] = ch;
            pos += 1;
        }
    }
    return out;
}

/// Structured parser for `package.json` with Node/Bun-compliant `exports`, `imports`,
/// `browser`, `type`, `main`, `module`, and dependency resolution.
pub struct PackageJson {
    name: []const u8 = "",
    version: []const u8 = "",
    module_type: ModuleType = .Unknown,
    main: []const u8 = "",
    module: []const u8 = "",
    types: []const u8 = "",
    browser_str: []const u8 = "",
    root: JsonValue = .{},
    exports_val: JsonValue? = null,
    imports_val: JsonValue? = null,
    browser_val: JsonValue? = null,
    dependencies_val: JsonValue? = null,
    dev_dependencies_val: JsonValue? = null,
    peer_dependencies_val: JsonValue? = null,

    /// Parses a `package.json` payload into a structured `PackageJson` handle.
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): PackageJson! {
        const root = JSON.parse(allocator, src)?;
        if (root.tag != .Object) {
            return error.InvalidPackageJson;
        }

        let pkg = PackageJson{
            root: root,
        };

        if (v := root.get("name")) {
            if (s := v.asString()) pkg.name = s;
        }
        if (v := root.get("version")) {
            if (s := v.asString()) pkg.version = s;
        }
        if (v := root.get("type")) {
            if (s := v.asString()) {
                if (std.mem.eql(u8, s, "module")) {
                    pkg.module_type = .Module;
                } else if (std.mem.eql(u8, s, "commonjs")) {
                    pkg.module_type = .CommonJs;
                }
            }
        }
        if (v := root.get("main")) {
            if (s := v.asString()) pkg.main = s;
        }
        if (v := root.get("module")) {
            if (s := v.asString()) pkg.module = s;
        }
        if (v := root.get("types")) {
            if (s := v.asString()) pkg.types = s;
        } else if (v2 := root.get("typings")) {
            if (s2 := v2.asString()) pkg.types = s2;
        }
        if (v := root.get("browser")) {
            pkg.browser_val = v;
            if (s := v.asString()) pkg.browser_str = s;
        }
        if (v := root.get("exports")) {
            pkg.exports_val = v;
        }
        if (v := root.get("imports")) {
            pkg.imports_val = v;
        }
        if (v := root.get("dependencies")) {
            if (v.tag == .Object) pkg.dependencies_val = v;
        }
        if (v := root.get("devDependencies")) {
            if (v.tag == .Object) pkg.dev_dependencies_val = v;
        }
        if (v := root.get("peerDependencies")) {
            if (v.tag == .Object) pkg.peer_dependencies_val = v;
        }

        return pkg;
    }

    pub fn isEsm(): bool {
        return _.module_type == .Module;
    }

    pub fn isCommonJs(): bool {
        return _.module_type == .CommonJs;
    }

    pub fn getDependency(dep_name: []const u8): ([]const u8)? {
        if (deps := _.dependencies_val) {
            if (v := deps.get(dep_name)) {
                return v.asString();
            }
        }
        return null;
    }

    pub fn getDevDependency(dep_name: []const u8): ([]const u8)? {
        if (deps := _.dev_dependencies_val) {
            if (v := deps.get(dep_name)) {
                return v.asString();
            }
        }
        return null;
    }

    pub fn getPeerDependency(dep_name: []const u8): ([]const u8)? {
        if (deps := _.peer_dependencies_val) {
            if (v := deps.get(dep_name)) {
                return v.asString();
            }
        }
        return null;
    }

    pub fn getAnyDependency(dep_name: []const u8): ([]const u8)? {
        if (v := _.getDependency(dep_name)) return v;
        if (v := _.getDevDependency(dep_name)) return v;
        return _.getPeerDependency(dep_name);
    }

    pub fn hasDependency(dep_name: []const u8): bool {
        return _.getAnyDependency(dep_name) != null;
    }

    /// Resolves `"browser"` field mapping (string or object map).
    /// Returns `""` when explicitly disabled with `false`.
    pub fn resolveBrowser(subpath: []const u8): ([]const u8)? {
        const bval = _.browser_val ?? return null;
        if (bval.tag == .String) {
            if (std.mem.eql(u8, subpath, ".") || subpath.len == 0) {
                return bval.str_val;
            }
            return null;
        }
        if (bval.tag == .Object) {
            if (entry := bval.get(subpath)) {
                if (entry.tag == .Bool && !entry.bool_val) return "";
                if (entry.tag == .String) return entry.str_val;
            }
        }
        return null;
    }

    static fn resolveTargetNode(
        allocator: std.mem.Allocator,
        target: JsonValue,
        subpath_match: []const u8,
        is_imports: bool,
        conditions: []const []const u8,
    ): ExportResolveResult! {
        if (target.tag == .Null) {
            return ExportResolveResult{ status: .Disabled };
        }
        if (target.tag == .String) {
            const raw = target.str_val;
            if (hasPathTraversal(subpath_match)) {
                return ExportResolveResult{ status: .InvalidSpecifier };
            }
            if (!is_imports && !std.mem.startsWith(u8, raw, "./")) {
                return ExportResolveResult{ status: .InvalidConfig };
            }
            if (is_imports && !std.mem.startsWith(u8, raw, "./") && std.mem.startsWith(u8, raw, "/")) {
                return ExportResolveResult{ status: .InvalidConfig };
            }
            const expanded = if (subpath_match.len > 0 || std.mem.indexOfScalar(u8, raw, '*') != null)
                replaceAllStar(allocator, raw, subpath_match)?
            else
                raw;

            if (hasPathTraversal(expanded)) {
                return ExportResolveResult{ status: .InvalidSpecifier };
            }
            return ExportResolveResult{ status: .Matched, path: expanded };
        }
        if (target.tag == .Array) {
            let saw_disabled = false;
            for (item of target.arr_val) {
                const res = PackageJson.resolveTargetNode(allocator, item, subpath_match, is_imports, conditions)?;
                if (res.status == .Matched) return res;
                if (res.status == .Disabled) saw_disabled = true;
            }
            if (saw_disabled) return ExportResolveResult{ status: .Disabled };
            return ExportResolveResult{ status: .NoConditionsMatch };
        }
        if (target.tag == .Object) {
            for (field of target.obj_val) {
                if (std.mem.eql(u8, field.key, "default") || containsCondition(conditions, field.key)) {
                    const res = PackageJson.resolveTargetNode(allocator, field.value, subpath_match, is_imports, conditions)?;
                    if (res.status == .Matched || res.status == .Disabled || res.status == .InvalidSpecifier || res.status == .InvalidConfig) {
                        return res;
                    }
                }
            }
            return ExportResolveResult{ status: .NoConditionsMatch };
        }
        return ExportResolveResult{ status: .InvalidConfig };
    }

    static fn resolveSubpathMap(
        allocator: std.mem.Allocator,
        map_val: JsonValue,
        match_key: []const u8,
        is_imports: bool,
        conditions: []const []const u8,
    ): ExportResolveResult! {
        if (map_val.tag != .Object) {
            return ExportResolveResult{ status: .InvalidConfig };
        }

        // 1. Exact key match (when match_key has no '*' and doesn't end with '/')
        if (std.mem.indexOfScalar(u8, match_key, '*') == null && !std.mem.endsWith(u8, match_key, "/")) {
            if (exact_target := map_val.get(match_key)) {
                return PackageJson.resolveTargetNode(allocator, exact_target, "", is_imports, conditions);
            }
        }

        // 2. Pattern / prefix wildcard matching (pick longest specific prefix match)
        let best_score: usize = 0;
        let has_best = false;
        let best_target: JsonValue = .{};
        let best_subpath: []const u8 = "";

        for (field of map_val.obj_val) {
            const key = field.key;
            if (star_idx := std.mem.indexOfScalar(u8, key, '*')) {
                // Ensure only single '*' in key
                if (std.mem.indexOfScalarPos(u8, key, star_idx + 1, '*') != null) {
                    continue;
                }
                const prefix = key[0..star_idx];
                const trailer = key[star_idx + 1 ..];
                if (std.mem.startsWith(u8, match_key, prefix) && match_key.len >= prefix.len + trailer.len) {
                    if (trailer.len == 0 || std.mem.endsWith(u8, match_key, trailer)) {
                        const score = prefix.len * 1024 + trailer.len + 1;
                        if (!has_best || score > best_score) {
                            has_best = true;
                            best_score = score;
                            best_target = field.value;
                            best_subpath = match_key[prefix.len .. match_key.len - trailer.len];
                        }
                    }
                }
            } else if (std.mem.endsWith(u8, key, "/")) {
                if (std.mem.startsWith(u8, match_key, key)) {
                    const score = key.len * 1024;
                    if (!has_best || score > best_score) {
                        has_best = true;
                        best_score = score;
                        best_target = field.value;
                        best_subpath = match_key[key.len..];
                    }
                }
            }
        }

        if (has_best) {
            return PackageJson.resolveTargetNode(allocator, best_target, best_subpath, is_imports, conditions);
        }

        return ExportResolveResult{ status: .NotFound };
    }

    /// Resolves `exports` for `subpath` (e.g. `"."`, `"./foo"`, `"./utils/bar"`) under `conditions`.
    pub fn resolveExport(allocator: std.mem.Allocator, subpath: []const u8, conditions: []const []const u8): ExportResolveResult! {
        const exp = _.exports_val ?? return ExportResolveResult{ status: .NotFound };
        const norm_subpath = if (subpath.len == 0) "." else subpath;

        if (exp.tag == .Null) {
            return ExportResolveResult{ status: .Disabled };
        }

        // Mixed dot and non-dot keys in exports root is invalid per Node spec
        if (exp.tag == .Object && hasDotKeys(exp) && hasNonDotKeys(exp)) {
            return ExportResolveResult{ status: .InvalidConfig };
        }

        if (std.mem.eql(u8, norm_subpath, ".")) {
            if (exp.tag == .String || exp.tag == .Array || (exp.tag == .Object && !hasDotKeys(exp))) {
                return PackageJson.resolveTargetNode(allocator, exp, "", false, conditions);
            }
            if (exp.tag == .Object) {
                if (dot_entry := exp.get(".")) {
                    return PackageJson.resolveTargetNode(allocator, dot_entry, "", false, conditions);
                }
            }
            return ExportResolveResult{ status: .NotFound };
        }

        if (exp.tag == .Object && hasDotKeys(exp)) {
            return PackageJson.resolveSubpathMap(allocator, exp, norm_subpath, false, conditions);
        }

        return ExportResolveResult{ status: .NotFound };
    }

    /// Convenience helper returning `?[]const u8` when `exports` resolves cleanly.
    pub fn resolveExportPath(allocator: std.mem.Allocator, subpath: []const u8, conditions: []const []const u8): ([]const u8)? {
        const res = _.resolveExport(allocator, subpath, conditions) catch return null;
        if (res.status == .Matched) return res.path;
        return null;
    }

    /// Resolves `#internal` private package `imports` under `conditions`.
    pub fn resolveImport(allocator: std.mem.Allocator, specifier: []const u8, conditions: []const []const u8): ExportResolveResult! {
        if (!std.mem.startsWith(u8, specifier, "#")) {
            return ExportResolveResult{ status: .InvalidSpecifier };
        }
        const imp = _.imports_val ?? return ExportResolveResult{ status: .NotFound };
        if (imp.tag != .Object) {
            return ExportResolveResult{ status: .InvalidConfig };
        }
        return PackageJson.resolveSubpathMap(allocator, imp, specifier, true, conditions);
    }

    /// Convenience helper returning `?[]const u8` when `imports` resolves cleanly.
    pub fn resolveImportPath(allocator: std.mem.Allocator, specifier: []const u8, conditions: []const []const u8): ([]const u8)? {
        const res = _.resolveImport(allocator, specifier, conditions) catch return null;
        if (res.status == .Matched) return res.path;
        return null;
    }

    /// Resolves the package entry point (`exports["."]` -> `browser` -> `module` -> `main`).
    pub fn resolveMainEntry(allocator: std.mem.Allocator, conditions: []const []const u8): ([]const u8)? {
        if (_.exports_val != null) {
            const res = _.resolveExport(allocator, ".", conditions) catch return null;
            if (res.status == .Matched) return res.path;
            return null;
        }
        if (containsCondition(conditions, "browser")) {
            if (b := _.resolveBrowser(".")) {
                if (b.len > 0) return b;
            }
        }
        if (containsCondition(conditions, "import") && _.module.len > 0) {
            return _.module;
        }
        if (_.main.len > 0) {
            return _.main;
        }
        return null;
    }
}
