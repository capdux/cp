// Source: Absorbed from /data/dev/bun/src/semver/Version.cp | Synced: 2026-11-22T07:05:10Z
// ============================================================================
// lib/semver/version.cp — Semver 2.0.0 Specification Core Types & Logic
// ============================================================================

const std = @import("std");

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
    major: u32? = null,
    minor: u32? = null,
    patch: u32? = null,
    prerelease: []const u8 = "",
    build: []const u8 = "",
};

let inc_scratch: [128]u8 = undefined;

pub const Version = struct {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",

    fn isValidTagIdentChar(c: u8): bool {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '-';
    }

    fn validateTagSegments(tag: []const u8, check_no_leading_zero: bool): bool {
        if (tag.len == 0) return false;
        let iter = std.mem.splitScalar(u8, tag, '.');
        while (seg := iter.next()) {
            if (seg.len == 0) return false;
            let all_digits = true;
            for (c of seg) {
                if (!isValidTagIdentChar(c)) return false;
                if (c < '0' || c > '9') all_digits = false;
            }
            if (check_no_leading_zero && all_digits && seg.len > 1 && seg[0] == '0') {
                return false;
            }
        }
        return true;
    }

    fn isStrictNumericPart(part: []const u8): bool {
        if (part.len == 0) return false;
        if (part.len > 1 && part[0] == '0') return false;
        for (c of part) {
            if (c < '0' || c > '9') return false;
        }
        return true;
    }

    pub fn parseStrict(input_str: []const u8): !Version {
        let str = std.mem.trim(u8, input_str, " \t\r\n");
        if (str.len == 0) return error.InvalidVersion;

        let v = Version{};

        if (plus_pos := std.mem.indexOfScalar(u8, str, '+')) {
            v.build = str[plus_pos + 1 ..];
            if (!validateTagSegments(v.build, false)) return error.InvalidVersion;
            str = str[0..plus_pos];
        }

        if (dash_pos := std.mem.indexOfScalar(u8, str, '-')) {
            v.prerelease = str[dash_pos + 1 ..];
            if (!validateTagSegments(v.prerelease, true)) return error.InvalidVersion;
            str = str[0..dash_pos];
        }

        let iter = std.mem.splitScalar(u8, str, '.');
        const p_maj = iter.next() orelse return error.InvalidVersion;
        const p_min = iter.next() orelse return error.InvalidVersion;
        const p_pat = iter.next() orelse return error.InvalidVersion;
        if (iter.next() != null) return error.InvalidVersion;

        if (!isStrictNumericPart(p_maj) || !isStrictNumericPart(p_min) || !isStrictNumericPart(p_pat)) {
            return error.InvalidVersion;
        }

        v.major = std.fmt.parseInt(u32, p_maj, 10) catch return error.InvalidVersion;
        v.minor = std.fmt.parseInt(u32, p_min, 10) catch return error.InvalidVersion;
        v.patch = std.fmt.parseInt(u32, p_pat, 10) catch return error.InvalidVersion;
        return v;
    }

    pub fn parseLoose(input_str: []const u8): !Version {
        return Version.parse(input_str);
    }

    pub fn parse(input_str: []const u8): !Version {
        let str = std.mem.trim(u8, input_str, " \t\r\n");
        while (str.len > 0 && (str[0] == 'v' || str[0] == 'V' || str[0] == '=' || str[0] == ' ' || str[0] == '\t')) {
            str = str[1..];
        }
        str = std.mem.trim(u8, str, " \t\r\n");
        if (str.len == 0) return error.InvalidVersion;

        let v = Version{};

        // Extract build metadata (+...)
        if (p_idx := std.mem.indexOfScalar(u8, str, '+')) {
            v.build = str[p_idx + 1 ..];
            if (!validateTagSegments(v.build, false)) return error.InvalidVersion;
            str = str[0..p_idx];
        }

        // Extract prerelease (-...)
        if (d_idx := std.mem.indexOfScalar(u8, str, '-')) {
            v.prerelease = str[d_idx + 1 ..];
            if (!validateTagSegments(v.prerelease, false)) return error.InvalidVersion;
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
                        if (!validateTagSegments(v.prerelease, false)) return error.InvalidVersion;
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
        v.major = std.fmt.parseInt(u32, opt_major.?, 10) catch return error.InvalidVersion;

        if (opt_minor := iter.next()) {
            v.minor = std.fmt.parseInt(u32, opt_minor, 10) catch return error.InvalidVersion;
            if (opt_patch := iter.next()) {
                v.patch = std.fmt.parseInt(u32, opt_patch, 10) catch return error.InvalidVersion;
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

    pub fn parsePartial(input_str: []const u8): !PartialVersion {
        let str = std.mem.trim(u8, input_str, " \t\r\n");
        while (str.len > 0 && (str[0] == 'v' || str[0] == 'V' || str[0] == '=' || str[0] == ' ' || str[0] == '\t')) {
            str = str[1..];
        }
        str = std.mem.trim(u8, str, " \t\r\n");
        if (str.len == 0) return error.InvalidVersion;

        let pv = PartialVersion{};

        // Extract build metadata
        if (p_idx := std.mem.indexOfScalar(u8, str, '+')) {
            pv.build = str[p_idx + 1 ..];
            if (!validateTagSegments(pv.build, false)) return error.InvalidVersion;
            str = str[0..p_idx];
        }

        // Extract prerelease
        if (d_idx := std.mem.indexOfScalar(u8, str, '-')) {
            pv.prerelease = str[d_idx + 1 ..];
            if (!validateTagSegments(pv.prerelease, false)) return error.InvalidVersion;
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
                        if (!validateTagSegments(pv.prerelease, false)) return error.InvalidVersion;
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
            if (isWildcard(opt_major)) {
                saw_wildcard = true;
            } else {
                pv.major = std.fmt.parseInt(u32, opt_major, 10) catch return error.InvalidVersion;
            }
        } else {
            return error.InvalidVersion;
        }

        if (opt_minor := iter.next()) {
            if (saw_wildcard || isWildcard(opt_minor)) {
                saw_wildcard = true;
            } else {
                pv.minor = std.fmt.parseInt(u32, opt_minor, 10) catch return error.InvalidVersion;
            }
        }

        if (opt_patch := iter.next()) {
            if (saw_wildcard || isWildcard(opt_patch)) {
                saw_wildcard = true;
            } else {
                pv.patch = std.fmt.parseInt(u32, opt_patch, 10) catch return error.InvalidVersion;
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

    fn isWildcard(part: []const u8): bool {
        return std.mem.eql(u8, part, "*") ||
               std.mem.eql(u8, part, "x") ||
               std.mem.eql(u8, part, "X");
    }

    fn isAllDigits(part: []const u8): bool {
        if (part.len == 0) return false;
        for (c of part) {
            if (c < '0' || c > '9') return false;
        }
        return true;
    }

    fn compareNumericIdent(a: []const u8, b: []const u8): std.math.Order {
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

    pub fn order(a: Version, b: Version): std.math.Order {
        if (a.major != b.major) return if (a.major < b.major) .lt else .gt;
        if (a.minor != b.minor) return if (a.minor < b.minor) .lt else .gt;
        if (a.patch != b.patch) return if (a.patch < b.patch) .lt else .gt;

        // Prerelease comparison per Semver 2.0.0 Spec Section 11:
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

            let a_is_num = isAllDigits(part_a);
            let b_is_num = isAllDigits(part_b);

            if (a_is_num && b_is_num) {
                let ord = compareNumericIdent(part_a, part_b);
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

    pub fn compare(a: Version, b: Version): std.math.Order {
        return Version.order(a, b);
    }

    pub fn eql(a: Version, b: Version): bool {
        return Version.order(a, b) == .eq;
    }

    pub fn lt(a: Version, b: Version): bool {
        return Version.order(a, b) == .lt;
    }

    pub fn lte(a: Version, b: Version): bool {
        let ord = Version.order(a, b);
        return ord == .lt || ord == .eq;
    }

    pub fn gt(a: Version, b: Version): bool {
        return Version.order(a, b) == .gt;
    }

    pub fn gte(a: Version, b: Version): bool {
        let ord = Version.order(a, b);
        return ord == .gt || ord == .eq;
    }

    pub fn format(self: Version, buf: []u8): []const u8 {
        let s = std.fmt.bufPrint(buf, "{d}.{d}.{d}", .{ self.major, self.minor, self.patch }) catch return "";
        let out_len = s.len;

        if (self.prerelease.len > 0) {
            if (out_len < buf.len) {
                buf[out_len] = '-';
                out_len += 1;
            }
            let copy_len = if (out_len + self.prerelease.len <= buf.len) self.prerelease.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_len], self.prerelease[0 .. copy_len]);
            out_len += copy_len;
        }

        if (self.build.len > 0) {
            if (out_len < buf.len) {
                buf[out_len] = '+';
                out_len += 1;
            }
            let copy_len = if (out_len + self.build.len <= buf.len) self.build.len else (buf.len - out_len);
            @memcpy(buf[out_len .. out_len + copy_len], self.build[0 .. copy_len]);
            out_len += copy_len;
        }

        return buf[0 .. out_len];
    }

    pub fn increment(self: Version, release_type: ReleaseType, identifier: ([]const u8)?): Version {
        return incrementWithBuf(self, release_type, identifier, &inc_scratch);
    }

    pub fn incrementWithBuf(self: Version, release_type: ReleaseType, identifier: ([]const u8)?, buf: []u8): Version {
        let id_val = "0";
        if (id := identifier) {
            id_val = id;
        }

        switch (release_type) {
            .major => {
                return Version{
                    major: self.major + 1,
                    minor: 0,
                    patch: 0,
                    prerelease: "",
                    build: "",
                };
            },
            .minor => {
                return Version{
                    major: self.major,
                    minor: self.minor + 1,
                    patch: 0,
                    prerelease: "",
                    build: "",
                };
            },
            .patch => {
                return Version{
                    major: self.major,
                    minor: self.minor,
                    patch: self.patch + 1,
                    prerelease: "",
                    build: "",
                };
            },
            .premajor => {
                let pre = if (identifier != null) id_val else "0";
                return Version{
                    major: self.major + 1,
                    minor: 0,
                    patch: 0,
                    prerelease: pre,
                    build: "",
                };
            },
            .preminor => {
                let pre = if (identifier != null) id_val else "0";
                return Version{
                    major: self.major,
                    minor: self.minor + 1,
                    patch: 0,
                    prerelease: pre,
                    build: "",
                };
            },
            .prepatch => {
                let pre = if (identifier != null) id_val else "0";
                return Version{
                    major: self.major,
                    minor: self.minor,
                    patch: self.patch + 1,
                    prerelease: pre,
                    build: "",
                };
            },
            .prerelease => {
                if (self.prerelease.len == 0) {
                    let pre = if (identifier != null) id_val else "0";
                    return Version{
                        major: self.major,
                        minor: self.minor,
                        patch: self.patch + 1,
                        prerelease: pre,
                        build: "",
                    };
                }

                // If identifier specified and prerelease does not begin with it
                if (id := identifier) {
                    if (!std.mem.startsWith(u8, self.prerelease, id)) {
                        let formatted = std.fmt.bufPrint(buf, "{s}.0", .{id}) catch id;
                        return Version{
                            major: self.major,
                            minor: self.minor,
                            patch: self.patch,
                            prerelease: formatted,
                            build: "",
                        };
                    }
                }

                // Find trailing numeric identifier to increment
                let last_dot: usize? = null;
                let idx: usize = self.prerelease.len;
                while (idx > 0) {
                    idx -= 1;
                    if (self.prerelease[idx] == '.') {
                        last_dot = idx;
                        break;
                    }
                }

                let prefix = "";
                let suffix = self.prerelease;
                if (d := last_dot) {
                    prefix = self.prerelease[0 .. d + 1];
                    suffix = self.prerelease[d + 1 ..];
                }

                if (isAllDigits(suffix)) {
                    let num = std.fmt.parseInt(u64, suffix, 10) catch 0;
                    let formatted = std.fmt.bufPrint(buf, "{s}{d}", .{ prefix, num + 1 }) catch self.prerelease;
                    return Version{
                        major: self.major,
                        minor: self.minor,
                        patch: self.patch,
                        prerelease: formatted,
                        build: "",
                    };
                } else {
                    let formatted = std.fmt.bufPrint(buf, "{s}.0", .{self.prerelease}) catch self.prerelease;
                    return Version{
                        major: self.major,
                        minor: self.minor,
                        patch: self.patch,
                        prerelease: formatted,
                        build: "",
                    };
                }
            },
        }
    }
};
