// Source: cp/cap/test/test_npm_package_name_production_deep.cp (Production deep tests for cap:npm_package_name)

import { test, expect_eq } from "cap:test";
import {
    PackageSpecifier,
    parsePackageSpecifier,
    isValidPackageName,
    isRelativeSpecifier,
    isBareSpecifier,
} from "cap:npm_package_name";

test("npm_package_name: parsePackageSpecifier scoped and unscoped with subpaths") {
    const p1 = parsePackageSpecifier("@types/node/fs/promises").?;
    expect_eq(std.mem.eql(u8, p1.scope.?, "@types"), true);
    expect_eq(std.mem.eql(u8, p1.name, "node"), true);
    expect_eq(std.mem.eql(u8, p1.full_name, "@types/node"), true);
    expect_eq(std.mem.eql(u8, p1.subpath.?, "fs/promises"), true);

    const p2 = parsePackageSpecifier("@babel/core").?;
    expect_eq(std.mem.eql(u8, p2.scope.?, "@babel"), true);
    expect_eq(std.mem.eql(u8, p2.name, "core"), true);
    expect_eq(std.mem.eql(u8, p2.full_name, "@babel/core"), true);
    expect_eq(p2.subpath == null, true);

    const p3 = parsePackageSpecifier("lodash/fp/map").?;
    expect_eq(p3.scope == null, true);
    expect_eq(std.mem.eql(u8, p3.name, "lodash"), true);
    expect_eq(std.mem.eql(u8, p3.full_name, "lodash"), true);
    expect_eq(std.mem.eql(u8, p3.subpath.?, "fp/map"), true);

    const p4 = parsePackageSpecifier("react").?;
    expect_eq(p4.scope == null, true);
    expect_eq(std.mem.eql(u8, p4.name, "react"), true);
    expect_eq(std.mem.eql(u8, p4.full_name, "react"), true);
    expect_eq(p4.subpath == null, true);
}

test("npm_package_name: isRelativeSpecifier and isBareSpecifier classification") {
    expect_eq(isRelativeSpecifier("."), true);
    expect_eq(isRelativeSpecifier(".."), true);
    expect_eq(isRelativeSpecifier("./index.js"), true);
    expect_eq(isRelativeSpecifier("../utils/hash"), true);
    expect_eq(isRelativeSpecifier(".hidden"), false);
    expect_eq(isRelativeSpecifier("..foo"), false);
    expect_eq(isRelativeSpecifier("/abs/path"), false);

    expect_eq(isBareSpecifier("lodash"), true);
    expect_eq(isBareSpecifier("@types/node"), true);
    expect_eq(isBareSpecifier("react/jsx-runtime"), true);

    expect_eq(isBareSpecifier(""), false);
    expect_eq(isBareSpecifier("./local"), false);
    expect_eq(isBareSpecifier("../parent"), false);
    expect_eq(isBareSpecifier("/usr/local/lib"), false);
    expect_eq(isBareSpecifier("node:fs"), false);
    expect_eq(isBareSpecifier("bun:sqlite"), false);
    expect_eq(isBareSpecifier("cap:crypto"), false);
    expect_eq(isBareSpecifier("https://cdn.example/pkg.js"), false);
}

test("npm_package_name: isValidPackageName validate-npm-package-name compliance") {
    // 合法包名
    expect_eq(isValidPackageName("lodash"), true);
    expect_eq(isValidPackageName("my-pkg_1.2"), true);
    expect_eq(isValidPackageName("@types/node"), true);
    expect_eq(isValidPackageName("@cap-lang/http.core"), true);

    // 非法包名：空串、大写、前导 . 或 _、首尾空白、非 URL 安全字符、黑名单、畸形 scope
    expect_eq(isValidPackageName(""), false);
    expect_eq(isValidPackageName("MyPkg"), false);
    expect_eq(isValidPackageName(".dotstart"), false);
    expect_eq(isValidPackageName("_understart"), false);
    expect_eq(isValidPackageName(" leading"), false);
    expect_eq(isValidPackageName("trailing "), false);
    expect_eq(isValidPackageName("bad~name"), false);
    expect_eq(isValidPackageName("bad!name"), false);
    expect_eq(isValidPackageName("bad*name"), false);
    expect_eq(isValidPackageName("bad(name)"), false);
    expect_eq(isValidPackageName("bad'name"), false);
    expect_eq(isValidPackageName("node_modules"), false);
    expect_eq(isValidPackageName("favicon.ico"), false);
    expect_eq(isValidPackageName("@onlyscope"), false);
    expect_eq(isValidPackageName("@scope/a/b"), false);
    expect_eq(isValidPackageName("@.bad/pkg"), false);
}

test("npm_package_name: 214-char length boundary enforcement") {
    let exact214: [214]u8 = ['a'; 214];
    expect_eq(isValidPackageName(exact214[0..]), true);

    let over215: [215]u8 = ['a'; 215];
    expect_eq(isValidPackageName(over215[0..]), false);
}

test("npm_package_name: parsePackageSpecifier rejects relative, absolute, protocol and malformed specs") {
    expect_eq(parsePackageSpecifier("") == null, true);
    expect_eq(parsePackageSpecifier("./a") == null, true);
    expect_eq(parsePackageSpecifier("../b") == null, true);
    expect_eq(parsePackageSpecifier("/abs/mod") == null, true);
    expect_eq(parsePackageSpecifier("node:path") == null, true);
    expect_eq(parsePackageSpecifier("cap:net") == null, true);
    expect_eq(parsePackageSpecifier("@") == null, true);
    expect_eq(parsePackageSpecifier("@scope") == null, true);
    expect_eq(parsePackageSpecifier("@scope/") == null, true);
    expect_eq(parsePackageSpecifier("@/pkg") == null, true);
    expect_eq(parsePackageSpecifier("@scope/pkg/") == null, true);
    expect_eq(parsePackageSpecifier("@scope/pkg/a//b") == null, true);
    expect_eq(parsePackageSpecifier("pkg/") == null, true);
    expect_eq(parsePackageSpecifier("pkg//sub") == null, true);
}
