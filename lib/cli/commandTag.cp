// Source: absorbed from bun/src/options_types/CommandTag.cp (cap:command_tag)
const std = @import("std");

pub const Tag = enum {
    AddCommand,
    AutoCommand,
    BuildCommand,
    BunxCommand,
    CreateCommand,
    DiscordCommand,
    GetCompletionsCommand,
    HelpCommand,
    InitCommand,
    InfoCommand,
    InstallCommand,
    InstallCompletionsCommand,
    LinkCommand,
    PackageManagerCommand,
    RemoveCommand,
    RunCommand,
    RunAsNodeCommand,
    TestCommand,
    UnlinkCommand,
    UpdateCommand,
    UpgradeCommand,
    ReplCommand,
    ReservedCommand,
    ExecCommand,
    PatchCommand,
    PatchCommitCommand,
    OutdatedCommand,
    UpdateInteractiveCommand,
    PublishCommand,
    AuditCommand,
    WhyCommand,
    FuzzilliCommand,

    pub fn char(): u8 {
        return switch (_) {
            .AddCommand => 'I',
            .AutoCommand => 'a',
            .BuildCommand => 'b',
            .BunxCommand => 'B',
            .CreateCommand => 'c',
            .DiscordCommand => 'D',
            .GetCompletionsCommand => 'g',
            .HelpCommand => 'h',
            .InitCommand => 'j',
            .InfoCommand => 'v',
            .InstallCommand => 'i',
            .InstallCompletionsCommand => 'C',
            .LinkCommand => 'l',
            .PackageManagerCommand => 'P',
            .RemoveCommand => 'R',
            .RunCommand => 'r',
            .RunAsNodeCommand => 'n',
            .TestCommand => 't',
            .UnlinkCommand => 'U',
            .UpdateCommand => 'u',
            .UpgradeCommand => 'p',
            .ReplCommand => 'G',
            .ReservedCommand => 'w',
            .ExecCommand => 'e',
            .PatchCommand => 'x',
            .PatchCommitCommand => 'z',
            .OutdatedCommand => 'o',
            .UpdateInteractiveCommand => 'U',
            .PublishCommand => 'k',
            .AuditCommand => 'A',
            .WhyCommand => 'W',
            .FuzzilliCommand => 'F',
        };
    }

    pub fn readGlobalConfig(): bool {
        return switch (_) {
            .BunxCommand, .PackageManagerCommand, .InstallCommand, .AddCommand, .RemoveCommand, .UpdateCommand, .PatchCommand, .PatchCommitCommand, .OutdatedCommand, .PublishCommand, .AuditCommand => true,
            else => false,
        };
    }

    pub fn isNPMRelated(): bool {
        return switch (_) {
            .BunxCommand, .LinkCommand, .UnlinkCommand, .PackageManagerCommand, .InstallCommand, .AddCommand, .RemoveCommand, .UpdateCommand, .PatchCommand, .PatchCommitCommand, .OutdatedCommand, .PublishCommand, .AuditCommand => true,
            else => false,
        };
    }

    pub fn loadsConfig(): bool {
        return switch (_) {
            .BuildCommand,
            .TestCommand,
            .InstallCommand,
            .AddCommand,
            .RemoveCommand,
            .UpdateCommand,
            .PatchCommand,
            .PatchCommitCommand,
            .PackageManagerCommand,
            .BunxCommand,
            .AutoCommand,
            .RunCommand,
            .RunAsNodeCommand,
            .OutdatedCommand,
            .UpdateInteractiveCommand,
            .PublishCommand,
            .AuditCommand,
            => true,
            else => false,
        };
    }

    pub fn alwaysLoadsConfig(): bool {
        return switch (_) {
            .BuildCommand,
            .TestCommand,
            .InstallCommand,
            .AddCommand,
            .RemoveCommand,
            .UpdateCommand,
            .PatchCommand,
            .PatchCommitCommand,
            .PackageManagerCommand,
            .BunxCommand,
            .OutdatedCommand,
            .UpdateInteractiveCommand,
            .PublishCommand,
            .AuditCommand,
            => true,
            else => false,
        };
    }

    pub fn usesGlobalOptions(): bool {
        return switch (_) {
            .AddCommand,
            .AuditCommand,
            .BunxCommand,
            .CreateCommand,
            .InfoCommand,
            .InstallCommand,
            .LinkCommand,
            .OutdatedCommand,
            .UpdateInteractiveCommand,
            .PackageManagerCommand,
            .PatchCommand,
            .PatchCommitCommand,
            .PublishCommand,
            .RemoveCommand,
            .UnlinkCommand,
            .UpdateCommand,
            => false,
            else => true,
        };
    }
};

pub fn fromSubcommand(name: []const u8): Tag? {
    return std.StaticStringMap(Tag).initComptime(.{
        .{ "add", .AddCommand },
        .{ "build", .BuildCommand },
        .{ "x", .BunxCommand },
        .{ "create", .CreateCommand },
        .{ "help", .HelpCommand },
        .{ "init", .InitCommand },
        .{ "info", .InfoCommand },
        .{ "install", .InstallCommand },
        .{ "i", .InstallCommand },
        .{ "link", .LinkCommand },
        .{ "pm", .PackageManagerCommand },
        .{ "remove", .RemoveCommand },
        .{ "rm", .RemoveCommand },
        .{ "run", .RunCommand },
        .{ "test", .TestCommand },
        .{ "unlink", .UnlinkCommand },
        .{ "update", .UpdateCommand },
        .{ "upgrade", .UpgradeCommand },
        .{ "repl", .ReplCommand },
        .{ "exec", .ExecCommand },
        .{ "patch", .PatchCommand },
        .{ "patch-commit", .PatchCommitCommand },
        .{ "outdated", .OutdatedCommand },
        .{ "publish", .PublishCommand },
        .{ "audit", .AuditCommand },
        .{ "why", .WhyCommand },
    }).get(name);
}
