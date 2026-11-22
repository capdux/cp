import { test, expect } from "cap:test";
import {
    PkgAuthType,
    PkgInstallOptions,
    PkgLogLevel,
    PkgPublishAccess,
    PkgPublishConfig,
} from "cap:pkg";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

test("pkg install_options: PkgLogLevel parsing, verbosity, and progress visibility", () => {
    expect(PkgLogLevel.Default.isVerbose()).toBe(false);
    expect(PkgLogLevel.Default.showProgress()).toBe(true);
    expect(PkgLogLevel.Verbose.isVerbose()).toBe(true);
    expect(PkgLogLevel.Verbose.showProgress()).toBe(true);
    expect(PkgLogLevel.VerboseNoProgress.isVerbose()).toBe(true);
    expect(PkgLogLevel.VerboseNoProgress.showProgress()).toBe(false);
    expect(PkgLogLevel.DefaultNoProgress.showProgress()).toBe(false);
    expect(PkgLogLevel.Silent.isSilent()).toBe(true);
    expect(PkgLogLevel.Quiet.showProgress()).toBe(false);

    expect((PkgLogLevel.fromStr("verbose") catch PkgLogLevel.Silent) == PkgLogLevel.Verbose).toBe(true);
    expect((PkgLogLevel.fromStr("QUIET") catch PkgLogLevel.Default) == PkgLogLevel.Quiet).toBe(true);

    let bad_level = false;
    * = PkgLogLevel.fromStr("noisy") catch {
        bad_level = true;
    };
    expect(bad_level).toBe(true);
});

test("pkg install_options: PkgPublishConfig access, auth_type, tag defaults, and scoped validation", () => {
    expect((PkgPublishAccess.fromStr("public") catch PkgPublishAccess.Restricted) == PkgPublishAccess.Public).toBe(true);
    expect((PkgPublishAccess.fromStr("RESTRICTED") catch PkgPublishAccess.Public) == PkgPublishAccess.Restricted).toBe(true);
    expect(sliceEq(PkgPublishAccess.Restricted.asStr(), "restricted")).toBe(true);

    expect((PkgAuthType.fromStr("web") catch PkgAuthType.Legacy) == PkgAuthType.Web).toBe(true);
    expect(sliceEq(PkgAuthType.Legacy.asStr(), "legacy")).toBe(true);

    let cfg = PkgPublishConfig{};
    expect(sliceEq(cfg.effectiveTag(), "latest")).toBe(true);
    cfg.tag = "next";
    expect(sliceEq(cfg.effectiveTag(), "next")).toBe(true);

    // Scoped package with restricted access is allowed
    cfg.access = PkgPublishAccess.Restricted;
    let scoped_ok = true;
    cfg.validateForPackage("@acme/core") catch {
        scoped_ok = false;
    };
    expect(scoped_ok).toBe(true);

    // Unscoped package with restricted access is rejected
    let unscoped_err = false;
    cfg.validateForPackage("plain-pkg") catch {
        unscoped_err = true;
    };
    expect(unscoped_err).toBe(true);

    // Invalid gzip level (> 9) is rejected
    cfg.access = PkgPublishAccess.Public;
    cfg.gzip_level = 12;
    let gzip_err = false;
    cfg.validateForPackage("plain-pkg") catch {
        gzip_err = true;
    };
    expect(gzip_err).toBe(true);
});

test("pkg install_options: applyDryRun, applyFrozenLockfile, applyProduction, and --omit", () => {
    let opts = PkgInstallOptions.init();
    expect(opts.do_flags.save_lockfile).toBe(true);
    expect(opts.do_flags.install_packages).toBe(true);
    expect(opts.do_flags.run_scripts).toBe(true);

    opts.applyDryRun();
    expect(opts.do_flags.save_lockfile).toBe(false);
    expect(opts.do_flags.install_packages).toBe(false);
    expect(opts.do_flags.write_package_json).toBe(false);
    expect(opts.do_flags.run_scripts).toBe(false);
    expect(opts.publish.dry_run).toBe(true);

    let ci_opts = PkgInstallOptions.init();
    ci_opts.applyFrozenLockfile();
    expect(ci_opts.enable.frozen_lockfile).toBe(true);
    expect(ci_opts.do_flags.save_lockfile).toBe(false);

    ci_opts.applyProduction();
    expect(ci_opts.omit.omit_dev).toBe(true);
    expect(ci_opts.omit.omit_optional).toBe(true);
    expect(ci_opts.omit.omit_peer).toBe(false);

    let omit_peer_ok = true;
    ci_opts.applyOmit("peer") catch {
        omit_peer_ok = false;
    };
    expect(omit_peer_ok).toBe(true);
    expect(ci_opts.omit.omit_peer).toBe(true);

    let bad_omit = false;
    ci_opts.applyOmit("unknown") catch {
        bad_omit = true;
    };
    expect(bad_omit).toBe(true);
});

test("pkg install_options: global virtual store requires isolated linker", () => {
    let opts = PkgInstallOptions.init();
    expect(opts.shouldUseGlobalVirtualStore(true)).toBe(false);
    expect(opts.shouldUseGlobalVirtualStore(false)).toBe(false);

    // Rejects enabling under hoisted linker
    let hoisted_err = false;
    opts.enableGlobalVirtualStore(false) catch {
        hoisted_err = true;
    };
    expect(hoisted_err).toBe(true);
    expect(opts.enable.global_virtual_store).toBe(false);

    // Succeeds under isolated linker
    let iso_ok = true;
    opts.enableGlobalVirtualStore(true) catch {
        iso_ok = false;
    };
    expect(iso_ok).toBe(true);
    expect(opts.shouldUseGlobalVirtualStore(true)).toBe(true);
    // Still false if queried with hoisted linker
    expect(opts.shouldUseGlobalVirtualStore(false)).toBe(false);
});

test("pkg install_options: minimum_release_age_secs supply-chain cooldown gate", () => {
    let opts = PkgInstallOptions.init();
    // Disabled (0) always allows
    expect(opts.isReleaseAgeSatisfied(1_700_000_000, 1_700_000_001)).toBe(true);

    // 24h cooldown (86400s)
    opts.minimum_release_age_secs = 86400;
    expect(opts.isReleaseAgeSatisfied(1_700_000_000, 1_700_086_400)).toBe(true);
    expect(opts.isReleaseAgeSatisfied(1_700_000_000, 1_700_086_399)).toBe(false);
    // Future publish timestamp (clock skew) rejected safely without underflow
    expect(opts.isReleaseAgeSatisfied(1_700_010_000, 1_700_000_000)).toBe(false);
});

test("pkg install_options: invalid access and auth_type parser guards", () => {
    let acc_err = false;
    * = PkgPublishAccess.fromStr("private") catch {
        acc_err = true;
    };
    expect(acc_err).toBe(true);

    let auth_err = false;
    * = PkgAuthType.fromStr("oauth") catch {
        auth_err = true;
    };
    expect(auth_err).toBe(true);
});
