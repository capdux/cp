// Source: cp/cap/test/test_bundle_options_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Format,
    parseFormat,
    GlobalCache,
    parseGlobalCache,
    OfflineMode,
    parseOfflineMode,
    NodeLinker,
    parseNodeLinker,
    WindowsOptions,
} from "cap:bundle_options";

test "bundle_options: Format enum & ESM semantics" {
    const esm = parseFormat("esm").?;
    const cjs = parseFormat("cjs").?;
    try expect_eq(esm.isESM(), true);
    try expect_eq(esm.keepES6ImportExportSyntax(), true);
    try expect_eq(esm.isAlwaysStrictMode(), true);
    try expect_eq(cjs.isESM(), false);
    try expect_eq(parseFormat("umd") == null, true);
}

test "bundle_options: GlobalCache policy matrix with/without node_modules" {
    const auto_gc = parseGlobalCache("auto").?;
    const force_gc = parseGlobalCache("force").?;
    const disable_gc = parseGlobalCache("disable").?;

    try expect_eq(auto_gc.canUse(false), true);
    try expect_eq(auto_gc.canUse(true), false);
    try expect_eq(auto_gc.canInstall(), true);

    try expect_eq(force_gc.allowVersionSpecifier(), true);
    try expect_eq(force_gc.canUse(true), true);

    try expect_eq(disable_gc.isEnabled(), false);
    try expect_eq(disable_gc.canInstall(), false);
}

test "bundle_options: OfflineMode & NodeLinker parsing" {
    try expect_eq(parseOfflineMode("offline").? == .offline, true);
    try expect_eq(parseOfflineMode("latest").? == .latest, true);
    try expect_eq(parseOfflineMode("online").? == .online, true);
    try expect_eq(parseNodeLinker("hoisted").? == .hoisted, true);
    try expect_eq(parseNodeLinker("isolated").? == .isolated, true);
}

test "bundle_options: WindowsOptions defaults" {
    const w = WindowsOptions{ .hide_console = true, .title = "App" };
    try expect_eq(w.hide_console, true);
    try expect_eq(w.icon == null, true);
}

test "bundle_options: cap.bundle_options dynamic mount" {
    try expect_eq(cap.bundle_options.parseFormat("iife").? == Format.iife, true);
    try expect_eq(cap.bundle_options.parseGlobalCache("fallback").?.canUse(true), true);
}
