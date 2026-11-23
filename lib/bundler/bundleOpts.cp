// Source: absorbed from bun/src/options_types/{BundleEnums,GlobalCache,OfflineMode}.cp (cap:bundle_options)
const std = @import("std");

pub const Format = enum {
    esm,
    iife,
    cjs,
    internal_bake_dev,

    pub fn keepES6ImportExportSyntax(self: Format): bool {
        return self == Format.esm;
    }

    pub fn isESM(self: Format): bool {
        return self == Format.esm;
    }

    pub fn isAlwaysStrictMode(self: Format): bool {
        return self == Format.esm;
    }
};

pub fn parseFormat(s: []const u8): Format? {
    return std.StaticStringMap(Format).initComptime(.{
        .{ "esm", .esm },
        .{ "cjs", .cjs },
        .{ "iife", .iife },
        .{ "internal_bake_dev", .internal_bake_dev },
    }).get(s);
}

pub const BundlePackage = enum {
    always,
    never,
};

pub const GlobalCache = enum {
    allow_install,
    read_only,
    @"auto",
    force,
    fallback,
    disable,

    pub fn allowVersionSpecifier(self: GlobalCache): bool {
        return self == .force;
    }

    pub fn canUse(self: GlobalCache, has_node_modules: bool): bool {
        if (has_node_modules) {
            return switch (self) {
                .fallback, .allow_install, .force => true,
                .read_only, .disable, .@"auto" => false,
            };
        }
        return switch (self) {
            .read_only, .fallback, .allow_install, .@"auto", .force => true,
            .disable => false,
        };
    }

    pub fn isEnabled(self: GlobalCache): bool {
        return self != .disable;
    }

    pub fn canInstall(self: GlobalCache): bool {
        return switch (self) {
            .@"auto", .allow_install, .force, .fallback => true,
            else => false,
        };
    }
};

pub fn parseGlobalCache(s: []const u8): GlobalCache? {
    return std.StaticStringMap(GlobalCache).initComptime(.{
        .{ "auto", .@"auto" },
        .{ "force", .force },
        .{ "disable", .disable },
        .{ "fallback", .fallback },
    }).get(s);
}

pub const OfflineMode = enum {
    online,
    latest,
    offline,
};

pub fn parseOfflineMode(s: []const u8): OfflineMode? {
    return std.StaticStringMap(OfflineMode).initComptime(.{
        .{ "offline", .offline },
        .{ "latest", .latest },
        .{ "online", .online },
    }).get(s);
}

pub const NodeLinker = enum {
    hoisted,
    isolated,
};

pub fn parseNodeLinker(s: []const u8): NodeLinker? {
    return std.StaticStringMap(NodeLinker).initComptime(.{
        .{ "hoisted", .hoisted },
        .{ "isolated", .isolated },
    }).get(s);
}

pub const WindowsOptions = struct {
    hide_console: bool = false,
    icon: ([]const u8)? = null,
    title: ([]const u8)? = null,
    publisher: ([]const u8)? = null,
    version: ([]const u8)? = null,
    description: ([]const u8)? = null,
    copyright: ([]const u8)? = null,
};
