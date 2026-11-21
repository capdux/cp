// Source: cp/cap/test/test_coverage_options_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Reporter,
    Reporters,
    CoverageFraction,
    CoverageMetrics,
    CodeCoverageOptions,
    formatLcovRecord,
} from "cap:coverage_options";

test "coverage_options: threshold evaluation & zero-total edge cases" {
    const opts = CodeCoverageOptions{
        .enabled = true,
        .fail_on_low_coverage = true,
        .fractions = CoverageFraction{ .lines = 0.80, .functions = 0.75, .branches = 0.50 },
    };
    const pass_m = CoverageMetrics{
        .lines_hit = 85,
        .lines_total = 100,
        .funcs_hit = 3,
        .funcs_total = 4,
        .branches_hit = 1,
        .branches_total = 2,
    };
    try expect_eq(opts.meetsThresholds(pass_m), true);

    const fail_m = CoverageMetrics{
        .lines_hit = 79,
        .lines_total = 100,
        .funcs_hit = 4,
        .funcs_total = 4,
    };
    try expect_eq(opts.meetsThresholds(fail_m), false);
}

test "coverage_options: formatLcovRecord output & zero leak" {
    const alloc = std.testing.allocator;
    const rec = try formatLcovRecord(alloc, "src/main.cp", 120, 108, 10, 9);
    defer alloc.free(rec);
    try expect_eq(std.mem.indexOf(u8, rec, "SF:src/main.cp\n") != null, true);
    try expect_eq(std.mem.indexOf(u8, rec, "LF:120\nLH:108\nend_of_record\n") != null, true);
}

test "coverage_options: defaults & Reporter enum" {
    const opts = CodeCoverageOptions{};
    try expect_eq(opts.reporters.text, true);
    try expect_eq(opts.reporters.lcov, false);
    try expect_eq(std.mem.eql(u8, opts.reports_directory, "coverage"), true);
    const r = Reporter.lcov;
    try expect_eq(r == .lcov, true);
}

test "coverage_options: cap.coverage_options dynamic mount" {
    const opts = cap.coverage_options.CodeCoverageOptions{
        .fractions = cap.coverage_options.CoverageFraction{ .lines = 0.90 },
    };
    try expect_eq(opts.meetsThresholds(cap.coverage_options.CoverageMetrics{ .lines_hit = 9, .lines_total = 10 }), true);
}
