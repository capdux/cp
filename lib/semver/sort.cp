// Source: Absorbed from /data/dev/bun/src/install/semver.zig | Synced: 2026-11-21T16:39:55Z (SemVer 2.0.0 Comparator & In-Place Sorter)

const U64_MAX: u64 = 0xFFFF_FFFF_FFFF_FFFF;

/// SemVer 2.0.0 优先级比较结果
export enum SemverOrder {
    less,
    equal,
    greater,
}

/// 零拷贝解析后的 SemVer 2.0.0 版本结构
export struct ParsedSemver {
    major: u64;
    minor: u64;
    patch: u64;
    prerelease: []const u8;
    build: []const u8;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentChar(c: u8): bool {
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '-';
}

fn isAllDigits(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    for (c of s) {
        if (!isDigit(c)) {
            return false;
        }
    }
    return true;
}

fn parseNumericPart(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    if (s.len > 1 && s[0] == '0') {
        return null;
    }
    let val: u64 = 0;
    for (c of s) {
        if (!isDigit(c)) {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (val > (U64_MAX - digit) / 10) {
            return null;
        }
        val = val * 10 + digit;
    }
    return val;
}

fn validateDotSeparated(s: []const u8, forbid_numeric_leading_zero: bool): bool {
    if (s.len == 0) {
        return false;
    }
    let start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        if (i == s.len || s[i] == '.') {
            const seg = s[start..i];
            if (seg.len == 0) {
                return false;
            }
            for (c of seg) {
                if (!isIdentChar(c)) {
                    return false;
                }
            }
            if (forbid_numeric_leading_zero && isAllDigits(seg) && seg.len > 1 && seg[0] == '0') {
                return false;
            }
            start = i + 1;
        }
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 严格按 SemVer 2.0.0 规范解析版本字符串（拒绝前导零、空标识符与非法字符）
export fn parseSemverStrict(s: []const u8): ParsedSemver? {
    if (s.len == 0) {
        return null;
    }

    // 1. 拆分可选 +build 元数据
    let core_and_pre = s;
    let build: []const u8 = "";
    if (plus := findByte(s, '+')) {
        core_and_pre = s[0..plus];
        build = s[plus + 1..];
        if (!validateDotSeparated(build, false)) {
            return null;
        }
    }

    // 2. 拆分可选 -prerelease
    let core = core_and_pre;
    let prerelease: []const u8 = "";
    if (dash := findByte(core_and_pre, '-')) {
        core = core_and_pre[0..dash];
        prerelease = core_and_pre[dash + 1..];
        if (!validateDotSeparated(prerelease, true)) {
            return null;
        }
    }

    // 3. 解析 major.minor.patch
    const dot1 = findByte(core, '.') ?? return null;
    const after1 = core[dot1 + 1..];
    const dot2_rel = findByte(after1, '.') ?? return null;
    const major_str = core[0..dot1];
    const minor_str = after1[0..dot2_rel];
    const patch_str = after1[dot2_rel + 1..];

    const major = parseNumericPart(major_str) ?? return null;
    const minor = parseNumericPart(minor_str) ?? return null;
    const patch = parseNumericPart(patch_str) ?? return null;

    return ParsedSemver{
        major: major,
        minor: minor,
        patch: patch,
        prerelease: prerelease,
        build: build,
    };
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/// 宽松解析 SemVer（允许首尾空白与可选单个 `v`/`V` 前缀，核心仍遵守 SemVer 2.0.0）
export fn parseSemverLoose(s: []const u8): ParsedSemver? {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return null;
    }
    let trimmed = s[start..end];
    if (trimmed[0] == 'v' || trimmed[0] == 'V') {
        trimmed = trimmed[1..];
    }
    return parseSemverStrict(trimmed);
}

fn compareU64(a: u64, b: u64): SemverOrder {
    if (a < b) return SemverOrder.less;
    if (a > b) return SemverOrder.greater;
    return SemverOrder.equal;
}

fn compareAsciiLex(a: []const u8, b: []const u8): SemverOrder {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return SemverOrder.less;
        if (a[i] > b[i]) return SemverOrder.greater;
        i += 1;
    }
    if (a.len < b.len) return SemverOrder.less;
    if (a.len > b.len) return SemverOrder.greater;
    return SemverOrder.equal;
}

fn comparePrereleaseIdent(a: []const u8, b: []const u8): SemverOrder {
    const a_num = isAllDigits(a);
    const b_num = isAllDigits(b);

    if (a_num && b_num) {
        // 合法纯数字标识符无前导零：先比较位数，等长再逐字节比较（任意精度无溢出）
        if (a.len < b.len) return SemverOrder.less;
        if (a.len > b.len) return SemverOrder.greater;
        return compareAsciiLex(a, b);
    }
    if (a_num && !b_num) {
        return SemverOrder.less;
    }
    if (!a_num && b_num) {
        return SemverOrder.greater;
    }
    return compareAsciiLex(a, b);
}

fn comparePrerelease(a: []const u8, b: []const u8): SemverOrder {
    if (a.len == 0 && b.len == 0) return SemverOrder.equal;
    if (a.len == 0) return SemverOrder.greater;
    if (b.len == 0) return SemverOrder.less;

    let ia: usize = 0;
    let ib: usize = 0;

    while (ia < a.len && ib < b.len) {
        const start_a = ia;
        while (ia < a.len && a[ia] != '.') {
            ia += 1;
        }
        const seg_a = a[start_a..ia];
        if (ia < a.len && a[ia] == '.') {
            ia += 1;
        }

        const start_b = ib;
        while (ib < b.len && b[ib] != '.') {
            ib += 1;
        }
        const seg_b = b[start_b..ib];
        if (ib < b.len && b[ib] == '.') {
            ib += 1;
        }

        const ord = comparePrereleaseIdent(seg_a, seg_b);
        if (ord != SemverOrder.equal) {
            return ord;
        }
    }

    if (ia < a.len) return SemverOrder.greater;
    if (ib < b.len) return SemverOrder.less;
    return SemverOrder.equal;
}

/// 按 SemVer 2.0.0 §11 比较两个已解析版本（忽略 +build 元数据）
export fn compareSemver(a: ParsedSemver, b: ParsedSemver): SemverOrder {
    const c_maj = compareU64(a.major, b.major);
    if (c_maj != SemverOrder.equal) return c_maj;

    const c_min = compareU64(a.minor, b.minor);
    if (c_min != SemverOrder.equal) return c_min;

    const c_pat = compareU64(a.patch, b.patch);
    if (c_pat != SemverOrder.equal) return c_pat;

    return comparePrerelease(a.prerelease, b.prerelease);
}

/// 解析并比较两个 SemVer 2.0.0 字符串；任一非法返回 null
export fn compareSemverStr(a: []const u8, b: []const u8): SemverOrder? {
    const pa = parseSemverStrict(a) ?? return null;
    const pb = parseSemverStrict(b) ?? return null;
    return compareSemver(pa, pb);
}

fn sortSemverInPlace(versions: [][]const u8, descending: bool): bool {
    // 预校验全部版本，避免非法输入造成半排序破坏
    for (v of versions) {
        if (parseSemverStrict(v) == null) {
            return false;
        }
    }

    let i: usize = 1;
    while (i < versions.len) {
        const key_str = versions[i];
        const key_parsed = parseSemverStrict(key_str) ?? return false;
        let j: usize = i;
        while (j > 0) {
            const prev_parsed = parseSemverStrict(versions[j - 1]) ?? return false;
            const ord = compareSemver(prev_parsed, key_parsed);
            const should_shift = if (descending) (ord == SemverOrder.less) else (ord == SemverOrder.greater);
            if (!should_shift) {
                break;
            }
            versions[j] = versions[j - 1];
            j -= 1;
        }
        versions[j] = key_str;
        i += 1;
    }
    return true;
}

/// 原地稳定升序排列 SemVer 字符串切片；遇非法版本返回 false
export fn sortSemverAsc(versions: [][]const u8): bool {
    return sortSemverInPlace(versions, false);
}

/// 原地稳定降序排列 SemVer 字符串切片；遇非法版本返回 false
export fn sortSemverDesc(versions: [][]const u8): bool {
    return sortSemverInPlace(versions, true);
}

/// 查找最大版本下标；空切片或包含非法版本返回 null
export fn maxSemverIndex(versions: []const []const u8): usize? {
    if (versions.len == 0) {
        return null;
    }
    let best_idx: usize = 0;
    let best = parseSemverStrict(versions[0]) ?? return null;
    let i: usize = 1;
    while (i < versions.len) {
        const cur = parseSemverStrict(versions[i]) ?? return null;
        if (compareSemver(cur, best) == SemverOrder.greater) {
            best = cur;
            best_idx = i;
        }
        i += 1;
    }
    return best_idx;
}

/// 查找最小版本下标；空切片或包含非法版本返回 null
export fn minSemverIndex(versions: []const []const u8): usize? {
    if (versions.len == 0) {
        return null;
    }
    let best_idx: usize = 0;
    let best = parseSemverStrict(versions[0]) ?? return null;
    let i: usize = 1;
    while (i < versions.len) {
        const cur = parseSemverStrict(versions[i]) ?? return null;
        if (compareSemver(cur, best) == SemverOrder.less) {
            best = cur;
            best_idx = i;
        }
        i += 1;
    }
    return best_idx;
}
