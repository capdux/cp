// Source: cp/cap/test/test_makefile_parse_production_deep.cp (Production deep tests for cap:makefile_parse)

import { test, expect_eq } from "cap:test";
import {
    MakeAssignOp,
    MakeVar,
    MakeRule,
    MakeRuleIter,
    MakeVarIter,
    isPhonyTarget,
} from "cap:makefile_parse";

test("makefile_parse: MakeVarIter parses =, :=, ::=, ?=, += and export/override prefixes") {
    const mk =
        "\xEF\xBB\xBF" ++
        "# build config\n" ++
        "CC = gcc\n" ++
        "CXX := clang++ # inline comment\n" ++
        "BUILD_TYPE ?= release\n" ++
        "CFLAGS += -O2 -Wall\n" ++
        "export PATH ::= /usr/local/bin:$(PATH)\n" ++
        "all: build\n" ++
        "\tFOO=bar ./run\n";

    let it = MakeVarIter.init(mk);

    const v1 = it.next().?;
    expect_eq(std.mem.eql(u8, v1.name, "CC"), true);
    expect_eq(v1.op == .recursive, true);
    expect_eq(std.mem.eql(u8, v1.value, "gcc"), true);
    expect_eq(v1.line_no, 2);

    const v2 = it.next().?;
    expect_eq(std.mem.eql(u8, v2.name, "CXX"), true);
    expect_eq(v2.op == .simple, true);
    expect_eq(std.mem.eql(u8, v2.value, "clang++"), true);
    expect_eq(std.mem.eql(u8, v2.op.asStr(), ":="), true);

    const v3 = it.next().?;
    expect_eq(std.mem.eql(u8, v3.name, "BUILD_TYPE"), true);
    expect_eq(v3.op == .conditional, true);
    expect_eq(std.mem.eql(u8, v3.value, "release"), true);

    const v4 = it.next().?;
    expect_eq(std.mem.eql(u8, v4.name, "CFLAGS"), true);
    expect_eq(v4.op == .append, true);
    expect_eq(std.mem.eql(u8, v4.value, "-O2 -Wall"), true);

    const v5 = it.next().?;
    expect_eq(std.mem.eql(u8, v5.name, "PATH"), true);
    expect_eq(v5.op == .simple, true);

    // 跳过 all: build 与 \t recipe 中的 FOO=bar
    expect_eq(it.next() == null, true);
}

test("makefile_parse: MakeRuleIter extracts targets/deps while skipping vars, recipes and comments") {
    const mk =
        "CC := clang\n" ++
        ".PHONY: all test\n" ++
        "all: build test # run all\n" ++
        "\t@echo done: not a rule\n" ++
        "build: main.o util.o ; $(CC) -o app main.o util.o\n" ++
        "clean::\n" ++
        "\trm -rf bin/\n";

    let it = MakeRuleIter.init(mk);

    const r1 = it.next().?;
    expect_eq(std.mem.eql(u8, r1.target, ".PHONY"), true);
    expect_eq(std.mem.eql(u8, r1.deps_raw, "all test"), true);
    expect_eq(r1.line_no, 2);

    const r2 = it.next().?;
    expect_eq(std.mem.eql(u8, r2.target, "all"), true);
    expect_eq(std.mem.eql(u8, r2.deps_raw, "build test"), true);
    expect_eq(r2.line_no, 3);

    const r3 = it.next().?;
    expect_eq(std.mem.eql(u8, r3.target, "build"), true);
    expect_eq(std.mem.eql(u8, r3.deps_raw, "main.o util.o"), true);
    expect_eq(r3.line_no, 5);

    const r4 = it.next().?;
    expect_eq(std.mem.eql(u8, r4.target, "clean"), true);
    expect_eq(r4.deps_raw.len, 0);
    expect_eq(r4.line_no, 6);

    expect_eq(it.next() == null, true);
}

test("makefile_parse: isPhonyTarget scans multiple .PHONY declarations") {
    const mk =
        ".PHONY: all build\n" ++
        "all: build\n" ++
        ".PHONY: test clean fmt\n" ++
        "clean:\n" ++
        "\trm -rf dist\n";

    expect_eq(isPhonyTarget(mk, "all"), true);
    expect_eq(isPhonyTarget(mk, "build"), true);
    expect_eq(isPhonyTarget(mk, "test"), true);
    expect_eq(isPhonyTarget(mk, "clean"), true);
    expect_eq(isPhonyTarget(mk, "fmt"), true);
    expect_eq(isPhonyTarget(mk, "dist"), false);
    expect_eq(isPhonyTarget(mk, ""), false);
}

test("makefile_parse: CRLF line endings and escaped hash in values") {
    const mk = "TAG := v1\\#beta\r\nbuild: src/a.cp\r\n\techo ok\r\n";

    let vit = MakeVarIter.init(mk);
    const v = vit.next().?;
    expect_eq(std.mem.eql(u8, v.name, "TAG"), true);
    expect_eq(std.mem.eql(u8, v.value, "v1\\#beta"), true);

    let rit = MakeRuleIter.init(mk);
    const r = rit.next().?;
    expect_eq(std.mem.eql(u8, r.target, "build"), true);
    expect_eq(std.mem.eql(u8, r.deps_raw, "src/a.cp"), true);
}

test("makefile_parse: empty, comments-only and recipe-only inputs") {
    const only_comments = "# comment\n\n\t@echo recipe_without_rule\n";
    let rit = MakeRuleIter.init(only_comments);
    let vit = MakeVarIter.init(only_comments);
    expect_eq(rit.next() == null, true);
    expect_eq(vit.next() == null, true);
    expect_eq(isPhonyTarget(only_comments, "all"), false);
}
