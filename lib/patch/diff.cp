// Source: Custom implementation by Antigravity AI
// lib/patch/diff.cp
// Unified Diff Hunk Parser, Patch Applicator & Line-Level Diff Statistics
// Absorbed from Bun src/patch/patch.cp

const std = @import("std");

pub enum DiffOpKind: u8 {
    Equal = 0,
    Insert = 1,
    Delete = 2,
}

pub struct HunkRange {
    start_line: usize = 1;
    line_count: usize = 1;
}

pub struct UnifiedHunkHeader {
    old_range: HunkRange = HunkRange {};
    new_range: HunkRange = HunkRange {};

    /// Parses a unified diff hunk header line like `@@ -10,5 +12,7 @@`.
    pub static fn parse(line: []const u8): UnifiedHunkHeader? {
        const trimmed = std.mem.trim(u8, line, " \t\r\n");
        if (!std.mem.startsWith(u8, trimmed, "@@ ")) {
            return null;
        }
        const close_opt = std.mem.indexOfPos(u8, trimmed, 3, " @@");
        if (close_opt == null) {
            return null;
        }
        const body = std.mem.trim(u8, trimmed[3..close_opt.?], " \t");
        const space_opt = std.mem.indexOfScalar(u8, body, ' ');
        if (space_opt == null) {
            return null;
        }
        const old_part = body[0..space_opt.?];
        const new_part = std.mem.trimStart(u8, body[space_opt.? + 1 ..], " ");
        if (old_part.len < 2 || old_part[0] != '-' || new_part.len < 2 || new_part[0] != '+') {
            return null;
        }

        const old_r_opt = UnifiedHunkHeader.parseRangeSpec(old_part[1..]);
        const new_r_opt = UnifiedHunkHeader.parseRangeSpec(new_part[1..]);
        if (old_r_opt == null || new_r_opt == null) {
            return null;
        }

        return UnifiedHunkHeader {
            old_range: old_r_opt.?,
            new_range: new_r_opt.?,
        };
    }

    pub static fn parseRangeSpec(spec: []const u8): HunkRange? {
        if (comma_idx := std.mem.indexOfScalar(u8, spec, ',')) {
            const start_opt = UnifiedHunkHeader.parseUint(spec[0..comma_idx]);
            const count_opt = UnifiedHunkHeader.parseUint(spec[comma_idx + 1 ..]);
            if (start_opt == null || count_opt == null) {
                return null;
            }
            return HunkRange {
                start_line: start_opt.?,
                line_count: count_opt.?,
            };
        }
        if (start_only := UnifiedHunkHeader.parseUint(spec)) {
            return HunkRange {
                start_line: start_only,
                line_count: 1,
            };
        }
        return null;
    }

    pub static fn parseUint(s: []const u8): usize? {
        if (s.len == 0) {
            return null;
        }
        let acc: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return null;
            }
            acc = acc * 10 + ((c - '0') as usize);
            i += 1;
        }
        return acc;
    }
}

pub struct LineDiffStats {
    equal_lines: usize = 0;
    inserted_lines: usize = 0;
    deleted_lines: usize = 0;
    hunk_count: usize = 0;
}

pub struct UnifiedDiff {
    /// Scans a unified diff text and tallies hunk count, inserted (`+`), deleted (`-`), and context (` `) lines.
    pub static fn analyzePatch(patch_text: []const u8): LineDiffStats {
        let stats = LineDiffStats {};
        let line_iter = std.mem.splitScalar(u8, patch_text, '\n');
        let in_hunk: bool = false;

        while (raw_line := line_iter.next()) {
            const line = std.mem.trimEnd(u8, raw_line, "\r");
            if (line.len == 0) {
                continue;
            }
            if (std.mem.startsWith(u8, line, "@@ ")) {
                if (UnifiedHunkHeader.parse(line) != null) {
                    stats.hunk_count += 1;
                    in_hunk = true;
                }
                continue;
            }
            if (!in_hunk) {
                continue;
            }
            if (std.mem.startsWith(u8, line, "--- ") || std.mem.startsWith(u8, line, "+++ ")) {
                continue;
            }
            if (line[0] == '+') {
                stats.inserted_lines += 1;
            } else if (line[0] == '-') {
                stats.deleted_lines += 1;
            } else if (line[0] == ' ') {
                stats.equal_lines += 1;
            }
        }

        return stats;
    }

    /// Applies a single hunk body (` `, `-`, `+` lines) to build the patched output into `out_buf`.
    /// Returns the slice of `out_buf` written.
    pub static fn renderHunkNewSide(hunk_lines: []const u8, out_buf: []u8): []const u8 {
        let out_pos: usize = 0;
        let line_iter = std.mem.splitScalar(u8, hunk_lines, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trimEnd(u8, raw_line, "\r");
            if (line.len == 0 || std.mem.startsWith(u8, line, "@@ ") || std.mem.startsWith(u8, line, "--- ") || std.mem.startsWith(u8, line, "+++ ")) {
                continue;
            }
            if (line[0] == ' ' || line[0] == '+') {
                const payload = line[1..];
                if (out_pos + payload.len + 1 > out_buf.len) {
                    break;
                }
                let k: usize = 0;
                while (k < payload.len) {
                    out_buf[out_pos + k] = payload[k];
                    k += 1;
                }
                out_pos += payload.len;
                out_buf[out_pos] = '\n';
                out_pos += 1;
            }
        }

        return out_buf[0..out_pos];
    }
}
