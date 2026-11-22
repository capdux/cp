import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MYSQL_COM_QUERY_CMD,
    MYSQL_MAX_QUERY_PARAMS,
    MySqlQueryCodecError,
    MySqlQueryParamKind,
    MySqlQueryParam,
    MySqlDecodedQuery,
    encodeMySqlSimpleQuery,
    encodeMySqlQueryWithAttrs,
    decodeMySqlSimpleQuery,
    decodeMySqlQueryWithAttrs,
} from "cap:sql";

test("1. Simple COM_QUERY encode and zero-copy decode roundtrip", () => {
    let buf: [64]u8 = [0; 64];
    const sql = "SELECT id, name FROM users WHERE active = 1";
    const wire = encodeMySqlSimpleQuery(buf[0..], 3, sql) catch unreachable;

    expect_eq(wire.len, usize(4 + 1 + sql.len));
    expect_eq(wire[3], u8(3)); // sequence_id
    expect_eq(wire[4], MYSQL_COM_QUERY_CMD);

    const decoded = decodeMySqlSimpleQuery(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(3));
    expect_eq(decoded.payload_len, usize(1 + sql.len));
    expect_eq(decoded.param_count, usize(0));
    expect_eq(std.mem.eql(u8, decoded.query, sql), true);
});

test("2. MySQL 8.0.23+ Query Attributes zero-parameter mode roundtrip", () => {
    let buf: [64]u8 = [0; 64];
    const no_params: [0]MySqlQueryParam = [];
    const sql = "SELECT VERSION()";
    const wire = encodeMySqlQueryWithAttrs(buf[0..], 0, sql, no_params[0..]) catch unreachable;

    // 4B header + 1B cmd + 1B param_count(0) + 1B param_set_count(1) + sql.len
    expect_eq(wire.len, usize(4 + 3 + sql.len));
    expect_eq(wire[5], u8(0));
    expect_eq(wire[6], u8(1));

    const decoded = decodeMySqlQueryWithAttrs(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(0));
    expect_eq(decoded.param_count, usize(0));
    expect_eq(std.mem.eql(u8, decoded.query, sql), true);
});

test("3. Query Attributes mixed types (int32/int64/uint64/float64/string/blob/null) and auto :pN naming", () => {
    let buf: [512]u8 = [0; 512];
    const params: [7]MySqlQueryParam = [
        MySqlQueryParam.int32Param("tenant", -7),
        MySqlQueryParam.int64Param("", 9007199254740991),
        MySqlQueryParam.uint64Param("u_id", 18446744073709551615),
        MySqlQueryParam.float64Param("score", 98.5),
        MySqlQueryParam.stringParam("", "alice"),
        MySqlQueryParam.blobParam("raw", "\x00\xff\x10"),
        MySqlQueryParam.nullParam("deleted_at"),
    ];

    expect_eq(MySqlQueryParamKind.int64.isBinaryFixed(), true);
    expect_eq(MySqlQueryParamKind.string.isBinaryFixed(), false);

    const sql = "INSERT INTO audit VALUES (?, ?, ?, ?, ?, ?, ?)";
    const wire = encodeMySqlQueryWithAttrs(buf[0..], 1, sql, params[0..]) catch unreachable;

    // 验证自动命名 :p2 和 :p5 以及 unsigned flag 0x80 均已编码进报文
    expect_eq(std.mem.indexOf(u8, wire, ":p2") != null, true);
    expect_eq(std.mem.indexOf(u8, wire, ":p5") != null, true);
    expect_eq(std.mem.indexOf(u8, wire, "tenant") != null, true);
    expect_eq(std.mem.indexOf(u8, wire, "alice") != null, true);

    const decoded = decodeMySqlQueryWithAttrs(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(1));
    expect_eq(decoded.param_count, usize(7));
    expect_eq(std.mem.eql(u8, decoded.query, sql), true);
});

test("4. Query Attributes multi-byte NULL bitmap (9 parameters spanning 2 bitmap bytes)", () => {
    let buf: [512]u8 = [0; 512];
    let params: [9]MySqlQueryParam = [MySqlQueryParam.int32Param("", 1); 9];
    params[0] = MySqlQueryParam.nullParam("n0");
    params[8] = MySqlQueryParam.nullParam("n8");

    const sql = "SELECT 9";
    const wire = encodeMySqlQueryWithAttrs(buf[0..], 2, sql, params[0..]) catch unreachable;

    // 4B hdr + 1B cmd + 1B count(9) + 1B set_count(1) => bitmap starts at index 7 (2 bytes)
    expect_eq(wire[7], u8(0x01)); // bit 0 set
    expect_eq(wire[8], u8(0x01)); // bit 8 set (bit 0 of byte 1)

    const decoded = decodeMySqlQueryWithAttrs(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(2));
    expect_eq(decoded.param_count, usize(9));
    expect_eq(std.mem.eql(u8, decoded.query, sql), true);
});

test("5. Encoder boundary protection: EmptyQuery, BufferTooSmall, and TooManyParameters", () => {
    let buf: [64]u8 = [0; 64];
    let got_empty = false;
    * = encodeMySqlSimpleQuery(buf[0..], 0, "") catch |e| {
        if (e == MySqlQueryCodecError.EmptyQuery) got_empty = true;
    };
    expect_eq(got_empty, true);

    let tiny: [6]u8 = [0; 6];
    let got_small = false;
    * = encodeMySqlSimpleQuery(tiny[0..], 0, "SELECT 12345") catch |e| {
        if (e == MySqlQueryCodecError.BufferTooSmall) got_small = true;
    };
    expect_eq(got_small, true);

    expect_eq(MYSQL_MAX_QUERY_PARAMS, usize(64));
    let too_many: [65]MySqlQueryParam = [MySqlQueryParam.nullParam(""); 65];
    let big: [1024]u8 = [0; 1024];
    let got_many = false;
    * = encodeMySqlQueryWithAttrs(big[0..], 0, "SELECT 1", too_many[0..]) catch |e| {
        if (e == MySqlQueryCodecError.TooManyParameters) got_many = true;
    };
    expect_eq(got_many, true);
});

test("6. Decoder malformed packet protection: truncated header/payload, wrong command, bad bind flag", () => {
    // 1. 截断包头
    const short_hdr: [3]u8 = [1, 0, 0];
    let e1 = false;
    * = decodeMySqlSimpleQuery(short_hdr[0..]) catch |e| {
        if (e == MySqlQueryCodecError.InvalidPacket) e1 = true;
    };
    expect_eq(e1, true);

    // 2. 声明长度大于实际载荷
    const trunc_pkt: [6]u8 = [10, 0, 0, 0, MYSQL_COM_QUERY_CMD, 'S'];
    let e2 = false;
    * = decodeMySqlSimpleQuery(trunc_pkt[0..]) catch |e| {
        if (e == MySqlQueryCodecError.TruncatedPayload) e2 = true;
    };
    expect_eq(e2, true);

    // 3. 非 COM_QUERY 命令字节 (0x01 COM_QUIT)
    const wrong_cmd: [6]u8 = [2, 0, 0, 0, 0x01, 'X'];
    let e3 = false;
    * = decodeMySqlSimpleQuery(wrong_cmd[0..]) catch |e| {
        if (e == MySqlQueryCodecError.InvalidCommand) e3 = true;
    };
    expect_eq(e3, true);

    // 4. Query Attributes 非法 bind_flag=0
    const bad_bind: [9]u8 = [5, 0, 0, 0, MYSQL_COM_QUERY_CMD, 1, 1, 0x00, 0x00];
    let e4 = false;
    * = decodeMySqlQueryWithAttrs(bad_bind[0..]) catch |e| {
        if (e == MySqlQueryCodecError.ParameterMismatch) e4 = true;
    };
    expect_eq(e4, true);
});
