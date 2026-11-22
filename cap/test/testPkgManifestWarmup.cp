import { test, expect } from "cap:test";
import {
    MANIFEST_WARMUP_MAX_PKGS,
    MANIFEST_WARMUP_MAX_DEPS,
    MANIFEST_WARMUP_MAX_EXCLUDES,
    MANIFEST_WARMUP_INVALID_PKG_ID,
    MANIFEST_WARMUP_STABILITY_WINDOW_SECS,
    WarmupResolutionKind,
    WarmupCacheState,
    ReleaseAgeDecision,
    WarmupPkgNode,
    WarmupDepEdge,
    WarmupOptions,
    manifestTaskHash,
    isManifestCacheSatisfied,
    evaluateReleaseAgeGate,
    ManifestWarmupPlanner,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("planAll filters invalid IDs, non-npm resolutions, cache hits, and deduplicates pkg_id", () => {
    let p = ManifestWarmupPlanner.init();

    const p_npm_miss = p.addPackage(WarmupPkgNode{ name: "zod", resolution: .Npm, cache_state: .Miss }) catch unreachable;
    const p_npm_mem = p.addPackage(WarmupPkgNode{ name: "react", resolution: .Npm, cache_state: .MemoryHit }) catch unreachable;
    const p_npm_disk = p.addPackage(WarmupPkgNode{ name: "vite", resolution: .Npm, cache_state: .DiskHit }) catch unreachable;
    const p_git = p.addPackage(WarmupPkgNode{ name: "my-git-dep", resolution: .Git, cache_state: .Miss }) catch unreachable;
    const p_ws = p.addPackage(WarmupPkgNode{ name: "@app/shared", resolution: .Workspace, cache_state: .Miss }) catch unreachable;
    const p_tar = p.addPackage(WarmupPkgNode{ name: "local-tar", resolution: .Tarball, cache_state: .Miss }) catch unreachable;

    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: MANIFEST_WARMUP_INVALID_PKG_ID }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: 999 }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_npm_miss, is_optional: false }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_npm_miss, is_optional: true }) catch unreachable; // duplicate
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_npm_mem }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_npm_disk }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_git }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_ws }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_tar }) catch unreachable;

    const st = p.planAll(WarmupOptions{}) catch unreachable;
    expect(st.scanned_deps == 9);
    expect(st.skipped_invalid_pkg == 2);
    expect(st.skipped_non_npm == 3);
    expect(st.skipped_duplicate_pkg == 1);
    expect(st.skipped_cache_hit == 2);
    expect(st.enqueued_tasks == 1);
    expect(st.extended_manifest_tasks == 0);

    const tasks = p.tasks();
    expect(tasks.len == 1);
    expect(strEq(tasks[0].pkg_name, "zod"));
    expect(!tasks[0].is_optional);
    expect(!tasks[0].needs_extended_manifest);
    expect(tasks[0].task_hash == manifestTaskHash("zod"));
});

test("AbbreviatedOnly cache requires extended manifest only when minimumReleaseAge is active and not excluded", () => {
    expect(isManifestCacheSatisfied(.AbbreviatedOnly, false));
    expect(!isManifestCacheSatisfied(.AbbreviatedOnly, true));
    expect(isManifestCacheSatisfied(.MemoryHit, true));
    expect(isManifestCacheSatisfied(.DiskHit, true));
    expect(!isManifestCacheSatisfied(.Miss, false));

    let p = ManifestWarmupPlanner.init();
    const p_lodash = p.addPackage(WarmupPkgNode{ name: "lodash", resolution: .Npm, cache_state: .AbbreviatedOnly }) catch unreachable;
    const p_types = p.addPackage(WarmupPkgNode{ name: "@types/node", resolution: .Npm, cache_state: .AbbreviatedOnly }) catch unreachable;

    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_lodash }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_types }) catch unreachable;

    // Without minimumReleaseAge: both satisfied by abbreviated cache
    const st_no_age = p.planAll(WarmupOptions{}) catch unreachable;
    expect(st_no_age.enqueued_tasks == 0);
    expect(st_no_age.skipped_cache_hit == 2);

    // With minimumReleaseAge and @types/node excluded: only lodash needs extended manifest
    let opts = WarmupOptions{ minimum_release_age_secs: 259200 };
    * = opts.addExclude("@types/node") catch unreachable;
    const st_age = p.planAll(opts) catch unreachable;
    expect(st_age.enqueued_tasks == 1);
    expect(st_age.extended_manifest_tasks == 1);
    expect(st_age.skipped_cache_hit == 1);

    const tasks = p.tasks();
    expect(tasks.len == 1);
    expect(strEq(tasks[0].pkg_name, "lodash"));
    expect(tasks[0].needs_extended_manifest);
});

test("Optional manifest task upgrades to required when referenced by non-optional dependency", () => {
    let p = ManifestWarmupPlanner.init();
    const p_fsevents = p.addPackage(WarmupPkgNode{ name: "fsevents", resolution: .Npm, cache_state: .Miss }) catch unreachable;

    // First encountered as optional, later as required
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_fsevents, is_optional: true }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: p_fsevents, is_optional: false }) catch unreachable;

    const st = p.planAll(WarmupOptions{}) catch unreachable;
    expect(st.enqueued_tasks == 1);
    expect(st.optional_upgraded_to_required == 1);

    const tasks = p.tasks();
    expect(tasks.len == 1);
    expect(!tasks[0].is_optional);
});

test("planForRootIds only scans direct dependencies of requested root packages", () => {
    let p = ManifestWarmupPlanner.init();

    // Deps layout:
    // dep 0 -> pkg 2 (direct dep of root A)
    // dep 1 -> pkg 3 (direct dep of root A, git -> skipped)
    // dep 2 -> pkg 4 (direct dep of root B, not requested)
    const root_a = p.addPackage(WarmupPkgNode{
        name: "app-a",
        resolution: .Root,
        dep_begin: 0,
        dep_len: 2,
    }) catch unreachable;
    const root_b = p.addPackage(WarmupPkgNode{
        name: "app-b",
        resolution: .Workspace,
        dep_begin: 2,
        dep_len: 1,
    }) catch unreachable;
    const dep_a1 = p.addPackage(WarmupPkgNode{ name: "chalk", resolution: .Npm, cache_state: .Miss }) catch unreachable;
    const dep_a2 = p.addPackage(WarmupPkgNode{ name: "custom-git", resolution: .Github, cache_state: .Miss }) catch unreachable;
    const dep_b1 = p.addPackage(WarmupPkgNode{ name: "express", resolution: .Npm, cache_state: .Miss }) catch unreachable;

    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: dep_a1, is_optional: false }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: dep_a2, is_optional: false }) catch unreachable;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: dep_b1, is_optional: false }) catch unreachable;

    const ids_a: [1]u32 = [root_a];
    const st = p.planForRootIds(ids_a[0..], WarmupOptions{}) catch unreachable;
    expect(st.scanned_deps == 2);
    expect(st.skipped_non_npm == 1);
    expect(st.enqueued_tasks == 1);
    expect(strEq(p.tasks()[0].pkg_name, "chalk"));

    // Requesting both root_a and root_b picks up express too
    const ids_both: [2]u32 = [root_a, root_b];
    const st2 = p.planForRootIds(ids_both[0..], WarmupOptions{}) catch unreachable;
    expect(st2.enqueued_tasks == 2);
});

test("evaluateReleaseAgeGate enforces age threshold, excludes, missing timestamp, and 7-day stability check", () => {
    const now: u64 = 1_800_000_000;
    const three_days: u64 = 3 * 86400;

    let opts = WarmupOptions{ minimum_release_age_secs: three_days };
    * = opts.addExclude("typescript") catch unreachable;

    // 1. Disabled gate always allows
    expect(evaluateReleaseAgeGate("foo", now - 10, now, false, true, WarmupOptions{}) == .Allowed);

    // 2. Excluded package bypasses age gate
    expect(evaluateReleaseAgeGate("typescript", now - 60, now, false, true, opts) == .AllowedExcluded);

    // 3. Missing timestamp treated as passing
    expect(evaluateReleaseAgeGate("foo", null, now, false, false, opts) == .AllowedNoTimestamp);

    // 4. Too new or future timestamp blocked
    expect(evaluateReleaseAgeGate("foo", now - three_days + 1, now, false, false, opts) == .BlockedTooNew);
    expect(evaluateReleaseAgeGate("foo", now + 100, now, true, false, opts) == .BlockedTooNew);

    // 5. Past age gate, within 7-day stability window with rapid follow-up:
    //    range request -> blocked; exact request -> allowed
    const four_days_ago = now - (4 * 86400);
    expect(evaluateReleaseAgeGate("foo", four_days_ago, now, false, true, opts) == .BlockedUnstableRapidRelease);
    expect(evaluateReleaseAgeGate("foo", four_days_ago, now, true, true, opts) == .Allowed);

    // 6. Beyond 7-day stability window (age - min_age > 7 days) -> allowed even with rapid follow-up
    const old_ts = now - three_days - MANIFEST_WARMUP_STABILITY_WINDOW_SECS - 1;
    expect(evaluateReleaseAgeGate("foo", old_ts, now, false, true, opts) == .Allowed);
});

test("Boundary guards: empty names, invalid root IDs, capacity limits, and hash determinism", () => {
    expect(manifestTaskHash("lodash") == manifestTaskHash("lodash"));
    expect(manifestTaskHash("lodash") != manifestTaskHash("lodash-es"));

    let p = ManifestWarmupPlanner.init();

    // Empty package name rejected
    let saw_empty_pkg = false;
    * = p.addPackage(WarmupPkgNode{ name: "" }) catch {
        saw_empty_pkg = true;
    };
    expect(saw_empty_pkg);

    // Invalid root ID rejected
    const bad_ids: [1]u32 = [42];
    let saw_bad_root = false;
    * = p.planForRootIds(bad_ids[0..], WarmupOptions{}) catch {
        saw_bad_root = true;
    };
    expect(saw_bad_root);

    // Excludes capacity & empty name guard
    let opts = WarmupOptions{};
    let saw_empty_excl = false;
    * = opts.addExclude("") catch {
        saw_empty_excl = true;
    };
    expect(saw_empty_excl);

    for (_ of 0..MANIFEST_WARMUP_MAX_EXCLUDES) {
        * = opts.addExclude("pkg") catch unreachable;
    }
    let saw_too_many_excl = false;
    * = opts.addExclude("overflow") catch {
        saw_too_many_excl = true;
    };
    expect(saw_too_many_excl);

    // Package and dependency capacity limits
    for (_ of 0..MANIFEST_WARMUP_MAX_PKGS) {
        * = p.addPackage(WarmupPkgNode{ name: "p" }) catch unreachable;
    }
    let saw_too_many_pkgs = false;
    * = p.addPackage(WarmupPkgNode{ name: "overflow" }) catch {
        saw_too_many_pkgs = true;
    };
    expect(saw_too_many_pkgs);

    for (_ of 0..MANIFEST_WARMUP_MAX_DEPS) {
        * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: 0 }) catch unreachable;
    }
    let saw_too_many_deps = false;
    * = p.addDependency(WarmupDepEdge{ resolved_pkg_id: 0 }) catch {
        saw_too_many_deps = true;
    };
    expect(saw_too_many_deps);
});
