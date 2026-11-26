import { test, expect_eq } from "cap:test";
import {
    parseVersion, compareVersions, isPrerelease, matchesVersionReq,
    isCaretCompatible, formatVersion, bumpMajor, bumpMinor, bumpPatch
} from "../../packages/@cap/semver-crate/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/semver-crate — parseVersion, compareVersions & isPrerelease", () => {
        let v1 = parseVersion("1.2.3");
        let v2 = parseVersion("1.2.3-alpha.1");
        let v3 = parseVersion("2.0.0");
        expect_eq(v1.valid, true);
        expect_eq(isPrerelease(v1), false);
        expect_eq(isPrerelease(v2), true);
        expect_eq(compareVersions(v2, v1), -1);
        expect_eq(compareVersions(v1, v3), -1);
    });

    test("2. @cap/semver-crate — Cargo comma-separated VersionReq (>=1.2.3, <1.8.0)", () => {
        expect_eq(matchesVersionReq("1.2.3", ">=1.2.3, <1.8.0"), true);
        expect_eq(matchesVersionReq("1.7.9", ">=1.2.3, <1.8.0"), true);
        expect_eq(matchesVersionReq("1.8.0", ">=1.2.3, <1.8.0"), false);
        expect_eq(matchesVersionReq("1.2.2", ">=1.2.3, <1.8.0"), false);
    });

    test("3. @cap/semver-crate — Cargo bare version defaults to caret (^)", () => {
        expect_eq(matchesVersionReq("1.5.0", "1.2.3"), true);
        expect_eq(matchesVersionReq("2.0.0", "1.2.3"), false);
        expect_eq(isCaretCompatible("0.2.5", "0.2.3"), true);
        expect_eq(isCaretCompatible("0.3.0", "0.2.3"), false);
    });

    test("4. @cap/semver-crate — Tilde (~), exact (=) & wildcard (*) requirements", () => {
        expect_eq(matchesVersionReq("1.2.9", "~1.2.3"), true);
        expect_eq(matchesVersionReq("1.3.0", "~1.2.3"), false);
        expect_eq(matchesVersionReq("1.2.3", "=1.2.3"), true);
        expect_eq(matchesVersionReq("1.2.4", "=1.2.3"), false);
        expect_eq(matchesVersionReq("3.4.5", "*"), true);
        expect_eq(matchesVersionReq("1.2.7", "1.2.*"), true);
    });

    test("5. @cap/semver-crate — bumpMajor, bumpMinor, bumpPatch & formatVersion", () => {
        let v = parseVersion("1.2.3");
        let out: [32]u8 = undefined;
        let n1 = formatVersion(bumpPatch(v), out[0..32]);
        expect_eq(eqBuf(out[0..n1], n1, "1.2.4"), true);
        let n2 = formatVersion(bumpMinor(v), out[0..32]);
        expect_eq(eqBuf(out[0..n2], n2, "1.3.0"), true);
        let n3 = formatVersion(bumpMajor(v), out[0..32]);
        expect_eq(eqBuf(out[0..n3], n3, "2.0.0"), true);
    });

    test("6. @cap/semver-crate — Invalid version or empty req returns false", () => {
        expect_eq(matchesVersionReq("not-semver", "^1.0.0"), false);
        expect_eq(matchesVersionReq("1.0.0", ""), false);
        expect_eq(matchesVersionReq("1.0.0-alpha", "*"), false);
    });
}
