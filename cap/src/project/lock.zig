const std = @import("std");
const manifest = @import("manifest.zig");

pub const LockedPackage = struct {
    version: []const u8,
    spec: []const u8,
    resolved: []const u8,
    integrity: []const u8,
    native_lib: ?[]const u8 = null,
    is_dev: bool = false,

    pub fn deinit(self: *LockedPackage, allocator: std.mem.Allocator) void {
        allocator.free(self.version);
        allocator.free(self.spec);
        allocator.free(self.resolved);
        allocator.free(self.integrity);
        if (self.native_lib) |nl| allocator.free(nl);
    }

    pub fn clone(self: LockedPackage, allocator: std.mem.Allocator) !LockedPackage {
        return LockedPackage{
            .version = try allocator.dupe(u8, self.version),
            .spec = try allocator.dupe(u8, self.spec),
            .resolved = try allocator.dupe(u8, self.resolved),
            .integrity = try allocator.dupe(u8, self.integrity),
            .native_lib = if (self.native_lib) |nl| try allocator.dupe(u8, nl) else null,
            .is_dev = self.is_dev,
        };
    }
};

pub const Lockfile = struct {
    version: []const u8,
    dependencies: std.StringHashMap([]const u8),
    dev_dependencies: std.StringHashMap([]const u8),
    packages: std.StringHashMap(LockedPackage),

    pub fn initEmpty(allocator: std.mem.Allocator) !Lockfile {
        return Lockfile{
            .version = try allocator.dupe(u8, "2"),
            .dependencies = std.StringHashMap([]const u8).init(allocator),
            .dev_dependencies = std.StringHashMap([]const u8).init(allocator),
            .packages = std.StringHashMap(LockedPackage).init(allocator),
        };
    }

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
        var pkg_it = self.packages.iterator();
        while (pkg_it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            entry.value_ptr.deinit(allocator);
        }
        self.packages.deinit();
    }

    pub fn putPackage(
        self: *Lockfile,
        allocator: std.mem.Allocator,
        name: []const u8,
        pkg: LockedPackage,
    ) !void {
        if (self.packages.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            var old_val = kv.value;
            old_val.deinit(allocator);
        }
        try self.packages.put(try allocator.dupe(u8, name), pkg);

        if (pkg.is_dev) {
            if (self.dev_dependencies.fetchRemove(name)) |kv| {
                allocator.free(kv.key);
                allocator.free(kv.value);
            }
            try self.dev_dependencies.put(try allocator.dupe(u8, name), try allocator.dupe(u8, pkg.spec));
        } else {
            if (self.dependencies.fetchRemove(name)) |kv| {
                allocator.free(kv.key);
                allocator.free(kv.value);
            }
            try self.dependencies.put(try allocator.dupe(u8, name), try allocator.dupe(u8, pkg.spec));
        }
    }

    pub fn putWorkspacePackage(
        self: *Lockfile,
        allocator: std.mem.Allocator,
        name: []const u8,
        pkg: LockedPackage,
    ) !void {
        if (self.packages.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            var old_val = kv.value;
            old_val.deinit(allocator);
        }
        try self.packages.put(try allocator.dupe(u8, name), pkg);
    }

    pub fn removePackage(self: *Lockfile, allocator: std.mem.Allocator, name: []const u8) bool {
        var removed = false;
        if (self.packages.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            var old_val = kv.value;
            old_val.deinit(allocator);
            removed = true;
        }
        if (self.dependencies.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            allocator.free(kv.value);
            removed = true;
        }
        if (self.dev_dependencies.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            allocator.free(kv.value);
            removed = true;
        }
        return removed;
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

        var packages = std.StringHashMap(LockedPackage).init(allocator);
        if (root.get("packages")) |pkgs| {
            if (pkgs == .object) {
                var iter = pkgs.object.iterator();
                while (iter.next()) |entry| {
                    if (entry.value_ptr.* == .object) {
                        const obj = entry.value_ptr.object;
                        const ver = if (obj.get("version")) |v| (if (v == .string) v.string else "0.0.0") else "0.0.0";
                        const spec = if (obj.get("spec")) |v| (if (v == .string) v.string else ver) else ver;
                        const resolved = if (obj.get("resolved")) |v| (if (v == .string) v.string else "") else "";
                        const integrity = if (obj.get("integrity")) |v| (if (v == .string) v.string else "") else "";
                        const native_lib: ?[]const u8 = if (obj.get("native_lib")) |v| (if (v == .string and v.string.len > 0) try allocator.dupe(u8, v.string) else null) else null;
                        const is_dev = if (obj.get("dev")) |v| (if (v == .bool) v.bool else false) else false;
                        try packages.put(try allocator.dupe(u8, entry.key_ptr.*), LockedPackage{
                            .version = try allocator.dupe(u8, ver),
                            .spec = try allocator.dupe(u8, spec),
                            .resolved = try allocator.dupe(u8, resolved),
                            .integrity = try allocator.dupe(u8, integrity),
                            .native_lib = native_lib,
                            .is_dev = is_dev,
                        });
                    }
                }
            }
        }

        return Lockfile{
            .version = version,
            .dependencies = deps,
            .dev_dependencies = dev_deps,
            .packages = packages,
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
            .version = try allocator.dupe(u8, "2"),
            .dependencies = deps,
            .dev_dependencies = dev_deps,
            .packages = std.StringHashMap(LockedPackage).init(allocator),
        };
    }

    pub fn verifyFrozen(self: *const Lockfile, mf: manifest.Manifest) !void {
        var it = mf.dependencies.iterator();
        while (it.next()) |entry| {
            const name = entry.key_ptr.*;
            const spec = entry.value_ptr.*;
            if (self.packages.get(name)) |pkg| {
                if (!std.mem.eql(u8, pkg.spec, spec) and !std.mem.eql(u8, pkg.version, spec)) {
                    return error.FrozenLockfileMismatch;
                }
            } else if (self.dependencies.get(name)) |ver| {
                if (!std.mem.eql(u8, ver, spec)) {
                    return error.FrozenLockfileMismatch;
                }
            } else {
                return error.FrozenLockfileMissingDependency;
            }
        }
        var dev_it = mf.dev_dependencies.iterator();
        while (dev_it.next()) |entry| {
            const name = entry.key_ptr.*;
            const spec = entry.value_ptr.*;
            if (self.packages.get(name)) |pkg| {
                if (!std.mem.eql(u8, pkg.spec, spec) and !std.mem.eql(u8, pkg.version, spec)) {
                    return error.FrozenLockfileMismatch;
                }
            } else if (self.dev_dependencies.get(name)) |ver| {
                if (!std.mem.eql(u8, ver, spec)) {
                    return error.FrozenLockfileMismatch;
                }
            } else {
                return error.FrozenLockfileMissingDependency;
            }
        }
        if (self.dependencies.count() != mf.dependencies.count() or
            self.dev_dependencies.count() != mf.dev_dependencies.count())
        {
            return error.FrozenLockfileMismatch;
        }
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

        if (self.packages.count() > 0) {
            var pkgs_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            var pkg_iter = self.packages.iterator();
            while (pkg_iter.next()) |entry| {
                const pkg = entry.value_ptr.*;
                var obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
                try obj.put(aa, "version", std.json.Value{ .string = pkg.version });
                try obj.put(aa, "spec", std.json.Value{ .string = pkg.spec });
                try obj.put(aa, "resolved", std.json.Value{ .string = pkg.resolved });
                try obj.put(aa, "integrity", std.json.Value{ .string = pkg.integrity });
                if (pkg.native_lib) |nl| {
                    try obj.put(aa, "native_lib", std.json.Value{ .string = nl });
                }
                if (pkg.is_dev) {
                    try obj.put(aa, "dev", std.json.Value{ .bool = true });
                }
                try pkgs_map.put(aa, entry.key_ptr.*, std.json.Value{ .object = obj });
            }
            try root.object.put(aa, "packages", std.json.Value{ .object = pkgs_map });
        }

        var buf: [8192]u8 = undefined;
        var writer = out_file.writer(@import("../main.zig").global_io, &buf);
        try std.json.Stringify.value(root, .{ .whitespace = .indent_2 }, &writer.interface);
        try writer.end();
    }
};
