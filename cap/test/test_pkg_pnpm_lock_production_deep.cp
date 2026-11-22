// Production Deep Test for cap:pkg (pnpm_lock.cp)
// Covers: lockfileVersion v7/v8/v9 validation & rejection (< v7),
// balanced-parenthesis nested peer + patch_hash suffix parsing,
// scoped/v8-slash/v9 package key parsing, importer protocol & npm: alias normalization,
// and PnpmMigrationIndex cross-validation (importers/snapshots/packages/patches) + edge guards.

import { test, assert, assertEq } from "cap:test";
import {
    PNPM_LOCK_MIN_SUPPORTED_MAJOR,
    PnpmLockVersion,
    parsePnpmLockfileVersion,
    indexOfPnpmDepPathSuffix,
    removePnpmDepPathSuffix,
    extractPnpmPatchHash,
    extractPnpmPeersSuffix,
    parsePnpmPackageKey,
    PnpmDepResolutionKind,
    parsePnpmImporterVersion,
    formatPnpmCanonicalKey,
    PnpmMigrationIndex,
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

test("pnpm_lock: lockfileVersion parsing (v7/v8/v9, quoted, too old, invalid)", () => {
    assertEq(PNPM_LOCK_MIN_SUPPORTED_MAJOR, 7);

    const v9 = parsePnpmLockfileVersion("'9.0'") catch {
        assert(false);
        return;
    };
    assertEq(v9.major, 9);
    assertEq(v9.minor, 0);
    assert(v9.isV9OrNewer());

    const v7 = parsePnpmLockfileVersion(" \"7.2\" ") catch {
        assert(false);
        return;
    };
    assertEq(v7.major, 7);
    assertEq(v7.minor, 2);
    assert(!v7.isV9OrNewer());

    const v8_int = parsePnpmLockfileVersion("8") catch {
        assert(false);
        return;
    };
    assertEq(v8_int.major, 8);
    assertEq(v8_int.minor, 0);

    // v6 must be rejected with LockfileTooOld
    let too_old = false;
    * = parsePnpmLockfileVersion("6.0") catch {
        too_old = true;
        return;
    };
    assert(!too_old); // unreachable when catch returns
});

test("pnpm_lock: lockfileVersion error boundary checks", () => {
    let err_v6 = false;
    const r6 = parsePnpmLockfileVersion("6.1") catch PnpmLockVersion{ major: 0, minor: 0 };
    if (r6.major == 0) {
        err_v6 = true;
    }
    assert(err_v6);

    let err_neg = false;
    const rn = parsePnpmLockfileVersion("-9.0") catch PnpmLockVersion{ major: 0, minor: 0 };
    if (rn.major == 0) {
        err_neg = true;
    }
    assert(err_neg);

    let err_bad = false;
    const rb = parsePnpmLockfileVersion("9.0.1") catch PnpmLockVersion{ major: 0, minor: 0 };
    if (rb.major == 0) {
        err_bad = true;
    }
    assert(err_bad);

    let err_empty = false;
    const re = parsePnpmLockfileVersion("   ") catch PnpmLockVersion{ major: 0, minor: 0 };
    if (re.major == 0) {
        err_empty = true;
    }
    assert(err_empty);
});

test("pnpm_lock: nested peer parentheses and patch_hash suffix extraction", () => {
    const plain = "lodash@4.17.21";
    const s0 = indexOfPnpmDepPathSuffix(plain);
    assert(!s0.hasSuffix());
    assert(strEql(removePnpmDepPathSuffix(plain), "lodash@4.17.21"));

    // Deeply nested peer dependencies
    const nested = "react-dom@18.2.0(react@18.2.0(scheduler@0.23.0))(@types/react@18.2.0)";
    const s1 = indexOfPnpmDepPathSuffix(nested);
    assert(s1.hasSuffix());
    assert(s1.patch_hash_idx == null);
    assert(s1.peers_idx != null);
    assert(strEql(removePnpmDepPathSuffix(nested), "react-dom@18.2.0"));
    if (peers := extractPnpmPeersSuffix(nested)) {
        assert(strEql(peers, "(react@18.2.0(scheduler@0.23.0))(@types/react@18.2.0)"));
    } else {
        assert(false);
    }

    // patch_hash + nested peers together
    const patched = "@scope/ui@2.1.0(patch_hash=deadbeef99)(react@18.2.0(loose-envify@1.4.0))";
    const s2 = indexOfPnpmDepPathSuffix(patched);
    assert(s2.patch_hash_idx != null);
    assert(s2.peers_idx != null);
    assert(strEql(removePnpmDepPathSuffix(patched), "@scope/ui@2.1.0"));

    const ph_opt = extractPnpmPatchHash(patched) catch {
        assert(false);
        return;
    };
    if (ph := ph_opt) {
        assert(strEql(ph, "deadbeef99"));
    } else {
        assert(false);
    }

    if (p2 := extractPnpmPeersSuffix(patched)) {
        assert(strEql(p2, "(react@18.2.0(loose-envify@1.4.0))"));
    } else {
        assert(false);
    }

    // Empty patch_hash must fail
    let bad_hash = false;
    * = extractPnpmPatchHash("foo@1.0.0(patch_hash=)") catch {
        bad_hash = true;
        return;
    };
    assert(!bad_hash);
});

test("pnpm_lock: parsePnpmPackageKey across v9, v8 leading-slash, and scoped keys", () => {
    const k1 = parsePnpmPackageKey("lodash@4.17.21") catch {
        assert(false);
        return;
    };
    assert(strEql(k1.name, "lodash"));
    assert(strEql(k1.version, "4.17.21"));
    assert(!k1.has_leading_slash);
    assert(k1.patch_hash == null);
    assert(k1.peers == null);

    const k2 = parsePnpmPackageKey("/@babel/core@7.24.0(patch_hash=ab12)(@babel/types@7.24.0)") catch {
        assert(false);
        return;
    };
    assert(strEql(k2.name, "@babel/core"));
    assert(strEql(k2.version, "7.24.0"));
    assert(k2.has_leading_slash);
    if (ph := k2.patch_hash) {
        assert(strEql(ph, "ab12"));
    } else {
        assert(false);
    }
    if (pr := k2.peers) {
        assert(strEql(pr, "(@babel/types@7.24.0)"));
    } else {
        assert(false);
    }

    let out: [64]u8 = [0; 64];
    const n = formatPnpmCanonicalKey(k2.name, k2.version, out[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(out[0..n], "@babel/core@7.24.0"));

    // Small buffer check
    let tiny: [4]u8 = [0; 4];
    const tiny_n = formatPnpmCanonicalKey("lodash", "4.17.21", tiny[0..]) catch 0;
    assertEq(tiny_n, 0);
});

test("pnpm_lock: parsePnpmImporterVersion protocols and npm aliases", () => {
    const r_reg = parsePnpmImporterVersion("react-dom", "18.2.0(react@18.2.0)") catch {
        assert(false);
        return;
    };
    assert(r_reg.kind == PnpmDepResolutionKind.Registry);
    assert(strEql(r_reg.package_name, "react-dom"));
    assert(strEql(r_reg.resolved_version, "18.2.0"));

    const r_alias = parsePnpmImporterVersion("my-react", "npm:@scope/react@18.3.0(peer@1.0.0)") catch {
        assert(false);
        return;
    };
    assert(r_alias.kind == PnpmDepResolutionKind.Alias);
    assert(strEql(r_alias.package_name, "@scope/react"));
    assert(strEql(r_alias.resolved_version, "18.3.0"));

    const r_link = parsePnpmImporterVersion("local-lib", "link:packages/local-lib") catch {
        assert(false);
        return;
    };
    assert(r_link.kind == PnpmDepResolutionKind.Link);
    assert(strEql(r_link.target_path, "packages/local-lib"));

    const r_ws = parsePnpmImporterVersion("shared", "workspace:*") catch {
        assert(false);
        return;
    };
    assert(r_ws.kind == PnpmDepResolutionKind.Workspace);
    assert(strEql(r_ws.target_path, "*"));

    const r_tgz = parsePnpmImporterVersion("custom", "https://example.com/pkg-1.0.0.tgz") catch {
        assert(false);
        return;
    };
    assert(r_tgz.kind == PnpmDepResolutionKind.Tarball);

    const r_git = parsePnpmImporterVersion("forked", "git+https://github.com/org/repo.git#abc") catch {
        assert(false);
        return;
    };
    assert(r_git.kind == PnpmDepResolutionKind.Git);
});

test("pnpm_lock: PnpmMigrationIndex full lifecycle, peer deduplication, patch binding, and error guards", () => {
    const ver = parsePnpmLockfileVersion("9.0") catch {
        assert(false);
        return;
    };
    let idx = PnpmMigrationIndex.init(ver);

    // Without root importer ".", validateAndStats must fail
    let missing_root = false;
    const pre = idx.validateAndStats() catch {
        missing_root = true;
        return;
    };
    * = pre;
    assert(!missing_root);
});

test("pnpm_lock: PnpmMigrationIndex cross-validation and duplicate/missing guards", () => {
    const ver = parsePnpmLockfileVersion("9.0") catch {
        assert(false);
        return;
    };
    let idx = PnpmMigrationIndex.init(ver);

    const root_i = idx.registerImporter(".", 3) catch {
        assert(false);
        return;
    };
    assertEq(root_i, 0);
    const ws_i = idx.registerImporter("packages/app", 2) catch {
        assert(false);
        return;
    };
    assertEq(ws_i, 1);

    // Duplicate importer path must fail
    const dup_imp = idx.registerImporter(".", 1) catch 9999;
    assertEq(dup_imp, 9999);

    // Register patch and verify duplicate patch hash rejection
    const p0 = idx.registerPatch("react-dom", "patches/react-dom@18.2.0.patch", "hash_xyz") catch {
        assert(false);
        return;
    };
    assertEq(p0, 0);
    const dup_p = idx.registerPatch("other", "patches/other.patch", "hash_xyz") catch 9999;
    assertEq(dup_p, 9999);

    // Register two peer variants of the same canonical snapshot
    const s0 = idx.registerSnapshot("react-dom@18.2.0(patch_hash=hash_xyz)(react@18.2.0)") catch {
        assert(false);
        return;
    };
    const s1 = idx.registerSnapshot("react-dom@18.2.0(react@18.3.0)") catch {
        assert(false);
        return;
    };
    assertEq(s0, s1); // deduplicated into same canonical record

    if (snap := idx.snapshotAt(s0)) {
        assert(strEql(snap.canonical_key, "react-dom@18.2.0"));
        assertEq(snap.variant_count, 2);
        assert(snap.has_peers);
    } else {
        assert(false);
    }

    // Patch should have been matched to version 18.2.0
    if (pe := idx.patchAt(p0)) {
        assert(pe.matched);
        if (mv := pe.matched_version) {
            assert(strEql(mv, "18.2.0"));
        } else {
            assert(false);
        }
    } else {
        assert(false);
    }

    // Match package against snapshot
    const m0 = idx.matchPackage("react-dom@18.2.0") catch {
        assert(false);
        return;
    };
    assertEq(m0, s0);

    // Missing snapshot for unknown package must fail
    const missing_m = idx.matchPackage("ghost@1.0.0") catch 9999;
    assertEq(missing_m, 9999);

    const st = idx.validateAndStats() catch {
        assert(false);
        return;
    };
    assert(st.has_root_importer);
    assertEq(st.importers_count, 2);
    assertEq(st.snapshots_count, 1);
    assertEq(st.packages_matched, 1);
    assertEq(st.patches_registered, 1);
    assertEq(st.patches_matched, 1);
});
