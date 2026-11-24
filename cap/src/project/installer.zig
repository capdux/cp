const std = @import("std");
const builtin = @import("builtin");
const main_mod = @import("../main.zig");
const manifest = @import("manifest.zig");
const semver = @import("semver.zig");
const lock = @import("lock.zig");
const native = @import("native.zig");

pub const InstallOptions = struct {
    frozen_lockfile: bool = false,
    rebuild_native: bool = false,
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

    // Hash cap.json if present, plus top-level file names & sizes for deterministic integrity
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

fn discoverWorkspaces(allocator: std.mem.Allocator, workspaces: [][]const u8) !std.StringHashMap([]const u8) {
    var map = std.StringHashMap([]const u8).init(allocator);
    for (workspaces) |pattern| {
        if (pattern.len == 0) continue;
        if (std.mem.endsWith(u8, pattern, "/*")) {
            const base = pattern[0 .. pattern.len - 2];
            var dir = std.Io.Dir.openDir(.cwd(), main_mod.global_io, base, .{ .iterate = true }) catch continue;
            defer dir.close(main_mod.global_io);
            var it = dir.iterate();
            while (try it.next(main_mod.global_io)) |entry| {
                if (entry.kind != .directory and entry.kind != .sym_link) continue;
                const sub_rel = try std.Io.Dir.path.join(allocator, &[_][]const u8{ base, entry.name });
                defer allocator.free(sub_rel);
                const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ sub_rel, "cap.json" });
                defer allocator.free(mf_path);
                const content = readFileAny(allocator, mf_path) catch continue;
                defer allocator.free(content);
                var mf = manifest.Manifest.parse(allocator, content) catch continue;
                defer mf.deinit(allocator);
                const abs_dir = realpathAny(allocator, sub_rel) catch continue;
                try map.put(try allocator.dupe(u8, mf.name), abs_dir);
            }
        } else {
            const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pattern, "cap.json" });
            defer allocator.free(mf_path);
            const content = readFileAny(allocator, mf_path) catch continue;
            defer allocator.free(content);
            var mf = manifest.Manifest.parse(allocator, content) catch continue;
            defer mf.deinit(allocator);
            const abs_dir = realpathAny(allocator, pattern) catch continue;
            try map.put(try allocator.dupe(u8, mf.name), abs_dir);
        }
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

fn installSingleDep(
    allocator: std.mem.Allocator,
    name: []const u8,
    spec: []const u8,
    is_dev: bool,
    ws_map: *const std.StringHashMap([]const u8),
    lk: *lock.Lockfile,
    opts: InstallOptions,
) !void {
    const target_key = opts.target_key orelse native.hostTargetKey();

    var resolved_dir: []const u8 = undefined;
    var resolved_url: []const u8 = undefined;
    var resolved_ver: []const u8 = undefined;
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

        // Read version from package's cap.json
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
        // Registry semver resolution + global cache (~/.cap/cache/pkgs/<safe>@<ver>)
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
    defer if (owns_resolved_dir) allocator.free(resolved_dir);
    defer allocator.free(resolved_url);
    defer allocator.free(resolved_ver);

    // Materialize into cap_modules/<name>
    const link_path = try materializePackage(allocator, name, resolved_dir);
    defer allocator.free(link_path);
    std.debug.print("  ✓ Linked {s}@{s} -> {s}\n", .{ name, resolved_ver, link_path });

    // Compute integrity hash
    const integrity = try computeDirSha256(allocator, resolved_dir);
    errdefer allocator.free(integrity);

    // Check native C configuration and resolve/build immediately
    var native_lib_path: ?[]const u8 = null;
    const pkg_mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ resolved_dir, "cap.json" });
    defer allocator.free(pkg_mf_path);
    if (readFileAny(allocator, pkg_mf_path)) |pkg_mf_content| {
        defer allocator.free(pkg_mf_content);
        if (manifest.Manifest.parse(allocator, pkg_mf_content)) |pmf_val| {
            var pmf = pmf_val;
            defer pmf.deinit(allocator);
            if (pmf.native_config) |nc| {
                if (nc.lib_name.len > 0) {
                    const lib_p = try native.resolveOrBuildNativeLib(
                        allocator,
                        pmf.name,
                        pmf.version,
                        resolved_dir,
                        nc,
                        target_key,
                        opts.rebuild_native,
                    );
                    native_lib_path = lib_p;
                    std.debug.print("  ✓ Native [{s}] ready: {s}\n", .{ nc.lib_name, lib_p });
                }
            }
        } else |_| {}
    } else |_| {}

    try lk.putPackage(allocator, name, lock.LockedPackage{
        .version = try allocator.dupe(u8, resolved_ver),
        .spec = try allocator.dupe(u8, spec),
        .resolved = try allocator.dupe(u8, resolved_url),
        .integrity = integrity,
        .native_lib = native_lib_path,
        .is_dev = is_dev,
    });
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

    var dep_it = mf.dependencies.iterator();
    while (dep_it.next()) |entry| {
        try installSingleDep(allocator, entry.key_ptr.*, entry.value_ptr.*, false, &ws_map, &lk, opts);
    }

    var dev_it = mf.dev_dependencies.iterator();
    while (dev_it.next()) |entry| {
        try installSingleDep(allocator, entry.key_ptr.*, entry.value_ptr.*, true, &ws_map, &lk, opts);
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
        } else if (std.mem.startsWith(u8, arg, "--target=")) {
            opts.target_key = arg["--target=".len..];
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print("Usage: cap install [--frozen-lockfile] [--rebuild] [--target=<os-arch>]\n", .{});
            return;
        }
    }
    try installProject(allocator, opts);
}

fn parseAddSpec(allocator: std.mem.Allocator, raw_arg: []const u8) !struct { name: []const u8, spec: []const u8 } {
    // Case 1: direct local path `file:./pkgs/foo` -> read name from <path>/cap.json
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

    // Case 2: scoped package `@scope/name` or `@scope/name@spec`
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

    // Case 3: unscoped `name@spec` or `name`
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

        // Update cap.lock if it exists
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

fn rebuildPkgDir(
    allocator: std.mem.Allocator,
    pkg_dir: []const u8,
    filter_pkg: ?[]const u8,
    target_opt: ?[]const u8,
    write_prebuilt: bool,
) !usize {
    const mf_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, "cap.json" });
    defer allocator.free(mf_path);
    const content = readFileAny(allocator, mf_path) catch return 0;
    defer allocator.free(content);
    var mf = manifest.Manifest.parse(allocator, content) catch return 0;
    defer mf.deinit(allocator);

    if (filter_pkg) |fp| {
        if (!std.mem.eql(u8, mf.name, fp)) return 0;
    }

    const nc = mf.native_config orelse return 0;
    if (nc.lib_name.len == 0) return 0;

    const is_all = if (target_opt) |t| std.mem.eql(u8, t, "all") else false;
    const should_write_prebuilt = write_prebuilt or is_all;

    var built_count: usize = 0;
    if (is_all) {
        for (native.ALL_TARGETS) |t_key| {
            const out_path = try native.resolveOrBuildNativeLibEx(allocator, mf.name, mf.version, pkg_dir, nc, t_key, .{
                .force_rebuild = true,
                .write_prebuilt = should_write_prebuilt,
            });
            defer allocator.free(out_path);
            built_count += 1;
        }
    } else {
        const t_key = if (target_opt) |t| native.zigTripleToTargetKey(t) else native.hostTargetKey();
        const out_path = try native.resolveOrBuildNativeLibEx(allocator, mf.name, mf.version, pkg_dir, nc, t_key, .{
            .force_rebuild = true,
            .write_prebuilt = should_write_prebuilt,
        });
        defer allocator.free(out_path);
        built_count += 1;
    }
    return built_count;
}

pub fn handleRebuild(allocator: std.mem.Allocator, args: []const []const u8) !void {
    var filter_pkg: ?[]const u8 = null;
    var target_opt: ?[]const u8 = null;
    var write_prebuilt = false;

    for (args) |arg| {
        if (std.mem.startsWith(u8, arg, "--target=")) {
            target_opt = arg["--target=".len..];
        } else if (std.mem.eql(u8, arg, "--write-prebuilt")) {
            write_prebuilt = true;
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print("Usage: cap rebuild [pkg] [--target=<os-arch|all>] [--write-prebuilt]\n", .{});
            std.debug.print("  Targets: linux-x86_64, linux-aarch64, macos-aarch64, macos-x86_64, windows-x86_64, all\n", .{});
            return;
        } else if (!std.mem.startsWith(u8, arg, "-")) {
            filter_pkg = arg;
        }
    }

    var total_built: usize = 0;

    // 1. Scan cap_modules/ (including @scope/*)
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
                    total_built += try rebuildPkgDir(allocator, pkg_dir, filter_pkg, target_opt, write_prebuilt);
                }
            } else {
                const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", entry.name });
                defer allocator.free(pkg_dir);
                total_built += try rebuildPkgDir(allocator, pkg_dir, filter_pkg, target_opt, write_prebuilt);
            }
        }
    } else |_| {}

    // 2. Also check current directory's cap.json if it defines a native library
    if (total_built == 0 or filter_pkg == null) {
        total_built += try rebuildPkgDir(allocator, ".", filter_pkg, target_opt, write_prebuilt);
    }

    if (total_built == 0) {
        std.debug.print("No matching native packages found to rebuild.\n", .{});
    } else {
        std.debug.print("Rebuilt {d} native artifact(s).\n", .{total_built});
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
                    std.debug.print("  native_lib:     {s}\n", .{nl});
                }
                found = true;
            }
        } else |_| {}
    } else |_| {}

    if (!found) {
        std.debug.print("Package '{s}' is not listed in cap.json or cap.lock.\n", .{pkg_name});
    }
}

fn printInstalledPkg(allocator: std.mem.Allocator, pkg_dir: []const u8, display_name: []const u8) void {
    const mf_path = std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, "cap.json" }) catch return;
    defer allocator.free(mf_path);
    if (readFileAny(allocator, mf_path)) |content| {
        defer allocator.free(content);
        if (manifest.Manifest.parse(allocator, content)) |mf_val| {
            var mf = mf_val;
            defer mf.deinit(allocator);
            if (mf.native_config) |nc| {
                std.debug.print("  {s}@{s} [native: lib{s}.a]\n", .{ mf.name, mf.version, nc.lib_name });
            } else {
                std.debug.print("  {s}@{s}\n", .{ mf.name, mf.version });
            }
            return;
        } else |_| {}
    } else |_| {}
    std.debug.print("  {s}\n", .{display_name});
}

pub fn handlePm(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const sub = if (args.len > 0) args[0] else "ls";
    if (!std.mem.eql(u8, sub, "ls")) {
        std.debug.print("Usage: cap pm ls\n", .{});
        return;
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
                printInstalledPkg(allocator, pkg_dir, disp);
                count += 1;
            }
        } else {
            const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", entry.name });
            defer allocator.free(pkg_dir);
            printInstalledPkg(allocator, pkg_dir, entry.name);
            count += 1;
        }
    }
    if (count == 0) {
        std.debug.print("  (none)\n", .{});
    }
}
