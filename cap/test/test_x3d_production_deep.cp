// Source: cp/cap/test/test_x3d_production_deep.cp (Production deep tests for cap:x3d)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    X3dHeader,
    X3dMeta,
    X3dNode,
    X3dStats,
    isX3d,
    parseX3dHeader,
    X3dMetaIter,
    findX3dMeta,
    X3dNodeIter,
    X3dFloatIter,
    X3dIntIter,
    summarizeX3d,
} from "cap:x3d";

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff < 0.0001;
}

const SAMPLE_X3D_40: []const u8 =
    "\xEF\xBB\xBF" ++
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" ++
    "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 4.0//EN\" \"https://www.web3d.org/specifications/x3d-4.0.dtd\">\n" ++
    "<!-- X3D 4.0 test scene -->\n" ++
    "<X3D profile=\"Immersive\" version=\"4.0\">\n" ++
    "  <head>\n" ++
    "    <meta name=\"title\" content=\"CP Cube Scene\"/>\n" ++
    "    <meta content='Ada Lovelace' name='creator'/>\n" ++
    "    <meta name=\"license\" content=\"MIT\"/>\n" ++
    "  </head>\n" ++
    "  <Scene>\n" ++
    "    <Transform DEF=\"MainXform\" translation=\"1.5, -2.25, 3.0e1\">\n" ++
    "      <Shape DEF=\"BoxShape\">\n" ++
    "        <Appearance>\n" ++
    "          <Material diffuseColor=\"0.2 0.6 0.9\"/>\n" ++
    "        </Appearance>\n" ++
    "        <IndexedFaceSet coordIndex=\"0 1 2 -1 2 3 0 -1\">\n" ++
    "          <Coordinate point=\"0 0 0, 1 0 0, 1 1 0, 0 1 0\"/>\n" ++
    "        </IndexedFaceSet>\n" ++
    "      </Shape>\n" ++
    "    </Transform>\n" ++
    "    <Transform translation=\"5 0 0\">\n" ++
    "      <Shape USE=\"BoxShape\"/>\n" ++
    "    </Transform>\n" ++
    "    <TimeSensor DEF=\"Clock\" cycleInterval=\"4.0\"/>\n" ++
    "    <ROUTE fromNode=\"Clock\" fromField=\"fraction_changed\" toNode=\"MainXform\" toField=\"set_rotation\"/>\n" ++
    "  </Scene>\n" ++
    "</X3D>";

test("1. isX3d & parseX3dHeader handle UTF-8 BOM, XML decl, DOCTYPE, and <X3D> attributes", () => {
    expect_eq(isX3d(SAMPLE_X3D_40), true);
    const hdr = parseX3dHeader(SAMPLE_X3D_40) ?? unreachable;
    expect_eq(std.mem.eql(u8, hdr.profile, "Immersive"), true);
    expect_eq(std.mem.eql(u8, hdr.version, "4.0"), true);
    expect_eq(hdr.scene_offset > 0, true);
});

test("2. X3dMetaIter & findX3dMeta extract <head>/<meta> entries with mixed quotes and order", () => {
    let it = X3dMetaIter.init(SAMPLE_X3D_40);
    const m0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m0.name, "title"), true);
    expect_eq(std.mem.eql(u8, m0.content, "CP Cube Scene"), true);

    const m1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.name, "creator"), true);
    expect_eq(std.mem.eql(u8, m1.content, "Ada Lovelace"), true);

    const m2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m2.name, "license"), true);
    expect_eq(std.mem.eql(u8, m2.content, "MIT"), true);
    expect_eq(it.next() == null, true);

    const creator = findX3dMeta(SAMPLE_X3D_40, "creator") ?? unreachable;
    expect_eq(std.mem.eql(u8, creator, "Ada Lovelace"), true);
    expect_eq(findX3dMeta(SAMPLE_X3D_40, "missing") == null, true);
});

test("3. X3dNodeIter pre-order traversal, DEF/USE extraction, and getAttr + X3dFloatIter/X3dIntIter decoding", () => {
    let nit = X3dNodeIter.init(SAMPLE_X3D_40);

    // 0: Transform DEF="MainXform" (depth 0)
    const n0 = nit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, n0.tag, "Transform"), true);
    expect_eq(std.mem.eql(u8, n0.def_name, "MainXform"), true);
    expect_eq(n0.depth, 0);

    const tr_attr = n0.getAttr("translation") ?? unreachable;
    let fit = X3dFloatIter.init(tr_attr);
    expect_eq(approxEq(fit.next() ?? 0.0, 1.5), true);
    expect_eq(approxEq(fit.next() ?? 0.0, -2.25), true);
    expect_eq(approxEq(fit.next() ?? 0.0, 30.0), true);
    expect_eq(fit.next() == null, true);
    expect_eq(fit.malformed, false);

    // Advance to IndexedFaceSet & Coordinate
    let found_ifs = false;
    let found_coord = false;
    while (n := nit.next()) {
        if (std.mem.eql(u8, n.tag, "IndexedFaceSet")) {
            found_ifs = true;
            expect_eq(n.depth, 2);
            const ci_attr = n.getAttr("coordIndex") ?? unreachable;
            let iit = X3dIntIter.init(ci_attr);
            let face_seps: u32 = 0;
            let idx_cnt: u32 = 0;
            while (idx := iit.next()) {
                if (idx == -1) face_seps += 1;
                idx_cnt += 1;
            }
            expect_eq(iit.malformed, false);
            expect_eq(idx_cnt, 8);
            expect_eq(face_seps, 2);
        } else if (std.mem.eql(u8, n.tag, "Coordinate")) {
            found_coord = true;
            expect_eq(n.depth, 3);
            const pt_attr = n.getAttr("point") ?? unreachable;
            let pfit = X3dFloatIter.init(pt_attr);
            let fcnt: u32 = 0;
            while (fv := pfit.next()) {
                * = fv;
                fcnt += 1;
            }
            expect_eq(pfit.malformed, false);
            expect_eq(fcnt, 12); // 4 vertices * 3 floats
        }
    }
    expect_eq(found_ifs, true);
    expect_eq(found_coord, true);
});

test("4. summarizeX3d counts nodes, DEF, USE, Shape, Transform, ROUTE, and max_depth", () => {
    const st = summarizeX3d(SAMPLE_X3D_40) ?? unreachable;
    // Transform(Main), Shape(Box), Appearance, Material, IndexedFaceSet, Coordinate, Transform(2), Shape(USE), TimeSensor, ROUTE => 10
    expect_eq(st.node_count, 10);
    expect_eq(st.def_count, 3);
    expect_eq(st.use_count, 1);
    expect_eq(st.shape_count, 2);
    expect_eq(st.transform_count, 2);
    expect_eq(st.route_count, 1);
    // Max depth: Transform(1) -> Shape(2) -> IndexedFaceSet(3) -> Coordinate(4)
    expect_eq(st.max_depth, 4);
});

test("5. Non-X3D XML, missing <Scene>, and malformed attributes/numbers fail safely", () => {
    expect_eq(isX3d(""), false);
    expect_eq(isX3d("<html><body>not x3d</body></html>"), false);
    expect_eq(parseX3dHeader("<html></html>") == null, true);

    // X3D without <Scene>
    expect_eq(parseX3dHeader("<X3D profile=\"Core\" version=\"3.3\"><head/></X3D>") == null, true);
    expect_eq(summarizeX3d("<X3D profile=\"Core\" version=\"3.3\"><head/></X3D>") == null, true);

    // Unclosed <Scene>
    expect_eq(parseX3dHeader("<X3D version=\"4.0\"><Scene><Shape/>") == null, true);

    // Malformed float & int iterators
    let bad_f = X3dFloatIter.init("1.0 not_a_float 2.0");
    expect_eq(approxEq(bad_f.next() ?? 0.0, 1.0), true);
    expect_eq(bad_f.next() == null, true);
    expect_eq(bad_f.malformed, true);

    let bad_i = X3dIntIter.init("0 1 xyz");
    expect_eq(bad_i.next() ?? 99, 0);
    expect_eq(bad_i.next() ?? 99, 1);
    expect_eq(bad_i.next() == null, true);
    expect_eq(bad_i.malformed, true);
});
