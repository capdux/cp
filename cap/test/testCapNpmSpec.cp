// cap/test/testCapNpmSpec.cp
// Comprehensive unit tests for @cap/npm-spec (npm package name & spec parser)

import { test, expect_eq } from "cap:test";
import {
    PackageSpec,
    isValidPackageName,
    parsePackageSpec,
    formatPackageSpec,
} from "../../packages/@cap/npm-spec/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/npm-spec — scoped package with semver range '@cap/http@^1.0.0'", () => {
        const s: PackageSpec = parsePackageSpec("@cap/http@^1.0.0");
        expect_eq(s.valid, true);
        expect_eq(s.is_scoped, true);
        expect_eq(sliceEq(s.scope, "cap"), true);
        expect_eq(sliceEq(s.name, "http"), true);
        expect_eq(sliceEq(s.full_name, "@cap/http"), true);
        expect_eq(sliceEq(s.version_spec, "^1.0.0"), true);
    });

    test("2. @cap/npm-spec — unscoped package with dist-tag 'lodash@latest'", () => {
        const s: PackageSpec = parsePackageSpec("lodash@latest");
        expect_eq(s.valid, true);
        expect_eq(s.is_scoped, false);
        expect_eq(s.scope.len, 0 as usize);
        expect_eq(sliceEq(s.name, "lodash"), true);
        expect_eq(sliceEq(s.full_name, "lodash"), true);
        expect_eq(sliceEq(s.version_spec, "latest"), true);
    });

    test("3. @cap/npm-spec — versionless package 'react' & '@cap/core'", () => {
        const s1: PackageSpec = parsePackageSpec("react");
        expect_eq(s1.valid, true);
        expect_eq(s1.is_scoped, false);
        expect_eq(sliceEq(s1.name, "react"), true);
        expect_eq(s1.version_spec.len, 0 as usize);

        const s2: PackageSpec = parsePackageSpec("@cap/core");
        expect_eq(s2.valid, true);
        expect_eq(s2.is_scoped, true);
        expect_eq(sliceEq(s2.scope, "cap"), true);
        expect_eq(sliceEq(s2.name, "core"), true);
        expect_eq(s2.version_spec.len, 0 as usize);
    });

    test("4. @cap/npm-spec — isValidPackageName rejects uppercase, leading dot/underscore, >214 & empty segments", () => {
        expect_eq(isValidPackageName("express"), true);
        expect_eq(isValidPackageName("@types/node"), true);
        expect_eq(isValidPackageName("my.pkg_name-1"), true);

        expect_eq(isValidPackageName(""), false);
        expect_eq(isValidPackageName("MyPkg"), false);
        expect_eq(isValidPackageName(".hidden"), false);
        expect_eq(isValidPackageName("_private"), false);
        expect_eq(isValidPackageName("@scope/"), false);
        expect_eq(isValidPackageName("@/pkg"), false);
        expect_eq(isValidPackageName("@scope/a/b"), false);
        expect_eq(isValidPackageName("bad name"), false);

        // 215 chars exceeds npm 214 limit
        let too_long: [215]u8 = undefined;
        let i: usize = 0;
        while (i < 215) {
            too_long[i] = 'a' as u8;
            i += 1;
        }
        expect_eq(isValidPackageName(too_long[0..215]), false);
        expect_eq(isValidPackageName(too_long[0..214]), true);
    });

    test("5. @cap/npm-spec — formatPackageSpec round-trip", () => {
        const s1: PackageSpec = parsePackageSpec("@cap/http@^1.0.0");
        let buf: [64]u8 = undefined;
        const n1: usize = formatPackageSpec(s1, buf[0..64]);
        expect_eq(sliceEq(buf[0..n1], "@cap/http@^1.0.0"), true);

        const s2: PackageSpec = parsePackageSpec("express");
        const n2: usize = formatPackageSpec(s2, buf[0..64]);
        expect_eq(sliceEq(buf[0..n2], "express"), true);
    });

    test("6. @cap/npm-spec — rejects malformed specs & tiny buffers", () => {
        expect_eq(parsePackageSpec("").valid, false);
        expect_eq(parsePackageSpec("lodash@").valid, false);
        expect_eq(parsePackageSpec("@cap/http@").valid, false);
        expect_eq(parsePackageSpec("@onlyscope").valid, false);
        expect_eq(parsePackageSpec("UPPER@1.0.0").valid, false);

        const ok: PackageSpec = parsePackageSpec("@cap/http@^1.0.0");
        let tiny: [6]u8 = undefined;
        expect_eq(formatPackageSpec(ok, tiny[0..6]), 0 as usize);
    });
}
