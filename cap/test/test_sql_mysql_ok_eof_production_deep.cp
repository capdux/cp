import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MYSQL_OK_HEADER,
    MYSQL_EOF_HEADER,
    MYSQL_FLAG_IN_TRANS,
    MYSQL_FLAG_AUTOCOMMIT,
    MYSQL_FLAG_MORE_RESULTS_EXISTS,
    MYSQL_FLAG_QUERY_WAS_SLOW,
    MYSQL_FLAG_SESSION_STATE_CHANGED,
    MySqlOkEofError,
    MySqlServerStatusFlags,
    MySqlOkPacketView,
    MySqlEofPacketView,
    encodeMySqlOkPacket,
    decodeMySqlOkPacket,
    encodeMySqlEofPacket,
    decodeMySqlEofPacket,
} from "cap:sql";

test("1. MySqlServerStatusFlags bitmask helpers and composition", () => {
    const f = MySqlServerStatusFlags.fromInt(MYSQL_FLAG_IN_TRANS)
        .withFlag(MYSQL_FLAG_AUTOCOMMIT)
        .withFlag(MYSQL_FLAG_MORE_RESULTS_EXISTS)
        .withFlag(MYSQL_FLAG_QUERY_WAS_SLOW)
        .withFlag(MYSQL_FLAG_SESSION_STATE_CHANGED);

    expect_eq(f.inTransaction(), true);
    expect_eq(f.isAutoCommit(), true);
    expect_eq(f.hasMoreResults(), true);
    expect_eq(f.wasSlowQuery(), true);
    expect_eq(f.hasSessionStateChanged(), true);
    expect_eq(f.toInt(), u16(1 | 2 | 8 | 2048 | 16384));
});

test("2. OK_Packet (0x00) encode and zero-copy decode with large lenenc ints and info string", () => {
    let buf: [128]u8 = [0; 128];
    const flags = MySqlServerStatusFlags.fromInt(MYSQL_FLAG_IN_TRANS | MYSQL_FLAG_AUTOCOMMIT);
    const wire = encodeMySqlOkPacket(buf[0..], MySqlOkPacketView{
        sequence_id: 2,
        header: MYSQL_OK_HEADER,
        affected_rows: 100000,
        last_insert_id: 9876543210,
        status_flags: flags,
        warnings: 4,
        info: "Rows matched: 100000  Changed: 100000",
    }) catch unreachable;

    const decoded = decodeMySqlOkPacket(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(2));
    expect_eq(decoded.header, MYSQL_OK_HEADER);
    expect_eq(decoded.affected_rows, u64(100000));
    expect_eq(decoded.last_insert_id, u64(9876543210));
    expect_eq(decoded.warnings, u16(4));
    expect_eq(decoded.status_flags.inTransaction(), true);
    expect_eq(decoded.status_flags.isAutoCommit(), true);
    expect_eq(std.mem.eql(u8, decoded.info, "Rows matched: 100000  Changed: 100000"), true);
});

test("3. OK_Packet with 0xFE header (CLIENT_DEPRECATE_EOF) roundtrip", () => {
    let buf: [64]u8 = [0; 64];
    const wire = encodeMySqlOkPacket(buf[0..], MySqlOkPacketView{
        sequence_id: 5,
        header: MYSQL_EOF_HEADER,
        affected_rows: 0,
        last_insert_id: 0,
        status_flags: MySqlServerStatusFlags.fromInt(MYSQL_FLAG_AUTOCOMMIT),
        warnings: 1,
        info: "",
    }) catch unreachable;

    const decoded = decodeMySqlOkPacket(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(5));
    expect_eq(decoded.header, MYSQL_EOF_HEADER);
    expect_eq(decoded.warnings, u16(1));
    expect_eq(decoded.status_flags.isAutoCommit(), true);
    expect_eq(decoded.info.len, usize(0));
});

test("4. EOF_Packet (0xFE) fixed 9-byte wire frame roundtrip", () => {
    let buf: [16]u8 = [0; 16];
    const flags = MySqlServerStatusFlags.fromInt(MYSQL_FLAG_AUTOCOMMIT | MYSQL_FLAG_MORE_RESULTS_EXISTS);
    const wire = encodeMySqlEofPacket(buf[0..], MySqlEofPacketView{
        sequence_id: 7,
        warnings: 3,
        status_flags: flags,
    }) catch unreachable;

    expect_eq(wire.len, usize(9));
    expect_eq(wire[3], u8(7));
    expect_eq(wire[4], MYSQL_EOF_HEADER);

    const decoded = decodeMySqlEofPacket(wire) catch unreachable;
    expect_eq(decoded.sequence_id, u8(7));
    expect_eq(decoded.header, MYSQL_EOF_HEADER);
    expect_eq(decoded.warnings, u16(3));
    expect_eq(decoded.status_flags.isAutoCommit(), true);
    expect_eq(decoded.status_flags.hasMoreResults(), true);
});

test("5. Encoder boundary guards: InvalidOkHeader and BufferTooSmall", () => {
    let buf: [64]u8 = [0; 64];
    let bad_hdr = false;
    * = encodeMySqlOkPacket(buf[0..], MySqlOkPacketView{ header: 0xFF }) catch |e| {
        if (e == MySqlOkEofError.InvalidOkHeader) bad_hdr = true;
    };
    expect_eq(bad_hdr, true);

    let tiny: [5]u8 = [0; 5];
    let small_eof = false;
    * = encodeMySqlEofPacket(tiny[0..], MySqlEofPacketView{}) catch |e| {
        if (e == MySqlOkEofError.BufferTooSmall) small_eof = true;
    };
    expect_eq(small_eof, true);
});

test("6. Decoder malformed packet guards: wrong header, truncated payload, and EOF length >= 9", () => {
    // 1. OK_Packet 错误头字节 0xFF (ERR_Packet)
    const err_pkt: [11]u8 = [7, 0, 0, 1, 0xFF, 0, 0, 0, 0, 0, 0];
    let e1 = false;
    * = decodeMySqlOkPacket(err_pkt[0..]) catch |e| {
        if (e == MySqlOkEofError.InvalidOkHeader) e1 = true;
    };
    expect_eq(e1, true);

    // 2. OK_Packet 声明长度大于实际载荷
    const trunc_ok: [11]u8 = [20, 0, 0, 1, 0x00, 0, 0, 0, 0, 0, 0];
    let e2 = false;
    * = decodeMySqlOkPacket(trunc_ok[0..]) catch |e| {
        if (e == MySqlOkEofError.TruncatedPayload) e2 = true;
    };
    expect_eq(e2, true);

    // 3. EOF_Packet 载荷长度 >= 9（属于长数据包而非 4.1 EOF_Packet）
    const long_eof: [13]u8 = [9, 0, 0, 1, 0xFE, 0, 0, 0, 0, 0, 0, 0, 0];
    let e3 = false;
    * = decodeMySqlEofPacket(long_eof[0..]) catch |e| {
        if (e == MySqlOkEofError.InvalidEofLength) e3 = true;
    };
    expect_eq(e3, true);

    // 4. EOF_Packet 错误头字节 0x00
    const bad_eof: [9]u8 = [5, 0, 0, 1, 0x00, 0, 0, 0, 0];
    let e4 = false;
    * = decodeMySqlEofPacket(bad_eof[0..]) catch |e| {
        if (e == MySqlOkEofError.InvalidEofHeader) e4 = true;
    };
    expect_eq(e4, true);
});
