// Source: cp/cap/test/test_ini_production_deep.cp
// Deep production test suite for pure CP `cap:ini` & `cap.ini` dynamic mount.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { IniDoc, IniError, IniOptions, IniLineKind } from "cap:ini";
import { JsonParser, JsonValue } from "cap:json";
import cap from "cap";

fn expectTrue(cond: bool): ! {
    expect_eq(cond, true);
}

fn expectStrEq(actual: []const u8, expected: []const u8): ! {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn expectParseErr(alloc: std.mem.Allocator, src: []const u8, expected: anyerror): ! {
    let doc = IniDoc.parse(alloc, src) catch e => {
        expect_eq(e, expected);
        return;
    };
    doc.deinit();
    expect_eq(true, false);
}

pub fn main() {
    test("1. Systemd .service (Multi-value Environment/ExecStart & backslash continuation) and .gitconfig", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        const systemd_unit =
            "# Systemd service unit\n" ++
            "[Unit]\n" ++
            "Description=Nexus AI Worker\n" ++
            "After=network.target\n" ++
            "\n" ++
            "[Service]\n" ++
            "Type=simple\n" ++
            "Environment=\"RUST_LOG=info\"\n" ++
            "Environment=PORT=8080\n" ++
            "ExecStart=/usr/local/bin/nexus \\\n" ++
            "    --workers 8 \\\n" ++
            "    --bind 0.0.0.0:8080\n" ++
            "ExecStart=/usr/local/bin/nexus-healthcheck\n" ++
            "Restart=on-failure\n";

        let unit_doc: IniDoc = IniDoc.parse(alloc, systemd_unit) catch unreachable;
        defer unit_doc.deinit();

        expectTrue(unit_doc.hasSection("Unit"))?;
        expectTrue(unit_doc.hasSection("Service"))?;
        expectStrEq(unit_doc.get("Unit", "Description").?, "Nexus AI Worker")?;

        const envs: [][]const u8 = unit_doc.getAll(alloc, "Service", "Environment") catch unreachable;
        defer alloc.free(envs);
        expect_eq(envs.len, 2);
        expectStrEq(envs[0], "RUST_LOG=info")?;
        expectStrEq(envs[1], "PORT=8080")?;

        const execs: [][]const u8 = unit_doc.getAll(alloc, "Service", "ExecStart") catch unreachable;
        defer alloc.free(execs);
        expect_eq(execs.len, 2);
        expectStrEq(execs[0], "/usr/local/bin/nexus --workers 8 --bind 0.0.0.0:8080")?;
        expectStrEq(execs[1], "/usr/local/bin/nexus-healthcheck")?;

        const gitconfig =
            "[core]\n" ++
            "\trepositoryformatversion = 0\n" ++
            "\tfilemode = true\n" ++
            "[remote \"origin\"]\n" ++
            "\turl = https://github.com/cp-lang/cp.git\n" ++
            "\tfetch = +refs/heads/*:refs/remotes/origin/*\n";

        let git_doc: IniDoc = IniDoc.parse(alloc, gitconfig) catch unreachable;
        defer git_doc.deinit();

        expectTrue(git_doc.hasSection("remote \"origin\""))?;
        expectStrEq(git_doc.get("remote \"origin\"", "url").?, "https://github.com/cp-lang/cp.git")?;
        expect_eq(git_doc.getInt("core", "repositoryformatversion") catch unreachable, 0);
        expect_eq(git_doc.getBool("core", "filemode") catch unreachable, true);
    });

    test("2. Type-safe Readers (getInt/getFloat/getBool), Quoted #/; Preservation & Global Section", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const src =
            "; Global properties without section header\n" ++
            "global_name = \"cp#core;v1\"\n" ++
            "global_port: 9000\n" ++
            "\n" ++
            "[server]\n" ++
            "workers = 16\n" ++
            "hex_flags = 0xFF\n" ++
            "ratio = 3.14159\n" ++
            "enabled = yes\n" ++
            "tls = ON\n" ++
            "debug = false\n" ++
            "legacy = 0\n" ++
            "banner = 'hello # not comment ; still literal' # trailing comment\n" ++
            "escaped = \"line1\\nline2\\\"q\\\"\"\n";

        let doc: IniDoc = IniDoc.parse(alloc, src) catch unreachable;
        defer doc.deinit();

        expectTrue(doc.hasSection(""))?;
        expectStrEq(doc.get("", "global_name").?, "cp#core;v1")?;
        expect_eq(doc.getInt("", "global_port") catch unreachable, 9000);

        expect_eq(doc.getInt("server", "workers") catch unreachable, 16);
        expect_eq(doc.getInt("server", "hex_flags") catch unreachable, 255);
        expectTrue((doc.getFloat("server", "ratio") catch unreachable) > 3.14)?;
        expect_eq(doc.getBool("server", "enabled") catch unreachable, true);
        expect_eq(doc.getBool("server", "tls") catch unreachable, true);
        expect_eq(doc.getBool("server", "debug") catch unreachable, false);
        expect_eq(doc.getBool("server", "legacy") catch unreachable, false);
        expectStrEq(doc.get("server", "banner").?, "hello # not comment ; still literal")?;
        expectStrEq(doc.get("server", "escaped").?, "line1\nline2\"q\"")?;

        const secs: [][]const u8 = doc.sections(alloc) catch unreachable;
        defer alloc.free(secs);
        expect_eq(secs.len, 1);
        expectStrEq(secs[0], "server")?;

        const skeys: [][]const u8 = doc.keys(alloc, "server") catch unreachable;
        defer alloc.free(skeys);
        expect_eq(skeys.len, 9);
    });

    test("3. Lossless Incremental Editing (set/removeKey/removeSection) & stringify() Comment/Blank Preservation", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const orig =
            "# Top-level header comment\n" ++
            "; Second comment line\n" ++
            "\n" ++
            "[database]\n" ++
            "# Primary host\n" ++
            "host = 127.0.0.1\n" ++
            "port = 5432\n" ++
            "deprecated = old_val\n" ++
            "\n" ++
            "[temp_section]\n" ++
            "foo = bar\n" ++
            "\n" ++
            "[logging]\n" ++
            "; Keep this comment intact\n" ++
            "level = info\n";

        let doc: IniDoc = IniDoc.parse(alloc, orig) catch unreachable;
        defer doc.deinit();

        doc.set("database", "host", "10.0.0.42") catch unreachable;
        doc.set("database", "ssl", "true") catch unreachable;
        expect_eq(doc.removeKey("database", "deprecated"), true);
        expect_eq(doc.removeKey("database", "nonexistent"), false);
        expect_eq(doc.removeSection("temp_section"), true);
        doc.set("metrics", "port", "9100") catch unreachable;

        const out: []u8 = doc.stringify() catch unreachable;
        defer alloc.free(out);

        expectTrue(std.mem.indexOf(u8, out, "# Top-level header comment\n; Second comment line\n\n") != null)?;
        expectTrue(std.mem.indexOf(u8, out, "# Primary host\nhost = 10.0.0.42\nport = 5432\nssl = true\n") != null)?;
        expectTrue(std.mem.indexOf(u8, out, "deprecated") == null)?;
        expectTrue(std.mem.indexOf(u8, out, "[temp_section]") == null)?;
        expectTrue(std.mem.indexOf(u8, out, "[logging]\n; Keep this comment intact\nlevel = info\n") != null)?;
        expectTrue(std.mem.indexOf(u8, out, "[metrics]\nport = 9100\n") != null)?;

        // Round-trip re-parse verification
        let doc2: IniDoc = IniDoc.parse(alloc, out) catch unreachable;
        defer doc2.deinit();
        expectStrEq(doc2.get("database", "host").?, "10.0.0.42")?;
        expect_eq(doc2.getBool("database", "ssl") catch unreachable, true);
        expect_eq(doc2.getInt("metrics", "port") catch unreachable, 9100);
        expectStrEq(doc2.get("logging", "level").?, "info")?;
    });

    test("4. Malformed Syntax Guard (UnterminatedSection/MissingEquals/UnterminatedQuote) & toJson() via cap:json", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        expectParseErr(alloc, "[unclosed_section\nkey = val\n", IniError.UnterminatedSection)?;
        expectParseErr(alloc, "[sec]\nline_without_delimiter\n", IniError.MissingEquals)?;
        expectParseErr(alloc, "[sec]\nkey = \"unterminated\n", IniError.UnterminatedQuote)?;

        const valid =
            "app = nexus\n" ++
            "[http]\n" ++
            "port = 8080\n" ++
            "tag = alpha\n" ++
            "tag = beta\n";

        let doc: IniDoc = IniDoc.parse(alloc, valid) catch unreachable;
        defer doc.deinit();

        const json_bytes: []u8 = doc.toJson() catch unreachable;
        defer alloc.free(json_bytes);

        let json_arena: std.heap.ArenaAllocator = std.heap.ArenaAllocator.init(alloc);
        defer json_arena.deinit();
        const jv: JsonValue = JsonParser.parse(json_arena.allocator(), json_bytes) catch unreachable;

        expectStrEq(jv.get("app").?.asString().?, "nexus")?;
        expectStrEq(jv.get("http").?.get("port").?.asString().?, "8080")?;
        expectStrEq(jv.get("http").?.get("tag").?.getIndex(0).?.asString().?, "alpha")?;
        expectStrEq(jv.get("http").?.get("tag").?.getIndex(1).?.asString().?, "beta")?;
    });

    test("5. import cap from \"cap\"; cap.ini Dynamic Lazy Mounting Verification", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const cfg =
            "[service]\n" ++
            "name = \"cp-ini-hub\"\n" ++
            "workers = 12\n" ++
            "enabled = on\n";

        let doc: IniDoc = cap.ini.IniDoc.parse(alloc, cfg) catch unreachable;
        defer doc.deinit();

        expectStrEq(doc.get("service", "name").?, "cp-ini-hub")?;
        expect_eq(doc.getInt("service", "workers") catch unreachable, 12);
        expect_eq(doc.getBool("service", "enabled") catch unreachable, true);

        const out: []u8 = doc.stringify() catch unreachable;
        defer alloc.free(out);
        expectTrue(std.mem.indexOf(u8, out, "workers = 12") != null)?;
    });
}
