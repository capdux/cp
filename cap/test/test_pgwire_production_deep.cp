// Source: cp/cap/test/test_pgwire_production_deep.cp (Deep Production Tests for cap:pgwire)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PG_PROTOCOL_VERSION_3_0,
    PG_SSL_REQUEST_CODE,
    DEFAULT_MAX_PACKET_BYTES,
    PgError,
    PgTxStatus,
    PgKv,
    PgColumnDesc,
    PgFieldError,
    PgAuthKind,
    PgFrontendMsg,
    PgBackendMsg,
    PgBackendStreamDecoder,
    PgFrontendStreamDecoder,
    encodeFrontendMsg,
    encodeBackendMsg,
    computePgMd5Password,
} from "cap:pgwire";

test("pgwire frontend roundtrip: SslRequest, StartupMessage, Query, Parse/Bind/Execute/Sync") {
    const alloc = std.testing.allocator;

    const ssl_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{ .ssl_request = {} });
    defer alloc.free(ssl_bytes);

    let kvs: [2]PgKv = [
        PgKv{ .key = "user", .value = "postgres" },
        PgKv{ .key = "database", .value = "mydb" },
    ];
    const startup_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{ .startup = &kvs });
    defer alloc.free(startup_bytes);

    const query_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{ .query = "SELECT 1" });
    defer alloc.free(query_bytes);

    let ptypes: [1]u32 = [23];
    const parse_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{
        .parse = .{ .statement = "s1", .query = "SELECT $1::int4", .param_types = &ptypes },
    });
    defer alloc.free(parse_bytes);

    let pfmts: [1]i16 = [0];
    let params: [1]([]const u8)? = ["42"];
    let rfmts: [1]i16 = [0];
    const bind_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{
        .bind = .{
            .portal = "p1",
            .statement = "s1",
            .param_formats = &pfmts,
            .params = &params,
            .result_formats = &rfmts,
        },
    });
    defer alloc.free(bind_bytes);

    const exec_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{
        .execute = .{ .portal = "p1", .max_rows = 100 },
    });
    defer alloc.free(exec_bytes);

    const sync_bytes = try encodeFrontendMsg(alloc, PgFrontendMsg{ .sync = {} });
    defer alloc.free(sync_bytes);

    let decoder = PgFrontendStreamDecoder.init(alloc, DEFAULT_MAX_PACKET_BYTES, true);
    defer decoder.deinit();

    try decoder.feed(ssl_bytes);
    try decoder.feed(startup_bytes);
    try decoder.feed(query_bytes);
    try decoder.feed(parse_bytes);
    try decoder.feed(bind_bytes);
    try decoder.feed(exec_bytes);
    try decoder.feed(sync_bytes);

    let m1 = (try decoder.next()).?;
    defer m1.deinit(alloc);
    try std.testing.expect(m1 == .ssl_request);

    let m2 = (try decoder.next()).?;
    defer m2.deinit(alloc);
    try std.testing.expect(m2 == .startup);
    try expect_eq(@as(usize, 2), m2.startup.len);
    try std.testing.expectEqualStrings("user", m2.startup[0].key);
    try std.testing.expectEqualStrings("postgres", m2.startup[0].value);
    try std.testing.expectEqualStrings("database", m2.startup[1].key);
    try std.testing.expectEqualStrings("mydb", m2.startup[1].value);

    let m3 = (try decoder.next()).?;
    defer m3.deinit(alloc);
    try std.testing.expectEqualStrings("SELECT 1", m3.query);

    let m4 = (try decoder.next()).?;
    defer m4.deinit(alloc);
    try std.testing.expectEqualStrings("s1", m4.parse.statement);
    try std.testing.expectEqualStrings("SELECT $1::int4", m4.parse.query);
    try expect_eq(@as(u32, 23), m4.parse.param_types[0]);

    let m5 = (try decoder.next()).?;
    defer m5.deinit(alloc);
    try std.testing.expectEqualStrings("p1", m5.bind.portal);
    try std.testing.expectEqualStrings("42", m5.bind.params[0].?);

    let m6 = (try decoder.next()).?;
    defer m6.deinit(alloc);
    try std.testing.expectEqualStrings("p1", m6.execute.portal);
    try expect_eq(@as(i32, 100), m6.execute.max_rows);

    let m7 = (try decoder.next()).?;
    defer m7.deinit(alloc);
    try std.testing.expect(m7 == .sync);
    try std.testing.expect((try decoder.next()) == null);
}

test("pgwire backend streaming 1..7 byte chunks: Auth, RowDescription, DataRow (with NULL), CommandComplete, ReadyForQuery") {
    const alloc = std.testing.allocator;

    let stream_buf = std.ArrayList(u8).empty;
    defer stream_buf.deinit(alloc);

    const salt: [4]u8 = [0xaa, 0xbb, 0xcc, 0xdd];
    const auth_bytes = try encodeBackendMsg(alloc, PgBackendMsg{
        .authentication = PgAuthKind{ .md5_password = salt },
    });
    defer alloc.free(auth_bytes);
    try stream_buf.appendSlice(alloc, auth_bytes);

    let cols: [2]PgColumnDesc = [
        PgColumnDesc{ .name = "id", .table_oid = 100, .column_attr = 1, .type_oid = 23, .type_size = 4, .type_modifier = -1, .format_code = 0 },
        PgColumnDesc{ .name = "note", .table_oid = 100, .column_attr = 2, .type_oid = 25, .type_size = -1, .type_modifier = -1, .format_code = 0 },
    ];
    const row_desc_bytes = try encodeBackendMsg(alloc, PgBackendMsg{ .row_description = &cols });
    defer alloc.free(row_desc_bytes);
    try stream_buf.appendSlice(alloc, row_desc_bytes);

    let row_cells: [2]([]const u8)? = ["7", null];
    const data_row_bytes = try encodeBackendMsg(alloc, PgBackendMsg{ .data_row = &row_cells });
    defer alloc.free(data_row_bytes);
    try stream_buf.appendSlice(alloc, data_row_bytes);

    const cmd_bytes = try encodeBackendMsg(alloc, PgBackendMsg{ .command_complete = "SELECT 1" });
    defer alloc.free(cmd_bytes);
    try stream_buf.appendSlice(alloc, cmd_bytes);

    const ready_bytes = try encodeBackendMsg(alloc, PgBackendMsg{ .ready_for_query = PgTxStatus.Idle });
    defer alloc.free(ready_bytes);
    try stream_buf.appendSlice(alloc, ready_bytes);

    let decoder = PgBackendStreamDecoder.init(alloc, DEFAULT_MAX_PACKET_BYTES);
    defer decoder.deinit();

    let decoded = std.ArrayList(PgBackendMsg).empty;
    defer {
        for (item of decoded.items) {
            let mut_item = item;
            mut_item.deinit(alloc);
        }
        decoded.deinit(alloc);
    }

    let pos: usize = 0;
    let step: usize = 1;
    while (pos < stream_buf.items.len) {
        const end = @min(pos + step, stream_buf.items.len);
        try decoder.feed(stream_buf.items[pos..end]);
        pos = end;
        step = (step % 7) + 1;

        while (msg := try decoder.next()) {
            try decoded.append(alloc, msg);
        }
    }

    try expect_eq(@as(usize, 5), decoded.items.len);
    try std.testing.expectEqualSlices(u8, &salt, &decoded.items[0].authentication.md5_password);
    try expect_eq(@as(usize, 2), decoded.items[1].row_description.len);
    try std.testing.expectEqualStrings("id", decoded.items[1].row_description[0].name);
    try std.testing.expectEqualStrings("note", decoded.items[1].row_description[1].name);
    try std.testing.expectEqualStrings("7", decoded.items[2].data_row[0].?);
    try std.testing.expect(decoded.items[2].data_row[1] == null);
    try std.testing.expectEqualStrings("SELECT 1", decoded.items[3].command_complete);
    try expect_eq(PgTxStatus.Idle, decoded.items[4].ready_for_query);
}

test("pgwire ErrorResponse multi-field parsing & max_packet_bytes / truncated / invalid length guards") {
    const alloc = std.testing.allocator;

    const err_bytes = try encodeBackendMsg(alloc, PgBackendMsg{
        .error_response = .{
            .severity = "ERROR",
            .code = "42P01",
            .message = "relation \"missing\" does not exist",
            .detail = "table lookup failed",
            .hint = null,
        },
    });
    defer alloc.free(err_bytes);

    let decoder = PgBackendStreamDecoder.init(alloc, 1024);
    defer decoder.deinit();
    try decoder.feed(err_bytes);

    let msg = (try decoder.next()).?;
    defer msg.deinit(alloc);
    try std.testing.expectEqualStrings("ERROR", msg.error_response.severity);
    try std.testing.expectEqualStrings("42P01", msg.error_response.code);
    try std.testing.expectEqualStrings("relation \"missing\" does not exist", msg.error_response.message);
    try std.testing.expectEqualStrings("table lookup failed", msg.error_response.detail.?);
    try std.testing.expect(msg.error_response.hint == null);

    // Truncated packet returns null (Incomplete)
    let trunc_decoder = PgBackendStreamDecoder.init(alloc, 1024);
    defer trunc_decoder.deinit();
    try trunc_decoder.feed(err_bytes[0..6]);
    try std.testing.expect((try trunc_decoder.next()) == null);

    // Oversized packet exceeds max_packet_bytes -> PacketTooLarge
    let small_decoder = PgBackendStreamDecoder.init(alloc, 16);
    defer small_decoder.deinit();
    try small_decoder.feed(err_bytes);
    try std.testing.expectError(PgError.PacketTooLarge, small_decoder.next());

    // Invalid length < 4 -> InvalidPacket
    let bad_decoder = PgBackendStreamDecoder.init(alloc, 1024);
    defer bad_decoder.deinit();
    const bad_frame: [5]u8 = ['Z', 0, 0, 0, 2];
    try bad_decoder.feed(&bad_frame);
    try std.testing.expectError(PgError.InvalidPacket, bad_decoder.next());
}

test("pgwire computePgMd5Password matches official PostgreSQL MD5 algorithm") {
    const alloc = std.testing.allocator;
    const salt: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    const actual = try computePgMd5Password(alloc, "postgres", "secret", salt);
    defer alloc.free(actual);

    // Reference 2-round MD5 calculation: "md5" + md5(md5("secret" + "postgres") + salt)
    let h1 = std.crypto.hash.Md5.init(.{});
    h1.update("secret");
    h1.update("postgres");
    let d1: [16]u8 = undefined;
    h1.final(&d1);
    const hex1 = std.fmt.bytesToHex(d1, .lower);

    let h2 = std.crypto.hash.Md5.init(.{});
    h2.update(&hex1);
    h2.update(&salt);
    let d2: [16]u8 = undefined;
    h2.final(&d2);
    const hex2 = std.fmt.bytesToHex(d2, .lower);

    let expected: [35]u8 = undefined;
    expected[0] = 'm';
    expected[1] = 'd';
    expected[2] = '5';
    @memcpy(expected[3..35], &hex2);

    try expect_eq(@as(usize, 35), actual.len);
    try std.testing.expectEqualStrings(&expected, actual);
}

test("pgwire cap.pgwire dynamic mount & zero memory leaks") {
    const alloc = std.testing.allocator;
    const bytes = try cap.pgwire.encodeBackendMsg(alloc, cap.pgwire.PgBackendMsg{ .parse_complete = {} });
    defer alloc.free(bytes);

    let decoder = cap.pgwire.PgBackendStreamDecoder.init(alloc, cap.pgwire.DEFAULT_MAX_PACKET_BYTES);
    defer decoder.deinit();
    try decoder.feed(bytes);
    let msg = (try decoder.next()).?;
    defer msg.deinit(alloc);
    try std.testing.expect(msg == .parse_complete);
}
