import { test, expect_eq } from "cap:test";
import {
    PROTOCOL_VERSION_3_0,
    PgWireError,
    FormatCode,
    formatCodeFromInt,
    MessageTag,
    messageTagFromByte,
    MessageHeader,
    encodeMessageHeader,
    decodeMessageHeader,
    FieldDescription,
    encodeFieldDescription,
    decodeFieldDescription,
    StartupParam,
    StartupMessage,
    encodeStartupMessage,
    decodeStartupMessage,
} from "cap:pg";

test("1. FormatCode & MessageTag round-trip and invalid byte protection", () => {
    const text_fmt = formatCodeFromInt(0) catch unreachable;
    const bin_fmt = formatCodeFromInt(1) catch unreachable;
    expect_eq(text_fmt.isText(), true);
    expect_eq(text_fmt.isBinary(), false);
    expect_eq(text_fmt.toInt(), i16(0));
    expect_eq(bin_fmt.isBinary(), true);
    expect_eq(bin_fmt.toInt(), i16(1));

    std.testing.expectError(PgWireError.InvalidFormatCode, formatCodeFromInt(2)) catch unreachable;
    std.testing.expectError(PgWireError.InvalidFormatCode, formatCodeFromInt(-1)) catch unreachable;

    const row_desc = messageTagFromByte('T') catch unreachable;
    expect_eq(row_desc == MessageTag.RowDescription, true);
    expect_eq(row_desc.toByte(), u8('T'));
    expect_eq(row_desc.isBackend(), true);
    expect_eq(row_desc.isFrontend(), false);

    const query_tag = messageTagFromByte('Q') catch unreachable;
    expect_eq(query_tag == MessageTag.Query, true);
    expect_eq(query_tag.isFrontend(), true);
    expect_eq(query_tag.isBackend(), false);

    std.testing.expectError(PgWireError.InvalidMessageTag, messageTagFromByte(0x01)) catch unreachable;
});

test("2. FieldDescription big-endian round-trip (text & binary columns, zero leak)", () => {
    const alloc = std.testing.allocator;
    const orig = FieldDescription{
        name: "user_id",
        table_oid: 16384,
        column_index: 1,
        type_oid: 23,
        type_size: 4,
        type_modifier: -1,
        format: FormatCode.binary,
    };

    let wire = encodeFieldDescription(alloc, orig) catch unreachable;
    // "user_id" (7) + NUL (1) + 18 metadata bytes = 26 bytes
    expect_eq(wire.len(), usize(26));

    const decoded = decodeFieldDescription(wire.slice()) catch unreachable;
    expect_eq(decoded.bytes_read, usize(26));
    expect_eq(std.mem.eql(u8, decoded.field.name, "user_id"), true);
    expect_eq(decoded.field.table_oid, i32(16384));
    expect_eq(decoded.field.column_index, i16(1));
    expect_eq(decoded.field.type_oid, i32(23));
    expect_eq(decoded.field.type_size, i16(4));
    expect_eq(decoded.field.type_modifier, i32(-1));
    expect_eq(decoded.field.isBinary(), true);

    // Text column with negative type_size & type_modifier
    const text_col = FieldDescription{
        name: "payload",
        table_oid: 0,
        column_index: 0,
        type_oid: 25,
        type_size: -1,
        type_modifier: -1,
        format: FormatCode.text,
    };
    let wire2 = encodeFieldDescription(alloc, text_col) catch unreachable;
    const decoded2 = decodeFieldDescription(wire2.slice()) catch unreachable;
    expect_eq(std.mem.eql(u8, decoded2.field.name, "payload"), true);
    expect_eq(decoded2.field.type_oid, i32(25));
    expect_eq(decoded2.field.type_size, i16(-1));
    expect_eq(decoded2.field.isBinary(), false);
});

test("3. StartupMessage v3.0 big-endian encode & decode round-trip with defaults and extra params", () => {
    const alloc = std.testing.allocator;

    // Default database falls back to user, client_encoding defaults to UTF8
    let buf1 = encodeStartupMessage(alloc, StartupMessage{
        user: "admin",
    }) catch unreachable;
    let dec1 = decodeStartupMessage(alloc, buf1.slice()) catch unreachable;
    expect_eq(dec1.protocol_version, PROTOCOL_VERSION_3_0);
    expect_eq(dec1.total_length, buf1.len() as u32);
    expect_eq(dec1.paramCount(), usize(3));
    expect_eq(std.mem.eql(u8, dec1.getParam("user") ?? "", "admin"), true);
    expect_eq(std.mem.eql(u8, dec1.getParam("database") ?? "", "admin"), true);
    expect_eq(std.mem.eql(u8, dec1.getParam("client_encoding") ?? "", "UTF8"), true);
    expect_eq(dec1.getParam("nonexistent") == null, true);

    // Explicit database + application_name + extra_params
    const extras: [2]StartupParam = [
        StartupParam{ key: "TimeZone", value: "UTC" },
        StartupParam{ key: "search_path", value: "public" },
    ];
    let buf2 = encodeStartupMessage(alloc, StartupMessage{
        user: "postgres",
        database: "analytics",
        client_encoding: "UTF8",
        application_name: "cp-sql",
        extra_params: extras[0..],
    }) catch unreachable;
    let dec2 = decodeStartupMessage(alloc, buf2.slice()) catch unreachable;
    expect_eq(dec2.paramCount(), usize(6));
    expect_eq(std.mem.eql(u8, dec2.getParam("database") ?? "", "analytics"), true);
    expect_eq(std.mem.eql(u8, dec2.getParam("application_name") ?? "", "cp-sql"), true);
    expect_eq(std.mem.eql(u8, dec2.getParam("TimeZone") ?? "", "UTC"), true);
    expect_eq(std.mem.eql(u8, dec2.getParam("search_path") ?? "", "public"), true);
});

test("4. MessageHeader 5-byte frame encode/decode & malformed length guard", () => {
    let hdr_buf: [5]u8 = [0, 0, 0, 0, 0];
    const written = encodeMessageHeader(hdr_buf[0..], MessageTag.ReadyForQuery, 1) catch unreachable;
    expect_eq(written, usize(5));
    expect_eq(hdr_buf[0], u8('Z'));
    expect_eq(hdr_buf[4], u8(5));

    const hdr = decodeMessageHeader(hdr_buf[0..]) catch unreachable;
    expect_eq(hdr.tag == MessageTag.ReadyForQuery, true);
    expect_eq(hdr.body_len, u32(1));

    // Truncated header (< 5 bytes)
    std.testing.expectError(PgWireError.UnexpectedEof, decodeMessageHeader(hdr_buf[0..3])) catch unreachable;

    // Malformed length < 4
    const bad_len: [5]u8 = ['Z', 0, 0, 0, 3];
    std.testing.expectError(PgWireError.InvalidMessageLength, decodeMessageHeader(bad_len[0..])) catch unreachable;
});

test("5. Truncated & malformed packet guards for FieldDescription and StartupMessage", () => {
    const alloc = std.testing.allocator;

    // Empty user rejected on encode
    std.testing.expectError(
        PgWireError.EmptyUser,
        encodeStartupMessage(alloc, StartupMessage{ user: "" }),
    ) catch unreachable;

    // FieldDescription missing NUL terminator
    const no_nul: [5]u8 = ['a', 'b', 'c', 'd', 'e'];
    std.testing.expectError(
        PgWireError.InvalidNullTerminator,
        decodeFieldDescription(no_nul[0..]),
    ) catch unreachable;

    // FieldDescription truncated metadata (< 18 bytes after NUL)
    const short_meta: [10]u8 = ['x', 0, 0, 0, 0, 1, 0, 1, 0, 0];
    std.testing.expectError(
        PgWireError.UnexpectedEof,
        decodeFieldDescription(short_meta[0..]),
    ) catch unreachable;

    // FieldDescription invalid format code (0x0002)
    let bad_fmt_pkt: [20]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    bad_fmt_pkt[0] = 'c';
    bad_fmt_pkt[1] = 0;
    bad_fmt_pkt[19] = 2;
    std.testing.expectError(
        PgWireError.InvalidFormatCode,
        decodeFieldDescription(bad_fmt_pkt[0..]),
    ) catch unreachable;

    // StartupMessage truncated (< 9 bytes)
    const short_startup: [6]u8 = [0, 0, 0, 9, 0, 3];
    std.testing.expectError(
        PgWireError.UnexpectedEof,
        decodeStartupMessage(alloc, short_startup[0..]),
    ) catch unreachable;

    // StartupMessage invalid protocol version (v2.0 = 131072)
    const bad_ver: [9]u8 = [0, 0, 0, 9, 0, 2, 0, 0, 0];
    std.testing.expectError(
        PgWireError.InvalidProtocolVersion,
        decodeStartupMessage(alloc, bad_ver[0..]),
    ) catch unreachable;

    // StartupMessage declared length exceeds buffer
    const over_len: [9]u8 = [0, 0, 0, 64, 0, 3, 0, 0, 0];
    std.testing.expectError(
        PgWireError.UnexpectedEof,
        decodeStartupMessage(alloc, over_len[0..]),
    ) catch unreachable;
});
