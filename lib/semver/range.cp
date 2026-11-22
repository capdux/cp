// Source: Absorbed from /data/dev/bun/src/semver/SemverRange.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/semver/range.cp — Semver Range & Disjunctive Set Matching Engine
// ============================================================================

const std = @import("std");
import { Version } from "./version";
import { Query, ComparatorSet, Comparator, Op } from "./query";

pub const SemverRange = struct {
    pub static fn isValidRange(range_str: []const u8): bool {
        let trimmed = std.mem.trim(u8, range_str, " \t\r\n");
        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "*")) {
            return true;
        }

        let cursor: usize = 0;
        while (cursor < trimmed.len) {
            let start = cursor;
            let found_or: usize? = null;

            let i = cursor;
            while (i + 1 < trimmed.len) {
                if (trimmed[i] == '|' && trimmed[i + 1] == '|') {
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

            * = Query.parseClause(clause) catch return false;
        }

        return true;
    }

    pub static fn validRange(range_str: []const u8): ([]const u8)? {
        if (!SemverRange.isValidRange(range_str)) {
            return null;
        }
        let trimmed = std.mem.trim(u8, range_str, " \t\r\n");
        if (trimmed.len == 0) {
            return "*";
        }
        return trimmed;
    }

    pub static fn validRangeBuf(range_str: []const u8, buf: []u8): ([]const u8)? {
        let trimmed = std.mem.trim(u8, range_str, " \t\r\n");
        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "*")) {
            if (buf.len > 0) {
                buf[0] = '*';
                return buf[0..1];
            }
            return "*";
        }

        let pos: usize = 0;
        let clause_count: usize = 0;
        let cursor: usize = 0;
        while (cursor < trimmed.len) {
            let start = cursor;
            let found_or: usize? = null;
            let i = cursor;
            while (i + 1 < trimmed.len) {
                if (trimmed[i] == '|' && trimmed[i + 1] == '|') {
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

            let set = Query.parseClause(clause) catch return null;
            if (clause_count > 0) {
                const sep = " || ";
                let k: usize = 0;
                while (k < sep.len && pos < buf.len) {
                    buf[pos] = sep[k];
                    pos += 1;
                    k += 1;
                }
            }
            let c_buf: [512]u8 = undefined;
            const c_str = set.format(&c_buf);
            let k: usize = 0;
            while (k < c_str.len && pos < buf.len) {
                buf[pos] = c_str[k];
                pos += 1;
                k += 1;
            }
            clause_count += 1;
        }

        if (clause_count == 0) {
            if (buf.len > 0) {
                buf[0] = '*';
                return buf[0..1];
            }
            return "*";
        }
        return buf[0..pos];
    }

    pub static fn satisfiesStr(version_str: []const u8, range_str: []const u8): bool {
        return SemverRange.satisfiesStrWithOptions(version_str, range_str, false);
    }

    pub static fn satisfiesStrWithOptions(version_str: []const u8, range_str: []const u8, include_prerelease: bool): bool {
        let v = Version.parse(version_str) catch return false;
        return SemverRange.satisfiesWithOptions(v, range_str, include_prerelease);
    }

    pub static fn satisfies(v: Version, range_str: []const u8): bool {
        return SemverRange.satisfiesWithOptions(v, range_str, false);
    }

    pub static fn satisfiesWithOptions(v: Version, range_str: []const u8, include_prerelease: bool): bool {
        let trimmed = std.mem.trim(u8, range_str, " \t\r\n");
        if (trimmed.len == 0 || std.mem.eql(u8, trimmed, "*")) {
            return include_prerelease || v.prerelease.len == 0;
        }

        let matched = false;
        let cursor: usize = 0;
        while (cursor < trimmed.len) {
            let start = cursor;
            let found_or: usize? = null;

            let i = cursor;
            while (i + 1 < trimmed.len) {
                if (trimmed[i] == '|' && trimmed[i + 1] == '|') {
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

            let set = Query.parseClause(clause) catch return false;
            if (set.matchesWithOptions(v, include_prerelease)) {
                matched = true;
            }
        }

        return matched;
    }

    pub static fn sort(versions: []Version) {
        if (versions.len < 2) return;
        let i: usize = 1;
        while (i < versions.len) {
            const key = versions[i];
            let j: usize = i;
            while (j > 0 && Version.order(versions[j - 1], key) == .gt) {
                versions[j] = versions[j - 1];
                j -= 1;
            }
            versions[j] = key;
            i += 1;
        }
    }

    pub static fn rsort(versions: []Version) {
        if (versions.len < 2) return;
        let i: usize = 1;
        while (i < versions.len) {
            const key = versions[i];
            let j: usize = i;
            while (j > 0 && Version.order(versions[j - 1], key) == .lt) {
                versions[j] = versions[j - 1];
                j -= 1;
            }
            versions[j] = key;
            i += 1;
        }
    }

    pub static fn sortStr(version_strs: [][]const u8) {
        if (version_strs.len < 2) return;
        let i: usize = 1;
        while (i < version_strs.len) {
            const key = version_strs[i];
            const kv_opt: Version? = Version.parse(key) catch null;
            let j: usize = i;
            while (j > 0) {
                const pv_opt: Version? = Version.parse(version_strs[j - 1]) catch null;
                let swap = false;
                if (kv := kv_opt) {
                    if (pv := pv_opt) {
                        swap = Version.order(pv, kv) == .gt;
                    } else {
                        // Valid versions sort before invalid strings
                        swap = true;
                    }
                }
                if (!swap) break;
                version_strs[j] = version_strs[j - 1];
                j -= 1;
            }
            version_strs[j] = key;
            i += 1;
        }
    }

    pub static fn rsortStr(version_strs: [][]const u8) {
        if (version_strs.len < 2) return;
        let i: usize = 1;
        while (i < version_strs.len) {
            const key = version_strs[i];
            const kv_opt: Version? = Version.parse(key) catch null;
            let j: usize = i;
            while (j > 0) {
                const pv_opt: Version? = Version.parse(version_strs[j - 1]) catch null;
                let swap = false;
                if (kv := kv_opt) {
                    if (pv := pv_opt) {
                        swap = Version.order(pv, kv) == .lt;
                    } else {
                        swap = true;
                    }
                }
                if (!swap) break;
                version_strs[j] = version_strs[j - 1];
                j -= 1;
            }
            version_strs[j] = key;
            i += 1;
        }
    }

    pub static fn maxSatisfying(versions: []const Version, range_str: []const u8): Version? {
        return SemverRange.maxSatisfyingWithOptions(versions, range_str, false);
    }

    pub static fn maxSatisfyingWithOptions(versions: []const Version, range_str: []const u8, include_prerelease: bool): Version? {
        let best: Version? = null;
        for (v of versions) {
            if (SemverRange.satisfiesWithOptions(v, range_str, include_prerelease)) {
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

    pub static fn minSatisfying(versions: []const Version, range_str: []const u8): Version? {
        return SemverRange.minSatisfyingWithOptions(versions, range_str, false);
    }

    pub static fn minSatisfyingWithOptions(versions: []const Version, range_str: []const u8, include_prerelease: bool): Version? {
        let best: Version? = null;
        for (v of versions) {
            if (SemverRange.satisfiesWithOptions(v, range_str, include_prerelease)) {
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

    pub static fn maxSatisfyingStr(version_strs: []const []const u8, range_str: []const u8): ([]const u8)? {
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

    pub static fn minSatisfyingStr(version_strs: []const []const u8, range_str: []const u8): ([]const u8)? {
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
