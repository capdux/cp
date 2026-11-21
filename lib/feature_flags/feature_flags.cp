// Source: absorbed from bun/src/bun_core/{feature_flags,env_var}.cp (cap:feature_flags)
const std = @import("std");

pub const FeatureFlags = struct {
    breaking_changes_1_4: bool = false,
    store_file_descriptors: bool = true,
    tracing: bool = true,
    css_supports_fence: bool = true,
    enable_entry_cache: bool = true,
    verbose_fs: bool = false,
    watch_directories: bool = true,
    allow_json_single_quotes: bool = true,
    disable_compression_in_http_client: bool = false,
    enable_keepalive: bool = true,
    http_buffer_pooling: bool = true,
    support_jsxs_in_jsx_transform: bool = true,
    inline_properties_in_transpiler: bool = true,
    unwrap_commonjs_to_esm: bool = true,
};

pub fn parseBoolEnv(raw: []const u8): bool? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) { return null; }
    if (std.ascii.eqlIgnoreCase(s, "1") or std.ascii.eqlIgnoreCase(s, "true") or
        std.ascii.eqlIgnoreCase(s, "yes") or std.ascii.eqlIgnoreCase(s, "on"))
    {
        return true;
    }
    if (std.ascii.eqlIgnoreCase(s, "0") or std.ascii.eqlIgnoreCase(s, "false") or
        std.ascii.eqlIgnoreCase(s, "no") or std.ascii.eqlIgnoreCase(s, "off"))
    {
        return false;
    }
    return null;
}

pub fn parseUintEnv(raw: []const u8): u64? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) { return null; }
    return std.fmt.parseInt(u64, s, 10) catch null;
}

pub fn parseBoolOr(raw: ([]const u8)?, default_val: bool): bool {
    if (v := raw) {
        return parseBoolEnv(v) orelse default_val;
    }
    return default_val;
}

pub fn parseUintOr(raw: ([]const u8)?, default_val: u64): u64 {
    if (v := raw) {
        return parseUintEnv(v) orelse default_val;
    }
    return default_val;
}
