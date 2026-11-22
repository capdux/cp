// Source: Absorbed from /data/dev/bun/src/paths/Path.cp | Synced: 2026-11-22T07:42:24Z
// ============================================================================
// lib/path/path.cp — Unified Cross-Platform Path Object (cap:path)
// ============================================================================

const std = @import("std");

import { PosixPath } from "./posix";
import { WindowsPath } from "./windows";

pub const ParsedPath = struct {
    root: []const u8 = "",
    dir: []const u8 = "",
    base: []const u8 = "",
    ext: []const u8 = "",
    name: []const u8 = "",
};

pub fn formatParsedPath(p: ParsedPath, sep_char: u8, buf: []u8): []const u8 {
    if (buf.len == 0) return "";
    let out_len: usize = 0;

    let prefix = if (p.dir.len > 0) p.dir else p.root;
    if (prefix.len > 0) {
        let copy_prefix = if (out_len + prefix.len <= buf.len) prefix.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_prefix], prefix[0 .. copy_prefix]);
        out_len += copy_prefix;

        let needs_sep = (p.dir.len > 0) and
            !std.mem.eql(u8, p.dir, p.root) and
            (buf[out_len - 1] != sep_char) and
            (sep_char != '\\' or buf[out_len - 1] != '/');
        if (needs_sep and out_len < buf.len) {
            buf[out_len] = sep_char;
            out_len += 1;
        }
    }

    if (p.base.len > 0) {
        let copy_base = if (out_len + p.base.len <= buf.len) p.base.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_base], p.base[0 .. copy_base]);
        out_len += copy_base;
    } else {
        if (p.name.len > 0) {
            let copy_name = if (out_len + p.name.len <= buf.len) p.name.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_name], p.name[0 .. copy_name]);
            out_len += copy_name;
        }
        if (p.ext.len > 0) {
            let copy_ext = if (out_len + p.ext.len <= buf.len) p.ext.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_ext], p.ext[0 .. copy_ext]);
            out_len += copy_ext;
        }
    }

    return buf[0 .. out_len];
}

pub const Path = struct {
    pub const posix = PosixPath;
    pub const win32 = WindowsPath;

    pub const sep: u8 = if (@import("builtin").os.tag == .windows) WindowsPath.sep else PosixPath.sep;
    pub const sep_str: []const u8 = if (@import("builtin").os.tag == .windows) WindowsPath.sep_str else PosixPath.sep_str;
    pub const delimiter: u8 = if (@import("builtin").os.tag == .windows) WindowsPath.delimiter else PosixPath.delimiter;

    pub fn isAbsolute(filepath: []const u8): bool {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.isAbsolute(filepath);
        } else {
            return PosixPath.isAbsolute(filepath);
        }
    }

    pub fn basename(filepath: []const u8, ext: ([]const u8)?): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.basename(filepath, ext);
        } else {
            return PosixPath.basename(filepath, ext);
        }
    }

    pub fn dirname(filepath: []const u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.dirname(filepath);
        } else {
            return PosixPath.dirname(filepath);
        }
    }

    pub fn extname(filepath: []const u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.extname(filepath);
        } else {
            return PosixPath.extname(filepath);
        }
    }

    pub fn normalize(filepath: []const u8, buf: []u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.normalize(filepath, buf);
        } else {
            return PosixPath.normalize(filepath, buf);
        }
    }

    pub fn join(paths: []const []const u8, buf: []u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.join(paths, buf);
        } else {
            return PosixPath.join(paths, buf);
        }
    }

    pub fn resolve(paths: []const []const u8, buf: []u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.resolve(paths, buf);
        } else {
            return PosixPath.resolve(paths, buf);
        }
    }

    pub fn relative(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.relative(from_path, to_path, buf);
        } else {
            return PosixPath.relative(from_path, to_path, buf);
        }
    }

    pub fn toNamespacedPath(filepath: []const u8, buf: []u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.toNamespacedPath(filepath, buf);
        } else {
            return PosixPath.toNamespacedPath(filepath, buf);
        }
    }

    pub fn longestCommonPath(paths: []const []const u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.longestCommonPath(paths);
        } else {
            return PosixPath.longestCommonPath(paths);
        }
    }

    pub fn parse(filepath: []const u8): ParsedPath {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.parse(filepath);
        } else {
            return PosixPath.parse(filepath);
        }
    }

    pub fn format(p: ParsedPath, buf: []u8): []const u8 {
        return formatParsedPath(p, sep, buf);
    }
};
