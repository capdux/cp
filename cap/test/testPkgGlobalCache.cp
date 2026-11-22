// cap/test/testPkgGlobalCache.cp
// Production-grade deep tests for cap:pkg globalCache.cp
// (Bun GlobalCache auto-install & resolution fallback policy engine)

import { test, assert, assertEq } from "cap:test";
import {
    GlobalCacheMode,
    GlobalCacheAction,
    GlobalCacheDecision,
    GlobalCachePolicy,
} from "cap:pkg";

test("GlobalCacheMode: CLI flag parsing, extended parse, asStr & invalid rejection", () => {
    if (m := GlobalCacheMode.parseCliFlag("auto")) assertEq(m, GlobalCacheMode.Auto) else assert(false);
    if (m := GlobalCacheMode.parseCliFlag("force")) assertEq(m, GlobalCacheMode.Force) else assert(false);
    if (m := GlobalCacheMode.parseCliFlag("disable")) assertEq(m, GlobalCacheMode.Disable) else assert(false);
    if (m := GlobalCacheMode.parseCliFlag("fallback")) assertEq(m, GlobalCacheMode.Fallback) else assert(false);

    // Internal modes are not accepted by CLI flag parser
    assert(GlobalCacheMode.parseCliFlag("allow_install") == null);
    assert(GlobalCacheMode.parseCliFlag("read_only") == null);
    assert(GlobalCacheMode.parseCliFlag("AUTO") == null);
    assert(GlobalCacheMode.parseCliFlag("") == null);

    // Full parser accepts both snake_case and kebab-case
    if (m := GlobalCacheMode.parse("allow_install")) assertEq(m, GlobalCacheMode.AllowInstall) else assert(false);
    if (m := GlobalCacheMode.parse("allow-install")) assertEq(m, GlobalCacheMode.AllowInstall) else assert(false);
    if (m := GlobalCacheMode.parse("read_only")) assertEq(m, GlobalCacheMode.ReadOnly) else assert(false);
    if (m := GlobalCacheMode.parse("read-only")) assertEq(m, GlobalCacheMode.ReadOnly) else assert(false);
    assert(GlobalCacheMode.parse("unknown") == null);

    assertEq(GlobalCacheMode.Auto.asStr(), "auto");
    assertEq(GlobalCacheMode.Force.asStr(), "force");
    assertEq(GlobalCacheMode.Fallback.asStr(), "fallback");
    assertEq(GlobalCacheMode.Disable.asStr(), "disable");
    assertEq(GlobalCacheMode.AllowInstall.asStr(), "allow_install");
    assertEq(GlobalCacheMode.ReadOnly.asStr(), "read_only");
});

test("GlobalCacheMode: exact upstream truth table for canUse, isEnabled, canInstall & allowVersionSpecifier", () => {
    // When local node_modules exists
    assert(GlobalCacheMode.Fallback.canUse(true));
    assert(GlobalCacheMode.AllowInstall.canUse(true));
    assert(GlobalCacheMode.Force.canUse(true));
    assert(!GlobalCacheMode.ReadOnly.canUse(true));
    assert(!GlobalCacheMode.Disable.canUse(true));
    assert(!GlobalCacheMode.Auto.canUse(true));

    // When no local node_modules exists
    assert(GlobalCacheMode.ReadOnly.canUse(false));
    assert(GlobalCacheMode.Fallback.canUse(false));
    assert(GlobalCacheMode.AllowInstall.canUse(false));
    assert(GlobalCacheMode.Auto.canUse(false));
    assert(GlobalCacheMode.Force.canUse(false));
    assert(!GlobalCacheMode.Disable.canUse(false));

    // isEnabled
    assert(GlobalCacheMode.Auto.isEnabled());
    assert(GlobalCacheMode.Force.isEnabled());
    assert(GlobalCacheMode.Fallback.isEnabled());
    assert(GlobalCacheMode.AllowInstall.isEnabled());
    assert(GlobalCacheMode.ReadOnly.isEnabled());
    assert(!GlobalCacheMode.Disable.isEnabled());

    // canInstall
    assert(GlobalCacheMode.Auto.canInstall());
    assert(GlobalCacheMode.AllowInstall.canInstall());
    assert(GlobalCacheMode.Force.canInstall());
    assert(GlobalCacheMode.Fallback.canInstall());
    assert(!GlobalCacheMode.ReadOnly.canInstall());
    assert(!GlobalCacheMode.Disable.canInstall());

    // allowVersionSpecifier & isReadOnly
    assert(GlobalCacheMode.Force.allowVersionSpecifier());
    assert(!GlobalCacheMode.Auto.allowVersionSpecifier());
    assert(!GlobalCacheMode.Fallback.allowVersionSpecifier());
    assert(GlobalCacheMode.ReadOnly.isReadOnly());
    assert(!GlobalCacheMode.Auto.isReadOnly());
});

test("GlobalCachePolicy: Auto mode with and without local node_modules", () => {
    // Auto without node_modules: uses global cache hit or installs
    const auto_no_nm = GlobalCachePolicy.init(GlobalCacheMode.Auto, false);
    const d_hit = auto_no_nm.evaluate(false, true, false);
    assertEq(d_hit.action, GlobalCacheAction.UseGlobalCacheHit);
    assert(d_hit.isResolved());
    assert(d_hit.use_global_cache);

    const d_inst = auto_no_nm.evaluate(false, false, false);
    assertEq(d_inst.action, GlobalCacheAction.InstallToGlobalCache);
    assert(d_inst.isResolved());
    assert(d_inst.allow_network_install);

    // Auto with node_modules: local found -> use local; local missing -> reject (no global fallback)
    const auto_with_nm = GlobalCachePolicy.init(GlobalCacheMode.Auto, true);
    const d_local = auto_with_nm.evaluate(true, true, false);
    assertEq(d_local.action, GlobalCacheAction.UseLocalNodeModules);
    assert(d_local.isResolved());

    const d_miss = auto_with_nm.evaluate(false, true, false);
    assertEq(d_miss.action, GlobalCacheAction.RejectDisabled);
    assert(!d_miss.isResolved());
});

test("GlobalCachePolicy: Force mode bypasses local node_modules & permits inline version specifiers", () => {
    const force = GlobalCachePolicy.init(GlobalCacheMode.Force, true);

    // Even when local_found is true, Force resolves via global cache
    const d_hit = force.evaluate(true, true, true);
    assertEq(d_hit.action, GlobalCacheAction.UseGlobalCacheHit);
    assert(d_hit.isResolved());
    assert(d_hit.allow_version_specifier);

    const d_inst = force.evaluate(true, false, true);
    assertEq(d_inst.action, GlobalCacheAction.InstallToGlobalCache);
    assert(d_inst.allow_network_install);

    // Offline Force mode allows cache hits but rejects cache misses
    const force_offline = GlobalCachePolicy.offlinePolicy(GlobalCacheMode.Force, true);
    assertEq(force_offline.evaluate(true, true, true).action, GlobalCacheAction.UseGlobalCacheHit);
    assertEq(force_offline.evaluate(true, false, true).action, GlobalCacheAction.RejectDisabled);
});

test("GlobalCachePolicy: Fallback, ReadOnly & Disable modes", () => {
    // Fallback prefers local, falls back to global hit or install
    const fb = GlobalCachePolicy.init(GlobalCacheMode.Fallback, true);
    assertEq(fb.evaluate(true, true, false).action, GlobalCacheAction.UseLocalNodeModules);
    assertEq(fb.evaluate(false, true, false).action, GlobalCacheAction.UseGlobalCacheHit);
    assertEq(fb.evaluate(false, false, false).action, GlobalCacheAction.InstallToGlobalCache);

    // ReadOnly without node_modules serves cache hits only, never installs
    const ro = GlobalCachePolicy.init(GlobalCacheMode.ReadOnly, false);
    assertEq(ro.evaluate(false, true, false).action, GlobalCacheAction.UseGlobalCacheHit);
    const ro_miss = ro.evaluate(false, false, false);
    assertEq(ro_miss.action, GlobalCacheAction.RejectDisabled);
    assert(!ro_miss.allow_network_install);

    // Disable only allows local node_modules
    const dis = GlobalCachePolicy.init(GlobalCacheMode.Disable, false);
    assertEq(dis.evaluate(true, false, false).action, GlobalCacheAction.UseLocalNodeModules);
    assertEq(dis.evaluate(false, true, false).action, GlobalCacheAction.RejectDisabled);
});

test("GlobalCachePolicy: rejects inline version specifiers outside Force mode", () => {
    const modes: [5]GlobalCacheMode = [
        GlobalCacheMode.Auto,
        GlobalCacheMode.Fallback,
        GlobalCacheMode.AllowInstall,
        GlobalCacheMode.ReadOnly,
        GlobalCacheMode.Disable,
    ];
    let i: usize = 0;
    while (i < modes.len) {
        const pol = GlobalCachePolicy.init(modes[i], false);
        const decision = pol.evaluate(true, true, true);
        assertEq(decision.action, GlobalCacheAction.RejectVersionSpecifier);
        assert(!decision.isResolved());
        assert(!decision.allow_version_specifier);
        i += 1;
    }
});
