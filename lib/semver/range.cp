// Source: /data/dev/bun/src/semver/
// ============================================================================
// lib/semver/range.cp — Semver Range & Disjunctive Set Matching Engine
// ============================================================================

const std = @import("std");
import { Version } from "./version";
import { Query, ComparatorSet, Comparator, Op } from "./query";

pub const SemverRange = struct {
    pub fn satisfiesStr(version_str: []const u8, range_str: []const u8): bool {
        let v = Version.parse(version_str) catch return false;
        return SemverRange.satisfies(v, range_str);
    }

    pub fn satisfies(v: Version, range_str: []const u8): bool {
        let trimmed = std.mem.trim(u8, range_str, " \t\r\n");
        if (trimmed.len == 0 or std.mem.eql(u8, trimmed, "*")) {
            return true;
        }

        // Split by logical OR: "||"
        let cursor: usize = 0;
        while (cursor < trimmed.len) {
            let start = cursor;
            let found_or: usize? = null;

            let i = cursor;
            while (i + 1 < trimmed.len) {
                if (trimmed[i] == '|' and trimmed[i + 1] == '|') {
                    found_or = i;
                    break;
                }
                i += 1;
            }

            let end_idx = trimmed.len;
            if (f := found_or) {
                end_idx = f;
                cursor = f + 2;
            } else {
                cursor = trimmed.len;
            }

            let clause = std.mem.trim(u8, trimmed[start .. end_idx], " \t\r\n");
            if (clause.len == 0) continue;

            let set = Query.parseClause(clause) catch continue;
            if (set.matches(v)) {
                return true;
            }
        }

        return false;
    }

    pub fn maxSatisfying(versions: []const Version, range_str: []const u8): Version? {
        let best: Version? = null;
        for (v of versions) {
            if (SemverRange.satisfies(v, range_str)) {
                if (b := best) {
                    if (v.gt(b)) {
                        best = v;
                    }
                } else {
                    best = v;
                }
            }
        }
        return best;
    }

    pub fn minSatisfying(versions: []const Version, range_str: []const u8): Version? {
        let best: Version? = null;
        for (v of versions) {
            if (SemverRange.satisfies(v, range_str)) {
                if (b := best) {
                    if (v.lt(b)) {
                        best = v;
                    }
                } else {
                    best = v;
                }
            }
        }
        return best;
    }

    pub fn maxSatisfyingStr(version_strs: []const []const u8, range_str: []const u8): ([]const u8)? {
        let best_str: ([]const u8)? = null;
        let best_ver: Version? = null;

        for (s of version_strs) {
            let v = Version.parse(s) catch continue;
            if (SemverRange.satisfies(v, range_str)) {
                if (bv := best_ver) {
                    if (v.gt(bv)) {
                        best_ver = v;
                        best_str = s;
                    }
                } else {
                    best_ver = v;
                    best_str = s;
                }
            }
        }
        return best_str;
    }

    pub fn minSatisfyingStr(version_strs: []const []const u8, range_str: []const u8): ([]const u8)? {
        let best_str: ([]const u8)? = null;
        let best_ver: Version? = null;

        for (s of version_strs) {
            let v = Version.parse(s) catch continue;
            if (SemverRange.satisfies(v, range_str)) {
                if (bv := best_ver) {
                    if (v.lt(bv)) {
                        best_ver = v;
                        best_str = s;
                    }
                } else {
                    best_ver = v;
                    best_str = s;
                }
            }
        }
        return best_str;
    }
};

pub const Range = SemverRange;
