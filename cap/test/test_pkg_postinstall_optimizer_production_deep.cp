import { test, expect_eq } from "cap:test";
import {
    MAX_DYNAMIC_OPTIMIZER_RULES,
    PostinstallMode,
    SemverTriple,
    PostinstallPkgInfo,
    PostinstallOptimizerConfig,
    findNativeBinlinkReplacement,
    PackageID,
    PackageMeta,
    OperatingSystem,
    Architecture,
} from "cap:pkg";

fn main() {
    test("postinstall_optimizer: SemverTriple parse & minimum version gating", () => {
        const v1 = SemverTriple.parse("0.33.0").?;
        expect_eq(v1.major, 0 as u32);
        expect_eq(v1.minor, 33 as u32);
        expect_eq(v1.patch, 0 as u32);

        const v2 = SemverTriple.parse("v0.33.5-beta.1+build.42").?;
        expect_eq(v2.major, 0 as u32);
        expect_eq(v2.minor, 33 as u32);
        expect_eq(v2.patch, 5 as u32);
        expect_eq(v2.isAtLeast(v1), true);
        expect_eq(v1.isAtLeast(v2), false);

        const v_old = SemverTriple.parse("0.32.9").?;
        expect_eq(v_old.isAtLeast(v1), false);

        expect_eq(SemverTriple.parse("") == null, true);
        expect_eq(SemverTriple.parse("0.33") == null, true);
        expect_eq(SemverTriple.parse("abc.1.2") == null, true);
        expect_eq(SemverTriple.parse("1.2.3x") == null, true);
        expect_eq(SemverTriple.parse("999999999999999999.0.0") == null, true);
    });

    test("postinstall_optimizer: built-in defaults for esbuild, claude-code & sharp version gate", () => {
        const cfg = PostinstallOptimizerConfig.init();

        const m_esbuild = cfg.resolveMode(PostinstallPkgInfo{ name: "esbuild" }).?;
        expect_eq(m_esbuild == PostinstallMode.native_binlink, true);

        const m_claude = cfg.resolveMode(PostinstallPkgInfo{ name: "@anthropic-ai/claude-code" }).?;
        expect_eq(m_claude == PostinstallMode.native_binlink, true);

        const m_sharp_new = cfg.resolveMode(PostinstallPkgInfo{
            name: "sharp",
            version: SemverTriple.init(0, 33, 0),
        }).?;
        expect_eq(m_sharp_new == PostinstallMode.ignore, true);

        const m_sharp_1 = cfg.resolveMode(PostinstallPkgInfo{
            name: "sharp",
            version: SemverTriple.init(1, 0, 0),
        }).?;
        expect_eq(m_sharp_1 == PostinstallMode.ignore, true);

        // sharp < 0.33.0 must NOT be ignored
        expect_eq(cfg.resolveMode(PostinstallPkgInfo{
            name: "sharp",
            version: SemverTriple.init(0, 32, 6),
        }) == null, true);

        // sharp with null version -> .ignore (matches Bun)
        const m_sharp_unver = cfg.resolveMode(PostinstallPkgInfo{ name: "sharp", version: null }).?;
        expect_eq(m_sharp_unver == PostinstallMode.ignore, true);

        expect_eq(cfg.resolveMode(PostinstallPkgInfo{ name: "lodash" }) == null, true);
        expect_eq(cfg.resolveMode(PostinstallPkgInfo{ name: "" }) == null, true);
    });

    test("postinstall_optimizer: findNativeBinlinkReplacement platform matching & win32 guard", () => {
        const metas: [4]PackageMeta = [
            PackageMeta{ id: 0, os: OperatingSystem.ALL, cpu: Architecture.ALL },
            PackageMeta{ id: 1, os: OperatingSystem.DARWIN, cpu: Architecture.ARM64 },
            PackageMeta{ id: 2, os: OperatingSystem.LINUX, cpu: Architecture.X64 },
            PackageMeta{ id: 3, os: OperatingSystem.WIN32, cpu: Architecture.X64 },
        ];
        const resolutions: [5]PackageID = [99 as PackageID, 0 as PackageID, 1 as PackageID, 2 as PackageID, 3 as PackageID];

        const linux_hit = findNativeBinlinkReplacement(resolutions[0..], metas[0..], Architecture.X64, OperatingSystem.LINUX);
        expect_eq(linux_hit != null, true);
        expect_eq(linux_hit.?, 2 as PackageID);

        const mac_hit = findNativeBinlinkReplacement(resolutions[0..], metas[0..], Architecture.ARM64, OperatingSystem.DARWIN);
        expect_eq(mac_hit != null, true);
        expect_eq(mac_hit.?, 1 as PackageID);

        expect_eq(findNativeBinlinkReplacement(resolutions[0..], metas[0..], Architecture.ARM64, OperatingSystem.LINUX) == null, true);
        // Windows always returns null (needs .exe extensions)
        expect_eq(findNativeBinlinkReplacement(resolutions[0..], metas[0..], Architecture.X64, OperatingSystem.WIN32) == null, true);
    });

    test("postinstall_optimizer: shouldIgnoreLifecycleScripts tree_id & feature flags", () => {
        let cfg = PostinstallOptimizerConfig.init();
        const metas: [1]PackageMeta = [
            PackageMeta{ id: 0, os: OperatingSystem.LINUX, cpu: Architecture.X64 },
        ];
        const res: [1]PackageID = [0 as PackageID];

        expect_eq(cfg.shouldIgnoreLifecycleScripts(
            PostinstallPkgInfo{ name: "esbuild" },
            res[0..],
            metas[0..],
            Architecture.X64,
            OperatingSystem.LINUX,
            null,
        ), true);
        expect_eq(cfg.shouldIgnoreLifecycleScripts(
            PostinstallPkgInfo{ name: "esbuild" },
            res[0..],
            metas[0..],
            Architecture.X64,
            OperatingSystem.LINUX,
            0,
        ), true);

        // Non-root tree_id != 0 -> false for native_binlink
        expect_eq(cfg.shouldIgnoreLifecycleScripts(
            PostinstallPkgInfo{ name: "esbuild" },
            res[0..],
            metas[0..],
            Architecture.X64,
            OperatingSystem.LINUX,
            2,
        ), false);

        // disable_native_linker_flag blocks native_binlink but not .ignore
        cfg.disable_native_linker_flag = true;
        expect_eq(cfg.isNativeBinlinkEnabled(), false);
        expect_eq(cfg.shouldIgnoreLifecycleScripts(
            PostinstallPkgInfo{ name: "esbuild" },
            res[0..],
            metas[0..],
            Architecture.X64,
            OperatingSystem.LINUX,
            0,
        ), false);
        expect_eq(cfg.shouldIgnoreLifecycleScripts(
            PostinstallPkgInfo{ name: "sharp", version: SemverTriple.init(0, 34, 0) },
            res[0..],
            metas[0..],
            Architecture.X64,
            OperatingSystem.LINUX,
            0,
        ), true);

        // disable_ignore_scripts_flag blocks everything
        cfg.disable_ignore_scripts_flag = true;
        expect_eq(cfg.shouldIgnoreLifecycleScripts(
            PostinstallPkgInfo{ name: "sharp", version: SemverTriple.init(0, 34, 0) },
            res[0..],
            metas[0..],
            Architecture.X64,
            OperatingSystem.LINUX,
            0,
        ), false);
    });

    test("postinstall_optimizer: dynamic overrides replace built-in defaults", () => {
        let cfg = PostinstallOptimizerConfig.init();
        const custom_native: [2][]const u8 = ["my-native-tool", ""];
        expect_eq(cfg.configureNativeDependencies(custom_native[0..]), true);
        expect_eq(cfg.disable_default_native_binlinks, true);

        expect_eq(cfg.resolveMode(PostinstallPkgInfo{ name: "esbuild" }) == null, true);
        const m_custom = cfg.resolveMode(PostinstallPkgInfo{ name: "my-native-tool" }).?;
        expect_eq(m_custom == PostinstallMode.native_binlink, true);

        const custom_ignore: [1][]const u8 = ["legacy-pkg"];
        expect_eq(cfg.configureIgnoreScripts(custom_ignore[0..]), true);
        expect_eq(cfg.disable_default_ignore, true);
        expect_eq(cfg.resolveMode(PostinstallPkgInfo{ name: "sharp", version: SemverTriple.init(0, 35, 0) }) == null, true);
        const m_ign = cfg.resolveMode(PostinstallPkgInfo{ name: "legacy-pkg" }).?;
        expect_eq(m_ign == PostinstallMode.ignore, true);

        const prev_count = cfg.rule_count;
        expect_eq(cfg.addDynamicRule("legacy-pkg", PostinstallMode.native_binlink), true);
        expect_eq(cfg.rule_count, prev_count);
        const m_updated = cfg.resolveMode(PostinstallPkgInfo{ name: "legacy-pkg" }).?;
        expect_eq(m_updated == PostinstallMode.native_binlink, true);
    });

    test("postinstall_optimizer: dynamic rule capacity limit backpressure", () => {
        let cfg = PostinstallOptimizerConfig.init();
        expect_eq(cfg.addDynamicRule("", PostinstallMode.ignore), false);

        const names: [32][]const u8 = [
            "p00", "p01", "p02", "p03", "p04", "p05", "p06", "p07",
            "p08", "p09", "p10", "p11", "p12", "p13", "p14", "p15",
            "p16", "p17", "p18", "p19", "p20", "p21", "p22", "p23",
            "p24", "p25", "p26", "p27", "p28", "p29", "p30", "p31",
        ];
        let i: usize = 0;
        while (i < MAX_DYNAMIC_OPTIMIZER_RULES) : (i += 1) {
            expect_eq(cfg.addDynamicRule(names[i], PostinstallMode.ignore), true);
        }
        expect_eq(cfg.rule_count, MAX_DYNAMIC_OPTIMIZER_RULES);
        expect_eq(cfg.addDynamicRule("p32", PostinstallMode.ignore), false);
        expect_eq(cfg.addDynamicRule("p00", PostinstallMode.native_binlink), true);
    });
}
