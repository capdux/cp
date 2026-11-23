// Source: Absorbed from /data/dev/bun/src/cli/scan_command.cp | Synced: 2026-11-22T01:23:34Z
// ============================================================================
// lib/cli/securityScan.cp — `cap scan` Supply-Chain Security CLI & SARIF 2.1.0 Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven supply-chain security scan engine:
//   1. CLI argument parser (`parseSecScanCliArgs`) supporting `--fail-on=<sev>`,
//      `--ignore=<cve_or_glob>` (comma-separated), `--production`, `--omit=optional`,
//      `--format=<table|sarif|json>`, `--sarif`, `--scanner=<pkg>`, `--quiet`;
//   2. `cap:simd`-accelerated case-insensitive CVE/GHSA & wildcard rule matcher
//      (`matchSecScanIgnorePattern`) with guaranteed O(N*M) non-backtracking loop;
//   3. Deterministic advisory deduplication, suppression, severity sorting, and
//      CI gate evaluation (`evaluateSecurityScan`);
//   4. Dual report exporters: OASIS **SARIF 2.1.0 JSON** (`formatSecurityScanSarif`)
//      with full JSON string escaping and aligned terminal table
//      (`formatSecurityScanTable`).

import { Simd } from "cap:simd";

/// OASIS SARIF specification version emitted by `formatSecurityScanSarif`.
pub const SEC_SCAN_SARIF_VERSION: []const u8 = "2.1.0";

/// JSON Schema URI for SARIF 2.1.0 documents.
pub const SEC_SCAN_SARIF_SCHEMA: []const u8 = "https://json.schemastore.org/sarif-2.1.0.json";

/// Maximum byte length of an advisory ID or ignore pattern.
pub const SEC_SCAN_MAX_PATTERN_LEN: usize = 128;

/// Error set for `cap scan` CLI parsing, rule validation, and report formatting.
pub err SecScanError {
    InvalidSeverity,
    InvalidFormat,
    InvalidIgnoreRule,
    MissingFlagValue,
    UnknownFlag,
    CapacityExceeded,
    BufferTooSmall,
    NoScannerConfigured,
}

// ============================================================================
// Internal Zero-Alloc Byte, String, Escaping, and Formatting Helpers
// ============================================================================

const SS_HEX_LOWER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

fn ssBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn ssLexLess(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn ssStartsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return ssBytesEql(s[0..prefix.len], prefix);
}

fn ssAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ssAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ssAppendPadSpaces(out: []u8, pos: *usize, count: usize)! {
    if (count > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < count) {
        out[pos.* + i] = ' ';
        i += 1;
    }
    pos.* += count;
}

fn ssAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ssAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        ssAppendByte(out, pos, tmp[n])?;
    }
}

fn ssAppendJsonString(out: []u8, pos: *usize, s: []const u8)! {
    ssAppendByte(out, pos, '"')?;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            ssAppendBytes(out, pos, "\\\"")?;
        } else if (c == '\\') {
            ssAppendBytes(out, pos, "\\\\")?;
        } else if (c == '\n') {
            ssAppendBytes(out, pos, "\\n")?;
        } else if (c == '\r') {
            ssAppendBytes(out, pos, "\\r")?;
        } else if (c == '\t') {
            ssAppendBytes(out, pos, "\\t")?;
        } else if (c < 0x20) {
            ssAppendBytes(out, pos, "\\u00")?;
            ssAppendByte(out, pos, SS_HEX_LOWER[@as(usize, c >> 4)])?;
            ssAppendByte(out, pos, SS_HEX_LOWER[@as(usize, c & 0xF)])?;
        } else {
            ssAppendByte(out, pos, c)?;
        }
        i += 1;
    }
    ssAppendByte(out, pos, '"')?;
}

// ============================================================================
// Section 1: Severity Classification & SIMD-Accelerated Ignore Rule Matching
// ============================================================================

/// Security advisory severity levels ordered from lowest (`Info`) to highest (`Critical`).
pub enum SecScanSeverity {
    Info,
    Low,
    Moderate,
    High,
    Critical,
}

/// Returns numeric severity rank (`0 = Info` .. `4 = Critical`).
pub fn secScanSeverityRank(sev: SecScanSeverity): u8 {
    return match (sev) {
        SecScanSeverity.Info => 0,
        SecScanSeverity.Low => 1,
        SecScanSeverity.Moderate => 2,
        SecScanSeverity.High => 3,
        SecScanSeverity.Critical => 4,
    };
}

/// Returns the canonical lowercase ASCII label for a severity level.
pub fn secScanSeverityName(sev: SecScanSeverity): []const u8 {
    return match (sev) {
        SecScanSeverity.Info => "info",
        SecScanSeverity.Low => "low",
        SecScanSeverity.Moderate => "moderate",
        SecScanSeverity.High => "high",
        SecScanSeverity.Critical => "critical",
    };
}

/// Maps `SecScanSeverity` to the OASIS SARIF 2.1.0 result level (`error`/`warning`/`note`).
pub fn secScanSarifLevel(sev: SecScanSeverity): []const u8 {
    return match (sev) {
        SecScanSeverity.Critical => "error",
        SecScanSeverity.High => "error",
        SecScanSeverity.Moderate => "warning",
        SecScanSeverity.Low => "note",
        SecScanSeverity.Info => "note",
    };
}

/// Parses a severity string case-insensitively using `cap:simd` (`Simd.toLowerAscii`).
/// Accepts `"medium"` as an alias for `Moderate`.
pub fn parseSecScanSeverity(raw: []const u8): SecScanError!SecScanSeverity {
    if (raw.len == 0 || raw.len > 16) return err.InvalidSeverity;
    if (!Simd.isAscii(raw)) return err.InvalidSeverity;
    let lower: [16]u8 = [0; 16];
    const n = Simd.toLowerAscii(raw, lower[0..]);
    const s = lower[0..n];

    if (ssBytesEql(s, "info")) return SecScanSeverity.Info;
    if (ssBytesEql(s, "low")) return SecScanSeverity.Low;
    if (ssBytesEql(s, "moderate") || ssBytesEql(s, "medium")) return SecScanSeverity.Moderate;
    if (ssBytesEql(s, "high")) return SecScanSeverity.High;
    if (ssBytesEql(s, "critical")) return SecScanSeverity.Critical;
    return err.InvalidSeverity;
}

/// Report output format for `cap scan`.
pub enum SecScanOutputFormat {
    Table,
    Sarif,
    Json,
}

/// Parses an output format name case-insensitively using `cap:simd`.
pub fn parseSecScanOutputFormat(raw: []const u8): SecScanError!SecScanOutputFormat {
    if (raw.len == 0 || raw.len > 16) return err.InvalidFormat;
    if (!Simd.isAscii(raw)) return err.InvalidFormat;
    let lower: [16]u8 = [0; 16];
    const n = Simd.toLowerAscii(raw, lower[0..]);
    const s = lower[0..n];

    if (ssBytesEql(s, "table") || ssBytesEql(s, "text")) return SecScanOutputFormat.Table;
    if (ssBytesEql(s, "sarif")) return SecScanOutputFormat.Sarif;
    if (ssBytesEql(s, "json")) return SecScanOutputFormat.Json;
    return err.InvalidFormat;
}

/// Validates a CVE / GHSA / package ignore rule using `cap:simd` pre-filtering.
/// Rejects empty rules, overlength patterns, non-ASCII bytes, control characters,
/// whitespace, and dangerous wildcard-only patterns (`*`).
pub fn validateSecScanIgnoreRule(pattern: []const u8)! {
    if (pattern.len == 0 || pattern.len > SEC_SCAN_MAX_PATTERN_LEN) {
        return err.InvalidIgnoreRule;
    }
    if (!Simd.isAscii(pattern)) return err.InvalidIgnoreRule;
    if (Simd.indexOfAny4(pattern, 0, ' ', '\t', '\n') != null) {
        return err.InvalidIgnoreRule;
    }

    let has_non_wildcard = false;
    let i: usize = 0;
    while (i < pattern.len) {
        const c = pattern[i];
        if (c < 0x21 || c == 0x7F || c == '\\' || c == ',') {
            return err.InvalidIgnoreRule;
        }
        if (c != '*' && c != '?') {
            has_non_wildcard = true;
        }
        i += 1;
    }
    if (!has_non_wildcard) return err.InvalidIgnoreRule;
}

/// Case-insensitive wildcard matcher (`*` and `?`) accelerated by `cap:simd`
/// (`Simd.toLowerAscii`, `Simd.indexOfAny2`). Uses a bounded two-pointer loop
/// with zero recursion and zero exponential backtracking.
pub fn matchSecScanIgnorePattern(pattern: []const u8, candidate: []const u8): bool {
    if (pattern.len == 0 || pattern.len > SEC_SCAN_MAX_PATTERN_LEN) return false;
    if (candidate.len == 0 || candidate.len > SEC_SCAN_MAX_PATTERN_LEN) return false;
    if (!Simd.isAscii(pattern) || !Simd.isAscii(candidate)) return false;

    let pat_low: [SEC_SCAN_MAX_PATTERN_LEN]u8 = [0; SEC_SCAN_MAX_PATTERN_LEN];
    let cand_low: [SEC_SCAN_MAX_PATTERN_LEN]u8 = [0; SEC_SCAN_MAX_PATTERN_LEN];
    const pn = Simd.toLowerAscii(pattern, pat_low[0..]);
    const cn = Simd.toLowerAscii(candidate, cand_low[0..]);
    const p = pat_low[0..pn];
    const t = cand_low[0..cn];

    // Fast path: exact match when pattern has no wildcard characters
    if (Simd.indexOfAny2(p, '*', '?') == null) {
        return ssBytesEql(p, t);
    }

    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = SEC_SCAN_MAX_PATTERN_LEN + 1;
    let star_ti: usize = 0;

    while (ti < t.len) {
        if (pi < p.len && (p[pi] == '?' || p[pi] == t[ti])) {
            pi += 1;
            ti += 1;
        } else if (pi < p.len && p[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (star_pi <= SEC_SCAN_MAX_PATTERN_LEN) {
            pi = star_pi + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }

    while (pi < p.len && p[pi] == '*') {
        pi += 1;
    }
    return pi == p.len;
}

// ============================================================================
// Section 2: `cap scan` CLI Argument Parser
// ============================================================================

/// Parsed CLI configuration for `cap scan`.
pub struct SecScanCliConfig {
    fail_on: SecScanSeverity = SecScanSeverity.High,
    format: SecScanOutputFormat = SecScanOutputFormat.Table,
    production_only: bool = false,
    include_optional: bool = true,
    quiet: bool = false,
    ignore_count: usize = 0,
    scanner_package: []const u8 = "",
}

fn ssAddIgnoreCsv(
    csv: []const u8,
    out_ignores: [][]const u8,
    ignore_count: *usize,
)! {
    if (csv.len == 0) return err.InvalidIgnoreRule;
    let pos: usize = 0;
    while (pos <= csv.len) {
        let end = csv.len;
        if (comma := Simd.indexOfByte(csv[pos..], ',')) {
            end = pos + comma;
        }
        const item = csv[pos..end];
        validateSecScanIgnoreRule(item)?;
        if (ignore_count.* >= out_ignores.len) return err.CapacityExceeded;
        out_ignores[ignore_count.*] = item;
        ignore_count.* += 1;
        if (end >= csv.len) break;
        pos = end + 1;
    }
}

/// Parses `cap scan` CLI arguments into `SecScanCliConfig`, writing validated
/// `--ignore` rules (including comma-separated lists) into `out_ignores`.
pub fn parseSecScanCliArgs(
    argv: []const []const u8,
    out_ignores: [][]const u8,
): SecScanError!SecScanCliConfig {
    let cfg = SecScanCliConfig{};
    let i: usize = 0;

    while (i < argv.len) {
        const arg = argv[i];
        if (arg.len == 0) return err.UnknownFlag;

        if (ssBytesEql(arg, "--production") || ssBytesEql(arg, "--prod") || ssBytesEql(arg, "--omit=dev")) {
            cfg.production_only = true;
        } else if (ssBytesEql(arg, "--omit=optional")) {
            cfg.include_optional = false;
        } else if (ssBytesEql(arg, "--sarif")) {
            cfg.format = SecScanOutputFormat.Sarif;
        } else if (ssBytesEql(arg, "--json")) {
            cfg.format = SecScanOutputFormat.Json;
        } else if (ssBytesEql(arg, "--quiet") || ssBytesEql(arg, "-q")) {
            cfg.quiet = true;
        } else if (ssStartsWith(arg, "--fail-on=")) {
            cfg.fail_on = parseSecScanSeverity(arg[10..])?;
        } else if (ssBytesEql(arg, "--fail-on")) {
            if (i + 1 >= argv.len) return err.MissingFlagValue;
            i += 1;
            cfg.fail_on = parseSecScanSeverity(argv[i])?;
        } else if (ssStartsWith(arg, "--format=")) {
            cfg.format = parseSecScanOutputFormat(arg[9..])?;
        } else if (ssBytesEql(arg, "--format")) {
            if (i + 1 >= argv.len) return err.MissingFlagValue;
            i += 1;
            cfg.format = parseSecScanOutputFormat(argv[i])?;
        } else if (ssStartsWith(arg, "--ignore=")) {
            ssAddIgnoreCsv(arg[9..], out_ignores, &cfg.ignore_count)?;
        } else if (ssBytesEql(arg, "--ignore")) {
            if (i + 1 >= argv.len) return err.MissingFlagValue;
            i += 1;
            ssAddIgnoreCsv(argv[i], out_ignores, &cfg.ignore_count)?;
        } else if (ssStartsWith(arg, "--scanner=")) {
            const pkg = arg[10..];
            if (pkg.len == 0) return err.MissingFlagValue;
            cfg.scanner_package = pkg;
        } else if (ssBytesEql(arg, "--scanner")) {
            if (i + 1 >= argv.len) return err.MissingFlagValue;
            i += 1;
            if (argv[i].len == 0) return err.MissingFlagValue;
            cfg.scanner_package = argv[i];
        } else {
            return err.UnknownFlag;
        }

        i += 1;
    }

    return cfg;
}

// ============================================================================
// Section 3: Advisory Deduplication, Suppression & Severity Gate Evaluation
// ============================================================================

/// Single raw security advisory emitted by a scanner provider.
pub struct SecScanFinding {
    advisory_id: []const u8 = "",
    package_name: []const u8 = "",
    installed_version: []const u8 = "",
    patched_range: []const u8 = "",
    title: []const u8 = "",
    url: []const u8 = "",
    severity: SecScanSeverity = SecScanSeverity.Info,
    is_dev_dep: bool = false,
    is_optional_dep: bool = false,
    cvss_score_x10: u16 = 0,
}

/// Deduplicated and gate-annotated advisory record.
pub struct SecScanAggregatedItem {
    finding: SecScanFinding = SecScanFinding{},
    occurrence_count: u32 = 0,
    suppressed: bool = false,

    static fn empty(): SecScanAggregatedItem {
        return SecScanAggregatedItem{};
    }
}

/// Per-severity active advisory counters.
pub struct SecScanSeverityCounts {
    critical: u32 = 0,
    high: u32 = 0,
    moderate: u32 = 0,
    low: u32 = 0,
    info: u32 = 0,

    fn total(): u32 {
        return _.critical + _.high + _.moderate + _.low + _.info;
    }

    fn countAtOrAbove(threshold: SecScanSeverity): u32 {
        const min_rank = secScanSeverityRank(threshold);
        let sum: u32 = 0;
        if (4 >= min_rank) sum += _.critical;
        if (3 >= min_rank) sum += _.high;
        if (2 >= min_rank) sum += _.moderate;
        if (1 >= min_rank) sum += _.low;
        if (0 >= min_rank) sum += _.info;
        return sum;
    }
}

/// Gate evaluation summary produced by `evaluateSecurityScan`.
pub struct SecScanGateSummary {
    total_raw_findings: usize = 0,
    deduped_count: usize = 0,
    active_count: usize = 0,
    suppressed_by_ignore: usize = 0,
    skipped_dev_or_opt: usize = 0,
    counts: SecScanSeverityCounts = SecScanSeverityCounts{},
    fail_threshold: SecScanSeverity = SecScanSeverity.High,
    gate_failed: bool = false,
    exit_code: u8 = 0,
}

fn ssIsIgnored(finding: SecScanFinding, rules: []const []const u8): bool {
    let i: usize = 0;
    while (i < rules.len) {
        if (matchSecScanIgnorePattern(rules[i], finding.advisory_id) ||
            matchSecScanIgnorePattern(rules[i], finding.package_name))
        {
            return true;
        }
        i += 1;
    }
    return false;
}

fn ssSameFindingKey(a: SecScanFinding, b: SecScanFinding): bool {
    return ssBytesEql(a.advisory_id, b.advisory_id) &&
        ssBytesEql(a.package_name, b.package_name) &&
        ssBytesEql(a.installed_version, b.installed_version);
}

fn ssItemBefore(a: SecScanAggregatedItem, b: SecScanAggregatedItem): bool {
    // Active findings precede suppressed ones
    if (a.suppressed != b.suppressed) return !a.suppressed;
    const ra = secScanSeverityRank(a.finding.severity);
    const rb = secScanSeverityRank(b.finding.severity);
    if (ra != rb) return ra > rb;
    if (a.finding.cvss_score_x10 != b.finding.cvss_score_x10) {
        return a.finding.cvss_score_x10 > b.finding.cvss_score_x10;
    }
    if (!ssBytesEql(a.finding.advisory_id, b.finding.advisory_id)) {
        return ssLexLess(a.finding.advisory_id, b.finding.advisory_id);
    }
    return ssLexLess(a.finding.package_name, b.finding.package_name);
}

/// Filters, deduplicates, applies `--ignore` suppression patterns, sorts by
/// severity/CVSS, and evaluates the `--fail-on` CI gate.
pub fn evaluateSecurityScan(
    findings: []const SecScanFinding,
    cfg: SecScanCliConfig,
    ignore_rules: []const []const u8,
    out_items: []SecScanAggregatedItem,
): SecScanError!SecScanGateSummary {
    let summary = SecScanGateSummary{
        total_raw_findings: findings.len,
        fail_threshold: cfg.fail_on,
    };

    let count: usize = 0;
    let i: usize = 0;
    while (i < findings.len) {
        const f = findings[i];
        if ((cfg.production_only && f.is_dev_dep) || (!cfg.include_optional && f.is_optional_dep)) {
            summary.skipped_dev_or_opt += 1;
            i += 1;
            continue;
        }

        // Deduplicate by (advisory_id, package_name, installed_version)
        let merged = false;
        let k: usize = 0;
        while (k < count) {
            if (ssSameFindingKey(out_items[k].finding, f)) {
                const cur = out_items[k];
                const elevated_sev = if (secScanSeverityRank(f.severity) > secScanSeverityRank(cur.finding.severity))
                    f.severity
                else
                    cur.finding.severity;
                out_items[k] = SecScanAggregatedItem{
                    finding: SecScanFinding{
                        advisory_id: cur.finding.advisory_id,
                        package_name: cur.finding.package_name,
                        installed_version: cur.finding.installed_version,
                        patched_range: cur.finding.patched_range,
                        title: cur.finding.title,
                        url: cur.finding.url,
                        severity: elevated_sev,
                        is_dev_dep: cur.finding.is_dev_dep,
                        is_optional_dep: cur.finding.is_optional_dep,
                        cvss_score_x10: cur.finding.cvss_score_x10,
                    },
                    occurrence_count: cur.occurrence_count + 1,
                    suppressed: cur.suppressed,
                };
                merged = true;
                break;
            }
            k += 1;
        }

        if (!merged) {
            if (count >= out_items.len) return err.CapacityExceeded;
            const sup = ssIsIgnored(f, ignore_rules);
            out_items[count] = SecScanAggregatedItem{
                finding: f,
                occurrence_count: 1,
                suppressed: sup,
            };
            count += 1;
        }
        i += 1;
    }

    // Deterministic insertion sort by (active first, severity DESC, CVSS DESC, ID ASC)
    let s: usize = 1;
    while (s < count) {
        const key_item = out_items[s];
        let j = s;
        while (j > 0 && ssItemBefore(key_item, out_items[j - 1])) {
            out_items[j] = out_items[j - 1];
            j -= 1;
        }
        out_items[j] = key_item;
        s += 1;
    }

    // Tally active vs suppressed counts
    let m: usize = 0;
    while (m < count) {
        if (out_items[m].suppressed) {
            summary.suppressed_by_ignore += 1;
        } else {
            summary.active_count += 1;
            const sev = out_items[m].finding.severity;
            if (sev == SecScanSeverity.Critical) {
                summary.counts.critical += 1;
            } else if (sev == SecScanSeverity.High) {
                summary.counts.high += 1;
            } else if (sev == SecScanSeverity.Moderate) {
                summary.counts.moderate += 1;
            } else if (sev == SecScanSeverity.Low) {
                summary.counts.low += 1;
            } else {
                summary.counts.info += 1;
            }
        }
        m += 1;
    }

    summary.deduped_count = count;
    summary.gate_failed = summary.counts.countAtOrAbove(cfg.fail_on) > 0;
    summary.exit_code = if (summary.gate_failed) 1 else 0;
    return summary;
}

// ============================================================================
// Section 4: OASIS SARIF 2.1.0 JSON & Terminal Table Report Exporters
// ============================================================================

/// Exports deduplicated security scan results as an OASIS **SARIF 2.1.0** JSON document
/// suitable for GitHub Code Scanning, GitLab SAST, and CI artifact ingestion.
pub fn formatSecurityScanSarif(
    tool_name: []const u8,
    tool_version: []const u8,
    items: []const SecScanAggregatedItem,
    summary: SecScanGateSummary,
    out: []u8,
): SecScanError!usize {
    const limit = if (summary.deduped_count < items.len) summary.deduped_count else items.len;
    let pos: usize = 0;

    ssAppendBytes(out, &pos, "{\"$schema\":")?;
    ssAppendJsonString(out, &pos, SEC_SCAN_SARIF_SCHEMA)?;
    ssAppendBytes(out, &pos, ",\"version\":")?;
    ssAppendJsonString(out, &pos, SEC_SCAN_SARIF_VERSION)?;
    ssAppendBytes(out, &pos, ",\"runs\":[{\"tool\":{\"driver\":{\"name\":")?;
    ssAppendJsonString(out, &pos, if (tool_name.len > 0) tool_name else "cap-scan")?;
    ssAppendBytes(out, &pos, ",\"version\":")?;
    ssAppendJsonString(out, &pos, if (tool_version.len > 0) tool_version else "0.1.0")?;
    ssAppendBytes(out, &pos, ",\"rules\":[")?;

    // Emit unique rule descriptors
    let r: usize = 0;
    while (r < limit) {
        if (r > 0) ssAppendByte(out, &pos, ',')?;
        const f = items[r].finding;
        ssAppendBytes(out, &pos, "{\"id\":")?;
        ssAppendJsonString(out, &pos, f.advisory_id)?;
        ssAppendBytes(out, &pos, ",\"shortDescription\":{\"text\":")?;
        ssAppendJsonString(out, &pos, f.title)?;
        ssAppendBytes(out, &pos, "},\"helpUri\":")?;
        ssAppendJsonString(out, &pos, f.url)?;
        ssAppendBytes(out, &pos, ",\"properties\":{\"severity\":")?;
        ssAppendJsonString(out, &pos, secScanSeverityName(f.severity))?;
        ssAppendBytes(out, &pos, "}}")?;
        r += 1;
    }

    ssAppendBytes(out, &pos, "]}},\"results\":[")?;

    // Emit SARIF result entries
    let i: usize = 0;
    while (i < limit) {
        if (i > 0) ssAppendByte(out, &pos, ',')?;
        const item = items[i];
        const f = item.finding;
        ssAppendBytes(out, &pos, "{\"ruleId\":")?;
        ssAppendJsonString(out, &pos, f.advisory_id)?;
        ssAppendBytes(out, &pos, ",\"level\":")?;
        ssAppendJsonString(out, &pos, secScanSarifLevel(f.severity))?;
        ssAppendBytes(out, &pos, ",\"message\":{\"text\":")?;
        ssAppendJsonString(out, &pos, f.title)?;
        ssAppendBytes(out, &pos, "},\"locations\":[{\"physicalLocation\":{\"artifactLocation\":{\"uri\":\"cap.lock\"}}}]")?;
        if (item.suppressed) {
            ssAppendBytes(out, &pos, ",\"suppressions\":[{\"kind\":\"inSource\",\"justification\":\"ignored via --ignore\"}]")?;
        }
        ssAppendBytes(out, &pos, ",\"properties\":{\"package\":")?;
        ssAppendJsonString(out, &pos, f.package_name)?;
        ssAppendBytes(out, &pos, ",\"installedVersion\":")?;
        ssAppendJsonString(out, &pos, f.installed_version)?;
        ssAppendBytes(out, &pos, ",\"patchedRange\":")?;
        ssAppendJsonString(out, &pos, if (f.patched_range.len > 0) f.patched_range else "none")?;
        ssAppendBytes(out, &pos, ",\"occurrences\":")?;
        ssAppendU64(out, &pos, @as(u64, item.occurrence_count))?;
        ssAppendBytes(out, &pos, "}}")?;
        i += 1;
    }

    ssAppendBytes(out, &pos, "]}]}")?;
    return pos;
}

/// Formats an aligned human-readable terminal table and CI gate summary for `cap scan`.
pub fn formatSecurityScanTable(
    items: []const SecScanAggregatedItem,
    summary: SecScanGateSummary,
    out: []u8,
): SecScanError!usize {
    const limit = if (summary.deduped_count < items.len) summary.deduped_count else items.len;
    let pos: usize = 0;

    ssAppendBytes(out, &pos, "cap scan summary: active=")?;
    ssAppendU64(out, &pos, @as(u64, summary.active_count))?;
    ssAppendBytes(out, &pos, " (critical=")?;
    ssAppendU64(out, &pos, @as(u64, summary.counts.critical))?;
    ssAppendBytes(out, &pos, ", high=")?;
    ssAppendU64(out, &pos, @as(u64, summary.counts.high))?;
    ssAppendBytes(out, &pos, ", moderate=")?;
    ssAppendU64(out, &pos, @as(u64, summary.counts.moderate))?;
    ssAppendBytes(out, &pos, ", low=")?;
    ssAppendU64(out, &pos, @as(u64, summary.counts.low))?;
    ssAppendBytes(out, &pos, ", info=")?;
    ssAppendU64(out, &pos, @as(u64, summary.counts.info))?;
    ssAppendBytes(out, &pos, "), suppressed=")?;
    ssAppendU64(out, &pos, @as(u64, summary.suppressed_by_ignore))?;
    ssAppendBytes(out, &pos, ", skipped=")?;
    ssAppendU64(out, &pos, @as(u64, summary.skipped_dev_or_opt))?;
    ssAppendByte(out, &pos, '\n')?;

    if (limit == 0) {
        ssAppendBytes(out, &pos, "No advisories found.\n")?;
    } else {
        ssAppendBytes(out, &pos, "SEVERITY   ADVISORY             PACKAGE              VERSION      PATCHED      STATUS\n")?;
        ssAppendBytes(out, &pos, "---------- -------------------- -------------------- ------------ ------------ --------\n")?;
        let i: usize = 0;
        while (i < limit) {
            const item = items[i];
            const f = item.finding;
            const sev_s = secScanSeverityName(f.severity);
            ssAppendBytes(out, &pos, sev_s)?;
            if (sev_s.len < 11) ssAppendPadSpaces(out, &pos, 11 - sev_s.len)?; else ssAppendByte(out, &pos, ' ')?;

            ssAppendBytes(out, &pos, f.advisory_id)?;
            if (f.advisory_id.len < 21) ssAppendPadSpaces(out, &pos, 21 - f.advisory_id.len)?; else ssAppendByte(out, &pos, ' ')?;

            ssAppendBytes(out, &pos, f.package_name)?;
            if (f.package_name.len < 21) ssAppendPadSpaces(out, &pos, 21 - f.package_name.len)?; else ssAppendByte(out, &pos, ' ')?;

            ssAppendBytes(out, &pos, f.installed_version)?;
            if (f.installed_version.len < 13) ssAppendPadSpaces(out, &pos, 13 - f.installed_version.len)?; else ssAppendByte(out, &pos, ' ')?;

            const patch_s = if (f.patched_range.len > 0) f.patched_range else "-";
            ssAppendBytes(out, &pos, patch_s)?;
            if (patch_s.len < 13) ssAppendPadSpaces(out, &pos, 13 - patch_s.len)?; else ssAppendByte(out, &pos, ' ')?;

            if (item.suppressed) {
                ssAppendBytes(out, &pos, "IGNORED")?;
            } else {
                ssAppendBytes(out, &pos, "ACTIVE")?;
            }
            if (item.occurrence_count > 1) {
                ssAppendBytes(out, &pos, " (x")?;
                ssAppendU64(out, &pos, @as(u64, item.occurrence_count))?;
                ssAppendByte(out, &pos, ')')?;
            }
            ssAppendByte(out, &pos, '\n')?;
            i += 1;
        }
    }

    ssAppendBytes(out, &pos, "gate [--fail-on=")?;
    ssAppendBytes(out, &pos, secScanSeverityName(summary.fail_threshold))?;
    ssAppendBytes(out, &pos, "]: ")?;
    ssAppendBytes(out, &pos, if (summary.gate_failed) "FAIL" else "PASS")?;
    ssAppendByte(out, &pos, '\n')?;
    return pos;
}
