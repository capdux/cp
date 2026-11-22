// Deep production regression tests for cap:pkg CLI argument parser (lib/pkg/cliArgs.cp)

import { test, expect, expectError } from "cap:test";
import {
    PKG_CLI_DEFAULT_NETWORK_CONCURRENCY,
    PkgCliArgsError,
    PkgCliSubcommand,
    PkgCliLogLevel,
    PkgCliLinker,
    PkgCliBackend,
    PkgCliDepTarget,
    parsePkgCliArgs,
} from "cap:pkg";

test("pkgCliArgs: subcommand alias parsing and default values", () => {
    const s_i = PkgCliSubcommand.parse("i") ?? return;
    expect(s_i == .Install).toBe(true);
    expect(s_i.asStr()).toEqual("install");

    const s_rm = PkgCliSubcommand.parse("uninstall") ?? return;
    expect(s_rm == .Remove).toBe(true);
    expect(s_rm.asStr()).toEqual("remove");

    const s_up = PkgCliSubcommand.parse("up") ?? return;
    expect(s_up == .Update).toBe(true);
    expect(PkgCliSubcommand.parse("unknown") == null).toBe(true);

    const empty_argv: [0][]const u8 = [];
    const parsed = parsePkgCliArgs(.Install, empty_argv[0..])?;
    expect(parsed.save).toBe(true);
    expect(parsed.network_concurrency).toBe(PKG_CLI_DEFAULT_NETWORK_CONCURRENCY);
    expect(parsed.shouldPersistChanges()).toBe(true);
    expect(parsed.canModifyLockfile()).toBe(true);
});

test("pkgCliArgs: install long flags, equals syntax, repeated filters and platforms", () => {
    const argv: [12][]const u8 = [
        "--frozen-lockfile",
        "--linker=isolated",
        "--backend",
        "hardlink",
        "--omit",
        "optional",
        "--omit=peer",
        "--filter",
        "./packages/*",
        "--cpu=arm64",
        "--os=linux",
        "--minimum-release-age=86400",
    ];
    const p = parsePkgCliArgs(.Install, argv[0..])?;
    expect(p.frozen_lockfile).toBe(true);
    expect(p.canModifyLockfile()).toBe(false);
    expect(p.linker == .Isolated).toBe(true);
    expect(p.backend == .Hardlink).toBe(true);
    expect(p.omit_dev).toBe(false);
    expect(p.omit_optional).toBe(true);
    expect(p.omit_peer).toBe(true);
    expect(p.filterSlice().len).toBe(1 as usize);
    expect(p.filterSlice()[0]).toEqual("./packages/*");
    expect(p.cpuSlice().len).toBe(1 as usize);
    expect(p.cpuSlice()[0]).toEqual("arm64");
    expect(p.osSlice().len).toBe(1 as usize);
    expect(p.osSlice()[0]).toEqual("linux");
    expect(p.minimum_release_age ?? 0).toBe(86400 as u64);
});

test("pkgCliArgs: add clustered short flags, config path and positional separator", () => {
    const argv: [7][]const u8 = [
        "-dE",
        "-c",
        "bunfig.toml",
        "--trust",
        "esbuild@0.24.0",
        "--",
        "--not-a-flag",
    ];
    const p = parsePkgCliArgs(.Add, argv[0..])?;
    expect(p.dep_target == .Dev).toBe(true);
    expect(p.exact).toBe(true);
    expect(p.trust).toBe(true);
    expect(p.config_path ?? "").toEqual("bunfig.toml");
    expect(p.positionalSlice().len).toBe(2 as usize);
    expect(p.positionalSlice()[0]).toEqual("esbuild@0.24.0");
    expect(p.positionalSlice()[1]).toEqual("--not-a-flag");
});

test("pkgCliArgs: production shorthand normalizes omit_dev and dry-run disables persist", () => {
    const argv: [5][]const u8 = ["-pf", "--dry-run", "--network-concurrency", "16", "--concurrent-scripts=4"];
    const p = parsePkgCliArgs(.Install, argv[0..])?;
    expect(p.production).toBe(true);
    expect(p.omit_dev).toBe(true);
    expect(p.force).toBe(true);
    expect(p.dry_run).toBe(true);
    expect(p.shouldPersistChanges()).toBe(false);
    expect(p.canModifyLockfile()).toBe(false);
    expect(p.network_concurrency).toBe(16 as u32);
    expect(p.concurrent_scripts ?? 0).toBe(4 as u32);
});

test("pkgCliArgs: update and pm subcommand-specific flags", () => {
    const up_argv: [3][]const u8 = ["-ir", "--latest", "--filter=apps/*"];
    const up = parsePkgCliArgs(.Update, up_argv[0..])?;
    expect(up.interactive).toBe(true);
    expect(up.recursive).toBe(true);
    expect(up.latest).toBe(true);
    expect(up.filterSlice()[0]).toEqual("apps/*");

    const pm_argv: [4][]const u8 = ["-a", "--json", "--top", "--depth=2"];
    const pm = parsePkgCliArgs(.Pm, pm_argv[0..])?;
    expect(pm.all_flag).toBe(true);
    expect(pm.json_output).toBe(true);
    expect(pm.top_only).toBe(true);
    expect(pm.depth ?? 99).toBe(2 as u32);
});

test("pkgCliArgs: error guards for unknown flags, missing values, subcommand misuse and conflicts", () => {
    const a1: [1][]const u8 = ["--bogus-flag"];
    expectError(parsePkgCliArgs(.Install, a1[0..]), PkgCliArgsError.UnknownFlag);

    const a2: [1][]const u8 = ["--registry"];
    expectError(parsePkgCliArgs(.Install, a2[0..]), PkgCliArgsError.MissingFlagValue);

    const a3: [1][]const u8 = ["--linker=invalid"];
    expectError(parsePkgCliArgs(.Install, a3[0..]), PkgCliArgsError.InvalidFlagValue);

    const a4: [2][]const u8 = ["--network-concurrency", "0"];
    expectError(parsePkgCliArgs(.Install, a4[0..]), PkgCliArgsError.InvalidFlagValue);

    const a5: [1][]const u8 = ["--latest"];
    expectError(parsePkgCliArgs(.Remove, a5[0..]), PkgCliArgsError.FlagNotAllowedForSubcommand);

    const a6: [2][]const u8 = ["--silent", "--verbose"];
    expectError(parsePkgCliArgs(.Install, a6[0..]), PkgCliArgsError.ConflictingFlags);

    const a7: [2][]const u8 = ["--dev", "--optional"];
    expectError(parsePkgCliArgs(.Add, a7[0..]), PkgCliArgsError.ConflictingFlags);

    const a8: [1][]const u8 = ["--omit=unknown"];
    expectError(parsePkgCliArgs(.Install, a8[0..]), PkgCliArgsError.InvalidFlagValue);
});
