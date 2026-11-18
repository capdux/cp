// cap/test/test_semver_complete.cp
// ============================================================================
// Comprehensive Semver 2.0.0 Specification & Query Engine Test Suite
// ============================================================================

import { test, expect_eq } from "cap:test";
import {
    Semver,
    Version,
    ReleaseType,
    Op,
    Comparator,
    Query,
    Range,
    SemverRange,
} from "cap:semver";

const std = @import("std");

fn testSemver2PrecedenceLadder(): !void {
    // Semver 2.0.0 Spec Section 11 precedence ladder:
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
    let v_alpha = try Version.parse("1.0.0-alpha");
    let v_alpha1 = try Version.parse("1.0.0-alpha.1");
    let v_alphabeta = try Version.parse("1.0.0-alpha.beta");
    let v_beta = try Version.parse("1.0.0-beta");
    let v_beta2 = try Version.parse("1.0.0-beta.2");
    let v_beta11 = try Version.parse("1.0.0-beta.11");
    let v_rc1 = try Version.parse("1.0.0-rc.1");
    let v_rel = try Version.parse("1.0.0");

    expect_eq(v_alpha.lt(v_alpha1), true);
    expect_eq(v_alpha1.lt(v_alphabeta), true);
    expect_eq(v_alphabeta.lt(v_beta), true);
    expect_eq(v_beta.lt(v_beta2), true);
    expect_eq(v_beta2.lt(v_beta11), true);
    expect_eq(v_beta11.lt(v_rc1), true);
    expect_eq(v_rc1.lt(v_rel), true);

    // Reverse greater than
    expect_eq(v_rel.gt(v_rc1), true);
    expect_eq(v_rc1.gt(v_beta11), true);
    expect_eq(v_beta11.gt(v_beta2), true);
    expect_eq(v_beta2.gt(v_beta), true);

    // Numeric vs non-numeric identifier rule:
    // Numeric identifiers always have lower precedence than non-numeric identifiers
    let v_num = try Version.parse("1.0.0-1");
    let v_str = try Version.parse("1.0.0-a");
    expect_eq(v_num.lt(v_str), true);
    expect_eq(v_str.gt(v_num), true);

    // Build metadata is ignored in precedence comparison
    let v_build1 = try Version.parse("1.0.0+20130313144700");
    let v_build2 = try Version.parse("1.0.0+exp.sha.5114f85");
    expect_eq(v_build1.eql(v_rel), true);
    expect_eq(v_build1.eql(v_build2), true);
    expect_eq(v_build2.eql(v_rel), true);
}

fn testAdvancedRangeSyntaxes(): !void {
    // 1. Caret ranges ^
    // Major > 0
    expect_eq(Semver.satisfiesStr("1.2.3", "^1.2.3"), true);
    expect_eq(Semver.satisfiesStr("1.9.9", "^1.2.3"), true);
    expect_eq(Semver.satisfiesStr("2.0.0", "^1.2.3"), false);
    expect_eq(Semver.satisfiesStr("1.2.2", "^1.2.3"), false);

    // Major == 0, minor > 0
    expect_eq(Semver.satisfiesStr("0.2.3", "^0.2.3"), true);
    expect_eq(Semver.satisfiesStr("0.2.9", "^0.2.3"), true);
    expect_eq(Semver.satisfiesStr("0.3.0", "^0.2.3"), false);

    // Major == 0, minor == 0
    expect_eq(Semver.satisfiesStr("0.0.3", "^0.0.3"), true);
    expect_eq(Semver.satisfiesStr("0.0.4", "^0.0.3"), false);

    // Partial caret: ^1.2.x, ^0.x
    expect_eq(Semver.satisfiesStr("1.2.0", "^1.2.x"), true);
    expect_eq(Semver.satisfiesStr("1.9.0", "^1.2.x"), true);
    expect_eq(Semver.satisfiesStr("2.0.0", "^1.2.x"), false);
    expect_eq(Semver.satisfiesStr("0.5.0", "^0.x"), true);
    expect_eq(Semver.satisfiesStr("1.0.0", "^0.x"), false);

    // 2. Tilde ranges ~
    expect_eq(Semver.satisfiesStr("1.2.3", "~1.2.3"), true);
    expect_eq(Semver.satisfiesStr("1.2.9", "~1.2.3"), true);
    expect_eq(Semver.satisfiesStr("1.3.0", "~1.2.3"), false);

    expect_eq(Semver.satisfiesStr("1.2.0", "~1.2"), true);
    expect_eq(Semver.satisfiesStr("1.2.9", "~1.2"), true);
    expect_eq(Semver.satisfiesStr("1.3.0", "~1.2"), false);

    expect_eq(Semver.satisfiesStr("1.0.0", "~1"), true);
    expect_eq(Semver.satisfiesStr("1.9.9", "~1"), true);
    expect_eq(Semver.satisfiesStr("2.0.0", "~1"), false);

    // 3. X-ranges / wildcards
    expect_eq(Semver.satisfiesStr("1.5.0", "1.x"), true);
    expect_eq(Semver.satisfiesStr("2.0.0", "1.x"), false);
    expect_eq(Semver.satisfiesStr("1.2.5", "1.2.*"), true);
    expect_eq(Semver.satisfiesStr("1.3.0", "1.2.*"), false);

    // 4. Hyphen ranges
    expect_eq(Semver.satisfiesStr("1.2.3", "1.2.3 - 2.3.4"), true);
    expect_eq(Semver.satisfiesStr("2.0.0", "1.2.3 - 2.3.4"), true);
    expect_eq(Semver.satisfiesStr("2.3.4", "1.2.3 - 2.3.4"), true);
    expect_eq(Semver.satisfiesStr("2.3.5", "1.2.3 - 2.3.4"), false);
    expect_eq(Semver.satisfiesStr("1.2.2", "1.2.3 - 2.3.4"), false);

    expect_eq(Semver.satisfiesStr("1.2.0", "1.2 - 2.3.4"), true);
    expect_eq(Semver.satisfiesStr("1.1.9", "1.2 - 2.3.4"), false);

    expect_eq(Semver.satisfiesStr("2.3.9", "1.2.3 - 2.3"), true);
    expect_eq(Semver.satisfiesStr("2.4.0", "1.2.3 - 2.3"), false);

    expect_eq(Semver.satisfiesStr("2.9.9", "1.2.3 - 2"), true);
    expect_eq(Semver.satisfiesStr("3.0.0", "1.2.3 - 2"), false);
}

fn testLogicalOrDisjunction(): !void {
    expect_eq(Semver.satisfiesStr("1.5.0", "^1.0.0 || ^3.0.0"), true);
    expect_eq(Semver.satisfiesStr("3.2.0", "^1.0.0 || ^3.0.0"), true);
    expect_eq(Semver.satisfiesStr("2.1.0", "^1.0.0 || ^3.0.0"), false);

    expect_eq(Semver.satisfiesStr("0.9.0", "<1.0.0 || >=2.0.0"), true);
    expect_eq(Semver.satisfiesStr("2.5.0", "<1.0.0 || >=2.0.0"), true);
    expect_eq(Semver.satisfiesStr("1.5.0", "<1.0.0 || >=2.0.0"), false);
}

fn testMaxAndMinSatisfying(): !void {
    let v0 = try Version.parse("1.0.0");
    let v1 = try Version.parse("1.2.0");
    let v2 = try Version.parse("1.2.3");
    let v3 = try Version.parse("2.0.0");
    let v4 = try Version.parse("2.1.0");

    let list: [5]Version = [ v0, v1, v2, v3, v4 ];

    let max_v = Semver.maxSatisfying(list[0..5], "^1.0.0");
    expect_eq(max_v != null, true);
    expect_eq(max_v.?.major, 1);
    expect_eq(max_v.?.minor, 2);
    expect_eq(max_v.?.patch, 3);

    let min_v = Semver.minSatisfying(list[0..5], "^1.0.0");
    expect_eq(min_v != null, true);
    expect_eq(min_v.?.major, 1);
    expect_eq(min_v.?.minor, 0);
    expect_eq(min_v.?.patch, 0);

    let no_v = Semver.maxSatisfying(list[0..5], "^3.0.0");
    expect_eq(no_v == null, true);

    // String slices
    let str_list: [5][]const u8 = [ "1.0.0", "1.2.0", "1.2.3", "2.0.0", "2.1.0" ];
    let max_str = Semver.maxSatisfyingStr(str_list[0..5], "^1.0.0");
    expect_eq(std.mem.eql(u8, max_str.?, "1.2.3"), true);

    let min_str = Semver.minSatisfyingStr(str_list[0..5], "^1.0.0");
    expect_eq(std.mem.eql(u8, min_str.?, "1.0.0"), true);
}

fn testVersionIncrement(): !void {
    let v = try Version.parse("1.2.3");

    // major
    let v_maj = v.increment(.major, null);
    expect_eq(v_maj.major, 2);
    expect_eq(v_maj.minor, 0);
    expect_eq(v_maj.patch, 0);
    expect_eq(v_maj.prerelease.len, 0);

    // minor
    let v_min = v.increment(.minor, null);
    expect_eq(v_min.major, 1);
    expect_eq(v_min.minor, 3);
    expect_eq(v_min.patch, 0);

    // patch
    let v_pat = v.increment(.patch, null);
    expect_eq(v_pat.major, 1);
    expect_eq(v_pat.minor, 2);
    expect_eq(v_pat.patch, 4);

    // premajor
    let v_pmaj = v.increment(.premajor, "alpha");
    expect_eq(v_pmaj.major, 2);
    expect_eq(v_pmaj.minor, 0);
    expect_eq(v_pmaj.patch, 0);
    expect_eq(std.mem.eql(u8, v_pmaj.prerelease, "alpha"), true);

    // preminor
    let v_pmin = v.increment(.preminor, "beta");
    expect_eq(v_pmin.major, 1);
    expect_eq(v_pmin.minor, 3);
    expect_eq(v_pmin.patch, 0);
    expect_eq(std.mem.eql(u8, v_pmin.prerelease, "beta"), true);

    // prepatch
    let v_ppat = v.increment(.prepatch, "rc");
    expect_eq(v_ppat.major, 1);
    expect_eq(v_ppat.minor, 2);
    expect_eq(v_ppat.patch, 4);
    expect_eq(std.mem.eql(u8, v_ppat.prerelease, "rc"), true);

    // prerelease progression: 1.2.3-alpha.1 -> 1.2.3-alpha.2
    let v_pre1 = try Version.parse("1.2.3-alpha.1");
    let v_pre2 = v_pre1.increment(.prerelease, null);
    expect_eq(v_pre2.major, 1);
    expect_eq(v_pre2.minor, 2);
    expect_eq(v_pre2.patch, 3);
    expect_eq(std.mem.eql(u8, v_pre2.prerelease, "alpha.2"), true);

    // prerelease numeric: 1.2.3-0 -> 1.2.3-1
    let v_zero = try Version.parse("1.2.3-0");
    let v_one = v_zero.increment(.prerelease, null);
    expect_eq(std.mem.eql(u8, v_one.prerelease, "1"), true);
}

fn main() {
    test("1. Semver 2.0.0 Section 11 precedence ladder & build metadata", () => {
        try testSemver2PrecedenceLadder();
    });
    test("2. Advanced range syntaxes (^, ~, x-range, hyphen)", () => {
        try testAdvancedRangeSyntaxes();
    });
    test("3. Disjunction logical OR (||) multi-clause matching", () => {
        try testLogicalOrDisjunction();
    });
    test("4. maxSatisfying & minSatisfying version queries", () => {
        try testMaxAndMinSatisfying();
    });
    test("5. Version increment operations (major, minor, patch, pre*)", () => {
        try testVersionIncrement();
    });
}
