const std = @import("std");

fn openPackageDir(io: std.Io, path: []const u8) ?std.Io.Dir {
    if (std.fs.path.isAbsolute(path)) {
        return std.Io.Dir.openDirAbsolute(io, path, .{}) catch null;
    }
    return std.Io.Dir.openDir(.cwd(), io, path, .{}) catch null;
}

fn dirExists(io: std.Io, path: []const u8) bool {
    if (openPackageDir(io, path)) |d_val| {
        var d = d_val;
        d.close(io);
        return true;
    }
    return false;
}

fn buildNameVariants(allocator: std.mem.Allocator, raw_pkg: []const u8) !std.ArrayList([]const u8) {
    var list: std.ArrayList([]const u8) = .empty;
    try list.append(allocator, try allocator.dupe(u8, raw_pkg));

    // Support both bare (`email_address`) and scoped (`@cap/email_address`) forms
    var prefix: []const u8 = "";
    var pkg_part: []const u8 = raw_pkg;
    if (std.mem.startsWith(u8, raw_pkg, "@")) {
        if (std.mem.indexOfScalar(u8, raw_pkg, '/')) |slash| {
            prefix = raw_pkg[0 .. slash + 1];
            pkg_part = raw_pkg[slash + 1 ..];
        }
    }

    if (std.mem.indexOfScalar(u8, pkg_part, '_') != null) {
        const replaced = try allocator.alloc(u8, pkg_part.len);
        for (pkg_part, 0..) |c, i| {
            replaced[i] = if (c == '_') '-' else c;
        }
        const cand = if (prefix.len > 0)
            try std.fmt.allocPrint(allocator, "{s}{s}", .{ prefix, replaced })
        else
            replaced;
        if (!std.mem.eql(u8, cand, raw_pkg)) {
            try list.append(allocator, cand);
        }
    }

    if (std.mem.indexOfScalar(u8, pkg_part, '-') != null) {
        const replaced = try allocator.alloc(u8, pkg_part.len);
        for (pkg_part, 0..) |c, i| {
            replaced[i] = if (c == '-') '_' else c;
        }
        const cand = if (prefix.len > 0)
            try std.fmt.allocPrint(allocator, "{s}{s}", .{ prefix, replaced })
        else
            replaced;
        var dup = false;
        for (list.items) |existing| {
            if (std.mem.eql(u8, existing, cand)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            try list.append(allocator, cand);
        }
    }

    return list;
}

fn findPackageDirInBase(
    allocator: std.mem.Allocator,
    io: std.Io,
    base: ?[]const u8,
    variants: []const []const u8,
    is_scoped: bool,
) !?[]const u8 {
    const roots_bare = [_][]const u8{
        "packages",
        "packages/@cap",
        "packages/@web",
        "cap_modules",
        "cap_modules/@cap",
        "cap_modules/@web",
    };
    const roots_scoped = [_][]const u8{
        "packages",
        "cap_modules",
    };

    const roots: []const []const u8 = if (is_scoped) &roots_scoped else &roots_bare;

    for (roots) |r| {
        for (variants) |v| {
            const candidate = if (base) |b|
                try std.fmt.allocPrint(allocator, "{s}/{s}/{s}", .{ b, r, v })
            else
                try std.fmt.allocPrint(allocator, "{s}/{s}", .{ r, v });
            if (dirExists(io, candidate)) {
                return candidate;
            }
        }
    }
    return null;
}

fn readFileInDir(allocator: std.mem.Allocator, io: std.Io, dir: std.Io.Dir, sub_path: []const u8) ?[]u8 {
    return std.Io.Dir.readFileAlloc(dir, io, sub_path, allocator, @fromBackingInt(@intCast(2 * 1024 * 1024))) catch null;
}

fn writeFirstNLines(io: std.Io, out: std.Io.File, text: []const u8, max_lines: usize) void {
    var line_count: usize = 0;
    var i: usize = 0;
    var truncated = false;

    while (i < text.len) {
        const next_nl = std.mem.indexOfScalarPos(u8, text, i, '\n');
        if (next_nl) |nl| {
            const line = text[i .. nl + 1];
            out.writeStreamingAll(io, line) catch {};
            i = nl + 1;
            line_count += 1;
            if (line_count >= max_lines) {
                if (i < text.len) truncated = true;
                break;
            }
        } else {
            out.writeStreamingAll(io, text[i..]) catch {};
            out.writeStreamingAll(io, "\n") catch {};
            break;
        }
    }
    if (truncated) {
        out.writeStreamingAll(io, "... (truncated to first 40 lines)\n") catch {};
    }
}

pub fn handleBrief(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    args: []const []const u8,
) !void {
    const out = std.Io.File.stdout();

    if (args.len == 0 or std.mem.eql(u8, args[0], "-h") or std.mem.eql(u8, args[0], "--help")) {
        out.writeStreamingAll(io,
            \\Usage: cap brief <pkg>
            \\  AI-First package summary: prints README_AGENT.md (or first 40 lines of README.md)
            \\  followed by src/index.d.cp exact type signatures.
            \\  Searches packages/, packages/@cap/, packages/@web/, cap_modules/ with _/- fallback.
            \\
        ) catch {};
        return;
    }

    const raw_pkg = args[0];
    const is_scoped = std.mem.startsWith(u8, raw_pkg, "@");
    var variants = try buildNameVariants(allocator, raw_pkg);
    defer variants.deinit(allocator);

    var found_path = try findPackageDirInBase(allocator, io, null, variants.items, is_scoped);
    if (found_path == null) {
        found_path = try findPackageDirInBase(allocator, io, cap_root, variants.items, is_scoped);
    }

    if (found_path == null) {
        std.debug.print("Error: package '{s}' not found in packages/, packages/@cap/, packages/@web/, or cap_modules/ (including _/- variants).\n", .{raw_pkg});
        std.process.exit(1);
    }

    const pkg_path = found_path.?;
    var dir = openPackageDir(io, pkg_path) orelse {
        std.debug.print("Error: failed to open package directory '{s}'.\n", .{pkg_path});
        std.process.exit(1);
    };
    defer dir.close(io);

    const header = try std.fmt.allocPrint(allocator, "=== cap brief: {s} ({s}) ===\n", .{ raw_pkg, pkg_path });
    out.writeStreamingAll(io, header) catch {};

    var printed_any = false;

    if (readFileInDir(allocator, io, dir, "README_AGENT.md")) |agent_doc| {
        printed_any = true;
        out.writeStreamingAll(io, "\n--- README_AGENT.md ---\n") catch {};
        out.writeStreamingAll(io, agent_doc) catch {};
        if (agent_doc.len > 0 and agent_doc[agent_doc.len - 1] != '\n') {
            out.writeStreamingAll(io, "\n") catch {};
        }
    } else if (readFileInDir(allocator, io, dir, "README.md")) |readme_doc| {
        printed_any = true;
        out.writeStreamingAll(io, "\n--- README.md (first 40 lines) ---\n") catch {};
        writeFirstNLines(io, out, readme_doc, 40);
    }

    if (readFileInDir(allocator, io, dir, "src/index.d.cp")) |dcp| {
        printed_any = true;
        out.writeStreamingAll(io, "\n--- src/index.d.cp ---\n") catch {};
        out.writeStreamingAll(io, dcp) catch {};
        if (dcp.len > 0 and dcp[dcp.len - 1] != '\n') {
            out.writeStreamingAll(io, "\n") catch {};
        }
    } else if (readFileInDir(allocator, io, dir, "index.d.cp")) |dcp| {
        printed_any = true;
        out.writeStreamingAll(io, "\n--- index.d.cp ---\n") catch {};
        out.writeStreamingAll(io, dcp) catch {};
        if (dcp.len > 0 and dcp[dcp.len - 1] != '\n') {
            out.writeStreamingAll(io, "\n") catch {};
        }
    }

    if (!printed_any) {
        out.writeStreamingAll(io, "(no README_AGENT.md, README.md, or src/index.d.cp found in package directory)\n") catch {};
    }
}
