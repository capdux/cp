const std = @import("std");
const builtin = @import("builtin");
const main_mod = @import("../main.zig");
const manifest = @import("manifest.zig");

pub const ALL_TARGETS = [_][]const u8{
    "linux-x86_64",
    "linux-aarch64",
    "macos-aarch64",
    "macos-x86_64",
    "windows-x86_64",
};

pub fn hostTargetKey() []const u8 {
    return switch (builtin.os.tag) {
        .linux => switch (builtin.cpu.arch) {
            .aarch64 => "linux-aarch64",
            else => "linux-x86_64",
        },
        .macos => switch (builtin.cpu.arch) {
            .x86_64 => "macos-x86_64",
            else => "macos-aarch64",
        },
        .windows => "windows-x86_64",
        else => "linux-x86_64",
    };
}

pub fn targetKeyToZigTriple(target_key: []const u8) []const u8 {
    if (target_key.len == 0 or std.mem.eql(u8, target_key, "native")) {
        return targetKeyToZigTriple(hostTargetKey());
    }
    if (std.mem.eql(u8, target_key, "linux-x86_64")) return "x86_64-linux-gnu";
    if (std.mem.eql(u8, target_key, "linux-aarch64")) return "aarch64-linux-gnu";
    if (std.mem.eql(u8, target_key, "macos-aarch64")) return "aarch64-macos";
    if (std.mem.eql(u8, target_key, "macos-x86_64")) return "x86_64-macos";
    if (std.mem.eql(u8, target_key, "windows-x86_64")) return "x86_64-windows-gnu";
    return target_key;
}

pub fn zigTripleToTargetKey(triple: []const u8) []const u8 {
    if (triple.len == 0 or std.mem.eql(u8, triple, "native")) {
        return hostTargetKey();
    }
    for (ALL_TARGETS) |t| {
        if (std.mem.eql(u8, triple, t)) return t;
    }
    if (std.mem.indexOf(u8, triple, "linux") != null) {
        if (std.mem.indexOf(u8, triple, "aarch64") != null or std.mem.indexOf(u8, triple, "arm64") != null) {
            return "linux-aarch64";
        }
        return "linux-x86_64";
    }
    if (std.mem.indexOf(u8, triple, "macos") != null or std.mem.indexOf(u8, triple, "darwin") != null) {
        if (std.mem.indexOf(u8, triple, "x86_64") != null) {
            return "macos-x86_64";
        }
        return "macos-aarch64";
    }
    if (std.mem.indexOf(u8, triple, "windows") != null) {
        return "windows-x86_64";
    }
    return hostTargetKey();
}

pub fn dynamicLibExtForTarget(target_key: []const u8) []const u8 {
    if (std.mem.startsWith(u8, target_key, "macos")) return ".dylib";
    if (std.mem.startsWith(u8, target_key, "windows")) return ".dll";
    return ".so";
}

pub fn staticLibExtForTarget(target_key: []const u8) []const u8 {
    if (std.mem.startsWith(u8, target_key, "windows")) return ".lib";
    return ".a";
}

pub fn isCppSource(src_path: []const u8) bool {
    return std.mem.endsWith(u8, src_path, ".cpp") or
        std.mem.endsWith(u8, src_path, ".cc") or
        std.mem.endsWith(u8, src_path, ".cxx") or
        std.mem.endsWith(u8, src_path, ".C");
}

pub fn binHasCppSources(bin_cfg: manifest.BinDependencyConfig) bool {
    for (bin_cfg.sources) |src| {
        if (isCppSource(src)) return true;
    }
    return false;
}

pub fn sanitizePkgName(allocator: std.mem.Allocator, pkg_name: []const u8) ![]const u8 {
    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    for (pkg_name) |c| {
        if (c == '/' or c == '\\' or c == ':') {
            try out.appendSlice(allocator, "__");
        } else {
            try out.append(allocator, c);
        }
    }
    return try out.toOwnedSlice(allocator);
}

pub fn getCapCacheRoot(allocator: std.mem.Allocator) ![]const u8 {
    if (main_mod.global_env.get("CAP_CACHE_DIR")) |custom| {
        if (custom.len > 0) return try allocator.dupe(u8, custom);
    }
    const home = main_mod.global_env.get("HOME") orelse "/root";
    return try std.Io.Dir.path.join(allocator, &[_][]const u8{ home, ".cap", "cache" });
}

pub fn ensureDirPath(path: []const u8) !void {
    if (path.len == 0) return;
    if (path[0] == '/') {
        var root_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
        defer root_dir.close(main_mod.global_io);
        if (path.len > 1) {
            try root_dir.createDirPath(main_mod.global_io, path[1..]);
        }
    } else {
        try std.Io.Dir.createDirPath(.cwd(), main_mod.global_io, path);
    }
}

pub fn fileExists(path: []const u8) bool {
    if (path.len == 0) return false;
    if (path[0] == '/') {
        var f = std.Io.Dir.openFileAbsolute(main_mod.global_io, path, .{}) catch return false;
        f.close(main_mod.global_io);
        return true;
    }
    _ = std.Io.Dir.statFile(.cwd(), main_mod.global_io, path, .{}) catch return false;
    return true;
}

pub fn copyFilePath(allocator: std.mem.Allocator, src_path: []const u8, dest_path: []const u8) !void {
    if (std.Io.Dir.path.dirname(dest_path)) |dir| {
        try ensureDirPath(dir);
    }
    const data = blk: {
        if (src_path[0] == '/') {
            var root_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
            defer root_dir.close(main_mod.global_io);
            break :blk try root_dir.readFileAlloc(main_mod.global_io, src_path[1..], allocator, @fromBackingInt(@intCast(128 * 1024 * 1024)));
        } else {
            break :blk try std.Io.Dir.readFileAlloc(.cwd(), main_mod.global_io, src_path, allocator, @fromBackingInt(@intCast(128 * 1024 * 1024)));
        }
    };
    defer allocator.free(data);

    var out_file = blk: {
        if (dest_path[0] == '/') {
            var root_dir = try std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{});
            defer root_dir.close(main_mod.global_io);
            break :blk try root_dir.createFile(main_mod.global_io, dest_path[1..], .{});
        } else {
            break :blk try std.Io.Dir.createFile(.cwd(), main_mod.global_io, dest_path, .{});
        }
    };
    defer out_file.close(main_mod.global_io);
    try out_file.writeStreamingAll(main_mod.global_io, data);
}

fn spawnSuccess(argv: []const []const u8) !bool {
    var child = try std.process.spawn(main_mod.global_io, .{ .argv = argv });
    const res = try child.wait(main_mod.global_io);
    return switch (res) {
        .exited => |code| code == 0,
        else => false,
    };
}

pub const ResolveOptions = struct {
    force_rebuild: bool = false,
    write_prebuilt: bool = false,
};

pub fn joinBinSubPath(
    allocator: std.mem.Allocator,
    pkg_dir: []const u8,
    bin_path: ?[]const u8,
    rel: []const u8,
) ![]const u8 {
    if (bin_path) |bp| {
        if (bp.len > 0) {
            if (rel.len == 0 or std.mem.eql(u8, rel, ".")) {
                return try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, bp });
            }
            return try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, bp, rel });
        }
    }
    if (rel.len == 0 or std.mem.eql(u8, rel, ".")) {
        return try allocator.dupe(u8, pkg_dir);
    }
    return try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, rel });
}

fn hasOptFlag(flags: [][]const u8) bool {
    for (flags) |f| {
        if (std.mem.startsWith(u8, f, "-O")) return true;
    }
    return false;
}

fn optModeToCFlag(opt: manifest.OptimizeMode) []const u8 {
    return switch (opt) {
        .Debug => "-O0",
        .ReleaseSafe => "-O2",
        .ReleaseFast => "-O3",
        .ReleaseSmall => "-Os",
    };
}

/// Returns true if a `bin` entry represents a local C/C++ static library (rather than an external package version ref).
pub fn isLocalBinLib(bin_cfg: manifest.BinDependencyConfig) bool {
    return bin_cfg.sources.len > 0 or bin_cfg.path != null or bin_cfg.prebuilt != null or bin_cfg.build_cmd != null;
}

/// Three-state static library resolution & Zig C cross-compilation engine for a single `bin.<k>` entry:
/// 1. Prebuilt in package: `<pkg_dir>/<prebuilt>/<target_key>/lib<k>.a` (0ms hit)
/// 2. Global cache: `~/.cap/cache/native/<safe_pkg>@<ver>/<target_key>/lib<k>.a`
/// 3. Compile C sources with `zig cc -target <triple> -c ...` + `zig ar rcs ...`
pub fn resolveOrBuildBinLib(
    allocator: std.mem.Allocator,
    pkg_name: []const u8,
    pkg_ver: []const u8,
    pkg_dir: []const u8,
    bin_name: []const u8,
    bin_cfg: manifest.BinDependencyConfig,
    target_key_in: []const u8,
    force_rebuild: bool,
) ![]const u8 {
    return resolveOrBuildBinLibEx(allocator, pkg_name, pkg_ver, pkg_dir, bin_name, bin_cfg, target_key_in, .{
        .force_rebuild = force_rebuild,
        .write_prebuilt = false,
    });
}

pub fn resolveOrBuildBinLibEx(
    allocator: std.mem.Allocator,
    pkg_name: []const u8,
    pkg_ver: []const u8,
    pkg_dir: []const u8,
    bin_name: []const u8,
    bin_cfg: manifest.BinDependencyConfig,
    target_key_in: []const u8,
    opts: ResolveOptions,
) ![]const u8 {
    if (bin_name.len == 0) return error.InvalidBinConfig;

    const target_key = zigTripleToTargetKey(target_key_in);
    const zig_triple = targetKeyToZigTriple(target_key);
    const lib_ext = staticLibExtForTarget(target_key);
    const lib_filename = try std.fmt.allocPrint(allocator, "lib{s}{s}", .{ bin_name, lib_ext });
    defer allocator.free(lib_filename);

    const prebuilt_dir_name = if (bin_cfg.prebuilt) |pb| (if (pb.len > 0) pb else "prebuilt") else "prebuilt";
    const prebuilt_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{
        pkg_dir,
        prebuilt_dir_name,
        target_key,
        lib_filename,
    });
    errdefer allocator.free(prebuilt_path);

    // State 1: Package-bundled prebuilt static archive (0ms hit)
    if (!opts.force_rebuild) {
        if (fileExists(prebuilt_path)) {
            return prebuilt_path;
        }
        if (std.mem.startsWith(u8, target_key, "windows")) {
            const win_alt_names = [_][]const u8{
                try std.fmt.allocPrint(allocator, "{s}.lib", .{bin_name}),
                try std.fmt.allocPrint(allocator, "lib{s}.a", .{bin_name}),
            };
            defer for (win_alt_names) |n| allocator.free(n);
            for (win_alt_names) |alt_name| {
                const alt_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{
                    pkg_dir,
                    prebuilt_dir_name,
                    target_key,
                    alt_name,
                });
                if (fileExists(alt_path)) {
                    allocator.free(prebuilt_path);
                    return alt_path;
                }
                allocator.free(alt_path);
            }
        }
    }

    const cache_root = try getCapCacheRoot(allocator);
    defer allocator.free(cache_root);
    const safe_pkg = try sanitizePkgName(allocator, pkg_name);
    defer allocator.free(safe_pkg);
    const pkg_tag = try std.fmt.allocPrint(allocator, "{s}@{s}", .{ safe_pkg, pkg_ver });
    defer allocator.free(pkg_tag);

    const cache_target_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{
        cache_root,
        "native",
        pkg_tag,
        target_key,
    });
    defer allocator.free(cache_target_dir);

    const cache_lib_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{
        cache_target_dir,
        lib_filename,
    });
    errdefer allocator.free(cache_lib_path);

    // State 2: Global native cache hit
    if (!opts.force_rebuild and fileExists(cache_lib_path)) {
        if (opts.write_prebuilt) {
            try copyFilePath(allocator, cache_lib_path, prebuilt_path);
            allocator.free(cache_lib_path);
            return prebuilt_path;
        }
        allocator.free(prebuilt_path);
        return cache_lib_path;
    }

    // State 3: Compile C sources via `zig cc` and pack with `zig ar`
    if (bin_cfg.sources.len == 0) {
        return error.BinSourcesMissing;
    }

    const obj_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ cache_target_dir, "obj", bin_name });
    defer allocator.free(obj_dir);
    try ensureDirPath(obj_dir);

    std.debug.print("  ⚙ [bin] Compiling {s}@{s} [{s}] ({s} -> {s})...\n", .{
        pkg_name,
        pkg_ver,
        bin_name,
        target_key,
        zig_triple,
    });
    const t0 = std.Io.Timestamp.now(main_mod.global_io, .awake);

    var obj_paths: std.ArrayList([]const u8) = .empty;
    defer {
        for (obj_paths.items) |p| allocator.free(p);
        obj_paths.deinit(allocator);
    }

    // Pre-build -I flags
    var inc_flags: std.ArrayList([]const u8) = .empty;
    defer {
        for (inc_flags.items) |f| allocator.free(f);
        inc_flags.deinit(allocator);
    }
    if (bin_cfg.include_dirs.len == 0) {
        if (bin_cfg.path) |bp| {
            if (bp.len > 0) {
                const inc_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, bp });
                defer allocator.free(inc_path);
                try inc_flags.append(allocator, try std.fmt.allocPrint(allocator, "-I{s}", .{inc_path}));
            }
        }
    } else {
        for (bin_cfg.include_dirs) |inc| {
            if (inc.len == 0) continue;
            const inc_path = try joinBinSubPath(allocator, pkg_dir, bin_cfg.path, inc);
            defer allocator.free(inc_path);
            try inc_flags.append(allocator, try std.fmt.allocPrint(allocator, "-I{s}", .{inc_path}));
        }
    }

    const opt_cflag: ?[]const u8 = if (bin_cfg.opt) |o|
        (if (!hasOptFlag(bin_cfg.c_flags)) optModeToCFlag(o) else null)
    else
        null;

    for (bin_cfg.sources, 0..) |src_rel, idx| {
        if (src_rel.len == 0) continue;
        const src_path = try joinBinSubPath(allocator, pkg_dir, bin_cfg.path, src_rel);
        defer allocator.free(src_path);

        const stem = std.Io.Dir.path.stem(src_rel);
        const obj_name = try std.fmt.allocPrint(allocator, "{d}_{s}.o", .{ idx, stem });
        defer allocator.free(obj_name);
        const obj_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ obj_dir, obj_name });
        try obj_paths.append(allocator, obj_path);

        const is_cpp = isCppSource(src_rel);
        const compiler_subcmd = if (is_cpp) "c++" else "cc";

        var argv: std.ArrayList([]const u8) = .empty;
        defer argv.deinit(allocator);
        try argv.append(allocator, "zig");
        try argv.append(allocator, compiler_subcmd);
        try argv.append(allocator, "-target");
        try argv.append(allocator, zig_triple);
        try argv.append(allocator, "-c");
        if (opt_cflag) |of| {
            try argv.append(allocator, of);
        }
        for (bin_cfg.c_flags) |cf| {
            if (cf.len > 0) try argv.append(allocator, cf);
        }
        for (inc_flags.items) |inc_f| {
            try argv.append(allocator, inc_f);
        }
        try argv.append(allocator, src_path);
        try argv.append(allocator, "-o");
        try argv.append(allocator, obj_path);

        const ok = try spawnSuccess(argv.items);
        if (!ok) {
            std.debug.print("  ✗ [bin] zig {s} failed for {s}\n", .{ compiler_subcmd, src_path });
            return error.NativeBuildFailed;
        }
    }

    // Archive all .o into lib<bin_name>.a
    var ar_argv: std.ArrayList([]const u8) = .empty;
    defer ar_argv.deinit(allocator);
    try ar_argv.append(allocator, "zig");
    try ar_argv.append(allocator, "ar");
    try ar_argv.append(allocator, "rcs");
    try ar_argv.append(allocator, cache_lib_path);
    for (obj_paths.items) |obj_p| {
        try ar_argv.append(allocator, obj_p);
    }

    const ar_ok = try spawnSuccess(ar_argv.items);
    if (!ar_ok) {
        std.debug.print("  ✗ [bin] zig ar failed for {s}\n", .{cache_lib_path});
        return error.NativeArchiveFailed;
    }

    const ms = t0.durationTo(std.Io.Timestamp.now(main_mod.global_io, .awake)).toMilliseconds();
    std.debug.print("  ✓ [bin] Built {s} ({s}) in {d}ms\n", .{ lib_filename, target_key, ms });

    if (opts.write_prebuilt) {
        try copyFilePath(allocator, cache_lib_path, prebuilt_path);
        std.debug.print("  ✓ [bin] Wrote prebuilt -> {s}\n", .{prebuilt_path});
        allocator.free(cache_lib_path);
        return prebuilt_path;
    }

    allocator.free(prebuilt_path);
    return cache_lib_path;
}

pub const NativeLinkSet = struct {
    include_dirs: std.ArrayList([]const u8),
    static_libs: std.ArrayList([]const u8),
    link_libs: std.ArrayList([]const u8),
    runtime_lib_dirs: std.ArrayList([]const u8),
    runtime_lib_files: std.ArrayList([]const u8),
    has_cpp: bool = false,

    pub fn init() NativeLinkSet {
        return .{
            .include_dirs = .empty,
            .static_libs = .empty,
            .link_libs = .empty,
            .runtime_lib_dirs = .empty,
            .runtime_lib_files = .empty,
            .has_cpp = false,
        };
    }

    pub fn deinit(self: *NativeLinkSet, allocator: std.mem.Allocator) void {
        for (self.include_dirs.items) |p| allocator.free(p);
        self.include_dirs.deinit(allocator);
        for (self.static_libs.items) |p| allocator.free(p);
        self.static_libs.deinit(allocator);
        for (self.link_libs.items) |p| allocator.free(p);
        self.link_libs.deinit(allocator);
        for (self.runtime_lib_dirs.items) |p| allocator.free(p);
        self.runtime_lib_dirs.deinit(allocator);
        for (self.runtime_lib_files.items) |p| allocator.free(p);
        self.runtime_lib_files.deinit(allocator);
    }

    pub fn appendUnique(list: *std.ArrayList([]const u8), allocator: std.mem.Allocator, value: []const u8) void {
        for (list.items) |existing| {
            if (std.mem.eql(u8, existing, value)) {
                allocator.free(value);
                return;
            }
        }
        list.append(allocator, value) catch allocator.free(value);
    }
};

fn readManifestInDir(allocator: std.mem.Allocator, pkg_dir: []const u8) ?manifest.Manifest {
    const mf_path = std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, "cap.json" }) catch return null;
    defer allocator.free(mf_path);

    const content = blk: {
        if (mf_path[0] == '/') {
            var root_dir = std.Io.Dir.openDirAbsolute(main_mod.global_io, "/", .{}) catch return null;
            defer root_dir.close(main_mod.global_io);
            break :blk root_dir.readFileAlloc(main_mod.global_io, mf_path[1..], allocator, @fromBackingInt(@intCast(1024 * 1024))) catch return null;
        } else {
            break :blk std.Io.Dir.readFileAlloc(.cwd(), main_mod.global_io, mf_path, allocator, @fromBackingInt(@intCast(1024 * 1024))) catch return null;
        }
    };
    defer allocator.free(content);
    return manifest.Manifest.parse(allocator, content) catch return null;
}

fn realpathOrDupe(allocator: std.mem.Allocator, path: []const u8) ![]const u8 {
    return std.Io.Dir.realPathFileAlloc(.cwd(), main_mod.global_io, path, allocator) catch try allocator.dupe(u8, path);
}

fn isBuiltinSysLib(name: []const u8) bool {
    const builtins = [_][]const u8{ "c", "m", "pthread", "dl", "rt", "gcc_s" };
    for (builtins) |b| {
        if (std.mem.eql(u8, name, b)) return true;
    }
    return false;
}

fn extractLogicalLibName(sys_dep: manifest.SingleSysDep, target_triple: []const u8) ?[]const u8 {
    if (sys_dep.getRawForTarget(target_triple)) |raw| {
        var n = raw;
        if (std.mem.startsWith(u8, n, "lib")) n = n[3..];
        if (std.mem.indexOf(u8, n, ".")) |dot_idx| n = n[0..dot_idx];
        if (n.len > 0) return n;
    }
    if (sys_dep.name) |k| {
        if (k.len > 0) return k;
    }
    return null;
}

fn dirHasSharedLib(dir_path: []const u8, lib_name: []const u8, raw_name_opt: ?[]const u8, ext: []const u8) bool {
    if (dir_path.len == 0) return false;
    var dir = if (dir_path[0] == '/')
        std.Io.Dir.openDirAbsolute(main_mod.global_io, dir_path, .{ .iterate = true }) catch return false
    else
        std.Io.Dir.openDir(.cwd(), main_mod.global_io, dir_path, .{ .iterate = true }) catch return false;
    defer dir.close(main_mod.global_io);

    var prefix_buf: [256]u8 = undefined;
    const lib_prefix = std.fmt.bufPrint(&prefix_buf, "lib{s}", .{lib_name}) catch return false;

    var exact_buf: [256]u8 = undefined;
    const exact_name = std.fmt.bufPrint(&exact_buf, "lib{s}{s}", .{ lib_name, ext }) catch return false;

    var so_dot_buf: [256]u8 = undefined;
    const so_dot_prefix = std.fmt.bufPrint(&so_dot_buf, "lib{s}.so.", .{lib_name}) catch return false;

    var it = dir.iterate();
    while (it.next(main_mod.global_io) catch null) |entry| {
        const ename = entry.name;
        if (raw_name_opt) |raw| {
            if (std.mem.eql(u8, ename, raw)) return true;
        }
        if (std.mem.eql(u8, ename, exact_name)) return true;
        if (std.mem.eql(u8, ext, ".so")) {
            if (std.mem.startsWith(u8, ename, so_dot_prefix)) return true;
        } else if (std.mem.eql(u8, ext, ".dylib")) {
            if (std.mem.startsWith(u8, ename, lib_prefix) and std.mem.endsWith(u8, ename, ".dylib")) return true;
        } else if (std.mem.eql(u8, ext, ".dll")) {
            var dll_buf: [256]u8 = undefined;
            if (std.fmt.bufPrint(&dll_buf, "{s}.dll", .{lib_name})) |plain_dll| {
                if (std.mem.eql(u8, ename, plain_dll)) return true;
            } else |_| {}
        }
    }
    return false;
}

fn sysLibExistsOnHost(lib_name: []const u8, raw_name_opt: ?[]const u8, ext: []const u8) bool {
    const env_key = if (builtin.os.tag == .macos) "DYLD_LIBRARY_PATH" else "LD_LIBRARY_PATH";
    if (main_mod.global_env.get(env_key)) |env_paths| {
        var it = std.mem.splitScalar(u8, env_paths, ':');
        while (it.next()) |dir_p| {
            if (dir_p.len > 0 and dirHasSharedLib(dir_p, lib_name, raw_name_opt, ext)) {
                return true;
            }
        }
    }

    const std_dirs = [_][]const u8{
        "/usr/lib64",
        "/usr/lib",
        "/lib64",
        "/lib",
        "/usr/local/lib",
        "/usr/lib/x86_64-linux-gnu",
        "/usr/lib/aarch64-linux-gnu",
    };
    for (std_dirs) |dir_p| {
        if (dirHasSharedLib(dir_p, lib_name, raw_name_opt, ext)) {
            return true;
        }
    }
    return false;
}

fn tryMatchBundledDynLibInDir(
    allocator: std.mem.Allocator,
    dir_path: []const u8,
    lib_name: []const u8,
    raw_name_opt: ?[]const u8,
    ext: []const u8,
) !?[]const u8 {
    const std_dyn = try std.fmt.allocPrint(allocator, "lib{s}{s}", .{ lib_name, ext });
    defer allocator.free(std_dyn);
    const full_std = try std.Io.Dir.path.join(allocator, &[_][]const u8{ dir_path, std_dyn });
    if (fileExists(full_std)) {
        return full_std;
    }
    allocator.free(full_std);

    if (raw_name_opt) |raw| {
        if (raw.len > 0) {
            const full_raw = try std.Io.Dir.path.join(allocator, &[_][]const u8{ dir_path, raw });
            if (fileExists(full_raw)) {
                return full_raw;
            }
            allocator.free(full_raw);
        }
    }
    return null;
}

/// Compile-time verification & resolution for a single `sys` dynamic library entry:
/// 1. Bundled prebuilt (`<pkg>/<prebuilt>/<os>-<arch>/lib<name>.so`) or `sys.<name>.path`
///    -> injects `-L<dir>`, `-l<name>`, `-Wl,-rpath,<dir>` (Linux/macOS), and records runtime dir/file.
/// 2. Builtin system libraries (`c`, `m`, `pthread`, `dl`, `rt`, `gcc_s`) -> passes `-l<name>`.
/// 3. External system libraries -> checks `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` + standard system dirs
///    on host builds; fails with clear compile-time error if missing and not `optional: true`.
pub fn verifyAndResolveSysLib(
    allocator: std.mem.Allocator,
    pkg_dir: []const u8,
    sys_dep: manifest.SingleSysDep,
    target_key: []const u8,
    target_triple: []const u8,
    out: *NativeLinkSet,
) !void {
    const ext = dynamicLibExtForTarget(target_key);
    const lib_name = extractLogicalLibName(sys_dep, target_triple) orelse return;
    const raw_opt = sys_dep.getRawForTarget(target_triple);

    // 1. Check bundled prebuilt dynamic library (`<prebuilt>/<os>-<arch>/lib<name><ext>`) or `sys.<name>.path`
    var hit_dir: ?[]const u8 = null;
    var hit_file: ?[]const u8 = null;
    defer if (hit_dir) |d| allocator.free(d);
    defer if (hit_file) |f| allocator.free(f);

    const pb_dir_name = if (sys_dep.prebuilt) |pb| (if (pb.len > 0) pb else "prebuilt") else "prebuilt";
    {
        const dyn_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, pb_dir_name, target_key });
        if (try tryMatchBundledDynLibInDir(allocator, dyn_dir, lib_name, raw_opt, ext)) |matched_file| {
            hit_dir = dyn_dir;
            hit_file = matched_file;
        } else {
            allocator.free(dyn_dir);
        }
    }

    if (hit_file == null) {
        if (sys_dep.path) |p| {
            if (p.len > 0) {
                const path_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, p });
                if (try tryMatchBundledDynLibInDir(allocator, path_dir, lib_name, raw_opt, ext)) |matched_file| {
                    hit_dir = path_dir;
                    hit_file = matched_file;
                } else {
                    const path_target_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, p, target_key });
                    if (try tryMatchBundledDynLibInDir(allocator, path_target_dir, lib_name, raw_opt, ext)) |matched_file2| {
                        hit_dir = path_target_dir;
                        hit_file = matched_file2;
                        allocator.free(path_dir);
                    } else {
                        allocator.free(path_target_dir);
                        allocator.free(path_dir);
                    }
                }
            }
        }
    }

    if (hit_dir != null and hit_file != null) {
        const abs_dir = try realpathOrDupe(allocator, hit_dir.?);
        const abs_file = try realpathOrDupe(allocator, hit_file.?);
        NativeLinkSet.appendUnique(&out.link_libs, allocator, try std.fmt.allocPrint(allocator, "-L{s}", .{abs_dir}));
        NativeLinkSet.appendUnique(&out.link_libs, allocator, try std.fmt.allocPrint(allocator, "-l{s}", .{lib_name}));
        NativeLinkSet.appendUnique(&out.runtime_lib_dirs, allocator, abs_dir);
        NativeLinkSet.appendUnique(&out.runtime_lib_files, allocator, abs_file);
        return;
    }

    // 2. Builtin system libraries pass directly
    if (isBuiltinSysLib(lib_name)) {
        NativeLinkSet.appendUnique(&out.link_libs, allocator, try std.fmt.allocPrint(allocator, "-l{s}", .{lib_name}));
        return;
    }

    // 3. External system dynamic libraries: verify existence on host builds
    if (std.mem.eql(u8, target_key, hostTargetKey())) {
        if (!sysLibExistsOnHost(lib_name, raw_opt, ext)) {
            if (sys_dep.optional) {
                return;
            }
            std.debug.print(
                \\error[cap:sys]: dynamic library 'lib{s}{s}' not found on this host ({s})
                \\  = cause: declared in `sys.{s}` without `"optional": true`, and not found in LD_LIBRARY_PATH or standard system library directories
                \\  = help: install the system package providing lib{s}{s}, place it under `{s}/{s}/{s}/lib{s}{s}`, or set `"{s}": {{ "optional": true }}`
                \\
            , .{
                lib_name,
                ext,
                target_key,
                lib_name,
                lib_name,
                ext,
                pkg_dir,
                pb_dir_name,
                target_key,
                lib_name,
                ext,
                lib_name,
            });
            return error.MissingSysLibrary;
        }
    }

    if (try sys_dep.getLibFlagForTarget(target_triple, allocator)) |flag| {
        NativeLinkSet.appendUnique(&out.link_libs, allocator, flag);
    }
}

pub fn verifyAndResolveSysDeps(
    allocator: std.mem.Allocator,
    pkg_dir: []const u8,
    sys_cfg: manifest.SysDependencyConfig,
    target_key: []const u8,
    target_triple: []const u8,
    out: *NativeLinkSet,
) !void {
    for (sys_cfg.libs) |sys_dep| {
        try verifyAndResolveSysLib(allocator, pkg_dir, sys_dep, target_key, target_triple, out);
    }
}

fn collectPackageRecursive(
    allocator: std.mem.Allocator,
    modules_dir_path: []const u8,
    pkg_name: []const u8,
    target_key: []const u8,
    target_triple: []const u8,
    visited: *std.StringHashMap(void),
    out: *NativeLinkSet,
) !void {
    if (visited.contains(pkg_name)) return;
    try visited.put(try allocator.dupe(u8, pkg_name), {});

    const pkg_dir = try std.Io.Dir.path.join(allocator, &[_][]const u8{ modules_dir_path, pkg_name });
    defer allocator.free(pkg_dir);

    var mf = readManifestInDir(allocator, pkg_dir) orelse return;
    defer mf.deinit(allocator);

    // 1. Collect all `bin` static libraries declared in this package
    var bin_it = mf.bin_dependencies.iterator();
    while (bin_it.next()) |entry| {
        const bin_name = entry.key_ptr.*;
        const bin_cfg = entry.value_ptr.*;
        if (isLocalBinLib(bin_cfg)) {
            if (binHasCppSources(bin_cfg)) {
                out.has_cpp = true;
            }
            if (resolveOrBuildBinLib(allocator, mf.name, mf.version, pkg_dir, bin_name, bin_cfg, target_key, false)) |lib_path| {
                NativeLinkSet.appendUnique(&out.static_libs, allocator, lib_path);
            } else |_| {}

            if (bin_cfg.include_dirs.len == 0) {
                if (bin_cfg.path) |bp| {
                    if (bp.len > 0) {
                        if (std.Io.Dir.path.join(allocator, &[_][]const u8{ pkg_dir, bp }) catch null) |inc_path| {
                            NativeLinkSet.appendUnique(&out.include_dirs, allocator, inc_path);
                        }
                    }
                }
            } else {
                for (bin_cfg.include_dirs) |inc| {
                    if (inc.len == 0) continue;
                    if (joinBinSubPath(allocator, pkg_dir, bin_cfg.path, inc) catch null) |inc_path| {
                        NativeLinkSet.appendUnique(&out.include_dirs, allocator, inc_path);
                    }
                }
            }
        } else if (bin_cfg.version != null) {
            // External static library package reference (e.g. "pkg": "^1.0.0")
            try collectPackageRecursive(allocator, modules_dir_path, bin_name, target_key, target_triple, visited, out);
        }
    }

    // 2. Verify & collect all `sys` dynamic/system libraries declared in this package
    if (mf.sys_dependencies) |sys_cfg| {
        try verifyAndResolveSysDeps(allocator, pkg_dir, sys_cfg, target_key, target_triple, out);
    }

    // 3. Recursively traverse transitive `dep`
    var dep_it = mf.dependencies.iterator();
    while (dep_it.next()) |dep_entry| {
        try collectPackageRecursive(allocator, modules_dir_path, dep_entry.key_ptr.*, target_key, target_triple, visited, out);
    }
}

/// On-demand recursive link collector: ONLY links packages actually referenced by the compilation unit
/// and their transitive `dep`, `bin` static archives (C & C++), and `sys` dynamic libraries.
pub fn collectUsedPackageDepsForTarget(
    allocator: std.mem.Allocator,
    project_root: []const u8,
    used_pkgs: *const std.StringHashMap(void),
    target_triple: []const u8,
) !NativeLinkSet {
    var result = NativeLinkSet.init();
    errdefer result.deinit(allocator);

    if (used_pkgs.count() == 0) return result;

    const target_key = zigTripleToTargetKey(target_triple);
    const modules_dir_path = if (std.mem.eql(u8, project_root, ".") or project_root.len == 0)
        try allocator.dupe(u8, "cap_modules")
    else
        try std.Io.Dir.path.join(allocator, &[_][]const u8{ project_root, "cap_modules" });
    defer allocator.free(modules_dir_path);

    var visited = std.StringHashMap(void).init(allocator);
    defer {
        var vit = visited.iterator();
        while (vit.next()) |e| allocator.free(e.key_ptr.*);
        visited.deinit();
    }

    var it = used_pkgs.iterator();
    while (it.next()) |entry| {
        try collectPackageRecursive(allocator, modules_dir_path, entry.key_ptr.*, target_key, target_triple, &visited, &result);
    }

    return result;
}
