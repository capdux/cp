// Source: cp/cap/test/test_obj_production_deep.cp (Production deep tests for cap:obj)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ObjRecordKind,
    ObjVec3,
    ObjFaceVertex,
    ObjStats,
    ObjRecord,
    ObjLineIter,
    parseObjVec3,
    parseObjFaceVertex,
    summarizeObj,
} from "cap:obj";

const SAMPLE_OBJ: []const u8 =
    "# Wavefront OBJ sample mesh\r\n" ++
    "mtllib cube.mtl # material library\r\n" ++
    "o UnitTriangle\n" ++
    "g FrontFace\n" ++
    "usemtl Steel\n" ++
    "v -1.5 0.0 2.25 1.0 # homogeneous w ignored\n" ++
    "v 1.5e0 0.0 -2.5e-1\n" ++
    "v 0.0 3.0 0.0\n" ++
    "vt 0.0 0.0\n" ++
    "vt 1.0 0.5\n" ++
    "vn 0.0 0.0 1.0\n" ++
    "s off\n" ++
    "f 1/1/1 2/2/1 3//1\n";

test("1. ObjLineIter streams classified records, strips CRLF and inline comments, and skips blank lines", () => {
    let it = ObjLineIter.init(SAMPLE_OBJ);

    const r0 = it.next() ?? unreachable;
    expect_eq(r0.kind == ObjRecordKind.mtllib, true);
    expect_eq(std.mem.eql(u8, r0.rest, "cube.mtl"), true);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.kind == ObjRecordKind.object_name, true);
    expect_eq(std.mem.eql(u8, r1.rest, "UnitTriangle"), true);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.kind == ObjRecordKind.group_name, true);
    expect_eq(std.mem.eql(u8, r2.rest, "FrontFace"), true);

    const r3 = it.next() ?? unreachable;
    expect_eq(r3.kind == ObjRecordKind.usemtl, true);
    expect_eq(std.mem.eql(u8, r3.rest, "Steel"), true);

    const r4 = it.next() ?? unreachable;
    expect_eq(r4.kind == ObjRecordKind.vertex, true);
    expect_eq(std.mem.eql(u8, r4.rest, "-1.5 0.0 2.25 1.0"), true);

    // Advance to `s off` (other) and `f ...` (face)
    let saw_other = false;
    let saw_face = false;
    while (rec := it.next()) {
        if (rec.kind == ObjRecordKind.other && std.mem.eql(u8, rec.rest, "off")) saw_other = true;
        if (rec.kind == ObjRecordKind.face && std.mem.eql(u8, rec.rest, "1/1/1 2/2/1 3//1")) saw_face = true;
    }
    expect_eq(saw_other, true);
    expect_eq(saw_face, true);
});

test("2. parseObjVec3 decodes signed floats, scientific notation, optional w, and rejects malformed args", () => {
    const v0 = parseObjVec3("-1.5 0.0 2.25 1.0") ?? unreachable;
    expect_eq(v0.x, -1.5);
    expect_eq(v0.y, 0.0);
    expect_eq(v0.z, 2.25);

    const v1 = parseObjVec3("  1.5e2\t-2.5E-1  +4  ") ?? unreachable;
    expect_eq(v1.x, 150.0);
    expect_eq(v1.y, -0.25);
    expect_eq(v1.z, 4.0);

    // Too few components or invalid float
    expect_eq(parseObjVec3("1.0 2.0") == null, true);
    expect_eq(parseObjVec3("1.0 abc 3.0") == null, true);
    expect_eq(parseObjVec3("1.0 2.0e 3.0") == null, true);
});

test("3. parseObjFaceVertex handles all 4 standard formats and negative relative indices", () => {
    // v
    const f0 = parseObjFaceVertex("12") ?? unreachable;
    expect_eq(f0.v, 12);
    expect_eq(f0.vt == null, true);
    expect_eq(f0.vn == null, true);

    // v/vt
    const f1 = parseObjFaceVertex("3/7") ?? unreachable;
    expect_eq(f1.v, 3);
    expect_eq(f1.vt ?? 0, 7);
    expect_eq(f1.vn == null, true);

    // v//vn
    const f2 = parseObjFaceVertex("-1//-2") ?? unreachable;
    expect_eq(f2.v, -1);
    expect_eq(f2.vt == null, true);
    expect_eq(f2.vn ?? 0, -2);

    // v/vt/vn
    const f3 = parseObjFaceVertex("4/5/6") ?? unreachable;
    expect_eq(f3.v, 4);
    expect_eq(f3.vt ?? 0, 5);
    expect_eq(f3.vn ?? 0, 6);
});

test("4. parseObjFaceVertex rejects 0 indices, extra slashes, empty slots, and non-numeric tokens", () => {
    expect_eq(parseObjFaceVertex("0") == null, true);
    expect_eq(parseObjFaceVertex("1/0") == null, true);
    expect_eq(parseObjFaceVertex("1//0") == null, true);
    expect_eq(parseObjFaceVertex("1/2/0") == null, true);
    expect_eq(parseObjFaceVertex("1/") == null, true);
    expect_eq(parseObjFaceVertex("1//") == null, true);
    expect_eq(parseObjFaceVertex("1/2/3/4") == null, true);
    expect_eq(parseObjFaceVertex("/1/2") == null, true);
    expect_eq(parseObjFaceVertex("abc") == null, true);
    expect_eq(parseObjFaceVertex("") == null, true);
});

test("5. summarizeObj aggregates exact counts across vertices, texcoords, normals, faces, objects, and groups", () => {
    const stats = summarizeObj(SAMPLE_OBJ);
    expect_eq(stats.vertices, 3);
    expect_eq(stats.texcoords, 2);
    expect_eq(stats.normals, 1);
    expect_eq(stats.faces, 1);
    expect_eq(stats.objects, 1);
    expect_eq(stats.groups, 1);

    const empty = summarizeObj("# comment only\n   \r\n");
    expect_eq(empty.vertices, 0);
    expect_eq(empty.faces, 0);
});
