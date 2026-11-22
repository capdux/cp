// Source: Absorbed from /data/dev/bun/src/cli/outdated_command.cp | Synced: 2026-11-22T12:49:17Z
// Ported from Bun: /data/dev/bun/src/cli/outdated_command.cp & /data/dev/bun/docs/pm/cli/outdated.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Package Outdated Analysis & Table Formatter:
// - Parses Semver 2.0.0 versions and classifies Current -> Update -> Latest diffs (UpToDate / Patch / Minor / Major / Prerelease)
// - Supports positive and negated glob dependency filters (e.g. `eslint*`, `@types/*`, `!@types/*`, `*`)
// - Formats dependency type suffixes (`(dev)`, `(peer)`, `(optional)`) and catalog labels (`catalog (<ws>)`, `catalog:<name> (<ws>)`)
// - Deduplicates/groups workspace catalog dependencies and renders aligned Markdown tables with zero heap allocation.

/// 单次过期检查最多记录的条目数
pub const PM_OUTDATED_MAX_ENTRIES: usize = 64;

/// 最多支持的包名通配过滤规则数
pub const PM_OUTDATED_MAX_PATTERNS: usize = 16;

/// 单个 Catalog 分组条目最多记录的附加 Workspace 数
pub const PM_OUTDATED_MAX_GROUPED_WS: usize = 8;

/// 包过期检查错误集
pub err PmOutdatedError {
    InvalidVersion,
    TooManyEntries,
    TooManyPatterns,
    EmptyPackageName,
    BufferTooSmall,
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

fn cmpBytes(a: []const u8, b: []const u8): i8 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) { return -1; }
        if (a[i] > b[i]) { return 1; }
        i += 1;
    }
    if (a.len < b.len) { return -1; }
    if (a.len > b.len) { return 1; }
    return 0;
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): PmOutdatedError!bool {
    if (pos.* + s.len > buf.len) {
        return PmOutdatedError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(buf: []u8, pos: *usize, b: u8): PmOutdatedError!bool {
    if (pos.* + 1 > buf.len) {
        return PmOutdatedError.BufferTooSmall;
    }
    buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn appendRepeat(buf: []u8, pos: *usize, b: u8, count: usize): PmOutdatedError!bool {
    if (pos.* + count > buf.len) {
        return PmOutdatedError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < count) {
        buf[pos.* + i] = b;
        i += 1;
    }
    pos.* += count;
    return true;
}

/// 依赖类型（对应 `dependencies` / `devDependencies` / `peerDependencies` / `optionalDependencies`）
pub enum PmOutdatedDepKind: u8 {
    Normal = 0,
    Dev = 1,
    Peer = 2,
    Optional = 3,

    /// 返回表格中附加在包名后的标准后缀
    fn suffix(): []const u8 {
        return switch (_) {
            .Normal => "",
            .Dev => " (dev)",
            .Peer => " (peer)",
            .Optional => " (optional)",
        };
    }

    fn sortRank(): u8 {
        return switch (_) {
            .Normal => 0,
            .Dev => 1,
            .Peer => 2,
            .Optional => 3,
        };
    }
}

/// 版本升级跨度类别
pub enum PmOutdatedUpdateKind: u8 {
    UpToDate = 0,
    Patch = 1,
    Minor = 2,
    Major = 3,
    Prerelease = 4,

    fn asStr(): []const u8 {
        return switch (_) {
            .UpToDate => "up-to-date",
            .Patch => "patch",
            .Minor => "minor",
            .Major => "major",
            .Prerelease => "prerelease",
        };
    }
}

/// 过滤规则类别
pub enum PmOutdatedFilterKind: u8 {
    All = 0,
    Include = 1,
    Exclude = 2,
}

/// 轻量 Semver 2.0.0 版本号（零堆分配）
pub struct PmOutdatedSemver {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    pre: []const u8 = "",

    /// 解析 `major.minor.patch[-pre][+build]`（允许可选前导 `v`）
    static fn parse(raw: []const u8): PmOutdatedError!PmOutdatedSemver {
        if (raw.len == 0) { return PmOutdatedError.InvalidVersion; }
        let start: usize = 0;
        if (raw[0] == 'v' || raw[0] == 'V') {
            start = 1;
        }
        if (start >= raw.len) { return PmOutdatedError.InvalidVersion; }

        // 剥离 `+build` 元数据
        let end = raw.len;
        let bi: usize = start;
        while (bi < raw.len) {
            if (raw[bi] == '+') {
                end = bi;
                break;
            }
            bi += 1;
        }

        // 分离 `-prerelease`
        let core_end = end;
        let pre_slice: []const u8 = "";
        let pi: usize = start;
        while (pi < end) {
            if (raw[pi] == '-') {
                core_end = pi;
                if (pi + 1 >= end) { return PmOutdatedError.InvalidVersion; }
                pre_slice = raw[(pi + 1)..end];
                break;
            }
            pi += 1;
        }

        // 解析三段数字 major.minor.patch
        let nums: [3]u32 = [0; 3];
        let seg: usize = 0;
        let idx = start;
        while (seg < 3) {
            if (idx >= core_end) { return PmOutdatedError.InvalidVersion; }
            let val: u32 = 0;
            let digits: usize = 0;
            while (idx < core_end && raw[idx] != '.') {
                const c = raw[idx];
                if (c < '0' || c > '9') { return PmOutdatedError.InvalidVersion; }
                const d = @as(u32, c - '0');
                if (val > 429496729) { return PmOutdatedError.InvalidVersion; }
                val = val * 10 + d;
                digits += 1;
                idx += 1;
            }
            if (digits == 0) { return PmOutdatedError.InvalidVersion; }
            nums[seg] = val;
            if (seg < 2) {
                if (idx >= core_end || raw[idx] != '.') { return PmOutdatedError.InvalidVersion; }
                idx += 1;
            }
            seg += 1;
        }
        if (idx != core_end) { return PmOutdatedError.InvalidVersion; }

        return PmOutdatedSemver{
            major: nums[0],
            minor: nums[1],
            patch: nums[2],
            pre: pre_slice,
        };
    }

    /// Semver 2.0.0 比较：返回 -1 (小于), 0 (等于), 1 (大于)
    fn compare(other: PmOutdatedSemver): i8 {
        if (_.major < other.major) { return -1; }
        if (_.major > other.major) { return 1; }
        if (_.minor < other.minor) { return -1; }
        if (_.minor > other.minor) { return 1; }
        if (_.patch < other.patch) { return -1; }
        if (_.patch > other.patch) { return 1; }

        // 无预发布标识 > 有预发布标识
        if (_.pre.len == 0 && other.pre.len == 0) { return 0; }
        if (_.pre.len == 0) { return 1; }
        if (other.pre.len == 0) { return -1; }
        return cmpBytes(_.pre, other.pre);
    }

    /// 计算从当前版本升级到 `target` 的变更级别
    fn classifyUpgradeTo(target: PmOutdatedSemver): PmOutdatedUpdateKind {
        if (_.compare(target) >= 0) {
            return .UpToDate;
        }
        if (target.major > _.major) { return .Major; }
        if (target.minor > _.minor) { return .Minor; }
        if (target.patch > _.patch) { return .Patch; }
        return .Prerelease;
    }
}

/// 迭代式零递归 `*`/`?` 通配符匹配（免疫 ReDoS）
pub fn matchOutdatedGlob(pattern: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = pattern.len;
    let star_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pattern.len && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
            pi += 1;
            ti += 1;
        } else if (pi < pattern.len && pattern[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (star_pi < pattern.len) {
            pi = star_pi + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }
    while (pi < pattern.len && pattern[pi] == '*') {
        pi += 1;
    }
    return pi == pattern.len;
}

/// 单条包名过滤规则
pub struct PmOutdatedFilterRule {
    kind: PmOutdatedFilterKind = .All,
    pattern: []const u8 = "",
}

/// 依赖名称通配过滤器（支持 `eslint*`、`@types/*`、`!@types/*`、`*`）
pub struct PmOutdatedFilter {
    rule_count: usize = 0,
    rules: [PM_OUTDATED_MAX_PATTERNS]PmOutdatedFilterRule = [PmOutdatedFilterRule{}; PM_OUTDATED_MAX_PATTERNS],
    has_positive_rule: bool = false,

    static fn init(): PmOutdatedFilter {
        return PmOutdatedFilter{};
    }

    fn addPattern(*_, raw: []const u8): PmOutdatedError!bool {
        if (raw.len == 0) { return false; }
        if (_.rule_count >= PM_OUTDATED_MAX_PATTERNS) {
            return PmOutdatedError.TooManyPatterns;
        }
        if (eqlBytes(raw, "*") || eqlBytes(raw, "**")) {
            _.rules[_.rule_count] = PmOutdatedFilterRule{ kind: .All, pattern: "*" };
            _.rule_count += 1;
            _.has_positive_rule = true;
            return true;
        }
        if (raw[0] == '!') {
            if (raw.len == 1) { return false; }
            _.rules[_.rule_count] = PmOutdatedFilterRule{
                kind: .Exclude,
                pattern: raw[1..raw.len],
            };
            _.rule_count += 1;
            return true;
        }
        _.rules[_.rule_count] = PmOutdatedFilterRule{
            kind: .Include,
            pattern: raw,
        };
        _.rule_count += 1;
        _.has_positive_rule = true;
        return true;
    }

    /// 判定包名是否通过过滤：若存在正向规则需命中至少一条，且不得命中任何 `!` 排除规则
    fn matches(pkg_name: []const u8): bool {
        if (_.rule_count == 0) { return true; }
        let matched_pos = !_.has_positive_rule;
        let i: usize = 0;
        while (i < _.rule_count) {
            const r = _.rules[i];
            switch (r.kind) {
                .All => {
                    matched_pos = true;
                },
                .Include => {
                    if (matchOutdatedGlob(r.pattern, pkg_name)) {
                        matched_pos = true;
                    }
                },
                .Exclude => {
                    if (matchOutdatedGlob(r.pattern, pkg_name)) {
                        return false;
                    }
                },
            }
            i += 1;
        }
        return matched_pos;
    }
}

/// 单条过期依赖记录
pub struct PmOutdatedEntry {
    package_name: []const u8 = "",
    dep_kind: PmOutdatedDepKind = .Normal,
    current: []const u8 = "",
    update: []const u8 = "",
    latest: []const u8 = "",
    workspace_name: []const u8 = "",
    is_catalog: bool = false,
    /// `is_catalog == true` 时为空或 `"default"` 表示默认 catalog，否则为命名 catalog（如 `"testing"`）
    catalog_name: []const u8 = "",
    grouped_ws_count: usize = 0,
    grouped_workspaces: [PM_OUTDATED_MAX_GROUPED_WS]([]const u8) = [""; PM_OUTDATED_MAX_GROUPED_WS],

    /// 判断该条目是否确实存在可用更新（`update > current` 或 `latest > current`）
    fn isOutdated(): PmOutdatedError!bool {
        const cur = PmOutdatedSemver.parse(_.current)?;
        const upd = PmOutdatedSemver.parse(_.update)?;
        const lat = PmOutdatedSemver.parse(_.latest)?;
        return cur.compare(upd) < 0 || cur.compare(lat) < 0;
    }

    fn updateKind(): PmOutdatedError!PmOutdatedUpdateKind {
        const cur = PmOutdatedSemver.parse(_.current)?;
        const upd = PmOutdatedSemver.parse(_.update)?;
        return cur.classifyUpgradeTo(upd);
    }

    fn latestKind(): PmOutdatedError!PmOutdatedUpdateKind {
        const cur = PmOutdatedSemver.parse(_.current)?;
        const lat = PmOutdatedSemver.parse(_.latest)?;
        return cur.classifyUpgradeTo(lat);
    }

    /// 格式化展示包名（如 `"eslint (dev)"` 或 `"lodash"`）
    fn formatDisplayName(buf: []u8): PmOutdatedError!usize {
        let pos: usize = 0;
        * = appendSlice(buf, &pos, _.package_name)?;
        * = appendSlice(buf, &pos, _.dep_kind.suffix())?;
        return pos;
    }

    /// 格式化 Workspace / Catalog 列（如 `"@test/shared"`、`"catalog (@test/app)"`、`"catalog:testing (@test/app, @test/web)"`）
    fn formatWorkspaceLabel(buf: []u8): PmOutdatedError!usize {
        let pos: usize = 0;
        if (!_.is_catalog) {
            * = appendSlice(buf, &pos, _.workspace_name)?;
            return pos;
        }
        * = appendSlice(buf, &pos, "catalog")?;
        if (_.catalog_name.len > 0 && !eqlBytes(_.catalog_name, "default")) {
            * = appendByte(buf, &pos, ':')?;
            * = appendSlice(buf, &pos, _.catalog_name)?;
        }
        if (_.workspace_name.len > 0 || _.grouped_ws_count > 0) {
            * = appendSlice(buf, &pos, " (")?;
            let wrote = false;
            if (_.workspace_name.len > 0) {
                * = appendSlice(buf, &pos, _.workspace_name)?;
                wrote = true;
            }
            let gi: usize = 0;
            while (gi < _.grouped_ws_count) {
                if (wrote) {
                    * = appendSlice(buf, &pos, ", ")?;
                }
                * = appendSlice(buf, &pos, _.grouped_workspaces[gi])?;
                wrote = true;
                gi += 1;
            }
            * = appendByte(buf, &pos, ')')?;
        }
        return pos;
    }
}

/// 零堆分配过期依赖汇总与对齐表格引擎
pub struct PmOutdatedTable {
    entry_count: usize = 0,
    entries: [PM_OUTDATED_MAX_ENTRIES]PmOutdatedEntry = [PmOutdatedEntry{}; PM_OUTDATED_MAX_ENTRIES],
    up_to_date_skipped: u32 = 0,
    filtered_out_count: u32 = 0,

    static fn init(): PmOutdatedTable {
        return PmOutdatedTable{};
    }

    fn slice(): []const PmOutdatedEntry {
        return _.entries[0.._.entry_count];
    }

    /// 添加候选依赖条目：校验版本、过滤非过期项与不匹配过滤器的项，并合并同 Catalog 跨 Workspace 条目
    fn addEntry(*_, entry: PmOutdatedEntry, filter: PmOutdatedFilter): PmOutdatedError!bool {
        if (entry.package_name.len == 0) {
            return PmOutdatedError.EmptyPackageName;
        }
        const outdated = entry.isOutdated()?;
        if (!outdated) {
            _.up_to_date_skipped += 1;
            return false;
        }
        if (!filter.matches(entry.package_name)) {
            _.filtered_out_count += 1;
            return false;
        }

        // 若为 catalog 依赖，尝试合并到已有同 (package_name, catalog_name, dep_kind) 条目
        if (entry.is_catalog) {
            let i: usize = 0;
            while (i < _.entry_count) {
                if (_.entries[i].is_catalog &&
                    _.entries[i].dep_kind == entry.dep_kind &&
                    eqlBytes(_.entries[i].package_name, entry.package_name) &&
                    eqlBytes(_.entries[i].catalog_name, entry.catalog_name))
                {
                    if (entry.workspace_name.len > 0 &&
                        !eqlBytes(_.entries[i].workspace_name, entry.workspace_name))
                    {
                        let dup = false;
                        let gi: usize = 0;
                        while (gi < _.entries[i].grouped_ws_count) {
                            if (eqlBytes(_.entries[i].grouped_workspaces[gi], entry.workspace_name)) {
                                dup = true;
                                break;
                            }
                            gi += 1;
                        }
                        if (!dup && _.entries[i].grouped_ws_count < PM_OUTDATED_MAX_GROUPED_WS) {
                            const gc = _.entries[i].grouped_ws_count;
                            _.entries[i].grouped_workspaces[gc] = entry.workspace_name;
                            _.entries[i].grouped_ws_count += 1;
                        }
                    }
                    return true;
                }
                i += 1;
            }
        }

        if (_.entry_count >= PM_OUTDATED_MAX_ENTRIES) {
            return PmOutdatedError.TooManyEntries;
        }
        _.entries[_.entry_count] = entry;
        _.entry_count += 1;
        return true;
    }

    /// 按 `(dep_kind, package_name)` 确定性升序排列（普通依赖优先于 dev/peer/optional）
    fn sortDeterministic(*_) {
        let i: usize = 1;
        while (i < _.entry_count) {
            const cur = _.entries[i];
            let j = i;
            while (j > 0) {
                const prev = _.entries[j - 1];
                let before = false;
                if (cur.dep_kind.sortRank() != prev.dep_kind.sortRank()) {
                    before = cur.dep_kind.sortRank() < prev.dep_kind.sortRank();
                } else {
                    before = cmpBytes(cur.package_name, prev.package_name) < 0;
                }
                if (!before) { break; }
                _.entries[j] = prev;
                j -= 1;
            }
            _.entries[j] = cur;
            i += 1;
        }
    }

    /// 渲染对齐的 Markdown 表格（`show_workspace=true` 时附加 `Workspace` 列）
    fn formatMarkdownTable(buf: []u8, show_workspace: bool): PmOutdatedError!usize {
        if (_.entry_count == 0) { return 0; }

        let w_pkg: usize = 7; // "Package"
        let w_cur: usize = 7; // "Current"
        let w_upd: usize = 6; // "Update"
        let w_lat: usize = 6; // "Latest"
        let w_ws: usize = 9;  // "Workspace"

        let tmp: [128]u8 = [0; 128];
        let i: usize = 0;
        while (i < _.entry_count) {
            const e = _.entries[i];
            const dn = e.formatDisplayName(tmp[0..])?;
            if (dn > w_pkg) { w_pkg = dn; }
            if (e.current.len > w_cur) { w_cur = e.current.len; }
            if (e.update.len > w_upd) { w_upd = e.update.len; }
            if (e.latest.len > w_lat) { w_lat = e.latest.len; }
            if (show_workspace) {
                const wn = e.formatWorkspaceLabel(tmp[0..])?;
                if (wn > w_ws) { w_ws = wn; }
            }
            i += 1;
        }

        let pos: usize = 0;
        // 表头行
        * = appendSlice(buf, &pos, "| Package")?;
        * = appendRepeat(buf, &pos, ' ', w_pkg - 7)?;
        * = appendSlice(buf, &pos, " | Current")?;
        * = appendRepeat(buf, &pos, ' ', w_cur - 7)?;
        * = appendSlice(buf, &pos, " | Update")?;
        * = appendRepeat(buf, &pos, ' ', w_upd - 6)?;
        * = appendSlice(buf, &pos, " | Latest")?;
        * = appendRepeat(buf, &pos, ' ', w_lat - 6)?;
        if (show_workspace) {
            * = appendSlice(buf, &pos, " | Workspace")?;
            * = appendRepeat(buf, &pos, ' ', w_ws - 9)?;
        }
        * = appendSlice(buf, &pos, " |\n")?;

        // 分隔行
        * = appendSlice(buf, &pos, "| ")?;
        * = appendRepeat(buf, &pos, '-', w_pkg)?;
        * = appendSlice(buf, &pos, " | ")?;
        * = appendRepeat(buf, &pos, '-', w_cur)?;
        * = appendSlice(buf, &pos, " | ")?;
        * = appendRepeat(buf, &pos, '-', w_upd)?;
        * = appendSlice(buf, &pos, " | ")?;
        * = appendRepeat(buf, &pos, '-', w_lat)?;
        if (show_workspace) {
            * = appendSlice(buf, &pos, " | ")?;
            * = appendRepeat(buf, &pos, '-', w_ws)?;
        }
        * = appendSlice(buf, &pos, " |\n")?;

        // 数据行
        let r: usize = 0;
        while (r < _.entry_count) {
            const e = _.entries[r];
            const dn = e.formatDisplayName(tmp[0..])?;
            * = appendSlice(buf, &pos, "| ")?;
            * = appendSlice(buf, &pos, tmp[0..dn])?;
            * = appendRepeat(buf, &pos, ' ', w_pkg - dn)?;

            * = appendSlice(buf, &pos, " | ")?;
            * = appendSlice(buf, &pos, e.current)?;
            * = appendRepeat(buf, &pos, ' ', w_cur - e.current.len)?;

            * = appendSlice(buf, &pos, " | ")?;
            * = appendSlice(buf, &pos, e.update)?;
            * = appendRepeat(buf, &pos, ' ', w_upd - e.update.len)?;

            * = appendSlice(buf, &pos, " | ")?;
            * = appendSlice(buf, &pos, e.latest)?;
            * = appendRepeat(buf, &pos, ' ', w_lat - e.latest.len)?;

            if (show_workspace) {
                const wn = e.formatWorkspaceLabel(tmp[0..])?;
                * = appendSlice(buf, &pos, " | ")?;
                * = appendSlice(buf, &pos, tmp[0..wn])?;
                * = appendRepeat(buf, &pos, ' ', w_ws - wn)?;
            }
            * = appendSlice(buf, &pos, " |\n")?;
            r += 1;
        }

        return pos;
    }
}
