// Source: cp/cap/test/test_interchange_production_deep.cp
// Deep production test suite for pure CP `cap:interchange` & `cap.interchange` dynamic mount.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    INTERCHANGE_MAX_DEPTH,
    InterchangeFormat,
    InterchangeError,
    InterchangeValue,
    Interchange,
    detectFormat,
    parse,
    stringify,
} from "cap:interchange";
import cap from "cap";

fn expectTrue(cond: bool) {
    expect_eq(cond, true);
}

fn expectStrEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn expectParseErr(
    alloc: std.mem.Allocator,
    input: []const u8,
    fmt: InterchangeFormat,
    expected: InterchangeError,
) {
    let res = parse(alloc, input, fmt);
    switch (res) {
        err(e) => {
            expect_eq(e == expected, true);
        },
        ok => {
            let v = ok;
            v.deinit(alloc);
            expect_eq(false, true);
        },
    }
}

test("1. Format detection and JSON / JSONC / JSON5 parsing") {
    const alloc = std.testing.allocator;

    expect_eq(detectFormat("config.json").? == InterchangeFormat.Json, true);
    expect_eq(detectFormat("tsconfig.JSONC").? == InterchangeFormat.Jsonc, true);
    expect_eq(detectFormat(".json5").? == InterchangeFormat.Json5, true);
    expect_eq(detectFormat("Cargo.toml").? == InterchangeFormat.Toml, true);
    expect_eq(detectFormat("deploy.yml").? == InterchangeFormat.Yaml, true);
    expect_eq(detectFormat("values.yaml").? == InterchangeFormat.Yaml, true);
    expect_eq(detectFormat("unknown.xml") == null, true);

    const json_src = "{\"app\":\"cp\",\"v\":1,\"ok\":true,\"items\":[1,2,3]}";
    let jv = parse(alloc, json_src, .Json) catch unreachable;
    defer jv.deinit(alloc);
    expectStrEq(jv.getPath("app").?.asString().?, "cp");
    expect_eq(jv.getPath("v").?.asInt().?, 1);
    expect_eq(jv.getPath("ok").?.asBool().?, true);
    expect_eq(jv.getPath("items.2").?.asInt().?, 3);

    const jsonc_src =
        "// line comment\n" ++
        "{\n" ++
        "  /* block\n" ++
        "     comment */\n" ++
        "  \"compilerOptions\": {\n" ++
        "    \"target\": \"esnext\",\n" ++
        "  },\n" ++
        "}\n";
    let jc = parse(alloc, jsonc_src, .Jsonc) catch unreachable;
    defer jc.deinit(alloc);
    expectStrEq(jc.getPath("compilerOptions.target").?.asString().?, "esnext");

    const json5_src =
        "// JSON5 full features\n" ++
        "{\n" ++
        "  unquoted_key: 'single-quoted\\x20str',\n" ++
        "  $dollar: 0x10,\n" ++
        "  leading_dot: .5,\n" ++
        "  trailing_dot: 5.,\n" ++
        "  plus_num: +42,\n" ++
        "  list: [1, 2,],\n" ++
        "}\n";
    let j5 = parse(alloc, json5_src, .Json5) catch unreachable;
    defer j5.deinit(alloc);
    expectStrEq(j5.get("unquoted_key").?.asString().?, "single-quoted str");
    expect_eq(j5.get("$dollar").?.asInt().?, 16);
    expect_eq(j5.get("leading_dot").?.asFloat().?, 0.5);
    expect_eq(j5.get("trailing_dot").?.asFloat().?, 5.0);
    expect_eq(j5.get("plus_num").?.asInt().?, 42);
    expect_eq(j5.getPath("list.1").?.asInt().?, 2);
}

test("2. TOML / YAML parsing and lossless cross-format round-trip conversion") {
    const alloc = std.testing.allocator;

    const toml_src =
        "# Service config\n" ++
        "name = \"gateway\"\n" ++
        "port = 8080\n" ++
        "enabled = true\n" ++
        "\n" ++
        "[database]\n" ++
        "host = \"127.0.0.1\"\n" ++
        "pool = 16\n" ++
        "tags = [\"primary\", \"eu-west\"]\n";

    const json_from_toml = Interchange.convert(alloc, toml_src, .Toml, .Json) catch unreachable;
    defer alloc.free(json_from_toml);
    const yaml_from_toml = Interchange.convert(alloc, toml_src, .Toml, .Yaml) catch unreachable;
    defer alloc.free(yaml_from_toml);

    let v_orig = parse(alloc, toml_src, .Toml) catch unreachable;
    defer v_orig.deinit(alloc);
    let v_json = parse(alloc, json_from_toml, .Json) catch unreachable;
    defer v_json.deinit(alloc);
    let v_yaml = parse(alloc, yaml_from_toml, .Yaml) catch unreachable;
    defer v_yaml.deinit(alloc);

    expectTrue(v_orig.equals(v_json));
    expectTrue(v_orig.equals(v_yaml));

    const toml_roundtrip = Interchange.convert(alloc, yaml_from_toml, .Yaml, .Toml) catch unreachable;
    defer alloc.free(toml_roundtrip);
    let v_toml2 = parse(alloc, toml_roundtrip, .Toml) catch unreachable;
    defer v_toml2.deinit(alloc);
    expectTrue(v_orig.equals(v_toml2));
}

test("3. Nesting depth limit (>64) and malformed syntax error guards") {
    const alloc = std.testing.allocator;

    // Build 66-level nested JSON array -> MaxDepthExceeded
    let deep_buf: std.ArrayList(u8) = .empty;
    defer deep_buf.deinit(alloc);
    let i: usize = 0;
    while (i <= INTERCHANGE_MAX_DEPTH + 1) : (i += 1) {
        deep_buf.append(alloc, '[') catch unreachable;
    }
    deep_buf.append(alloc, '1') catch unreachable;
    i = 0;
    while (i <= INTERCHANGE_MAX_DEPTH + 1) : (i += 1) {
        deep_buf.append(alloc, ']') catch unreachable;
    }
    expectParseErr(alloc, deep_buf.items, .Json, InterchangeError.MaxDepthExceeded);

    // Strict JSON rejects comments, trailing commas, and single quotes
    expectParseErr(alloc, "// comment\n{}", .Json, InterchangeError.UnexpectedChar);
    expectParseErr(alloc, "[1, 2,]", .Json, InterchangeError.UnexpectedChar);
    expectParseErr(alloc, "'hello'", .Json, InterchangeError.UnexpectedChar);

    // JSON5 / JSONC error guards
    expectParseErr(alloc, "/* unclosed", .Jsonc, InterchangeError.UnterminatedComment);
    expectParseErr(alloc, "{a: 'unterminated}", .Json5, InterchangeError.UnterminatedString);
    expectParseErr(alloc, "{\"a\":1} extra", .Json, InterchangeError.TrailingData);

    // YAML tab indentation & TOML duplicate key guards
    expectParseErr(alloc, "root:\n\tchild: 1\n", .Yaml, InterchangeError.TabIndentationForbidden);
    expectParseErr(alloc, "a = 1\na = 2\n", .Toml, InterchangeError.DuplicateKey);
}

test("4. Complex object stringify formatting (compact JSON, pretty JSON, TOML, YAML)") {
    const alloc = std.testing.allocator;

    const src =
        "{\n" ++
        "  service: 'auth-api',\n" ++
        "  replicas: 3,\n" ++
        "  tls: { enabled: true, min_version: '1.3' },\n" ++
        "  routes: [{ path: '/login', rate: 100 }, { path: '/health', rate: 500 }],\n" ++
        "}\n";

    let doc = parse(alloc, src, .Json5) catch unreachable;
    defer doc.deinit(alloc);

    const compact = stringify(alloc, doc, .Json) catch unreachable;
    defer alloc.free(compact);
    expectTrue(std.mem.indexOfScalar(u8, compact, '\n') == null);

    const pretty = Interchange.stringifyPretty(alloc, doc, .Json, true) catch unreachable;
    defer alloc.free(pretty);
    expectTrue(std.mem.indexOf(u8, pretty, "\n  \"service\": \"auth-api\"") != null);

    const toml_out = stringify(alloc, doc, .Toml) catch unreachable;
    defer alloc.free(toml_out);
    expectTrue(std.mem.indexOf(u8, toml_out, "[tls]") != null);
    expectTrue(std.mem.indexOf(u8, toml_out, "[[routes]]") != null);

    const yaml_out = stringify(alloc, doc, .Yaml) catch unreachable;
    defer alloc.free(yaml_out);
    expectTrue(std.mem.indexOf(u8, yaml_out, "service: auth-api") != null);
    expectTrue(std.mem.indexOf(u8, yaml_out, "path: /login") != null);

    let re_toml = parse(alloc, toml_out, .Toml) catch unreachable;
    defer re_toml.deinit(alloc);
    let re_yaml = parse(alloc, yaml_out, .Yaml) catch unreachable;
    defer re_yaml.deinit(alloc);
    expectTrue(doc.equals(re_toml));
    expectTrue(doc.equals(re_yaml));
}

test("5. cap.interchange dynamic lazy mount and std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;

    const fmt = cap.interchange.detectFormat("settings.json5").?;
    expect_eq(fmt == InterchangeFormat.Json5, true);

    const cfg = "{ app: 'nexus', workers: 0x4, ratio: .75, flags: ['fast', 'safe',], }";
    let val = cap.interchange.Interchange.parseAuto(alloc, cfg, "settings.json5") catch unreachable;
    defer val.deinit(alloc);

    expectStrEq(val.get("app").?.asString().?, "nexus");
    expect_eq(val.get("workers").?.asInt().?, 4);
    expect_eq(val.get("ratio").?.asFloat().?, 0.75);

    const yaml_str = cap.interchange.stringify(alloc, val, .Yaml) catch unreachable;
    defer alloc.free(yaml_str);

    let val_clone = val.clone(alloc) catch unreachable;
    defer val_clone.deinit(alloc);
    expectTrue(val.equals(val_clone));
}
