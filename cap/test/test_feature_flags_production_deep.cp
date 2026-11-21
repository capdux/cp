// Source: cp/cap/test/test_feature_flags_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { FeatureFlags, parseBoolEnv, parseUintEnv, parseBoolOr, parseUintOr } from "cap:feature_flags";

test "feature_flags: default flags match Bun core defaults" {
    const f = FeatureFlags{};
    try expect_eq(f.breaking_changes_1_4, false);
    try expect_eq(f.enable_keepalive, true);
    try expect_eq(f.unwrap_commonjs_to_esm, true);
    try expect_eq(f.verbose_fs, false);
}

test "feature_flags: parseBoolEnv truthy, falsy, and invalid" {
    try expect_eq(parseBoolEnv("1").?, true);
    try expect_eq(parseBoolEnv("TRUE").?, true);
    try expect_eq(parseBoolEnv(" yes ").?, true);
    try expect_eq(parseBoolEnv("on").?, true);
    try expect_eq(parseBoolEnv("0").?, false);
    try expect_eq(parseBoolEnv("False").?, false);
    try expect_eq(parseBoolEnv("off").?, false);
    try expect_eq(parseBoolEnv("maybe") == null, true);
    try expect_eq(parseBoolEnv("") == null, true);
}

test "feature_flags: parseUintEnv & fallback defaults" {
    try expect_eq(parseUintEnv("300").?, 300);
    try expect_eq(parseUintEnv(" 42 ").?, 42);
    try expect_eq(parseUintEnv("-5") == null, true);
    try expect_eq(parseUintEnv("abc") == null, true);

    try expect_eq(parseBoolOr(null, true), true);
    try expect_eq(parseBoolOr("garbage", false), false);
    try expect_eq(parseUintOr(null, 30), 30);
    try expect_eq(parseUintOr("bad", 30), 30);
}

test "feature_flags: cap.feature_flags dynamic mount" {
    try expect_eq(cap.feature_flags.parseBoolOr("on", false), true);
    try expect_eq(cap.feature_flags.parseUintOr("1024", 0), 1024);
}
