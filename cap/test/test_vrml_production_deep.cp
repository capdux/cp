// Source: cp/cap/test/test_vrml_production_deep.cp (Production deep tests for cap:vrml)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    VrmlFlavor,
    VrmlTokenKind,
    VrmlHeader,
    VrmlToken,
    VrmlNode,
    VrmlStats,
    parseVrmlHeader,
    isVrml,
    VrmlLexer,
    VrmlNodeIter,
    findVrmlField,
    summarizeVrml,
} from "cap:gfx";

const SAMPLE_VRML97: []const u8 =
    "#VRML V2.0 utf8\n" ++
    "# Scene comment\n" ++
    "DEF RootXform Transform {\n" ++
    "  translation 1.0, -2.5, 3.0\n" ++
    "  children [\n" ++
    "    Shape {\n" ++
    "      geometry IndexedFaceSet {\n" ++
    "        coord Coordinate {\n" ++
    "          point [ 0 0 0, 1 0 0, 0 1 0 ]\n" ++
    "        }\n" ++
    "        coordIndex [ 0, 1, 2, -1 ]\n" ++
    "      }\n" ++
    "    }\n" ++
    "  ]\n" ++
    "}\n" ++
    "USE RootXform\n" ++
    "DEF Clock TimeSensor { cycleInterval 2.0 loop TRUE }\n" ++
    "ROUTE Clock.fraction_changed TO RootXform.set_rotation\n";

test("1. parseVrmlHeader & isVrml recognize VRML97 and Classic X3D headers", () => {
    expect_eq(isVrml(SAMPLE_VRML97), true);
    const h1 = parseVrmlHeader(SAMPLE_VRML97) ?? unreachable;
    expect_eq(h1.flavor, VrmlFlavor.vrml97);
    expect_eq(std.mem.eql(u8, h1.version, "V2.0"), true);
    expect_eq(std.mem.eql(u8, h1.encoding, "utf8"), true);

    const x3d = "#X3D V3.3 utf8\r\nPROFILE Interchange\r\nGroup { }\r\n";
    expect_eq(isVrml(x3d), true);
    const h2 = parseVrmlHeader(x3d) ?? unreachable;
    expect_eq(h2.flavor, VrmlFlavor.x3d_classic);
    expect_eq(std.mem.eql(u8, h2.version, "V3.3"), true);
    expect_eq(std.mem.eql(u8, h2.encoding, "utf8"), true);
});

test("2. VrmlLexer treats commas as whitespace, skips comments, and decodes escaped strings & hex/sci numbers", () => {
    const src = "url [ \"a\\\"b\\\\c\", \"d\" ] # tail\ncolor 0xFF00AA, -1.5e-2, .25";
    let lex = VrmlLexer.init(src);

    const t0 = lex.next() ?? unreachable;
    expect_eq(t0.kind, VrmlTokenKind.ident);
    expect_eq(std.mem.eql(u8, t0.text, "url"), true);

    const t1 = lex.next() ?? unreachable;
    expect_eq(t1.kind, VrmlTokenKind.lbracket);

    const t2 = lex.next() ?? unreachable;
    expect_eq(t2.kind, VrmlTokenKind.string);
    expect_eq(std.mem.eql(u8, t2.text, "a\\\"b\\\\c"), true);

    const t3 = lex.next() ?? unreachable;
    expect_eq(t3.kind, VrmlTokenKind.string);
    expect_eq(std.mem.eql(u8, t3.text, "d"), true);

    const t4 = lex.next() ?? unreachable;
    expect_eq(t4.kind, VrmlTokenKind.rbracket);

    const t5 = lex.next() ?? unreachable;
    expect_eq(t5.kind, VrmlTokenKind.ident);
    expect_eq(std.mem.eql(u8, t5.text, "color"), true);

    const t6 = lex.next() ?? unreachable;
    expect_eq(t6.kind, VrmlTokenKind.number);
    expect_eq(std.mem.eql(u8, t6.text, "0xFF00AA"), true);

    const t7 = lex.next() ?? unreachable;
    expect_eq(t7.kind, VrmlTokenKind.number);
    expect_eq(std.mem.eql(u8, t7.text, "-1.5e-2"), true);

    const t8 = lex.next() ?? unreachable;
    expect_eq(t8.kind, VrmlTokenKind.number);
    expect_eq(std.mem.eql(u8, t8.text, ".25"), true);

    expect_eq(lex.next() == null, true);
    expect_eq(lex.malformed, false);
});

test("3. VrmlNodeIter & findVrmlField traverse DEF/USE/nested nodes and extract translation/point fields", () => {
    let nit = VrmlNodeIter.init(SAMPLE_VRML97);

    // 1st top-level node: DEF RootXform Transform { ... }
    const n0 = nit.next() ?? unreachable;
    expect_eq(n0.is_use, false);
    expect_eq(std.mem.eql(u8, n0.def_name, "RootXform"), true);
    expect_eq(std.mem.eql(u8, n0.node_type, "Transform"), true);

    const tr = findVrmlField(n0.body_slice, "translation") ?? unreachable;
    expect_eq(std.mem.eql(u8, tr, "1.0, -2.5, 3.0"), true);

    const children = findVrmlField(n0.body_slice, "children") ?? unreachable;
    // Nested node iteration inside children [ ... ]
    let child_it = VrmlNodeIter.init(children);
    const shape = child_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, shape.node_type, "Shape"), true);

    const geom = findVrmlField(shape.body_slice, "geometry") ?? unreachable;
    let geom_it = VrmlNodeIter.init(geom);
    const ifs = geom_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, ifs.node_type, "IndexedFaceSet"), true);

    const coord = findVrmlField(ifs.body_slice, "coord") ?? unreachable;
    let coord_it = VrmlNodeIter.init(coord);
    const coord_node = coord_it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, coord_node.node_type, "Coordinate"), true);

    const pts = findVrmlField(coord_node.body_slice, "point") ?? unreachable;
    expect_eq(std.mem.eql(u8, pts, "[ 0 0 0, 1 0 0, 0 1 0 ]"), true);

    // 2nd top-level node: USE RootXform
    const n1 = nit.next() ?? unreachable;
    expect_eq(n1.is_use, true);
    expect_eq(std.mem.eql(u8, n1.def_name, "RootXform"), true);

    // 3rd top-level node: DEF Clock TimeSensor
    const n2 = nit.next() ?? unreachable;
    expect_eq(n2.is_use, false);
    expect_eq(std.mem.eql(u8, n2.def_name, "Clock"), true);
    expect_eq(std.mem.eql(u8, n2.node_type, "TimeSensor"), true);

    expect_eq(nit.next() == null, true);
});

test("4. summarizeVrml counts nodes, DEF, USE, ROUTE, and max nesting depth", () => {
    const st = summarizeVrml(SAMPLE_VRML97) ?? unreachable;
    // Nodes: Transform, Shape, IndexedFaceSet, Coordinate, TimeSensor => 5
    expect_eq(st.node_count, 5);
    expect_eq(st.def_count, 2);
    expect_eq(st.use_count, 1);
    expect_eq(st.route_count, 1);
    // Max depth: Transform(1) -> Shape(2) -> IndexedFaceSet(3) -> Coordinate(4)
    expect_eq(st.max_depth, 4);
});

test("5. Malformed VRML (bad header, unbalanced braces/brackets, unterminated string) fail safely", () => {
    expect_eq(isVrml(""), false);
    expect_eq(isVrml("Transform { }"), false);
    expect_eq(isVrml("#VRML V2.0\n"), false); // missing encoding
    expect_eq(summarizeVrml("not vrml") == null, true);

    // Unclosed brace
    expect_eq(summarizeVrml("#VRML V2.0 utf8\nTransform { children [ ]\n") == null, true);

    // Crossed brackets { ]
    expect_eq(summarizeVrml("#VRML V2.0 utf8\nTransform { ] }\n") == null, true);

    // Unterminated string literal
    const bad_str = "#VRML V2.0 utf8\nWorldInfo { title \"unterminated\n}\n";
    expect_eq(summarizeVrml(bad_str) == null, true);

    let lex = VrmlLexer.init("\"unterminated");
    expect_eq(lex.next() == null, true);
    expect_eq(lex.malformed, true);
});
