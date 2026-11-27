// Source: absorbed from bun/src/options_types/CodeCoverageOptions.cp (cap:coverage_options)
const std = @import("std");

pub const Reporter = enum {
    text,
    lcov,
};

pub const Reporters = struct {
    text: bool = true,
    lcov: bool = false,
};

pub const CoverageFraction = struct {
    lines: f64 = 0.0,
    functions: f64 = 0.0,
    statements: f64 = 0.0,
    branches: f64 = 0.0,
};

pub const CoverageMetrics = struct {
    lines_hit: usize = 0,
    lines_total: usize = 0,
    funcs_hit: usize = 0,
    funcs_total: usize = 0,
    branches_hit: usize = 0,
    branches_total: usize = 0,

    pub fn lineRatio(): f64 {
        if (_.lines_total == 0) { return 1.0; }
        return @as(f64, @floatFromInt(_.lines_hit)) / @as(f64, @floatFromInt(_.lines_total));
    }

    pub fn funcRatio(): f64 {
        if (_.funcs_total == 0) { return 1.0; }
        return @as(f64, @floatFromInt(_.funcs_hit)) / @as(f64, @floatFromInt(_.funcs_total));
    }

    pub fn branchRatio(): f64 {
        if (_.branches_total == 0) { return 1.0; }
        return @as(f64, @floatFromInt(_.branches_hit)) / @as(f64, @floatFromInt(_.branches_total));
    }
};

pub const CodeCoverageOptions = struct {
    skip_test_files: bool = true,
    reporters: Reporters = Reporters{ .text = true, .lcov = false },
    reports_directory: []const u8 = "coverage",
    fractions: CoverageFraction = CoverageFraction{},
    ignore_sourcemap: bool = false,
    enabled: bool = false,
    fail_on_low_coverage: bool = false,

    pub fn meetsThresholds(m: CoverageMetrics): bool {
        if (m.lineRatio() < _.fractions.lines) { return false; }
        if (m.funcRatio() < _.fractions.functions) { return false; }
        if (m.branchRatio() < _.fractions.branches) { return false; }
        return true;
    }
};

pub fn formatLcovRecord(alloc: std.mem.Allocator, sf_path: []const u8, lf: usize, lh: usize, fnf: usize, fnh: usize): ![]u8 {
    return std.fmt.allocPrint(alloc, "SF:{s}\nFNF:{d}\nFNH:{d}\nLF:{d}\nLH:{d}\nend_of_record\n", .{
        sf_path, fnf, fnh, lf, lh,
    });
}
