// Source: cp/cap/test/test_varint_leb128_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    VarintError,
    zigzagEncode64,
    zigzagDecode64,
    zigzagEncode32,
    zigzagDecode32,
    encodeUleb128,
    decodeUleb128,
    encodeSleb128,
    decodeSleb128,
} from "cap:binfmt";

test "varint_leb128: DWARF official ULEB128 vector (624485 -> 0xE5 0x8E 0x26)" {
    let buf: [10]u8 = undefined;
    const n = try encodeUleb128(624485, &buf);
    try expect_eq(n, 3);
    try expect_eq(buf[0], 0xE5);
    try expect_eq(buf[1], 0x8E);
    try expect_eq(buf[2], 0x26);

    const dec_res = try decodeUleb128(buf[0..n]);
    try expect_eq(dec_res.value, 624485);
    try expect_eq(dec_res.bytes_read, 3);
}

test "varint_leb128: DWARF official SLEB128 negative vector (-123456 -> 0xC0 0xBB 0x78)" {
    let buf: [10]u8 = undefined;
    const n = try encodeSleb128(-123456, &buf);
    try expect_eq(n, 3);
    try expect_eq(buf[0], 0xC0);
    try expect_eq(buf[1], 0xBB);
    try expect_eq(buf[2], 0x78);

    const dec_res = try decodeSleb128(buf[0..n]);
    try expect_eq(dec_res.value, -123456);
    try expect_eq(dec_res.bytes_read, 3);
}

test "varint_leb128: ZigZag 32/64-bit roundtrip & extremes" {
    try expect_eq(zigzagEncode64(0), 0);
    try expect_eq(zigzagEncode64(-1), 1);
    try expect_eq(zigzagEncode64(1), 2);
    try expect_eq(zigzagEncode64(-2), 3);
    try expect_eq(zigzagDecode64(zigzagEncode64(std.math.minInt(i64))), std.math.minInt(i64));
    try expect_eq(zigzagDecode64(zigzagEncode64(std.math.maxInt(i64))), std.math.maxInt(i64));
    try expect_eq(zigzagDecode32(zigzagEncode32(-12345)), -12345);
}

test "varint_leb128: max u64, EOF, Overflow, and BufferTooSmall guards" {
    let buf: [10]u8 = undefined;
    const n = try encodeUleb128(std.math.maxInt(u64), &buf);
    try expect_eq(n, 10);
    try expect_eq((try decodeUleb128(buf[0..n])).value, std.math.maxInt(u64));

    let tiny: [1]u8 = undefined;
    try expect_eq(encodeUleb128(300, &tiny) == VarintError.BufferTooSmall, true);
    const trunc: [2]u8 = [ 0x80, 0x80 ];
    try expect_eq(decodeUleb128(&trunc) == VarintError.UnexpectedEof, true);
    const over: [10]u8 = [ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02 ];
    try expect_eq(decodeUleb128(&over) == VarintError.Overflow, true);
}

test "varint_leb128: cap.varint_leb128 dynamic mount" {
    let buf: [10]u8 = undefined;
    const n = try cap.binfmt.encodeSleb128(-42, &buf);
    const res = try cap.binfmt.decodeSleb128(buf[0..n]);
    try expect_eq(res.value, -42);
}
