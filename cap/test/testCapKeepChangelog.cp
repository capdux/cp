// cap/test/testCapKeepChangelog.cp
// Comprehensive unit tests for @cap/keep-changelog

import { test, expect_eq } from "cap:test";
import {
    ChangeKind,
    classifySection,
    parseChangelog,
    findRelease,
    latestRelease,
    formatReleaseHeader,
} from "../../packages/keep-a-changelog/src/index.cp";

const SAMPLE_MD = "# Changelog\n\nAll notable changes to this project will be documented in this file.\n\n## [Unreleased]\n\n### Added\n- Streaming parser support\n\n## [1.2.0] - 2026-11-26\n\n### Added\n- Zero-allocation `@cap/jcs` package\n* `@cap/boxen` CLI border renderer\n\n### Changed\n- Improved error diagnostics\n\n### Fixed\n- Fixed edge case in header parsing\n- Fixed UTF-8 boundary check\n\n### Security\n- Hardened input length limits\n\n## [1.1.0] - 2026-10-01 [YANKED]\n\n### Fixed\n- Initial release bugfix\n\n[Unreleased]: https://github.com/example/repo/compare/v1.2.0...HEAD\n[1.2.0]: https://github.com/example/repo/compare/v1.1.0...v1.2.0\n";

fn main() {
    test("1. @cap/keep-changelog — parse Unreleased, standard release & YANKED release", () => {
        const log = parseChangelog(SAMPLE_MD);
        expect_eq(log.count, 3 as usize);

        const r0 = log.releaseAt(0 as usize);
        expect_eq(r0.version, "Unreleased");
        expect_eq(r0.is_unreleased, true);
        expect_eq(r0.is_yanked, false);
        expect_eq(r0.date, "");

        const r1 = log.releaseAt(1 as usize);
        expect_eq(r1.version, "1.2.0");
        expect_eq(r1.date, "2026-11-26");
        expect_eq(r1.is_unreleased, false);
        expect_eq(r1.is_yanked, false);

        const r2 = log.releaseAt(2 as usize);
        expect_eq(r2.version, "1.1.0");
        expect_eq(r2.date, "2026-10-01");
        expect_eq(r2.is_yanked, true);
    });

    test("2. @cap/keep-changelog — accurate Added/Changed/Fixed/Security entry counts", () => {
        const log = parseChangelog(SAMPLE_MD);

        const unrel = log.releaseAt(0 as usize);
        expect_eq(unrel.added_count, 1 as usize);
        expect_eq(unrel.fixed_count, 0 as usize);

        const r120 = log.releaseAt(1 as usize);
        expect_eq(r120.added_count, 2 as usize);
        expect_eq(r120.changed_count, 1 as usize);
        expect_eq(r120.fixed_count, 2 as usize);
        expect_eq(r120.security_count, 1 as usize);

        const r110 = log.releaseAt(2 as usize);
        expect_eq(r110.fixed_count, 1 as usize);
        // Bottom reference link definitions must not leak into last release body
        expect_eq(r110.body, "### Fixed\n- Initial release bugfix");
    });

    test("3. @cap/keep-changelog — findRelease & latestRelease lookup", () => {
        const log = parseChangelog(SAMPLE_MD);

        const latest = latestRelease(log);
        expect_eq(latest != null, true);
        expect_eq(latest.?.version, "1.2.0");
        expect_eq(latest.?.date, "2026-11-26");

        const found_unrel = findRelease(log, "unreleased");
        expect_eq(found_unrel != null, true);
        expect_eq(found_unrel.?.is_unreleased, true);

        const found_110 = findRelease(log, "1.1.0");
        expect_eq(found_110 != null, true);
        expect_eq(found_110.?.is_yanked, true);

        expect_eq(findRelease(log, "9.9.9") == null, true);
    });

    test("4. @cap/keep-changelog — classifySection for all Keep a Changelog kinds", () => {
        expect_eq(classifySection("Added"), ChangeKind.Added);
        expect_eq(classifySection("changed"), ChangeKind.Changed);
        expect_eq(classifySection("DEPRECATED"), ChangeKind.Deprecated);
        expect_eq(classifySection("Removed"), ChangeKind.Removed);
        expect_eq(classifySection("Fixed"), ChangeKind.Fixed);
        expect_eq(classifySection("Security"), ChangeKind.Security);
        expect_eq(classifySection("Notes"), ChangeKind.Other);
    });

    test("5. @cap/keep-changelog — formatReleaseHeader variants", () => {
        let buf: [128]u8 = undefined;
        expect_eq(formatReleaseHeader("1.2.0", "2026-11-26", false, buf[0..128]), "## [1.2.0] - 2026-11-26");
        expect_eq(formatReleaseHeader("1.1.0", "2026-10-01", true, buf[0..128]), "## [1.1.0] - 2026-10-01 [YANKED]");
        expect_eq(formatReleaseHeader("Unreleased", "", false, buf[0..128]), "## [Unreleased]");
        expect_eq(formatReleaseHeader("0.1.0", "", false, buf[0..128]), "## [0.1.0]");
    });

    test("6. @cap/keep-changelog — empty changelog & only-Unreleased edge cases", () => {
        const empty = parseChangelog("");
        expect_eq(empty.count, 0 as usize);
        expect_eq(latestRelease(empty) == null, true);
        expect_eq(findRelease(empty, "1.0.0") == null, true);

        const only_unrel = parseChangelog("## [Unreleased]\n### Added\n- Work in progress\n");
        expect_eq(only_unrel.count, 1 as usize);
        expect_eq(latestRelease(only_unrel) == null, true);
        expect_eq(findRelease(only_unrel, "Unreleased").?.added_count, 1 as usize);
    });
}
