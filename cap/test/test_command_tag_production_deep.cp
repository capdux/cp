// Source: cp/cap/test/test_command_tag_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { Tag, fromSubcommand } from "cap:command_tag";

test "command_tag: subcommand lookup & telemetry char" {
    const inst = fromSubcommand("install").?;
    const alias_i = fromSubcommand("i").?;
    try expect_eq(inst == alias_i, true);
    try expect_eq(inst.char(), 'i');
    try expect_eq(Tag.BuildCommand.char(), 'b');
    try expect_eq(Tag.TestCommand.char(), 't');
    try expect_eq(fromSubcommand("nonexistent") == null, true);
}

test "command_tag: NPM & global config predicates" {
    try expect_eq(Tag.InstallCommand.isNPMRelated(), true);
    try expect_eq(Tag.AddCommand.readGlobalConfig(), true);
    try expect_eq(Tag.AuditCommand.isNPMRelated(), true);
    try expect_eq(Tag.BuildCommand.isNPMRelated(), false);
    try expect_eq(Tag.HelpCommand.readGlobalConfig(), false);
}

test "command_tag: loadsConfig vs alwaysLoadsConfig & usesGlobalOptions" {
    try expect_eq(Tag.RunCommand.loadsConfig(), true);
    try expect_eq(Tag.RunCommand.alwaysLoadsConfig(), false);
    try expect_eq(Tag.BuildCommand.alwaysLoadsConfig(), true);
    try expect_eq(Tag.RunCommand.usesGlobalOptions(), true);
    try expect_eq(Tag.InstallCommand.usesGlobalOptions(), false);
}

test "command_tag: cap.command_tag dynamic mount" {
    const t = cap.command_tag.fromSubcommand("publish").?;
    try expect_eq(t.char(), 'k');
    try expect_eq(t.isNPMRelated(), true);
}
