// Source: Absorbed from /data/dev/bun/src/install/versioned_url.cp | Synced: 2026-11-22T12:12:00Z
// Pure-CP zero-allocation Bun lockfile/registry `VersionedURL` (v2 u32 -> v3 u64 migration,
// strict Semver 2.0 parser/comparator, tarball URL validator, and fixed-capacity candidate
// version resolver set).

/// 单个 `VersionedUrlSet` 支持的最大候选版本条目数
pub const VERSIONED_URL_MAX_ENTRIES: usize = 64;

/// `VersionedUrl` 解析、校验与集合操作错误集
pub err VersionedUrlError {
    InvalidVersion,
    InvalidUrl,
    Overflow,
    TooManyEntries,
    BufferTooSmall,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) {
        return false;
    }
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (toLowerAscii(s[off + i]) != toLowerAscii(suffix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isNumericIdent(s: []const u8): bool {
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

fn parseStrictU64(s: []const u8): u64! {
    if (s.len == 0) {
        return err.InvalidVersion;
    }
    if (s.len > 1 && s[0] == '0') {
        return err.InvalidVersion;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return err.InvalidVersion;
        }
        const digit: u64 = @as(u64, c - '0');
        if (val > (18446744073709551615 - digit) / 10) {
            return err.Overflow;
        }
        val = val * 10 + digit;
        i += 1;
    }
    return val;
}

fn cmpLex(a: []const u8, b: []const u8): i8 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn comparePrerelease(a: []const u8, b: []const u8): i8 {
    if (a.len == 0 && b.len == 0) return 0;
    if (a.len == 0) return 1;
    if (b.len == 0) return -1;

    let ia: usize = 0;
    let ib: usize = 0;
    while (ia < a.len && ib < b.len) {
        let ea = ia;
        while (ea < a.len && a[ea] != '.') {
            ea += 1;
        }
        let eb = ib;
        while (eb < b.len && b[eb] != '.') {
            eb += 1;
        }
        const sa = a[ia..ea];
        const sb = b[ib..eb];
        const na = isNumericIdent(sa);
        const nb = isNumericIdent(sb);
        if (na && nb) {
            const va = parseStrictU64(sa) catch 0;
            const vb = parseStrictU64(sb) catch 0;
            if (va < vb) return -1;
            if (va > vb) return 1;
        } else if (na && !nb) {
            // Semver 2.0: 数字标识符优先级低于非数字标识符
            return -1;
        } else if (!na && nb) {
            return 1;
        } else {
            const c = cmpLex(sa, sb);
            if (c != 0) return c;
        }
        ia = if (ea < a.len) ea + 1 else a.len;
        ib = if (eb < b.len) eb + 1 else b.len;
    }
    if (ia < a.len) return 1;
    if (ib < b.len) return -1;
    return 0;
}

fn writeByte(out: []u8, pos: *usize, b: u8): usize! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
    return 1;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

fn writeU64Decimal(out: []u8, pos: *usize, val: u64): usize! {
    let tmp: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        return writeByte(out, pos, '0');
    }
    while (v > 0) {
        tmp[len] = @as(u8, @intCast(v % 10)) + '0';
        v = v / 10;
        len += 1;
    }
    if (len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out[pos.* + i] = tmp[len - 1 - i];
        i += 1;
    }
    pos.* += len;
    return len;
}

fn isValidPackageUrl(url: []const u8): bool {
    if (url.len < 8) {
        return false;
    }
    if (!startsWithIgnoreCase(url, "https://") &&
        !startsWithIgnoreCase(url, "http://") &&
        !startsWithIgnoreCase(url, "file://"))
    {
        return false;
    }
    let i: usize = 0;
    while (i < url.len) {
        if (url[i] <= 0x20 || url[i] == 0x7f) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零堆分配 Semver 2.0 版本三元组（64 位无符号段，对齐 Bun `Semver.VersionType(u64)`）
pub struct PkgSemverTriple {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",

    static fn parse(raw: []const u8): PkgSemverTriple! {
        let s = raw;
        if (s.len > 0 && (s[0] == 'v' || s[0] == 'V')) {
            s = s[1..];
        }
        if (s.len == 0) {
            return err.InvalidVersion;
        }
        // 剥离 +build
        let core_and_pre = s;
        let build_str: []const u8 = "";
        let plus_pos: usize? = null;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '+') {
                plus_pos = i;
                break;
            }
            i += 1;
        }
        if (p := plus_pos) {
            core_and_pre = s[0..p];
            build_str = s[p + 1 ..];
            if (build_str.len == 0) {
                return err.InvalidVersion;
            }
        }
        // 剥离 -prerelease
        let core = core_and_pre;
        let pre_str: []const u8 = "";
        let dash_pos: usize? = null;
        let j: usize = 0;
        while (j < core_and_pre.len) {
            if (core_and_pre[j] == '-') {
                dash_pos = j;
                break;
            }
            j += 1;
        }
        if (d := dash_pos) {
            core = core_and_pre[0..d];
            pre_str = core_and_pre[d + 1 ..];
            if (pre_str.len == 0) {
                return err.InvalidVersion;
            }
        }

        // 解析 major.minor.patch
        let dot1: usize? = null;
        let dot2: usize? = null;
        let k: usize = 0;
        while (k < core.len) {
            if (core[k] == '.') {
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
        const d1 = dot1 ?? return err.InvalidVersion;
        const d2 = dot2 ?? return err.InvalidVersion;

        const maj = parseStrictU64(core[0..d1])?;
        const min = parseStrictU64(core[d1 + 1 .. d2])?;
        const pat = parseStrictU64(core[d2 + 1 ..])?;
        return PkgSemverTriple{
            major: maj,
            minor: min,
            patch: pat,
            prerelease: pre_str,
            build: build_str,
        };
    }

    fn isPrerelease(): bool {
        return _.prerelease.len > 0;
    }

    /// Semver 2.0 优先级比较（返回 `-1` / `0` / `1`，忽略 `+build` 元数据）
    fn compare(other: PkgSemverTriple): i8 {
        if (_.major < other.major) return -1;
        if (_.major > other.major) return 1;
        if (_.minor < other.minor) return -1;
        if (_.minor > other.minor) return 1;
        if (_.patch < other.patch) return -1;
        if (_.patch > other.patch) return 1;
        return comparePrerelease(_.prerelease, other.prerelease);
    }

    fn eql(other: PkgSemverTriple): bool {
        return _.compare(other) == 0;
    }

    fn formatInto(out: []u8): usize! {
        let pos: usize = 0;
        * = writeU64Decimal(out, *pos, _.major)?;
        * = writeByte(out, *pos, '.')?;
        * = writeU64Decimal(out, *pos, _.minor)?;
        * = writeByte(out, *pos, '.')?;
        * = writeU64Decimal(out, *pos, _.patch)?;
        if (_.prerelease.len > 0) {
            * = writeByte(out, *pos, '-')?;
            * = writeSlice(out, *pos, _.prerelease)?;
        }
        if (_.build.len > 0) {
            * = writeByte(out, *pos, '+')?;
            * = writeSlice(out, *pos, _.build)?;
        }
        return pos;
    }
}

/// 旧版 lockfile v2 的 32 位版本 URL 结构（对齐 Bun `OldV2VersionedURL`）
pub struct LegacyV2VersionedUrl {
    url: []const u8 = "",
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",

    /// 无损迁移至 64 位 `VersionedUrl`（对齐 Bun `VersionedURLType(u32).migrate()`）
    fn migrate(): VersionedUrl! {
        if (!isValidPackageUrl(_.url)) {
            return err.InvalidUrl;
        }
        return VersionedUrl{
            url: _.url,
            version: PkgSemverTriple{
                major: @as(u64, _.major),
                minor: @as(u64, _.minor),
                patch: @as(u64, _.patch),
                prerelease: _.prerelease,
                build: _.build,
            },
        };
    }
}

/// 包管理器版本化 URL 绑定结构（对齐 Bun `VersionedURL`）
pub struct VersionedUrl {
    url: []const u8 = "",
    version: PkgSemverTriple = PkgSemverTriple{},

    static fn init(url: []const u8, version: PkgSemverTriple): VersionedUrl! {
        if (!isValidPackageUrl(url)) {
            return err.InvalidUrl;
        }
        return VersionedUrl{
            url: url,
            version: version,
        };
    }

    static fn parse(url: []const u8, version_str: []const u8): VersionedUrl! {
        if (!isValidPackageUrl(url)) {
            return err.InvalidUrl;
        }
        const ver = PkgSemverTriple.parse(version_str)?;
        return VersionedUrl{
            url: url,
            version: ver,
        };
    }

    /// 对齐 Bun `VersionedURL.eql()`：按 Semver 版本等价判定
    fn eql(other: VersionedUrl): bool {
        return _.version.eql(other.version);
    }

    /// 严格等价判定：同时校验 Semver 版本与完整 URL 一致
    fn eqlStrict(other: VersionedUrl): bool {
        return _.version.eql(other.version) && bytesEql(_.url, other.url);
    }

    /// 对齐 Bun `VersionedURL.order()`：按 Semver 2.0 优先级比较 (`-1` / `0` / `1`)
    fn order(other: VersionedUrl): i8 {
        return _.version.compare(other.version);
    }

    fn isHttps(): bool {
        return startsWithIgnoreCase(_.url, "https://");
    }

    fn isTarball(): bool {
        return endsWithIgnoreCase(_.url, ".tgz") || endsWithIgnoreCase(_.url, ".tar.gz");
    }

    /// 格式化为锁文件/诊断条目：`<pkg_name>@<version> -> <url>`
    fn formatLockEntry(pkg_name: []const u8, out: []u8): usize! {
        let pos: usize = 0;
        * = writeSlice(out, *pos, pkg_name)?;
        * = writeByte(out, *pos, '@')?;
        const vn = _.version.formatInto(out[pos..])?;
        pos += vn;
        * = writeSlice(out, *pos, " -> ")?;
        * = writeSlice(out, *pos, _.url)?;
        return pos;
    }
}

/// 零堆分配候选版本化 URL 集合（支持去重、Semver 原地排序与最高稳定/最新候选选择）
pub struct VersionedUrlSet {
    items: [VERSIONED_URL_MAX_ENTRIES]VersionedUrl = [VersionedUrl{}; VERSIONED_URL_MAX_ENTRIES],
    len: usize = 0,

    static fn init(): VersionedUrlSet {
        return VersionedUrlSet{};
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): VersionedUrl? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }

    /// 插入条目（若已存在相同 Semver 版本则覆盖 URL 而不增加计数）
    fn push(*_, entry: VersionedUrl): usize! {
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].eql(entry)) {
                _.items[i] = entry;
                return _.len;
            }
            i += 1;
        }
        if (_.len >= VERSIONED_URL_MAX_ENTRIES) {
            return err.TooManyEntries;
        }
        _.items[_.len] = entry;
        _.len += 1;
        return _.len;
    }

    /// 按 Semver 升序原地插入排序
    fn sortAsc(*_) {
        let i: usize = 1;
        while (i < _.len) {
            const key = _.items[i];
            let j = i;
            while (j > 0 && _.items[j - 1].order(key) > 0) {
                _.items[j] = _.items[j - 1];
                j -= 1;
            }
            _.items[j] = key;
            i += 1;
        }
    }

    /// 按 Semver 降序原地插入排序
    fn sortDesc(*_) {
        let i: usize = 1;
        while (i < _.len) {
            const key = _.items[i];
            let j = i;
            while (j > 0 && _.items[j - 1].order(key) < 0) {
                _.items[j] = _.items[j - 1];
                j -= 1;
            }
            _.items[j] = key;
            i += 1;
        }
    }

    /// 返回最高稳定版本（自动跳过预发布版本）；若无稳定版返回 `null`
    fn latestStable(): VersionedUrl? {
        let best: VersionedUrl? = null;
        let i: usize = 0;
        while (i < _.len) {
            const cur = _.items[i];
            if (!cur.version.isPrerelease()) {
                if (b := best) {
                    if (cur.order(b) > 0) {
                        best = cur;
                    }
                } else {
                    best = cur;
                }
            }
            i += 1;
        }
        return best;
    }

    /// 返回最高版本（包含预发布版本）
    fn latestAny(): VersionedUrl? {
        let best: VersionedUrl? = null;
        let i: usize = 0;
        while (i < _.len) {
            const cur = _.items[i];
            if (b := best) {
                if (cur.order(b) > 0) {
                    best = cur;
                }
            } else {
                best = cur;
            }
            i += 1;
        }
        return best;
    }

    fn findByVersion(ver: PkgSemverTriple): VersionedUrl? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i].version.eql(ver)) {
                return _.items[i];
            }
            i += 1;
        }
        return null;
    }
}
