// cap/test/test_pkg_config_version_production_deep.cp
// Deep production tests for cap:pkg ConfigVersion, Linker Policy & Isolated Store Formatter (config_version.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    PKG_CONFIG_VERSION_CURRENT,
    PKG_ISOLATED_STORE_DIR,
    LockfileMigrationSource,
    PkgLinkerStrategy,
    PkgConfigVersion,
    formatIsolatedStoreEntry,
    formatIsolatedStorePath,
} from "cap:pkg";

test("pkg.config_version: fromInt and forward-compatibility clamping", () => {
    expect_eq(PKG_CONFIG_VERSION_CURRENT, 1);
    expect_eq(PKG_ISOLATED_STORE_DIR, ".bun");
    expect_eq(PkgConfigVersion.current().toInt(), 1);

    const v0 = PkgConfigVersion.fromInt(0);
    expect_true(v0 != null);
    if (x := v0) { expect_eq(x.toInt(), 0); }

    const v1 = PkgConfigVersion.fromInt(1);
    expect_true(v1 != null);
    if (x := v1) { expect_eq(x.toInt(), 1); }

    // Future version > current clamps to current (Bun ConfigVersion.fromInt contract)
    const v99 = PkgConfigVersion.fromInt(99);
    expect_true(v99 != null);
    if (x := v99) { expect_eq(x.toInt(), 1); }
});

test("pkg.config_version: fromFloat validates integers and rejects negatives/fractions", () => {
    const f0 = PkgConfigVersion.fromFloat(0.0);
    expect_true(f0 != null);
    if (x := f0) { expect_eq(x.toInt(), 0); }

    const f1 = PkgConfigVersion.fromFloat(1.0);
    expect_true(f1 != null);
    if (x := f1) { expect_eq(x.toInt(), 1); }

    const f5 = PkgConfigVersion.fromFloat(5.0);
    expect_true(f5 != null);
    if (x := f5) { expect_eq(x.toInt(), 1); }

    // Fractional & negative values must be rejected
    expect_true(PkgConfigVersion.fromFloat(1.5) == null);
    expect_true(PkgConfigVersion.fromFloat(0.1) == null);
    expect_true(PkgConfigVersion.fromFloat(-1.0) == null);
});

test("pkg.config_version: defaultLinker 4-cell matrix & migration sources", () => {
    // v1 + workspaces => isolated
    expect_true(PkgConfigVersion.v1.defaultLinker(true).isIsolated());
    expect_eq(PkgConfigVersion.v1.defaultLinker(true).asStr(), "isolated");

    // v1 + non-workspaces => hoisted
    expect_true(PkgConfigVersion.v1.defaultLinker(false).isHoisted());
    expect_eq(PkgConfigVersion.v1.defaultLinker(false).asStr(), "hoisted");

    // v0 + workspaces => hoisted
    expect_true(PkgConfigVersion.v0.defaultLinker(true).isHoisted());

    // v0 + non-workspaces => hoisted
    expect_true(PkgConfigVersion.v0.defaultLinker(false).isHoisted());

    // Migration source defaults (Bun isolated-installs.mdx)
    expect_eq(LockfileMigrationSource.none.defaultConfigVersion().toInt(), 1);
    expect_eq(LockfileMigrationSource.pnpm.defaultConfigVersion().toInt(), 1);
    expect_eq(LockfileMigrationSource.legacy_bun.defaultConfigVersion().toInt(), 0);
    expect_eq(LockfileMigrationSource.npm.defaultConfigVersion().toInt(), 0);
    expect_eq(LockfileMigrationSource.yarn.defaultConfigVersion().toInt(), 0);
});

test("pkg.config_version: PkgLinkerStrategy.fromStr parsing and error guard", () => {
    const h = PkgLinkerStrategy.fromStr("hoisted") catch { expect_true(false); return; };
    expect_true(h.isHoisted());
    expect_false(h.isIsolated());

    const iso = PkgLinkerStrategy.fromStr("isolated") catch { expect_true(false); return; };
    expect_true(iso.isIsolated());
    expect_false(iso.isHoisted());

    let bad = false;
    * = PkgLinkerStrategy.fromStr("pnp") catch { bad = true; };
    expect_true(bad);
});

test("pkg.config_version: Isolated Store entry & full path formatting (unscoped & scoped)", () => {
    let buf: [128]u8 = [0; 128];

    const e1 = formatIsolatedStoreEntry(buf[0..], "lodash", "4.17.21") catch {
        expect_true(false);
        return;
    };
    expect_eq(e1, "lodash@4.17.21");

    const e2 = formatIsolatedStoreEntry(buf[0..], "@scope/package", "2.1.0-rc.1") catch {
        expect_true(false);
        return;
    };
    expect_eq(e2, "@scope+package@2.1.0-rc.1");

    let pbuf: [256]u8 = [0; 256];
    const p1 = formatIsolatedStorePath(pbuf[0..], "react", "18.3.1") catch {
        expect_true(false);
        return;
    };
    expect_eq(p1, "node_modules/.bun/react@18.3.1/node_modules/react");

    const p2 = formatIsolatedStorePath(pbuf[0..], "@acme/core", "3.0.0") catch {
        expect_true(false);
        return;
    };
    expect_eq(p2, "node_modules/.bun/@acme+core@3.0.0/node_modules/@acme/core");
});

test("pkg.config_version: security & boundary error guards on store path formatter", () => {
    let buf: [128]u8 = [0; 128];

    // Empty name / empty version
    let f1 = false;
    * = formatIsolatedStoreEntry(buf[0..], "", "1.0.0") catch { f1 = true; };
    expect_true(f1);

    let f2 = false;
    * = formatIsolatedStoreEntry(buf[0..], "pkg", "") catch { f2 = true; };
    expect_true(f2);

    // Path traversal & invalid slash forms
    let f3 = false;
    * = formatIsolatedStorePath(buf[0..], "../evil", "1.0.0") catch { f3 = true; };
    expect_true(f3);

    let f4 = false;
    * = formatIsolatedStorePath(buf[0..], "@scope/a/b", "1.0.0") catch { f4 = true; };
    expect_true(f4);

    let f5 = false;
    * = formatIsolatedStorePath(buf[0..], "pkg", "1.0.0/../2") catch { f5 = true; };
    expect_true(f5);

    let f6 = false;
    * = formatIsolatedStoreEntry(buf[0..], "pkg/sub", "1.0.0") catch { f6 = true; };
    expect_true(f6);

    // BufferTooSmall
    let tiny: [8]u8 = [0; 8];
    let f7 = false;
    * = formatIsolatedStoreEntry(tiny[0..], "lodash", "4.17.21") catch { f7 = true; };
    expect_true(f7);

    let f8 = false;
    * = formatIsolatedStorePath(tiny[0..], "a", "1") catch { f8 = true; };
    expect_true(f8);
});
