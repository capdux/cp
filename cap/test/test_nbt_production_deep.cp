// Source: cp/cap/test/test_nbt_production_deep.cp (Production deep tests for cap:nbt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MAX_NBT_DEPTH,
    NbtTagType,
    NbtEndian,
    NbtTagHeader,
    NbtListHeader,
    NbtFieldRef,
    NbtReader,
    isNbtCompound,
    findNbtField,
    findNbtFieldEndian,
} from "cap:binfmt";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn putU16(buf: []u8, pos: usize, val: u16, little: bool): usize {
    if (!little) {
        buf[pos] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 1] = (val & 0xFF) as u8;
    } else {
        buf[pos] = (val & 0xFF) as u8;
        buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
    }
    return pos + 2;
}

fn putU32(buf: []u8, pos: usize, val: u32, little: bool): usize {
    if (!little) {
        buf[pos] = ((val >> 24) & 0xFF) as u8;
        buf[pos + 1] = ((val >> 16) & 0xFF) as u8;
        buf[pos + 2] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 3] = (val & 0xFF) as u8;
    } else {
        buf[pos] = (val & 0xFF) as u8;
        buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
        buf[pos + 3] = ((val >> 24) & 0xFF) as u8;
    }
    return pos + 4;
}

fn putU64(buf: []u8, pos: usize, val: u64, little: bool): usize {
    if (!little) {
        buf[pos] = ((val >> 56) & 0xFF) as u8;
        buf[pos + 1] = ((val >> 48) & 0xFF) as u8;
        buf[pos + 2] = ((val >> 40) & 0xFF) as u8;
        buf[pos + 3] = ((val >> 32) & 0xFF) as u8;
        buf[pos + 4] = ((val >> 24) & 0xFF) as u8;
        buf[pos + 5] = ((val >> 16) & 0xFF) as u8;
        buf[pos + 6] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 7] = (val & 0xFF) as u8;
    } else {
        buf[pos] = (val & 0xFF) as u8;
        buf[pos + 1] = ((val >> 8) & 0xFF) as u8;
        buf[pos + 2] = ((val >> 16) & 0xFF) as u8;
        buf[pos + 3] = ((val >> 24) & 0xFF) as u8;
        buf[pos + 4] = ((val >> 32) & 0xFF) as u8;
        buf[pos + 5] = ((val >> 40) & 0xFF) as u8;
        buf[pos + 6] = ((val >> 48) & 0xFF) as u8;
        buf[pos + 7] = ((val >> 56) & 0xFF) as u8;
    }
    return pos + 8;
}

fn putStr(buf: []u8, pos: usize, s: []const u8, little: bool): usize {
    let p = putU16(buf, pos, s.len as u16, little);
    let i: usize = 0;
    while (i < s.len) {
        buf[p + i] = s[i];
        i += 1;
    }
    return p + s.len;
}

fn putTag(buf: []u8, pos: usize, tag: u8, name: []const u8, little: bool): usize {
    buf[pos] = tag;
    return putStr(buf, pos + 1, name, little);
}

test("1. Java Big-Endian full nested Compound + List + scalars + String decoding", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let p: usize = 0;

    // Root Compound "level"
    p = putTag(buf[0..], p, 10, "level", false);

    // Byte "flag" = -5
    p = putTag(buf[0..], p, 1, "flag", false);
    buf[p] = 0xFB; // -5 as i8
    p += 1;

    // Short "ver" = 258
    p = putTag(buf[0..], p, 2, "ver", false);
    p = putU16(buf[0..], p, 258, false);

    // Int "score" = 100000
    p = putTag(buf[0..], p, 3, "score", false);
    p = putU32(buf[0..], p, 100000, false);

    // Long "seed" = 0x0102030405060708
    p = putTag(buf[0..], p, 4, "seed", false);
    p = putU64(buf[0..], p, 0x0102030405060708, false);

    // Float "ratio" = 1.5
    p = putTag(buf[0..], p, 5, "ratio", false);
    let fbits: u32 = 0;
    const fval: f32 = 1.5;
    unsafe { fbits = @bitCast(fval); }
    p = putU32(buf[0..], p, fbits, false);

    // Double "pi" = 3.14159
    p = putTag(buf[0..], p, 6, "pi", false);
    let dbits: u64 = 0;
    const dval: f64 = 3.14159;
    unsafe { dbits = @bitCast(dval); }
    p = putU64(buf[0..], p, dbits, false);

    // String "name" = "world"
    p = putTag(buf[0..], p, 8, "name", false);
    p = putStr(buf[0..], p, "world", false);

    // List "nums" of 2 Ints [10, 20]
    p = putTag(buf[0..], p, 9, "nums", false);
    buf[p] = 3; // elem_type = int
    p += 1;
    p = putU32(buf[0..], p, 2, false);
    p = putU32(buf[0..], p, 10, false);
    p = putU32(buf[0..], p, 20, false);

    // End root compound
    buf[p] = 0;
    p += 1;

    expect_eq(isNbtCompound(buf[0..p]), true);

    let r = NbtReader.init(buf[0..p]);
    const root = r.readRootHeader() ?? unreachable;
    expect_eq(root.tag_type, NbtTagType.compound);
    expect_eq(sliceEq(root.name, "level"), true);

    const e1 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e1.name, "flag"), true);
    expect_eq(r.readByte() ?? 0, -5);

    const e2 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e2.name, "ver"), true);
    expect_eq(r.readShort() ?? 0, 258);

    const e3 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e3.name, "score"), true);
    expect_eq(r.readInt() ?? 0, 100000);

    const e4 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e4.name, "seed"), true);
    expect_eq(r.readLong() ?? 0, 0x0102030405060708);

    const e5 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e5.name, "ratio"), true);
    const rv = r.readFloat() ?? 0.0;
    expect_eq(rv > 1.49 && rv < 1.51, true);

    const e6 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e6.name, "pi"), true);
    const dv = r.readDouble() ?? 0.0;
    expect_eq(dv > 3.1415 && dv < 3.1417, true);

    const e7 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e7.name, "name"), true);
    const sv = r.readString() ?? unreachable;
    expect_eq(sliceEq(sv, "world"), true);

    const e8 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e8.name, "nums"), true);
    const lh = r.readListHeader() ?? unreachable;
    expect_eq(lh.elem_type, NbtTagType.int);
    expect_eq(lh.count, 2);
    expect_eq(r.readInt() ?? 0, 10);
    expect_eq(r.readInt() ?? 0, 20);

    expect_eq(r.nextCompoundEntry() == null, true);
});

test("2. Bedrock Little-Endian decoding and findNbtFieldEndian", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let p: usize = 0;

    p = putTag(buf[0..], p, 10, "", true);
    p = putTag(buf[0..], p, 3, "port", true);
    p = putU32(buf[0..], p, 19132, true);
    p = putTag(buf[0..], p, 8, "motd", true);
    p = putStr(buf[0..], p, "bedrock", true);
    buf[p] = 0;
    p += 1;

    let r = NbtReader.initLittle(buf[0..p]);
    const root = r.readRootHeader() ?? unreachable;
    expect_eq(root.name.len, 0);

    const e1 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e1.name, "port"), true);
    expect_eq(r.readInt() ?? 0, 19132);

    const e2 = r.nextCompoundEntry() ?? unreachable;
    expect_eq(sliceEq(e2.name, "motd"), true);
    const motd = r.readString() ?? unreachable;
    expect_eq(sliceEq(motd, "bedrock"), true);

    const found = findNbtFieldEndian(buf[0..p], "motd", NbtEndian.little) ?? unreachable;
    expect_eq(found.tag_type, NbtTagType.string);
    let fr = NbtReader.initLittle(found.payload);
    const s = fr.readString() ?? unreachable;
    expect_eq(sliceEq(s, "bedrock"), true);
});

test("3. skipPayload traverses deeply nested Compound/List and findNbtField locates trailing field", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let p: usize = 0;

    p = putTag(buf[0..], p, 10, "root", false);

    // Nested compound "meta" containing a list of strings
    p = putTag(buf[0..], p, 10, "meta", false);
    p = putTag(buf[0..], p, 9, "tags", false);
    buf[p] = 8; // elem_type = string
    p += 1;
    p = putU32(buf[0..], p, 2, false);
    p = putStr(buf[0..], p, "alpha", false);
    p = putStr(buf[0..], p, "beta", false);
    buf[p] = 0; // end "meta"
    p += 1;

    // Target field after complex nested structure
    p = putTag(buf[0..], p, 3, "target", false);
    p = putU32(buf[0..], p, 424242, false);

    buf[p] = 0; // end root
    p += 1;

    const f = findNbtField(buf[0..p], "target") ?? unreachable;
    expect_eq(f.tag_type, NbtTagType.int);
    expect_eq(f.payload.len, 4);
    let vr = NbtReader.init(f.payload);
    expect_eq(vr.readInt() ?? 0, 424242);

    // Missing field returns null cleanly
    expect_eq(findNbtField(buf[0..p], "nonexistent") == null, true);
});

test("4. ByteArray, IntArray, and LongArray raw reads and negative/overflow length rejection", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let p: usize = 0;

    p = putTag(buf[0..], p, 10, "arrs", false);

    // ByteArray of 3 bytes
    p = putTag(buf[0..], p, 7, "ba", false);
    p = putU32(buf[0..], p, 3, false);
    buf[p] = 0xAA; buf[p + 1] = 0xBB; buf[p + 2] = 0xCC;
    p += 3;

    // IntArray of 2 ints (8 bytes)
    p = putTag(buf[0..], p, 11, "ia", false);
    p = putU32(buf[0..], p, 2, false);
    p = putU32(buf[0..], p, 111, false);
    p = putU32(buf[0..], p, 222, false);

    // LongArray of 1 long (8 bytes)
    p = putTag(buf[0..], p, 12, "la", false);
    p = putU32(buf[0..], p, 1, false);
    p = putU64(buf[0..], p, 0xCAFEBABE_12345678, false);

    buf[p] = 0;
    p += 1;

    let r = NbtReader.init(buf[0..p]);
    * = r.readRootHeader() ?? unreachable;

    * = r.nextCompoundEntry() ?? unreachable;
    const ba = r.readByteArray() ?? unreachable;
    expect_eq(ba.len, 3);
    expect_eq(ba[0], 0xAA);
    expect_eq(ba[2], 0xCC);

    * = r.nextCompoundEntry() ?? unreachable;
    const ia = r.readIntArrayRaw() ?? unreachable;
    expect_eq(ia.len, 8);

    * = r.nextCompoundEntry() ?? unreachable;
    const la = r.readLongArrayRaw() ?? unreachable;
    expect_eq(la.len, 8);

    // Negative array length (-1 = 0xFFFFFFFF) rejected
    let neg: [4]u8 = [0xFF, 0xFF, 0xFF, 0xFF];
    let nr = NbtReader.init(neg[0..]);
    expect_eq(nr.readByteArray() == null, true);

    // Huge IntArray length (0x40000000) rejected without overflow
    let huge: [4]u8 = [0x40, 0x00, 0x00, 0x00];
    let hr = NbtReader.init(huge[0..]);
    expect_eq(hr.readIntArrayRaw() == null, true);
});

test("5. Malformed NBT protection: invalid tag_type, truncated payload, and excessive nesting depth", () => {
    // Non-compound root rejected
    let non_comp: [4]u8 = [3, 0, 0, 0];
    expect_eq(isNbtCompound(non_comp[0..]), false);

    // Invalid tag_type (99) inside compound rejected
    let bad_tag: [5]u8 = [10, 0, 0, 99, 0];
    expect_eq(findNbtField(bad_tag[0..], "x") == null, true);

    // Truncated string payload rejected
    let trunc_str: [7]u8 = [10, 0, 0, 8, 0, 1, 'a'];
    expect_eq(findNbtField(trunc_str[0..], "a") == null, true);

    // Excessive nesting (> MAX_NBT_DEPTH = 64 nested compounds) rejected safely
    let deep: [256]u8 = std.mem.zeroes([256]u8);
    let dp: usize = 0;
    // Build 70 nested anonymous compounds: [10, 0, 0] repeated 70 times = 210 bytes
    let k: u32 = 0;
    while (k < MAX_NBT_DEPTH + 6) {
        deep[dp] = 10;
        deep[dp + 1] = 0;
        deep[dp + 2] = 0;
        dp += 3;
        k += 1;
    }
    let dr = NbtReader.init(deep[0..dp]);
    * = dr.readRootHeader() ?? unreachable;
    expect_eq(dr.skipPayload(NbtTagType.compound), false);
});
