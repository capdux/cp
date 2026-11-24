const std = @import("std");

pub var global_io: std.Io = undefined;
pub var global_env: *std.process.Environ.Map = undefined;
pub var cap_root: []const u8 = undefined;
pub var registry_path: []const u8 = undefined;

pub const fs_shim = struct {
    pub fn cwd() CwdShim {
        return CwdShim{};
    }
};

pub const CwdShim = struct {
    pub fn statFile(self: CwdShim, sub_path: []const u8) !std.Io.Dir.Stat {
        _ = self;
        return std.Io.Dir.statFile(.cwd(), global_io, sub_path, .{});
    }
    pub fn openDir(self: CwdShim, sub_path: []const u8, options: std.Io.Dir.OpenOptions) !std.Io.Dir {
        _ = self;
        return std.Io.Dir.openDir(.cwd(), global_io, sub_path, options);
    }
    pub fn openFile(self: CwdShim, sub_path: []const u8, options: std.Io.Dir.OpenFileOptions) !std.Io.File {
        _ = self;
        return std.Io.Dir.openFile(.cwd(), global_io, sub_path, options);
    }
    pub fn createDirPath(self: CwdShim, io: std.Io, sub_path: []const u8) !void {
        _ = io;
        _ = self;
        return std.Io.Dir.createDirPath(.cwd(), global_io, sub_path);
    }
    pub fn deleteFile(self: CwdShim, sub_path: []const u8) !void {
        _ = self;
        return std.Io.Dir.deleteFile(.cwd(), global_io, sub_path);
    }
    pub fn deleteTree(self: CwdShim, sub_path: []const u8) !void {
        _ = self;
        return std.Io.Dir.deleteTree(.cwd(), global_io, sub_path);
    }
    pub fn realpathAlloc(self: CwdShim, allocator: std.mem.Allocator, sub_path: []const u8) ![]u8 {
        _ = self;
        return std.Io.Dir.realPathFileAlloc(.cwd(), global_io, sub_path, allocator);
    }
    pub fn readFileAlloc(self: CwdShim, allocator: std.mem.Allocator, sub_path: []const u8, max_bytes: usize) ![]u8 {
        _ = self;
        return std.Io.Dir.readFileAlloc(.cwd(), global_io, sub_path, allocator, @fromBackingInt(@intCast(max_bytes)));
    }

    pub fn createFile(self: CwdShim, sub_path: []const u8, options: std.Io.Dir.CreateFileOptions) !std.Io.File {
        _ = self;
        return std.Io.Dir.createFile(.cwd(), global_io, sub_path, options);
    }
    pub fn rename(self: CwdShim, old_sub_path: []const u8, new_sub_path: []const u8) !void {
        _ = self;
        return std.Io.Dir.rename(.cwd(), old_sub_path, .cwd(), new_sub_path, global_io);
    }
};
const process = std.process;
const builtin = @import("builtin");
const manifest = @import("project/manifest.zig");
const semver = @import("project/semver.zig");
const resolver = @import("project/resolver.zig");
const lock = @import("project/lock.zig");
const native = @import("project/native.zig");
const installer = @import("project/installer.zig");
const modular_obj = @import("project/modular_obj.zig");

fn getCapRoot(allocator: std.mem.Allocator) ![]const u8 {
    if (global_env.get("CAP_ROOT")) |root| {
        return try allocator.dupe(u8, root);
    }
    return try allocator.dupe(u8, "/data/dev/cp");
}

fn getRegistryPath(allocator: std.mem.Allocator, root: []const u8) ![]const u8 {
    if (global_env.get("CAP_REGISTRY_PATH")) |path| {
        return try allocator.dupe(u8, path);
    }
    return try std.Io.Dir.path.join(allocator, &[_][]const u8{ root, "mock_registry" });
}

pub fn main(init: std.process.Init) !void {
    global_io = init.io;
    global_env = init.environ_map;
    const allocator = init.arena.allocator();

    cap_root = try getCapRoot(allocator);
    registry_path = try getRegistryPath(allocator, cap_root);

    var args_iter = std.process.Args.iterate(init.minimal.args);
    var args_list: std.ArrayList([]const u8) = .empty;
    while (args_iter.next()) |arg| {
        try args_list.append(allocator, arg);
    }
    const args = args_list.items;

    if (args.len < 2) {
        usage();
        return;
    }

    const cmd = args[1];
    if (std.mem.eql(u8, cmd, "run")) {
        _ = try handleRun(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "build")) {
        try buildPackage(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "clean")) {
        try handleClean(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "init")) {
        try handleInit(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "add")) {
        try installer.handleAdd(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "remove") or std.mem.eql(u8, cmd, "rm")) {
        try installer.handleRemove(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "update") or std.mem.eql(u8, cmd, "up")) {
        try installer.handleUpdate(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "install") or std.mem.eql(u8, cmd, "i")) {
        try installer.handleInstall(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "rebuild")) {
        try installer.handleRebuild(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "pack")) {
        try installer.handlePack(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "publish")) {
        try installer.handlePublish(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "cache")) {
        try installer.handleCache(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "why")) {
        try installer.handleWhy(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "pm")) {
        try installer.handlePm(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "test")) {
        try handleTest(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "fuzz")) {
        try handleFuzz(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "bench")) {
        try handleBench(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "fmt")) {
        try handleFmt(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "lint")) {
        try handleLint(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "from")) {
        try handleFrom(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "demangle")) {
        try handleDemangle(allocator, args[2..]);
    } else if (std.mem.eql(u8, cmd, "sniff")) {
        try sniffHardware();
    } else if (std.mem.eql(u8, cmd, "version") or std.mem.eql(u8, cmd, "--version") or std.mem.eql(u8, cmd, "-V")) {
        std.Io.File.stdout().writeStreamingAll(global_io, "cap 1.0.0\n") catch {};
    } else if (std.mem.eql(u8, cmd, "help") or std.mem.eql(u8, cmd, "--help") or std.mem.eql(u8, cmd, "-h")) {
        usage();
    } else {
        if (std.mem.endsWith(u8, cmd, ".cp")) {
            try runFile(allocator, cmd);
        } else {
            const stat = fs_shim.cwd().statFile(cmd) catch null;
            if (stat != null and stat.?.kind == .directory) {
                if (try resolveDirectoryMain(allocator, cmd)) |main_file| {
                    defer allocator.free(main_file);
                    try runFile(allocator, main_file);
                } else {
                    std.debug.print("Error: Could not resolve entry point in directory '{s}'\n", .{cmd});
                }
            } else {
                const is_script = try handleRun(allocator, args[1..]);
                if (!is_script) {
                    std.debug.print("Unknown command, script, or directory: {s}\n", .{cmd});
                    usage();
                }
            }
        }
    }
}

fn resolveDirectoryMain(allocator: std.mem.Allocator, dir_path: []const u8) !?[]const u8 {
    var dir = fs_shim.cwd().openDir(dir_path, .{}) catch return null;
    defer dir.close(global_io);

    // 1. Check main.cp
    if (dir.statFile(global_io, "main.cp", .{})) |_| {
        return try std.fmt.allocPrint(allocator, "{s}/main.cp", .{dir_path});
    } else |_| {}

    // 2. Check index.cp
    if (dir.statFile(global_io, "index.cp", .{})) |_| {
        return try std.fmt.allocPrint(allocator, "{s}/index.cp", .{dir_path});
    } else |_| {}

    // 3. Check cap.json main
    if (std.Io.Dir.readFileAlloc(dir, global_io, "cap.json", allocator, @fromBackingInt(@intCast(1024 * 1024)))) |content| {
        defer allocator.free(content);
        var mf = manifest.Manifest.parse(allocator, content) catch return null;
        defer mf.deinit(allocator);
        if (mf.main) |main_file| {
            return try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, main_file });
        }
    } else |_| {}

    return null;
}

fn handleFmt(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const cpc_bin = try getCpcBin(allocator);
    defer allocator.free(cpc_bin);

    var cmd_list: std.ArrayList([]const u8) = .empty;
    defer cmd_list.deinit(allocator);
    try cmd_list.append(allocator, cpc_bin);
    try cmd_list.append(allocator, "fmt");

    var has_path = false;
    for (args) |arg| {
        try cmd_list.append(allocator, arg);
        if (!std.mem.startsWith(u8, arg, "-")) {
            has_path = true;
        }
    }
    if (!has_path) {
        try cmd_list.append(allocator, ".");
    }

    var child = try std.process.spawn(global_io, .{ .argv = cmd_list.items });
    const term = try child.wait(global_io);
    switch (term) {
        .exited => |code| {
            if (code != 0) std.process.exit(code);
        },
        else => std.process.exit(1),
    }
}

fn shouldSkipLintDir(name: []const u8) bool {
    const skip_names = [_][]const u8{
        ".cap",
        "cap_modules",
        "vendor",
        "bin",
        "temp",
        ".git",
        "target",
        "zig-cache",
        ".zig-cache",
        "zig-out",
        "node_modules",
    };
    for (skip_names) |s| {
        if (std.mem.eql(u8, name, s)) return true;
    }
    return false;
}

fn collectCpFilesForLint(allocator: std.mem.Allocator, dir_path: []const u8, list: *std.ArrayList([]const u8)) !void {
    var dir = fs_shim.cwd().openDir(dir_path, .{ .iterate = true }) catch return;
    defer dir.close(global_io);
    var it = dir.iterate();
    while (try it.next(global_io)) |entry| {
        if (entry.kind == .directory) {
            if (shouldSkipLintDir(entry.name)) continue;
            const sub_path = if (std.mem.eql(u8, dir_path, "."))
                try allocator.dupe(u8, entry.name)
            else
                try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name });
            defer allocator.free(sub_path);
            try collectCpFilesForLint(allocator, sub_path, list);
        } else if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cp")) {
            const full_path = if (std.mem.eql(u8, dir_path, "."))
                try allocator.dupe(u8, entry.name)
            else
                try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name });
            try list.append(allocator, full_path);
        }
    }
}

fn handleLint(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const cpc_bin = try getCpcBin(allocator);
    defer allocator.free(cpc_bin);

    var flags: std.ArrayList([]const u8) = .empty;
    defer flags.deinit(allocator);
    var raw_paths: std.ArrayList([]const u8) = .empty;
    defer raw_paths.deinit(allocator);

    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "-n") or std.mem.eql(u8, arg, "--zig")) {
            try flags.append(allocator, arg);
            if (i + 1 < args.len) {
                i += 1;
                try flags.append(allocator, args[i]);
            }
        } else if (std.mem.startsWith(u8, arg, "-")) {
            try flags.append(allocator, arg);
        } else {
            try raw_paths.append(allocator, arg);
        }
    }
    if (raw_paths.items.len == 0) {
        try raw_paths.append(allocator, ".");
    }

    var target_files: std.ArrayList([]const u8) = .empty;
    defer {
        for (target_files.items) |f| allocator.free(f);
        target_files.deinit(allocator);
    }

    for (raw_paths.items) |p| {
        const stat = fs_shim.cwd().statFile(p) catch {
            std.debug.print("Error: Lint target '{s}' not found.\n", .{p});
            std.process.exit(1);
        };
        if (stat.kind == .directory) {
            try collectCpFilesForLint(allocator, p, &target_files);
        } else {
            try target_files.append(allocator, try allocator.dupe(u8, p));
        }
    }

    if (target_files.items.len == 0) {
        std.debug.print("No .cp files found to lint.\n", .{});
        return;
    }

    var cmd_list: std.ArrayList([]const u8) = .empty;
    defer cmd_list.deinit(allocator);
    try cmd_list.append(allocator, cpc_bin);
    try cmd_list.append(allocator, "brief");
    for (flags.items) |f| {
        try cmd_list.append(allocator, f);
    }
    for (target_files.items) |f| {
        try cmd_list.append(allocator, f);
    }

    var child = try std.process.spawn(global_io, .{ .argv = cmd_list.items });
    const term = try child.wait(global_io);
    switch (term) {
        .exited => |code| {
            if (code != 0) std.process.exit(code);
        },
        else => std.process.exit(1),
    }
}

fn handleFrom(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage:\n  cap from -ts <src_dir> [-o <out_dir>] [--no-deps] [--max-depth <N>]\n  cap from -rs <src_dir> [-o <out_dir>] [--no-deps] [--max-depth <N>]\n", .{});
        std.process.exit(1);
    }
    const cpc_bin = try getCpcBin(allocator);
    defer allocator.free(cpc_bin);

    var cmd_list: std.ArrayList([]const u8) = .empty;
    defer cmd_list.deinit(allocator);
    try cmd_list.append(allocator, cpc_bin);
    try cmd_list.append(allocator, "from");
    for (args) |a| {
        if (std.mem.eql(u8, a, "-ts")) {
            try cmd_list.append(allocator, "--ts");
        } else if (std.mem.eql(u8, a, "-rs")) {
            try cmd_list.append(allocator, "--rs");
        } else {
            try cmd_list.append(allocator, a);
        }
    }

    var child = try std.process.spawn(global_io, .{ .argv = cmd_list.items });
    const term = try child.wait(global_io);
    switch (term) {
        .exited => |code| {
            if (code != 0) std.process.exit(code);
        },
        else => std.process.exit(1),
    }
}

fn usage() void {
    std.debug.print(
        \\  ██████╗      ██╗     ██████╗ 
        \\ ██╔════╝     ████╗    ██╔══██╗   CAP : Concurrent Application Plus v1.0.0
        \\ ██║         ██╔═██╗   ██████╔╝   The All-in-One AI Agent & Native Systems Toolchain
        \\ ██║        ██╔╝ ╚██╗  ██╔═══╝    [ Package Manager • Compiler • Runner • Test • Bench ]
        \\ ╚██████╗  ██╔╝   ╚██╗ ██║        ^ (Upward Arrow) Linking C & P as the Letter 'A' (Application)
        \\  ╚═════╝  ╚═╝     ╚═╝ ╚═╝
        \\
        \\Usage:
        \\  cap <file.cp|dir>             Run a CP file or directory directly (Debug)
        \\  cap run [options] <file|script> Run a script or file (-m modular+parallel .o cache)
        \\  cap build [target] [options]  Build a CP package or directory
        \\  cap clean [--all]             Clean .cap/ build caches (use --all to also remove bin/)
        \\  cap fmt [--check] [--stdout] [path...] Format CP source files (default: .)
        \\  cap lint [path...]            Run syntax, borrow & 4-gate static audit (cpc brief)
        \\  cap from <-ts|-rs> <dir>      Migrate TypeScript or Rust project & deps to CP + migrate.md
        \\  cap test [dir] [-m] [--fuzz]  Run test suite (default: test/) or libFuzzer
        \\  cap fuzz [target] [options]   Fuzz test target using LLVM libFuzzer + ASan
        \\  cap bench <file.cp>           Run microsecond benchmark (ReleaseFast)
        \\  cap demangle [trace_file]     Demangle Zig stack traces to original CP lines
        \\  cap init [app|agent|lib]      Initialize a new CP project template
        \\  cap add <pkg> [-d] [-g]       Add a dependency (@scope/pkg, file:./..., -d dev, -g global)
        \\  cap remove <package>          Remove a dependency from cap.json, cap.lock & cap_modules
        \\  cap update [package]          Update dependencies and refresh cap.lock
        \\  cap install [options]         Install dependencies (--production|--prod, --frozen-lockfile, --rebuild)
        \\  cap rebuild [pkg] [options]   Rebuild bin C/C++ libs (--filter <pkg>, --target=<os-arch|all>, --write-prebuilt)
        \\  cap pack [options]            Pack .tar.zst (or --npm .tgz) archives (--strip, --target=<os-arch|all>, --out=<dir>)
        \\  cap publish [options]         Publish .tgz package to npm/Verdaccio registry (--registry, --access, --tag, --otp, --dry-run)
        \\  cap cache <dir|ls|clean>      Inspect or clean global ~/.cap/cache
        \\  cap why <package>             Explain why a package is installed
        \\  cap pm ls [--filter <pkg>]    List installed packages in cap_modules/
        \\  cap sniff                     Detect hardware features (AVX2, CPU, OS)
        \\  cap version, --version, -V    Show cap version (1.0.0)
        \\  cap help                      Show this help message
        \\
        \\Test Options:
        \\  -m, --modular                 Share modular Zig cache (.cap/test-mod/) across tests
        \\
        \\Fuzz Options:
        \\  --fuzz                        Enable LLVM libFuzzer mode with AddressSanitizer
        \\  --timeout <sec>               Set per-input timeout / max execution time in seconds
        \\  --jobs <n>                    Number of parallel fuzzer workers
        \\  --repro <crash_file.bin>      Reproduce crash using saved input artifact
        \\
        \\Build Options:
        \\  -B, --force                   Force rebuild even if targets are up to date
        \\  -r, --release                 Build for all target platforms (matrix cross-compile)
        \\  -f, --fast                    Optimize for maximum execution speed (ReleaseFast)
        \\  -s, --small                   Optimize for smallest binary size (ReleaseSmall)
        \\  --safe                        Optimize with safety checks enabled (ReleaseSafe, default)
        \\  -d, --debug                   Build with full debug info and checks (Debug, -fno-llvm on x86_64-linux)
        \\  --llvm                        Force LLVM backend even in Debug mode
        \\  --opt=<debug|safe|fast|small> Specify optimization mode explicitly
        \\  -o, --out-dir=<dir>           Override output directory (default: bin or cap.json)
        \\  --cache-dir=<path>            Override Zig compiler cache directory (isolates concurrent builds)
        \\  -m, --modular                 Modular codegen + parallel .o cache & incremental link (use --low for low-mem serial)
        \\  -p, --parallel-obj            Alias for -m (parallel per-module .o cache & incremental link)
        \\  --low                         Low-resource serial mode (disable parallel .o worker pool under -m)
        \\  --emit-types                  Emit .d.cp type definitions even when building executables
        \\  --timings                     Print per-stage build timings breakdown in ms
        \\  -t, --target=<triple>         Cross-compile for specific target triple
        \\  -b, --beam                    Build dynamic library (.so) for BEAM/Nest runtime
        \\  --exe                         Produce standalone executable (default)
        \\  --lib                         Produce static library (.a)
        \\  --obj                         Produce object file (.o)
        \\
    , .{});
}

const RunOptions = struct {
    opt_mode: []const u8 = "Debug",
    is_modular: bool = false,
    parallel_obj: bool = false,
    force_rebuild: bool = false,
};

fn handleRun(allocator: std.mem.Allocator, args: []const []const u8) !bool {
    if (args.len == 0) return false;

    var run_opts = RunOptions{};
    var target_opt: ?[]const u8 = null;

    for (args) |arg| {
        if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print(
                \\Usage: cap run [options] <file.cp|script>
                \\
                \\Options:
                \\  -m, --modular       Enable modular multi-file Zig codegen (cpc compile --emit-dir)
                \\  --parallel-obj      Enable parallel per-module .o caching & incremental linking (implies -m)
                \\  -B, --force         Force rebuild even if targets are up to date
                \\
            , .{});
            return true;
        } else if (std.mem.eql(u8, arg, "-m") or std.mem.eql(u8, arg, "--modular") or std.mem.eql(u8, arg, "-p") or std.mem.eql(u8, arg, "--parallel-obj")) {
            run_opts.is_modular = true;
            run_opts.parallel_obj = true;
        } else if (std.mem.eql(u8, arg, "--low") or std.mem.eql(u8, arg, "--no-parallel")) {
            run_opts.parallel_obj = false;
        } else if (std.mem.eql(u8, arg, "-B") or std.mem.eql(u8, arg, "--force")) {
            run_opts.force_rebuild = true;
        } else if (target_opt == null) {
            target_opt = arg;
        }
    }

    const target = target_opt orelse return false;

    if (std.mem.endsWith(u8, target, ".cp")) {
        try runFileWithOpts(allocator, target, run_opts);
        return true;
    }

    const content = fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        return false;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    if (mf.scripts.get(target)) |script_cmd| {
        std.debug.print("Executing script: {s}\n", .{script_cmd});
        var count: usize = 0;
        var it_count = std.mem.tokenizeAny(u8, script_cmd, " ");
        while (it_count.next()) |_| count += 1;
        var cmd_list = try allocator.alloc([]const u8, count);
        defer allocator.free(cmd_list);
        var it = std.mem.tokenizeAny(u8, script_cmd, " ");
        var i: usize = 0;
        while (it.next()) |token| {
            cmd_list[i] = token;
            i += 1;
        }
        var child = try std.process.spawn(global_io, .{ .argv = cmd_list });
        _ = try child.wait(global_io);
        return true;
    }
    return false;
}

fn handleClean(allocator: std.mem.Allocator, args: []const []const u8) !void {
    var clean_all = false;
    for (args) |arg| {
        if (std.mem.eql(u8, arg, "--all") or std.mem.eql(u8, arg, "-a")) {
            clean_all = true;
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            std.debug.print("Usage: cap clean [--all]\n", .{});
            std.debug.print("  (default)  Clean .cap/ build caches and .cap/ws/ isolated workspaces (preserves src/ and bin/)\n", .{});
            std.debug.print("  --all      Also remove bin/ and configured output directory\n", .{});
            return;
        } else if (std.mem.startsWith(u8, arg, "-")) {
            std.debug.print("Unknown option for `cap clean`: {s}\nUsage: cap clean [--all]\n", .{arg});
            return error.InvalidArgs;
        }
    }

    std.debug.print("Cleaning build caches and intermediate artifacts...\n", .{});

    var removed_any = false;
    if (fs_shim.cwd().statFile(".cap/ws")) |_| {
        fs_shim.cwd().deleteTree(".cap/ws") catch {};
        std.debug.print("  ✓ Removed isolated build workspaces (.cap/ws/)\n", .{});
        removed_any = true;
    } else |_| {}

    if (fs_shim.cwd().statFile(".cap")) |_| {
        fs_shim.cwd().deleteTree(".cap") catch {};
        std.debug.print("  ✓ Removed build cache directory (.cap/)\n", .{});
        removed_any = true;
    } else |_| {}

    if (clean_all) {
        if (fs_shim.cwd().statFile("bin")) |_| {
            fs_shim.cwd().deleteTree("bin") catch {};
            std.debug.print("  ✓ Removed build output directory (bin/)\n", .{});
            removed_any = true;
        } else |_| {}

        if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
            defer allocator.free(content);
            if (manifest.Manifest.parse(allocator, content)) |mf_val| {
                var mf = mf_val;
                defer mf.deinit(allocator);
                if (mf.build_config) |bc| {
                    if (bc.out_dir) |od| {
                        const norm_od = trimTrailingSlashes(od);
                        if (!std.mem.eql(u8, norm_od, "bin") and
                            !std.mem.eql(u8, norm_od, "./bin") and
                            !std.mem.eql(u8, norm_od, "cap/bin") and
                            !std.mem.eql(u8, norm_od, ".") and
                            !std.mem.eql(u8, norm_od, "src") and
                            !std.mem.eql(u8, norm_od, "test"))
                        {
                            if (fs_shim.cwd().statFile(norm_od)) |_| {
                                fs_shim.cwd().deleteTree(norm_od) catch {};
                                std.debug.print("  ✓ Removed custom output directory ({s}/)\n", .{norm_od});
                                removed_any = true;
                            } else |_| {}
                        }
                    }
                }
            } else |_| {}
        } else |_| {}
    } else {
        if (fs_shim.cwd().statFile("bin")) |_| {
            std.debug.print("  • Preserved output directory (bin/); pass --all to remove\n", .{});
        } else |_| {}
    }

    if (!removed_any) {
        std.debug.print("  • Nothing to clean (already clean)\n", .{});
    }
    std.debug.print("Clean completed successfully!\n", .{});
}

fn handleAdd(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage: cap add <package> [-g] [-d|--dev]\n", .{});
        return;
    }
    var pkg_arg: []const u8 = undefined;
    var is_global = false;
    var is_dev = false;

    for (args) |arg| {
        if (std.mem.eql(u8, arg, "-g") or std.mem.eql(u8, arg, "--global")) {
            is_global = true;
        } else if (std.mem.eql(u8, arg, "-d") or std.mem.eql(u8, arg, "--dev")) {
            is_dev = true;
        } else {
            pkg_arg = arg;
        }
    }

    var pkg_name = pkg_arg;
    var version_range = try allocator.dupe(u8, "^0.1.0"); // Default

    if (std.mem.indexOfScalar(u8, pkg_arg, '@')) |idx| {
        pkg_name = pkg_arg[0..idx];
        version_range = try allocator.dupe(u8, pkg_arg[idx + 1 ..]);
    }

    if (is_global) {
        const best_ver = try resolveBestVersion(allocator, pkg_name, try semver.VersionRange.parse(version_range));
        try fetchAndLinkGlobal(allocator, pkg_name, best_ver);
        std.debug.print("Globally installed: {s}@{s}\n", .{ pkg_name, version_range });
        return;
    }

    const content = fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found. Run `cap init` first.\n", .{});
        return;
    };
    defer allocator.free(content);
    var mf = manifest.Manifest.parse(allocator, content) catch return;
    defer mf.deinit(allocator);

    try mf.addDependency(allocator, pkg_name, version_range, is_dev);
    try mf.save(allocator, "cap.json");

    if (is_dev) {
        std.debug.print("Added dev dependency: {s}@{s}\n", .{ pkg_name, version_range });
    } else {
        std.debug.print("Added dependency: {s}@{s}\n", .{ pkg_name, version_range });
    }
    try handleInstall(allocator);
}

fn handleRemove(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage: cap remove <package>\n", .{});
        return;
    }
    const pkg_name = args[0];
    const content = fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
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
        fs_shim.cwd().deleteFile(link_path) catch {};
        fs_shim.cwd().deleteTree(link_path) catch {};
        std.debug.print("Removed dependency: {s}\n", .{pkg_name});
    } else {
        std.debug.print("Package '{s}' not found in dependencies.\n", .{pkg_name});
    }
}

fn handleUpdate(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const content = fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found.\n", .{});
        return;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    const target_pkg = if (args.len > 0) args[0] else null;

    var iter = mf.dependencies.iterator();
    while (iter.next()) |entry| {
        const name = entry.key_ptr.*;
        if (target_pkg == null or std.mem.eql(u8, name, target_pkg.?)) {
            const range_text = entry.value_ptr.*;
            const range = semver.VersionRange.parse(range_text) catch continue;
            const best_ver = resolveBestVersion(allocator, name, range) catch {
                std.debug.print("Could not resolve update for {s}\n", .{name});
                continue;
            };
            try fetchAndLink(allocator, name, best_ver);
            const v_str = try best_ver.format(allocator);
            defer allocator.free(v_str);
            std.debug.print("Updated {s} -> {s}\n", .{ name, v_str });
        }
    }
}

fn handleInstall(allocator: std.mem.Allocator) !void {
    const content = fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024) catch {
        std.debug.print("Error: cap.json not found.\n", .{});
        return;
    };
    defer allocator.free(content);
    var mf = try manifest.Manifest.parse(allocator, content);
    defer mf.deinit(allocator);

    // Install production dependencies
    var iter = mf.dependencies.iterator();
    while (iter.next()) |entry| {
        const name = entry.key_ptr.*;
        const range_text = entry.value_ptr.*;
        const range = try semver.VersionRange.parse(range_text);

        const best_ver = try resolveBestVersion(allocator, name, range);
        try fetchAndLink(allocator, name, best_ver);
    }

    // Install dev dependencies
    var dev_iter = mf.dev_dependencies.iterator();
    while (dev_iter.next()) |entry| {
        const name = entry.key_ptr.*;
        const range_text = entry.value_ptr.*;
        const range = try semver.VersionRange.parse(range_text);

        const best_ver = try resolveBestVersion(allocator, name, range);
        try fetchAndLink(allocator, name, best_ver);
    }

    // Generate cap.lock
    var lk = try lock.Lockfile.fromManifest(allocator, mf);
    defer lk.deinit(allocator);
    try lk.save(allocator, "cap.lock");
    std.debug.print("Generated cap.lock\n", .{});
}

fn resolveBestVersion(_: std.mem.Allocator, name: []const u8, range: semver.VersionRange) !semver.Version {
    var reg_dir = try std.Io.Dir.openDirAbsolute(global_io, registry_path, .{ .iterate = true });
    defer reg_dir.close(global_io);
    var pkg_dir = try reg_dir.openDir(global_io, name, .{ .iterate = true });
    defer pkg_dir.close(global_io);

    var best: ?semver.Version = null;
    var it = pkg_dir.iterate();
    while (try it.next(global_io)) |entry| {
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

fn fetchAndLinkGlobal(allocator: std.mem.Allocator, name: []const u8, version: semver.Version) !void {
    const v_str = try version.format(allocator);
    defer allocator.free(v_str);

    const home = global_env.get("HOME") orelse "/root";
    defer allocator.free(home);
    const global_root = try std.Io.Dir.path.join(allocator, &[_][]const u8{ home, ".cap", "global", "cap_modules" });
    defer allocator.free(global_root);
    try fs_shim.cwd().createDirPath(global_io, global_root);

    const global_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ global_root, name });
    defer allocator.free(global_pkg_path);

    fs_shim.cwd().deleteFile(global_pkg_path) catch {};
    fs_shim.cwd().deleteTree(global_pkg_path) catch {};

    const reg_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ registry_path, name, v_str });
    defer allocator.free(reg_pkg_path);

    try copyDir(reg_pkg_path, global_pkg_path);
}

fn fetchAndLink(allocator: std.mem.Allocator, name: []const u8, version: semver.Version) !void {
    const v_str = try version.format(allocator);
    defer allocator.free(v_str);

    // 1. Mock Global Cache (~/.cap/cache)
    const home = global_env.get("HOME") orelse "/root";
    defer allocator.free(home);
    const cache_root = try std.Io.Dir.path.join(allocator, &[_][]const u8{ home, ".cap", "cache" });
    defer allocator.free(cache_root);
    try fs_shim.cwd().createDirPath(global_io, cache_root);

    const cache_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ cache_root, name, v_str });
    defer allocator.free(cache_pkg_path);

    // Make sure parent directory exists (~/.cap/cache/<name>)
    const cache_pkg_parent = std.Io.Dir.path.dirname(cache_pkg_path).?;
    {
        var root_dir = try std.Io.Dir.openDirAbsolute(global_io, "/", .{});
        defer root_dir.close(global_io);
        try root_dir.createDirPath(global_io, cache_pkg_parent[1..]);
    }

    var cache_exists = true;
    var d = std.Io.Dir.openDirAbsolute(global_io, cache_pkg_path, .{}) catch blk: {
        cache_exists = false;
        break :blk std.Io.Dir.cwd();
    };
    if (cache_exists) {
        d.close(global_io);
    } else {
        std.debug.print("Fetching {s}@{s} to cache...\n", .{ name, v_str });
        const reg_pkg_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ registry_path, name, v_str });
        defer allocator.free(reg_pkg_path);

        try copyDir(reg_pkg_path, cache_pkg_path);
    }

    // 2. Link to cap_modules/
    try fs_shim.cwd().createDirPath(global_io, "cap_modules");
    const link_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ "cap_modules", name });
    defer allocator.free(link_path);

    fs_shim.cwd().deleteFile(link_path) catch {};
    fs_shim.cwd().deleteTree(link_path) catch {};

    std.debug.print("Linking {s}@{s} -> cap_modules/{s}\n", .{ name, v_str, name });
    if (@import("builtin").os.tag == .windows) {
        try copyDir(cache_pkg_path, link_path);
    } else {
        try std.Io.Dir.symLinkAbsolute(global_io, cache_pkg_path, link_path, .{});
    }
    precompilePackageBinDeps(allocator, link_path) catch {};
}

fn copyDir(src: []const u8, dest: []const u8) !void {
    const argv = &[_][]const u8{ "cp", "-r", src, dest };
    var child = try std.process.spawn(global_io, .{ .argv = argv });
    _ = try child.wait(global_io);
}

fn getCpcBin(allocator: std.mem.Allocator) ![]const u8 {
    if (global_env.get("CPC_BIN")) |custom_bin| {
        if (custom_bin.len > 0) return try allocator.dupe(u8, custom_bin);
    }
    if (fs_shim.cwd().readFileAlloc(allocator, ".cap/cpc_bin", 512)) |content| {
        const trimmed = std.mem.trim(u8, content, " \t\r\n");
        if (trimmed.len > 0) return try allocator.dupe(u8, trimmed);
    } else |_| {}
    return try std.Io.Dir.path.join(allocator, &[_][]const u8{ cap_root, "cpc", "target", "release", "cpc" });
}

fn addArg(args_buf: [][]const u8, argc: *usize, arg: []const u8) void {
    for (args_buf[0..argc.*]) |existing| {
        if (std.mem.eql(u8, existing, arg)) return;
    }
    if (argc.* < args_buf.len) {
        args_buf[argc.*] = arg;
        argc.* += 1;
    }
}

fn hasArg(args: []const []const u8, target: []const u8) bool {
    for (args) |a| {
        if (std.mem.eql(u8, a, target)) return true;
    }
    return false;
}

fn spawnAndWaitSuccess(argv: []const []const u8) !bool {
    var child = try std.process.spawn(global_io, .{ .argv = argv });
    const res = try child.wait(global_io);
    return switch (res) {
        .exited => |code| code == 0,
        else => false,
    };
}

fn getCurrentPid() i64 {
    if (builtin.os.tag == .linux) {
        return @intCast(std.os.linux.getpid());
    }
    return std.Io.Timestamp.now(global_io, .awake).nanoseconds;
}

fn scanZigFileForLinkPragmas(
    allocator: std.mem.Allocator,
    zig_path: []const u8,
    args_buf: [][]const u8,
    argc: *usize,
    allocated_libs: *[64][]const u8,
    allocated_libs_len: *usize,
) void {
    const content = fs_shim.cwd().readFileAlloc(allocator, zig_path, 4096) catch return;
    defer allocator.free(content);

    var line_it = std.mem.splitScalar(u8, content, '\n');
    while (line_it.next()) |line| {
        const trimmed = std.mem.trim(u8, line, " \r\t");
        if (trimmed.len == 0) continue;
        if (!std.mem.startsWith(u8, trimmed, "//")) {
            // Stop scanning once comments header ends
            break;
        }
        if (std.mem.startsWith(u8, trimmed, "// @linkPath(\"")) {
            const rest = trimmed["// @linkPath(\"".len..];
            if (std.mem.indexOfScalar(u8, rest, '"')) |end_idx| {
                const path = rest[0..end_idx];
                const arg = std.fmt.allocPrint(allocator, "-L{s}", .{path}) catch continue;
                if (allocated_libs_len.* < 64) {
                    allocated_libs[allocated_libs_len.*] = arg;
                    allocated_libs_len.* += 1;
                }
                addArg(args_buf, argc, arg);
            }
        } else if (std.mem.startsWith(u8, trimmed, "// @linkLibrary(\"")) {
            const rest = trimmed["// @linkLibrary(\"".len..];
            if (std.mem.indexOfScalar(u8, rest, '"')) |end_idx| {
                const lib = rest[0..end_idx];
                const arg = std.fmt.allocPrint(allocator, "-l{s}", .{lib}) catch continue;
                if (allocated_libs_len.* < 64) {
                    allocated_libs[allocated_libs_len.*] = arg;
                    allocated_libs_len.* += 1;
                }
                addArg(args_buf, argc, arg);
            }
        }
    }
}

fn scanModularManifestForLinkPragmas(
    allocator: std.mem.Allocator,
    zig_dir: []const u8,
    args_buf: [][]const u8,
    argc: *usize,
    allocated_libs: *[64][]const u8,
    allocated_libs_len: *usize,
) void {
    const manifest_path = std.fmt.allocPrint(allocator, "{s}/.cp_modular_manifest.txt", .{zig_dir}) catch return;
    defer allocator.free(manifest_path);

    const content = fs_shim.cwd().readFileAlloc(allocator, manifest_path, 512 * 1024) catch return;
    defer allocator.free(content);

    var line_it = std.mem.splitScalar(u8, content, '\n');
    while (line_it.next()) |line| {
        const trimmed = std.mem.trim(u8, line, " \r\t");
        if (std.mem.startsWith(u8, trimmed, "linkPath=")) {
            const path = trimmed["linkPath=".len..];
            if (path.len == 0) continue;
            const arg = std.fmt.allocPrint(allocator, "-L{s}", .{path}) catch continue;
            if (allocated_libs_len.* < 64) {
                allocated_libs[allocated_libs_len.*] = arg;
                allocated_libs_len.* += 1;
            }
            addArg(args_buf, argc, arg);
        } else if (std.mem.startsWith(u8, trimmed, "linkLib=")) {
            const lib = trimmed["linkLib=".len..];
            if (lib.len == 0) continue;
            const arg = std.fmt.allocPrint(allocator, "-l{s}", .{lib}) catch continue;
            if (allocated_libs_len.* < 64) {
                allocated_libs[allocated_libs_len.*] = arg;
                allocated_libs_len.* += 1;
            }
            addArg(args_buf, argc, arg);
        }
    }
}

fn resolveSearchPath(allocator: std.mem.Allocator, rel_path: []const u8) ![]const u8 {
    if (fs_shim.cwd().statFile(rel_path) catch null != null) {
        return try allocator.dupe(u8, rel_path);
    }
    const global_bases = [_][]const u8{
        "/data/dev/cp",
        "/root/.cap",
    };
    for (global_bases) |base| {
        const full = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, rel_path });
        if (fs_shim.cwd().statFile(full) catch null != null) {
            return full;
        }
        allocator.free(full);
    }
    return try allocator.dupe(u8, rel_path);
}

fn resolveSubPath(allocator: std.mem.Allocator, base_path: ?[]const u8, sub: []const u8) ![]const u8 {
    if (base_path) |base| {
        if (std.mem.eql(u8, sub, ".") or sub.len == 0) {
            return resolveSearchPath(allocator, base);
        }
        if (!std.mem.startsWith(u8, sub, base)) {
            const combined = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ base, sub });
            defer allocator.free(combined);
            const resolved = try resolveSearchPath(allocator, combined);
            if (fs_shim.cwd().statFile(resolved) catch null != null) {
                return resolved;
            }
            allocator.free(resolved);
        }
    }
    return resolveSearchPath(allocator, sub);
}

fn loadCentralLibIndex(allocator: std.mem.Allocator) ?manifest.CentralLibIndex {
    const search_paths = [_][]const u8{
        "/root/.cap/lib/cap.json",
        "/data/dev/cp/lib/cap.json",
        "lib/cap.json",
    };
    for (search_paths) |sp| {
        if (fs_shim.cwd().readFileAlloc(allocator, sp, 1024 * 1024)) |content| {
            defer allocator.free(content);
            return manifest.CentralLibIndex.parse(allocator, content) catch null;
        } else |_| {}
    }
    return null;
}

fn scanBufferForCapImports(allocator: std.mem.Allocator, file_path: []const u8, imported_caps: *std.StringHashMap(void)) void {
    const content = fs_shim.cwd().readFileAlloc(allocator, file_path, 2 * 1024 * 1024) catch return;
    defer allocator.free(content);

    var cursor: usize = 0;
    while (std.mem.indexOfPos(u8, content, cursor, "cap:")) |idx| {
        const start = idx + 4;
        var end = start;
        while (end < content.len and (std.ascii.isAlphanumeric(content[end]) or content[end] == '_' or content[end] == '-')) : (end += 1) {}
        if (end > start) {
            const mod_name = content[start..end];
            imported_caps.put(allocator.dupe(u8, mod_name) catch continue, {}) catch {};
        }
        cursor = end;
    }
}

fn scanDirRecursivelyForCaps(allocator: std.mem.Allocator, dir_path: []const u8, imported_caps: *std.StringHashMap(void)) void {
    var dir = fs_shim.cwd().openDir(dir_path, .{ .iterate = true }) catch return;
    defer dir.close(global_io);
    var iter = dir.iterate();
    while (iter.next(global_io) catch null) |entry| {
        if (entry.kind == .directory) {
            if (!std.mem.eql(u8, entry.name, ".cap") and !std.mem.eql(u8, entry.name, "zig-cache") and !std.mem.eql(u8, entry.name, "node_modules")) {
                const sub_path = std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name }) catch continue;
                defer allocator.free(sub_path);
                scanDirRecursivelyForCaps(allocator, sub_path, imported_caps);
            }
        } else if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cp")) {
            const sub_file = std.fmt.allocPrint(allocator, "{s}/{s}", .{ dir_path, entry.name }) catch continue;
            defer allocator.free(sub_file);
            scanBufferForCapImports(allocator, sub_file, imported_caps);
        }
    }
}

fn extractPkgNameFromSlice(raw: []const u8) []const u8 {
    if (raw.len == 0) return "";
    if (raw[0] == '@') {
        if (std.mem.indexOfScalar(u8, raw, '/')) |first_slash| {
            var end = first_slash + 1;
            while (end < raw.len and raw[end] != '/' and raw[end] != '"' and raw[end] != '\'' and raw[end] != ')' and !std.ascii.isWhitespace(raw[end])) : (end += 1) {}
            if (end > first_slash + 1) return raw[0..end];
        }
        return "";
    }
    var end: usize = 0;
    while (end < raw.len and raw[end] != '/' and raw[end] != '"' and raw[end] != '\'' and raw[end] != ')' and !std.ascii.isWhitespace(raw[end])) : (end += 1) {}
    return raw[0..end];
}

fn addUsedPkg(allocator: std.mem.Allocator, used_pkgs: *std.StringHashMap(void), pkg_name: []const u8) void {
    if (pkg_name.len == 0) return;
    if (used_pkgs.contains(pkg_name)) return;
    used_pkgs.put(allocator.dupe(u8, pkg_name) catch return, {}) catch {};
}

fn scanCpImportsFallback(allocator: std.mem.Allocator, cp_path: []const u8, used_pkgs: *std.StringHashMap(void)) void {
    const content = fs_shim.cwd().readFileAlloc(allocator, cp_path, 2 * 1024 * 1024) catch return;
    defer allocator.free(content);

    var lines = std.mem.splitScalar(u8, content, '\n');
    while (lines.next()) |line| {
        const trimmed = std.mem.trim(u8, line, " \t\r");
        if (!std.mem.startsWith(u8, trimmed, "import ")) continue;
        const from_idx = std.mem.indexOf(u8, trimmed, "from ") orelse continue;
        const after_from = std.mem.trimStart(u8, trimmed[from_idx + 5 ..], " \t");
        if (after_from.len < 2) continue;
        const quote = after_from[0];
        if (quote != '"' and quote != '\'') continue;
        const rest = after_from[1..];
        const end_q = std.mem.indexOfScalar(u8, rest, quote) orelse continue;
        const spec = rest[0..end_q];
        if (spec.len == 0 or spec[0] == '.' or spec[0] == '/' or std.mem.startsWith(u8, spec, "cap:")) continue;
        const pkg = extractPkgNameFromSlice(spec);
        if (pkg.len == 0) continue;
        const mod_check = std.fmt.allocPrint(allocator, "cap_modules/{s}/cap.json", .{pkg}) catch continue;
        defer allocator.free(mod_check);
        if (native.fileExists(mod_check)) {
            addUsedPkg(allocator, used_pkgs, pkg);
        }
    }
}

/// On-Demand Link Tree-Shaking scanner:
/// 1. Primary: scans `// @cap-used-pkg: <pkg>` comments emitted at the top of generated `.zig` files.
/// 2. Fallback (only when no `@cap-used-pkg` pragma is present): scans `@import("...cap_modules/<pkg>/...")`
///    references and original `.cp` `import ... from "<pkg>"` statements.
/// Never blindly links unused packages in `cap_modules/`.
fn scanUsedPackagesFromFile(allocator: std.mem.Allocator, file_path: []const u8, used_pkgs: *std.StringHashMap(void)) void {
    const content = fs_shim.cwd().readFileAlloc(allocator, file_path, 4 * 1024 * 1024) catch return;
    defer allocator.free(content);

    // 1. Primary pass: scan `// @cap-used-pkg: <pkg>` annotations
    var lines = std.mem.splitScalar(u8, content, '\n');
    while (lines.next()) |line| {
        const trimmed = std.mem.trim(u8, line, " \t\r");
        if (std.mem.startsWith(u8, trimmed, "//")) {
            const after_slashes = std.mem.trimStart(u8, trimmed[2..], " \t");
            if (std.mem.startsWith(u8, after_slashes, "@cap-used-pkg:")) {
                const raw_pkg = std.mem.trim(u8, after_slashes["@cap-used-pkg:".len..], " \t\r");
                const pkg = extractPkgNameFromSlice(raw_pkg);
                addUsedPkg(allocator, used_pkgs, pkg);
            }
        }
    }
    if (used_pkgs.count() > 0) return;

    // 2. Fallback pass: scan `@import` / path references containing `cap_modules/`
    var cursor: usize = 0;
    while (std.mem.indexOfPos(u8, content, cursor, "cap_modules/")) |idx| {
        const start = idx + "cap_modules/".len;
        if (start < content.len) {
            const pkg = extractPkgNameFromSlice(content[start..]);
            addUsedPkg(allocator, used_pkgs, pkg);
        }
        cursor = start;
    }

    // 3. Fallback pass for `.cp` source imports (when file_path is `.cp` or mapped from `.cap/zig/*.zig`)
    if (std.mem.endsWith(u8, file_path, ".cp")) {
        scanCpImportsFallback(allocator, file_path, used_pkgs);
    } else if (std.mem.startsWith(u8, file_path, ".cap/zig/") and std.mem.endsWith(u8, file_path, ".zig")) {
        const rel_stem = file_path[".cap/zig/".len .. file_path.len - 4];
        const cp_path = std.fmt.allocPrint(allocator, "{s}.cp", .{rel_stem}) catch return;
        defer allocator.free(cp_path);
        scanCpImportsFallback(allocator, cp_path, used_pkgs);
    } else if (std.mem.startsWith(u8, file_path, ".cap/ws/") and std.mem.endsWith(u8, file_path, ".zig")) {
        if (std.mem.indexOf(u8, file_path, "/zig/")) |zig_idx| {
            const rel_stem = file_path[zig_idx + "/zig/".len .. file_path.len - 4];
            const cp_path = std.fmt.allocPrint(allocator, "{s}.cp", .{rel_stem}) catch return;
            defer allocator.free(cp_path);
            scanCpImportsFallback(allocator, cp_path, used_pkgs);
        }
    }
}

fn scanSourceForCapImports(allocator: std.mem.Allocator, file_path: []const u8, imported_caps: *std.StringHashMap(void)) void {
    scanBufferForCapImports(allocator, file_path, imported_caps);

    if (std.mem.startsWith(u8, file_path, ".cap/zig/") and std.mem.endsWith(u8, file_path, ".zig")) {
        const rel_stem = file_path[".cap/zig/".len .. file_path.len - 4];
        const cp_path = std.fmt.allocPrint(allocator, "{s}.cp", .{rel_stem}) catch return;
        defer allocator.free(cp_path);
        scanBufferForCapImports(allocator, cp_path, imported_caps);
    } else if (std.mem.startsWith(u8, file_path, ".cap/ws/") and std.mem.endsWith(u8, file_path, ".zig")) {
        if (std.mem.indexOf(u8, file_path, "/zig/")) |zig_idx| {
            const rel_stem = file_path[zig_idx + "/zig/".len .. file_path.len - 4];
            const cp_path = std.fmt.allocPrint(allocator, "{s}.cp", .{rel_stem}) catch return;
            defer allocator.free(cp_path);
            scanBufferForCapImports(allocator, cp_path, imported_caps);
        }
    }

    if (fs_shim.cwd().statFile("src") catch null != null) {
        scanDirRecursivelyForCaps(allocator, "src", imported_caps);
    }
}

fn buildCWithZig(
    allocator: std.mem.Allocator,
    name: []const u8,
    cfg: manifest.BinDependencyConfig,
    target_triple: []const u8,
    opt_mode: []const u8,
    out_dir: []const u8,
    cache_dir: []const u8,
) !?[]const u8 {
    if (cfg.sources.len == 0 and cfg.build_cmd == null) return null;

    fs_shim.cwd().createDirPath(global_io, out_dir) catch {};

    const out_lib_path = try std.fmt.allocPrint(allocator, "{s}/lib{s}.a", .{ out_dir, name });
    defer allocator.free(out_lib_path);

    if (cfg.sources.len > 0) {
        std.debug.print("  → [zig cc] Compiling {s} sources into {s}...\n", .{ name, out_lib_path });
        var zig_cmd_buf: [128][]const u8 = undefined;
        var argc: usize = 0;
        zig_cmd_buf[argc] = "zig";
        argc += 1;
        zig_cmd_buf[argc] = "build-lib";
        argc += 1;

        var allocated_source_paths: [32][]const u8 = undefined;
        var allocated_source_count: usize = 0;
        defer for (allocated_source_paths[0..allocated_source_count]) |p| allocator.free(p);

        for (cfg.sources) |src| {
            if (argc < 120) {
                const resolved_src = try resolveSubPath(allocator, cfg.path, src);
                if (allocated_source_count < 32) {
                    allocated_source_paths[allocated_source_count] = resolved_src;
                    allocated_source_count += 1;
                }
                zig_cmd_buf[argc] = resolved_src;
                argc += 1;
            }
        }

        zig_cmd_buf[argc] = "-lc";
        argc += 1;
        if (native.binHasCppSources(cfg)) {
            zig_cmd_buf[argc] = "-lc++";
            argc += 1;
        }

        const emit_arg = try std.fmt.allocPrint(allocator, "-femit-bin={s}", .{out_lib_path});
        defer allocator.free(emit_arg);
        zig_cmd_buf[argc] = emit_arg;
        argc += 1;

        zig_cmd_buf[argc] = "--cache-dir";
        argc += 1;
        zig_cmd_buf[argc] = cache_dir;
        argc += 1;

        const resolved_opt = if (cfg.opt) |o| o.toZigFlag() else opt_mode;
        zig_cmd_buf[argc] = "-O";
        argc += 1;
        zig_cmd_buf[argc] = resolved_opt;
        argc += 1;

        if (!std.mem.eql(u8, target_triple, "native")) {
            zig_cmd_buf[argc] = "-target";
            argc += 1;
            zig_cmd_buf[argc] = target_triple;
            argc += 1;
        }

        var allocated_inc_paths: [16][]const u8 = undefined;
        var allocated_inc_count: usize = 0;
        defer for (allocated_inc_paths[0..allocated_inc_count]) |p| allocator.free(p);

        if (cfg.include_dirs.len == 0 and cfg.path != null) {
            // Default include path to cfg.path if not specified
            const resolved_base = try resolveSearchPath(allocator, cfg.path.?);
            const i_arg = try std.fmt.allocPrint(allocator, "-I{s}", .{resolved_base});
            allocator.free(resolved_base);
            if (allocated_inc_count < 16) {
                allocated_inc_paths[allocated_inc_count] = i_arg;
                allocated_inc_count += 1;
            }
            zig_cmd_buf[argc] = i_arg;
            argc += 1;
        } else {
            for (cfg.include_dirs) |inc| {
                if (argc < 120) {
                    const resolved_inc = try resolveSubPath(allocator, cfg.path, inc);
                    const i_arg = try std.fmt.allocPrint(allocator, "-I{s}", .{resolved_inc});
                    allocator.free(resolved_inc);
                    if (allocated_inc_count < 16) {
                        allocated_inc_paths[allocated_inc_count] = i_arg;
                        allocated_inc_count += 1;
                    }
                    zig_cmd_buf[argc] = i_arg;
                    argc += 1;
                }
            }
        }

        for (cfg.c_flags) |cf| {
            if (std.mem.startsWith(u8, cf, "-O")) continue;
            if (argc < 120) {
                zig_cmd_buf[argc] = cf;
                argc += 1;
            }
        }

        var child = try std.process.spawn(global_io, .{ .argv = zig_cmd_buf[0..argc] });
        const res = try child.wait(global_io);
        if (res.exited == 0) {
            std.debug.print("  ✓ Compiled {s} -> {s}\n", .{ name, out_lib_path });
            return try allocator.dupe(u8, out_dir);
        } else {
            std.debug.print("  ✗ Failed to compile {s} with zig build-lib (exit code {d})\n", .{ name, res.exited });
        }
    } else if (cfg.build_cmd) |cmd| {
        std.debug.print("  → Executing build_cmd for {s}: {s}\n", .{ name, cmd });
        const argv = &[_][]const u8{ "sh", "-c", cmd };
        var child = try std.process.spawn(global_io, .{ .argv = argv });
        const res = try child.wait(global_io);
        if (res.exited == 0) {
            return try allocator.dupe(u8, out_dir);
        }
    }

    return null;
}

fn resolveBinDependency(
    allocator: std.mem.Allocator,
    name: []const u8,
    cfg: manifest.BinDependencyConfig,
    target_triple: []const u8,
    opt_mode: []const u8,
    cache_dir: []const u8,
) !?[]const u8 {
    const base_path = cfg.path orelse "";

    // 1. Candidate paths to look for existing static library
    var candidates: [24][]const u8 = undefined;
    var cand_count: usize = 0;
    defer for (candidates[0..cand_count]) |c| allocator.free(c);

    if (base_path.len > 0) {
        if (!std.mem.eql(u8, target_triple, "native") and cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "{s}/bin/{s}/lib{s}.a", .{ base_path, target_triple, name });
            cand_count += 1;
        }
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "{s}/bin/lib{s}.a", .{ base_path, name });
            cand_count += 1;
        }
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "{s}/build/lib{s}.a", .{ base_path, name });
            cand_count += 1;
        }
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "{s}/lib{s}.a", .{ base_path, name });
            cand_count += 1;
        }
        if (std.mem.eql(u8, name, "quickjs") or std.mem.eql(u8, name, "qjs")) {
            if (cand_count < 24) {
                candidates[cand_count] = try std.fmt.allocPrint(allocator, "{s}/build/libqjs.a", .{base_path});
                cand_count += 1;
            }
            if (cand_count < 24) {
                candidates[cand_count] = try std.fmt.allocPrint(allocator, "{s}/libqjs.a", .{base_path});
                cand_count += 1;
            }
        }
    }
    if (cand_count < 24) {
        candidates[cand_count] = try std.fmt.allocPrint(allocator, "cap_modules/{s}/bin/lib{s}.a", .{ name, name });
        cand_count += 1;
    }
    if (!std.mem.eql(u8, target_triple, "native")) {
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "bin/{s}/lib{s}.a", .{ target_triple, name });
            cand_count += 1;
        }
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, ".cap/lib/{s}/lib{s}.a", .{ target_triple, name });
            cand_count += 1;
        }
    }
    if (cand_count < 24) {
        candidates[cand_count] = try std.fmt.allocPrint(allocator, "bin/lib{s}.a", .{name});
        cand_count += 1;
    }
    if (cand_count < 24) {
        candidates[cand_count] = try std.fmt.allocPrint(allocator, ".cap/lib/lib{s}.a", .{name});
        cand_count += 1;
    }

    // Special global locations (e.g. in /data/dev/cp/)
    if (base_path.len > 0) {
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "/data/dev/cp/{s}/bin/lib{s}.a", .{ base_path, name });
            cand_count += 1;
        }
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "/data/dev/cp/{s}/build/lib{s}.a", .{ base_path, name });
            cand_count += 1;
        }
        if (cand_count < 24) {
            candidates[cand_count] = try std.fmt.allocPrint(allocator, "/data/dev/cp/{s}/lib{s}.a", .{ base_path, name });
            cand_count += 1;
        }
    }

    // Check if already exists
    for (candidates[0..cand_count]) |cand| {
        if (fs_shim.cwd().statFile(cand) catch null != null) {
            return try allocator.dupe(u8, cand);
        }
    }

    // 2. If not found, use zig to compile sources!
    const out_dir = if (base_path.len > 0)
        try std.fmt.allocPrint(allocator, "{s}/bin", .{base_path})
    else if (!std.mem.eql(u8, target_triple, "native"))
        try std.fmt.allocPrint(allocator, ".cap/lib/{s}", .{target_triple})
    else
        try allocator.dupe(u8, ".cap/lib");
    defer allocator.free(out_dir);

    if (try buildCWithZig(allocator, name, cfg, target_triple, opt_mode, out_dir, cache_dir)) |compiled_dir| {
        allocator.free(compiled_dir);
        return try std.fmt.allocPrint(allocator, "{s}/lib{s}.a", .{ out_dir, name });
    }
    return null;
}

fn precompilePackageBinDeps(allocator: std.mem.Allocator, pkg_path: []const u8) !void {
    const manifest_path = try std.fmt.allocPrint(allocator, "{s}/cap.json", .{pkg_path});
    defer allocator.free(manifest_path);

    const content = fs_shim.cwd().readFileAlloc(allocator, manifest_path, 1024 * 1024) catch return;
    defer allocator.free(content);

    var mf = manifest.Manifest.parse(allocator, content) catch return;
    defer mf.deinit(allocator);

    var iter = mf.bin_dependencies.iterator();
    while (iter.next()) |entry| {
        const bin_name = entry.key_ptr.*;
        const bin_cfg = entry.value_ptr.*;
        if (bin_cfg.sources.len > 0) {
            const bin_out_dir = try std.fmt.allocPrint(allocator, "{s}/bin", .{pkg_path});
            defer allocator.free(bin_out_dir);
            _ = try buildCWithZig(allocator, bin_name, bin_cfg, "native", "ReleaseFast", bin_out_dir, ".cap/zig-cache");
            std.debug.print("  ✓ Pre-compiled static lib {s}/bin/lib{s}.a\n", .{ pkg_path, bin_name });
        }
    }
}

fn appendLinkArgs(
    allocator: std.mem.Allocator,
    args_buf: [][]const u8,
    argc: *usize,
    allocated_libs: *[64][]const u8,
    allocated_libs_len: *usize,
    mf_opt: ?manifest.Manifest,
    target_triple: []const u8,
    opt_mode: []const u8,
    scan_file: ?[]const u8,
    cache_dir: []const u8,
) !void {
    // 1. Process bin_dependencies from project cap.json
    if (mf_opt) |mf| {
        var bd_iter = mf.bin_dependencies.iterator();
        while (bd_iter.next()) |entry| {
            const dep_name = entry.key_ptr.*;
            const dep_cfg = entry.value_ptr.*;
            if (std.mem.eql(u8, dep_name, "quickjs") or std.mem.eql(u8, dep_name, "qjs")) {
                if (resolveBinDependency(allocator, "qjs", dep_cfg, target_triple, opt_mode, cache_dir)) |path_opt| {
                    if (path_opt) |path| {
                        if (std.mem.endsWith(u8, path, ".a")) {
                            const llib = try allocator.dupe(u8, path);
                            if (allocated_libs_len.* < 64) {
                                allocated_libs[allocated_libs_len.*] = llib;
                                allocated_libs_len.* += 1;
                            }
                            addArg(args_buf, argc, llib);
                            addArg(args_buf, argc, "-lm");
                        } else {
                            const ldir = try std.fmt.allocPrint(allocator, "-L{s}", .{path});
                            if (allocated_libs_len.* < 64) {
                                allocated_libs[allocated_libs_len.*] = ldir;
                                allocated_libs_len.* += 1;
                            }
                            addArg(args_buf, argc, ldir);
                            addArg(args_buf, argc, "-lqjs");
                            addArg(args_buf, argc, "-lm");
                        }
                        allocator.free(path);
                    }
                } else |_| {}
            } else {
                if (resolveBinDependency(allocator, dep_name, dep_cfg, target_triple, opt_mode, cache_dir)) |path_opt| {
                    if (path_opt) |path| {
                        if (std.mem.endsWith(u8, path, ".a")) {
                            const llib = try allocator.dupe(u8, path);
                            if (allocated_libs_len.* < 64) {
                                allocated_libs[allocated_libs_len.*] = llib;
                                allocated_libs_len.* += 1;
                            }
                            addArg(args_buf, argc, llib);
                        } else {
                            const ldir = try std.fmt.allocPrint(allocator, "-L{s}", .{path});
                            if (allocated_libs_len.* < 64) {
                                allocated_libs[allocated_libs_len.*] = ldir;
                                allocated_libs_len.* += 1;
                            }
                            addArg(args_buf, argc, ldir);

                            const llib = try std.fmt.allocPrint(allocator, "-l{s}", .{dep_name});
                            if (allocated_libs_len.* < 64) {
                                allocated_libs[allocated_libs_len.*] = llib;
                                allocated_libs_len.* += 1;
                            }
                            addArg(args_buf, argc, llib);
                        }
                        allocator.free(path);
                    }
                } else |_| {}
            }
            if (native.binHasCppSources(dep_cfg)) {
                addArg(args_buf, argc, "-lc++");
            }
        }

        // 1.5 Process top-level `sys` dynamic/system libraries from project cap.json (with compile-time verification)
        if (mf.sys_dependencies) |sys_cfg| {
            var sys_set = native.NativeLinkSet.init();
            defer sys_set.deinit(allocator);
            const target_key = native.zigTripleToTargetKey(target_triple);
            try native.verifyAndResolveSysDeps(allocator, ".", sys_cfg, target_key, target_triple, &sys_set);
            for (sys_set.link_libs.items) |flag| {
                const flag_dup = try allocator.dupe(u8, flag);
                if (allocated_libs_len.* < allocated_libs.len) {
                    allocated_libs[allocated_libs_len.*] = flag_dup;
                    allocated_libs_len.* += 1;
                }
                addArg(args_buf, argc, flag_dup);
            }
            if (!std.mem.startsWith(u8, target_key, "windows")) {
                for (sys_set.runtime_lib_dirs.items) |rdir| {
                    const rdir_dup = try allocator.dupe(u8, rdir);
                    if (allocated_libs_len.* < allocated_libs.len) {
                        allocated_libs[allocated_libs_len.*] = rdir_dup;
                        allocated_libs_len.* += 1;
                    }
                    addArg(args_buf, argc, "-rpath");
                    addArg(args_buf, argc, rdir_dup);
                }
            }
        }

        // 2. Process c_flags and link_libs from build_config
        if (mf.build_config) |bc| {
            for (bc.c_flags) |cf| {
                addArg(args_buf, argc, cf);
            }
            for (bc.link_libs) |ll| {
                if (std.mem.startsWith(u8, ll, "-l") or std.mem.startsWith(u8, ll, "-L")) {
                    addArg(args_buf, argc, ll);
                } else {
                    const lib_arg = try std.fmt.allocPrint(allocator, "-l{s}", .{ll});
                    if (allocated_libs_len.* < 64) {
                        allocated_libs[allocated_libs_len.*] = lib_arg;
                        allocated_libs_len.* += 1;
                    }
                    addArg(args_buf, argc, lib_arg);
                }
            }
            if (bc.sys) |s| {
                if (std.json.parseFromSlice(std.json.Value, allocator, s, .{})) |parsed| {
                    defer parsed.deinit();
                    if (manifest.SysDependencyConfig.parseValue(allocator, parsed.value)) |sys_cfg| {
                        defer @constCast(&sys_cfg).deinit(allocator);
                        if (sys_cfg.getAllLibFlagsForTarget(target_triple, allocator) catch null) |flags| {
                            defer allocator.free(flags);
                            for (flags) |flag| {
                                if (allocated_libs_len.* < 64) {
                                    allocated_libs[allocated_libs_len.*] = flag;
                                    allocated_libs_len.* += 1;
                                }
                                addArg(args_buf, argc, flag);
                            }
                        }
                    } else |_| {}
                } else |_| {}
            }
        }
    }

    // 3. CentralLibIndex (lib/cap.json) on-demand resolution
    var central_index_opt = loadCentralLibIndex(allocator);
    defer if (central_index_opt) |*ci| ci.deinit(allocator);

    if (central_index_opt) |*central_index| {
        var imported_caps = std.StringHashMap(void).init(allocator);
        defer {
            var it = imported_caps.iterator();
            while (it.next()) |entry| allocator.free(entry.key_ptr.*);
            imported_caps.deinit();
        }

        if (scan_file) |sf| {
            scanSourceForCapImports(allocator, sf, &imported_caps);
        }

        var it = imported_caps.iterator();
        while (it.next()) |entry| {
            const mod_name = entry.key_ptr.*;
            if (central_index.libs.get(mod_name)) |cent_entry| {
                // Bin dependencies for this module
                var b_iter = cent_entry.bin.iterator();
                while (b_iter.next()) |b_entry| {
                    const bin_name = b_entry.key_ptr.*;
                    const bin_cfg = b_entry.value_ptr.*;
                    if (resolveBinDependency(allocator, bin_name, bin_cfg, target_triple, opt_mode, cache_dir)) |path_opt| {
                        if (path_opt) |path| {
                            if (std.mem.endsWith(u8, path, ".a")) {
                                const llib = try allocator.dupe(u8, path);
                                if (allocated_libs_len.* < 64) {
                                    allocated_libs[allocated_libs_len.*] = llib;
                                    allocated_libs_len.* += 1;
                                }
                                addArg(args_buf, argc, llib);
                            } else {
                                const ldir = try std.fmt.allocPrint(allocator, "-L{s}", .{path});
                                if (allocated_libs_len.* < 64) {
                                    allocated_libs[allocated_libs_len.*] = ldir;
                                    allocated_libs_len.* += 1;
                                }
                                addArg(args_buf, argc, ldir);

                                const llib = try std.fmt.allocPrint(allocator, "-l{s}", .{bin_name});
                                if (allocated_libs_len.* < 64) {
                                    allocated_libs[allocated_libs_len.*] = llib;
                                    allocated_libs_len.* += 1;
                                }
                                addArg(args_buf, argc, llib);
                            }
                            allocator.free(path);
                        }
                    } else |_| {}
                }

                // Sys dependencies for this module
                if (cent_entry.sys) |sys_cfg| {
                    if (sys_cfg.getAllLibFlagsForTarget(target_triple, allocator) catch null) |flags| {
                        defer allocator.free(flags);
                        for (flags) |flag| {
                            if (allocated_libs_len.* < 64) {
                                allocated_libs[allocated_libs_len.*] = flag;
                                allocated_libs_len.* += 1;
                            }
                            addArg(args_buf, argc, flag);
                        }
                    }
                }
            }
        }
    }

    // 3.5 On-Demand Link Tree-Shaking for cap_modules/ packages (only links referenced packages + transitive deps)
    if (scan_file) |sf| {
        var used_pkgs = std.StringHashMap(void).init(allocator);
        defer {
            var uit = used_pkgs.iterator();
            while (uit.next()) |e| allocator.free(e.key_ptr.*);
            used_pkgs.deinit();
        }
        scanUsedPackagesFromFile(allocator, sf, &used_pkgs);

        if (used_pkgs.count() > 0) {
            var native_deps = try native.collectUsedPackageDepsForTarget(allocator, ".", &used_pkgs, target_triple);
            defer native_deps.deinit(allocator);
            for (native_deps.include_dirs.items) |inc_dir| {
                const inc_arg = try std.fmt.allocPrint(allocator, "-I{s}", .{inc_dir});
                if (allocated_libs_len.* < allocated_libs.len) {
                    allocated_libs[allocated_libs_len.*] = inc_arg;
                    allocated_libs_len.* += 1;
                }
                addArg(args_buf, argc, inc_arg);
            }
            for (native_deps.static_libs.items) |static_lib| {
                const lib_dup = try allocator.dupe(u8, static_lib);
                if (allocated_libs_len.* < allocated_libs.len) {
                    allocated_libs[allocated_libs_len.*] = lib_dup;
                    allocated_libs_len.* += 1;
                }
                addArg(args_buf, argc, lib_dup);
            }
            for (native_deps.link_libs.items) |ll| {
                const l_arg = if (std.mem.startsWith(u8, ll, "-l") or std.mem.startsWith(u8, ll, "-L") or std.mem.startsWith(u8, ll, "-Wl,"))
                    try allocator.dupe(u8, ll)
                else
                    try std.fmt.allocPrint(allocator, "-l{s}", .{ll});
                if (allocated_libs_len.* < allocated_libs.len) {
                    allocated_libs[allocated_libs_len.*] = l_arg;
                    allocated_libs_len.* += 1;
                }
                addArg(args_buf, argc, l_arg);
            }
            if (native_deps.static_libs.items.len > 0) {
                addArg(args_buf, argc, "-lc");
            }
            if (!std.mem.startsWith(u8, native.zigTripleToTargetKey(target_triple), "windows")) {
                for (native_deps.runtime_lib_dirs.items) |rdir| {
                    // Use single-arg -rpath=<dir> (or push -rpath without deduping the flag token)
                    // so multiple prebuilt dynamic packages each preserve their own runtime search dir.
                    if (argc.* + 2 < args_buf.len) {
                        const rdir_dup = try allocator.dupe(u8, rdir);
                        if (allocated_libs_len.* < allocated_libs.len) {
                            allocated_libs[allocated_libs_len.*] = rdir_dup;
                            allocated_libs_len.* += 1;
                        }
                        args_buf[argc.*] = "-rpath";
                        argc.* += 1;
                        args_buf[argc.*] = rdir_dup;
                        argc.* += 1;
                    }
                }
            }
            if (native_deps.has_cpp) {
                addArg(args_buf, argc, "-lc++");
            }
        }
    }

    // 4. Fallback auto-detection for uSockets / QuickJS if not already linked
    if (fs_shim.cwd().statFile("zig-out/lib/libusockets.a") catch null != null) {
        addArg(args_buf, argc, "-Lzig-out/lib");
        addArg(args_buf, argc, "-lusockets");
    }
    if (fs_shim.cwd().statFile("vendor/quickjs/build/libqjs.a") catch null != null) {
        addArg(args_buf, argc, "-Lvendor/quickjs/build");
        addArg(args_buf, argc, "-lqjs");
        addArg(args_buf, argc, "-lm");
    } else if (fs_shim.cwd().statFile("vendor/quickjs/libqjs.a") catch null != null) {
        addArg(args_buf, argc, "-Lvendor/quickjs");
        addArg(args_buf, argc, "-lqjs");
        addArg(args_buf, argc, "-lm");
    }
}

fn spawnRunBinary(allocator: std.mem.Allocator, run_path: []const u8, mf_opt: ?manifest.Manifest, scan_file: ?[]const u8) !void {
    var runtime_set = native.NativeLinkSet.init();
    defer runtime_set.deinit(allocator);
    const host_key = native.hostTargetKey();

    if (mf_opt) |mf| {
        if (mf.sys_dependencies) |sys_cfg| {
            try native.verifyAndResolveSysDeps(allocator, ".", sys_cfg, host_key, "native", &runtime_set);
        }
    }
    if (scan_file) |sf| {
        var used_pkgs = std.StringHashMap(void).init(allocator);
        defer {
            var uit = used_pkgs.iterator();
            while (uit.next()) |e| allocator.free(e.key_ptr.*);
            used_pkgs.deinit();
        }
        scanUsedPackagesFromFile(allocator, sf, &used_pkgs);
        if (used_pkgs.count() > 0) {
            var pkg_deps = try native.collectUsedPackageDepsForTarget(allocator, ".", &used_pkgs, "native");
            defer pkg_deps.deinit(allocator);
            for (pkg_deps.runtime_lib_dirs.items) |d| {
                native.NativeLinkSet.appendUnique(&runtime_set.runtime_lib_dirs, allocator, try allocator.dupe(u8, d));
            }
            for (pkg_deps.runtime_lib_files.items) |f| {
                native.NativeLinkSet.appendUnique(&runtime_set.runtime_lib_files, allocator, try allocator.dupe(u8, f));
            }
        }
    }

    // Verify all bundled runtime dynamic library files exist before executing
    for (runtime_set.runtime_lib_files.items) |lib_file| {
        if (!native.fileExists(lib_file)) {
            std.debug.print("error[cap:sys]: runtime dynamic library missing: {s}\n", .{lib_file});
            return error.MissingRuntimeSysLibrary;
        }
    }

    std.debug.print("Executing: {s}...\n", .{run_path});
    if (runtime_set.runtime_lib_dirs.items.len > 0) {
        var env_map = try global_env.clone(allocator);
        defer env_map.deinit();
        const env_key = if (builtin.os.tag == .macos) "DYLD_LIBRARY_PATH" else "LD_LIBRARY_PATH";
        var joined: std.ArrayList(u8) = .empty;
        defer joined.deinit(allocator);
        for (runtime_set.runtime_lib_dirs.items, 0..) |dir_p, idx| {
            if (idx > 0) try joined.append(allocator, ':');
            try joined.appendSlice(allocator, dir_p);
        }
        if (env_map.get(env_key)) |existing| {
            if (existing.len > 0) {
                try joined.append(allocator, ':');
                try joined.appendSlice(allocator, existing);
            }
        }
        try env_map.put(env_key, joined.items);
        var run_child = try std.process.spawn(global_io, .{ .argv = &[_][]const u8{run_path}, .environ_map = &env_map });
        const run_res = try run_child.wait(global_io);
        switch (run_res) {
            .exited => |code| if (code != 0) return error.RunFailed,
            else => return error.RunFailed,
        }
        return;
    }

    var run_child = try std.process.spawn(global_io, .{ .argv = &[_][]const u8{run_path} });
    const run_res = try run_child.wait(global_io);
    switch (run_res) {
        .exited => |code| if (code != 0) return error.RunFailed,
        else => return error.RunFailed,
    }
}

fn runFile(allocator: std.mem.Allocator, filename: []const u8) !void {
    try runFileWithOpts(allocator, filename, .{ .opt_mode = "Debug" });
}

fn runFileWithOpt(allocator: std.mem.Allocator, filename: []const u8, opt_mode: []const u8) !void {
    try runFileWithOpts(allocator, filename, .{ .opt_mode = opt_mode });
}

fn runFileWithOpts(allocator: std.mem.Allocator, filename: []const u8, opts: RunOptions) !void {
    const opt_mode = opts.opt_mode;
    const full_base_name = filename[0 .. filename.len - 3];
    var bin_subpath: []const u8 = full_base_name;
    if (std.mem.startsWith(u8, full_base_name, "src/")) {
        bin_subpath = full_base_name[4..];
    }
    const run_path = try std.fmt.allocPrint(allocator, "bin/{s}", .{bin_subpath});
    defer allocator.free(run_path);

    const src_stat = fs_shim.cwd().statFile(filename) catch null;
    const bin_stat = fs_shim.cwd().statFile(run_path) catch null;

    if (!opts.force_rebuild and !opts.is_modular and src_stat != null and bin_stat != null and bin_stat.?.mtime.nanoseconds >= src_stat.?.mtime.nanoseconds) {
        std.debug.print("[Up-to-date] bin/{s} (skipped compile)\n", .{bin_subpath});
        var cached_mf: ?manifest.Manifest = null;
        if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
            defer allocator.free(content);
            if (manifest.Manifest.parse(allocator, content)) |mf| {
                cached_mf = mf;
            } else |_| {}
        } else |_| {}
        defer if (cached_mf) |*m| m.deinit(allocator);
        const cached_zig = try std.fmt.allocPrint(allocator, ".cap/zig/{s}.zig", .{full_base_name});
        defer allocator.free(cached_zig);
        const scan_p: []const u8 = if (native.fileExists(cached_zig)) cached_zig else filename;
        try spawnRunBinary(allocator, run_path, cached_mf, scan_p);
        return;
    }

    if (opts.is_modular and opts.parallel_obj) {
        std.debug.print("Compiling: {s} (Mode: {s}, modular + parallel-obj)...\n", .{ filename, opt_mode });
    } else if (opts.is_modular) {
        std.debug.print("Compiling: {s} (Mode: {s}, modular)...\n", .{ filename, opt_mode });
    } else {
        std.debug.print("Compiling: {s} (Mode: {s})...\n", .{ filename, opt_mode });
    }
    const comp_start = std.Io.Timestamp.now(global_io, .awake);

    var mod_zig_dir_opt: ?[]u8 = null;
    defer if (mod_zig_dir_opt) |d| allocator.free(d);
    var zig_file_buf: ?[]const u8 = null;
    defer if (zig_file_buf) |zf| allocator.free(zf);

    const base_with_ext = std.Io.Dir.path.basename(filename);
    const entry_stem = if (std.mem.endsWith(u8, base_with_ext, ".cp"))
        base_with_ext[0 .. base_with_ext.len - 3]
    else
        base_with_ext;

    if (opts.is_modular) {
        const mod_zig_dir = try std.fmt.allocPrint(allocator, ".cap/run-mod/{s}/zig", .{bin_subpath});
        mod_zig_dir_opt = mod_zig_dir;
        fs_shim.cwd().createDirPath(global_io, mod_zig_dir) catch {};

        const cpc_bin = try getCpcBin(allocator);
        defer allocator.free(cpc_bin);
        const cpc_start = std.Io.Timestamp.now(global_io, .awake);
        var cpc_args_buf: [20][]const u8 = undefined;
        var cpc_argc: usize = 0;
        cpc_args_buf[cpc_argc] = cpc_bin;
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "compile";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "--emit-dir";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = mod_zig_dir;
        cpc_argc += 1;
        if (opts.force_rebuild) {
            cpc_args_buf[cpc_argc] = "--force";
            cpc_argc += 1;
        }
        cpc_args_buf[cpc_argc] = filename;
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "-I";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "cap_modules";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "-I";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "/data/dev/cp/cap/cap_modules";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "-I";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "/data/dev/cp/lib";
        cpc_argc += 1;

        var compile_child = try std.process.spawn(global_io, .{ .argv = cpc_args_buf[0..cpc_argc] });
        const compile_res = try compile_child.wait(global_io);
        switch (compile_res) {
            .exited => |code| if (code != 0) return error.CompileFailed,
            else => return error.CompileFailed,
        }

        zig_file_buf = try resolveModularEntryFile(allocator, mod_zig_dir, entry_stem);
        if (zig_file_buf == null) {
            std.debug.print("Error: --modular compilation did not produce {s}/__cp_entry.zig or {s}/{s}.zig\n", .{ mod_zig_dir, mod_zig_dir, entry_stem });
            return error.CompileFailed;
        }
        const cpc_ms = cpc_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toMilliseconds();
        std.debug.print("  → [cpc] Transpiled in {d}ms\n", .{cpc_ms});
    } else {
        zig_file_buf = try std.fmt.allocPrint(allocator, ".cap/zig/{s}.zig", .{full_base_name});
        const zf = zig_file_buf.?;
        if (std.Io.Dir.path.dirname(zf)) |dirname| {
            fs_shim.cwd().createDirPath(global_io, dirname) catch {};
        }

        const zig_stat = fs_shim.cwd().statFile(zf) catch null;
        if (!opts.force_rebuild and zig_stat != null and src_stat != null and zig_stat.?.mtime.nanoseconds >= src_stat.?.mtime.nanoseconds) {
            std.debug.print("  → [cpc] Intermediate Zig source is up to date (skipped)\n", .{});
        } else {
            const cpc_bin = try getCpcBin(allocator);
            defer allocator.free(cpc_bin);
            const cpc_start = std.Io.Timestamp.now(global_io, .awake);
            var cpc_args_buf: [15][]const u8 = undefined;
            var cpc_argc: usize = 0;
            cpc_args_buf[cpc_argc] = cpc_bin;
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "compile";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = filename;
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "--output";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = zf;
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "-I";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "cap_modules";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "-I";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "/data/dev/cp/cap/cap_modules";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "-I";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "/data/dev/cp/lib";
            cpc_argc += 1;

            var compile_child = try std.process.spawn(global_io, .{ .argv = cpc_args_buf[0..cpc_argc] });
            const compile_res = try compile_child.wait(global_io);
            switch (compile_res) {
                .exited => |code| if (code != 0) return error.CompileFailed,
                else => return error.CompileFailed,
            }
            const cpc_ms = cpc_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toMilliseconds();
            std.debug.print("  → [cpc] Transpiled in {d}ms\n", .{cpc_ms});
        }
    }

    const zig_file = zig_file_buf.?;

    // Emit Type Definitions (.d.cp)
    {
        const cpc_bin = try getCpcBin(allocator);
        defer allocator.free(cpc_bin);
        const type_out_path = try std.fmt.allocPrint(allocator, "bin/{s}.d.cp", .{bin_subpath});
        defer allocator.free(type_out_path);
        if (std.Io.Dir.path.dirname(type_out_path)) |dirname| {
            fs_shim.cwd().createDirPath(global_io, dirname) catch {};
        }
        const emit_types_args = &[_][]const u8{ cpc_bin, "emit-types", filename, "--output", type_out_path };
        var emit_types_child = try std.process.spawn(global_io, .{ .argv = emit_types_args });
        _ = try emit_types_child.wait(global_io);
    }

    var par_obj_res = modular_obj.ParallelObjResult.init();
    defer par_obj_res.deinit(allocator);
    var effective_zig_file: []const u8 = zig_file;

    if (opts.is_modular and opts.parallel_obj) {
        if (mod_zig_dir_opt) |mod_zig_dir| {
            const obj_root = try std.fmt.allocPrint(allocator, ".cap/run-mod/{s}/obj", .{bin_subpath});
            defer allocator.free(obj_root);
            par_obj_res = modular_obj.prepareParallelObjects(allocator, .{
                .zig_root_dir = mod_zig_dir,
                .obj_root_dir = obj_root,
                .optimize_mode = opt_mode,
                .target_triple = "native",
                .force_rebuild = opts.force_rebuild,
            }) catch modular_obj.ParallelObjResult.init();

            if (par_obj_res.active and par_obj_res.link_entry_zig != null) {
                effective_zig_file = par_obj_res.link_entry_zig.?;
                const mod_ms = @as(f64, @floatFromInt(par_obj_res.elapsed_ns)) / 1_000_000.0;
                std.debug.print("  → [modular-obj] {d} cached, {d} rebuilt in {d:.2}ms\n", .{
                    par_obj_res.hits,
                    par_obj_res.misses,
                    mod_ms,
                });
            }
        }
    }

    std.debug.print("Linking: {s}...\n", .{effective_zig_file});

    if (std.Io.Dir.path.dirname(run_path)) |dirname| {
        fs_shim.cwd().createDirPath(global_io, dirname) catch {};
    }

    const tmp_run_path = try std.fmt.allocPrint(allocator, "{s}.tmp.{d}", .{ run_path, getCurrentPid() });
    defer allocator.free(tmp_run_path);
    const emit_bin_arg = try std.fmt.allocPrint(allocator, "-femit-bin={s}", .{tmp_run_path});
    defer allocator.free(emit_bin_arg);
    fs_shim.cwd().createDirPath(global_io, ".cap/zig-cache") catch {};

    var mf_opt: ?manifest.Manifest = null;
    var mf_content: ?[]const u8 = null;
    defer if (mf_content) |c| allocator.free(c);
    if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
        mf_content = content;
        if (manifest.Manifest.parse(allocator, content)) |mf| {
            mf_opt = mf;
        } else |_| {}
    } else |_| {}
    defer if (mf_opt) |*m| m.deinit(allocator);

    var build_args_buf: [512][]const u8 = undefined;
    var build_argc: usize = 0;
    build_args_buf[build_argc] = "zig";
    build_argc += 1;
    build_args_buf[build_argc] = "build-exe";
    build_argc += 1;
    const entry_arg_idx = build_argc;
    build_args_buf[build_argc] = effective_zig_file;
    build_argc += 1;
    build_args_buf[build_argc] = emit_bin_arg;
    build_argc += 1;
    build_args_buf[build_argc] = "--cache-dir";
    build_argc += 1;
    build_args_buf[build_argc] = ".cap/zig-cache";
    build_argc += 1;
    build_args_buf[build_argc] = "-O";
    build_argc += 1;
    build_args_buf[build_argc] = opt_mode;
    build_argc += 1;
    build_args_buf[build_argc] = "-lc";
    build_argc += 1;

    var allocated_libs: [64][]const u8 = undefined;
    var allocated_libs_len: usize = 0;
    defer for (allocated_libs[0..allocated_libs_len]) |al| allocator.free(al);

    try appendLinkArgs(allocator, &build_args_buf, &build_argc, &allocated_libs, &allocated_libs_len, mf_opt, "native", opt_mode, zig_file, ".cap/zig-cache");
    scanZigFileForLinkPragmas(allocator, zig_file, &build_args_buf, &build_argc, &allocated_libs, &allocated_libs_len);
    if (opts.is_modular) {
        if (mod_zig_dir_opt) |mod_zig_dir| {
            scanModularManifestForLinkPragmas(allocator, mod_zig_dir, &build_args_buf, &build_argc, &allocated_libs, &allocated_libs_len);
        }
    }

    const pre_obj_argc = build_argc;
    if (par_obj_res.active) {
        for (par_obj_res.obj_paths.items) |obj_p| {
            addArg(&build_args_buf, &build_argc, obj_p);
        }
    }

    var build_ok = try spawnAndWaitSuccess(build_args_buf[0..build_argc]);
    if (!build_ok and par_obj_res.active) {
        fs_shim.cwd().deleteFile(tmp_run_path) catch {};
        std.debug.print("  ! [modular-obj] Incremental link failed, falling back to joint modular build...\n", .{});
        build_args_buf[entry_arg_idx] = zig_file;
        build_ok = try spawnAndWaitSuccess(build_args_buf[0..pre_obj_argc]);
    }
    if (!build_ok) {
        fs_shim.cwd().deleteFile(tmp_run_path) catch {};
        return error.BuildFailed;
    }
    fs_shim.cwd().rename(tmp_run_path, run_path) catch |err| {
        fs_shim.cwd().deleteFile(tmp_run_path) catch {};
        return err;
    };

    const comp_ms = comp_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toMilliseconds();
    if (comp_ms >= 1000) {
        const comp_sec = @as(f64, @floatFromInt(comp_ms)) / 1000.0;
        std.debug.print("Compiled and linked in {d:.2}s\n", .{comp_sec});
    } else {
        std.debug.print("Compiled and linked in {d}ms\n", .{comp_ms});
    }

    try spawnRunBinary(allocator, run_path, mf_opt, zig_file);
}

fn sniffHardware() !void {
    std.debug.print("--- CAP Hardware Sniffing ---\n", .{});
    std.debug.print("CPU: {s}\n", .{@tagName(builtin.cpu.arch)});
    if (builtin.cpu.arch == .x86_64) {
        const avx2 = std.Target.x86.featureSetHas(builtin.cpu.features, .avx2);
        std.debug.print("AVX2 Support: {}\n", .{avx2});
    }
    std.debug.print("OS: {s}\n", .{@tagName(builtin.os.tag)});
}

const BuildOptions = struct {
    is_release: bool = false,
    is_beam: bool = false,
    optimize_mode: []const u8 = "ReleaseSafe",
    build_type: []const u8 = "build-exe",
    out_dir_val: []const u8 = "bin",
    explicit_target: ?[]const u8 = null,
    configured_targets: [][]const u8 = &.{},
    zig_args: []const []const u8 = &.{},
    force_rebuild: bool = false,
    force_llvm: bool = false,
    emit_types: bool = false,
    is_modular: bool = false,
    parallel_obj: bool = false,
    timings: bool = false,
    explicit_cache_dir: ?[]const u8 = null,
};

const BuildTimings = struct {
    source_scan_ns: i128 = 0,
    cpc_transpile_ns: i128 = 0,
    modular_obj_ns: i128 = 0,
    modular_obj_hits: usize = 0,
    modular_obj_misses: usize = 0,
    modular_obj_active: bool = false,
    zig_compile_ns: i128 = 0,
    atomic_install_ns: i128 = 0,
};

const WorkspacePaths = struct {
    zig_root_dir: []const u8,
    zig_cache_dir: []const u8,

    fn deinit(self: WorkspacePaths, allocator: std.mem.Allocator) void {
        allocator.free(self.zig_root_dir);
        allocator.free(self.zig_cache_dir);
    }
};

fn trimTrailingSlashes(path: []const u8) []const u8 {
    var s = path;
    while (s.len > 1 and s[s.len - 1] == '/') {
        s = s[0 .. s.len - 1];
    }
    return s;
}

fn sanitizePathForWs(allocator: std.mem.Allocator, raw_path: []const u8) ![]const u8 {
    var s = raw_path;
    while (std.mem.startsWith(u8, s, "./")) {
        s = s[2..];
    }
    while (s.len > 0 and s[0] == '/') {
        s = s[1..];
    }
    s = trimTrailingSlashes(s);
    if (s.len == 0) return try allocator.dupe(u8, "default");

    const buf = try allocator.alloc(u8, s.len);
    for (s, 0..) |c, idx| {
        if (std.ascii.isAlphanumeric(c) or c == '-' or c == '_') {
            buf[idx] = c;
        } else {
            buf[idx] = '_';
        }
    }
    return buf;
}

fn resolveWorkspacePaths(allocator: std.mem.Allocator, out_dir_val: []const u8, explicit_cache_dir: ?[]const u8) !WorkspacePaths {
    const norm_out = trimTrailingSlashes(out_dir_val);
    const is_default_out = std.mem.eql(u8, norm_out, "bin") or std.mem.eql(u8, norm_out, "./bin");

    if (explicit_cache_dir) |cd| {
        if (cd.len > 0) {
            const ws_key = if (!is_default_out) norm_out else cd;
            const sanitized = try sanitizePathForWs(allocator, ws_key);
            defer allocator.free(sanitized);
            return .{
                .zig_root_dir = try std.fmt.allocPrint(allocator, ".cap/ws/{s}/zig", .{sanitized}),
                .zig_cache_dir = try allocator.dupe(u8, cd),
            };
        }
    }

    if (!is_default_out) {
        const sanitized = try sanitizePathForWs(allocator, norm_out);
        defer allocator.free(sanitized);
        return .{
            .zig_root_dir = try std.fmt.allocPrint(allocator, ".cap/ws/{s}/zig", .{sanitized}),
            .zig_cache_dir = try std.fmt.allocPrint(allocator, ".cap/ws/{s}/zig-cache", .{sanitized}),
        };
    }

    return .{
        .zig_root_dir = try allocator.dupe(u8, ".cap/zig"),
        .zig_cache_dir = try allocator.dupe(u8, ".cap/zig-cache"),
    };
}

fn computeMaxMtimeInDir(dir: std.Io.Dir, ext: []const u8, exclude_d_cp: bool) ?i96 {
    var max_mtime: ?i96 = null;
    var iter = dir.iterate();
    while (iter.next(global_io) catch null) |entry| {
        if (entry.kind == .directory) {
            if (std.mem.eql(u8, entry.name, ".cap") or
                std.mem.eql(u8, entry.name, "zig-cache") or
                std.mem.eql(u8, entry.name, ".zig-cache") or
                std.mem.eql(u8, entry.name, "node_modules") or
                std.mem.eql(u8, entry.name, "bin") or
                std.mem.eql(u8, entry.name, ".git"))
            {
                continue;
            }
            var sub_dir = dir.openDir(global_io, entry.name, .{ .iterate = true }) catch continue;
            defer sub_dir.close(global_io);
            if (computeMaxMtimeInDir(sub_dir, ext, exclude_d_cp)) |sub_max| {
                max_mtime = if (max_mtime) |m| @max(m, sub_max) else sub_max;
            }
        } else if (entry.kind == .file or entry.kind == .sym_link) {
            if (dir.statFile(global_io, entry.name, .{})) |st| {
                if (st.kind == .directory) {
                    var sub_dir = dir.openDir(global_io, entry.name, .{ .iterate = true }) catch continue;
                    defer sub_dir.close(global_io);
                    if (computeMaxMtimeInDir(sub_dir, ext, exclude_d_cp)) |sub_max| {
                        max_mtime = if (max_mtime) |m| @max(m, sub_max) else sub_max;
                    }
                } else {
                    if (!std.mem.endsWith(u8, entry.name, ext)) continue;
                    if (exclude_d_cp and std.mem.endsWith(u8, entry.name, ".d.cp")) continue;
                    const ns = st.mtime.nanoseconds;
                    max_mtime = if (max_mtime) |m| @max(m, ns) else ns;
                }
            } else |_| {}
        }
    }
    return max_mtime;
}

fn computeMaxSourceMtimeNs(filename: []const u8) ?i96 {
    const src_stat = fs_shim.cwd().statFile(filename) catch return null;
    var max_mtime: i96 = src_stat.mtime.nanoseconds;

    if (fs_shim.cwd().statFile("cap.json")) |cj_stat| {
        max_mtime = @max(max_mtime, cj_stat.mtime.nanoseconds);
    } else |_| {}

    if (fs_shim.cwd().openDir("src", .{ .iterate = true })) |src_dir_val| {
        var src_dir = src_dir_val;
        defer src_dir.close(global_io);
        if (computeMaxMtimeInDir(src_dir, ".cp", true)) |dir_max| {
            max_mtime = @max(max_mtime, dir_max);
        }
    } else |_| {}

    return max_mtime;
}

fn resolveModularEntryFile(allocator: std.mem.Allocator, zig_dir: []const u8, entry_stem: []const u8) !?[]const u8 {
    const cp_entry_cand = try std.fmt.allocPrint(allocator, "{s}/__cp_entry.zig", .{zig_dir});
    if (fs_shim.cwd().statFile(cp_entry_cand) catch null != null) {
        return cp_entry_cand;
    }
    allocator.free(cp_entry_cand);

    const entry_cand = try std.fmt.allocPrint(allocator, "{s}/__entry.zig", .{zig_dir});
    if (fs_shim.cwd().statFile(entry_cand) catch null != null) {
        return entry_cand;
    }
    allocator.free(entry_cand);

    const stem_cand = try std.fmt.allocPrint(allocator, "{s}/{s}.zig", .{ zig_dir, entry_stem });
    if (fs_shim.cwd().statFile(stem_cand) catch null != null) {
        return stem_cand;
    }
    allocator.free(stem_cand);

    const src_stem_cand = try std.fmt.allocPrint(allocator, "{s}/src/{s}.zig", .{ zig_dir, entry_stem });
    if (fs_shim.cwd().statFile(src_stem_cand) catch null != null) {
        return src_stem_cand;
    }
    allocator.free(src_stem_cand);
    return null;
}

fn resolveBuildType(allocator: std.mem.Allocator, build_type: []const u8, mf_opt: ?manifest.Manifest, filename: []const u8) ![]const u8 {
    if (!std.mem.eql(u8, build_type, "build-exe")) {
        return build_type;
    }
    if (mf_opt) |mf| {
        const main_target = mf.main orelse (if (mf.build_config) |bc| bc.entry else null);
        if (main_target) |main_file| {
            const full_main_path = if (std.mem.startsWith(u8, main_file, "src/"))
                try allocator.dupe(u8, main_file)
            else
                try std.fmt.allocPrint(allocator, "src/{s}", .{main_file});
            defer allocator.free(full_main_path);
            if (std.mem.eql(u8, filename, full_main_path)) {
                return "build-exe";
            }
        }
    }
    return "build-lib";
}

fn buildPackage(allocator: std.mem.Allocator, args: []const []const u8) !void {
    // 1. Verify lockfile
    const lock_content = fs_shim.cwd().readFileAlloc(allocator, "cap.lock", 1024 * 1024) catch {
        std.debug.print("Error: cap.lock missing. Run `cap install` first.\n", .{});
        return error.LockfileMissing;
    };
    defer allocator.free(lock_content);
    var lk = try lock.Lockfile.parse(allocator, lock_content);
    defer lk.deinit(allocator);

    var mf_opt: ?manifest.Manifest = null;
    var mf_content: ?[]const u8 = null;
    defer if (mf_content) |c| allocator.free(c);
    if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
        mf_content = content;
        if (manifest.Manifest.parse(allocator, content)) |mf| {
            mf_opt = mf;
        } else |_| {}
    } else |_| {}
    defer if (mf_opt) |*m| m.deinit(allocator);

    if (mf_opt) |*mf| {
        mf.verifyLockfile(lk) catch |e| {
            std.debug.print("Error: Lockfile mismatch with cap.json. Run `cap install` to update.\n", .{});
            return e;
        };
    }

    var targets_to_build: [100][]const u8 = undefined;
    var targets_len: usize = 0;

    var opts = BuildOptions{};
    const build_start = std.Io.Timestamp.now(global_io, .awake);

    if (mf_opt) |mf| {
        if (mf.build_config) |bc| {
            if (bc.opt) |o| {
                opts.optimize_mode = o.toZigFlag();
            }
            if (bc.out_dir) |od| {
                opts.out_dir_val = od;
            }
            if (bc.target) |t| {
                opts.explicit_target = t;
            }
            if (bc.targets.len > 0) {
                opts.configured_targets = bc.targets;
            }
            if (bc.sys) |s| {
                const parsed = std.json.parseFromSlice(std.json.Value, allocator, s, .{}) catch unreachable;
                defer parsed.deinit();
                const os_tag = @tagName(@import("builtin").os.tag);
                if (parsed.value.object.get(os_tag)) |lib_path| {
                    std.debug.print("Linking dynamic sys lib for {s}: {s}\n", .{ os_tag, lib_path.string });
                }
            }
        }
    }

    var zig_args: [128][]const u8 = undefined;
    var zig_args_len: usize = 0;

    // Append c_flags, link_libs, bin_dependencies, and auto-detected libs after CLI flags
    var allocated_libs: [64][]const u8 = undefined;
    var allocated_libs_len: usize = 0;
    defer for (allocated_libs[0..allocated_libs_len]) |al| allocator.free(al);

    var has_target = false;
    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "-B") or std.mem.eql(u8, arg, "--force")) {
            opts.force_rebuild = true;
        } else if (std.mem.eql(u8, arg, "-r") or std.mem.eql(u8, arg, "--release") or std.mem.eql(u8, arg, "release")) {
            opts.is_release = true;
        } else if (std.mem.eql(u8, arg, "-b") or std.mem.eql(u8, arg, "--beam")) {
            opts.is_beam = true;
            opts.build_type = "build-lib";
        } else if (std.mem.eql(u8, arg, "-f") or std.mem.eql(u8, arg, "--fast")) {
            opts.optimize_mode = "ReleaseFast";
        } else if (std.mem.eql(u8, arg, "-s") or std.mem.eql(u8, arg, "--small")) {
            opts.optimize_mode = "ReleaseSmall";
        } else if (std.mem.eql(u8, arg, "-d") or std.mem.eql(u8, arg, "--debug")) {
            opts.optimize_mode = "Debug";
        } else if (std.mem.eql(u8, arg, "--llvm")) {
            opts.force_llvm = true;
        } else if (std.mem.eql(u8, arg, "--emit-types")) {
            opts.emit_types = true;
        } else if (std.mem.eql(u8, arg, "--timings")) {
            opts.timings = true;
        } else if (std.mem.eql(u8, arg, "-m") or std.mem.eql(u8, arg, "--modular") or std.mem.eql(u8, arg, "-p") or std.mem.eql(u8, arg, "--parallel-obj")) {
            opts.is_modular = true;
            opts.parallel_obj = true;
        } else if (std.mem.eql(u8, arg, "--low") or std.mem.eql(u8, arg, "--no-parallel")) {
            opts.parallel_obj = false;
        } else if (std.mem.eql(u8, arg, "--safe")) {
            opts.optimize_mode = "ReleaseSafe";
        } else if (std.mem.startsWith(u8, arg, "--opt=")) {
            const opt_val = arg[6..];
            if (manifest.OptimizeMode.fromString(opt_val)) |m| {
                opts.optimize_mode = m.toZigFlag();
            }
        } else if (std.mem.startsWith(u8, arg, "--out-dir=")) {
            opts.out_dir_val = arg[10..];
        } else if (std.mem.eql(u8, arg, "-o")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.out_dir_val = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--cache-dir=")) {
            opts.explicit_cache_dir = arg["--cache-dir=".len..];
        } else if (std.mem.eql(u8, arg, "--cache-dir")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.explicit_cache_dir = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--target=")) {
            opts.explicit_target = arg[9..];
        } else if (std.mem.eql(u8, arg, "-t")) {
            if (i + 1 < args.len) {
                i += 1;
                opts.explicit_target = args[i];
            }
        } else if (std.mem.eql(u8, arg, "--exe")) {
            opts.build_type = "build-exe";
        } else if (std.mem.eql(u8, arg, "--obj")) {
            opts.build_type = "build-obj";
        } else if (std.mem.eql(u8, arg, "--lib")) {
            opts.build_type = "build-lib";
        } else if (std.mem.startsWith(u8, arg, "-")) {
            if (zig_args_len < zig_args.len) {
                zig_args[zig_args_len] = arg;
                zig_args_len += 1;
            }
        } else {
            has_target = true;
            if (targets_len < 100) {
                targets_to_build[targets_len] = arg;
                targets_len += 1;
            }
        }
    }

    const ws_paths = try resolveWorkspacePaths(allocator, opts.out_dir_val, opts.explicit_cache_dir);
    defer ws_paths.deinit(allocator);

    try appendLinkArgs(allocator, &zig_args, &zig_args_len, &allocated_libs, &allocated_libs_len, mf_opt, "native", opts.optimize_mode, null, ws_paths.zig_cache_dir);
    opts.zig_args = zig_args[0..zig_args_len];

    if (!has_target) {
        if (fs_shim.cwd().statFile("src") catch null != null) {
            if (targets_len < 100) {
                targets_to_build[targets_len] = "src";
                targets_len += 1;
            }
        } else if (fs_shim.cwd().statFile("test") catch null != null) {
            if (targets_len < 100) {
                targets_to_build[targets_len] = "test";
                targets_len += 1;
            }
        }
    }

    var timings = BuildTimings{};
    for (targets_to_build[0..targets_len]) |target_path| {
        try buildTarget(allocator, target_path, opts, ws_paths, &timings);
    }
    const total_dur = build_start.durationTo(std.Io.Timestamp.now(global_io, .awake));
    const total_ms = total_dur.toMilliseconds();
    if (total_ms >= 1000) {
        const total_sec = @as(f64, @floatFromInt(total_ms)) / 1000.0;
        std.debug.print("Build completed in {d:.2}s! Artifacts in {s}/\n", .{ total_sec, opts.out_dir_val });
    } else {
        std.debug.print("Build completed in {d}ms! Artifacts in {s}/\n", .{ total_ms, opts.out_dir_val });
    }

    if (opts.timings) {
        const total_ns: i128 = @intCast(total_dur.toNanoseconds());
        const scan_ms = @as(f64, @floatFromInt(timings.source_scan_ns)) / 1_000_000.0;
        const cpc_ms = @as(f64, @floatFromInt(timings.cpc_transpile_ns)) / 1_000_000.0;
        const zig_ms = @as(f64, @floatFromInt(timings.zig_compile_ns)) / 1_000_000.0;
        const install_ms = @as(f64, @floatFromInt(timings.atomic_install_ns)) / 1_000_000.0;
        const tot_f_ms = @as(f64, @floatFromInt(total_ns)) / 1_000_000.0;
        const denom: f64 = if (tot_f_ms > 0.0) tot_f_ms else 1.0;

        std.debug.print("\nBuild Timings:\n", .{});
        std.debug.print("  Source mtime scan  : {d:>8.2}ms ({d:>5.1}%)\n", .{ scan_ms, scan_ms * 100.0 / denom });
        std.debug.print("  cpc Transpile      : {d:>8.2}ms ({d:>5.1}%)\n", .{ cpc_ms, cpc_ms * 100.0 / denom });
        if (timings.modular_obj_active) {
            const mod_obj_ms = @as(f64, @floatFromInt(timings.modular_obj_ns)) / 1_000_000.0;
            std.debug.print("  modular-obj: {d} cached, {d} rebuilt in {d:.2}ms\n", .{
                timings.modular_obj_hits,
                timings.modular_obj_misses,
                mod_obj_ms,
            });
        }
        std.debug.print("  Zig Native Compile : {d:>8.2}ms ({d:>5.1}%)\n", .{ zig_ms, zig_ms * 100.0 / denom });
        std.debug.print("  Atomic Install     : {d:>8.2}ms ({d:>5.1}%)\n", .{ install_ms, install_ms * 100.0 / denom });
        std.debug.print("  Total              : {d:>8.2}ms\n", .{tot_f_ms});
    }
}

fn buildTarget(
    allocator: std.mem.Allocator,
    target_path: []const u8,
    opts: BuildOptions,
    ws_paths: WorkspacePaths,
    timings: *BuildTimings,
) !void {
    const stat = fs_shim.cwd().statFile(target_path) catch return;
    if (stat.kind == .directory) {
        var dir = try fs_shim.cwd().openDir(target_path, .{ .iterate = true });
        defer dir.close(global_io);
        var it = dir.iterate();
        while (try it.next(global_io)) |entry| {
            const child_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ target_path, entry.name });
            defer allocator.free(child_path);
            try buildTarget(allocator, child_path, opts, ws_paths, timings);
        }
    } else if (std.mem.endsWith(u8, target_path, ".cp")) {
        try compileSingleFile(allocator, target_path, opts, ws_paths, timings);
    }
}

fn compileSingleFile(
    allocator: std.mem.Allocator,
    filename: []const u8,
    opts: BuildOptions,
    ws_paths: WorkspacePaths,
    timings: *BuildTimings,
) !void {
    const base_name = filename[0 .. filename.len - 3];
    var bin_subpath: []const u8 = base_name;
    if (std.mem.startsWith(u8, base_name, "src/")) {
        bin_subpath = base_name[4..];
    }

    const Target = struct { name: []const u8, triple: []const u8, ext: []const u8 };
    const default_targets = [_]Target{
        .{ .name = "Native", .triple = "native", .ext = "" },
        .{ .name = "cap-macos-aarch64", .triple = "aarch64-macos", .ext = "" },
        .{ .name = "cap-macos-x86_64", .triple = "x86_64-macos", .ext = "" },
        .{ .name = "cap-linux-x86_64", .triple = "x86_64-linux", .ext = "" },
        .{ .name = "cap-windows-x86_64", .triple = "x86_64-windows", .ext = ".exe" },
    };

    var targets_to_run: [16]Target = undefined;
    var target_count: usize = 0;

    if (opts.explicit_target) |et| {
        const ext = if (std.mem.indexOf(u8, et, "windows") != null) ".exe" else "";
        const name = if (std.mem.eql(u8, et, "native") or std.mem.eql(u8, et, "Native")) "Native" else et;
        targets_to_run[0] = .{ .name = name, .triple = et, .ext = ext };
        target_count = 1;
    } else if (opts.is_release) {
        if (opts.configured_targets.len > 0) {
            for (opts.configured_targets) |ct| {
                if (target_count < 16) {
                    const ext = if (std.mem.indexOf(u8, ct, "windows") != null) ".exe" else "";
                    const name = if (std.mem.eql(u8, ct, "native") or std.mem.eql(u8, ct, "Native"))
                        "Native"
                    else if (std.mem.startsWith(u8, ct, "cap-"))
                        ct
                    else
                        try std.fmt.allocPrint(allocator, "cap-{s}", .{ct});
                    const triple = if (std.mem.startsWith(u8, ct, "cap-")) ct["cap-".len..] else ct;
                    targets_to_run[target_count] = .{ .name = name, .triple = triple, .ext = ext };
                    target_count += 1;
                }
            }
        } else {
            for (default_targets) |dt| {
                targets_to_run[target_count] = dt;
                target_count += 1;
            }
        }
    } else {
        targets_to_run[0] = default_targets[0]; // Native only
        target_count = 1;
    }

    const base_ext = if (std.mem.eql(u8, opts.build_type, "build-obj")) ".o" else if (opts.is_beam) ".so" else if (std.mem.eql(u8, opts.build_type, "build-lib")) ".a" else "";
    const entry_stem = std.Io.Dir.path.stem(filename);

    // 1. Recursive source mtime (all .cp under src/ + filename + cap.json)
    const scan_start = std.Io.Timestamp.now(global_io, .awake);
    const max_src_mtime_ns = computeMaxSourceMtimeNs(filename);

    var zig_file_buf: ?[]const u8 = null;
    defer if (zig_file_buf) |zf| allocator.free(zf);
    var existing_zig_mtime_ns: ?i96 = null;

    if (opts.is_modular) {
        if (try resolveModularEntryFile(allocator, ws_paths.zig_root_dir, entry_stem)) |resolved| {
            zig_file_buf = resolved;
            if (fs_shim.cwd().openDir(ws_paths.zig_root_dir, .{ .iterate = true })) |mod_dir_val| {
                var mod_dir = mod_dir_val;
                defer mod_dir.close(global_io);
                existing_zig_mtime_ns = computeMaxMtimeInDir(mod_dir, ".zig", false);
            } else |_| {}
            if (existing_zig_mtime_ns == null) {
                if (fs_shim.cwd().statFile(resolved)) |st| {
                    existing_zig_mtime_ns = st.mtime.nanoseconds;
                } else |_| {}
            }
        }
    } else {
        const single_zig = try std.fmt.allocPrint(allocator, "{s}/{s}.zig", .{ ws_paths.zig_root_dir, base_name });
        zig_file_buf = single_zig;
        if (fs_shim.cwd().statFile(single_zig)) |st| {
            existing_zig_mtime_ns = st.mtime.nanoseconds;
        } else |_| {}
    }

    // 2. Fast path: check if all target binaries are already up to date
    var all_up_to_date = !opts.force_rebuild and max_src_mtime_ns != null;
    if (all_up_to_date) {
        for (targets_to_run[0..target_count]) |t| {
            var target_out_dir: []const u8 = opts.out_dir_val;
            var dir_path: ?[]const u8 = null;
            defer if (dir_path) |d| allocator.free(d);
            const is_target_native = std.mem.eql(u8, t.name, "Native") or std.mem.eql(u8, t.triple, "native");
            if (!is_target_native and opts.explicit_target == null) {
                dir_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ opts.out_dir_val, t.name });
                target_out_dir = dir_path.?;
            }
            const out_file_path = try std.fmt.allocPrint(allocator, "{s}/{s}{s}{s}", .{ target_out_dir, bin_subpath, base_ext, t.ext });
            defer allocator.free(out_file_path);
            const out_stat = fs_shim.cwd().statFile(out_file_path) catch null;
            if (out_stat == null or
                out_stat.?.mtime.nanoseconds < max_src_mtime_ns.? or
                (existing_zig_mtime_ns != null and out_stat.?.mtime.nanoseconds < existing_zig_mtime_ns.?))
            {
                all_up_to_date = false;
                break;
            }
        }
    }
    timings.source_scan_ns += @intCast(scan_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toNanoseconds());

    if (all_up_to_date) {
        std.debug.print("Building: {s} (Mode: {s})...\n", .{ filename, opts.optimize_mode });
        std.debug.print("  → [Up-to-date] Output binaries are up to date (skipped)\n", .{});
        return;
    }

    std.debug.print("Building: {s} (Mode: {s})...\n", .{ filename, opts.optimize_mode });

    if (opts.is_modular) {
        fs_shim.cwd().createDirPath(global_io, ws_paths.zig_root_dir) catch {};
        if (opts.is_beam) {
            copyDir("/data/dev/cp/sdk/beam.zig", ws_paths.zig_root_dir) catch {};
            copyDir("/data/dev/cp/sdk/erl_nif.zig", ws_paths.zig_root_dir) catch {};
        }
    } else if (std.Io.Dir.path.dirname(zig_file_buf.?)) |dirname| {
        fs_shim.cwd().createDirPath(global_io, dirname) catch {};
        if (opts.is_beam) {
            copyDir("/data/dev/cp/sdk/beam.zig", dirname) catch {};
            copyDir("/data/dev/cp/sdk/erl_nif.zig", dirname) catch {};
        }
    }

    var cur_zig_mtime_ns: ?i96 = existing_zig_mtime_ns;
    if (!opts.force_rebuild and existing_zig_mtime_ns != null and max_src_mtime_ns != null and existing_zig_mtime_ns.? >= max_src_mtime_ns.?) {
        std.debug.print("  → [cpc] Intermediate Zig source is up to date (skipped)\n", .{});
    } else {
        const cpc_bin = try getCpcBin(allocator);
        defer allocator.free(cpc_bin);
        const cpc_start = std.Io.Timestamp.now(global_io, .awake);
        var cpc_args_buf: [20][]const u8 = undefined;
        var cpc_argc: usize = 0;
        cpc_args_buf[cpc_argc] = cpc_bin;
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "compile";
        cpc_argc += 1;
        if (opts.is_modular) {
            cpc_args_buf[cpc_argc] = "--emit-dir";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = ws_paths.zig_root_dir;
            cpc_argc += 1;
            if (opts.force_rebuild) {
                cpc_args_buf[cpc_argc] = "--force";
                cpc_argc += 1;
            }
            cpc_args_buf[cpc_argc] = filename;
            cpc_argc += 1;
        } else {
            cpc_args_buf[cpc_argc] = filename;
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = "--output";
            cpc_argc += 1;
            cpc_args_buf[cpc_argc] = zig_file_buf.?;
            cpc_argc += 1;
        }
        cpc_args_buf[cpc_argc] = "-I";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "cap_modules";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "-I";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "/data/dev/cp/cap/cap_modules";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "-I";
        cpc_argc += 1;
        cpc_args_buf[cpc_argc] = "/data/dev/cp/lib";
        cpc_argc += 1;
        if (opts.is_beam) {
            cpc_args_buf[cpc_argc] = "-b";
            cpc_argc += 1;
        }

        var compile_child = try std.process.spawn(global_io, .{ .argv = cpc_args_buf[0..cpc_argc] });
        const compile_res = try compile_child.wait(global_io);
        switch (compile_res) {
            .exited => |code| if (code != 0) return error.CompileFailed,
            else => return error.CompileFailed,
        }

        if (opts.is_modular) {
            if (zig_file_buf) |old_zf| {
                allocator.free(old_zf);
                zig_file_buf = null;
            }
            zig_file_buf = try resolveModularEntryFile(allocator, ws_paths.zig_root_dir, entry_stem);
            if (zig_file_buf == null) {
                std.debug.print("Error: --modular compilation did not produce {s}/__cp_entry.zig or {s}/{s}.zig\n", .{ ws_paths.zig_root_dir, ws_paths.zig_root_dir, entry_stem });
                return error.CompileFailed;
            }
            if (fs_shim.cwd().openDir(ws_paths.zig_root_dir, .{ .iterate = true })) |mod_dir_val| {
                var mod_dir = mod_dir_val;
                defer mod_dir.close(global_io);
                cur_zig_mtime_ns = computeMaxMtimeInDir(mod_dir, ".zig", false);
            } else |_| {}
            if (cur_zig_mtime_ns == null) {
                if (fs_shim.cwd().statFile(zig_file_buf.?)) |st| {
                    cur_zig_mtime_ns = st.mtime.nanoseconds;
                } else |_| {}
            }
        } else {
            if (fs_shim.cwd().statFile(zig_file_buf.?)) |st| {
                cur_zig_mtime_ns = st.mtime.nanoseconds;
            } else |_| {}
        }

        const cpc_dur = cpc_start.durationTo(std.Io.Timestamp.now(global_io, .awake));
        timings.cpc_transpile_ns += @intCast(cpc_dur.toNanoseconds());
        const cpc_ms = cpc_dur.toMilliseconds();
        std.debug.print("  → [cpc] Transpiled in {d}ms\n", .{cpc_ms});
    }

    const zig_file = zig_file_buf.?;

    var mf_opt: ?manifest.Manifest = null;
    var mf_content: ?[]const u8 = null;
    defer if (mf_content) |c| allocator.free(c);
    if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
        mf_content = content;
        if (manifest.Manifest.parse(allocator, content)) |mf| {
            mf_opt = mf;
        } else |_| {}
    } else |_| {}
    defer if (mf_opt) |*m| m.deinit(allocator);

    const build_type_resolved = try resolveBuildType(allocator, opts.build_type, mf_opt, filename);

    // Emit Type Definitions (.d.cp) only for libraries or when explicitly requested
    if (opts.emit_types or std.mem.eql(u8, build_type_resolved, "build-lib")) {
        const emit_start = std.Io.Timestamp.now(global_io, .awake);
        const cpc_bin = try getCpcBin(allocator);
        defer allocator.free(cpc_bin);
        const type_out_path = try std.fmt.allocPrint(allocator, "{s}/{s}.d.cp", .{ opts.out_dir_val, bin_subpath });
        defer allocator.free(type_out_path);
        if (std.Io.Dir.path.dirname(type_out_path)) |dirname| {
            fs_shim.cwd().createDirPath(global_io, dirname) catch {};
        }
        const emit_types_args = &[_][]const u8{ cpc_bin, "emit-types", filename, "--output", type_out_path };
        var emit_types_child = try std.process.spawn(global_io, .{ .argv = emit_types_args });
        _ = try emit_types_child.wait(global_io);
        timings.cpc_transpile_ns += @intCast(emit_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toNanoseconds());
    }

    fs_shim.cwd().createDirPath(global_io, ws_paths.zig_cache_dir) catch {};

    const is_x86_64_linux_host = builtin.cpu.arch == .x86_64 and builtin.os.tag == .linux;
    const has_explicit_llvm_flag = opts.force_llvm or hasArg(opts.zig_args, "-fllvm") or hasArg(opts.zig_args, "-fno-llvm");

    var any_failed = false;
    for (targets_to_run[0..target_count]) |t| {
        var dir_path: ?[]const u8 = null;
        defer if (dir_path) |d| allocator.free(d);
        var target_out_dir: []const u8 = opts.out_dir_val;
        const is_target_native = std.mem.eql(u8, t.name, "Native") or std.mem.eql(u8, t.triple, "native");
        if (!is_target_native and opts.explicit_target == null) {
            dir_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ opts.out_dir_val, t.name });
            target_out_dir = dir_path.?;
            fs_shim.cwd().createDirPath(global_io, target_out_dir) catch {};
        }

        const out_file_path = try std.fmt.allocPrint(allocator, "{s}/{s}{s}{s}", .{ target_out_dir, bin_subpath, base_ext, t.ext });
        defer allocator.free(out_file_path);

        if (std.Io.Dir.path.dirname(out_file_path)) |dirname| {
            fs_shim.cwd().createDirPath(global_io, dirname) catch {};
        }

        const out_stat = fs_shim.cwd().statFile(out_file_path) catch null;
        if (!opts.force_rebuild and
            out_stat != null and
            cur_zig_mtime_ns != null and
            max_src_mtime_ns != null and
            out_stat.?.mtime.nanoseconds >= cur_zig_mtime_ns.? and
            out_stat.?.mtime.nanoseconds >= max_src_mtime_ns.?)
        {
            std.debug.print("  → [{s}] Target binary is up to date (skipped)\n", .{t.name});
            continue;
        }

        var emit_flag: []const u8 = "-femit-bin=";
        if (std.mem.eql(u8, opts.build_type, "build-obj")) emit_flag = "-femit-obj=";

        const tmp_out_path = try std.fmt.allocPrint(allocator, "{s}.tmp.{d}", .{ out_file_path, getCurrentPid() });
        defer allocator.free(tmp_out_path);
        const emit_arg = try std.fmt.allocPrint(allocator, "{s}{s}", .{ emit_flag, tmp_out_path });
        defer allocator.free(emit_arg);

        const use_no_llvm = is_x86_64_linux_host and
            std.mem.eql(u8, opts.optimize_mode, "Debug") and
            is_target_native and
            !opts.is_beam and
            std.mem.eql(u8, build_type_resolved, "build-exe") and
            !has_explicit_llvm_flag;

        if (use_no_llvm) {
            std.debug.print("  → Compiling for {s} ({s}, -fno-llvm)...\n", .{ t.name, opts.optimize_mode });
        } else {
            std.debug.print("  → Compiling for {s} ({s})...\n", .{ t.name, opts.optimize_mode });
        }
        const zig_start = std.Io.Timestamp.now(global_io, .awake);

        var par_obj_res = modular_obj.ParallelObjResult.init();
        defer par_obj_res.deinit(allocator);
        var effective_zig_file: []const u8 = zig_file;

        if (opts.is_modular and opts.parallel_obj and !opts.is_beam and std.mem.eql(u8, build_type_resolved, "build-exe")) {
            const obj_root = try std.fmt.allocPrint(allocator, "{s}/../obj", .{ws_paths.zig_root_dir});
            defer allocator.free(obj_root);
            par_obj_res = modular_obj.prepareParallelObjects(allocator, .{
                .zig_root_dir = ws_paths.zig_root_dir,
                .obj_root_dir = obj_root,
                .optimize_mode = opts.optimize_mode,
                .target_triple = t.triple,
                .force_rebuild = opts.force_rebuild,
            }) catch modular_obj.ParallelObjResult.init();

            if (par_obj_res.active and par_obj_res.link_entry_zig != null) {
                effective_zig_file = par_obj_res.link_entry_zig.?;
                timings.modular_obj_active = true;
                timings.modular_obj_ns += par_obj_res.elapsed_ns;
                timings.modular_obj_hits += par_obj_res.hits;
                timings.modular_obj_misses += par_obj_res.misses;
                const mod_ms = @as(f64, @floatFromInt(par_obj_res.elapsed_ns)) / 1_000_000.0;
                std.debug.print("  → [modular-obj] {d} cached, {d} rebuilt in {d:.2}ms\n", .{
                    par_obj_res.hits,
                    par_obj_res.misses,
                    mod_ms,
                });
            }
        }

        var child_args: [512][]const u8 = undefined;
        var argc: usize = 0;
        child_args[argc] = "zig";
        argc += 1;
        child_args[argc] = build_type_resolved;
        argc += 1;
        if (opts.is_beam) {
            child_args[argc] = "-dynamic";
            argc += 1;
        }
        const entry_arg_idx = argc;
        child_args[argc] = effective_zig_file;
        argc += 1;
        child_args[argc] = emit_arg;
        argc += 1;
        child_args[argc] = "--cache-dir";
        argc += 1;
        child_args[argc] = ws_paths.zig_cache_dir;
        argc += 1;
        child_args[argc] = "-O";
        argc += 1;
        child_args[argc] = opts.optimize_mode;
        argc += 1;
        child_args[argc] = "-lc";
        argc += 1;

        if (!std.mem.eql(u8, t.triple, "native")) {
            child_args[argc] = "-target";
            argc += 1;
            child_args[argc] = t.triple;
            argc += 1;
        }

        for (opts.zig_args) |za| {
            addArg(&child_args, &argc, za);
        }

        var local_alloc_libs: [64][]const u8 = undefined;
        var local_alloc_libs_len: usize = 0;
        defer for (local_alloc_libs[0..local_alloc_libs_len]) |al| allocator.free(al);

        try appendLinkArgs(allocator, &child_args, &argc, &local_alloc_libs, &local_alloc_libs_len, mf_opt, t.triple, opts.optimize_mode, zig_file, ws_paths.zig_cache_dir);
        scanZigFileForLinkPragmas(allocator, zig_file, &child_args, &argc, &local_alloc_libs, &local_alloc_libs_len);
        if (opts.is_modular) {
            scanModularManifestForLinkPragmas(allocator, ws_paths.zig_root_dir, &child_args, &argc, &local_alloc_libs, &local_alloc_libs_len);
        }

        const pre_obj_argc = argc;
        if (par_obj_res.active) {
            for (par_obj_res.obj_paths.items) |obj_p| {
                addArg(&child_args, &argc, obj_p);
            }
        }

        const base_argc = argc;
        if (use_no_llvm and argc < child_args.len) {
            child_args[argc] = "-fno-llvm";
            argc += 1;
        }

        var build_ok = try spawnAndWaitSuccess(child_args[0..argc]);
        if (!build_ok and use_no_llvm) {
            fs_shim.cwd().deleteFile(tmp_out_path) catch {};
            std.debug.print("  ! [-fno-llvm] Native backend failed for {s}, falling back to LLVM...\n", .{t.name});
            build_ok = try spawnAndWaitSuccess(child_args[0..base_argc]);
        }
        if (!build_ok and par_obj_res.active) {
            fs_shim.cwd().deleteFile(tmp_out_path) catch {};
            std.debug.print("  ! [modular-obj] Incremental link failed for {s}, falling back to joint modular build...\n", .{t.name});
            child_args[entry_arg_idx] = zig_file;
            build_ok = try spawnAndWaitSuccess(child_args[0..pre_obj_argc]);
        }

        const zig_dur = zig_start.durationTo(std.Io.Timestamp.now(global_io, .awake));
        timings.zig_compile_ns += @intCast(zig_dur.toNanoseconds());

        if (!build_ok) {
            fs_shim.cwd().deleteFile(tmp_out_path) catch {};
            std.debug.print("Failed to build for {s}\n", .{t.name});
            any_failed = true;
        } else {
            const install_start = std.Io.Timestamp.now(global_io, .awake);
            fs_shim.cwd().rename(tmp_out_path, out_file_path) catch |err| {
                fs_shim.cwd().deleteFile(tmp_out_path) catch {};
                return err;
            };
            timings.atomic_install_ns += @intCast(install_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toNanoseconds());
            const zig_ms = zig_dur.toMilliseconds();
            if (zig_ms >= 1000) {
                const zig_sec = @as(f64, @floatFromInt(zig_ms)) / 1000.0;
                std.debug.print("  ✓ Built {s} in {d:.2}s\n", .{ t.name, zig_sec });
            } else {
                std.debug.print("  ✓ Built {s} in {d}ms\n", .{ t.name, zig_ms });
            }
        }
    }
    if (any_failed) return error.BuildFailed;
}

fn handleInit(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const template = if (args.len > 0) args[0] else "app";
    std.debug.print("Welcome to CAP (Concurrent Agent Packager)!\n", .{});
    std.debug.print("Initializing project with '{s}' template...\n\n", .{template});

    // Create src and test directories
    fs_shim.cwd().createDirPath(global_io, "src") catch {};
    fs_shim.cwd().createDirPath(global_io, "test") catch {};

    const cwd_path = try fs_shim.cwd().realpathAlloc(allocator, ".");
    defer allocator.free(cwd_path);
    const basename = std.Io.Dir.path.basename(cwd_path);

    // Create cap.json
    if (fs_shim.cwd().statFile("cap.json")) |_| {
        std.debug.print("cap.json already exists.\n", .{});
    } else |_| {
        var file = try fs_shim.cwd().createFile("cap.json", .{});
        defer file.close(global_io);

        var json_content: []u8 = undefined;
        if (std.mem.eql(u8, template, "agent")) {
            json_content = try std.fmt.allocPrint(allocator,
                \\{{
                \\  "name": "{s}",
                \\  "ver": "0.1.0",
                \\  "description": "High-concurrency AI Agent project",
                \\  "main": "src/main.cp",
                \\  "mode": "parallel",
                \\  "scripts": {{
                \\    "start": "cap src/main.cp",
                \\    "build": "cap build -f",
                \\    "test": "cap test"
                \\  }},
                \\  "dep": {{}},
                \\  "dev": {{}},
                \\  "build": {{
                \\    "opt": "fast",
                \\    "out_dir": "bin"
                \\  }},
                \\  "nest": {{
                \\    "mailbox_capacity": 65536,
                \\    "reduction_slice": 2000
                \\  }}
                \\}}
                \\
            , .{basename});
        } else if (std.mem.eql(u8, template, "lib")) {
            json_content = try std.fmt.allocPrint(allocator,
                \\{{
                \\  "name": "{s}",
                \\  "ver": "0.1.0",
                \\  "description": "Reusable CP Native library",
                \\  "main": "src/lib.cp",
                \\  "scripts": {{
                \\    "build": "cap build --lib",
                \\    "test": "cap test"
                \\  }},
                \\  "dep": {{}},
                \\  "dev": {{}},
                \\  "build": {{
                \\    "opt": "safe",
                \\    "out_dir": "bin"
                \\  }}
                \\}}
                \\
            , .{basename});
        } else {
            // Default "app"
            json_content = try std.fmt.allocPrint(allocator,
                \\{{
                \\  "name": "{s}",
                \\  "ver": "0.1.0",
                \\  "description": "Native CP Application",
                \\  "main": "src/main.cp",
                \\  "scripts": {{
                \\    "start": "cap src/main.cp",
                \\    "build": "cap build",
                \\    "test": "cap test"
                \\  }},
                \\  "dep": {{}},
                \\  "dev": {{}},
                \\  "build": {{
                \\    "opt": "safe",
                \\    "out_dir": "bin"
                \\  }}
                \\}}
                \\
            , .{basename});
        }
        defer allocator.free(json_content);
        try file.writeStreamingAll(global_io, json_content);
        std.debug.print("Created cap.json ({s} template)\n", .{template});
    }

    // Create entry file
    if (std.mem.eql(u8, template, "lib")) {
        if (fs_shim.cwd().statFile("src/lib.cp")) |_| {} else |_| {
            var file = try fs_shim.cwd().createFile("src/lib.cp", .{});
            defer file.close(global_io);
            try file.writeStreamingAll(global_io, "export fn add(a: i32, b: i32): i32 {\n    return a + b;\n}\n");
            std.debug.print("Created src/lib.cp\n", .{});
        }
    } else if (std.mem.eql(u8, template, "agent")) {
        if (fs_shim.cwd().statFile("src/main.cp")) |_| {} else |_| {
            var file = try fs_shim.cwd().createFile("src/main.cp", .{});
            defer file.close(global_io);
            const agent_code =
                \\import { print } from "cap:io";
                \\
                \\fn main(): ?void {
                \\    @print("Starting Nest AI Agent... 🚀");
                \\
                \\    const worker = @spawn((msg) => {
                \\        @print("Agent Worker spawned and running! 🤖");
                \\    });
                \\
                \\    @print("Agent initialized successfully. ✓");
                \\}
                \\
            ;
            try file.writeStreamingAll(global_io, agent_code);
            std.debug.print("Created src/main.cp (Agent template)\n", .{});
        }
    } else {
        if (fs_shim.cwd().statFile("src/main.cp")) |_| {} else |_| {
            var file = try fs_shim.cwd().createFile("src/main.cp", .{});
            defer file.close(global_io);
            try file.writeStreamingAll(global_io, "import { print } from \"cap:io\";\n\nfn main(): ?void {\n    @print(\"Hello from CP! 🚀\");\n}\n");
            std.debug.print("Created src/main.cp\n", .{});
        }
    }

    // Create default test file
    if (fs_shim.cwd().statFile("test/test_basic.cp")) |_| {} else |_| {
        var file = try fs_shim.cwd().createFile("test/test_basic.cp", .{});
        defer file.close(global_io);
        try file.writeStreamingAll(global_io, "import { print } from \"cap:io\";\n\nfn main(): ?void {\n    @print(\"Basic test passed! ✓\");\n}\n");
        std.debug.print("Created test/test_basic.cp\n", .{});
    }

    // Create .gitignore
    if (fs_shim.cwd().statFile(".gitignore")) |_| {
        std.debug.print(".gitignore already exists.\n", .{});
    } else |_| {
        var file = try fs_shim.cwd().createFile(".gitignore", .{});
        defer file.close(global_io);
        try file.writeStreamingAll(global_io, ".cap/\nbin/\ncap_modules/\n*.cp.zig\n");
        std.debug.print("Created .gitignore\n", .{});
    }

    std.debug.print("\nDone! Run `cap start` to run your project, or `cap test` to run tests.\n", .{});
}

pub const FuzzOptions = struct {
    is_fuzz: bool = false,
    is_modular: bool = false,
    timeout: ?[]const u8 = null,
    jobs: ?[]const u8 = null,
    repro: ?[]const u8 = null,
};

fn handleTest(allocator: std.mem.Allocator, args: []const []const u8) !void {
    try handleTestWithFuzz(allocator, args, false);
}

fn handleFuzz(allocator: std.mem.Allocator, args: []const []const u8) !void {
    try handleTestWithFuzz(allocator, args, true);
}

fn handleTestWithFuzz(allocator: std.mem.Allocator, args: []const []const u8, default_fuzz: bool) !void {
    var fuzz_opts = FuzzOptions{ .is_fuzz = default_fuzz };
    var test_target: []const u8 = "test";

    var i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (std.mem.eql(u8, arg, "--fuzz")) {
            fuzz_opts.is_fuzz = true;
        } else if (std.mem.eql(u8, arg, "-m") or std.mem.eql(u8, arg, "--modular")) {
            fuzz_opts.is_modular = true;
        } else if (std.mem.eql(u8, arg, "--timeout")) {
            if (i + 1 < args.len) {
                i += 1;
                fuzz_opts.timeout = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--timeout=")) {
            fuzz_opts.timeout = arg["--timeout=".len..];
        } else if (std.mem.eql(u8, arg, "--jobs")) {
            if (i + 1 < args.len) {
                i += 1;
                fuzz_opts.jobs = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--jobs=")) {
            fuzz_opts.jobs = arg["--jobs=".len..];
        } else if (std.mem.eql(u8, arg, "--repro")) {
            fuzz_opts.is_fuzz = true;
            if (i + 1 < args.len) {
                i += 1;
                fuzz_opts.repro = args[i];
            }
        } else if (std.mem.startsWith(u8, arg, "--repro=")) {
            fuzz_opts.is_fuzz = true;
            fuzz_opts.repro = arg["--repro=".len..];
        } else if (!std.mem.startsWith(u8, arg, "-")) {
            test_target = arg;
        }
    }

    if (fuzz_opts.is_fuzz) {
        std.debug.print(
            \\  ██████╗      ██╗     ██████╗ 
            \\ ██╔════╝     ████╗    ██╔══██╗   CAP libFuzzer Engine v0.1.0
            \\ ██║         ██╔═██╗   ██████╔╝   Fast & Native Coverage-Guided Fuzzing
            \\ ██║        ██╔╝ ╚██╗  ██╔═══╝    [ AddressSanitizer • Mutation Loop • Reproducer ]
            \\ ╚██████╗  ██╔╝   ╚██╗ ██║        CAP : Concurrent Application Plus
            \\  ╚═════╝  ╚═╝     ╚═╝ ╚═╝
            \\
        , .{});
    } else {
        std.debug.print(
            \\  ██████╗      ██╗     ██████╗ 
            \\ ██╔════╝     ████╗    ██╔══██╗   CAP Test Runner v0.1.0
            \\ ██║         ██╔═██╗   ██████╔╝   Fast & Native CP Testing Engine
            \\ ██║        ██╔╝ ╚██╗  ██╔═══╝    [ Test Discovery • Parallel • Debug Mode ]
            \\ ╚██████╗  ██╔╝   ╚██╗ ██║        CAP : Concurrent Application Plus
            \\  ╚═════╝  ╚═╝     ╚═╝ ╚═╝
            \\
        , .{});
    }

    const stat = fs_shim.cwd().statFile(test_target) catch {
        std.debug.print("Error: Test target '{s}' not found.\n", .{test_target});
        return;
    };

    var test_files: std.ArrayList([]const u8) = .empty;
    defer {
        for (test_files.items) |f| allocator.free(f);
        test_files.deinit(allocator);
    }

    if (stat.kind == .directory) {
        try collectTestFiles(allocator, test_target, &test_files);
    } else if (std.mem.endsWith(u8, test_target, ".cp")) {
        try test_files.append(allocator, try allocator.dupe(u8, test_target));
    }

    if (test_files.items.len == 0) {
        std.debug.print("No test files found in '{s}'.\n", .{test_target});
        return;
    }

    if (fuzz_opts.is_fuzz) {
        std.debug.print("Discovered {d} fuzz target(s). Running with LLVM libFuzzer + ASan...\n\n", .{test_files.items.len});
    } else if (fuzz_opts.is_modular) {
        std.debug.print("Discovered {d} test file(s). Running in Debug mode (modular .cap/test-mod/)...\n\n", .{test_files.items.len});
    } else {
        std.debug.print("Discovered {d} test file(s). Running in Debug mode...\n\n", .{test_files.items.len});
    }

    var passed_count: usize = 0;
    var failed_count: usize = 0;
    const start_ts = std.Io.Timestamp.now(global_io, .awake);

    for (test_files.items) |tfile| {
        const file_start = std.Io.Timestamp.now(global_io, .awake);
        if (fuzz_opts.is_fuzz) {
            std.debug.print("  ▶ [libFuzzer] Fuzzing {s} ...\n", .{tfile});
        } else {
            std.debug.print("  ▶ Running {s} ... ", .{tfile});
        }

        const run_res = if (fuzz_opts.is_fuzz)
            runFuzzSingleFile(allocator, tfile, fuzz_opts)
        else
            runTestSingleFile(allocator, tfile, fuzz_opts.is_modular);

        const duration = file_start.durationTo(std.Io.Timestamp.now(global_io, .awake)).toMilliseconds();

        if (run_res) |_| {
            passed_count += 1;
            std.debug.print("✓ PASSED ({d}ms)\n", .{duration});
        } else |err| {
            failed_count += 1;
            std.debug.print("✗ FAILED ({s})\n", .{@errorName(err)});
        }
    }

    const total_time = start_ts.durationTo(std.Io.Timestamp.now(global_io, .awake)).toMilliseconds();
    std.debug.print("\n--------------------------------------------------\n", .{});
    const label: []const u8 = if (fuzz_opts.is_fuzz) "Fuzzing" else "Tests";
    std.debug.print("{s}:  {d} passed, {d} failed, {d} total\n", .{ label, passed_count, failed_count, test_files.items.len });
    std.debug.print("Time:   {d}ms\n", .{total_time});
    std.debug.print("--------------------------------------------------\n", .{});

    if (failed_count > 0) return error.TestsFailed;
}

fn collectTestFiles(allocator: std.mem.Allocator, dir_path: []const u8, list: *std.ArrayList([]const u8)) !void {
    var dir = fs_shim.cwd().openDir(dir_path, .{ .iterate = true }) catch return;
    defer dir.close(global_io);
    var it = dir.iterate();
    while (try it.next(global_io)) |entry| {
        const child_path = try std.Io.Dir.path.join(allocator, &[_][]const u8{ dir_path, entry.name });
        if (entry.kind == .directory) {
            if (std.mem.eql(u8, entry.name, "fixtures") or std.mem.eql(u8, entry.name, ".cap") or std.mem.eql(u8, entry.name, "temp")) {
                allocator.free(child_path);
                continue;
            }
            try collectTestFiles(allocator, child_path, list);
            allocator.free(child_path);
        } else if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".cp") and !std.mem.endsWith(u8, entry.name, ".d.cp")) {
            try list.append(allocator, child_path);
        } else {
            allocator.free(child_path);
        }
    }
}

fn runTestSingleFile(allocator: std.mem.Allocator, filename: []const u8, is_modular: bool) !void {
    const base_name = filename[0 .. filename.len - 3];
    const test_mod_dir = ".cap/test-mod";
    var zig_file_buf: ?[]const u8 = null;
    defer if (zig_file_buf) |zf| allocator.free(zf);

    const cpc_bin = try getCpcBin(allocator);
    defer allocator.free(cpc_bin);

    if (is_modular) {
        fs_shim.cwd().createDirPath(global_io, test_mod_dir) catch {};
        const cpc_args = &[_][]const u8{
            cpc_bin,
            "compile",
            "--emit-dir",
            test_mod_dir,
            filename,
            "-I",
            "cap_modules",
            "-I",
            "/data/dev/cp/cap/cap_modules",
            "-I",
            "/data/dev/cp/lib",
        };
        var compile_child = try std.process.spawn(global_io, .{ .argv = cpc_args });
        const compile_res = try compile_child.wait(global_io);
        switch (compile_res) {
            .exited => |code| if (code != 0) return error.CompileFailed,
            else => return error.CompileFailed,
        }

        const entry_stem = std.Io.Dir.path.stem(filename);
        zig_file_buf = try resolveModularEntryFile(allocator, test_mod_dir, entry_stem);
        if (zig_file_buf == null) {
            std.debug.print("Error: --modular test compilation did not produce {s}/__cp_entry.zig or {s}/{s}.zig\n", .{ test_mod_dir, test_mod_dir, entry_stem });
            return error.CompileFailed;
        }
    } else {
        const single_zig = try std.fmt.allocPrint(allocator, ".cap/zig/{s}.zig", .{base_name});
        zig_file_buf = single_zig;

        if (std.Io.Dir.path.dirname(single_zig)) |dirname| {
            fs_shim.cwd().createDirPath(global_io, dirname) catch {};
        }

        const cpc_args = &[_][]const u8{
            cpc_bin,
            "compile",
            filename,
            "--output",
            single_zig,
            "-I",
            "cap_modules",
            "-I",
            "/data/dev/cp/cap/cap_modules",
            "-I",
            "/data/dev/cp/lib",
        };
        var compile_child = try std.process.spawn(global_io, .{ .argv = cpc_args });
        const compile_res = try compile_child.wait(global_io);
        switch (compile_res) {
            .exited => |code| if (code != 0) return error.CompileFailed,
            else => return error.CompileFailed,
        }
    }

    const zig_file = zig_file_buf.?;

    const bin_path = try std.fmt.allocPrint(allocator, "bin/test/{s}", .{std.Io.Dir.path.basename(base_name)});
    defer allocator.free(bin_path);
    if (std.Io.Dir.path.dirname(bin_path)) |dirname| {
        fs_shim.cwd().createDirPath(global_io, dirname) catch {};
    }

    const emit_arg = try std.fmt.allocPrint(allocator, "-femit-bin={s}", .{bin_path});
    defer allocator.free(emit_arg);
    fs_shim.cwd().createDirPath(global_io, ".cap/zig-cache") catch {};

    var mf_opt: ?manifest.Manifest = null;
    var mf_content: ?[]const u8 = null;
    defer if (mf_content) |c| allocator.free(c);
    if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
        mf_content = content;
        if (manifest.Manifest.parse(allocator, content)) |mf| {
            mf_opt = mf;
        } else |_| {}
    } else |_| {}
    defer if (mf_opt) |*m| m.deinit(allocator);

    var build_args_buf: [100][]const u8 = undefined;
    var build_argc: usize = 0;
    build_args_buf[build_argc] = "zig";
    build_argc += 1;
    build_args_buf[build_argc] = "build-exe";
    build_argc += 1;
    build_args_buf[build_argc] = zig_file;
    build_argc += 1;
    build_args_buf[build_argc] = emit_arg;
    build_argc += 1;
    build_args_buf[build_argc] = "--cache-dir";
    build_argc += 1;
    build_args_buf[build_argc] = ".cap/zig-cache";
    build_argc += 1;
    build_args_buf[build_argc] = "-O";
    build_argc += 1;
    build_args_buf[build_argc] = "Debug";
    build_argc += 1;
    build_args_buf[build_argc] = "-lc";
    build_argc += 1;

    var allocated_libs: [64][]const u8 = undefined;
    var allocated_libs_len: usize = 0;
    defer for (allocated_libs[0..allocated_libs_len]) |al| allocator.free(al);

    const link_scan_file = if (is_modular) filename else zig_file;
    try appendLinkArgs(allocator, &build_args_buf, &build_argc, &allocated_libs, &allocated_libs_len, mf_opt, "native", "ReleaseSafe", link_scan_file, ".cap/zig-cache");
    scanZigFileForLinkPragmas(allocator, zig_file, &build_args_buf, &build_argc, &allocated_libs, &allocated_libs_len);
    if (is_modular) {
        scanModularManifestForLinkPragmas(allocator, test_mod_dir, &build_args_buf, &build_argc, &allocated_libs, &allocated_libs_len);
    }

    var build_child = try std.process.spawn(global_io, .{ .argv = build_args_buf[0..build_argc] });
    const build_res = try build_child.wait(global_io);
    switch (build_res) {
        .exited => |code| if (code != 0) return error.BuildFailed,
        else => return error.BuildFailed,
    }

    // Run test binary
    var test_child = try std.process.spawn(global_io, .{ .argv = &[_][]const u8{bin_path} });
    const test_res = try test_child.wait(global_io);
    switch (test_res) {
        .exited => |code| if (code != 0) return error.TestExecutionFailed,
        else => return error.TestExecutionFailed,
    }
}

fn runFuzzSingleFile(allocator: std.mem.Allocator, filename: []const u8, fuzz_opts: FuzzOptions) !void {
    const base_name = filename[0 .. filename.len - 3];
    const zig_file = try std.fmt.allocPrint(allocator, ".cap/zig/{s}.zig", .{base_name});
    defer allocator.free(zig_file);

    if (std.Io.Dir.path.dirname(zig_file)) |dirname| {
        fs_shim.cwd().createDirPath(global_io, dirname) catch {};
    }

    // 1. Compile CP source with cpc --fuzz
    const cpc_bin = try getCpcBin(allocator);
    defer allocator.free(cpc_bin);
    const cpc_args = &[_][]const u8{
        cpc_bin,
        "compile",
        filename,
        "--output",
        zig_file,
        "--fuzz",
        "-I",
        "/data/dev/cp/cap/cap_modules",
        "-I",
        "/data/dev/cp/lib",
    };
    var compile_child = try std.process.spawn(global_io, .{ .argv = cpc_args });
    const compile_res = try compile_child.wait(global_io);
    if (compile_res.exited != 0) return error.CompileFailed;

    // 2. Build object file with zig build-obj
    const obj_path = try std.fmt.allocPrint(allocator, ".cap/obj/{s}.o", .{std.Io.Dir.path.basename(base_name)});
    defer allocator.free(obj_path);
    if (std.Io.Dir.path.dirname(obj_path)) |dirname| {
        fs_shim.cwd().createDirPath(global_io, dirname) catch {};
    }

    const emit_obj_arg = try std.fmt.allocPrint(allocator, "-femit-bin={s}", .{obj_path});
    defer allocator.free(emit_obj_arg);
    fs_shim.cwd().createDirPath(global_io, ".cap/zig-cache") catch {};

    const zig_obj_args = &[_][]const u8{
        "zig",
        "build-obj",
        zig_file,
        emit_obj_arg,
        "--cache-dir",
        ".cap/zig-cache",
        "-O",
        "Debug",
        "-lc",
    };
    var build_child = try std.process.spawn(global_io, .{ .argv = zig_obj_args });
    const build_res = try build_child.wait(global_io);
    if (build_res.exited != 0) return error.BuildFailed;

    // 3. Link with clang -fsanitize=fuzzer,address
    const bin_path = try std.fmt.allocPrint(allocator, "bin/test/{s}_fuzz", .{std.Io.Dir.path.basename(base_name)});
    defer allocator.free(bin_path);
    if (std.Io.Dir.path.dirname(bin_path)) |dirname| {
        fs_shim.cwd().createDirPath(global_io, dirname) catch {};
    }

    var link_args_buf: [100][]const u8 = undefined;
    var link_argc: usize = 0;
    link_args_buf[link_argc] = "clang";
    link_argc += 1;
    link_args_buf[link_argc] = "-fsanitize=fuzzer,address";
    link_argc += 1;
    link_args_buf[link_argc] = obj_path;
    link_argc += 1;
    link_args_buf[link_argc] = "-o";
    link_argc += 1;
    link_args_buf[link_argc] = bin_path;
    link_argc += 1;

    // C dependencies if available
    if (fs_shim.cwd().statFile("zig-out/lib/libusockets.a") catch null != null) {
        link_args_buf[link_argc] = "-Lzig-out/lib";
        link_argc += 1;
        link_args_buf[link_argc] = "-lusockets";
        link_argc += 1;
    }

    // Also support custom link_libs from cap.json if present
    var mf_opt: ?manifest.Manifest = null;
    if (fs_shim.cwd().readFileAlloc(allocator, "cap.json", 1024 * 1024)) |content| {
        defer allocator.free(content);
        if (manifest.Manifest.parse(allocator, content)) |mf| {
            mf_opt = mf;
        } else |_| {}
    } else |_| {}
    defer if (mf_opt) |*m| m.deinit(allocator);

    var allocated_libs: [64][]const u8 = undefined;
    var allocated_libs_len: usize = 0;
    defer for (allocated_libs[0..allocated_libs_len]) |al| allocator.free(al);

    try appendLinkArgs(allocator, &link_args_buf, &link_argc, &allocated_libs, &allocated_libs_len, mf_opt, "native", "ReleaseSafe", zig_file, ".cap/zig-cache");
    scanZigFileForLinkPragmas(allocator, zig_file, &link_args_buf, &link_argc, &allocated_libs, &allocated_libs_len);

    var link_child = try std.process.spawn(global_io, .{ .argv = link_args_buf[0..link_argc] });
    const link_res = try link_child.wait(global_io);
    if (link_res.exited != 0) return error.LinkFailed;

    // 4. Execute: Reproduce mode or Fuzzing mode
    if (fuzz_opts.repro) |repro_file| {
        std.debug.print("  [libFuzzer Repro] Executing crash artifact: {s} ...\n", .{repro_file});
        var repro_child = try std.process.spawn(global_io, .{ .argv = &[_][]const u8{ bin_path, repro_file } });
        const repro_res = try repro_child.wait(global_io);
        if (repro_res.exited != 0) {
            std.debug.print("  [libFuzzer Repro] Crash successfully reproduced! (exit code: {d})\n", .{repro_res.exited});
        } else {
            std.debug.print("  [libFuzzer Repro] Input finished cleanly (no crash triggered).\n", .{});
        }
        return;
    }

    var run_args_buf: [16][]const u8 = undefined;
    var run_argc: usize = 0;
    run_args_buf[run_argc] = bin_path;
    run_argc += 1;

    var timeout_arg_buf: ?[]const u8 = null;
    defer if (timeout_arg_buf) |tab| allocator.free(tab);
    var max_time_buf: ?[]const u8 = null;
    defer if (max_time_buf) |mtb| allocator.free(mtb);

    if (fuzz_opts.timeout) |t| {
        timeout_arg_buf = try std.fmt.allocPrint(allocator, "-timeout={s}", .{t});
        run_args_buf[run_argc] = timeout_arg_buf.?;
        run_argc += 1;
        max_time_buf = try std.fmt.allocPrint(allocator, "-max_total_time={s}", .{t});
        run_args_buf[run_argc] = max_time_buf.?;
        run_argc += 1;
    } else {
        run_args_buf[run_argc] = "-runs=1000";
        run_argc += 1;
    }

    var jobs_arg_buf: ?[]const u8 = null;
    defer if (jobs_arg_buf) |jab| allocator.free(jab);
    if (fuzz_opts.jobs) |j| {
        jobs_arg_buf = try std.fmt.allocPrint(allocator, "-jobs={s}", .{j});
        run_args_buf[run_argc] = jobs_arg_buf.?;
        run_argc += 1;
    }

    var fuzz_child = try std.process.spawn(global_io, .{ .argv = run_args_buf[0..run_argc] });
    const fuzz_res = try fuzz_child.wait(global_io);
    if (fuzz_res.exited != 0) return error.FuzzExecutionFailed;
}

fn handleBench(allocator: std.mem.Allocator, args: []const []const u8) !void {
    if (args.len == 0) {
        std.debug.print("Usage: cap bench <bench_file.cp>\n", .{});
        return;
    }
    const target = args[0];
    std.debug.print(
        \\  ██████╗      ██╗     ██████╗ 
        \\ ██╔════╝     ████╗    ██╔══██╗   CAP Benchmark Suite v0.1.0
        \\ ██║         ██╔═██╗   ██████╔╝   Microsecond State Machine Profiler
        \\ ██║        ██╔╝ ╚██╗  ██╔═══╝    [ ReleaseFast • High-Precision Timing ]
        \\ ╚██████╗  ██╔╝   ╚██╗ ██║        CAP : Concurrent Application Plus
        \\  ╚═════╝  ╚═╝     ╚═╝ ╚═╝
        \\
    , .{});
    std.debug.print("Benchmarking target: {s} (ReleaseFast)...\n\n", .{target});

    if (std.mem.endsWith(u8, target, ".cp")) {
        const start_ts = std.Io.Timestamp.now(global_io, .awake);
        try runFileWithOpt(allocator, target, "ReleaseFast");
        const elapsed_dur = start_ts.durationTo(std.Io.Timestamp.now(global_io, .awake));
        const elapsed_ns = elapsed_dur.toNanoseconds();
        const elapsed_ms: f64 = @as(f64, @floatFromInt(elapsed_ns)) / 1_000_000.0;
        std.debug.print("\nBenchmark completed in {d:.2} ms ({d} ns)\n", .{ elapsed_ms, elapsed_ns });
    } else {
        std.debug.print("Target must be a .cp file: cap bench <file.cp>\n", .{});
    }
}

fn handleDemangle(allocator: std.mem.Allocator, args: []const []const u8) !void {
    const cpc_bin = try getCpcBin(allocator);
    defer allocator.free(cpc_bin);
    var cmd_list: [16][]const u8 = undefined;
    var argc: usize = 0;
    cmd_list[argc] = cpc_bin;
    argc += 1;
    cmd_list[argc] = "demangle";
    argc += 1;
    for (args) |arg| {
        if (argc < 16) {
            cmd_list[argc] = arg;
            argc += 1;
        }
    }
    var child = try std.process.spawn(global_io, .{ .argv = cmd_list[0..argc] });
    _ = try child.wait(global_io);
}
