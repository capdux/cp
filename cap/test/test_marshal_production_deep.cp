// Source: cp/cap/test/test_marshal_production_deep.cp (Production deep tests for cap:marshal)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MARSHAL_MAJOR,
    MARSHAL_MINOR,
    MAX_MARSHAL_DEPTH,
    TYPE_NIL,
    TYPE_TRUE,
    TYPE_FALSE,
    TYPE_FIXNUM,
    TYPE_STRING,
    TYPE_SYMBOL,
    TYPE_SYMLINK,
    TYPE_ARRAY,
    TYPE_HASH,
    TYPE_IVAR,
    TYPE_BIGNUM,
    MarshalHeader,
    MarshalIntResult,
    parseMarshalHeader,
    isMarshal,
    readMarshalInt,
    MarshalReader,
} from "cap:marshal";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Ruby Marshal Fixnum compressed integer boundary decoding across full signed range", () => {
    // 0 -> [0x00]
    const b0: [1]u8 = [0x00];
    expect_eq((readMarshalInt(b0[0..], 0) ?? unreachable).value, 0);

    // 1 -> [0x06] (1 + 5)
    const b1: [1]u8 = [0x06];
    expect_eq((readMarshalInt(b1[0..], 0) ?? unreachable).value, 1);

    // 122 -> [0x7F] (122 + 5)
    const b122: [1]u8 = [0x7F];
    expect_eq((readMarshalInt(b122[0..], 0) ?? unreachable).value, 122);

    // 123 -> [0x01, 0x7B]
    const b123: [2]u8 = [0x01, 0x7B];
    const r123 = readMarshalInt(b123[0..], 0) ?? unreachable;
    expect_eq(r123.value, 123);
    expect_eq(r123.next_pos, 2);

    // 65535 -> [0x02, 0xFF, 0xFF]
    const b65535: [3]u8 = [0x02, 0xFF, 0xFF];
    expect_eq((readMarshalInt(b65535[0..], 0) ?? unreachable).value, 65535);

    // 1_000_000 (0x0F4240) -> [0x03, 0x40, 0x42, 0x0F]
    const b1m: [4]u8 = [0x03, 0x40, 0x42, 0x0F];
    expect_eq((readMarshalInt(b1m[0..], 0) ?? unreachable).value, 1_000_000);

    // -1 -> [0xFA] (-6 + 5)
    const bm1: [1]u8 = [0xFA];
    expect_eq((readMarshalInt(bm1[0..], 0) ?? unreachable).value, -1);

    // -123 -> [0x80] (-128 + 5)
    const bm123: [1]u8 = [0x80];
    expect_eq((readMarshalInt(bm123[0..], 0) ?? unreachable).value, -123);

    // -124 (0xFFFFFF84) -> [0xFF, 0x84]
    const bm124: [2]u8 = [0xFF, 0x84];
    expect_eq((readMarshalInt(bm124[0..], 0) ?? unreachable).value, -124);

    // -1_000_000 (0xFFF0BDC0) -> [0xFD, 0xC0, 0xBD, 0xF0]
    const bm1m: [4]u8 = [0xFD, 0xC0, 0xBD, 0xF0];
    expect_eq((readMarshalInt(bm1m[0..], 0) ?? unreachable).value, -1_000_000);
});

test("2. IVAR UTF-8 wrapped Ruby 1.9+ string unwrapping and bare string reading", () => {
    // Marshal.dump("hello") in Ruby 3:
    // [4, 8, 'I', '"', 10(5+5), 'h','e','l','l','o', 6(1+5), ':', 6(1+5), 'E', 'T']
    const ivar_str: [15]u8 = [
        4, 8,
        TYPE_IVAR, TYPE_STRING, 10, 'h', 'e', 'l', 'l', 'o',
        6, TYPE_SYMBOL, 6, 'E', TYPE_TRUE,
    ];
    expect_eq(isMarshal(ivar_str[0..]), true);
    const hdr = parseMarshalHeader(ivar_str[0..]) ?? unreachable;
    expect_eq(hdr.major, MARSHAL_MAJOR);
    expect_eq(hdr.minor, MARSHAL_MINOR);

    let r = MarshalReader.init(ivar_str[0..]);
    const s = r.readString() ?? unreachable;
    expect_eq(sliceEq(s, "hello"), true);
    expect_eq(r.isEof(), true);

    // Bare string without IVAR wrapper: [4, 8, '"', 7(2+5), 'o', 'k']
    const bare: [6]u8 = [4, 8, TYPE_STRING, 7, 'o', 'k'];
    let rb = MarshalReader.init(bare[0..]);
    const sb = rb.readString() ?? unreachable;
    expect_eq(sliceEq(sb, "ok"), true);
    expect_eq(rb.isEof(), true);
});

test("3. Nested Hash, Array, Symbol, Symlink, Fixnum, Bool, and Nil decoding", () => {
    // { :a => [true, nil, 42], :a(symlink 0) => false }
    // Hash(2 pairs):
    //   ':' len=1 'a'  -> '[' len=3 'T' '0' 'i' 47(42+5)
    //   ';' idx=0      -> 'F'
    const data: [19]u8 = [
        4, 8,
        TYPE_HASH, 7, // 2 pairs (2+5)
        TYPE_SYMBOL, 6, 'a',
        TYPE_ARRAY, 8, // 3 elements (3+5)
        TYPE_TRUE, TYPE_NIL, TYPE_FIXNUM, 47,
        TYPE_SYMLINK, 0, // symlink index 0
        TYPE_FALSE,
        // trailing dummy not included, slice up to 16
        0, 0, 0,
    ];

    let r = MarshalReader.init(data[0..16]);
    expect_eq(r.readHashLen() ?? 0, 2);

    const k0 = r.readSymbol() ?? unreachable;
    expect_eq(sliceEq(k0, "a"), true);

    expect_eq(r.readArrayLen() ?? 0, 3);
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readNil(), true);
    expect_eq(r.readFixnum() ?? 0, 42);

    expect_eq(r.readSymlink() ?? 9, 0);
    expect_eq(r.readBool() ?? true, false);
    expect_eq(r.isEof(), true);
});

test("4. skipValue traverses complex nested Array + Bignum + IVAR and lands on trailing Fixnum", () => {
    // Top-level Array of 2 elements:
    //   elem 0: Hash(1 pair): :big => Bignum('+', 2 shorts = 4 bytes)
    //   elem 1: Fixnum(100 => 105)
    const data: [22]u8 = [
        4, 8,
        TYPE_ARRAY, 7, // 2 elements
        TYPE_HASH, 6,  // 1 pair
        TYPE_SYMBOL, 8, 'b', 'i', 'g',
        TYPE_BIGNUM, '+', 7, 0x01, 0x02, 0x03, 0x04,
        TYPE_FIXNUM, 105, // 100
        0, 0,
    ];

    let r = MarshalReader.init(data[0..20]);
    expect_eq(r.readArrayLen() ?? 0, 2);
    expect_eq(r.skipValue(), true);
    expect_eq(r.readFixnum() ?? 0, 100);
    expect_eq(r.isEof(), true);
});

test("5. Malformed Marshal protection: wrong version, truncated Fixnum/string, and excessive nesting depth", () => {
    // Wrong version header [4, 7]
    const bad_ver: [3]u8 = [4, 7, TYPE_NIL];
    expect_eq(isMarshal(bad_ver[0..]), false);
    let rv = MarshalReader.init(bad_ver[0..]);
    expect_eq(rv.readNil(), false);

    // Truncated 3-byte Fixnum (only 2 payload bytes present)
    const trunc_int: [5]u8 = [4, 8, TYPE_FIXNUM, 0x03, 0x01];
    let ri = MarshalReader.init(trunc_int[0..]);
    expect_eq(ri.readFixnum() == null, true);

    // Truncated string (declares 50 bytes, only 2 present)
    const trunc_str: [6]u8 = [4, 8, TYPE_STRING, 55, 'a', 'b'];
    let rs = MarshalReader.init(trunc_str[0..]);
    expect_eq(rs.readString() == null, true);

    // Excessive nesting (> MAX_MARSHAL_DEPTH = 64 nested 1-element arrays)
    let deep: [160]u8 = std.mem.zeroes([160]u8);
    deep[0] = 4;
    deep[1] = 8;
    let p: usize = 2;
    let k: u32 = 0;
    while (k < MAX_MARSHAL_DEPTH + 6) {
        deep[p] = TYPE_ARRAY;
        deep[p + 1] = 6; // len = 1
        p += 2;
        k += 1;
    }
    let rd = MarshalReader.init(deep[0..p]);
    expect_eq(rd.skipValue(), false);
});
