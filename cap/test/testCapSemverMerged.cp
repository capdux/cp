import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    SemVer,
    Version,
    VersionReq,
    parseSemver,
    parseVersion,
    compareSemver,
    compareVersions,
    isPrerelease,
    matchesVersionReq,
    isCaretCompatible,
    formatSemver,
    formatVersion,
    bumpMajor,
    bumpMinor,
    bumpPatch,
    satisfies,
    valid,
    compare,
    gt,
    gte,
    lt,
    lte,
    eq,
    neq,
    major,
    minor,
    patch,
    clean,
    inc,
    maxSatisfying,
    minSatisfying,
    incMajor,
    incMinor,
    incPatch,
} from "../../packages/semver/src/index.cp";

fn main() {
    test("1. @cap/semver [core] — parseSemver, parseVersion, valid, clean, and accessors", () => {
        const v1 = parseSemver("=v1.2.3-alpha.1+build.42");
        expect_eq(v1.valid, true);
        expect_eq(v1.major, 1);
        expect_eq(v1.minor, 2);
        expect_eq(v1.patch, 3);
        expect_eq(isPrerelease(v1), true);
        expect_eq(strEq(v1.preStr(), "alpha.1"), true);
        expect_eq(strEq(v1.buildStr(), "build.42"), true);

        const v2 = parseVersion("2.0.0");
        expect_eq(v2.valid, true);
        expect_eq(v2.isPrerelease(), false);
        expect_eq(valid("1.2.3"), true);
        expect_eq(valid("01.2.3"), false);
        expect_eq(valid("not-a-semver"), false);

        expect_eq(major("3.4.5") ?? 0, 3);
        expect_eq(minor("3.4.5") ?? 0, 4);
        expect_eq(patch("3.4.5") ?? 0, 5);
        expect_eq(major("bad") == null, true);

        let buf: [64]u8 = undefined;
        const cn = clean("  =v1.2.3  ", buf[0..]);
        expect_eq(strEq(buf[0..cn], "1.2.3"), true);
    });

    test("2. @cap/semver [npm] — compare, gt, gte, lt, lte, eq, neq", () => {
        expect_eq(compare("1.0.0-alpha", "1.0.0-alpha.1"), -1);
        expect_eq(compare("1.0.0-alpha.1", "1.0.0-alpha.beta"), -1);
        expect_eq(compare("1.0.0-beta", "1.0.0"), -1);
        expect_eq(compare("1.2.3+build1", "1.2.3+build2"), 0);
        expect_eq(compareVersions(parseVersion("2.0.0"), parseVersion("1.9.9")), 1);
        expect_eq(compareSemver(SemVer.make(1, 0, 0), SemVer.make(1, 0, 0)), 0);

        expect_eq(gt("2.0.0", "1.9.9"), true);
        expect_eq(gt("1.0.0", "2.0.0"), false);
        expect_eq(gte("1.2.3", "1.2.3"), true);
        expect_eq(lt("1.0.0-rc.1", "1.0.0"), true);
        expect_eq(lte("1.2.3", "1.2.3"), true);
        expect_eq(eq("1.2.3+a", "1.2.3+b"), true);
        expect_eq(neq("1.2.3", "1.2.4"), true);
    });

    test("3. @cap/semver [npm] — satisfies ranges (^, ~, ||, hyphen, x-range, prerelease)", () => {
        expect_eq(satisfies("1.5.0", "^1.2.3"), true);
        expect_eq(satisfies("2.0.0", "^1.2.3"), false);
        expect_eq(satisfies("0.2.5", "^0.2.3"), true);
        expect_eq(satisfies("0.3.0", "^0.2.3"), false);
        expect_eq(satisfies("1.2.9", "~1.2.3"), true);
        expect_eq(satisfies("1.3.0", "~1.2.3"), false);
        expect_eq(satisfies("2.1.0", ">=1.0.0 <1.5.0 || >=2.0.0 <3.0.0"), true);
        expect_eq(satisfies("1.7.0", "1.2.0 - 2.0.0"), true);
        expect_eq(satisfies("1.4.9", "1.4.x"), true);
        expect_eq(satisfies("1.5.0-alpha.1", "^1.0.0"), false);
        expect_eq(satisfies("1.2.3-alpha.5", ">=1.2.3-alpha.1 <1.2.3"), true);
    });

    test("4. @cap/semver [npm] — inc, incMajor/Minor/Patch, maxSatisfying & minSatisfying", () => {
        let buf: [64]u8 = undefined;
        let n = inc("1.2.3", "major", buf[0..]);
        expect_eq(strEq(buf[0..n], "2.0.0"), true);
        n = inc("1.2.3", "minor", buf[0..]);
        expect_eq(strEq(buf[0..n], "1.3.0"), true);
        n = inc("1.2.3", "patch", buf[0..]);
        expect_eq(strEq(buf[0..n], "1.2.4"), true);
        expect_eq(inc("1.2.3", "invalid", buf[0..]), 0);

        const base = parseSemver("1.2.3");
        expect_eq(incMajor(base).major, 2);
        expect_eq(incMinor(base).minor, 3);
        expect_eq(incPatch(base).patch, 4);

        const list: [5]string = ["0.9.0", "1.2.0", "1.8.3", "1.5.1", "2.0.0"];
        expect_eq(maxSatisfying(list[0..5], "^1.0.0"), 2); // "1.8.3"
        expect_eq(minSatisfying(list[0..5], "^1.0.0"), 1); // "1.2.0"
        expect_eq(maxSatisfying(list[0..5], "^3.0.0"), -1);
    });

    test("5. @cap/semver [crates.io] — VersionReq struct and Cargo comma-separated matching", () => {
        const req = VersionReq.parse(">=1.2.3, <1.8.0");
        expect_eq(req.valid, true);
        expect_eq(req.matches(Version.parse("1.5.0")), true);
        expect_eq(req.matchesStr("1.8.0"), false);
        expect_eq(req.matchesStr("1.2.2"), false);

        // Cargo bare version implies caret semantics
        const caret_req = VersionReq.parse("1.2.3");
        expect_eq(caret_req.valid, true);
        expect_eq(caret_req.matchesStr("1.9.0"), true);
        expect_eq(caret_req.matchesStr("2.0.0"), false);

        const bad_req = VersionReq.parse("not-valid-req");
        expect_eq(bad_req.valid, false);
        expect_eq(bad_req.matchesStr("1.0.0"), false);

        expect_eq(matchesVersionReq("1.4.2", ">=1.2.0, <2.0.0"), true);
        expect_eq(matchesVersionReq("1.2.5", "1.2.*"), true);
        expect_eq(matchesVersionReq("1.0.0", "*"), true);
        expect_eq(isCaretCompatible("1.7.0", "1.2.0"), true);
        expect_eq(isCaretCompatible("2.0.0", "1.2.0"), false);
    });

    test("6. @cap/semver [crates.io] — bumpMajor/Minor/Patch and formatVersion/formatSemver", () => {
        const v = Version.parse("1.2.3");
        let buf: [64]u8 = undefined;
        let n1 = formatVersion(bumpMajor(v), buf[0..]);
        expect_eq(strEq(buf[0..n1], "2.0.0"), true);

        let n2 = formatSemver(bumpMinor(v), buf[0..]);
        expect_eq(strEq(buf[0..n2], "1.3.0"), true);

        const bp = bumpPatch(v);
        let n3 = bp.writeTo(buf[0..]);
        expect_eq(strEq(buf[0..n3], "1.2.4"), true);
    });
}
