const std = @import("std");

fn isDir(io: std.Io, path: []const u8) bool {
    if (std.fs.path.isAbsolute(path)) {
        if (std.Io.Dir.openDirAbsolute(io, path, .{})) |d_val| {
            var d = d_val;
            d.close(io);
            return true;
        } else |_| return false;
    }
    if (std.Io.Dir.openDir(.cwd(), io, path, .{})) |d_val| {
        var d = d_val;
        d.close(io);
        return true;
    } else |_| return false;
}

fn dirExistsIn(allocator: std.mem.Allocator, io: std.Io, cap_root: []const u8, pkg: []const u8) !bool {
    const p1 = try std.fmt.allocPrint(allocator, "{s}/packages/{s}", .{ cap_root, pkg });
    defer allocator.free(p1);
    if (isDir(io, p1)) return true;

    const p2 = try std.fmt.allocPrint(allocator, "packages/{s}", .{pkg});
    defer allocator.free(p2);
    if (isDir(io, p2)) return true;

    return false;
}

/// Resolve legacy `@cap/<name>` to bare `<orig>` if a bare package exists (supporting _/- normalization)
/// and `@cap/<name>` is NOT a native C/C++ package under `packages/@cap/<name>`.
fn resolveLegacyCapPkg(allocator: std.mem.Allocator, io: std.Io, cap_root: []const u8, sub_name: []const u8) !?[]u8 {
    // If it still exists under packages/@cap/<sub_name>, keep it as @cap/<sub_name>.
    const native_cap = try std.fmt.allocPrint(allocator, "@cap/{s}", .{sub_name});
    defer allocator.free(native_cap);
    if (try dirExistsIn(allocator, io, cap_root, native_cap)) {
        return null;
    }

    // 1. Exact bare name
    if (try dirExistsIn(allocator, io, cap_root, sub_name)) {
        return try allocator.dupe(u8, sub_name);
    }

    // 2. '_' -> '-'
    if (std.mem.indexOfScalar(u8, sub_name, '_') != null) {
        const alt = try allocator.dupe(u8, sub_name);
        for (alt) |*c| {
            if (c.* == '_') c.* = '-';
        }
        if (try dirExistsIn(allocator, io, cap_root, alt)) return alt;
        allocator.free(alt);
    }

    // 3. '-' -> '_'
    if (std.mem.indexOfScalar(u8, sub_name, '-') != null) {
        const alt = try allocator.dupe(u8, sub_name);
        for (alt) |*c| {
            if (c.* == '-') c.* = '_';
        }
        if (try dirExistsIn(allocator, io, cap_root, alt)) return alt;
        allocator.free(alt);
    }

    return null;
}

fn fixSingleFile(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    file_path: []const u8,
    dry_run: bool,
) !usize {
    const content = std.Io.Dir.readFileAlloc(.cwd(), io, file_path, allocator, @fromBackingInt(@intCast(16 * 1024 * 1024))) catch |err| {
        const stderr = std.Io.File.stderr();
        const msg = try std.fmt.allocPrint(allocator, "cap fix: cannot read '{s}': {}\n", .{ file_path, err });
        defer allocator.free(msg);
        stderr.writeStreamingAll(io, msg) catch {};
        return 0;
    };
    defer allocator.free(content);

    var out_buf: std.ArrayList(u8) = .empty;
    defer out_buf.deinit(allocator);

    var fixes: usize = 0;
    var i: usize = 0;
    const needle = "\"@cap/";

    while (i < content.len) {
        if (std.mem.indexOfPos(u8, content, i, needle)) |pos| {
            const name_start = pos + needle.len;
            if (std.mem.indexOfScalarPos(u8, content, name_start, '"')) |name_end| {
                const sub_name = content[name_start..name_end];
                // Only rewrite clean package specifiers without slashes or spaces
                if (sub_name.len > 0 and std.mem.indexOfAny(u8, sub_name, "/ \t\r\n") == null) {
                    if (try resolveLegacyCapPkg(allocator, io, cap_root, sub_name)) |bare| {
                        defer allocator.free(bare);
                        try out_buf.appendSlice(allocator, content[i..pos]);
                        try out_buf.append(allocator, '"');
                        try out_buf.appendSlice(allocator, bare);
                        try out_buf.append(allocator, '"');
                        fixes += 1;
                        i = name_end + 1;
                        continue;
                    }
                }
            }
            try out_buf.appendSlice(allocator, content[i .. pos + needle.len]);
            i = pos + needle.len;
        } else {
            try out_buf.appendSlice(allocator, content[i..]);
            break;
        }
    }

    if (fixes > 0 and !dry_run) {
        try std.Io.Dir.writeFile(.cwd(), io, .{
            .sub_path = file_path,
            .data = out_buf.items,
        });
    }

    return fixes;
}

pub fn handleFix(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    args: []const []const u8,
) !void {
    const out = std.Io.File.stdout();
    var dry_run = false;
    var targets: std.ArrayList([]const u8) = .empty;
    defer targets.deinit(allocator);

    for (args) |arg| {
        if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            out.writeStreamingAll(io,
                \\Usage: cap fix [--dry-run] <file.cp|dir> [...]
                \\  AI-First auto-fixer for CP projects & packages:
                \\    - Rewrites legacy "@cap/<pkg>" bare-ecosystem imports to original bare names "<pkg>"
                \\      (preserves native C/C++ packages under packages/@cap/*).
                \\    - Supports "_" <-> "-" crate/npm name normalization automatically.
                \\
            ) catch {};
            return;
        } else if (std.mem.eql(u8, arg, "--dry-run")) {
            dry_run = true;
        } else {
            try targets.append(allocator, arg);
        }
    }

    if (targets.items.len == 0) {
        try targets.append(allocator, ".");
    }

    var total_files: usize = 0;
    var total_fixes: usize = 0;

    for (targets.items) |target| {
        if (std.mem.endsWith(u8, target, ".cp")) {
            const n = try fixSingleFile(allocator, io, cap_root, target, dry_run);
            if (n > 0) {
                total_files += 1;
                total_fixes += n;
                const line = try std.fmt.allocPrint(allocator, "  {s}: {d} import(s) {s}\n", .{
                    target,
                    n,
                    if (dry_run) "would be fixed" else "fixed",
                });
                defer allocator.free(line);
                out.writeStreamingAll(io, line) catch {};
            }
        } else if (isDir(io, target)) {
            var dir = std.Io.Dir.openDir(.cwd(), io, target, .{ .iterate = true }) catch continue;
            defer dir.close(io);
            var walker = try dir.walk(allocator);
            defer walker.deinit();
            while (try walker.next(io)) |entry| {
                if (entry.kind != .file) continue;
                if (!std.mem.endsWith(u8, entry.path, ".cp")) continue;
                if (std.mem.endsWith(u8, entry.path, ".d.cp")) continue;
                const full = if (std.mem.eql(u8, target, "."))
                    try allocator.dupe(u8, entry.path)
                else
                    try std.fmt.allocPrint(allocator, "{s}/{s}", .{ target, entry.path });
                defer allocator.free(full);
                const n = try fixSingleFile(allocator, io, cap_root, full, dry_run);
                if (n > 0) {
                    total_files += 1;
                    total_fixes += n;
                    const line = try std.fmt.allocPrint(allocator, "  {s}: {d} import(s) {s}\n", .{
                        full,
                        n,
                        if (dry_run) "would be fixed" else "fixed",
                    });
                    defer allocator.free(line);
                    out.writeStreamingAll(io, line) catch {};
                }
            }
        }
    }

    const summary = try std.fmt.allocPrint(allocator, "cap fix: {d} fix(es) across {d} file(s){s}.\n", .{
        total_fixes,
        total_files,
        if (dry_run) " (dry-run)" else "",
    });
    defer allocator.free(summary);
    out.writeStreamingAll(io, summary) catch {};
}
