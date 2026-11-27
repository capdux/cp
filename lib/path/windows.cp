// Source: Absorbed from /data/dev/bun/src/paths/Path.cp | Synced: 2026-11-22T08:02:36Z
// ============================================================================
// lib/path/windows.cp — Windows (Win32) Path Implementation (cap:path)
// ============================================================================

const std = @import("std");
import {
    isSeparatorWindows,
    checkIsDriveLetter,
    windowsVolumeName,
    isAbsoluteWindows,
    normalizeWindows,
    joinWindows,
    resolveWindows,
    relativeWindows,
    toNamespacedPathWindows,
    longestCommonPathWindows,
} from "./resolve";
import { ParsedPath, formatParsedPath } from "./path";

pub const WindowsPath = struct {
    pub const sep: u8 = '\\';
    pub const sep_str: []const u8 = "\\";
    pub const delimiter: u8 = ';';

    pub fn isSeparator(c: u8): bool {
        return isSeparatorWindows(c);
    }

    pub fn isDriveLetter(c: u8): bool {
        return checkIsDriveLetter(c);
    }

    pub fn volumeName(filepath: []const u8): []const u8 {
        return windowsVolumeName(filepath);
    }

    pub fn isAbsolute(filepath: []const u8): bool {
        return isAbsoluteWindows(filepath);
    }

    pub static fn basename(filepath: []const u8, ext: ([]const u8)?): []const u8 {
        let p = filepath;
        let vol = volumeName(p);
        while (p.len > vol.len + 1 and isSeparator(p[p.len - 1])) {
            p = p[0 .. p.len - 1];
        }
        if (p.len == 0 or p.len == vol.len) return "";
        if (p.len == vol.len + 1 and isSeparator(p[vol.len])) return "";

        let last_slash: usize? = null;
        let i: usize = p.len;
        while (i > vol.len) {
            i -= 1;
            if (isSeparator(p[i])) {
                last_slash = i;
                break;
            }
        }

        let base = p[vol.len ..];
        if (idx := last_slash) {
            base = p[idx + 1 ..];
        }
        if (e := ext) {
            if (e.len > 0 and std.ascii.endsWithIgnoreCase(base, e) and base.len > e.len) {
                base = base[0 .. base.len - e.len];
            }
        }
        return base;
    }

    pub static fn dirname(filepath: []const u8): []const u8 {
        let p = filepath;
        let vol = volumeName(p);
        while (p.len > vol.len + 1 and isSeparator(p[p.len - 1])) {
            p = p[0 .. p.len - 1];
        }
        if (p.len == 0) return ".";

        if (p.len == vol.len) return p;
        if (p.len == vol.len + 1 and isSeparator(p[vol.len])) return p;

        let last_slash: usize? = null;
        let i: usize = p.len;
        while (i > vol.len) {
            i -= 1;
            if (isSeparator(p[i])) {
                last_slash = i;
                break;
            }
        }

        if (idx := last_slash) {
            if (idx == vol.len) {
                return p[0 .. vol.len + 1];
            }
            let end_idx = idx;
            while (end_idx > vol.len + 1 and isSeparator(p[end_idx - 1])) {
                end_idx -= 1;
            }
            return p[0 .. end_idx];
        }

        if (vol.len > 0) return vol;
        return ".";
    }

    pub static fn extname(filepath: []const u8): []const u8 {
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
        return normalizeWindows(filepath, buf);
    }

    pub fn join(paths: []const []const u8, buf: []u8): []const u8 {
        return joinWindows(paths, buf);
    }

    pub fn resolve(paths: []const []const u8, buf: []u8): []const u8 {
        return resolveWindows(paths, buf);
    }

    pub fn relative(from_path: []const u8, to_path: []const u8, buf: []u8): []const u8 {
        return relativeWindows(from_path, to_path, buf);
    }

    pub fn toNamespacedPath(filepath: []const u8, buf: []u8): []const u8 {
        return toNamespacedPathWindows(filepath, buf);
    }

    pub fn longestCommonPath(paths: []const []const u8): []const u8 {
        return longestCommonPathWindows(paths);
    }

    pub static fn parse(filepath: []const u8): ParsedPath {
        if (filepath.len == 0) return ParsedPath{};
        let vol = volumeName(filepath);
        let r: []const u8 = "";
        if (vol.len > 0) {
            if (filepath.len > vol.len and isSeparator(filepath[vol.len])) {
                r = filepath[0 .. vol.len + 1];
            } else {
                r = vol;
            }
        } else if (isSeparator(filepath[0])) {
            r = filepath[0 .. 1];
        }

        let b = basename(filepath, null);
        let d = dirname(filepath);
        if (std.mem.eql(u8, d, ".") and vol.len == 0 and !std.mem.startsWith(u8, filepath, ".\\") and !std.mem.startsWith(u8, filepath, "./") and !std.mem.eql(u8, filepath, ".")) {
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
