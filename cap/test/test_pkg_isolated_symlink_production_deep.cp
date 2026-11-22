// cap/test/test_pkg_isolated_symlink_production_deep.cp
// Production-deep regression tests for cap:pkg isolated_symlink module
// Source: /data/dev/bun/src/install/isolated_install/Symlinker.cp & /data/dev/bun/docs/pm/isolated-installs.mdx

import { test, expect } from "cap:test";
import {
    ISOLATED_LINK_MAX_OPS,
    IsolatedSymlinkError,
    IsolatedLinkStrategy,
    IsolatedLinkPlatform,
    IsolatedDestEntryKind,
    IsolatedLinkAction,
    stripTrailingSlash,
    reconcileIsolatedSymlink,
    computeRelativeSymlinkTarget,
    IsolatedLinkPlan,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("pkg isolated_symlink: ExpectExisting on POSIX protects bun patch RealDirectory") {
    const want = ".bun/lodash@4.17.21/node_modules/lodash";

    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Posix,
        IsolatedDestEntryKind.Missing,
        "",
        want,
        "",
    ) == IsolatedLinkAction.CreateSymlink);

    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Posix,
        IsolatedDestEntryKind.MissingParent,
        "",
        want,
        "",
    ) == IsolatedLinkAction.MkdirAndCreateSymlink);

    // Critical bun patch invariant: never destroy a real directory
    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Posix,
        IsolatedDestEntryKind.RealDirectory,
        "",
        want,
        "",
    ) == IsolatedLinkAction.PreservePatchedDir);

    // Regular file replaced via unlink
    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Posix,
        IsolatedDestEntryKind.RegularFile,
        "",
        want,
        "",
    ) == IsolatedLinkAction.UnlinkAndCreateSymlink);

    // Matching symlink (with trailing slash) -> Noop
    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Posix,
        IsolatedDestEntryKind.Symlink,
        ".bun/lodash@4.17.21/node_modules/lodash/",
        want,
        "",
    ) == IsolatedLinkAction.Noop);

    // Mismatched symlink -> UnlinkAndCreateSymlink on POSIX
    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Posix,
        IsolatedDestEntryKind.Symlink,
        ".bun/lodash@4.17.20/node_modules/lodash",
        want,
        "",
    ) == IsolatedLinkAction.UnlinkAndCreateSymlink);
}

test("pkg isolated_symlink: ExpectExisting on Windows junction fallback & rmdir") {
    const rel_want = ".bun\\react@19.0.0\\node_modules\\react";
    const abs_fb = "C:\\proj\\node_modules\\.bun\\react@19.0.0\\node_modules\\react";

    // libuv appends trailing backslash to Windows junctions -> must match fallback and Noop
    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Windows,
        IsolatedDestEntryKind.Symlink,
        "C:\\proj\\node_modules\\.bun\\react@19.0.0\\node_modules\\react\\",
        rel_want,
        abs_fb,
    ) == IsolatedLinkAction.Noop);

    // Mismatched Windows directory symlink -> RmdirAndCreateSymlink
    expect(reconcileIsolatedSymlink(
        IsolatedLinkStrategy.ExpectExisting,
        IsolatedLinkPlatform.Windows,
        IsolatedDestEntryKind.Symlink,
        "C:\\proj\\node_modules\\.bun\\react@18.2.0\\node_modules\\react\\",
        rel_want,
        abs_fb,
    ) == IsolatedLinkAction.RmdirAndCreateSymlink);
}

test("pkg isolated_symlink: ExpectMissing & IgnoreFailure strategies") {
    const want = ".bun/zod@3.23.0/node_modules/zod";

    // ExpectMissing deletes any pre-existing entry before creating
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.ExpectMissing, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.Missing, "", want, "") == IsolatedLinkAction.CreateSymlink);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.ExpectMissing, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.MissingParent, "", want, "") == IsolatedLinkAction.MkdirAndCreateSymlink);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.ExpectMissing, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.Symlink, "old", want, "") == IsolatedLinkAction.DeleteTreeAndCreateSymlink);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.ExpectMissing, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.RealDirectory, "", want, "") == IsolatedLinkAction.DeleteTreeAndCreateSymlink);

    // IgnoreFailure only creates when missing or missing parent; existing is Noop
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.IgnoreFailure, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.Missing, "", want, "") == IsolatedLinkAction.CreateSymlink);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.IgnoreFailure, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.MissingParent, "", want, "") == IsolatedLinkAction.MkdirAndCreateSymlink);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.IgnoreFailure, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.Symlink, "old", want, "") == IsolatedLinkAction.Noop);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.IgnoreFailure, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.RegularFile, "", want, "") == IsolatedLinkAction.Noop);
    expect(reconcileIsolatedSymlink(IsolatedLinkStrategy.IgnoreFailure, IsolatedLinkPlatform.Posix, IsolatedDestEntryKind.RealDirectory, "", want, "") == IsolatedLinkAction.Noop);
}

test("pkg isolated_symlink: computeRelativeSymlinkTarget unscoped, scoped & workspace links") {
    let buf: [256]u8 = [0; 256];

    // Unscoped package in top-level node_modules
    const n1 = computeRelativeSymlinkTarget(
        "node_modules",
        "node_modules/.bun/foo@1.0.0/node_modules/foo",
        buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(buf[0..n1], ".bun/foo@1.0.0/node_modules/foo"));

    // Scoped package under node_modules/@scope
    const n2 = computeRelativeSymlinkTarget(
        "node_modules/@scope",
        "node_modules/.bun/@scope+bar@2.1.0/node_modules/@scope/bar",
        buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "../.bun/@scope+bar@2.1.0/node_modules/@scope/bar"));

    // Workspace dependency symlink directly to source directory
    const n3 = computeRelativeSymlinkTarget(
        "packages/app/node_modules",
        "packages/core",
        buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "../../core"));

    // Windows backslashes + redundant '.' and '..' segments normalized
    const n4 = computeRelativeSymlinkTarget(
        "packages\\web\\.\\node_modules\\@ws",
        "packages\\ui\\..\\shared",
        buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(buf[0..n4], "../../../shared"));

    // Identical paths -> "."
    const n5 = computeRelativeSymlinkTarget("node_modules/a", "node_modules/a/", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n5], "."));
}

test("pkg isolated_symlink: stripTrailingSlash boundary cases") {
    expect(strEq(stripTrailingSlash("/"), "/"));
    expect(strEq(stripTrailingSlash(""), ""));
    expect(strEq(stripTrailingSlash("a/b/c///"), "a/b/c"));
    expect(strEq(stripTrailingSlash("C:\\foo\\bar\\\\"), "C:\\foo\\bar"));
    expect(strEq(stripTrailingSlash("plain"), "plain"));
}

test("pkg isolated_symlink: malformed input & capacity boundary guards") {
    let buf: [64]u8 = [0; 64];

    // Empty paths rejected
    let e1 = false;
    * = computeRelativeSymlinkTarget("", "node_modules/a", buf[0..]) catch { e1 = true; };
    expect(e1);
    let e2 = false;
    * = computeRelativeSymlinkTarget("node_modules", "", buf[0..]) catch { e2 = true; };
    expect(e2);

    // BufferTooSmall rejected
    let tiny: [4]u8 = [0; 4];
    let e3 = false;
    * = computeRelativeSymlinkTarget("node_modules/@scope", "node_modules/.bun/foo@1.0.0", tiny[0..]) catch { e3 = true; };
    expect(e3);

    // InvalidLinkTarget in IsolatedLinkPlan
    let plan = IsolatedLinkPlan.init(IsolatedLinkPlatform.Posix);
    let e4 = false;
    * = plan.planLink(IsolatedLinkStrategy.ExpectExisting, "", IsolatedDestEntryKind.Missing, "", "target", "") catch { e4 = true; };
    expect(e4);
    let e5 = false;
    * = plan.planLink(IsolatedLinkStrategy.ExpectExisting, "dest", IsolatedDestEntryKind.Missing, "", "", "") catch { e5 = true; };
    expect(e5);

    // Capacity overflow guard (ISOLATED_LINK_MAX_OPS = 64)
    let i: usize = 0;
    while (i < ISOLATED_LINK_MAX_OPS) {
        * = plan.planLink(IsolatedLinkStrategy.ExpectExisting, "d", IsolatedDestEntryKind.Missing, "", "t", "") catch {
            expect(false);
            return;
        };
        i += 1;
    }
    let cap_err = false;
    * = plan.planLink(IsolatedLinkStrategy.ExpectExisting, "d", IsolatedDestEntryKind.Missing, "", "t", "") catch { cap_err = true; };
    expect(cap_err);
}

test("pkg isolated_symlink: batch IsolatedLinkPlan monorepo reconciliation round-trip") {
    let plan = IsolatedLinkPlan.init(IsolatedLinkPlatform.Posix);

    // 1. Fresh install for @acme/ui (missing parent @acme/)
    const a1 = plan.planLink(
        IsolatedLinkStrategy.ExpectExisting,
        "node_modules/@acme/ui",
        IsolatedDestEntryKind.MissingParent,
        "",
        "../.bun/@acme+ui@2.0.0/node_modules/@acme/ui",
        "",
    ) catch { expect(false); return; };
    expect(a1 == IsolatedLinkAction.MkdirAndCreateSymlink);

    // 2. Up-to-date react link -> Noop
    const a2 = plan.planLink(
        IsolatedLinkStrategy.ExpectExisting,
        "node_modules/react",
        IsolatedDestEntryKind.Symlink,
        ".bun/react@19.0.0/node_modules/react",
        ".bun/react@19.0.0/node_modules/react",
        "",
    ) catch { expect(false); return; };
    expect(a2 == IsolatedLinkAction.Noop);

    // 3. Stale zod version -> UnlinkAndCreateSymlink
    const a3 = plan.planLink(
        IsolatedLinkStrategy.ExpectExisting,
        "node_modules/zod",
        IsolatedDestEntryKind.Symlink,
        ".bun/zod@3.22.0/node_modules/zod",
        ".bun/zod@3.23.8/node_modules/zod",
        "",
    ) catch { expect(false); return; };
    expect(a3 == IsolatedLinkAction.UnlinkAndCreateSymlink);

    // 4. Active bun patch directory for lodash -> PreservePatchedDir
    const a4 = plan.planLink(
        IsolatedLinkStrategy.ExpectExisting,
        "node_modules/lodash",
        IsolatedDestEntryKind.RealDirectory,
        "",
        ".bun/lodash@4.17.21/node_modules/lodash",
        "",
    ) catch { expect(false); return; };
    expect(a4 == IsolatedLinkAction.PreservePatchedDir);

    const st = plan.stats();
    expect(st.total == 4);
    expect(st.create_count == 1);
    expect(st.noop_count == 1);
    expect(st.replace_count == 1);
    expect(st.preserved_patch_count == 1);
}
