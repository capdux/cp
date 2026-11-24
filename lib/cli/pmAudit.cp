// Source: Absorbed from /data/dev/bun/src/cli/audit_command.cp | Synced: 2026-11-23T20:18:24Z
// Ported from Bun: /data/dev/bun/src/cli/audit_command.cp & /data/dev/bun/docs/pm/cli/audit.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Package Security Audit Engine:
// - Parses & compares severity levels (low / moderate / high / critical) with --audit-level threshold filtering
// - Supports --ignore CVE/GHSA advisory ID & advisory URL substring suppression
// - Filters production-only (--prod) and default-registry packages and serializes NPM bulk advisory JSON payloads
// - Aggregates vulnerability counts, computes standard CLI exit codes (0 = clean, 1 = findings), and formats reports
// - Traces cycle-safe dependency chains from root packages to affected transitive dependencies.

/// 单次审计最多支持的 `--ignore` CVE/GHSA 忽略条目数
pub const PM_AUDIT_MAX_IGNORE: usize = 32;

/// 单次审计请求最多包含的独立包名数量
pub const PM_AUDIT_MAX_PACKAGES: usize = 64;

/// 单个包名最多去重记录的版本数量
pub const PM_AUDIT_MAX_VERSIONS_PER_PKG: usize = 8;

/// 单份审计报告最多容纳的漏洞条目数量
pub const PM_AUDIT_MAX_ADVISORIES: usize = 64;

/// 依赖图最多记录的有向依赖边数量
pub const PM_AUDIT_MAX_DEP_EDGES: usize = 128;

/// 依赖路径追溯最大深度（防环路与栈溢出）
pub const PM_AUDIT_MAX_PATH_DEPTH: usize = 16;

/// 包安全审计错误集
pub err PmAuditError {
    InvalidSeverity,
    TooManyIgnores,
    TooManyPackages,
    TooManyVersions,
    TooManyAdvisories,
    TooManyDepEdges,
    EmptyPackageName,
    EmptyVersion,
    BufferTooSmall,
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) { return false; }
        i += 1;
    }
    return true;
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

fn containsBytes(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) { return true; }
    if (needle.len > haystack.len) { return false; }
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let matched = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) { return true; }
        i += 1;
    }
    return false;
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): PmAuditError!bool {
    if (pos.* + s.len > buf.len) {
        return PmAuditError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(buf: []u8, pos: *usize, b: u8): PmAuditError!bool {
    if (pos.* + 1 > buf.len) {
        return PmAuditError.BufferTooSmall;
    }
    buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn appendU32(buf: []u8, pos: *usize, val: u32): PmAuditError!bool {
    let tmp: [12]u8 = [0; 12];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0 && n < 12) {
            tmp[n] = @as(u8, @intCast(v % 10)) + '0';
            v = v / 10;
            n += 1;
        }
    }
    if (pos.* + n > buf.len) {
        return PmAuditError.BufferTooSmall;
    }
    let i: usize = n;
    while (i > 0) {
        i -= 1;
        buf[pos.*] = tmp[i];
        pos.* += 1;
    }
    return true;
}

fn appendJsonString(buf: []u8, pos: *usize, s: []const u8): PmAuditError!bool {
    const HEX = "0123456789abcdef";
    * = appendByte(buf, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            * = appendSlice(buf, pos, "\\\"")?;
        } else if (c == '\\') {
            * = appendSlice(buf, pos, "\\\\")?;
        } else if (c == '\n') {
            * = appendSlice(buf, pos, "\\n")?;
        } else if (c == '\r') {
            * = appendSlice(buf, pos, "\\r")?;
        } else if (c == '\t') {
            * = appendSlice(buf, pos, "\\t")?;
        } else if (c < 0x20) {
            * = appendSlice(buf, pos, "\\u00")?;
            * = appendByte(buf, pos, HEX[@as(usize, (c >> 4) & 0xF)])?;
            * = appendByte(buf, pos, HEX[@as(usize, c & 0xF)])?;
        } else {
            * = appendByte(buf, pos, c)?;
        }
        i += 1;
    }
    * = appendByte(buf, pos, '"')?;
    return true;
}

/// 漏洞严重性等级（对应 Bun `--audit-level=<low|moderate|high|critical>`）
pub enum PmAuditSeverity: u8 {
    Low = 0,
    Moderate = 1,
    High = 2,
    Critical = 3,

    /// 严格大小写无关解析严重性字符串
    static fn parse(raw: []const u8): PmAuditError!PmAuditSeverity {
        if (eqlIgnoreCaseAscii(raw, "low")) { return .Low; }
        if (eqlIgnoreCaseAscii(raw, "moderate")) { return .Moderate; }
        if (eqlIgnoreCaseAscii(raw, "high")) { return .High; }
        if (eqlIgnoreCaseAscii(raw, "critical")) { return .Critical; }
        return PmAuditError.InvalidSeverity;
    }

    /// 宽容解析严重性字符串（对齐 Bun audit_command.cp：未知级别默认归类为 Moderate）
    static fn parseOrDefault(raw: []const u8): PmAuditSeverity {
        if (eqlIgnoreCaseAscii(raw, "low")) { return .Low; }
        if (eqlIgnoreCaseAscii(raw, "moderate")) { return .Moderate; }
        if (eqlIgnoreCaseAscii(raw, "high")) { return .High; }
        if (eqlIgnoreCaseAscii(raw, "critical")) { return .Critical; }
        return .Moderate;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Low => "low",
            .Moderate => "moderate",
            .High => "high",
            .Critical => "critical",
        };
    }

    fn rank(): u8 {
        return switch (_) {
            .Low => 0,
            .Moderate => 1,
            .High => 2,
            .Critical => 3,
        };
    }

    /// 当前严重性是否达到或超过 `--audit-level` 阈值
    fn meetsThreshold(min_level: PmAuditSeverity): bool {
        return _.rank() >= min_level.rank();
    }
}

/// 审计过滤策略（`--audit-level` / `--prod` / `--ignore` / `--json`）
pub struct PmAuditFilter {
    min_level: PmAuditSeverity? = null,
    prod_only: bool = false,
    json_output: bool = false,
    ignore_count: usize = 0,
    ignore_ids: [PM_AUDIT_MAX_IGNORE]([]const u8) = [""; PM_AUDIT_MAX_IGNORE],

    static fn init(): PmAuditFilter {
        return PmAuditFilter{};
    }

    fn addIgnore(*_, cve_or_ghsa: []const u8): PmAuditError!bool {
        if (cve_or_ghsa.len == 0) { return false; }
        let i: usize = 0;
        while (i < _.ignore_count) {
            if (eqlBytes(_.ignore_ids[i], cve_or_ghsa)) {
                return false;
            }
            i += 1;
        }
        if (_.ignore_count >= PM_AUDIT_MAX_IGNORE) {
            return PmAuditError.TooManyIgnores;
        }
        _.ignore_ids[_.ignore_count] = cve_or_ghsa;
        _.ignore_count += 1;
        return true;
    }

    /// 判定某条漏洞是否应纳入审计报告（精确对齐 Bun `audit_command.cp:752-771`）
    fn shouldInclude(severity: PmAuditSeverity, advisory_id: []const u8, advisory_url: []const u8): bool {
        if (lvl := _.min_level) {
            if (!severity.meetsThreshold(lvl)) {
                return false;
            }
        }
        let i: usize = 0;
        while (i < _.ignore_count) {
            const ignored = _.ignore_ids[i];
            if (eqlBytes(advisory_id, ignored) || containsBytes(advisory_url, ignored)) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

/// 待审计包的去重版本列表条目
pub struct PmAuditPackageEntry {
    name: []const u8 = "",
    version_count: usize = 0,
    versions: [PM_AUDIT_MAX_VERSIONS_PER_PKG]([]const u8) = [""; PM_AUDIT_MAX_VERSIONS_PER_PKG],

    fn versionsSlice(): []const []const u8 {
        return _.versions[0.._.version_count];
    }
}

/// NPM Bulk Advisory 审计请求体构建器（自动过滤非默认源与 `--prod` 范围）
pub struct PmAuditRequestBuilder {
    prod_only: bool = false,
    package_count: usize = 0,
    packages: [PM_AUDIT_MAX_PACKAGES]PmAuditPackageEntry = [PmAuditPackageEntry{}; PM_AUDIT_MAX_PACKAGES],
    skipped_non_default_count: u32 = 0,
    skipped_dev_count: u32 = 0,

    static fn init(prod_only: bool): PmAuditRequestBuilder {
        return PmAuditRequestBuilder{ prod_only: prod_only };
    }

    /// 添加待审计包版本；若因 `--prod` 或非默认源跳过则返回 `false`
    fn addPackage(
        *_,
        name: []const u8,
        version: []const u8,
        is_prod: bool,
        is_default_registry: bool,
    ): PmAuditError!bool {
        if (name.len == 0) { return PmAuditError.EmptyPackageName; }
        if (version.len == 0) { return PmAuditError.EmptyVersion; }
        if (_.prod_only && !is_prod) {
            _.skipped_dev_count += 1;
            return false;
        }
        if (!is_default_registry) {
            _.skipped_non_default_count += 1;
            return false;
        }

        let pkg_idx: usize = _.package_count;
        let i: usize = 0;
        while (i < _.package_count) {
            if (eqlBytes(_.packages[i].name, name)) {
                pkg_idx = i;
                break;
            }
            i += 1;
        }
        if (pkg_idx == _.package_count) {
            if (_.package_count >= PM_AUDIT_MAX_PACKAGES) {
                return PmAuditError.TooManyPackages;
            }
            _.packages[pkg_idx] = PmAuditPackageEntry{ name: name };
            _.package_count += 1;
        }

        // 版本去重
        let vi: usize = 0;
        while (vi < _.packages[pkg_idx].version_count) {
            if (eqlBytes(_.packages[pkg_idx].versions[vi], version)) {
                return true;
            }
            vi += 1;
        }
        if (_.packages[pkg_idx].version_count >= PM_AUDIT_MAX_VERSIONS_PER_PKG) {
            return PmAuditError.TooManyVersions;
        }
        const vc = _.packages[pkg_idx].version_count;
        _.packages[pkg_idx].versions[vc] = version;
        _.packages[pkg_idx].version_count += 1;
        return true;
    }

    /// 序列化为 NPM `/-/npm/v1/security/advisories/bulk` 标准 JSON 请求体
    fn formatBulkAdvisoryJson(buf: []u8): PmAuditError!usize {
        let pos: usize = 0;
        * = appendByte(buf, &pos, '{')?;
        let i: usize = 0;
        while (i < _.package_count) {
            if (i > 0) {
                * = appendByte(buf, &pos, ',')?;
            }
            * = appendJsonString(buf, &pos, _.packages[i].name)?;
            * = appendSlice(buf, &pos, ":[")?;
            let vi: usize = 0;
            while (vi < _.packages[i].version_count) {
                if (vi > 0) {
                    * = appendByte(buf, &pos, ',')?;
                }
                * = appendJsonString(buf, &pos, _.packages[i].versions[vi])?;
                vi += 1;
            }
            * = appendByte(buf, &pos, ']')?;
            i += 1;
        }
        * = appendByte(buf, &pos, '}')?;
        return pos;
    }
}

/// 单条安全公告记录（对应 Bun `VulnerabilityInfo`）
pub struct PmAuditAdvisory {
    package_name: []const u8 = "",
    id: []const u8 = "",
    severity: PmAuditSeverity = .Moderate,
    title: []const u8 = "",
    url: []const u8 = "",
    vulnerable_versions: []const u8 = "",
}

/// 分级漏洞计数摘要（对应 Bun `VulnCounts`）
pub struct PmAuditCounts {
    low: u32 = 0,
    moderate: u32 = 0,
    high: u32 = 0,
    critical: u32 = 0,

    fn total(): u32 {
        return _.low + _.moderate + _.high + _.critical;
    }

    fn add(*_, sev: PmAuditSeverity) {
        switch (sev) {
            .Low => { _.low += 1; },
            .Moderate => { _.moderate += 1; },
            .High => { _.high += 1; },
            .Critical => { _.critical += 1; },
        }
    }

    /// CLI 退出码：0 表示无漏洞，1 表示发现任意漏洞（与 `bun audit` 一致）
    fn exitCode(): u8 {
        return if (_.total() == 0) 0 else 1;
    }
}

/// 安全审计聚合报告与格式化引擎
pub struct PmAuditReport {
    advisory_count: usize = 0,
    advisories: [PM_AUDIT_MAX_ADVISORIES]PmAuditAdvisory = [PmAuditAdvisory{}; PM_AUDIT_MAX_ADVISORIES],
    counts: PmAuditCounts = PmAuditCounts{},
    filtered_out_count: u32 = 0,

    static fn init(): PmAuditReport {
        return PmAuditReport{};
    }

    fn advisoriesSlice(): []const PmAuditAdvisory {
        return _.advisories[0.._.advisory_count];
    }

    /// 录入一条公告并按 `filter` 过滤；纳入报告返回 `true`，被过滤返回 `false`
    fn addAdvisory(*_, adv: PmAuditAdvisory, filter: PmAuditFilter): PmAuditError!bool {
        if (!filter.shouldInclude(adv.severity, adv.id, adv.url)) {
            _.filtered_out_count += 1;
            return false;
        }
        if (_.advisory_count >= PM_AUDIT_MAX_ADVISORIES) {
            return PmAuditError.TooManyAdvisories;
        }
        _.advisories[_.advisory_count] = adv;
        _.advisory_count += 1;
        _.counts.add(adv.severity);
        return true;
    }

    fn exitCode(): u8 {
        return _.counts.exitCode();
    }

    /// 格式化单行摘要（例如 `"3 vulnerabilities (1 high, 2 moderate)"` 或 `"No vulnerabilities found"`）
    fn formatSummaryLine(buf: []u8): PmAuditError!usize {
        let pos: usize = 0;
        const tot = _.counts.total();
        if (tot == 0) {
            * = appendSlice(buf, &pos, "No vulnerabilities found")?;
            return pos;
        }

        * = appendU32(buf, &pos, tot)?;
        if (tot == 1) {
            * = appendSlice(buf, &pos, " vulnerability (")?;
        } else {
            * = appendSlice(buf, &pos, " vulnerabilities (")?;
        }

        let need_comma = false;
        if (_.counts.critical > 0) {
            * = appendU32(buf, &pos, _.counts.critical)?;
            * = appendSlice(buf, &pos, " critical")?;
            need_comma = true;
        }
        if (_.counts.high > 0) {
            if (need_comma) { * = appendSlice(buf, &pos, ", ")?; }
            * = appendU32(buf, &pos, _.counts.high)?;
            * = appendSlice(buf, &pos, " high")?;
            need_comma = true;
        }
        if (_.counts.moderate > 0) {
            if (need_comma) { * = appendSlice(buf, &pos, ", ")?; }
            * = appendU32(buf, &pos, _.counts.moderate)?;
            * = appendSlice(buf, &pos, " moderate")?;
            need_comma = true;
        }
        if (_.counts.low > 0) {
            if (need_comma) { * = appendSlice(buf, &pos, ", ")?; }
            * = appendU32(buf, &pos, _.counts.low)?;
            * = appendSlice(buf, &pos, " low")?;
        }
        * = appendByte(buf, &pos, ')')?;
        return pos;
    }

    /// 格式化完整 CLI 审计报告（包含明细、汇总行与升级建议）
    fn formatReport(buf: []u8): PmAuditError!usize {
        let pos: usize = 0;
        if (_.counts.total() == 0) {
            * = appendSlice(buf, &pos, "No vulnerabilities found\n")?;
            return pos;
        }

        let i: usize = 0;
        while (i < _.advisory_count) {
            const adv = _.advisories[i];
            * = appendSlice(buf, &pos, adv.package_name)?;
            if (adv.vulnerable_versions.len > 0) {
                * = appendSlice(buf, &pos, "  ")?;
                * = appendSlice(buf, &pos, adv.vulnerable_versions)?;
            }
            * = appendSlice(buf, &pos, "\n  ")?;
            * = appendSlice(buf, &pos, adv.severity.asStr())?;
            * = appendSlice(buf, &pos, ": ")?;
            * = appendSlice(buf, &pos, adv.title)?;
            if (adv.url.len > 0) {
                * = appendSlice(buf, &pos, " - ")?;
                * = appendSlice(buf, &pos, adv.url)?;
            }
            * = appendByte(buf, &pos, '\n')?;
            i += 1;
        }

        * = appendByte(buf, &pos, '\n')?;
        let summary_tmp: [128]u8 = [0; 128];
        const s_len = _.formatSummaryLine(summary_tmp[0..])?;
        * = appendSlice(buf, &pos, summary_tmp[0..s_len])?;
        * = appendSlice(
            buf,
            &pos,
            "\nTo update all dependencies to the latest compatible versions:\n  cap update\nTo update all dependencies to the latest versions (including breaking changes):\n  cap update --latest\n",
        )?;
        return pos;
    }
}

/// 依赖边（`parent` 依赖 `child`）
pub struct PmAuditDepEdge {
    parent: []const u8 = "",
    child: []const u8 = "",
}

/// 零堆分配依赖链追溯图（无环反向查找受漏洞影响的依赖路径）
pub struct PmAuditDepGraph {
    edge_count: usize = 0,
    edges: [PM_AUDIT_MAX_DEP_EDGES]PmAuditDepEdge = [PmAuditDepEdge{}; PM_AUDIT_MAX_DEP_EDGES],

    static fn init(): PmAuditDepGraph {
        return PmAuditDepGraph{};
    }

    fn addEdge(*_, parent: []const u8, child: []const u8): PmAuditError!bool {
        if (parent.len == 0 || child.len == 0) {
            return PmAuditError.EmptyPackageName;
        }
        if (_.edge_count >= PM_AUDIT_MAX_DEP_EDGES) {
            return PmAuditError.TooManyDepEdges;
        }
        _.edges[_.edge_count] = PmAuditDepEdge{ parent: parent, child: child };
        _.edge_count += 1;
        return true;
    }

    /// 反向追溯从 `target_pkg` 到顶层依赖方的一条无环路径（返回写入 `out_chain` 的节点数，含 `target_pkg`）
    fn findDependentChain(target_pkg: []const u8, out_chain: [][]const u8): usize {
        if (target_pkg.len == 0 || out_chain.len == 0) { return 0; }
        let len: usize = 0;
        let current = target_pkg;
        while (len < out_chain.len && len < PM_AUDIT_MAX_PATH_DEPTH) {
            out_chain[len] = current;
            len += 1;

            // 查找首个未出现在当前路径中的父依赖方（防止环路死循环）
            let found_parent = false;
            let next_parent: []const u8 = "";
            let ei: usize = 0;
            while (ei < _.edge_count) {
                if (eqlBytes(_.edges[ei].child, current)) {
                    const cand = _.edges[ei].parent;
                    let seen = false;
                    let si: usize = 0;
                    while (si < len) {
                        if (eqlBytes(out_chain[si], cand)) {
                            seen = true;
                            break;
                        }
                        si += 1;
                    }
                    if (!seen) {
                        next_parent = cand;
                        found_parent = true;
                        break;
                    }
                }
                ei += 1;
            }
            if (found_parent) {
                current = next_parent;
            } else {
                break;
            }
        }
        return len;
    }
}
