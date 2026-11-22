import { test, expect } from "cap:test";
import {
    PREINSTALL_MAX_PKGS,
    LIFECYCLE_LOG_MAX_ENTRIES,
    SLOW_SCRIPT_DEFAULT_INTERVAL_NS,
    SLOW_SCRIPT_VERBOSE_INTERVAL_NS,
    PreinstallStateKind,
    LifecycleScriptSlot,
    MonitorLogLevel,
    TrustSourceKind,
    PreinstallProbe,
    ActiveLifecycleSnapshot,
    stripPatchHashFolderSuffix,
    formatDurationOneDecimal,
    isLifecycleScriptTrusted,
    PreinstallStateTable,
    LifecycleTimeLog,
    SlowLifecycleMonitor,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("PreinstallStateTable determines all state transitions and preserves non-Unknown states", () => {
    let tbl = PreinstallStateTable.init();

    // 1. Platform disabled -> Done
    const s0 = tbl.determine(PreinstallProbe{
        pkg_id: 0,
        is_disabled_for_platform: true,
    }) catch unreachable;
    expect(s0 == .Done);

    // 2. Patched with missing patchfile hash -> CalcPatchHash
    const s1 = tbl.determine(PreinstallProbe{
        pkg_id: 1,
        is_patched: true,
        patchfile_hash_missing: true,
    }) catch unreachable;
    expect(s1 == .CalcPatchHash);

    // 3. Cached folder present -> Done
    const s2 = tbl.determine(PreinstallProbe{
        pkg_id: 2,
        cached_folder_path: "zod@3.23.0",
        is_folder_in_cache: true,
    }) catch unreachable;
    expect(s2 == .Done);

    // 4. Patched, patched folder missing, base folder present -> ApplyPatch
    const s3 = tbl.determine(PreinstallProbe{
        pkg_id: 3,
        is_patched: true,
        cached_folder_path: "is-even@1.0.0_patch_hash=deadbeef",
        is_folder_in_cache: false,
        is_base_folder_in_cache: true,
    }) catch unreachable;
    expect(s3 == .ApplyPatch);

    // 5. Patched, base folder missing -> Extract
    const s4 = tbl.determine(PreinstallProbe{
        pkg_id: 4,
        is_patched: true,
        cached_folder_path: "is-odd@1.0.0_patch_hash=cafebabe",
        is_folder_in_cache: false,
        is_base_folder_in_cache: false,
    }) catch unreachable;
    expect(s4 == .Extract);

    // 6. Empty cached_folder_path -> Extract
    const s5 = tbl.determine(PreinstallProbe{
        pkg_id: 5,
        cached_folder_path: "",
    }) catch unreachable;
    expect(s5 == .Extract);

    // 7. Already determined state returns existing state idempotently
    const s3_again = tbl.determine(PreinstallProbe{
        pkg_id: 3,
        is_disabled_for_platform: true,
    }) catch unreachable;
    expect(s3_again == .ApplyPatch);
});

test("stripPatchHashFolderSuffix extracts base folder and rejects malformed names", () => {
    const base = stripPatchHashFolderSuffix("is-even@1.0.0_patch_hash=abc123def") catch unreachable;
    expect(strEq(base, "is-even@1.0.0"));

    let saw_err = false;
    * = stripPatchHashFolderSuffix("is-even@1.0.0") catch {
        saw_err = true;
    };
    expect(saw_err);
});

test("LifecycleTimeLog tracks slowest script and formats one-decimal durations", () => {
    let dbuf: [64]u8 = [0; 64];
    expect(strEq(dbuf[0..(formatDurationOneDecimal(850, dbuf[0..]) catch unreachable)], "850ns"));
    expect(strEq(dbuf[0..(formatDurationOneDecimal(42_500, dbuf[0..]) catch unreachable)], "42.5us"));
    expect(strEq(dbuf[0..(formatDurationOneDecimal(125_300_000, dbuf[0..]) catch unreachable)], "125.3ms"));
    expect(strEq(dbuf[0..(formatDurationOneDecimal(2_350_000_000, dbuf[0..]) catch unreachable)], "2.3s"));

    let log = LifecycleTimeLog.init();
    expect(log.longestEntry() == null);

    * = log.record("sharp", .Install, 400_000_000) catch unreachable;
    * = log.record("esbuild", .Postinstall, 2_350_000_000) catch unreachable;
    * = log.record("husky", .Prepare, 120_000_000) catch unreachable;
    expect(log.slice().len == 3);

    if (longest := log.longestEntry()) {
        expect(strEq(longest.package_name, "esbuild"));
        expect(longest.script == .Postinstall);
    } else {
        expect(false);
    }

    let wbuf: [128]u8 = [0; 128];
    const wn = log.formatLongestWarning(wbuf[0..]) catch unreachable;
    expect(strEq(wbuf[0..wn], "esbuild's postinstall script took 2.3s"));
});

test("SlowLifecycleMonitor enforces interval thresholds, tick dedup, and possessive grammar", () => {
    let mon = SlowLifecycleMonitor.init(.Verbose, false);
    expect(mon.thresholdNs() == SLOW_SCRIPT_VERBOSE_INTERVAL_NS);

    let buf: [128]u8 = [0; 128];
    const snap_s = ActiveLifecycleSnapshot{ package_name: "fsevents", started_at_ns: 1_000_000_000 };

    // Running only 3s (< 5s verbose threshold) -> no warning
    const n0 = mon.pollAndFormatWarning(1, 4_000_000_000, snap_s, buf[0..]) catch unreachable;
    expect(n0 == 0);

    // Running 6.2s (> 5s) -> warning with "fsevents'" possessive
    const n1 = mon.pollAndFormatWarning(2, 7_200_000_000, snap_s, buf[0..]) catch unreachable;
    expect(strEq(buf[0..n1], "fsevents' postinstall cost you 6.2s"));
    expect(mon.report_count == 1);

    // Same tick -> deduplicated
    const n2 = mon.pollAndFormatWarning(2, 20_000_000_000, snap_s, buf[0..]) catch unreachable;
    expect(n2 == 0);

    // Next tick, only 2s since last report (< 5s) -> throttled
    const n3 = mon.pollAndFormatWarning(3, 9_200_000_000, snap_s, buf[0..]) catch unreachable;
    expect(n3 == 0);

    // Next tick, > 5s since last report -> emits again with "'s" grammar for non-s package
    const snap_es = ActiveLifecycleSnapshot{ package_name: "esbuild", started_at_ns: 1_000_000_000 };
    const n4 = mon.pollAndFormatWarning(4, 13_500_000_000, snap_es, buf[0..]) catch unreachable;
    expect(strEq(buf[0..n4], "esbuild's postinstall cost you 12.5s"));
    expect(mon.report_count == 2);

    // Silent and disabled monitors never emit
    let silent = SlowLifecycleMonitor.init(.Silent, false);
    expect((silent.pollAndFormatWarning(1, 99_000_000_000, snap_es, buf[0..]) catch unreachable) == 0);
    let disabled = SlowLifecycleMonitor.init(.Default, true);
    expect(disabled.thresholdNs() == SLOW_SCRIPT_DEFAULT_INTERVAL_NS);
    expect((disabled.pollAndFormatWarning(1, 99_000_000_000, snap_es, buf[0..]) catch unreachable) == 0);
});

test("isLifecycleScriptTrusted enforces npm-only default allowlist and explicit trust for non-npm sources", () => {
    // Default allowlist applies only to npm
    expect(isLifecycleScriptTrusted(.Npm, false, true));
    expect(!isLifecycleScriptTrusted(.Npm, false, false));

    // Non-npm sources ignore default allowlist and require explicit trustedDependencies entry
    expect(!isLifecycleScriptTrusted(.File, false, true));
    expect(!isLifecycleScriptTrusted(.Link, false, true));
    expect(!isLifecycleScriptTrusted(.Git, false, true));
    expect(!isLifecycleScriptTrusted(.Github, false, true));
    expect(!isLifecycleScriptTrusted(.Tarball, false, true));

    // Explicit trust always allows
    expect(isLifecycleScriptTrusted(.Git, true, false));
    expect(isLifecycleScriptTrusted(.File, true, false));
});

test("Boundary guards: out-of-bounds pkg_id, empty name, log capacity, and tiny output buffer", () => {
    let tbl = PreinstallStateTable.init();
    expect(tbl.get(999) == .Unknown);

    let saw_oob = false;
    * = tbl.set(PREINSTALL_MAX_PKGS as u32, .Done) catch {
        saw_oob = true;
    };
    expect(saw_oob);

    let log = LifecycleTimeLog.init();
    let saw_empty = false;
    * = log.record("", .Postinstall, 100) catch {
        saw_empty = true;
    };
    expect(saw_empty);

    for (_ of 0..LIFECYCLE_LOG_MAX_ENTRIES) {
        * = log.record("pkg", .Postinstall, 1000) catch unreachable;
    }
    let saw_full = false;
    * = log.record("overflow", .Postinstall, 1000) catch {
        saw_full = true;
    };
    expect(saw_full);

    let tiny: [4]u8 = [0; 4];
    let saw_small = false;
    * = log.formatLongestWarning(tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);
});
