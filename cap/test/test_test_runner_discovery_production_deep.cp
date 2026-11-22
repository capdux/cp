// cap/test/test_test_runner_discovery_production_deep.cp
// Deep production tests for cap:test_runner Test File Discovery & Pruning Engine (discovery.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    TEST_SUFFIX_DOT_TEST,
    TEST_SUFFIX_UNDER_TEST,
    TEST_SUFFIX_DOT_SPEC,
    TEST_SUFFIX_UNDER_SPEC,
    TEST_DEFAULT_IGNORE_DIR,
    isSupportedTestExt,
    hasBunTestSuffix,
    hasTestNameSuffix,
    isExplicitPathArg,
    shouldSkipDirectory,
    matchesDiscoveryGlob,
    TestDiscoveryFilter,
} from "cap:test_runner";

test("test_runner.discovery: supported extensions and 4 Bun suffixes + CP prefix", () => {
    expect_eq(TEST_SUFFIX_DOT_TEST, ".test");
    expect_eq(TEST_SUFFIX_UNDER_TEST, "_test");
    expect_eq(TEST_SUFFIX_DOT_SPEC, ".spec");
    expect_eq(TEST_SUFFIX_UNDER_SPEC, "_spec");
    expect_eq(TEST_DEFAULT_IGNORE_DIR, "node_modules");

    expect_true(isSupportedTestExt(".cp"));
    expect_true(isSupportedTestExt(".CP"));
    expect_true(isSupportedTestExt(".ts"));
    expect_true(isSupportedTestExt(".tsx"));
    expect_true(isSupportedTestExt(".js"));
    expect_true(isSupportedTestExt(".jsx"));
    expect_true(isSupportedTestExt(".mjs"));
    expect_true(isSupportedTestExt(".cjs"));
    expect_true(isSupportedTestExt(".mts"));
    expect_true(isSupportedTestExt(".cts"));
    expect_false(isSupportedTestExt(".md"));
    expect_false(isSupportedTestExt(".json"));
    expect_false(isSupportedTestExt(""));

    // All 4 Bun suffixes
    expect_true(hasBunTestSuffix("src/math.test.ts"));
    expect_true(hasBunTestSuffix("src/math_test.js"));
    expect_true(hasBunTestSuffix("src/math.spec.tsx"));
    expect_true(hasBunTestSuffix("src/math_spec.cp"));
    expect_false(hasBunTestSuffix("src/math.ts"));
    expect_false(hasBunTestSuffix("src/test_math.cp"));

    // CP prefix test_*.cp enabled vs disabled
    expect_true(hasTestNameSuffix("cap/test/test_sql_pg_extended.cp", true));
    expect_false(hasTestNameSuffix("cap/test/test_sql_pg_extended.cp", false));
    expect_false(hasTestNameSuffix("cap/test/test_.cp", true));
    expect_false(hasTestNameSuffix("cap/test/test_math.ts", true));
});

test("test_runner.discovery: directory skip rules and explicit path detection", () => {
    const custom_ex: [2][]const u8 = ["fixtures", "dist"];
    expect_true(shouldSkipDirectory(".git", custom_ex[0..]));
    expect_true(shouldSkipDirectory(".cap", custom_ex[0..]));
    expect_true(shouldSkipDirectory("node_modules", custom_ex[0..]));
    expect_true(shouldSkipDirectory("fixtures", custom_ex[0..]));
    expect_true(shouldSkipDirectory("dist", custom_ex[0..]));
    expect_false(shouldSkipDirectory("src", custom_ex[0..]));
    expect_false(shouldSkipDirectory("tests", custom_ex[0..]));
    expect_false(shouldSkipDirectory("", custom_ex[0..]));

    // Explicit path arguments vs substring filters
    expect_true(isExplicitPathArg("./test/foo.test.ts"));
    expect_true(isExplicitPathArg("../shared/bar.test.cp"));
    expect_true(isExplicitPathArg("/data/dev/cp/cap/test/test_a.cp"));
    expect_false(isExplicitPathArg("utils"));
    expect_false(isExplicitPathArg("math.test.ts"));
    expect_false(isExplicitPathArg(""));
});

test("test_runner.discovery: glob ignore patterns and ** vs * directory pruning semantics", () => {
    // Double-star matches nested files AND prunes directory itself via trailing '/' retry
    expect_true(matchesDiscoveryGlob("vendor/**", "vendor/pkg/a.test.ts"));
    expect_true(matchesDiscoveryGlob("vendor/**", "vendor"));
    expect_true(matchesDiscoveryGlob("**/generated/**", "src/api/generated/schema.test.cp"));
    expect_true(matchesDiscoveryGlob("**/generated/**", "generated"));

    // Single-star MUST NOT cross '/' and MUST NOT prune entire directory
    expect_true(matchesDiscoveryGlob("vendor/*", "vendor/a.test.ts"));
    expect_false(matchesDiscoveryGlob("vendor/*", "vendor/sub/a.test.ts"));
    expect_false(matchesDiscoveryGlob("vendor/*", "vendor"));

    // Question mark matches single non-slash character
    expect_true(matchesDiscoveryGlob("test/v?/app.test.cp", "test/v2/app.test.cp"));
    expect_false(matchesDiscoveryGlob("test/v?/app.test.cp", "test/v10/app.test.cp"));
    expect_false(matchesDiscoveryGlob("test/v?/app.test.cp", "test/v//app.test.cp"));
    expect_false(matchesDiscoveryGlob("", "any/path.test.cp"));
});

test("test_runner.discovery: TestDiscoveryFilter positional substring & absolute prefix filtering", () => {
    const empty: [0][]const u8 = [];
    const filters: [2][]const u8 = ["utils", "/repo/packages/core/"];
    const disc = TestDiscoveryFilter{
        exclusion_names: empty[0..],
        filter_names: filters[0..],
        path_ignore_patterns: empty[0..],
        include_cp_prefix: true,
    };

    // Substring match on relative path
    expect_true(disc.shouldRunFile("/repo/src/utils/string.test.ts", "src/utils/string.test.ts", true));
    expect_true(disc.shouldRunFile("/repo/lib/utils/array_test.js", "lib/utils/array_test.js", true));

    // Absolute prefix match even when relative path does not contain "utils"
    expect_true(disc.shouldRunFile("/repo/packages/core/lexer.spec.cp", "packages/core/lexer.spec.cp", true));

    // Non-matching path rejected
    expect_false(disc.shouldRunFile("/repo/src/net/http.test.ts", "src/net/http.test.ts", true));

    // Non-test file rejected when needs_test_suffix=true, allowed when needs_test_suffix=false (explicit file mode)
    expect_false(disc.shouldRunFile("/repo/src/utils/helper.ts", "src/utils/helper.ts", true));
    expect_true(disc.shouldRunFile("/repo/src/utils/helper.ts", "src/utils/helper.ts", false));
});

test("test_runner.discovery: directory pruning combines exclusions and path_ignore_patterns", () => {
    const excludes: [1][]const u8 = ["__fixtures__"];
    const empty: [0][]const u8 = [];
    const ignores: [2][]const u8 = ["vendor/**", "tmp/*"];
    const disc = TestDiscoveryFilter{
        exclusion_names: excludes[0..],
        filter_names: empty[0..],
        path_ignore_patterns: ignores[0..],
        include_cp_prefix: true,
    };

    expect_true(disc.shouldPruneDirectory(".git", ".git"));
    expect_true(disc.shouldPruneDirectory("node_modules", "packages/a/node_modules"));
    expect_true(disc.shouldPruneDirectory("__fixtures__", "test/__fixtures__"));
    expect_true(disc.shouldPruneDirectory("vendor", "vendor"));
    // Single-star "tmp/*" must not prune "tmp" directory itself
    expect_false(disc.shouldPruneDirectory("tmp", "tmp"));
    expect_false(disc.shouldPruneDirectory("src", "src"));

    // Files inside vendor/** are ignored; direct file in tmp/* is ignored, nested file in tmp/sub/ is not
    expect_false(disc.shouldRunFile("/repo/vendor/x.test.ts", "vendor/x.test.ts", true));
    expect_false(disc.shouldRunFile("/repo/tmp/a.test.ts", "tmp/a.test.ts", true));
    expect_true(disc.shouldRunFile("/repo/tmp/sub/a.test.ts", "tmp/sub/a.test.ts", true));
});

test("test_runner.discovery: edge cases with dotfiles, empty filters, and mixed slashes", () => {
    const empty: [0][]const u8 = [];
    const disc = TestDiscoveryFilter{
        exclusion_names: empty[0..],
        filter_names: empty[0..],
        path_ignore_patterns: empty[0..],
        include_cp_prefix: true,
    };

    // Empty filter list allows all valid test files
    expect_true(disc.doesPathMatchFilter("anything/at/all.test.cp"));
    expect_true(disc.doesAbsolutePathMatchFilter("/any/abs/path.test.cp"));
    expect_false(disc.matchesPathIgnorePattern("anything/at/all.test.cp"));

    // Dotfiles like ".test.ts" have no stem before ".test" -> extension is ".ts", stem is ".test"
    expect_true(hasBunTestSuffix(".test.ts"));
    expect_false(hasBunTestSuffix(".cp"));
    expect_false(hasBunTestSuffix("Makefile"));
    expect_false(hasBunTestSuffix("src/dir/"));
    expect_true(hasBunTestSuffix("src\\win\\math.SPEC.TS"));
});
