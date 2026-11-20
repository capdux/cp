// Source: cp/cap/test/test_yaml_production_deep.cp
// Deep Production Test Suite for cap:yaml
// Covers:
//   1. Complex GitHub Actions / Kubernetes YAML (---, nested mappings, on:, - name: sequence mappings, flow arrays/maps, comments)
//   2. Full scalar & block scalar coverage (|, |-, >, >-, single/double quote escapes, null/bool/int/float inference) & anchors/aliases (&ref / *ref)
//   3. Security & boundary guards: TabIndentationForbidden, DuplicateKey, UnterminatedString/Flow, MaxDepthExceeded (>64)
//   4. YamlSerializer.stringify round-trip equivalence (parse -> stringify -> parse) & toJson validated with cap:json
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.yaml`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    YamlError,
    YamlKind,
    YamlValue,
    YamlDoc,
    YamlParser,
    YamlSerializer,
} from "cap:yaml";
import cap from "cap";

fn expectParseError(alloc: std.mem.Allocator, input: []const u8, expected: YamlError) {
    let res = YamlParser.parse(alloc, input);
    switch (res) {
        err(e) => {
            expect_eq(e == expected, true);
        },
        ok => {
            let d = ok;
            d.deinit();
            expect_eq(false, true);
        },
    }
}

fn testWorkflowAndK8sManifest()! {
    const alloc = std.heap.page_allocator;

    const workflow_yaml =
        "\xEF\xBB\xBF" ++
        "# GitHub Actions CI Workflow\n" ++
        "---\n" ++
        "name: CP Compiler CI\n" ++
        "on: [push, pull_request]\n" ++
        "env: { RUST_BACKTRACE: \"1\", ZIG_OPT: ReleaseFast }\n" ++
        "jobs:\n" ++
        "  build:\n" ++
        "    runs-on: ubuntu-latest # runner image\n" ++
        "    timeout-minutes: 30\n" ++
        "    steps:\n" ++
        "      - name: Checkout repository\n" ++
        "        uses: actions/checkout@v4\n" ++
        "        with:\n" ++
        "          fetch-depth: 1\n" ++
        "      - name: Run unit tests\n" ++
        "        run: cap test\n" ++
        "...\n";

    let doc = YamlParser.parse(alloc, workflow_yaml) catch unreachable;
    defer doc.deinit();

    expect_eq(std.mem.eql(u8, doc.get("name").?.asString().?, "CP Compiler CI"), true);
    expect_eq(doc.getPath("on.0").?.asString().?[0], 'p');
    expect_eq(std.mem.eql(u8, doc.getPath("on.1").?.asString().?, "pull_request"), true);
    expect_eq(std.mem.eql(u8, doc.getPath("env.RUST_BACKTRACE").?.asString().?, "1"), true);
    expect_eq(std.mem.eql(u8, doc.getPath("env.ZIG_OPT").?.asString().?, "ReleaseFast"), true);
    expect_eq(std.mem.eql(u8, doc.getPath("jobs.build.runs-on").?.asString().?, "ubuntu-latest"), true);
    expect_eq(doc.getPath("jobs.build.timeout-minutes").?.asInt().?, 30 as i64);
    expect_eq(std.mem.eql(u8, doc.getPath("jobs.build.steps.0.name").?.asString().?, "Checkout repository"), true);
    expect_eq(doc.getPath("jobs.build.steps.0.with.fetch-depth").?.asInt().?, 1 as i64);
    expect_eq(std.mem.eql(u8, doc.getPath("jobs.build.steps.1.run").?.asString().?, "cap test"), true);

    const k8s_yaml =
        "apiVersion: apps/v1\n" ++
        "kind: Deployment\n" ++
        "metadata:\n" ++
        "  name: agent-gateway\n" ++
        "  labels: { app: gateway, tier: backend }\n" ++
        "spec:\n" ++
        "  replicas: 3\n" ++
        "  containers:\n" ++
        "  - name: proxy\n" ++
        "    image: cp/gateway:1.2.0\n" ++
        "    ports: [8080, 8443]\n";

    let k8s = YamlParser.parse(alloc, k8s_yaml) catch unreachable;
    defer k8s.deinit();

    expect_eq(std.mem.eql(u8, k8s.get("kind").?.asString().?, "Deployment"), true);
    expect_eq(std.mem.eql(u8, k8s.getPath("metadata.labels.app").?.asString().?, "gateway"), true);
    expect_eq(k8s.getPath("spec.replicas").?.asInt().?, 3 as i64);
    expect_eq(std.mem.eql(u8, k8s.getPath("spec.containers.0.name").?.asString().?, "proxy"), true);
    expect_eq(k8s.getPath("spec.containers.0.ports.1").?.asInt().?, 8443 as i64);
}

fn testScalarsBlockScalarsAndAnchors()! {
    const alloc = std.heap.page_allocator;

    const yaml_input =
        "n1: null\n" ++
        "n2: ~\n" ++
        "b1: true\n" ++
        "b2: FALSE\n" ++
        "i_dec: -42\n" ++
        "i_hex: 0x2A\n" ++
        "f_val: 3.1415\n" ++
        "dq: \"line1\\n\\t\\\"quoted\\\"\\\\\"\n" ++
        "sq: 'it''s literal \\n'\n" ++
        "lit_keep: |\n" ++
        "  alpha\n" ++
        "  beta\n" ++
        "lit_strip: |-\n" ++
        "  gamma\n" ++
        "  delta\n" ++
        "fold_clip: >\n" ++
        "  hello\n" ++
        "  world\n" ++
        "fold_strip: >-\n" ++
        "  foo\n" ++
        "  bar\n" ++
        "defaults: &base_cfg\n" ++
        "  retries: 5\n" ++
        "  region: us-east\n" ++
        "prod: *base_cfg\n" ++
        "tag_anchor: &my_tag v1.0\n" ++
        "tag_alias: *my_tag\n";

    let doc = YamlParser.parse(alloc, yaml_input) catch unreachable;
    defer doc.deinit();

    expect_eq(doc.get("n1").?.isNull(), true);
    expect_eq(doc.get("n2").?.isNull(), true);
    expect_eq(doc.get("b1").?.asBool().?, true);
    expect_eq(doc.get("b2").?.asBool().?, false);
    expect_eq(doc.get("i_dec").?.asInt().?, -42 as i64);
    expect_eq(doc.get("i_hex").?.asInt().?, 42 as i64);
    expect_eq(doc.get("f_val").?.isFloat(), true);
    expect_eq(doc.get("f_val").?.asFloat().? > 3.14 && doc.get("f_val").?.asFloat().? < 3.15, true);

    expect_eq(std.mem.eql(u8, doc.get("dq").?.asString().?, "line1\n\t\"quoted\"\\"), true);
    expect_eq(std.mem.eql(u8, doc.get("sq").?.asString().?, "it's literal \\n"), true);

    expect_eq(std.mem.eql(u8, doc.get("lit_keep").?.asString().?, "alpha\nbeta\n"), true);
    expect_eq(std.mem.eql(u8, doc.get("lit_strip").?.asString().?, "gamma\ndelta"), true);
    expect_eq(std.mem.eql(u8, doc.get("fold_clip").?.asString().?, "hello world\n"), true);
    expect_eq(std.mem.eql(u8, doc.get("fold_strip").?.asString().?, "foo bar"), true);

    expect_eq(doc.getPath("prod.retries").?.asInt().?, 5 as i64);
    expect_eq(std.mem.eql(u8, doc.getPath("prod.region").?.asString().?, "us-east"), true);
    expect_eq(std.mem.eql(u8, doc.get("tag_alias").?.asString().?, "v1.0"), true);
}

fn testSecurityAndBoundaryGuards()! {
    const alloc = std.heap.page_allocator;

    // 1. Tab indentation forbidden
    expectParseError(alloc, "root:\n\tchild: 1\n", YamlError.TabIndentationForbidden);

    // 2. Duplicate keys in block mapping and flow mapping
    expectParseError(alloc, "host: a\nhost: b\n", YamlError.DuplicateKey);
    expectParseError(alloc, "obj: { x: 1, x: 2 }\n", YamlError.DuplicateKey);

    // 3. Unterminated string and unterminated flow collection
    expectParseError(alloc, "msg: \"unclosed\n", YamlError.UnterminatedString);
    expectParseError(alloc, "msg: 'unclosed\n", YamlError.UnterminatedString);
    expectParseError(alloc, "arr: [1, 2, 3\n", YamlError.UnterminatedFlow);

    // 4. Invalid alias
    expectParseError(alloc, "ref: *nonexistent\n", YamlError.InvalidAlias);

    // 5. MaxDepthExceeded (>64 nested flow arrays)
    let deep_buf: std.ArrayList(u8) = .empty;
    defer deep_buf.deinit(alloc);
    let i: usize = 0;
    while (i < 70) {
        deep_buf.append(alloc, '[') catch unreachable;
        i += 1;
    }
    deep_buf.append(alloc, '1') catch unreachable;
    i = 0;
    while (i < 70) {
        deep_buf.append(alloc, ']') catch unreachable;
        i += 1;
    }
    expectParseError(alloc, deep_buf.items, YamlError.MaxDepthExceeded);
}

fn testSerializerRoundTripAndToJson()! {
    const alloc = std.heap.page_allocator;

    const src_yaml =
        "service: api-server\n" ++
        "enabled: true\n" ++
        "port: 9090\n" ++
        "ratio: 0.75\n" ++
        "empty_note: null\n" ++
        "tags: [\"v1\", \"prod\"]\n" ++
        "routes:\n" ++
        "  - path: /health\n" ++
        "    method: GET\n" ++
        "  - path: /v1/chat\n" ++
        "    method: POST\n";

    let doc1 = YamlParser.parse(alloc, src_yaml) catch unreachable;
    defer doc1.deinit();

    const yaml_out = YamlSerializer.stringify(alloc, doc1) catch unreachable;
    defer alloc.free(yaml_out);

    let doc2 = YamlParser.parse(alloc, yaml_out) catch unreachable;
    defer doc2.deinit();

    expect_eq(std.mem.eql(u8, doc2.get("service").?.asString().?, "api-server"), true);
    expect_eq(doc2.get("enabled").?.asBool().?, true);
    expect_eq(doc2.get("port").?.asInt().?, 9090 as i64);
    expect_eq(doc2.get("empty_note").?.isNull(), true);
    expect_eq(std.mem.eql(u8, doc2.getPath("tags.1").?.asString().?, "prod"), true);
    expect_eq(std.mem.eql(u8, doc2.getPath("routes.0.path").?.asString().?, "/health"), true);
    expect_eq(std.mem.eql(u8, doc2.getPath("routes.1.method").?.asString().?, "POST"), true);

    const json_str = YamlSerializer.toJson(alloc, doc1) catch unreachable;
    defer alloc.free(json_str);

    let json_val = JSON.parse(json_str) catch unreachable;
    expect_eq(json_val.isObject(), true);
    expect_eq(std.mem.eql(u8, json_val.get("service").?.asString().?, "api-server"), true);
    expect_eq(json_val.get("enabled").?.asBool().?, true);
    expect_eq(json_val.get("routes").?.getIndex(1).?.get("method").?.asString().?[0], 'P');
}

fn testDynamicCapYamlMount()! {
    const alloc = std.heap.page_allocator;

    const compose_yaml =
        "version: \"3.9\"\n" ++
        "services:\n" ++
        "  redis:\n" ++
        "    image: redis:7-alpine\n" ++
        "    ports:\n" ++
        "      - \"6379:6379\"\n";

    let doc = cap.yaml.YamlParser.parse(alloc, compose_yaml) catch unreachable;
    defer doc.deinit();

    expect_eq(std.mem.eql(u8, doc.getPath("services.redis.image").?.asString().?, "redis:7-alpine"), true);
    expect_eq(std.mem.eql(u8, doc.getPath("services.redis.ports.0").?.asString().?, "6379:6379"), true);

    const json_out = cap.yaml.YamlSerializer.toJson(alloc, doc) catch unreachable;
    defer alloc.free(json_out);
    expect_eq(std.mem.indexOf(u8, json_out, "\"redis:7-alpine\"") != null, true);
}

test("cap:yaml GitHub Actions & Kubernetes manifest parsing") {
    testWorkflowAndK8sManifest() catch unreachable;
}

test("cap:yaml scalars, block scalars, quotes & anchors/aliases") {
    testScalarsBlockScalarsAndAnchors() catch unreachable;
}

test("cap:yaml security & boundary guards (tab, duplicate key, unterminated, max depth)") {
    testSecurityAndBoundaryGuards() catch unreachable;
}

test("cap:yaml YamlSerializer stringify round-trip & toJson with JSON.parse") {
    testSerializerRoundTripAndToJson() catch unreachable;
}

test("cap:yaml dynamic lazy mounting via cap.yaml") {
    testDynamicCapYamlMount() catch unreachable;
}
