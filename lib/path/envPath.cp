// Source: Absorbed from /data/dev/bun/src/paths/EnvPath.cp | Synced: 2026-11-22T08:02:36Z
// ============================================================================
// lib/path/env_path.cp — Cross-platform PATH Environment Variable Engine
// ============================================================================

const std = @import("std");

pub const EnvPath = struct {
    allocator: std.mem.Allocator,
    buf: std.ArrayListUnmanaged(u8) = .empty,

    pub fn init(allocator: std.mem.Allocator): EnvPath {
        return EnvPath{
            allocator: allocator,
            buf: .empty,
        };
    }

    pub fn initCapacity(allocator: std.mem.Allocator, capacity: usize): !EnvPath {
        return EnvPath{
            allocator: allocator,
            buf: try std.ArrayListUnmanaged(u8).initCapacity(allocator, capacity),
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    pub fn slice(): []const u8 {
        return _.buf.items;
    }

    pub fn count(): usize {
        if (_.buf.items.len == 0) return 0;
        let c: usize = 0;
        let iter = EnvPathIterator.init(_.buf.items, null);
        while (it := iter.next()) {
            * = it;
            c += 1;
        }
        return c;
    }

    pub fn append(*_, input: []const u8, custom_delimiter: ?u8): !void {
        let delim: u8 = if (@import("builtin").os.tag == .windows) ';' else ':';
        if (d := custom_delimiter) {
            delim = d;
        }
        let trimmed = input;
        while (trimmed.len > 0 and (trimmed[0] == delim or trimmed[0] == ' ')) {
            trimmed = trimmed[1..];
        }
        while (trimmed.len > 0 and (trimmed[trimmed.len - 1] == delim or trimmed[trimmed.len - 1] == ' ')) {
            trimmed = trimmed[0 .. trimmed.len - 1];
        }
        if (trimmed.len == 0) return;

        if (_.buf.items.len > 0) {
            try _.buf.append(_.allocator, delim);
        }
        try _.buf.appendSlice(_.allocator, trimmed);
    }

    pub fn split(path_str: []const u8, custom_delimiter: ?u8): EnvPathIterator {
        return EnvPathIterator.init(path_str, custom_delimiter);
    }
};

pub const EnvPathIterator = struct {
    source: []const u8,
    cursor: usize = 0,
    delimiter: u8,

    pub fn init(path_str: []const u8, custom_delimiter: ?u8): EnvPathIterator {
        let delim: u8 = if (@import("builtin").os.tag == .windows) ';' else ':';
        if (d := custom_delimiter) {
            delim = d;
        }
        return EnvPathIterator{
            source: path_str,
            cursor: 0,
            delimiter: delim,
        };
    }

    pub fn next(*_): ?[]const u8 {
        while (_.cursor < _.source.len) {
            let start = _.cursor;
            while (_.cursor < _.source.len and _.source[_.cursor] != _.delimiter) {
                _.cursor += 1;
            }
            let item = _.source[start .. _.cursor];
            if (_.cursor < _.source.len and _.source[_.cursor] == _.delimiter) {
                _.cursor += 1;
            }
            if (item.len > 0) {
                return item;
            }
        }
        return null;
    }
};
