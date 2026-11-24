// Source: Absorbed from /data/dev/bun/src/options_types/CommandTag.cp | Synced: 2026-11-23T22:02:17Z
// Pure-CP zero-allocation top-level CLI subcommand discriminant (`CliCommandTag`),
// bun.report crash-report character codes, canonical name/alias lookup, and
// configuration-loading / global-option / NPM-domain capability predicates.

pub const CLI_COMMAND_TAG_COUNT: usize = 32;

pub struct CliCommandCapabilities {
    read_global_config: bool = false,
    is_npm_related: bool = false,
    loads_config: bool = false,
    always_loads_config: bool = false,
    uses_global_options: bool = true,
    crash_report_char: u8 = 'a',
}

/// Top-level CLI subcommand discriminant mirroring all 32 variants of Bun `Command.Tag`.
pub enum CliCommandTag: u8 {
    AddCommand = 0,
    AutoCommand = 1,
    BuildCommand = 2,
    BunxCommand = 3,
    CreateCommand = 4,
    DiscordCommand = 5,
    GetCompletionsCommand = 6,
    HelpCommand = 7,
    InitCommand = 8,
    InfoCommand = 9,
    InstallCommand = 10,
    InstallCompletionsCommand = 11,
    LinkCommand = 12,
    PackageManagerCommand = 13,
    RemoveCommand = 14,
    RunCommand = 15,
    RunAsNodeCommand = 16,
    TestCommand = 17,
    UnlinkCommand = 18,
    UpdateCommand = 19,
    UpgradeCommand = 20,
    ReplCommand = 21,
    ReservedCommand = 22,
    ExecCommand = 23,
    PatchCommand = 24,
    PatchCommitCommand = 25,
    OutdatedCommand = 26,
    UpdateInteractiveCommand = 27,
    PublishCommand = 28,
    AuditCommand = 29,
    WhyCommand = 30,
    FuzzilliCommand = 31,

    /// Single-character crash-report code kept in exact parity with Bun `Command.Tag.char()`.
    fn crashReportChar(): u8 {
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

    /// Canonical CLI subcommand name.
    fn canonicalName(): []const u8 {
        return switch (_) {
            .AddCommand => "add",
            .AutoCommand => "auto",
            .BuildCommand => "build",
            .BunxCommand => "x",
            .CreateCommand => "create",
            .DiscordCommand => "discord",
            .GetCompletionsCommand => "getcompletes",
            .HelpCommand => "help",
            .InitCommand => "init",
            .InfoCommand => "info",
            .InstallCommand => "install",
            .InstallCompletionsCommand => "completions",
            .LinkCommand => "link",
            .PackageManagerCommand => "pm",
            .RemoveCommand => "remove",
            .RunCommand => "run",
            .RunAsNodeCommand => "node",
            .TestCommand => "test",
            .UnlinkCommand => "unlink",
            .UpdateCommand => "update",
            .UpgradeCommand => "upgrade",
            .ReplCommand => "repl",
            .ReservedCommand => "reserved",
            .ExecCommand => "exec",
            .PatchCommand => "patch",
            .PatchCommitCommand => "patch-commit",
            .OutdatedCommand => "outdated",
            .UpdateInteractiveCommand => "update-interactive",
            .PublishCommand => "publish",
            .AuditCommand => "audit",
            .WhyCommand => "why",
            .FuzzilliCommand => "fuzzilli",
        };
    }

    /// Parses a CLI subcommand name or common shorthand alias into `CliCommandTag`.
    static fn fromName(cmd_name: []const u8): CliCommandTag? {
        if (cmd_name == "add" || cmd_name == "a") return .AddCommand;
        if (cmd_name == "build") return .BuildCommand;
        if (cmd_name == "x" || cmd_name == "bunx") return .BunxCommand;
        if (cmd_name == "create" || cmd_name == "c") return .CreateCommand;
        if (cmd_name == "discord") return .DiscordCommand;
        if (cmd_name == "getcompletes") return .GetCompletionsCommand;
        if (cmd_name == "help" || cmd_name == "--help" || cmd_name == "-h") return .HelpCommand;
        if (cmd_name == "init") return .InitCommand;
        if (cmd_name == "info") return .InfoCommand;
        if (cmd_name == "install" || cmd_name == "i") return .InstallCommand;
        if (cmd_name == "completions") return .InstallCompletionsCommand;
        if (cmd_name == "link") return .LinkCommand;
        if (cmd_name == "pm") return .PackageManagerCommand;
        if (cmd_name == "remove" || cmd_name == "rm" || cmd_name == "uninstall") return .RemoveCommand;
        if (cmd_name == "run") return .RunCommand;
        if (cmd_name == "node") return .RunAsNodeCommand;
        if (cmd_name == "test") return .TestCommand;
        if (cmd_name == "unlink") return .UnlinkCommand;
        if (cmd_name == "update") return .UpdateCommand;
        if (cmd_name == "upgrade") return .UpgradeCommand;
        if (cmd_name == "repl") return .ReplCommand;
        if (cmd_name == "exec") return .ExecCommand;
        if (cmd_name == "patch") return .PatchCommand;
        if (cmd_name == "patch-commit") return .PatchCommitCommand;
        if (cmd_name == "outdated") return .OutdatedCommand;
        if (cmd_name == "publish") return .PublishCommand;
        if (cmd_name == "audit") return .AuditCommand;
        if (cmd_name == "why") return .WhyCommand;
        if (cmd_name == "fuzzilli") return .FuzzilliCommand;
        return null;
    }

    /// True if this command reads global bunfig/npmrc configuration (mirrors Bun `readGlobalConfig`).
    fn readGlobalConfig(): bool {
        return switch (_) {
            .BunxCommand,
            .PackageManagerCommand,
            .InstallCommand,
            .AddCommand,
            .RemoveCommand,
            .UpdateCommand,
            .PatchCommand,
            .PatchCommitCommand,
            .OutdatedCommand,
            .PublishCommand,
            .AuditCommand => true,
            else => false,
        };
    }

    /// True if this command belongs to the package-manager / NPM ecosystem (mirrors Bun `isNPMRelated`).
    fn isNpmRelated(): bool {
        return switch (_) {
            .BunxCommand,
            .LinkCommand,
            .UnlinkCommand,
            .PackageManagerCommand,
            .InstallCommand,
            .AddCommand,
            .RemoveCommand,
            .UpdateCommand,
            .PatchCommand,
            .PatchCommitCommand,
            .OutdatedCommand,
            .PublishCommand,
            .AuditCommand => true,
            else => false,
        };
    }

    /// True if this command may load `bunfig.toml` / project config (mirrors Bun `loads_config`).
    fn loadsConfig(): bool {
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
            .AuditCommand => true,
            else => false,
        };
    }

    /// True if this command unconditionally loads project config before execution (mirrors Bun `always_loads_config`).
    fn alwaysLoadsConfig(): bool {
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
            .AuditCommand => true,
            else => false,
        };
    }

    /// True if this command accepts global runtime CLI options (mirrors Bun `uses_global_options`).
    /// Package-manager subcommands opt out so their flags don't collide with runtime flags.
    fn usesGlobalOptions(): bool {
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
            .UpdateCommand => false,
            else => true,
        };
    }

    /// Returns a complete capability summary snapshot for this command tag.
    fn capabilities(): CliCommandCapabilities {
        return CliCommandCapabilities{
            read_global_config: _.readGlobalConfig(),
            is_npm_related: _.isNpmRelated(),
            loads_config: _.loadsConfig(),
            always_loads_config: _.alwaysLoadsConfig(),
            uses_global_options: _.usesGlobalOptions(),
            crash_report_char: _.crashReportChar(),
        };
    }
}
