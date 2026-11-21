import {
    SatisfiesVersion as SemverVersion,
    satisfiesRange,
    satisfiesVersion,
    isValidSemverRange,
} from "cap:semver";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SemverVersion.parse and SemVer 2.0.0 precedence comparison", () => {
    const v1 = SemverVersion.parse("1.2.3").?;
    expect(v1.major == 1 && v1.minor == 2 && v1.patch == 3);
    expect(strEq(v1.pre, ""));

    // Optional v/V prefix and ignored +build metadata
    const v2 = SemverVersion.parse("v1.2.3+build.99").?;
    expect(v1.compare(v2) == 0);
    const v3 = SemverVersion.parse("V1.2.3-alpha.1+exp.sha.5114f85").?;
    expect(strEq(v3.pre, "alpha.1"));
    expect(v3.compare(v1) < 0);
    expect(v1.compare(v3) > 0);

    // SemVer 2.0.0 Section 11 canonical ordering:
    // 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0
    const a = SemverVersion.parse("1.0.0-alpha").?;
    const a1 = SemverVersion.parse("1.0.0-alpha.1").?;
    const ab = SemverVersion.parse("1.0.0-alpha.beta").?;
    const b = SemverVersion.parse("1.0.0-beta").?;
    const b2 = SemverVersion.parse("1.0.0-beta.2").?;
    const b11 = SemverVersion.parse("1.0.0-beta.11").?;
    const rc1 = SemverVersion.parse("1.0.0-rc.1").?;
    const rel = SemverVersion.parse("1.0.0").?;

    expect(a.compare(a1) < 0);
    expect(a1.compare(ab) < 0);
    expect(ab.compare(b) < 0);
    expect(b.compare(b2) < 0);
    expect(b2.compare(b11) < 0);
    expect(b11.compare(rc1) < 0);
    expect(rc1.compare(rel) < 0);

    // Reject leading zeros and malformed version strings
    expect(SemverVersion.parse("01.2.3") == null);
    expect(SemverVersion.parse("1.02.3") == null);
    expect(SemverVersion.parse("1.2.03") == null);
    expect(SemverVersion.parse("1.2.3-01") == null);
    expect(SemverVersion.parse("1.2") == null);
    expect(SemverVersion.parse("1.2.3-") == null);
    expect(SemverVersion.parse("1.2.3+") == null);
    expect(SemverVersion.parse("") == null);
});

test("caret (^) and tilde (~) ranges across 0.0.x, 0.x, and >=1.x", () => {
    // ^1.2.3 -> >=1.2.3 <2.0.0
    expect(satisfiesRange("1.2.3", "^1.2.3"));
    expect(satisfiesRange("1.9.9", "^1.2.3"));
    expect(!satisfiesRange("1.2.2", "^1.2.3"));
    expect(!satisfiesRange("2.0.0", "^1.2.3"));
    expect(!satisfiesRange("2.0.0-alpha.1", "^1.2.3"));

    // ^0.2.3 -> >=0.2.3 <0.3.0
    expect(satisfiesRange("0.2.3", "^0.2.3"));
    expect(satisfiesRange("0.2.9", "^0.2.3"));
    expect(!satisfiesRange("0.2.2", "^0.2.3"));
    expect(!satisfiesRange("0.3.0", "^0.2.3"));

    // ^0.0.3 -> >=0.0.3 <0.0.4
    expect(satisfiesRange("0.0.3", "^0.0.3"));
    expect(!satisfiesRange("0.0.2", "^0.0.3"));
    expect(!satisfiesRange("0.0.4", "^0.0.3"));

    // ~1.2.3 -> >=1.2.3 <1.3.0, ~1.2 -> >=1.2.0 <1.3.0
    expect(satisfiesRange("1.2.3", "~1.2.3"));
    expect(satisfiesRange("1.2.9", "~1.2.3"));
    expect(!satisfiesRange("1.2.2", "~1.2.3"));
    expect(!satisfiesRange("1.3.0", "~1.2.3"));
    expect(satisfiesRange("1.2.0", "~1.2"));
    expect(satisfiesRange("1.2.5", "~1.2"));
    expect(!satisfiesRange("1.3.0", "~1.2"));
});

test("comparators, OR (||), AND, wildcards, and hyphen ranges", () => {
    // Comparators with and without spaces + AND + OR
    expect(satisfiesRange("1.5.0", ">=1.2.0 <2.0.0"));
    expect(satisfiesRange("1.5.0", ">= 1.2.0 <= 1.5.0"));
    expect(!satisfiesRange("2.0.0", ">=1.2.0 <2.0.0"));
    expect(satisfiesRange("3.1.0", "^1.0.0 || >=3.0.0 <4.0.0"));
    expect(!satisfiesRange("2.5.0", "^1.0.0 || >=3.0.0 <4.0.0"));
    expect(satisfiesRange("1.2.3", "=1.2.3"));
    expect(satisfiesRange("1.2.4", ">1.2.3"));
    expect(!satisfiesRange("1.2.3", ">1.2.3"));

    // Wildcards: *, x, X, 1.x, 1.2.x
    expect(satisfiesRange("0.0.1", "*"));
    expect(satisfiesRange("9.8.7", "x"));
    expect(satisfiesRange("9.8.7", "X"));
    expect(satisfiesRange("1.0.0", "1.x"));
    expect(satisfiesRange("1.9.9", "1.x"));
    expect(!satisfiesRange("2.0.0", "1.x"));
    expect(satisfiesRange("1.2.0", "1.2.x"));
    expect(satisfiesRange("1.2.9", "1.2.X"));
    expect(!satisfiesRange("1.3.0", "1.2.x"));

    // Hyphen ranges (inclusive of both bounds)
    expect(satisfiesRange("1.2.3", "1.2.3 - 2.3.4"));
    expect(satisfiesRange("2.0.0", "1.2.3 - 2.3.4"));
    expect(satisfiesRange("2.3.4", "1.2.3 - 2.3.4"));
    expect(!satisfiesRange("1.2.2", "1.2.3 - 2.3.4"));
    expect(!satisfiesRange("2.3.5", "1.2.3 - 2.3.4"));
    expect(satisfiesRange("2.3.9", "1.2 - 2.3"));
    expect(!satisfiesRange("2.4.0", "1.2 - 2.3"));
});

test("prerelease tuple guard enforces NPM/Bun prerelease isolation", () => {
    // Prerelease versions do NOT match general ranges without a matching prerelease tuple
    expect(!satisfiesRange("1.2.3-alpha.1", "*"));
    expect(!satisfiesRange("1.2.3-alpha.1", "^1.0.0"));
    expect(!satisfiesRange("1.2.3-alpha.1", ">=1.0.0 <2.0.0"));
    expect(!satisfiesRange("1.2.3-alpha.1", "1.0.0 - 2.0.0"));

    // Same [major, minor, patch] prerelease comparator authorizes prerelease match
    expect(satisfiesRange("1.2.3-alpha.3", ">=1.2.3-alpha.1 <2.0.0"));
    expect(satisfiesRange("1.2.3-beta.0", "^1.2.3-alpha.1"));
    expect(satisfiesRange("1.2.3-alpha.5", "1.2.3-alpha.1 - 1.2.3-beta.0"));

    // Different [major, minor, patch] prerelease is rejected even if numerically within bounds
    expect(!satisfiesRange("1.2.4-alpha.1", ">=1.2.3-alpha.1 <2.0.0"));
    expect(!satisfiesRange("1.3.0-alpha.1", "^1.2.3-alpha.1"));

    // satisfiesVersion works directly on pre-parsed SemverVersion
    const pre_ver = SemverVersion.parse("1.2.3-rc.2").?;
    expect(satisfiesVersion(pre_ver, ">=1.2.3-alpha.0 <2.0.0"));
    expect(!satisfiesVersion(pre_ver, ">=1.2.0 <2.0.0"));
});

test("isValidSemverRange and malformed range boundary protection", () => {
    expect(isValidSemverRange(""));
    expect(isValidSemverRange("*"));
    expect(isValidSemverRange("^1.2.3 || ~2.0.0 || 3.0.0 - 4.0.0"));
    expect(isValidSemverRange(">= 1.0.0 < 2.0.0"));

    // Malformed ranges must be rejected by both isValidSemverRange and satisfiesRange
    expect(!isValidSemverRange(">>1.0.0"));
    expect(!isValidSemverRange("^abc"));
    expect(!isValidSemverRange("1.2.3.4"));
    expect(!isValidSemverRange("1.0.0 -"));
    expect(!isValidSemverRange("- 2.0.0"));
    expect(!isValidSemverRange(">="));
    expect(!isValidSemverRange("1.x.2"));
    expect(!isValidSemverRange("01.2.3"));
    expect(!isValidSemverRange("^1.0.0 || >>invalid"));

    expect(!satisfiesRange("1.2.3", "^1.0.0 || >>invalid"));
    expect(!satisfiesRange("not-a-version", "*"));
});
