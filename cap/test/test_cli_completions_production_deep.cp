// Production Deep Tests for cap:cli Shell Completions (lib/cli/completions.cp)
import { test, expect_eq } from "cap:test";
import {
    CompletionError,
    CompletionShell,
    CompletionItem,
    formatCompletions,
    formatCompletionItems,
} from "cap:cli";

test("cli completions: CompletionShell.fromShellPath across POSIX, Windows .exe and unknown", fn() {
    expect_eq(CompletionShell.fromShellPath("/bin/bash"), CompletionShell.bash);
    expect_eq(CompletionShell.fromShellPath("/usr/bin/zsh"), CompletionShell.zsh);
    expect_eq(CompletionShell.fromShellPath("/opt/homebrew/bin/fish/"), CompletionShell.fish);
    expect_eq(CompletionShell.fromShellPath("C:\\Program Files\\PowerShell\\7\\pwsh.exe"), CompletionShell.pwsh);
    expect_eq(CompletionShell.fromShellPath("POWERSHELL.EXE"), CompletionShell.pwsh);
    expect_eq(CompletionShell.fromShellPath("/bin/sh"), CompletionShell.unknown);
    expect_eq(CompletionShell.fromShellPath(""), CompletionShell.unknown);

    expect_eq(CompletionShell.bash.name(), "bash");
    expect_eq(CompletionShell.fish.name(), "fish");
    expect_eq(CompletionShell.pwsh.name(), "pwsh");
    expect_eq(CompletionShell.unknown.name(), "unknown");
    expect_eq(CompletionShell.fish.delimiter(), " ");
    expect_eq(CompletionShell.zsh.delimiter(), "\n");
});

test("cli completions: formatCompletions newline (bash/zsh) vs space (fish) delimiters", fn() {
    const cmds: [3]([]const u8) = ["build", "test", "run"];
    const no_desc: [0]([]const u8) = [];
    let buf: [64]u8 = [0; 64];

    const n_bash = formatCompletions(CompletionShell.bash, cmds[0..], no_desc[0..], buf[0..]) catch unreachable;
    expect_eq(buf[0..n_bash], "build\ntest\nrun");

    const n_fish = formatCompletions(CompletionShell.fish, cmds[0..], no_desc[0..], buf[0..]) catch unreachable;
    expect_eq(buf[0..n_fish], "build test run");
});

test("cli completions: formatCompletions with tab-separated and partial descriptions", fn() {
    const cmds: [3]([]const u8) = ["init", "add", "clean"];
    const descs: [2]([]const u8) = ["Create project", ""];
    let buf: [64]u8 = [0; 64];

    const n = formatCompletions(CompletionShell.zsh, cmds[0..], descs[0..], buf[0..]) catch unreachable;
    // "init\tCreate project\nadd\nclean" (empty/missing descriptions omit '\t')
    expect_eq(buf[0..n], "init\tCreate project\nadd\nclean");
});

test("cli completions: formatCompletionItems prefix filtering", fn() {
    const items: [4]CompletionItem = [
        CompletionItem{ command: "build", description: "Compile binary" },
        CompletionItem{ command: "bench", description: "Run benchmarks" },
        CompletionItem{ command: "test", description: "Run tests" },
        CompletionItem{ command: "fmt", description: "" },
    ];
    let buf: [128]u8 = [0; 128];

    // Prefix "b" matches build + bench
    const n1 = formatCompletionItems(CompletionShell.zsh, items[0..], "b", buf[0..]) catch unreachable;
    expect_eq(buf[0..n1], "build\tCompile binary\nbench\tRun benchmarks");

    // Prefix "bu" matches only build
    const n2 = formatCompletionItems(CompletionShell.fish, items[0..], "bu", buf[0..]) catch unreachable;
    expect_eq(buf[0..n2], "build\tCompile binary");

    // Unmatched prefix returns 0 bytes
    const n3 = formatCompletionItems(CompletionShell.bash, items[0..], "zzz", buf[0..]) catch unreachable;
    expect_eq(n3, @as(usize, 0));
});

test("cli completions: empty list and BufferTooSmall guards", fn() {
    const empty_cmds: [0]([]const u8) = [];
    let buf: [8]u8 = [0; 8];
    expect_eq(formatCompletions(CompletionShell.bash, empty_cmds[0..], empty_cmds[0..], buf[0..]) catch unreachable, @as(usize, 0));

    const cmds: [2]([]const u8) = ["install", "uninstall"];
    let got_small = false;
    * = formatCompletions(CompletionShell.bash, cmds[0..], empty_cmds[0..], buf[0..]) catch |e| {
        got_small = (e == CompletionError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(got_small, true);
});
