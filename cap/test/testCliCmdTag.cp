// Origin: Pure-CP Stdlib Test Suite (lib/cli/cmdTag.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Top-Level CLI CommandTag:
//   1. fromName canonical & shorthand alias resolution + canonicalName roundtrip
//   2. crashReportChar parity with bun.report across subcommand families
//   3. readGlobalConfig vs isNpmRelated classification (Link/Unlink boundary)
//   4. loadsConfig vs alwaysLoadsConfig semantics (Run/Auto/Node vs Build/Test/Install)
//   5. usesGlobalOptions isolation & capabilities() snapshot consistency

import { test, expect_eq, expect_true } from "cap:test";
import {
    CLI_COMMAND_TAG_COUNT,
    CliCommandTag,
} from "cap:cli";

test("CliCommandTag: fromName aliases & canonicalName roundtrip", fn() {
    expect_eq(CLI_COMMAND_TAG_COUNT, 32);

    if (t := CliCommandTag.fromName("i")) {
        expect_eq(t, CliCommandTag.InstallCommand);
        expect_eq(t.canonicalName(), "install");
    } else {
        expect_true(false);
    }

    if (t := CliCommandTag.fromName("a")) {
        expect_eq(t, CliCommandTag.AddCommand);
    } else {
        expect_true(false);
    }

    if (t := CliCommandTag.fromName("rm")) {
        expect_eq(t, CliCommandTag.RemoveCommand);
    } else {
        expect_true(false);
    }

    if (t := CliCommandTag.fromName("uninstall")) {
        expect_eq(t, CliCommandTag.RemoveCommand);
    } else {
        expect_true(false);
    }

    if (t := CliCommandTag.fromName("bunx")) {
        expect_eq(t, CliCommandTag.BunxCommand);
        expect_eq(t.canonicalName(), "x");
    } else {
        expect_true(false);
    }

    if (t := CliCommandTag.fromName("--help")) {
        expect_eq(t, CliCommandTag.HelpCommand);
    } else {
        expect_true(false);
    }

    if (t := CliCommandTag.fromName("patch-commit")) {
        expect_eq(t, CliCommandTag.PatchCommitCommand);
    } else {
        expect_true(false);
    }

    expect_true(CliCommandTag.fromName("not-a-command") == null);
    expect_true(CliCommandTag.fromName("") == null);
});

test("CliCommandTag: crashReportChar parity with bun.report", fn() {
    expect_eq(CliCommandTag.AddCommand.crashReportChar(), 'I');
    expect_eq(CliCommandTag.AutoCommand.crashReportChar(), 'a');
    expect_eq(CliCommandTag.BuildCommand.crashReportChar(), 'b');
    expect_eq(CliCommandTag.BunxCommand.crashReportChar(), 'B');
    expect_eq(CliCommandTag.InstallCommand.crashReportChar(), 'i');
    expect_eq(CliCommandTag.PackageManagerCommand.crashReportChar(), 'P');
    expect_eq(CliCommandTag.RunCommand.crashReportChar(), 'r');
    expect_eq(CliCommandTag.RunAsNodeCommand.crashReportChar(), 'n');
    expect_eq(CliCommandTag.TestCommand.crashReportChar(), 't');
    expect_eq(CliCommandTag.PatchCommand.crashReportChar(), 'x');
    expect_eq(CliCommandTag.PatchCommitCommand.crashReportChar(), 'z');
    expect_eq(CliCommandTag.PublishCommand.crashReportChar(), 'k');
    expect_eq(CliCommandTag.AuditCommand.crashReportChar(), 'A');
    expect_eq(CliCommandTag.WhyCommand.crashReportChar(), 'W');
    expect_eq(CliCommandTag.FuzzilliCommand.crashReportChar(), 'F');
});

test("CliCommandTag: readGlobalConfig vs isNpmRelated boundaries", fn() {
    expect_true(CliCommandTag.InstallCommand.readGlobalConfig());
    expect_true(CliCommandTag.InstallCommand.isNpmRelated());
    expect_true(CliCommandTag.AuditCommand.readGlobalConfig());
    expect_true(CliCommandTag.AuditCommand.isNpmRelated());

    // Link / Unlink are NPM-related but do NOT read global config
    expect_true(CliCommandTag.LinkCommand.isNpmRelated());
    expect_true(!CliCommandTag.LinkCommand.readGlobalConfig());
    expect_true(CliCommandTag.UnlinkCommand.isNpmRelated());
    expect_true(!CliCommandTag.UnlinkCommand.readGlobalConfig());

    // Runtime & bundler commands are neither
    expect_true(!CliCommandTag.BuildCommand.isNpmRelated());
    expect_true(!CliCommandTag.TestCommand.readGlobalConfig());
    expect_true(!CliCommandTag.RunCommand.isNpmRelated());
});

test("CliCommandTag: loadsConfig vs alwaysLoadsConfig semantics", fn() {
    // Unconditional config loaders
    expect_true(CliCommandTag.BuildCommand.loadsConfig());
    expect_true(CliCommandTag.BuildCommand.alwaysLoadsConfig());
    expect_true(CliCommandTag.TestCommand.loadsConfig());
    expect_true(CliCommandTag.TestCommand.alwaysLoadsConfig());
    expect_true(CliCommandTag.PublishCommand.loadsConfig());
    expect_true(CliCommandTag.PublishCommand.alwaysLoadsConfig());

    // Conditional config loaders: loadsConfig == true, alwaysLoadsConfig == false
    expect_true(CliCommandTag.RunCommand.loadsConfig());
    expect_true(!CliCommandTag.RunCommand.alwaysLoadsConfig());
    expect_true(CliCommandTag.AutoCommand.loadsConfig());
    expect_true(!CliCommandTag.AutoCommand.alwaysLoadsConfig());
    expect_true(CliCommandTag.RunAsNodeCommand.loadsConfig());
    expect_true(!CliCommandTag.RunAsNodeCommand.alwaysLoadsConfig());

    // Non-config commands
    expect_true(!CliCommandTag.HelpCommand.loadsConfig());
    expect_true(!CliCommandTag.InitCommand.loadsConfig());
    expect_true(!CliCommandTag.WhyCommand.loadsConfig());
});

test("CliCommandTag: usesGlobalOptions isolation & capabilities snapshot", fn() {
    // Runtime / build / test commands accept global options
    expect_true(CliCommandTag.RunCommand.usesGlobalOptions());
    expect_true(CliCommandTag.BuildCommand.usesGlobalOptions());
    expect_true(CliCommandTag.TestCommand.usesGlobalOptions());
    expect_true(CliCommandTag.WhyCommand.usesGlobalOptions());

    // Package-manager commands opt out of global runtime options
    expect_true(!CliCommandTag.InstallCommand.usesGlobalOptions());
    expect_true(!CliCommandTag.AddCommand.usesGlobalOptions());
    expect_true(!CliCommandTag.RemoveCommand.usesGlobalOptions());
    expect_true(!CliCommandTag.PackageManagerCommand.usesGlobalOptions());
    expect_true(!CliCommandTag.AuditCommand.usesGlobalOptions());

    const caps = CliCommandTag.InstallCommand.capabilities();
    expect_true(caps.read_global_config);
    expect_true(caps.is_npm_related);
    expect_true(caps.loads_config);
    expect_true(caps.always_loads_config);
    expect_true(!caps.uses_global_options);
    expect_eq(caps.crash_report_char, 'i');
});
