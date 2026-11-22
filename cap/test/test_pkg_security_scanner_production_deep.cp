// cap/test/test_pkg_security_scanner_production_deep.cp
// Production-deep regression tests for cap:pkg security_scanner module
// Source: /data/dev/bun/src/install/PackageManager/security_scanner.cp & /data/dev/bun/docs/pm/security-scanner-api.mdx

import { test, expect } from "cap:test";
import {
    SECURITY_SCANNER_PROTOCOL_VERSION,
    SECURITY_SCANNER_MAX_ADVISORIES,
    SECURITY_SCANNER_MAX_PATH_DEPTH,
    SecurityScannerError,
    SecurityScannerErrorCode,
    SecurityAdvisoryLevel,
    SecurityInstallDecision,
    SecurityScanPackageInput,
    SecurityAdvisory,
    SecurityScanResults,
    validateScannerProtocolVersion,
    parseWarningConfirmation,
    formatScanRequestJson,
    parseSecurityAdvisoriesJson,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
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

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEq(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

test("pkg security_scanner: protocol version, error codes & advisory levels") {
    validateScannerProtocolVersion(SECURITY_SCANNER_PROTOCOL_VERSION) catch {
        expect(false);
        return;
    };

    let bad_ver = false;
    validateScannerProtocolVersion("2") catch |e| {
        expect(e == SecurityScannerError.InvalidVersion);
        bad_ver = true;
    };
    expect(bad_ver);

    expect(strEq(SecurityScannerErrorCode.module_not_found.asCode(), "MODULE_NOT_FOUND"));
    expect(strEq(SecurityScannerErrorCode.invalid_version.asCode(), "INVALID_VERSION"));
    expect(strEq(SecurityScannerErrorCode.scan_failed.asCode(), "SCAN_FAILED"));

    const lf = SecurityAdvisoryLevel.parse("fatal") catch {
        expect(false);
        return;
    };
    expect(lf == SecurityAdvisoryLevel.fatal);
    expect(strEq(lf.asKeyword(), "fatal"));
    expect(strEq(lf.asLabel(), "FATAL"));

    const lw = SecurityAdvisoryLevel.parse("warn") catch {
        expect(false);
        return;
    };
    expect(lw == SecurityAdvisoryLevel.warn);
    expect(strEq(lw.asKeyword(), "warn"));
    expect(strEq(lw.asLabel(), "WARNING"));

    let bad_lvl = false;
    * = SecurityAdvisoryLevel.parse("high") catch |e| {
        expect(e == SecurityScannerError.InvalidLevelValue);
        bad_lvl = true;
    };
    expect(bad_lvl);
}

test("pkg security_scanner: strict JSON advisories array & wrapper object parsing") {
    const raw_array =
        "[" ++
        "{\"package\":\"evil-pkg\",\"level\":\"fatal\",\"description\":\"Malicious postinstall\",\"url\":\"https://sec.example/1\"}," ++
        "{\"package\":\"old-lib\",\"level\":\"warn\",\"description\":null,\"url\":null,\"extra\":{\"nested\":[1,2,true]}}" ++
        "]";

    const res = parseSecurityAdvisoriesJson(raw_array, "@acme/scanner") catch {
        expect(false);
        return;
    };
    expect(res.count() == 2);
    expect(res.fatal_count == 1);
    expect(res.warn_count == 1);
    expect(res.hasFatalAdvisories());
    expect(res.hasWarnings());
    expect(res.hasAdvisories());
    expect(strEq(res.scanner_name, "@acme/scanner"));

    if (a0 := res.advisoryAt(0)) {
        expect(a0.level == SecurityAdvisoryLevel.fatal);
        expect(strEq(a0.package, "evil-pkg"));
        if (d := a0.description) { expect(strEq(d, "Malicious postinstall")); } else { expect(false); }
        if (u := a0.url) { expect(strEq(u, "https://sec.example/1")); } else { expect(false); }
    } else {
        expect(false);
    }

    if (a1 := res.advisoryAt(1)) {
        expect(a1.level == SecurityAdvisoryLevel.warn);
        expect(strEq(a1.package, "old-lib"));
        expect(a1.description == null);
        expect(a1.url == null);
    } else {
        expect(false);
    }
    expect(res.advisoryAt(2) == null);

    // Wrapper object `{ "version": 1, "advisories": [] }`
    const wrapped = "{\"version\": 1, \"advisories\": []}";
    const res_empty = parseSecurityAdvisoriesJson(wrapped, "@acme/scanner") catch {
        expect(false);
        return;
    };
    expect(!res_empty.hasAdvisories());
    expect(res_empty.count() == 0);
}

test("pkg security_scanner: TTY vs CI install decision matrix & prompt confirmation") {
    // Clean scan -> proceed everywhere
    const clean = SecurityScanResults.init("scanner");
    expect(clean.evaluateInstallDecision(true, false) == SecurityInstallDecision.proceed);
    expect(clean.evaluateInstallDecision(false, true) == SecurityInstallDecision.proceed);

    // Warn-only -> prompt in interactive non-CI TTY, cancel in CI or non-TTY
    let warn_only = SecurityScanResults.init("scanner");
    warn_only.addAdvisory(SecurityAdvisory{
        level: SecurityAdvisoryLevel.warn,
        package: "deprecated-pkg",
    }) catch {
        expect(false);
        return;
    };
    expect(warn_only.evaluateInstallDecision(true, false) == SecurityInstallDecision.prompt_user);
    expect(warn_only.evaluateInstallDecision(false, false) == SecurityInstallDecision.cancel_non_interactive_warn);
    expect(warn_only.evaluateInstallDecision(true, true) == SecurityInstallDecision.cancel_non_interactive_warn);
    expect(warn_only.evaluateInstallDecision(false, true) == SecurityInstallDecision.cancel_non_interactive_warn);

    // Fatal + warn -> cancel_fatal everywhere
    warn_only.addAdvisory(SecurityAdvisory{
        level: SecurityAdvisoryLevel.fatal,
        package: "compromised-pkg",
    }) catch {
        expect(false);
        return;
    };
    expect(warn_only.evaluateInstallDecision(true, false) == SecurityInstallDecision.cancel_fatal);
    expect(warn_only.evaluateInstallDecision(false, true) == SecurityInstallDecision.cancel_fatal);

    // Strict prompt confirmation semantics
    expect(parseWarningConfirmation("y"));
    expect(parseWarningConfirmation("Y"));
    expect(parseWarningConfirmation("y\n"));
    expect(parseWarningConfirmation("Y\n"));
    expect(parseWarningConfirmation("y\r\n"));
    expect(parseWarningConfirmation("Y\r\n"));
    expect(!parseWarningConfirmation(""));
    expect(!parseWarningConfirmation("\n"));
    expect(!parseWarningConfirmation("\r\n"));
    expect(!parseWarningConfirmation("n\n"));
    expect(!parseWarningConfirmation("yes\n"));
    expect(!parseWarningConfirmation("y \n"));
}

test("pkg security_scanner: summary pluralization & full dependency-chain report formatting") {
    let buf: [512]u8 = [0; 512];

    let r0 = SecurityScanResults.init("scanner");
    const n0 = r0.formatSummaryLine(buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n0], "0 advisories"));

    // 1 fatal
    let adv1 = SecurityAdvisory{
        level: SecurityAdvisoryLevel.fatal,
        package: "bad-leaf",
        description: "Critical CVE-2026-0001",
        url: "https://cve.example/2026-0001",
    };
    adv1.addAncestor("app-framework") catch { expect(false); return; };
    adv1.addAncestor("sub-router") catch { expect(false); return; };
    expect(!adv1.isDirectDependency());
    r0.addAdvisory(adv1) catch { expect(false); return; };

    const n1 = r0.formatSummaryLine(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "1 advisory (1 fatal)"));

    // 1 fatal + 1 warning -> singular "1 warning"
    r0.addAdvisory(SecurityAdvisory{
        level: SecurityAdvisoryLevel.warn,
        package: "direct-lib",
        description: "Unmaintained",
    }) catch { expect(false); return; };
    const n2 = r0.formatSummaryLine(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "2 advisories (1 fatal, 1 warning)"));

    // 1 fatal + 2 warnings -> plural "2 warnings"
    r0.addAdvisory(SecurityAdvisory{
        level: SecurityAdvisoryLevel.warn,
        package: "another-lib",
    }) catch { expect(false); return; };
    const n3 = r0.formatSummaryLine(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "3 advisories (1 fatal, 2 warnings)"));

    // Full multi-line report
    const rn = r0.formatReport(buf[0..]) catch { expect(false); return; };
    const report = buf[0..rn];
    expect(containsStr(report, "FATAL: bad-leaf"));
    expect(containsStr(report, "via app-framework > sub-router > bad-leaf"));
    expect(containsStr(report, "Critical CVE-2026-0001"));
    expect(containsStr(report, "https://cve.example/2026-0001"));
    expect(containsStr(report, "WARNING: direct-lib"));
    expect(containsStr(report, "(direct dependency)"));
    expect(containsStr(report, "3 advisories (1 fatal, 2 warnings)"));
}

test("pkg security_scanner: malformed JSON & schema validation defenses") {
    // Non-array root
    let err1 = false;
    * = parseSecurityAdvisoriesJson("123", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidAdvisoriesFormat);
        err1 = true;
    };
    expect(err1);

    // Trailing garbage
    let err2 = false;
    * = parseSecurityAdvisoriesJson("[] trailing", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidAdvisoriesFormat);
        err2 = true;
    };
    expect(err2);

    // Non-object item in array
    let err3 = false;
    * = parseSecurityAdvisoriesJson("[\"not-obj\"]", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidAdvisoryFormat);
        err3 = true;
    };
    expect(err3);

    // Missing package field
    let err4 = false;
    * = parseSecurityAdvisoriesJson("[{\"level\":\"fatal\"}]", "s") catch |e| {
        expect(e == SecurityScannerError.MissingPackageField);
        err4 = true;
    };
    expect(err4);

    // Empty package field
    let err5 = false;
    * = parseSecurityAdvisoriesJson("[{\"package\":\"\",\"level\":\"warn\"}]", "s") catch |e| {
        expect(e == SecurityScannerError.EmptyPackageField);
        err5 = true;
    };
    expect(err5);

    // Non-string package field
    let err6 = false;
    * = parseSecurityAdvisoriesJson("[{\"package\":42,\"level\":\"warn\"}]", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidPackageField);
        err6 = true;
    };
    expect(err6);

    // Missing level field
    let err7 = false;
    * = parseSecurityAdvisoriesJson("[{\"package\":\"foo\"}]", "s") catch |e| {
        expect(e == SecurityScannerError.MissingLevelField);
        err7 = true;
    };
    expect(err7);

    // Invalid level value
    let err8 = false;
    * = parseSecurityAdvisoriesJson("[{\"package\":\"foo\",\"level\":\"critical\"}]", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidLevelValue);
        err8 = true;
    };
    expect(err8);

    // Invalid description type (number instead of string|null)
    let err9 = false;
    * = parseSecurityAdvisoriesJson("[{\"package\":\"foo\",\"level\":\"warn\",\"description\":99}]", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidDescriptionField);
        err9 = true;
    };
    expect(err9);

    // Invalid url type (boolean instead of string|null)
    let err10 = false;
    * = parseSecurityAdvisoriesJson("[{\"package\":\"foo\",\"level\":\"warn\",\"url\":true}]", "s") catch |e| {
        expect(e == SecurityScannerError.InvalidUrlField);
        err10 = true;
    };
    expect(err10);
}

test("pkg security_scanner: request JSON escaping, capacity limits & buffer guards") {
    const pkgs: [2]SecurityScanPackageInput = [
        SecurityScanPackageInput{
            name: "@acme/sdk",
            version: "1.2.3",
            tarball: "https://registry.npmjs.org/@acme/sdk/-/sdk-1.2.3.tgz",
            requested_range: "^1.0.0",
        },
        SecurityScanPackageInput{
            name: "quote\"pkg\\line\n",
            version: "0.0.1",
            tarball: "https://example.com/a.tgz",
            requested_range: "*",
        },
    ];
    let json_buf: [512]u8 = [0; 512];
    const jn = formatScanRequestJson(pkgs[0..], json_buf[0..]) catch {
        expect(false);
        return;
    };
    const json_str = json_buf[0..jn];
    expect(containsStr(json_str, "\"name\":\"@acme/sdk\""));
    expect(containsStr(json_str, "\"requestedRange\":\"^1.0.0\""));
    expect(containsStr(json_str, "quote\\\"pkg\\\\line\\n"));

    // BufferTooSmall protection on JSON serialization & report
    let tiny: [8]u8 = [0; 8];
    let small_json = false;
    * = formatScanRequestJson(pkgs[0..], tiny[0..]) catch |e| {
        expect(e == SecurityScannerError.BufferTooSmall);
        small_json = true;
    };
    expect(small_json);

    // Ancestor depth limit
    let adv = SecurityAdvisory{ level: SecurityAdvisoryLevel.warn, package: "leaf" };
    let d: usize = 0;
    while (d < SECURITY_SCANNER_MAX_PATH_DEPTH) {
        adv.addAncestor("parent") catch { expect(false); return; };
        d += 1;
    }
    let depth_err = false;
    adv.addAncestor("overflow") catch |e| {
        expect(e == SecurityScannerError.TooManyPathSegments);
        depth_err = true;
    };
    expect(depth_err);

    // Advisory capacity limit
    let res = SecurityScanResults.init("scanner");
    let k: usize = 0;
    while (k < SECURITY_SCANNER_MAX_ADVISORIES) {
        res.addAdvisory(SecurityAdvisory{ level: SecurityAdvisoryLevel.warn, package: "p" }) catch {
            expect(false);
            return;
        };
        k += 1;
    }
    let cap_err = false;
    res.addAdvisory(SecurityAdvisory{ level: SecurityAdvisoryLevel.fatal, package: "overflow" }) catch |e| {
        expect(e == SecurityScannerError.TooManyAdvisories);
        cap_err = true;
    };
    expect(cap_err);
}
