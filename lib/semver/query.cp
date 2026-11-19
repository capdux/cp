// Source: /data/dev/bun/src/semver/
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
};

pub const ComparatorSet = struct {
    comparators: [8]Comparator = [Comparator{}; 8],
    count: usize = 0,

    pub fn add(*_, c: Comparator): void {
        if (_.count < 8) {
            _.comparators[_.count] = c;
            _.count += 1;
        }
    }

    pub fn matches(self: ComparatorSet, v: Version): bool {
        if (self.count == 0) return true;
        let i: usize = 0;
        while (i < self.count) {
            if (!self.comparators[i].matches(v)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    pub fn satisfies(self: ComparatorSet, v: Version): bool {
        return self.matches(v);
    }
};

pub const Query = struct {
    pub fn parseClause(clause: []const u8): !ComparatorSet {
        let trimmed = std.mem.trim(u8, clause, " \t\r\n");
        let set = ComparatorSet{};

        if (trimmed.len == 0 or std.mem.eql(u8, trimmed, "*")) {
            set.add(Comparator{ op: Op.any });
            return set;
        }

        // 1. Hyphen range: "1.2.3 - 2.3.4"
        let hyphen_pos: usize? = null;
        let i: usize = 0;
        while (i + 2 < trimmed.len) {
            if (trimmed[i] == ' ' and trimmed[i + 1] == '-' and trimmed[i + 2] == ' ') {
                hyphen_pos = i;
                break;
            }
            i += 1;
        }

        if (h_idx := hyphen_pos) {
            let left_str = std.mem.trim(u8, trimmed[0 .. h_idx], " ");
            let right_str = std.mem.trim(u8, trimmed[h_idx + 3 ..], " ");
            let p_left = Version.parsePartial(left_str) catch return error.InvalidRange;
            let p_right = Version.parsePartial(right_str) catch return error.InvalidRange;

            // Lower bound: >= left
            let min_major: u32 = 0;
            if (m := p_left.major) { min_major = m; }
            let min_minor: u32 = 0;
            if (m := p_left.minor) { min_minor = m; }
            let min_patch: u32 = 0;
            if (p := p_left.patch) { min_patch = p; }

            let v_min = Version{
                major: min_major,
                minor: min_minor,
                patch: min_patch,
                prerelease: p_left.prerelease,
            };
            set.add(Comparator{ op: Op.gte, version: v_min });

            // Upper bound: if right is partial (missing patch or minor), use < next
            if (p_right.minor == null) {
                let max_maj: u32 = 1;
                if (m := p_right.major) { max_maj = m + 1; }
                let v_max = Version{
                    major: max_maj,
                    minor: 0,
                    patch: 0,
                };
                set.add(Comparator{ op: Op.lt, version: v_max });
            } else if (p_right.patch == null) {
                let max_maj: u32 = 0;
                if (m := p_right.major) { max_maj = m; }
                let max_min: u32 = 1;
                if (m := p_right.minor) { max_min = m + 1; }
                let v_max = Version{
                    major: max_maj,
                    minor: max_min,
                    patch: 0,
                };
                set.add(Comparator{ op: Op.lt, version: v_max });
            } else {
                let max_maj: u32 = 0;
                if (m := p_right.major) { max_maj = m; }
                let max_min: u32 = 0;
                if (m := p_right.minor) { max_min = m; }
                let max_pat: u32 = 0;
                if (p := p_right.patch) { max_pat = p; }
                let v_max = Version{
                    major: max_maj,
                    minor: max_min,
                    patch: max_pat,
                    prerelease: p_right.prerelease,
                };
                set.add(Comparator{ op: Op.lte, version: v_max });
            }
            return set;
        }

        // 2. Space-separated comparators
        let cursor: usize = 0;
        while (cursor < trimmed.len) {
            while (cursor < trimmed.len and (trimmed[cursor] == ' ' or trimmed[cursor] == '\t')) {
                cursor += 1;
            }
            if (cursor >= trimmed.len) break;
            let start = cursor;
            while (cursor < trimmed.len and trimmed[cursor] != ' ' and trimmed[cursor] != '\t') {
                cursor += 1;
            }
            let token = trimmed[start .. cursor];
            if (isStandaloneOperator(token)) {
                while (cursor < trimmed.len and (trimmed[cursor] == ' ' or trimmed[cursor] == '\t')) {
                    cursor += 1;
                }
                let v_start = cursor;
                while (cursor < trimmed.len and trimmed[cursor] != ' ' and trimmed[cursor] != '\t') {
                    cursor += 1;
                }
                let v_tok = trimmed[v_start .. cursor];
                let comb_buf: [128]u8 = undefined;
                if (v_tok.len > 0 and token.len + v_tok.len <= 128) {
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
        return std.mem.eql(u8, token, ">=") or
               std.mem.eql(u8, token, "<=") or
               std.mem.eql(u8, token, ">") or
               std.mem.eql(u8, token, "<") or
               std.mem.eql(u8, token, "=") or
               std.mem.eql(u8, token, "^") or
               std.mem.eql(u8, token, "~");
    }

    fn parseTokenIntoSet(token: []const u8, set: *ComparatorSet): !void {
        if (token.len == 0 or std.mem.eql(u8, token, "*")) {
            set.add(Comparator{ op: Op.any });
            return;
        }

        // Caret range: ^
        if (token[0] == '^') {
            let rest = token[1..];
            let pv = try Version.parsePartial(rest);

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

        // Tilde range: ~
        if (token[0] == '~') {
            let rest = token[1..];
            let pv = try Version.parsePartial(rest);

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
                let pat: u32 = 0;
                if (p := pv.patch) { pat = p; }
                set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: pat, prerelease: pv.prerelease } });
                set.add(Comparator{ op: Op.lt, version: Version{ major: maj, minor: min + 1, patch: 0 } });
            }
            return;
        }

        // >=
        if (std.mem.startsWith(u8, token, ">=")) {
            let v = try Version.parse(token[2..]);
            set.add(Comparator{ op: Op.gte, version: v });
            return;
        }

        // <=
        if (std.mem.startsWith(u8, token, "<=")) {
            let v = try Version.parse(token[2..]);
            set.add(Comparator{ op: Op.lte, version: v });
            return;
        }

        // >
        if (token[0] == '>') {
            let v = try Version.parse(token[1..]);
            set.add(Comparator{ op: Op.gt, version: v });
            return;
        }

        // <
        if (token[0] == '<') {
            let v = try Version.parse(token[1..]);
            set.add(Comparator{ op: Op.lt, version: v });
            return;
        }

        // =
        if (token[0] == '=') {
            let v = try Version.parse(token[1..]);
            set.add(Comparator{ op: Op.eq, version: v });
            return;
        }

        // Wildcard or x-range: 1.x, 1.2.*, 1
        let pv = Version.parsePartial(token) catch {
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
