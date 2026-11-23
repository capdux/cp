// cap/test/testCliSecurityScan.cp — Deep production tests for cap:cli securityScan
// Covers:
//   1. CLI argument parsing (`--fail-on`, `--ignore` CSV, `--production`, `--sarif`, `--scanner`)
//   2. SIMD case-insensitive CVE/GHSA & wildcard rule validation + non-backtracking matching
//   3. Advisory deduplication, severity elevation, suppression & deterministic sorting
//   4. Production/optional filtering & `--fail-on` severity gate threshold logic
//   5. OASIS SARIF 2.1.0 JSON export with full JSON string escaping & suppressions
//   6. Aligned terminal table report formatting (populated & clean zero-advisory cases)
//   7. 500-iteration deterministic malformed CLI args & ignore-rule Fuzz gate

import { expect, expectEqual, test } from "cap:test";
import {
    SEC_SCAN_SARIF_VERSION,
    SecScanSeverity,
    secScanSeverityRank,
    secScanSarifLevel,
    parseSecScanSeverity,
    SecScanOutputFormat,
    parseSecScanOutputFormat,
    validateSecScanIgnoreRule,
    matchSecScanIgnorePattern,
    parseSecScanCliArgs,
    SecScanFinding,
    SecScanAggregatedItem,
    evaluateSecurityScan,
    formatSecurityScanSarif,
    formatSecurityScanTable,
} from "cap:cli";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("securityScan: CLI argument parsing (--fail-on, --ignore CSV, --production, --sarif)", () => {
    const argv: [6][]const u8 = [
        "--fail-on=MEDIUM",
        "--ignore=CVE-2024-0001,GHSA-abcd-*",
        "--ignore",
        "lodash",
        "--production",
        "--sarif",
    ];
    let ignores: [8][]const u8 = [""; 8];
    const cfg = parseSecScanCliArgs(argv[0..], ignores[0..]) catch unreachable;

    expect(cfg.fail_on == SecScanSeverity.Moderate);
    expect(cfg.format == SecScanOutputFormat.Sarif);
    expect(cfg.production_only);
    expectEqual(cfg.ignore_count, @as(usize, 3));
    expect(strEql(ignores[0], "CVE-2024-0001"));
    expect(strEql(ignores[1], "GHSA-abcd-*"));
    expect(strEql(ignores[2], "lodash"));

    expect(parseSecScanSeverity("CRITICAL") catch unreachable == SecScanSeverity.Critical);
    expect(parseSecScanOutputFormat("JSON") catch unreachable == SecScanOutputFormat.Json);
    expectEqual(secScanSeverityRank(SecScanSeverity.Critical), @as(u8, 4));
    expect(strEql(secScanSarifLevel(SecScanSeverity.High), "error"));
    expect(strEql(secScanSarifLevel(SecScanSeverity.Moderate), "warning"));
    expect(strEql(secScanSarifLevel(SecScanSeverity.Low), "note"));
});

test("securityScan: SIMD case-insensitive CVE/wildcard matching & rule validation", () => {
    expect(matchSecScanIgnorePattern("cve-2024-3094", "CVE-2024-3094"));
    expect(matchSecScanIgnorePattern("CVE-2024-*", "cve-2024-99999"));
    expect(matchSecScanIgnorePattern("GHSA-????-1234", "ghsa-abcd-1234"));
    expect(!matchSecScanIgnorePattern("GHSA-????-1234", "ghsa-abc-1234"));
    expect(matchSecScanIgnorePattern("@acme/*", "@ACME/ui-kit"));
    expect(!matchSecScanIgnorePattern("@acme/*", "@other/ui-kit"));

    // Reject wildcard-only `*` or empty or whitespace rules
    let bad_star = false;
    validateSecScanIgnoreRule("***") catch { bad_star = true; };
    expect(bad_star);

    let bad_space = false;
    validateSecScanIgnoreRule("CVE 2024") catch { bad_space = true; };
    expect(bad_space);

    let bad_empty = false;
    validateSecScanIgnoreRule("") catch { bad_empty = true; };
    expect(bad_empty);
});

test("securityScan: deduplication, suppression, severity sorting & CI gate evaluation", () => {
    const argv: [2][]const u8 = ["--fail-on=high", "--ignore=CVE-2024-1111"];
    let ignores: [4][]const u8 = [""; 4];
    const cfg = parseSecScanCliArgs(argv[0..], ignores[0..]) catch unreachable;

    const raw: [5]SecScanFinding = [
        SecScanFinding{
            advisory_id: "CVE-2024-2222",
            package_name: "tar",
            installed_version: "6.1.0",
            patched_range: ">=6.2.1",
            title: "Tar path escape",
            severity: SecScanSeverity.Moderate,
            cvss_score_x10: 65,
        },
        // Duplicate of CVE-2024-2222 with elevated severity (High) -> merged, occurrence_count=2
        SecScanFinding{
            advisory_id: "CVE-2024-2222",
            package_name: "tar",
            installed_version: "6.1.0",
            patched_range: ">=6.2.1",
            title: "Tar path escape",
            severity: SecScanSeverity.High,
            cvss_score_x10: 75,
        },
        // Ignored critical advisory -> suppressed, must NOT fail gate
        SecScanFinding{
            advisory_id: "CVE-2024-1111",
            package_name: "legacy-xml",
            installed_version: "1.0.0",
            title: "Ignored XXE",
            severity: SecScanSeverity.Critical,
            cvss_score_x10: 98,
        },
        // Active critical advisory -> sorted first
        SecScanFinding{
            advisory_id: "CVE-2024-9999",
            package_name: "crypto-old",
            installed_version: "0.9.1",
            patched_range: ">=1.0.0",
            title: "Weak signature check",
            severity: SecScanSeverity.Critical,
            cvss_score_x10: 91,
        },
        // Active low advisory
        SecScanFinding{
            advisory_id: "GHSA-0001",
            package_name: "ansi-regex",
            installed_version: "5.0.0",
            title: "ReDoS",
            severity: SecScanSeverity.Low,
            cvss_score_x10: 31,
        },
    ];

    let items: [8]SecScanAggregatedItem = [SecScanAggregatedItem.empty(); 8];
    const sum = evaluateSecurityScan(raw[0..], cfg, ignores[0..cfg.ignore_count], items[0..]) catch unreachable;

    expectEqual(sum.total_raw_findings, @as(usize, 5));
    expectEqual(sum.deduped_count, @as(usize, 4));
    expectEqual(sum.active_count, @as(usize, 3));
    expectEqual(sum.suppressed_by_ignore, @as(usize, 1));
    expectEqual(sum.counts.critical, @as(u32, 1));
    expectEqual(sum.counts.high, @as(u32, 1));
    expectEqual(sum.counts.low, @as(u32, 1));
    expect(sum.gate_failed);
    expectEqual(sum.exit_code, @as(u8, 1));

    // Verify sort order: active Critical -> active High (merged x2) -> active Low -> suppressed
    expect(strEql(items[0].finding.advisory_id, "CVE-2024-9999"));
    expect(strEql(items[1].finding.advisory_id, "CVE-2024-2222"));
    expectEqual(items[1].occurrence_count, @as(u32, 2));
    expect(items[1].finding.severity == SecScanSeverity.High);
    expect(strEql(items[2].finding.advisory_id, "GHSA-0001"));
    expect(items[3].suppressed);
    expect(strEql(items[3].finding.advisory_id, "CVE-2024-1111"));
});

test("securityScan: --production & --omit=optional filtering allows gate to pass", () => {
    const argv: [3][]const u8 = ["--fail-on=high", "--production", "--omit=optional"];
    let ignores: [4][]const u8 = [""; 4];
    const cfg = parseSecScanCliArgs(argv[0..], ignores[0..]) catch unreachable;

    const raw: [3]SecScanFinding = [
        SecScanFinding{
            advisory_id: "CVE-2024-DEV",
            package_name: "jest-old",
            installed_version: "26.0.0",
            severity: SecScanSeverity.Critical,
            is_dev_dep: true,
        },
        SecScanFinding{
            advisory_id: "CVE-2024-OPT",
            package_name: "fsevents-old",
            installed_version: "1.2.0",
            severity: SecScanSeverity.High,
            is_optional_dep: true,
        },
        SecScanFinding{
            advisory_id: "CVE-2024-MOD",
            package_name: "http-util",
            installed_version: "2.0.0",
            severity: SecScanSeverity.Moderate,
        },
    ];

    let items: [4]SecScanAggregatedItem = [SecScanAggregatedItem.empty(); 4];
    const sum = evaluateSecurityScan(raw[0..], cfg, ignores[0..cfg.ignore_count], items[0..]) catch unreachable;

    expectEqual(sum.skipped_dev_or_opt, @as(usize, 2));
    expectEqual(sum.active_count, @as(usize, 1));
    expect(!sum.gate_failed);
    expectEqual(sum.exit_code, @as(u8, 0));
});

test("securityScan: OASIS SARIF 2.1.0 JSON export with string escaping & suppressions", () => {
    const argv: [1][]const u8 = ["--ignore=CVE-2024-SUP"];
    let ignores: [4][]const u8 = [""; 4];
    const cfg = parseSecScanCliArgs(argv[0..], ignores[0..]) catch unreachable;

    const raw: [2]SecScanFinding = [
        SecScanFinding{
            advisory_id: "CVE-2024-ESC",
            package_name: "json-lib",
            installed_version: "1.0.0",
            patched_range: ">=1.0.1",
            title: "Quote \"escape\" and\\backslash\nnewline",
            url: "https://example.com/advisory/1",
            severity: SecScanSeverity.High,
        },
        SecScanFinding{
            advisory_id: "CVE-2024-SUP",
            package_name: "old-pkg",
            installed_version: "0.1.0",
            title: "Suppressed issue",
            url: "https://example.com/advisory/2",
            severity: SecScanSeverity.Low,
        },
    ];

    let items: [4]SecScanAggregatedItem = [SecScanAggregatedItem.empty(); 4];
    const sum = evaluateSecurityScan(raw[0..], cfg, ignores[0..cfg.ignore_count], items[0..]) catch unreachable;

    let sarif: [4096]u8 = [0; 4096];
    const n = formatSecurityScanSarif("cap-scan", "1.0.0", items[0..sum.deduped_count], sum, sarif[0..]) catch unreachable;
    const doc = sarif[0..n];

    expect(containsSubstr(doc, "\"version\":\"2.1.0\""));
    expect(containsSubstr(doc, SEC_SCAN_SARIF_VERSION));
    expect(containsSubstr(doc, "\"ruleId\":\"CVE-2024-ESC\""));
    expect(containsSubstr(doc, "\"level\":\"error\""));
    expect(containsSubstr(doc, "Quote \\\"escape\\\" and\\\\backslash\\nnewline"));
    expect(containsSubstr(doc, "\"uri\":\"cap.lock\""));
    expect(containsSubstr(doc, "\"suppressions\":[{\"kind\":\"inSource\""));
});

test("securityScan: aligned terminal table formatter (populated & clean zero-advisory)", () => {
    const argv: [1][]const u8 = ["--fail-on=critical"];
    let ignores: [4][]const u8 = [""; 4];
    const cfg = parseSecScanCliArgs(argv[0..], ignores[0..]) catch unreachable;

    // Clean case
    const empty_raw: [0]SecScanFinding = [];
    let items: [4]SecScanAggregatedItem = [SecScanAggregatedItem.empty(); 4];
    const clean_sum = evaluateSecurityScan(empty_raw[0..], cfg, ignores[0..0], items[0..]) catch unreachable;
    let out: [2048]u8 = [0; 2048];
    const n0 = formatSecurityScanTable(items[0..0], clean_sum, out[0..]) catch unreachable;
    expect(containsSubstr(out[0..n0], "No advisories found."));
    expect(containsSubstr(out[0..n0], "gate [--fail-on=critical]: PASS"));

    // Populated case
    const raw: [2]SecScanFinding = [
        SecScanFinding{
            advisory_id: "CVE-2024-5555",
            package_name: "ws-lib",
            installed_version: "3.0.0",
            patched_range: ">=3.1.0",
            severity: SecScanSeverity.Critical,
        },
        SecScanFinding{
            advisory_id: "CVE-2024-5555",
            package_name: "ws-lib",
            installed_version: "3.0.0",
            patched_range: ">=3.1.0",
            severity: SecScanSeverity.Critical,
        },
    ];
    const sum = evaluateSecurityScan(raw[0..], cfg, ignores[0..0], items[0..]) catch unreachable;
    const n1 = formatSecurityScanTable(items[0..sum.deduped_count], sum, out[0..]) catch unreachable;
    const tbl = out[0..n1];
    expect(containsSubstr(tbl, "CVE-2024-5555"));
    expect(containsSubstr(tbl, "ACTIVE (x2)"));
    expect(containsSubstr(tbl, "gate [--fail-on=critical]: FAIL"));
});

test("securityScan: 500-iteration deterministic malformed CLI args & ignore-rule Fuzz gate", () => {
    let pat_buf: [256]u8 = [0; 256];
    let ignores: [8][]const u8 = [""; 8];
    let rng: u64 = 0x5CA11B00CAFE1234;
    let rejected: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const r = rng *% 0x2545F4914F6CDD1D;
        const mode = iter % 5;

        if (mode == 0) {
            // Invalid --fail-on severity
            const argv: [2][]const u8 = ["--fail-on", "super-extreme"];
            let failed = false;
            * = parseSecScanCliArgs(argv[0..], ignores[0..]) catch { failed = true; };
            expect(failed);
            if (failed) rejected += 1;
        } else if (mode == 1) {
            // Missing flag value at end of argv
            const pick = r % 4;
            const flag = if (pick == 0) "--fail-on"
                else if (pick == 1) "--ignore"
                else if (pick == 2) "--format"
                else "--scanner";
            const argv: [1][]const u8 = [flag];
            let failed = false;
            * = parseSecScanCliArgs(argv[0..], ignores[0..]) catch { failed = true; };
            expect(failed);
            if (failed) rejected += 1;
        } else if (mode == 2) {
            // Random control / high-bit bytes in ignore rule
            const len = 1 + @as(usize, @intCast(r % 40));
            let k: usize = 0;
            while (k < len) {
                rng ^= rng >> 12;
                rng ^= rng << 25;
                rng ^= rng >> 27;
                pat_buf[k] = @as(u8, @truncate(rng));
                k += 1;
            }
            pat_buf[len / 2] = if ((r & 1) == 0) 0x00 else 0xFF;
            let failed = false;
            validateSecScanIgnoreRule(pat_buf[0..len]) catch { failed = true; };
            expect(failed);
            if (failed) rejected += 1;
        } else if (mode == 3) {
            // Pathological multi-star wildcard pattern against candidate (must terminate linearly, 0 hang)
            const stars = "a*b*c*d*e*f*g*h*i*j*k*l*m*n*o*p*z";
            const cand = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
            const matched = matchSecScanIgnorePattern(stars, cand);
            expect(!matched);
            if (!matched) rejected += 1;
        } else {
            // Unknown or malformed CLI flag
            const len = 2 + @as(usize, @intCast(r % 20));
            pat_buf[0] = '-';
            pat_buf[1] = '-';
            let k: usize = 2;
            while (k < len) {
                pat_buf[k] = @as(u8, 'a') + @as(u8, @intCast((r +% @as(u64, k)) % 26));
                k += 1;
            }
            const argv: [1][]const u8 = [pat_buf[0..len]];
            let failed = false;
            * = parseSecScanCliArgs(argv[0..], ignores[0..]) catch { failed = true; };
            expect(failed);
            if (failed) rejected += 1;
        }

        iter += 1;
    }

    expectEqual(rejected, @as(usize, 500));
});
