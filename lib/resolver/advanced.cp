// Source: Absorbed from /data/dev/bun/src/resolver/tsconfig_json.cp | Synced: 2026-11-21T23:11:30Z
// ============================================================================
// lib/resolver/advanced.cp — TsConfig Inheritance, Browser Map, SideEffects & Self-Ref
// ============================================================================

const std = @import("std");
import { exists, isDirectory, readFileSync } from "cap:fs";
import { PosixPath, normalizePosix, relativePosix, isAbsolutePosix } from "cap:path";
import { Glob } from "cap:glob";
import { JSON, JsonValue, JsonTag } from "cap:json";
import { TsConfigJson, TsPathMapping } from "./tsconfig";
import { PackageJson, ModuleType, ExportResolveStatus } from "./pkgJson";

pub const MAX_EXTENDS_DEPTH: usize = 16;

pub err TsConfigChainError {
    CircularExtends,
    MaxExtendsDepthExceeded,
    FileNotFound,
    InvalidTsConfig,
    OutOfMemory,
}

fn joinPosixOwned(allocator: std.mem.Allocator, a: []const u8, b: []const u8): ([]const u8)! {
    const raw = if (a.len == 0 || std.mem.eql(u8, a, "."))
        std.fmt.allocPrint(allocator, "{s}", .{b})?
    else if (std.mem.endsWith(u8, a, "/"))
        std.fmt.allocPrint(allocator, "{s}{s}", .{ a, b })?
    else
        std.fmt.allocPrint(allocator, "{s}/{s}", .{ a, b })?;
    defer allocator.free(raw);

    let norm_buf: [2048]u8 = undefined;
    const normed = normalizePosix(raw, &norm_buf);
    return allocator.dupe(u8, normed);
}

fn relativePosixOwned(allocator: std.mem.Allocator, from_dir: []const u8, to_path: []const u8): ([]const u8)! {
    let rel_buf: [2048]u8 = undefined;
    const rel = relativePosix(from_dir, to_path, &rel_buf);
    if (rel.len == 0) {
        return allocator.dupe(u8, ".");
    }
    return allocator.dupe(u8, rel);
}

fn stripLeadingDotSlash(p: []const u8): []const u8 {
    let start: usize = 0;
    while (start + 2 <= p.len && p[start] == '.' && p[start + 1] == '/') {
        start += 2;
    }
    return p[start..];
}

fn normalizeForwardSlashesAlloc(allocator: std.mem.Allocator, p: []const u8): ([]u8)! {
    let out = allocator.dupe(u8, p)?;
    let i: usize = 0;
    while (i < out.len) : (i += 1) {
        if (out[i] == '\\') out[i] = '/';
    }
    return out;
}

fn isFileExists(path: []const u8): bool {
    return exists(path) && !isDirectory(path);
}

fn tryReadPackageJsonAt(allocator: std.mem.Allocator, dir: []const u8): PackageJson? {
    const pkg_path = joinPosixOwned(allocator, dir, "package.json") catch return null;
    defer allocator.free(pkg_path);
    if (!isFileExists(pkg_path)) return null;
    const raw = readFileSync(pkg_path);
    if (raw.len == 0) return null;
    const owned = allocator.dupe(u8, raw) catch return null;
    const pkg = PackageJson.parse(allocator, owned) catch return null;
    return pkg;
}

// ============================================================================
// 1. TsConfigChainLoader — Recursive `extends` Inheritance & BaseUrl Relocation
// ============================================================================

pub struct TsConfigChainResult {
    config: TsConfigJson = .{},
    chain_paths: [][]const u8 = [],
    abs_config_dir: []const u8 = "",
    abs_base_url: []const u8 = "",
}

struct LoadedTsConfigLayer {
    path: []const u8 = "",
    dir: []const u8 = "",
    config: TsConfigJson = .{},
    has_explicit_base_url: bool = false,
}

/// Recursively loads a `tsconfig.json` and its `extends` parent chain,
/// merging compilerOptions & `paths` (child overrides parent), relocating `baseUrl`
/// relative to each defining config file, and preventing circular `extends`.
pub struct TsConfigChainLoader {
    static fn resolveExtendsTarget(allocator: std.mem.Allocator, from_dir: []const u8, extends_spec: []const u8): ([]const u8)? {
        if (extends_spec.len == 0) return null;

        // Relative or absolute config path
        if (std.mem.startsWith(u8, extends_spec, "/") ||
            std.mem.startsWith(u8, extends_spec, "./") ||
            std.mem.startsWith(u8, extends_spec, "../") ||
            std.mem.eql(u8, extends_spec, ".") ||
            std.mem.eql(u8, extends_spec, ".."))
        {
            const base = if (isAbsolutePosix(extends_spec))
                allocator.dupe(u8, extends_spec) catch return null
            else
                joinPosixOwned(allocator, from_dir, extends_spec) catch return null;

            if (isFileExists(base)) return base;
            if (!std.mem.endsWith(u8, base, ".json")) {
                const with_json = std.fmt.allocPrint(allocator, "{s}.json", .{base}) catch {
                    allocator.free(base);
                    return null;
                };
                if (isFileExists(with_json)) {
                    allocator.free(base);
                    return with_json;
                }
                allocator.free(with_json);
            }
            if (isDirectory(base)) {
                const idx_cfg = joinPosixOwned(allocator, base, "tsconfig.json") catch {
                    allocator.free(base);
                    return null;
                };
                if (isFileExists(idx_cfg)) {
                    allocator.free(base);
                    return idx_cfg;
                }
                allocator.free(idx_cfg);
            }
            allocator.free(base);
            return null;
        }

        // Package-style extends (e.g. "@org/tsconfig/base.json" or "base-config")
        let cur_dir = from_dir;
        let depth: usize = 0;
        while (depth < 64) : (depth += 1) {
            const dirs: [2][]const u8 = ["cap_modules", "node_modules"];
            let di: usize = 0;
            while (di < 2) : (di += 1) {
                const mod_root = joinPosixOwned(allocator, cur_dir, dirs[di]) catch continue;
                defer allocator.free(mod_root);
                const cand = joinPosixOwned(allocator, mod_root, extends_spec) catch continue;
                if (isFileExists(cand)) return cand;
                if (!std.mem.endsWith(u8, cand, ".json")) {
                    const with_json = std.fmt.allocPrint(allocator, "{s}.json", .{cand}) catch "";
                    if (with_json.len > 0 && isFileExists(with_json)) {
                        allocator.free(cand);
                        return with_json;
                    }
                    if (with_json.len > 0) allocator.free(with_json);
                }
                if (isDirectory(cand)) {
                    const idx_cfg = joinPosixOwned(allocator, cand, "tsconfig.json") catch "";
                    if (idx_cfg.len > 0 && isFileExists(idx_cfg)) {
                        allocator.free(cand);
                        return idx_cfg;
                    }
                    if (idx_cfg.len > 0) allocator.free(idx_cfg);
                }
                allocator.free(cand);
            }
            const parent = PosixPath.dirname(cur_dir);
            if (parent.len == 0 || std.mem.eql(u8, parent, cur_dir)) break;
            cur_dir = parent;
        }
        return null;
    }

    static fn containsVisited(visited: []const []const u8, path: []const u8): bool {
        for (v of visited) {
            if (std.mem.eql(u8, v, path)) return true;
        }
        return false;
    }

    static fn loadLayersRecursive(
        allocator: std.mem.Allocator,
        config_path: []const u8,
        visited: *std.ArrayList(string),
        layers: *std.ArrayList(LoadedTsConfigLayer),
        depth: usize,
    ): ! {
        if (depth >= MAX_EXTENDS_DEPTH) {
            return error.MaxExtendsDepthExceeded;
        }

        let norm_buf: [2048]u8 = undefined;
        const normed = normalizePosix(config_path, &norm_buf);
        const norm_owned = allocator.dupe(u8, normed)?;

        if (TsConfigChainLoader.containsVisited(visited.items, norm_owned)) {
            allocator.free(norm_owned);
            return error.CircularExtends;
        }
        if (!isFileExists(norm_owned)) {
            allocator.free(norm_owned);
            return error.FileNotFound;
        }

        const raw = readFileSync(norm_owned);
        const raw_owned = allocator.dupe(u8, raw)?;
        const parsed = TsConfigJson.parse(allocator, raw_owned) catch {
            return error.InvalidTsConfig;
        };

        visited.append(allocator, norm_owned)?;
        const cfg_dir = allocator.dupe(u8, PosixPath.dirname(norm_owned))?;

        if (parsed.extends.len > 0) {
            const parent_path = TsConfigChainLoader.resolveExtendsTarget(allocator, cfg_dir, parsed.extends) orelse {
                return error.FileNotFound;
            };
            TsConfigChainLoader.loadLayersRecursive(allocator, parent_path, visited, layers, depth + 1)?;
        }

        layers.append(allocator, LoadedTsConfigLayer{
            path: norm_owned,
            dir: cfg_dir,
            config: parsed,
            has_explicit_base_url: parsed.base_url.len > 0,
        })?;
    }

    static fn upsertPathMapping(
        allocator: std.mem.Allocator,
        merged_paths: *std.ArrayList(TsPathMapping),
        pattern: []const u8,
        targets: []const []const u8,
    ): ! {
        let i: usize = 0;
        while (i < merged_paths.items.len) : (i += 1) {
            if (std.mem.eql(u8, merged_paths.items[i].pattern, pattern)) {
                merged_paths.items[i].targets = targets;
                return;
            }
        }
        merged_paths.append(allocator, TsPathMapping{
            pattern: pattern,
            targets: targets,
        })?;
    }

    static fn relocateTargetsToChildDir(
        allocator: std.mem.Allocator,
        layer_dir: []const u8,
        child_dir: []const u8,
        targets: []const []const u8,
    ): ([]const []const u8)! {
        if (std.mem.eql(u8, layer_dir, child_dir)) return targets;
        let out = allocator.alloc(string, targets.len)?;
        let i: usize = 0;
        while (i < targets.len) : (i += 1) {
            const t = targets[i];
            if (isAbsolutePosix(t)) {
                out[i] = t;
            } else {
                const abs_t = joinPosixOwned(allocator, layer_dir, t)?;
                defer allocator.free(abs_t);
                out[i] = relativePosixOwned(allocator, child_dir, abs_t)?;
            }
        }
        return out;
    }

    /// Loads `tsconfig_path` and resolves its full `extends` chain from base -> child.
    pub static fn loadFile(allocator: std.mem.Allocator, tsconfig_path: []const u8): TsConfigChainResult! {
        let visited: std.ArrayList(string) = .empty;
        defer visited.deinit(allocator);

        let layers: std.ArrayList(LoadedTsConfigLayer) = .empty;
        defer layers.deinit(allocator);

        TsConfigChainLoader.loadLayersRecursive(allocator, tsconfig_path, &visited, &layers, 0)?;

        const child_layer = layers.items[layers.items.len - 1];
        const child_dir = child_layer.dir;

        let merged = TsConfigJson{};
        merged.extends = child_layer.config.extends;

        let abs_base_url: []const u8 = "";
        let merged_paths: std.ArrayList(TsPathMapping) = .empty;
        errdefer merged_paths.deinit(allocator);

        // Apply layers in base -> child order so child overrides parent
        for (layer of layers.items) {
            const c = layer.config;
            if (c.root_dir.len > 0) merged.root_dir = c.root_dir;
            if (c.out_dir.len > 0) merged.out_dir = c.out_dir;
            if (c.target.len > 0) merged.target = c.target;
            if (c.jsx.len > 0) merged.jsx = c.jsx;
            if (c.jsx_factory.len > 0) merged.jsx_factory = c.jsx_factory;
            if (c.jsx_fragment_factory.len > 0) merged.jsx_fragment_factory = c.jsx_fragment_factory;
            if (c.jsx_import_source.len > 0) merged.jsx_import_source = c.jsx_import_source;

            if (c.base_url.len > 0) {
                abs_base_url = if (isAbsolutePosix(c.base_url))
                    allocator.dupe(u8, c.base_url)?
                else
                    joinPosixOwned(allocator, layer.dir, c.base_url)?;
                merged.base_url = relativePosixOwned(allocator, child_dir, abs_base_url)?;
            }
        }

        // Merge paths (relocating targets if neither parent nor merged has baseUrl)
        for (layer of layers.items) {
            const c = layer.config;
            for (mapping of c.paths) {
                const eff_targets = if (abs_base_url.len == 0)
                    TsConfigChainLoader.relocateTargetsToChildDir(allocator, layer.dir, child_dir, mapping.targets)?
                else
                    mapping.targets;
                TsConfigChainLoader.upsertPathMapping(allocator, &merged_paths, mapping.pattern, eff_targets)?;
            }
        }
        merged.paths = merged_paths.toOwnedSlice(allocator)?;

        let chain_paths = allocator.alloc(string, layers.items.len)?;
        let li: usize = 0;
        while (li < layers.items.len) : (li += 1) {
            chain_paths[li] = layers.items[li].path;
        }

        return TsConfigChainResult{
            config: merged,
            chain_paths: chain_paths,
            abs_config_dir: child_dir,
            abs_base_url: abs_base_url,
        };
    }
}

// ============================================================================
// 2. BrowserFieldMap — `package.json` `"browser"` Object Remapping & Disabling
// ============================================================================

pub enum BrowserMapAction: u8 {
    Unmapped = 0,
    Remap = 1,
    Disabled = 2,
}

pub struct BrowserLookupResult {
    action: BrowserMapAction = .Unmapped,
    target: []const u8 = "",
}

pub struct BrowserMappingEntry {
    key: []const u8 = "",
    norm_key: []const u8 = "",
    disabled: bool = false,
    target: []const u8 = "",
}

/// Parses and queries the `package.json` `"browser"` field (string or object map),
/// supporting module replacement (`{"./server.js": "./browser.js"}`) and module
/// disabling (`{"fs": false}`).
pub struct BrowserFieldMap {
    package_entry: ([]const u8)? = null,
    entries: []BrowserMappingEntry = [],

    static fn normalizeBrowserKey(allocator: std.mem.Allocator, raw: []const u8): ([]const u8)! {
        let s = raw;
        if (std.mem.startsWith(u8, s, "./")) {
            let norm_buf: [1024]u8 = undefined;
            const n = normalizePosix(s, &norm_buf);
            s = stripLeadingDotSlash(n);
            return std.fmt.allocPrint(allocator, "./{s}", .{s});
        }
        return allocator.dupe(u8, s);
    }

    pub static fn fromPackageJson(allocator: std.mem.Allocator, pkg: PackageJson): BrowserFieldMap! {
        let bmap = BrowserFieldMap{};
        const bval = pkg.browser_val ?? return bmap;

        if (bval.tag == .String) {
            bmap.package_entry = bval.str_val;
            return bmap;
        }

        if (bval.tag == .Object) {
            let list: std.ArrayList(BrowserMappingEntry) = .empty;
            errdefer list.deinit(allocator);

            for (field of bval.obj_val) {
                const nk = BrowserFieldMap.normalizeBrowserKey(allocator, field.key)?;
                if (field.value.tag == .Bool && !field.value.bool_val) {
                    list.append(allocator, BrowserMappingEntry{
                        key: field.key,
                        norm_key: nk,
                        disabled: true,
                        target: "",
                    })?;
                } else if (field.value.tag == .String) {
                    list.append(allocator, BrowserMappingEntry{
                        key: field.key,
                        norm_key: nk,
                        disabled: false,
                        target: field.value.str_val,
                    })?;
                } else {
                    allocator.free(nk);
                }
            }
            bmap.entries = list.toOwnedSlice(allocator)?;
        }

        return bmap;
    }

    pub static fn parse(allocator: std.mem.Allocator, pkg_json_src: []const u8): BrowserFieldMap! {
        const pkg = PackageJson.parse(allocator, pkg_json_src)?;
        return BrowserFieldMap.fromPackageJson(allocator, pkg);
    }

    pub fn count(): usize {
        return _.entries.len;
    }

    fn matchKeyVariants(entry: BrowserMappingEntry, spec: []const u8): bool {
        if (std.mem.eql(u8, entry.key, spec) || std.mem.eql(u8, entry.norm_key, spec)) {
            return true;
        }
        const stripped_entry = stripLeadingDotSlash(entry.norm_key);
        const stripped_spec = stripLeadingDotSlash(spec);
        if (std.mem.eql(u8, stripped_entry, stripped_spec)) {
            // Only match without "./" if both represent relative-style keys or bare names
            if (std.mem.startsWith(u8, entry.key, "./") == std.mem.startsWith(u8, spec, "./")) {
                return true;
            }
            // Also support Browserify convention "./foo.js" <-> "./foo"
        }
        // Extensionless query against ".js"/".cp"/".ts" mapping key
        if (std.mem.startsWith(u8, entry.norm_key, "./") && std.mem.startsWith(u8, spec, "./")) {
            const exts: [4][]const u8 = [".js", ".cp", ".ts", ".mjs"];
            let ei: usize = 0;
            while (ei < exts.len) : (ei += 1) {
                const ext = exts[ei];
                if (std.mem.endsWith(u8, entry.norm_key, ext)) {
                    const stem = entry.norm_key[0 .. entry.norm_key.len - ext.len];
                    if (std.mem.eql(u8, stem, spec)) return true;
                }
            }
        }
        return false;
    }

    pub fn lookup(specifier: []const u8): BrowserLookupResult {
        if (specifier.len == 0) return BrowserLookupResult{ action: .Unmapped };

        if (std.mem.eql(u8, specifier, ".")) {
            if (pe := _.package_entry) {
                return BrowserLookupResult{ action: .Remap, target: pe };
            }
        }

        // Exact pass first
        for (entry of _.entries) {
            if (std.mem.eql(u8, entry.key, specifier)) {
                if (entry.disabled) return BrowserLookupResult{ action: .Disabled, target: "" };
                return BrowserLookupResult{ action: .Remap, target: entry.target };
            }
        }

        // Normalized & extensionless pass
        for (entry of _.entries) {
            if (BrowserFieldMap.matchKeyVariants(entry, specifier)) {
                if (entry.disabled) return BrowserLookupResult{ action: .Disabled, target: "" };
                return BrowserLookupResult{ action: .Remap, target: entry.target };
            }
        }

        return BrowserLookupResult{ action: .Unmapped };
    }

    pub fn isDisabled(specifier: []const u8): bool {
        return _.lookup(specifier).action == .Disabled;
    }

    pub fn remap(specifier: []const u8): ([]const u8)? {
        const res = _.lookup(specifier);
        return switch (res.action) {
            .Unmapped => null,
            .Disabled => "",
            .Remap => res.target,
        };
    }
}

// ============================================================================
// 3. SideEffectsMatcher — `package.json` `"sideEffects"` Tree-Shaking Matcher
// ============================================================================

pub enum SideEffectsMode: u8 {
    Unspecified = 0,
    All = 1,
    None = 2,
    Patterns = 3,
}

/// Evaluates `package.json` `"sideEffects": false | true | [...]` for tree-shaking
/// using exact matching and `cap:glob` (`Glob.match`).
pub struct SideEffectsMatcher {
    mode: SideEffectsMode = .Unspecified,
    package_dir: []const u8 = "",
    exact_paths: [][]const u8 = [],
    glob_patterns: [][]const u8 = [],

    static fn cleanPattern(allocator: std.mem.Allocator, raw: []const u8): ([]const u8)! {
        let s = raw;
        while (std.mem.startsWith(u8, s, "./") || std.mem.startsWith(u8, s, ".\\")) {
            s = s[2..];
        }
        return normalizeForwardSlashesAlloc(allocator, s);
    }

    pub static fn fromPackageJson(
        allocator: std.mem.Allocator,
        pkg: PackageJson,
        package_dir: []const u8,
    ): SideEffectsMatcher! {
        let matcher = SideEffectsMatcher{
            package_dir: package_dir,
        };

        const se_val = pkg.root.get("sideEffects") ?? return matcher;

        if (se_val.tag == .Bool) {
            matcher.mode = if (se_val.bool_val) .All else .None;
            return matcher;
        }

        if (se_val.tag == .Array) {
            if (se_val.arr_val.len == 0) {
                matcher.mode = .None;
                return matcher;
            }
            let exacts: std.ArrayList(string) = .empty;
            let globs: std.ArrayList(string) = .empty;
            errdefer exacts.deinit(allocator);
            errdefer globs.deinit(allocator);

            for (elem of se_val.arr_val) {
                if (pat_raw := elem.asString()) {
                    const pat = SideEffectsMatcher.cleanPattern(allocator, pat_raw)?;
                    if (Glob.hasMagic(pat)) {
                        globs.append(allocator, pat)?;
                    } else {
                        exacts.append(allocator, pat)?;
                    }
                }
            }

            if (exacts.items.len == 0 && globs.items.len == 0) {
                matcher.mode = .None;
                exacts.deinit(allocator);
                globs.deinit(allocator);
                return matcher;
            }

            matcher.mode = .Patterns;
            matcher.exact_paths = exacts.toOwnedSlice(allocator)?;
            matcher.glob_patterns = globs.toOwnedSlice(allocator)?;
            return matcher;
        }

        return matcher;
    }

    pub static fn parse(
        allocator: std.mem.Allocator,
        pkg_json_src: []const u8,
        package_dir: []const u8,
    ): SideEffectsMatcher! {
        const pkg = PackageJson.parse(allocator, pkg_json_src)?;
        return SideEffectsMatcher.fromPackageJson(allocator, pkg, package_dir);
    }

    fn relativizePath(file_path: []const u8, buf: []u8): []const u8 {
        let len: usize = 0;
        for (ch of file_path) {
            if (len >= buf.len) break;
            buf[len] = if (ch == '\\') '/' else ch;
            len += 1;
        }
        let start: usize = 0;
        if (_.package_dir.len > 0) {
            let pkg_buf: [2048]u8 = undefined;
            let plen: usize = 0;
            for (ch of _.package_dir) {
                if (plen >= pkg_buf.len) break;
                pkg_buf[plen] = if (ch == '\\') '/' else ch;
                plen += 1;
            }
            let pkg_len = plen;
            while (pkg_len > 1 && pkg_buf[pkg_len - 1] == '/') {
                pkg_len -= 1;
            }
            if (std.mem.startsWith(u8, buf[0..len], pkg_buf[0..pkg_len])) {
                if (len > pkg_len && buf[pkg_len] == '/') {
                    start = pkg_len + 1;
                }
            }
        }
        return stripLeadingDotSlash(buf[start..len]);
    }

    pub fn hasSideEffects(file_path: []const u8): bool {
        switch (_.mode) {
            .Unspecified, .All => return true,
            .None => return false,
            .Patterns => {
                let norm_buf: [2048]u8 = undefined;
                const rel = _.relativizePath(file_path, &norm_buf);

                for (ex of _.exact_paths) {
                    if (std.mem.eql(u8, rel, ex)) return true;
                }

                const base = PosixPath.basename(rel, null);
                for (pat of _.glob_patterns) {
                    if (Glob.match(pat, rel)) return true;
                    // Per webpack/npm/Bun convention, patterns without '/' match at any depth
                    if (std.mem.indexOfScalar(u8, pat, '/') == null && Glob.match(pat, base)) {
                        return true;
                    }
                }
                return false;
            },
        }
    }
}

// ============================================================================
// 4. SelfReferenceResolver — Package Self-Referencing via `exports`
// ============================================================================

pub struct SelfReferenceMatch {
    package_name: []const u8 = "",
    package_dir: []const u8 = "",
    subpath: []const u8 = ".",
    resolved_path: []const u8 = "",
    status: ExportResolveStatus = .NotFound,
    module_type: ModuleType = .Unknown,
}

/// Resolves imports targeting the enclosing package's own `name` via its `exports` map
/// (e.g. inside package `"my-lib"`, `import "my-lib/utils"` -> `./dist/utils.js`).
pub struct SelfReferenceResolver {
    probe_fs: bool = true,
    extensions: [8][]const u8 = [".cp", ".ts", ".tsx", ".js", ".jsx", ".mjs", ".cjs", ".json"],

    pub static fn init(probe_fs: bool): SelfReferenceResolver {
        return SelfReferenceResolver{
            probe_fs: probe_fs,
        };
    }

    fn probeTarget(allocator: std.mem.Allocator, candidate: []const u8): ([]const u8)? {
        if (!_.probe_fs) return candidate;
        if (isFileExists(candidate)) return candidate;

        let ei: usize = 0;
        while (ei < _.extensions.len) : (ei += 1) {
            const ext = _.extensions[ei];
            const with_ext = std.fmt.allocPrint(allocator, "{s}{s}", .{ candidate, ext }) catch continue;
            if (isFileExists(with_ext)) return with_ext;
            allocator.free(with_ext);
        }

        if (exists(candidate) && isDirectory(candidate)) {
            let di: usize = 0;
            while (di < _.extensions.len) : (di += 1) {
                const ext = _.extensions[di];
                const idx_name = std.fmt.allocPrint(allocator, "index{s}", .{ext}) catch continue;
                defer allocator.free(idx_name);
                const idx_path = joinPosixOwned(allocator, candidate, idx_name) catch continue;
                if (isFileExists(idx_path)) return idx_path;
                allocator.free(idx_path);
            }
        }
        return null;
    }

    static fn extractSubpathForPackage(pkg_name: []const u8, specifier: []const u8): ([]const u8)? {
        if (pkg_name.len == 0) return null;
        if (std.mem.eql(u8, specifier, pkg_name)) return ".";
        if (specifier.len > pkg_name.len &&
            std.mem.startsWith(u8, specifier, pkg_name) &&
            specifier[pkg_name.len] == '/')
        {
            return specifier[pkg_name.len..];
        }
        return null;
    }

    /// Attempts to resolve `specifier` as a self-reference against `pkg` rooted at `pkg_dir`.
    /// Returns `null` when `specifier` does not target `pkg.name` or `pkg.exports` is absent.
    pub fn resolveWithPackage(
        allocator: std.mem.Allocator,
        pkg: PackageJson,
        pkg_dir: []const u8,
        specifier: []const u8,
        conditions: []const []const u8,
    ): SelfReferenceMatch?! {
        if (pkg.exports_val == null) return null;
        const raw_sub = SelfReferenceResolver.extractSubpathForPackage(pkg.name, specifier) ?? return null;
        const subpath = if (std.mem.eql(u8, raw_sub, "."))
            "."
        else
            std.fmt.allocPrint(allocator, ".{s}", .{raw_sub})?;

        const exp_res = pkg.resolveExport(allocator, subpath, conditions)?;
        if (exp_res.status != .Matched) {
            return SelfReferenceMatch{
                package_name: pkg.name,
                package_dir: pkg_dir,
                subpath: subpath,
                resolved_path: "",
                status: exp_res.status,
                module_type: pkg.module_type,
            };
        }

        const rel = stripLeadingDotSlash(exp_res.path);
        const joined = joinPosixOwned(allocator, pkg_dir, rel)?;
        const final_path = _.probeTarget(allocator, joined) ?? {
            if (_.probe_fs) {
                allocator.free(joined);
                return SelfReferenceMatch{
                    package_name: pkg.name,
                    package_dir: pkg_dir,
                    subpath: subpath,
                    resolved_path: "",
                    status: .NotFound,
                    module_type: pkg.module_type,
                };
            }
            return SelfReferenceMatch{
                package_name: pkg.name,
                package_dir: pkg_dir,
                subpath: subpath,
                resolved_path: joined,
                status: .Matched,
                module_type: pkg.module_type,
            };
        };

        return SelfReferenceMatch{
            package_name: pkg.name,
            package_dir: pkg_dir,
            subpath: subpath,
            resolved_path: final_path,
            status: .Matched,
            module_type: pkg.module_type,
        };
    }

    /// Walks up from `from_dir` to find the nearest enclosing `package.json` and
    /// attempts self-reference resolution for `specifier`.
    pub fn resolveFromDir(
        allocator: std.mem.Allocator,
        from_dir: []const u8,
        specifier: []const u8,
        conditions: []const []const u8,
    ): SelfReferenceMatch?! {
        let cur = from_dir;
        let depth: usize = 0;
        while (depth < 64) : (depth += 1) {
            if (pkg := tryReadPackageJsonAt(allocator, cur)) {
                return _.resolveWithPackage(allocator, pkg, cur, specifier, conditions);
            }
            const parent = PosixPath.dirname(cur);
            if (parent.len == 0 || std.mem.eql(u8, parent, cur)) break;
            cur = parent;
        }
        return null;
    }
}
