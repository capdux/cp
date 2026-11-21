// Source: cp/cap/test/test_semver_bump_production_deep.cp (Production deep tests for cap:semver_bump)

import { test, expect_eq } from "cap:test";
import {
    BumpKind,
    bumpMajor,
    bumpMinor,
    bumpPatch,
    bumpPrerelease,
    stripPrereleaseAndBuild,
    bumpVersion,
} from "cap:semver_bump";

test("semver_bump: bumpMajor, bumpMinor and bumpPatch with prerelease promotion") {
    let buf: [64]u8 = [0; 64];

    // bumpMajor
    expect_eq(std.mem.eql(u8, bumpMajor("1.2.3", buf[0..]).?, "2.0.0"), true);
    expect_eq(std.mem.eql(u8, bumpMajor("2.0.0-alpha.1", buf[0..]).?, "2.0.0"), true);
    expect_eq(std.mem.eql(u8, bumpMajor("1.2.3-beta.0", buf[0..]).?, "2.0.0"), true);
    expect_eq(std.mem.eql(u8, bumpMajor("v0.9.5+build.12", buf[0..]).?, "1.0.0"), true);

    // bumpMinor
    expect_eq(std.mem.eql(u8, bumpMinor("1.2.3", buf[0..]).?, "1.3.0"), true);
    expect_eq(std.mem.eql(u8, bumpMinor("1.3.0-alpha.1", buf[0..]).?, "1.3.0"), true);
    expect_eq(std.mem.eql(u8, bumpMinor("1.2.3-alpha.1", buf[0..]).?, "1.3.0"), true);

    // bumpPatch
    expect_eq(std.mem.eql(u8, bumpPatch("1.2.3", buf[0..]).?, "1.2.4"), true);
    expect_eq(std.mem.eql(u8, bumpPatch("1.2.4-alpha.1", buf[0..]).?, "1.2.4"), true);
}

test("semver_bump: bumpPrerelease with same preid, switched preid and empty preid") {
    let buf: [64]u8 = [0; 64];

    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.3", "alpha", buf[0..]).?, "1.2.4-alpha.0"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-alpha.0", "alpha", buf[0..]).?, "1.2.4-alpha.1"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-alpha.9", "alpha", buf[0..]).?, "1.2.4-alpha.10"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-alpha.1", "beta", buf[0..]).?, "1.2.4-beta.0"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-alpha", "alpha", buf[0..]).?, "1.2.4-alpha.0"), true);

    // 空 preid 语义
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.3", "", buf[0..]).?, "1.2.4-0"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-0", "", buf[0..]).?, "1.2.4-1"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-rc.2", "", buf[0..]).?, "1.2.4-rc.3"), true);
    expect_eq(std.mem.eql(u8, bumpPrerelease("1.2.4-rc", "", buf[0..]).?, "1.2.4-rc.0"), true);
}

test("semver_bump: stripPrereleaseAndBuild and unified bumpVersion dispatch") {
    let buf: [64]u8 = [0; 64];

    expect_eq(std.mem.eql(u8, stripPrereleaseAndBuild("1.2.3-rc.1+sha.abc", buf[0..]).?, "1.2.3"), true);
    expect_eq(std.mem.eql(u8, stripPrereleaseAndBuild("2.4.6+20260101", buf[0..]).?, "2.4.6"), true);

    expect_eq(std.mem.eql(u8, bumpVersion("1.2.3", .major, "", buf[0..]).?, "2.0.0"), true);
    expect_eq(std.mem.eql(u8, bumpVersion("1.2.3", .minor, "", buf[0..]).?, "1.3.0"), true);
    expect_eq(std.mem.eql(u8, bumpVersion("1.2.3", .patch, "", buf[0..]).?, "1.2.4"), true);
    expect_eq(std.mem.eql(u8, bumpVersion("1.2.3", .prerelease, "rc", buf[0..]).?, "1.2.4-rc.0"), true);
    expect_eq(std.mem.eql(u8, bumpVersion("1.2.4-rc.0+exp", .release, "", buf[0..]).?, "1.2.4"), true);
    expect_eq(std.mem.eql(u8, BumpKind.prerelease.asStr(), "prerelease"), true);
}

test("semver_bump: strict SemVer 2.0.0 validation rejects malformed versions and preids") {
    let buf: [64]u8 = [0; 64];

    expect_eq(bumpPatch("", buf[0..]) == null, true);
    expect_eq(bumpPatch("1.2", buf[0..]) == null, true);
    expect_eq(bumpPatch("1.2.3.4", buf[0..]) == null, true);
    expect_eq(bumpPatch("01.2.3", buf[0..]) == null, true);
    expect_eq(bumpPatch("1.02.3", buf[0..]) == null, true);
    expect_eq(bumpPatch("1.2.3-", buf[0..]) == null, true);
    expect_eq(bumpPatch("1.2.3+", buf[0..]) == null, true);
    expect_eq(bumpPatch("1.2.3-alpha.01", buf[0..]) == null, true);
    expect_eq(bumpPrerelease("1.2.3", "bad/preid", buf[0..]) == null, true);
}

test("semver_bump: small output buffer overflow guards") {
    let tiny: [3]u8 = [0; 3];
    expect_eq(bumpMajor("1.2.3", tiny[0..]) == null, true);
    expect_eq(bumpMinor("1.2.3", tiny[0..]) == null, true);
    expect_eq(bumpPatch("1.2.3", tiny[0..]) == null, true);
    expect_eq(bumpPrerelease("1.2.3", "alpha", tiny[0..]) == null, true);
    expect_eq(stripPrereleaseAndBuild("1.2.3-alpha", tiny[0..]) == null, true);
}
