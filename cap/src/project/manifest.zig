const std = @import("std");
const lock = @import("lock.zig");

pub const OptimizeMode = enum {
    Debug,
    ReleaseSafe,
    ReleaseFast,
    ReleaseSmall,

    pub fn fromString(str: []const u8) ?OptimizeMode {
        if (std.ascii.eqlIgnoreCase(str, "debug")) return .Debug;
        if (std.ascii.eqlIgnoreCase(str, "safe") or std.ascii.eqlIgnoreCase(str, "releasesafe")) return .ReleaseSafe;
        if (std.ascii.eqlIgnoreCase(str, "fast") or std.ascii.eqlIgnoreCase(str, "releasefast")) return .ReleaseFast;
        if (std.ascii.eqlIgnoreCase(str, "small") or std.ascii.eqlIgnoreCase(str, "releasesmall")) return .ReleaseSmall;
        return null;
    }

    pub fn toZigFlag(self: OptimizeMode) []const u8 {
        return switch (self) {
            .Debug => "Debug",
            .ReleaseSafe => "ReleaseSafe",
            .ReleaseFast => "ReleaseFast",
            .ReleaseSmall => "ReleaseSmall",
        };
    }

    pub fn toConfigString(self: OptimizeMode) []const u8 {
        return switch (self) {
            .Debug => "debug",
            .ReleaseSafe => "safe",
            .ReleaseFast => "fast",
            .ReleaseSmall => "small",
        };
    }
};

pub const BuildConfig = struct {
    entry: ?[]const u8 = null,
    opt: ?OptimizeMode = null,
    out_dir: ?[]const u8 = null,
    target: ?[]const u8 = null,
    targets: [][]const u8 = &.{},
    c_flags: [][]const u8 = &.{},
    link_libs: [][]const u8 = &.{},
    sys_bin: [][]const u8 = &.{},
    sys: ?[]const u8 = null,

    pub fn deinit(self: *BuildConfig, allocator: std.mem.Allocator) void {
        if (self.entry) |e| allocator.free(e);
        if (self.out_dir) |od| allocator.free(od);
        if (self.target) |t| allocator.free(t);
        for (self.targets) |t| allocator.free(t);
        if (self.targets.len > 0) allocator.free(self.targets);
        for (self.c_flags) |cf| allocator.free(cf);
        if (self.c_flags.len > 0) allocator.free(self.c_flags);
        for (self.link_libs) |ll| allocator.free(ll);
        if (self.link_libs.len > 0) allocator.free(self.link_libs);
        for (self.sys_bin) |sb| allocator.free(sb);
        if (self.sys_bin.len > 0) allocator.free(self.sys_bin);
        if (self.sys) |s| allocator.free(s);
    }
};

pub const NestConfig = struct {
    mailbox_capacity: ?u64 = null,
    reduction_slice: ?u64 = null,
};

pub const BinDependencyConfig = struct {
    name: []const u8 = "",
    version: ?[]const u8 = null,
    dep_type: []const u8 = "static_lib",
    path: ?[]const u8 = null,
    sources: [][]const u8 = &.{},
    include_dirs: [][]const u8 = &.{},
    c_flags: [][]const u8 = &.{},
    opt: ?OptimizeMode = null,
    prebuilt: ?[]const u8 = null,
    out_dir: ?[]const u8 = null,
    target: ?[]const u8 = null,
    build_cmd: ?[]const u8 = null,

    pub fn deinit(self: *BinDependencyConfig, allocator: std.mem.Allocator) void {
        if (self.name.len > 0) allocator.free(self.name);
        if (self.version) |v| allocator.free(v);
        if (self.dep_type.len > 0) allocator.free(self.dep_type);
        if (self.path) |p| allocator.free(p);
        for (self.sources) |s| allocator.free(s);
        if (self.sources.len > 0) allocator.free(self.sources);
        for (self.include_dirs) |id| allocator.free(id);
        if (self.include_dirs.len > 0) allocator.free(self.include_dirs);
        for (self.c_flags) |cf| allocator.free(cf);
        if (self.c_flags.len > 0) allocator.free(self.c_flags);
        if (self.prebuilt) |pb| allocator.free(pb);
        if (self.out_dir) |od| allocator.free(od);
        if (self.target) |t| allocator.free(t);
        if (self.build_cmd) |bc| allocator.free(bc);
    }

    pub fn clone(self: BinDependencyConfig, allocator: std.mem.Allocator) !BinDependencyConfig {
        var copy = BinDependencyConfig{
            .name = try allocator.dupe(u8, self.name),
            .version = if (self.version) |v| try allocator.dupe(u8, v) else null,
            .dep_type = try allocator.dupe(u8, self.dep_type),
            .path = if (self.path) |p| try allocator.dupe(u8, p) else null,
            .opt = self.opt,
            .prebuilt = if (self.prebuilt) |pb| try allocator.dupe(u8, pb) else null,
            .out_dir = if (self.out_dir) |od| try allocator.dupe(u8, od) else null,
            .target = if (self.target) |t| try allocator.dupe(u8, t) else null,
            .build_cmd = if (self.build_cmd) |bc| try allocator.dupe(u8, bc) else null,
        };
        if (self.sources.len > 0) {
            var list = try allocator.alloc([]const u8, self.sources.len);
            for (self.sources, 0..) |s, i| list[i] = try allocator.dupe(u8, s);
            copy.sources = list;
        }
        if (self.include_dirs.len > 0) {
            var list = try allocator.alloc([]const u8, self.include_dirs.len);
            for (self.include_dirs, 0..) |id, i| list[i] = try allocator.dupe(u8, id);
            copy.include_dirs = list;
        }
        if (self.c_flags.len > 0) {
            var list = try allocator.alloc([]const u8, self.c_flags.len);
            for (self.c_flags, 0..) |cf, i| list[i] = try allocator.dupe(u8, cf);
            copy.c_flags = list;
        }
        return copy;
    }

    pub fn parseValue(allocator: std.mem.Allocator, name: []const u8, val: std.json.Value) !BinDependencyConfig {
        var cfg = BinDependencyConfig{};
        cfg.name = try allocator.dupe(u8, name);
        if (val == .string) {
            cfg.version = try allocator.dupe(u8, val.string);
        } else if (val == .object) {
            if (val.object.get("ver") orelse val.object.get("version")) |v| {
                if (v == .string) cfg.version = try allocator.dupe(u8, v.string);
            }
            if (val.object.get("type")) |t| {
                if (t == .string) cfg.dep_type = try allocator.dupe(u8, t.string);
            }
            if (val.object.get("path")) |p| {
                if (p == .string) cfg.path = try allocator.dupe(u8, p.string);
            }
            // 1. src / sources
            const src_val = val.object.get("src") orelse val.object.get("sources");
            if (src_val) |srcs| {
                if (srcs == .array) {
                    var list = try allocator.alloc([]const u8, srcs.array.items.len);
                    for (srcs.array.items, 0..) |item, i| {
                        list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                    }
                    cfg.sources = list;
                } else if (srcs == .string) {
                    var list = try allocator.alloc([]const u8, 1);
                    list[0] = try allocator.dupe(u8, srcs.string);
                    cfg.sources = list;
                }
            }
            // 2. include / include_dirs
            const inc_val = val.object.get("include") orelse val.object.get("include_dirs");
            if (inc_val) |ids| {
                if (ids == .array) {
                    var list = try allocator.alloc([]const u8, ids.array.items.len);
                    for (ids.array.items, 0..) |item, i| {
                        list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                    }
                    cfg.include_dirs = list;
                } else if (ids == .string) {
                    var list = try allocator.alloc([]const u8, 1);
                    list[0] = try allocator.dupe(u8, ids.string);
                    cfg.include_dirs = list;
                }
            }
            // 3. flag / flags / c_flags
            const flag_val = val.object.get("flag") orelse val.object.get("flags") orelse val.object.get("c_flags");
            if (flag_val) |cfs| {
                if (cfs == .array) {
                    var list = try allocator.alloc([]const u8, cfs.array.items.len);
                    for (cfs.array.items, 0..) |item, i| {
                        list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                    }
                    cfg.c_flags = list;
                } else if (cfs == .string) {
                    var list = try allocator.alloc([]const u8, 1);
                    list[0] = try allocator.dupe(u8, cfs.string);
                    cfg.c_flags = list;
                }
            }
            if (val.object.get("opt")) |o| {
                if (o == .string) cfg.opt = OptimizeMode.fromString(o.string);
            }
            if (val.object.get("prebuilt") orelse val.object.get("prebuilt_dir")) |pb| {
                if (pb == .string and pb.string.len > 0) cfg.prebuilt = try allocator.dupe(u8, pb.string);
            }
            if (val.object.get("out_dir")) |od| {
                if (od == .string) cfg.out_dir = try allocator.dupe(u8, od.string);
            }
            if (val.object.get("target")) |t| {
                if (t == .string) cfg.target = try allocator.dupe(u8, t.string);
            }
            if (val.object.get("build_cmd")) |bc| {
                if (bc == .string) cfg.build_cmd = try allocator.dupe(u8, bc.string);
            }
        }
        return cfg;
    }
};

pub const SingleSysDep = struct {
    name: ?[]const u8 = null,
    linux: ?[]const u8 = null,
    mac: ?[]const u8 = null,
    win: ?[]const u8 = null,
    path: ?[]const u8 = null,
    prebuilt: ?[]const u8 = null,

    pub fn deinit(self: *SingleSysDep, allocator: std.mem.Allocator) void {
        if (self.name) |n| allocator.free(n);
        if (self.linux) |l| allocator.free(l);
        if (self.mac) |m| allocator.free(m);
        if (self.win) |w| allocator.free(w);
        if (self.path) |p| allocator.free(p);
        if (self.prebuilt) |pb| allocator.free(pb);
    }

    pub fn clone(self: SingleSysDep, allocator: std.mem.Allocator) !SingleSysDep {
        return SingleSysDep{
            .name = if (self.name) |n| try allocator.dupe(u8, n) else null,
            .linux = if (self.linux) |l| try allocator.dupe(u8, l) else null,
            .mac = if (self.mac) |m| try allocator.dupe(u8, m) else null,
            .win = if (self.win) |w| try allocator.dupe(u8, w) else null,
            .path = if (self.path) |p| try allocator.dupe(u8, p) else null,
            .prebuilt = if (self.prebuilt) |pb| try allocator.dupe(u8, pb) else null,
        };
    }

    pub fn getRawForTarget(self: SingleSysDep, target_triple: []const u8) ?[]const u8 {
        if (std.mem.indexOf(u8, target_triple, "linux") != null) {
            return self.linux;
        } else if (std.mem.indexOf(u8, target_triple, "macos") != null or std.mem.indexOf(u8, target_triple, "darwin") != null or std.mem.indexOf(u8, target_triple, "mac") != null) {
            return self.mac;
        } else if (std.mem.indexOf(u8, target_triple, "windows") != null or std.mem.indexOf(u8, target_triple, "win") != null) {
            return self.win;
        }
        const os_tag = @import("builtin").os.tag;
        return switch (os_tag) {
            .linux => self.linux,
            .macos => self.mac,
            .windows => self.win,
            else => self.linux,
        };
    }

    pub fn getLibFlagForTarget(self: SingleSysDep, target_triple: []const u8, allocator: std.mem.Allocator) !?[]const u8 {
        const raw_opt = self.getRawForTarget(target_triple);
        if (raw_opt) |raw| {
            var name = raw;
            if (std.mem.startsWith(u8, name, "lib")) {
                name = name[3..];
            }
            if (std.mem.indexOf(u8, name, ".")) |dot_idx| {
                name = name[0..dot_idx];
            }
            if (name.len > 0) {
                return try std.fmt.allocPrint(allocator, "-l{s}", .{name});
            }
        }
        if (self.name) |k| {
            if (k.len > 0) {
                return try std.fmt.allocPrint(allocator, "-l{s}", .{k});
            }
        }
        return null;
    }
};

pub const SysDependencyConfig = struct {
    libs: []SingleSysDep = &.{},

    pub fn deinit(self: *SysDependencyConfig, allocator: std.mem.Allocator) void {
        for (self.libs) |*l| l.deinit(allocator);
        if (self.libs.len > 0) allocator.free(self.libs);
    }

    pub fn clone(self: SysDependencyConfig, allocator: std.mem.Allocator) !SysDependencyConfig {
        var list = try allocator.alloc(SingleSysDep, self.libs.len);
        for (self.libs, 0..) |l, i| {
            list[i] = try l.clone(allocator);
        }
        return SysDependencyConfig{ .libs = list };
    }

    pub fn parseValue(allocator: std.mem.Allocator, val: std.json.Value) !SysDependencyConfig {
        if (val == .string) {
            var list = try allocator.alloc(SingleSysDep, 1);
            list[0] = SingleSysDep{
                .linux = try allocator.dupe(u8, val.string),
                .mac = try allocator.dupe(u8, val.string),
                .win = try allocator.dupe(u8, val.string),
            };
            return SysDependencyConfig{ .libs = list };
        } else if (val == .object) {
            const has_linux = val.object.contains("linux");
            const has_mac = val.object.contains("mac") or val.object.contains("macos");
            const has_win = val.object.contains("win") or val.object.contains("windows");

            if (has_linux or has_mac or has_win) {
                var list = try allocator.alloc(SingleSysDep, 1);
                var s = SingleSysDep{};
                if (val.object.get("linux")) |l| {
                    if (l == .string) s.linux = try allocator.dupe(u8, l.string);
                }
                if (val.object.get("mac")) |m| {
                    if (m == .string) s.mac = try allocator.dupe(u8, m.string);
                } else if (val.object.get("macos")) |m| {
                    if (m == .string) s.mac = try allocator.dupe(u8, m.string);
                }
                if (val.object.get("win")) |w| {
                    if (w == .string) s.win = try allocator.dupe(u8, w.string);
                } else if (val.object.get("windows")) |w| {
                    if (w == .string) s.win = try allocator.dupe(u8, w.string);
                }
                list[0] = s;
                return SysDependencyConfig{ .libs = list };
            } else {
                // Named map: {"z": {"linux": "..."}, ...}
                var list = std.ArrayList(SingleSysDep).initCapacity(allocator, val.object.count()) catch unreachable;
                var it = val.object.iterator();
                while (it.next()) |entry| {
                    const lib_key = entry.key_ptr.*;
                    const sub_val = entry.value_ptr.*;
                    var s = SingleSysDep{
                        .name = try allocator.dupe(u8, lib_key),
                    };
                    if (sub_val == .string) {
                        s.linux = try allocator.dupe(u8, sub_val.string);
                        s.mac = try allocator.dupe(u8, sub_val.string);
                        s.win = try allocator.dupe(u8, sub_val.string);
                    } else if (sub_val == .object) {
                        if (sub_val.object.get("linux")) |l| {
                            if (l == .string) s.linux = try allocator.dupe(u8, l.string);
                        }
                        if (sub_val.object.get("mac")) |m| {
                            if (m == .string) s.mac = try allocator.dupe(u8, m.string);
                        } else if (sub_val.object.get("macos")) |m| {
                            if (m == .string) s.mac = try allocator.dupe(u8, m.string);
                        }
                        if (sub_val.object.get("win")) |w| {
                            if (w == .string) s.win = try allocator.dupe(u8, w.string);
                        } else if (sub_val.object.get("windows")) |w| {
                            if (w == .string) s.win = try allocator.dupe(u8, w.string);
                        }
                        if (sub_val.object.get("path")) |p| {
                            if (p == .string) s.path = try allocator.dupe(u8, p.string);
                        }
                        if (sub_val.object.get("prebuilt") orelse sub_val.object.get("prebuilt_dir")) |pb| {
                            if (pb == .string and pb.string.len > 0) s.prebuilt = try allocator.dupe(u8, pb.string);
                        }
                    }
                    try list.append(allocator, s);
                }
                return SysDependencyConfig{ .libs = try list.toOwnedSlice(allocator) };
            }
        }
        return SysDependencyConfig{};
    }

    pub fn getLibFlagForTarget(self: SysDependencyConfig, target_triple: []const u8, allocator: std.mem.Allocator) !?[]const u8 {
        for (self.libs) |lib| {
            if (try lib.getLibFlagForTarget(target_triple, allocator)) |flag| {
                return flag;
            }
        }
        return null;
    }

    pub fn getAllLibFlagsForTarget(self: SysDependencyConfig, target_triple: []const u8, allocator: std.mem.Allocator) ![][]const u8 {
        var flags = std.ArrayList([]const u8).initCapacity(allocator, self.libs.len) catch unreachable;
        for (self.libs) |lib| {
            if (try lib.getLibFlagForTarget(target_triple, allocator)) |flag| {
                try flags.append(allocator, flag);
            }
        }
        return try flags.toOwnedSlice(allocator);
    }
};

pub const CentralLibEntry = struct {
    path: ?[]const u8 = null,
    bin: std.StringHashMap(BinDependencyConfig),
    sys: ?SysDependencyConfig = null,

    pub fn deinit(self: *CentralLibEntry, allocator: std.mem.Allocator) void {
        if (self.path) |p| allocator.free(p);
        var iter = self.bin.iterator();
        while (iter.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            entry.value_ptr.deinit(allocator);
        }
        self.bin.deinit();
        if (self.sys) |*s| s.deinit(allocator);
    }
};

pub const CentralLibIndex = struct {
    opt: ?OptimizeMode = null,
    libs: std.StringHashMap(CentralLibEntry),

    pub fn deinit(self: *CentralLibIndex, allocator: std.mem.Allocator) void {
        var iter = self.libs.iterator();
        while (iter.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            entry.value_ptr.deinit(allocator);
        }
        self.libs.deinit();
    }

    pub fn parse(allocator: std.mem.Allocator, contents: []const u8) !CentralLibIndex {
        var parsed = try std.json.parseFromSlice(std.json.Value, allocator, contents, .{});
        defer parsed.deinit();

        var index = CentralLibIndex{ .libs = std.StringHashMap(CentralLibEntry).init(allocator) };
        if (parsed.value != .object) return index;

        // Parse top-level opt / default_opt
        if (parsed.value.object.get("opt")) |o| {
            if (o == .string) index.opt = OptimizeMode.fromString(o.string);
        } else if (parsed.value.object.get("default_opt")) |o| {
            if (o == .string) index.opt = OptimizeMode.fromString(o.string);
        } else if (parsed.value.object.get("global")) |g| {
            if (g == .object) {
                if (g.object.get("default_opt")) |o| {
                    if (o == .string) index.opt = OptimizeMode.fromString(o.string);
                } else if (g.object.get("opt")) |o| {
                    if (o == .string) index.opt = OptimizeMode.fromString(o.string);
                }
            }
        }

        if (parsed.value.object.get("lib")) |lib_val| {
            if (lib_val == .object) {
                var iter = lib_val.object.iterator();
                while (iter.next()) |entry| {
                    const lib_name = entry.key_ptr.*;
                    const val = entry.value_ptr.*;
                    if (val == .object) {
                        var cent_entry = CentralLibEntry{
                            .bin = std.StringHashMap(BinDependencyConfig).init(allocator),
                        };
                        if (val.object.get("path")) |p| {
                            if (p == .string) cent_entry.path = try allocator.dupe(u8, p.string);
                        }
                        if (val.object.get("bin")) |b| {
                            if (b == .object) {
                                var b_iter = b.object.iterator();
                                while (b_iter.next()) |b_entry| {
                                    const b_cfg = try BinDependencyConfig.parseValue(allocator, b_entry.key_ptr.*, b_entry.value_ptr.*);
                                    try cent_entry.bin.put(try allocator.dupe(u8, b_entry.key_ptr.*), b_cfg);
                                }
                            }
                        }
                        if (val.object.get("sys")) |s| {
                            cent_entry.sys = try SysDependencyConfig.parseValue(allocator, s);
                        }
                        try index.libs.put(try allocator.dupe(u8, lib_name), cent_entry);
                    }
                }
            }
        }
        return index;
    }
};

pub const Manifest = struct {
    name: []const u8,
    version: []const u8,
    license: ?[]const u8 = null,
    description: ?[]const u8 = null,
    main: ?[]const u8 = null,
    mode: ?[]const u8 = null,
    workspaces: [][]const u8 = &.{},
    scripts: std.StringHashMap([]const u8),
    dependencies: std.StringHashMap([]const u8),
    dev_dependencies: std.StringHashMap([]const u8),
    bin_dependencies: std.StringHashMap(BinDependencyConfig),
    sys_dependencies: ?SysDependencyConfig = null,
    build_config: ?BuildConfig = null,
    nest_config: ?NestConfig = null,

    pub fn parse(allocator: std.mem.Allocator, contents: []const u8) !Manifest {
        var parsed = try std.json.parseFromSlice(std.json.Value, allocator, contents, .{});
        defer parsed.deinit();

        const root = parsed.value.object;

        const name = try allocator.dupe(u8, if (root.get("name")) |n| (if (n == .string) n.string else "unknown") else "unknown");
        const ver_val = root.get("ver") orelse root.get("version");
        const version = try allocator.dupe(u8, if (ver_val) |v| (if (v == .string) v.string else "0.1.0") else "0.1.0");

        var license: ?[]const u8 = null;
        if (root.get("license")) |l| {
            if (l == .string) {
                license = try allocator.dupe(u8, l.string);
            }
        }

        var description: ?[]const u8 = null;
        if (root.get("description")) |d| {
            if (d == .string) {
                description = try allocator.dupe(u8, d.string);
            }
        }

        var main: ?[]const u8 = null;
        if (root.get("main")) |m| {
            if (m == .string) {
                main = try allocator.dupe(u8, m.string);
            }
        }

        var mode: ?[]const u8 = null;
        if (root.get("mode")) |m| {
            if (m == .string) {
                mode = try allocator.dupe(u8, m.string);
            }
        }

        var scripts = std.StringHashMap([]const u8).init(allocator);
        if (root.get("scripts")) |s| {
            if (s == .object) {
                var iter = s.object.iterator();
                while (iter.next()) |entry| {
                    if (entry.value_ptr.* == .string) {
                        try scripts.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.string));
                    }
                }
            }
        }

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
        if (root.get("deps")) |d| {
            if (d == .object) {
                var iter = d.object.iterator();
                while (iter.next()) |entry| {
                    if (entry.value_ptr.* == .string) {
                        if (deps.fetchRemove(entry.key_ptr.*)) |old| {
                            allocator.free(old.key);
                            allocator.free(old.value);
                        }
                        try deps.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.string));
                    }
                }
            }
        }

        var dev_deps = std.StringHashMap([]const u8).init(allocator);
        const dev_sources = [_]?std.json.Value{
            root.get("dev_dependencies"),
            root.get("devDependencies"),
            root.get("dev"),
        };
        for (dev_sources) |dev_opt| {
            if (dev_opt) |dd| {
                if (dd == .object) {
                    var iter = dd.object.iterator();
                    while (iter.next()) |entry| {
                        if (entry.value_ptr.* == .string) {
                            if (dev_deps.fetchRemove(entry.key_ptr.*)) |old| {
                                allocator.free(old.key);
                                allocator.free(old.value);
                            }
                            try dev_deps.put(try allocator.dupe(u8, entry.key_ptr.*), try allocator.dupe(u8, entry.value_ptr.string));
                        }
                    }
                }
            }
        }

        var bin_deps = std.StringHashMap(BinDependencyConfig).init(allocator);
        const bin_sources = [_]?std.json.Value{
            root.get("bin_dependencies"),
            root.get("bin"),
        };
        for (bin_sources) |bin_opt| {
            if (bin_opt) |bd| {
                if (bd == .object) {
                    var iter = bd.object.iterator();
                    while (iter.next()) |entry| {
                        const b_cfg = try BinDependencyConfig.parseValue(allocator, entry.key_ptr.*, entry.value_ptr.*);
                        if (bin_deps.fetchRemove(entry.key_ptr.*)) |old| {
                            allocator.free(old.key);
                            var old_val = old.value;
                            old_val.deinit(allocator);
                        }
                        try bin_deps.put(try allocator.dupe(u8, entry.key_ptr.*), b_cfg);
                    }
                }
            }
        }

        var sys_deps: ?SysDependencyConfig = null;
        if (root.get("sys") orelse root.get("sys_dependencies")) |sd| {
            sys_deps = try SysDependencyConfig.parseValue(allocator, sd);
        }

        var build_cfg: ?BuildConfig = null;
        if (root.get("build")) |b| {
            if (b == .object) {
                var bc = BuildConfig{};
                if (b.object.get("entry")) |e| {
                    if (e == .string) {
                        bc.entry = try allocator.dupe(u8, e.string);
                    }
                }
                if (b.object.get("opt")) |o| {
                    if (o == .string) {
                        bc.opt = OptimizeMode.fromString(o.string);
                    }
                }
                if (b.object.get("out_dir")) |od| {
                    if (od == .string) {
                        bc.out_dir = try allocator.dupe(u8, od.string);
                    }
                }
                if (b.object.get("target")) |t| {
                    if (t == .string) {
                        bc.target = try allocator.dupe(u8, t.string);
                    }
                }
                if (b.object.get("targets")) |ts| {
                    if (ts == .array) {
                        var list = try allocator.alloc([]const u8, ts.array.items.len);
                        for (ts.array.items, 0..) |item, i| {
                            list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                        }
                        bc.targets = list;
                    }
                }
                if (b.object.get("c_flags")) |cfs| {
                    if (cfs == .array) {
                        var list = try allocator.alloc([]const u8, cfs.array.items.len);
                        for (cfs.array.items, 0..) |item, i| {
                            list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                        }
                        bc.c_flags = list;
                    }
                }
                if (b.object.get("link_libs")) |lls| {
                    if (lls == .array) {
                        var list = try allocator.alloc([]const u8, lls.array.items.len);
                        for (lls.array.items, 0..) |item, i| {
                            list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                        }
                        bc.link_libs = list;
                    }
                }
                if (b.object.get("sys_bin")) |sbs| {
                    if (sbs == .array) {
                        var list = try allocator.alloc([]const u8, sbs.array.items.len);
                        for (sbs.array.items, 0..) |item, i| {
                            list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                        }
                        bc.sys_bin = list;
                    }
                }
                if (b.object.get("sys")) |s| {
                    var buf = std.ArrayList(u8).initCapacity(allocator, 1024) catch unreachable;
                    defer buf.deinit(allocator);
                    var w = std.Io.Writer.fromArrayList(&buf);
                    try std.json.Stringify.value(s, .{}, &w);
                    bc.sys = try buf.toOwnedSlice(allocator);
                }
                build_cfg = bc;
            }
        }

        if (root.get("opt")) |o| {
            if (o == .string) {
                if (build_cfg != null) {
                    if (build_cfg.?.opt == null) build_cfg.?.opt = OptimizeMode.fromString(o.string);
                } else {
                    build_cfg = BuildConfig{ .opt = OptimizeMode.fromString(o.string) };
                }
            }
        }

        var nest_cfg: ?NestConfig = null;
        if (root.get("nest")) |n| {
            if (n == .object) {
                var nc = NestConfig{};
                if (n.object.get("mailbox_capacity")) |mc| {
                    if (mc == .integer) {
                        nc.mailbox_capacity = @intCast(mc.integer);
                    }
                }
                if (n.object.get("reduction_slice")) |rs| {
                    if (rs == .integer) {
                        nc.reduction_slice = @intCast(rs.integer);
                    }
                }
                nest_cfg = nc;
            }
        }

        var workspaces: [][]const u8 = &.{};
        if (root.get("workspaces") orelse root.get("ws")) |ws| {
            if (ws == .array) {
                var list = try allocator.alloc([]const u8, ws.array.items.len);
                for (ws.array.items, 0..) |item, i| {
                    list[i] = try allocator.dupe(u8, if (item == .string) item.string else "");
                }
                workspaces = list;
            }
        }

        return Manifest{
            .name = name,
            .version = version,
            .license = license,
            .description = description,
            .main = main,
            .mode = mode,
            .workspaces = workspaces,
            .scripts = scripts,
            .dependencies = deps,
            .dev_dependencies = dev_deps,
            .bin_dependencies = bin_deps,
            .sys_dependencies = sys_deps,
            .build_config = build_cfg,
            .nest_config = nest_cfg,
        };
    }

    pub fn save(self: *Manifest, allocator: std.mem.Allocator, path: []const u8) !void {
        var out_file = try @import("../main.zig").fs_shim.cwd().createFile(path, .{});
        defer out_file.close(@import("../main.zig").global_io);

        var arena = std.heap.ArenaAllocator.init(allocator);
        defer arena.deinit();
        const aa = arena.allocator();

        var root = std.json.Value{ .object = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable };
        try root.object.put(aa, "name", std.json.Value{ .string = self.name });
        try root.object.put(aa, "ver", std.json.Value{ .string = self.version });
        if (self.license) |l| {
            try root.object.put(aa, "license", std.json.Value{ .string = l });
        }
        if (self.description) |d| {
            try root.object.put(aa, "description", std.json.Value{ .string = d });
        }
        if (self.main) |m| {
            try root.object.put(aa, "main", std.json.Value{ .string = m });
        }
        if (self.mode) |m| {
            try root.object.put(aa, "mode", std.json.Value{ .string = m });
        }

        if (self.workspaces.len > 0) {
            var ws_arr = std.json.Array.init(aa);
            for (self.workspaces) |w| {
                try ws_arr.append(std.json.Value{ .string = w });
            }
            try root.object.put(aa, "workspaces", std.json.Value{ .array = ws_arr });
        }

        if (self.scripts.count() > 0) {
            var scripts_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            var script_iter = self.scripts.iterator();
            while (script_iter.next()) |entry| {
                try scripts_map.put(aa, entry.key_ptr.*, std.json.Value{ .string = entry.value_ptr.* });
            }
            try root.object.put(aa, "scripts", std.json.Value{ .object = scripts_map });
        }

        if (self.dependencies.count() > 0) {
            var deps_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            var dep_iter = self.dependencies.iterator();
            while (dep_iter.next()) |entry| {
                try deps_map.put(aa, entry.key_ptr.*, std.json.Value{ .string = entry.value_ptr.* });
            }
            try root.object.put(aa, "deps", std.json.Value{ .object = deps_map });
        }

        if (self.dev_dependencies.count() > 0) {
            var dev_deps_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            var dev_dep_iter = self.dev_dependencies.iterator();
            while (dev_dep_iter.next()) |entry| {
                try dev_deps_map.put(aa, entry.key_ptr.*, std.json.Value{ .string = entry.value_ptr.* });
            }
            try root.object.put(aa, "dev", std.json.Value{ .object = dev_deps_map });
        }

        if (self.bin_dependencies.count() > 0) {
            var bin_deps_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            var bin_dep_iter = self.bin_dependencies.iterator();
            while (bin_dep_iter.next()) |entry| {
                const b = entry.value_ptr.*;
                if (b.version != null and b.sources.len == 0 and b.path == null) {
                    try bin_deps_map.put(aa, entry.key_ptr.*, std.json.Value{ .string = b.version.? });
                } else {
                    var obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
                    if (b.path) |p| try obj.put(aa, "path", std.json.Value{ .string = p });
                    if (b.sources.len > 0) {
                        var arr = std.json.Array.init(aa);
                        for (b.sources) |s| try arr.append(std.json.Value{ .string = s });
                        try obj.put(aa, "src", std.json.Value{ .array = arr });
                    }
                    if (b.include_dirs.len > 0) {
                        var arr = std.json.Array.init(aa);
                        for (b.include_dirs) |id| try arr.append(std.json.Value{ .string = id });
                        try obj.put(aa, "include", std.json.Value{ .array = arr });
                    }
                    if (b.c_flags.len > 0) {
                        var arr = std.json.Array.init(aa);
                        for (b.c_flags) |cf| try arr.append(std.json.Value{ .string = cf });
                        try obj.put(aa, "flag", std.json.Value{ .array = arr });
                    }
                    if (b.opt) |o| try obj.put(aa, "opt", std.json.Value{ .string = o.toConfigString() });
                    if (b.prebuilt) |pb| try obj.put(aa, "prebuilt", std.json.Value{ .string = pb });
                    try bin_deps_map.put(aa, entry.key_ptr.*, std.json.Value{ .object = obj });
                }
            }
            try root.object.put(aa, "bin", std.json.Value{ .object = bin_deps_map });
        }

        if (self.sys_dependencies) |sys| {
            var sys_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            for (sys.libs) |lib| {
                const key = lib.name orelse continue;
                var obj = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
                if (lib.linux) |l| try obj.put(aa, "linux", std.json.Value{ .string = l });
                if (lib.mac) |m| try obj.put(aa, "mac", std.json.Value{ .string = m });
                if (lib.win) |w| try obj.put(aa, "win", std.json.Value{ .string = w });
                if (lib.path) |p| try obj.put(aa, "path", std.json.Value{ .string = p });
                if (lib.prebuilt) |pb| try obj.put(aa, "prebuilt", std.json.Value{ .string = pb });
                try sys_map.put(aa, key, std.json.Value{ .object = obj });
            }
            try root.object.put(aa, "sys", std.json.Value{ .object = sys_map });
        }

        if (self.build_config) |bc| {
            var b_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            if (bc.entry) |e| {
                try b_map.put(aa, "entry", std.json.Value{ .string = e });
            }
            if (bc.opt) |opt| {
                try b_map.put(aa, "opt", std.json.Value{ .string = opt.toConfigString() });
            }
            if (bc.out_dir) |od| {
                try b_map.put(aa, "out_dir", std.json.Value{ .string = od });
            }
            if (bc.target) |t| {
                try b_map.put(aa, "target", std.json.Value{ .string = t });
            }
            if (bc.targets.len > 0) {
                var arr = std.json.Array.init(aa);
                for (bc.targets) |t| {
                    try arr.append(std.json.Value{ .string = t });
                }
                try b_map.put(aa, "targets", std.json.Value{ .array = arr });
            }
            if (bc.c_flags.len > 0) {
                var arr = std.json.Array.init(aa);
                for (bc.c_flags) |cf| {
                    try arr.append(std.json.Value{ .string = cf });
                }
                try b_map.put(aa, "c_flags", std.json.Value{ .array = arr });
            }
            if (bc.link_libs.len > 0) {
                var arr = std.json.Array.init(aa);
                for (bc.link_libs) |ll| {
                    try arr.append(std.json.Value{ .string = ll });
                }
                try b_map.put(aa, "link_libs", std.json.Value{ .array = arr });
            }
            if (bc.sys_bin.len > 0) {
                var arr = std.json.Array.init(aa);
                for (bc.sys_bin) |sb| {
                    try arr.append(std.json.Value{ .string = sb });
                }
                try b_map.put(aa, "sys_bin", std.json.Value{ .array = arr });
            }
            if (bc.sys) |s| {
                const parsed = try std.json.parseFromSlice(std.json.Value, aa, s, .{});
                try b_map.put(aa, "sys", parsed.value);
            }
            try root.object.put(aa, "build", std.json.Value{ .object = b_map });
        }

        if (self.nest_config) |nc| {
            var n_map = std.json.ObjectMap.init(aa, &.{}, &.{}) catch unreachable;
            if (nc.mailbox_capacity) |mc| {
                try n_map.put(aa, "mailbox_capacity", std.json.Value{ .integer = @intCast(mc) });
            }
            if (nc.reduction_slice) |rs| {
                try n_map.put(aa, "reduction_slice", std.json.Value{ .integer = @intCast(rs) });
            }
            try root.object.put(aa, "nest", std.json.Value{ .object = n_map });
        }

        var buf: [8192]u8 = undefined;
        var writer = out_file.writer(@import("../main.zig").global_io, &buf);
        try std.json.Stringify.value(root, .{ .whitespace = .indent_2 }, &writer.interface);
        try writer.end();
    }

    pub fn addDependency(self: *Manifest, allocator: std.mem.Allocator, name: []const u8, version_range: []const u8, is_dev: bool) !void {
        const name_dup = try allocator.dupe(u8, name);
        const ver_dup = try allocator.dupe(u8, version_range);
        if (is_dev) {
            if (self.dev_dependencies.fetchRemove(name)) |kv| {
                allocator.free(kv.key);
                allocator.free(kv.value);
            }
            try self.dev_dependencies.put(name_dup, ver_dup);
        } else {
            if (self.dependencies.fetchRemove(name)) |kv| {
                allocator.free(kv.key);
                allocator.free(kv.value);
            }
            try self.dependencies.put(name_dup, ver_dup);
        }
    }

    pub fn removeDependency(self: *Manifest, allocator: std.mem.Allocator, name: []const u8) bool {
        var found = false;
        if (self.dependencies.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            allocator.free(kv.value);
            found = true;
        }
        if (self.dev_dependencies.fetchRemove(name)) |kv| {
            allocator.free(kv.key);
            allocator.free(kv.value);
            found = true;
        }
        return found;
    }

    pub fn deinit(self: *Manifest, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        allocator.free(self.version);
        if (self.license) |l| allocator.free(l);
        if (self.description) |d| allocator.free(d);
        if (self.main) |m| allocator.free(m);
        if (self.mode) |m| allocator.free(m);
        var script_iter = self.scripts.iterator();
        while (script_iter.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        self.scripts.deinit();
        var dep_iter = self.dependencies.iterator();
        while (dep_iter.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        self.dependencies.deinit();
        var dev_dep_iter = self.dev_dependencies.iterator();
        while (dev_dep_iter.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            allocator.free(entry.value_ptr.*);
        }
        self.dev_dependencies.deinit();
        var bin_dep_iter = self.bin_dependencies.iterator();
        while (bin_dep_iter.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            entry.value_ptr.deinit(allocator);
        }
        self.bin_dependencies.deinit();
        if (self.sys_dependencies) |*sd| {
            sd.deinit(allocator);
        }
        for (self.workspaces) |w| allocator.free(w);
        if (self.workspaces.len > 0) allocator.free(self.workspaces);
        if (self.build_config) |*bc| {
            bc.deinit(allocator);
        }
    }

    pub fn verifyLockfile(self: *Manifest, lk: lock.Lockfile) !void {
        var it = self.dependencies.iterator();
        while (it.next()) |entry| {
            if (lk.dependencies.get(entry.key_ptr.*)) |ver| {
                if (!std.mem.eql(u8, ver, entry.value_ptr.*)) {
                    return error.LockfileMismatch;
                }
            } else {
                return error.LockfileMissingDependency;
            }
        }
        var dev_it = self.dev_dependencies.iterator();
        while (dev_it.next()) |entry| {
            if (lk.dev_dependencies.get(entry.key_ptr.*)) |ver| {
                if (!std.mem.eql(u8, ver, entry.value_ptr.*)) {
                    return error.LockfileMismatch;
                }
            } else {
                return error.LockfileMissingDependency;
            }
        }
    }
};
