// cap/test/test_pkg_folder_resolver_production_deep.cp
// Production-deep regression tests for cap:pkg folder_resolver module
// Source: /data/dev/bun/src/install/resolvers/folder_resolver.cp & /data/dev/bun/docs/pm/workspaces.mdx

import { test, expect } from "cap:test";
import {
    FOLDER_RESOLVER_MAX_ENTRIES,
    FolderResolverError,
    FolderResolverKind,
    FolderResolutionTag,
    FolderResolutionFailKind,
    hashFolderPath,
    stripFolderProtocolPrefix,
    shouldCheckBundledDependencies,
    normalizeFolderPackageJsonPath,
    formatWorkspaceSearchPath,
    resolveWorkspacePublishVersion,
    FolderResolverCache,
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

test("pkg folder_resolver: normalizeFolderPackageJsonPath across workspace, file, global & cache") {
    let abs_buf: [256]u8 = [0; 256];
    let rel_buf: [256]u8 = [0; 256];

    // 1. Workspace relative path with trailing slash
    const l1 = normalizeFolderPackageJsonPath(
        FolderResolverKind.Workspace,
        "/repo",
        "",
        "workspace:packages/pkg-a/",
        abs_buf[0..],
        rel_buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(abs_buf[0..l1.abs_len], "/repo/packages/pkg-a/package.json"));
    expect(strEq(rel_buf[0..l1.rel_len], "packages/pkg-a"));

    // 2. file: protocol navigating to sibling directory outside repo
    const l2 = normalizeFolderPackageJsonPath(
        FolderResolverKind.Folder,
        "/work/app",
        "",
        "file:../shared-lib",
        abs_buf[0..],
        rel_buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(abs_buf[0..l2.abs_len], "/work/shared-lib/package.json"));
    expect(strEq(rel_buf[0..l2.rel_len], "../shared-lib"));

    // 3. GlobalLink with base_override
    const l3 = normalizeFolderPackageJsonPath(
        FolderResolverKind.GlobalLink,
        "/work/app",
        "/home/dev/.bun/install/global",
        "link:my-tool",
        abs_buf[0..],
        rel_buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(abs_buf[0..l3.abs_len], "/home/dev/.bun/install/global/my-tool/package.json"));
    expect(strEq(rel_buf[0..l3.rel_len], "../../home/dev/.bun/install/global/my-tool"));

    // 4. Current root "."
    const l4 = normalizeFolderPackageJsonPath(
        FolderResolverKind.Folder,
        "/repo",
        "",
        ".",
        abs_buf[0..],
        rel_buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(abs_buf[0..l4.abs_len], "/repo/package.json"));
    expect(strEq(rel_buf[0..l4.rel_len], "."));
}

test("pkg folder_resolver: protocol prefix stripping & bundledDependencies policy") {
    expect(strEq(stripFolderProtocolPrefix("file:./local"), "./local"));
    expect(strEq(stripFolderProtocolPrefix("link:my-pkg"), "my-pkg"));
    expect(strEq(stripFolderProtocolPrefix("workspace:packages/a"), "packages/a"));
    expect(strEq(stripFolderProtocolPrefix("packages/b"), "packages/b"));

    expect(shouldCheckBundledDependencies(FolderResolverKind.Folder));
    expect(shouldCheckBundledDependencies(FolderResolverKind.Symlink));
    expect(shouldCheckBundledDependencies(FolderResolverKind.CacheFolder));
    expect(shouldCheckBundledDependencies(FolderResolverKind.GlobalLink));
    expect(!shouldCheckBundledDependencies(FolderResolverKind.Workspace));
}

test("pkg folder_resolver: formatWorkspaceSearchPath prefixing & JSON quoting") {
    let buf: [128]u8 = [0; 128];

    const n1 = formatWorkspaceSearchPath("packages/a", false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "./packages/a"));

    const n2 = formatWorkspaceSearchPath("packages/a", true, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "\"./packages/a\""));

    // Already starts with '.' or '/' -> no extra "./"
    const n3 = formatWorkspaceSearchPath("../shared", true, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "\"../shared\""));

    const n4 = formatWorkspaceSearchPath("/abs/pkg", false, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n4], "/abs/pkg"));

    // Escapes embedded quotes and backslashes when quoted = true
    const n5 = formatWorkspaceSearchPath("pkg\\\"x", true, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n5], "\"./pkg\\\\\\\"x\""));
}

test("pkg folder_resolver: resolveWorkspacePublishVersion exact workspaces.mdx semantics") {
    let buf: [64]u8 = [0; 64];

    // "workspace:*" -> "1.0.1"
    const n1 = resolveWorkspacePublishVersion("workspace:*", "1.0.1", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "1.0.1"));

    // "workspace:^" -> "^1.0.1"
    const n2 = resolveWorkspacePublishVersion("workspace:^", "1.0.1", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "^1.0.1"));

    // "workspace:~" -> "~1.0.1"
    const n3 = resolveWorkspacePublishVersion("workspace:~", "1.0.1", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "~1.0.1"));

    // Explicit version takes precedence over current package.json version
    const n4 = resolveWorkspacePublishVersion("workspace:1.0.2", "1.0.1", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n4], "1.0.2"));

    // Invalid specs rejected
    let e1 = false;
    * = resolveWorkspacePublishVersion("^1.0.0", "1.0.1", buf[0..]) catch { e1 = true; };
    expect(e1);
    let e2 = false;
    * = resolveWorkspacePublishVersion("workspace:", "1.0.1", buf[0..]) catch { e2 = true; };
    expect(e2);
    let e3 = false;
    * = resolveWorkspacePublishVersion("workspace:*", "", buf[0..]) catch { e3 = true; };
    expect(e3);
}

test("pkg folder_resolver: FolderResolverCache dedup, error caching & stats") {
    let cache = FolderResolverCache.init();
    expect(cache.lookup("packages/a") == null);

    const s0 = cache.putResolved("packages/a", 10, true) catch { expect(false); return; };
    expect(s0 == 0);
    if (e := cache.lookup("packages/a")) {
        expect(e.tag == FolderResolutionTag.NewPackageId);
        expect(e.package_id == 10);
        expect(e.path_hash == hashFolderPath("packages/a"));
    } else {
        expect(false);
    }

    // In-place update reuses slot 0
    const s0_upd = cache.putResolved("packages/a", 10, false) catch { expect(false); return; };
    expect(s0_upd == 0);
    if (e := cache.lookup("packages/a")) {
        expect(e.tag == FolderResolutionTag.PackageId);
    } else {
        expect(false);
    }

    // Error entry cached
    * = cache.putError("packages/broken", FolderResolutionFailKind.MissingPackageJson) catch { expect(false); return; };
    if (eb := cache.lookup("packages/broken")) {
        expect(eb.tag == FolderResolutionTag.Failed);
        expect(eb.fail_kind == FolderResolutionFailKind.MissingPackageJson);
    } else {
        expect(false);
    }

    const st = cache.stats();
    expect(st.entry_count == 2);
    expect(st.resolved_count == 1);
    expect(st.failed_count == 1);
}

test("pkg folder_resolver: malformed input & capacity boundary guards") {
    let abs_buf: [64]u8 = [0; 64];
    let rel_buf: [64]u8 = [0; 64];
    let tiny: [4]u8 = [0; 4];

    // Empty path rejected
    let e1 = false;
    * = normalizeFolderPackageJsonPath(FolderResolverKind.Folder, "/repo", "", "", abs_buf[0..], rel_buf[0..]) catch { e1 = true; };
    expect(e1);

    let e2 = false;
    * = formatWorkspaceSearchPath("", false, abs_buf[0..]) catch { e2 = true; };
    expect(e2);

    // BufferTooSmall rejected
    let e3 = false;
    * = normalizeFolderPackageJsonPath(FolderResolverKind.Workspace, "/repo", "", "packages/a", tiny[0..], rel_buf[0..]) catch { e3 = true; };
    expect(e3);

    // CacheFull at 64 entries
    let cache = FolderResolverCache.init();
    let paths: [64][4]u8 = [[0; 4]; 64];
    let i: usize = 0;
    while (i < FOLDER_RESOLVER_MAX_ENTRIES) {
        paths[i][0] = 'w';
        paths[i][1] = ('0' as u8) + ((i / 10) as u8);
        paths[i][2] = ('0' as u8) + ((i % 10) as u8);
        paths[i][3] = 'p';
        * = cache.putResolved(paths[i][0..], i as u32, true) catch { expect(false); return; };
        i += 1;
    }
    let full_err = false;
    * = cache.putResolved("overflow", 99, true) catch { full_err = true; };
    expect(full_err);
}

test("pkg folder_resolver: monorepo workspace discovery & publish round-trip") {
    let abs_buf: [160]u8 = [0; 160];
    let rel_buf: [128]u8 = [0; 128];
    let disp_buf: [128]u8 = [0; 128];
    let ver_buf: [32]u8 = [0; 32];

    const lens = normalizeFolderPackageJsonPath(
        FolderResolverKind.Workspace,
        "/monorepo",
        "",
        "packages\\pkg-b\\.\\",
        abs_buf[0..],
        rel_buf[0..],
    ) catch { expect(false); return; };
    expect(strEq(abs_buf[0..lens.abs_len], "/monorepo/packages/pkg-b/package.json"));
    const rel = rel_buf[0..lens.rel_len];
    expect(strEq(rel, "packages/pkg-b"));

    const dn = formatWorkspaceSearchPath(rel, true, disp_buf[0..]) catch { expect(false); return; };
    expect(strEq(disp_buf[0..dn], "\"./packages/pkg-b\""));

    let cache = FolderResolverCache.init();
    * = cache.putResolved(rel, 3, true) catch { expect(false); return; };
    expect(cache.lookup(rel) != null);

    const vn = resolveWorkspacePublishVersion("workspace:~", "2.4.0", ver_buf[0..]) catch { expect(false); return; };
    expect(strEq(ver_buf[0..vn], "~2.4.0"));
}
