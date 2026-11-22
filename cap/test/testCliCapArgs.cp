import { test, assert, assertEq } from "cap:test";
import {
    CapArgsError,
    CliFlagValueKind,
    CliFlagValueSource,
    CapSubcommand,
    CapCompletionShell,
    CliFlagSpec,
    CapEnvBinding,
    CliParsedFlag,
    CapArgsDiagnostic,
    parseCliInt,
    parseCliUint,
    parseCliByteSize,
    resolveCapSubcommand,
    capSubcommandName,
    levenshteinDistance,
    suggestClosestSubcommand,
    suggestClosestFlag,
    formatCapArgsDiagnostic,
    getParsedFlag,
    parseCapArgs,
    formatCommandHelp,
    formatShellCompletion,
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return true;
        i += 1;
    }
    return false;
}

test("Declarative flags: long, short, clustered, negated, counted, repeated, enum, and byte sizes", () => {
    const specs: [7]CliFlagSpec = [
        CliFlagSpec{ long_name: "verbose", short_name: 'v', kind: CliFlagValueKind.Count, is_global: true, description: "Increase verbosity" },
        CliFlagSpec{ long_name: "cache", kind: CliFlagValueKind.Bool, allow_negation: true, default_val: "true", description: "Enable cache" },
        CliFlagSpec{ long_name: "port", short_name: 'p', kind: CliFlagValueKind.Uint, default_val: "3000", description: "Server port" },
        CliFlagSpec{ long_name: "max-mem", short_name: 'm', kind: CliFlagValueKind.ByteSize, description: "Memory cap" },
        CliFlagSpec{ long_name: "mode", kind: CliFlagValueKind.Enum, allowed_values: "auto,fallback,force,disable", description: "Install mode" },
        CliFlagSpec{ long_name: "define", short_name: 'd', kind: CliFlagValueKind.StringList, description: "Macro definitions" },
        CliFlagSpec{ long_name: "offset", kind: CliFlagValueKind.Int, description: "Signed offset" },
    ];

    const argv: [11][]const u8 = [
        "-vvp8080",
        "build",
        "--no-cache",
        "--max-mem=16MB",
        "--mode",
        "fallback",
        "-d",
        "A=1",
        "--define=B=2",
        "--offset=-42",
        "src/main.cp",
    ];
    const empty_env: [0]CapEnvBinding = [];
    let flags: [8]CliParsedFlag = [CliParsedFlag.empty(); 8];
    let lists: [8][]const u8 = [""; 8];
    let pos: [8][]const u8 = [""; 8];
    let pass: [8][]const u8 = [""; 8];
    let diag = CapArgsDiagnostic.none();

    const res = parseCapArgs(
        argv[0..],
        specs[0..],
        empty_env[0..],
        true,
        flags[0..],
        lists[0..],
        pos[0..],
        pass[0..],
        &diag,
    ) catch unreachable;

    assert(res.subcommand == CapSubcommand.Build);
    assertEq(res.positionals_len, @as(usize, 1));
    assert(strEql(pos[0], "src/main.cp"));

    const f_verb = getParsedFlag(specs[0..], flags[0..], "verbose").?;
    assertEq(f_verb.count_val, @as(u32, 2));

    const f_cache = getParsedFlag(specs[0..], flags[0..], "cache").?;
    assert(!f_cache.bool_val);
    assert(f_cache.source == CliFlagValueSource.Cli);

    const f_port = getParsedFlag(specs[0..], flags[0..], "port").?;
    assertEq(f_port.uint_val, @as(u64, 8080));

    const f_mem = getParsedFlag(specs[0..], flags[0..], "max-mem").?;
    assertEq(f_mem.uint_val, @as(u64, 16 * 1048576));

    const f_mode = getParsedFlag(specs[0..], flags[0..], "mode").?;
    assert(strEql(f_mode.str_val, "fallback"));

    const f_def = getParsedFlag(specs[0..], flags[0..], "define").?;
    assertEq(f_def.list_len, @as(usize, 2));
    assert(strEql(lists[f_def.list_start], "A=1"));
    assert(strEql(lists[f_def.list_start + 1], "B=2"));

    const f_off = getParsedFlag(specs[0..], flags[0..], "offset").?;
    assertEq(f_off.int_val, @as(i64, -42));
});

test("Subcommand aliases and `--` passthrough argument separation", () => {
    assert(resolveCapSubcommand("i").? == CapSubcommand.Install);
    assert(resolveCapSubcommand("rm").? == CapSubcommand.Remove);
    assert(resolveCapSubcommand("x").? == CapSubcommand.Exec);
    assert(resolveCapSubcommand("b").? == CapSubcommand.Build);
    assert(resolveCapSubcommand("t").? == CapSubcommand.Test);
    assert(strEql(capSubcommandName(CapSubcommand.Install), "install"));

    const specs: [1]CliFlagSpec = [
        CliFlagSpec{ long_name: "cwd", kind: CliFlagValueKind.String, is_global: true },
    ];
    const argv: [7][]const u8 = ["--cwd", "/app", "x", "eslint", "--", "--fix", "--unknown-child-flag"];
    const empty_env: [0]CapEnvBinding = [];
    let flags: [4]CliParsedFlag = [CliParsedFlag.empty(); 4];
    let lists: [4][]const u8 = [""; 4];
    let pos: [4][]const u8 = [""; 4];
    let pass: [4][]const u8 = [""; 4];
    let diag = CapArgsDiagnostic.none();

    const res = parseCapArgs(argv[0..], specs[0..], empty_env[0..], true, flags[0..], lists[0..], pos[0..], pass[0..], &diag) catch unreachable;
    assert(res.subcommand == CapSubcommand.Exec);
    assertEq(res.positionals_len, @as(usize, 1));
    assert(strEql(pos[0], "eslint"));
    assertEq(res.passthrough_len, @as(usize, 2));
    assert(strEql(pass[0], "--fix"));
    assert(strEql(pass[1], "--unknown-child-flag"));
});

test("Priority fallback (CLI > Env > Default) and required flag validation", () => {
    const specs: [3]CliFlagSpec = [
        CliFlagSpec{ long_name: "registry", kind: CliFlagValueKind.String, default_val: "https://default.io", env_var: "CAP_REGISTRY" },
        CliFlagSpec{ long_name: "token", kind: CliFlagValueKind.String, required: true, env_var: "CAP_TOKEN" },
        CliFlagSpec{ long_name: "jobs", kind: CliFlagValueKind.Uint, default_val: "4" },
    ];
    const env_vars: [2]CapEnvBinding = [
        CapEnvBinding{ key: "CAP_REGISTRY", value: "https://env.registry.io" },
        CapEnvBinding{ key: "CAP_TOKEN", value: "tok_from_env" },
    ];
    const argv: [1][]const u8 = ["publish"];
    let flags: [4]CliParsedFlag = [CliParsedFlag.empty(); 4];
    let lists: [4][]const u8 = [""; 4];
    let pos: [4][]const u8 = [""; 4];
    let pass: [4][]const u8 = [""; 4];
    let diag = CapArgsDiagnostic.none();

    const res = parseCapArgs(argv[0..], specs[0..], env_vars[0..], true, flags[0..], lists[0..], pos[0..], pass[0..], &diag) catch unreachable;
    assert(res.subcommand == CapSubcommand.Publish);
    assert(strEql(flags[0].str_val, "https://env.registry.io"));
    assert(flags[0].source == CliFlagValueSource.Env);
    assert(strEql(flags[1].str_val, "tok_from_env"));
    assertEq(flags[2].uint_val, @as(u64, 4));
    assert(flags[2].source == CliFlagValueSource.Default);

    // Without env var, missing required flag triggers MissingRequiredFlag
    const empty_env: [0]CapEnvBinding = [];
    let req_err = false;
    * = parseCapArgs(argv[0..], specs[0..], empty_env[0..], true, flags[0..], lists[0..], pos[0..], pass[0..], &diag) catch |e| {
        req_err = (e == CapArgsError.MissingRequiredFlag);
    };
    assert(req_err);
    assert(strEql(diag.offending_token, "token"));
});

test("Levenshtein spell-check suggestions for unknown flags and subcommands", () => {
    assertEq(levenshteinDistance("install", "instlal"), @as(u32, 2));
    assertEq(levenshteinDistance("frozen-lockfile", "frozn-lockfile"), @as(u32, 1));

    const specs: [2]CliFlagSpec = [
        CliFlagSpec{ long_name: "frozen-lockfile", kind: CliFlagValueKind.Bool },
        CliFlagSpec{ long_name: "production", kind: CliFlagValueKind.Bool },
    ];
    assert(strEql(suggestClosestFlag("frozn-lockfile", specs[0..]).?, "frozen-lockfile"));
    assert(strEql(suggestClosestSubcommand("instlal").?, "install"));

    const empty_env: [0]CapEnvBinding = [];
    let flags: [4]CliParsedFlag = [CliParsedFlag.empty(); 4];
    let lists: [4][]const u8 = [""; 4];
    let pos: [4][]const u8 = [""; 4];
    let pass: [4][]const u8 = [""; 4];
    let diag = CapArgsDiagnostic.none();
    let msg_buf: [256]u8 = [0; 256];

    // Unknown flag with suggestion
    const bad_flag_argv: [2][]const u8 = ["install", "--frozn-lockfile"];
    let flag_err = false;
    * = parseCapArgs(bad_flag_argv[0..], specs[0..], empty_env[0..], true, flags[0..], lists[0..], pos[0..], pass[0..], &diag) catch |e| {
        flag_err = (e == CapArgsError.UnknownFlag);
    };
    assert(flag_err);
    assert(diag.has_suggestion);
    const m1 = formatCapArgsDiagnostic(diag, msg_buf[0..]) catch unreachable;
    assert(strEql(msg_buf[0..m1], "error: unknown flag '--frozn-lockfile' — did you mean '--frozen-lockfile'?"));

    // Unknown subcommand with suggestion
    const bad_cmd_argv: [1][]const u8 = ["publsih"];
    let cmd_err = false;
    * = parseCapArgs(bad_cmd_argv[0..], specs[0..], empty_env[0..], true, flags[0..], lists[0..], pos[0..], pass[0..], &diag) catch |e| {
        cmd_err = (e == CapArgsError.UnknownSubcommand);
    };
    assert(cmd_err);
    assert(diag.has_suggestion);
    const m2 = formatCapArgsDiagnostic(diag, msg_buf[0..]) catch unreachable;
    assert(strEql(msg_buf[0..m2], "error: unknown subcommand 'publsih' — did you mean 'publish'?"));
});

test("Byte-size, int/uint, and enum validation error paths", () => {
    assertEq(parseCliByteSize("64k") catch unreachable, @as(u64, 65536));
    assertEq(parseCliByteSize("2GiB") catch unreachable, @as(u64, 2147483648));
    assertEq(parseCliInt("-1_024") catch unreachable, @as(i64, -1024));
    assertEq(parseCliUint("9_000") catch unreachable, @as(u64, 9000));

    let bad_sz = false;
    * = parseCliByteSize("100xyz") catch |e| {
        bad_sz = (e == CapArgsError.InvalidByteSize);
    };
    assert(bad_sz);

    const specs: [1]CliFlagSpec = [
        CliFlagSpec{ long_name: "linker", kind: CliFlagValueKind.Enum, allowed_values: "hoisted,isolated" },
    ];
    const empty_env: [0]CapEnvBinding = [];
    let flags: [2]CliParsedFlag = [CliParsedFlag.empty(); 2];
    let lists: [2][]const u8 = [""; 2];
    let pos: [2][]const u8 = [""; 2];
    let pass: [2][]const u8 = [""; 2];
    let diag = CapArgsDiagnostic.none();

    const argv_bad_enum: [2][]const u8 = ["install", "--linker=symlink"];
    let enum_err = false;
    * = parseCapArgs(argv_bad_enum[0..], specs[0..], empty_env[0..], true, flags[0..], lists[0..], pos[0..], pass[0..], &diag) catch |e| {
        enum_err = (e == CapArgsError.InvalidEnumValue);
    };
    assert(enum_err);
});

test("Declarative help document formatter aligns commands and options", () => {
    const specs: [2]CliFlagSpec = [
        CliFlagSpec{ long_name: "frozen-lockfile", short_name: 'f', kind: CliFlagValueKind.Bool, description: "Disallow lockfile updates" },
        CliFlagSpec{ long_name: "registry", kind: CliFlagValueKind.String, value_placeholder: "<URL>", description: "Override package registry" },
    ];
    let buf: [2048]u8 = [0; 2048];
    const n = formatCommandHelp("cap", "", "Fast & native CP toolchain", specs[0..], true, buf[0..]) catch unreachable;
    const text = buf[0..n];
    assert(containsSubstr(text, "Usage: cap <command> [options]\n"));
    assert(containsSubstr(text, "Commands:\n"));
    assert(containsSubstr(text, "  install     Install project dependencies from lockfile\n"));
    assert(containsSubstr(text, "Options:\n"));
    assert(containsSubstr(text, "  -f, --frozen-lockfile     Disallow lockfile updates\n"));
    assert(containsSubstr(text, "      --registry <URL>      Override package registry\n"));
});

test("Bash and Zsh shell completion script generation", () => {
    const specs: [2]CliFlagSpec = [
        CliFlagSpec{ long_name: "frozen-lockfile", description: "Disallow lockfile updates" },
        CliFlagSpec{ long_name: "production", description: "Omit devDependencies" },
    ];
    let buf: [2048]u8 = [0; 2048];

    const bash_len = formatShellCompletion("cap", CapCompletionShell.Bash, specs[0..], buf[0..]) catch unreachable;
    const bash = buf[0..bash_len];
    assert(containsSubstr(bash, "_cap_completions()"));
    assert(containsSubstr(bash, "--frozen-lockfile"));
    assert(containsSubstr(bash, "complete -F _cap_completions cap"));

    const zsh_len = formatShellCompletion("cap", CapCompletionShell.Zsh, specs[0..], buf[0..]) catch unreachable;
    const zsh = buf[0..zsh_len];
    assert(containsSubstr(zsh, "#compdef cap"));
    assert(containsSubstr(zsh, "'--production[Omit devDependencies]'"));
});
