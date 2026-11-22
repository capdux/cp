// Source: Absorbed from /data/dev/bun/src/resolver/resolver.zig | Synced: 2026-11-22T10:11:34Z
// ============================================================================
// lib/resolver/resolver.cp — Module Resolver, Conditions & StatHash Cache Key
// ============================================================================

const std = @import("std");
import { exists, isDirectory, readFileSync } from "cap:fs";
import { PosixPath, normalizePosix } from "cap:path";
import { DataURL } from "./data_url";
import { PackageJson, ModuleType, ExportResolveStatus } from "./package_json";
import { TsConfigJson } from "./tsconfig";

pub enum ResolveKind: u8 {
    ImportStmt = 0,
    RequireCall = 1,
    DynamicImport = 2,
    EntryPoint = 3,
    CssAtImport = 4,
}

pub enum ResolvedKind: u8 {
    File = 0,
    Builtin = 1,
    DataUrl = 2,
    External = 3,
    Disabled = 4,
}

pub err ResolveError {
    ModuleNotFound,
    InvalidSpecifier,
    PackagePathNotExported,
    PackagePathDisabled,
    PackageImportNotDefined,
    OutOfMemory,
}

/// Ordered condition list for `package.json` `exports` / `imports` routing.
pub struct ResolveConditions {
    items: [16][]const u8 = undefined,
    len: usize = 0,

    pub static fn init(conds: []const []const u8): ResolveConditions {
        let rc = ResolveConditions{};
        for (c of conds) {
            * = rc.add(c);
        }
        return rc;
    }

    pub static fn defaultForKind(kind: ResolveKind, is_browser: bool): ResolveConditions {
        let rc = ResolveConditions{};
        * = rc.add("cp");
        * = rc.add("bun");
        if (kind == .RequireCall) {
            * = rc.add("require");
        } else {
            * = rc.add("import");
        }
        if (is_browser) {
            * = rc.add("browser");
        } else {
            * = rc.add("node");
        }
        * = rc.add("default");
        return rc;
    }

    pub fn add(*_, cond: []const u8): bool {
        if (_.len >= 16) return false;
        if (_.contains(cond)) return true;
        _.items[_.len] = cond;
        _.len += 1;
        return true;
    }

    pub fn contains(cond: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (std.mem.eql(u8, _.items[i], cond)) return true;
        }
        return false;
    }

    pub fn slice(*_): []const []const u8 {
        return _.items[0.._.len];
    }
}

/// 64-bit filesystem metadata cache fingerprint (absorbed from Bun `StatHash`).
pub struct StatHash {
    value: u64 = 0,

    /// Computes a deterministic 64-bit XxHash64 fingerprint from file metadata & path.
    pub static fn fromMetadata(size: u64, mode: u32, mtime: i64, ino: u64, path: []const u8): StatHash {
        let h: u64 = 0;
        zig {
            var hasher = std.hash.XxHash64.init(42);
            hasher.update(std.mem.asBytes(&size));
            hasher.update(std.mem.asBytes(&mode));
            hasher.update(std.mem.asBytes(&mtime));
            hasher.update(std.mem.asBytes(&ino));
            hasher.update(path);
            h = hasher.final();
        }
        return StatHash{ value: h };
    }

    /// Stats `path` on disk via Zig 0.17 `std.c.statx` and returns its 64-bit `StatHash`.
    pub static fn fromPath(path: []const u8): StatHash! {
        let path_buf: [1024]u8 = undefined;
        if (path.len >= path_buf.len) return error.NameTooLong;
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
        const path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;

        let f_size: u64 = 0;
        let f_mode: u32 = 0;
        let f_mtime: i64 = 0;
        let f_ino: u64 = 0;
        let ok = false;
        zig {
            var stx: std.os.linux.Statx = undefined;
            const mask = std.os.linux.STATX{
                .TYPE = true,
                .MODE = true,
                .MTIME = true,
                .INO = true,
                .SIZE = true,
            };
            const rc = std.c.statx(std.os.linux.AT.FDCWD, path_z, 0, mask, &stx);
            if (rc == 0) {
                f_size = stx.size;
                f_mode = @as(u32, stx.mode);
                f_mtime = stx.mtime.sec;
                f_ino = stx.ino;
                ok = true;
            }
        }
        if (!ok) return error.StatFailed;
        return StatHash.fromMetadata(f_size, f_mode, f_mtime, f_ino, path);
    }

    pub fn eql(other: StatHash): bool {
        return _.value == other.value;
    }
}

pub struct ResolveResult {
    kind: ResolvedKind = .File,
    path: []const u8 = "",
    protocol: []const u8 = "",
    module_type: ModuleType = .Unknown,
    data_url: DataURL? = null,
    stat_hash: StatHash? = null,
}

fn isRelativeSpecifier(spec: []const u8): bool {
    return std.mem.eql(u8, spec, ".") ||
        std.mem.eql(u8, spec, "..") ||
        std.mem.startsWith(u8, spec, "./") ||
        std.mem.startsWith(u8, spec, "../");
}

struct SplitPackageResult {
    pkg_name: []const u8 = "",
    subpath: []const u8 = ".",
}

fn splitPackageSpecifier(spec: []const u8): SplitPackageResult {
    // Handle scoped packages "@scope/pkg" or "@scope/pkg/sub"
    if (std.mem.startsWith(u8, spec, "@")) {
        if (first_slash := std.mem.indexOfScalar(u8, spec, '/')) {
            if (second_slash := std.mem.indexOfScalarPos(u8, spec, first_slash + 1, '/')) {
                return SplitPackageResult{
                    pkg_name: spec[0..second_slash],
                    subpath: spec[second_slash..],
                };
            }
            return SplitPackageResult{ pkg_name: spec, subpath: "." };
        }
        return SplitPackageResult{ pkg_name: spec, subpath: "." };
    }
    if (slash := std.mem.indexOfScalar(u8, spec, '/')) {
        return SplitPackageResult{
            pkg_name: spec[0..slash],
            subpath: spec[slash..],
        };
    }
    return SplitPackageResult{ pkg_name: spec, subpath: "." };
}

fn joinTwoPosix(allocator: std.mem.Allocator, a: []const u8, b: []const u8): ([]const u8)! {
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

fn tryReadPackageJson(allocator: std.mem.Allocator, pkg_dir: []const u8): PackageJson? {
    const pkg_json_path = joinTwoPosix(allocator, pkg_dir, "package.json") catch return null;
    defer allocator.free(pkg_json_path);
    if (!exists(pkg_json_path) || isDirectory(pkg_json_path)) return null;
    const raw = readFileSync(pkg_json_path);
    if (raw.len == 0) return null;
    const owned = allocator.dupe(u8, raw) catch {
        return null;
    };
    const parsed = PackageJson.parse(allocator, owned) catch {
        return null;
    };
    return parsed;
}

/// Industrial-grade module resolver supporting `cap:*` / `node:*` / `bun:*` protocols,
/// `data:` URLs, `tsconfig.json` `paths` aliases, relative extension/index probing,
/// and `package.json` `exports` / `imports` conditional routing.
pub struct ModuleResolver {
    root_dir: []const u8 = ".",
    tsconfig: TsConfigJson? = null,
    package_json: PackageJson? = null,
    is_browser: bool = false,
    probe_fs: bool = true,
    extensions: [10][]const u8 = [
        ".cp", ".ts", ".tsx", ".js", ".jsx", ".mjs", ".cjs", ".json", "", "",
    ],
    ext_len: usize = 8,

    pub static fn init(root_dir: []const u8): ModuleResolver {
        return ModuleResolver{
            root_dir: root_dir,
        };
    }

    pub fn setTsConfig(*_, cfg: TsConfigJson) {
        _.tsconfig = cfg;
    }

    pub fn setPackageJson(*_, pkg: PackageJson) {
        _.package_json = pkg;
    }

    pub fn setBrowser(*_, is_browser: bool) {
        _.is_browser = is_browser;
    }

    pub fn setProbeFs(*_, probe_fs: bool) {
        _.probe_fs = probe_fs;
    }

    pub static fn isBuiltin(specifier: []const u8): bool {
        return std.mem.startsWith(u8, specifier, "cap:") ||
            std.mem.startsWith(u8, specifier, "node:") ||
            std.mem.startsWith(u8, specifier, "bun:");
    }

    pub static fn builtinProtocol(specifier: []const u8): []const u8 {
        if (std.mem.startsWith(u8, specifier, "cap:")) return "cap";
        if (std.mem.startsWith(u8, specifier, "node:")) return "node";
        if (std.mem.startsWith(u8, specifier, "bun:")) return "bun";
        return "";
    }

    fn probeExactOrExt(allocator: std.mem.Allocator, candidate: []const u8): ([]const u8)? {
        if (exists(candidate) && !isDirectory(candidate)) {
            return candidate;
        }
        let i: usize = 0;
        while (i < _.ext_len) : (i += 1) {
            const ext = _.extensions[i];
            if (ext.len == 0) continue;
            const with_ext = std.fmt.allocPrint(allocator, "{s}{s}", .{ candidate, ext }) catch return null;
            if (exists(with_ext) && !isDirectory(with_ext)) {
                return with_ext;
            }
            allocator.free(with_ext);
        }
        return null;
    }

    /// Probes a file path, appending known extensions (`.cp`, `.ts`, `.js`, `.json`, ...)
    /// and falling back to directory `index.*` probing if `candidate` is a directory.
    pub fn probeFile(allocator: std.mem.Allocator, candidate: []const u8, conditions: []const []const u8): ([]const u8)? {
        if (!_.probe_fs) {
            return candidate;
        }
        if (found := _.probeExactOrExt(allocator, candidate)) {
            return found;
        }
        if (exists(candidate) && isDirectory(candidate)) {
            // Check package.json inside directory first
            if (dir_pkg := tryReadPackageJson(allocator, candidate)) {
                if (entry := dir_pkg.resolveMainEntry(allocator, conditions)) {
                    const rel_entry = if (std.mem.startsWith(u8, entry, "./")) entry[2..] else entry;
                    const joined = joinTwoPosix(allocator, candidate, rel_entry) catch return null;
                    if (found_entry := _.probeExactOrExt(allocator, joined)) {
                        return found_entry;
                    }
                    allocator.free(joined);
                }
            }
            // Fallback to index.<ext>
            let i: usize = 0;
            while (i < _.ext_len) : (i += 1) {
                const ext = _.extensions[i];
                if (ext.len == 0) continue;
                const idx_name = std.fmt.allocPrint(allocator, "index{s}", .{ext}) catch continue;
                defer allocator.free(idx_name);
                const idx_path = joinTwoPosix(allocator, candidate, idx_name) catch continue;
                if (exists(idx_path) && !isDirectory(idx_path)) {
                    return idx_path;
                }
                allocator.free(idx_path);
            }
        }
        return null;
    }

    fn tryStatHash(path: []const u8): StatHash? {
        const h = StatHash.fromPath(path) catch {
            return null;
        };
        return h;
    }

    fn tryParseDataUrl(specifier: []const u8): DataURL? {
        const d = DataURL.parse(specifier) catch {
            return null;
        };
        return d;
    }

    fn buildFileResult(resolved_path: []const u8, mod_type: ModuleType): ResolveResult {
        let inferred_type = mod_type;
        if (inferred_type == .Unknown) {
            if (std.mem.endsWith(u8, resolved_path, ".mjs") || std.mem.endsWith(u8, resolved_path, ".cp")) {
                inferred_type = .Module;
            } else if (std.mem.endsWith(u8, resolved_path, ".cjs")) {
                inferred_type = .CommonJs;
            }
        }
        const sh = ModuleResolver.tryStatHash(resolved_path);
        return ResolveResult{
            kind: .File,
            path: resolved_path,
            protocol: "file",
            module_type: inferred_type,
            stat_hash: sh,
        };
    }

    fn resolvePackageInDir(
        allocator: std.mem.Allocator,
        pkg_dir: []const u8,
        subpath: []const u8,
        conditions: []const []const u8,
    ): ResolveResult?! {
        if (!exists(pkg_dir) || !isDirectory(pkg_dir)) return null;

        if (pkg := tryReadPackageJson(allocator, pkg_dir)) {
            if (pkg.exports_val != null) {
                const exp_res = pkg.resolveExport(allocator, subpath, conditions)?;
                if (exp_res.status == .Disabled) {
                    return ResolveResult{ kind: .Disabled, path: "" };
                }
                if (exp_res.status != .Matched) {
                    return error.PackagePathNotExported;
                }
                const rel = if (std.mem.startsWith(u8, exp_res.path, "./")) exp_res.path[2..] else exp_res.path;
                const joined = joinTwoPosix(allocator, pkg_dir, rel)?;
                if (probed := _.probeFile(allocator, joined, conditions)) {
                    return ModuleResolver.buildFileResult(probed, pkg.module_type);
                }
                return error.ModuleNotFound;
            }

            if (std.mem.eql(u8, subpath, ".")) {
                if (main_entry := pkg.resolveMainEntry(allocator, conditions)) {
                    const rel = if (std.mem.startsWith(u8, main_entry, "./")) main_entry[2..] else main_entry;
                    const joined = joinTwoPosix(allocator, pkg_dir, rel)?;
                    if (probed := _.probeFile(allocator, joined, conditions)) {
                        return ModuleResolver.buildFileResult(probed, pkg.module_type);
                    }
                }
            } else {
                const rel = if (std.mem.startsWith(u8, subpath, "./"))
                    subpath[2..]
                else if (std.mem.startsWith(u8, subpath, "/"))
                    subpath[1..]
                else
                    subpath;
                const joined = joinTwoPosix(allocator, pkg_dir, rel)?;
                if (probed := _.probeFile(allocator, joined, conditions)) {
                    return ModuleResolver.buildFileResult(probed, pkg.module_type);
                }
            }
        }

        // No package.json in pkg_dir, probe directly
        const target = if (std.mem.eql(u8, subpath, "."))
            pkg_dir
        else blk: {
            const rel = if (std.mem.startsWith(u8, subpath, "./"))
                subpath[2..]
            else if (std.mem.startsWith(u8, subpath, "/"))
                subpath[1..]
            else
                subpath;
            break :blk joinTwoPosix(allocator, pkg_dir, rel)?;
        };
        if (probed := _.probeFile(allocator, target, conditions)) {
            return ModuleResolver.buildFileResult(probed, .Unknown);
        }
        return null;
    }

    /// Resolves `specifier` from `importer_dir` using `kind` and default conditions.
    pub fn resolve(
        allocator: std.mem.Allocator,
        importer_dir: []const u8,
        specifier: []const u8,
        kind: ResolveKind,
    ): ResolveResult! {
        let conds = ResolveConditions.defaultForKind(kind, _.is_browser);
        return _.resolveWithConditions(allocator, importer_dir, specifier, conds.items[0..conds.len]);
    }

    /// Resolves `specifier` from `importer_dir` with explicit condition list.
    pub fn resolveWithConditions(
        allocator: std.mem.Allocator,
        importer_dir: []const u8,
        specifier: []const u8,
        conditions: []const []const u8,
    ): ResolveResult! {
        if (specifier.len == 0) {
            return error.InvalidSpecifier;
        }

        // 1. Builtin protocols: cap:*, node:*, bun:*
        if (ModuleResolver.isBuiltin(specifier)) {
            return ResolveResult{
                kind: .Builtin,
                path: specifier,
                protocol: ModuleResolver.builtinProtocol(specifier),
                module_type: .Module,
            };
        }

        // 2. RFC 2397 data: URL
        if (durl_opt := ModuleResolver.tryParseDataUrl(specifier)) {
            return ResolveResult{
                kind: .DataUrl,
                path: specifier,
                protocol: "data",
                module_type: .Module,
                data_url: durl_opt,
            };
        }

        // 3. Package private #imports
        if (std.mem.startsWith(u8, specifier, "#")) {
            if (pkg := _.package_json) {
                const imp_res = pkg.resolveImport(allocator, specifier, conditions)?;
                if (imp_res.status == .Disabled) {
                    return ResolveResult{ kind: .Disabled, path: "" };
                }
                if (imp_res.status != .Matched) {
                    return error.PackageImportNotDefined;
                }
                if (!std.mem.startsWith(u8, imp_res.path, "./") && !std.mem.startsWith(u8, imp_res.path, "/")) {
                    // Mapped to another bare specifier
                    return _.resolveWithConditions(allocator, importer_dir, imp_res.path, conditions);
                }
                const rel = if (std.mem.startsWith(u8, imp_res.path, "./")) imp_res.path[2..] else imp_res.path;
                const joined = joinTwoPosix(allocator, _.root_dir, rel)?;
                if (probed := _.probeFile(allocator, joined, conditions)) {
                    return ModuleResolver.buildFileResult(probed, pkg.module_type);
                }
                return error.ModuleNotFound;
            }
            return error.PackageImportNotDefined;
        }

        // 4. tsconfig.json paths alias expansion
        if (tsc := _.tsconfig) {
            if (aliased := tsc.resolveAlias(allocator, specifier)) {
                const joined = if (std.mem.startsWith(u8, aliased, "/"))
                    aliased
                else
                    joinTwoPosix(allocator, _.root_dir, aliased)?;
                if (probed := _.probeFile(allocator, joined, conditions)) {
                    return ModuleResolver.buildFileResult(probed, .Unknown);
                }
                if (!_.probe_fs) {
                    return ResolveResult{ kind: .File, path: joined, protocol: "file" };
                }
            }
        }

        // 5. Absolute or Relative path resolution
        if (std.mem.startsWith(u8, specifier, "/") || isRelativeSpecifier(specifier)) {
            const candidate = if (std.mem.startsWith(u8, specifier, "/"))
                specifier
            else
                joinTwoPosix(allocator, importer_dir, specifier)?;

            if (probed := _.probeFile(allocator, candidate, conditions)) {
                let mtype: ModuleType = .Unknown;
                if (pkg := _.package_json) {
                    mtype = pkg.module_type;
                }
                return ModuleResolver.buildFileResult(probed, mtype);
            }
            return error.ModuleNotFound;
        }

        // 6. Bare package resolution walking up cap_modules and node_modules
        const split_res = splitPackageSpecifier(specifier);
        const pkg_name = split_res.pkg_name;
        const raw_subpath = split_res.subpath;
        const subpath = if (std.mem.eql(u8, raw_subpath, "."))
            "."
        else
            std.fmt.allocPrint(allocator, ".{s}", .{raw_subpath})?;

        let cur_dir = importer_dir;
        let depth: usize = 0;
        while (depth < 64) : (depth += 1) {
            // Probe cap_modules first, then node_modules
            const dirs: [2][]const u8 = ["cap_modules", "node_modules"];
            let m_i: usize = 0;
            while (m_i < 2) : (m_i += 1) {
                const mod_root = dirs[m_i];
                const mod_base = joinTwoPosix(allocator, cur_dir, mod_root)?;
                defer allocator.free(mod_base);
                const pkg_dir = joinTwoPosix(allocator, mod_base, pkg_name)?;
                if (res_opt := _.resolvePackageInDir(allocator, pkg_dir, subpath, conditions)?) {
                    return res_opt;
                }
                allocator.free(pkg_dir);
            }

            const parent = PosixPath.dirname(cur_dir);
            if (parent.len == 0 || std.mem.eql(u8, parent, cur_dir)) {
                break;
            }
            cur_dir = parent;
        }

        return error.ModuleNotFound;
    }
}
