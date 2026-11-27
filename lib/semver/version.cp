// Reference-hardened against dtolnay/semver (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/semver/Version.cp | Synced: 2026-11-22T14:51:00Z
// ============================================================================
// lib/semver/version.cp — Semver 2.0.0 Specification Core Types & Strict Parser
// ============================================================================

import { Simd } from "cap:simd";
const std = @import("std");

/// Hard upper bound for a single SemVer 2.0.0 version string to guard against DoS inputs.
pub const SEMVER_MAX_LEN: usize = 256;

/// 16-lane SIMD scan for primary SemVer delimiters (`'.'`, `'-'`, `'+'`, `' '`).
pub fn simdFindSemverDelimiter(input: []const u8): usize? {
    return Simd.indexOfAny4(input, 46 as u8, 45 as u8, 43 as u8, 32 as u8);
}

/// Scalar reference for `simdFindSemverDelimiter` (for differential verification).
pub fn simdFindSemverDelimiterScalar(input: []const u8): usize? {
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == (46 as u8) || c == (45 as u8) || c == (43 as u8) || c == (32 as u8)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// SIMD-accelerated ASCII preflight check for SemVer 2.0.0 character set (`[0-9A-Za-z.+-]`).
pub fn simdIsValidSemverAscii(input: []const u8): bool {
    if (input.len == 0 || input.len > SEMVER_MAX_LEN) return false;
    if (!Simd.isAscii(input)) return false;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        const ok = (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '.' || c == '-' || c == '+';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

fn parseStrictU64(part: []const u8): !u64 {
    if (part.len == 0 || part.len > 20) return error.InvalidVersion;
    if (part.len > 1 && part[0] == '0') return error.InvalidVersion;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < part.len) {
        const c = part[i];
        if (c < '0' || c > '9') return error.InvalidVersion;
        const digit: u64 = (c - '0') as u64;
        if (val > 1844674407370955161) return error.InvalidVersion;
        if (val == 1844674407370955161 && digit > 5) return error.InvalidVersion;
        val = val * 10 + digit;
        i += 1;
    }
    return val;
}

fn parseLooseU64(part: []const u8): !u64 {
    if (part.len == 0) return error.InvalidVersion;
    let s = part;
    while (s.len > 1 && s[0] == '0') {
        s = s[1..];
    }
    if (s.len > 20) return error.InvalidVersion;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return error.InvalidVersion;
        const digit: u64 = (c - '0') as u64;
        if (val > 1844674407370955161) return error.InvalidVersion;
        if (val == 1844674407370955161 && digit > 5) return error.InvalidVersion;
        val = val * 10 + digit;
        i += 1;
    }
    return val;
}

pub const ReleaseType = enum {
    major,
    minor,
    patch,
    premajor,
    preminor,
    prepatch,
    prerelease,
};

pub const PartialVersion = struct {
    major: u64? = null,
    minor: u64? = null,
    patch: u64? = null,
    prerelease: []const u8 = "",
    build: []const u8 = "",
};

let inc_scratch: [128]u8 = undefined;

pub const Version = struct {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",

    static fn isValidTagIdentChar(c: u8): bool {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '-';
    }

    static fn validateTagSegments(tag: []const u8, check_no_leading_zero: bool): bool {
        if (tag.len == 0) return false;
        let start: usize = 0;
        while (start <= tag.len) {
            const dot_rel = Simd.indexOfByte(tag[start..], 46 as u8);
            const end_idx = if (off := dot_rel) (start + off) else tag.len;
            const seg = tag[start..end_idx];
            if (seg.len == 0) return false;

            let all_digits = true;
            let k: usize = 0;
            while (k < seg.len) {
                const c = seg[k];
                if (!Version.isValidTagIdentChar(c)) return false;
                if (c < '0' || c > '9') all_digits = false;
                k += 1;
            }
            if (check_no_leading_zero && all_digits && seg.len > 1 && seg[0] == '0') {
                return false;
            }
            if (end_idx == tag.len) break;
            start = end_idx + 1;
        }
        return true;
    }

    static fn isStrictNumericPart(part: []const u8): bool {
        if (part.len == 0) return false;
        if (part.len > 1 && part[0] == '0') return false;
        let i: usize = 0;
        while (i < part.len) {
            const c = part[i];
            if (c < '0' || c > '9') return false;
            i += 1;
        }
        return true;
    }

    /// Strict SemVer 2.0.0 parser (aligned with `dtolnay/semver`):
    /// - Rejects leading/trailing whitespace, non-ASCII, empty segments, leading zeros in
    ///   major/minor/patch or numeric prerelease identifiers, and `u64` integer overflow;
    /// - Allows alphanumeric prerelease segments with leading zero (e.g. `1.0.0-01a`)
    ///   and numeric build metadata with leading zeros (e.g. `1.0.0+0851523`).
    pub static fn parseStrict(input_str: []const u8): !Version {
        if (!simdIsValidSemverAscii(input_str)) return error.InvalidVersion;

        let str = input_str;
        let v = Version{};

        if (plus_pos := Simd.indexOfByte(str, 43 as u8)) { // '+'
            v.build = str[plus_pos + 1 ..];
            if (!Version.validateTagSegments(v.build, false)) return error.InvalidVersion;
            str = str[0..plus_pos];
        }

        if (dash_pos := Simd.indexOfByte(str, 45 as u8)) { // '-'
            v.prerelease = str[dash_pos + 1 ..];
            if (!Version.validateTagSegments(v.prerelease, true)) return error.InvalidVersion;
            str = str[0..dash_pos];
        }

        // Exactly 3 dot-separated numeric segments: major.minor.patch
        const d1 = Simd.indexOfByte(str, 46 as u8) orelse return error.InvalidVersion;
        const p_maj = str[0..d1];
        const rest1 = str[d1 + 1 ..];
        const d2 = Simd.indexOfByte(rest1, 46 as u8) orelse return error.InvalidVersion;
        const p_min = rest1[0..d2];
        const p_pat = rest1[d2 + 1 ..];
        if (Simd.indexOfByte(p_pat, 46 as u8) != null) return error.InvalidVersion;

        v.major = parseStrictU64(p_maj) catch return error.InvalidVersion;
        v.minor = parseStrictU64(p_min) catch return error.InvalidVersion;
        v.patch = parseStrictU64(p_pat) catch return error.InvalidVersion;
        return v;
    }

    pub static fn parseLoose(input_str: []const u8): !Version {
        return Version.parse(input_str);
    }

    pub static fn parse(input_str: []const u8): !Version {
        if (input_str.len == 0 || input_str.len > SEMVER_MAX_LEN) return error.InvalidVersion;
        if (!Simd.isAscii(input_str)) return error.InvalidVersion;

        let str = std.mem.trim(u8, input_str, " \t\r\n");
        while (str.len > 0 && (str[0] == 'v' || str[0] == 'V' || str[0] == '=' || str[0] == ' ' || str[0] == '\t')) {
            str = str[1..];
        }
        str = std.mem.trim(u8, str, " \t\r\n");
        if (str.len == 0) return error.InvalidVersion;

        let v = Version{};

        // Extract build metadata (+...)
        if (p_idx := Simd.indexOfByte(str, 43 as u8)) {
            v.build = str[p_idx + 1 ..];
            if (!Version.validateTagSegments(v.build, false)) return error.InvalidVersion;
            str = str[0..p_idx];
        }

        // Extract prerelease (-...)
        if (d_idx := Simd.indexOfByte(str, 45 as u8)) {
            v.prerelease = str[d_idx + 1 ..];
            if (!Version.validateTagSegments(v.prerelease, false)) return error.InvalidVersion;
            str = str[0..d_idx];
        }

        // Handle loose inline prerelease without '-' like "1.0.0rc.1"
        if (v.prerelease.len == 0) {
            let dot_count: usize = 0;
            let k: usize = 0;
            while (k < str.len) {
                const c = str[k];
                if (c == '.') {
                    dot_count += 1;
                } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    if (dot_count >= 2 && k > 0) {
                        v.prerelease = str[k..];
                        if (!Version.validateTagSegments(v.prerelease, false)) return error.InvalidVersion;
                        str = str[0..k];
                        break;
                    }
                    return error.InvalidVersion;
                } else if (c < '0' || c > '9') {
                    return error.InvalidVersion;
                }
                k += 1;
            }
        }

        // Parse major.minor.patch
        let iter = std.mem.splitScalar(u8, str, '.');
        let opt_major = iter.next();
        if (opt_major == null) return error.InvalidVersion;
        v.major = parseLooseU64(opt_major.?) catch return error.InvalidVersion;

        if (opt_minor := iter.next()) {
            v.minor = parseLooseU64(opt_minor) catch return error.InvalidVersion;
            if (opt_patch := iter.next()) {
                v.patch = parseLooseU64(opt_patch) catch return error.InvalidVersion;
            } else {
                v.patch = 0;
            }
        } else {
            v.minor = 0;
            v.patch = 0;
        }

        if (iter.next() != null) return error.InvalidVersion;
        return v;
    }

    pub static fn parsePartial(input_str: []const u8): !PartialVersion {
        if (input_str.len == 0 || input_str.len > SEMVER_MAX_LEN) return error.InvalidVersion;
        if (!Simd.isAscii(input_str)) return error.InvalidVersion;

        let str = std.mem.trim(u8, input_str, " \t\r\n");
        while (str.len > 0 && (str[0] == 'v' || str[0] == 'V' || str[0] == '=' || str[0] == ' ' || str[0] == '\t')) {
            str = str[1..];
        }
        str = std.mem.trim(u8, str, " \t\r\n");
        if (str.len == 0) return error.InvalidVersion;

        let pv = PartialVersion{};

        // Extract build metadata
        if (p_idx := Simd.indexOfByte(str, 43 as u8)) {
            pv.build = str[p_idx + 1 ..];
            if (!Version.validateTagSegments(pv.build, false)) return error.InvalidVersion;
            str = str[0..p_idx];
        }

        // Extract prerelease
        if (d_idx := Simd.indexOfByte(str, 45 as u8)) {
            pv.prerelease = str[d_idx + 1 ..];
            if (!Version.validateTagSegments(pv.prerelease, false)) return error.InvalidVersion;
            str = str[0..d_idx];
        }

        // Handle loose inline prerelease without '-' like "1.2.3rc.1"
        if (pv.prerelease.len == 0) {
            let dot_count: usize = 0;
            let k: usize = 0;
            while (k < str.len) {
                const c = str[k];
                if (c == '.') {
                    dot_count += 1;
                } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    if (c != 'x' && c != 'X' && dot_count >= 2 && k > 0) {
                        pv.prerelease = str[k..];
                        if (!Version.validateTagSegments(pv.prerelease, false)) return error.InvalidVersion;
                        str = str[0..k];
                        break;
                    }
                }
                k += 1;
            }
        }

        let iter = std.mem.splitScalar(u8, str, '.');
        let saw_wildcard = false;

        if (opt_major := iter.next()) {
            if (Version.isWildcard(opt_major)) {
                saw_wildcard = true;
            } else {
                pv.major = parseLooseU64(opt_major) catch return error.InvalidVersion;
            }
        } else {
            return error.InvalidVersion;
        }

        if (opt_minor := iter.next()) {
            if (saw_wildcard || Version.isWildcard(opt_minor)) {
                saw_wildcard = true;
            } else {
                pv.minor = parseLooseU64(opt_minor) catch return error.InvalidVersion;
            }
        }

        if (opt_patch := iter.next()) {
            if (saw_wildcard || Version.isWildcard(opt_patch)) {
                saw_wildcard = true;
            } else {
                pv.patch = parseLooseU64(opt_patch) catch return error.InvalidVersion;
            }
        }

        if (iter.next() != null) return error.InvalidVersion;

        return pv;
    }

    pub static fn valid(input_str: []const u8, buf: []u8): ([]const u8)? {
        let v = Version.parse(input_str) catch return null;
        return v.format(buf);
    }

    pub static fn clean(input_str: []const u8, buf: []u8): ([]const u8)? {
        return Version.valid(input_str, buf);
    }

    static fn isWildcard(part: []const u8): bool {
        return std.mem.eql(u8, part, "*") ||
               std.mem.eql(u8, part, "x") ||
               std.mem.eql(u8, part, "X");
    }

    static fn isAllDigits(part: []const u8): bool {
        if (part.len == 0) return false;
        let i: usize = 0;
        while (i < part.len) {
            const c = part[i];
            if (c < '0' || c > '9') return false;
            i += 1;
        }
        return true;
    }

    static fn compareNumericIdent(a: []const u8, b: []const u8): std.math.Order {
        // Strip leading zeros if any
        let sa = a;
        while (sa.len > 1 && sa[0] == '0') sa = sa[1..];
        let sb = b;
        while (sb.len > 1 && sb[0] == '0') sb = sb[1..];

        if (sa.len != sb.len) {
            return if (sa.len < sb.len) .lt else .gt;
        }
        return std.mem.order(u8, sa, sb);
    }

    pub fn isPrerelease(): bool {
        return _.prerelease.len > 0;
    }

    /// SemVer 2.0.0 Section 11 precedence comparison (build metadata is strictly ignored).
    pub static fn order(a: Version, b: Version): std.math.Order {
        if (a.major != b.major) return if (a.major < b.major) .lt else .gt;
        if (a.minor != b.minor) return if (a.minor < b.minor) .lt else .gt;
        if (a.patch != b.patch) return if (a.patch < b.patch) .lt else .gt;

        // A normal version without a prerelease has higher precedence than with prerelease
        if (a.prerelease.len == 0 && b.prerelease.len == 0) return .eq;
        if (a.prerelease.len == 0 && b.prerelease.len > 0) return .gt;
        if (a.prerelease.len > 0 && b.prerelease.len == 0) return .lt;

        // Both have prereleases: compare dot-separated identifiers
        let it_a = std.mem.splitScalar(u8, a.prerelease, '.');
        let it_b = std.mem.splitScalar(u8, b.prerelease, '.');

        while (true) {
            let next_a = it_a.next();
            let next_b = it_b.next();

            if (next_a == null && next_b == null) return .eq;
            if (next_a == null) return .lt; // fewer parts = lower precedence
            if (next_b == null) return .gt;

            let part_a = next_a.?;
            let part_b = next_b.?;

            let a_is_num = Version.isAllDigits(part_a);
            let b_is_num = Version.isAllDigits(part_b);

            if (a_is_num && b_is_num) {
                let ord = Version.compareNumericIdent(part_a, part_b);
                if (ord != .eq) return ord;
            } else if (a_is_num && !b_is_num) {
                // Numeric identifier has lower precedence than non-numeric
                return .lt;
            } else if (!a_is_num && b_is_num) {
                return .gt;
            } else {
                // Both string identifiers: lexical ASCII comparison
                let ord = std.mem.order(u8, part_a, part_b);
                if (ord != .eq) return ord;
            }
        }
    }

    pub static fn compare(a: Version, b: Version): std.math.Order {
        return Version.order(a, b);
    }

    pub fn eql(b: Version): bool {
        return Version.order(_, b) == .eq;
    }

    pub fn lt(b: Version): bool {
        return Version.order(_, b) == .lt;
    }

    pub fn lte(b: Version): bool {
        let ord = Version.order(_, b);
        return ord == .lt || ord == .eq;
    }

    pub fn gt(b: Version): bool {
        return Version.order(_, b) == .gt;
    }

    pub fn gte(b: Version): bool {
        let ord = Version.order(_, b);
        return ord == .gt || ord == .eq;
    }

    pub fn format(buf: []u8): []const u8 {
        let s = std.fmt.bufPrint(buf, "{d}.{d}.{d}", .{ _.major, _.minor, _.patch }) catch return "";
        let out_len = s.len;

        if (_.prerelease.len > 0) {
            if (out_len < buf.len) {
                buf[out_len] = '-';
                out_len += 1;
            }
            let copy_len = if (out_len + _.prerelease.len <= buf.len) _.prerelease.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_len], _.prerelease[0 .. copy_len]);
            out_len += copy_len;
        }

        if (_.build.len > 0) {
            if (out_len < buf.len) {
                buf[out_len] = '+';
                out_len += 1;
            }
            let copy_len = if (out_len + _.build.len <= buf.len) _.build.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_len], _.build[0 .. copy_len]);
            out_len += copy_len;
        }

        return buf[0 .. out_len];
    }

    pub fn increment(release_type: ReleaseType, identifier: ([]const u8)?): Version {
        return _.incrementWithBuf(release_type, identifier, &inc_scratch);
    }

    pub fn incrementWithBuf(release_type: ReleaseType, identifier: ([]const u8)?, buf: []u8): Version {
        const max_u64: u64 = std.math.maxInt(u64);
        let id_val = "0";
        if (id := identifier) {
            id_val = id;
        }

        switch (release_type) {
            .major => {
                return Version{
                    major: if (_.major < max_u64) (_.major + 1) else max_u64,
                    minor: 0,
                    patch: 0,
                    prerelease: "",
                    build: "",
                };
            },
            .minor => {
                return Version{
                    major: _.major,
                    minor: if (_.minor < max_u64) (_.minor + 1) else max_u64,
                    patch: 0,
                    prerelease: "",
                    build: "",
                };
            },
            .patch => {
                return Version{
                    major: _.major,
                    minor: _.minor,
                    patch: if (_.patch < max_u64) (_.patch + 1) else max_u64,
                    prerelease: "",
                    build: "",
                };
            },
            .premajor => {
                let pre = if (identifier != null) id_val else "0";
                return Version{
                    major: if (_.major < max_u64) (_.major + 1) else max_u64,
                    minor: 0,
                    patch: 0,
                    prerelease: pre,
                    build: "",
                };
            },
            .preminor => {
                let pre = if (identifier != null) id_val else "0";
                return Version{
                    major: _.major,
                    minor: if (_.minor < max_u64) (_.minor + 1) else max_u64,
                    patch: 0,
                    prerelease: pre,
                    build: "",
                };
            },
            .prepatch => {
                let pre = if (identifier != null) id_val else "0";
                return Version{
                    major: _.major,
                    minor: _.minor,
                    patch: if (_.patch < max_u64) (_.patch + 1) else max_u64,
                    prerelease: pre,
                    build: "",
                };
            },
            .prerelease => {
                if (_.prerelease.len == 0) {
                    let pre = if (identifier != null) id_val else "0";
                    return Version{
                        major: _.major,
                        minor: _.minor,
                        patch: if (_.patch < max_u64) (_.patch + 1) else max_u64,
                        prerelease: pre,
                        build: "",
                    };
                }

                // If identifier specified and prerelease does not begin with it
                if (id := identifier) {
                    if (!std.mem.startsWith(u8, _.prerelease, id)) {
                        let formatted = std.fmt.bufPrint(buf, "{s}.0", .{id}) catch id;
                        return Version{
                            major: _.major,
                            minor: _.minor,
                            patch: _.patch,
                            prerelease: formatted,
                            build: "",
                        };
                    }
                }

                // Find trailing numeric identifier to increment
                let last_dot: usize? = null;
                let idx: usize = _.prerelease.len;
                while (idx > 0) {
                    idx -= 1;
                    if (_.prerelease[idx] == '.') {
                        last_dot = idx;
                        break;
                    }
                }

                let prefix = "";
                let suffix = _.prerelease;
                if (d := last_dot) {
                    prefix = _.prerelease[0 .. d + 1];
                    suffix = _.prerelease[d + 1 ..];
                }

                if (Version.isAllDigits(suffix)) {
                    let num = std.fmt.parseInt(u64, suffix, 10) catch 0;
                    const next_num = if (num < max_u64) (num + 1) else max_u64;
                    let formatted = std.fmt.bufPrint(buf, "{s}{d}", .{ prefix, next_num }) catch _.prerelease;
                    return Version{
                        major: _.major,
                        minor: _.minor,
                        patch: _.patch,
                        prerelease: formatted,
                        build: "",
                    };
                } else {
                    let formatted = std.fmt.bufPrint(buf, "{s}.0", .{_.prerelease}) catch _.prerelease;
                    return Version{
                        major: _.major,
                        minor: _.minor,
                        patch: _.patch,
                        prerelease: formatted,
                        build: "",
                    };
                }
            },
        }
    }

    /// 64-bit Wyhash fingerprint over (major, minor, patch, prerelease, build), synced from Bun Version.cp.
    pub fn hash(): u64 {
        let h = std.hash.Wyhash.init(0);
        h.update(std.mem.asBytes(&_.major));
        h.update(std.mem.asBytes(&_.minor));
        h.update(std.mem.asBytes(&_.patch));
        h.update(_.prerelease);
        h.update(&[0xff]);
        h.update(_.build);
        return h.final();
    }
};

/// Zero-allocation strict SemVer 2.0.0 validity predicate.
pub fn isValidSemverStrict(input: []const u8): bool {
    * = Version.parseStrict(input) catch return false;
    return true;
}

/// Strictly parses two SemVer 2.0.0 strings and compares their §11 precedence (build metadata ignored).
pub fn compareSemverPrecedence(a_str: []const u8, b_str: []const u8): std.math.Order? {
    const a = Version.parseStrict(a_str) catch return null;
    const b = Version.parseStrict(b_str) catch return null;
    return Version.order(a, b);
}
