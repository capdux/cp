// Production Deep Tests for cap:bundler Barrel File Optimizer (lib/bundler/barrel_imports.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_BARREL_REQUESTED_EXPORTS,
    MAX_BARREL_IMPORT_RECORDS,
    BarrelError,
    RequestedExportsKind,
    RequestedExportsSet,
    BarrelEligibility,
    BarrelModule,
} from "cap:bundler";

test("bundler barrel: eligibility predicate matrix", fn() {
    // Pure sideEffects:false barrel -> eligible
    const ok = BarrelEligibility{
        side_effects_false: true,
        import_record_count: 3,
        reexport_count: 3,
    };
    expect_eq(ok.isEligible(), true);

    // Explicit optimize_imports allowlist -> eligible even without sideEffects:false
    const explicit_ok = BarrelEligibility{
        is_explicit_optimize: true,
        import_record_count: 2,
        export_star_count: 1,
    };
    expect_eq(explicit_ok.isEligible(), true);

    // Local definitions present -> NOT a pure barrel
    const has_local = BarrelEligibility{
        side_effects_false: true,
        has_local_definitions: true,
        import_record_count: 2,
        reexport_count: 2,
    };
    expect_eq(has_local.isEligible(), false);

    // Target of another barrel's `export *` -> NOT eligible (avoids circular races)
    const star_target = BarrelEligibility{
        side_effects_false: true,
        is_export_star_target: true,
        import_record_count: 2,
        reexport_count: 2,
    };
    expect_eq(star_target.isEligible(), false);

    // Neither sideEffects:false nor explicit -> NOT eligible
    const neither = BarrelEligibility{
        import_record_count: 2,
        reexport_count: 2,
    };
    expect_eq(neither.isEligible(), false);

    // Zero imports or zero exports -> NOT eligible
    const no_imports = BarrelEligibility{ side_effects_false: true, reexport_count: 1 };
    const no_exports = BarrelEligibility{ side_effects_false: true, import_record_count: 2 };
    expect_eq(no_imports.isEligible(), false);
    expect_eq(no_exports.isEligible(), false);
});

test("bundler barrel: selective deferring and never deferring export * records", fn() {
    let mod = BarrelModule.init(4) catch unreachable;
    // record 0: Button, record 1: Modal, record 2: Icon, record 3: export * from './utils'
    mod.addReExport("Button", "Button", 0, false) catch unreachable;
    mod.addReExport("Modal", "ModalDialog", 1, false) catch unreachable;
    mod.addReExport("Icon", "Icon", 2, false) catch unreachable;
    mod.markExportStar(3) catch unreachable;

    // Only "Button" was eagerly requested before barrel loaded
    expect_eq(mod.requested.recordPartial("Button") catch unreachable, true);

    const plan = mod.computePlan();
    expect_eq(plan.active_count, @as(u32, 2));
    expect_eq(plan.deferred_count, @as(u32, 2));
    expect_eq(plan.isActive(0), true);   // Button requested
    expect_eq(plan.isDeferred(1), true); // Modal deferred
    expect_eq(plan.isDeferred(2), true); // Icon deferred
    expect_eq(plan.isActive(3), true);   // export * never deferred
});

test("bundler barrel: incremental wakeup, alias propagation, and cycle deduplication", fn() {
    let mod = BarrelModule.init(3) catch unreachable;
    mod.addReExport("Button", "Btn", 0, false) catch unreachable;
    mod.addReExport("Modal", "ModalDialog", 1, false) catch unreachable;
    mod.addReExport("ModalTitle", "ModalHeader", 1, false) catch unreachable;

    * = mod.computePlan();

    // First request for "Modal" wakes record 1 and propagates original alias "ModalDialog"
    const w1 = mod.requestExport("Modal") catch unreachable;
    expect_eq(w1.newly_activated, true);
    expect_eq(w1.import_record_index ?? 99, @as(u32, 1));
    expect_eq(w1.propagate_alias ?? "", "ModalDialog");
    expect_eq(w1.propagate_all, false);

    // Duplicate request for "Modal" is deduplicated (prevents cycles)
    const w_dup = mod.requestExport("Modal") catch unreachable;
    expect_eq(w_dup.newly_activated, false);
    expect_eq(w_dup.import_record_index == null, true);

    // Second export from same already-active record 1: not newly_activated, still propagates alias
    const w2 = mod.requestExport("ModalTitle") catch unreachable;
    expect_eq(w2.newly_activated, false);
    expect_eq(w2.import_record_index ?? 99, @as(u32, 1));
    expect_eq(w2.propagate_alias ?? "", "ModalHeader");
});

test("bundler barrel: namespace star re-export propagates all exports downstream", fn() {
    let mod = BarrelModule.init(2) catch unreachable;
    // `import * as icons from './icons'; export { icons as Icons }`
    mod.addReExport("Icons", null, 1, true) catch unreachable;

    const w = mod.requestExport("Icons") catch unreachable;
    expect_eq(w.newly_activated, true);
    expect_eq(w.import_record_index ?? 99, @as(u32, 1));
    expect_eq(w.propagate_alias == null, true);
    expect_eq(w.propagate_all, true);
});

test("bundler barrel: upgrading to RequestedExports.all activates all deferred modules", fn() {
    let mod = BarrelModule.init(3) catch unreachable;
    mod.addReExport("A", "A", 0, false) catch unreachable;
    mod.addReExport("B", "B", 1, false) catch unreachable;
    mod.addReExport("C", "C", 2, false) catch unreachable;

    const p0 = mod.computePlan();
    expect_eq(p0.deferred_count, @as(u32, 3));

    const p_all = mod.requestAll();
    expect_eq(mod.requested.kind == RequestedExportsKind.all, true);
    expect_eq(p_all.active_count, @as(u32, 3));
    expect_eq(p_all.deferred_count, @as(u32, 0));

    // Subsequent partial requests short-circuit as already covered
    expect_eq(mod.requested.recordPartial("A") catch unreachable, false);
    expect_eq(mod.requested.recordAll(), false);
});

test("bundler barrel: out-of-range record index and requested capacity guards", fn() {
    let init_err = false;
    * = BarrelModule.init(@as(u32, @intCast(MAX_BARREL_IMPORT_RECORDS)) + 1) catch |e| {
        init_err = (e == BarrelError.ImportRecordOutOfRange);
        BarrelModule{}
    };
    expect_eq(init_err, true);

    let mod = BarrelModule.init(2) catch unreachable;
    let rec_err = false;
    mod.addReExport("Bad", "Bad", 2, false) catch |e| {
        rec_err = (e == BarrelError.ImportRecordOutOfRange);
    };
    expect_eq(rec_err, true);

    // Fill RequestedExportsSet to MAX_BARREL_REQUESTED_EXPORTS and verify overflow error
    let req = RequestedExportsSet.init();
    const names = [
        "e00", "e01", "e02", "e03", "e04", "e05", "e06", "e07",
        "e08", "e09", "e10", "e11", "e12", "e13", "e14", "e15",
        "e16", "e17", "e18", "e19", "e20", "e21", "e22", "e23",
        "e24", "e25", "e26", "e27", "e28", "e29", "e30", "e31",
        "e32", "e33", "e34", "e35", "e36", "e37", "e38", "e39",
        "e40", "e41", "e42", "e43", "e44", "e45", "e46", "e47",
        "e48", "e49", "e50", "e51", "e52", "e53", "e54", "e55",
        "e56", "e57", "e58", "e59", "e60", "e61", "e62", "e63",
    ];
    let i: usize = 0;
    while (i < MAX_BARREL_REQUESTED_EXPORTS) {
        expect_eq(req.recordPartial(names[i]) catch unreachable, true);
        i += 1;
    }
    let cap_err = false;
    * = req.recordPartial("e64_overflow") catch |e| {
        cap_err = (e == BarrelError.RequestedCapacityExceeded);
        false
    };
    expect_eq(cap_err, true);
});
