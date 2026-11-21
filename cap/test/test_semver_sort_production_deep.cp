import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SemverOrder,
    parseSemverStrict,
    parseSemverLoose,
    compareSemverStr,
    sortSemverAsc,
    sortSemverDesc,
    maxSemverIndex,
    minSemverIndex,
} from "cap:semver_sort";

test("1. parseSemverStrict vs parseSemverLoose validation and leading-zero rejection", () => {
    const p = parseSemverStrict("1.2.3-alpha.0.x+build.007") ?? unreachable;
    expect_eq(p.major, u64(1));
    expect_eq(p.minor, u64(2));
    expect_eq(p.patch, u64(3));
    expect_eq(std.mem.eql(u8, p.prerelease, "alpha.0.x"), true);
    expect_eq(std.mem.eql(u8, p.build, "build.007"), true);

    // 严格模式拒绝 v 前缀、前导零与空标识符
    expect_eq(parseSemverStrict("v1.2.3") == null, true);
    expect_eq(parseSemverStrict("01.2.3") == null, true);
    expect_eq(parseSemverStrict("1.02.3") == null, true);
    expect_eq(parseSemverStrict("1.2.03") == null, true);
    expect_eq(parseSemverStrict("1.2.3-01") == null, true);
    expect_eq(parseSemverStrict("1.2.3-") == null, true);
    expect_eq(parseSemverStrict("1.2.3+") == null, true);
    expect_eq(parseSemverStrict("1.2.3-alpha..1") == null, true);

    // 宽松模式支持 v/V 前缀与首尾空白
    const l = parseSemverLoose("  v2.0.0-rc.1  ") ?? unreachable;
    expect_eq(l.major, u64(2));
    expect_eq(std.mem.eql(u8, l.prerelease, "rc.1"), true);
});

test("2. SemVer 2.0.0 section 11 official precedence chain", () => {
    // 官方规范经典顺序：
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
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
        expect_eq((compareSemverStr(chain[i], chain[i + 1]) ?? unreachable) == SemverOrder.less, true);
        expect_eq((compareSemverStr(chain[i + 1], chain[i]) ?? unreachable) == SemverOrder.greater, true);
        i += 1;
    }

    // +build 元数据不影响优先级比较
    expect_eq((compareSemverStr("1.0.0+20130313144700", "1.0.0+exp.sha.5114f85") ?? unreachable) == SemverOrder.equal, true);
    expect_eq((compareSemverStr("1.0.0-beta+exp.1", "1.0.0-beta+exp.2") ?? unreachable) == SemverOrder.equal, true);
});

test("3. sortSemverAsc and sortSemverDesc in-place sorting", () => {
    let versions: [6][]const u8 = [
        "2.0.0",
        "1.0.0-beta.11",
        "1.0.0",
        "1.0.0-beta.2",
        "1.0.0-alpha",
        "0.9.9",
    ];

    expect_eq(sortSemverAsc(&versions), true);
    expect_eq(std.mem.eql(u8, versions[0], "0.9.9"), true);
    expect_eq(std.mem.eql(u8, versions[1], "1.0.0-alpha"), true);
    expect_eq(std.mem.eql(u8, versions[2], "1.0.0-beta.2"), true);
    expect_eq(std.mem.eql(u8, versions[3], "1.0.0-beta.11"), true);
    expect_eq(std.mem.eql(u8, versions[4], "1.0.0"), true);
    expect_eq(std.mem.eql(u8, versions[5], "2.0.0"), true);

    expect_eq(sortSemverDesc(&versions), true);
    expect_eq(std.mem.eql(u8, versions[0], "2.0.0"), true);
    expect_eq(std.mem.eql(u8, versions[5], "0.9.9"), true);
});

test("4. maxSemverIndex and minSemverIndex selection", () => {
    const list: [5][]const u8 = ["1.2.0", "1.10.0-rc.1", "1.10.0", "0.1.0", "1.2.1"];
    expect_eq(maxSemverIndex(&list) ?? unreachable, usize(2));
    expect_eq(minSemverIndex(&list) ?? unreachable, usize(3));

    const empty: [0][]const u8 = [];
    expect_eq(maxSemverIndex(&empty) == null, true);
    expect_eq(minSemverIndex(&empty) == null, true);
});

test("5. invalid version aborts sort without mutating input and fails min/max", () => {
    let bad: [3][]const u8 = ["2.0.0", "not-a-semver", "1.0.0"];
    expect_eq(sortSemverAsc(&bad), false);
    // 原数组保持未破坏
    expect_eq(std.mem.eql(u8, bad[0], "2.0.0"), true);
    expect_eq(std.mem.eql(u8, bad[1], "not-a-semver"), true);
    expect_eq(std.mem.eql(u8, bad[2], "1.0.0"), true);

    expect_eq(maxSemverIndex(&bad) == null, true);
    expect_eq(minSemverIndex(&bad) == null, true);
    expect_eq(compareSemverStr("1.0.0", "bad") == null, true);
});
