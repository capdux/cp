// Source: cp/cap/test/test_mysql_control_production_deep.cp (Production deep tests for cap:mysql control)

import { test, expect_eq } from "cap:test";
import {
    HandshakeV10,
    OKPacket,
    ErrorPacket,
    EOFPacket,
    AuthSwitchRequest,
    CommandType,
    CAP_CLIENT_SSL,
    CAP_CLIENT_PLUGIN_AUTH,
} from "cap:mysql";

test("HandshakeV10 full decode, 32-bit capability merge and 20-byte salt assembly") {
    let pkt: [128]u8 = [0; 128];
    let p: usize = 0;

    // protocol_version = 10
    pkt[p] = 10; p += 1;
    // server_version = "8.0.36\0"
    const ver = "8.0.36";
    @memcpy(pkt[p .. p + ver.len], ver); p += ver.len;
    pkt[p] = 0; p += 1;
    // connection_id = 0x11223344 LE
    pkt[p] = 0x44; pkt[p + 1] = 0x33; pkt[p + 2] = 0x22; pkt[p + 3] = 0x11; p += 4;
    // auth_plugin_data_part_1 (8 bytes)
    const salt1 = "ABCDEFGH";
    @memcpy(pkt[p .. p + 8], salt1); p += 8;
    // filler = 0x00
    pkt[p] = 0; p += 1;
    // capability_flags_lower: SSL (0x0800) | PROTOCOL_41 (0x0200) = 0x0A00
    pkt[p] = 0x00; pkt[p + 1] = 0x0A; p += 2;
    // character_set = 45 (utf8mb4_general_ci)
    pkt[p] = 45; p += 1;
    // status_flags = 0x0002 (AUTOCOMMIT)
    pkt[p] = 0x02; pkt[p + 1] = 0x00; p += 2;
    // capability_flags_upper: PLUGIN_AUTH (0x0008 << 16 = 1 << 19) | DEPRECATE_EOF (0x0100 << 16) = 0x0108
    pkt[p] = 0x08; pkt[p + 1] = 0x01; p += 2;
    // auth_plugin_data_len = 21
    pkt[p] = 21; p += 1;
    // 10 reserved bytes
    p += 10;
    // auth_plugin_data_part_2 (12 bytes + 1 NUL = 13 bytes)
    const salt2 = "123456789012";
    @memcpy(pkt[p .. p + 12], salt2); p += 12;
    pkt[p] = 0; p += 1;
    // auth_plugin_name = "caching_sha2_password\0"
    const plugin = "caching_sha2_password";
    @memcpy(pkt[p .. p + plugin.len], plugin); p += plugin.len;
    pkt[p] = 0; p += 1;

    const hs = HandshakeV10.decode(pkt[0..p]) catch unreachable;
    expect_eq(hs.protocol_version, @as(u8, 10));
    expect_eq(hs.server_version, "8.0.36");
    expect_eq(hs.connection_id, @as(u32, 0x11223344));
    expect_eq(hs.character_set, @as(u8, 45));
    expect_eq(hs.status_flags, @as(u16, 0x0002));
    expect_eq(hs.capability_flags, @as(u32, 0x01080A00));
    expect_eq(hs.supportsSsl(), true);
    expect_eq(hs.supportsPluginAuth(), true);
    expect_eq(hs.auth_plugin_name, "caching_sha2_password");
    expect_eq((hs.capability_flags & CAP_CLIENT_SSL) != 0, true);
    expect_eq((hs.capability_flags & CAP_CLIENT_PLUGIN_AUTH) != 0, true);

    let full_salt: [20]u8 = [0; 20];
    const salt_len = hs.writeAuthPluginData(full_salt[0..20]);
    expect_eq(salt_len, @as(usize, 20));
    expect_eq(std.mem.eql(u8, full_salt[0..20], "ABCDEFGH123456789012"), true);
}

test("OKPacket decoding with length-encoded integers, status, warnings and info") {
    // 1. 标准 0x00 OKPacket: affected=3, last_insert_id=0x012C (300 via 0xFC), status=0x0002, warnings=1, info="Rows matched: 3"
    let ok1: [32]u8 = [0; 32];
    let p: usize = 0;
    ok1[p] = 0x00; p += 1;
    ok1[p] = 3; p += 1;
    ok1[p] = 0xFC; ok1[p + 1] = 0x2C; ok1[p + 2] = 0x01; p += 3; // 300
    ok1[p] = 0x02; ok1[p + 1] = 0x00; p += 2; // status
    ok1[p] = 0x01; ok1[p + 1] = 0x00; p += 2; // warnings
    const info_msg = "Rows matched: 3";
    @memcpy(ok1[p .. p + info_msg.len], info_msg); p += info_msg.len;

    const parsed = OKPacket.decode(ok1[0..p]) catch unreachable;
    expect_eq(parsed.header, @as(u8, 0x00));
    expect_eq(parsed.isEofStyle(), false);
    expect_eq(parsed.affected_rows, @as(u64, 3));
    expect_eq(parsed.last_insert_id, @as(u64, 300));
    expect_eq(parsed.status_flags, @as(u16, 0x0002));
    expect_eq(parsed.warnings, @as(u16, 1));
    expect_eq(parsed.info, "Rows matched: 3");

    // 2. DEPRECATE_EOF 风格的 0xFE OKPacket（含 3-byte 0xFD lenenc）
    const ok2: [10]u8 = [0xFE, 0xFD, 0x01, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x00, 0x00];
    const parsed2 = OKPacket.decode(ok2[0..10]) catch unreachable;
    expect_eq(parsed2.isEofStyle(), true);
    expect_eq(parsed2.affected_rows, @as(u64, 0x010001));
    expect_eq(parsed2.last_insert_id, @as(u64, 0));
    expect_eq(parsed2.status_flags, @as(u16, 0x000A));
    expect_eq(parsed2.info, "");
}

test("ErrorPacket decoding with and without SQLSTATE") {
    // 1. MySQL 4.1+ 带 '#' + 5 字节 SQLSTATE
    const err_pkt = "\xFF\x7A\x04#42S02Table 'test.users' doesn't exist";
    const e1 = ErrorPacket.decode(err_pkt) catch unreachable;
    expect_eq(e1.header, @as(u8, 0xFF));
    expect_eq(e1.error_code, @as(u16, 1146)); // 0x047A
    expect_eq(e1.has_sql_state, true);
    expect_eq(e1.sqlStateSlice().?, "42S02");
    expect_eq(e1.error_message, "Table 'test.users' doesn't exist");

    // 2. 无 SQLSTATE 的旧协议错误报文
    const err_legacy = "\xFF\xE8\x03Access denied";
    const e2 = ErrorPacket.decode(err_legacy) catch unreachable;
    expect_eq(e2.error_code, @as(u16, 1000));
    expect_eq(e2.has_sql_state, false);
    expect_eq(e2.sqlStateSlice() == null, true);
    expect_eq(e2.error_message, "Access denied");
}

test("EOFPacket and AuthSwitchRequest zero-copy decoding") {
    const eof_bytes: [5]u8 = [0xFE, 0x02, 0x00, 0x0A, 0x00];
    const eof = EOFPacket.decode(eof_bytes[0..5]) catch unreachable;
    expect_eq(eof.header, @as(u8, 0xFE));
    expect_eq(eof.warnings, @as(u16, 2));
    expect_eq(eof.status_flags, @as(u16, 0x000A));

    const auth_sw = "\xFEmysql_native_password\x000123456789abcdef0123";
    const req = AuthSwitchRequest.decode(auth_sw) catch unreachable;
    expect_eq(req.header, @as(u8, 0xFE));
    expect_eq(req.plugin_name, "mysql_native_password");
    expect_eq(std.mem.eql(u8, req.plugin_data, "0123456789abcdef0123"), true);
}

test("CommandType 31 commands roundtrip and prepared statement classification") {
    expect_eq(CommandType.fromByte(0x00) == null, true);
    expect_eq(CommandType.fromByte(0x20) == null, true);
    expect_eq(CommandType.fromByte(0xFF) == null, true);

    let stmt_count: usize = 0;
    let b: u8 = 0x01;
    while (b <= 0x1F) {
        if (cmd := CommandType.fromByte(b)) {
            expect_eq(cmd.asByte(), b);
            if (cmd.isPreparedStmt()) {
                stmt_count += 1;
            }
        } else {
            expect_eq(true, false);
        }
        b += 1;
    }
    expect_eq(stmt_count, @as(usize, 6));
    expect_eq(CommandType.COM_QUERY.asByte(), @as(u8, 0x03));
    expect_eq(CommandType.COM_STMT_EXECUTE.isPreparedStmt(), true);
}

test("Malformed and truncated control packet boundary guards") {
    // 1. HandshakeV10 不支持版本 9 & 截断
    const bad_ver: [4]u8 = [9, '8', 0, 0];
    expect_eq(switch (HandshakeV10.decode(bad_ver[0..4])) { err.UnsupportedProtocolVersion => true, else => false }, true);
    const trunc_hs: [6]u8 = [10, '8', '.', '0', 0, 1];
    expect_eq(switch (HandshakeV10.decode(trunc_hs[0..6])) { err.TruncatedPacket => true, else => false }, true);

    // 2. OKPacket 非法头 & 非法 lenenc 0xFB
    const bad_ok: [5]u8 = [0x01, 0, 0, 0, 0];
    expect_eq(switch (OKPacket.decode(bad_ok[0..5])) { err.InvalidOKPacket => true, else => false }, true);
    const bad_lenenc: [6]u8 = [0x00, 0xFB, 0, 0, 0, 0];
    expect_eq(switch (OKPacket.decode(bad_lenenc[0..6])) { err.InvalidLengthEncodedInt => true, else => false }, true);

    // 3. ErrorPacket 非法头 & 截断 SQLSTATE
    const bad_err: [3]u8 = [0x00, 1, 0];
    expect_eq(switch (ErrorPacket.decode(bad_err[0..3])) { err.InvalidErrorPacket => true, else => false }, true);
    const trunc_sqlstate = "\xFF\x01\x00#42S";
    expect_eq(switch (ErrorPacket.decode(trunc_sqlstate)) { err.TruncatedPacket => true, else => false }, true);

    // 4. EOFPacket 截断 & 非法头
    const trunc_eof: [3]u8 = [0xFE, 0, 0];
    expect_eq(switch (EOFPacket.decode(trunc_eof[0..3])) { err.TruncatedPacket => true, else => false }, true);
    const bad_eof: [5]u8 = [0x00, 0, 0, 0, 0];
    expect_eq(switch (EOFPacket.decode(bad_eof[0..5])) { err.InvalidEOFPacket => true, else => false }, true);

    // 5. AuthSwitchRequest 缺少 NUL 分隔符
    const bad_auth = "\xFEno_nul_terminator";
    expect_eq(switch (AuthSwitchRequest.decode(bad_auth)) { err.InvalidAuthSwitchRequest => true, else => false }, true);
}
