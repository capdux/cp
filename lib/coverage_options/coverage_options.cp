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

    pub fn lineRatio(self: CoverageMetrics): f64 {
        if (self.lines_total == 0) { return 1.0; }
        return @as(f64, @floatFromInt(self.lines_hit)) / @as(f64, @floatFromInt(self.lines_total));
    }

    pub fn funcRatio(self: CoverageMetrics): f64 {
        if (self.funcs_total == 0) { return 1.0; }
        return @as(f64, @floatFromInt(self.funcs_hit)) / @as(f64, @floatFromInt(self.funcs_total));
    }

    pub fn branchRatio(self: CoverageMetrics): f64 {
        if (self.branches_total == 0) { return 1.0; }
        return @as(f64, @floatFromInt(self.branches_hit)) / @as(f64, @floatFromInt(self.branches_total));
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

    pub fn meetsThresholds(self: CodeCoverageOptions, m: CoverageMetrics): bool {
        if (m.lineRatio() < self.fractions.lines) { return false; }
        if (m.funcRatio() < self.fractions.functions) { return false; }
        if (m.branchRatio() < self.fractions.branches) { return false; }
        return true;
    }
};

pub fn formatLcovRecord(alloc: std.mem.Allocator, sf_path: []const u8, lf: usize, lh: usize, fnf: usize, fnh: usize): ![]u8 {
    return std.fmt.allocPrint(alloc, "SF:{s}\nFNF:{d}\nFNH:{d}\nLF:{d}\nLH:{d}\nend_of_record\n", .{
        sf_path, fnf, fnh, lf, lh,
    });
}
