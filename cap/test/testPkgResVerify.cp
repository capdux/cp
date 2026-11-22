import { test, expect } from "cap:test";
import {
    RES_VERIFY_MAX_SCOPES,
    RES_FEAT_NORMAL,
    RES_FEAT_DEV,
    RES_FEAT_LOCAL_DEFAULT,
    RES_FEAT_REMOTE_DEFAULT,
    ResParentKind,
    ResPkgEntry,
    ResDepEntry,
    CachedSemverCandidate,
    pkgNameHash64,
    extractPackageScope,
    formatUnresolvedDepError,
    selectBestCachedSemver,
    ScopedRegistryRouter,
    ResolutionVerifier,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ScopedRegistryRouter routes scoped packages and falls back to default registry", () => {
    expect(strEq(extractPackageScope("@myorg/pkg"), "myorg"));
    expect(strEq(extractPackageScope("@solo"), "solo"));
    expect(strEq(extractPackageScope("lodash"), ""));
    expect(strEq(extractPackageScope("@"), ""));
    expect(strEq(extractPackageScope(""), ""));

    let router = ScopedRegistryRouter.init("https://registry.npmjs.org/");
    * = router.addScope("@myorg", "https://npm.myorg.internal/", true) catch unreachable;
    * = router.addScope("corp", "https://pkg.corp.dev/", false) catch unreachable;

    expect(strEq(router.resolveRegistryUrl("@myorg/ui"), "https://npm.myorg.internal/"));
    expect(strEq(router.resolveRegistryUrl("@corp/core"), "https://pkg.corp.dev/"));
    expect(strEq(router.resolveRegistryUrl("@other/lib"), "https://registry.npmjs.org/"));
    expect(strEq(router.resolveRegistryUrl("react"), "https://registry.npmjs.org/"));

    // Updating an existing scope overwrites cleanly without duplicating
    * = router.addScope("@myorg", "https://v2.myorg.internal/", true) catch unreachable;
    expect(strEq(router.resolveRegistryUrl("@myorg/ui"), "https://v2.myorg.internal/"));
    expect(router.scope_count == 2);
});

test("assignResolution and assignRootResolution backfill names and guard root overwrites", () => {
    let v = ResolutionVerifier.init(RES_FEAT_LOCAL_DEFAULT, RES_FEAT_REMOTE_DEFAULT);

    const p_sdk = v.addPackage(ResPkgEntry{ name: "@myorg/sdk", kind: .Npm }) catch unreachable;
    const p_alt = v.addPackage(ResPkgEntry{ name: "alt-pkg", kind: .Npm }) catch unreachable;

    // 1. Empty name -> backfilled from target package
    const d_empty = v.addDependency(ResDepEntry{ name: "", version_literal: "github:myorg/sdk" }) catch unreachable;
    * = v.assignRootResolution(d_empty, p_sdk) catch unreachable;
    if (dep := v.getDependency(d_empty)) {
        expect(strEq(dep.name, "@myorg/sdk"));
        expect(dep.name_hash == pkgNameHash64("@myorg/sdk"));
    } else {
        expect(false);
    }

    // 2. Name equal to version_literal -> backfilled from target package
    const d_lit = v.addDependency(ResDepEntry{ name: "./local-pkg", version_literal: "./local-pkg" }) catch unreachable;
    * = v.assignResolution(d_lit, p_alt) catch unreachable;
    if (dep2 := v.getDependency(d_lit)) {
        expect(strEq(dep2.name, "alt-pkg"));
    } else {
        expect(false);
    }

    // 3. Explicit alias distinct from literal -> preserved
    const d_alias = v.addDependency(ResDepEntry{ name: "my-alias", version_literal: "^1.0.0" }) catch unreachable;
    * = v.assignResolution(d_alias, p_sdk) catch unreachable;
    if (dep3 := v.getDependency(d_alias)) {
        expect(strEq(dep3.name, "my-alias"));
    } else {
        expect(false);
    }

    // 4. assignRootResolution rejects already-assigned slot
    let saw_already = false;
    * = v.assignRootResolution(d_empty, p_alt) catch {
        saw_already = true;
    };
    expect(saw_already);
});

test("verifyResolutions respects local vs remote feature masks, peer, and optional exemptions", () => {
    let v = ResolutionVerifier.init(RES_FEAT_LOCAL_DEFAULT, RES_FEAT_REMOTE_DEFAULT);

    // Root package (local: dev enabled) owns deps 0..4
    * = v.addPackage(ResPkgEntry{ name: "app", kind: .Root, dep_begin: 0, dep_len: 4 }) catch unreachable;
    // Remote npm package (remote: dev disabled) owns deps 4..6
    const p_remote = v.addPackage(ResPkgEntry{ name: "remote-lib", kind: .Npm, dep_begin: 4, dep_len: 2 }) catch unreachable;

    // dep 0: resolved normal dep of root
    const d0 = v.addDependency(ResDepEntry{ name: "remote-lib", version_literal: "^1.0.0", behavior_mask: RES_FEAT_NORMAL }) catch unreachable;
    // dep 1: unresolved peer dep of root -> skipped
    * = v.addDependency(ResDepEntry{ name: "react", version_literal: "^18.0.0", is_peer: true }) catch unreachable;
    // dep 2: unresolved optional dep of root -> skipped
    * = v.addDependency(ResDepEntry{ name: "fsevents", version_literal: "^2.3.0", is_optional: true }) catch unreachable;
    // dep 3: unresolved dev dep of root -> FAILURE (root is local)
    * = v.addDependency(ResDepEntry{ name: "vitest", version_literal: "^2.0.0", behavior_mask: RES_FEAT_DEV }) catch unreachable;

    // dep 4: unresolved dev dep of remote-lib -> skipped (remote disables dev)
    * = v.addDependency(ResDepEntry{ name: "eslint", version_literal: "^9.0.0", behavior_mask: RES_FEAT_DEV }) catch unreachable;
    // dep 5: unresolved normal dep of remote-lib -> FAILURE
    * = v.addDependency(ResDepEntry{ name: "missing-core", version_literal: "1.2.3", behavior_mask: RES_FEAT_NORMAL }) catch unreachable;

    * = v.assignRootResolution(d0, p_remote) catch unreachable;

    const rep = v.verifyResolutions();
    expect(!rep.isOk());
    expect(rep.total_checked == 6);
    expect(rep.resolved_count == 1);
    expect(rep.skipped_peer_count == 1);
    expect(rep.skipped_optional_or_disabled == 2);
    expect(rep.failure_count == 2);

    const fails = rep.failureSlice();
    expect(strEq(fails[0].name, "vitest"));
    expect(strEq(fails[1].name, "missing-core"));
});

test("formatUnresolvedDepError formats empty, literal-equal, and named dependencies", () => {
    let buf: [128]u8 = [0; 128];

    const n1 = formatUnresolvedDepError("zod", "^3.23.0", buf[0..]) catch unreachable;
    expect(strEq(buf[0..n1], "zod@^3.23.0 failed to resolve"));

    const n2 = formatUnresolvedDepError("", "github:oven-sh/bun", buf[0..]) catch unreachable;
    expect(strEq(buf[0..n2], "github:oven-sh/bun failed to resolve"));

    const n3 = formatUnresolvedDepError("./local", "./local", buf[0..]) catch unreachable;
    expect(strEq(buf[0..n3], "./local failed to resolve"));
});

test("selectBestCachedSemver picks highest compatible version and handles prereleases", () => {
    const cands: [5]CachedSemverCandidate = [
        CachedSemverCandidate{ major: 1, minor: 2, patch: 0, is_prerelease: false, index: 0 },
        CachedSemverCandidate{ major: 1, minor: 5, patch: 3, is_prerelease: false, index: 1 },
        CachedSemverCandidate{ major: 1, minor: 6, patch: 0, is_prerelease: true, index: 2 },
        CachedSemverCandidate{ major: 2, minor: 0, patch: 0, is_prerelease: false, index: 3 },
        CachedSemverCandidate{ major: 1, minor: 5, patch: 3, is_prerelease: true, index: 4 },
    ];
    const min_v = CachedSemverCandidate{ major: 1, minor: 2, patch: 0 };

    // ^1.2.0 without prerelease -> 1.5.3 (index 1)
    if (best := selectBestCachedSemver(cands[0..], min_v, 2, false)) {
        expect(best == 1);
    } else {
        expect(false);
    }

    // ^1.2.0 with prerelease allowed -> 1.6.0-pre (index 2)
    if (best_pre := selectBestCachedSemver(cands[0..], min_v, 2, true)) {
        expect(best_pre == 2);
    } else {
        expect(false);
    }

    // >=3.0.0 -> null
    const too_high = CachedSemverCandidate{ major: 3, minor: 0, patch: 0 };
    expect(selectBestCachedSemver(cands[0..], too_high, 0, true) == null);
});

test("Boundary guards: invalid IDs, empty scope/URL, scope capacity, and tiny buffer", () => {
    let v = ResolutionVerifier.init(RES_FEAT_LOCAL_DEFAULT, RES_FEAT_REMOTE_DEFAULT);

    let saw_bad_dep = false;
    * = v.assignResolution(99, 0) catch {
        saw_bad_dep = true;
    };
    expect(saw_bad_dep);

    const d0 = v.addDependency(ResDepEntry{ name: "a", version_literal: "1.0.0" }) catch unreachable;
    let saw_bad_pkg = false;
    * = v.assignResolution(d0, 99) catch {
        saw_bad_pkg = true;
    };
    expect(saw_bad_pkg);

    let router = ScopedRegistryRouter.init("");
    let saw_empty_scope = false;
    * = router.addScope("@", "https://r.example/", false) catch {
        saw_empty_scope = true;
    };
    expect(saw_empty_scope);

    let saw_empty_url = false;
    * = router.addScope("@ok", "", false) catch {
        saw_empty_url = true;
    };
    expect(saw_empty_url);

    const scope_names: [16][]const u8 = [
        "@s00", "@s01", "@s02", "@s03", "@s04", "@s05", "@s06", "@s07",
        "@s08", "@s09", "@s10", "@s11", "@s12", "@s13", "@s14", "@s15",
    ];
    for (i of 0..RES_VERIFY_MAX_SCOPES) {
        * = router.addScope(scope_names[i], "https://r.example/", false) catch unreachable;
    }
    let saw_too_many_scopes = false;
    * = router.addScope("@s16", "https://r.example/", false) catch {
        saw_too_many_scopes = true;
    };
    expect(saw_too_many_scopes);

    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = formatUnresolvedDepError("zod", "^3.0.0", tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);
});
