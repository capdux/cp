const std = @import("std");
const builtin = @import("builtin");
const main_mod = @import("../main.zig");

pub const ParallelObjOptions = struct {
    zig_root_dir: []const u8,
    obj_root_dir: []const u8,
    optimize_mode: []const u8,
    target_triple: []const u8,
    force_rebuild: bool = false,
};

pub const ParallelObjResult = struct {
    link_entry_zig: ?[]const u8 = null,
    obj_paths: std.ArrayList([]const u8),
    hits: usize = 0,
    misses: usize = 0,
    fallback_count: usize = 0,
    elapsed_ns: i128 = 0,
    active: bool = false,

    pub fn init() ParallelObjResult {
        return .{
            .link_entry_zig = null,
            .obj_paths = .empty,
            .hits = 0,
            .misses = 0,
            .fallback_count = 0,
            .elapsed_ns = 0,
            .active = false,
        };
    }

    pub fn deinit(self: *ParallelObjResult, allocator: std.mem.Allocator) void {
        if (self.link_entry_zig) |p| allocator.free(p);
        for (self.obj_paths.items) |p| allocator.free(p);
        self.obj_paths.deinit(allocator);
    }
};

const ModuleJob = struct {
    rel_zig_path: []const u8,
    orig_zig_path: []const u8,
    compile_entry_path: []const u8,
    obj_path: []const u8,
    stub_content: []const u8,
    cached: bool = false,
    compiled_ok: bool = false,
};

const WorkerContext = struct {
    allocator: std.mem.Allocator,
    jobs: []ModuleJob,
    pending_indices: []const usize,
    next_idx: *std.atomic.Value(usize),
    obj_root_dir: []const u8,
    optimize_mode: []const u8,
    target_triple: []const u8,
    pid: i64,
};

const WorkerArgs = struct {
    ctx: *const WorkerContext,
    worker_id: usize,
};

fn getCurrentPid() i64 {
    if (builtin.os.tag == .linux) {
        return @intCast(std.os.linux.getpid());
    }
    return std.Io.Timestamp.now(main_mod.global_io, .awake).nanoseconds;
}

fn trimRelPrefix(p: []const u8) []const u8 {
    var s = p;
    while (std.mem.startsWith(u8, s, "./")) {
        s = s[2..];
    }
    while (std.mem.startsWith(u8, s, "/")) {
        s = s[1..];
    }
    return s;
}

fn sanitizeModKey(allocator: std.mem.Allocator, rel_path: []const u8) ![]u8 {
    var trimmed = trimRelPrefix(rel_path);
    if (std.mem.endsWith(u8, trimmed, ".zig")) {
        trimmed = trimmed[0 .. trimmed.len - 4];
    }
    const out = try allocator.alloc(u8, trimmed.len);
    for (trimmed, 0..) |c, i| {
        if ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '-' or c == '_') {
            out[i] = c;
        } else {
            out[i] = '_';
        }
    }
    return out;
}

fn computeHash16(source: []const u8, opt_mode: []const u8, target_triple: []const u8) [16]u8 {
    var hasher = std.crypto.hash.sha2.Sha256.init(.{});
    hasher.update(source);
    hasher.update("\nopt=");
    hasher.update(opt_mode);
    hasher.update("\ntarget=");
    hasher.update(target_triple);
    hasher.update("\nv=1\n");
    var digest: [32]u8 = undefined;
    hasher.final(&digest);

    const hex_chars = "0123456789abcdef";
    var out: [16]u8 = undefined;
    for (0..8) |i| {
        out[i * 2] = hex_chars[(digest[i] >> 4) & 0x0f];
        out[i * 2 + 1] = hex_chars[digest[i] & 0x0f];
    }
    return out;
}

/// Checks if a generated modular `.zig` file is eligible for standalone `zig build-obj`
/// compilation and C-ABI `extern fn` stub generation.
/// Returns owned stub source text if eligible, or null to fall back cleanly to joint compilation.
fn analyzeModuleForObj(allocator: std.mem.Allocator, src: []const u8) !?[]u8 {
    var stub: std.ArrayList(u8) = .empty;
    errdefer stub.deinit(allocator);

    try stub.appendSlice(allocator, "// Auto-generated C-ABI extern stub (cap --parallel-obj)\n");

    var export_fn_count: usize = 0;
    var brace_depth: i32 = 0;
    var line_it = std.mem.splitScalar(u8, src, '\n');

    while (line_it.next()) |raw_line| {
        const line = std.mem.trim(u8, raw_line, " \t\r");
        if (line.len == 0 or std.mem.startsWith(u8, line, "//")) {
            continue;
        }

        const at_top_level = (brace_depth == 0);

        // Check for cross-module @import anywhere in the file
        if (std.mem.indexOf(u8, line, "@import(")) |imp_pos| {
            const after = line[imp_pos + "@import(".len ..];
            if (std.mem.indexOfScalar(u8, after, '"')) |q1| {
                const rest = after[q1 + 1 ..];
                if (std.mem.indexOfScalar(u8, rest, '"')) |q2| {
                    const target = rest[0..q2];
                    const is_std = std.mem.eql(u8, target, "std") or std.mem.eql(u8, target, "builtin");
                    const is_rt = std.mem.endsWith(u8, target, "__cp_runtime.zig");
                    if (!is_std and !is_rt) {
                        stub.deinit(allocator);
                        return null;
                    }
                }
            }
        }

        if (at_top_level) {
            if (std.mem.startsWith(u8, line, "pub export fn ")) {
                const open_brace = std.mem.indexOfScalar(u8, line, '{') orelse {
                    // Multi-line signature: conservatively fall back
                    stub.deinit(allocator);
                    return null;
                };
                const sig = std.mem.trim(u8, line["pub export fn ".len..open_brace], " \t");
                if (sig.len == 0) {
                    stub.deinit(allocator);
                    return null;
                }
                // Reject generic/comptime signatures
                if (std.mem.indexOf(u8, sig, "comptime") != null or
                    std.mem.indexOf(u8, sig, "anytype") != null or
                    std.mem.indexOf(u8, sig, ": type") != null)
                {
                    stub.deinit(allocator);
                    return null;
                }
                // Reject error-union return types (not C-ABI compatible)
                if (std.mem.indexOfScalar(u8, sig, '!') != null) {
                    stub.deinit(allocator);
                    return null;
                }
                try stub.appendSlice(allocator, "pub extern fn ");
                try stub.appendSlice(allocator, sig);
                try stub.appendSlice(allocator, ";\n");
                export_fn_count += 1;
            } else if (std.mem.startsWith(u8, line, "pub const ") or
                std.mem.startsWith(u8, line, "pub var ") or
                std.mem.startsWith(u8, line, "pub fn ") or
                std.mem.startsWith(u8, line, "pub usingnamespace"))
            {
                // Exports non-C-ABI symbols (types, generics, globals): fall back safely
                stub.deinit(allocator);
                return null;
            }
        }

        // Track brace depth (simple scan skipping string literals)
        var in_str = false;
        var escaped = false;
        var i: usize = 0;
        while (i < line.len) : (i += 1) {
            const ch = line[i];
            if (in_str) {
                if (escaped) {
                    escaped = false;
                } else if (ch == '\\') {
                    escaped = true;
                } else if (ch == '"') {
                    in_str = false;
                }
                continue;
            }
            if (ch == '/' and i + 1 < line.len and line[i + 1] == '/') {
                break;
            }
            if (ch == '"') {
                in_str = true;
            } else if (ch == '{') {
                brace_depth += 1;
            } else if (ch == '}') {
                brace_depth -= 1;
            }
        }
    }

    if (export_fn_count == 0) {
        stub.deinit(allocator);
        return null;
    }

    return try stub.toOwnedSlice(allocator);
}

fn runWorker(args: WorkerArgs) void {
    const ctx = args.ctx;
    const worker_id = args.worker_id;

    const worker_cache_dir = std.fmt.allocPrint(ctx.allocator, "{s}/cache/w{d}", .{ ctx.obj_root_dir, worker_id }) catch return;
    defer ctx.allocator.free(worker_cache_dir);
    main_mod.fs_shim.cwd().createDirPath(main_mod.global_io, worker_cache_dir) catch {};

    while (true) {
        const q_idx = ctx.next_idx.fetchAdd(1, .monotonic);
        if (q_idx >= ctx.pending_indices.len) break;

        const job_idx = ctx.pending_indices[q_idx];
        var job = &ctx.jobs[job_idx];

        const tmp_obj = std.fmt.allocPrint(ctx.allocator, "{s}.tmp.{d}.{d}", .{ job.obj_path, ctx.pid, worker_id }) catch continue;
        defer ctx.allocator.free(tmp_obj);
        const emit_arg = std.fmt.allocPrint(ctx.allocator, "-femit-bin={s}", .{tmp_obj}) catch continue;
        defer ctx.allocator.free(emit_arg);

        var argv_buf: [16][]const u8 = undefined;
        var argc: usize = 0;
        argv_buf[argc] = "zig";
        argc += 1;
        argv_buf[argc] = "build-obj";
        argc += 1;
        argv_buf[argc] = job.compile_entry_path;
        argc += 1;
        argv_buf[argc] = emit_arg;
        argc += 1;
        argv_buf[argc] = "--cache-dir";
        argc += 1;
        argv_buf[argc] = worker_cache_dir;
        argc += 1;
        argv_buf[argc] = "-O";
        argc += 1;
        argv_buf[argc] = ctx.optimize_mode;
        argc += 1;
        argv_buf[argc] = "-lc";
        argc += 1;
        if (ctx.target_triple.len > 0 and !std.mem.eql(u8, ctx.target_triple, "native")) {
            argv_buf[argc] = "-target";
            argc += 1;
            argv_buf[argc] = ctx.target_triple;
            argc += 1;
        }

        var child = std.process.spawn(main_mod.global_io, .{
            .argv = argv_buf[0..argc],
            .stdout = .ignore,
            .stderr = .ignore,
        }) catch continue;
        const res = child.wait(main_mod.global_io) catch continue;
        const ok = switch (res) {
            .exited => |code| code == 0,
            else => false,
        };
        if (ok) {
            main_mod.fs_shim.cwd().rename(tmp_obj, job.obj_path) catch {
                main_mod.fs_shim.cwd().deleteFile(tmp_obj) catch {};
                continue;
            };
            job.compiled_ok = true;
        } else {
            main_mod.fs_shim.cwd().deleteFile(tmp_obj) catch {};
        }
    }
}

fn writeFileIfChanged(allocator: std.mem.Allocator, dest_path: []const u8, content: []const u8) !void {
    if (main_mod.fs_shim.cwd().readFileAlloc(allocator, dest_path, 16 * 1024 * 1024)) |existing| {
        defer allocator.free(existing);
        if (std.mem.eql(u8, existing, content)) {
            return;
        }
    } else |_| {}

    if (std.Io.Dir.path.dirname(dest_path)) |parent| {
        main_mod.fs_shim.cwd().createDirPath(main_mod.global_io, parent) catch {};
    }
    var file = try main_mod.fs_shim.cwd().createFile(dest_path, .{});
    defer file.close(main_mod.global_io);
    try file.writeStreamingAll(main_mod.global_io, content);
}

fn syncViewTree(
    allocator: std.mem.Allocator,
    src_root: []const u8,
    dst_root: []const u8,
    rel_prefix: []const u8,
    stub_map: *const std.StringHashMap([]const u8),
) !void {
    const cur_src_dir = if (rel_prefix.len == 0)
        try allocator.dupe(u8, src_root)
    else
        try std.fmt.allocPrint(allocator, "{s}/{s}", .{ src_root, rel_prefix });
    defer allocator.free(cur_src_dir);

    var dir = main_mod.fs_shim.cwd().openDir(cur_src_dir, .{ .iterate = true }) catch return;
    defer dir.close(main_mod.global_io);

    var it = dir.iterate();
    while (it.next(main_mod.global_io) catch null) |entry| {
        if (std.mem.startsWith(u8, entry.name, ".cp_")) continue;

        const child_rel = if (rel_prefix.len == 0)
            try allocator.dupe(u8, entry.name)
        else
            try std.fmt.allocPrint(allocator, "{s}/{s}", .{ rel_prefix, entry.name });
        defer allocator.free(child_rel);

        if (entry.kind == .directory) {
            try syncViewTree(allocator, src_root, dst_root, child_rel, stub_map);
        } else if (entry.kind == .file or entry.kind == .sym_link) {
            if (!std.mem.endsWith(u8, entry.name, ".zig")) continue;

            const dst_file = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ dst_root, child_rel });
            defer allocator.free(dst_file);

            if (stub_map.get(child_rel)) |stub_text| {
                try writeFileIfChanged(allocator, dst_file, stub_text);
            } else {
                const src_file = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ src_root, child_rel });
                defer allocator.free(src_file);
                const bytes = main_mod.fs_shim.cwd().readFileAlloc(allocator, src_file, 16 * 1024 * 1024) catch continue;
                defer allocator.free(bytes);
                try writeFileIfChanged(allocator, dst_file, bytes);
            }
        }
    }
}

/// Prepares per-module content-hashed `.o` object files in parallel and constructs
/// a clean extern-stub link view so the final `zig build-exe` performs true incremental linking.
pub fn prepareParallelObjects(
    allocator: std.mem.Allocator,
    opts: ParallelObjOptions,
) !ParallelObjResult {
    var result = ParallelObjResult.init();
    errdefer result.deinit(allocator);

    const t0 = std.Io.Timestamp.now(main_mod.global_io, .awake);

    const manifest_path = try std.fmt.allocPrint(allocator, "{s}/.cp_modular_manifest.txt", .{opts.zig_root_dir});
    defer allocator.free(manifest_path);

    const manifest_content = main_mod.fs_shim.cwd().readFileAlloc(allocator, manifest_path, 1024 * 1024) catch {
        result.elapsed_ns = @intCast(t0.durationTo(std.Io.Timestamp.now(main_mod.global_io, .awake)).toNanoseconds());
        return result;
    };
    defer allocator.free(manifest_content);

    var entry_path_raw: ?[]const u8 = null;
    var entry_module_raw: ?[]const u8 = null;

    var rel_modules: std.ArrayList([]const u8) = .empty;
    defer rel_modules.deinit(allocator);

    var line_it = std.mem.splitScalar(u8, manifest_content, '\n');
    while (line_it.next()) |raw_line| {
        const line = std.mem.trim(u8, raw_line, " \t\r");
        if (std.mem.startsWith(u8, line, "entry=")) {
            entry_path_raw = line["entry=".len..];
        } else if (std.mem.startsWith(u8, line, "entry_module=")) {
            entry_module_raw = line["entry_module=".len..];
        } else if (std.mem.startsWith(u8, line, "module=")) {
            if (std.mem.indexOf(u8, line, " -> ")) |arrow| {
                const rel_zig = trimRelPrefix(line[arrow + " -> ".len ..]);
                if (rel_zig.len > 0) {
                    try rel_modules.append(allocator, rel_zig);
                }
            }
        }
    }

    if (entry_path_raw == null or rel_modules.items.len == 0) {
        result.elapsed_ns = @intCast(t0.durationTo(std.Io.Timestamp.now(main_mod.global_io, .awake)).toNanoseconds());
        return result;
    }

    // Derive entry_rel relative to zig_root_dir
    const entry_rel = blk: {
        const ep = entry_path_raw.?;
        const prefix = try std.fmt.allocPrint(allocator, "{s}/", .{opts.zig_root_dir});
        defer allocator.free(prefix);
        if (std.mem.startsWith(u8, ep, prefix)) {
            break :blk trimRelPrefix(ep[prefix.len..]);
        }
        break :blk std.Io.Dir.path.basename(ep);
    };

    const entry_mod_rel = blk: {
        if (entry_module_raw) |em| {
            const prefix = try std.fmt.allocPrint(allocator, "{s}/", .{opts.zig_root_dir});
            defer allocator.free(prefix);
            if (std.mem.startsWith(u8, em, prefix)) {
                break :blk trimRelPrefix(em[prefix.len..]);
            }
        }
        break :blk entry_rel;
    };

    main_mod.fs_shim.cwd().createDirPath(main_mod.global_io, opts.obj_root_dir) catch {};

    var jobs: std.ArrayList(ModuleJob) = .empty;
    defer {
        for (jobs.items) |j| {
            allocator.free(j.rel_zig_path);
            allocator.free(j.orig_zig_path);
            allocator.free(j.compile_entry_path);
            allocator.free(j.obj_path);
            allocator.free(j.stub_content);
        }
        jobs.deinit(allocator);
    }

    var pending_indices: std.ArrayList(usize) = .empty;
    defer pending_indices.deinit(allocator);

    const stage_root = try std.fmt.allocPrint(allocator, "{s}/stage", .{opts.obj_root_dir});
    defer allocator.free(stage_root);

    for (rel_modules.items) |rel_zig| {
        if (std.mem.eql(u8, rel_zig, entry_rel) or
            std.mem.eql(u8, rel_zig, entry_mod_rel) or
            std.mem.eql(u8, rel_zig, "__cp_entry.zig") or
            std.mem.eql(u8, rel_zig, "__cp_runtime.zig") or
            std.mem.startsWith(u8, rel_zig, "_lib/"))
        {
            continue;
        }

        const orig_zig_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ opts.zig_root_dir, rel_zig });
        const src = main_mod.fs_shim.cwd().readFileAlloc(allocator, orig_zig_path, 8 * 1024 * 1024) catch {
            allocator.free(orig_zig_path);
            result.fallback_count += 1;
            continue;
        };
        defer allocator.free(src);

        const stub_opt = try analyzeModuleForObj(allocator, src);
        if (stub_opt == null) {
            allocator.free(orig_zig_path);
            result.fallback_count += 1;
            continue;
        }

        const hash16 = computeHash16(src, opts.optimize_mode, opts.target_triple);
        const mod_key = try sanitizeModKey(allocator, rel_zig);
        defer allocator.free(mod_key);

        const obj_path = try std.fmt.allocPrint(allocator, "{s}/{s}_{s}.o", .{ opts.obj_root_dir, mod_key, hash16 });
        const compile_entry_path = try std.fmt.allocPrint(allocator, "{s}/__obj_{s}.zig", .{ stage_root, mod_key });
        const is_cached = blk: {
            if (opts.force_rebuild) break :blk false;
            const st = main_mod.fs_shim.cwd().statFile(obj_path) catch break :blk false;
            break :blk st.size > 0;
        };

        const job_idx = jobs.items.len;
        try jobs.append(allocator, .{
            .rel_zig_path = try allocator.dupe(u8, rel_zig),
            .orig_zig_path = orig_zig_path,
            .compile_entry_path = compile_entry_path,
            .obj_path = obj_path,
            .stub_content = stub_opt.?,
            .cached = is_cached,
            .compiled_ok = is_cached,
        });

        if (is_cached) {
            result.hits += 1;
        } else {
            result.misses += 1;
            try pending_indices.append(allocator, job_idx);
        }
    }

    if (jobs.items.len == 0) {
        result.elapsed_ns = @intCast(t0.durationTo(std.Io.Timestamp.now(main_mod.global_io, .awake)).toNanoseconds());
        return result;
    }

    // Parallel zig build-obj pool (capped at min(cpu_count, 8))
    if (pending_indices.items.len > 0) {
        main_mod.fs_shim.cwd().createDirPath(main_mod.global_io, stage_root) catch {};
        const rt_src_path = try std.fmt.allocPrint(allocator, "{s}/__cp_runtime.zig", .{opts.zig_root_dir});
        defer allocator.free(rt_src_path);
        const rt_dst_path = try std.fmt.allocPrint(allocator, "{s}/__cp_runtime.zig", .{stage_root});
        defer allocator.free(rt_dst_path);
        if (main_mod.fs_shim.cwd().readFileAlloc(allocator, rt_src_path, 16 * 1024 * 1024)) |rt_bytes| {
            defer allocator.free(rt_bytes);
            try writeFileIfChanged(allocator, rt_dst_path, rt_bytes);
        } else |_| {}

        for (pending_indices.items) |job_idx| {
            const j = jobs.items[job_idx];
            const stage_mod_path = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ stage_root, j.rel_zig_path });
            defer allocator.free(stage_mod_path);
            if (main_mod.fs_shim.cwd().readFileAlloc(allocator, j.orig_zig_path, 8 * 1024 * 1024)) |m_bytes| {
                defer allocator.free(m_bytes);
                try writeFileIfChanged(allocator, stage_mod_path, m_bytes);
            } else |_| {}

            const wrapper_src = try std.fmt.allocPrint(allocator, "comptime {{\n    _ = @import(\"./{s}\");\n}}\n", .{j.rel_zig_path});
            defer allocator.free(wrapper_src);
            try writeFileIfChanged(allocator, j.compile_entry_path, wrapper_src);
        }
        var next_idx = std.atomic.Value(usize).init(0);
        const ctx = WorkerContext{
            .allocator = allocator,
            .jobs = jobs.items,
            .pending_indices = pending_indices.items,
            .next_idx = &next_idx,
            .obj_root_dir = opts.obj_root_dir,
            .optimize_mode = opts.optimize_mode,
            .target_triple = opts.target_triple,
            .pid = getCurrentPid(),
        };

        const cpu_count = std.Thread.getCpuCount() catch 4;
        const max_workers = @min(cpu_count, 8);
        const worker_count = @max(@as(usize, 1), @min(max_workers, pending_indices.items.len));

        if (worker_count == 1) {
            runWorker(.{ .ctx = &ctx, .worker_id = 0 });
        } else {
            var threads: [8]?std.Thread = .{ null, null, null, null, null, null, null, null };
            for (0..worker_count) |w| {
                threads[w] = std.Thread.spawn(.{}, runWorker, .{WorkerArgs{ .ctx = &ctx, .worker_id = w }}) catch null;
            }
            for (0..worker_count) |w| {
                if (threads[w]) |t| t.join();
            }
            // Drain any remaining items if any thread spawn failed
            runWorker(.{ .ctx = &ctx, .worker_id = 0 });
        }
    }

    // Build stub map for all successfully compiled / cached modules
    var stub_map = std.StringHashMap([]const u8).init(allocator);
    defer stub_map.deinit();

    for (jobs.items) |j| {
        if (j.compiled_ok) {
            try stub_map.put(j.rel_zig_path, j.stub_content);
            try result.obj_paths.append(allocator, try allocator.dupe(u8, j.obj_path));
        } else {
            result.fallback_count += 1;
        }
    }

    if (result.obj_paths.items.len == 0) {
        result.elapsed_ns = @intCast(t0.durationTo(std.Io.Timestamp.now(main_mod.global_io, .awake)).toNanoseconds());
        return result;
    }

    const view_root = try std.fmt.allocPrint(allocator, "{s}/view", .{opts.obj_root_dir});
    defer allocator.free(view_root);
    main_mod.fs_shim.cwd().createDirPath(main_mod.global_io, view_root) catch {};

    try syncViewTree(allocator, opts.zig_root_dir, view_root, "", &stub_map);

    result.link_entry_zig = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ view_root, entry_rel });
    result.active = true;
    result.elapsed_ns = @intCast(t0.durationTo(std.Io.Timestamp.now(main_mod.global_io, .awake)).toNanoseconds());
    return result;
}
