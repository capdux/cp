// Source: Absorbed from /data/dev/bun/src/paths/Path.cp | Synced: 2026-11-22T07:42:24Z
// ============================================================================
// lib/path/posix.cp — POSIX Path Implementation (cap:path)
// ============================================================================

const std = @import("std");
import {
    isAbsolutePosix,
    normalizePosix,
    joinPosix,
    resolvePosix,
    relativePosix,
    toNamespacedPathPosix,
    longestCommonPathPosix,
} from "./resolve";
import { ParsedPath, formatParsedPath } from "./path";

pub const PosixPath = struct {
    pub const sep: u8 = '/';
    pub const sep_str: []const u8 = "/";
    pub const delimiter: u8 = ':';

    pub fn isAbsolute(filepath: []const u8): bool {
        return isAbsolutePosix(filepath);
    }

    pub fn basename(filepath: []const u8, ext: ([]const u8)?): []const u8 {
        let p = filepath;
        while (p.len > 1 and p[p.len - 1] == '/') {
            p = p[0 .. p.len - 1];
        }
        if (p.len == 0) return "";
        if (p.len == 1 and p[0] == '/') return "/";

        let last_slash: usize? = null;
        let i: usize = p.len;
        while (i > 0) {
            i -= 1;
            if (p[i] == '/') {
                last_slash = i;
                break;
            }
        }

        let base = p;
        if (idx := last_slash) {
            base = p[idx + 1 ..];
        }
        if (e := ext) {
            if (e.len > 0 and std.mem.endsWith(u8, base, e) and base.len > e.len) {
                base = base[0 .. base.len - e.len];
            }
        }
        return base;
    }

    pub fn dirname(filepath: []const u8): []const u8 {
        let p = filepath;
        while (p.len > 1 and p[p.len - 1] == '/') {
            p = p[0 .. p.len - 1];
        }
        if (p.len == 0 or std.mem.eql(u8, p, ".")) return ".";
        if (p.len == 1 and p[0] == '/') return "/";

        let last_slash: usize? = null;
        let i: usize = p.len;
        while (i > 0) {
            i -= 1;
            if (p[i] == '/') {
                last_slash = i;
                break;
            }
        }

        if (idx := last_slash) {
            if (idx == 0) return "/";
            let end_idx = idx;
            while (end_idx > 1 and p[end_idx - 1] == '/') {
                end_idx -= 1;
            }
            return p[0 .. end_idx];
        }
        return ".";
    }

    pub fn extname(filepath: []const u8): []const u8 {
        const base = basename(filepath, null);
        if (base.len <= 1) return "";
        if (std.mem.eql(u8, base, "..")) return "";

        let i: usize = base.len;
        while (i > 0) {
            i -= 1;
            if (base[i] == '.') {
                if (i == 0) return "";
                return base[i ..];
            }
        }
        return "";
    }

    pub fn normalize(filepath: []const u8, buf: []u8): []const u8 {
        return normalizePosix(filepath, buf);
    }

    pub fn join(paths: []const []const u8, buf: []u8): []const u8 {
        return joinPosix(paths, buf);
    }

    pub fn resolve(paths: []const []const u8, buf: []u8): []const u8 {
        return resolvePosix(paths, buf);
    }

    pub fn relative(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
        return relativePosix(from_path, to_path, buf);
    }

    pub fn toNamespacedPath(filepath: []const u8, buf: []u8): []const u8 {
        return toNamespacedPathPosix(filepath, buf);
    }

    pub fn longestCommonPath(paths: []const []const u8): []const u8 {
        return longestCommonPathPosix(paths);
    }

    pub fn parse(filepath: []const u8): ParsedPath {
        if (filepath.len == 0) return ParsedPath{};
        let r: []const u8 = if (isAbsolute(filepath)) "/" else "";
        let b = basename(filepath, null);
        if (std.mem.eql(u8, b, "/")) b = "";
        let d = dirname(filepath);
        if (b.len == 0 and d.len == 1 and d[0] == '/') {
            return ParsedPath{ root: "/", dir: "/", base: "", ext: "", name: "" };
        }
        if (std.mem.eql(u8, d, ".") and !std.mem.startsWith(u8, filepath, "./") and !std.mem.eql(u8, filepath, ".")) {
            d = "";
        }
        let e = extname(filepath);
        let n = b;
        if (e.len > 0 and b.len >= e.len) {
            n = b[0 .. b.len - e.len];
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
        return formatParsedPath(p, sep, buf);
    }
};
