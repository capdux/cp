// Source: Absorbed from /data/dev/bun/src/semver/SemverQuery.cp | Synced: 2026-11-22T02:34:23Z
// ============================================================================
// lib/semver/query.cp — Semver Query & Range Comparator Engine
// ============================================================================

const std = @import("std");
import { Version, PartialVersion } from "./version";

pub const Op = enum {
    any,
    eq,
    lt,
    lte,
    gt,
    gte,
};

pub const Comparator = struct {
    op: Op = Op.any,
    version: Version = Version{},

    pub fn matches(self: Comparator, v: Version): bool {
        switch (self.op) {
            Op.any => return true,
            Op.eq => return v.eql(self.version),
            Op.lt => return v.lt(self.version),
            Op.lte => return v.lte(self.version),
            Op.gt => return v.gt(self.version),
            Op.gte => return v.gte(self.version),
        }
    }

    pub fn satisfies(self: Comparator, v: Version): bool {
        return self.matches(v);
    }

    pub fn sameTuple(self: Comparator, v: Version): bool {
        return self.version.major == v.major &&
               self.version.minor == v.minor &&
               self.version.patch == v.patch;
    }

    pub fn format(self: Comparator, buf: []u8): []const u8 {
        if (self.op == Op.any) {
            if (buf.len > 0) {
                buf[0] = '*';
                return buf[0..1];
            }
            return "";
        }
        let op_str: []const u8 = switch (self.op) {
            Op.any => "*",
            Op.eq => "=",
            Op.lt => "<",
            Op.lte => "<=",
            Op.gt => ">",
            Op.gte => ">=",
        };
        let v_buf: [128]u8 = undefined;
        const v_str = self.version.format(&v_buf);
        return std.fmt.bufPrint(buf, "{s}{s}", .{ op_str, v_str }) catch "";
    }
};

pub const ComparatorSet = struct {
    comparators: [32]Comparator = [Comparator{}; 32],
    count: usize = 0,

    pub fn add(*_, c: Comparator) {
        if (_.count < 32) {
            _.comparators[_.count] = c;
            _.count += 1;
        }
    }

    pub fn matches(self: ComparatorSet, v: Version): bool {
        return self.matchesWithOptions(v, false);
    }

    pub fn matchesWithOptions(self: ComparatorSet, v: Version, include_prerelease: bool): bool {
        if (self.count == 0) {
            return include_prerelease || v.prerelease.len == 0;
        }

        let i: usize = 0;
        while (i < self.count) {
            if (!self.comparators[i].matches(v)) {
                return false;
            }
            i += 1;
        }

        // Semver 2.0 / Bun / npm prerelease matching rule:
        // If target version has a prerelease tag, at least one comparator in the set
        // must also have a prerelease tag with the exact same [major, minor, patch] tuple.
        if (v.prerelease.len > 0 && !include_prerelease) {
            let allowed = false;
            let k: usize = 0;
            while (k < self.count) {
                const c = self.comparators[k];
                if (c.op != Op.any && c.version.prerelease.len > 0 && c.sameTuple(v)) {
                    allowed = true;
                    break;
                }
                k += 1;
            }
            if (!allowed) return false;
        }

        return true;
    }

    pub fn satisfies(self: ComparatorSet, v: Version): bool {
        return self.matches(v);
    }

    pub fn format(self: ComparatorSet, buf: []u8): []const u8 {
        if (self.count == 0) {
            if (buf.len > 0) {
                buf[0] = '*';
                return buf[0..1];
            }
            return "";
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < self.count) {
            if (i > 0 && pos < buf.len) {
                buf[pos] = ' ';
                pos += 1;
            }
            let c_buf: [140]u8 = undefined;
            const c_str = self.comparators[i].format(&c_buf);
            let k: usize = 0;
            while (k < c_str.len && pos < buf.len) {
                buf[pos] = c_str[k];
                pos += 1;
                k += 1;
            }
            i += 1;
        }
        return buf[0..pos];
    }
};

pub const Query = struct {
    pub fn parseClause(clause: []const u8): !ComparatorSet {
        let trimmed = std.mem.trim(u8, clause, " \t\r\n");
        let set = ComparatorSet{};

        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "*") || std.mem.eql(u8, trimmed, "x") || std.mem.eql(u8, trimmed, "X")) {
            set.add(Comparator{ op: Op.any });
            return set;
        }

        // 1. Hyphen range: "1.2.3 - 2.3.4"
        let hyphen_pos: usize? = null;
        let i: usize = 0;
        while (i + 2 < trimmed.len) {
            const c0 = trimmed[i];
            const c1 = trimmed[i + 1];
            const c2 = trimmed[i + 2];
            if ((c0 == ' ' || c0 == '\t') && c1 == '-' && (c2 == ' ' || c2 == '\t')) {
                hyphen_pos = i;
                break;
            }
            i += 1;
        }

        if (h_idx := hyphen_pos) {
            let left_str = std.mem.trim(u8, trimmed[0 .. h_idx], " \t");
            let right_str = std.mem.trim(u8, trimmed[h_idx + 3 ..], " \t");
            if (left_str.len == 0 || right_str.len == 0) {
                return error.InvalidRange;
            }
            // Disallow operators or multiple tokens inside hyphen endpoints
            if (std.mem.indexOfAny(u8, left_str, " \t<>^~=") != null ||
                std.mem.indexOfAny(u8, right_str, " \t<>^~=") != null)
            {
                return error.InvalidRange;
            }

            let p_left = Version.parsePartial(left_str) catch return error.InvalidRange;
            let p_right = Version.parsePartial(right_str) catch return error.InvalidRange;

            // Lower bound: >= left (unless left is "*")
            if (p_left.major != null) {
                let min_major = p_left.major.?;
                let min_minor: u32 = p_left.minor orelse 0;
                let min_patch: u32 = p_left.patch orelse 0;
                let v_min = Version{
                    major: min_major,
                    minor: min_minor,
                    patch: min_patch,
                    prerelease: p_left.prerelease,
                };
                set.add(Comparator{ op: Op.gte, version: v_min });
            } else {
                set.add(Comparator{ op: Op.gte, version: Version{ major: 0, minor: 0, patch: 0 } });
            }

            // Upper bound:
            // If right is "*", no upper bound needed.
            // If right is partial (missing minor or patch), use < next.
            if (p_right.major == null) {
                // No upper bound ("1.0.0 - *")
            } else if (p_right.minor == null) {
                let max_maj = p_right.major.? + 1;
                set.add(Comparator{ op: Op.lt, version: Version{ major: max_maj, minor: 0, patch: 0 } });
            } else if (p_right.patch == null) {
                let max_maj = p_right.major.?;
                let max_min = p_right.minor.? + 1;
                set.add(Comparator{ op: Op.lt, version: Version{ major: max_maj, minor: max_min, patch: 0 } });
            } else {
                let v_max = Version{
                    major: p_right.major.?,
                    minor: p_right.minor.?,
                    patch: p_right.patch.?,
                    prerelease: p_right.prerelease,
                };
                set.add(Comparator{ op: Op.lte, version: v_max });
            }
            return set;
        }

        // 2. Space-separated comparators (logical AND intersection)
        let cursor: usize = 0;
        while (cursor < trimmed.len) {
            while (cursor < trimmed.len && (trimmed[cursor] == ' ' || trimmed[cursor] == '\t')) {
                cursor += 1;
            }
            if (cursor >= trimmed.len) break;
            let start = cursor;
            while (cursor < trimmed.len && trimmed[cursor] != ' ' && trimmed[cursor] != '\t') {
                cursor += 1;
            }
            let token = trimmed[start .. cursor];
            if (isStandaloneOperator(token)) {
                while (cursor < trimmed.len && (trimmed[cursor] == ' ' || trimmed[cursor] == '\t')) {
                    cursor += 1;
                }
                let v_start = cursor;
                while (cursor < trimmed.len && trimmed[cursor] != ' ' && trimmed[cursor] != '\t') {
                    cursor += 1;
                }
                let v_tok = trimmed[v_start .. cursor];
                let comb_buf: [128]u8 = undefined;
                if (v_tok.len > 0 && token.len + v_tok.len <= 128) {
                    @memcpy(comb_buf[0 .. token.len], token);
                    @memcpy(comb_buf[token.len .. token.len + v_tok.len], v_tok);
                    try parseTokenIntoSet(comb_buf[0 .. token.len + v_tok.len], &set);
                } else {
                    return error.InvalidRange;
                }
            } else {
                try parseTokenIntoSet(token, &set);
            }
        }

        return set;
    }

    fn isStandaloneOperator(token: []const u8): bool {
        return std.mem.eql(u8, token, ">=") ||
               std.mem.eql(u8, token, "<=") ||
               std.mem.eql(u8, token, "==") ||
               std.mem.eql(u8, token, "~>") ||
               std.mem.eql(u8, token, ">") ||
               std.mem.eql(u8, token, "<") ||
               std.mem.eql(u8, token, "=") ||
               std.mem.eql(u8, token, "^") ||
               std.mem.eql(u8, token, "~");
    }

    fn parseTokenIntoSet(token: []const u8, set: *ComparatorSet): !void {
        if (token.len == 0 || std.mem.eql(u8, token, "*") || std.mem.eql(u8, token, "x") || std.mem.eql(u8, token, "X")) {
            set.add(Comparator{ op: Op.any });
            return;
        }

        // Caret range: ^
        if (token[0] == '^') {
            let rest = token[1..];
            let pv = Version.parsePartial(rest) catch return error.InvalidRange;

            if (pv.major == null) {
                set.add(Comparator{ op: Op.any });
                return;
            }

            let maj = pv.major.?;
            if (pv.minor == null) {
                set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: 0, patch: 0 } });
                set.add(Comparator{ op: Op.lt, version: Version{ major: maj + 1, minor: 0, patch: 0 } });
            } else if (pv.patch == null) {
                let min = pv.minor.?;
                if (maj > 0) {
                    set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: 0 } });
                    set.add(Comparator{ op: Op.lt, version: Version{ major: maj + 1, minor: 0, patch: 0 } });
                } else {
                    set.add(Comparator{ op: Op.gte, version: Version{ major: 0, minor: min, patch: 0 } });
                    set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: min + 1, patch: 0 } });
                }
            } else {
                let min = pv.minor.?;
                let pat = pv.patch.?;
                set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: pat, prerelease: pv.prerelease } });
                if (maj > 0) {
                    set.add(Comparator{ op: Op.lt, version: Version{ major: maj + 1, minor: 0, patch: 0 } });
                } else if (min > 0) {
                    set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: min + 1, patch: 0 } });
                } else {
                    set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: 0, patch: pat + 1 } });
                }
            }
            return;
        }

        // Tilde range: ~ or ~>
        if (token[0] == '~') {
            let rest = if (token.len >= 2 && token[1] == '>') token[2..] else token[1..];
            let pv = Version.parsePartial(rest) catch return error.InvalidRange;

            if (pv.major == null) {
                set.add(Comparator{ op: Op.any });
                return;
            }

            let maj = pv.major.?;
            if (pv.minor == null) {
                set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: 0, patch: 0 } });
                set.add(Comparator{ op: Op.lt, version: Version{ major: maj + 1, minor: 0, patch: 0 } });
            } else {
                let min = pv.minor.?;
                let pat: u32 = pv.patch orelse 0;
                set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: pat, prerelease: pv.prerelease } });
                set.add(Comparator{ op: Op.lt, version: Version{ major: maj, minor: min + 1, patch: 0 } });
            }
            return;
        }

        // >=
        if (std.mem.startsWith(u8, token, ">=")) {
            let pv = Version.parsePartial(token[2..]) catch return error.InvalidRange;
            if (pv.major == null) {
                set.add(Comparator{ op: Op.any });
            } else {
                set.add(Comparator{ op: Op.gte, version: Version{
                    major: pv.major.?,
                    minor: pv.minor orelse 0,
                    patch: pv.patch orelse 0,
                    prerelease: pv.prerelease,
                } });
            }
            return;
        }

        // <=
        if (std.mem.startsWith(u8, token, "<=")) {
            let pv = Version.parsePartial(token[2..]) catch return error.InvalidRange;
            if (pv.major == null) {
                set.add(Comparator{ op: Op.any });
            } else if (pv.minor == null) {
                set.add(Comparator{ op: Op.lt, version: Version{ major: pv.major.? + 1, minor: 0, patch: 0 } });
            } else if (pv.patch == null) {
                set.add(Comparator{ op: Op.lt, version: Version{ major: pv.major.?, minor: pv.minor.? + 1, patch: 0 } });
            } else {
                set.add(Comparator{ op: Op.lte, version: Version{
                    major: pv.major.?,
                    minor: pv.minor.?,
                    patch: pv.patch.?,
                    prerelease: pv.prerelease,
                } });
            }
            return;
        }

        // >
        if (token[0] == '>') {
            let pv = Version.parsePartial(token[1..]) catch return error.InvalidRange;
            if (pv.major == null) {
                // >* matches nothing
                set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: 0, patch: 0 } });
            } else if (pv.minor == null) {
                set.add(Comparator{ op: Op.gte, version: Version{ major: pv.major.? + 1, minor: 0, patch: 0 } });
            } else if (pv.patch == null) {
                set.add(Comparator{ op: Op.gte, version: Version{ major: pv.major.?, minor: pv.minor.? + 1, patch: 0 } });
            } else {
                set.add(Comparator{ op: Op.gt, version: Version{
                    major: pv.major.?,
                    minor: pv.minor.?,
                    patch: pv.patch.?,
                    prerelease: pv.prerelease,
                } });
            }
            return;
        }

        // <
        if (token[0] == '<') {
            let pv = Version.parsePartial(token[1..]) catch return error.InvalidRange;
            if (pv.major == null) {
                // <* matches nothing
                set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: 0, patch: 0 } });
            } else {
                set.add(Comparator{ op: Op.lt, version: Version{
                    major: pv.major.?,
                    minor: pv.minor orelse 0,
                    patch: pv.patch orelse 0,
                    prerelease: pv.prerelease,
                } });
            }
            return;
        }

        // == or = or exact/x-range
        let rest_tok = token;
        if (std.mem.startsWith(u8, rest_tok, "==")) {
            rest_tok = rest_tok[2..];
        } else if (rest_tok[0] == '=') {
            rest_tok = rest_tok[1..];
        }

        // Wildcard or x-range: 1.x, 1.2.*, 1
        let pv = Version.parsePartial(rest_tok) catch {
            return error.InvalidRange;
        };

        if (pv.major == null) {
            set.add(Comparator{ op: Op.any });
        } else if (pv.minor == null) {
            let maj = pv.major.?;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: 0, patch: 0 } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: maj + 1, minor: 0, patch: 0 } });
        } else if (pv.patch == null) {
            let maj = pv.major.?;
            let min = pv.minor.?;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: 0 } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: maj, minor: min + 1, patch: 0 } });
        } else {
            let v = Version{
                major: pv.major.?,
                minor: pv.minor.?,
                patch: pv.patch.?,
                prerelease: pv.prerelease,
            };
            set.add(Comparator{ op: Op.eq, version: v });
        }
    }
};
