import { test, expect_eq } from "cap:test";
import {
    CoverageMetric,
    CoverageSummary,
    LineHit,
    formatLcovRecord,
    parseLcovSummary,
} from "../../packages/istanbul-lib-coverage/src/index.cp";

fn main() {
    test("1. @cap/istanbul-lib-coverage — CoverageMetric pct and empty 100%", () => {
        const m = CoverageMetric{ total: 4, covered: 3, skipped: 0 };
        expect_eq(m.pct(), 75.0);

        const empty = CoverageMetric{ total: 0, covered: 0, skipped: 0 };
        expect_eq(empty.pct(), 100.0);

        const zero = CoverageMetric{ total: 5, covered: 0, skipped: 1 };
        expect_eq(zero.pct(), 0.0);
    });

    test("2. @cap/istanbul-lib-coverage — CoverageMetric merge", () => {
        const a = CoverageMetric{ total: 10, covered: 8, skipped: 1 };
        const b = CoverageMetric{ total: 10, covered: 6, skipped: 2 };
        const merged = a.merge(b);
        expect_eq(merged.total, 20 as u32);
        expect_eq(merged.covered, 14 as u32);
        expect_eq(merged.skipped, 3 as u32);
        expect_eq(merged.pct(), 70.0);
    });

    test("3. @cap/istanbul-lib-coverage — CoverageSummary merge and isAboveThreshold", () => {
        const s1 = CoverageSummary{
            lines: CoverageMetric{ total: 10, covered: 9, skipped: 0 },
            statements: CoverageMetric{ total: 20, covered: 18, skipped: 0 },
            functions: CoverageMetric{ total: 4, covered: 4, skipped: 0 },
            branches: CoverageMetric{ total: 8, covered: 6, skipped: 0 },
        };
        const s2 = CoverageSummary{
            lines: CoverageMetric{ total: 10, covered: 7, skipped: 0 },
            statements: CoverageMetric{ total: 20, covered: 14, skipped: 0 },
            functions: CoverageMetric{ total: 4, covered: 4, skipped: 0 },
            branches: CoverageMetric{ total: 8, covered: 6, skipped: 0 },
        };
        const merged = s1.merge(s2);
        expect_eq(merged.lines.total, 20 as u32);
        expect_eq(merged.lines.covered, 16 as u32);
        expect_eq(merged.lines.pct(), 80.0);
        expect_eq(merged.branches.pct(), 75.0);
        expect_eq(merged.isAboveThreshold(75.0), true);
        expect_eq(merged.isAboveThreshold(80.0), false);
    });

    test("4. @cap/istanbul-lib-coverage — formatLcovRecord produces standard LCOV", () => {
        let hits: [3]LineHit = undefined;
        hits[0] = LineHit{ line: 1, hits: 3 };
        hits[1] = LineHit{ line: 2, hits: 0 };
        hits[2] = LineHit{ line: 5, hits: 1 };
        let buf: [256]u8 = undefined;
        const n = formatLcovRecord("src/app.cp", hits[0..3], buf[0..256]);
        const expected = "SF:src/app.cp\nDA:1,3\nDA:2,0\nDA:5,1\nLF:3\nLH:2\nend_of_record\n";
        expect_eq(buf[0..n], expected);
    });

    test("5. @cap/istanbul-lib-coverage — parseLcovSummary round-trip and multi-record", () => {
        const lcov = "SF:a.cp\nDA:1,2\nDA:2,0\nLF:2\nLH:1\nend_of_record\nSF:b.cp\nDA:10,5\nDA:11,1\nDA:12,0\nLF:3\nLH:2\nend_of_record\n";
        const metric = parseLcovSummary(lcov);
        expect_eq(metric.total, 5 as u32);
        expect_eq(metric.covered, 3 as u32);
        expect_eq(metric.pct(), 60.0);
    });

    test("6. @cap/istanbul-lib-coverage — parseLcovSummary supports DA-only and LF/LH-only", () => {
        const da_only = "SF:only_da.cp\nDA:1,1\nDA:2,0\nDA:3,4\nDA:4,0\n";
        const m1 = parseLcovSummary(da_only);
        expect_eq(m1.total, 4 as u32);
        expect_eq(m1.covered, 2 as u32);
        expect_eq(m1.pct(), 50.0);

        const lf_only = "SF:summary.cp\nLF:10\nLH:8\nend_of_record\n";
        const m2 = parseLcovSummary(lf_only);
        expect_eq(m2.total, 10 as u32);
        expect_eq(m2.covered, 8 as u32);
        expect_eq(m2.pct(), 80.0);
    });
}
