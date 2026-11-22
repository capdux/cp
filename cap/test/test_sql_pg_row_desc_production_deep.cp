import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PG_ROW_DESCRIPTION_TAG,
    PG_MAX_ROW_FIELDS,
    PgRowDescError,
    PgFieldFormatCode,
    PgFieldDescription,
    PgRowDescriptionIter,
    encodePgRowDescription,
    decodePgRowDescription,
} from "cap:sql";

test("1. encodePgRowDescription and decodePgRowDescription roundtrip with mixed text/binary columns", () => {
    let buf: [256]u8 = [0; 256];
    const cols: [3]PgFieldDescription = [
        PgFieldDescription{
            name: "id",
            table_oid: 16384,
            column_index: 1,
            type_oid: 23, // int4
            type_size: 4,
            type_modifier: -1,
            format_code: PgFieldFormatCode.binary,
        },
        PgFieldDescription{
            name: "username",
            table_oid: 16384,
            column_index: 2,
            type_oid: 1043, // varchar
            type_size: -1,
            type_modifier: 68, // varchar(64)
            format_code: PgFieldFormatCode.text,
        },
        PgFieldDescription{
            name: "0",
            table_oid: 0,
            column_index: 0,
            type_oid: 20, // int8
            type_size: 8,
            type_modifier: -1,
            format_code: PgFieldFormatCode.binary,
        },
    ];

    const wire = encodePgRowDescription(buf[0..], cols[0..]) catch unreachable;
    expect_eq(wire[0], PG_ROW_DESCRIPTION_TAG);

    let it = decodePgRowDescription(wire) catch unreachable;
    expect_eq(it.count(), usize(3));
    expect_eq(it.remainingFields(), usize(3));

    const c0 = (it.next() catch unreachable) ?? unreachable;
    expect_eq(std.mem.eql(u8, c0.name, "id"), true);
    expect_eq(c0.table_oid, i32(16384));
    expect_eq(c0.column_index, i16(1));
    expect_eq(c0.type_oid, i32(23));
    expect_eq(c0.type_size, i16(4));
    expect_eq(c0.type_modifier, i32(-1));
    expect_eq(c0.isBinary(), true);
    expect_eq(c0.numericColumnIndex() == null, true);

    const c1 = (it.next() catch unreachable) ?? unreachable;
    expect_eq(std.mem.eql(u8, c1.name, "username"), true);
    expect_eq(c1.type_oid, i32(1043));
    expect_eq(c1.type_modifier, i32(68));
    expect_eq(c1.isBinary(), false);

    const c2 = (it.next() catch unreachable) ?? unreachable;
    expect_eq(std.mem.eql(u8, c2.name, "0"), true);
    expect_eq(c2.numericColumnIndex() ?? 99, u32(0));
    expect_eq(it.remainingFields(), usize(0));
    expect_eq((it.next() catch unreachable) == null, true);
});

test("2. Zero-column RowDescription roundtrip", () => {
    let buf: [32]u8 = [0; 32];
    const empty: [0]PgFieldDescription = [];
    const wire = encodePgRowDescription(buf[0..], empty[0..]) catch unreachable;
    // 1B tag + 4B len(6) + 2B count(0) = 7 bytes
    expect_eq(wire.len, usize(7));

    let it = decodePgRowDescription(wire) catch unreachable;
    expect_eq(it.count(), usize(0));
    expect_eq((it.next() catch unreachable) == null, true);
});

test("3. PgFieldDescription.numericColumnIndex identifies valid decimal indices and rejects non-numeric/overflow", () => {
    const f_ok = PgFieldDescription{ name: "42" };
    expect_eq(f_ok.numericColumnIndex() ?? 0, u32(42));

    const f_empty = PgFieldDescription{ name: "" };
    expect_eq(f_empty.numericColumnIndex() == null, true);

    const f_alpha = PgFieldDescription{ name: "12a" };
    expect_eq(f_alpha.numericColumnIndex() == null, true);

    const f_huge = PgFieldDescription{ name: "99999999999" };
    expect_eq(f_huge.numericColumnIndex() == null, true);
});

test("4. Encoder BufferTooSmall guard on tight output buffer", () => {
    let tiny: [12]u8 = [0; 12];
    const cols: [1]PgFieldDescription = [
        PgFieldDescription{ name: "long_column_name", type_oid: 25 },
    ];
    let got_small = false;
    * = encodePgRowDescription(tiny[0..], cols[0..]) catch |e| {
        if (e == PgRowDescError.BufferTooSmall) got_small = true;
    };
    expect_eq(got_small, true);
    expect_eq(PG_MAX_ROW_FIELDS, usize(1664));
});

test("5. Decoder guards: InvalidTag, NegativeFieldCount, TooManyFields, InvalidFrame", () => {
    // 1. 非 'T' 标签 (例如 'D' DataRow)
    const wrong_tag: [7]u8 = ['D', 0, 0, 0, 6, 0, 0];
    let e1 = false;
    * = decodePgRowDescription(wrong_tag[0..]) catch |e| {
        if (e == PgRowDescError.InvalidTag) e1 = true;
    };
    expect_eq(e1, true);

    // 2. 负数列数 (-1 = 0xFFFF)
    const neg_cnt: [7]u8 = ['T', 0, 0, 0, 6, 0xFF, 0xFF];
    let e2 = false;
    * = decodePgRowDescription(neg_cnt[0..]) catch |e| {
        if (e == PgRowDescError.NegativeFieldCount) e2 = true;
    };
    expect_eq(e2, true);

    // 3. 超过 PG_MAX_ROW_FIELDS (2000 = 0x07D0)
    const too_many: [7]u8 = ['T', 0, 0, 0, 6, 0x07, 0xD0];
    let e3 = false;
    * = decodePgRowDescription(too_many[0..]) catch |e| {
        if (e == PgRowDescError.TooManyFields) e3 = true;
    };
    expect_eq(e3, true);

    // 4. 截断帧
    const trunc: [4]u8 = ['T', 0, 0, 0];
    let e4 = false;
    * = decodePgRowDescription(trunc[0..]) catch |e| {
        if (e == PgRowDescError.InvalidFrame) e4 = true;
    };
    expect_eq(e4, true);
});

test("6. Decoder field-level guards: UnknownFormatCode and ShortRead on truncated column body", () => {
    let buf: [64]u8 = [0; 64];
    const cols: [1]PgFieldDescription = [
        PgFieldDescription{ name: "x", type_oid: 23, format_code: PgFieldFormatCode.binary },
    ];
    const wire = encodePgRowDescription(buf[0..], cols[0..]) catch unreachable;

    // 篡改最后 2 字节 format_code 为非法值 2
    let mutated: [64]u8 = [0; 64];
    @memcpy(mutated[0..wire.len], wire);
    mutated[wire.len - 1] = 2;

    let it = decodePgRowDescription(mutated[0..wire.len]) catch unreachable;
    let got_fmt = false;
    * = it.next() catch |e| {
        if (e == PgRowDescError.UnknownFormatCode) got_fmt = true;
    };
    expect_eq(got_fmt, true);

    // 构造声称有 1 列但字段体被截断的帧
    const short_col: [11]u8 = ['T', 0, 0, 0, 10, 0, 1, 'a', 0, 0, 0];
    let it2 = decodePgRowDescription(short_col[0..]) catch unreachable;
    let got_short = false;
    * = it2.next() catch |e| {
        if (e == PgRowDescError.ShortRead) got_short = true;
    };
    expect_eq(got_short, true);
});
