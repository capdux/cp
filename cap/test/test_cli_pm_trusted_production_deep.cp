// cap/test/test_cli_pm_trusted_production_deep.cp
// 深度生产级回归测试：cap:cli 生命周期脚本信任与 trustedDependencies 供应链防护引擎 (lib/cli/pm_trusted.cp)

import { test } from "cap:test";
import {
    PM_TRUST_MAX_ALLOWLIST,
    PmDepSourceKind,
    PmLifecyclePhase,
    PmTrustDecision,
    PmTrustSpec,
    PmLifecyclePackage,
    PmTrustPolicy,
} from "cap:cli";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn containsStr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) { return true; }
    if (needle.len > haystack.len) { return false; }
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return true; }
        i += 1;
    }
    return false;
}

test("pm_trusted: spec parsing supports plain, scoped and version-pinned entries") {
    const s1 = PmTrustSpec.parse("esbuild")?;
    if (!bytesEq(s1.name, "esbuild") || s1.version.len != 0) { return err.PlainSpecMismatch; }
    if (!s1.matches("esbuild", "0.24.0") || s1.matches("sharp", "0.33.0")) { return err.PlainSpecMatchFailed; }

    const s2 = PmTrustSpec.parse("@swc/core")?;
    if (!bytesEq(s2.name, "@swc/core") || s2.version.len != 0) { return err.ScopedSpecMismatch; }
    if (!s2.matches("@swc/core", "1.7.0")) { return err.ScopedSpecMatchFailed; }

    const s3 = PmTrustSpec.parse("@swc/core@1.7.2")?;
    if (!bytesEq(s3.name, "@swc/core") || !bytesEq(s3.version, "1.7.2")) { return err.ScopedVersionSpecMismatch; }
    if (!s3.matches("@swc/core", "1.7.2") || s3.matches("@swc/core", "1.7.3")) {
        return err.VersionPinnedMatchFailed;
    }
}

test("pm_trusted: default trust allows npm registry but blocks non-registry spoofing") {
    let pol = PmTrustPolicy.initWithBuiltins(false);

    // npm 来源的 esbuild 命中默认白名单 -> AllowedDefault
    let npm_esbuild = PmLifecyclePackage{ name: "esbuild", version: "0.24.0", source: PmDepSourceKind.NpmRegistry };
    npm_esbuild.addScript(PmLifecyclePhase.Postinstall);
    const d1 = pol.evaluate(npm_esbuild);
    if (d1 != PmTrustDecision.AllowedDefault || !d1.isAllowed()) {
        return err.NpmDefaultTrustShouldAllow;
    }

    // file:/git:/github: 来源即便同名 esbuild 也必须拦截为 BlockedNonRegistrySpoof
    let git_esbuild = PmLifecyclePackage{ name: "esbuild", version: "0.24.0", source: PmDepSourceKind.Git };
    git_esbuild.addScript(PmLifecyclePhase.Postinstall);
    const d2 = pol.evaluate(git_esbuild);
    if (d2 != PmTrustDecision.BlockedNonRegistrySpoof || !d2.isBlocked()) {
        return err.GitSpoofMustBeBlocked;
    }

    // 显式加入 trustedDependencies 后，允许该非 npm 来源执行脚本
    * = pol.addExplicitTrust("esbuild@0.24.0")?;
    if (pol.evaluate(git_esbuild) != PmTrustDecision.AllowedExplicit) {
        return err.ExplicitTrustShouldAllowGit;
    }
}

test("pm_trusted: --ignore-scripts overrides both explicit and default allowlists") {
    let pol = PmTrustPolicy.initWithBuiltins(true);
    * = pol.addExplicitTrust("node-sass")?;

    let pkg = PmLifecyclePackage{ name: "node-sass", version: "9.0.0", source: PmDepSourceKind.NpmRegistry };
    pkg.addScript(PmLifecyclePhase.Postinstall);
    if (pol.evaluate(pkg) != PmTrustDecision.BlockedIgnoreScripts) {
        return err.IgnoreScriptsMustOverrideExplicit;
    }

    // 无任何生命周期脚本的包不受阻断
    const no_scripts = PmLifecyclePackage{ name: "lodash", version: "4.17.21", source: PmDepSourceKind.NpmRegistry };
    if (pol.evaluate(no_scripts) != PmTrustDecision.NoLifecycleScripts) {
        return err.NoScriptsDecisionMismatch;
    }
}

test("pm_trusted: lifecycle phase parsing and bitmask round-trip") {
    if (PmLifecyclePhase.parse("postinstall") == null) { return err.ParsePostinstallFailed; }
    if (PmLifecyclePhase.parse("preinstall") == null) { return err.ParsePreinstallFailed; }
    if (PmLifecyclePhase.parse("build") != null) { return err.NonLifecycleShouldBeNull; }

    let pkg = PmLifecyclePackage{ name: "native-addon", version: "1.0.0" };
    pkg.addScript(PmLifecyclePhase.Preinstall);
    pkg.addScript(PmLifecyclePhase.Postinstall);
    if (!pkg.hasScripts() || !pkg.hasScript(PmLifecyclePhase.Preinstall) ||
        !pkg.hasScript(PmLifecyclePhase.Postinstall) || pkg.hasScript(PmLifecyclePhase.Prepare))
    {
        return err.ScriptMaskMismatch;
    }
}

test("pm_trusted: sorting explicit allowlist and JSON array serialization") {
    let pol = PmTrustPolicy.init(false);
    * = pol.addExplicitTrust("node-sass")?;
    * = pol.addExplicitTrust("@swc/core")?;
    * = pol.addExplicitTrust("esbuild")?;
    // 重复添加幂等返回 false
    if (pol.addExplicitTrust("esbuild")?) { return err.DuplicateTrustShouldReturnFalse; }

    pol.sortExplicitAllowlist();
    let buf: [128]u8 = [0; 128];
    const n = pol.formatTrustedJsonArray(buf[0..])?;
    if (!bytesEq(buf[0..n], "[\"@swc/core\",\"esbuild\",\"node-sass\"]")) {
        return err.SortedJsonArrayMismatch;
    }
}

test("pm_trusted: formatUntrustedReport handles zero and multiple blocked dependencies") {
    let pol = PmTrustPolicy.initWithBuiltins(false);

    let p_ok = PmLifecyclePackage{ name: "esbuild", version: "0.24.0", source: PmDepSourceKind.NpmRegistry };
    p_ok.addScript(PmLifecyclePhase.Postinstall);

    let p_bad1 = PmLifecyclePackage{ name: "custom-native", version: "2.1.0", source: PmDepSourceKind.NpmRegistry };
    p_bad1.addScript(PmLifecyclePhase.Preinstall);
    p_bad1.addScript(PmLifecyclePhase.Postinstall);

    let p_bad2 = PmLifecyclePackage{ name: "sharp", version: "0.33.0", source: PmDepSourceKind.FilePath };
    p_bad2.addScript(PmLifecyclePhase.Install);

    // 全部受信任场景
    const clean_list: [1]PmLifecyclePackage = [p_ok];
    let buf: [512]u8 = [0; 512];
    const n_clean = pol.formatUntrustedReport(clean_list[0..], buf[0..])?;
    if (!bytesEq(buf[0..n_clean], "Found 0 untrusted dependencies with scripts.\n")) {
        return err.CleanReportMismatch;
    }

    // 混合受信任与未受信任场景
    const mixed: [3]PmLifecyclePackage = [p_ok, p_bad1, p_bad2];
    const n_mix = pol.formatUntrustedReport(mixed[0..], buf[0..])?;
    const rep = buf[0..n_mix];
    if (!containsStr(rep, "custom-native@2.1.0 [preinstall, postinstall]") ||
        !containsStr(rep, "sharp@0.33.0 [install]") ||
        !containsStr(rep, "Found 2 untrusted dependencies with scripts.") ||
        containsStr(rep, "esbuild@0.24.0"))
    {
        return err.MixedUntrustedReportMismatch;
    }
}

test("pm_trusted: empty input, capacity overflow and tiny buffer guards") {
    let pol = PmTrustPolicy.init(false);

    // 空包名拒绝
    let empty_err = false;
    pol.addExplicitTrust("") catch {
        empty_err = true;
    };
    if (!empty_err) { return err.EmptySpecNotRejected; }

    // 超出白名单容量拦截
    const names: [65]([]const u8) = [
        "p01", "p02", "p03", "p04", "p05", "p06", "p07", "p08",
        "p09", "p10", "p11", "p12", "p13", "p14", "p15", "p16",
        "p17", "p18", "p19", "p20", "p21", "p22", "p23", "p24",
        "p25", "p26", "p27", "p28", "p29", "p30", "p31", "p32",
        "p33", "p34", "p35", "p36", "p37", "p38", "p39", "p40",
        "p41", "p42", "p43", "p44", "p45", "p46", "p47", "p48",
        "p49", "p50", "p51", "p52", "p53", "p54", "p55", "p56",
        "p57", "p58", "p59", "p60", "p61", "p62", "p63", "p64",
        "p65",
    ];
    let i: usize = 0;
    while (i < PM_TRUST_MAX_ALLOWLIST) {
        * = pol.addExplicitTrust(names[i])?;
        i += 1;
    }
    let overflow_err = false;
    pol.addExplicitTrust(names[64]) catch {
        overflow_err = true;
    };
    if (!overflow_err) { return err.AllowlistOverflowNotRejected; }

    // 缓冲区过小拦截 BufferTooSmall
    let tiny: [4]u8 = [0; 4];
    let buf_small = false;
    pol.formatTrustedJsonArray(tiny[0..]) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }
}
