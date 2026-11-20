// Source: cp/cap/test/test_mysql_wire_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MysqlStreamFramer,
    encodePacketFrame,
    readLenEncInt,
    writeLenEncInt,
    writeLenEncNull,
    readLenEncStr,
    writeLenEncStr,
    HandshakeV10,
    HandshakeResponse41,
    computeNativePasswordAuth,
    DEFAULT_CLIENT_CAPABILITIES,
    OkPacket,
    ErrPacket,
    EofPacket,
    ComPacket,
    COM_QUERY,
    COM_PING,
    COM_QUIT,
    ColumnDef41,
    CellValue,
    TextResultRow,
} from "cap:mysql_wire";

test("mysql_wire length-encoded int and string round-trip + truncation") {
    const alloc = std.testing.allocator;
    let buf = std.ArrayList(u8).empty;
    defer buf.deinit(alloc);

    try writeLenEncInt(alloc, &buf, 42);
    try writeLenEncInt(alloc, &buf, 300);
    try writeLenEncInt(alloc, &buf, 70000);
    try writeLenEncInt(alloc, &buf, 0x123456789ABCDEF0);
    try writeLenEncNull(alloc, &buf);
    try writeLenEncStr(alloc, &buf, "hello_mysql");

    let offset: usize = 0;
    const v1 = try readLenEncInt(buf.items, &offset);
    expect_eq(v1.value, @as(u64, 42));
    expect_eq(v1.is_null, false);

    const v2 = try readLenEncInt(buf.items, &offset);
    expect_eq(v2.value, @as(u64, 300));

    const v3 = try readLenEncInt(buf.items, &offset);
    expect_eq(v3.value, @as(u64, 70000));

    const v4 = try readLenEncInt(buf.items, &offset);
    expect_eq(v4.value, @as(u64, 0x123456789ABCDEF0));

    const vnull = try readLenEncStr(buf.items, &offset);
    expect_eq(vnull == null, true);

    const s1 = try readLenEncStr(buf.items, &offset);
    expect_eq(std.mem.eql(u8, s1.?, "hello_mysql"), true);
    expect_eq(offset, buf.items.len);

    // Truncated 0xFC integer should return error.UnexpectedEof
    const trunc: [2]u8 = [0xFC, 0x01];
    let trunc_off: usize = 0;
    let got_err = false;
    * = readLenEncInt(&trunc, &trunc_off) catch {
        got_err = true;
    };
    expect_eq(got_err, true);
}

test("mysql_wire HandshakeV10, HandshakeResponse41 & native password auth") {
    const alloc = std.testing.allocator;
    const scramble: [20]u8 = [
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    ];

    const hs = HandshakeV10{
        .protocol_version = 10,
        .server_version = try alloc.dupe(u8, "8.0.36-cp"),
        .connection_id = 1024,
        .auth_plugin_data = scramble,
        .capability_flags = DEFAULT_CLIENT_CAPABILITIES,
        .character_set = 45,
        .status_flags = 0x0002,
        .auth_plugin_name = try alloc.dupe(u8, "mysql_native_password"),
    };
    let hs_mut = hs;
    defer hs_mut.deinit(alloc);

    const hs_raw = try hs_mut.encode(alloc);
    defer alloc.free(hs_raw);

    let hs_dec = try HandshakeV10.decode(alloc, hs_raw);
    defer hs_dec.deinit(alloc);

    expect_eq(hs_dec.protocol_version, @as(u8, 10));
    expect_eq(hs_dec.connection_id, @as(u32, 1024));
    expect_eq(std.mem.eql(u8, hs_dec.server_version, "8.0.36-cp"), true);
    expect_eq(std.mem.eql(u8, &hs_dec.auth_plugin_data, &scramble), true);
    expect_eq(std.mem.eql(u8, hs_dec.auth_plugin_name, "mysql_native_password"), true);

    const empty_auth = try computeNativePasswordAuth(alloc, "", scramble);
    defer alloc.free(empty_auth);
    expect_eq(empty_auth.len, @as(usize, 0));

    const auth_token = try computeNativePasswordAuth(alloc, "s3cr3t!", scramble);
    defer alloc.free(auth_token);
    expect_eq(auth_token.len, @as(usize, 20));

    let resp = HandshakeResponse41{
        .capability_flags = DEFAULT_CLIENT_CAPABILITIES,
        .max_packet_size = 16777216,
        .character_set = 45,
        .username = try alloc.dupe(u8, "root"),
        .auth_response = try alloc.dupe(u8, auth_token),
        .database = try alloc.dupe(u8, "app_db"),
        .auth_plugin_name = try alloc.dupe(u8, "mysql_native_password"),
    };
    defer resp.deinit(alloc);

    const resp_raw = try resp.encode(alloc);
    defer alloc.free(resp_raw);

    let resp_dec = try HandshakeResponse41.decode(alloc, resp_raw);
    defer resp_dec.deinit(alloc);

    expect_eq(std.mem.eql(u8, resp_dec.username, "root"), true);
    expect_eq(std.mem.eql(u8, resp_dec.auth_response, auth_token), true);
    expect_eq(std.mem.eql(u8, resp_dec.database.?, "app_db"), true);
}

test("mysql_wire OK, ERR, EOF, COM, ColumnDef41 and TextResultRow round-trip") {
    const alloc = std.testing.allocator;

    let ok_pkt = OkPacket{
        .affected_rows = 3,
        .last_insert_id = 99,
        .status_flags = 0x0002,
        .warnings = 1,
        .info = try alloc.dupe(u8, "Rows matched: 3"),
    };
    defer ok_pkt.deinit(alloc);
    const ok_raw = try ok_pkt.encode(alloc);
    defer alloc.free(ok_raw);
    let ok_dec = try OkPacket.decode(alloc, ok_raw);
    defer ok_dec.deinit(alloc);
    expect_eq(ok_dec.affected_rows, @as(u64, 3));
    expect_eq(ok_dec.last_insert_id, @as(u64, 99));
    expect_eq(std.mem.eql(u8, ok_dec.info, "Rows matched: 3"), true);

    let err_pkt = ErrPacket{
        .error_code = 1045,
        .sql_state_marker = '#',
        .sql_state = ['2', '8', '0', '0', '0'],
        .error_message = try alloc.dupe(u8, "Access denied for user"),
    };
    defer err_pkt.deinit(alloc);
    const err_raw = try err_pkt.encode(alloc);
    defer alloc.free(err_raw);
    let err_dec = try ErrPacket.decode(alloc, err_raw);
    defer err_dec.deinit(alloc);
    expect_eq(err_dec.error_code, @as(u16, 1045));
    expect_eq(std.mem.eql(u8, &err_dec.sql_state, "28000"), true);
    expect_eq(std.mem.eql(u8, err_dec.error_message, "Access denied for user"), true);

    const eof_pkt = EofPacket{ .warnings = 0, .status_flags = 0x0002 };
    const eof_raw = try eof_pkt.encode(alloc);
    defer alloc.free(eof_raw);
    const eof_dec = try EofPacket.decode(eof_raw);
    expect_eq(eof_dec.status_flags, @as(u16, 0x0002));

    const q_raw = try ComPacket.encodeComQuery(alloc, "SELECT id, note FROM users");
    defer alloc.free(q_raw);
    let q_dec = try ComPacket.decode(alloc, q_raw);
    defer q_dec.deinit(alloc);
    expect_eq(q_dec.command, COM_QUERY);
    expect_eq(std.mem.eql(u8, q_dec.argument, "SELECT id, note FROM users"), true);

    const ping_raw = try ComPacket.encodeComPing(alloc);
    defer alloc.free(ping_raw);
    expect_eq(ping_raw[0], COM_PING);

    const quit_raw = try ComPacket.encodeComQuit(alloc);
    defer alloc.free(quit_raw);
    expect_eq(quit_raw[0], COM_QUIT);

    let col = ColumnDef41{
        .catalog = try alloc.dupe(u8, "def"),
        .schema = try alloc.dupe(u8, "app_db"),
        .table = try alloc.dupe(u8, "users"),
        .org_table = try alloc.dupe(u8, "users"),
        .name = try alloc.dupe(u8, "id"),
        .org_name = try alloc.dupe(u8, "id"),
        .character_set = 63,
        .column_length = 11,
        .column_type = 0x03,
        .flags = 0x0001,
        .decimals = 0,
    };
    defer col.deinit(alloc);
    const col_raw = try col.encode(alloc);
    defer alloc.free(col_raw);
    let col_dec = try ColumnDef41.decode(alloc, col_raw);
    defer col_dec.deinit(alloc);
    expect_eq(std.mem.eql(u8, col_dec.name, "id"), true);
    expect_eq(col_dec.column_length, @as(u32, 11));

    let row_vals = try alloc.alloc(CellValue, 2);
    row_vals[0] = try alloc.dupe(u8, "42");
    row_vals[1] = null;
    let row = TextResultRow{ .values = row_vals };
    defer row.deinit(alloc);

    const row_raw = try row.encode(alloc);
    defer alloc.free(row_raw);
    let row_dec = try TextResultRow.decode(alloc, row_raw, 2);
    defer row_dec.deinit(alloc);
    expect_eq(std.mem.eql(u8, row_dec.values[0].?, "42"), true);
    expect_eq(row_dec.values[1] == null, true);
}

test("mysql_wire MysqlStreamFramer 1..7 byte chunking, sequence_id & max_packet_bytes") {
    const alloc = std.testing.allocator;

    const p0 = try ComPacket.encodeComQuery(alloc, "SELECT 1");
    defer alloc.free(p0);
    const p1 = try ComPacket.encodeComQuery(alloc, "SELECT 2");
    defer alloc.free(p1);

    const f0 = try encodePacketFrame(alloc, 0, p0);
    defer alloc.free(f0);
    const f1 = try encodePacketFrame(alloc, 1, p1);
    defer alloc.free(f1);

    let stream = std.ArrayList(u8).empty;
    defer stream.deinit(alloc);
    try stream.appendSlice(alloc, f0);
    try stream.appendSlice(alloc, f1);

    let framer = MysqlStreamFramer.init(64);
    defer framer.deinit(alloc);

    let pos: usize = 0;
    let step: usize = 1;
    let decoded_count: usize = 0;
    while (pos < stream.items.len) {
        const end = @min(pos + step, stream.items.len);
        try framer.feed(alloc, stream.items[pos..end]);
        pos = end;
        step = if (step >= 7) 1 else step + 1;

        while (true) {
            const maybe_pkt = try framer.nextPacket(alloc);
            if (pkt := maybe_pkt) {
                let owned = pkt;
                defer owned.deinit(alloc);
                expect_eq(owned.sequence_id, @as(u8, @intCast(decoded_count)));
                decoded_count += 1;
            } else {
                break;
            }
        }
    }
    expect_eq(decoded_count, @as(usize, 2));
    expect_eq(framer.pendingBytes(), @as(usize, 0));

    // Oversized packet interception
    let small_framer = MysqlStreamFramer.init(4);
    defer small_framer.deinit(alloc);
    const big_hdr: [5]u8 = [0x05, 0x00, 0x00, 0x00, 0x01];
    try small_framer.feed(alloc, &big_hdr);
    let too_big = false;
    * = small_framer.nextPacket(alloc) catch {
        too_big = true;
    };
    expect_eq(too_big, true);
}

test("mysql_wire cap.mysql_wire dynamic mount & zero leak") {
    const alloc = std.testing.allocator;
    const ping = try cap.mysql_wire.ComPacket.encodeComPing(alloc);
    defer alloc.free(ping);
    expect_eq(ping.len, @as(usize, 1));
    expect_eq(ping[0], cap.mysql_wire.COM_PING);
}
