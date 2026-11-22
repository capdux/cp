// Production deep test suite for cap:mysql COM_QUERY (simple & parameterized) & session commands.

import { test, expect } from "cap:test";
import {
    MYSQL_CMD_QUIT,
    MYSQL_CMD_INIT_DB,
    MYSQL_CMD_QUERY,
    MYSQL_CMD_PING,
    MYSQL_FTYPE_LONG,
    MYSQL_FTYPE_VAR_STRING,
    MYSQL_QUERY_MAX_PARAMS,
    MYSQL_PARAM_UNSIGNED_FLAG,
    MYSQL_NEW_PARAMS_BOUND_FLAG,
    mysqlNullBitmapLen,
    isNullInBitmap,
    MySqlQueryParam,
    MySqlQueryPacket,
} from "cap:mysql";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("mysql QueryPacket: simple COM_QUERY encode and decode roundtrip", fn() {
    let buf: [128]u8 = [0; 128];
    const sql = "SELECT id, name FROM users WHERE active = 1";
    const q = MySqlQueryPacket.init(sql);
    expect(q.paramCount() == 0);
    expect(q.nullBitmapLen() == 0);
    expect(q.encodedPayloadLen() == 1 + sql.len);
    expect(q.encodedFrameLen() == 4 + 1 + sql.len);

    const n = q.encodeFrame(0, buf[0..]) catch unreachable;
    expect(n == q.encodedFrameLen());
    expect(buf[3] == 0);
    expect(buf[4] == MYSQL_CMD_QUERY);

    const decoded_sql = MySqlQueryPacket.decodeSimpleFrame(buf[0..n]) catch unreachable;
    expect(strEql(decoded_sql, sql));
});

test("mysql QueryPacket: parameterized COM_QUERY with NULL-bitmap, unsigned fixed binary, and varString", fn() {
    let buf: [256]u8 = [0; 256];
    const sql = "INSERT INTO items VALUES (?, ?, ?)";
    let q = MySqlQueryPacket.init(sql);

    const id_le: [4]u8 = [0x2A, 0x00, 0x00, 0x00]; // u32 = 42 LE
    const i0 = q.addParam(MySqlQueryParam.fixedBinary(MYSQL_FTYPE_LONG, true, id_le[0..])) catch unreachable;
    const i1 = q.addParam(MySqlQueryParam.nullParam()) catch unreachable;
    const i2 = q.addParam(MySqlQueryParam.varString("widget")) catch unreachable;
    expect(i0 == 0 && i1 == 1 && i2 == 2);
    expect(q.paramCount() == 3);
    expect(q.nullBitmapLen() == 1);

    const n = q.encodeFrame(0, buf[0..]) catch unreachable;
    expect(n == q.encodedFrameLen());

    // Verify wire layout after [4B hdr][0x03][sql]
    const bm_off: usize = sql.len + 5;
    const bm = buf[bm_off..(bm_off + 1)];
    expect(!isNullInBitmap(bm, 0));
    expect(isNullInBitmap(bm, 1));
    expect(!isNullInBitmap(bm, 2));

    // Verify new_params_bound_flag = 0x01
    const flag_off = bm_off + 1;
    expect(buf[flag_off] == MYSQL_NEW_PARAMS_BOUND_FLAG);

    // Verify param 0 metadata: [MYSQL_FTYPE_LONG][0x80][3][':']['p']['1']
    const m0 = flag_off + 1;
    expect(buf[m0] == MYSQL_FTYPE_LONG);
    expect(buf[m0 + 1] == MYSQL_PARAM_UNSIGNED_FLAG);
    expect(buf[m0 + 2] == 3 && buf[m0 + 3] == ':' && buf[m0 + 4] == 'p' && buf[m0 + 5] == '1');

    // Verify param 2 metadata: [MYSQL_FTYPE_VAR_STRING][0x00][3][':']['p']['3']
    const m2 = m0 + 6 + 6;
    expect(buf[m2] == MYSQL_FTYPE_VAR_STRING && buf[m2 + 1] == 0);
    expect(buf[m2 + 2] == 3 && buf[m2 + 3] == ':' && buf[m2 + 4] == 'p' && buf[m2 + 5] == '3');

    // Verify values section: 4B fixed binary (42 LE) + lenenc("widget") (1B len 6 + 6B)
    const vals_off = m2 + 6;
    expect(buf[vals_off] == 0x2A && buf[vals_off + 1] == 0 && buf[vals_off + 2] == 0 && buf[vals_off + 3] == 0);
    expect(buf[vals_off + 4] == 6);
    expect(strEql(buf[(vals_off + 5)..(vals_off + 11)], "widget"));
    expect(vals_off + 11 == n);
});

test("mysql QueryPacket: multi-byte NULL-bitmap (> 8 params) and two-digit :p10 attribute naming", fn() {
    expect(mysqlNullBitmapLen(0) == 0);
    expect(mysqlNullBitmapLen(1) == 1);
    expect(mysqlNullBitmapLen(8) == 1);
    expect(mysqlNullBitmapLen(9) == 2);
    expect(mysqlNullBitmapLen(16) == 2);
    expect(mysqlNullBitmapLen(17) == 3);

    let q = MySqlQueryPacket.init("SELECT 1");
    let i: usize = 0;
    while (i < 10) {
        if (i == 0 || i == 8 || i == 9) {
            * = q.addParam(MySqlQueryParam.nullParam()) catch unreachable;
        } else {
            * = q.addParam(MySqlQueryParam.varString("x")) catch unreachable;
        }
        i += 1;
    }
    expect(q.nullBitmapLen() == 2);

    let buf: [512]u8 = [0; 512];
    const n = q.encodeFrame(0, buf[0..]) catch unreachable;
    expect(n == q.encodedFrameLen());

    const bm_off: usize = 5 + 8;
    const bm = buf[bm_off..(bm_off + 2)];
    expect(isNullInBitmap(bm, 0));
    expect(!isNullInBitmap(bm, 1));
    expect(isNullInBitmap(bm, 8));
    expect(isNullInBitmap(bm, 9));
    expect(!isNullInBitmap(bm, 99)); // out-of-bounds safe false
});

test("mysql QueryPacket: COM_INIT_DB, COM_PING, and COM_QUIT single-command frames", fn() {
    let buf: [64]u8 = [0; 64];

    const n_db = MySqlQueryPacket.encodeInitDbFrame("analytics_db", 0, buf[0..]) catch unreachable;
    expect(n_db == 4 + 1 + 12);
    expect(buf[4] == MYSQL_CMD_INIT_DB);
    expect(strEql(buf[5..n_db], "analytics_db"));

    const n_ping = MySqlQueryPacket.encodeSingleCmdFrame(MYSQL_CMD_PING, 0, buf[0..]) catch unreachable;
    expect(n_ping == 5 && buf[0] == 1 && buf[4] == MYSQL_CMD_PING);

    const n_quit = MySqlQueryPacket.encodeSingleCmdFrame(MYSQL_CMD_QUIT, 0, buf[0..]) catch unreachable;
    expect(n_quit == 5 && buf[0] == 1 && buf[4] == MYSQL_CMD_QUIT);
});

test("mysql QueryPacket: empty query, capacity overflow, and buffer too small guards", fn() {
    let out: [64]u8 = [0; 64];

    // 1. Empty SQL rejected
    let e1 = false;
    * = MySqlQueryPacket.init("").encodeFrame(0, out[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Empty schema on COM_INIT_DB rejected
    let e2 = false;
    * = MySqlQueryPacket.encodeInitDbFrame("", 0, out[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Buffer too small on encodeFrame & encodeSingleCmdFrame
    let tiny: [4]u8 = [0; 4];
    let e3 = false;
    * = MySqlQueryPacket.init("SELECT 1").encodeFrame(0, tiny[0..]) catch { e3 = true; };
    expect(e3);

    let e4 = false;
    * = MySqlQueryPacket.encodeSingleCmdFrame(MYSQL_CMD_PING, 0, tiny[0..]) catch { e4 = true; };
    expect(e4);

    // 4. Max param capacity (> MYSQL_QUERY_MAX_PARAMS = 32)
    let full = MySqlQueryPacket.init("SELECT 1");
    let i: usize = 0;
    while (i < MYSQL_QUERY_MAX_PARAMS) {
        * = full.addParam(MySqlQueryParam.nullParam()) catch unreachable;
        i += 1;
    }
    expect(full.paramCount() == 32);
    expect(full.paramAt(31) != null);
    expect(full.paramAt(32) == null);
    let e5 = false;
    * = full.addParam(MySqlQueryParam.nullParam()) catch { e5 = true; };
    expect(e5);
});

test("mysql QueryPacket: decodeSimpleFrame malformed frame guards", fn() {
    // 1. Truncated (< 5B)
    const short_f: [4]u8 = [1, 0, 0, 0];
    let e1 = false;
    * = MySqlQueryPacket.decodeSimpleFrame(short_f[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Wrong command byte (COM_PING instead of COM_QUERY)
    const ping_f: [5]u8 = [1, 0, 0, 0, MYSQL_CMD_PING];
    let e2 = false;
    * = MySqlQueryPacket.decodeSimpleFrame(ping_f[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Empty SQL body (payload_len == 1, cmd == COM_QUERY)
    const empty_sql: [5]u8 = [1, 0, 0, 0, MYSQL_CMD_QUERY];
    let e3 = false;
    * = MySqlQueryPacket.decodeSimpleFrame(empty_sql[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Trailing bytes after declared length
    const trail_f: [7]u8 = [2, 0, 0, 0, MYSQL_CMD_QUERY, '1', 'x'];
    let e4 = false;
    * = MySqlQueryPacket.decodeSimpleFrame(trail_f[0..]) catch { e4 = true; };
    expect(e4);
});
