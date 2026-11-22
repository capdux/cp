// Source: Absorbed from /data/dev/bun/src/semver/SemverRange.cp | Synced: 2026-11-22T13:13:14Z

pub const MAX_COMPARATORS_PER_GROUP: usize = 16;
pub const MAX_GROUPS: usize = 16;

pub err Error {
    EmptyInput,
    InvalidVersion,
    InvalidRange,
    InvalidComparator,
    TooManyGroups,
    TooManyComparators,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentChar(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-';
}

fn trimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn parseStrictU64(s: string): u64! {
    if (s.len == 0) {
        return err.InvalidVersion;
    }
    if (s.len > 1 && s[0] == '0') {
        return err.InvalidVersion;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return err.InvalidVersion;
        }
        const d = (s[i] - '0') as u64;
        if (val > (std.math.maxInt(u64) - d) / 10) {
            return err.InvalidVersion;
        }
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

fn validatePreOrBuild(s: string, check_numeric_leading_zero: bool): bool {
    if (s.len == 0) {
        return false;
    }
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        if (i == s.len || s[i] == '.') {
            if (i == seg_start) {
                return false;
            }
            const seg = s[seg_start..i];
            let all_digits = true;
            let k: usize = 0;
            while (k < seg.len) {
                if (!isIdentChar(seg[k])) {
                    return false;
                }
                if (!isDigit(seg[k])) {
                    all_digits = false;
                }
                k += 1;
            }
            if (check_numeric_leading_zero && all_digits && seg.len > 1 && seg[0] == '0') {
                return false;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return true;
}

fn isAllDigits(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn comparePrerelease(a: string, b: string): std.math.Order {
    let ai: usize = 0;
    let bi: usize = 0;

    while (ai < a.len && bi < b.len) {
        let a_end = ai;
        while (a_end < a.len && a[a_end] != '.') { a_end += 1; }
        let b_end = bi;
        while (b_end < b.len && b[b_end] != '.') { b_end += 1; }

        const sa = a[ai..a_end];
        const sb = b[bi..b_end];

        const a_num = isAllDigits(sa);
        const b_num = isAllDigits(sb);

        if (a_num && b_num) {
            const va = parseStrictU64(sa) catch 0;
            const vb = parseStrictU64(sb) catch 0;
            if (va < vb) { return .lt; }
            if (va > vb) { return .gt; }
        } else if (a_num && !b_num) {
            return .lt;
        } else if (!a_num && b_num) {
            return .gt;
        } else {
            const ord = std.mem.order(u8, sa, sb);
            if (ord != .eq) {
                return ord;
            }
        }

        ai = if (a_end < a.len) (a_end + 1) else a.len;
        bi = if (b_end < b.len) (b_end + 1) else b.len;
    }

    if (ai < a.len) { return .gt; }
    if (bi < b.len) { return .lt; }
    return .eq;
}

/// Semver 2.0 版本号（零堆分配，prerelease/build 直接切片引用源字符串）
pub struct SemverVersion {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    prerelease: string = "",
    build: string = "",

    /// 解析标准 Semver 2.0 版本号（支持可选前导 'v'/'V'）
    static fn parse(input: string): SemverVersion! {
        let s = trimSpaces(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }
        if (s[0] == 'v' || s[0] == 'V') {
            s = s[1..];
        }

        // 剥离 +build
        let build_str: string = "";
        let plus_idx: usize? = null;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '+') {
                plus_idx = i;
                break;
            }
            i += 1;
        }
        if (p := plus_idx) {
            build_str = s[p + 1 ..];
            if (!validatePreOrBuild(build_str, false)) {
                return err.InvalidVersion;
            }
            s = s[0..p];
        }

        // 剥离 -prerelease
        let pre_str: string = "";
        let dash_idx: usize? = null;
        let j: usize = 0;
        while (j < s.len) {
            if (s[j] == '-') {
                dash_idx = j;
                break;
            }
            j += 1;
        }
        if (d := dash_idx) {
            pre_str = s[d + 1 ..];
            if (!validatePreOrBuild(pre_str, true)) {
                return err.InvalidVersion;
            }
            s = s[0..d];
        }

        // 解析 major.minor.patch
        let dot1: usize? = null;
        let dot2: usize? = null;
        let k: usize = 0;
        while (k < s.len) {
            if (s[k] == '.') {
                if (dot1 == null) {
                    dot1 = k;
                } else if (dot2 == null) {
                    dot2 = k;
                } else {
                    return err.InvalidVersion;
                }
            }
            k += 1;
        }

        const p1 = dot1 ?? return err.InvalidVersion;
        const p2 = dot2 ?? return err.InvalidVersion;

        const maj = parseStrictU64(s[0..p1])?;
        const min = parseStrictU64(s[p1 + 1 .. p2])?;
        const pat = parseStrictU64(s[p2 + 1 ..])?;

        return SemverVersion{
            major: maj,
            minor: min,
            patch: pat,
            prerelease: pre_str,
            build: build_str,
        };
    }

    fn hasPrerelease(): bool {
        return _.prerelease.len > 0;
    }

    fn hasBuild(): bool {
        return _.build.len > 0;
    }

    /// Semver 2.0 优先级全序比较（忽略 build 元数据）
    fn compare(other: SemverVersion): std.math.Order {
        if (_.major < other.major) { return .lt; }
        if (_.major > other.major) { return .gt; }
        if (_.minor < other.minor) { return .lt; }
        if (_.minor > other.minor) { return .gt; }
        if (_.patch < other.patch) { return .lt; }
        if (_.patch > other.patch) { return .gt; }

        const a_pre = _.hasPrerelease();
        const b_pre = other.hasPrerelease();
        if (!a_pre && !b_pre) { return .eq; }
        if (!a_pre && b_pre) { return .gt; }
        if (a_pre && !b_pre) { return .lt; }
        return comparePrerelease(_.prerelease, other.prerelease);
    }

    fn eql(other: SemverVersion): bool {
        return _.compare(other) == .eq;
    }

    fn lt(other: SemverVersion): bool {
        return _.compare(other) == .lt;
    }

    fn lte(other: SemverVersion): bool {
        const o = _.compare(other);
        return o == .lt || o == .eq;
    }

    fn gt(other: SemverVersion): bool {
        return _.compare(other) == .gt;
    }

    fn gte(other: SemverVersion): bool {
        const o = _.compare(other);
        return o == .gt || o == .eq;
    }
}

/// 比较操作符枚举
pub enum Op: u8 {
    wildcard = 0,
    eql = 1,
    lt = 2,
    lte = 3,
    gt = 4,
    gte = 5,
}

/// 单个 Semver 比较器
pub struct Comparator {
    op: Op = Op.wildcard,
    version: SemverVersion = SemverVersion{},

    fn matches(v: SemverVersion): bool {
        return switch (_.op) {
            .wildcard => true,
            .eql => v.eql(_.version),
            .lt => v.lt(_.version),
            .lte => v.lte(_.version),
            .gt => v.gt(_.version),
            .gte => v.gte(_.version),
        };
    }
}

struct PartialVersion {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    has_major: bool = false,
    has_minor: bool = false,
    has_patch: bool = false,
    prerelease: string = "",
}

fn isWildcardSeg(seg: string): bool {
    return std.mem.eql(u8, seg, "*") || std.mem.eql(u8, seg, "x") || std.mem.eql(u8, seg, "X");
}

fn parsePartialVersion(input: string): PartialVersion! {
    let s = trimSpaces(input);
    if (s.len == 0) {
        return err.InvalidVersion;
    }
    if (s[0] == 'v' || s[0] == 'V') {
        s = s[1..];
    }
    if (s.len == 0) {
        return err.InvalidVersion;
    }

    // 剥离 +build
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '+') {
            if (!validatePreOrBuild(s[i + 1 ..], false)) {
                return err.InvalidVersion;
            }
            s = s[0..i];
            break;
        }
        i += 1;
    }

    // 剥离 -prerelease
    let pre_str: string = "";
    let j: usize = 0;
    while (j < s.len) {
        if (s[j] == '-') {
            pre_str = s[j + 1 ..];
            if (!validatePreOrBuild(pre_str, true)) {
                return err.InvalidVersion;
            }
            s = s[0..j];
            break;
        }
        j += 1;
    }

    let segs: [3]string = ["", "", ""];
    let seg_count: usize = 0;
    let start: usize = 0;
    let k: usize = 0;
    while (k <= s.len) {
        if (k == s.len || s[k] == '.') {
            if (seg_count >= 3 || k == start) {
                return err.InvalidVersion;
            }
            segs[seg_count] = s[start..k];
            seg_count += 1;
            start = k + 1;
        }
        k += 1;
    }

    let pv = PartialVersion{ prerelease: pre_str };
    if (seg_count >= 1 && !isWildcardSeg(segs[0])) {
        pv.major = parseStrictU64(segs[0])?;
        pv.has_major = true;
    } else if (seg_count >= 1) {
        // major 为通配符，后续不能是具体数字
        return pv;
    }

    if (seg_count >= 2 && !isWildcardSeg(segs[1])) {
        pv.minor = parseStrictU64(segs[1])?;
        pv.has_minor = true;
    } else if (seg_count >= 2) {
        return pv;
    }

    if (seg_count == 3 && !isWildcardSeg(segs[2])) {
        pv.patch = parseStrictU64(segs[2])?;
        pv.has_patch = true;
    }

    return pv;
}

/// 单个 `&&` 逻辑与比较器组
pub struct ComparatorGroup {
    items: [MAX_COMPARATORS_PER_GROUP]Comparator = [Comparator{}; MAX_COMPARATORS_PER_GROUP],
    len: usize = 0,

    fn push(*_, c: Comparator)! {
        if (_.len >= MAX_COMPARATORS_PER_GROUP) {
            return err.TooManyComparators;
        }
        _.items[_.len] = c;
        _.len += 1;
    }

    fn matches(v: SemverVersion): bool {
        if (_.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.items[i].matches(v)) {
                return false;
            }
            i += 1;
        }

        // NPM Semver Prerelease 保护：prerelease 版本仅匹配显式声明相同 (major, minor, patch) prerelease 的分组
        if (v.hasPrerelease()) {
            let allowed = false;
            i = 0;
            while (i < _.len) {
                const cv = _.items[i].version;
                if (cv.hasPrerelease() && cv.major == v.major && cv.minor == v.minor && cv.patch == v.patch) {
                    allowed = true;
                    break;
                }
                i += 1;
            }
            if (!allowed) {
                return false;
            }
        }
        return true;
    }
}

fn addWildcardOrExact(group: *ComparatorGroup, pv: PartialVersion)! {
    if (!pv.has_major) {
        group.push(Comparator{ op: .gte, version: SemverVersion{} })?;
        return;
    }
    if (!pv.has_minor) {
        group.push(Comparator{ op: .gte, version: SemverVersion{ major: pv.major } })?;
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major +| 1 } })?;
        return;
    }
    if (!pv.has_patch) {
        group.push(Comparator{ op: .gte, version: SemverVersion{ major: pv.major, minor: pv.minor } })?;
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major, minor: pv.minor +| 1 } })?;
        return;
    }
    group.push(Comparator{
        op: .eql,
        version: SemverVersion{ major: pv.major, minor: pv.minor, patch: pv.patch, prerelease: pv.prerelease },
    })?;
}

fn addCaret(group: *ComparatorGroup, pv: PartialVersion)! {
    if (!pv.has_major) {
        group.push(Comparator{ op: .gte, version: SemverVersion{} })?;
        return;
    }
    const lower = SemverVersion{
        major: pv.major,
        minor: pv.minor,
        patch: pv.patch,
        prerelease: pv.prerelease,
    };
    group.push(Comparator{ op: .gte, version: lower })?;

    if (pv.major > 0) {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major +| 1 } })?;
    } else if (!pv.has_minor) {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: 1 } })?;
    } else if (pv.minor > 0) {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: 0, minor: pv.minor +| 1 } })?;
    } else if (!pv.has_patch) {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: 0, minor: 1 } })?;
    } else {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: 0, minor: 0, patch: pv.patch +| 1 } })?;
    }
}

fn addTilde(group: *ComparatorGroup, pv: PartialVersion)! {
    if (!pv.has_major) {
        group.push(Comparator{ op: .gte, version: SemverVersion{} })?;
        return;
    }
    const lower = SemverVersion{
        major: pv.major,
        minor: pv.minor,
        patch: pv.patch,
        prerelease: pv.prerelease,
    };
    group.push(Comparator{ op: .gte, version: lower })?;

    if (!pv.has_minor) {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major +| 1 } })?;
    } else {
        group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major, minor: pv.minor +| 1 } })?;
    }
}

fn parseSingleGroup(raw_group: string): ComparatorGroup! {
    const s = trimSpaces(raw_group);
    if (s.len == 0) {
        return err.InvalidRange;
    }

    let group = ComparatorGroup{};

    // 检测连字符范围 "A - B"（必须两侧有空白，避免与 prerelease 的 "-" 混淆）
    let hyphen_pos: usize? = null;
    let i: usize = 0;
    while (i + 2 < s.len) {
        if (isSpace(s[i]) && s[i + 1] == '-' && isSpace(s[i + 2])) {
            hyphen_pos = i;
            break;
        }
        i += 1;
    }

    if (hp := hyphen_pos) {
        const left_str = trimSpaces(s[0..hp]);
        const right_str = trimSpaces(s[hp + 3 ..]);
        if (left_str.len == 0 || right_str.len == 0) {
            return err.InvalidRange;
        }
        const lhs = parsePartialVersion(left_str)?;
        const rhs = parsePartialVersion(right_str)?;

        group.push(Comparator{
            op: .gte,
            version: SemverVersion{
                major: lhs.major,
                minor: lhs.minor,
                patch: lhs.patch,
                prerelease: lhs.prerelease,
            },
        })?;

        if (!rhs.has_major) {
            // 右侧为 *，无上界
        } else if (!rhs.has_minor) {
            group.push(Comparator{ op: .lt, version: SemverVersion{ major: rhs.major +| 1 } })?;
        } else if (!rhs.has_patch) {
            group.push(Comparator{ op: .lt, version: SemverVersion{ major: rhs.major, minor: rhs.minor +| 1 } })?;
        } else {
            group.push(Comparator{
                op: .lte,
                version: SemverVersion{
                    major: rhs.major,
                    minor: rhs.minor,
                    patch: rhs.patch,
                    prerelease: rhs.prerelease,
                },
            })?;
        }
        return group;
    }

    // 空格分隔的多个比较器项（同时支持操作符与版本间带空格，如 ">= 1.0.0 < 2.0.0"）
    let pos: usize = 0;
    while (pos < s.len) {
        while (pos < s.len && isSpace(s[pos])) { pos += 1; }
        if (pos >= s.len) { break; }

        let op_kind: u8 = 0; // 0=none, 1=eql, 2=lt, 3=lte, 4=gt, 5=gte, 6=caret, 7=tilde
        if (pos + 1 < s.len && s[pos] == '>' && s[pos + 1] == '=') {
            op_kind = 5; pos += 2;
        } else if (pos + 1 < s.len && s[pos] == '<' && s[pos + 1] == '=') {
            op_kind = 3; pos += 2;
        } else if (pos + 1 < s.len && s[pos] == '=' && s[pos + 1] == '=') {
            op_kind = 1; pos += 2;
        } else if (s[pos] == '>') {
            op_kind = 4; pos += 1;
        } else if (s[pos] == '<') {
            op_kind = 2; pos += 1;
        } else if (s[pos] == '=') {
            op_kind = 1; pos += 1;
        } else if (s[pos] == '^') {
            op_kind = 6; pos += 1;
        } else if (s[pos] == '~') {
            op_kind = 7; pos += 1;
        }

        while (pos < s.len && isSpace(s[pos])) { pos += 1; }
        if (pos >= s.len) {
            return err.InvalidComparator;
        }

        const tok_start = pos;
        while (pos < s.len && !isSpace(s[pos])) { pos += 1; }
        const ver_tok = s[tok_start..pos];
        const pv = parsePartialVersion(ver_tok)?;

        if (op_kind == 0 || op_kind == 1) {
            addWildcardOrExact(*group, pv)?;
        } else if (op_kind == 6) {
            addCaret(*group, pv)?;
        } else if (op_kind == 7) {
            addTilde(*group, pv)?;
        } else if (op_kind == 5) {
            group.push(Comparator{
                op: .gte,
                version: SemverVersion{ major: pv.major, minor: pv.minor, patch: pv.patch, prerelease: pv.prerelease },
            })?;
        } else if (op_kind == 4) {
            if (!pv.has_major) {
                return err.InvalidComparator;
            } else if (!pv.has_minor) {
                group.push(Comparator{ op: .gte, version: SemverVersion{ major: pv.major +| 1 } })?;
            } else if (!pv.has_patch) {
                group.push(Comparator{ op: .gte, version: SemverVersion{ major: pv.major, minor: pv.minor +| 1 } })?;
            } else {
                group.push(Comparator{
                    op: .gt,
                    version: SemverVersion{ major: pv.major, minor: pv.minor, patch: pv.patch, prerelease: pv.prerelease },
                })?;
            }
        } else if (op_kind == 3) {
            if (!pv.has_major) {
                group.push(Comparator{ op: .gte, version: SemverVersion{} })?;
            } else if (!pv.has_minor) {
                group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major +| 1 } })?;
            } else if (!pv.has_patch) {
                group.push(Comparator{ op: .lt, version: SemverVersion{ major: pv.major, minor: pv.minor +| 1 } })?;
            } else {
                group.push(Comparator{
                    op: .lte,
                    version: SemverVersion{ major: pv.major, minor: pv.minor, patch: pv.patch, prerelease: pv.prerelease },
                })?;
            }
        } else if (op_kind == 2) {
            if (!pv.has_major) {
                group.push(Comparator{ op: .lt, version: SemverVersion{} })?;
            } else {
                group.push(Comparator{
                    op: .lt,
                    version: SemverVersion{ major: pv.major, minor: pv.minor, patch: pv.patch, prerelease: pv.prerelease },
                })?;
            }
        }
    }

    if (group.len == 0) {
        return err.InvalidRange;
    }
    return group;
}

/// 完整 NPM / Semver 2.0 范围匹配引擎（零堆分配，支持 `||` 多组逻辑或）
pub struct SemverRange {
    groups: [MAX_GROUPS]ComparatorGroup = [ComparatorGroup{}; MAX_GROUPS],
    len: usize = 0,

    /// 解析 NPM Semver 范围字符串（支持 `^`/`~`/`>=`/`<=`/`>`/`<`/`=`、连字符、通配符与 `||`）
    static fn parse(input: string): SemverRange! {
        const s = trimSpaces(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }

        let range = SemverRange{};
        let seg_start: usize = 0;
        let i: usize = 0;

        while (i <= s.len) {
            const is_or = i + 1 < s.len && s[i] == '|' && s[i + 1] == '|';
            if (i == s.len || is_or) {
                const seg = s[seg_start..i];
                const g = parseSingleGroup(seg)?;
                if (range.len >= MAX_GROUPS) {
                    return err.TooManyGroups;
                }
                range.groups[range.len] = g;
                range.len += 1;
                if (is_or) {
                    i += 2;
                    seg_start = i;
                    continue;
                }
                break;
            }
            i += 1;
        }

        return range;
    }

    fn groupCount(): usize {
        return _.len;
    }

    /// 判断版本号是否满足当前范围
    fn satisfies(v: SemverVersion): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.groups[i].matches(v)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 便捷字符串版本匹配（非法版本号返回 false）
    fn satisfiesStr(ver_str: string): bool {
        const v = SemverVersion.parse(ver_str) catch return false;
        return _.satisfies(v);
    }

    /// 从候选列表中选出满足范围的最大版本号
    fn maxSatisfying(versions: []const SemverVersion): SemverVersion? {
        let best: SemverVersion? = null;
        let i: usize = 0;
        while (i < versions.len) {
            const v = versions[i];
            if (_.satisfies(v)) {
                if (cur := best) {
                    if (v.gt(cur)) {
                        best = v;
                    }
                } else {
                    best = v;
                }
            }
            i += 1;
        }
        return best;
    }

    /// 从候选列表中选出满足范围的最小版本号
    fn minSatisfying(versions: []const SemverVersion): SemverVersion? {
        let best: SemverVersion? = null;
        let i: usize = 0;
        while (i < versions.len) {
            const v = versions[i];
            if (_.satisfies(v)) {
                if (cur := best) {
                    if (v.lt(cur)) {
                        best = v;
                    }
                } else {
                    best = v;
                }
            }
            i += 1;
        }
        return best;
    }
}
