// Source: cp/lib/semver_range/version.cp (Semver 2.0.0 Parser & Precedence Comparator)
// Strict Semver 2.0.0 parsing (with optional leading 'v'/'=' normalization) and Rule 11 ordering.

const std = @import("std");
import { SemverError, Semver } from "./types.cp";

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isValidIdentChar(c: u8): bool {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '-';
}

fn isAllDigits(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isAsciiDigit(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn parseUintStrict(s: []const u8): u64 ! SemverError {
    if (s.len == 0) {
        return SemverError.InvalidVersion;
    }
    if (s.len > 1 && s[0] == '0') {
        return SemverError.LeadingZero;
    }
    let acc: u64 = 0;
    const max_u64: u64 = std.math.maxInt(u64);
    let i: usize = 0;
    while (i < s.len) {
        let ch = s[i];
        if (!isAsciiDigit(ch)) {
            return SemverError.InvalidVersion;
        }
        let digit: u64 = (ch - '0') as u64;
        if (acc > (max_u64 - digit) / 10) {
            return SemverError.IntegerOverflow;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

pub fn validateIdentifiers(tag: []const u8, reject_leading_zero: bool, is_build: bool): void ! SemverError {
    if (tag.len == 0) {
        if (is_build) {
            return SemverError.InvalidBuildMetadata;
        }
        return SemverError.InvalidPrerelease;
    }
    let start: usize = 0;
    let i: usize = 0;
    while (i <= tag.len) {
        if (i == tag.len || tag[i] == '.') {
            let seg = tag[start..i];
            if (seg.len == 0) {
                if (is_build) {
                    return SemverError.InvalidBuildMetadata;
                }
                return SemverError.InvalidPrerelease;
            }
            let j: usize = 0;
            while (j < seg.len) {
                if (!isValidIdentChar(seg[j])) {
                    if (is_build) {
                        return SemverError.InvalidBuildMetadata;
                    }
                    return SemverError.InvalidPrerelease;
                }
                j += 1;
            }
            if (reject_leading_zero && isAllDigits(seg) && seg.len > 1 && seg[0] == '0') {
                return SemverError.LeadingZero;
            }
            start = i + 1;
        }
        i += 1;
    }
}

pub fn parseSemver(input: []const u8): Semver ! SemverError {
    let trimmed = std.mem.trim(u8, input, " \t\r\n");
    if (trimmed.len == 0) {
        return SemverError.EmptyInput;
    }

    // Normalize optional leading '=' and/or 'v'/'V'
    let s = trimmed;
    if (s.len > 0 && s[0] == '=') {
        s = std.mem.trimLeft(u8, s[1..], " \t");
    }
    if (s.len > 0 && (s[0] == 'v' || s[0] == 'V')) {
        s = s[1..];
    }
    if (s.len == 0) {
        return SemverError.InvalidVersion;
    }

    // Split off +build metadata first
    let main_and_pre = s;
    let build: []const u8 = "";
    let plus_idx = std.mem.indexOfScalar(u8, s, '+');
    if (plus_idx != null) {
        let idx = plus_idx.?;
        main_and_pre = s[0..idx];
        build = s[idx + 1 ..];
        try validateIdentifiers(build, false, true);
    }

    // Split off -prerelease
    let core = main_and_pre;
    let prerelease: []const u8 = "";
    let dash_idx = std.mem.indexOfScalar(u8, main_and_pre, '-');
    if (dash_idx != null) {
        let idx = dash_idx.?;
        core = main_and_pre[0..idx];
        prerelease = main_and_pre[idx + 1 ..];
        try validateIdentifiers(prerelease, true, false);
    }

    // Parse MAJOR.MINOR.PATCH
    let dot1 = std.mem.indexOfScalar(u8, core, '.');
    if (dot1 == null) {
        return SemverError.InvalidVersion;
    }
    let d1 = dot1.?;
    let rest1 = core[d1 + 1 ..];
    let dot2 = std.mem.indexOfScalar(u8, rest1, '.');
    if (dot2 == null) {
        return SemverError.InvalidVersion;
    }
    let d2 = dot2.?;
    let major_str = core[0..d1];
    let minor_str = rest1[0..d2];
    let patch_str = rest1[d2 + 1 ..];

    if (std.mem.indexOfScalar(u8, patch_str, '.') != null) {
        return SemverError.InvalidVersion;
    }

    let major = try parseUintStrict(major_str);
    let minor = try parseUintStrict(minor_str);
    let patch = try parseUintStrict(patch_str);

    return Semver{
        major: major,
        minor: minor,
        patch: patch,
        prerelease: prerelease,
        build: build,
    };
}

fn compareNumericStrings(a: []const u8, b: []const u8): i8 {
    // Strip any leading zeros safely (for numeric comparison)
    let sa = a;
    while (sa.len > 1 && sa[0] == '0') {
        sa = sa[1..];
    }
    let sb = b;
    while (sb.len > 1 && sb[0] == '0') {
        sb = sb[1..];
    }
    if (sa.len < sb.len) {
        return -1;
    }
    if (sa.len > sb.len) {
        return 1;
    }
    let i: usize = 0;
    while (i < sa.len) {
        if (sa[i] < sb[i]) {
            return -1;
        }
        if (sa[i] > sb[i]) {
            return 1;
        }
        i += 1;
    }
    return 0;
}

fn compareLexStrings(a: []const u8, b: []const u8): i8 {
    let min_len = if (a.len < b.len) { a.len } else { b.len };
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn comparePrerelease(a: []const u8, b: []const u8): i8 {
    if (a.len == 0 && b.len == 0) {
        return 0;
    }
    if (a.len == 0) {
        return 1;
    }
    if (b.len == 0) {
        return -1;
    }

    let ia = std.mem.splitScalar(u8, a, '.');
    let ib = std.mem.splitScalar(u8, b, '.');

    while (true) {
        let seg_a = ia.next();
        let seg_b = ib.next();
        if (seg_a == null && seg_b == null) {
            return 0;
        }
        if (seg_a == null) {
            return -1;
        }
        if (seg_b == null) {
            return 1;
        }

        let sa = seg_a.?;
        let sb = seg_b.?;
        let a_num = isAllDigits(sa);
        let b_num = isAllDigits(sb);

        if (a_num && b_num) {
            let cmp = compareNumericStrings(sa, sb);
            if (cmp != 0) {
                return cmp;
            }
        } else if (a_num && !b_num) {
            return -1;
        } else if (!a_num && b_num) {
            return 1;
        } else {
            let cmp = compareLexStrings(sa, sb);
            if (cmp != 0) {
                return cmp;
            }
        }
    }
}

pub fn compare(a: Semver, b: Semver): i8 {
    if (a.major < b.major) {
        return -1;
    }
    if (a.major > b.major) {
        return 1;
    }
    if (a.minor < b.minor) {
        return -1;
    }
    if (a.minor > b.minor) {
        return 1;
    }
    if (a.patch < b.patch) {
        return -1;
    }
    if (a.patch > b.patch) {
        return 1;
    }
    return comparePrerelease(a.prerelease, b.prerelease);
}

pub fn comparePrecedence(a: Semver, b: Semver): i8 {
    return compare(a, b);
}

pub fn equals(a: Semver, b: Semver): bool {
    return compare(a, b) == 0;
}

pub fn lt(a: Semver, b: Semver): bool {
    return compare(a, b) < 0;
}

pub fn lte(a: Semver, b: Semver): bool {
    return compare(a, b) <= 0;
}

pub fn gt(a: Semver, b: Semver): bool {
    return compare(a, b) > 0;
}

pub fn gte(a: Semver, b: Semver): bool {
    return compare(a, b) >= 0;
}
