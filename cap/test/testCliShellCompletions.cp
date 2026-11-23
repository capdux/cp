import { test, assert, assertEq } from "cap:test";
import {
    SHELL_COMP_MARKER_BEGIN,
    SHELL_COMP_MARKER_END,
    SHELL_COMP_NO_SUBCOMMAND,
    ShellCompError,
    ShellCompDialect,
    parseShellCompDialect,
    shellCompDialectName,
    ShellCompArgHint,
    ShellCompFlagRule,
    ShellCompCommandRule,
    generateCapShellCompletion,
    ShellCompCandidateKind,
    ShellCompCandidate,
    ShellCompProjectContext,
    computeDynamicCompletions,
    RcCompOperation,
    RcCompAction,
    planRcCompletionInstall,
} from "cap:cli";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("parseShellCompDialect and generateCapShellCompletion emit valid scripts for Bash, Zsh, Fish, PowerShell", () => {
    assert(parseShellCompDialect("BASH") == ShellCompDialect.Bash);
    assert(parseShellCompDialect("zsh") == ShellCompDialect.Zsh);
    assert(parseShellCompDialect("Fish") == ShellCompDialect.Fish);
    assert(parseShellCompDialect("pwsh") == ShellCompDialect.PowerShell);
    assert(parseShellCompDialect("PowerShell") == ShellCompDialect.PowerShell);
    assert(parseShellCompDialect("csh") == null);
    assert(strEql(shellCompDialectName(ShellCompDialect.PowerShell), "powershell"));

    const flags: [2]ShellCompFlagRule = [
        ShellCompFlagRule{
            long_name: "--target",
            short_name: "-t",
            description: "Target triple",
            arg_hint: ShellCompArgHint.EnumValues,
            enum_values_csv: "native,linux-x64,macos-arm64",
        },
        ShellCompFlagRule{
            long_name: "--minify",
            short_name: "-m",
            description: "Minify bundle",
            arg_hint: ShellCompArgHint.None,
        },
    ];
    const cmds: [2]ShellCompCommandRule = [
        ShellCompCommandRule{ name: "build", description: "Build project", arg_hint: ShellCompArgHint.FilePath, file_ext_filter: ".cp", flags_start: 0, flags_len: 2 },
        ShellCompCommandRule{ name: "run", description: "Run script", arg_hint: ShellCompArgHint.ScriptName, flags_start: 0, flags_len: 0 },
    ];

    let out: [4096]u8 = [0; 4096];

    const bash_n = generateCapShellCompletion(ShellCompDialect.Bash, "cap", cmds[0..], flags[0..], out[0..]) catch unreachable;
    assert(strContains(out[0..bash_n], "complete -F _cap_completions cap"));
    assert(strContains(out[0..bash_n], "--target -t --minify -m"));

    const zsh_n = generateCapShellCompletion(ShellCompDialect.Zsh, "cap", cmds[0..], flags[0..], out[0..]) catch unreachable;
    assert(strContains(out[0..zsh_n], "#compdef cap"));
    assert(strContains(out[0..zsh_n], "(native linux-x64 macos-arm64)"));

    const fish_n = generateCapShellCompletion(ShellCompDialect.Fish, "cap", cmds[0..], flags[0..], out[0..]) catch unreachable;
    assert(strContains(out[0..fish_n], "__fish_use_subcommand"));
    assert(strContains(out[0..fish_n], "-l target -s t -r -a 'native linux-x64 macos-arm64'"));

    const ps_n = generateCapShellCompletion(ShellCompDialect.PowerShell, "cap", cmds[0..], flags[0..], out[0..]) catch unreachable;
    assert(strContains(out[0..ps_n], "Register-ArgumentCompleter -Native -CommandName cap"));
    assert(strContains(out[0..ps_n], "CompletionResult"));
});

test("computeDynamicCompletions completes top-level subcommands with SIMD case-folding and ranking", () => {
    const flags: [0]ShellCompFlagRule = [];
    const cmds: [4]ShellCompCommandRule = [
        ShellCompCommandRule{ name: "build", description: "Build bundle" },
        ShellCompCommandRule{ name: "bench", description: "Run benchmarks" },
        ShellCompCommandRule{ name: "run", description: "Run script" },
        ShellCompCommandRule{ name: "bundle-analyze", description: "Analyze bundle" },
    ];
    const ctx = ShellCompProjectContext{};
    let cands: [8]ShellCompCandidate = [ShellCompCandidate.empty(); 8];

    // Case-insensitive prefix "BU" matches "build" and "bundle-analyze"
    const toks: [2][]const u8 = ["cap", "BU"];
    const sum = computeDynamicCompletions(toks[0..], 1, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
    assertEq(sum.active_subcommand_index, SHELL_COMP_NO_SUBCOMMAND);
    assertEq(sum.candidate_count, @as(usize, 2));
    assert(strEql(cands[0].value, "build"));
    assert(strEql(cands[1].value, "bundle-analyze"));
    assertEq(cands[0].score, @as(u32, 80));
});

test("computeDynamicCompletions completes flags, inline --flag=enum, scripts, packages, and filtered files", () => {
    const flags: [2]ShellCompFlagRule = [
        ShellCompFlagRule{
            long_name: "--target",
            short_name: "-t",
            description: "Target",
            arg_hint: ShellCompArgHint.EnumValues,
            enum_values_csv: "native,linux-x64,macos-arm64,windows-x64",
        },
        ShellCompFlagRule{ long_name: "--minify", short_name: "-m", description: "Minify" },
    ];
    const cmds: [3]ShellCompCommandRule = [
        ShellCompCommandRule{ name: "build", description: "Build", arg_hint: ShellCompArgHint.FilePath, file_ext_filter: ".cp,.ts", flags_start: 0, flags_len: 2 },
        ShellCompCommandRule{ name: "run", description: "Run", arg_hint: ShellCompArgHint.ScriptName },
        ShellCompCommandRule{ name: "add", description: "Add", arg_hint: ShellCompArgHint.PackageName },
    ];
    const scripts: [3][]const u8 = ["dev", "build:prod", "deploy"];
    const pkgs: [3][]const u8 = ["zod", "zig-utils", "react"];
    const files: [3][]const u8 = ["src/main.cp", "src/app.ts", "README.md"];
    const ctx = ShellCompProjectContext{ scripts: scripts[0..], packages: pkgs[0..], files: files[0..] };

    let cands: [8]ShellCompCandidate = [ShellCompCandidate.empty(); 8];

    // 1. Flag enum value after `--target`: `cap build --target mac`
    const t1: [4][]const u8 = ["cap", "build", "--target", "mac"];
    const s1 = computeDynamicCompletions(t1[0..], 3, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
    assertEq(s1.candidate_count, @as(usize, 1));
    assert(cands[0].kind == ShellCompCandidateKind.EnumValue && strEql(cands[0].value, "macos-arm64"));

    // 2. Inline `--target=lin`: `cap build --target=lin`
    const t2: [3][]const u8 = ["cap", "build", "--target=lin"];
    const s2 = computeDynamicCompletions(t2[0..], 2, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
    assertEq(s2.candidate_count, @as(usize, 1));
    assert(strEql(cands[0].value, "linux-x64"));

    // 3. Script completion: `cap run d` -> dev, deploy (alphabetical tie-break)
    const t3: [3][]const u8 = ["cap", "run", "d"];
    const s3 = computeDynamicCompletions(t3[0..], 2, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
    assertEq(s3.candidate_count, @as(usize, 2));
    assert(strEql(cands[0].value, "deploy"));
    assert(strEql(cands[1].value, "dev"));

    // 4. Package completion: `cap add z` -> zig-utils, zod
    const t4: [3][]const u8 = ["cap", "add", "z"];
    const s4 = computeDynamicCompletions(t4[0..], 2, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
    assertEq(s4.candidate_count, @as(usize, 2));
    assert(cands[0].kind == ShellCompCandidateKind.Package);

    // 5. File completion with extension filter `.cp,.ts` excludes README.md
    const t5: [3][]const u8 = ["cap", "build", ""];
    const s5 = computeDynamicCompletions(t5[0..], 2, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
    assertEq(s5.candidate_count, @as(usize, 2));
    assert(strEql(cands[0].value, "src/app.ts"));
    assert(strEql(cands[1].value, "src/main.cp"));
});

test("planRcCompletionInstall is strictly idempotent for append, no-change, replace, and uninstall", () => {
    const initial_rc = "export PATH=\"$HOME/.cap/bin:$PATH\"\n";
    let buf1: [1024]u8 = [0; 1024];
    let buf2: [1024]u8 = [0; 1024];
    let buf3: [1024]u8 = [0; 1024];
    let buf4: [1024]u8 = [0; 1024];

    // 1. First install appends block
    const r1 = planRcCompletionInstall(ShellCompDialect.Zsh, "cap", initial_rc, RcCompOperation.InstallOrUpdate, buf1[0..]) catch unreachable;
    assert(r1.action == RcCompAction.AppendBlock);
    assert(!r1.existing_block_found);
    assert(strEql(r1.rc_path_hint, "~/.zshrc"));
    const installed_rc = buf1[0..r1.bytes_written];
    assert(strContains(installed_rc, SHELL_COMP_MARKER_BEGIN));
    assert(strContains(installed_rc, "eval \"$(cap completions zsh)\""));
    assert(strContains(installed_rc, SHELL_COMP_MARKER_END));

    // 2. Second install on already-installed RC is a NoChange (idempotent!)
    const r2 = planRcCompletionInstall(ShellCompDialect.Zsh, "cap", installed_rc, RcCompOperation.InstallOrUpdate, buf2[0..]) catch unreachable;
    assert(r2.action == RcCompAction.NoChange);
    assert(r2.existing_block_found);
    assert(strEql(buf2[0..r2.bytes_written], installed_rc));

    // 3. Updating an outdated block replaces in-place without duplication
    const stale_rc = "export FOO=1\n# >>> cap completions >>>\n# old line\n# <<< cap completions <<<\nexport BAR=2\n";
    const r3 = planRcCompletionInstall(ShellCompDialect.Fish, "cap", stale_rc, RcCompOperation.InstallOrUpdate, buf3[0..]) catch unreachable;
    assert(r3.action == RcCompAction.ReplaceBlock);
    assert(r3.existing_block_found);
    const updated_rc = buf3[0..r3.bytes_written];
    assert(strContains(updated_rc, "export FOO=1\n"));
    assert(strContains(updated_rc, "cap completions fish | source\n"));
    assert(strContains(updated_rc, "export BAR=2\n"));
    assert(!strContains(updated_rc, "# old line"));

    // 4. Uninstall strips block cleanly back to original RC
    const r4 = planRcCompletionInstall(ShellCompDialect.Zsh, "cap", installed_rc, RcCompOperation.Uninstall, buf4[0..]) catch unreachable;
    assert(r4.action == RcCompAction.RemoveBlock);
    assert(strEql(buf4[0..r4.bytes_written], initial_rc));
});

test("planRcCompletionInstall handles RC without trailing newline and uninstall when absent", () => {
    const rc_no_nl = "alias ll='ls -la'";
    let out: [512]u8 = [0; 512];

    const un = planRcCompletionInstall(ShellCompDialect.Bash, "cap", rc_no_nl, RcCompOperation.Uninstall, out[0..]) catch unreachable;
    assert(un.action == RcCompAction.NoChange);
    assert(strEql(out[0..un.bytes_written], rc_no_nl));

    const ins = planRcCompletionInstall(ShellCompDialect.Bash, "cap", rc_no_nl, RcCompOperation.InstallOrUpdate, out[0..]) catch unreachable;
    assert(ins.action == RcCompAction.AppendBlock);
    assert(strContains(out[0..ins.bytes_written], "alias ll='ls -la'\n# >>> cap completions >>>\n"));
});

test("computeDynamicCompletions and generateCapShellCompletion validate invalid cursor and empty program name", () => {
    const cmds: [0]ShellCompCommandRule = [];
    const flags: [0]ShellCompFlagRule = [];
    let out: [128]u8 = [0; 128];

    let empty_prog = false;
    * = generateCapShellCompletion(ShellCompDialect.Bash, "", cmds[0..], flags[0..], out[0..]) catch {
        empty_prog = true;
    };
    assert(empty_prog);

    const toks: [1][]const u8 = ["cap"];
    let cands: [4]ShellCompCandidate = [ShellCompCandidate.empty(); 4];
    let bad_cursor = false;
    * = computeDynamicCompletions(toks[0..], 5, cmds[0..], flags[0..], ShellCompProjectContext{}, cands[0..]) catch {
        bad_cursor = true;
    };
    assert(bad_cursor);
});

test("deterministic 500-iteration fuzz over random CLI token lines causes 0 panics or out-of-bounds", () => {
    let seed: u64 = 0x94d049bb133111eb;
    const flags: [3]ShellCompFlagRule = [
        ShellCompFlagRule{ long_name: "--target", short_name: "-t", arg_hint: ShellCompArgHint.EnumValues, enum_values_csv: "native,linux-x64,wasm" },
        ShellCompFlagRule{ long_name: "--minify", short_name: "-m" },
        ShellCompFlagRule{ long_name: "--filter", short_name: "-F", arg_hint: ShellCompArgHint.PackageName },
    ];
    const cmds: [4]ShellCompCommandRule = [
        ShellCompCommandRule{ name: "build", arg_hint: ShellCompArgHint.FilePath, file_ext_filter: ".cp,.ts", flags_start: 0, flags_len: 2 },
        ShellCompCommandRule{ name: "run", arg_hint: ShellCompArgHint.ScriptName, flags_start: 2, flags_len: 1 },
        ShellCompCommandRule{ name: "add", arg_hint: ShellCompArgHint.PackageName },
        ShellCompCommandRule{ name: "test", arg_hint: ShellCompArgHint.FilePath, file_ext_filter: ".cp" },
    ];
    const scripts: [3][]const u8 = ["dev", "test:unit", "build"];
    const pkgs: [3][]const u8 = ["zod", "cap-http", "@org/core"];
    const files: [3][]const u8 = ["src/a.cp", "src/b.ts", "notes.txt"];
    const ctx = ShellCompProjectContext{ scripts: scripts[0..], packages: pkgs[0..], files: files[0..] };

    const vocab: [14][]const u8 = [
        "cap", "build", "run", "add", "test", "unknown",
        "--target", "--target=", "--target=L", "-m", "-", "--", "", "SRC/A",
    ];

    let toks: [6][]const u8 = [""; 6];
    let cands: [12]ShellCompCandidate = [ShellCompCandidate.empty(); 12];
    let iter: u32 = 0;
    let total_cands: usize = 0;

    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        let r = seed *% 2685821657736338717;

        const tok_n: usize = @as(usize, @intCast(r % 6)) + 1;
        let k: usize = 0;
        while (k < tok_n) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            toks[k] = vocab[@as(usize, @intCast((r >> 16) % 14))];
            k += 1;
        }
        const cursor: usize = @as(usize, @intCast((r >> 8) % (tok_n + 1)));
        const sum = computeDynamicCompletions(toks[0..tok_n], cursor, cmds[0..], flags[0..], ctx, cands[0..]) catch unreachable;
        assert(sum.candidate_count <= cands.len);
        // Verify sorted invariant: scores must be non-increasing
        let c: usize = 1;
        while (c < sum.candidate_count) {
            assert(cands[c - 1].score >= cands[c].score);
            c += 1;
        }
        total_cands += sum.candidate_count;
        iter += 1;
    }

    assert(total_cands > 200);
});
