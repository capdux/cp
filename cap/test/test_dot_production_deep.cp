// Source: cp/cap/test/test_dot_production_deep.cp (Production deep tests for cap:dot)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DotGraphKind,
    DotHeader,
    DotEdge,
    parseDotHeader,
    DotEdgeIter,
    validateDotBraces,
    formatDotEdge,
} from "cap:dot";

test("1. parseDotHeader handles strict, anonymous, quoted names, and all comment styles", () => {
    const h1 = parseDotHeader("// line comment\n# hash comment\n/* block */\nstrict digraph \"Dep Graph\" { a -> b; }") ?? unreachable;
    expect_eq(h1.is_strict, true);
    expect_eq(h1.kind == DotGraphKind.digraph, true);
    expect_eq(std.mem.eql(u8, h1.name ?? "", "Dep Graph"), true);

    const h2 = parseDotHeader("graph { a -- b; }") ?? unreachable;
    expect_eq(h2.is_strict, false);
    expect_eq(h2.kind == DotGraphKind.graph, true);
    expect_eq(h2.name == null, true);

    const h3 = parseDotHeader("DIGRAPH Workflow { }") ?? unreachable;
    expect_eq(h3.kind == DotGraphKind.digraph, true);
    expect_eq(std.mem.eql(u8, h3.name ?? "", "Workflow"), true);

    expect_eq(parseDotHeader("notagraph { }") == null, true);
    expect_eq(parseDotHeader("digraph NoBrace") == null, true);
});

test("2. DotEdgeIter parses directed/undirected edges, attributes, quoted nodes, and chained edges", () => {
    const src =
        "digraph G {\n" ++
        "  rankdir = LR;\n" ++
        "  node [shape=box];\n" ++
        "  \"cap:http\" -> \"cap:net\" [label=\"tcp\", style=bold];\n" ++
        "  a -> b -> c [label=\"chain\"];\n" ++
        "  x -- y;\n" ++
        "}\n";

    let it = DotEdgeIter.init(src);

    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.from_node, "cap:http"), true);
    expect_eq(std.mem.eql(u8, e0.to_node, "cap:net"), true);
    expect_eq(e0.is_directed, true);
    expect_eq(std.mem.eql(u8, e0.attrs ?? "", "label=\"tcp\", style=bold"), true);

    // Chained edge part 1: a -> b
    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.from_node, "a"), true);
    expect_eq(std.mem.eql(u8, e1.to_node, "b"), true);
    expect_eq(e1.is_directed, true);

    // Chained edge part 2: b -> c [label="chain"]
    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.from_node, "b"), true);
    expect_eq(std.mem.eql(u8, e2.to_node, "c"), true);
    expect_eq(e2.is_directed, true);
    expect_eq(std.mem.eql(u8, e2.attrs ?? "", "label=\"chain\""), true);

    // Undirected edge: x -- y
    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.from_node, "x"), true);
    expect_eq(std.mem.eql(u8, e3.to_node, "y"), true);
    expect_eq(e3.is_directed, false);
    expect_eq(e3.attrs == null, true);

    expect_eq(it.next() == null, true);
});

test("3. validateDotBraces ignores braces inside strings and comments and rejects mismatches", () => {
    expect_eq(validateDotBraces("digraph { a -> b [label=\"{unbalanced [}\"]; // {\n /* [ */ }"), true);
    expect_eq(validateDotBraces("digraph { subgraph cluster_0 { a -> b [k=\"v\\\"}\"]; } }"), true);

    expect_eq(validateDotBraces("digraph { a -> b"), false);
    expect_eq(validateDotBraces("digraph { a -> b [label=\"x\" }"), false);
    expect_eq(validateDotBraces("digraph { a -> b ] }"), false);
    expect_eq(validateDotBraces("digraph { a -> \"unclosed }"), false);
    expect_eq(validateDotBraces("digraph { /* unclosed comment }"), false);
});

test("4. formatDotEdge auto-quotes special node names and escapes quotes/backslashes", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const s1 = formatDotEdge("parser", "codegen", true, null, buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s1, "parser -> codegen;"), true);

    const s2 = formatDotEdge("cap:http", "node 2", false, "path\\to \"doc\"", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s2, "\"cap:http\" -- \"node 2\" [label=\"path\\\\to \\\"doc\\\"\"];"), true);

    // Empty node name is auto-quoted
    const s3 = formatDotEdge("", "end", true, "go", buf[0..]) ?? unreachable;
    expect_eq(std.mem.eql(u8, s3, "\"\" -> end [label=\"go\"];"), true);
});

test("5. formatDotEdge buffer bounds and subgraph edge extraction", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);
    // "a -> b;" is 7 bytes
    const ok = formatDotEdge("a", "b", true, null, buf[0..7]) ?? unreachable;
    expect_eq(std.mem.eql(u8, ok, "a -> b;"), true);
    expect_eq(formatDotEdge("a", "b", true, null, buf[0..6]) == null, true);
    expect_eq(formatDotEdge("a", "b", true, "long label", buf[0..]) == null, true);

    const sub = "digraph S { subgraph cluster_1 { u -> v; } }";
    let it = DotEdgeIter.init(sub);
    const e = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e.from_node, "u"), true);
    expect_eq(std.mem.eql(u8, e.to_node, "v"), true);
    expect_eq(it.next() == null, true);
});
