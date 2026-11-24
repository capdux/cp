const std = @import("std");
const builtin = @import("builtin");
const main_mod = @import("../main.zig");
const manifest = @import("manifest.zig");
const semver = @import("semver.zig");
const lock = @import("lock.zig");
const native = @import("native.zig");
const archive = @import("archive.zig");
const registry = @import("registry.zig");

pub const InstallOptions = struct {
    frozen_lockfile: bool = false,
    rebuild_native: bool = false,
    production: bool = false,
    target_key: ?[]const u8 = null,
};

fn readFileAny(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    if (path.len > 0 and path[0] == '/') {
        var root_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
        defer root_dir.close(main_mod.global_io);
        return try root_dir.readFileAlloc(main_mod.global_io, path[1..], allocator, @fromBackingInt(@intCast(16 * 1024 * 1024)));
    }
    return try std.Io.Dir.readFileAlloc(.cwd(), main_mod.global_io, path, allocator, @fromBackingInt(@intCast(16 * 1024 * 1024)));
}

fn realpathAny(allocator: std.mem.Allocator, path: []const u8) ![]u8 {
    return try std.Io.Dir.realPathFileAlloc(.cwd(), main_mod.global_io, path, allocator);
}

fn copyDirRecursive(src: []const u8, dest: []const u8) !void {
    if (std.Io.Dir.path.dirname(dest)) |parent| {
        try native.ensureDirPath(parent);
    }
    const argv = &[_][]const u8{ "cp", "-r", src, dest };
    var child = try std.process.spawn(main_mod.global_io, .{ .argv = argv });
    const res = try child.wait(main_mod.global_io);
    switch (res) {
        .exited => |code| if (code != 0) return error.CopyFailed,
        else => return error.CopyFailed,
    }
}

fn removePathAny(path: []const u8) void {
    if (path.len == 0) return;
    if (path[0] == '/') {
        var root_dir = std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{}) catch return;
        defer root_dir.close(main_mod.global_io);
        root_dir.deleteFile(main_mod.global_io, path[1..]) catch {};
        root_dir.deleteTree(main_mod.global_io, path[1..]) catch {};
    } else {
        std.Io.Dir.deleteFile(.cwd(), main_mod.global_io, path) catch {};
        std.Io.Dir.deleteTree(.cwd(), main_mod.global_io, path) catch {};
    }
}

pub fn computeDirSha256(allocator: std.mem.Allocator, dir_path: []const u8) ![]const u8 {
    var hasher = std.crypto.hash.sha2.Sha256.init(.{});

    const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ dir_path, "cap.json" });
    defer allocator.free(mf_path);
    if (readFileAny(allocator, mf_path)) |mf_bytes| {
        defer allocator.free(mf_bytes);
        hasher.update("cap.json:");
        hasher.update(mf_bytes);
    } else |_| {
        hasher.update(dir_path);
    }

    var digest: [32]u8 = undefined;
    hasher.final(&digest);

    const hex_chars = "0123456789abcdef";
    var out = try allocator.alloc(u8, 7 + 64);
    @memcpy(out[0..7], "sha256-");
    for (digest, 0..) |b, i| {
        out[7 + i * 2] = hex_chars[b >> 4];
        out[7 + i * 2 + 1] = hex_chars[b & 0x0f];
    }
    return out;
}

pub fn materializePackage(allocator: std.mem.Allocator, pkg_name: []const u8, abs_src_dir: []const u8) ![]const u8 {
    const link_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", pkg_name });
    errdefer allocator.free(link_path);

    if (std.Io.Dir.path.dirname(link_path)) |parent| {
        try native.ensureDirPath(parent);
    }

    removePathAny(link_path);

    if (builtin.os.tag == .windows) {
        try copyDirRecursive(abs_src_dir, link_path);
    } else {
        try std.Io.Dir.symLink(.cwd(), main_mod.global_io, abs_src_dir, link_path, .{});
    }
    return link_path;
}

fn registerWorkspaceDir(
    allocator: std.mem.Allocator,
    dir_rel: []const u8,
    map: *std.StringHashMap([]const u8),
) void {
    const mf_path = std.Io.Dir.path.join(allocator, &[_][]const u8{ dir_rel, "cap.json" }) catch return;
    defer allocator.free(mf_path);
    const content = readFileAny(allocator, mf_path) catch return;
    defer allocator.free(content);
    var mf = manifest.Manifest.parse(allocator, content) catch return;
    defer mf.deinit(allocator);
    if (mf.name.len == 0 or std.mem.eql(u8, mf.name, "unknown")) return;
    const abs_dir = realpathAny(allocator, dir_rel) catch return;
    if (map.fetchRemove(mf.name)) |old| {
        allocator.free(old.key);
        allocator.free(old.value);
    }
    map.put(allocator.dupe(u8, mf.name) catch {
        allocator.free(abs_dir);
        return;
    }, abs_dir) catch allocator.free(abs_dir);
}

fn expandWorkspaceSegments(
    allocator: std.mem.Allocator,
    current_dir: []const u8,
    segments: []const []const u8,
    map: *std.StringHashMap([]const u8),
) void {
    if (segments.len == 0) {
        if (current_dir.len > 0) {
            registerWorkspaceDir(allocator, current_dir, map);
        }
        return;
    }

    const seg = segments[0];
    const rest = segments[1..];

    if (std.mem.eql(u8, seg, "*")) {
        const open_path = if (current_dir.len == 0) "." else current_dir;
        var dir = std.Io.Dir.openDir(.cwd(), main_mod.global_io, open_path, .{ .iterate = true }) catch return;
        defer dir.close(main_mod.global_io);
        var it = dir.iterate();
        while (it.next(main_mod.global_io) catch null) |entry| {
            if (entry.name.len == 0 or entry.name[0] == '.') continue;
            if (entry.kind != .directory and entry.kind != .sym_link) continue;
            const next_dir = if (current_dir.len == 0)
                allocator.dupe(u8, entry.name) catch continue
            else
                std.Io.Dir.path.join(allocator, &[_][]const u8{ current_dir, entry.name }) catch continue;
            defer allocator.free(next_dir);
            expandWorkspaceSegments(allocator, next_dir, rest, map);
        }
    } else {
        const next_dir = if (current_dir.len == 0)
            allocator.dupe(u8, seg) catch return
        else
            std.Io.Dir.path.join(allocator, &[_][]const u8{ current_dir, seg }) catch return;
        defer allocator.free(next_dir);
        expandWorkspaceSegments(allocator, next_dir, rest, map);
    }
}

/// Discovers all workspace packages matching pnpm/Bun-style glob patterns
/// (e.g. "packages/*", "packages/@cap/*", "packages/*/*", "apps/web").
pub fn discoverWorkspaces(allocator: std.mem.Allocator, workspaces: [][]const u8) !std.StringHashMap([]const u8) {
    var map = std.StringHashMap([]const u8).init(allocator);
    for (workspaces) |pattern| {
        const trimmed = std.mem.trim(u8, pattern, " \t\r\n/");
        if (trimmed.len == 0) continue;
        var segs: std.ArrayList([]const u8) = .empty;
        defer segs.deinit(allocator);
        var split_it = std.mem.splitScalar(u8, trimmed, '/');
        while (split_it.next()) |part| {
            if (part.len == 0 or std.mem.eql(u8, part, ".")) continue;
            try segs.append(allocator, part);
        }
        expandWorkspaceSegments(allocator, "", segs.items, &map);
    }
    return map;
}

fn resolveBestRegistryVersion(name: []const u8, range: semver.VersionRange) !semver.Version {
    var reg_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, main_mod.registry_path, .{ .iterate = true });
    defer reg_dir.close(main_mod.global_io);
    var pkg_dir = try reg_dir.openDir(main_mod.global_io, name, .{ .iterate = true });
    defer pkg_dir.close(main_mod.global_io);

    var best: ?semver.Version = null;
    var it = pkg_dir.iterate();
    while (try it.next(main_mod.global_io)) |entry| {
        if (entry.kind == .directory) {
            const v = semver.Version.parse(entry.name) catch continue;
            if (range.satisfies(v)) {
                if (best == null or v.compare(best.?) > 0) {
                    best = v;
                }
            }
        }
    }
    if (best) |v| return v;
    return error.PackageNotFound;
}

/// Resolves/builds all local `bin` static libraries in a package directory.
/// Returns the first built static library path (for lockfile metadata) if any.
fn resolvePackageBinLibs(
    allocator: std.mem.Allocator,
    pkg_dir: []const u8,
    target_key: []const u8,
    rebuild_native: bool,
) !?[]const u8 {
    const pkg_mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, "cap.json" });
    defer allocator.free(pkg_mf_path);

    const pkg_mf_content = readFileAny(allocator, pkg_mf_path) catch return null;
    defer allocator.free(pkg_mf_content);

    var pmf = manifest.Manifest.parse(allocator, pkg_mf_content) catch return null;
    defer pmf.deinit(allocator);

    var first_lib: ?[]const u8 = null;
    var bin_it = pmf.bin_dependencies.iterator();
    while (bin_it.next()) |entry| {
        const bin_name = entry.key_ptr.*;
        const bin_cfg = entry.value_ptr.*;
        if (!native.isLocalBinLib(bin_cfg)) continue;

        const lib_p = try native.resolveOrBuildBinLib(
            allocator,
            pmf.name,
            pmf.version,
            pkg_dir,
            bin_name,
            bin_cfg,
            target_key,
            rebuild_native,
        );
        std.debug.print("  ✓ Bin [{s}] ready: {s}\n", .{ bin_name, lib_p });
        if (first_lib == null) {
            first_lib = lib_p;
        } else {
            allocator.free(lib_p);
        }
    }
    return first_lib;
}

fn localRegistryHasPkg(name: []const u8) bool {
    var reg_dir = std.Io.Dir.openDirAbsolute(main_mod.global_io, main_mod.registry_path, .{}) catch return false;
    defer reg_dir.close(main_mod.global_io);
    var pkg_dir = reg_dir.openDir(main_mod.global_io, name, .{}) catch return false;
    pkg_dir.close(main_mod.global_io);
    return true;
}

fn installSingleDep(
    allocator: std.mem.Allocator,
    name: []const u8,
    spec: []const u8,
    is_dev: bool,
    record_root_dep: bool,
    manifest_registry: ?[]const u8,
    ws_map: *const std.StringHashMap([]const u8),
    lk: *lock.Lockfile,
    opts: InstallOptions,
) !void {
    const target_key = opts.target_key orelse native.hostTargetKey();

    var resolved_dir: []const u8 = undefined;
    var resolved_url: []const u8 = undefined;
    var resolved_ver: []const u8 = undefined;
    var remote_integrity: ?[]const u8 = null;
    var owns_resolved_dir = false;

    if (std.mem.startsWith(u8, spec, "file:")) {
        const raw_path = spec[5..];
        const abs_dir = realpathAny(allocator, raw_path) catch {
            std.debug.print("Error: local package path not found: {s}\n", .{raw_path});
            return error.PackageNotFound;
        };
        resolved_dir = abs_dir;
        owns_resolved_dir = true;
        resolved_url = try std.fmt.allocPrint(allocator, "file:{s}", .{abs_dir});

        const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ abs_dir, "cap.json" });
        defer allocator.free(mf_path);
        if (readFileAny(allocator, mf_path)) |content| {
            defer allocator.free(content);
            if (manifest.Manifest.parse(allocator, content)) |pmf_val| {
                var pmf = pmf_val;
                defer pmf.deinit(allocator);
                resolved_ver = try allocator.dupe(u8, pmf.version);
            } else |_| {
                resolved_ver = try allocator.dupe(u8, "0.0.0");
            }
        } else |_| {
            resolved_ver = try allocator.dupe(u8, "0.0.0");
        }
    } else if (std.mem.startsWith(u8, spec, "workspace:")) {
        const ws_dir = ws_map.get(name) orelse {
            std.debug.print("Error: workspace package '{s}' not found in workspaces\n", .{name});
            return error.WorkspacePackageNotFound;
        };
        resolved_dir = ws_dir;
        resolved_url = try std.fmt.allocPrint(allocator, "workspace:{s}", .{ws_dir});

        const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ ws_dir, "cap.json" });
        defer allocator.free(mf_path);
        if (readFileAny(allocator, mf_path)) |content| {
            defer allocator.free(content);
            if (manifest.Manifest.parse(allocator, content)) |pmf_val| {
                var pmf = pmf_val;
                defer pmf.deinit(allocator);
                resolved_ver = try allocator.dupe(u8, pmf.version);
            } else |_| {
                resolved_ver = try allocator.dupe(u8, "0.0.0");
            }
        } else |_| {
            resolved_ver = try allocator.dupe(u8, "0.0.0");
        }
    } else {
        const use_remote = registry.hasExplicitRemoteRegistry(manifest_registry) or !localRegistryHasPkg(name);
        if (use_remote) {
            const reg_url = try registry.resolveRegistryUrl(allocator, manifest_registry);
            defer allocator.free(reg_url);

            var meta = try registry.resolvePackageMeta(allocator, reg_url, name, spec, target_key);
            defer meta.deinit(allocator);

            resolved_ver = try allocator.dupe(u8, meta.version);

            const cache_root = try native.getCapCacheRoot(allocator);
            defer allocator.free(cache_root);
            const safe_pkg = try native.sanitizePkgName(allocator, name);
            defer allocator.free(safe_pkg);
            const pkg_tag = try std.fmt.allocPrint(allocator, "{s}@{s}", .{ safe_pkg, meta.version });
            defer allocator.free(pkg_tag);

            const cache_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ cache_root, "pkgs", pkg_tag });
            resolved_dir = cache_pkg_path;
            owns_resolved_dir = true;

            var cache_exists = true;
            var d = std.Io.Dir.openDirAbsolute(main_mod.global_io, cache_pkg_path, .{}) catch blk: {
                cache_exists = false;
                break :blk std.Io.Dir.cwd();
            };
            if (cache_exists) {
                d.close(main_mod.global_io);
            }

            if (!cache_exists or opts.rebuild_native) {
                if (cache_exists) removePathAny(cache_pkg_path);
                var fetched = try registry.downloadAndExtractResolved(allocator, reg_url, name, &meta, cache_pkg_path, target_key);
                defer fetched.deinit(allocator);
                resolved_url = try allocator.dupe(u8, fetched.resolved_url);
                remote_integrity = try allocator.dupe(u8, fetched.integrity);
            } else {
                resolved_url = try std.fmt.allocPrint(allocator, "{s}/{s}@{s}", .{ reg_url, name, meta.version });
                if (meta.main_pkg.integrity) |mi| {
                    remote_integrity = try allocator.dupe(u8, mi);
                }
            }
        } else {
            // Local mock_registry fallback
            const range = try semver.VersionRange.parse(spec);
            const best_ver = try resolveBestRegistryVersion(name, range);
            const v_str = try best_ver.format(allocator);
            resolved_ver = v_str;

            const cache_root = try native.getCapCacheRoot(allocator);
            defer allocator.free(cache_root);
            const safe_pkg = try native.sanitizePkgName(allocator, name);
            defer allocator.free(safe_pkg);
            const pkg_tag = try std.fmt.allocPrint(allocator, "{s}@{s}", .{ safe_pkg, v_str });
            defer allocator.free(pkg_tag);

            const cache_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ cache_root, "pkgs", pkg_tag });
            resolved_dir = cache_pkg_path;
            owns_resolved_dir = true;
            resolved_url = try std.fmt.allocPrint(allocator, "registry:{s}@{s}", .{ name, v_str });

            var cache_exists = true;
            var d = std.Io.Dir.openDirAbsolute(main_mod.global_io, cache_pkg_path, .{}) catch blk: {
                cache_exists = false;
                break :blk std.Io.Dir.cwd();
            };
            if (cache_exists) {
                d.close(main_mod.global_io);
            } else {
                std.debug.print("  ↓ Fetching {s}@{s} -> {s}\n", .{ name, v_str, cache_pkg_path });
                const reg_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ main_mod.registry_path, name, v_str });
                defer allocator.free(reg_pkg_path);
                try copyDirRecursive(reg_pkg_path, cache_pkg_path);
            }
        }
    }
    defer if (owns_resolved_dir) allocator.free(resolved_dir);
    defer allocator.free(resolved_url);
    defer allocator.free(resolved_ver);

    // Materialize into cap_modules/<name>
    const link_path = try materializePackage(allocator, name, resolved_dir);
    defer allocator.free(link_path);
    std.debug.print("  ✓ Linked {s}@{s} -> {s}\n", .{ name, resolved_ver, link_path });

    // Compute integrity hash (prefer tarball SHA-256 from remote registry if available)
    const integrity = remote_integrity orelse try computeDirSha256(allocator, resolved_dir);
    errdefer allocator.free(integrity);

    // Resolve/build all `bin` static libraries declared in this package
    const first_bin_lib = try resolvePackageBinLibs(allocator, resolved_dir, target_key, opts.rebuild_native);

    const locked_pkg = lock.LockedPackage{
        .version = try allocator.dupe(u8, resolved_ver),
        .spec = try allocator.dupe(u8, spec),
        .resolved = try allocator.dupe(u8, resolved_url),
        .integrity = integrity,
        .native_lib = first_bin_lib,
        .is_dev = is_dev,
    };
    if (record_root_dep) {
        try lk.putPackage(allocator, name, locked_pkg);
    } else {
        try lk.putWorkspacePackage(allocator, name, locked_pkg);
    }
}

pub fn installProject(allocator: std.mem.Allocator, opts: InstallOptions) !void {
    const content = main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found. Run `cap init` first.\n", .{});
        return error.ManifestNotFound;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    if (opts.frozen_lockfile) {
        const lk_content = main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.lock", 4 * 1024 * 1024) catch {
            std.debug.print("Error: --frozen-lockfile specified, but cap.lock does not exist.\n", .{});
            return error.LockfileMissing;
        };
        defer allocator.free(lk_content);
        var existing_lk = try lock.Lockfile.parse(allocator, lk_content);
        defer existing_lk.deinit(allocator);
        existing_lk.verifyFrozen(mf) catch |err| {
            std.debug.print("Error: cap.lock is out of sync with cap.json ({s}). Run `cap install` without --frozen-lockfile.\n", .{@errorName(err)});
            return err;
        };
    }

    var ws_map = try discoverWorkspaces(allocator, mf.workspaces);
    defer {
        var it = ws_map.iterator();
        while (it.next()) |e| {
            allocator.free(e.key_ptr.*);
            allocator.free(e.value_ptr.*);
        }
        ws_map.deinit();
    }

    var lk = try lock.Lockfile.initEmpty(allocator);
    defer lk.deinit(allocator);

    // 1. Auto-link all discovered workspace packages into root cap_modules/ & cap.lock
    if (ws_map.count() > 0) {
        std.debug.print("Workspaces: discovered {d} package(s)\n", .{ws_map.count()});
        var ws_it = ws_map.iterator();
        while (ws_it.next()) |ws_entry| {
            const ws_name = ws_entry.key_ptr.*;
            const is_root_dep = mf.dependencies.contains(ws_name) or mf.dev_dependencies.contains(ws_name);
            if (!is_root_dep) {
                try installSingleDep(allocator, ws_name, "workspace:*", false, false, mf.registry, &ws_map, &lk, opts);
            }
        }
    }

    // 2. Install root dep & (unless --production) dev
    var dep_it = mf.dependencies.iterator();
    while (dep_it.next()) |entry| {
        try installSingleDep(allocator, entry.key_ptr.*, entry.value_ptr.*, false, true, mf.registry, &ws_map, &lk, opts);
    }

    if (!opts.production) {
        var dev_it = mf.dev_dependencies.iterator();
        while (dev_it.next()) |entry| {
            try installSingleDep(allocator, entry.key_ptr.*, entry.value_ptr.*, true, true, mf.registry, &ws_map, &lk, opts);
        }
    }

    // 3. Install dependencies declared inside workspace members (including inter-workspace `workspace:*`)
    if (ws_map.count() > 0) {
        var ws_it2 = ws_map.iterator();
        while (ws_it2.next()) |ws_entry| {
            const ws_dir = ws_entry.value_ptr.*;
            const ws_mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ ws_dir, "cap.json" });
            defer allocator.free(ws_mf_path);
            const ws_mf_bytes = readFileAny(allocator, ws_mf_path) catch continue;
            defer allocator.free(ws_mf_bytes);
            var ws_mf = manifest.Manifest.parse(allocator, ws_mf_bytes) catch continue;
            defer ws_mf.deinit(allocator);

            var wdep_it = ws_mf.dependencies.iterator();
            while (wdep_it.next()) |entry| {
                if (!lk.packages.contains(entry.key_ptr.*)) {
                    try installSingleDep(allocator, entry.key_ptr.*, entry.value_ptr.*, false, false, ws_mf.registry orelse mf.registry, &ws_map, &lk, opts);
                }
            }
            if (!opts.production) {
                var wdev_it = ws_mf.dev_dependencies.iterator();
                while (wdev_it.next()) |entry| {
                    if (!lk.packages.contains(entry.key_ptr.*)) {
                        try installSingleDep(allocator, entry.key_ptr.*, entry.value_ptr.*, true, false, ws_mf.registry orelse mf.registry, &ws_map, &lk, opts);
                    }
                }
            }
        }
    }

    if (!opts.frozen_lockfile) {
        try lk.save(allocator, "cap.lock");
        std.debug.print("Updated cap.lock ({d} packages)\n", .{lk.packages.count()});
    } else {
        std.debug.print("Verified frozen cap.lock ({d} packages)\n", .{lk.packages.count()});
    }
}

pub fn handleInstall(allocator: std.mem.Allocator, args: []const []const u8) !void {
    var opts = InstallOptions{};
    for (args) |arg| {
        if (std.mem.eql(u8, arg, "--frozen-lockfile")) {
            opts.frozen_lockfile = true;
        } else if (std.mem.eql(u8, arg, "--rebuild")) {
            opts.rebuild_native = true;
        } else if (std.mem.eql(u8, arg, "--production") or std.mem.eql(u8, arg, "--prod")) {
            opts.production = true;
        } else if (std.mem.startsWith(u8, arg, "--target=")) {
            opts.target_key = arg["--target=".len..];
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print("Usage: cap install [--production|--prod] [--frozen-lockfile] [--rebuild] [--target=<os-arch>]\n", .{});
            return;
        }
    }
    try installProject(allocator, opts);
}

fn parseAddSpec(allocator: std.mem.Allocator, raw_arg: []const u8) !struct { name: []const u8, spec: []const u8 } {
    if (std.mem.startsWith(u8, raw_arg, "file:")) {
        const local_path = raw_arg[5..];
        const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ local_path, "cap.json" });
        defer allocator.free(mf_path);
        const content = readFileAny(allocator, mf_path) catch {
            const fallback = std.Io.Dir.path.basename(local_path);
            return .{
                .name = try allocator.dupe(u8, fallback),
                .spec = try allocator.dupe(u8, raw_arg),
            };
        };
        defer allocator.free(content);
        var pmf = try manifest.Manifest.parse(allocator, content);
        defer pmf.deinit(allocator);
        return .{
            .name = try allocator.dupe(u8, pmf.name),
            .spec = try allocator.dupe(u8, raw_arg),
        };
    }

    if (raw_arg.len > 0 and raw_arg[0] == '@') {
        if (std.mem.indexOfScalarPos(u8, raw_arg, 1, '@')) |at_idx| {
            return .{
                .name = try allocator.dupe(u8, raw_arg[0..at_idx]),
                .spec = try allocator.dupe(u8, raw_arg[at_idx + 1 ..]),
            };
        }
        return .{
            .name = try allocator.dupe(u8, raw_arg),
            .spec = try allocator.dupe(u8, "^0.1.0"),
        };
    }

    if (std.mem.indexOfScalar(u8, raw_arg, '@')) |at_idx| {
        return .{
            .name = try allocator.dupe(u8, raw_arg[0..at_idx]),
            .spec = try allocator.dupe(u8, raw_arg[at_idx + 1 ..]),
        };
    }
    return .{
        .name = try allocator.dupe(u8, raw_arg),
        .spec = try allocator.dupe(u8, "^0.1.0"),
    };
}

pub fn handleAdd(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage: cap add <pkg|@scope/pkg|file:./path> [-d|--dev] [-g|--global]\n", .{});
        return;
    }
    var pkg_arg: ?[]const u8 = null;
    var is_global = false;
    var is_dev = false;

    for (args) |arg| {
        if (std.mem.eql(u8, arg, "-g") or std.mem.eql(u8, arg, "--global")) {
            is_global = true;
        } else if (std.mem.eql(u8, arg, "-d") or std.mem.eql(u8, arg, "--dev")) {
            is_dev = true;
        } else if (!std.mem.startsWith(u8, arg, "-")) {
            pkg_arg = arg;
        }
    }

    if (pkg_arg == null) {
        std.debug.print("Usage: cap add <pkg|@scope/pkg|file:./path> [-d|--dev] [-g|--global]\n", .{});
        return;
    }

    const parsed = try parseAddSpec(allocator, pkg_arg.?);
    defer allocator.free(parsed.name);
    defer allocator.free(parsed.spec);

    if (is_global) {
        const range = try semver.VersionRange.parse(parsed.spec);
        const best_ver = try resolveBestRegistryVersion(parsed.name, range);
        const v_str = try best_ver.format(allocator);
        defer allocator.free(v_str);
        const home = main_mod.global_env.get("HOME") orelse "/root";
        const global_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ home, ".cap", "global", "cap_modules", parsed.name });
        defer allocator.free(global_pkg_path);
        removePathAny(global_pkg_path);
        const reg_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ main_mod.registry_path, parsed.name, v_str });
        defer allocator.free(reg_pkg_path);
        try copyDirRecursive(reg_pkg_path, global_pkg_path);
        std.debug.print("Globally installed: {s}@{s}\n", .{ parsed.name, v_str });
        return;
    }

    const content = main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found. Run `cap init` first.\n", .{});
        return;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    try mf.addDependency(allocator, parsed.name, parsed.spec, is_dev);
    try mf.save(allocator, "cap.json");

    if (is_dev) {
        std.debug.print("Added dev dependency: {s} -> {s}\n", .{ parsed.name, parsed.spec });
    } else {
        std.debug.print("Added dependency: {s} -> {s}\n", .{ parsed.name, parsed.spec });
    }
    try installProject(allocator, .{});
}

pub fn handleRemove(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage: cap remove <package>\n", .{});
        return;
    }
    const pkg_name = args[0];
    const content = main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found.\n", .{});
        return;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    if (mf.removeDependency(allocator, pkg_name)) {
        try mf.save(allocator, "cap.json");
        const link_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", pkg_name });
        defer allocator.free(link_path);
        removePathAny(link_path);

        if (main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.lock", 4 * 1024 * 1024)) |lk_content| {
            defer allocator.free(lk_content);
            if (lock.Lockfile.parse(allocator, lk_content)) |lk_val| {
                var lk = lk_val;
                defer lk.deinit(allocator);
                _ = lk.removePackage(allocator, pkg_name);
                lk.save(allocator, "cap.lock") catch {};
            } else |_| {}
        } else |_| {}

        std.debug.print("Removed dependency: {s}\n", .{pkg_name});
    } else {
        std.debug.print("Package '{s}' not found in dependencies.\n", .{pkg_name});
    }
}

pub fn handleUpdate(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const target_pkg = if (args.len > 0) args[0] else null;
    if (target_pkg) |pkg| {
        std.debug.print("Updating {s}...\n", .{pkg});
    } else {
        std.debug.print("Updating all dependencies...\n", .{});
    }
    try installProject(allocator, .{ .frozen_lockfile = false, .rebuild_native = false });
}

fn matchesFilter(pkg_name: []const u8, filter: ?[]const u8) bool {
    const f = filter orelse return true;
    if (f.len == 0) return true;
    if (std.mem.eql(u8, pkg_name, f)) return true;
    if (std.mem.endsWith(u8, f, "*")) {
        return std.mem.startsWith(u8, pkg_name, f[0 .. f.len - 1]);
    }
    return false;
}

fn rebuildPkgDir(
    allocator: std.mem.Allocator,
    pkg_dir: []const u8,
    filter_pkg: ?[]const u8,
    target_opt: ?[]const u8,
    write_prebuilt: bool,
    seen_pkgs: *std.StringHashMap(void),
) !usize {
    const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, "cap.json" });
    defer allocator.free(mf_path);
    const content = readFileAny(allocator, mf_path) catch return 0;
    defer allocator.free(content);
    var mf = manifest.Manifest.parse(allocator, content) catch return 0;
    defer mf.deinit(allocator);

    if (!matchesFilter(mf.name, filter_pkg)) return 0;
    if (seen_pkgs.contains(mf.name)) return 0;
    try seen_pkgs.put(try allocator.dupe(u8, mf.name), {});

    const is_all = if (target_opt) |t| std.mem.eql(u8, t, "all") else false;
    const should_write_prebuilt = write_prebuilt or is_all;

    var built_count: usize = 0;
    var bin_it = mf.bin_dependencies.iterator();
    while (bin_it.next()) |entry| {
        const bin_name = entry.key_ptr.*;
        const bin_cfg = entry.value_ptr.*;
        if (!native.isLocalBinLib(bin_cfg) or bin_cfg.sources.len == 0) continue;

        if (is_all) {
            for (native.ALL_TARGETS) |t_key| {
                const out_path = try native.resolveOrBuildBinLibEx(allocator, mf.name, mf.version, pkg_dir, bin_name, bin_cfg, t_key, .{
                    .force_rebuild = true,
                    .write_prebuilt = should_write_prebuilt,
                });
                defer allocator.free(out_path);
                built_count += 1;
            }
        } else {
            const t_key = if (target_opt) |t| native.zigTripleToTargetKey(t) else native.hostTargetKey();
            const out_path = try native.resolveOrBuildBinLibEx(allocator, mf.name, mf.version, pkg_dir, bin_name, bin_cfg, t_key, .{
                .force_rebuild = true,
                .write_prebuilt = should_write_prebuilt,
            });
            defer allocator.free(out_path);
            built_count += 1;
        }
    }
    return built_count;
}

pub fn handleRebuild(allocator: std.mem.Allocator, args: []const []const u8) !void {
    var filter_pkg: ?[]const u8 = null;
    var target_opt: ?[]const u8 = null;
    var write_prebuilt = false;

    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.startsWith(u8, arg, "--target=")) {
            target_opt = arg["--target=".len..];
        } else if (std.mem.eql(u8, arg, "--write-prebuilt")) {
            write_prebuilt = true;
        } else if (std.mem.startsWith(u8, arg, "--filter=")) {
            filter_pkg = arg["--filter=".len..];
        } else if (std.mem.eql(u8, arg, "--filter") or std.mem.eql(u8, arg, "-F")) {
            if (i + 1 < args.len) {
                i += 1;
                filter_pkg = args[i];
            }
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print("Usage: cap rebuild [pkg] [--filter <pkg>] [--target=<os-arch|all>] [--write-prebuilt]\n", .{});
            std.debug.print("  Targets: linux-x86_64, linux-aarch64, macos-aarch64, macos-x86_64, windows-x86_64, all\n", .{});
            return;
        } else if (!std.mem.startsWith(u8, arg, "-")) {
            filter_pkg = arg;
        }
    }

    var seen_pkgs = std.StringHashMap(void).init(allocator);
    defer {
        var sit = seen_pkgs.iterator();
        while (sit.next()) |e| allocator.free(e.key_ptr.*);
        seen_pkgs.deinit();
    }

    var total_built: usize = 0;

    // 1. Scan workspace packages from root cap.json (if any)
    if (main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |root_content| {
        defer allocator.free(root_content);
        if (manifest.Manifest.parse(allocator, root_content)) |root_mf_val| {
            var root_mf = root_mf_val;
            defer root_mf.deinit(allocator);
            if (root_mf.workspaces.len > 0) {
                var ws_map = try discoverWorkspaces(allocator, root_mf.workspaces);
                defer {
                    var wit = ws_map.iterator();
                    while (wit.next()) |e| {
                        allocator.free(e.key_ptr.*);
                        allocator.free(e.value_ptr.*);
                    }
                    ws_map.deinit();
                }
                var wit = ws_map.iterator();
                while (wit.next()) |entry| {
                    total_built += try rebuildPkgDir(allocator, entry.value_ptr.*, filter_pkg, target_opt, write_prebuilt, &seen_pkgs);
                }
            }
        } else |_| {}
    } else |_| {}

    // 2. Scan cap_modules/ (including @scope/*)
    if (std.Io.Dir.openDir(.cwd(), main_mod.global_io, "cap_modules", .{ .iterate = true })) |mod_dir_val| {
        var mod_dir = mod_dir_val;
        defer mod_dir.close(main_mod.global_io);
        var it = mod_dir.iterate();
        while (try it.next(main_mod.global_io)) |entry| {
            if (entry.name.len == 0 or entry.name[0] == '.') continue;
            if (entry.name[0] == '@') {
                var scope_dir = mod_dir.openDir(main_mod.global_io, entry.name, .{ .iterate = true }) catch continue;
                defer scope_dir.close(main_mod.global_io);
                var scope_it = scope_dir.iterate();
                while (try scope_it.next(main_mod.global_io)) |sub| {
                    if (sub.name.len == 0 or sub.name[0] == '.') continue;
                    const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", entry.name, sub.name });
                    defer allocator.free(pkg_dir);
                    total_built += try rebuildPkgDir(allocator, pkg_dir, filter_pkg, target_opt, write_prebuilt, &seen_pkgs);
                }
            } else {
                const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", entry.name });
                defer allocator.free(pkg_dir);
                total_built += try rebuildPkgDir(allocator, pkg_dir, filter_pkg, target_opt, write_prebuilt, &seen_pkgs);
            }
        }
    } else |_| {}

    // 3. Also check current directory's cap.json if it defines `bin` static libraries
    total_built += try rebuildPkgDir(allocator, ".", filter_pkg, target_opt, write_prebuilt, &seen_pkgs);

    if (total_built == 0) {
        std.debug.print("No matching bin packages found to rebuild.\n", .{});
    } else {
        std.debug.print("Rebuilt {d} static library artifact(s).\n", .{total_built});
    }
}

pub fn handleCache(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const sub = if (args.len > 0) args[0] else "dir";
    const cache_root = try native.getCapCacheRoot(allocator);
    defer allocator.free(cache_root);

    if (std.mem.eql(u8, sub, "dir")) {
        std.debug.print("{s}\n", .{cache_root});
    } else if (std.mem.eql(u8, sub, "clean")) {
        removePathAny(cache_root);
        std.debug.print("Cleaned global cache: {s}\n", .{cache_root});
    } else if (std.mem.eql(u8, sub, "ls")) {
        std.debug.print("Cache root: {s}\n", .{cache_root});
        const subdirs = [_][]const u8{ "pkgs", "native" };
        for (subdirs) |sd| {
            const p = try std.Io.Dir.path.join(allocator, &[_][]const u8{ cache_root, sd });
            defer allocator.free(p);
            std.debug.print("[{s}]\n", .{sd});
            var d = std.Io.Dir.openDirAbsolute(main_mod.global_io, p, .{ .iterate = true }) catch {
                std.debug.print("  (empty)\n", .{});
                continue;
            };
            defer d.close(main_mod.global_io);
            var count: usize = 0;
            var it = d.iterate();
            while (try it.next(main_mod.global_io)) |entry| {
                std.debug.print("  - {s}\n", .{entry.name});
                count += 1;
            }
            if (count == 0) std.debug.print("  (empty)\n", .{});
        }
    } else {
        std.debug.print("Usage: cap cache <dir|ls|clean>\n", .{});
    }
}

pub fn handleWhy(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage: cap why <pkg>\n", .{});
        return;
    }
    const pkg_name = args[0];
    var found = false;

    if (main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
        defer allocator.free(content);
        if (manifest.Manifest.parse(allocator, content)) |mf_val| {
            var mf = mf_val;
            defer mf.deinit(allocator);
            if (mf.dependencies.get(pkg_name)) |spec| {
                std.debug.print("{s} (direct dependency in {s}@{s}): \"{s}\"\n", .{ pkg_name, mf.name, mf.version, spec });
                found = true;
            }
            if (mf.dev_dependencies.get(pkg_name)) |spec| {
                std.debug.print("{s} (dev dependency in {s}@{s}): \"{s}\"\n", .{ pkg_name, mf.name, mf.version, spec });
                found = true;
            }
        } else |_| {}
    } else |_| {}

    if (main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.lock", 4 * 1024 * 1024)) |lk_content| {
        defer allocator.free(lk_content);
        if (lock.Lockfile.parse(allocator, lk_content)) |lk_val| {
            var lk = lk_val;
            defer lk.deinit(allocator);
            if (lk.packages.get(pkg_name)) |pkg| {
                std.debug.print("  locked version: {s}\n", .{pkg.version});
                std.debug.print("  resolved:       {s}\n", .{pkg.resolved});
                std.debug.print("  integrity:      {s}\n", .{pkg.integrity});
                if (pkg.native_lib) |nl| {
                    std.debug.print("  bin_lib:        {s}\n", .{nl});
                }
                found = true;
            }
        } else |_| {}
    } else |_| {}

    if (!found) {
        std.debug.print("Package '{s}' is not listed in cap.json or cap.lock.\n", .{pkg_name});
    }
}

fn printInstalledPkg(allocator: std.mem.Allocator, pkg_dir: []const u8, display_name: []const u8, filter_pkg: ?[]const u8) bool {
    const mf_path = std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, "cap.json" }) catch return false;
    defer allocator.free(mf_path);
    if (readFileAny(allocator, mf_path)) |content| {
        defer allocator.free(content);
        if (manifest.Manifest.parse(allocator, content)) |mf_val| {
            var mf = mf_val;
            defer mf.deinit(allocator);
            if (!matchesFilter(mf.name, filter_pkg)) return false;
            if (mf.bin_dependencies.count() > 0) {
                std.debug.print("  {s}@{s} [bin:", .{ mf.name, mf.version });
                var bit = mf.bin_dependencies.iterator();
                while (bit.next()) |e| {
                    std.debug.print(" lib{s}.a", .{e.key_ptr.*});
                }
                std.debug.print("]\n", .{});
            } else {
                std.debug.print("  {s}@{s}\n", .{ mf.name, mf.version });
            }
            return true;
        } else |_| {}
    } else |_| {}
    if (!matchesFilter(display_name, filter_pkg)) return false;
    std.debug.print("  {s}\n", .{display_name});
    return true;
}

pub fn handlePm(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const sub = if (args.len > 0) args[0] else "ls";
    if (!std.mem.eql(u8, sub, "ls")) {
        std.debug.print("Usage: cap pm ls [--filter <pkg>]\n", .{});
        return;
    }

    var filter_pkg: ?[]const u8 = null;
    var i: usize = 1;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.startsWith(u8, arg, "--filter=")) {
            filter_pkg = arg["--filter=".len..];
        } else if (std.mem.eql(u8, arg, "--filter") or std.mem.eql(u8, arg, "-F")) {
            if (i + 1 < args.len) {
                i += 1;
                filter_pkg = args[i];
            }
        }
    }

    var mod_dir = std.Io.Dir.openDir(.cwd(), main_mod.global_io, "cap_modules", .{ .iterate = true }) catch {
        std.debug.print("cap_modules/ is empty or does not exist.\n", .{});
        return;
    };
    defer mod_dir.close(main_mod.global_io);

    std.debug.print("Installed packages (cap_modules/):\n", .{});
    var count: usize = 0;
    var it = mod_dir.iterate();
    while (try it.next(main_mod.global_io)) |entry| {
        if (entry.name.len == 0 or entry.name[0] == '.') continue;
        if (entry.name[0] == '@') {
            var scope_dir = mod_dir.openDir(main_mod.global_io, entry.name, .{ .iterate = true }) catch continue;
            defer scope_dir.close(main_mod.global_io);
            var scope_it = scope_dir.iterate();
            while (try scope_it.next(main_mod.global_io)) |sub_entry| {
                if (sub_entry.name.len == 0 or sub_entry.name[0] == '.') continue;
                const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", entry.name, sub_entry.name });
                defer allocator.free(pkg_dir);
                const disp = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ entry.name, sub_entry.name });
                defer allocator.free(disp);
                if (printInstalledPkg(allocator, pkg_dir, disp, filter_pkg)) count += 1;
            }
        } else {
            const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", entry.name });
            defer allocator.free(pkg_dir);
            if (printInstalledPkg(allocator, pkg_dir, entry.name, filter_pkg)) count += 1;
        }
    }
    if (count == 0) {
        std.debug.print("  (none)\n", .{});
    }
}

fn tarballSafeName(allocator: std.mem.Allocator, pkg_name: []const u8) ![]const u8 {
    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    const trimmed = if (pkg_name.len > 0 and pkg_name[0] == '@') pkg_name[1..] else pkg_name;
    for (trimmed) |c| {
        if (c == '/' or c == '\\' or c == ':') {
            try out.append(allocator, '-');
        } else {
            try out.append(allocator, c);
        }
    }
    return try out.toOwnedSlice(allocator);
}

fn isPackExcludedDir(name: []const u8, prebuilt_dirs: []const []const u8) bool {
    const fixed_excluded = [_][]const u8{
        "prebuilt",
        ".cap",
        "cap_modules",
        "bin",
        ".git",
        ".zig-cache",
        "zig-out",
        "node_modules",
    };
    for (fixed_excluded) |ex| {
        if (std.mem.eql(u8, name, ex)) return true;
    }
    for (prebuilt_dirs) |pb| {
        if (std.mem.eql(u8, name, pb)) return true;
    }
    return false;
}

fn collectFilesRecursive(
    allocator: std.mem.Allocator,
    base_dir: []const u8,
    rel_prefix: []const u8,
    out_files: *std.ArrayList([]const u8),
    exclude_top_dirs: ?[]const []const u8,
    exclude_abs_dir: ?[]const u8,
) !void {
    const open_path = if (rel_prefix.len == 0)
        base_dir
    else if (std.mem.eql(u8, base_dir, "."))
        rel_prefix
    else
        try std.Io.Dir.path.join(allocator, &[_][]const u8{ base_dir, rel_prefix });
    defer if (rel_prefix.len > 0 and !std.mem.eql(u8, base_dir, ".")) allocator.free(open_path);

    if (exclude_abs_dir) |ex_abs| {
        if (realpathAny(allocator, open_path)) |cur_abs| {
            defer allocator.free(cur_abs);
            if (std.mem.eql(u8, cur_abs, ex_abs)) return;
        } else |_| {}
    }

    var dir = blk: {
        if (open_path.len > 0 and open_path[0] == '/') {
            break :blk std.Io.Dir.openDirAbsolute(main_mod.global_io, open_path, .{ .iterate = true }) catch return;
        }
        break :blk std.Io.Dir.openDir(.cwd(), main_mod.global_io, open_path, .{ .iterate = true }) catch return;
    };
    defer dir.close(main_mod.global_io);

    var it = dir.iterate();
    while (try it.next(main_mod.global_io)) |entry| {
        if (entry.name.len == 0) continue;
        // Skip hidden dotfiles/dirs (except .gitignore if desired, skip .git/.cap)
        if (entry.name[0] == '.') {
            if (entry.kind == .directory) continue;
        }
        if (rel_prefix.len == 0) {
            if (exclude_top_dirs) |pb_dirs| {
                if (entry.kind == .directory and isPackExcludedDir(entry.name, pb_dirs)) continue;
            }
            if (std.mem.eql(u8, entry.name, "pack-manifest.json") or std.mem.endsWith(u8, entry.name, ".tar.zst")) continue;
        }

        const child_rel = if (rel_prefix.len == 0)
            try allocator.dupe(u8, entry.name)
        else
            try std.fmt.allocPrint(allocator, "{s}/{s}", .{ rel_prefix, entry.name });

        if (entry.kind == .directory) {
            defer allocator.free(child_rel);
            try collectFilesRecursive(allocator, base_dir, child_rel, out_files, exclude_top_dirs, exclude_abs_dir);
        } else if (entry.kind == .file or entry.kind == .sym_link) {
            try out_files.append(allocator, child_rel);
        } else {
            allocator.free(child_rel);
        }
    }
}

const BinaryPackEntry = struct {
    target_key: []const u8,
    tarball: []const u8,
    sha256: []const u8,
    raw_size: u64,
    compressed_size: u64,
    file_count: usize,
};

const StripSummary = struct {
    stripped_files: usize = 0,
    skipped_files: usize = 0,
    bytes_before: u64 = 0,
    bytes_after: u64 = 0,
};

fn isStrippableBinary(rel_path: []const u8) bool {
    if (std.mem.endsWith(u8, rel_path, ".so") or
        std.mem.endsWith(u8, rel_path, ".dylib") or
        std.mem.endsWith(u8, rel_path, ".a"))
    {
        return true;
    }
    const base = std.Io.Dir.path.basename(rel_path);
    return std.mem.indexOf(u8, base, ".so.") != null;
}

fn stageStrippedFiles(
    allocator: std.mem.Allocator,
    stage_dir: []const u8,
    files: []const []const u8,
) !StripSummary {
    var summary = StripSummary{};
    try native.ensureDirPath(stage_dir);

    for (files) |rel_path| {
        const stage_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ stage_dir, rel_path });
        defer allocator.free(stage_path);
        if (std.Io.Dir.path.dirname(stage_path)) |parent| {
            try native.ensureDirPath(parent);
        }

        if (isStrippableBinary(rel_path)) {
            const orig_stat = std.Io.Dir.statFile(.cwd(), main_mod.global_io, rel_path, .{}) catch null;
            const orig_size: u64 = if (orig_stat) |st| st.size else 0;

            var stripped_ok = false;
            var new_size: u64 = 0;
            const argv = &[_][]const u8{ "zig", "objcopy", "--strip-debug", rel_path, stage_path };
            if (std.process.spawn(main_mod.global_io, .{
                .argv = argv,
                .stdout = .ignore,
                .stderr = .ignore,
            })) |child_val| {
                var child = child_val;
                if (child.wait(main_mod.global_io)) |res| {
                    const exit_ok = switch (res) {
                        .exited => |code| code == 0,
                        else => false,
                    };
                    if (exit_ok) {
                        if (std.Io.Dir.statFile(.cwd(), main_mod.global_io, stage_path, .{})) |new_st| {
                            if (new_st.size > 0 and (orig_size == 0 or new_st.size <= orig_size)) {
                                stripped_ok = true;
                                new_size = new_st.size;
                            }
                        } else |_| {}
                    }
                } else |_| {}
            } else |_| {}

            if (stripped_ok) {
                summary.stripped_files += 1;
                summary.bytes_before += orig_size;
                summary.bytes_after += new_size;
                continue;
            }

            // Unsupported format (e.g. static .a archive or non-ELF): remove partial output and keep original
            removePathAny(stage_path);
            try native.copyFilePath(allocator, rel_path, stage_path);
            summary.skipped_files += 1;
        } else {
            try native.copyFilePath(allocator, rel_path, stage_path);
        }
    }

    return summary;
}

pub fn handlePack(allocator: std.mem.Allocator, args: []const []const u8) !void {
    var target_opt: ?[]const u8 = null;
    var out_dir: []const u8 = ".";
    var npm_mode = false;
    var strip_bins = false;

    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.startsWith(u8, arg, "--target=")) {
            target_opt = arg["--target=".len..];
        } else if (std.mem.eql(u8, arg, "--target") or std.mem.eql(u8, arg, "-t")) {
            if (i + 1 < args.len) {
                i += 1;
                target_opt = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--out=")) {
            out_dir = arg["--out=".len..];
        } else if (std.mem.eql(u8, arg, "--out") or std.mem.eql(u8, arg, "-o")) {
            if (i + 1 < args.len) {
                i += 1;
                out_dir = args[i];
            }
        } else if (std.mem.eql(u8, arg, "--npm") or std.mem.eql(u8, arg, "--tgz")) {
            npm_mode = true;
        } else if (std.mem.eql(u8, arg, "--strip")) {
            strip_bins = true;
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print("Usage: cap pack [--npm|--tgz] [--strip] [--target=<os-arch|all>] [--out=<dir>]\n", .{});
            return;
        }
    }

    const content = main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found in current directory.\n", .{});
        return error.ManifestNotFound;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    // If --target is specified, rebuild bin libraries and write into prebuilt/ first
    if (target_opt != null) {
        var seen_pkgs = std.StringHashMap(void).init(allocator);
        defer {
            var sit = seen_pkgs.iterator();
            while (sit.next()) |e| allocator.free(e.key_ptr.*);
            seen_pkgs.deinit();
        }
        _ = try rebuildPkgDir(allocator, ".", null, target_opt, true, &seen_pkgs);
    }

    try native.ensureDirPath(out_dir);
    const out_abs = realpathAny(allocator, out_dir) catch null;
    defer if (out_abs) |oa| allocator.free(oa);

    // Collect prebuilt directory names declared in bin / sys (default: "prebuilt")
    var pb_dirs: std.ArrayList([]const u8) = .empty;
    defer pb_dirs.deinit(allocator);
    try pb_dirs.append(allocator, "prebuilt");
    {
        var bit = mf.bin_dependencies.iterator();
        while (bit.next()) |entry| {
            if (entry.value_ptr.prebuilt) |pb| {
                var dup = false;
                for (pb_dirs.items) |existing| {
                    if (std.mem.eql(u8, existing, pb)) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) try pb_dirs.append(allocator, pb);
            }
        }
        if (mf.sys_dependencies) |sys| {
            for (sys.libs) |lib| {
                if (lib.prebuilt) |pb| {
                    var dup = false;
                    for (pb_dirs.items) |existing| {
                        if (std.mem.eql(u8, existing, pb)) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) try pb_dirs.append(allocator, pb);
                }
            }
        }
    }

    const safe_name = try tarballSafeName(allocator, mf.name);
    defer allocator.free(safe_name);

    // 1. Pack main source archive (<safe_name>-<ver>.tar.zst, or .tgz with --npm)
    var main_files: std.ArrayList([]const u8) = .empty;
    defer {
        for (main_files.items) |f| allocator.free(f);
        main_files.deinit(allocator);
    }
    try collectFilesRecursive(allocator, ".", "", &main_files, pb_dirs.items, out_abs);

    if (npm_mode) {
        const pkg_json_bytes = try registry.synthesizePackageJson(
            allocator,
            mf.name,
            mf.version,
            mf.main,
            mf.license,
            mf.description,
            &mf.dependencies,
            &mf.dev_dependencies,
        );
        defer allocator.free(pkg_json_bytes);

        const tgz_name = try std.fmt.allocPrint(allocator, "{s}-{s}.tgz", .{ safe_name, mf.version });
        defer allocator.free(tgz_name);
        const tgz_out_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ out_dir, tgz_name });
        defer allocator.free(tgz_out_path);

        var tgz_res = try archive.packDirToTgz(allocator, ".", main_files.items, pkg_json_bytes, tgz_out_path);
        defer tgz_res.deinit(allocator);

        const ratio: f64 = if (tgz_res.raw_size > 0)
            (@as(f64, @floatFromInt(tgz_res.compressed_size)) / @as(f64, @floatFromInt(tgz_res.raw_size))) * 100.0
        else
            0.0;
        std.debug.print("✓ Packed npm .tgz: {s} ({d} files, {d} B -> {d} B, {d:.1}%)\n", .{
            tgz_name,
            tgz_res.file_count,
            tgz_res.raw_size,
            tgz_res.compressed_size,
            ratio,
        });
        std.debug.print("  shasum:    {s}\n  integrity: {s}\n", .{ tgz_res.shasum, tgz_res.integrity });
        return;
    }

    const main_tarball_name = try std.fmt.allocPrint(allocator, "{s}-{s}.tar.zst", .{ safe_name, mf.version });
    defer allocator.free(main_tarball_name);
    const main_out_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ out_dir, main_tarball_name });
    defer allocator.free(main_out_path);

    var main_res = try archive.packDirToTarZst(allocator, ".", main_files.items, main_out_path);
    defer main_res.deinit(allocator);

    const main_ratio: f64 = if (main_res.raw_size > 0)
        (@as(f64, @floatFromInt(main_res.compressed_size)) / @as(f64, @floatFromInt(main_res.raw_size))) * 100.0
    else
        0.0;
    std.debug.print("✓ Packed main: {s} ({d} files, {d} B -> {d} B, {d:.1}%, {s})\n", .{
        main_tarball_name,
        main_res.file_count,
        main_res.raw_size,
        main_res.compressed_size,
        main_ratio,
        main_res.sha256,
    });

    // 2. Discover and pack per-platform prebuilt binary archives (<safe_name>-<ver>-<os>-<arch>.tar.zst)
    var target_files_map = std.StringHashMap(std.ArrayList([]const u8)).init(allocator);
    defer {
        var tit = target_files_map.iterator();
        while (tit.next()) |e| {
            allocator.free(e.key_ptr.*);
            for (e.value_ptr.items) |f| allocator.free(f);
            e.value_ptr.deinit(allocator);
        }
        target_files_map.deinit();
    }

    for (pb_dirs.items) |pb_dir| {
        var d = std.Io.Dir.openDir(.cwd(), main_mod.global_io, pb_dir, .{ .iterate = true }) catch continue;
        defer d.close(main_mod.global_io);
        var it = d.iterate();
        while (try it.next(main_mod.global_io)) |entry| {
            if (entry.kind != .directory) continue;
            if (entry.name.len == 0 or entry.name[0] == '.') continue;
            if (target_opt) |t_req| {
                if (!std.mem.eql(u8, t_req, "all")) {
                    const norm_req = native.zigTripleToTargetKey(t_req);
                    if (!std.mem.eql(u8, entry.name, norm_req)) continue;
                }
            }
            const sub_rel = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ pb_dir, entry.name });
            defer allocator.free(sub_rel);

            var sub_files: std.ArrayList([]const u8) = .empty;
            try collectFilesRecursive(allocator, ".", sub_rel, &sub_files, null, out_abs);
            if (sub_files.items.len == 0) {
                sub_files.deinit(allocator);
                continue;
            }

            const gop = try target_files_map.getOrPut(entry.name);
            if (!gop.found_existing) {
                gop.key_ptr.* = try allocator.dupe(u8, entry.name);
                gop.value_ptr.* = .empty;
            }
            for (sub_files.items) |sf| {
                try gop.value_ptr.append(allocator, sf);
            }
            sub_files.deinit(allocator);
        }
    }

    var bin_entries: std.ArrayList(BinaryPackEntry) = .empty;
    defer {
        for (bin_entries.items) |be| {
            allocator.free(be.target_key);
            allocator.free(be.tarball);
            allocator.free(be.sha256);
        }
        bin_entries.deinit(allocator);
    }

    var t_it = target_files_map.iterator();
    while (t_it.next()) |e| {
        const t_key = e.key_ptr.*;
        const files = e.value_ptr.items;
        if (files.len == 0) continue;

        const bin_tarball_name = try std.fmt.allocPrint(allocator, "{s}-{s}-{s}.tar.zst", .{ safe_name, mf.version, t_key });
        errdefer allocator.free(bin_tarball_name);
        const bin_out_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ out_dir, bin_tarball_name });
        defer allocator.free(bin_out_path);

        var stage_dir_opt: ?[]u8 = null;
        defer if (stage_dir_opt) |sd| {
            removePathAny(sd);
            allocator.free(sd);
        };
        var pack_root: []const u8 = ".";
        if (strip_bins) {
            const stage_dir = try std.fmt.allocPrint(allocator, "{s}/.cap-strip-tmp-{s}", .{ out_dir, t_key });
            stage_dir_opt = stage_dir;
            removePathAny(stage_dir);
            const strip_summary = try stageStrippedFiles(allocator, stage_dir, files);
            pack_root = stage_dir;
            if (strip_summary.stripped_files > 0) {
                const saved: u64 = if (strip_summary.bytes_before >= strip_summary.bytes_after)
                    strip_summary.bytes_before - strip_summary.bytes_after
                else
                    0;
                const saved_pct: f64 = if (strip_summary.bytes_before > 0)
                    (@as(f64, @floatFromInt(saved)) / @as(f64, @floatFromInt(strip_summary.bytes_before))) * 100.0
                else
                    0.0;
                std.debug.print("  ✂ Stripped [{s}]: {d} file(s), {d} B -> {d} B (-{d} B, -{d:.1}%)\n", .{
                    t_key,
                    strip_summary.stripped_files,
                    strip_summary.bytes_before,
                    strip_summary.bytes_after,
                    saved,
                    saved_pct,
                });
            } else if (strip_summary.skipped_files > 0) {
                std.debug.print("  ✂ Strip [{s}]: 0 stripped, {d} skipped (unsupported/non-ELF format)\n", .{
                    t_key,
                    strip_summary.skipped_files,
                });
            }
        }

        const bin_res = try archive.packDirToTarZst(allocator, pack_root, files, bin_out_path);
        const ratio: f64 = if (bin_res.raw_size > 0)
            (@as(f64, @floatFromInt(bin_res.compressed_size)) / @as(f64, @floatFromInt(bin_res.raw_size))) * 100.0
        else
            0.0;
        std.debug.print("✓ Packed binary [{s}]: {s} ({d} files, {d} B -> {d} B, {d:.1}%, {s})\n", .{
            t_key,
            bin_tarball_name,
            bin_res.file_count,
            bin_res.raw_size,
            bin_res.compressed_size,
            ratio,
            bin_res.sha256,
        });

        try bin_entries.append(allocator, .{
            .target_key = try allocator.dupe(u8, t_key),
            .tarball = bin_tarball_name,
            .sha256 = bin_res.sha256,
            .raw_size = bin_res.raw_size,
            .compressed_size = bin_res.compressed_size,
            .file_count = bin_res.file_count,
        });
    }

    // 3. Write pack-manifest.json (registry-ready metadata)
    const manifest_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ out_dir, "pack-manifest.json" });
    defer allocator.free(manifest_path);

    var arena = std.heap.ArenaAllocator.init(allocator);
    defer arena.deinit();
    const aa = arena.allocator();

    var root = std.json.Value{ .object = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable };
    try root.object.put(aa, "name", .{ .string = mf.name });
    try root.object.put(aa, "ver", .{ .string = mf.version });
    try root.object.put(aa, "tarball", .{ .string = main_tarball_name });
    try root.object.put(aa, "integrity", .{ .string = main_res.sha256 });
    try root.object.put(aa, "raw_size", .{ .integer = @intCast(main_res.raw_size) });
    try root.object.put(aa, "compressed_size", .{ .integer = @intCast(main_res.compressed_size) });
    try root.object.put(aa, "file_count", .{ .integer = @intCast(main_res.file_count) });

    var bins_obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    for (bin_entries.items) |be| {
        var bobj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
        try bobj.put(aa, "tarball", .{ .string = be.tarball });
        try bobj.put(aa, "integrity", .{ .string = be.sha256 });
        try bobj.put(aa, "raw_size", .{ .integer = @intCast(be.raw_size) });
        try bobj.put(aa, "compressed_size", .{ .integer = @intCast(be.compressed_size) });
        try bobj.put(aa, "file_count", .{ .integer = @intCast(be.file_count) });
        try bins_obj.put(aa, be.target_key, .{ .object = bobj });
    }
    try root.object.put(aa, "binaries", .{ .object = bins_obj });

    // Also include registry-compatible dist-tags & versions block
    var dist_tags = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try dist_tags.put(aa, "latest", .{ .string = mf.version });
    try root.object.put(aa, "dist-tags", .{ .object = dist_tags });

    var ver_entry = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try ver_entry.put(aa, "tarball", .{ .string = main_tarball_name });
    try ver_entry.put(aa, "integrity", .{ .string = main_res.sha256 });
    try ver_entry.put(aa, "binaries", .{ .object = bins_obj });

    var versions_obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
    try versions_obj.put(aa, mf.version, .{ .object = ver_entry });
    try root.object.put(aa, "versions", .{ .object = versions_obj });

    var out_file = blk: {
        if (manifest_path.len > 0 and manifest_path[0] == '/') {
            var rdir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
            defer rdir.close(main_mod.global_io);
            break :blk try rdir.createFile(main_mod.global_io, manifest_path[1..], .{});
        }
        break :blk try std.Io.Dir.createFile(.cwd(), main_mod.global_io, manifest_path, .{});
    };
    defer out_file.close(main_mod.global_io);

    var wbuf: [8192]u8 = undefined;
    var writer = out_file.writer(main_mod.global_io, &wbuf);
    try std.json.Stringify.value(root, .{ .whitespace = .indent_2 }, &writer.interface);
    try writer.end();

    std.debug.print("✓ Wrote manifest: {s}\n", .{manifest_path});
}

pub fn handlePublish(allocator: std.mem.Allocator, args: []const []const u8) !void {
    var opts = registry.PublishOptions{};

    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.startsWith(u8, arg, "--registry=")) {
            opts.registry = arg["--registry=".len..];
        } else if (std.mem.eql(u8, arg, "--registry")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.registry = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--access=")) {
            opts.access = arg["--access=".len..];
        } else if (std.mem.eql(u8, arg, "--access")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.access = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--tag=")) {
            opts.tag = arg["--tag=".len..];
        } else if (std.mem.eql(u8, arg, "--tag")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.tag = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--otp=")) {
            opts.otp = arg["--otp=".len..];
        } else if (std.mem.eql(u8, arg, "--otp")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.otp = args[i];
            }
        } else if (std.mem.eql(u8, arg, "--dry-run")) {
            opts.dry_run = true;
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print(
                "Usage: cap publish [--registry=<url>] [--access=public|restricted] [--tag=<tag>] [--otp=<code>] [--dry-run]\n",
                .{},
            );
            return;
        }
    }

    const content = main_mod.fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found in current directory.\n", .{});
        return error.ManifestNotFound;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    // Resolve target registry: --registry > CAP_REGISTRY > cap.json "registry" > https://registry.npmjs.org
    const target_registry = blk: {
        if (opts.registry) |r| {
            if (r.len > 0) break :blk try allocator.dupe(u8, std.mem.trimEnd(u8, r, "/"));
        }
        if (main_mod.global_env.get("CAP_REGISTRY")) |env_r| {
            if (env_r.len > 0) break :blk try allocator.dupe(u8, std.mem.trimEnd(u8, env_r, "/"));
        }
        if (mf.registry) |mr| {
            if (mr.len > 0) break :blk try allocator.dupe(u8, std.mem.trimEnd(u8, mr, "/"));
        }
        break :blk try allocator.dupe(u8, registry.NPM_DEFAULT_REGISTRY);
    };
    defer allocator.free(target_registry);

    // Collect source files (exclude prebuilt/ so npm main tarball stays lean and portable)
    var pb_dirs: std.ArrayList([]const u8) = .empty;
    defer pb_dirs.deinit(allocator);
    try pb_dirs.append(allocator, "prebuilt");
    {
        var bit = mf.bin_dependencies.iterator();
        while (bit.next()) |entry| {
            if (entry.value_ptr.prebuilt) |pb| {
                var dup = false;
                for (pb_dirs.items) |existing| {
                    if (std.mem.eql(u8, existing, pb)) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) try pb_dirs.append(allocator, pb);
            }
        }
    }

    var main_files: std.ArrayList([]const u8) = .empty;
    defer {
        for (main_files.items) |f| allocator.free(f);
        main_files.deinit(allocator);
    }
    try collectFilesRecursive(allocator, ".", "", &main_files, pb_dirs.items, null);

    const pkg_json_bytes = try registry.synthesizePackageJson(
        allocator,
        mf.name,
        mf.version,
        mf.main,
        mf.license,
        mf.description,
        &mf.dependencies,
        &mf.dev_dependencies,
    );
    defer allocator.free(pkg_json_bytes);

    var tgz_res = try archive.packDirToTgz(allocator, ".", main_files.items, pkg_json_bytes, null);
    defer tgz_res.deinit(allocator);

    std.debug.print("📦 Package:   {s}@{s}\n", .{ mf.name, mf.version });
    std.debug.print("   Registry:  {s}\n", .{target_registry});
    std.debug.print("   Tarball:   .tgz ({d} files, {d} B -> {d} B)\n", .{
        tgz_res.file_count,
        tgz_res.raw_size,
        tgz_res.compressed_size,
    });
    std.debug.print("   shasum:    {s}\n", .{tgz_res.shasum});
    std.debug.print("   integrity: {s}\n", .{tgz_res.integrity});

    if (opts.dry_run) {
        std.debug.print("✓ Dry-run complete (no network request sent).\n", .{});
        return;
    }

    const token = (try registry.resolveAuthToken(allocator, target_registry)) orelse {
        std.debug.print(
            "Error: No auth token found for {s}. Set CAP_TOKEN, NPM_TOKEN, or configure //host/:_authToken in ~/.npmrc.\n",
            .{target_registry},
        );
        return error.MissingAuthToken;
    };
    defer allocator.free(token);

    try registry.publishToRegistry(
        allocator,
        target_registry,
        mf.name,
        mf.version,
        pkg_json_bytes,
        &tgz_res,
        opts,
        token,
    );
    std.debug.print("✓ Published {s}@{s} to {s} (tag: {s})\n", .{
        mf.name,
        mf.version,
        target_registry,
        opts.tag,
    });
}
