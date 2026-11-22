// Source: cp/cap/test/test_dae_production_deep.cp (Production deep tests for cap:dae)

import { test, expect_eq } from "cap:test";
import {
    DaeAsset,
    DaeGeometry,
    DaeFloatArray,
    DaeSceneNode,
    DaeStats,
    DaeGeometryIter,
    DaeFloatArrayIter,
    DaeFloatIter,
    DaeSceneNodeIter,
    isCollada,
    parseDaeAsset,
    summarizeDae,
} from "cap:gfx";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. COLLADA 1.4.1 <asset> metadata parsing (Z_UP, unit meter=0.01, BOM & comments)", () => {
    const dae_xml =
        "\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" +
        "<!-- Exported from Blender -->\n" +
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">" +
        "  <asset>" +
        "    <contributor>" +
        "      <author>CP Graphics Lab</author>" +
        "      <authoring_tool>Blender 4.2.0</authoring_tool>" +
        "    </contributor>" +
        "    <created>2026-11-20T12:00:00Z</created>" +
        "    <modified>2026-11-21T18:30:00Z</modified>" +
        "    <unit name=\"centimeter\" meter=\"0.01\"/>" +
        "    <up_axis>Z_UP</up_axis>" +
        "  </asset>" +
        "</COLLADA>";

    expect_eq(isCollada(dae_xml), true);

    const asset = parseDaeAsset(dae_xml) ?? unreachable;
    expect_eq(bytesEq(asset.version, "1.4.1"), true);
    expect_eq(bytesEq(asset.author, "CP Graphics Lab"), true);
    expect_eq(bytesEq(asset.authoring_tool, "Blender 4.2.0"), true);
    expect_eq(bytesEq(asset.created, "2026-11-20T12:00:00Z"), true);
    expect_eq(bytesEq(asset.modified, "2026-11-21T18:30:00Z"), true);
    expect_eq(bytesEq(asset.unit_name, "centimeter"), true);
    expect_eq(asset.unit_meter > 0.0099 && asset.unit_meter < 0.0101, true);
    expect_eq(bytesEq(asset.up_axis, "Z_UP"), true);

    // Defaults when <asset> omits unit/up_axis
    const minimal = parseDaeAsset("<COLLADA version=\"1.5.0\"/>") ?? unreachable;
    expect_eq(bytesEq(minimal.version, "1.5.0"), true);
    expect_eq(minimal.unit_meter > 0.99 && minimal.unit_meter < 1.01, true);
    expect_eq(bytesEq(minimal.up_axis, "Y_UP"), true);
});

test("2. DaeGeometryIter + DaeFloatArrayIter + DaeFloatIter scientific notation vertex streaming", () => {
    const dae_xml =
        "<COLLADA version=\"1.4.1\">" +
        "  <library_geometries>" +
        "    <geometry id=\"Tri-mesh\" name=\"Triangle\">" +
        "      <mesh>" +
        "        <source id=\"Tri-positions\">" +
        "          <float_array id=\"Tri-pos-array\" count=\"6\"> -1.5e-2  2.5E+3\n0.0 1.25 -0.75 +3e2 </float_array>" +
        "        </source>" +
        "      </mesh>" +
        "    </geometry>" +
        "  </library_geometries>" +
        "</COLLADA>";

    let git = DaeGeometryIter.init(dae_xml);
    const g0 = git.next() ?? unreachable;
    expect_eq(bytesEq(g0.id, "Tri-mesh"), true);
    expect_eq(bytesEq(g0.name, "Triangle"), true);
    expect_eq(git.next() == null, true);

    let fait = DaeFloatArrayIter.init(g0.raw_xml);
    const fa0 = fait.next() ?? unreachable;
    expect_eq(bytesEq(fa0.id, "Tri-pos-array"), true);
    expect_eq(fa0.declared_count, 6);
    expect_eq(fait.next() == null, true);

    let fit = DaeFloatIter.init(fa0.raw_values);
    const v0 = fit.next() ?? unreachable; // -0.015
    const v1 = fit.next() ?? unreachable; // 2500.0
    const v2 = fit.next() ?? unreachable; // 0.0
    const v3 = fit.next() ?? unreachable; // 1.25
    const v4 = fit.next() ?? unreachable; // -0.75
    const v5 = fit.next() ?? unreachable; // 300.0
    expect_eq(fit.next() == null, true);
    expect_eq(fit.had_error, false);

    expect_eq(v0 < -0.0149 && v0 > -0.0151, true);
    expect_eq(v1 > 2499.9 && v1 < 2500.1, true);
    expect_eq(v2 > -0.001 && v2 < 0.001, true);
    expect_eq(v3 > 1.249 && v3 < 1.251, true);
    expect_eq(v4 < -0.749 && v4 > -0.751, true);
    expect_eq(v5 > 299.9 && v5 < 300.1, true);
});

test("3. <library_visual_scenes> DaeSceneNodeIter with nested nodes and <instance_geometry>", () => {
    const dae_xml =
        "<COLLADA version=\"1.4.1\">" +
        "  <library_visual_scenes>" +
        "    <visual_scene id=\"Scene\" name=\"MainScene\">" +
        "      <node id=\"Root\" name=\"RootNode\" type=\"NODE\">" +
        "        <node id=\"CubeNode\" name=\"Cube\" type=\"NODE\">" +
        "          <instance_geometry url=\"#Cube-mesh\"/>" +
        "        </node>" +
        "        <node id=\"ArmJoint\" name=\"Hip\" type=\"JOINT\"/>" +
        "      </node>" +
        "    </visual_scene>" +
        "  </library_visual_scenes>" +
        "</COLLADA>";

    let nit = DaeSceneNodeIter.init(dae_xml);

    // Pre-order: Root (no direct instance_geometry), then CubeNode (#Cube-mesh), then ArmJoint ("")
    const n0 = nit.next() ?? unreachable;
    expect_eq(bytesEq(n0.id, "Root"), true);
    expect_eq(bytesEq(n0.name, "RootNode"), true);
    expect_eq(bytesEq(n0.node_type, "NODE"), true);
    expect_eq(bytesEq(n0.instance_geometry_url, ""), true);

    const n1 = nit.next() ?? unreachable;
    expect_eq(bytesEq(n1.id, "CubeNode"), true);
    expect_eq(bytesEq(n1.instance_geometry_url, "#Cube-mesh"), true);

    const n2 = nit.next() ?? unreachable;
    expect_eq(bytesEq(n2.id, "ArmJoint"), true);
    expect_eq(bytesEq(n2.node_type, "JOINT"), true);
    expect_eq(bytesEq(n2.instance_geometry_url, ""), true);

    expect_eq(nit.next() == null, true);
});

test("4. summarizeDae full asset statistics", () => {
    const dae_xml =
        "<COLLADA version=\"1.4.1\">" +
        "  <library_materials>" +
        "    <material id=\"MatA\" name=\"Steel\"/>" +
        "    <material id=\"MatB\" name=\"Glass\"/>" +
        "  </library_materials>" +
        "  <library_geometries>" +
        "    <geometry id=\"G1\"><mesh>" +
        "      <source><float_array id=\"F1\" count=\"9\">0 0 0 1 0 0 0 1 0</float_array></source>" +
        "      <source><float_array id=\"F2\" count=\"3\">0 0 1</float_array></source>" +
        "    </mesh></geometry>" +
        "    <geometry id=\"G2\"><mesh>" +
        "      <source><float_array id=\"F3\" count=\"6\">1 2 3 4 5 6</float_array></source>" +
        "    </mesh></geometry>" +
        "  </library_geometries>" +
        "  <library_visual_scenes>" +
        "    <visual_scene id=\"S1\">" +
        "      <node id=\"N1\"><instance_geometry url=\"#G1\"/></node>" +
        "      <node id=\"N2\"><instance_geometry url=\"#G2\"/></node>" +
        "    </visual_scene>" +
        "  </library_visual_scenes>" +
        "</COLLADA>";

    const stats = summarizeDae(dae_xml);
    expect_eq(stats.geometry_count, 2);
    expect_eq(stats.float_array_count, 3);
    expect_eq(stats.total_floats, 18);
    expect_eq(stats.node_count, 2);
    expect_eq(stats.material_count, 2);
});

test("5. Fault tolerance: non-COLLADA XML, unclosed tags, and malformed floats", () => {
    // Non-COLLADA XML
    expect_eq(isCollada("<svg xmlns=\"http://www.w3.org/2000/svg\"/>"), false);
    expect_eq(isCollada("<COLLADA_EXTRA/>"), false);
    expect_eq(isCollada(""), false);
    expect_eq(parseDaeAsset("<html><body>nope</body></html>") == null, true);

    const bad_stats = summarizeDae("<html></html>");
    expect_eq(bad_stats.geometry_count, 0);
    expect_eq(bad_stats.node_count, 0);

    // Unclosed geometry tag terminates cleanly without infinite loop
    const unclosed = "<COLLADA version=\"1.4.1\"><library_geometries><geometry id=\"broken\"";
    let git = DaeGeometryIter.init(unclosed);
    expect_eq(git.next() == null, true);

    // Malformed float tokens trigger had_error=true and stop cleanly
    let fit1 = DaeFloatIter.init("1.5 not_a_float 2.0");
    const ok_first = fit1.next() ?? unreachable;
    expect_eq(ok_first > 1.49 && ok_first < 1.51, true);
    expect_eq(fit1.next() == null, true);
    expect_eq(fit1.had_error, true);

    let fit2 = DaeFloatIter.init("1e+");
    expect_eq(fit2.next() == null, true);
    expect_eq(fit2.had_error, true);

    let fit3 = DaeFloatIter.init(".");
    expect_eq(fit3.next() == null, true);
    expect_eq(fit3.had_error, true);
});
