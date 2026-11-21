import { test, expect_eq } from "cap:test";
import {
    CHARSET_UTF8MB4_GENERAL_CI,
    MySqlWireError,
    FieldType,
    fieldTypeFromByte,
    ColumnFlags,
    lenEncIntSize,
    encodeLenEncInt,
    decodeLenEncInt,
    encodeLenEncString,
    decodeLenEncString,
    ColumnDefinition41,
    encodeColumnDefinition41,
    decodeColumnDefinition41,
} from "cap:mysql";

test("1. FieldType all 31 variants round-trip, classification predicates & invalid byte rejection", () => {
    const valid_bytes: [31]u8 = [
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
        0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    ];
    for (b of valid_bytes) {
        const ft = fieldTypeFromByte(b) catch unreachable;
        expect_eq(ft.toByte(), b);
        expect_eq(ft.name().len > 0, true);
    }

    // Invalid gap bytes rejected
    std.testing.expectError(MySqlWireError.InvalidFieldType, fieldTypeFromByte(0x14)) catch unreachable;
    std.testing.expectError(MySqlWireError.InvalidFieldType, fieldTypeFromByte(0x80)) catch unreachable;
    std.testing.expectError(MySqlWireError.InvalidFieldType, fieldTypeFromByte(0xf4)) catch unreachable;

    // Classification predicates
    expect_eq(FieldType.MYSQL_TYPE_LONGLONG.isBinaryFormatSupported(), true);
    expect_eq(FieldType.MYSQL_TYPE_LONGLONG.isInteger(), true);
    expect_eq(FieldType.MYSQL_TYPE_LONGLONG.isFloat(), false);
    expect_eq(std.mem.eql(u8, FieldType.MYSQL_TYPE_LONGLONG.name(), "LONGLONG"), true);

    expect_eq(FieldType.MYSQL_TYPE_DOUBLE.isBinaryFormatSupported(), true);
    expect_eq(FieldType.MYSQL_TYPE_DOUBLE.isFloat(), true);
    expect_eq(FieldType.MYSQL_TYPE_NEWDECIMAL.isFloat(), true);
    expect_eq(FieldType.MYSQL_TYPE_NEWDECIMAL.isBinaryFormatSupported(), false);

    expect_eq(FieldType.MYSQL_TYPE_VARCHAR.isString(), true);
    expect_eq(FieldType.MYSQL_TYPE_JSON.isString(), true);
    expect_eq(FieldType.MYSQL_TYPE_JSON.isBinaryFormatSupported(), false);

    expect_eq(FieldType.MYSQL_TYPE_DATETIME.isTemporal(), true);
    expect_eq(FieldType.MYSQL_TYPE_DATETIME.isBinaryFormatSupported(), true);
    expect_eq(FieldType.MYSQL_TYPE_DATETIME2.isTemporal(), true);
    expect_eq(FieldType.MYSQL_TYPE_NULL.isInteger(), false);
    expect_eq(FieldType.MYSQL_TYPE_NULL.isString(), false);
});

test("2. ColumnFlags 16-bit bitfield bidirectional toInt/fromInt round-trip", () => {
    const empty = ColumnFlags{};
    expect_eq(empty.toInt(), u16(0));

    const pk_auto = ColumnFlags{
        not_null: true,
        pri_key: true,
        unsigned: true,
        auto_increment: true,
    };
    const packed = pk_auto.toInt();
    // 0x0001 | 0x0002 | 0x0020 | 0x0200 = 0x0223
    expect_eq(packed, u16(0x0223));

    const unpacked = ColumnFlags.fromInt(packed);
    expect_eq(unpacked.not_null, true);
    expect_eq(unpacked.pri_key, true);
    expect_eq(unpacked.unsigned, true);
    expect_eq(unpacked.auto_increment, true);
    expect_eq(unpacked.unique_key, false);
    expect_eq(unpacked.binary, false);

    // All 16 bits set round-trip
    const all_bits = ColumnFlags.fromInt(0xFFFF);
    expect_eq(all_bits.toInt(), u16(0xFFFF));
    expect_eq(all_bits.on_update_now, true);
    expect_eq(all_bits.part_key, true);
});

test("3. MySQL length-encoded integer & string round-trip across 1/3/4/9-byte boundaries", () => {
    const samples: [6]u64 = [0, 250, 251, 65535, 65536, 0x0102030405060708];
    const sizes: [6]usize = [1, 1, 3, 3, 4, 9];
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    let i: usize = 0;
    while (i < 6) {
        const v = samples[i];
        expect_eq(lenEncIntSize(v), sizes[i]);
        const n = encodeLenEncInt(buf[0..], v) catch unreachable;
        expect_eq(n, sizes[i]);
        const decoded = decodeLenEncInt(buf[0..n]) catch unreachable;
        expect_eq(decoded.value, v);
        expect_eq(decoded.bytes_read, sizes[i]);
        i += 1;
    }

    // Length-encoded string round-trip (empty & non-empty)
    let sbuf: [64]u8 = std.mem.zeroes([64]u8);
    const n1 = encodeLenEncString(sbuf[0..], "mysql_column") catch unreachable;
    const d1 = decodeLenEncString(sbuf[0..n1]) catch unreachable;
    expect_eq(std.mem.eql(u8, d1.value, "mysql_column"), true);
    expect_eq(d1.bytes_read, n1);

    const n2 = encodeLenEncString(sbuf[0..], "") catch unreachable;
    expect_eq(n2, usize(1));
    const d2 = decodeLenEncString(sbuf[0..n2]) catch unreachable;
    expect_eq(std.mem.eql(u8, d2.value, ""), true);
});

test("4. Full ColumnDefinition41 packet round-trip with std.testing.allocator (zero leak)", () => {
    const alloc = std.testing.allocator;
    const orig = ColumnDefinition41{
        catalog: "def",
        schema: "ecommerce",
        table: "o",
        org_table: "orders",
        name: "total_amount",
        org_name: "amount",
        character_set: CHARSET_UTF8MB4_GENERAL_CI,
        column_length: 12,
        column_type: FieldType.MYSQL_TYPE_NEWDECIMAL,
        flags: ColumnFlags{ not_null: true, unsigned: true },
        decimals: 2,
    };

    let wire = encodeColumnDefinition41(alloc, orig) catch unreachable;
    expect_eq(wire.len(), orig.encodedSize());

    const decoded = decodeColumnDefinition41(wire.slice()) catch unreachable;
    expect_eq(decoded.bytes_read, wire.len());
    expect_eq(std.mem.eql(u8, decoded.column.catalog, "def"), true);
    expect_eq(std.mem.eql(u8, decoded.column.schema, "ecommerce"), true);
    expect_eq(std.mem.eql(u8, decoded.column.table, "o"), true);
    expect_eq(std.mem.eql(u8, decoded.column.org_table, "orders"), true);
    expect_eq(std.mem.eql(u8, decoded.column.name, "total_amount"), true);
    expect_eq(std.mem.eql(u8, decoded.column.org_name, "amount"), true);
    expect_eq(decoded.column.character_set, CHARSET_UTF8MB4_GENERAL_CI);
    expect_eq(decoded.column.column_length, u32(12));
    expect_eq(decoded.column.column_type == FieldType.MYSQL_TYPE_NEWDECIMAL, true);
    expect_eq(decoded.column.flags.not_null, true);
    expect_eq(decoded.column.flags.unsigned, true);
    expect_eq(decoded.column.decimals, u8(2));
});

test("5. Truncated & malformed packet guards for len-enc & ColumnDefinition41", () => {
    // Empty buffer
    const empty: [0]u8 = [];
    std.testing.expectError(MySqlWireError.UnexpectedEof, decodeLenEncInt(empty[0..])) catch unreachable;
    std.testing.expectError(MySqlWireError.UnexpectedEof, decodeColumnDefinition41(empty[0..])) catch unreachable;

    // Invalid len-enc prefixes (0xfb NULL marker, 0xff error marker)
    const bad_fb: [1]u8 = [0xfb];
    const bad_ff: [1]u8 = [0xff];
    std.testing.expectError(MySqlWireError.InvalidLengthEncodedInt, decodeLenEncInt(bad_fb[0..])) catch unreachable;
    std.testing.expectError(MySqlWireError.InvalidLengthEncodedInt, decodeLenEncInt(bad_ff[0..])) catch unreachable;

    // Truncated 3-byte len-enc int
    const trunc_fc: [2]u8 = [0xfc, 0x01];
    std.testing.expectError(MySqlWireError.UnexpectedEof, decodeLenEncInt(trunc_fc[0..])) catch unreachable;

    // ColumnDefinition41 with wrong fixed_length_fields_length (0x0b instead of 0x0c)
    const bad_fixed: [19]u8 = [
        0, 0, 0, 0, 0, 0, // 6 empty strings
        0x0b,             // invalid fixed length (11 != 12)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    std.testing.expectError(MySqlWireError.InvalidFixedFieldsLength, decodeColumnDefinition41(bad_fixed[0..])) catch unreachable;

    // ColumnDefinition41 with invalid field type (0x55)
    const bad_type: [19]u8 = [
        0, 0, 0, 0, 0, 0, // 6 empty strings
        0x0c,             // valid fixed length = 12
        45, 0,            // charset
        10, 0, 0, 0,      // column_length
        0x55,             // invalid FieldType
        0, 0,             // flags
        0, 0, 0,          // decimals + filler
    ];
    std.testing.expectError(MySqlWireError.InvalidFieldType, decodeColumnDefinition41(bad_type[0..])) catch unreachable;

    // Truncated ColumnDefinition41 (missing filler bytes)
    std.testing.expectError(MySqlWireError.UnexpectedEof, decodeColumnDefinition41(bad_type[0..15])) catch unreachable;
});
