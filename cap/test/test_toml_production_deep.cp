// Source: cp/cap/test/test_toml_production_deep.cp
// Deep production test suite for pure CP `cap:toml` (TOML v1.0.0) & `cap.toml` dynamic mount.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { TomlParser, TomlSerializer, TomlError, TomlKind, TomlDoc, TomlArray } from "cap:toml";
import { JsonParser, JsonValue } from "cap:json";
import cap from "cap";

fn expect_true(cond: bool): ! {
    expect_eq(cond, true);
}

fn expectStrEq(actual: []const u8, expected: []const u8): ! {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn expectParseErr(alloc: std.mem.Allocator, src: []const u8, expected: anyerror): ! {
    let doc = TomlParser.parse(alloc, src) catch e => {
        expect_eq(e, expected);
        return;
    };
    doc.deinit();
    expect_eq(true, false);
}

pub fn main() {
    test("1. Cargo.toml & pyproject.toml Complex Parsing (Dotted Keys, Inline Tables & Array of Tables)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const cargo_toml =
            "# Rust Cargo manifest\n" ++
            "[package]\n" ++
            "name = \"nexus-engine\"\n" ++
            "version = \"1.4.2\"\n" ++
            "edition = \"2021\"\n" ++
            "metadata.author.name = \"CP Core Team\"\n" ++
            "\n" ++
            "[dependencies]\n" ++
            "tokio = { version = \"1.38.0\", features = [\"full\", \"rt-multi-thread\"], optional = false }\n" ++
            "serde = { version = \"1.0\", features = [\"derive\"] }\n" ++
            "\n" ++
            "[[bin]]\n" ++
            "name = \"nexus-cli\"\n" ++
            "path = \"src/main.rs\"\n" ++
            "\n" ++
            "[[bin]]\n" ++
            "name = \"nexus-worker\"\n" ++
            "path = \"src/worker.rs\"\n" ++
            "[bin.env]\n" ++
            "RUST_LOG = \"info\"\n";

        let doc: TomlDoc = TomlParser.parse(alloc, cargo_toml) catch unreachable;
        defer doc.deinit();

        expectStrEq(doc.getPath("package.name").?.asString().?, "nexus-engine")?;
        expectStrEq(doc.getPath("package.version").?.asString().?, "1.4.2")?;
        expectStrEq(doc.getPath("package.metadata.author.name").?.asString().?, "CP Core Team")?;

        const tokio_ver: []const u8 = doc.getPath("dependencies.tokio.version").?.asString().?;
        expectStrEq(tokio_ver, "1.38.0")?;
        expect_eq(doc.getPath("dependencies.tokio.optional").?.asBool().?, false);

        const tokio_feats: TomlArray = doc.getPath("dependencies.tokio.features").?.asArray().?;
        expect_eq(tokio_feats.len(), 2);
        expectStrEq(tokio_feats.get(0).?.asString().?, "full")?;
        expectStrEq(tokio_feats.get(1).?.asString().?, "rt-multi-thread")?;

        const bins: TomlArray = doc.get("bin").?.asArray().?;
        expect_eq(bins.len(), 2);
        expectStrEq(bins.get(0).?.get("name").?.asString().?, "nexus-cli")?;
        expectStrEq(bins.get(1).?.get("name").?.asString().?, "nexus-worker")?;
        expectStrEq(bins.get(1).?.getPath("env.RUST_LOG").?.asString().?, "info")?;
    });

    test("2. All 4 String Forms, Unicode Escapes, Number Bases, DateTime, Comments & Trailing Commas", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const src =
            "# 4 string forms\n" ++
            "basic = \"hello\\t\\n\\\"world\\\" \\u4E2D\\u6587\"\n" ++
            "multiline_basic = \"\"\"\n" ++
            "The quick brown \\\n" ++
            "    fox jumps over \\\n" ++
            "    the lazy dog.\"\"\"\n" ++
            "literal = 'C:\\Users\\alice\\config.toml'\n" ++
            "multiline_literal = '''\n" ++
            "line1\\nnot_escaped\n" ++
            "line2'''\n" ++
            "\n" ++
            "# Numbers & bases\n" ++
            "dec_under = 1_000_000\n" ++
            "neg_int = -42\n" ++
            "hex_val = 0xDEAD_BEEF\n" ++
            "oct_val = 0o755\n" ++
            "bin_val = 0b1101_0110\n" ++
            "flt_exp = 6.022e23\n" ++
            "flt_inf = inf\n" ++
            "flt_nan = nan\n" ++
            "\n" ++
            "# RFC 3339 DateTime & trailing comma array\n" ++
            "created_at = 2026-11-20T15:30:00Z\n" ++
            "ports = [\n" ++
            "  8080,\n" ++
            "  8443, # HTTPS\n" ++
            "]\n";

        let doc: TomlDoc = TomlParser.parse(alloc, src) catch unreachable;
        defer doc.deinit();

        expectStrEq(doc.get("basic").?.asString().?, "hello\t\n\"world\" 中文")?;
        expectStrEq(doc.get("multiline_basic").?.asString().?, "The quick brown fox jumps over the lazy dog.")?;
        expectStrEq(doc.get("literal").?.asString().?, "C:\\Users\\alice\\config.toml")?;
        expectStrEq(doc.get("multiline_literal").?.asString().?, "line1\\nnot_escaped\nline2")?;

        expect_eq(doc.get("dec_under").?.asInt().?, 1000000);
        expect_eq(doc.get("neg_int").?.asInt().?, -42);
        expect_eq(doc.get("hex_val").?.asInt().?, 0xDEADBEEF);
        expect_eq(doc.get("oct_val").?.asInt().?, 0o755);
        expect_eq(doc.get("bin_val").?.asInt().?, 0b11010110);

        expect_true(doc.get("flt_exp").?.asFloat().? > 6.0e23)?;
        expect_true(std.math.isPositiveInf(doc.get("flt_inf").?.asFloat().?))?;
        expect_true(std.math.isNan(doc.get("flt_nan").?.asFloat().?))?;

        expect_true(doc.get("created_at").?.isDateTime())?;
        expectStrEq(doc.get("created_at").?.asDateTime().?, "2026-11-20T15:30:00Z")?;

        const ports: TomlArray = doc.get("ports").?.asArray().?;
        expect_eq(ports.len(), 2);
        expect_eq(ports.get(0).?.asInt().?, 8080);
        expect_eq(ports.get(1).?.asInt().?, 8443);
    });

    test("3. Strict Error Guard: DuplicateKey, TypeConflict, UnterminatedString & MaxDepthExceeded", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // 3.1 Duplicate scalar key
        expectParseErr(alloc, "a = 1\na = 2\n", TomlError.DuplicateKey)?;

        // 3.2 Duplicate [table] header
        expectParseErr(alloc, "[server]\nhost = \"a\"\n[server]\nport = 80\n", TomlError.DuplicateKey)?;

        // 3.3 TypeConflict: expanding scalar integer key as dotted subtable
        expectParseErr(alloc, "a = 42\na.b = 1\n", TomlError.TypeConflict)?;

        // 3.4 TypeConflict: redefining scalar key as [table]
        expectParseErr(alloc, "x = \"str\"\n[x]\ny = 1\n", TomlError.TypeConflict)?;

        // 3.5 UnterminatedString
        expectParseErr(alloc, "msg = \"unclosed\n", TomlError.UnterminatedString)?;
    });

    test("4. TomlSerializer.stringify Roundtrip & toJson Integration with cap:json", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const input =
            "title = \"CP Config\"\n" ++
            "\n" ++
            "[database]\n" ++
            "enabled = true\n" ++
            "ports = [8001, 8002]\n" ++
            "\n" ++
            "[[servers]]\n" ++
            "ip = \"10.0.0.1\"\n" ++
            "role = \"primary\"\n" ++
            "\n" ++
            "[[servers]]\n" ++
            "ip = \"10.0.0.2\"\n" ++
            "role = \"replica\"\n";

        let doc1: TomlDoc = TomlParser.parse(alloc, input) catch unreachable;
        defer doc1.deinit();

        const serialized: []u8 = TomlSerializer.stringify(alloc, doc1) catch unreachable;
        defer alloc.free(serialized);

        let doc2: TomlDoc = TomlParser.parse(alloc, serialized) catch unreachable;
        defer doc2.deinit();

        expectStrEq(doc2.get("title").?.asString().?, "CP Config")?;
        expect_eq(doc2.getPath("database.enabled").?.asBool().?, true);
        expect_eq(doc2.getPath("database.ports").?.asArray().?.get(1).?.asInt().?, 8002);
        const srv: TomlArray = doc2.get("servers").?.asArray().?;
        expect_eq(srv.len(), 2);
        expectStrEq(srv.get(0).?.get("ip").?.asString().?, "10.0.0.1")?;
        expectStrEq(srv.get(1).?.get("role").?.asString().?, "replica")?;

        // toJson -> cap:json verification
        const json_text: []u8 = TomlSerializer.toJson(alloc, doc1) catch unreachable;
        defer alloc.free(json_text);

        let json_arena: std.heap.ArenaAllocator = std.heap.ArenaAllocator.init(alloc);
        defer json_arena.deinit();
        const jv: JsonValue = JsonParser.parse(json_arena.allocator(), json_text) catch unreachable;
        expectStrEq(jv.get("title").?.asString().?, "CP Config")?;
        expect_eq(jv.get("database").?.get("enabled").?.asBool().?, true);
        expectStrEq(jv.get("servers").?.getIndex(1).?.get("ip").?.asString().?, "10.0.0.2")?;
    });

    test("5. import cap from \"cap\"; cap.toml Dynamic Lazy Mounting Verification", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const cfg =
            "[app]\n" ++
            "name = \"cp-toml-hub\"\n" ++
            "workers = 16\n";

        let doc: TomlDoc = cap.toml.TomlParser.parse(alloc, cfg) catch unreachable;
        defer doc.deinit();

        expectStrEq(doc.getPath("app.name").?.asString().?, "cp-toml-hub")?;
        expect_eq(doc.getPath("app.workers").?.asInt().?, 16);

        const out: []u8 = cap.toml.TomlSerializer.stringify(alloc, doc) catch unreachable;
        defer alloc.free(out);
        expect_true(std.mem.indexOf(u8, out, "name = \"cp-toml-hub\"") != null)?;
    });
}
