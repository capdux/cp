// Source: Absorbed from /data/dev/bun/src/install/semver.zig | Synced: 2026-11-21T16:39:55Z (SemVer 2.0.0 Version Bumper & Formatter)

/// SemVer 2.0.0 版本递增类型枚举
pub enum BumpKind: u8 {
    major = 0,
    minor = 1,
    patch = 2,
    prerelease = 3,
    release = 4,

    fn asStr(): string {
        return switch (_) {
            .major => "major",
            .minor => "minor",
            .patch => "patch",
            .prerelease => "prerelease",
            .release => "release",
        };
    }
}

struct ParsedSemver {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentChar(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn parseStrictU64(s: []const u8): u64? {
    if (s.len == 0) { return null; }
    if (s.len > 1 && s[0] == '0') { return null; } // 禁止前导零
    const max_u64: u64 = 18446744073709551615;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) { return null; }
        const d: u64 = @as(u64, s[i] - '0');
        if (acc > ((max_u64 - d) / 10)) { return null; }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

fn isValidDotIdentifiers(s: []const u8, forbid_numeric_leading_zero: bool): bool {
    if (s.len == 0) { return false; }
    let i: usize = 0;
    while (i <= s.len) {
        const start = i;
        let all_digits = true;
        while (i < s.len && s[i] != '.') {
            if (!isIdentChar(s[i])) { return false; }
            if (!isDigit(s[i])) { all_digits = false; }
            i += 1;
        }
        const seg = s[start..i];
        if (seg.len == 0) { return false; }
        if (forbid_numeric_leading_zero && all_digits && seg.len > 1 && seg[0] == '0') {
            return false;
        }
        if (i == s.len) { break; }
        i += 1;
    }
    return true;
}

fn parseSemver(raw: []const u8): ParsedSemver? {
    let s = raw;
    if (s.len > 0 && (s[0] == 'v' || s[0] == 'V')) {
        s = s[1..s.len];
    }
    if (s.len == 0) { return null; }

    // 拆分 +build
    let core_pre = s;
    let build_str: []const u8 = "";
    let plus_idx: usize? = null;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '+') {
            plus_idx = i;
            break;
        }
        i += 1;
    }
    if (pi := plus_idx) {
        core_pre = s[0..pi];
        build_str = s[(pi + 1)..s.len];
        if (!isValidDotIdentifiers(build_str, false)) {
            return null;
        }
    }

    // 拆分 -prerelease
    let core = core_pre;
    let pre_str: []const u8 = "";
    let dash_idx: usize? = null;
    let k: usize = 0;
    while (k < core_pre.len) {
        if (core_pre[k] == '-') {
            dash_idx = k;
            break;
        }
        k += 1;
    }
    if (di := dash_idx) {
        core = core_pre[0..di];
        pre_str = core_pre[(di + 1)..core_pre.len];
        if (!isValidDotIdentifiers(pre_str, true)) {
            return null;
        }
    }

    // 拆分 major.minor.patch
    let d1: usize? = null;
    let d2: usize? = null;
    let m: usize = 0;
    while (m < core.len) {
        if (core[m] == '.') {
            if (d1 == null) {
                d1 = m;
            } else if (d2 == null) {
                d2 = m;
            } else {
                return null; // 超过两个点
            }
        }
        m += 1;
    }

    if (dot1 := d1) {
        if (dot2 := d2) {
            const maj = parseStrictU64(core[0..dot1]) ?? return null;
            const min = parseStrictU64(core[(dot1 + 1)..dot2]) ?? return null;
            const pat = parseStrictU64(core[(dot2 + 1)..core.len]) ?? return null;
            return ParsedSemver{
                major: maj,
                minor: min,
                patch: pat,
                prerelease: pre_str,
                build: build_str,
            };
        }
    }
    return null;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeU64(*_, val: u64) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

fn formatCore(major: u64, minor: u64, patch: u64, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    w.writeU64(major);
    w.writeByte('.');
    w.writeU64(minor);
    w.writeByte('.');
    w.writeU64(patch);
    return w.finish();
}

/// 递增 Major 版本号：`1.2.3` -> `2.0.0`、`2.0.0-alpha.1` -> `2.0.0`、`1.2.3-beta.0` -> `2.0.0`
pub fn bumpMajor(version: []const u8, out: []u8): ([]const u8)? {
    const v = parseSemver(version) ?? return null;
    if (v.minor == 0 && v.patch == 0 && v.prerelease.len > 0) {
        return formatCore(v.major, 0, 0, out);
    }
    if (v.major == 18446744073709551615) { return null; }
    return formatCore(v.major + 1, 0, 0, out);
}

/// 递增 Minor 版本号：`1.2.3` -> `1.3.0`、`1.3.0-alpha.1` -> `1.3.0`
pub fn bumpMinor(version: []const u8, out: []u8): ([]const u8)? {
    const v = parseSemver(version) ?? return null;
    if (v.patch == 0 && v.prerelease.len > 0) {
        return formatCore(v.major, v.minor, 0, out);
    }
    if (v.minor == 18446744073709551615) { return null; }
    return formatCore(v.major, v.minor + 1, 0, out);
}

/// 递增 Patch 版本号：`1.2.3` -> `1.2.4`、`1.2.4-alpha.1` -> `1.2.4`
pub fn bumpPatch(version: []const u8, out: []u8): ([]const u8)? {
    const v = parseSemver(version) ?? return null;
    if (v.prerelease.len > 0) {
        return formatCore(v.major, v.minor, v.patch, out);
    }
    if (v.patch == 18446744073709551615) { return null; }
    return formatCore(v.major, v.minor, v.patch + 1, out);
}

/// 剥离预发布标签与构建元数据：`1.2.3-alpha.1+build.9` -> `1.2.3`
pub fn stripPrereleaseAndBuild(version: []const u8, out: []u8): ([]const u8)? {
    const v = parseSemver(version) ?? return null;
    return formatCore(v.major, v.minor, v.patch, out);
}

/// 递增预发布版本号：
/// - `1.2.3` + `"alpha"` -> `1.2.4-alpha.0`
/// - `1.2.4-alpha.0` + `"alpha"` -> `1.2.4-alpha.1`
/// - `1.2.4-alpha.1` + `"beta"` -> `1.2.4-beta.0`
/// - `1.2.4-alpha.1` + `""` -> `1.2.4-alpha.2`
pub fn bumpPrerelease(version: []const u8, preid: []const u8, out: []u8): ([]const u8)? {
    const v = parseSemver(version) ?? return null;
    if (preid.len > 0 && !isValidDotIdentifiers(preid, true)) {
        return null;
    }

    let w = OutWriter{ buf: out };

    // 情况 1：当前无 prerelease，递增 patch 并附加预发布后缀
    if (v.prerelease.len == 0) {
        if (v.patch == 18446744073709551615) { return null; }
        w.writeU64(v.major);
        w.writeByte('.');
        w.writeU64(v.minor);
        w.writeByte('.');
        w.writeU64(v.patch + 1);
        w.writeByte('-');
        if (preid.len > 0) {
            w.writeStr(preid);
            w.writeStr(".0");
        } else {
            w.writeByte('0');
        }
        return w.finish();
    }

    // 情况 2：当前已有 prerelease
    w.writeU64(v.major);
    w.writeByte('.');
    w.writeU64(v.minor);
    w.writeByte('.');
    w.writeU64(v.patch);
    w.writeByte('-');

    const pre = v.prerelease;
    if (preid.len > 0) {
        if (eqlBytes(pre, preid)) {
            w.writeStr(preid);
            w.writeStr(".0");
            return w.finish();
        }
        if (pre.len > preid.len + 1 &&
            eqlBytes(pre[0..preid.len], preid) &&
            pre[preid.len] == '.')
        {
            const tail = pre[(preid.len + 1)..pre.len];
            if (num := parseStrictU64(tail)) {
                if (num == 18446744073709551615) { return null; }
                w.writeStr(preid);
                w.writeByte('.');
                w.writeU64(num + 1);
                return w.finish();
            }
        }
        w.writeStr(preid);
        w.writeStr(".0");
        return w.finish();
    }

    // preid 为空：定位最后一个数字段递增，若无数字段则追加 `.0`
    let last_dot: usize? = null;
    let idx: usize = 0;
    while (idx < pre.len) {
        if (pre[idx] == '.') { last_dot = idx; }
        idx += 1;
    }

    if (ld := last_dot) {
        const tail = pre[(ld + 1)..pre.len];
        if (num := parseStrictU64(tail)) {
            if (num == 18446744073709551615) { return null; }
            w.writeStr(pre[0..ld]);
            w.writeByte('.');
            w.writeU64(num + 1);
            return w.finish();
        }
    } else {
        if (num := parseStrictU64(pre)) {
            if (num == 18446744073709551615) { return null; }
            w.writeU64(num + 1);
            return w.finish();
        }
    }

    w.writeStr(pre);
    w.writeStr(".0");
    return w.finish();
}

/// 统一版本递增入口
pub fn bumpVersion(version: []const u8, kind: BumpKind, preid: []const u8, out: []u8): ([]const u8)? {
    return switch (kind) {
        .major => bumpMajor(version, out),
        .minor => bumpMinor(version, out),
        .patch => bumpPatch(version, out),
        .prerelease => bumpPrerelease(version, preid, out),
        .release => stripPrereleaseAndBuild(version, out),
    };
}
