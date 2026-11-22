// cap/test/testBundleEnums.cp
// Production-grade deep tests for cap:bundler bundleEnums.cp
// (BundleFormat, WindowsExeOptions PE metadata & BundlePackageMap zero-alloc policy)

import { test, assert, assertEq } from "cap:test";
import {
    BundleFormat,
    WindowsExeOptions,
    BundlePackageMode,
    BUNDLE_PACKAGE_MAX_RULES,
    BundlePackageError,
    BundlePackageMap,
} from "cap:bundler";

test("BundleFormat: fromString/asStr roundtrip & invalid input rejection", () => {
    if (f := BundleFormat.fromString("esm")) assertEq(f, BundleFormat.Esm) else assert(false);
    if (f := BundleFormat.fromString("cjs")) assertEq(f, BundleFormat.Cjs) else assert(false);
    if (f := BundleFormat.fromString("iife")) assertEq(f, BundleFormat.Iife) else assert(false);
    if (f := BundleFormat.fromString("internal_bake_dev")) assertEq(f, BundleFormat.InternalBakeDev) else assert(false);

    assert(BundleFormat.fromString("") == null);
    assert(BundleFormat.fromString("ESM") == null);
    assert(BundleFormat.fromString("umd") == null);
    assert(BundleFormat.fromString("amd") == null);

    assertEq(BundleFormat.Esm.asStr(), "esm");
    assertEq(BundleFormat.Cjs.asStr(), "cjs");
    assertEq(BundleFormat.Iife.asStr(), "iife");
    assertEq(BundleFormat.InternalBakeDev.asStr(), "internal_bake_dev");
});

test("BundleFormat: ES6 syntax, strict mode, code splitting & top-level await predicates", () => {
    assert(BundleFormat.Esm.isESM());
    assert(BundleFormat.Esm.keepES6ImportExportSyntax());
    assert(BundleFormat.Esm.isAlwaysStrictMode());
    assert(BundleFormat.Esm.supportsCodeSplitting());
    assert(BundleFormat.Esm.supportsTopLevelAwait());

    const non_esm: [3]BundleFormat = [
        BundleFormat.Cjs,
        BundleFormat.Iife,
        BundleFormat.InternalBakeDev,
    ];
    let i: usize = 0;
    while (i < non_esm.len) {
        const f = non_esm[i];
        assert(!f.isESM());
        assert(!f.keepES6ImportExportSyntax());
        assert(!f.isAlwaysStrictMode());
        assert(!f.supportsCodeSplitting());
        assert(!f.supportsTopLevelAwait());
        i += 1;
    }
});

test("WindowsExeOptions: defaults, subsystem switching & metadata detection", () => {
    const def = WindowsExeOptions{};
    assert(!def.hide_console);
    assert(!def.hasMetadata());
    assertEq(def.subsystemName(), "WINDOWS_CUI");
    assert(def.isValidVersion());

    const gui = WindowsExeOptions{
        hide_console: true,
        title: "CapApp",
        publisher: "CapLang",
        version: "2.4.0.1024",
    };
    assert(gui.hasMetadata());
    assertEq(gui.subsystemName(), "WINDOWS_GUI");
    assert(gui.isValidVersion());
});

test("WindowsExeOptions.isValidVersion: valid quads and malformed version rejection", () => {
    assert((WindowsExeOptions{ version: "0.0.0.0" }).isValidVersion());
    assert((WindowsExeOptions{ version: "1.2.3.4" }).isValidVersion());
    assert((WindowsExeOptions{ version: "65535.65535.65535.65535" }).isValidVersion());

    assert(!(WindowsExeOptions{ version: "" }).isValidVersion());
    assert(!(WindowsExeOptions{ version: "1.2.3" }).isValidVersion());
    assert(!(WindowsExeOptions{ version: "1.2.3.4.5" }).isValidVersion());
    assert(!(WindowsExeOptions{ version: "1.2.3." }).isValidVersion());
    assert(!(WindowsExeOptions{ version: ".1.2.3" }).isValidVersion());
    assert(!(WindowsExeOptions{ version: "1..2.3" }).isValidVersion());
    assert(!(WindowsExeOptions{ version: "65536.0.0.0" }).isValidVersion());
    assert(!(WindowsExeOptions{ version: "1.2.3.4a" }).isValidVersion());
});

test("BundlePackageMode & BundlePackageMap: set, update, lookup, remove & fallback", () => {
    if (m := BundlePackageMode.fromString("always")) assertEq(m, BundlePackageMode.Always) else assert(false);
    if (m := BundlePackageMode.fromString("never")) assertEq(m, BundlePackageMode.Never) else assert(false);
    assert(BundlePackageMode.fromString("auto") == null);
    assertEq(BundlePackageMode.Always.asStr(), "always");
    assertEq(BundlePackageMode.Never.asStr(), "never");
    assert(BundlePackageMode.Always.shouldBundle());
    assert(!BundlePackageMode.Never.shouldBundle());

    let map = BundlePackageMap.init();
    assertEq(map.count(), 0);
    assert(map.shouldBundle("lodash", BundlePackageMode.Always));
    assert(!map.shouldBundle("lodash", BundlePackageMode.Never));

    * = map.set("fsevents", BundlePackageMode.Never) catch {
        assert(false);
        return;
    };
    * = map.set("react", BundlePackageMode.Always) catch {
        assert(false);
        return;
    };
    assertEq(map.count(), 2);
    assert(!map.shouldBundle("fsevents", BundlePackageMode.Always));
    assert(map.shouldBundle("react", BundlePackageMode.Never));

    // In-place update does not increase count
    * = map.set("fsevents", BundlePackageMode.Always) catch {
        assert(false);
        return;
    };
    assertEq(map.count(), 2);
    assert(map.shouldBundle("fsevents", BundlePackageMode.Never));

    // Removal and non-existent removal
    assert(map.remove("fsevents"));
    assert(!map.remove("fsevents"));
    assertEq(map.count(), 1);
    assert(map.get("fsevents") == null);
    if (rm := map.get("react")) assertEq(rm, BundlePackageMode.Always) else assert(false);
});

test("BundlePackageMap: empty name rejection & capacity overflow protection", () => {
    let map = BundlePackageMap.init();
    let saw_empty = false;
    * = map.set("", BundlePackageMode.Never) catch e => {
        if (e == BundlePackageError.EmptyPackageName) saw_empty = true;
    };
    assert(saw_empty);

    // Fill to exact capacity (64) with unique slices from a backing buffer
    let backing: [128]u8 = [0; 128];
    let i: usize = 0;
    while (i < BUNDLE_PACKAGE_MAX_RULES) {
        backing[i * 2] = 'p';
        backing[i * 2 + 1] = @as(u8, @intCast(33 + i));
        const key = backing[i * 2 .. i * 2 + 2];
        * = map.set(key, BundlePackageMode.Never) catch {
            assert(false);
            return;
        };
        i += 1;
    }
    assertEq(map.count(), BUNDLE_PACKAGE_MAX_RULES);

    let saw_cap = false;
    * = map.set("overflow-pkg", BundlePackageMode.Always) catch e => {
        if (e == BundlePackageError.CapacityExceeded) saw_cap = true;
    };
    assert(saw_cap);
});
