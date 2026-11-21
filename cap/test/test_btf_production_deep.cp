// Source: cp/cap/test/test_btf_production_deep.cp (Production deep tests for cap:btf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BTF_MAGIC,
    BTF_VERSION,
    BTF_HEADER_SIZE,
    BTF_TYPE_HEADER_SIZE,
    KIND_UNKN,
    KIND_INT,
    KIND_PTR,
    KIND_ARRAY,
    KIND_STRUCT,
    KIND_UNION,
    KIND_ENUM,
    KIND_FWD,
    KIND_TYPEDEF,
    KIND_VOLATILE,
    KIND_CONST,
    KIND_RESTRICT,
    KIND_FUNC,
    KIND_FUNC_PROTO,
    KIND_VAR,
    KIND_DATASEC,
    KIND_FLOAT,
    KIND_DECL_TAG,
    KIND_TYPE_TAG,
    KIND_ENUM64,
    INT_SIGNED,
    INT_CHAR,
    INT_BOOL,
    BtfHeader,
    BtfType,
    BtfIntInfo,
    BtfArray,
    BtfMember,
    BtfTypeIter,
    BtfSection,
    isBtf,
    parseBtf,
    parseBtfInt,
    parseBtfArray,
    btfMemberAt,
} from "cap:btf";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off + 0] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off + 0] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putBytes(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

fn makeInfo(kind: u8, vlen: u16, kflag: bool): u32 {
    let kf: u32 = 0;
    if (kflag) {
        kf = (1 as u32) << 31;
    }
    return kf | ((kind as u32) << 24) | (vlen as u32);
}

fn slicesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. 24B BTF Header parsing and zero-copy string table lookup (stringAt)", () => {
    expect_eq(BTF_MAGIC, 0xEB9F as u16);
    expect_eq(BTF_VERSION, 1 as u8);
    expect_eq(BTF_HEADER_SIZE, 24 as usize);
    expect_eq(BTF_TYPE_HEADER_SIZE, 12 as usize);

    // String table: "\0int\0pkt_hdr\0" (len = 13)
    let raw: [64]u8 = std.mem.zeroes([64]u8);
    putU16Le(raw[0..], 0, BTF_MAGIC);
    raw[2] = BTF_VERSION;
    raw[3] = 0;
    putU32Le(raw[0..], 4, 24);  // hdr_len
    putU32Le(raw[0..], 8, 0);   // type_off
    putU32Le(raw[0..], 12, 0);  // type_len
    putU32Le(raw[0..], 16, 0);  // str_off
    putU32Le(raw[0..], 20, 13); // str_len

    raw[24] = 0;
    putBytes(raw[0..], 25, "int");
    raw[28] = 0;
    putBytes(raw[0..], 29, "pkt_hdr");
    raw[36] = 0;

    expect_eq(isBtf(raw[0..37]), true);
    if (sec := parseBtf(raw[0..37])) {
        expect_eq(sec.header.magic, BTF_MAGIC);
        expect_eq(sec.header.version, 1 as u8);
        expect_eq(sec.header.hdr_len, 24 as u32);
        expect_eq(sec.str_bytes.len, 13 as usize);

        if (s0 := sec.stringAt(0)) {
            expect_eq(s0.len, 0 as usize);
        } else {
            expect_eq(true, false);
        }
        if (s1 := sec.stringAt(1)) {
            expect_eq(slicesEq(s1, "int"), true);
        } else {
            expect_eq(true, false);
        }
        if (s2 := sec.stringAt(5)) {
            expect_eq(slicesEq(s2, "pkt_hdr"), true);
        } else {
            expect_eq(true, false);
        }
        // Out-of-bounds string offset returns null
        if (_ := sec.stringAt(13)) {
            expect_eq(true, false);
        } else {
            expect_eq(true, true);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. Full BTF type graph traversal (INT -> PTR -> ARRAY -> STRUCT w/ bitfields -> ENUM -> FUNC_PROTO -> FUNC -> VAR -> DATASEC -> ENUM64)", () => {
    let raw: [512]u8 = std.mem.zeroes([512]u8);
    let pos: usize = 24;

    // Type 1: KIND_INT (12B + 4B = 16B) - signed 32-bit int
    putU32Le(raw[0..], pos + 0, 1);                             // name_off -> "i32"
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_INT, 0, false));
    putU32Le(raw[0..], pos + 8, 4);                             // size = 4 bytes
    putU32Le(raw[0..], pos + 12, ((INT_SIGNED as u32) << 24) | 32);
    pos += 16;

    // Type 2: KIND_PTR (12B + 0B = 12B) -> points to type 1
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_PTR, 0, false));
    putU32Le(raw[0..], pos + 8, 1);
    pos += 12;

    // Type 3: KIND_ARRAY (12B + 12B = 24B) -> [16]type_1
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_ARRAY, 0, false));
    putU32Le(raw[0..], pos + 8, 0);
    putU32Le(raw[0..], pos + 12, 1);   // elem_type
    putU32Le(raw[0..], pos + 16, 1);   // index_type
    putU32Le(raw[0..], pos + 20, 16);  // nelems
    pos += 24;

    // Type 4: KIND_STRUCT (12B + 2*12B = 36B) with kflag=true bitfields
    putU32Le(raw[0..], pos + 0, 5);                             // name_off -> "flags"
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_STRUCT, 2, true));
    putU32Le(raw[0..], pos + 8, 4);                             // struct size = 4
    // Member 0: name_off=11 ("a"), type_id=1, bitfield_size=3, bit_offset=0
    putU32Le(raw[0..], pos + 12, 11);
    putU32Le(raw[0..], pos + 16, 1);
    putU32Le(raw[0..], pos + 20, ((3 as u32) << 24) | 0);
    // Member 1: name_off=13 ("b"), type_id=1, bitfield_size=5, bit_offset=3
    putU32Le(raw[0..], pos + 24, 13);
    putU32Le(raw[0..], pos + 28, 1);
    putU32Le(raw[0..], pos + 32, ((5 as u32) << 24) | 3);
    pos += 36;

    // Type 5: KIND_ENUM (12B + 2*8B = 28B)
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_ENUM, 2, false));
    putU32Le(raw[0..], pos + 8, 4);
    pos += 28;

    // Type 6: KIND_FUNC_PROTO (12B + 1*8B = 20B)
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_FUNC_PROTO, 1, false));
    putU32Le(raw[0..], pos + 8, 1); // return type 1
    pos += 20;

    // Type 7: KIND_FUNC (12B + 0B = 12B)
    putU32Le(raw[0..], pos + 0, 15); // "fn0"
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_FUNC, 0, false));
    putU32Le(raw[0..], pos + 8, 6);  // proto type 6
    pos += 12;

    // Type 8: KIND_VAR (12B + 4B = 16B)
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_VAR, 0, false));
    putU32Le(raw[0..], pos + 8, 1);
    putU32Le(raw[0..], pos + 12, 1); // linkage = global
    pos += 16;

    // Type 9: KIND_DATASEC (12B + 1*12B = 24B)
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_DATASEC, 1, false));
    putU32Le(raw[0..], pos + 8, 4);
    pos += 24;

    // Type 10: KIND_ENUM64 (12B + 1*12B = 24B)
    putU32Le(raw[0..], pos + 0, 0);
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_ENUM64, 1, true));
    putU32Le(raw[0..], pos + 8, 8);
    pos += 24;

    const type_len = pos - 24;
    const str_start = pos;
    // "\0i32\0flags\0a\0b\0fn0\0" (19 bytes)
    raw[pos] = 0; pos += 1;
    putBytes(raw[0..], pos, "i32"); pos += 3; raw[pos] = 0; pos += 1;
    putBytes(raw[0..], pos, "flags"); pos += 5; raw[pos] = 0; pos += 1;
    putBytes(raw[0..], pos, "a"); pos += 1; raw[pos] = 0; pos += 1;
    putBytes(raw[0..], pos, "b"); pos += 1; raw[pos] = 0; pos += 1;
    putBytes(raw[0..], pos, "fn0"); pos += 3; raw[pos] = 0; pos += 1;
    const str_len = pos - str_start;

    // Write 24B header
    putU16Le(raw[0..], 0, BTF_MAGIC);
    raw[2] = BTF_VERSION;
    putU32Le(raw[0..], 4, 24);
    putU32Le(raw[0..], 8, 0);
    putU32Le(raw[0..], 12, type_len as u32);
    putU32Le(raw[0..], 16, type_len as u32);
    putU32Le(raw[0..], 20, str_len as u32);

    if (sec := parseBtf(raw[0..pos])) {
        let it = sec.types();
        let count: u32 = 0;

        while (t := it.next()) {
            count += 1;
            expect_eq(t.type_id, count);

            if (count == 1) {
                expect_eq(t.kind, KIND_INT);
                if (ii := parseBtfInt(t)) {
                    expect_eq(ii.isSigned(), true);
                    expect_eq(ii.isChar(), false);
                    expect_eq(ii.isBool(), false);
                    expect_eq(ii.bits, 32 as u8);
                    expect_eq(ii.bit_offset, 0 as u8);
                } else {
                    expect_eq(true, false);
                }
            } else if (count == 2) {
                expect_eq(t.kind, KIND_PTR);
                expect_eq(t.size_or_type, 1 as u32);
                expect_eq(t.extra.len, 0 as usize);
            } else if (count == 3) {
                expect_eq(t.kind, KIND_ARRAY);
                if (arr := parseBtfArray(t)) {
                    expect_eq(arr.elem_type, 1 as u32);
                    expect_eq(arr.index_type, 1 as u32);
                    expect_eq(arr.nelems, 16 as u32);
                } else {
                    expect_eq(true, false);
                }
            } else if (count == 4) {
                expect_eq(t.kind, KIND_STRUCT);
                expect_eq(t.kflag, true);
                expect_eq(t.vlen, 2 as u16);
                if (m0 := btfMemberAt(t, 0)) {
                    expect_eq(m0.bitOffset(t.kflag), 0 as u32);
                    expect_eq(m0.bitfieldSize(t.kflag), 3 as u8);
                } else {
                    expect_eq(true, false);
                }
                if (m1 := btfMemberAt(t, 1)) {
                    expect_eq(m1.bitOffset(t.kflag), 3 as u32);
                    expect_eq(m1.bitfieldSize(t.kflag), 5 as u8);
                } else {
                    expect_eq(true, false);
                }
                if (_ := btfMemberAt(t, 2)) {
                    expect_eq(true, false);
                } else {
                    expect_eq(true, true);
                }
            } else if (count == 5) {
                expect_eq(t.kind, KIND_ENUM);
                expect_eq(t.extra.len, 16 as usize);
            } else if (count == 6) {
                expect_eq(t.kind, KIND_FUNC_PROTO);
                expect_eq(t.extra.len, 8 as usize);
            } else if (count == 7) {
                expect_eq(t.kind, KIND_FUNC);
                if (fn_name := sec.stringAt(t.name_off)) {
                    expect_eq(slicesEq(fn_name, "fn0"), true);
                } else {
                    expect_eq(true, false);
                }
            } else if (count == 8) {
                expect_eq(t.kind, KIND_VAR);
                expect_eq(t.extra.len, 4 as usize);
            } else if (count == 9) {
                expect_eq(t.kind, KIND_DATASEC);
                expect_eq(t.extra.len, 12 as usize);
            } else if (count == 10) {
                expect_eq(t.kind, KIND_ENUM64);
                expect_eq(t.extra.len, 12 as usize);
            }
        }
        expect_eq(count, 10 as u32);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
});

test("3. Non-bitfield struct member offsets, CHAR/BOOL INT flags, and remaining zero-payload kinds", () => {
    let raw: [160]u8 = std.mem.zeroes([160]u8);
    let pos: usize = 24;

    // Type 1: INT_CHAR | INT_BOOL
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_INT, 0, false));
    putU32Le(raw[0..], pos + 8, 1);
    putU32Le(raw[0..], pos + 12, (((INT_CHAR | INT_BOOL) as u32) << 24) | 8);
    pos += 16;

    // Type 2: KIND_STRUCT (kflag=false, vlen=1)
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_STRUCT, 1, false));
    putU32Le(raw[0..], pos + 8, 8);
    putU32Le(raw[0..], pos + 12, 0);
    putU32Le(raw[0..], pos + 16, 1);
    putU32Le(raw[0..], pos + 20, 64); // bit offset = 64
    pos += 24;

    // Type 3: KIND_DECL_TAG (12B + 4B = 16B)
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_DECL_TAG, 0, false));
    pos += 16;

    // Type 4: KIND_FLOAT (12B)
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_FLOAT, 0, false));
    pos += 12;

    // Type 5: KIND_TYPE_TAG (12B)
    putU32Le(raw[0..], pos + 4, makeInfo(KIND_TYPE_TAG, 0, false));
    pos += 12;

    const type_len = pos - 24;
    putU16Le(raw[0..], 0, BTF_MAGIC);
    raw[2] = BTF_VERSION;
    putU32Le(raw[0..], 4, 24);
    putU32Le(raw[0..], 12, type_len as u32);

    if (sec := parseBtf(raw[0..pos])) {
        let it = sec.types();
        if (t1 := it.next()) {
            if (ii := parseBtfInt(t1)) {
                expect_eq(ii.isSigned(), false);
                expect_eq(ii.isChar(), true);
                expect_eq(ii.isBool(), true);
            } else {
                expect_eq(true, false);
            }
            // Wrong-kind helpers return null
            if (_ := parseBtfArray(t1)) {
                expect_eq(true, false);
            } else {
                expect_eq(true, true);
            }
        } else {
            expect_eq(true, false);
        }

        if (t2 := it.next()) {
            if (m := btfMemberAt(t2, 0)) {
                expect_eq(m.bitOffset(false), 64 as u32);
                expect_eq(m.bitfieldSize(false), 0 as u8);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }

        let rem: u32 = 0;
        while (_ := it.next()) {
            rem += 1;
        }
        expect_eq(rem, 3 as u32);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }

    // Sanity check remaining kind constants
    expect_eq(KIND_UNKN, 0 as u8);
    expect_eq(KIND_UNION, 5 as u8);
    expect_eq(KIND_FWD, 7 as u8);
    expect_eq(KIND_TYPEDEF, 8 as u8);
    expect_eq(KIND_VOLATILE, 9 as u8);
    expect_eq(KIND_CONST, 10 as u8);
    expect_eq(KIND_RESTRICT, 11 as u8);
});

test("4. Unknown Kind and truncated type payload trigger BtfTypeIter.hasError()", () => {
    // 4a. Unknown kind (kind = 25)
    let bad_kind: [40]u8 = std.mem.zeroes([40]u8);
    putU16Le(bad_kind[0..], 0, BTF_MAGIC);
    bad_kind[2] = BTF_VERSION;
    putU32Le(bad_kind[0..], 4, 24);
    putU32Le(bad_kind[0..], 12, 12);
    putU32Le(bad_kind[0..], 24 + 4, makeInfo(25, 0, false));

    if (sec := parseBtf(bad_kind[0..36])) {
        let it = sec.types();
        if (_ := it.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(it.hasError(), true);
        }
    } else {
        expect_eq(true, false);
    }

    // 4b. Truncated ARRAY payload (only 8B extra instead of 12B)
    let trunc_arr: [48]u8 = std.mem.zeroes([48]u8);
    putU16Le(trunc_arr[0..], 0, BTF_MAGIC);
    trunc_arr[2] = BTF_VERSION;
    putU32Le(trunc_arr[0..], 4, 24);
    putU32Le(trunc_arr[0..], 12, 20); // 12B hdr + 8B extra (needs 12B)
    putU32Le(trunc_arr[0..], 24 + 4, makeInfo(KIND_ARRAY, 0, false));

    if (sec := parseBtf(trunc_arr[0..44])) {
        let it = sec.types();
        if (_ := it.next()) {
            expect_eq(true, false);
        } else {
            expect_eq(it.hasError(), true);
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed BTF header rejection: bad magic, bad version, invalid hdr_len, and out-of-bounds sections", () => {
    let hdr: [32]u8 = std.mem.zeroes([32]u8);

    // 5a. Wrong magic
    putU16Le(hdr[0..], 0, 0x1234);
    hdr[2] = BTF_VERSION;
    putU32Le(hdr[0..], 4, 24);
    expect_eq(isBtf(hdr[0..]), false);
    if (_ := parseBtf(hdr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Wrong version (version = 2)
    putU16Le(hdr[0..], 0, BTF_MAGIC);
    hdr[2] = 2;
    expect_eq(isBtf(hdr[0..]), true);
    if (_ := parseBtf(hdr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. hdr_len < 24
    hdr[2] = BTF_VERSION;
    putU32Le(hdr[0..], 4, 16);
    if (_ := parseBtf(hdr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. type_len overflows data length
    putU32Le(hdr[0..], 4, 24);
    putU32Le(hdr[0..], 12, 100);
    if (_ := parseBtf(hdr[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5e. Truncated header (< 24B)
    expect_eq(isBtf(hdr[0..23]), false);
    if (_ := parseBtf(hdr[0..23])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
