
const std = @import("std");
const manifest = @import("manifest.zig");

pub const Lockfile = struct {
    version: []const u8,
    dependencies: std.StringHashMap([]const u8),
    dev_dependencies: std.StringHashMap([]const u8),

    pub fn deinit(self: *Lockfile, allocator: std.mem.Allocator) void {
        allocator.free(self.version);
        var it = self.dependencies.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        self.dependencies.deinit();
        var dev_it = self.dev_dependencies.iterator();
        while (dev_it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        self.dev_dependencies.deinit();
    }

    pub fn parse(allocator: std.mem.Allocator, contents: []const u8) !Lockfile {
        var parsed = try std.json.parseFromSlice(std.json.Value, allocator, contents, .{});
        defer parsed.deinit();

        const root = parsed.value.object;
        const version = try allocator.dupe(u8, if (root.get("version")) |v| (if (v == .string) v.string else "1") else "1");

        var deps = std.StringHashMap([]const u8).init(allocator);
        if (root.get("dependencies")) |d| {
            if (d == .object) {
                var iter = d.object.iterator();
                while (iter.next()) |entry| {
                    if (entry.value_ptr.* == .string) {
                        try deps.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.string));
                    }
                }
            }
        }

        var dev_deps = std.StringHashMap([]const u8).init(allocator);
        if (root.get("dev_dependencies")) |dd| {
            if (dd == .object) {
                var iter = dd.object.iterator();
                while (iter.next()) |entry| {
                    if (entry.value_ptr.* == .string) {
                        try dev_deps.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.string));
                    }
                }
            }
        }

        return Lockfile{
            .version = version,
            .dependencies = deps,
            .dev_dependencies = dev_deps,
        };
    }

    pub fn fromManifest(allocator: std.mem.Allocator, mf: manifest.Manifest) !Lockfile {
        var deps = std.StringHashMap([]const u8).init(allocator);
        var it = mf.dependencies.iterator();
        while (it.next()) |entry| {
            try deps.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.*));
        }

        var dev_deps = std.StringHashMap([]const u8).init(allocator);
        var dev_it = mf.dev_dependencies.iterator();
        while (dev_it.next()) |entry| {
            try dev_deps.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.*));
        }

        return Lockfile{
            .version = try allocator.dupe(u8, "1"),
            .dependencies = deps,
            .dev_dependencies = dev_deps,
        };
    }

    pub fn save(self: *Lockfile, allocator: std.mem.Allocator, path: []const u8) !void {
        var out_file = try @import("../main.zig").fs_shim.cwd().createFile(path, .{});
        defer out_file.close(@import("../main.zig").global_io);

        var arena = std.heap.ArenaAllocator.init(allocator);
        defer arena.deinit();
        const aa = arena.allocator();

        var root = std.json.Value{ .object = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable };
        try root.object.put(aa, "version", std.json.Value{ .string = self.version });

        var deps_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
        var dep_iter = self.dependencies.iterator();
        while (dep_iter.next()) |entry| {
            try deps_map.put(aa, entry.key_ptr.*, std.json.Value{ .string = entry.value_ptr.* });
        }
        try root.object.put(aa, "dependencies", std.json.Value{ .object = deps_map });

        var dev_deps_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
        var dev_dep_iter = self.dev_dependencies.iterator();
        while (dev_dep_iter.next()) |entry| {
            try dev_deps_map.put(aa, entry.key_ptr.*, std.json.Value{ .string = entry.value_ptr.* });
        }
        try root.object.put(aa, "dev_dependencies", std.json.Value{ .object = dev_deps_map });

        var buf: [8192]u8 = undefined;
        var writer = out_file.writer(@import("../main.zig").global_io, &buf);
        try std.json.Stringify.value(root, .{ .whitespace = .indent_2 }, &writer.interface);
        try writer.end();
    }
};
