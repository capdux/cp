// Source: cp/cap/test/test_ion_production_deep.cp (Production deep tests for cap:ion)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ION_IVM_1_0,
    IonType,
    IonValue,
    IonReader,
    IonListIter,
    IonStructIter,
    isIonBinary,
    readIonVarUInt,
    readIonVarInt,
    ionAsBool,
    ionAsUInt,
    ionAsInt,
    ionAsFloat,
    ionAsSymbolId,
    ionAsString,
    ionAsBlob,
} from "cap:binfmt";

test("1. IVM 1.0 detection and all scalar Ion types (null, bool, pos_int, neg_int, f32, f64, symbol, string, blob)", () => {
    expect_eq(isIonBinary(ION_IVM_1_0), true);

    // Stream with IVM 1.0 + 9 scalar values:
    // 0: null.null     (0x0F)
    // 1: bool true     (0x11)
    // 2: bool false    (0x10)
    // 3: pos_int 300   (0x22, 0x01, 0x2C)
    // 4: neg_int -500  (0x32, 0x01, 0xF4)
    // 5: float32 2.5   (0x44, 0x40, 0x20, 0x00, 0x00)
    // 6: float64 0.0   (0x40)
    // 7: symbol 10     (0x71, 0x0A)
    // 8: string "ion"  (0x83, 'i', 'o', 'n')
    // 9: blob [0xAB, 0xCD] (0xA2, 0xAB, 0xCD)
    const stream: [29]u8 = [
        0xE0, 0x01, 0x00, 0xEA,
        0x0F,
        0x11,
        0x10,
        0x22, 0x01, 0x2C,
        0x32, 0x01, 0xF4,
        0x44, 0x40, 0x20, 0x00, 0x00,
        0x40,
        0x71, 0x0A,
        0x83, 'i', 'o', 'n',
        0xA2, 0xAB, 0xCD,
        0x8F, // null.string
    ];

    expect_eq(isIonBinary(stream[0..29]), true);
    let r = IonReader.init(stream[0..29]);

    const v0 = r.next() ?? unreachable;
    expect_eq(v0.ion_type == IonType.null_nop, true);
    expect_eq(v0.is_null, true);

    expect_eq(ionAsBool(r.next() ?? unreachable) ?? false, true);
    expect_eq(ionAsBool(r.next() ?? unreachable) ?? true, false);

    const v3 = r.next() ?? unreachable;
    expect_eq(ionAsUInt(v3) ?? 0, 300);
    expect_eq(ionAsInt(v3) ?? 0, 300);

    const v4 = r.next() ?? unreachable;
    expect_eq(ionAsInt(v4) ?? 0, -500);
    expect_eq(ionAsUInt(v4) == null, true);

    expect_eq(ionAsFloat(r.next() ?? unreachable) ?? 0.0, 2.5);
    expect_eq(ionAsFloat(r.next() ?? unreachable) ?? 1.0, 0.0);
    expect_eq(ionAsSymbolId(r.next() ?? unreachable) ?? 0, 10);
    expect_eq(std.mem.eql(u8, ionAsString(r.next() ?? unreachable) ?? "", "ion"), true);

    const bl = ionAsBlob(r.next() ?? unreachable) ?? unreachable;
    expect_eq(bl.len, 2);
    expect_eq(bl[0], 0xAB);
    expect_eq(bl[1], 0xCD);

    const v_null_str = r.next() ?? unreachable;
    expect_eq(v_null_str.ion_type == IonType.string, true);
    expect_eq(v_null_str.is_null, true);
    expect_eq(ionAsString(v_null_str) == null, true);

    expect_eq(r.next() == null, true);
});

test("2. Ion VarUInt and VarInt multi-byte big-endian decoding (high bit 0x80 terminates)", () => {
    // VarUInt 0 => [0x80]
    const vu0: [1]u8 = [0x80];
    expect_eq((readIonVarUInt(vu0[0..1], 0) ?? unreachable).value, 0);

    // VarUInt 127 => [0xFF]
    const vu127: [1]u8 = [0xFF];
    expect_eq((readIonVarUInt(vu127[0..1], 0) ?? unreachable).value, 127);

    // VarUInt 128 => [0x01, 0x80]
    const vu128: [2]u8 = [0x01, 0x80];
    const r128 = readIonVarUInt(vu128[0..2], 0) ?? unreachable;
    expect_eq(r128.value, 128);
    expect_eq(r128.next_pos, 2);

    // VarInt +14 => [0x8E], -14 => [0xCE]
    const vi_pos: [1]u8 = [0x8E];
    expect_eq((readIonVarInt(vi_pos[0..1], 0) ?? unreachable).value, 14);
    const vi_neg: [1]u8 = [0xCE];
    const r_neg = readIonVarInt(vi_neg[0..1], 0) ?? unreachable;
    expect_eq(r_neg.value, -14);
    expect_eq(r_neg.is_negative, true);

    // Multi-byte VarInt -1000 (1000 = (7 << 7) | 104 => b0 = 0x40 | 7 = 0x47, b1 = 0x80 | 104 = 0xE8)
    const vi_m1000: [2]u8 = [0x47, 0xE8];
    expect_eq((readIonVarInt(vi_m1000[0..2], 0) ?? unreachable).value, -1000);
});

test("3. nested Struct (IonStructIter) and List (IonListIter) decoding with field symbol IDs", () => {
    // IVM + Struct (len = 9):
    //   field sid=4 (0x84): string "ok" (0x82, 'o', 'k')  [4B]
    //   field sid=5 (0x85): list (0xB3: [pos_int 1 (0x21, 0x01), bool true (0x11)]) [5B]
    // Total struct payload = 9B => descriptor = 0xD9
    const data: [14]u8 = [
        0xE0, 0x01, 0x00, 0xEA,
        0xD9,
        0x84, 0x82, 'o', 'k',
        0x85, 0xB3, 0x21, 0x01, 0x11,
    ];

    let r = IonReader.init(data[0..14]);
    const st = r.next() ?? unreachable;
    expect_eq(st.ion_type == IonType.struct_val, true);

    let sit = IonStructIter.init(st.payload);
    const f0 = sit.next() ?? unreachable;
    expect_eq(f0.field_symbol_id, 4);
    expect_eq(std.mem.eql(u8, ionAsString(f0.value) ?? "", "ok"), true);

    const f1 = sit.next() ?? unreachable;
    expect_eq(f1.field_symbol_id, 5);
    expect_eq(f1.value.ion_type == IonType.list, true);

    let lit = IonListIter.init(f1.value.payload);
    expect_eq(ionAsUInt(lit.next() ?? unreachable) ?? 0, 1);
    expect_eq(ionAsBool(lit.next() ?? unreachable) ?? false, true);
    expect_eq(lit.next() == null, true);

    expect_eq(sit.next() == null, true);
});

test("4. extended length L==14 (>=14B string) and NOP Pad automatic skipping", () => {
    // IVM + 1B NOP (0x00) + 3B NOP (0x02, 0x00, 0x00) + 16B string (0x8E, 0x90, "0123456789ABCDEF")
    const buf: [26]u8 = [
        0xE0, 0x01, 0x00, 0xEA,
        0x00,
        0x02, 0x00, 0x00,
        0x8E, 0x90,
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    ];

    let r = IonReader.init(buf[0..26]);
    const v = r.next() ?? unreachable;
    expect_eq(v.ion_type == IonType.string, true);
    expect_eq(std.mem.eql(u8, ionAsString(v) ?? "", "0123456789ABCDEF"), true);
    expect_eq(r.next() == null, true);
});

test("5. wrong IVM, truncated VarUInt/payload, reserved type, and illegal bool L fail safely", () => {
    const bad_ivm: [4]u8 = [0xE0, 0x02, 0x00, 0xEA];
    expect_eq(isIonBinary(bad_ivm[0..4]), false);
    expect_eq(isIonBinary(bad_ivm[0..3]), false);

    // Unterminated VarUInt / VarInt (no 0x80 stop bit)
    const bad_vu: [3]u8 = [0x01, 0x02, 0x03];
    expect_eq(readIonVarUInt(bad_vu[0..3], 0) == null, true);
    expect_eq(readIonVarInt(bad_vu[0..3], 0) == null, true);

    // Truncated string claiming 10 bytes with only 2 available
    const trunc: [7]u8 = [0xE0, 0x01, 0x00, 0xEA, 0x8A, 'a', 'b'];
    let rt = IonReader.init(trunc[0..7]);
    expect_eq(rt.next() == null, true);

    // Reserved type T=15 (0xF0) and invalid bool L=2 (0x12)
    const bad_td: [5]u8 = [0xE0, 0x01, 0x00, 0xEA, 0xF0];
    let r1 = IonReader.init(bad_td[0..5]);
    expect_eq(r1.next() == null, true);

    const bad_bool: [5]u8 = [0xE0, 0x01, 0x00, 0xEA, 0x12];
    let r2 = IonReader.init(bad_bool[0..5]);
    expect_eq(r2.next() == null, true);
});
