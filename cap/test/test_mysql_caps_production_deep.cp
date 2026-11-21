// Source: cp/cap/test/test_mysql_caps_production_deep.cp (Production deep tests for cap:mysql_caps)

import { test, expect_eq } from "cap:test";
import {
    CapabilityFlag,
    Capabilities,
    StatusFlag,
    StatusFlags,
} from "cap:mysql_caps";

test("mysql_caps: all 32 capability bit positions round-trip and set/clear/has") {
    let caps = Capabilities.fromInt(0);
    expect_eq(caps.toInt(), 0);

    caps.set(.CLIENT_LONG_PASSWORD);
    caps.set(.CLIENT_PROTOCOL_41);
    caps.set(.CLIENT_SSL);
    caps.set(.CLIENT_PLUGIN_AUTH);
    caps.set(.CLIENT_DEPRECATE_EOF);
    caps.set(.CLIENT_REMEMBER_OPTIONS);

    expect_eq(caps.has(.CLIENT_LONG_PASSWORD), true);
    expect_eq(caps.has(.CLIENT_PROTOCOL_41), true);
    expect_eq(caps.has(.CLIENT_SSL), true);
    expect_eq(caps.has(.CLIENT_PLUGIN_AUTH), true);
    expect_eq(caps.has(.CLIENT_DEPRECATE_EOF), true);
    expect_eq(caps.has(.CLIENT_REMEMBER_OPTIONS), true);
    expect_eq(caps.has(.CLIENT_ODBC), false);

    const expected: u32 = 1 | 512 | 2048 | 524288 | 16777216 | 2147483648;
    expect_eq(caps.toInt(), expected);

    caps.clear(.CLIENT_SSL);
    expect_eq(caps.has(.CLIENT_SSL), false);
    expect_eq(caps.toInt(), expected & ~@as(u32, 2048));

    expect_eq(std.mem.eql(u8, CapabilityFlag.CLIENT_DEPRECATE_EOF.bitName(), "CLIENT_DEPRECATE_EOF"), true);
}

test("mysql_caps: defaultClient and client/server handshake intersect negotiation") {
    const client = Capabilities.defaultClient();
    expect_eq(client.has(.CLIENT_PROTOCOL_41), true);
    expect_eq(client.has(.CLIENT_SECURE_CONNECTION), true);
    expect_eq(client.has(.CLIENT_PLUGIN_AUTH), true);
    expect_eq(client.has(.CLIENT_DEPRECATE_EOF), true);
    expect_eq(client.has(.CLIENT_LOCAL_FILES), false);
    expect_eq(client.has(.CLIENT_ZSTD_COMPRESSION_ALGORITHM), false);

    // 模拟仅支持部分能力的旧版服务器（不支持 DEPRECATE_EOF 与 SESSION_TRACK，但广播了 ZSTD）
    let server = Capabilities.fromInt(0);
    server.set(.CLIENT_LONG_PASSWORD);
    server.set(.CLIENT_PROTOCOL_41);
    server.set(.CLIENT_TRANSACTIONS);
    server.set(.CLIENT_SECURE_CONNECTION);
    server.set(.CLIENT_PLUGIN_AUTH);
    server.set(.CLIENT_ZSTD_COMPRESSION_ALGORITHM);

    const negotiated = client.intersect(server);
    expect_eq(negotiated.has(.CLIENT_PROTOCOL_41), true);
    expect_eq(negotiated.has(.CLIENT_PLUGIN_AUTH), true);
    expect_eq(negotiated.has(.CLIENT_DEPRECATE_EOF), false);
    expect_eq(negotiated.has(.CLIENT_SESSION_TRACK), false);
    expect_eq(negotiated.has(.CLIENT_ZSTD_COMPRESSION_ALGORITHM), false);
}

test("mysql_caps: rejectUnsupported strips ZSTD, MFA, ODBC, LOCAL_FILES and preserves core bits") {
    let all = Capabilities.fromInt(0xFFFFFFFF);
    all.rejectUnsupported();

    // 已剔除的扩展/高风险位
    expect_eq(all.has(.CLIENT_ZSTD_COMPRESSION_ALGORITHM), false);
    expect_eq(all.has(.MULTI_FACTOR_AUTHENTICATION), false);
    expect_eq(all.has(.CLIENT_CAPABILITY_EXTENSION), false);
    expect_eq(all.has(.CLIENT_SSL_VERIFY_SERVER_CERT), false);
    expect_eq(all.has(.CLIENT_REMEMBER_OPTIONS), false);
    expect_eq(all.has(.CLIENT_COMPRESS), false);
    expect_eq(all.has(.CLIENT_ODBC), false);
    expect_eq(all.has(.CLIENT_LOCAL_FILES), false);
    expect_eq(all.has(.CLIENT_QUERY_ATTRIBUTES), false);
    expect_eq(all.has(.CLIENT_OPTIONAL_RESULTSET_METADATA), false);

    // 核心握手与协议能力位保留
    expect_eq(all.has(.CLIENT_LONG_PASSWORD), true);
    expect_eq(all.has(.CLIENT_PROTOCOL_41), true);
    expect_eq(all.has(.CLIENT_SSL), true);
    expect_eq(all.has(.CLIENT_TRANSACTIONS), true);
    expect_eq(all.has(.CLIENT_SECURE_CONNECTION), true);
    expect_eq(all.has(.CLIENT_PLUGIN_AUTH), true);
    expect_eq(all.has(.CLIENT_DEPRECATE_EOF), true);
}

test("mysql_caps: formatList human-readable output and buffer truncation guard") {
    let caps = Capabilities.fromInt(0);
    caps.set(.CLIENT_LONG_PASSWORD);
    caps.set(.CLIENT_PROTOCOL_41);
    caps.set(.CLIENT_SSL);

    let buf: [128]u8 = [0; 128];
    const formatted = caps.formatList(buf[0..]);
    expect_eq(
        std.mem.eql(u8, formatted, "CLIENT_LONG_PASSWORD, CLIENT_PROTOCOL_41, CLIENT_SSL"),
        true
    );

    const empty = Capabilities.fromInt(0);
    expect_eq(empty.formatList(buf[0..]).len, 0);

    // 小缓冲区截断保护不越界
    let tiny: [10]u8 = [0; 10];
    const trunc = caps.formatList(tiny[0..]);
    expect_eq(trunc.len, 10);
    expect_eq(std.mem.eql(u8, trunc, "CLIENT_LON"), true);
}

test("mysql_caps: StatusFlags 16-bit decoding, transaction, autocommit and multi-result helpers") {
    const raw_val: u16 =
        @intFromEnum(StatusFlag.SERVER_STATUS_IN_TRANS) |
        @intFromEnum(StatusFlag.SERVER_STATUS_AUTOCOMMIT) |
        @intFromEnum(StatusFlag.SERVER_MORE_RESULTS_EXISTS) |
        @intFromEnum(StatusFlag.SERVER_STATUS_IN_TRANS_READONLY) |
        @intFromEnum(StatusFlag.SERVER_SESSION_STATE_CHANGED);

    const st = StatusFlags.fromInt(raw_val);
    expect_eq(st.toInt(), raw_val);
    expect_eq(st.inTransaction(), true);
    expect_eq(st.isAutoCommit(), true);
    expect_eq(st.hasMoreResults(), true);
    expect_eq(st.isReadOnlyTransaction(), true);
    expect_eq(st.has(.SERVER_SESSION_STATE_CHANGED), true);
    expect_eq(st.has(.SERVER_QUERY_WAS_SLOW), false);
    expect_eq(st.has(.SERVER_STATUS_CURSOR_EXISTS), false);

    const idle = StatusFlags.fromInt(@intFromEnum(StatusFlag.SERVER_STATUS_AUTOCOMMIT));
    expect_eq(idle.inTransaction(), false);
    expect_eq(idle.isAutoCommit(), true);
    expect_eq(idle.hasMoreResults(), false);
    expect_eq(idle.isReadOnlyTransaction(), false);
}
