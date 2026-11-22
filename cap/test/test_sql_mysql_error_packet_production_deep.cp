// Production Deep Tests for cap:sql MySQL ERR_Packet Codec (lib/sql/mysql_error_packet.cp)
import { test, expect_eq } from "cap:test";
import {
    MYSQL_ERR_PACKET_HEADER,
    MYSQL_SQL_STATE_MARKER,
    MySqlErrorPacketError,
    MySqlErrorPacket,
} from "cap:sql";

test("sql mysql_error_packet: 4.1+ ERR_Packet with #SQLSTATE roundtrip", fn() {
    const pkt = MySqlErrorPacket.initWithSqlState(1064, "42000", "You have an error in your SQL syntax") catch unreachable;
    expect_eq(pkt.encodedSize(), @as(usize, 3 + 6 + 36));

    let wire: [64]u8 = [0; 64];
    const n = pkt.encode(wire[0..]) catch unreachable;
    expect_eq(n, @as(usize, 45));
    expect_eq(wire[0], MYSQL_ERR_PACKET_HEADER);
    // 1064 = 0x0428 little-endian -> 0x28, 0x04
    expect_eq(wire[1], @as(u8, 0x28));
    expect_eq(wire[2], @as(u8, 0x04));
    expect_eq(wire[3], MYSQL_SQL_STATE_MARKER);

    const decoded = MySqlErrorPacket.decode(wire[0..n]) catch unreachable;
    expect_eq(decoded.error_code, @as(u16, 1064));
    expect_eq(decoded.has_sql_state, true);
    expect_eq(decoded.sqlStateSlice() ?? "", "42000");
    expect_eq(decoded.sqlStateClass() ?? "", "42");
    expect_eq(decoded.error_message, "You have an error in your SQL syntax");
});

test("sql mysql_error_packet: legacy 4.0 ERR_Packet without #SQLSTATE", fn() {
    const pkt = MySqlErrorPacket.initLegacy(1045, "Access denied for user");
    expect_eq(pkt.has_sql_state, false);
    expect_eq(pkt.sqlStateSlice() == null, true);
    expect_eq(pkt.sqlStateClass() == null, true);

    let wire: [64]u8 = [0; 64];
    const n = pkt.encode(wire[0..]) catch unreachable;
    expect_eq(n, @as(usize, 3 + 22));

    const decoded = MySqlErrorPacket.decode(wire[0..n]) catch unreachable;
    expect_eq(decoded.error_code, @as(u16, 1045));
    expect_eq(decoded.has_sql_state, false);
    expect_eq(decoded.error_message, "Access denied for user");
});

test("sql mysql_error_packet: minimal empty-message packets (3-byte and 9-byte)", fn() {
    const min_legacy: [3]u8 = [0xff, 0x01, 0x00];
    const d1 = MySqlErrorPacket.decode(min_legacy[0..]) catch unreachable;
    expect_eq(d1.error_code, @as(u16, 1));
    expect_eq(d1.has_sql_state, false);
    expect_eq(d1.error_message, "");

    const min_41: [9]u8 = [0xff, 0x02, 0x00, '#', 'H', 'Y', '0', '0', '0'];
    const d2 = MySqlErrorPacket.decode(min_41[0..]) catch unreachable;
    expect_eq(d2.error_code, @as(u16, 2));
    expect_eq(d2.has_sql_state, true);
    expect_eq(d2.sqlStateSlice() ?? "", "HY000");
    expect_eq(d2.error_message, "");
});

test("sql mysql_error_packet: retryable deadlock and lock-timeout classification", fn() {
    const deadlock = MySqlErrorPacket.initWithSqlState(1213, "40001", "Deadlock found") catch unreachable;
    const lock_timeout = MySqlErrorPacket.initWithSqlState(1205, "HY000", "Lock wait timeout") catch unreachable;
    const too_many = MySqlErrorPacket.initLegacy(1040, "Too many connections");
    const syntax_err = MySqlErrorPacket.initWithSqlState(1064, "42000", "Syntax error") catch unreachable;

    expect_eq(deadlock.isRetryable(), true);
    expect_eq(lock_timeout.isRetryable(), true);
    expect_eq(too_many.isRetryable(), true);
    expect_eq(syntax_err.isRetryable(), false);
});

test("sql mysql_error_packet: malformed header and truncated SQLSTATE guards", fn() {
    // Wrong header (0x00 OK packet instead of 0xff ERR packet) -> InvalidHeader
    const bad_hdr: [4]u8 = [0x00, 0x01, 0x00, 'x'];
    let hdr_err = false;
    * = MySqlErrorPacket.decode(bad_hdr[0..]) catch |e| {
        hdr_err = (e == MySqlErrorPacketError.InvalidHeader);
        MySqlErrorPacket{}
    };
    expect_eq(hdr_err, true);

    // Payload shorter than 3 bytes -> TruncatedPacket
    const too_short: [2]u8 = [0xff, 0x01];
    let short_err = false;
    * = MySqlErrorPacket.decode(too_short[0..]) catch |e| {
        short_err = (e == MySqlErrorPacketError.TruncatedPacket);
        MySqlErrorPacket{}
    };
    expect_eq(short_err, true);

    // '#' marker present but only 3 SQLSTATE bytes follow -> TruncatedPacket
    const trunc_state: [7]u8 = [0xff, 0x28, 0x04, '#', '4', '2', '0'];
    let state_trunc = false;
    * = MySqlErrorPacket.decode(trunc_state[0..]) catch |e| {
        state_trunc = (e == MySqlErrorPacketError.TruncatedPacket);
        MySqlErrorPacket{}
    };
    expect_eq(state_trunc, true);
});

test("sql mysql_error_packet: invalid SQLSTATE length and BufferTooSmall guards", fn() {
    let bad_state = false;
    * = MySqlErrorPacket.initWithSqlState(1064, "420", "bad") catch |e| {
        bad_state = (e == MySqlErrorPacketError.InvalidSqlState);
        MySqlErrorPacket{}
    };
    expect_eq(bad_state, true);

    const pkt = MySqlErrorPacket.initWithSqlState(1064, "42000", "hello") catch unreachable;
    let tiny: [8]u8 = [0; 8];
    let buf_err = false;
    * = pkt.encode(tiny[0..]) catch |e| {
        buf_err = (e == MySqlErrorPacketError.BufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(buf_err, true);
});
