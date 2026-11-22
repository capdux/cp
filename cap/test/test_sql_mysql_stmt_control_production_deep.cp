// cap/test/test_sql_mysql_stmt_control_production_deep.cp
// Deep production tests for cap:sql MySQL Command Header, COM_STMT_PREPARE_OK & ResultSetHeader Codec (mysql_stmt_control.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    MYSQL_STMT_PREPARE_OK_STATUS,
    MYSQL_STMT_PREPARE_OK_MIN_LEN,
    MYSQL_STMT_PREPARE_OK_STD_LEN,
    MySqlWireCmd,
    MySqlWirePacketHeader,
    MySqlStmtPrepareOkView,
    encodeMySqlStmtPrepareOk,
    decodeMySqlStmtPrepareOk,
    encodeMySqlResultSetHeader,
    decodeMySqlResultSetHeader,
    encodeMySqlSimpleCommand,
    decodeMySqlSimpleCommand,
    encodeMySqlStmtIdCommand,
    decodeMySqlStmtIdCommand,
} from "cap:sql";

test("mysql_stmt_control: MySqlWireCmd 31-command lookup and classification", () => {
    expect_eq(MYSQL_STMT_PREPARE_OK_STATUS, 0);
    expect_eq(MYSQL_STMT_PREPARE_OK_MIN_LEN, 10);
    expect_eq(MYSQL_STMT_PREPARE_OK_STD_LEN, 12);

    const c_prep = MySqlWireCmd.fromByte(0x16) catch { expect_true(false); return; };
    expect_eq(c_prep.toByte(), 0x16);
    expect_true(c_prep.isPreparedCommand());
    expect_false(c_prep.isConnectionControl());

    const c_exec = MySqlWireCmd.fromByte(0x17) catch { expect_true(false); return; };
    expect_true(c_exec.isPreparedCommand());

    const c_ping = MySqlWireCmd.fromByte(0x0e) catch { expect_true(false); return; };
    expect_true(c_ping.isConnectionControl());
    expect_false(c_ping.isPreparedCommand());

    const c_reset = MySqlWireCmd.fromByte(0x1f) catch { expect_true(false); return; };
    expect_true(c_reset.isConnectionControl());

    let unknown = false;
    * = MySqlWireCmd.fromByte(0x00) catch { unknown = true; };
    expect_true(unknown);

    let unknown2 = false;
    * = MySqlWireCmd.fromByte(0x20) catch { unknown2 = true; };
    expect_true(unknown2);
});

test("mysql_stmt_control: MySqlWirePacketHeader 24-bit LE roundtrip & continuation chunk", () => {
    const hdr = MySqlWirePacketHeader{ length: 0x123456, sequence_id: 7 };
    const raw = hdr.encode();
    expect_eq(raw[0], 0x56);
    expect_eq(raw[1], 0x34);
    expect_eq(raw[2], 0x12);
    expect_eq(raw[3], 7);
    expect_false(hdr.isContinuationChunk());

    const parsed = MySqlWirePacketHeader.decode(raw[0..]);
    expect_true(parsed != null);
    if (p := parsed) {
        expect_eq(p.length, 0x123456);
        expect_eq(p.sequence_id, 7);
    }

    const max_hdr = MySqlWirePacketHeader{ length: 0xffffff, sequence_id: 0 };
    expect_true(max_hdr.isContinuationChunk());

    const short_bytes: [3]u8 = [1, 2, 3];
    expect_true(MySqlWirePacketHeader.decode(short_bytes[0..]) == null);
});

test("mysql_stmt_control: COM_STMT_PREPARE_OK 12B standard & 10B legacy roundtrip", () => {
    let buf: [64]u8 = [0; 64];
    const wire12 = encodeMySqlStmtPrepareOk(buf[0..], MySqlStmtPrepareOkView{
        sequence_id: 1,
        statement_id: 1024,
        num_columns: 5,
        num_params: 3,
        warning_count: 2,
        has_warning_count: true,
    }) catch { expect_true(false); return; };
    // 4B header + 12B payload = 16 bytes
    expect_eq(wire12.len, 16);
    expect_eq(wire12[0], 12);
    expect_eq(wire12[3], 1);

    const dec12 = decodeMySqlStmtPrepareOk(wire12) catch { expect_true(false); return; };
    expect_eq(dec12.sequence_id, 1);
    expect_eq(dec12.statement_id, 1024);
    expect_eq(dec12.num_columns, 5);
    expect_eq(dec12.num_params, 3);
    expect_eq(dec12.warning_count, 2);
    expect_true(dec12.has_warning_count);

    // 10-byte payload without warning_count
    const wire10 = encodeMySqlStmtPrepareOk(buf[0..], MySqlStmtPrepareOkView{
        sequence_id: 2,
        statement_id: 77,
        num_columns: 1,
        num_params: 0,
        warning_count: 0,
        has_warning_count: false,
    }) catch { expect_true(false); return; };
    expect_eq(wire10.len, 14);
    const dec10 = decodeMySqlStmtPrepareOk(wire10) catch { expect_true(false); return; };
    expect_eq(dec10.statement_id, 77);
    expect_eq(dec10.num_columns, 1);
    expect_eq(dec10.num_params, 0);
    expect_eq(dec10.warning_count, 0);
    expect_false(dec10.has_warning_count);
});

test("mysql_stmt_control: ResultSetHeader standard & optional metadata flag roundtrip", () => {
    let buf: [64]u8 = [0; 64];
    const w1 = encodeMySqlResultSetHeader(buf[0..], 1, 12, false, true) catch {
        expect_true(false);
        return;
    };
    expect_eq(w1.len, 5); // 4B header + 1B LenEnc(12)
    const r1 = decodeMySqlResultSetHeader(w1) catch { expect_true(false); return; };
    expect_eq(r1.sequence_id, 1);
    expect_eq(r1.field_count, 12);
    expect_true(r1.metadata_follows);

    // Large column count (300 -> 3-byte LenEnc 0xFC + u16 LE) with metadata_follows = false
    const w2 = encodeMySqlResultSetHeader(buf[0..], 3, 300, true, false) catch {
        expect_true(false);
        return;
    };
    expect_eq(w2.len, 8); // 4B header + 3B LenEnc(300) + 1B flag(0)
    const r2 = decodeMySqlResultSetHeader(w2) catch { expect_true(false); return; };
    expect_eq(r2.sequence_id, 3);
    expect_eq(r2.field_count, 300);
    expect_false(r2.metadata_follows);
});

test("mysql_stmt_control: simple commands & statement_id commands roundtrip", () => {
    let buf: [128]u8 = [0; 128];

    const w_ping = encodeMySqlSimpleCommand(buf[0..], 0, MySqlWireCmd.com_ping, "") catch {
        expect_true(false);
        return;
    };
    expect_eq(w_ping.len, 5);
    const r_ping = decodeMySqlSimpleCommand(w_ping) catch { expect_true(false); return; };
    expect_eq(r_ping.command.toByte(), 0x0e);
    expect_eq(r_ping.arg, "");

    const w_db = encodeMySqlSimpleCommand(buf[0..], 0, MySqlWireCmd.com_init_db, "analytics_prod") catch {
        expect_true(false);
        return;
    };
    const r_db = decodeMySqlSimpleCommand(w_db) catch { expect_true(false); return; };
    expect_eq(r_db.command.toByte(), 0x02);
    expect_eq(r_db.arg, "analytics_prod");

    const w_close = encodeMySqlStmtIdCommand(buf[0..], 0, MySqlWireCmd.com_stmt_close, 0x00abcdef) catch {
        expect_true(false);
        return;
    };
    expect_eq(w_close.len, 9);
    const stmt_id = decodeMySqlStmtIdCommand(w_close, MySqlWireCmd.com_stmt_close) catch {
        expect_true(false);
        return;
    };
    expect_eq(stmt_id, 0x00abcdef);
});

test("mysql_stmt_control: malformed packets and boundary error guards", () => {
    let buf: [32]u8 = [0; 32];

    // 1. BufferTooSmall
    let tiny: [3]u8 = [0; 3];
    let f1 = false;
    * = encodeMySqlStmtPrepareOk(tiny[0..], MySqlStmtPrepareOkView{}) catch { f1 = true; };
    expect_true(f1);

    // 2. Non-zero status in COM_STMT_PREPARE_OK rejected
    const bad_status: [16]u8 = [12, 0, 0, 1, 0xff, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0];
    let f2 = false;
    * = decodeMySqlStmtPrepareOk(bad_status[0..]) catch { f2 = true; };
    expect_true(f2);

    // 3. Truncated COM_STMT_PREPARE_OK (< 10B payload) rejected
    const short_prep: [12]u8 = [8, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0];
    let f3 = false;
    * = decodeMySqlStmtPrepareOk(short_prep[0..]) catch { f3 = true; };
    expect_true(f3);

    // 4. ResultSetHeader with field_count = 0 rejected on encode & decode
    let f4 = false;
    * = encodeMySqlResultSetHeader(buf[0..], 1, 0, false, true) catch { f4 = true; };
    expect_true(f4);

    const zero_rs: [5]u8 = [1, 0, 0, 1, 0];
    let f5 = false;
    * = decodeMySqlResultSetHeader(zero_rs[0..]) catch { f5 = true; };
    expect_true(f5);

    // 5. Invalid metadata_follows flag byte (2) rejected
    const bad_meta_flag: [6]u8 = [2, 0, 0, 1, 3, 2];
    let f6 = false;
    * = decodeMySqlResultSetHeader(bad_meta_flag[0..]) catch { f6 = true; };
    expect_true(f6);

    // 6. Wrong command on decodeMySqlStmtIdCommand
    const w_reset = encodeMySqlStmtIdCommand(buf[0..], 0, MySqlWireCmd.com_stmt_reset, 5) catch {
        expect_true(false);
        return;
    };
    let f7 = false;
    * = decodeMySqlStmtIdCommand(w_reset, MySqlWireCmd.com_stmt_close) catch { f7 = true; };
    expect_true(f7);

    // 7. Trailing bytes rejected
    const trailing: [6]u8 = [1, 0, 0, 0, 0x0e, 0xff];
    let f8 = false;
    * = decodeMySqlSimpleCommand(trailing[0..]) catch { f8 = true; };
    expect_true(f8);
});
