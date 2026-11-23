// Reference-hardened against dtolnay/semver (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/semver/SemverQuery.cp | Synced: 2026-11-22T07:44:11Z
// ============================================================================
// lib/semver/query.cp — Semver Query & Range Comparator Engine
// ============================================================================

import { Simd } from "cap:simd";
const std = @import("std");
import { Version, PartialVersion } from "./version";

/// Hard upper bound for a SemVer range clause to guard against adversarial inputs.
pub const SEMVER_RANGE_MAX_LEN: usize = 1024;

fn checkedAddOne(v: u64): !u64 {
    if (v == std.math.maxInt(u64)) return error.InvalidRange;
    return v + 1;
}

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

        // Semver 2.0 / dtolnay/semver / npm prerelease matching rule:
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
    pub static fn parseClause(clause: []const u8): !ComparatorSet {
        if (clause.len > SEMVER_RANGE_MAX_LEN) return error.InvalidRange;
        if (clause.len > 0 && !Simd.isAscii(clause)) return error.InvalidRange;

        let trimmed = std.mem.trim(u8, clause, " \t\r\n");
        let set = ComparatorSet{};

        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "*") || std.mem.eql(u8, trimmed, "x") || std.mem.eql(u8, trimmed, "X")) {
            set.add(Comparator{ op: Op.any });
            return set;
        }

        // 1. Hyphen range: "1.2.3 - 2.3.4"
        let hyphen_pos: usize? = null;
        let scan_i: usize = 0;
        while (scan_i + 2 < trimmed.len) {
            const rel = Simd.indexOfByte(trimmed[scan_i..], 45 as u8); // '-'
            if (off := rel) {
                const dash_idx = scan_i + off;
                if (dash_idx > 0 && dash_idx + 1 < trimmed.len) {
                    const prev_c = trimmed[dash_idx - 1];
                    const next_c = trimmed[dash_idx + 1];
                    if ((prev_c == ' ' || prev_c == '\t') && (next_c == ' ' || next_c == '\t')) {
                        hyphen_pos = dash_idx - 1;
                        break;
                    }
                }
                scan_i = dash_idx + 1;
            } else {
                break;
            }
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
                let min_minor: u64 = p_left.minor orelse 0;
                let min_patch: u64 = p_left.patch orelse 0;
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
                let max_maj = checkedAddOne(p_right.major.?) catch return error.InvalidRange;
                set.add(Comparator{ op: Op.lt, version: Version{ major: max_maj, minor: 0, patch: 0 } });
            } else if (p_right.patch == null) {
                let max_maj = p_right.major.?;
                let max_min = checkedAddOne(p_right.minor.?) catch return error.InvalidRange;
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
            if (Query.isStandaloneOperator(token)) {
                while (cursor < trimmed.len && (trimmed[cursor] == ' ' || trimmed[cursor] == '\t')) {
                    cursor += 1;
                }
                let v_start = cursor;
                while (cursor < trimmed.len && trimmed[cursor] != ' ' && trimmed[cursor] != '\t') {
                    cursor += 1;
                }
                let v_tok = trimmed[v_start .. cursor];
                if (v_tok.len == 0) {
                    return error.InvalidRange;
                }
                try Query.parseOpAndVersionIntoSet(token, v_tok, &set);
            } else {
                try Query.parseTokenIntoSet(token, &set);
            }
        }

        return set;
    }

    static fn isStandaloneOperator(token: []const u8): bool {
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

    /// Parses separated operator + version token directly from original input slices (zero dangling refs).
    static fn parseOpAndVersionIntoSet(op_tok: []const u8, ver_tok: []const u8, set: *ComparatorSet): !void {
        if (std.mem.eql(u8, op_tok, "^")) {
            return Query.addCaret(ver_tok, set);
        }
        if (std.mem.eql(u8, op_tok, "~") || std.mem.eql(u8, op_tok, "~>")) {
            return Query.addTilde(ver_tok, set);
        }
        if (std.mem.eql(u8, op_tok, ">=")) {
            return Query.addGte(ver_tok, set);
        }
        if (std.mem.eql(u8, op_tok, "<=")) {
            return Query.addLte(ver_tok, set);
        }
        if (std.mem.eql(u8, op_tok, ">")) {
            return Query.addGt(ver_tok, set);
        }
        if (std.mem.eql(u8, op_tok, "<")) {
            return Query.addLt(ver_tok, set);
        }
        if (std.mem.eql(u8, op_tok, "=") || std.mem.eql(u8, op_tok, "==")) {
            return Query.addEqOrWildcard(ver_tok, set);
        }
        return error.InvalidRange;
    }

    static fn addCaret(rest: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest) catch return error.InvalidRange;
        if (pv.major == null) {
            set.add(Comparator{ op: Op.any });
            return;
        }
        let maj = pv.major.?;
        if (pv.minor == null) {
            const next_maj = checkedAddOne(maj) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: 0, patch: 0 } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: next_maj, minor: 0, patch: 0 } });
        } else if (pv.patch == null) {
            let min = pv.minor.?;
            if (maj > 0) {
                const next_maj = checkedAddOne(maj) catch return error.InvalidRange;
                set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: 0 } });
                set.add(Comparator{ op: Op.lt, version: Version{ major: next_maj, minor: 0, patch: 0 } });
            } else {
                const next_min = checkedAddOne(min) catch return error.InvalidRange;
                set.add(Comparator{ op: Op.gte, version: Version{ major: 0, minor: min, patch: 0 } });
                set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: next_min, patch: 0 } });
            }
        } else {
            let min = pv.minor.?;
            let pat = pv.patch.?;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: pat, prerelease: pv.prerelease } });
            if (maj > 0) {
                const next_maj = checkedAddOne(maj) catch return error.InvalidRange;
                set.add(Comparator{ op: Op.lt, version: Version{ major: next_maj, minor: 0, patch: 0 } });
            } else if (min > 0) {
                const next_min = checkedAddOne(min) catch return error.InvalidRange;
                set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: next_min, patch: 0 } });
            } else {
                const next_pat = checkedAddOne(pat) catch return error.InvalidRange;
                set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: 0, patch: next_pat } });
            }
        }
    }

    static fn addTilde(rest: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest) catch return error.InvalidRange;
        if (pv.major == null) {
            set.add(Comparator{ op: Op.any });
            return;
        }
        let maj = pv.major.?;
        if (pv.minor == null) {
            const next_maj = checkedAddOne(maj) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: 0, patch: 0 } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: next_maj, minor: 0, patch: 0 } });
        } else {
            let min = pv.minor.?;
            let pat: u64 = pv.patch orelse 0;
            const next_min = checkedAddOne(min) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: pat, prerelease: pv.prerelease } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: maj, minor: next_min, patch: 0 } });
        }
    }

    static fn addGte(rest: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest) catch return error.InvalidRange;
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
    }

    static fn addLte(rest: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest) catch return error.InvalidRange;
        if (pv.major == null) {
            set.add(Comparator{ op: Op.any });
        } else if (pv.minor == null) {
            const next_maj = checkedAddOne(pv.major.?) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.lt, version: Version{ major: next_maj, minor: 0, patch: 0 } });
        } else if (pv.patch == null) {
            const next_min = checkedAddOne(pv.minor.?) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.lt, version: Version{ major: pv.major.?, minor: next_min, patch: 0 } });
        } else {
            set.add(Comparator{ op: Op.lte, version: Version{
                major: pv.major.?,
                minor: pv.minor.?,
                patch: pv.patch.?,
                prerelease: pv.prerelease,
            } });
        }
    }

    static fn addGt(rest: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest) catch return error.InvalidRange;
        if (pv.major == null) {
            set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: 0, patch: 0 } });
        } else if (pv.minor == null) {
            const next_maj = checkedAddOne(pv.major.?) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: next_maj, minor: 0, patch: 0 } });
        } else if (pv.patch == null) {
            const next_min = checkedAddOne(pv.minor.?) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: pv.major.?, minor: next_min, patch: 0 } });
        } else {
            set.add(Comparator{ op: Op.gt, version: Version{
                major: pv.major.?,
                minor: pv.minor.?,
                patch: pv.patch.?,
                prerelease: pv.prerelease,
            } });
        }
    }

    static fn addLt(rest: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest) catch return error.InvalidRange;
        if (pv.major == null) {
            set.add(Comparator{ op: Op.lt, version: Version{ major: 0, minor: 0, patch: 0 } });
        } else {
            set.add(Comparator{ op: Op.lt, version: Version{
                major: pv.major.?,
                minor: pv.minor orelse 0,
                patch: pv.patch orelse 0,
                prerelease: pv.prerelease,
            } });
        }
    }

    static fn addEqOrWildcard(rest_tok: []const u8, set: *ComparatorSet): !void {
        let pv = Version.parsePartial(rest_tok) catch return error.InvalidRange;
        if (pv.major == null) {
            set.add(Comparator{ op: Op.any });
        } else if (pv.minor == null) {
            let maj = pv.major.?;
            const next_maj = checkedAddOne(maj) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: 0, patch: 0 } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: next_maj, minor: 0, patch: 0 } });
        } else if (pv.patch == null) {
            let maj = pv.major.?;
            let min = pv.minor.?;
            const next_min = checkedAddOne(min) catch return error.InvalidRange;
            set.add(Comparator{ op: Op.gte, version: Version{ major: maj, minor: min, patch: 0 } });
            set.add(Comparator{ op: Op.lt, version: Version{ major: maj, minor: next_min, patch: 0 } });
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

    static fn parseTokenIntoSet(token: []const u8, set: *ComparatorSet): !void {
        if (token.len == 0 || std.mem.eql(u8, token, "*") || std.mem.eql(u8, token, "x") || std.mem.eql(u8, token, "X")) {
            set.add(Comparator{ op: Op.any });
            return;
        }
        if (token[0] == '^') {
            return Query.addCaret(token[1..], set);
        }
        if (token[0] == '~') {
            let rest = if (token.len >= 2 && token[1] == '>') token[2..] else token[1..];
            return Query.addTilde(rest, set);
        }
        if (std.mem.startsWith(u8, token, ">=")) {
            return Query.addGte(token[2..], set);
        }
        if (std.mem.startsWith(u8, token, "<=")) {
            return Query.addLte(token[2..], set);
        }
        if (token[0] == '>') {
            return Query.addGt(token[1..], set);
        }
        if (token[0] == '<') {
            return Query.addLt(token[1..], set);
        }
        let rest_tok = token;
        if (std.mem.startsWith(u8, rest_tok, "==")) {
            rest_tok = rest_tok[2..];
        } else if (rest_tok[0] == '=') {
            rest_tok = rest_tok[1..];
        }
        return Query.addEqOrWildcard(rest_tok, set);
    }
};
