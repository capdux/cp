// Source: Absorbed from /data/dev/bun/src/glob/GlobWalker.cp | Synced: 2026-11-22T07:47:14Z
// lib/glob/walker.cp
// Industrial-grade directory traversal and file scanner in pure CP

const std = @import("std");
import { Matcher, GlobMatcher, GlobMatchOptions } from "./matcher.cp";

pub struct GlobWalkerOptions {
    dot: bool = false,
    only_files: bool = true,
    onlyFiles: bool? = null,
    absolute: bool = false,
    case_sensitive: bool = true,
    follow_symlinks: bool = false,
    max_depth: usize = 64,
    cwd: []const u8 = ".",

    pub static fn default(): GlobWalkerOptions {
        return GlobWalkerOptions {
            dot: false,
            only_files: true,
            onlyFiles: null,
            absolute: false,
            case_sensitive: true,
            follow_symlinks: false,
            max_depth: 64,
            cwd: ".",
        };
    }

    pub fn isOnlyFiles(): bool {
        if (v := _.onlyFiles) {
            return v;
        }
        return _.only_files;
    }

    pub fn toMatchOptions(): GlobMatchOptions {
        return GlobMatchOptions {
            case_sensitive: _.case_sensitive,
            dot: _.dot,
            normalize: true,
            windows_paths: false,
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
        return Matcher.matchWithOptions(_.pattern, rel_path, _.options.toMatchOptions());
    }

    pub fn walk(allocator: std.mem.Allocator): [][]const u8! {
        let results: std.ArrayListUnmanaged(string) = .empty;
        errdefer {
            for (item of results.items) {
                allocator.free(item);
            }
            results.deinit(allocator);
        }

        const effective_base = if (_.base_dir.len > 0) _.base_dir else _.options.cwd;
        let abs_base: []const u8 = "";
        let owned_abs_base: ([]u8)? = null;
        defer {
            if (ab := owned_abs_base) {
                allocator.free(ab);
            }
        }

        if (_.options.absolute) {
            let cwd_buf: [4096]u8 = undefined;
            const cwd_ptr = std.c.getcwd(&cwd_buf, cwd_buf.len);
            let cwd_str: []const u8 = effective_base;
            if (cp := cwd_ptr) {
                cwd_str = std.mem.sliceTo(cp, 0);
            }
            if (effective_base.len > 0 && effective_base[0] == '/') {
                let norm_buf: [4096]u8 = undefined;
                const norm = Matcher.normalizePath(effective_base, &norm_buf, false);
                const dup_ab = try allocator.dupe(u8, norm);
                abs_base = dup_ab;
                owned_abs_base = dup_ab;
            } else {
                let join_buf: [4096]u8 = undefined;
                if (cwd_str.len + 1 + effective_base.len < join_buf.len) {
                    @memcpy(join_buf[0..cwd_str.len], cwd_str);
                    join_buf[cwd_str.len] = '/';
                    @memcpy(join_buf[cwd_str.len + 1 .. cwd_str.len + 1 + effective_base.len], effective_base);
                    let norm_buf: [4096]u8 = undefined;
                    const norm = Matcher.normalizePath(join_buf[0 .. cwd_str.len + 1 + effective_base.len], &norm_buf, false);
                    const dup_ab = try allocator.dupe(u8, norm);
                    abs_base = dup_ab;
                    owned_abs_base = dup_ab;
                } else {
                    abs_base = effective_base;
                }
            }
        }

        try walkDir(allocator, effective_base, abs_base, "", _.pattern, _.options, 0, &results);
        return try results.toOwnedSlice(allocator);
    }

    pub fn walkAll(allocator: std.mem.Allocator): [][]const u8! {
        return _.walk(allocator);
    }

    pub static fn scan(allocator: std.mem.Allocator, pattern: []const u8, base_dir: []const u8): [][]const u8! {
        let walker = GlobWalker.init(pattern, base_dir);
        return walker.walk(allocator);
    }

    pub static fn scanSync(allocator: std.mem.Allocator, pattern: []const u8, base_dir: []const u8, options: GlobWalkerOptions): [][]const u8! {
        let walker = GlobWalker.initWithOptions(pattern, base_dir, options);
        return walker.walk(allocator);
    }
}

fn emitMatch(
    allocator: std.mem.Allocator,
    abs_base: []const u8,
    child_rel: []const u8,
    options: GlobWalkerOptions,
    results: *std.ArrayListUnmanaged(string),
): !void {
    if (options.absolute && abs_base.len > 0) {
        let out_buf: [4096]u8 = undefined;
        let base_trimmed = abs_base;
        while (base_trimmed.len > 1 && base_trimmed[base_trimmed.len - 1] == '/') {
            base_trimmed = base_trimmed[0 .. base_trimmed.len - 1];
        }
        const need_slash = !(base_trimmed.len == 1 && base_trimmed[0] == '/');
        const extra: usize = if (need_slash) 1 else 0;
        if (base_trimmed.len + extra + child_rel.len <= out_buf.len) {
            @memcpy(out_buf[0..base_trimmed.len], base_trimmed);
            if (need_slash) {
                out_buf[base_trimmed.len] = '/';
            }
            @memcpy(out_buf[base_trimmed.len + extra .. base_trimmed.len + extra + child_rel.len], child_rel);
            const dup = try allocator.dupe(u8, out_buf[0 .. base_trimmed.len + extra + child_rel.len]);
            try results.append(allocator, dup);
            return;
        }
    }
    const dup = try allocator.dupe(u8, child_rel);
    try results.append(allocator, dup);
}

fn walkDir(
    allocator: std.mem.Allocator,
    base_dir: []const u8,
    abs_base: []const u8,
    rel_path: []const u8,
    pattern: []const u8,
    options: GlobWalkerOptions,
    depth: usize,
    results: *std.ArrayListUnmanaged(string),
): !void {
    if (depth > options.max_depth) return;

    let dir_buf: [4096]u8 = undefined;
    let dir_len: usize = 0;

    if (rel_path.len == 0) {
        if (base_dir.len >= dir_buf.len) return;
        @memcpy(dir_buf[0..base_dir.len], base_dir);
        dir_len = base_dir.len;
    } else {
        if (base_dir.len + 1 + rel_path.len >= dir_buf.len) return;
        @memcpy(dir_buf[0..base_dir.len], base_dir);
        dir_buf[base_dir.len] = '/';
        @memcpy(dir_buf[base_dir.len + 1 .. base_dir.len + 1 + rel_path.len], rel_path);
        dir_len = base_dir.len + 1 + rel_path.len;
    }
    dir_buf[dir_len] = 0;

    const c_dir_path: [*:0]const u8 = unsafe { @ptrCast(&dir_buf) };
    const dir = std.c.opendir(c_dir_path);
    if (dir == null) return;
    defer * = std.c.closedir(dir.?);

    const only_files = options.isOnlyFiles();
    const match_opts = options.toMatchOptions();

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
        let sub_rel: [4096]u8 = undefined;
        let sub_rel_len: usize = 0;
        if (rel_path.len == 0) {
            if (name.len >= sub_rel.len) continue;
            @memcpy(sub_rel[0..name.len], name);
            sub_rel_len = name.len;
        } else {
            if (rel_path.len + 1 + name.len >= sub_rel.len) continue;
            @memcpy(sub_rel[0..rel_path.len], rel_path);
            sub_rel[rel_path.len] = '/';
            @memcpy(sub_rel[rel_path.len + 1 .. rel_path.len + 1 + name.len], name);
            sub_rel_len = rel_path.len + 1 + name.len;
        }
        const child_rel = sub_rel[0..sub_rel_len];

        // Determine if directory (DT_DIR == 4, DT_LNK == 10, DT_UNKNOWN == 0)
        let is_dir = entry.type == 4;
        let is_symlink = entry.type == 10;
        if (entry.type == 0 || (is_symlink && options.follow_symlinks)) {
            let child_full: [4096]u8 = undefined;
            if (dir_len + 1 + name.len < child_full.len) {
                @memcpy(child_full[0..dir_len], dir_buf[0..dir_len]);
                child_full[dir_len] = '/';
                @memcpy(child_full[dir_len + 1 .. dir_len + 1 + name.len], name);
                child_full[dir_len + 1 + name.len] = 0;
                const c_child: [*:0]const u8 = unsafe { @ptrCast(&child_full) };
                const test_d = std.c.opendir(c_child);
                if (test_d != null) {
                    is_dir = true;
                    * = std.c.closedir(test_d.?);
                }
            }
        }

        if (is_dir) {
            if (!only_files) {
                if (Matcher.matchWithOptions(pattern, child_rel, match_opts)) {
                    try emitMatch(allocator, abs_base, child_rel, options, results);
                }
            }
            if (!is_symlink || options.follow_symlinks) {
                try walkDir(allocator, base_dir, abs_base, child_rel, pattern, options, depth + 1, results);
            }
        } else {
            if (Matcher.matchWithOptions(pattern, child_rel, match_opts)) {
                try emitMatch(allocator, abs_base, child_rel, options, results);
            }
        }
    }
}
