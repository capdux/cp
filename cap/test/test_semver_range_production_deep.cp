// Source: cp/cap/test/test_semver_range_production_deep.cp
// Deep Production Test Suite for cap:semver_range
// 5 test cases:
//   1. Semver 2.0 parsing, normalization, strict validation, Rule 11 ordering, +build ignored
//   2. Caret ^ (incl. ^0.2.3 and ^0.0.3), Tilde ~, wildcards x/X/*, and basic comparators
//   3. Hyphen ranges (A - B), || unions, maxSatisfying/minSatisfying, and intersects
//   4. Prerelease isolation rule, include_prerelease option, and malformed range rejection
//   5. Dynamic cap.semver_range namespace mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SemverError,
    RangeOptions,
    Semver,
    RangeExpr,
    parseSemver,
    compare,
    comparePrecedence,
    equals,
    lt,
    gt,
    parseRange,
    satisfies,
    satisfiesWithOptions,
    satisfiesStr,
    satisfiesStrWithOptions,
    maxSatisfying,
    minSatisfying,
    intersects,
} from "cap:semver_range";
import cap from "cap";

fn expectSemverErr(input: []const u8, expected: SemverError) {
    switch (parseSemver(input)) {
        val => {
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == expected, true);
        },
    }
}

fn expectRangeErr(alloc: std.mem.Allocator, input: []const u8) {
    switch (parseRange(alloc, input)) {
        val => {
            let r = val;
            r.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => {
            * = e;
            expect_eq(true, true);
        },
    }
}

test("semver_range: Semver 2.0 parsing, Rule 11 precedence, and build metadata ignored", () => {
    const alloc = std.testing.allocator;

    // Leading v / = normalization & format
    let v1 = parseSemver("  v1.2.3-alpha.1+build.42  ")?;
    expect_eq(v1.major, 1 as u64);
    expect_eq(v1.minor, 2 as u64);
    expect_eq(v1.patch, 3 as u64);
    expect_eq(std.mem.eql(u8, v1.prerelease, "alpha.1"), true);
    expect_eq(std.mem.eql(u8, v1.build, "build.42"), true);
    expect_eq(v1.hasPrerelease(), true);
    expect_eq(v1.hasBuild(), true);

    const formatted = v1.format(alloc)?;
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, "1.2.3-alpha.1+build.42"), true);

    let v_eq = parseSemver("=v1.2.3-alpha.1+other.99")?;
    expect_eq(compare(v1, v_eq), 0 as i8);
    expect_eq(comparePrecedence(v1, v_eq), 0 as i8);
    expect_eq(equals(v1, v_eq), true);

    // Semver 2.0 Rule 11 total ordering chain
    const chain: [8][]const u8 = [
        "1.0.0-alpha",
        "1.0.0-alpha.1",
        "1.0.0-alpha.beta",
        "1.0.0-beta",
        "1.0.0-beta.2",
        "1.0.0-beta.11",
        "1.0.0-rc.1",
        "1.0.0",
    ];
    let i: usize = 0;
    while (i + 1 < chain.len) {
        let a = parseSemver(chain[i])?;
        let b = parseSemver(chain[i + 1])?;
        expect_eq(lt(a, b), true);
        expect_eq(gt(b, a), true);
        expect_eq(comparePrecedence(a, b), -1 as i8);
        i += 1;
    }

    // Strict rejection of leading zeros, missing segments, empty tags
    expectSemverErr("", SemverError.EmptyInput);
    expectSemverErr("01.2.3", SemverError.LeadingZero);
    expectSemverErr("1.02.3", SemverError.LeadingZero);
    expectSemverErr("1.2.03", SemverError.LeadingZero);
    expectSemverErr("1.2.3-01", SemverError.LeadingZero);
    expectSemverErr("1.2", SemverError.InvalidVersion);
    expectSemverErr("1.2.3.4", SemverError.InvalidVersion);
    expectSemverErr("1.2.3-", SemverError.InvalidPrerelease);
    expectSemverErr("1.2.3+", SemverError.InvalidBuildMetadata);
    expectSemverErr("999999999999999999999999.0.0", SemverError.IntegerOverflow);
});

test("semver_range: Caret (^0.2.3, ^0.0.3), Tilde (~), wildcards (x/*), and comparators", () => {
    const alloc = std.testing.allocator;

    // Caret ^1.2.3 -> >=1.2.3 <2.0.0-0
    expect_eq(satisfiesStr(alloc, "1.2.3", "^1.2.3")?, true);
    expect_eq(satisfiesStr(alloc, "1.9.9", "^1.2.3")?, true);
    expect_eq(satisfiesStr(alloc, "1.2.2", "^1.2.3")?, false);
    expect_eq(satisfiesStr(alloc, "2.0.0", "^1.2.3")?, false);

    // Caret zero-major ^0.2.3 -> >=0.2.3 <0.3.0-0
    expect_eq(satisfiesStr(alloc, "0.2.3", "^0.2.3")?, true);
    expect_eq(satisfiesStr(alloc, "0.2.99", "^0.2.3")?, true);
    expect_eq(satisfiesStr(alloc, "0.2.2", "^0.2.3")?, false);
    expect_eq(satisfiesStr(alloc, "0.3.0", "^0.2.3")?, false);

    // Caret zero-major-zero-minor ^0.0.3 -> >=0.0.3 <0.0.4-0
    expect_eq(satisfiesStr(alloc, "0.0.3", "^0.0.3")?, true);
    expect_eq(satisfiesStr(alloc, "0.0.2", "^0.0.3")?, false);
    expect_eq(satisfiesStr(alloc, "0.0.4", "^0.0.3")?, false);

    // Tilde ~1.2.3 -> >=1.2.3 <1.3.0-0; ~1 -> >=1.0.0 <2.0.0-0
    expect_eq(satisfiesStr(alloc, "1.2.3", "~1.2.3")?, true);
    expect_eq(satisfiesStr(alloc, "1.2.9", "~1.2.3")?, true);
    expect_eq(satisfiesStr(alloc, "1.3.0", "~1.2.3")?, false);
    expect_eq(satisfiesStr(alloc, "1.9.0", "~1")?, true);
    expect_eq(satisfiesStr(alloc, "2.0.0", "~1")?, false);

    // Wildcards 1.2.x, 1.X, *
    expect_eq(satisfiesStr(alloc, "1.2.0", "1.2.x")?, true);
    expect_eq(satisfiesStr(alloc, "1.2.8", "1.2.x")?, true);
    expect_eq(satisfiesStr(alloc, "1.3.0", "1.2.x")?, false);
    expect_eq(satisfiesStr(alloc, "1.9.5", "1.X")?, true);
    expect_eq(satisfiesStr(alloc, "2.0.0", "1.X")?, false);
    expect_eq(satisfiesStr(alloc, "99.0.0", "*")?, true);

    // Basic operators with optional whitespace
    expect_eq(satisfiesStr(alloc, "1.5.0", ">= 1.2.0 <= 1.5.0")?, true);
    expect_eq(satisfiesStr(alloc, "1.5.1", ">= 1.2.0 <= 1.5.0")?, false);
    expect_eq(satisfiesStr(alloc, "1.2.0", ">1.2.0")?, false);
    expect_eq(satisfiesStr(alloc, "1.2.1", "=1.2.1")?, true);
});

test("semver_range: hyphen ranges, || unions, maxSatisfying/minSatisfying, and intersects", () => {
    const alloc = std.testing.allocator;

    // Hyphen ranges
    expect_eq(satisfiesStr(alloc, "1.2.3", "1.2.3 - 2.3.4")?, true);
    expect_eq(satisfiesStr(alloc, "2.3.4", "1.2.3 - 2.3.4")?, true);
    expect_eq(satisfiesStr(alloc, "2.3.5", "1.2.3 - 2.3.4")?, false);
    expect_eq(satisfiesStr(alloc, "2.3.9", "1.2 - 2.3")?, true);
    expect_eq(satisfiesStr(alloc, "2.4.0", "1.2 - 2.3")?, false);

    // Union || with intersection
    let r = parseRange(alloc, ">=1.2.0 <1.5.0 || >=2.0.0 <=2.2.0")?;
    defer r.deinit(alloc);

    let candidates: [7]Semver = [
        try parseSemver("1.1.9"),
        try parseSemver("1.2.0"),
        try parseSemver("1.4.9"),
        try parseSemver("1.5.0"),
        try parseSemver("2.0.0"),
        try parseSemver("2.1.5"),
        try parseSemver("2.2.1"),
    ];

    let max_v = maxSatisfying(&candidates, r);
    expect_eq(max_v != null, true);
    expect_eq(equals(max_v.?, try parseSemver("2.1.5")), true);

    let min_v = minSatisfying(&candidates, r);
    expect_eq(min_v != null, true);
    expect_eq(equals(min_v.?, try parseSemver("1.2.0")), true);

    let r_none = parseRange(alloc, ">=9.0.0")?;
    defer r_none.deinit(alloc);
    expect_eq(maxSatisfying(&candidates, r_none) == null, true);
    expect_eq(minSatisfying(&candidates, r_none) == null, true);

    // intersects
    let r_a = parseRange(alloc, "^1.2.0")?;
    defer r_a.deinit(alloc);
    let r_b = parseRange(alloc, "^1.5.0")?;
    defer r_b.deinit(alloc);
    let r_c = parseRange(alloc, "^2.0.0")?;
    defer r_c.deinit(alloc);
    let r_d = parseRange(alloc, ">1.2.3")?;
    defer r_d.deinit(alloc);
    let r_e = parseRange(alloc, "<1.2.4")?;
    defer r_e.deinit(alloc);

    expect_eq(intersects(r_a, r_b), true);
    expect_eq(intersects(r_a, r_c), false);
    expect_eq(intersects(r_d, r_e), false);
    expect_eq(intersects(r, r_c), true);
});

test("semver_range: prerelease isolation rule and malformed expression rejection", () => {
    const alloc = std.testing.allocator;

    // Default prerelease isolation: only same [major, minor, patch] tuple with explicit prerelease comparator matches
    let r_pre = parseRange(alloc, ">=1.2.3-alpha.1 <2.0.0")?;
    defer r_pre.deinit(alloc);

    expect_eq(satisfies(try parseSemver("1.2.3-alpha.2"), r_pre), true);
    expect_eq(satisfies(try parseSemver("1.2.3-beta"), r_pre), true);
    expect_eq(satisfies(try parseSemver("1.5.0"), r_pre), true);
    expect_eq(satisfies(try parseSemver("1.2.4-alpha.1"), r_pre), false);
    expect_eq(satisfies(try parseSemver("2.0.0-alpha.1"), r_pre), false);

    // ^1.2.3 must NOT match 2.0.0-alpha.1 or 1.2.4-alpha.1 by default
    expect_eq(satisfiesStr(alloc, "2.0.0-alpha.1", "^1.2.3")?, false);
    expect_eq(satisfiesStr(alloc, "1.2.4-alpha.1", "^1.2.3")?, false);

    // include_prerelease: true allows cross-tuple prerelease matching within bounds
    let opts = RangeOptions{ include_prerelease: true };
    expect_eq(satisfiesWithOptions(try parseSemver("1.2.4-alpha.1"), r_pre, opts), true);
    expect_eq(satisfiesStrWithOptions(alloc, "1.2.4-alpha.1", "^1.2.3", opts)?, true);
    expect_eq(satisfiesStrWithOptions(alloc, "2.0.0-alpha.1", "^1.2.3", opts)?, false);

    // Malformed expressions must be rejected cleanly
    expectRangeErr(alloc, "");
    expectRangeErr(alloc, ">");
    expectRangeErr(alloc, ">= ");
    expectRangeErr(alloc, "^");
    expectRangeErr(alloc, "~");
    expectRangeErr(alloc, "1.2.3 - ");
    expectRangeErr(alloc, " - 1.2.3");
    expectRangeErr(alloc, "1.0.0 || ");
    expectRangeErr(alloc, "|| 1.0.0");
    expectRangeErr(alloc, "==1.2.3");
    expectRangeErr(alloc, "~01.2.3");
    expectRangeErr(alloc, ">=1.2.3.4");
});

test("semver_range: dynamic cap.semver_range mounting and zero-leak check", () => {
    const alloc = std.testing.allocator;

    let v = cap.semver_range.parseSemver("v2.1.0-rc.1")?;
    let r = cap.semver_range.parseRange(alloc, "^2.0.0 || >=2.1.0-rc.1 <2.2.0")?;
    defer r.deinit(alloc);

    expect_eq(cap.semver_range.satisfies(v, r), true);
    expect_eq(cap.semver_range.satisfiesStr(alloc, "2.1.5", "^2.0.0")?, true);

    let r2 = cap.semver_range.parseRange(alloc, "~2.1.0")?;
    defer r2.deinit(alloc);
    expect_eq(cap.semver_range.intersects(r, r2), true);

    let pool: [3]Semver = [
        try cap.semver_range.parseSemver("2.0.0"),
        try cap.semver_range.parseSemver("2.1.0-rc.1"),
        try cap.semver_range.parseSemver("2.1.9"),
    ];
    let best = cap.semver_range.maxSatisfying(&pool, r2);
    expect_eq(best != null, true);
    expect_eq(cap.semver_range.equals(best.?, try parseSemver("2.1.9")), true);
});
