// Origin: Pure-CP Stdlib Test Suite (lib/pkg/pkgMeta.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Lockfile Package Meta (`PkgMeta`):
//   1. Default PkgMeta initialization (`PKG_META_INVALID_ID`, `PKG_OS_ALL`, `PKG_ARCH_ALL`)
//   2. npm `os` / `cpu` allowlist & `!` negation bitmask evaluation (`applyNpmOsList`, `applyNpmArchList`)
//   3. `isDisabled(cpu, os)` & `matchesPlatform(cpu, os)` cross-platform gating
//   4. 3-state `HasInstallScript` (`Old`/`False`/`True`), `needsUpdate()`, and `validateForSave()` guard
//   5. `setManDir`, `setIntegrity`, `cloneWithId` deep-value isolation
//   6. Oversized field (`FieldTooLong`) overflow protection

import { test, expect_eq, expect_true } from "cap:test";
import {
    PKG_META_INVALID_ID,
    PKG_OS_ALL,
    PKG_OS_LINUX,
    PKG_OS_DARWIN,
    PKG_OS_WIN32,
    PKG_ARCH_ALL,
    PKG_ARCH_X64,
    PKG_ARCH_ARM64,
    PKG_ARCH_IA32,
    PkgMetaOrigin,
    HasInstallScript,
    parseNpmOsBit,
    parseNpmArchBit,
    applyNpmOsList,
    applyNpmArchList,
    PkgMeta,
} from "../../lib/pkg/pkgMeta.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: Defaults & bit parsers
// ============================================================================
test("pkgMeta: default initialization and OS/Arch bit parsing", () => {
    const m = PkgMeta.init();
    expect_eq(m.id, PKG_META_INVALID_ID);
    expect_eq(m.os, PKG_OS_ALL);
    expect_eq(m.arch, PKG_ARCH_ALL);
    expect_true(!m.hasInstallScript());
    expect_true(!m.needsUpdate());
    expect_true(!m.hasIntegrity());

    expect_eq(parseNpmOsBit("linux"), PKG_OS_LINUX);
    expect_eq(parseNpmOsBit("DARWIN"), PKG_OS_DARWIN);
    expect_eq(parseNpmOsBit("win32"), PKG_OS_WIN32);
    expect_eq(parseNpmOsBit("unknown_os"), 0);

    expect_eq(parseNpmArchBit("x64"), PKG_ARCH_X64);
    expect_eq(parseNpmArchBit("ARM64"), PKG_ARCH_ARM64);
    expect_eq(parseNpmArchBit("unknown_cpu"), 0);
});

// ============================================================================
// Test 2: npm `os` / `cpu` positive allowlists & `!` negation exclusions
// ============================================================================
test("pkgMeta: applyNpmOsList and applyNpmArchList handle allowlist and negation", () => {
    const empty: [0][]const u8 = [];
    expect_eq(applyNpmOsList(empty[0..]), PKG_OS_ALL);
    expect_eq(applyNpmArchList(empty[0..]), PKG_ARCH_ALL);

    const unix_only: [2][]const u8 = ["linux", "darwin"];
    const os_mask = applyNpmOsList(unix_only[0..]);
    expect_true((os_mask & PKG_OS_LINUX) != 0);
    expect_true((os_mask & PKG_OS_DARWIN) != 0);
    expect_true((os_mask & PKG_OS_WIN32) == 0);

    // Negation only: ["!win32"] allows all except win32
    const not_win: [1][]const u8 = ["!win32"];
    const nw_mask = applyNpmOsList(not_win[0..]);
    expect_true((nw_mask & PKG_OS_LINUX) != 0);
    expect_true((nw_mask & PKG_OS_DARWIN) != 0);
    expect_true((nw_mask & PKG_OS_WIN32) == 0);

    // Negation on cpu: ["!ia32"]
    const not_ia32: [1][]const u8 = ["!ia32"];
    const arch_mask = applyNpmArchList(not_ia32[0..]);
    expect_true((arch_mask & PKG_ARCH_X64) != 0);
    expect_true((arch_mask & PKG_ARCH_ARM64) != 0);
    expect_true((arch_mask & PKG_ARCH_IA32) == 0);
});

// ============================================================================
// Test 3: isDisabled(cpu, os) & matchesPlatform(cpu, os)
// ============================================================================
test("pkgMeta: isDisabled gates native packages by target CPU and OS", () => {
    let m = PkgMeta.initWithId(42, PkgMetaOrigin.Npm);
    const os_list: [1][]const u8 = ["linux"];
    const cpu_list: [1][]const u8 = ["x64"];
    m.os = applyNpmOsList(os_list[0..]);
    m.arch = applyNpmArchList(cpu_list[0..]);

    expect_true(m.matchesPlatform(PKG_ARCH_X64, PKG_OS_LINUX));
    expect_true(!m.isDisabled(PKG_ARCH_X64, PKG_OS_LINUX));

    expect_true(m.isDisabled(PKG_ARCH_ARM64, PKG_OS_LINUX));
    expect_true(m.isDisabled(PKG_ARCH_X64, PKG_OS_DARWIN));
    expect_true(m.isDisabled(PKG_ARCH_ARM64, PKG_OS_WIN32));
});

// ============================================================================
// Test 4: 3-state HasInstallScript (`Old` / `False` / `True`) & save guard
// ============================================================================
test("pkgMeta: HasInstallScript 3-state migration and validateForSave guard", () => {
    let m = PkgMeta.init();
    m.has_install_script = HasInstallScript.Old;
    expect_true(m.needsUpdate());
    expect_true(!m.hasInstallScript());

    // Saving `.Old` must fail with LegacyInstallScriptState
    let rejected_old = false;
    m.validateForSave() catch {
        rejected_old = true;
    };
    expect_true(rejected_old);

    m.setHasInstallScript(true);
    expect_true(m.hasInstallScript());
    expect_true(!m.needsUpdate());
    m.validateForSave() catch {
        expect_true(false);
    };

    m.setHasInstallScript(false);
    expect_true(!m.hasInstallScript());
    m.validateForSave() catch {
        expect_true(false);
    };
});

// ============================================================================
// Test 5: manDir, integrity & cloneWithId isolation
// ============================================================================
test("pkgMeta: manDir, SRI integrity & cloneWithId value isolation", () => {
    let m = PkgMeta.initWithId(7, PkgMetaOrigin.Tarball);
    m.setManDir("man/man1") catch {
        expect_true(false);
    };
    m.setIntegrity("sha512-abcdef0123456789==") catch {
        expect_true(false);
    };
    m.setHasInstallScript(true);

    expect_true(strEq(m.manDir(), "man/man1"));
    expect_true(m.hasIntegrity());
    expect_true(strEq(m.integrity(), "sha512-abcdef0123456789=="));

    let cloned = m.cloneWithId(99);
    expect_eq(cloned.id, 99);
    expect_eq(m.id, 7);
    expect_true(cloned.origin == PkgMetaOrigin.Tarball);
    expect_true(cloned.hasInstallScript());
    expect_true(strEq(cloned.integrity(), "sha512-abcdef0123456789=="));

    cloned.setManDir("share/man") catch {
        expect_true(false);
    };
    expect_true(strEq(m.manDir(), "man/man1"));
    expect_true(strEq(cloned.manDir(), "share/man"));
});

// ============================================================================
// Test 6: Oversized field overflow guard
// ============================================================================
test("pkgMeta: rejects oversized manDir and integrity strings safely", () => {
    let m = PkgMeta.init();
    let long_buf: [200]u8 = ['x'; 200];

    let bad_man = false;
    m.setManDir(long_buf[0..]) catch {
        bad_man = true;
    };
    expect_true(bad_man);

    let bad_sri = false;
    m.setIntegrity(long_buf[0..]) catch {
        bad_sri = true;
    };
    expect_true(bad_sri);
});
