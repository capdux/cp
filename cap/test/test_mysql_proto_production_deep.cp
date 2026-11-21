// Source: cp/cap/test/test_mysql_proto_production_deep.cp (Production deep tests for cap:mysql_proto)

import { test, expect_eq } from "cap:test";
import {
    Capabilities,
    CAP_CLIENT_LONG_PASSWORD,
    CAP_CLIENT_CONNECT_WITH_DB,
    CAP_CLIENT_COMPRESS,
    CAP_CLIENT_PROTOCOL_41,
    CAP_CLIENT_SSL,
    CAP_CLIENT_TRANSACTIONS,
    CAP_CLIENT_SECURE_CONNECTION,
    CAP_CLIENT_MULTI_STATEMENTS,
    CAP_CLIENT_MULTI_RESULTS,
    CAP_CLIENT_PLUGIN_AUTH,
    CAP_CLIENT_DEPRECATE_EOF,
    CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM,
    CAP_MULTI_FACTOR_AUTHENTICATION,
    CAP_CLIENT_REMEMBER_OPTIONS,
    StatusFlag,
    StatusFlags,
    CharacterSet,
} from "cap:mysql_proto";

test("Capabilities 32-bit mask roundtrip and hasFlag") {
    // 零值与全置位边界
    const empty = Capabilities.fromU32(0);
    expect_eq(empty.toU32(), @as(u32, 0));
    expect_eq(empty.hasFlag(0), false);
    expect_eq(empty.hasFlag(CAP_CLIENT_PROTOCOL_41), false);

    const all_bits: u32 = 0xFFFFFFFF;
    const full = Capabilities.fromU32(all_bits);
    expect_eq(full.toU32(), all_bits);
    expect_eq(full.CLIENT_LONG_PASSWORD, true);
    expect_eq(full.CLIENT_REMEMBER_OPTIONS, true);
    expect_eq(full.hasFlag(CAP_CLIENT_LONG_PASSWORD | CAP_CLIENT_REMEMBER_OPTIONS), true);
    expect_eq(full.hasFlag(0), false);

    // 逐位验证 0..31 每一位独立编码与解码精确往返
    let bit: u5 = 0;
    while (true) {
        const mask: u32 = @as(u32, 1) << bit;
        const cap = Capabilities.fromU32(mask);
        expect_eq(cap.toU32(), mask);
        expect_eq(cap.hasFlag(mask), true);

        const other_bit: u5 = if (bit == 31) 0 else bit + 1;
        const other_mask: u32 = @as(u32, 1) << other_bit;
        expect_eq(cap.hasFlag(other_mask), false);
        expect_eq(cap.hasFlag(mask | other_mask), false);

        if (bit == 31) {
            break;
        }
        bit += 1;
    }
}

test("Capabilities defaultClient, intersect, unionWith and rejectUnsupported") {
    const c_ssl_db = Capabilities.defaultClient(true, true);
    expect_eq(c_ssl_db.CLIENT_PROTOCOL_41, true);
    expect_eq(c_ssl_db.CLIENT_PLUGIN_AUTH, true);
    expect_eq(c_ssl_db.CLIENT_SECURE_CONNECTION, true);
    expect_eq(c_ssl_db.CLIENT_DEPRECATE_EOF, true);
    expect_eq(c_ssl_db.CLIENT_MULTI_STATEMENTS, true);
    expect_eq(c_ssl_db.CLIENT_MULTI_RESULTS, true);
    expect_eq(c_ssl_db.CLIENT_SSL, true);
    expect_eq(c_ssl_db.CLIENT_CONNECT_WITH_DB, true);

    const c_plain = Capabilities.defaultClient(false, false);
    expect_eq(c_plain.CLIENT_SSL, false);
    expect_eq(c_plain.CLIENT_CONNECT_WITH_DB, false);
    expect_eq(c_plain.hasFlag(CAP_CLIENT_PROTOCOL_41 | CAP_CLIENT_PLUGIN_AUTH), true);

    // intersect 与 unionWith 验证
    const server_caps = Capabilities.fromU32(
        CAP_CLIENT_PROTOCOL_41 |
        CAP_CLIENT_SECURE_CONNECTION |
        CAP_CLIENT_SSL |
        CAP_CLIENT_TRANSACTIONS |
        CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM |
        CAP_MULTI_FACTOR_AUTHENTICATION |
        CAP_CLIENT_COMPRESS
    );

    const negotiated = c_ssl_db.intersect(server_caps);
    expect_eq(negotiated.toU32(), CAP_CLIENT_PROTOCOL_41 | CAP_CLIENT_SECURE_CONNECTION | CAP_CLIENT_SSL);

    const combined = c_plain.unionWith(server_caps);
    expect_eq(combined.hasFlag(CAP_CLIENT_TRANSACTIONS), true);
    expect_eq(combined.hasFlag(CAP_CLIENT_MULTI_RESULTS), true);
    expect_eq(combined.hasFlag(CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM), true);

    // rejectUnsupported 清除不支持的压缩、MFA 与扩展能力位
    let filtered = Capabilities.fromU32(0xFFFFFFFF);
    filtered.rejectUnsupported();
    expect_eq(filtered.CLIENT_ZSTD_COMPRESSION_ALGORITHM, false);
    expect_eq(filtered.MULTI_FACTOR_AUTHENTICATION, false);
    expect_eq(filtered.CLIENT_COMPRESS, false);
    expect_eq(filtered.CLIENT_REMEMBER_OPTIONS, false);
    expect_eq(filtered.CLIENT_PROTOCOL_41, true);
    expect_eq(filtered.CLIENT_SSL, true);
    expect_eq(filtered.CLIENT_DEPRECATE_EOF, true);
}

test("StatusFlags transaction, multi-result, set/clear and roundtrip") {
    let st = StatusFlags.fromU16(0);
    expect_eq(st.toU16(), @as(u16, 0));
    expect_eq(st.inTransaction(), false);
    expect_eq(st.inReadOnlyTransaction(), false);
    expect_eq(st.isAutoCommit(), false);
    expect_eq(st.hasMoreResults(), false);
    expect_eq(st.cursorExists(), false);
    expect_eq(st.lastRowSent(), false);
    expect_eq(st.isSlowQuery(), false);
    expect_eq(st.isSessionStateChanged(), false);

    // 常规事务 + 自动提交 + 多结果集
    st.set(.SERVER_STATUS_IN_TRANS);
    st.set(.SERVER_STATUS_AUTOCOMMIT);
    st.set(.SERVER_MORE_RESULTS_EXISTS);
    expect_eq(st.toU16(), @as(u16, 1 | 2 | 8));
    expect_eq(st.inTransaction(), true);
    expect_eq(st.inReadOnlyTransaction(), false);
    expect_eq(st.isAutoCommit(), true);
    expect_eq(st.hasMoreResults(), true);

    // 清除常规事务标志后，仅置位只读事务标志
    st.clear(.SERVER_STATUS_IN_TRANS);
    expect_eq(st.inTransaction(), false);
    st.set(.SERVER_STATUS_IN_TRANS_READONLY);
    expect_eq(st.inTransaction(), true);
    expect_eq(st.inReadOnlyTransaction(), true);

    // 组合全部高位标志验证
    const high_mask: u16 =
        (StatusFlag.SERVER_STATUS_CURSOR_EXISTS as u16) |
        (StatusFlag.SERVER_STATUS_LAST_ROW_SENT as u16) |
        (StatusFlag.SERVER_QUERY_WAS_SLOW as u16) |
        (StatusFlag.SERVER_SESSION_STATE_CHANGED as u16);
    const high_st = StatusFlags.fromU16(high_mask);
    expect_eq(high_st.toU16(), high_mask);
    expect_eq(high_st.cursorExists(), true);
    expect_eq(high_st.lastRowSent(), true);
    expect_eq(high_st.isSlowQuery(), true);
    expect_eq(high_st.isSessionStateChanged(), true);
    expect_eq(high_st.inTransaction(), false);
    expect_eq(high_st.hasMoreResults(), false);
}

test("CharacterSet UTF8MB4, UTF8MB3, Latin1, Binary and CJK metadata mapping") {
    // 默认 utf8mb4_general_ci (45)
    const def_cs = CharacterSet.default;
    expect_eq(def_cs.id(), @as(u16, 45));
    expect_eq(def_cs.name(), "utf8mb4_general_ci");
    expect_eq(def_cs.charset(), "utf8mb4");
    expect_eq(def_cs.maxBytesPerChar(), @as(u8, 4));
    expect_eq(def_cs.isUtf8(), true);
    expect_eq(def_cs.isUtf8mb4(), true);
    expect_eq(def_cs.isBinary(), false);

    // MySQL 8.0 默认 utf8mb4_0900_ai_ci (255)
    const cs80 = CharacterSet.utf8mb4_0900_ai_ci;
    expect_eq(cs80.id(), @as(u16, 255));
    expect_eq(cs80.name(), "utf8mb4_0900_ai_ci");
    expect_eq(cs80.charset(), "utf8mb4");
    expect_eq(cs80.maxBytesPerChar(), @as(u8, 4));
    expect_eq(cs80.isUtf8(), true);
    expect_eq(cs80.isUtf8mb4(), true);

    // utf8mb3_general_ci (33)
    const cs3 = CharacterSet.utf8mb3_general_ci;
    expect_eq(cs3.id(), @as(u16, 33));
    expect_eq(cs3.name(), "utf8mb3_general_ci");
    expect_eq(cs3.charset(), "utf8mb3");
    expect_eq(cs3.maxBytesPerChar(), @as(u8, 3));
    expect_eq(cs3.isUtf8(), true);
    expect_eq(cs3.isUtf8mb4(), false);

    // latin1_swedish_ci (8)
    const latin1 = CharacterSet.latin1_swedish_ci;
    expect_eq(latin1.id(), @as(u16, 8));
    expect_eq(latin1.name(), "latin1_swedish_ci");
    expect_eq(latin1.charset(), "latin1");
    expect_eq(latin1.maxBytesPerChar(), @as(u8, 1));
    expect_eq(latin1.isUtf8(), false);
    expect_eq(latin1.isBinary(), false);

    // binary (63)
    const bin = CharacterSet.binary;
    expect_eq(bin.id(), @as(u16, 63));
    expect_eq(bin.name(), "binary");
    expect_eq(bin.charset(), "binary");
    expect_eq(bin.maxBytesPerChar(), @as(u8, 1));
    expect_eq(bin.isUtf8(), false);
    expect_eq(bin.isBinary(), true);

    // CJK 与多字节字符集：gbk (28, max=2), gb18030 (248, max=4), utf16 (54, max=4), ucs2 (35, max=2), ujis (12, max=3)
    expect_eq(CharacterSet.gbk_chinese_ci.charset(), "gbk");
    expect_eq(CharacterSet.gbk_chinese_ci.maxBytesPerChar(), @as(u8, 2));
    expect_eq(CharacterSet.gb18030_chinese_ci.charset(), "gb18030");
    expect_eq(CharacterSet.gb18030_chinese_ci.maxBytesPerChar(), @as(u8, 4));
    expect_eq(CharacterSet.utf16_general_ci.charset(), "utf16");
    expect_eq(CharacterSet.utf16_general_ci.maxBytesPerChar(), @as(u8, 4));
    expect_eq(CharacterSet.ucs2_general_ci.charset(), "ucs2");
    expect_eq(CharacterSet.ucs2_general_ci.maxBytesPerChar(), @as(u8, 2));
    expect_eq(CharacterSet.ujis_japanese_ci.charset(), "ujis");
    expect_eq(CharacterSet.ujis_japanese_ci.maxBytesPerChar(), @as(u8, 3));
}

test("CharacterSet.fromId exhaustive boundary guard and unknown ID rejection") {
    // 明确未知与保留空洞 ID 必须返回 null
    const invalid_ids: [14]u16 = [0, 17, 100, 125, 127, 152, 158, 184, 191, 216, 222, 251, 254, 65535];
    let idx: usize = 0;
    while (idx < invalid_ids.len) {
        const res = CharacterSet.fromId(invalid_ids[idx]);
        expect_eq(res == null, true);
        idx += 1;
    }

    // 扫描 0..320 统计全部有效 collation 数量并验证双向一致性
    let valid_count: usize = 0;
    let id: u16 = 0;
    while (id <= 320) {
        if (cs := CharacterSet.fromId(id)) {
            expect_eq(cs.id(), id);
            expect_eq(cs.name().len > 0, true);
            expect_eq(cs.charset().len > 0, true);
            const ml = cs.maxBytesPerChar();
            expect_eq(ml >= 1 && ml <= 4, true);
            valid_count += 1;
        }
        id += 1;
    }
    // 98 (1..99 excl 17) + 24 (101..124) + 24 (128..151) + 1 (159)
    // + 24 (160..183) + 24 (192..215) + 1 (223) + 27 (224..250) + 3 (255,278,309) = 226
    expect_eq(valid_count, @as(usize, 226));
}

test("MySQL 8.0 handshake and multi-statement status workflow integration") {
    // 1. 服务端下发能力掩码（含 protocol 4.1、plugin auth、secure conn、deprecate eof、ssl、multi results、zstd）
    const server_raw: u32 =
        CAP_CLIENT_PROTOCOL_41 |
        CAP_CLIENT_PLUGIN_AUTH |
        CAP_CLIENT_SECURE_CONNECTION |
        CAP_CLIENT_DEPRECATE_EOF |
        CAP_CLIENT_SSL |
        CAP_CLIENT_MULTI_STATEMENTS |
        CAP_CLIENT_MULTI_RESULTS |
        CAP_CLIENT_CONNECT_WITH_DB |
        CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM;

    const client = Capabilities.defaultClient(true, true);
    let negotiated = client.intersect(Capabilities.fromU32(server_raw));
    negotiated.rejectUnsupported();

    expect_eq(negotiated.hasFlag(CAP_CLIENT_PROTOCOL_41 | CAP_CLIENT_SSL | CAP_CLIENT_CONNECT_WITH_DB), true);
    expect_eq(negotiated.hasFlag(CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM), false);

    // 2. 选择字符集 utf8mb4_0900_ai_ci (255)
    let selected_max_len: u8 = 0;
    if (cs := CharacterSet.fromId(255)) {
        expect_eq(cs.isUtf8mb4(), true);
        selected_max_len = cs.maxBytesPerChar();
    }
    expect_eq(selected_max_len, @as(u8, 4));

    // 3. 模拟多语句事务执行中的状态转移
    let ok_packet_status = StatusFlags.fromU16(
        (StatusFlag.SERVER_STATUS_IN_TRANS as u16) |
        (StatusFlag.SERVER_MORE_RESULTS_EXISTS as u16)
    );
    expect_eq(ok_packet_status.inTransaction(), true);
    expect_eq(ok_packet_status.hasMoreResults(), true);

    // 读取完最后一个结果集并 COMMIT 后
    ok_packet_status.clear(.SERVER_MORE_RESULTS_EXISTS);
    ok_packet_status.clear(.SERVER_STATUS_IN_TRANS);
    ok_packet_status.set(.SERVER_STATUS_AUTOCOMMIT);
    expect_eq(ok_packet_status.hasMoreResults(), false);
    expect_eq(ok_packet_status.inTransaction(), false);
    expect_eq(ok_packet_status.isAutoCommit(), true);
}
