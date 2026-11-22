// cap/test/test_cli_pm_audit_production_deep.cp
// 深度生产级回归测试：cap:cli 包安全漏洞审计引擎 (lib/cli/pm_audit.cp)

import { test } from "cap:test";
import {
    PM_AUDIT_MAX_IGNORE,
    PM_AUDIT_MAX_VERSIONS_PER_PKG,
    PmAuditSeverity,
    PmAuditFilter,
    PmAuditRequestBuilder,
    PmAuditAdvisory,
    PmAuditReport,
    PmAuditDepGraph,
} from "cap:cli";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) { return true; }
    if (needle.len > haystack.len) { return false; }
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return true; }
        i += 1;
    }
    return false;
}

test("pm_audit: severity parsing, round-trip and threshold ordering") {
    const s_low = PmAuditSeverity.parse("LOW")?;
    const s_mod = PmAuditSeverity.parse("Moderate")?;
    const s_hi = PmAuditSeverity.parse("high")?;
    const s_crit = PmAuditSeverity.parse("CrItIcAl")?;

    if (!bytesEq(s_low.asStr(), "low") || !bytesEq(s_mod.asStr(), "moderate") ||
        !bytesEq(s_hi.asStr(), "high") || !bytesEq(s_crit.asStr(), "critical"))
    {
        return err.SeverityRoundTripMismatch;
    }

    // 严格模式拒绝未知严重度
    let rejected = false;
    * = PmAuditSeverity.parse("unknown") catch {
        rejected = true;
        return;
    };
    * = rejected;
}

test("pm_audit: severity strict rejection and parseOrDefault fallback") {
    let invalid_caught = false;
    const fallback = PmAuditSeverity.parse("info") catch blk: {
        invalid_caught = true;
        break :blk PmAuditSeverity.Low;
    };
    * = fallback;
    if (!invalid_caught) { return err.InvalidSeverityNotRejected; }

    // 宽容解析回退为 Moderate（对齐 Bun audit_command.cp）
    if (PmAuditSeverity.parseOrDefault("info") != PmAuditSeverity.Moderate) {
        return err.DefaultFallbackMismatch;
    }
    if (PmAuditSeverity.parseOrDefault("") != PmAuditSeverity.Moderate) {
        return err.EmptyFallbackMismatch;
    }

    // 阈值矩阵校验
    if (!PmAuditSeverity.Critical.meetsThreshold(PmAuditSeverity.High)) { return err.ThresholdCritHigh; }
    if (!PmAuditSeverity.High.meetsThreshold(PmAuditSeverity.High)) { return err.ThresholdHighHigh; }
    if (PmAuditSeverity.Moderate.meetsThreshold(PmAuditSeverity.High)) { return err.ThresholdModHigh; }
    if (PmAuditSeverity.Low.meetsThreshold(PmAuditSeverity.Moderate)) { return err.ThresholdLowMod; }
}

test("pm_audit: filter audit-level threshold and CVE/URL ignore suppression") {
    let f = PmAuditFilter.init();
    f.min_level = PmAuditSeverity.Moderate;
    * = f.addIgnore("CVE-2022-25883")?;
    * = f.addIgnore("GHSA-aaaa-bbbb-cccc")?;
    // 重复添加忽略项应幂等返回 false
    if (f.addIgnore("CVE-2022-25883")?) { return err.DuplicateIgnoreShouldBeFalse; }

    // 低危被阈值过滤
    if (f.shouldInclude(PmAuditSeverity.Low, "CVE-2024-0001", "https://example.com/1")) {
        return err.LowShouldBeFiltered;
    }
    // 精确 ID 匹配被忽略
    if (f.shouldInclude(PmAuditSeverity.High, "CVE-2022-25883", "https://example.com/2")) {
        return err.ExactIdShouldBeIgnored;
    }
    // URL 子串命中 GHSA 编号被忽略
    if (f.shouldInclude(PmAuditSeverity.Critical, "1098765", "https://github.com/advisories/GHSA-aaaa-bbbb-cccc")) {
        return err.UrlSubstringShouldBeIgnored;
    }
    // 未命中的高危保留
    if (!f.shouldInclude(PmAuditSeverity.High, "CVE-2024-9999", "https://github.com/advisories/GHSA-1111-2222-3333")) {
        return err.ValidHighShouldBeIncluded;
    }
}

test("pm_audit: request builder filters prod/registry, deduplicates versions and escapes JSON") {
    let req = PmAuditRequestBuilder.init(true);
    // 生产包 + 默认源（含重复版本与多版本）
    if (!req.addPackage("lodash", "4.17.20", true, true)?) { return err.ProdDefaultShouldInclude; }
    if (!req.addPackage("lodash", "4.17.20", true, true)?) { return err.DupVersionShouldReturnTrue; }
    if (!req.addPackage("lodash", "4.17.19", true, true)?) { return err.SecondVersionShouldInclude; }
    // 开发依赖在 prod_only=true 下跳过
    if (req.addPackage("vitest", "1.0.0", false, true)?) { return err.DevDepShouldSkip; }
    // 私有 Registry 包跳过
    if (req.addPackage("@corp/internal", "2.0.0", true, false)?) { return err.PrivateRegistryShouldSkip; }
    // 含特殊引号字符的包名用于验证 RFC 8259 转义
    if (!req.addPackage("weird\"pkg\\x", "1.0.0", true, true)?) { return err.EscapedPkgShouldInclude; }

    if (req.skipped_dev_count != 1 || req.skipped_non_default_count != 1 || req.package_count != 2) {
        return err.SkipCountsMismatch;
    }

    let buf: [256]u8 = [0; 256];
    const n = req.formatBulkAdvisoryJson(buf[0..])?;
    const expected = "{\"lodash\":[\"4.17.20\",\"4.17.19\"],\"weird\\\"pkg\\\\x\":[\"1.0.0\"]}";
    if (!bytesEq(buf[0..n], expected)) {
        return err.BulkJsonPayloadMismatch;
    }
}

test("pm_audit: clean report summary and exit code 0") {
    let report = PmAuditReport.init();
    let filter = PmAuditFilter.init();
    filter.min_level = PmAuditSeverity.High;

    // 仅有一条被过滤掉的中危记录
    const kept = report.addAdvisory(PmAuditAdvisory{
        package_name: "minimist",
        id: "CVE-2021-44906",
        severity: PmAuditSeverity.Moderate,
        title: "Prototype Pollution",
        url: "https://example.com/advisory",
        vulnerable_versions: "<1.2.6",
    }, filter)?;
    if (kept || report.filtered_out_count != 1) { return err.FilteredAdvisoryMismatch; }
    if (report.exitCode() != 0) { return err.CleanExitCodeShouldBeZero; }

    let sbuf: [64]u8 = [0; 64];
    const sn = report.formatSummaryLine(sbuf[0..])?;
    if (!bytesEq(sbuf[0..sn], "No vulnerabilities found")) {
        return err.CleanSummaryMismatch;
    }

    let rbuf: [128]u8 = [0; 128];
    const rn = report.formatReport(rbuf[0..])?;
    if (!bytesEq(rbuf[0..rn], "No vulnerabilities found\n")) {
        return err.CleanReportMismatch;
    }
}

test("pm_audit: multi-severity aggregation, full report formatting and exit code 1") {
    let report = PmAuditReport.init();
    const filter = PmAuditFilter.init();

    * = report.addAdvisory(PmAuditAdvisory{
        package_name: "semver",
        id: "CVE-2022-25883",
        severity: PmAuditSeverity.High,
        title: "Regular Expression Denial of Service",
        url: "https://github.com/advisories/GHSA-c2qf-rxjj-qqgw",
        vulnerable_versions: ">=7.0.0 <7.5.2",
    }, filter)?;
    * = report.addAdvisory(PmAuditAdvisory{
        package_name: "qs",
        id: "GHSA-hrpp-h998-j3pp",
        severity: PmAuditSeverity.Moderate,
        title: "Prototype Pollution",
        url: "https://github.com/advisories/GHSA-hrpp-h998-j3pp",
        vulnerable_versions: "<6.10.3",
    }, filter)?;
    * = report.addAdvisory(PmAuditAdvisory{
        package_name: "axios",
        id: "GHSA-wf5p-g6vw-rhxx",
        severity: PmAuditSeverity.Moderate,
        title: "CSRF",
        url: "https://github.com/advisories/GHSA-wf5p-g6vw-rhxx",
        vulnerable_versions: "<1.6.0",
    }, filter)?;

    if (report.exitCode() != 1 || report.counts.total() != 3 ||
        report.counts.high != 1 || report.counts.moderate != 2)
    {
        return err.AggregationCountMismatch;
    }

    let sbuf: [128]u8 = [0; 128];
    const sn = report.formatSummaryLine(sbuf[0..])?;
    if (!bytesEq(sbuf[0..sn], "3 vulnerabilities (1 high, 2 moderate)")) {
        return err.MultiSummaryMismatch;
    }

    let rbuf: [1024]u8 = [0; 1024];
    const rn = report.formatReport(rbuf[0..])?;
    const full = rbuf[0..rn];
    if (!containsStr(full, "semver  >=7.0.0 <7.5.2") ||
        !containsStr(full, "high: Regular Expression Denial of Service") ||
        !containsStr(full, "3 vulnerabilities (1 high, 2 moderate)") ||
        !containsStr(full, "cap update --latest"))
    {
        return err.FullReportContentMismatch;
    }
}

test("pm_audit: dependency chain tracing handles transitive paths and cycles safely") {
    let g = PmAuditDepGraph.init();
    * = g.addEdge("my-app", "express")?;
    * = g.addEdge("express", "body-parser")?;
    * = g.addEdge("body-parser", "qs")?;
    // 故意制造循环边 qs -> express 验证防死循环
    * = g.addEdge("qs", "express")?;

    let chain: [8]([]const u8) = [""; 8];
    const depth = g.findDependentChain("qs", chain[0..]);
    if (depth != 4) { return err.ChainDepthMismatch; }
    if (!bytesEq(chain[0], "qs") || !bytesEq(chain[1], "body-parser") ||
        !bytesEq(chain[2], "express") || !bytesEq(chain[3], "my-app"))
    {
        return err.ChainOrderMismatch;
    }

    // 纯环路 A <-> B 也必须在有限步内终止
    let cyc = PmAuditDepGraph.init();
    * = cyc.addEdge("pkg-a", "pkg-b")?;
    * = cyc.addEdge("pkg-b", "pkg-a")?;
    const cyc_depth = cyc.findDependentChain("pkg-b", chain[0..]);
    if (cyc_depth != 2) { return err.CycleChainDidNotTerminate; }
}

test("pm_audit: malformed input and buffer/capacity boundary guards") {
    let req = PmAuditRequestBuilder.init(false);

    // 空包名与空版本拒绝
    let empty_name_err = false;
    req.addPackage("", "1.0.0", true, true) catch {
        empty_name_err = true;
    };
    if (!empty_name_err) { return err.EmptyNameNotRejected; }

    let empty_ver_err = false;
    req.addPackage("pkg", "", true, true) catch {
        empty_ver_err = true;
    };
    if (!empty_ver_err) { return err.EmptyVersionNotRejected; }

    // 单包版本数超限拦截
    let vi: usize = 0;
    const ver_list: [9]([]const u8) = ["1.0.0", "1.0.1", "1.0.2", "1.0.3", "1.0.4", "1.0.5", "1.0.6", "1.0.7", "1.0.8"];
    while (vi < PM_AUDIT_MAX_VERSIONS_PER_PKG) {
        * = req.addPackage("many-vers", ver_list[vi], true, true)?;
        vi += 1;
    }
    let ver_overflow = false;
    req.addPackage("many-vers", ver_list[8], true, true) catch {
        ver_overflow = true;
    };
    if (!ver_overflow) { return err.VersionOverflowNotRejected; }

    // 输出缓冲区过小拦截 BufferTooSmall
    let tiny: [4]u8 = [0; 4];
    let buf_small = false;
    req.formatBulkAdvisoryJson(tiny[0..]) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }

    // 忽略列表超限拦截
    let f = PmAuditFilter.init();
    const ids: [33]([]const u8) = [
        "C01", "C02", "C03", "C04", "C05", "C06", "C07", "C08",
        "C09", "C10", "C11", "C12", "C13", "C14", "C15", "C16",
        "C17", "C18", "C19", "C20", "C21", "C22", "C23", "C24",
        "C25", "C26", "C27", "C28", "C29", "C30", "C31", "C32",
        "C33",
    ];
    let ii: usize = 0;
    while (ii < PM_AUDIT_MAX_IGNORE) {
        * = f.addIgnore(ids[ii])?;
        ii += 1;
    }
    let ignore_overflow = false;
    f.addIgnore(ids[32]) catch {
        ignore_overflow = true;
    };
    if (!ignore_overflow) { return err.IgnoreOverflowNotRejected; }
}
