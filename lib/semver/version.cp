// Source: bun/src/semver/Version.cp
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

    pub fn parse(input_str: []const u8): !Version {
        let str = std.mem.trim(u8, input_str, " \t\r\n");
        if (str.len == 0) return error.InvalidVersion;

        // Strip leading 'v' or 'V' or '='
        if (str[0] == 'v' or str[0] == 'V' or str[0] == '=') {
            str = str[1..];
            str = std.mem.trim(u8, str, " \t\r\n");
        }
        if (str.len == 0) return error.InvalidVersion;

        let v = Version{};

        // Extract build metadata (+...)
        let plus_pos: usize? = null;
        let i: usize = 0;
        while (i < str.len) {
            if (str[i] == '+') {
                plus_pos = i;
                break;
            }
            i += 1;
        }
        if (p_idx := plus_pos) {
            v.build = str[p_idx + 1 ..];
            str = str[0 .. p_idx];
        }

        // Extract prerelease (-...)
        let dash_pos: usize? = null;
        i = 0;
        while (i < str.len) {
            if (str[i] == '-') {
                dash_pos = i;
                break;
            }
            i += 1;
        }
        if (d_idx := dash_pos) {
            v.prerelease = str[d_idx + 1 ..];
            str = str[0 .. d_idx];
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
        if (str.len == 0) return error.InvalidVersion;

        if (str[0] == 'v' or str[0] == 'V' or str[0] == '=') {
            str = str[1..];
            str = std.mem.trim(u8, str, " \t\r\n");
        }
        if (str.len == 0) return error.InvalidVersion;

        let pv = PartialVersion{};

        // Extract build metadata
        let plus_pos: usize? = null;
        let i: usize = 0;
        while (i < str.len) {
            if (str[i] == '+') {
                plus_pos = i;
                break;
            }
            i += 1;
        }
        if (p_idx := plus_pos) {
            pv.build = str[p_idx + 1 ..];
            str = str[0 .. p_idx];
        }

        // Extract prerelease
        let dash_pos: usize? = null;
        i = 0;
        while (i < str.len) {
            if (str[i] == '-') {
                dash_pos = i;
                break;
            }
            i += 1;
        }
        if (d_idx := dash_pos) {
            pv.prerelease = str[d_idx + 1 ..];
            str = str[0 .. d_idx];
        }

        let iter = std.mem.splitScalar(u8, str, '.');
        if (opt_major := iter.next()) {
            if (!isWildcard(opt_major)) {
                pv.major = std.fmt.parseInt(u32, opt_major, 10) catch return error.InvalidVersion;
            }
        }

        if (opt_minor := iter.next()) {
            if (!isWildcard(opt_minor)) {
                pv.minor = std.fmt.parseInt(u32, opt_minor, 10) catch return error.InvalidVersion;
            }
        }

        if (opt_patch := iter.next()) {
            if (!isWildcard(opt_patch)) {
                pv.patch = std.fmt.parseInt(u32, opt_patch, 10) catch return error.InvalidVersion;
            }
        }

        return pv;
    }

    fn isWildcard(part: []const u8): bool {
        return std.mem.eql(u8, part, "*") or
               std.mem.eql(u8, part, "x") or
               std.mem.eql(u8, part, "X");
    }

    fn isAllDigits(part: []const u8): bool {
        if (part.len == 0) return false;
        for (c of part) {
            if (c < '0' or c > '9') return false;
        }
        return true;
    }

    pub fn order(a: Version, b: Version): std.math.Order {
        if (a.major != b.major) return if (a.major < b.major) .lt else .gt;
        if (a.minor != b.minor) return if (a.minor < b.minor) .lt else .gt;
        if (a.patch != b.patch) return if (a.patch < b.patch) .lt else .gt;

        // Prerelease comparison per Semver 2.0.0 Spec Section 11:
        // A normal version without a prerelease has higher precedence than with prerelease
        if (a.prerelease.len == 0 and b.prerelease.len == 0) return .eq;
        if (a.prerelease.len == 0 and b.prerelease.len > 0) return .gt;
        if (a.prerelease.len > 0 and b.prerelease.len == 0) return .lt;

        // Both have prereleases: compare dot-separated identifiers
        let it_a = std.mem.splitScalar(u8, a.prerelease, '.');
        let it_b = std.mem.splitScalar(u8, b.prerelease, '.');

        while (true) {
            let next_a = it_a.next();
            let next_b = it_b.next();

            if (next_a == null and next_b == null) return .eq;
            if (next_a == null) return .lt; // fewer parts = lower precedence
            if (next_b == null) return .gt;

            let part_a = next_a.?;
            let part_b = next_b.?;

            let a_is_num = isAllDigits(part_a);
            let b_is_num = isAllDigits(part_b);

            if (a_is_num and b_is_num) {
                // Both numeric: compare numerically
                let num_a = std.fmt.parseInt(u64, part_a, 10) catch 0;
                let num_b = std.fmt.parseInt(u64, part_b, 10) catch 0;
                if (num_a != num_b) {
                    return if (num_a < num_b) .lt else .gt;
                }
            } else if (a_is_num and !b_is_num) {
                // Numeric identifier has lower precedence than non-numeric
                return .lt;
            } else if (!a_is_num and b_is_num) {
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
        return ord == .lt or ord == .eq;
    }

    pub fn gt(a: Version, b: Version): bool {
        return Version.order(a, b) == .gt;
    }

    pub fn gte(a: Version, b: Version): bool {
        let ord = Version.order(a, b);
        return ord == .gt or ord == .eq;
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
