// Source: cp/cap/test/test_fbx_production_deep.cp
// Deep Production Test Suite for cap:fbx
// Covers:
//   1. FBX 7400 (32-bit) header + all scalar ('Y','C','I','F','D','L') & string/raw ('S','R') properties
//   2. FBX 7500+ (64-bit) 25B node headers, 25B NULL sentinel & multi-sibling iteration
//   3. Typed array properties: raw float/double/bool arrays (encoding=0) & zlib array header (encoding=1)
//   4. Recursive multi-level scene tree traversal via `node.children(hdr.is_64bit)`
//   5. Bad magic, truncated header, out-of-bounds end_offset / prop_list_len & malformed property guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    FBX_MAGIC,
    FBX_HEADER_SIZE,
    FBX_V7500_THRESHOLD,
    FbxHeader,
    FbxNode,
    FbxProp,
    FbxNodeIter,
    FbxPropIter,
    isFbxBinary,
    parseFbxHeader,
} from "cap:gfx";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) < 0.0001;
}

// Stack-based binary writer helper for constructing exact FBX Binary test fixtures
struct BinWriter {
    buf: [1024]u8 = [0; 1024],
    len: usize = 0,

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.len] = s[i];
            _.len += 1;
            i += 1;
        }
    }

    fn writeU8(*_, v: u8) {
        _.buf[_.len] = v;
        _.len += 1;
    }

    fn writeU16Le(*_, v: u16) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
    }

    fn writeU32Le(*_, v: u32) {
        _.writeU8((v & 0xFF) as u8);
        _.writeU8(((v >> 8) & 0xFF) as u8);
        _.writeU8(((v >> 16) & 0xFF) as u8);
        _.writeU8(((v >> 24) & 0xFF) as u8);
    }

    fn writeU64Le(*_, v: u64) {
        _.writeU32Le((v & 0xFFFFFFFF) as u32);
        _.writeU32Le(((v >> 32) & 0xFFFFFFFF) as u32);
    }

    fn writeZeros(*_, count: usize) {
        let i: usize = 0;
        while (i < count) {
            _.writeU8(0);
            i += 1;
        }
    }

    fn writeFbxHeader(*_, version: u32) {
        _.writeBytes(FBX_MAGIC);
        _.writeU8(0x1A);
        _.writeU8(0x00);
        _.writeU32Le(version);
    }

    fn patchU32Le(*_, off: usize, v: u32) {
        _.buf[off] = (v & 0xFF) as u8;
        _.buf[off + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[off + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    }

    fn patchU64Le(*_, off: usize, v: u64) {
        _.patchU32Le(off, (v & 0xFFFFFFFF) as u32);
        _.patchU32Le(off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
    }

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

fn testFbx7400ScalarsAndStrings() {
    let w = BinWriter {};
    w.writeFbxHeader(7400);

    // Build 32-bit node "Meta" with 8 properties: Y, C, I, F, D, L, S, R
    const node_start = w.len;
    w.writeU32Le(0); // end_offset placeholder
    w.writeU32Le(8); // num_props
    w.writeU32Le(0); // prop_list_len placeholder
    w.writeU8(4);    // name_len
    w.writeBytes("Meta");

    const props_start = w.len;
    // 1. 'Y' (i16 = -321)
    w.writeU8('Y');
    const y_val: i16 = -321;
    const y_bits: u16 = @bitCast(y_val);
    w.writeU16Le(y_bits);
    // 2. 'C' (bool = true)
    w.writeU8('C');
    w.writeU8(1);
    // 3. 'I' (i32 = -123456)
    w.writeU8('I');
    const i_val: i32 = -123456;
    const i_bits: u32 = @bitCast(i_val);
    w.writeU32Le(i_bits);
    // 4. 'F' (f32 = 3.5)
    w.writeU8('F');
    const f_val: f32 = 3.5;
    const f_bits: u32 = @bitCast(f_val);
    w.writeU32Le(f_bits);
    // 5. 'D' (f64 = -42.125)
    w.writeU8('D');
    const d_val: f64 = -42.125;
    const d_bits: u64 = @bitCast(d_val);
    w.writeU64Le(d_bits);
    // 6. 'L' (i64 = 9876543210)
    w.writeU8('L');
    const l_val: i64 = 9876543210;
    const l_bits: u64 = @bitCast(l_val);
    w.writeU64Le(l_bits);
    // 7. 'S' ("Blender")
    w.writeU8('S');
    w.writeU32Le(7);
    w.writeBytes("Blender");
    // 8. 'R' (raw 3 bytes)
    w.writeU8('R');
    w.writeU32Le(3);
    w.writeU8(0xCA);
    w.writeU8(0xFE);
    w.writeU8(0x01);

    const props_len = (w.len - props_start) as u32;
    const node_end = w.len as u32;
    w.patchU32Le(node_start, node_end);
    w.patchU32Le(node_start + 8, props_len);

    // 13-byte NULL sentinel for 32-bit FBX
    w.writeZeros(13);

    const data = w.slice();
    expect_eq(isFbxBinary(data), true);
    const hdr = parseFbxHeader(data).?;
    expect_eq(hdr.version, 7400);
    expect_eq(hdr.is_64bit, false);
    expect_eq(hdr.nodes_offset, FBX_HEADER_SIZE);

    let nit = hdr.rootNodes(data);
    const node = nit.next().?;
    expect_eq(std.mem.eql(u8, node.name, "Meta"), true);
    expect_eq(node.num_properties, 8);
    expect_eq(nit.next() == null, true);

    let pit = node.properties();

    const p0 = pit.next().?;
    expect_eq(p0.type_code, 'Y');
    expect_eq(p0.int_val, -321);

    const p1 = pit.next().?;
    expect_eq(p1.type_code, 'C');
    expect_eq(p1.bool_val, true);

    const p2 = pit.next().?;
    expect_eq(p2.type_code, 'I');
    expect_eq(p2.int_val, -123456);

    const p3 = pit.next().?;
    expect_eq(p3.type_code, 'F');
    expect_eq(approxEq(p3.float_val, 3.5), true);

    const p4 = pit.next().?;
    expect_eq(p4.type_code, 'D');
    expect_eq(approxEq(p4.float_val, -42.125), true);

    const p5 = pit.next().?;
    expect_eq(p5.type_code, 'L');
    expect_eq(p5.int_val, 9876543210);

    const p6 = pit.next().?;
    expect_eq(p6.type_code, 'S');
    expect_eq(std.mem.eql(u8, p6.raw_bytes, "Blender"), true);

    const p7 = pit.next().?;
    expect_eq(p7.type_code, 'R');
    expect_eq(p7.raw_bytes.len, 3);
    expect_eq(p7.raw_bytes[0], 0xCA);
    expect_eq(p7.raw_bytes[2], 0x01);

    expect_eq(pit.next() == null, true);
}

fn testFbx750064BitAndMultiSiblings() {
    let w = BinWriter {};
    w.writeFbxHeader(7700);

    // Sibling 1: "Version" (64-bit 25B header) with 1 'I' prop
    const n1_start = w.len;
    w.writeU64Le(0); // end_offset
    w.writeU64Le(1); // num_props
    w.writeU64Le(5); // prop_list_len ('I' + 4B)
    w.writeU8(7);
    w.writeBytes("Version");
    w.writeU8('I');
    w.writeU32Le(7700);
    w.patchU64Le(n1_start, w.len as u64);

    // Sibling 2: "Creator" (64-bit 25B header) with 1 'S' prop
    const n2_start = w.len;
    w.writeU64Le(0);
    w.writeU64Le(1);
    w.writeU64Le(7); // 'S' + 4B len + "CP"
    w.writeU8(7);
    w.writeBytes("Creator");
    w.writeU8('S');
    w.writeU32Le(2);
    w.writeBytes("CP");
    w.patchU64Le(n2_start, w.len as u64);

    // 25-byte NULL sentinel for 64-bit FBX
    w.writeZeros(25);

    const data = w.slice();
    const hdr = parseFbxHeader(data).?;
    expect_eq(hdr.version, 7700);
    expect_eq(hdr.is_64bit, true);

    let nit = hdr.rootNodes(data);
    const s1 = nit.next().?;
    expect_eq(std.mem.eql(u8, s1.name, "Version"), true);
    let p1_it = s1.properties();
    expect_eq(p1_it.next().?.int_val, 7700);

    const s2 = nit.next().?;
    expect_eq(std.mem.eql(u8, s2.name, "Creator"), true);
    let p2_it = s2.properties();
    expect_eq(std.mem.eql(u8, p2_it.next().?.raw_bytes, "CP"), true);

    expect_eq(nit.next() == null, true);
}

fn testArrayPropertiesRawAndZlib() {
    let w = BinWriter {};
    w.writeFbxHeader(7500);

    const n_start = w.len;
    w.writeU64Le(0);
    w.writeU64Le(3); // 3 array properties: 'f' raw, 'i' zlib, 'b' raw
    w.writeU64Le(0); // prop_list_len placeholder
    w.writeU8(4);
    w.writeBytes("Mesh");

    const p_start = w.len;
    // 1. Raw float array 'f': 2 elements -> 8 bytes
    w.writeU8('f');
    w.writeU32Le(2);
    w.writeU32Le(0); // raw
    w.writeU32Le(8);
    const f0: f32 = 1.0;
    const f1: f32 = 2.0;
    const f0_b: u32 = @bitCast(f0);
    const f1_b: u32 = @bitCast(f1);
    w.writeU32Le(f0_b);
    w.writeU32Le(f1_b);

    // 2. Zlib-compressed i32 array 'i': 100 elements, encoding=1, 6 compressed bytes
    w.writeU8('i');
    w.writeU32Le(100);
    w.writeU32Le(1); // zlib
    w.writeU32Le(6);
    w.writeBytes("\x78\x9C\x01\x02\x03\x04");

    // 3. Raw bool array 'b': 3 elements -> 3 bytes
    w.writeU8('b');
    w.writeU32Le(3);
    w.writeU32Le(0);
    w.writeU32Le(3);
    w.writeU8(1);
    w.writeU8(0);
    w.writeU8(1);

    w.patchU64Le(n_start + 16, (w.len - p_start) as u64);
    w.patchU64Le(n_start, w.len as u64);
    w.writeZeros(25);

    const data = w.slice();
    const hdr = parseFbxHeader(data).?;
    let nit = hdr.rootNodes(data);
    const mesh = nit.next().?;
    let pit = mesh.properties();

    const af = pit.next().?;
    expect_eq(af.type_code, 'f');
    expect_eq(af.array_len, 2);
    expect_eq(af.array_encoding, 0);
    expect_eq(af.array_compressed_len, 8);
    expect_eq(af.raw_bytes.len, 8);

    const ai = pit.next().?;
    expect_eq(ai.type_code, 'i');
    expect_eq(ai.array_len, 100);
    expect_eq(ai.array_encoding, 1);
    expect_eq(ai.array_compressed_len, 6);
    expect_eq(ai.raw_bytes.len, 6);
    expect_eq(ai.raw_bytes[0], 0x78);

    const ab = pit.next().?;
    expect_eq(ab.type_code, 'b');
    expect_eq(ab.array_len, 3);
    expect_eq(ab.raw_bytes[0], 1);
    expect_eq(ab.raw_bytes[1], 0);
    expect_eq(ab.raw_bytes[2], 1);

    expect_eq(pit.next() == null, true);
}

fn testNestedChildrenTraversal() {
    let w = BinWriter {};
    w.writeFbxHeader(7400);

    // Root node "Objects" (no props, 1 child "Geometry")
    const obj_start = w.len;
    w.writeU32Le(0);
    w.writeU32Le(0);
    w.writeU32Le(0);
    w.writeU8(7);
    w.writeBytes("Objects");

    // Child node "Geometry" (1 'L' prop, 1 child "Vertices")
    const geo_start = w.len;
    w.writeU32Le(0);
    w.writeU32Le(1);
    w.writeU32Le(9); // 'L' + 8B
    w.writeU8(8);
    w.writeBytes("Geometry");
    w.writeU8('L');
    w.writeU64Le(42);

    // Grandchild node "Vertices" (1 'd' prop, 0 children)
    const vtx_start = w.len;
    w.writeU32Le(0);
    w.writeU32Le(1);
    w.writeU32Le(1 + 12 + 8); // 'd' + 12B array hdr + 1 f64
    w.writeU8(8);
    w.writeBytes("Vertices");
    w.writeU8('d');
    w.writeU32Le(1);
    w.writeU32Le(0);
    w.writeU32Le(8);
    const v0: f64 = 9.25;
    const v0_bits: u64 = @bitCast(v0);
    w.writeU64Le(v0_bits);
    w.patchU32Le(vtx_start, w.len as u32);

    // Close Geometry children with 13B NULL sentinel
    w.writeZeros(13);
    w.patchU32Le(geo_start, w.len as u32);

    // Close Objects children with 13B NULL sentinel
    w.writeZeros(13);
    w.patchU32Le(obj_start, w.len as u32);

    // Close root list with 13B NULL sentinel
    w.writeZeros(13);

    const data = w.slice();
    const hdr = parseFbxHeader(data).?;
    let root_it = hdr.rootNodes(data);
    const obj = root_it.next().?;
    expect_eq(std.mem.eql(u8, obj.name, "Objects"), true);
    expect_eq(root_it.next() == null, true);

    let obj_ch = obj.children(hdr.is_64bit);
    const geo = obj_ch.next().?;
    expect_eq(std.mem.eql(u8, geo.name, "Geometry"), true);
    let geo_p = geo.properties();
    expect_eq(geo_p.next().?.int_val, 42);
    expect_eq(obj_ch.next() == null, true);

    let geo_ch = geo.children(hdr.is_64bit);
    const vtx = geo_ch.next().?;
    expect_eq(std.mem.eql(u8, vtx.name, "Vertices"), true);
    let vtx_p = vtx.properties();
    const vp = vtx_p.next().?;
    expect_eq(vp.type_code, 'd');
    expect_eq(vp.array_len, 1);
    expect_eq(vp.raw_bytes.len, 8);
    expect_eq(geo_ch.next() == null, true);
}

fn testMalformedFbxGuards() {
    // Bad magic / truncated header
    expect_eq(isFbxBinary(""), false);
    expect_eq(isFbxBinary("Kaydara FBX Binary  \x00"), false); // < 27 bytes
    expect_eq(parseFbxHeader("Not an FBX Binary header at all!!") == null, true);

    // Out-of-bounds end_offset (> file.len)
    let w1 = BinWriter {};
    w1.writeFbxHeader(7400);
    w1.writeU32Le(999999); // invalid end_offset
    w1.writeU32Le(0);
    w1.writeU32Le(0);
    w1.writeU8(1);
    w1.writeU8('A');
    const hdr1 = parseFbxHeader(w1.slice()).?;
    let it1 = hdr1.rootNodes(w1.slice());
    expect_eq(it1.next() == null, true);

    // Backward end_offset (<= cur_pos)
    let w2 = BinWriter {};
    w2.writeFbxHeader(7400);
    w2.writeU32Le(10); // <= 27
    w2.writeU32Le(0);
    w2.writeU32Le(0);
    w2.writeU8(1);
    w2.writeU8('B');
    const hdr2 = parseFbxHeader(w2.slice()).?;
    let it2 = hdr2.rootNodes(w2.slice());
    expect_eq(it2.next() == null, true);

    // prop_list_len exceeds end_offset
    let w3 = BinWriter {};
    w3.writeFbxHeader(7400);
    w3.writeU32Le(27 + 13 + 2); // end_offset
    w3.writeU32Le(1);
    w3.writeU32Le(500); // prop_list_len way too large
    w3.writeU8(1);
    w3.writeU8('C');
    w3.writeU8(0);
    const hdr3 = parseFbxHeader(w3.slice()).?;
    let it3 = hdr3.rootNodes(w3.slice());
    expect_eq(it3.next() == null, true);

    // Unknown property type code terminates cleanly
    let bad_prop_it = FbxPropIter.init("Z1234", 1);
    expect_eq(bad_prop_it.next() == null, true);
}

test("1. FBX 7400 32-bit Header & All Scalar/String/Raw Properties", () => {
    testFbx7400ScalarsAndStrings();
});

test("2. FBX 7500+ 64-bit 25B Headers, NULL Sentinel & Multi-Siblings", () => {
    testFbx750064BitAndMultiSiblings();
});

test("3. Typed Array Properties (Raw Float/Bool & Zlib-Encoded Header)", () => {
    testArrayPropertiesRawAndZlib();
});

test("4. Multi-Level Recursive Scene Tree Traversal via children()", () => {
    testNestedChildrenTraversal();
});

test("5. Bad Magic, Out-of-Bounds Offsets & Malformed Property Guards", () => {
    testMalformedFbxGuards();
});
