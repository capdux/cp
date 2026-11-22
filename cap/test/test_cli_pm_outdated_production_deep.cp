// cap/test/test_cli_pm_outdated_production_deep.cp
// 深度生产级回归测试：cap:cli 依赖过期分析、Catalog 分组与对齐表格引擎 (lib/cli/pm_outdated.cp)

import { test } from "cap:test";
import {
    PM_OUTDATED_MAX_PATTERNS,
    PmOutdatedDepKind,
    PmOutdatedUpdateKind,
    PmOutdatedSemver,
    matchOutdatedGlob,
    PmOutdatedFilter,
    PmOutdatedEntry,
    PmOutdatedTable,
} from "cap:cli";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) { return true; }
    if (needle.len > haystack.len) { return false; }
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return true; }
        i += 1;
    }
    return false;
}

test("pm_outdated: semver parsing, prerelease ordering and upgrade classification") {
    const v1 = PmOutdatedSemver.parse("v1.2.3")?;
    const v1_patch = PmOutdatedSemver.parse("1.2.4+build.99")?;
    const v1_minor = PmOutdatedSemver.parse("1.3.0")?;
    const v2_major = PmOutdatedSemver.parse("2.0.0")?;
    const v1_pre = PmOutdatedSemver.parse("1.2.3-beta.1")?;

    if (v1.major != 1 || v1.minor != 2 || v1.patch != 3 || v1.pre.len != 0) {
        return err.SemverFieldMismatch;
    }
    // 正式版 > 同号预发布版
    if (v1_pre.compare(v1) >= 0 || v1.compare(v1_pre) <= 0) {
        return err.PrereleaseOrderMismatch;
    }
    if (v1_pre.classifyUpgradeTo(v1) != PmOutdatedUpdateKind.Prerelease) {
        return err.PrereleaseKindMismatch;
    }
    if (v1.classifyUpgradeTo(v1) != PmOutdatedUpdateKind.UpToDate) {
        return err.UpToDateKindMismatch;
    }
    if (v1.classifyUpgradeTo(v1_patch) != PmOutdatedUpdateKind.Patch) {
        return err.PatchKindMismatch;
    }
    if (v1.classifyUpgradeTo(v1_minor) != PmOutdatedUpdateKind.Minor) {
        return err.MinorKindMismatch;
    }
    if (v1.classifyUpgradeTo(v2_major) != PmOutdatedUpdateKind.Major) {
        return err.MajorKindMismatch;
    }
}

test("pm_outdated: glob matcher and positive/negated filter combinations") {
    if (!matchOutdatedGlob("eslint*", "eslint-plugin-security")) { return err.PrefixGlobFailed; }
    if (!matchOutdatedGlob("@types/*", "@types/bun")) { return err.ScopeGlobFailed; }
    if (matchOutdatedGlob("@types/*", "typescript")) { return err.ScopeGlobFalsePositive; }
    if (!matchOutdatedGlob("pkg-?", "pkg-a") || matchOutdatedGlob("pkg-?", "pkg-ab")) {
        return err.QuestionGlobMismatch;
    }

    // 仅排除规则：!@types/* -> 排除 @types/*，保留其余全部
    let neg_only = PmOutdatedFilter.init();
    * = neg_only.addPattern("!@types/*")?;
    if (neg_only.matches("@types/bun")) { return err.NegOnlyShouldExcludeTypes; }
    if (!neg_only.matches("typescript")) { return err.NegOnlyShouldKeepTs; }

    // 混合正向 + 排除：eslint* 且 !eslint-plugin-*
    let mixed = PmOutdatedFilter.init();
    * = mixed.addPattern("eslint*")?;
    * = mixed.addPattern("!eslint-plugin-*")?;
    if (!mixed.matches("eslint")) { return err.MixedShouldKeepEslint; }
    if (mixed.matches("eslint-plugin-sonarjs")) { return err.MixedShouldExcludePlugin; }
    if (mixed.matches("prettier")) { return err.MixedShouldExcludeUnmatched; }
}

test("pm_outdated: entry display names, upgrade kinds and catalog workspace labels") {
    const e_dev = PmOutdatedEntry{
        package_name: "eslint",
        dep_kind: PmOutdatedDepKind.Dev,
        current: "8.57.1",
        update: "8.57.1",
        latest: "9.20.0",
        workspace_name: "@test/app",
        is_catalog: true,
        catalog_name: "testing",
    };

    if (!(e_dev.isOutdated()?)) { return err.DevEntryShouldBeOutdated; }
    if ((e_dev.updateKind()?) != PmOutdatedUpdateKind.UpToDate) { return err.UpdateKindShouldBeUpToDate; }
    if ((e_dev.latestKind()?) != PmOutdatedUpdateKind.Major) { return err.LatestKindShouldBeMajor; }

    let buf: [128]u8 = [0; 128];
    const dn = e_dev.formatDisplayName(buf[0..])?;
    if (!bytesEq(buf[0..dn], "eslint (dev)")) { return err.DisplayNameMismatch; }

    const wn = e_dev.formatWorkspaceLabel(buf[0..])?;
    if (!bytesEq(buf[0..wn], "catalog:testing (@test/app)")) { return err.NamedCatalogLabelMismatch; }

    // 默认 catalog（catalog_name 为空或 "default"）不带冒号后缀
    const e_def = PmOutdatedEntry{
        package_name: "react",
        current: "17.0.0",
        update: "17.0.0",
        latest: "19.1.1",
        workspace_name: "@test/web",
        is_catalog: true,
        catalog_name: "default",
    };
    const def_wn = e_def.formatWorkspaceLabel(buf[0..])?;
    if (!bytesEq(buf[0..def_wn], "catalog (@test/web)")) { return err.DefaultCatalogLabelMismatch; }
}

test("pm_outdated: table skips up-to-date entries and groups shared catalog across workspaces") {
    let tbl = PmOutdatedTable.init();
    const empty_filter = PmOutdatedFilter.init();

    // 已是最新 -> 跳过并计数
    const added_fresh = tbl.addEntry(PmOutdatedEntry{
        package_name: "zod",
        current: "3.24.0",
        update: "3.24.0",
        latest: "3.24.0",
    }, empty_filter)?;
    if (added_fresh || tbl.up_to_date_skipped != 1) { return err.UpToDateNotSkipped; }

    // 同一默认 catalog 的 lodash 被 @app/a、@app/b、以及重复的 @app/a 引用 -> 应合并为 1 条记录
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "lodash",
        current: "4.17.15",
        update: "4.17.21",
        latest: "4.17.21",
        workspace_name: "@app/a",
        is_catalog: true,
        catalog_name: "default",
    }, empty_filter)?;
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "lodash",
        current: "4.17.15",
        update: "4.17.21",
        latest: "4.17.21",
        workspace_name: "@app/b",
        is_catalog: true,
        catalog_name: "default",
    }, empty_filter)?;
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "lodash",
        current: "4.17.15",
        update: "4.17.21",
        latest: "4.17.21",
        workspace_name: "@app/a",
        is_catalog: true,
        catalog_name: "default",
    }, empty_filter)?;

    if (tbl.entry_count != 1) { return err.CatalogGroupDidNotMerge; }
    let wbuf: [128]u8 = [0; 128];
    const wn = tbl.slice()[0].formatWorkspaceLabel(wbuf[0..])?;
    if (!bytesEq(wbuf[0..wn], "catalog (@app/a, @app/b)")) {
        return err.GroupedCatalogLabelMismatch;
    }
}

test("pm_outdated: deterministic sorting puts normal deps before dev/peer/optional") {
    let tbl = PmOutdatedTable.init();
    const f = PmOutdatedFilter.init();

    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "typescript",
        dep_kind: PmOutdatedDepKind.Dev,
        current: "5.7.2",
        update: "5.7.3",
        latest: "5.7.3",
    }, f)?;
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "react",
        dep_kind: PmOutdatedDepKind.Peer,
        current: "18.0.0",
        update: "18.3.1",
        latest: "19.0.0",
    }, f)?;
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "axios",
        dep_kind: PmOutdatedDepKind.Normal,
        current: "1.6.0",
        update: "1.7.9",
        latest: "1.7.9",
    }, f)?;
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "@sinclair/typebox",
        dep_kind: PmOutdatedDepKind.Normal,
        current: "0.34.15",
        update: "0.34.16",
        latest: "0.34.16",
    }, f)?;

    tbl.sortDeterministic();
    const rows = tbl.slice();
    if (rows.len != 4) { return err.SortLenMismatch; }
    if (!bytesEq(rows[0].package_name, "@sinclair/typebox") ||
        !bytesEq(rows[1].package_name, "axios") ||
        !bytesEq(rows[2].package_name, "typescript") ||
        !bytesEq(rows[3].package_name, "react"))
    {
        return err.DeterministicSortOrderMismatch;
    }
}

test("pm_outdated: markdown table rendering with and without workspace column") {
    let tbl = PmOutdatedTable.init();
    const f = PmOutdatedFilter.init();

    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "tsup",
        dep_kind: PmOutdatedDepKind.Dev,
        current: "8.3.5",
        update: "8.3.6",
        latest: "8.3.6",
        workspace_name: "@monorepo/cli",
    }, f)?;
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "cors",
        dep_kind: PmOutdatedDepKind.Normal,
        current: "2.8.0",
        update: "2.8.0",
        latest: "2.8.5",
        workspace_name: "@monorepo/api",
    }, f)?;
    tbl.sortDeterministic();

    let buf4: [512]u8 = [0; 512];
    const n4 = tbl.formatMarkdownTable(buf4[0..], false)?;
    const s4 = buf4[0..n4];
    if (!containsStr(s4, "| Package    | Current | Update | Latest |") ||
        !containsStr(s4, "| cors       | 2.8.0   | 2.8.0  | 2.8.5  |") ||
        !containsStr(s4, "| tsup (dev) | 8.3.5   | 8.3.6  | 8.3.6  |") ||
        containsStr(s4, "Workspace"))
    {
        return err.FourColTableMismatch;
    }

    let buf5: [512]u8 = [0; 512];
    const n5 = tbl.formatMarkdownTable(buf5[0..], true)?;
    const s5 = buf5[0..n5];
    if (!containsStr(s5, "| Workspace     |") ||
        !containsStr(s5, "| @monorepo/api |") ||
        !containsStr(s5, "| @monorepo/cli |"))
    {
        return err.FiveColTableMismatch;
    }
}

test("pm_outdated: empty table renders zero bytes cleanly") {
    const tbl = PmOutdatedTable.init();
    let buf: [64]u8 = [0; 64];
    const n = tbl.formatMarkdownTable(buf[0..], true)?;
    if (n != 0) { return err.EmptyTableShouldBeZero; }
}

test("pm_outdated: malformed semver, empty name, capacity and buffer guards") {
    // 非法 Semver 格式拒绝
    let bad_ver_1 = false;
    const v_dummy = PmOutdatedSemver.parse("1.2") catch blk: {
        bad_ver_1 = true;
        break :blk PmOutdatedSemver{};
    };
    * = v_dummy;
    if (!bad_ver_1) { return err.TwoPartSemverNotRejected; }

    let bad_ver_2 = false;
    const v_dummy2 = PmOutdatedSemver.parse("1.2.x") catch blk: {
        bad_ver_2 = true;
        break :blk PmOutdatedSemver{};
    };
    * = v_dummy2;
    if (!bad_ver_2) { return err.NonNumericSemverNotRejected; }

    let bad_ver_3 = false;
    const v_dummy3 = PmOutdatedSemver.parse("1.2.3-") catch blk: {
        bad_ver_3 = true;
        break :blk PmOutdatedSemver{};
    };
    * = v_dummy3;
    if (!bad_ver_3) { return err.TrailingDashSemverNotRejected; }

    // 空包名拒绝
    let tbl = PmOutdatedTable.init();
    const f_empty = PmOutdatedFilter.init();
    let empty_name_err = false;
    tbl.addEntry(PmOutdatedEntry{
        package_name: "",
        current: "1.0.0",
        update: "1.0.1",
        latest: "1.0.1",
    }, f_empty) catch {
        empty_name_err = true;
    };
    if (!empty_name_err) { return err.EmptyPkgNameNotRejected; }

    // 过滤规则超限拦截
    let flt = PmOutdatedFilter.init();
    let pi: usize = 0;
    while (pi < PM_OUTDATED_MAX_PATTERNS) {
        * = flt.addPattern("pkg*")?;
        pi += 1;
    }
    let pat_overflow = false;
    flt.addPattern("extra*") catch {
        pat_overflow = true;
    };
    if (!pat_overflow) { return err.PatternOverflowNotRejected; }

    // 表格输出缓冲区过小拦截 BufferTooSmall
    * = tbl.addEntry(PmOutdatedEntry{
        package_name: "lodash",
        current: "4.17.15",
        update: "4.17.21",
        latest: "4.17.21",
    }, f_empty)?;
    let tiny: [16]u8 = [0; 16];
    let buf_small = false;
    tbl.formatMarkdownTable(tiny[0..], false) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }
}
