// cap/test/test_cli_test_scanner_production_deep.cp
// Production-deep regression tests for cap:cli test_scanner module
// Source: /data/dev/bun/src/cli/test/Scanner.cp & /data/dev/bun/docs/test/discovery.mdx

import { test, expect } from "cap:test";
import {
    TEST_SCANNER_MAX_FILTERS,
    TEST_SCANNER_MAX_EXCLUSIONS,
    TEST_SCANNER_MAX_IGNORE_PATTERNS,
    TestScannerError,
    isTestSupportedExtension,
    hasTestNameSuffix,
    couldBeTestFile,
    isExactTestPathArg,
    formatScopedTestName,
    TestFileScanner,
} from "cap:cli";

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

test("cli test_scanner: default test suffixes, extensions & exact-path arg detection") {
    expect(isTestSupportedExtension(".ts"));
    expect(isTestSupportedExtension(".TSX"));
    expect(isTestSupportedExtension(".cp"));
    expect(!isTestSupportedExtension(".json"));
    expect(!isTestSupportedExtension(".md"));

    expect(hasTestNameSuffix("src/math.test.ts"));
    expect(hasTestNameSuffix("src/math_test.js"));
    expect(hasTestNameSuffix("src/math.spec.tsx"));
    expect(hasTestNameSuffix("src/math_spec.cp"));
    expect(!hasTestNameSuffix("src/math.ts"));
    expect(!hasTestNameSuffix("src/test.ts"));
    expect(!hasTestNameSuffix(".test.ts"));

    // needs_test_suffix=true vs false (explicit file path mode)
    expect(couldBeTestFile("src/app.test.ts", true));
    expect(!couldBeTestFile("src/app.ts", true));
    expect(couldBeTestFile("src/app.ts", false));
    expect(!couldBeTestFile("src/readme.md", false));

    // Exact path vs substring filter argument
    expect(isExactTestPathArg("./test/foo.test.ts"));
    expect(isExactTestPathArg("../foo.test.ts"));
    expect(isExactTestPathArg("/abs/foo.test.ts"));
    expect(!isExactTestPathArg("utils"));
    expect(!isExactTestPathArg("src/utils"));
    expect(!isExactTestPathArg(""));
}

test("cli test_scanner: hidden/node_modules/custom directory exclusions & ** vs * pruning") {
    let sc = TestFileScanner.init();
    sc.addExclusionDir("dist") catch { expect(false); return; };
    sc.addPathIgnorePattern("vendor/**") catch { expect(false); return; };
    sc.addPathIgnorePattern("fixtures/*") catch { expect(false); return; };

    // Default hidden & node_modules skipped
    expect(!sc.shouldScanDir(".git", ".git"));
    expect(!sc.shouldScanDir(".cache", "src/.cache"));
    expect(!sc.shouldScanDir("node_modules", "node_modules"));
    expect(!sc.shouldScanDir("dist", "dist"));
    expect(sc.shouldScanDir("src", "src"));

    // `vendor/**` prunes directory early
    expect(!sc.shouldScanDir("vendor", "vendor"));
    expect(!sc.shouldScanDir("sub", "vendor/sub"));

    // `fixtures/*` (single star) MUST NOT prune `fixtures` directory itself
    expect(sc.shouldScanDir("fixtures", "fixtures"));
    // ...but files directly inside `fixtures/*` are still ignored
    expect(!sc.shouldIncludeFile("/repo/fixtures/a.test.ts", "fixtures/a.test.ts", true));
    expect(sc.shouldIncludeFile("/repo/fixtures/deep/a.test.ts", "fixtures/deep/a.test.ts", true));
}

test("cli test_scanner: positional substring & absolute prefix filters") {
    let sc = TestFileScanner.init();
    sc.addFilter("utils") catch { expect(false); return; };
    sc.addFilter("/repo/core/") catch { expect(false); return; };

    expect(sc.shouldIncludeFile("/repo/src/utils/str.test.ts", "src/utils/str.test.ts", true));
    expect(sc.shouldIncludeFile("/repo/lib/utils/arr_test.js", "lib/utils/arr_test.js", true));
    expect(sc.shouldIncludeFile("/repo/core/engine.spec.cp", "core/engine.spec.cp", true));
    expect(!sc.shouldIncludeFile("/repo/src/ui/btn.test.ts", "src/ui/btn.test.ts", true));
    // Non-test file rejected even if filter matches
    expect(!sc.shouldIncludeFile("/repo/src/utils/helper.ts", "src/utils/helper.ts", true));
}

test("cli test_scanner: path_ignore_patterns overrides filter match") {
    let sc = TestFileScanner.init();
    sc.addFilter("api") catch { expect(false); return; };
    sc.addPathIgnorePattern("**/legacy/**") catch { expect(false); return; };

    expect(sc.shouldIncludeFile("/repo/src/api/auth.test.ts", "src/api/auth.test.ts", true));
    expect(!sc.shouldIncludeFile("/repo/src/legacy/api/old.test.ts", "src/legacy/api/old.test.ts", true));
}

test("cli test_scanner: describe-chain test name formatting (-t / --test-name-pattern)") {
    const labels: [2]([]const u8) = ["Math", "operations"];
    let buf: [64]u8 = [0; 64];
    const n1 = formatScopedTestName(labels[0..], "should add correctly", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "Math operations should add correctly"));

    const empty_labels: [0]([]const u8) = [];
    const n2 = formatScopedTestName(empty_labels[0..], "standalone test", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "standalone test"));

    let tiny: [8]u8 = [0; 8];
    let got_small = false;
    * = formatScopedTestName(labels[0..], "should add", tiny[0..]) catch |e| {
        expect(e == TestScannerError.BufferTooSmall);
        got_small = true;
    };
    expect(got_small);
}

test("cli test_scanner: malformed empty patterns & capacity limit guards") {
    let sc = TestFileScanner.init();

    let e1 = false;
    sc.addFilter("") catch |e| {
        expect(e == TestScannerError.EmptyPattern);
        e1 = true;
    };
    expect(e1);

    let e2 = false;
    sc.addExclusionDir("") catch |e| {
        expect(e == TestScannerError.EmptyPattern);
        e2 = true;
    };
    expect(e2);

    let e3 = false;
    sc.addPathIgnorePattern("") catch |e| {
        expect(e == TestScannerError.EmptyPattern);
        e3 = true;
    };
    expect(e3);

    // Fill filters to TEST_SCANNER_MAX_FILTERS (32) then verify overflow
    expect(TEST_SCANNER_MAX_FILTERS == 32);
    expect(TEST_SCANNER_MAX_EXCLUSIONS == 32);
    expect(TEST_SCANNER_MAX_IGNORE_PATTERNS == 32);

    let i: usize = 0;
    while (i < TEST_SCANNER_MAX_FILTERS) {
        sc.addFilter("x") catch { expect(false); return; };
        i += 1;
    }
    let overflow = false;
    sc.addFilter("y") catch |e| {
        expect(e == TestScannerError.TooManyFilters);
        overflow = true;
    };
    expect(overflow);
}
