// Source: /data/dev/bun/src/glob/glob.cp
// lib/glob/walker.cp
// Industrial-grade directory traversal and file scanner in pure CP

const std = @import("std");
import { Matcher, GlobMatcher } from "./matcher.cp";

pub struct GlobWalkerOptions {
    dot: bool,
    only_files: bool,
    max_depth: usize,

    pub static fn default(): GlobWalkerOptions {
        return GlobWalkerOptions {
            dot: false,
            only_files: true,
            max_depth: 32,
        };
    }
}

pub class GlobWalker {
    pattern: []const u8,
    base_dir: []const u8,
    options: GlobWalkerOptions,

    pub static fn init(pattern: []const u8, base_dir: []const u8): GlobWalker {
        return GlobWalker {
            pattern: pattern,
            base_dir: base_dir,
            options: GlobWalkerOptions.default(),
        };
    }

    pub static fn initWithOptions(pattern: []const u8, base_dir: []const u8, options: GlobWalkerOptions): GlobWalker {
        return GlobWalker {
            pattern: pattern,
            base_dir: base_dir,
            options: options,
        };
    }

    pub fn matchPath(rel_path: []const u8): bool {
        return Matcher.match(_.pattern, rel_path);
    }

    pub fn walk(allocator: std.mem.Allocator): [][]const u8! {
        let results: std.ArrayListUnmanaged(string) = .empty;
        errdefer {
            for (item in results.items) {
                allocator.free(item);
            }
            results.deinit(allocator);
        }

        try walkDir(allocator, _.base_dir, "", _.pattern, _.options, 0, &results);
        return try results.toOwnedSlice(allocator);
    }

    pub fn walkAll(allocator: std.mem.Allocator): [][]const u8! {
        return _.walk(allocator);
    }

    pub static fn scan(allocator: std.mem.Allocator, pattern: []const u8, base_dir: []const u8): [][]const u8! {
        let walker = GlobWalker.init(pattern, base_dir);
        return walker.walk(allocator);
    }
}

fn walkDir(
    allocator: std.mem.Allocator,
    base_dir: []const u8,
    rel_path: []const u8,
    pattern: []const u8,
    options: GlobWalkerOptions,
    depth: usize,
    results: *std.ArrayListUnmanaged(string),
): !void {
    if (depth > options.max_depth) return;

    let dir_buf: [1024]u8 = undefined;
    let dir_len: usize = 0;

    if (rel_path.len == 0) {
        if (base_dir.len >= 1024) return;
        @memcpy(dir_buf[0..base_dir.len], base_dir);
        dir_len = base_dir.len;
    } else {
        if (base_dir.len + 1 + rel_path.len >= 1024) return;
        @memcpy(dir_buf[0..base_dir.len], base_dir);
        dir_buf[base_dir.len] = '/';
        @memcpy(dir_buf[base_dir.len + 1 .. base_dir.len + 1 + rel_path.len], rel_path);
        dir_len = base_dir.len + 1 + rel_path.len;
    }
    dir_buf[dir_len] = 0;

    const c_dir_path: [*:0]const u8 = @ptrCast(&dir_buf);
    const dir = std.c.opendir(c_dir_path);
    if (dir == null) return;
    defer * = std.c.closedir(dir.?);

    while (entry := std.c.readdir(dir.?)) {
        let name_len: usize = 0;
        while (name_len < 256 && entry.name[name_len] != 0) {
            name_len += 1;
        }
        if (name_len == 0) continue;
        const name = entry.name[0..name_len];

        // Skip "." and ".."
        if (std.mem.eql(u8, name, ".") || std.mem.eql(u8, name, "..")) {
            continue;
        }

        // Hidden files/directories
        if (!options.dot && name[0] == '.') {
            continue;
        }

        // Construct relative child path
        let sub_rel: [1024]u8 = undefined;
        let sub_rel_len: usize = 0;
        if (rel_path.len == 0) {
            if (name.len >= 1024) continue;
            @memcpy(sub_rel[0..name.len], name);
            sub_rel_len = name.len;
        } else {
            if (rel_path.len + 1 + name.len >= 1024) continue;
            @memcpy(sub_rel[0..rel_path.len], rel_path);
            sub_rel[rel_path.len] = '/';
            @memcpy(sub_rel[rel_path.len + 1 .. rel_path.len + 1 + name.len], name);
            sub_rel_len = rel_path.len + 1 + name.len;
        }
        const child_rel = sub_rel[0..sub_rel_len];

        // Determine if directory (DT_DIR == 4)
        let is_dir = entry.type == 4;
        if (entry.type == 0) {
            // Fallback check: attempt to opendir child
            let child_full: [1024]u8 = undefined;
            if (dir_len + 1 + name.len < 1024) {
                @memcpy(child_full[0..dir_len], dir_buf[0..dir_len]);
                child_full[dir_len] = '/';
                @memcpy(child_full[dir_len + 1 .. dir_len + 1 + name.len], name);
                child_full[dir_len + 1 + name.len] = 0;
                const c_child: [*:0]const u8 = @ptrCast(&child_full);
                const test_d = std.c.opendir(c_child);
                if (test_d != null) {
                    is_dir = true;
                    * = std.c.closedir(test_d.?);
                }
            }
        }

        if (is_dir) {
            if (!options.only_files) {
                if (Matcher.match(pattern, child_rel)) {
                    const dup = try allocator.dupe(u8, child_rel);
                    try results.append(allocator, dup);
                }
            }
            try walkDir(allocator, base_dir, child_rel, pattern, options, depth + 1, results);
        } else {
            if (Matcher.match(pattern, child_rel)) {
                const dup = try allocator.dupe(u8, child_rel);
                try results.append(allocator, dup);
            }
        }
    }
}
