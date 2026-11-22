// Source: Absorbed from /data/dev/bun/src/interchange/interchange.cp | Synced: 2026-11-22T11:07:59Z
// lib/conf/getter.cp
// Type-Safe Configuration Value Parser & Unified ConfGetter (Booleans, Numbers, Ports, Durations & Byte Sizes)

const std = @import("std");

pub struct ConfValueParser {
    /// Parses a configuration boolean string (`true`/`1`/`yes`/`on` vs `false`/`0`/`no`/`off`, case-insensitive).
    pub static fn parseBool(raw: []const u8): bool? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "true") ||
            std.ascii.eqlIgnoreCase(s, "1") ||
            std.ascii.eqlIgnoreCase(s, "yes") ||
            std.ascii.eqlIgnoreCase(s, "on"))
        {
            return true;
        }
        if (std.ascii.eqlIgnoreCase(s, "false") ||
            std.ascii.eqlIgnoreCase(s, "0") ||
            std.ascii.eqlIgnoreCase(s, "no") ||
            std.ascii.eqlIgnoreCase(s, "off"))
        {
            return false;
        }
        return null;
    }

    /// Parses an unsigned 64-bit integer from a decimal string.
    pub static fn parseU64(raw: []const u8): u64? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) {
            return null;
        }
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return null;
            }
            acc = acc * 10 + ((c - '0') as u64);
            i += 1;
        }
        return acc;
    }

    /// Parses a signed 64-bit integer from a decimal string.
    pub static fn parseI64(raw: []const u8): i64? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        return std.fmt.parseInt(i64, s, 10) catch null;
    }

    /// Parses a 64-bit floating-point number from a string.
    pub static fn parseF64(raw: []const u8): f64? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        return std.fmt.parseFloat(f64, s) catch null;
    }

    /// Parses a valid TCP/UDP port number (`1..65535`).
    pub static fn parsePort(raw: []const u8): u16? {
        const val_opt = ConfValueParser.parseU64(raw);
        if (val_opt == null) {
            return null;
        }
        const val = val_opt.?;
        if (val == 0 || val > 65535) {
            return null;
        }
        return val as u16;
    }

    /// Parses a duration string with unit suffix (`ms`, `s`, `m`, `h`) into milliseconds.
    pub static fn parseDurationMs(raw: []const u8): u64? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) {
            return null;
        }
        let digit_end: usize = 0;
        while (digit_end < s.len && s[digit_end] >= '0' && s[digit_end] <= '9') {
            digit_end += 1;
        }
        if (digit_end == 0) {
            return null;
        }
        const num_opt = ConfValueParser.parseU64(s[0..digit_end]);
        if (num_opt == null) {
            return null;
        }
        const num = num_opt.?;
        const unit = std.mem.trim(u8, s[digit_end..s.len], " \t");
        if (unit.len == 0 || std.ascii.eqlIgnoreCase(unit, "ms")) {
            return num;
        }
        if (std.ascii.eqlIgnoreCase(unit, "s")) {
            return num * 1000;
        }
        if (std.ascii.eqlIgnoreCase(unit, "m")) {
            return num * 60000;
        }
        if (std.ascii.eqlIgnoreCase(unit, "h")) {
            return num * 3600000;
        }
        return null;
    }

    /// Parses a byte size string with unit suffix (`b`, `kb`, `mb`, `gb`, base-1024) into bytes.
    pub static fn parseByteSize(raw: []const u8): u64? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) {
            return null;
        }
        let digit_end: usize = 0;
        while (digit_end < s.len && s[digit_end] >= '0' && s[digit_end] <= '9') {
            digit_end += 1;
        }
        if (digit_end == 0) {
            return null;
        }
        const num_opt = ConfValueParser.parseU64(s[0..digit_end]);
        if (num_opt == null) {
            return null;
        }
        const num = num_opt.?;
        const unit = std.mem.trim(u8, s[digit_end..s.len], " \t");
        if (unit.len == 0 || std.ascii.eqlIgnoreCase(unit, "b")) {
            return num;
        }
        if (std.ascii.eqlIgnoreCase(unit, "k") || std.ascii.eqlIgnoreCase(unit, "kb") || std.ascii.eqlIgnoreCase(unit, "kib")) {
            return num * 1024;
        }
        if (std.ascii.eqlIgnoreCase(unit, "m") || std.ascii.eqlIgnoreCase(unit, "mb") || std.ascii.eqlIgnoreCase(unit, "mib")) {
            return num * 1048576;
        }
        if (std.ascii.eqlIgnoreCase(unit, "g") || std.ascii.eqlIgnoreCase(unit, "gb") || std.ascii.eqlIgnoreCase(unit, "gib")) {
            return num * 1073741824;
        }
        return null;
    }

    /// Unified fallback extractors accepting optional raw slices from INI/TOML/YAML/DotEnv
    pub static fn getStringOr(raw: ([]const u8)?, default_val: []const u8): []const u8 {
        if (v := raw) return v;
        return default_val;
    }

    pub static fn getBoolOr(raw: ([]const u8)?, default_val: bool): bool {
        if (v := raw) {
            if (b := ConfValueParser.parseBool(v)) return b;
        }
        return default_val;
    }

    pub static fn getU64Or(raw: ([]const u8)?, default_val: u64): u64 {
        if (v := raw) {
            if (n := ConfValueParser.parseU64(v)) return n;
        }
        return default_val;
    }

    pub static fn getI64Or(raw: ([]const u8)?, default_val: i64): i64 {
        if (v := raw) {
            if (n := ConfValueParser.parseI64(v)) return n;
        }
        return default_val;
    }

    pub static fn getF64Or(raw: ([]const u8)?, default_val: f64): f64 {
        if (v := raw) {
            if (n := ConfValueParser.parseF64(v)) return n;
        }
        return default_val;
    }

    pub static fn getPortOr(raw: ([]const u8)?, default_val: u16): u16 {
        if (v := raw) {
            if (p := ConfValueParser.parsePort(v)) return p;
        }
        return default_val;
    }

    pub static fn getDurationMsOr(raw: ([]const u8)?, default_val: u64): u64 {
        if (v := raw) {
            if (d := ConfValueParser.parseDurationMs(v)) return d;
        }
        return default_val;
    }

    pub static fn getByteSizeOr(raw: ([]const u8)?, default_val: u64): u64 {
        if (v := raw) {
            if (sz := ConfValueParser.parseByteSize(v)) return sz;
        }
        return default_val;
    }
}

pub const ConfGetter = ConfValueParser;
