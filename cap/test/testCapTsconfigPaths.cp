// cap/test/testCapTsconfigPaths.cp
// Comprehensive unit tests for @cap/tsconfig-paths

import { test, expect_eq } from "cap:test";
import {
    PathRule,
    PatternMatch,
    CandidateList,
    PathsMatcher,
    matchesPattern,
    resolvePath,
    resolveAllCandidates,
} from "../../packages/@cap/tsconfig-paths/src/index.cp";

fn main() {
    test("1. @cap/tsconfig-paths — Exact match takes precedence over wildcard '*'", () => {
        let m = PathsMatcher.init("src");
        m.addRule("@app/*", "app/*");
        m.addRule("@app/config", "config/override.ts");

        let out: [256]u8 = undefined;
        expect_eq(resolvePath(m, "@app/config", out[0..256]) ?? "", "src/config/override.ts");
        expect_eq(resolvePath(m, "@app/utils/log", out[0..256]) ?? "", "src/app/utils/log");
    });

    test("2. @cap/tsconfig-paths — Longest static prefix wins among wildcards", () => {
        let m = PathsMatcher.init("./src");
        m.addRule("*", "vendor/*");
        m.addRule("@ui/*", "ui/*");
        m.addRule("@ui/icons/*", "assets/svg-icons/*");

        let out: [256]u8 = undefined;
        expect_eq(resolvePath(m, "@ui/icons/arrow", out[0..256]) ?? "", "src/assets/svg-icons/arrow");
        expect_eq(resolvePath(m, "@ui/button", out[0..256]) ?? "", "src/ui/button");
        expect_eq(resolvePath(m, "lodash", out[0..256]) ?? "", "src/vendor/lodash");
    });

    test("3. @cap/tsconfig-paths — matchesPattern exact, prefix/suffix wildcard & capture", () => {
        const m1 = matchesPattern("@core/env", "@core/env");
        expect_eq(m1.matched, true);
        expect_eq(m1.is_exact, true);
        expect_eq(m1.captured, "");

        const m2 = matchesPattern("@pkg/*.json", "@pkg/i18n/en.json");
        expect_eq(m2.matched, true);
        expect_eq(m2.is_exact, false);
        expect_eq(m2.prefix_len, 5 as usize);
        expect_eq(m2.captured, "i18n/en");

        expect_eq(matchesPattern("@pkg/*.json", "@pkg/i18n/en.ts").matched, false);
    });

    test("4. @cap/tsconfig-paths — Multi-target fallback candidates via resolveAllCandidates", () => {
        let m = PathsMatcher.init("src");
        m.addRuleMulti("@shared/*", "./local-shared/*", "../packages/shared/*");

        let out: [512]u8 = undefined;
        const cands = resolveAllCandidates(m, "@shared/math/vec", out[0..512]);
        expect_eq(cands.count, 2 as usize);
        expect_eq(cands.itemAt(0) ?? "", "src/local-shared/math/vec");
        expect_eq(cands.itemAt(1) ?? "", "packages/shared/math/vec");
        expect_eq(cands.itemAt(2) == null, true);
    });

    test("5. @cap/tsconfig-paths — baseUrl normalization (slashes, ./ and ..)", () => {
        let m = PathsMatcher.init("/workspace/app//src/");
        m.addRule("@lib/*", "./lib//core/*");

        let out: [256]u8 = undefined;
        expect_eq(resolvePath(m, "@lib/http", out[0..256]) ?? "", "/workspace/app/src/lib/core/http");

        // Dot baseUrl "." omits leading "./"
        let m2 = PathsMatcher.init(".");
        m2.addRule("@/*", "./src/*");
        expect_eq(resolvePath(m2, "@/index", out[0..256]) ?? "", "src/index");
    });

    test("6. @cap/tsconfig-paths — Unmatched specifiers return null / empty list", () => {
        let m = PathsMatcher.init("src");
        m.addRule("@app/*", "app/*");

        let out: [256]u8 = undefined;
        expect_eq(resolvePath(m, "react", out[0..256]) == null, true);
        expect_eq(resolvePath(m, "", out[0..256]) == null, true);

        const cands = resolveAllCandidates(m, "react", out[0..256]);
        expect_eq(cands.count, 0 as usize);
    });
}
