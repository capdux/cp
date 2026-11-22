// Source: cp/cap/test/test_flatbuf_production_deep.cp (Production deep tests for cap:flatbuf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FB_SIZE_PREFIX_BYTES,
    FB_FILE_ID_OFFSET,
    FB_FILE_ID_LEN,
    FbVector,
    FbTable,
    fbFileIdentifier,
    fbHasIdentifier,
    fbStripSizePrefix,
    fbRootTable,
} from "cap:compiler";

fn putU16LE(buf: *[]u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copyBytes(dst: *[]u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

test("1. Monster-style root table with 4B File Identifier, scalars, string, and absent default fields", () => {
    // Layout:
    // 0..4:   root uoffset = 24
    // 4..8:   File ID = "MONS"
    // 8..20:  VTable (len=12 => 4 slots: slot4=hp(i16), slot6=mana(absent=0), slot8=name(str), slot10=speed(f32))
    //         [vtable_len=12, table_size=16, off_hp=4, off_mana=0, off_name=8, off_speed=12]
    // 24..40: Table (pos=24):
    //         24..28: soffset = 24 - 8 = 16
    //         28..30: hp = 300 (i16)
    //         32..36: name uoffset = 40 - 32 = 8
    //         36..40: speed f32 = 2.5 (0x40200000)
    // 40..48: String "Orc" => [len=3, 'O', 'r', 'c', 0]
    let buf: [48]u8 = [0; 48];
    let b = buf[0..48];

    putU32LE(*b, 0, 24);
    copyBytes(*b, 4, "MONS");

    // VTable at offset 8
    putU16LE(*b, 8, 12); // vtable_len
    putU16LE(*b, 10, 16); // table_size
    putU16LE(*b, 12, 4); // slot 4 (hp)
    putU16LE(*b, 14, 0); // slot 6 (mana absent)
    putU16LE(*b, 16, 8); // slot 8 (name)
    putU16LE(*b, 18, 12); // slot 10 (speed)

    // Table at offset 24
    putU32LE(*b, 24, 16); // soffset = 24 - 8 = 16
    putU16LE(*b, 28, 300); // hp
    putU32LE(*b, 32, 8); // name uoffset => 32 + 8 = 40
    putU32LE(*b, 36, 0x40200000); // speed = 2.5

    // String at offset 40
    putU32LE(*b, 40, 3);
    copyBytes(*b, 44, "Orc");

    expect_eq(fbHasIdentifier(b, "MONS"), true);
    expect_eq(fbHasIdentifier(b, "NOPE"), false);
    expect_eq(std.mem.eql(u8, fbFileIdentifier(b) ?? "", "MONS"), true);

    const root = fbRootTable(b) ?? unreachable;
    expect_eq(root.vtablePos() ?? 0, 8);
    expect_eq(root.fieldCount() ?? 0, 4);
    expect_eq(root.getI16(4) ?? 0, 300);
    expect_eq(root.getI16(6) == null, true); // absent => caller supplies default
    expect_eq(root.getI16(12) == null, true); // beyond vtable_len => null
    expect_eq(std.mem.eql(u8, root.getString(8) ?? "", "Orc"), true);
    expect_eq((root.getF32(10) ?? 0.0) as f64, 2.5);
});

test("2. fbStripSizePrefix unwraps 4B LE size-prefixed FlatBuffers messages", () => {
    let framed: [32]u8 = [0; 32];
    let f = framed[0..32];

    // Inner message of 20 bytes: root at 12, vtable at 4 (len=6: 1 bool slot at +4)
    putU32LE(*f, 0, 20); // size prefix = 20
    let inner = f[4..24];
    putU32LE(*inner, 0, 12); // root pos = 12
    putU16LE(*inner, 4, 6); // vtable len = 6
    putU16LE(*inner, 6, 8); // table size = 8
    putU16LE(*inner, 8, 4); // slot 4 => +4
    putU32LE(*inner, 12, 8); // soffset = 12 - 4 = 8
    inner[16] = 1; // bool true

    const stripped = fbStripSizePrefix(f) ?? unreachable;
    expect_eq(stripped.len, 20);
    const root = fbRootTable(stripped) ?? unreachable;
    expect_eq(root.getBool(4) ?? false, true);

    // Oversized prefix returns null
    putU32LE(*f, 0, 100);
    expect_eq(fbStripSizePrefix(f) == null, true);
});

test("3. nested FbTable and FbVector (u32 vector, string vector, nested table vector)", () => {
    // Layout:
    // 0..4:   root uoffset = 16
    // 4..14:  root vtable (len=10 => slot4=nums_vec, slot6=strs_vec, slot8=child_tbl)
    // 16..32: root table (soff=12 => vtable 4):
    //         20..24: nums_vec uoffset => 32 (rel=12)
    //         24..28: strs_vec uoffset => 48 (rel=24)
    //         28..32: child_tbl uoffset => 80 (rel=52)
    // 32..48: nums_vec: len=3, [10, 20, 30]
    // 48..60: strs_vec: len=2, [uoff@52 -> 60 (rel=8), uoff@56 -> 68 (rel=12)]
    // 60..68: str0 "hi" ([2, 'h', 'i', 0])
    // 68..76: str1 "cp" ([2, 'c', 'p', 0])
    // 80..92: child table (pos=80, negative soffset = -12 => vtable at 80 - (-12) = 92!)
    //         84..88: child id = 777 (u32)
    // 92..98: child vtable (len=6, tbl_sz=8, slot4=4)
    let buf: [104]u8 = [0; 104];
    let b = buf[0..104];

    putU32LE(*b, 0, 16);
    putU16LE(*b, 4, 10);
    putU16LE(*b, 6, 16);
    putU16LE(*b, 8, 4); // slot 4: nums_vec
    putU16LE(*b, 10, 8); // slot 6: strs_vec
    putU16LE(*b, 12, 12); // slot 8: child_tbl

    putU32LE(*b, 16, 12); // soffset = 16 - 4 = 12
    putU32LE(*b, 20, 12); // 20 + 12 = 32
    putU32LE(*b, 24, 24); // 24 + 24 = 48
    putU32LE(*b, 28, 52); // 28 + 52 = 80

    // nums_vec at 32
    putU32LE(*b, 32, 3);
    putU32LE(*b, 36, 10);
    putU32LE(*b, 40, 20);
    putU32LE(*b, 44, 30);

    // strs_vec at 48
    putU32LE(*b, 48, 2);
    putU32LE(*b, 52, 8); // 52 + 8 = 60
    putU32LE(*b, 56, 12); // 56 + 12 = 68
    putU32LE(*b, 60, 2);
    copyBytes(*b, 64, "hi");
    putU32LE(*b, 68, 2);
    copyBytes(*b, 72, "cp");

    // child table at 80 with NEGATIVE soffset (-12) pointing forward to vtable at 92
    const neg_soff: i32 = -12;
    putU32LE(*b, 80, neg_soff as u32);
    putU32LE(*b, 84, 777);

    // child vtable at 92
    putU16LE(*b, 92, 6);
    putU16LE(*b, 94, 8);
    putU16LE(*b, 96, 4);

    const root = fbRootTable(b) ?? unreachable;

    const nums = root.getVector(4) ?? unreachable;
    expect_eq(nums.len, 3);
    expect_eq(nums.getU32(0) ?? 0, 10);
    expect_eq(nums.getU32(1) ?? 0, 20);
    expect_eq(nums.getU32(2) ?? 0, 30);
    expect_eq(nums.getU32(3) == null, true);

    const strs = root.getVector(6) ?? unreachable;
    expect_eq(strs.len, 2);
    expect_eq(std.mem.eql(u8, strs.getString(0) ?? "", "hi"), true);
    expect_eq(std.mem.eql(u8, strs.getString(1) ?? "", "cp"), true);
    expect_eq(strs.getString(2) == null, true);

    const child = root.getTable(8) ?? unreachable;
    expect_eq(child.vtablePos() ?? 0, 92);
    expect_eq(child.getU32(4) ?? 0, 777);
});

test("4. FbVector byteSlice and nested table vector access", () => {
    // Root table at 12, vtable at 4 (len=8: slot4=bytes_vec, slot6=tbl_vec)
    // bytes_vec at 28: len=4, [0xDE, 0xAD, 0xBE, 0xEF]
    // tbl_vec at 36: len=1, elem0 uoffset at 40 => 48 (rel=8)
    // elem0 table at 48 (soff=-8 => vtable at 56), slot4=u8(99) at 52
    let buf: [64]u8 = [0; 64];
    let b = buf[0..64];

    putU32LE(*b, 0, 12);
    putU16LE(*b, 4, 8);
    putU16LE(*b, 6, 12);
    putU16LE(*b, 8, 4); // slot 4: bytes_vec
    putU16LE(*b, 10, 8); // slot 6: tbl_vec

    putU32LE(*b, 12, 8); // soff = 12 - 4 = 8
    putU32LE(*b, 16, 12); // 16 + 12 = 28
    putU32LE(*b, 20, 16); // 20 + 16 = 36

    // bytes_vec at 28
    putU32LE(*b, 28, 4);
    b[32] = 0xDE;
    b[33] = 0xAD;
    b[34] = 0xBE;
    b[35] = 0xEF;

    // tbl_vec at 36
    putU32LE(*b, 36, 1);
    putU32LE(*b, 40, 8); // 40 + 8 = 48

    // elem0 table at 48
    const neg8: i32 = -8;
    putU32LE(*b, 48, neg8 as u32);
    b[52] = 99;
    putU16LE(*b, 56, 6);
    putU16LE(*b, 58, 8);
    putU16LE(*b, 60, 4);

    const root = fbRootTable(b) ?? unreachable;
    const bv = root.getVector(4) ?? unreachable;
    const raw = bv.byteSlice() ?? unreachable;
    expect_eq(raw.len, 4);
    expect_eq(raw[0], 0xDE);
    expect_eq(raw[3], 0xEF);
    expect_eq(bv.getU8(1) ?? 0, 0xAD);

    const tv = root.getVector(6) ?? unreachable;
    const elem = tv.getTable(0) ?? unreachable;
    expect_eq(elem.getU8(4) ?? 0, 99);
    expect_eq(tv.getTable(1) == null, true);
});

test("5. truncated buffers, out-of-bounds vtable soffset, invalid vtable_len, and oversized string fail safely", () => {
    const short_buf: [3]u8 = [0, 0, 0];
    expect_eq(fbRootTable(short_buf[0..3]) == null, true);
    expect_eq(fbFileIdentifier(short_buf[0..3]) == null, true);
    expect_eq(fbStripSizePrefix(short_buf[0..3]) == null, true);

    // Root offset out of bounds
    let bad: [32]u8 = [0; 32];
    let b = bad[0..32];
    putU32LE(*b, 0, 999);
    expect_eq(fbRootTable(b) == null, true);

    // Positive soffset underflowing before offset 0 (pos=8, soff=100 => -92)
    putU32LE(*b, 0, 8);
    putU32LE(*b, 8, 100);
    expect_eq(fbRootTable(b) == null, true);

    // Invalid vtable_len (< 4 or exceeding buffer)
    putU32LE(*b, 0, 12);
    putU32LE(*b, 12, 8); // vtable at 4
    putU16LE(*b, 4, 2); // vtable_len = 2 < 4
    expect_eq(fbRootTable(b) == null, true);
    putU16LE(*b, 4, 200); // vtable_len = 200 > remaining
    expect_eq(fbRootTable(b) == null, true);

    // Valid table pointing to oversized string (len = 500 > buffer)
    putU16LE(*b, 4, 6);
    putU16LE(*b, 6, 8);
    putU16LE(*b, 8, 4); // slot 4 => +4 (offset 16)
    putU32LE(*b, 16, 4); // string at 20
    putU32LE(*b, 20, 500); // claims 500 bytes
    const tbl = fbRootTable(b) ?? unreachable;
    expect_eq(tbl.getString(4) == null, true);
});
