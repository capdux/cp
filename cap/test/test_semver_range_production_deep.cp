// Source: cp/cap/test/test_semver_range_production_deep.cp (Production deep tests for cap:semver_range)

import { test, expect_eq } from "cap:test";
import { SemverVersion, SemverRange } from "cap:semver_range";

test("SemverVersion parsing (prerelease/build) and Semver 2.0 total ordering") {
    const v_full = SemverVersion.parse("v1.2.3-beta.11+build.42") catch unreachable;
    expect_eq(v_full.major, @as(u64, 1));
    expect_eq(v_full.minor, @as(u64, 2));
    expect_eq(v_full.patch, @as(u64, 3));
    expect_eq(v_full.hasPrerelease(), true);
    expect_eq(v_full.hasBuild(), true);
    expect_eq(v_full.prerelease, "beta.11");
    expect_eq(v_full.build, "build.42");

    // 构建元数据不影响优先级相等性
    const b1 = SemverVersion.parse("1.0.0+001") catch unreachable;
    const b2 = SemverVersion.parse("1.0.0+20130313144700") catch unreachable;
    expect_eq(b1.eql(b2), true);

    // Semver 2.0 规范标准优先级链
    const chain: [8]string = [
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
        const va = SemverVersion.parse(chain[i]) catch unreachable;
        const vb = SemverVersion.parse(chain[i + 1]) catch unreachable;
        expect_eq(va.lt(vb), true);
        expect_eq(vb.gt(va), true);
        expect_eq(va.eql(vb), false);
        i += 1;
    }
}

test("Standard comparators =, >, >=, <, <= and compound AND constraints") {
    const r_eq = SemverRange.parse("=1.2.3") catch unreachable;
    expect_eq(r_eq.satisfiesStr("1.2.3"), true);
    expect_eq(r_eq.satisfiesStr("1.2.4"), false);

    const r_band = SemverRange.parse(">= 1.2.0 < 2.0.0") catch unreachable;
    expect_eq(r_band.satisfiesStr("1.1.9"), false);
    expect_eq(r_band.satisfiesStr("1.2.0"), true);
    expect_eq(r_band.satisfiesStr("1.9.9"), true);
    expect_eq(r_band.satisfiesStr("2.0.0"), false);

    const r_open = SemverRange.parse(">1.0.0 <=1.5.0") catch unreachable;
    expect_eq(r_open.satisfiesStr("1.0.0"), false);
    expect_eq(r_open.satisfiesStr("1.0.1"), true);
    expect_eq(r_open.satisfiesStr("1.5.0"), true);
    expect_eq(r_open.satisfiesStr("1.5.1"), false);
}

test("Caret (^) including 0.x / 0.0.x edge cases and Tilde (~) ranges") {
    // ^1.2.3 -> >=1.2.3 <2.0.0
    const c1 = SemverRange.parse("^1.2.3") catch unreachable;
    expect_eq(c1.satisfiesStr("1.2.2"), false);
    expect_eq(c1.satisfiesStr("1.2.3"), true);
    expect_eq(c1.satisfiesStr("1.9.9"), true);
    expect_eq(c1.satisfiesStr("2.0.0"), false);

    // ^0.2.3 -> >=0.2.3 <0.3.0
    const c2 = SemverRange.parse("^0.2.3") catch unreachable;
    expect_eq(c2.satisfiesStr("0.2.2"), false);
    expect_eq(c2.satisfiesStr("0.2.3"), true);
    expect_eq(c2.satisfiesStr("0.2.9"), true);
    expect_eq(c2.satisfiesStr("0.3.0"), false);

    // ^0.0.3 -> >=0.0.3 <0.0.4
    const c3 = SemverRange.parse("^0.0.3") catch unreachable;
    expect_eq(c3.satisfiesStr("0.0.2"), false);
    expect_eq(c3.satisfiesStr("0.0.3"), true);
    expect_eq(c3.satisfiesStr("0.0.4"), false);

    // ~1.2.3 -> >=1.2.3 <1.3.0; ~1 -> >=1.0.0 <2.0.0
    const t1 = SemverRange.parse("~1.2.3") catch unreachable;
    expect_eq(t1.satisfiesStr("1.2.3"), true);
    expect_eq(t1.satisfiesStr("1.2.9"), true);
    expect_eq(t1.satisfiesStr("1.3.0"), false);

    const t2 = SemverRange.parse("~1") catch unreachable;
    expect_eq(t2.satisfiesStr("1.0.0"), true);
    expect_eq(t2.satisfiesStr("1.9.0"), true);
    expect_eq(t2.satisfiesStr("2.0.0"), false);
}

test("Hyphen ranges (1.2.3 - 2.3.4), wildcards (1.x, 1.2.*, *) and prerelease guard") {
    const h1 = SemverRange.parse("1.2.3 - 2.3.4") catch unreachable;
    expect_eq(h1.satisfiesStr("1.2.2"), false);
    expect_eq(h1.satisfiesStr("1.2.3"), true);
    expect_eq(h1.satisfiesStr("2.3.4"), true);
    expect_eq(h1.satisfiesStr("2.3.5"), false);

    // 不完整右端：1.2.3 - 2.3 -> >=1.2.3 <2.4.0
    const h2 = SemverRange.parse("1.2.3 - 2.3") catch unreachable;
    expect_eq(h2.satisfiesStr("2.3.9"), true);
    expect_eq(h2.satisfiesStr("2.4.0"), false);

    // 通配符
    const w_any = SemverRange.parse("*") catch unreachable;
    expect_eq(w_any.satisfiesStr("0.0.1"), true);
    expect_eq(w_any.satisfiesStr("99.0.0"), true);
    // Prerelease 保护：未显式声明 prerelease 的范围不匹配 prerelease
    expect_eq(w_any.satisfiesStr("1.0.0-alpha.1"), false);

    const w_minor = SemverRange.parse("1.x") catch unreachable;
    expect_eq(w_minor.satisfiesStr("1.0.0"), true);
    expect_eq(w_minor.satisfiesStr("1.8.4"), true);
    expect_eq(w_minor.satisfiesStr("2.0.0"), false);

    const w_patch = SemverRange.parse("1.2.*") catch unreachable;
    expect_eq(w_patch.satisfiesStr("1.2.0"), true);
    expect_eq(w_patch.satisfiesStr("1.2.99"), true);
    expect_eq(w_patch.satisfiesStr("1.3.0"), false);
}

test("Logical OR (||) multi-group matching and maxSatisfying / minSatisfying selection") {
    const r = SemverRange.parse("^1.2.0 || >=3.0.0 <3.2.0 || 4.0.0-alpha.1") catch unreachable;
    expect_eq(r.groupCount(), @as(usize, 3));

    const pool: [6]SemverVersion = [
        SemverVersion.parse("1.1.0") catch unreachable,
        SemverVersion.parse("1.2.3") catch unreachable,
        SemverVersion.parse("1.8.0") catch unreachable,
        SemverVersion.parse("2.5.0") catch unreachable,
        SemverVersion.parse("3.1.9") catch unreachable,
        SemverVersion.parse("4.0.0-alpha.1") catch unreachable,
    ];

    const best = r.maxSatisfying(pool[0..6]).?;
    expect_eq(best.eql(SemverVersion.parse("4.0.0-alpha.1") catch unreachable), true);

    const lowest = r.minSatisfying(pool[0..6]).?;
    expect_eq(lowest.eql(SemverVersion.parse("1.2.3") catch unreachable), true);

    // 显式 prerelease 分组仅匹配同 (4,0,0) 且满足比较器的 prerelease
    expect_eq(r.satisfiesStr("4.0.0-alpha.1"), true);
    expect_eq(r.satisfiesStr("3.1.0-beta.1"), false);
}

test("Malformed version and range syntax error guards") {
    // 非法 SemverVersion
    expect_eq(switch (SemverVersion.parse("")) { err.EmptyInput => true, else => false }, true);
    expect_eq(switch (SemverVersion.parse("1.2")) { err.InvalidVersion => true, else => false }, true);
    expect_eq(switch (SemverVersion.parse("01.2.3")) { err.InvalidVersion => true, else => false }, true);
    expect_eq(switch (SemverVersion.parse("1.2.3-01")) { err.InvalidVersion => true, else => false }, true);
    expect_eq(switch (SemverVersion.parse("1.2.3-")) { err.InvalidVersion => true, else => false }, true);
    expect_eq(switch (SemverVersion.parse("1.2.3+")) { err.InvalidVersion => true, else => false }, true);

    // 非法 SemverRange
    expect_eq(switch (SemverRange.parse("")) { err.EmptyInput => true, else => false }, true);
    expect_eq(switch (SemverRange.parse("|| ^1.0.0")) { err.InvalidRange => true, else => false }, true);
    expect_eq(switch (SemverRange.parse("^1.0.0 ||")) { err.InvalidRange => true, else => false }, true);
    expect_eq(switch (SemverRange.parse(">=")) { err.InvalidComparator => true, else => false }, true);
    expect_eq(switch (SemverRange.parse("^abc")) { err.InvalidVersion => true, else => false }, true);
}
