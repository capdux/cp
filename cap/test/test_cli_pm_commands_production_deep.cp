// cap/test/test_cli_pm_commands_production_deep.cp
// Deep production tests for cap:cli Yarn v1/v2 Command Classifier & PM Progress Labels (pm_commands.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    YarnCommandEra,
    PmStage,
    yarnCommandEra,
    isYarnCommand,
    mapYarnToCapSubcommand,
    pmStagePlainName,
    pmStageLabel,
    formatPmStageBanner,
} from "cap:cli";

test("cli.pm_commands: Yarn v1-only, v2-only, shared & unknown command classification", () => {
    // Shared v1 + v2 commands
    expect_true(isYarnCommand("add"));
    expect_true(yarnCommandEra("add") == YarnCommandEra.both);
    expect_true(yarnCommandEra("install").supportsV1());
    expect_true(yarnCommandEra("install").supportsV2());
    expect_true(yarnCommandEra("workspaces") == YarnCommandEra.both);
    expect_true(yarnCommandEra("why") == YarnCommandEra.both);

    // v2-only commands
    expect_true(yarnCommandEra("dlx") == YarnCommandEra.v2_only);
    expect_true(yarnCommandEra("dlx").supportsV2());
    expect_false(yarnCommandEra("dlx").supportsV1());
    expect_true(yarnCommandEra("dedupe") == YarnCommandEra.v2_only);
    expect_true(yarnCommandEra("patch") == YarnCommandEra.v2_only);
    expect_true(yarnCommandEra("up") == YarnCommandEra.v2_only);

    // v1-only commands (including camelCase aliases from Bun list-of-yarn-commands.cp)
    expect_true(yarnCommandEra("autoclean") == YarnCommandEra.v1_only);
    expect_true(yarnCommandEra("autoclean").supportsV1());
    expect_false(yarnCommandEra("autoclean").supportsV2());
    expect_true(yarnCommandEra("upgrade-interactive") == YarnCommandEra.v1_only);
    expect_true(yarnCommandEra("upgradeInteractive") == YarnCommandEra.v1_only);
    expect_true(yarnCommandEra("generate-lock-entry") == YarnCommandEra.v1_only);
    expect_true(yarnCommandEra("generateLockEntry") == YarnCommandEra.v1_only);

    // Unknown & empty
    expect_false(isYarnCommand(""));
    expect_false(isYarnCommand("unknown-subcmd"));
    expect_false(yarnCommandEra("ADD").isKnown());
});

test("cli.pm_commands: mapYarnToCapSubcommand routing table", () => {
    const m_up = mapYarnToCapSubcommand("up");
    expect_true(m_up != null);
    if (s := m_up) { expect_eq(s, "update"); }

    const m_upg = mapYarnToCapSubcommand("upgrade-interactive");
    expect_true(m_upg != null);
    if (s := m_upg) { expect_eq(s, "update"); }

    const m_dlx = mapYarnToCapSubcommand("dlx");
    expect_true(m_dlx != null);
    if (s := m_dlx) { expect_eq(s, "x"); }

    const m_pack = mapYarnToCapSubcommand("pack");
    expect_true(m_pack != null);
    if (s := m_pack) { expect_eq(s, "pm pack"); }

    const m_list = mapYarnToCapSubcommand("list");
    expect_true(m_list != null);
    if (s := m_list) { expect_eq(s, "pm ls"); }

    const m_audit = mapYarnToCapSubcommand("audit");
    expect_true(m_audit != null);
    if (s := m_audit) { expect_eq(s, "audit"); }

    // Yarn-specific commands with no direct 1:1 cap equivalent return null
    expect_true(mapYarnToCapSubcommand("autoclean") == null);
    expect_true(mapYarnToCapSubcommand("unplug") == null);
    expect_true(mapYarnToCapSubcommand("") == null);
});

test("cli.pm_commands: pmStagePlainName & pmStageLabel ANSI vs non-ANSI degradation", () => {
    expect_eq(pmStagePlainName(PmStage.resolve), "Resolving");
    expect_eq(pmStagePlainName(PmStage.extract), "Resolving & extracting");
    expect_eq(pmStagePlainName(PmStage.install), "Installing");
    expect_eq(pmStagePlainName(PmStage.save_lockfile), "Saving lockfile");
    expect_eq(pmStagePlainName(PmStage.run_script), "Running script");

    // Non-ANSI labels match Bun ProgressStrings.cp exact newline behavior
    expect_eq(pmStageLabel(PmStage.resolve, false), "Resolving\n");
    expect_eq(pmStageLabel(PmStage.extract, false), "Resolving & extracting\n");
    expect_eq(pmStageLabel(PmStage.install, false), "Installing\n");
    expect_eq(pmStageLabel(PmStage.save_lockfile, false), "Saving lockfile");
    expect_eq(pmStageLabel(PmStage.run_script, false), "Running script\n");

    // ANSI labels carry emoji prefix and are strictly longer than plain names
    const ansi_res = pmStageLabel(PmStage.resolve, true);
    const ansi_inst = pmStageLabel(PmStage.install, true);
    expect_true(ansi_res.len > pmStagePlainName(PmStage.resolve).len);
    expect_true(ansi_inst.len > pmStagePlainName(PmStage.install).len);
});

test("cli.pm_commands: formatPmStageBanner zero-alloc formatting with and without detail", () => {
    let buf: [128]u8 = [0; 128];

    const b1 = formatPmStageBanner(buf[0..], PmStage.install, "lodash@4.17.21", false) catch {
        expect_true(false);
        return;
    };
    expect_eq(b1, "Installing lodash@4.17.21");

    const b2 = formatPmStageBanner(buf[0..], PmStage.save_lockfile, "", false) catch {
        expect_true(false);
        return;
    };
    expect_eq(b2, "Saving lockfile");

    const b3 = formatPmStageBanner(buf[0..], PmStage.resolve, "128 packages", true) catch {
        expect_true(false);
        return;
    };
    expect_true(b3.len > 15);
});

test("cli.pm_commands: BufferTooSmall guard on formatPmStageBanner", () => {
    let tiny: [5]u8 = [0; 5];
    let failed_plain = false;
    * = formatPmStageBanner(tiny[0..], PmStage.install, "pkg", false) catch {
        failed_plain = true;
    };
    expect_true(failed_plain);

    let failed_ansi = false;
    * = formatPmStageBanner(tiny[0..], PmStage.resolve, "", true) catch {
        failed_ansi = true;
    };
    expect_true(failed_ansi);
});
