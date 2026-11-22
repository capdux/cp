import { test, assert, assertEq } from "cap:test";
import {
    TOOL_CONFIG_DEFAULT_REGISTRY,
    ToolConfigError,
    ToolConfigLayer,
    ToolConfigLogLevel,
    ToolConfigLinker,
    ToolConfigAutoInstall,
    ToolConfigShell,
    ToolConfigDiagSeverity,
    ToolEnvVar,
    ToolScopeRegistry,
    ToolKeyValueEntry,
    ToolConfigDiagnostic,
    ToolConfig,
    ToolConfigParseOptions,
    ToolConfigStorage,
    expandEnvVars,
    unescapeTomlString,
    isValidRegistryUrl,
    parseToolConfig,
    mergeToolConfig,
    mergeScopeRegistries,
    mergeKeyValueEntries,
    resolveScopedRegistry,
    redactSecretToken,
    formatRedactedRegistryRoute,
    validateToolConfig,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("Full multi-section capfig.toml parsing with inline tables, arrays, and env expansion", () => {
    const toml =
        "logLevel = \"debug\"\n" ++
        "smol = true\n" ++
        "telemetry = false\n" ++
        "preload = [\"./setup.cp\", \"./hooks.cp\"]\n" ++
        "\n" ++
        "[install]\n" ++
        "registry = \"https://registry.company.dev\"\n" ++
        "auto = \"fallback\"\n" ++
        "linker = \"isolated\"\n" ++
        "exact = true\n" ++
        "frozenLockfile = true\n" ++
        "concurrentScripts = 8\n" ++
        "minimumReleaseAge = 86400\n" ++
        "\n" ++
        "[install.cache]\n" ++
        "dir = \"${HOME}/.cap/cache\"\n" ++
        "disable = false\n" ++
        "\n" ++
        "[install.lockfile]\n" ++
        "save = true\n" ++
        "print = \"yarn\"\n" ++
        "\n" ++
        "[install.security]\n" ++
        "scanner = \"@acme/sec-scanner\"\n" ++
        "\n" ++
        "[install.scopes]\n" ++
        "\"@acme\" = { url = \"https://npm.acme.io\", token = \"$ACME_TOKEN\", email = \"ci@acme.io\" }\n" ++
        "\"@internal\" = \"https://:priv_tok_987654@git.corp.local/npm\"\n" ++
        "\n" ++
        "[test]\n" ++
        "root = \"./tests\"\n" ++
        "coverage = true\n" ++
        "coverageThreshold = 85.5\n" ++
        "timeout = 12000\n" ++
        "retry = 2\n" ++
        "\n" ++
        "[run]\n" ++
        "silent = true\n" ++
        "shell = \"cap\"\n" ++
        "elide-lines = 15\n" ++
        "\n" ++
        "[define]\n" ++
        "\"process.env.NODE_ENV\" = \"'production'\"\n" ++
        "\n" ++
        "[loader]\n" ++
        "\".txt\" = \"text\"\n" ++
        "\n" ++
        "[debug]\n" ++
        "editor = \"code\"\n" ++
        "\n" ++
        "[console]\n" ++
        "depth = 4\n" ++
        "\n" ++
        "[serve]\n" ++
        "port = 8080\n" ++
        "hmr = true\n";

    let scopes: [8]ToolScopeRegistry = [ToolScopeRegistry.empty(); 8];
    let defines: [8]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 8];
    let loaders: [8]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 8];
    let preloads: [8]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 8];
    let diags: [8]ToolConfigDiagnostic = [ToolConfigDiagnostic.empty(); 8];
    let arena: [4096]u8 = [0; 4096];
    let storage = ToolConfigStorage{
        scopes: scopes[0..],
        defines: defines[0..],
        loaders: loaders[0..],
        preloads: preloads[0..],
        diags: diags[0..],
        str_arena: arena[0..],
    };

    const env_vars: [2]ToolEnvVar = [
        ToolEnvVar{ key: "HOME", value: "/home/dev" },
        ToolEnvVar{ key: "ACME_TOKEN", value: "acme_secret_123456" },
    ];

    const cfg = parseToolConfig(toml, env_vars[0..], ToolConfigParseOptions.defaults(), &storage) catch unreachable;

    assert(cfg.log_level.? == ToolConfigLogLevel.Debug);
    assert(cfg.smol.?);
    assert(!cfg.telemetry.?);
    assertEq(cfg.preloads_len, @as(usize, 2));
    assert(strEql(preloads[0].value, "./setup.cp"));
    assert(strEql(preloads[1].value, "./hooks.cp"));

    assert(strEql(cfg.install.registry_url.?, "https://registry.company.dev"));
    assert(cfg.install.auto_install.? == ToolConfigAutoInstall.Fallback);
    assert(cfg.install.linker.? == ToolConfigLinker.Isolated);
    assert(cfg.install.exact.?);
    assert(cfg.install.frozen_lockfile.?);
    assertEq(cfg.install.concurrent_scripts.?, @as(u32, 8));
    assertEq(cfg.install.minimum_release_age_secs.?, @as(u64, 86400));
    assert(strEql(cfg.install.cache_dir.?, "/home/dev/.cap/cache"));
    assert(!cfg.install.cache_disable.?);
    assert(cfg.install.lockfile_save.?);
    assert(strEql(cfg.install.lockfile_print.?, "yarn"));
    assert(strEql(cfg.install.security_scanner.?, "@acme/sec-scanner"));

    assertEq(cfg.scopes_len, @as(usize, 2));
    assert(strEql(scopes[0].scope, "@acme"));
    assert(strEql(scopes[0].url, "https://npm.acme.io"));
    assert(strEql(scopes[0].token, "acme_secret_123456"));
    assert(strEql(scopes[0].email, "ci@acme.io"));

    assert(strEql(scopes[1].scope, "@internal"));
    assert(strEql(scopes[1].url, "https://git.corp.local/npm"));
    assert(strEql(scopes[1].token, "priv_tok_987654"));

    assert(strEql(cfg.test_cfg.root.?, "./tests"));
    assert(cfg.test_cfg.coverage.?);
    assert(cfg.test_cfg.coverage_threshold_lines.? > 85.4 && cfg.test_cfg.coverage_threshold_lines.? < 85.6);
    assertEq(cfg.test_cfg.timeout_ms.?, @as(u32, 12000));
    assertEq(cfg.test_cfg.retry.?, @as(u32, 2));

    assert(cfg.run.silent.?);
    assert(cfg.run.shell.? == ToolConfigShell.Cap);
    assertEq(cfg.run.elide_lines.?, @as(u32, 15));

    assertEq(cfg.defines_len, @as(usize, 1));
    assert(strEql(defines[0].key, "process.env.NODE_ENV"));
    assert(strEql(defines[0].value, "'production'"));

    assertEq(cfg.loaders_len, @as(usize, 1));
    assert(strEql(loaders[0].key, ".txt"));
    assert(strEql(loaders[0].value, "text"));

    assert(strEql(cfg.debug.editor.?, "code"));
    assertEq(cfg.debug.console_depth.?, @as(u32, 4));
    assertEq(cfg.debug.serve_port.?, @as(u16, 8080));
    assert(cfg.debug.serve_hmr.?);
    assertEq(cfg.diags_len, @as(usize, 0));
});

test("Environment variable interpolation and TOML string unescaping", () => {
    const env_vars: [2]ToolEnvVar = [
        ToolEnvVar{ key: "HOST", value: "pkg.local" },
        ToolEnvVar{ key: "PORT", value: "9000" },
    ];
    let buf: [256]u8 = [0; 256];

    const n1 = expandEnvVars("https://$HOST:${PORT}/$$api/$MISSING", env_vars[0..], true, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "https://pkg.local:9000/$api/$MISSING"));

    const n2 = expandEnvVars("prefix_${MISSING}_suffix", env_vars[0..], false, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], "prefix__suffix"));

    const u1 = unescapeTomlString("\"line1\\nline2\\t\\\"q\\\"\\\\\"", buf[0..]) catch unreachable;
    assert(strEql(buf[0..u1], "line1\nline2\t\"q\"\\"));

    const u2 = unescapeTomlString("'literal\\npath'", buf[0..]) catch unreachable;
    assert(strEql(buf[0..u2], "literal\\npath"));
});

test("Four-layer priority merger preserves unset fields and overrides with explicit false/0", () => {
    // System layer sets defaults: telemetry=true, concurrentScripts=16, smol=true
    let sys = ToolConfig.empty(ToolConfigLayer.System);
    sys.telemetry = true;
    sys.smol = true;
    sys.install.concurrent_scripts = 16;
    sys.install.registry_url = "https://sys.registry.local";
    sys.test_cfg.timeout_ms = 5000;

    // User layer overrides registry & sets telemetry=false (explicit false!)
    let usr = ToolConfig.empty(ToolConfigLayer.User);
    usr.telemetry = false;
    usr.install.registry_url = "https://user.registry.local";
    usr.debug.console_depth = 3;

    // Project layer sets linker=isolated, timeout=15000, leaves telemetry unset (null)
    let proj = ToolConfig.empty(ToolConfigLayer.Project);
    proj.install.linker = ToolConfigLinker.Isolated;
    proj.test_cfg.timeout_ms = 15000;

    // CLI override explicitly sets smol=false and elide_lines=0
    let cli = ToolConfig.empty(ToolConfigLayer.CliOverride);
    cli.smol = false;
    cli.run.elide_lines = 0;

    const m1 = mergeToolConfig(sys, usr);
    const m2 = mergeToolConfig(m1, proj);
    const final_cfg = mergeToolConfig(m2, cli);

    assert(final_cfg.layer == ToolConfigLayer.CliOverride);
    assert(!final_cfg.telemetry.?); // User explicit false preserved through Project/CLI null
    assert(!final_cfg.smol.?); // CLI explicit false overrode System true
    assertEq(final_cfg.install.concurrent_scripts.?, @as(u32, 16)); // Preserved from System
    assert(strEql(final_cfg.install.registry_url.?, "https://user.registry.local")); // From User
    assert(final_cfg.install.linker.? == ToolConfigLinker.Isolated); // From Project
    assertEq(final_cfg.test_cfg.timeout_ms.?, @as(u32, 15000)); // From Project
    assertEq(final_cfg.debug.console_depth.?, @as(u32, 3)); // From User
    assertEq(final_cfg.run.elide_lines.?, @as(u32, 0)); // Explicit 0 from CLI

    // Scope & Define layer merging
    const base_scopes: [2]ToolScopeRegistry = [
        ToolScopeRegistry{ scope: "@acme", url: "https://old.acme.io", token: "old_tok", layer: ToolConfigLayer.User },
        ToolScopeRegistry{ scope: "@keep", url: "https://keep.io", layer: ToolConfigLayer.User },
    ];
    const proj_scopes: [2]ToolScopeRegistry = [
        ToolScopeRegistry{ scope: "@acme", url: "https://new.acme.io", token: "new_tok", layer: ToolConfigLayer.Project },
        ToolScopeRegistry{ scope: "@extra", url: "https://extra.io", layer: ToolConfigLayer.Project },
    ];
    let merged_scopes: [8]ToolScopeRegistry = [ToolScopeRegistry.empty(); 8];
    const ms_len = mergeScopeRegistries(base_scopes[0..], proj_scopes[0..], merged_scopes[0..]) catch unreachable;
    assertEq(ms_len, @as(usize, 3));
    assert(strEql(merged_scopes[0].url, "https://new.acme.io"));
    assert(strEql(merged_scopes[1].scope, "@keep"));
    assert(strEql(merged_scopes[2].scope, "@extra"));

    const base_defs: [1]ToolKeyValueEntry = [ToolKeyValueEntry{ key: "VER", value: "\"1\"" }];
    const ov_defs: [2]ToolKeyValueEntry = [
        ToolKeyValueEntry{ key: "VER", value: "\"2\"" },
        ToolKeyValueEntry{ key: "MODE", value: "\"prod\"" },
    ];
    let merged_defs: [4]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 4];
    const md_len = mergeKeyValueEntries(base_defs[0..], ov_defs[0..], merged_defs[0..]) catch unreachable;
    assertEq(md_len, @as(usize, 2));
    assert(strEql(merged_defs[0].value, "\"2\""));
    assert(strEql(merged_defs[1].key, "MODE"));
});

test("Scoped registry longest-prefix routing and automatic secret token redaction", () => {
    let cfg = ToolConfig.empty(ToolConfigLayer.Project);
    cfg.install.registry_url = "https://default.registry.dev";
    cfg.install.registry_token = "def_token_abcdef1234";

    const scopes: [2]ToolScopeRegistry = [
        ToolScopeRegistry{ scope: "@acme", url: "https://acme.registry.dev", token: "acme_tok_00112233" },
        ToolScopeRegistry{ scope: "@acme/ui", url: "https://ui.acme.dev", token: "short" },
    ];

    // Longest-prefix match hits @acme/ui
    const r_ui = resolveScopedRegistry(cfg, scopes[0..], "@acme/ui/button");
    assert(r_ui.is_scoped);
    assert(strEql(r_ui.matched_scope, "@acme/ui"));
    assert(strEql(r_ui.url, "https://ui.acme.dev"));

    // Standard scope match hits @acme
    const r_core = resolveScopedRegistry(cfg, scopes[0..], "@acme/core");
    assert(r_core.is_scoped);
    assert(strEql(r_core.matched_scope, "@acme"));
    assert(strEql(r_core.url, "https://acme.registry.dev"));

    // Unscoped package falls back to default registry
    const r_pub = resolveScopedRegistry(cfg, scopes[0..], "lodash");
    assert(!r_pub.is_scoped);
    assert(strEql(r_pub.url, "https://default.registry.dev"));

    // Token redaction & formatted diagnostic route
    let red_buf: [128]u8 = [0; 128];
    const r1 = redactSecretToken("acme_tok_00112233", red_buf[0..]) catch unreachable;
    assert(strEql(red_buf[0..r1], "acme***2233"));

    const r2 = redactSecretToken("short", red_buf[0..]) catch unreachable;
    assert(strEql(red_buf[0..r2], "***"));

    const fmt_len = formatRedactedRegistryRoute(r_core, red_buf[0..]) catch unreachable;
    assert(strEql(red_buf[0..fmt_len], "scope=@acme url=https://acme.registry.dev token=acme***2233"));
});

test("Strict vs lenient unknown section/key diagnostics and duplicate key detection", () => {
    const toml_unknown =
        "[install]\n" ++
        "unknownFlag = true\n" ++
        "[futureSection]\n" ++
        "foo = \"bar\"\n";

    let scopes: [4]ToolScopeRegistry = [ToolScopeRegistry.empty(); 4];
    let defines: [4]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 4];
    let loaders: [4]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 4];
    let preloads: [4]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 4];
    let diags: [8]ToolConfigDiagnostic = [ToolConfigDiagnostic.empty(); 8];
    let arena: [512]u8 = [0; 512];
    let storage = ToolConfigStorage{
        scopes: scopes[0..],
        defines: defines[0..],
        loaders: loaders[0..],
        preloads: preloads[0..],
        diags: diags[0..],
        str_arena: arena[0..],
    };

    const empty_env: [0]ToolEnvVar = [];

    // Lenient mode succeeds and records 2 warnings
    const cfg_lenient = parseToolConfig(toml_unknown, empty_env[0..], ToolConfigParseOptions.defaults(), &storage) catch unreachable;
    assertEq(cfg_lenient.diags_len, @as(usize, 2));
    assert(diags[0].severity == ToolConfigDiagSeverity.Warning);
    assert(strEql(diags[0].key, "unknownFlag"));
    assert(strEql(diags[1].section, "futureSection"));

    // Strict mode rejects unknown key immediately
    let strict_err = false;
    * = parseToolConfig(toml_unknown, empty_env[0..], ToolConfigParseOptions.strict(ToolConfigLayer.Project), &storage) catch |e| {
        strict_err = (e == ToolConfigError.UnknownKey);
    };
    assert(strict_err);

    // Duplicate define key rejected
    const dup_toml =
        "[define]\n" ++
        "FOO = \"1\"\n" ++
        "FOO = \"2\"\n";
    let dup_err = false;
    * = parseToolConfig(dup_toml, empty_env[0..], ToolConfigParseOptions.defaults(), &storage) catch |e| {
        dup_err = (e == ToolConfigError.DuplicateKey);
    };
    assert(dup_err);
});

test("Semantic validation catches invalid URLs, concurrency, timeout, and thresholds", () => {
    assert(isValidRegistryUrl(TOOL_CONFIG_DEFAULT_REGISTRY));
    assert(!isValidRegistryUrl("ftp://invalid.host"));
    assert(!isValidRegistryUrl("https://"));

    let bad_cfg = ToolConfig.empty(ToolConfigLayer.Project);
    bad_cfg.install.registry_url = "not-a-url";
    bad_cfg.install.concurrent_scripts = 0;
    bad_cfg.test_cfg.timeout_ms = 0;
    bad_cfg.test_cfg.coverage_threshold_lines = 120.0;

    const bad_scopes: [1]ToolScopeRegistry = [
        ToolScopeRegistry{ scope: "missing_at", url: "ftp://bad.host" },
    ];
    let diags: [16]ToolConfigDiagnostic = [ToolConfigDiagnostic.empty(); 16];
    const count = validateToolConfig(bad_cfg, bad_scopes[0..], diags[0..]);
    assertEq(count, @as(usize, 6));
    assert(diags[0].severity == ToolConfigDiagSeverity.Error);
});

test("Boundary and malformed input guards: empty, unterminated syntax, invalid enums", () => {
    let scopes: [2]ToolScopeRegistry = [ToolScopeRegistry.empty(); 2];
    let defines: [2]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 2];
    let loaders: [2]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 2];
    let preloads: [2]ToolKeyValueEntry = [ToolKeyValueEntry.empty(); 2];
    let diags: [4]ToolConfigDiagnostic = [ToolConfigDiagnostic.empty(); 4];
    let arena: [256]u8 = [0; 256];
    let storage = ToolConfigStorage{
        scopes: scopes[0..],
        defines: defines[0..],
        loaders: loaders[0..],
        preloads: preloads[0..],
        diags: diags[0..],
        str_arena: arena[0..],
    };
    const empty_env: [0]ToolEnvVar = [];

    let err_empty = false;
    * = parseToolConfig("   \n\t ", empty_env[0..], ToolConfigParseOptions.defaults(), &storage) catch |e| {
        err_empty = (e == ToolConfigError.EmptyInput);
    };
    assert(err_empty);

    let err_sec = false;
    * = parseToolConfig("[unclosed\n", empty_env[0..], ToolConfigParseOptions.defaults(), &storage) catch |e| {
        err_sec = (e == ToolConfigError.InvalidSectionHeader);
    };
    assert(err_sec);

    let err_lvl = false;
    * = parseToolConfig("logLevel = \"verbose\"\n", empty_env[0..], ToolConfigParseOptions.defaults(), &storage) catch |e| {
        err_lvl = (e == ToolConfigError.InvalidLogLevel);
    };
    assert(err_lvl);

    let err_neg = false;
    * = parseToolConfig("[test]\ntimeout = -500\n", empty_env[0..], ToolConfigParseOptions.defaults(), &storage) catch |e| {
        err_neg = (e == ToolConfigError.InvalidNumber);
    };
    assert(err_neg);
});
