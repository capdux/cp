// Production Deep Test for cap:pkg (cache_dirs.cp)
// Covers: multi-tier cache directory precedence & disabled-cache local fallback,
// EXDEV same-filesystem .tmp fallback, NPM/custom-registry/long-host/pre+build/patch_hash naming,
// GitHub & Tarball folder naming, Global Virtual Store eligibility propagation & 16-hex link path formatting.

import { test, assert, assertEq } from "cap:test";
import {
    PKG_CACHE_VERSION_CURRENT,
    PKG_CACHE_MAX_HOSTNAME_INLINE,
    hashCacheKey64,
    PkgCacheDirSource,
    PkgCacheEnv,
    resolvePkgCacheDirPath,
    resolvePkgTempDirFallback,
    PkgCacheSemver,
    formatCachedNpmFolderName,
    formatCachedGitHubFolderName,
    formatCachedTarballFolderName,
    PkgGlobalStoreEligibility,
    PkgGlobalStoreInput,
    checkGlobalStoreEligibility,
    formatGlobalStoreLinkDir,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("cache_dirs: multi-tier environment precedence and cache_enabled=false fallback", () => {
    assertEq(PKG_CACHE_VERSION_CURRENT, 1);
    assertEq(PKG_CACHE_MAX_HOSTNAME_INLINE, 32);

    let buf: [128]u8 = [0; 128];

    // Tier 1: explicit_cache_dir wins over all env vars
    const r1 = resolvePkgCacheDirPath(PkgCacheEnv{
        explicit_cache_dir: "/custom/cache/",
        bun_install_cache_dir: "/env/cache",
        home_dir: "/home/u",
    }, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(r1.source == PkgCacheDirSource.ExplicitConfig);
    assert(strEql(buf[0..r1.len], "/custom/cache"));

    // Tier 2: BUN_INSTALL_CACHE_DIR
    const r2 = resolvePkgCacheDirPath(PkgCacheEnv{
        bun_install_cache_dir: "/var/bun-cache/",
        bun_install: "/opt/bun",
    }, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(r2.source == PkgCacheDirSource.EnvCacheDir);
    assert(strEql(buf[0..r2.len], "/var/bun-cache"));

    // Tier 3: BUN_INSTALL
    const r3 = resolvePkgCacheDirPath(PkgCacheEnv{
        bun_install: "/opt/bun/",
        xdg_cache_home: "/home/u/.cache",
    }, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(r3.source == PkgCacheDirSource.EnvBunInstall);
    assert(strEql(buf[0..r3.len], "/opt/bun/install/cache"));

    // Tier 4: XDG_CACHE_HOME
    const r4 = resolvePkgCacheDirPath(PkgCacheEnv{
        xdg_cache_home: "/home/u/.cache",
        home_dir: "/home/u",
    }, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(r4.source == PkgCacheDirSource.XdgCacheHome);
    assert(strEql(buf[0..r4.len], "/home/u/.cache/bun/install/cache"));

    // Tier 5: HOME default
    const r5 = resolvePkgCacheDirPath(PkgCacheEnv{
        home_dir: "/home/u/",
    }, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(r5.source == PkgCacheDirSource.HomeDefault);
    assert(strEql(buf[0..r5.len], "/home/u/.bun/install/cache"));

    // Disabled cache forces project-local node_modules/.cache
    const r6 = resolvePkgCacheDirPath(PkgCacheEnv{
        explicit_cache_dir: "/custom/cache",
        project_root: "/workspace/app/",
        cache_enabled: false,
    }, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(r6.source == PkgCacheDirSource.LocalNodeModulesFallback);
    assert(strEql(buf[0..r6.len], "/workspace/app/node_modules/.cache"));
});

test("cache_dirs: temp dir fallback on cross-device rename failure", () => {
    let buf: [128]u8 = [0; 128];

    const n1 = resolvePkgTempDirFallback("/home/u/.bun/install/cache", "/tmp/", false, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n1], "/tmp"));

    // Cross-device failure must fall back to <cache_dir>/.tmp
    const n2 = resolvePkgTempDirFallback("/home/u/.bun/install/cache/", "/tmp", true, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n2], "/home/u/.bun/install/cache/.tmp"));

    // Missing system temp dir also falls back to <cache_dir>/.tmp
    const n3 = resolvePkgTempDirFallback("/data/cache", null, false, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n3], "/data/cache/.tmp"));
});

test("cache_dirs: NPM cache folder naming (default, pre+build, custom host, long host, patch_hash)", () => {
    let buf: [160]u8 = [0; 160];

    const n1 = formatCachedNpmFolderName(
        "react",
        PkgCacheSemver{ major: 18, minor: 3, patch: 1 },
        null,
        null,
        null,
        buf[0..],
    ) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n1], "react@18.3.1@@@1"));

    // Pre-release (lowercase hex) + build (uppercase hex) + patch_hash
    const n2 = formatCachedNpmFolderName(
        "@scope/ui",
        PkgCacheSemver{ major: 2, minor: 0, patch: 0, pre_hash: 0xab12, build_hash: 0xcd34 },
        null,
        null,
        0xdeadbeef,
        buf[0..],
    ) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n2], "@scope/ui@2.0.0-ab12+CD34@@@1_patch_hash=deadbeef"));

    // Custom short registry host (<= 32 chars)
    const n3 = formatCachedNpmFolderName(
        "pkg",
        PkgCacheSemver{ major: 1, minor: 2, patch: 3 },
        "npm.pkg.github.com",
        "https://npm.pkg.github.com/",
        null,
        buf[0..],
    ) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n3], "pkg@1.2.3@@npm.pkg.github.com@@@1"));

    // Custom long registry host (> 32 chars) truncates to 12 chars + __<hash>
    const long_host = "very-long-internal-enterprise-registry.corp.example.com";
    const n4 = formatCachedNpmFolderName(
        "pkg",
        PkgCacheSemver{ major: 1, minor: 0, patch: 0 },
        long_host,
        "https://very-long-internal-enterprise-registry.corp.example.com/npm/",
        null,
        buf[0..],
    ) catch {
        assert(false);
        return;
    };
    assert(startsWith(buf[0..n4], "pkg@1.0.0@@very-long-in__"));
});

test("cache_dirs: GitHub and Tarball cache folder naming", () => {
    let buf: [128]u8 = [0; 128];

    const gh_n = formatCachedGitHubFolderName("oven-sh", "bun", "main", 0x1a2b, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..gh_n], "@GH@oven-sh-bun-main@@@1_patch_hash=1a2b"));

    const tgz_n1 = formatCachedTarballFolderName("https://example.com/pkg-1.0.0.tgz", null, buf[0..]) catch {
        assert(false);
        return;
    };
    assert(startsWith(buf[0..tgz_n1], "@T@"));

    // Deterministic hash stability
    const h1 = hashCacheKey64("https://example.com/pkg-1.0.0.tgz");
    const h2 = hashCacheKey64("https://example.com/pkg-1.0.0.tgz");
    const h3 = hashCacheKey64("https://example.com/pkg-2.0.0.tgz");
    assertEq(h1, h2);
    assert(h1 != h3);
});

test("cache_dirs: Global Virtual Store eligibility and 16-hex link dir layout", () => {
    assert(checkGlobalStoreEligibility(PkgGlobalStoreInput{}) == PkgGlobalStoreEligibility.Eligible);
    assert(checkGlobalStoreEligibility(PkgGlobalStoreInput{ has_patch: true }) == PkgGlobalStoreEligibility.IneligiblePatched);
    assert(checkGlobalStoreEligibility(PkgGlobalStoreInput{ is_trusted: true }) == PkgGlobalStoreEligibility.IneligibleTrusted);
    assert(checkGlobalStoreEligibility(PkgGlobalStoreInput{ is_local_protocol: true }) == PkgGlobalStoreEligibility.IneligibleLocalProtocol);
    assert(checkGlobalStoreEligibility(PkgGlobalStoreInput{ has_local_transitive_dep: true }) == PkgGlobalStoreEligibility.IneligibleTransitiveLocal);

    let buf: [256]u8 = [0; 256];
    const ln = formatGlobalStoreLinkDir(
        "/home/dev/.bun/install/cache/",
        "react@18.3.1",
        0x0064d3cd670b3205,
        buf[0..],
    ) catch {
        assert(false);
        return;
    };
    // Must preserve leading zeros in 16-hex entry hash
    assert(strEql(buf[0..ln], "/home/dev/.bun/install/cache/links/react@18.3.1-0064d3cd670b3205/node_modules"));
});

test("cache_dirs: error boundary checks (empty inputs, control chars, tiny buffers)", () => {
    let tiny: [8]u8 = [0; 8];
    const e1 = formatCachedNpmFolderName("react", PkgCacheSemver{ major: 18, minor: 0, patch: 0 }, null, null, null, tiny[0..]) catch 9999;
    assertEq(e1, 9999);

    let buf: [64]u8 = [0; 64];
    const e2 = formatCachedNpmFolderName("", PkgCacheSemver{}, null, null, null, buf[0..]) catch 9999;
    assertEq(e2, 9999);

    const e3 = formatCachedGitHubFolderName("owner", "", "main", null, buf[0..]) catch 9999;
    assertEq(e3, 9999);

    const e4 = formatCachedTarballFolderName("", null, buf[0..]) catch 9999;
    assertEq(e4, 9999);

    const e5 = formatGlobalStoreLinkDir("/cache\n", "react@18.3.1", 1, buf[0..]) catch 9999;
    assertEq(e5, 9999);
});
