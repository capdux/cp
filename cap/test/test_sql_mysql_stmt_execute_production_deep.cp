// cap/test/test_sql_mysql_stmt_execute_production_deep.cp
// Production-deep regression tests for cap:sql mysql_stmt_execute module
// Source: /data/dev/bun/src/sql/mysql/protocol/PreparedStatement.cp & /data/dev/bun/docs/runtime/sql.mdx

import { test, expect } from "cap:test";
import {
    MYSQL_COM_STMT_EXECUTE_CMD,
    MYSQL_STMT_MAX_PARAMS,
    MYSQL_CURSOR_TYPE_NO_CURSOR,
    MYSQL_CURSOR_TYPE_READ_ONLY,
    MYSQL_PARAM_UNSIGNED_FLAG,
    MySqlStmtExecuteError,
    MySqlExecFieldType,
    MySqlExecParam,
    MySqlStmtExecuteRequest,
    mysqlNullBitmapLen,
    mysqlIsNullBitmapSet,
    encodeMySqlStmtExecutePayload,
    encodeMySqlStmtExecute,
    decodeMySqlStmtExecutePayload,
    decodeMySqlStmtExecuteFrame,
} from "cap:sql";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("sql mysql_stmt_execute: zero-parameter framed packet round-trip") {
    const req = MySqlStmtExecuteRequest{
        statement_id: 0x01020304,
        flags: MYSQL_CURSOR_TYPE_READ_ONLY,
        iteration_count: 1,
        new_params_bind_flag: true,
    };
    const empty: [0]MySqlExecParam = [];
    let wire: [32]u8 = [0; 32];
    const n = encodeMySqlStmtExecute(req, empty[0..], 0, wire[0..]) catch {
        expect(false);
        return;
    };
    // 4B header + 1B cmd + 4B stmt_id + 1B flags + 4B iter_count = 14 bytes
    expect(n == 14);
    expect(wire[0] == 10);
    expect(wire[3] == 0);
    expect(wire[4] == MYSQL_COM_STMT_EXECUTE_CMD);

    const decoded = decodeMySqlStmtExecuteFrame(wire[0..n], 0) catch {
        expect(false);
        return;
    };
    expect(decoded.statement_id == 0x01020304);
    expect(decoded.flags == MYSQL_CURSOR_TYPE_READ_ONLY);
    expect(decoded.iteration_count == 1);
    expect(decoded.param_count == 0);
}

test("sql mysql_stmt_execute: signed & unsigned integer binary round-trip") {
    const req = MySqlStmtExecuteRequest{
        statement_id: 77,
        flags: MYSQL_CURSOR_TYPE_NO_CURSOR,
        iteration_count: 1,
        new_params_bind_flag: true,
    };
    const params: [8]MySqlExecParam = [
        MySqlExecParam.tiny(-42),
        MySqlExecParam.tinyUnsigned(250),
        MySqlExecParam.short(-12345),
        MySqlExecParam.shortUnsigned(65000),
        MySqlExecParam.long(-1_000_000_000),
        MySqlExecParam.longUnsigned(4_000_000_000),
        MySqlExecParam.longLong(-9_000_000_000_000),
        MySqlExecParam.longLongUnsigned(18_000_000_000_000_000_000),
    ];

    let wire: [256]u8 = [0; 256];
    const n = encodeMySqlStmtExecute(req, params[0..], 1, wire[0..]) catch {
        expect(false);
        return;
    };
    expect(wire[3] == 1);

    const decoded = decodeMySqlStmtExecuteFrame(wire[0..n], 8) catch {
        expect(false);
        return;
    };
    expect(decoded.statement_id == 77);
    expect(decoded.new_params_bind_flag);
    expect(decoded.params[0].int_val == -42 && !decoded.params[0].is_unsigned);
    expect(decoded.params[1].uint_val == 250 && decoded.params[1].is_unsigned);
    expect(decoded.params[2].int_val == -12345 && !decoded.params[2].is_unsigned);
    expect(decoded.params[3].uint_val == 65000 && decoded.params[3].is_unsigned);
    expect(decoded.params[4].int_val == -1_000_000_000 && !decoded.params[4].is_unsigned);
    expect(decoded.params[5].uint_val == 4_000_000_000 && decoded.params[5].is_unsigned);
    expect(decoded.params[6].int_val == -9_000_000_000_000 && !decoded.params[6].is_unsigned);
    expect(decoded.params[7].uint_val == 18_000_000_000_000_000_000 && decoded.params[7].is_unsigned);
}

test("sql mysql_stmt_execute: float/double & length-encoded string/blob/json/decimal round-trip") {
    const req = MySqlStmtExecuteRequest{ statement_id: 9, iteration_count: 1, new_params_bind_flag: true };
    const bin_data: [4]u8 = [0x00, 0xff, 0x10, 0x00];
    const params: [7]MySqlExecParam = [
        MySqlExecParam.float32(3.5),
        MySqlExecParam.float64(-123.75),
        MySqlExecParam.text("hello, mysql!"),
        MySqlExecParam.text(""),
        MySqlExecParam.blob(bin_data[0..]),
        MySqlExecParam.json("{\"ok\":true}"),
        MySqlExecParam.decimal("999999999999.12345"),
    ];

    let wire: [256]u8 = [0; 256];
    const n = encodeMySqlStmtExecute(req, params[0..], 0, wire[0..]) catch {
        expect(false);
        return;
    };
    const decoded = decodeMySqlStmtExecuteFrame(wire[0..n], 7) catch {
        expect(false);
        return;
    };
    expect(decoded.params[0].float_val == 3.5);
    expect(decoded.params[1].double_val == -123.75);
    expect(strEq(decoded.params[2].bytes_val, "hello, mysql!"));
    expect(strEq(decoded.params[3].bytes_val, ""));
    expect(strEq(decoded.params[4].bytes_val, bin_data[0..]));
    expect(strEq(decoded.params[5].bytes_val, "{\"ok\":true}"));
    expect(strEq(decoded.params[6].bytes_val, "999999999999.12345"));
}

test("sql mysql_stmt_execute: multi-byte NULL bitmap packing & unpacking") {
    expect(mysqlNullBitmapLen(0) == 0);
    expect(mysqlNullBitmapLen(1) == 1);
    expect(mysqlNullBitmapLen(8) == 1);
    expect(mysqlNullBitmapLen(9) == 2);
    expect(mysqlNullBitmapLen(64) == 8);

    // 10 params: indices 0, 7, 8, 9 are NULL -> bitmap = [0x81, 0x03]
    const params: [10]MySqlExecParam = [
        MySqlExecParam.nullParam(),
        MySqlExecParam.long(1),
        MySqlExecParam.long(2),
        MySqlExecParam.long(3),
        MySqlExecParam.long(4),
        MySqlExecParam.long(5),
        MySqlExecParam.long(6),
        MySqlExecParam.nullParam(),
        MySqlExecParam.nullParam(),
        MySqlExecParam.nullParam(),
    ];
    const req = MySqlStmtExecuteRequest{ statement_id: 5, iteration_count: 1, new_params_bind_flag: true };
    let payload: [128]u8 = [0; 128];
    const n = encodeMySqlStmtExecutePayload(req, params[0..], payload[0..]) catch {
        expect(false);
        return;
    };
    // Bitmap starts at payload offset 10 (1B cmd + 4B stmt + 1B flags + 4B iter)
    expect(payload[10] == 0x81);
    expect(payload[11] == 0x03);
    expect(mysqlIsNullBitmapSet(payload[10..12], 0));
    expect(!mysqlIsNullBitmapSet(payload[10..12], 1));
    expect(mysqlIsNullBitmapSet(payload[10..12], 7));
    expect(mysqlIsNullBitmapSet(payload[10..12], 8));
    expect(mysqlIsNullBitmapSet(payload[10..12], 9));

    const decoded = decodeMySqlStmtExecutePayload(payload[0..n], 10) catch {
        expect(false);
        return;
    };
    expect(decoded.isParamNull(0));
    expect(!decoded.isParamNull(1));
    expect(decoded.params[1].int_val == 1);
    expect(decoded.params[6].int_val == 6);
    expect(decoded.isParamNull(7));
    expect(decoded.isParamNull(8));
    expect(decoded.isParamNull(9));
}

test("sql mysql_stmt_execute: new_params_bind_flag = false omits type headers") {
    const req = MySqlStmtExecuteRequest{
        statement_id: 12,
        iteration_count: 1,
        new_params_bind_flag: false,
    };
    const params: [2]MySqlExecParam = [
        MySqlExecParam.long(10),
        MySqlExecParam.long(20),
    ];
    let payload: [64]u8 = [0; 64];
    const n = encodeMySqlStmtExecutePayload(req, params[0..], payload[0..]) catch {
        expect(false);
        return;
    };
    // 10B header + 1B null_bitmap + 1B bind_flag(0) + 8B values (no 4B type headers) = 20B
    expect(n == 20);
    expect(payload[11] == 0);

    // Decoder without external schema returns MissingTypeMetadata
    let missing_meta = false;
    * = decodeMySqlStmtExecutePayload(payload[0..n], 2) catch { missing_meta = true; };
    expect(missing_meta);
}

test("sql mysql_stmt_execute: malformed wire input & capacity guards") {
    // 1. Invalid command byte (0x03 = COM_QUERY instead of 0x17)
    const bad_cmd: [10]u8 = [0x03, 1, 0, 0, 0, 0, 1, 0, 0, 0];
    let e1 = false;
    * = decodeMySqlStmtExecutePayload(bad_cmd[0..], 0) catch { e1 = true; };
    expect(e1);

    // 2. Truncated payload (< 10 bytes)
    const short_pay: [5]u8 = [0x17, 1, 0, 0, 0];
    let e2 = false;
    * = decodeMySqlStmtExecutePayload(short_pay[0..], 0) catch { e2 = true; };
    expect(e2);

    // 3. Invalid bind_flag (0x02)
    const bad_bind: [12]u8 = [0x17, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0x00, 0x02];
    let e3 = false;
    * = decodeMySqlStmtExecutePayload(bad_bind[0..], 1) catch { e3 = true; };
    expect(e3);

    // 4. Unsupported field type (0x99)
    const bad_type: [14]u8 = [0x17, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0x00, 0x01, 0x99, 0x00];
    let e4 = false;
    * = decodeMySqlStmtExecutePayload(bad_type[0..], 1) catch { e4 = true; };
    expect(e4);

    // 5. Trailing garbage byte on 0-param payload
    const trailing: [11]u8 = [0x17, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0xff];
    let e5 = false;
    * = decodeMySqlStmtExecutePayload(trailing[0..], 0) catch { e5 = true; };
    expect(e5);

    // 6. BufferTooSmall during encode
    let tiny: [8]u8 = [0; 8];
    const req = MySqlStmtExecuteRequest{ statement_id: 1, iteration_count: 1, new_params_bind_flag: true };
    const empty: [0]MySqlExecParam = [];
    let e6 = false;
    * = encodeMySqlStmtExecute(req, empty[0..], 0, tiny[0..]) catch { e6 = true; };
    expect(e6);

    // 7. TooManyParams (> 64)
    let e7 = false;
    * = decodeMySqlStmtExecutePayload(bad_cmd[0..], MYSQL_STMT_MAX_PARAMS + 1) catch { e7 = true; };
    expect(e7);
}

test("sql mysql_stmt_execute: MySqlExecFieldType classification & unsigned flag constant") {
    expect(MYSQL_PARAM_UNSIGNED_FLAG == 0x80);
    expect(MySqlExecFieldType.Tiny.isBinaryFixedWidth());
    expect(MySqlExecFieldType.Short.isBinaryFixedWidth());
    expect(MySqlExecFieldType.Long.isBinaryFixedWidth());
    expect(MySqlExecFieldType.LongLong.isBinaryFixedWidth());
    expect(MySqlExecFieldType.Float.isBinaryFixedWidth());
    expect(MySqlExecFieldType.Double.isBinaryFixedWidth());
    expect(!MySqlExecFieldType.VarString.isBinaryFixedWidth());
    expect(!MySqlExecFieldType.Blob.isBinaryFixedWidth());
    expect(!MySqlExecFieldType.Json.isBinaryFixedWidth());
    expect(!MySqlExecFieldType.NewDecimal.isBinaryFixedWidth());
    expect(!MySqlExecFieldType.Null.isBinaryFixedWidth());

    const ft = MySqlExecFieldType.fromByte(0xf5) catch { expect(false); return; };
    expect(ft == MySqlExecFieldType.Json);
}
