// cap/test/test_pkg_manifest_cache_production_deep.cp
// Production-deep regression tests for cap:pkg manifest_cache module
// Source: /data/dev/bun/src/install/PackageManifestMap.cp & /data/dev/bun/docs/pm/global-cache.mdx

import { test, expect } from "cap:test";
import {
    MANIFEST_CACHE_MAX_ENTRIES,
    MANIFEST_DEFAULT_CACHE_SUBDIR,
    ManifestCacheError,
    ManifestCacheBehavior,
    ManifestCacheState,
    ManifestLookupOutcome,
    InstallCopyBackend,
    hashPackageManifestName,
    parseInstallCopyBackend,
    formatGlobalCacheEntryDir,
    PackageManifestCache,
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

test("pkg manifest_cache: insertFresh, in-place update & LoadFromMemory lookup") {
    let cache = PackageManifestCache.init(true, true);
    const s1 = cache.insertFresh("lodash", "\"e1\"", "4.17.20", 1_000, false) catch {
        expect(false);
        return;
    };
    expect(s1 == 0);
    expect(cache.count == 1);

    // Updating same package name reuses slot 0
    const s2 = cache.insertFresh("lodash", "\"e2\"", "4.17.21", 2_000, true) catch {
        expect(false);
        return;
    };
    expect(s2 == 0);
    expect(cache.count == 1);

    const r = cache.lookup("lodash", ManifestCacheBehavior.LoadFromMemory, false, false);
    expect(r.outcome == ManifestLookupOutcome.HitFresh);
    expect(!r.is_expired);
    expect(strEq(r.record.etag, "\"e2\""));
    expect(strEq(r.record.latest_version, "4.17.21"));
    expect(r.record.has_extended_manifest);
}

test("pkg manifest_cache: extended manifest upgrade demotion retains ETag when allow_expired") {
    let cache = PackageManifestCache.init(true, true);
    * = cache.insertFresh("react", "\"etag-corgi\"", "19.0.0", 9_999_999, false) catch {
        expect(false);
        return;
    };

    // Normal lookup succeeds as Fresh
    const r1 = cache.lookup("react", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, true, false);
    expect(r1.outcome == ManifestLookupOutcome.HitFresh);
    expect(!r1.demoted_for_extended);

    // Extended manifest lookup demotes Fresh -> Expired, returning HitExpired with ETag
    const r2 = cache.lookup("react", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, true, true);
    expect(r2.outcome == ManifestLookupOutcome.HitExpired);
    expect(r2.is_expired);
    expect(r2.demoted_for_extended);
    expect(strEq(r2.record.etag, "\"etag-corgi\""));
    expect(cache.stats.extended_demotions == 1);

    // Subsequent lookup with allow_expired = false misses cleanly
    const r3 = cache.lookup("react", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, false, true);
    expect(r3.outcome == ManifestLookupOutcome.MissNeedDiskOrNetwork);
    expect(r3.is_expired);
}

test("pkg manifest_cache: negative NotFound caching short-circuits disk probes") {
    let cache = PackageManifestCache.init(true, true);
    // Absent entry asks for disk/network
    const r0 = cache.lookup("nonexistent-pkg", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, true, false);
    expect(r0.outcome == ManifestLookupOutcome.MissNeedDiskOrNetwork);

    // Mark negative cache entry
    * = cache.markNotFound("nonexistent-pkg") catch { expect(false); return; };

    const r1 = cache.lookup("nonexistent-pkg", ManifestCacheBehavior.LoadFromMemoryFallbackToDisk, true, false);
    expect(r1.outcome == ManifestLookupOutcome.MissNotFound);
    const r2 = cache.lookup("nonexistent-pkg", ManifestCacheBehavior.LoadFromMemory, true, false);
    expect(r2.outcome == ManifestLookupOutcome.MissNotFound);
    expect(cache.stats.not_found_hits == 2);
}

test("pkg manifest_cache: ingestDiskManifest Cache-Control freshness & disableManifest") {
    let cache = PackageManifestCache.init(true, true);

    // Fresh disk manifest (public_max_age > now_sec)
    const d1 = cache.ingestDiskManifest("zod", "\"z1\"", "3.23.8", 5000, false, 4999, true, false) catch {
        expect(false);
        return;
    };
    expect(d1.outcome == ManifestLookupOutcome.HitFresh);
    expect(d1.record.state == ManifestCacheState.Fresh);

    // Exact boundary (public_max_age == now_sec) -> Expired
    const d2 = cache.ingestDiskManifest("hono", "\"h1\"", "4.6.0", 5000, false, 5000, true, false) catch {
        expect(false);
        return;
    };
    expect(d2.outcome == ManifestLookupOutcome.HitExpired);
    expect(d2.is_expired);
    expect(d2.record.state == ManifestCacheState.Expired);

    // Disk manifest with needs_extended_manifest = true but has_extended_manifest = false -> Expired
    const d3 = cache.ingestDiskManifest("vite", "\"v1\"", "6.0.0", 9999, false, 1000, true, true) catch {
        expect(false);
        return;
    };
    expect(d3.outcome == ManifestLookupOutcome.HitExpired);
    expect(d3.demoted_for_extended);

    // disableManifest (enable_manifest_cache = false) bypasses disk ingest
    let disabled = PackageManifestCache.init(false, true);
    const d4 = disabled.ingestDiskManifest("zod", "\"z1\"", "3.23.8", 9999, true, 1000, true, false) catch {
        expect(false);
        return;
    };
    expect(d4.outcome == ManifestLookupOutcome.MissNeedDiskOrNetwork);
    expect(disabled.count == 0);
}

test("pkg manifest_cache: formatGlobalCacheEntryDir pre-release/build hashing & traversal guard") {
    expect(strEq(MANIFEST_DEFAULT_CACHE_SUBDIR, ".bun/install/cache"));
    let buf: [128]u8 = [0; 128];

    const n1 = formatGlobalCacheEntryDir("react", "19.0.0", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "react@19.0.0"));

    const n2 = formatGlobalCacheEntryDir("@scope/ui", "2.1.0", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "@scope/ui@2.1.0"));

    // Pre-release suffix replaced by deterministic 8-hex hash
    const n3 = formatGlobalCacheEntryDir("pkg", "1.0.0-beta.0", buf[0..]) catch { expect(false); return; };
    // "pkg@1.0.0-" (10 bytes) + 8 hex chars = 18 bytes
    expect(n3 == 18);
    expect(strEq(buf[0..10], "pkg@1.0.0-"));

    // Build metadata suffix replaced by deterministic 8-hex hash
    let buf2: [128]u8 = [0; 128];
    const n4 = formatGlobalCacheEntryDir("pkg", "1.0.0+20220101", buf2[0..]) catch { expect(false); return; };
    expect(n4 == 18);
    expect(strEq(buf2[0..10], "pkg@1.0.0+"));

    // Path traversal & control chars rejected
    let e1 = false;
    * = formatGlobalCacheEntryDir("../evil", "1.0.0", buf[0..]) catch { e1 = true; };
    expect(e1);
    let e2 = false;
    * = formatGlobalCacheEntryDir("pkg", "1.0.0\n", buf[0..]) catch { e2 = true; };
    expect(e2);
}

test("pkg manifest_cache: parseInstallCopyBackend & deterministic package name hashing") {
    expect(parseInstallCopyBackend("hardlink") == InstallCopyBackend.Hardlink);
    expect(parseInstallCopyBackend("clonefile") == InstallCopyBackend.Clonefile);
    expect(parseInstallCopyBackend("clonefile_each_dir") == InstallCopyBackend.ClonefileEachDir);
    expect(parseInstallCopyBackend("copyfile") == InstallCopyBackend.Copyfile);
    expect(parseInstallCopyBackend("symlink") == InstallCopyBackend.Symlink);
    expect(parseInstallCopyBackend("invalid") == null);

    const h1 = hashPackageManifestName("@scope/pkg");
    const h2 = hashPackageManifestName("@scope/pkg");
    const h3 = hashPackageManifestName("@scope/other");
    expect(h1 == h2);
    expect(h1 != h3);
}

test("pkg manifest_cache: malformed input & capacity boundary guards") {
    let cache = PackageManifestCache.init(true, true);

    // Empty package name rejected
    let e1 = false;
    * = cache.insertFresh("", "", "1.0.0", 0, false) catch { e1 = true; };
    expect(e1);
    let e2 = false;
    * = cache.markNotFound("") catch { e2 = true; };
    expect(e2);

    // BufferTooSmall in formatGlobalCacheEntryDir
    let tiny: [4]u8 = [0; 4];
    let e3 = false;
    * = formatGlobalCacheEntryDir("react", "19.0.0", tiny[0..]) catch { e3 = true; };
    expect(e3);

    // Capacity overflow guard (MANIFEST_CACHE_MAX_ENTRIES = 64)
    let names: [64][4]u8 = [[0; 4]; 64];
    let i: usize = 0;
    while (i < MANIFEST_CACHE_MAX_ENTRIES) {
        names[i][0] = 'p';
        names[i][1] = ('0' as u8) + ((i / 10) as u8);
        names[i][2] = ('0' as u8) + ((i % 10) as u8);
        names[i][3] = 'x';
        * = cache.insertFresh(names[i][0..], "", "1.0.0", 100, false) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect(cache.stats.entry_count == MANIFEST_CACHE_MAX_ENTRIES);

    let full_err = false;
    * = cache.insertFresh("overflow-pkg", "", "1.0.0", 100, false) catch { full_err = true; };
    expect(full_err);
}
