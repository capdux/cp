const std = @import("std");

pub const TargetInfo = struct {
    cp_target: []const u8,
    is_stdlib: bool,
};

pub const RegistryMap = struct {
    arena: std.heap.ArenaAllocator,
    entries: std.StringHashMap(TargetInfo),

    pub fn init(allocator: std.mem.Allocator) RegistryMap {
        const arena = std.heap.ArenaAllocator.init(allocator);
        return .{
            .arena = arena,
            .entries = std.StringHashMap(TargetInfo).init(allocator),
        };
    }

    pub fn deinit(self: *RegistryMap) void {
        self.entries.deinit();
        self.arena.deinit();
    }

    fn lowerDup(alloc: std.mem.Allocator, s: []const u8) ![]u8 {
        const buf = try alloc.dupe(u8, s);
        for (buf) |*c| {
            c.* = std.ascii.toLower(c.*);
        }
        return buf;
    }

    pub fn load(allocator: std.mem.Allocator, io: std.Io, cap_root: []const u8) !RegistryMap {
        var map = RegistryMap.init(allocator);
        errdefer map.deinit();

        const reg_path_1 = try std.fmt.allocPrint(allocator, "{s}/packages/registry.json", .{cap_root});
        defer allocator.free(reg_path_1);

        const raw: ?[]u8 = blk: {
            if (std.fs.path.isAbsolute(reg_path_1)) {
                if (std.Io.Dir.openFileAbsolute(io, reg_path_1, .{})) |f_val| {
                    var f = f_val;
                    defer f.close(io);
                    const data = std.Io.Dir.readFileAlloc(.cwd(), io, reg_path_1, allocator, @fromBackingInt(@intCast(32 * 1024 * 1024))) catch null;
                    if (data != null) break :blk data;
                } else |_| {}
            }
            if (std.Io.Dir.readFileAlloc(.cwd(), io, reg_path_1, allocator, @fromBackingInt(@intCast(32 * 1024 * 1024)))) |data| {
                break :blk data;
            } else |_| {}
            if (std.Io.Dir.readFileAlloc(.cwd(), io, "packages/registry.json", allocator, @fromBackingInt(@intCast(32 * 1024 * 1024)))) |data| {
                break :blk data;
            } else |_| {}
            break :blk null;
        };

        const json_bytes = raw orelse return map;
        defer allocator.free(json_bytes);

        const a = map.arena.allocator();
        const parsed = std.json.parseFromSlice(std.json.Value, a, json_bytes, .{}) catch return map;
        const root_obj = switch (parsed.value) {
            .object => |o| o,
            else => return map,
        };
        const entries_val = root_obj.get("entries") orelse return map;
        const arr = switch (entries_val) {
            .array => |items| items.items,
            else => return map,
        };

        // Pass 1: primary source_name mappings (highest precedence)
        for (arr) |item| {
            const obj = switch (item) {
                .object => |o| o,
                else => continue,
            };
            const src_val = obj.get("source_name") orelse continue;
            const tgt_val = obj.get("cp_target") orelse continue;
            const src_name = switch (src_val) {
                .string => |s| s,
                else => continue,
            };
            const cp_target = switch (tgt_val) {
                .string => |s| s,
                else => continue,
            };
            const is_stdlib = if (obj.get("kind")) |kv| switch (kv) {
                .string => |ks| std.mem.eql(u8, ks, "stdlib"),
                else => false,
            } else false;

            if (src_name.len == 0 or cp_target.len == 0) continue;
            const key = try lowerDup(a, src_name);
            const val = TargetInfo{
                .cp_target = try a.dupe(u8, cp_target),
                .is_stdlib = is_stdlib,
            };
            if (!map.entries.contains(key)) {
                try map.entries.put(key, val);
            }
        }

        // Pass 2: aliases (never override an existing primary source_name)
        for (arr) |item| {
            const obj = switch (item) {
                .object => |o| o,
                else => continue,
            };
            const tgt_val = obj.get("cp_target") orelse continue;
            const cp_target = switch (tgt_val) {
                .string => |s| s,
                else => continue,
            };
            const is_stdlib = if (obj.get("kind")) |kv| switch (kv) {
                .string => |ks| std.mem.eql(u8, ks, "stdlib"),
                else => false,
            } else false;
            const aliases_val = obj.get("aliases") orelse continue;
            const aliases = switch (aliases_val) {
                .array => |items| items.items,
                else => continue,
            };
            const target_dup = try a.dupe(u8, cp_target);
            for (aliases) |al| {
                const al_s = switch (al) {
                    .string => |s| s,
                    else => continue,
                };
                if (al_s.len == 0) continue;
                const key = try lowerDup(a, al_s);
                if (!map.entries.contains(key)) {
                    try map.entries.put(key, .{
                        .cp_target = target_dup,
                        .is_stdlib = is_stdlib,
                    });
                }
            }
        }

        return map;
    }

    pub fn lookup(self: *const RegistryMap, allocator: std.mem.Allocator, spec: []const u8) !?TargetInfo {
        if (spec.len == 0) return null;
        const low = try lowerDup(allocator, spec);
        defer allocator.free(low);

        if (self.entries.get(low)) |hit| return hit;

        // Try '_' -> '-'
        if (std.mem.indexOfScalar(u8, low, '_') != null) {
            const alt = try allocator.dupe(u8, low);
            defer allocator.free(alt);
            for (alt) |*c| {
                if (c.* == '_') c.* = '-';
            }
            if (self.entries.get(alt)) |hit| return hit;
        }

        // Try '-' -> '_'
        if (std.mem.indexOfScalar(u8, low, '-') != null) {
            const alt = try allocator.dupe(u8, low);
            defer allocator.free(alt);
            for (alt) |*c| {
                if (c.* == '-') c.* = '_';
            }
            if (self.entries.get(alt)) |hit| return hit;
        }

        return null;
    }
};

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

const ResolvedSpec = struct {
    target: []u8,
    is_stdlib: bool,
};

/// Resolve any import specifier or dependency name:
/// 1. Legacy `@cap/<sub_name>` -> bare `<orig>` or registry target (preserving native `packages/@cap/*`).
/// 2. Upstream npm/crates package or alias in `packages/registry.json` -> `cp_target` (or `cap:*` stdlib).
fn resolveImportSpec(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    reg_map: *const RegistryMap,
    spec: []const u8,
) !?ResolvedSpec {
    if (spec.len == 0) return null;
    // Skip relative paths, built-in cap:* channels, or whitespace
    if (std.mem.startsWith(u8, spec, "./") or
        std.mem.startsWith(u8, spec, "../") or
        std.mem.startsWith(u8, spec, "/") or
        std.mem.startsWith(u8, spec, "cap:") or
        std.mem.indexOfAny(u8, spec, " \t\r\n") != null)
    {
        return null;
    }

    // Case 1: Legacy `@cap/<sub_name>`
    if (std.mem.startsWith(u8, spec, "@cap/")) {
        const sub_name = spec["@cap/".len..];
        if (sub_name.len == 0 or std.mem.indexOfScalar(u8, sub_name, '/') != null) return null;

        // Preserve real native C/C++ packages under packages/@cap/<sub_name>
        if (try dirExistsIn(allocator, io, cap_root, spec)) {
            return null;
        }

        // Exact bare directory
        if (try dirExistsIn(allocator, io, cap_root, sub_name)) {
            return .{ .target = try allocator.dupe(u8, sub_name), .is_stdlib = false };
        }
        // '_' -> '-'
        if (std.mem.indexOfScalar(u8, sub_name, '_') != null) {
            const alt = try allocator.dupe(u8, sub_name);
            for (alt) |*c| {
                if (c.* == '_') c.* = '-';
            }
            if (try dirExistsIn(allocator, io, cap_root, alt)) {
                return .{ .target = alt, .is_stdlib = false };
            }
            allocator.free(alt);
        }
        // '-' -> '_'
        if (std.mem.indexOfScalar(u8, sub_name, '-') != null) {
            const alt = try allocator.dupe(u8, sub_name);
            for (alt) |*c| {
                if (c.* == '-') c.* = '_';
            }
            if (try dirExistsIn(allocator, io, cap_root, alt)) {
                return .{ .target = alt, .is_stdlib = false };
            }
            allocator.free(alt);
        }
        // Registry lookup for legacy @cap/<sub_name> (e.g. @cap/toml -> toml_edit)
        if (try reg_map.lookup(allocator, sub_name)) |hit| {
            if (!std.mem.eql(u8, hit.cp_target, spec)) {
                return .{ .target = try allocator.dupe(u8, hit.cp_target), .is_stdlib = hit.is_stdlib };
            }
        }
        return null;
    }

    // Case 2: Bare package or scoped package (e.g. serde_json, humantime, @msgpack/msgpack, yargs)
    // Skip subpath imports like "foo/bar" unless scoped "@scope/pkg"
    if (std.mem.indexOfScalar(u8, spec, '/')) |slash_idx| {
        if (spec[0] != '@') return null;
        if (std.mem.indexOfScalarPos(u8, spec, slash_idx + 1, '/') != null) return null;
    }

    // If a standalone package directory already exists with this exact name, keep it!
    if (try dirExistsIn(allocator, io, cap_root, spec)) {
        return null;
    }

    // Check registry.json mapping (source_name + aliases + _/- normalization)
    if (try reg_map.lookup(allocator, spec)) |hit| {
        if (!std.mem.eql(u8, hit.cp_target, spec)) {
            return .{ .target = try allocator.dupe(u8, hit.cp_target), .is_stdlib = hit.is_stdlib };
        }
    }

    return null;
}

fn fixCpFile(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    reg_map: *const RegistryMap,
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
    const needle = "from \"";

    while (i < content.len) {
        if (std.mem.indexOfPos(u8, content, i, needle)) |pos| {
            // Ensure `from "` is not part of an identifier like `transform "`
            const valid_boundary = (pos == 0) or (!std.ascii.isAlphanumeric(content[pos - 1]) and content[pos - 1] != '_');
            const name_start = pos + needle.len;
            if (valid_boundary) {
                if (std.mem.indexOfScalarPos(u8, content, name_start, '"')) |name_end| {
                    const spec = content[name_start..name_end];
                    if (try resolveImportSpec(allocator, io, cap_root, reg_map, spec)) |res| {
                        defer allocator.free(res.target);
                        try out_buf.appendSlice(allocator, content[i..pos]);
                        try out_buf.appendSlice(allocator, "from \"");
                        try out_buf.appendSlice(allocator, res.target);
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

/// Rewrite `dependencies` and `devDependencies` sections in a `cap.json` file:
/// - Remaps legacy `@cap/<old>` and upstream source/alias package names to `cp_target`
/// - Removes dependencies that have been absorbed into built-in `cap:*` stdlib
/// - Deduplicates keys if the mapped `cp_target` is already listed
/// - Preserves original formatting and all other JSON fields
fn fixCapJsonFile(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    reg_map: *const RegistryMap,
    file_path: []const u8,
    dry_run: bool,
) !usize {
    const content = std.Io.Dir.readFileAlloc(.cwd(), io, file_path, allocator, @fromBackingInt(@intCast(4 * 1024 * 1024))) catch return 0;
    defer allocator.free(content);

    var lines: std.ArrayList([]const u8) = .empty;
    defer lines.deinit(allocator);
    var it = std.mem.splitScalar(u8, content, '\n');
    while (it.next()) |line| {
        try lines.append(allocator, line);
    }

    var out_lines: std.ArrayList([]u8) = .empty;
    defer {
        for (out_lines.items) |l| allocator.free(l);
        out_lines.deinit(allocator);
    }

    var fixes: usize = 0;
    var in_deps = false;
    var seen_keys = std.StringHashMap(void).init(allocator);
    defer {
        var kit = seen_keys.keyIterator();
        while (kit.next()) |k| allocator.free(k.*);
        seen_keys.deinit();
    }

    for (lines.items) |raw_line| {
        const trimmed = std.mem.trim(u8, raw_line, " \t\r");
        if (!in_deps) {
            if ((std.mem.startsWith(u8, trimmed, "\"dependencies\"") or
                std.mem.startsWith(u8, trimmed, "\"devDependencies\"")) and
                std.mem.indexOfScalar(u8, trimmed, '{') != null and
                std.mem.indexOfScalar(u8, trimmed, '}') == null)
            {
                in_deps = true;
                // Clear seen_keys for this block
                var kit = seen_keys.keyIterator();
                while (kit.next()) |k| allocator.free(k.*);
                seen_keys.clearRetainingCapacity();
            }
            try out_lines.append(allocator, try allocator.dupe(u8, raw_line));
            continue;
        }

        // Inside dependencies/devDependencies block
        if (std.mem.startsWith(u8, trimmed, "}")) {
            in_deps = false;
            // Ensure previous emitted line inside the block does not have a dangling trailing comma
            if (out_lines.items.len > 0) {
                const prev = out_lines.items[out_lines.items.len - 1];
                const prev_trim = std.mem.trimEnd(u8, prev, " \t\r");
                if (prev_trim.len > 0 and prev_trim[prev_trim.len - 1] == ',') {
                    const stripped = try allocator.dupe(u8, prev_trim[0 .. prev_trim.len - 1]);
                    allocator.free(prev);
                    out_lines.items[out_lines.items.len - 1] = stripped;
                }
            }
            try out_lines.append(allocator, try allocator.dupe(u8, raw_line));
            continue;
        }

        // Parse `"key": ...` line
        if (trimmed.len > 2 and trimmed[0] == '"') {
            if (std.mem.indexOfScalarPos(u8, trimmed, 1, '"')) |q_end| {
                const key = trimmed[1..q_end];
                if (try resolveImportSpec(allocator, io, cap_root, reg_map, key)) |res| {
                    defer allocator.free(res.target);
                    fixes += 1;
                    if (res.is_stdlib) {
                        // Built-in cap:* stdlib — drop from external dependencies
                        continue;
                    }
                    if (seen_keys.contains(res.target)) {
                        // Duplicate after remap — drop
                        continue;
                    }
                    const kdup = try allocator.dupe(u8, res.target);
                    try seen_keys.put(kdup, {});

                    // Replace `"key"` with `"res.target"` preserving indentation & value
                    if (std.mem.indexOfScalar(u8, raw_line, '"')) |first_q| {
                        const rest_start = first_q + 1 + key.len + 1;
                        const new_line = try std.fmt.allocPrint(allocator, "{s}\"{s}\"{s}", .{
                            raw_line[0..first_q],
                            res.target,
                            raw_line[rest_start..],
                        });
                        try out_lines.append(allocator, new_line);
                        continue;
                    }
                } else {
                    if (seen_keys.contains(key)) {
                        fixes += 1;
                        continue;
                    }
                    const kdup = try allocator.dupe(u8, key);
                    try seen_keys.put(kdup, {});
                }
            }
        }

        try out_lines.append(allocator, try allocator.dupe(u8, raw_line));
    }

    if (fixes > 0 and !dry_run) {
        var joined: std.ArrayList(u8) = .empty;
        defer joined.deinit(allocator);
        for (out_lines.items, 0..) |l, idx| {
            try joined.appendSlice(allocator, l);
            if (idx + 1 < out_lines.items.len) {
                try joined.append(allocator, '\n');
            }
        }
        try std.Io.Dir.writeFile(.cwd(), io, .{
            .sub_path = file_path,
            .data = joined.items,
        });
    }

    return fixes;
}

/// Programmatic entry point called by `cap from -ts/-rs/-npm/-crate` to auto-fix
/// all generated `.cp` imports and `cap.json` dependencies in `target_path`.
pub fn fixPathAuto(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    target_path: []const u8,
) !void {
    const argv = [_][]const u8{target_path};
    try handleFix(allocator, io, cap_root, &argv);
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
                \\Usage: cap fix [--dry-run] <file.cp|cap.json|dir> [...]
                \\  AI-First auto-fixer for CP projects & packages (powered by packages/registry.json):
                \\    - Rewrites legacy "@cap/<pkg>" imports to bare original package names "<pkg>"
                \\      (preserves native C/C++ packages under packages/@cap/*).
                \\    - Maps upstream npm/crates package names & aliases to CP targets or built-in "cap:*" stdlib.
                \\    - Updates cap.json "dependencies" / "devDependencies" (remaps aliases, drops built-in cap:* deps).
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

    var reg_map = try RegistryMap.load(allocator, io, cap_root);
    defer reg_map.deinit();

    var total_files: usize = 0;
    var total_fixes: usize = 0;

    for (targets.items) |target| {
        if (std.mem.endsWith(u8, target, ".cp")) {
            const n = try fixCpFile(allocator, io, cap_root, &reg_map, target, dry_run);
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
        } else if (std.mem.endsWith(u8, target, "cap.json")) {
            const n = try fixCapJsonFile(allocator, io, cap_root, &reg_map, target, dry_run);
            if (n > 0) {
                total_files += 1;
                total_fixes += n;
                const line = try std.fmt.allocPrint(allocator, "  {s}: {d} dep(s) {s}\n", .{
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
                // Skip .cap/ and .git/ directories
                if (std.mem.startsWith(u8, entry.path, ".cap/") or
                    std.mem.startsWith(u8, entry.path, ".git/") or
                    std.mem.indexOf(u8, entry.path, "/.cap/") != null or
                    std.mem.indexOf(u8, entry.path, "/.git/") != null)
                {
                    continue;
                }
                const full = if (std.mem.eql(u8, target, "."))
                    try allocator.dupe(u8, entry.path)
                else
                    try std.fmt.allocPrint(allocator, "{s}/{s}", .{ target, entry.path });
                defer allocator.free(full);

                if (std.mem.endsWith(u8, entry.path, ".cp") and !std.mem.endsWith(u8, entry.path, ".d.cp")) {
                    const n = try fixCpFile(allocator, io, cap_root, &reg_map, full, dry_run);
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
                } else if (std.mem.eql(u8, std.fs.path.basename(entry.path), "cap.json")) {
                    const n = try fixCapJsonFile(allocator, io, cap_root, &reg_map, full, dry_run);
                    if (n > 0) {
                        total_files += 1;
                        total_fixes += n;
                        const line = try std.fmt.allocPrint(allocator, "  {s}: {d} dep(s) {s}\n", .{
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
    }

    const summary = try std.fmt.allocPrint(allocator, "cap fix: {d} fix(es) across {d} file(s){s}.\n", .{
        total_fixes,
        total_files,
        if (dry_run) " (dry-run)" else "",
    });
    defer allocator.free(summary);
    out.writeStreamingAll(io, summary) catch {};
}

fn checkSingleFile(
    io: std.Io,
    cpc_bin: []const u8,
    file_path: []const u8,
) !bool {
    const argv = [_][]const u8{ cpc_bin, "check", file_path };
    var child = std.process.spawn(io, .{
        .argv = &argv,
        .stdout = .inherit,
        .stderr = .inherit,
    }) catch return false;
    const term = child.wait(io) catch return false;
    return switch (term) {
        .exited => |code| code == 0,
        else => false,
    };
}

pub fn handleCheck(
    allocator: std.mem.Allocator,
    io: std.Io,
    cap_root: []const u8,
    args: []const []const u8,
) !void {
    const out = std.Io.File.stdout();
    var targets: std.ArrayList([]const u8) = .empty;
    defer targets.deinit(allocator);

    for (args) |arg| {
        if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            out.writeStreamingAll(io,
                \\Usage: cap check <file.cp|dir> [...]
                \\  AI-First zero-build fast type/borrow/syntax check via `cpc --check`.
                \\  Outputs Rustc-style three-stage self-healing diagnostics for agents & engineers.
                \\
            ) catch {};
            return;
        }
        try targets.append(allocator, arg);
    }

    if (targets.items.len == 0) {
        try targets.append(allocator, ".");
    }

    const cpc_rel = try std.fmt.allocPrint(allocator, "{s}/cpc/target/release/cpc", .{cap_root});
    defer allocator.free(cpc_rel);
    const cpc_bin: []const u8 = blk: {
        if (std.Io.Dir.accessAbsolute(io, cpc_rel, .{})) |_| {
            break :blk cpc_rel;
        } else |_| {
            break :blk "cpc";
        }
    };

    var passed: usize = 0;
    var failed: usize = 0;

    for (targets.items) |target| {
        if (std.mem.endsWith(u8, target, ".cp")) {
            if (try checkSingleFile(io, cpc_bin, target)) {
                passed += 1;
            } else {
                failed += 1;
            }
        } else if (isDir(io, target)) {
            var dir = std.Io.Dir.openDir(.cwd(), io, target, .{ .iterate = true }) catch continue;
            defer dir.close(io);
            var walker = try dir.walk(allocator);
            defer walker.deinit();
            while (try walker.next(io)) |entry| {
                if (entry.kind != .file) continue;
                if (std.mem.startsWith(u8, entry.path, ".cap/") or
                    std.mem.startsWith(u8, entry.path, ".git/") or
                    std.mem.indexOf(u8, entry.path, "/.cap/") != null or
                    std.mem.indexOf(u8, entry.path, "/.git/") != null)
                {
                    continue;
                }
                if (!std.mem.endsWith(u8, entry.path, ".cp")) continue;
                if (std.mem.endsWith(u8, entry.path, ".d.cp")) continue;
                const full = if (std.mem.eql(u8, target, "."))
                    try allocator.dupe(u8, entry.path)
                else
                    try std.fmt.allocPrint(allocator, "{s}/{s}", .{ target, entry.path });
                defer allocator.free(full);
                if (try checkSingleFile(io, cpc_bin, full)) {
                    passed += 1;
                } else {
                    failed += 1;
                }
            }
        }
    }

    const summary = try std.fmt.allocPrint(allocator, "cap check: {d} passed, {d} failed ({d} total).\n", .{
        passed,
        failed,
        passed + failed,
    });
    defer allocator.free(summary);
    out.writeStreamingAll(io, summary) catch {};
    if (failed > 0) {
        std.process.exit(1);
    }
}
