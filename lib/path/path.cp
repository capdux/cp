// Source: /data/dev/bun/src/paths/Path.cp
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

    pub fn longestCommonPath(paths: []const []const u8): []const u8 {
        if (@import("builtin").os.tag == .windows) {
            return WindowsPath.longestCommonPath(paths);
        } else {
            return PosixPath.longestCommonPath(paths);
        }
    }

    pub fn parse(filepath: []const u8): ParsedPath {
        let b = basename(filepath, null);
        let d = dirname(filepath);
        let e = extname(filepath);
        let n = b;
        if (e.len > 0 and b.len > e.len) {
            n = b[0 .. b.len - e.len];
        }
        let r = "";
        if (@import("builtin").os.tag == .windows) {
            r = WindowsPath.volumeName(filepath);
        } else if (isAbsolute(filepath)) {
            r = "/";
        }
        return ParsedPath{
            root: r,
            dir: d,
            base: b,
            ext: e,
            name: n,
        };
    }

    pub fn format(p: ParsedPath, buf: []u8): []const u8 {
        let out_len: usize = 0;
        let b_str = p.base;
        if (b_str.len == 0 and (p.name.len > 0 or p.ext.len > 0)) {
            // Reconstruct base from name + ext
            let copy_name = if (out_len + p.name.len <= buf.len) p.name.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_name], p.name[0 .. copy_name]);
            out_len += copy_name;

            let copy_ext = if (out_len + p.ext.len <= buf.len) p.ext.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_ext], p.ext[0 .. copy_ext]);
            out_len += copy_ext;
            return buf[0 .. out_len];
        }

        if (p.dir.len > 0) {
            let copy_dir = if (out_len + p.dir.len <= buf.len) p.dir.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_dir], p.dir[0 .. copy_dir]);
            out_len += copy_dir;

            if (out_len > 0 and buf[out_len - 1] != sep and out_len < buf.len) {
                buf[out_len] = sep;
                out_len += 1;
            }
        }

        let copy_base = if (out_len + b_str.len <= buf.len) b_str.len else (buf.len - out_len);
        @memcpy(buf[out_len .. out_len + copy_base], b_str[0 .. copy_base]);
        out_len += copy_base;

        return buf[0 .. out_len];
    }
};
