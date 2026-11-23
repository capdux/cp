// Source: cp/cap/test/test_lcov_report_production_deep.cp (Production deep tests for cap:lcov_report)

import { test, expect_eq } from "cap:test";
import {
    LineHit,
    CoverageThresholds,
    FileCoverage,
    parseLcovRecord,
} from "cap:obs";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const diff = if (a >= b) a - b else b - a;
    return diff <= eps;
}

test("lcov_report: recordLineHit accumulation, hit transition and percentage calculation") {
    let cov = FileCoverage.init("src/math.cp");
    expect_eq(cov.recordLineHit(10, 0), true);
    expect_eq(cov.recordLineHit(11, 5), true);
    expect_eq(cov.recordLineHit(12, 0), true);
    expect_eq(cov.recordLineHit(13, 2), true);

    expect_eq(cov.lines_found, 4);
    expect_eq(cov.lines_hit, 2);
    expect_eq(approxEq(cov.lineCoveragePct(), 50.0, 0.001), true);

    // 同号行从 0 次累加到 3 次，lines_hit 自动 +1，lines_found 不变
    expect_eq(cov.recordLineHit(10, 3), true);
    expect_eq(cov.lines_found, 4);
    expect_eq(cov.lines_hit, 3);
    expect_eq(cov.lines[0].hits, 3);
    expect_eq(approxEq(cov.lineCoveragePct(), 75.0, 0.001), true);

    // 再次累加已命中行：命中数累加，lines_hit 不重复加
    expect_eq(cov.recordLineHit(11, 4), true);
    expect_eq(cov.lines[1].hits, 9);
    expect_eq(cov.lines_hit, 3);

    // 非法行号 0 必须拒绝
    expect_eq(cov.recordLineHit(0, 1), false);
}

test("lcov_report: thresholds evaluation and empty file 100% convention") {
    // 零行文件默认 100% 覆盖率
    const empty = FileCoverage.init("src/empty.cp");
    expect_eq(approxEq(empty.lineCoveragePct(), 100.0, 0.001), true);
    expect_eq(approxEq(empty.functionCoveragePct(), 100.0, 0.001), true);
    expect_eq(approxEq(empty.branchCoveragePct(), 100.0, 0.001), true);
    expect_eq(
        empty.meetsThresholds(CoverageThresholds{ lines_pct: 100.0, functions_pct: 100.0, branches_pct: 100.0 }),
        true
    );

    let cov = FileCoverage.init("src/core.cp");
    * = cov.recordLineHit(1, 1);
    * = cov.recordLineHit(2, 1);
    * = cov.recordLineHit(3, 1);
    * = cov.recordLineHit(4, 0); // 75% lines
    cov.functions_found = 5;
    cov.functions_hit = 4; // 80% functions
    cov.branches_found = 10;
    cov.branches_hit = 9; // 90% branches

    expect_eq(
        cov.meetsThresholds(CoverageThresholds{ lines_pct: 75.0, functions_pct: 80.0, branches_pct: 85.0 }),
        true
    );
    expect_eq(
        cov.meetsThresholds(CoverageThresholds{ lines_pct: 80.0, functions_pct: 80.0, branches_pct: 85.0 }),
        false
    );
    expect_eq(
        cov.meetsThresholds(CoverageThresholds{ lines_pct: 70.0, functions_pct: 85.0, branches_pct: 85.0 }),
        false
    );
}

test("lcov_report: formatLcov -> parseLcovRecord exact round-trip") {
    let orig = FileCoverage.init("lib/http/router.cp");
    * = orig.recordLineHit(5, 12);
    * = orig.recordLineHit(8, 0);
    * = orig.recordLineHit(14, 4);
    orig.functions_found = 3;
    orig.functions_hit = 2;
    orig.branches_found = 6;
    orig.branches_hit = 5;

    let buf: [512]u8 = [0; 512];
    const formatted = orig.formatLcov(buf[0..]).?;
    const expected =
        "SF:lib/http/router.cp\n" ++
        "FNF:3\n" ++
        "FNH:2\n" ++
        "BRF:6\n" ++
        "BRH:5\n" ++
        "DA:5,12\n" ++
        "DA:8,0\n" ++
        "DA:14,4\n" ++
        "LF:3\n" ++
        "LH:2\n" ++
        "end_of_record\n";
    expect_eq(std.mem.eql(u8, formatted, expected), true);

    const parsed = parseLcovRecord(formatted).?;
    expect_eq(std.mem.eql(u8, parsed.source_file, "lib/http/router.cp"), true);
    expect_eq(parsed.line_count, 3);
    expect_eq(parsed.lines[0].line_no, 5);
    expect_eq(parsed.lines[0].hits, 12);
    expect_eq(parsed.lines[1].line_no, 8);
    expect_eq(parsed.lines[1].hits, 0);
    expect_eq(parsed.lines[2].line_no, 14);
    expect_eq(parsed.lines[2].hits, 4);
    expect_eq(parsed.lines_found, 3);
    expect_eq(parsed.lines_hit, 2);
    expect_eq(parsed.functions_found, 3);
    expect_eq(parsed.functions_hit, 2);
    expect_eq(parsed.branches_found, 6);
    expect_eq(parsed.branches_hit, 5);
}

test("lcov_report: parseLcovRecord handles optional TN/FN/FNDA/BRDA tags and CRLF") {
    const raw =
        "TN:unit_suite\r\n" ++
        "SF:src/parser.cp\r\n" ++
        "FN:1,parse\r\n" ++
        "FNDA:7,parse\r\n" ++
        "FNF:1\r\n" ++
        "FNH:1\r\n" ++
        "BRDA:3,0,0,7\r\n" ++
        "DA:1,7,abc123checksum\r\n" ++
        "DA:2,0\r\n" ++
        "LF:2\r\n" ++
        "LH:1\r\n" ++
        "end_of_record\r\n";

    const cov = parseLcovRecord(raw).?;
    expect_eq(std.mem.eql(u8, cov.source_file, "src/parser.cp"), true);
    expect_eq(cov.lines_found, 2);
    expect_eq(cov.lines_hit, 1);
    expect_eq(cov.functions_found, 1);
    expect_eq(cov.functions_hit, 1);
}

test("lcov_report: malformed LCOV records and small output buffer guards") {
    // 缺失 SF: 或缺失 end_of_record 或畸形 DA: 行
    expect_eq(parseLcovRecord("") == null, true);
    expect_eq(parseLcovRecord("DA:1,1\nend_of_record\n") == null, true);
    expect_eq(parseLcovRecord("SF:a.cp\nDA:1,1\n") == null, true);
    expect_eq(parseLcovRecord("SF:\nend_of_record\n") == null, true);
    expect_eq(parseLcovRecord("SF:a.cp\nDA:bad,1\nend_of_record\n") == null, true);
    expect_eq(parseLcovRecord("SF:a.cp\nDA:0,1\nend_of_record\n") == null, true);
    expect_eq(parseLcovRecord("SF:a.cp\nUNKNOWN:foo\nend_of_record\n") == null, true);

    // 过小输出缓冲区安全返回 null
    let cov = FileCoverage.init("src/big.cp");
    * = cov.recordLineHit(1, 100);
    let tiny: [8]u8 = [0; 8];
    expect_eq(cov.formatLcov(tiny[0..]) == null, true);
}
