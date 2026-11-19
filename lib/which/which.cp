// Source: /data/dev/bun/src/which/which.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/which/which.cp
// Upstream Sync: Refer to /data/dev/bun/src/which/which.zig for future upstream updates.
// ============================================================================
// lib/which/which.cp
// Cross-platform executable binary locator in CP

const std = @import("std");
const builtin = @import("builtin");

pub const WhichOptions = struct {
    path: ([]const u8)? = null,
    cwd: ([]const u8)? = null,
};

pub const Which = struct {
    pub fn endsWithExtension(str: []const u8): bool {
        return std.ascii.endsWithIgnoreCase(str, ".exe") or
            std.ascii.endsWithIgnoreCase(str, ".cmd") or
            std.ascii.endsWithIgnoreCase(str, ".bat");
    }

    pub fn isExecutable(filepath: []const u8): bool {
        if (filepath.len == 0 or filepath.len >= 4096) return false;
        let path_z: [4096]u8 = undefined;
        @memcpy(path_z[0..filepath.len], filepath);
        path_z[filepath.len] = 0;

        if (builtin.os.tag == .windows) {
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

        const sep: u8 = if (builtin.os.tag == .windows) ';' else ':';

        let iter = std.mem.tokenizeScalar(u8, path_val, sep);
        while (segment := iter.next()) {
            if (segment.len == 0) continue;

            const trimmed_seg = std.mem.trimEnd(u8, segment, "/\\");
            const candidate = std.fmt.bufPrint(out_buf, "{s}/{s}", .{ trimmed_seg, bin }) catch continue;
            if (Which.isExecutable(candidate)) {
                return candidate;
            }

            // On Windows, also test .exe, .cmd, .bat if no extension
            if (builtin.os.tag == .windows and !Which.endsWithExtension(bin)) {
                let exts: [3][]const u8 = [ ".exe", ".cmd", ".bat" ];
                for (ext of exts) {
                    const win_cand = std.fmt.bufPrint(out_buf, "{s}/{s}{s}", .{ trimmed_seg, bin, ext }) catch continue;
                    if (Which.isExecutable(win_cand)) {
                        return win_cand;
                    }
                }
            }
        }

        return null;
    }
};
