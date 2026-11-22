import { test, expect } from "cap:test";
import {
    InstallTreePrinter,
    TreeDepEdge,
    TreePackageBinKind,
    TreePrintDecisionKind,
    TreePrintLogLevel,
    TreePrintOptions,
    formatInstalledPackageLine,
    formatUpdatedPackageLine,
} from "cap:pkg";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (sliceEq(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

test("pkg tree_printer: single-line updated and installed formatters (plain and ANSI)", () => {
    let buf: [512]u8 = [0; 512];

    const n1 = formatUpdatedPackageLine("zod", "3.22.0", "3.24.1", false, buf[0..]) catch 0;
    expect(sliceEq(buf[0..n1], "^ zod 3.22.0 -> 3.24.1\n")).toBe(true);

    const n2 = formatUpdatedPackageLine("zod", "3.22.0", "3.24.1", true, buf[0..]) catch 0;
    expect(containsSubstr(buf[0..n2], "↑")).toBe(true);
    expect(containsSubstr(buf[0..n2], "3.22.0 →")).toBe(true);
    expect(containsSubstr(buf[0..n2], "3.24.1")).toBe(true);

    const n3 = formatInstalledPackageLine("react", "18.3.1", "", false, buf[0..]) catch 0;
    expect(sliceEq(buf[0..n3], "+ react@18.3.1\n")).toBe(true);

    const n4 = formatInstalledPackageLine("react", "18.3.1", "19.1.0", false, buf[0..]) catch 0;
    expect(sliceEq(buf[0..n4], "+ react@18.3.1 (v19.1.0 available)\n")).toBe(true);

    const n5 = formatInstalledPackageLine("react", "18.3.1", "19.1.0", true, buf[0..]) catch 0;
    expect(containsSubstr(buf[0..n5], "v19.1.0 available")).toBe(true);
    expect(containsSubstr(buf[0..n5], "\x1b[32m+")).toBe(true);
});

test("pkg tree_printer: dual-pass upgrades + installs with dedupe, disabled and uninstalled filtering", () => {
    let p = InstallTreePrinter.init();
    const root_id = p.addPackage("root-app", "1.0.0", true, true) catch 99;
    const zod_id = p.addPackage("zod", "3.24.1", false, true) catch 99;
    const react_id = p.addPackage("react", "18.3.1", false, true) catch 99;
    const omitted_id = p.addPackage("vitest", "2.0.0", false, true) catch 99;
    const skipped_id = p.addPackage("cached-only", "1.0.0", false, false) catch 99;

    let set_ok = true;
    p.setPackageLaterVersion(react_id, "19.1.1") catch {
        set_ok = false;
    };
    expect(set_ok).toBe(true);

    // Upgrade edge
    const d0 = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "zod",
        target_pkg_id: zod_id,
        original_version: "3.22.0",
    }) catch 99;
    // Duplicate install edges for "react" (second must be deduplicated)
    const d1 = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "react",
        target_pkg_id: react_id,
    }) catch 99;
    * = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "react",
        target_pkg_id: react_id,
    }) catch 99;
    // Disabled edge (--omit=dev)
    const d3 = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "vitest",
        target_pkg_id: omitted_id,
        is_disabled: true,
    }) catch 99;
    // Not installed edge
    const d4 = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "cached-only",
        target_pkg_id: skipped_id,
    }) catch 99;

    expect(p.shouldPrintDep(d0, false) == TreePrintDecisionKind.Update).toBe(true);
    expect(p.shouldPrintDep(d1, false) == TreePrintDecisionKind.Yes).toBe(true);
    expect(p.shouldPrintDep(d3, false) == TreePrintDecisionKind.No).toBe(true);
    expect(p.shouldPrintDep(d4, false) == TreePrintDecisionKind.No).toBe(true);

    let out: [1024]u8 = [0; 1024];
    const stats = p.render(TreePrintOptions{
        ansi_colors: false,
        log_level: TreePrintLogLevel.Default,
        has_installed_filter: true,
        current_workspace_id: root_id,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };

    const expected = "\n^ zod 3.22.0 -> 3.24.1\n\n+ react@18.3.1 (v19.1.1 available)\n\n";
    expect(stats.updated_count).toBe(1);
    expect(stats.installed_count).toBe(1);
    expect(sliceEq(out[0..stats.bytes_written], expected)).toBe(true);
});

test("pkg tree_printer: verbose multi-workspace sections and current-workspace default mode", () => {
    let p = InstallTreePrinter.init();
    const root_id = p.addPackage("monorepo", "0.0.0", true, true) catch 99;
    const ws_a = p.addPackage("@acme/web", "1.0.0", true, true) catch 99;
    const ws_b = p.addPackage("@acme/empty", "1.0.0", true, true) catch 99;
    const lodash_id = p.addPackage("lodash", "4.17.21", false, true) catch 99;
    const preact_id = p.addPackage("preact", "10.25.0", false, true) catch 99;

    // Root -> workspaces
    * = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "@acme/web",
        target_pkg_id: ws_a,
        is_workspace_dep: true,
    }) catch 99;
    * = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "@acme/empty",
        target_pkg_id: ws_b,
        is_workspace_dep: true,
    }) catch 99;

    // Root dep & ws_a dep (ws_b has no deps -> should not print section header)
    * = p.addDep(TreeDepEdge{
        owner_pkg_id: root_id,
        dep_name: "lodash",
        target_pkg_id: lodash_id,
    }) catch 99;
    * = p.addDep(TreeDepEdge{
        owner_pkg_id: ws_a,
        dep_name: "preact",
        target_pkg_id: preact_id,
    }) catch 99;

    let out: [1024]u8 = [0; 1024];
    const verbose_stats = p.render(TreePrintOptions{
        ansi_colors: false,
        log_level: TreePrintLogLevel.Verbose,
        has_installed_filter: true,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };

    const expected_verbose = "\n+ lodash@4.17.21\n\n@acme/web:\n+ preact@10.25.0\n\n";
    expect(verbose_stats.workspace_sections).toBe(1);
    expect(verbose_stats.installed_count).toBe(2);
    expect(sliceEq(out[0..verbose_stats.bytes_written], expected_verbose)).toBe(true);

    // Default mode targeting ws_a only
    const ws_stats = p.render(TreePrintOptions{
        ansi_colors: false,
        log_level: TreePrintLogLevel.Default,
        has_installed_filter: true,
        current_workspace_id: ws_a,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    const expected_ws = "\n+ preact@10.25.0\n\n";
    expect(ws_stats.installed_count).toBe(1);
    expect(sliceEq(out[0..ws_stats.bytes_written], expected_ws)).toBe(true);
});

test("pkg tree_printer: explicit update requests with binaries and without binaries", () => {
    let p = InstallTreePrinter.init();
    const root_id = p.addPackage("app", "1.0.0", true, true) catch 99;
    const tsc_id = p.addPackage("typescript", "5.8.2", false, true) catch 99;
    const zod_id = p.addPackage("zod", "3.24.1", false, true) catch 99;
    const bg_id = p.addPackage("chalk", "5.4.1", false, true) catch 99;

    * = p.addPackageBin(tsc_id, TreePackageBinKind.Map, "tsc") catch {};
    * = p.addPackageBin(tsc_id, TreePackageBinKind.Map, "tsserver") catch {};

    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "typescript", target_pkg_id: tsc_id }) catch 99;
    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "zod", target_pkg_id: zod_id }) catch 99;
    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "chalk", target_pkg_id: bg_id }) catch 99;

    * = p.addUpdateRequest("typescript", false) catch 99;
    * = p.addUpdateRequest("zod", false) catch 99;

    let out: [1024]u8 = [0; 1024];
    const stats = p.render(TreePrintOptions{
        ansi_colors: false,
        log_level: TreePrintLogLevel.Default,
        has_installed_filter: true,
        current_workspace_id: root_id,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };

    const expected = "\n+ chalk@5.4.1\n\ninstalled typescript@5.8.2 with binaries:\n - tsc\n - tsserver\ninstalled zod@3.24.1\n\n";
    expect(stats.installed_count).toBe(1);
    expect(stats.explicit_request_count).toBe(2);
    expect(stats.bin_count).toBe(2);
    expect(sliceEq(out[0..stats.bytes_written], expected)).toBe(true);
});

test("pkg tree_printer: fallback full-lockfile mode skips peer deps and handles ANSI output", () => {
    let p = InstallTreePrinter.init();
    const root_id = p.addPackage("app", "1.0.0", true, false) catch 99;
    const pkg_a = p.addPackage("hono", "4.7.0", false, false) catch 99;
    const peer_b = p.addPackage("react", "19.0.0", false, false) catch 99;
    const cli_c = p.addPackage("esbuild", "0.25.0", false, false) catch 99;

    * = p.addPackageBin(cli_c, TreePackageBinKind.File, "esbuild") catch {};
    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "hono", target_pkg_id: pkg_a }) catch 99;
    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "react", target_pkg_id: peer_b, is_peer: true }) catch 99;
    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "esbuild", target_pkg_id: cli_c }) catch 99;
    * = p.addUpdateRequest("esbuild", false) catch 99;

    let out: [1024]u8 = [0; 1024];
    const stats = p.render(TreePrintOptions{
        ansi_colors: false,
        log_level: TreePrintLogLevel.Default,
        has_installed_filter: false,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };

    const expected = "\n hono@4.7.0\ninstalled esbuild@0.25.0 with binaries:\n - esbuild\n\n";
    expect(stats.installed_count).toBe(1);
    expect(stats.explicit_request_count).toBe(1);
    expect(stats.bin_count).toBe(1);
    expect(sliceEq(out[0..stats.bytes_written], expected)).toBe(true);

    // ANSI mode produces escape sequences
    const ansi_stats = p.render(TreePrintOptions{
        ansi_colors: true,
        log_level: TreePrintLogLevel.Default,
        has_installed_filter: false,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(containsSubstr(out[0..ansi_stats.bytes_written], "\x1b[32minstalled")).toBe(true);
    expect(containsSubstr(out[0..ansi_stats.bytes_written], "esbuild")).toBe(true);
});

test("pkg tree_printer: silent mode, failed update abort, invalid id, and BufferTooSmall guard", () => {
    let p = InstallTreePrinter.init();
    const root_id = p.addPackage("app", "1.0.0", true, true) catch 99;
    const pkg_id = p.addPackage("axios", "1.8.0", false, true) catch 99;
    * = p.addDep(TreeDepEdge{ owner_pkg_id: root_id, dep_name: "axios", target_pkg_id: pkg_id }) catch 99;

    // Silent mode returns 0 bytes
    let out: [256]u8 = [0; 256];
    const silent_stats = p.render(TreePrintOptions{
        log_level: TreePrintLogLevel.Silent,
    }, out[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(silent_stats.bytes_written).toBe(0);

    // Invalid package id rejected
    let invalid_id_err = false;
    p.setPackageLaterVersion(999, "2.0.0") catch {
        invalid_id_err = true;
    };
    expect(invalid_id_err).toBe(true);

    // BufferTooSmall rejected cleanly
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = p.render(TreePrintOptions{
        log_level: TreePrintLogLevel.Default,
    }, tiny[0..]) catch {
        buf_err = true;
    };
    expect(buf_err).toBe(true);

    // Failed update request aborts rendering with UpdateFailed
    * = p.addUpdateRequest("axios", true) catch 99;
    expect(p.shouldPrintDep(0, true) == TreePrintDecisionKind.Abort).toBe(true);

    let failed_err = false;
    * = p.render(TreePrintOptions{
        log_level: TreePrintLogLevel.Default,
    }, out[0..]) catch {
        failed_err = true;
    };
    expect(failed_err).toBe(true);
});
