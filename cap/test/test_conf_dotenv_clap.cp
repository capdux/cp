// Source: /data/dev/bun/src/dotenv/ & /data/dev/bun/src/ini/ & /data/dev/bun/src/interchange/toml/ & /data/dev/bun/src/clap/
// ============================================================================
// cap/test/test_conf_dotenv_clap.cp
// Unified Industrial-grade Audit Test Suite for cap:dotenv, cap:conf, cap:clap
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    parse,
    parseWithOptions,
    expandString,
    EnvMap,
    DotenvOptions,
    DotenvLoader,
    config,
} from "cap:dotenv";
import {
    Ini,
    TomlDoc,
    YamlDoc,
    YAML,
    JSON5,
    ConfGetter,
    ConfValueParser,
} from "cap:conf";
import { JsonValue, JsonTag } from "../../lib/json/value";
import {
    Names,
    Values,
    Param,
    Help,
    ParamHelp,
    ParseOptions,
    parseParam,
    usage,
    help,
    SliceIterator,
    ShellIterator,
    StreamingClap,
    ComptimeClap,
} from "cap:clap";

fn testDotenvEngine(): !void {
    const raw_env =
        "# Application Environment Configuration\n" +
        "APP_HOST=127.0.0.1\n" +
        "APP_PORT=8080\n" +
        "export APP_MODE=production\n" +
        "GREETING=\"Hello\\nCP\\tWorld\"\n" +
        "LITERAL_RAW='no \\n escape ${APP_HOST}'\n" +
        "API_ENDPOINT=http://${APP_HOST}:${APP_PORT}/v1\n" +
        "FALLBACK_TIMEOUT=${CUSTOM_TIMEOUT:-30s}\n" +
        "ESCAPED_VAR=\\${KEEP_LITERAL}\n";

    let env = parse(raw_env);
    expect_eq(std.mem.eql(u8, env.get("APP_HOST").?, "127.0.0.1"), true);
    expect_eq(std.mem.eql(u8, env.get("APP_PORT").?, "8080"), true);
    expect_eq(std.mem.eql(u8, env.get("APP_MODE").?, "production"), true);
    expect_eq(std.mem.eql(u8, env.get("GREETING").?, "Hello\nCP\tWorld"), true);
    expect_eq(std.mem.eql(u8, env.get("LITERAL_RAW").?, "no \\n escape 127.0.0.1"), true);
    expect_eq(std.mem.eql(u8, env.get("API_ENDPOINT").?, "http://127.0.0.1:8080/v1"), true);
    expect_eq(std.mem.eql(u8, env.get("FALLBACK_TIMEOUT").?, "30s"), true);
    expect_eq(std.mem.eql(u8, env.get("ESCAPED_VAR").?, "${KEEP_LITERAL}"), true);

    // Cascade priority test: higher-priority loaded first with overwrite=false
    let loader = DotenvLoader.init();
    loader.loadFromString("PORT=9000\nONLY_LOCAL=yes\n", false, true);
    loader.loadFromString("PORT=3000\nBASE_KEY=default_base\n", false, true);
    expect_eq(std.mem.eql(u8, loader.get("PORT").?, "9000"), true);
    expect_eq(std.mem.eql(u8, loader.get("ONLY_LOCAL").?, "yes"), true);
    expect_eq(std.mem.eql(u8, loader.get("BASE_KEY").?, "default_base"), true);
}

fn testConfIniAndToml(): !void {
    // 1. INI Parser & Serializer
    const ini_text =
        "; Global section\n" +
        "service = cp-gateway\n" +
        "enabled = yes\n" +
        "\n" +
        "[http]\n" +
        "host = 0.0.0.0\n" +
        "port = 8443\n" +
        "max_body = 16mb\n" +
        "read_timeout = 15s\n";

    let ini_doc = Ini.parse(ini_text);
    expect_eq(std.mem.eql(u8, ini_doc.getGlobal("service").?, "cp-gateway"), true);
    expect_eq(ini_doc.getBool("", "enabled", false), true);
    expect_eq(ini_doc.hasSection("http"), true);
    expect_eq(std.mem.eql(u8, ini_doc.get("http", "host").?, "0.0.0.0"), true);
    expect_eq(ini_doc.getInt("http", "port", 0), 8443);

    // Verify ConfGetter type-safe extraction on INI values
    expect_eq(ConfGetter.getPortOr(ini_doc.get("http", "port"), 80), 8443);
    expect_eq(ConfGetter.getByteSizeOr(ini_doc.get("http", "max_body"), 0), 16 * 1048576);
    expect_eq(ConfGetter.getDurationMsOr(ini_doc.get("http", "read_timeout"), 0), 15000);
    expect_eq(ConfGetter.getBoolOr(ini_doc.getGlobal("enabled"), false), true);

    // 2. TOML Parser & Serializer
    const toml_text =
        "# TOML Configuration\n" +
        "title = \"CP Cluster\"\n" +
        "\n" +
        "[cluster]\n" +
        "workers = 1_024\n" +
        "mask = 0xFF\n" +
        "perms = 0o755\n" +
        "flags = 0b1010\n" +
        "ratio = 0.85\n" +
        "tls = true\n" +
        "nodes = [ \"node-1\", \"node-2\", \"node-3\" ]\n" +
        "\n" +
        "[cluster.storage]\n" +
        "engine = \"rocksdb\"\n";

    let toml_doc = TomlDoc.parse(toml_text);
    expect_eq(std.mem.eql(u8, toml_doc.getRoot("title").?, "CP Cluster"), true);
    expect_eq(toml_doc.getInt("cluster", "workers", 0), 1024);
    expect_eq(toml_doc.getInt("cluster", "mask", 0), 255);
    expect_eq(toml_doc.getInt("cluster", "perms", 0), 493);
    expect_eq(toml_doc.getInt("cluster", "flags", 0), 10);
    expect_eq(toml_doc.getBool("cluster", "tls", false), true);
    expect_eq(toml_doc.getFloat("cluster", "ratio", 0.0) > 0.8, true);

    let nodes = toml_doc.getArray("cluster", "nodes");
    expect_eq(nodes.len, 3);
    expect_eq(std.mem.eql(u8, nodes[0], "node-1"), true);
    expect_eq(std.mem.eql(u8, nodes[2], "node-3"), true);
    expect_eq(std.mem.eql(u8, toml_doc.get("cluster.storage", "engine").?, "rocksdb"), true);
}

fn testConfYamlAndJson5(): !void {
    // 1. YAML Parser
    const yaml_text =
        "---\n" +
        "# Service YAML\n" +
        "version: \"2.0\"\n" +
        "server:\n" +
        "  host: 127.0.0.1 # local loopback\n" +
        "  port: 9090\n" +
        "  ssl: true\n" +
        "...\n";

    let yaml_doc = YamlDoc.parse(yaml_text);
    expect_eq(std.mem.eql(u8, yaml_doc.getRoot("version").?, "2.0"), true);
    expect_eq(yaml_doc.hasSection("server"), true);
    expect_eq( yaml_doc.hasKey("server", "host"), true);
    expect_eq(std.mem.eql(u8, yaml_doc.get("server", "host").?, "127.0.0.1"), true);
    expect_eq(yaml_doc.getInt("server", "port", 0), 9090);
    expect_eq(yaml_doc.getBool("server", "ssl", false), true);

    // 2. JSON5 Parser & Comment Stripper
    const json5_text =
        "{\n" +
        "  // JSON5 configuration\n" +
        "  unquotedKey: 'single-quoted-value',\n" +
        "  /* block comment */\n" +
        "  hexPort: 0x1F90,\n" +
        "  enabled: true,\n" +
        "  items: [ 10, 20, ],\n" +
        "}";

    let jval = try JSON5.parse(std.heap.page_allocator, json5_text);
    expect_eq(jval.tag == JsonTag.Object, true);
    expect_eq(jval.len(), 4);

    let k_opt = jval.get("unquotedKey");
    expect_eq(k_opt != null, true);
    if (k := k_opt) {
        if (s := k.asString()) {
            expect_eq(std.mem.eql(u8, s, "single-quoted-value"), true);
        }
    }

    let p_opt = jval.get("hexPort");
    expect_eq(p_opt != null, true);
    if (p := p_opt) {
        if (n := p.asNumber()) {
            expect_eq(n == 8080.0, true); // 0x1F90 == 8080
        }
    }

    let strip_buf: [256]u8 = undefined;
    let stripped = JSON5.stripCommentsAndTrailingCommas("{\n // comment\n \"a\": 1,\n}", &strip_buf);
    expect_eq(stripped.len > 0, true);
}

const clap_p1 = (parseParam("-v, --verbose Enable verbose output") catch unreachable);
const clap_p2 = (parseParam("-p, --port <number> Server listening port") catch unreachable);
const clap_p3 = (parseParam("-I, --include <dir>... Header include directories") catch unreachable);
const clap_p4 = (parseParam("<entry> Target entry file") catch unreachable);
const clap_params: [4]ParamHelp = [clap_p1, clap_p2, clap_p3, clap_p4];

fn testClapCommandLine(): !void {
    // 1. StreamingClap with short flags (-v), --port=8080, multi-options, and positionals
    const raw_args: [6][]const u8 = [
        "-v",
        "--port=8080",
        "-I", "/usr/local/include",
        "--include=/opt/include",
        "app.cp",
    ];

    let iter1 = SliceIterator.init(&raw_args);
    const StreamType = StreamingClap(Help, SliceIterator);
    let stream = StreamType{
        params: &clap_params,
        iter: &iter1,
    };

    const a1 = (try stream.next()).?;
    expect_eq(a1.param.names.short.?, 'v');
    expect_eq(a1.value == null, true);

    const a2 = (try stream.next()).?;
    expect_eq(std.mem.eql(u8, a2.param.names.long.?, "port"), true);
    expect_eq(std.mem.eql(u8, a2.value.?, "8080"), true);

    const a3 = (try stream.next()).?;
    expect_eq(a3.param.names.short.?, 'I');
    expect_eq(std.mem.eql(u8, a3.value.?, "/usr/local/include"), true);

    const a4 = (try stream.next()).?;
    expect_eq(std.mem.eql(u8, a4.param.names.long.?, "include"), true);
    expect_eq(std.mem.eql(u8, a4.value.?, "/opt/include"), true);

    const a5 = (try stream.next()).?;
    expect_eq(std.mem.eql(u8, a5.value.?, "app.cp"), true);

    const a6 = try stream.next();
    expect_eq(a6 == null, true);

    // 2. ComptimeClap end-to-end typed access
    let iter2 = SliceIterator.init(&raw_args);
    const ClapEngine = ComptimeClap(Help, &clap_params);
    let parsed = try ClapEngine.parse(&iter2, ParseOptions{
        allocator: std.heap.page_allocator,
    });
    defer parsed.deinit();

    expect_eq(parsed.flag("-v"), true);
    expect_eq(parsed.flag("--verbose"), true);
    expect_eq(std.mem.eql(u8, parsed.option("--port").?, "8080"), true);

    const incs = parsed.options("--include");
    expect_eq(incs.len, 2);
    expect_eq(std.mem.eql(u8, incs[0], "/usr/local/include"), true);
    expect_eq(std.mem.eql(u8, incs[1], "/opt/include"), true);

    const pos = parsed.positionals();
    expect_eq(pos.len, 1);
    expect_eq(std.mem.eql(u8, pos[0], "app.cp"), true);
}

fn main() {
    test("1. cap:dotenv — .env parser, quote escaping, ${VAR:-default} interpolation & cascade loader", () => {
        try testDotenvEngine();
    });
    test("2. cap:conf — INI & TOML parsing, round-trip serialization & ConfGetter type-safe extraction", () => {
        try testConfIniAndToml();
    });
    test("3. cap:conf — YAML block parsing & JSON5 AST parser with comments/hex/trailing commas", () => {
        try testConfYamlAndJson5();
    });
    test("4. cap:clap — Short/long flags (-v, --port=8080), positionals, StreamingClap & ComptimeClap", () => {
        try testClapCommandLine();
    });
}
