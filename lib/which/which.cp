// Source: Absorbed from /data/dev/bun/src/which/which.cp | Synced: 2026-11-22T12:57:27Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/which/which.cp
// Upstream Sync: Refer to /data/dev/bun/src/which/which.zig for future upstream updates.
// ============================================================================
// lib/which/which.cp
// Cross-platform executable binary locator in CP

const std = @import("std");

pub const WhichOptions = struct {
    path: ([]const u8)? = null,
    cwd: ([]const u8)? = null,
    pathext: ([]const u8)? = null,
};

pub const Which = struct {
    pub fn endsWithExtension(str: []const u8): bool {
        return std.ascii.endsWithIgnoreCase(str, ".exe") or
            std.ascii.endsWithIgnoreCase(str, ".cmd") or
            std.ascii.endsWithIgnoreCase(str, ".bat");
    }

    pub fn endsWithPathext(str: []const u8, pathext: []const u8): bool {
        let iter = std.mem.tokenizeScalar(u8, pathext, ';');
        while (ext := iter.next()) {
            const clean = std.mem.trim(u8, ext, " \t");
            if (clean.len > 0 and std.ascii.endsWithIgnoreCase(str, clean)) {
                return true;
            }
        }
        return false;
    }

    pub fn isExecutable(filepath: []const u8): bool {
        if (filepath.len == 0 or filepath.len >= 4096) return false;
        let path_z: [4096]u8 = undefined;
        @memcpy(path_z[0..filepath.len], filepath);
        path_z[filepath.len] = 0;

        if (@import("builtin").os.tag == .windows) {
            if (Which.endsWithExtension(filepath)) {
                std.fs.accessAbsolute(filepath) catch return false;
                return true;
            }
            return false;
        } else {
            const c_path: [*:0]const u8 = @ptrCast(&path_z);
            if (std.c.access(c_path, 1) != 0) return false;
            // Ensure it is a regular executable file and not a directory
            const dir = std.c.opendir(c_path);
            if (dir != null) {
                * = std.c.closedir(dir.?);
                return false;
            }
            return true;
        }
    }

    pub fn which(bin: []const u8, options: WhichOptions, out_buf: []u8): ([]const u8)? {
        if (bin.len == 0 or bin.len >= out_buf.len) return null;

        // 1. Absolute path
        if (std.fs.path.isAbsolute(bin)) {
            if (Which.isExecutable(bin)) {
                @memcpy(out_buf[0..bin.len], bin);
                return out_buf[0..bin.len];
            }
            return null;
        }

        // 2. Relative path with separator (e.g. ./app or bin/tool)
        if (std.mem.indexOfScalar(u8, bin, '/') != null or std.mem.indexOfScalar(u8, bin, '\\') != null) {
            if (cwd := options.cwd) {
                if (cwd.len == 0) return null;
                let clean_bin = bin;
                if (std.mem.startsWith(u8, clean_bin, "./") or std.mem.startsWith(u8, clean_bin, ".\\")) {
                    clean_bin = clean_bin[2..];
                }
                const trimmed_cwd = std.mem.trimEnd(u8, cwd, "/\\");
                const combined = std.fmt.bufPrint(out_buf, "{s}/{s}", .{ trimmed_cwd, clean_bin }) catch return null;
                if (Which.isExecutable(combined)) {
                    return combined;
                }
            } else {
                if (Which.isExecutable(bin)) {
                    @memcpy(out_buf[0..bin.len], bin);
                    return out_buf[0..bin.len];
                }
            }
            return null;
        }

        // 3. Search in PATH
        let path_val: []const u8 = "";
        if (opt_p := options.path) {
            path_val = opt_p;
        } else {
            const raw_env = std.c.getenv("PATH");
            if (raw_env == null) return null;
            path_val = std.mem.span(raw_env.?);
        }

        const sep: u8 = if (@import("builtin").os.tag == .windows) ';' else ':';

        let iter = std.mem.tokenizeScalar(u8, path_val, sep);
        while (segment := iter.next()) {
            if (segment.len == 0) continue;

            const trimmed_seg = std.mem.trimEnd(u8, segment, "/\\");
            const candidate = std.fmt.bufPrint(out_buf, "{s}/{s}", .{ trimmed_seg, bin }) catch continue;
            if (Which.isExecutable(candidate)) {
                return candidate;
            }

            // Probe PATHEXT extensions when provided explicitly or on Windows
            let effective_pathext: ([]const u8)? = options.pathext;
            if (effective_pathext == null and @import("builtin").os.tag == .windows) {
                const raw_pe = std.c.getenv("PATHEXT");
                if (raw_pe != null) {
                    effective_pathext = std.mem.span(raw_pe.?);
                } else {
                    effective_pathext = ".COM;.EXE;.BAT;.CMD";
                }
            }
            if (pe := effective_pathext) {
                if (!Which.endsWithPathext(bin, pe)) {
                    let ext_iter = std.mem.tokenizeScalar(u8, pe, ';');
                    while (ext := ext_iter.next()) {
                        const clean_ext = std.mem.trim(u8, ext, " \t");
                        if (clean_ext.len == 0) continue;
                        const ext_cand = std.fmt.bufPrint(out_buf, "{s}/{s}{s}", .{ trimmed_seg, bin, clean_ext }) catch continue;
                        if (Which.isExecutable(ext_cand)) {
                            return ext_cand;
                        }
                    }
                }
            }
        }

        return null;
    }

    /// Finds all executable matches for `bin` across PATH segments, packing slices into `out_buf` and `out_paths`.
    pub fn whichAll(bin: []const u8, options: WhichOptions, out_buf: []u8, out_paths: [][]const u8): usize {
        if (bin.len == 0 or out_paths.len == 0) return 0;

        if (std.fs.path.isAbsolute(bin) or std.mem.indexOfScalar(u8, bin, '/') != null or std.mem.indexOfScalar(u8, bin, '\\') != null) {
            if (found := Which.which(bin, options, out_buf)) {
                out_paths[0] = found;
                return 1;
            }
            return 0;
        }

        let path_val: []const u8 = "";
        if (opt_p := options.path) {
            path_val = opt_p;
        } else {
            const raw_env = std.c.getenv("PATH");
            if (raw_env == null) return 0;
            path_val = std.mem.span(raw_env.?);
        }

        const sep: u8 = if (@import("builtin").os.tag == .windows) ';' else ':';
        let iter = std.mem.tokenizeScalar(u8, path_val, sep);
        let count: usize = 0;
        let used: usize = 0;

        while (segment := iter.next()) {
            if (count >= out_paths.len or used >= out_buf.len) break;
            if (segment.len == 0) continue;
            const trimmed_seg = std.mem.trimEnd(u8, segment, "/\\");
            const candidate = std.fmt.bufPrint(out_buf[used..], "{s}/{s}", .{ trimmed_seg, bin }) catch break;
            if (Which.isExecutable(candidate)) {
                out_paths[count] = candidate;
                count += 1;
                used += candidate.len;
            }
        }
        return count;
    }
};
