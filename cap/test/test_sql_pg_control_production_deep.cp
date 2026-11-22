// cap/test/test_sql_pg_control_production_deep.cp
// Deep production tests for cap:sql PostgreSQL v3 Backend Control Frame Codec (pg_control.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    PG_READY_FOR_QUERY_TAG,
    PG_BACKEND_KEY_DATA_TAG,
    PG_COMMAND_COMPLETE_TAG,
    PG_PARAMETER_STATUS_TAG,
    PG_BACKEND_KEY_DATA_FRAME_LEN,
    PgTxStatus,
    PgBackendKeyData,
    encodePgReadyForQuery,
    decodePgReadyForQuery,
    encodePgBackendKeyData,
    decodePgBackendKeyData,
    encodePgCommandComplete,
    decodePgCommandComplete,
    encodePgParameterStatus,
    decodePgParameterStatus,
} from "cap:sql";

test("pg_control: ReadyForQuery all 3 transaction status states roundtrip", () => {
    let buf: [32]u8 = [0; 32];

    const w_idle = encodePgReadyForQuery(buf[0..], PgTxStatus.idle) catch { expect_true(false); return; };
    expect_eq(w_idle.len, 6);
    expect_eq(w_idle[0], PG_READY_FOR_QUERY_TAG);
    expect_eq(w_idle[4], 5);
    expect_eq(w_idle[5], 'I');
    const s_idle = decodePgReadyForQuery(w_idle) catch { expect_true(false); return; };
    expect_true(s_idle.isIdle());
    expect_false(s_idle.inTransaction());
    expect_false(s_idle.isFailed());

    const w_tx = encodePgReadyForQuery(buf[0..], PgTxStatus.in_transaction) catch { expect_true(false); return; };
    const s_tx = decodePgReadyForQuery(w_tx) catch { expect_true(false); return; };
    expect_true(s_tx.inTransaction());
    expect_eq(s_tx.toByte(), 'T');

    const w_fail = encodePgReadyForQuery(buf[0..], PgTxStatus.failed_transaction) catch { expect_true(false); return; };
    const s_fail = decodePgReadyForQuery(w_fail) catch { expect_true(false); return; };
    expect_true(s_fail.isFailed());
    expect_eq(s_fail.toByte(), 'E');
});

test("pg_control: BackendKeyData 13-byte fixed frame big-endian roundtrip", () => {
    expect_eq(PG_BACKEND_KEY_DATA_FRAME_LEN, 12);
    let buf: [32]u8 = [0; 32];
    const key = PgBackendKeyData{
        process_id: 0x01020304,
        secret_key: 0xdeadbeef,
    };
    const wire = encodePgBackendKeyData(buf[0..], key) catch { expect_true(false); return; };
    expect_eq(wire.len, 13);
    expect_eq(wire[0], PG_BACKEND_KEY_DATA_TAG);
    expect_eq(wire[4], 12);
    expect_eq(wire[5], 0x01);
    expect_eq(wire[8], 0x04);
    expect_eq(wire[9], 0xde);
    expect_eq(wire[12], 0xef);

    const decoded_key = decodePgBackendKeyData(wire) catch { expect_true(false); return; };
    expect_eq(decoded_key.process_id, 0x01020304);
    expect_eq(decoded_key.secret_key, 0xdeadbeef);
});

test("pg_control: CommandComplete roundtrip and rowsAffected parsing", () => {
    let buf: [128]u8 = [0; 128];

    const w_ins = encodePgCommandComplete(buf[0..], "INSERT 0 42") catch { expect_true(false); return; };
    expect_eq(w_ins[0], PG_COMMAND_COMPLETE_TAG);
    const c_ins = decodePgCommandComplete(w_ins) catch { expect_true(false); return; };
    expect_eq(c_ins.tag, "INSERT 0 42");
    const r_ins = c_ins.rowsAffected();
    expect_true(r_ins != null);
    if (n := r_ins) { expect_eq(n, 42); }

    const w_upd = encodePgCommandComplete(buf[0..], "UPDATE 7") catch { expect_true(false); return; };
    const c_upd = decodePgCommandComplete(w_upd) catch { expect_true(false); return; };
    if (n := c_upd.rowsAffected()) { expect_eq(n, 7); } else { expect_true(false); }

    const w_zero = encodePgCommandComplete(buf[0..], "SELECT 0") catch { expect_true(false); return; };
    const c_zero = decodePgCommandComplete(w_zero) catch { expect_true(false); return; };
    if (n := c_zero.rowsAffected()) { expect_eq(n, 0); } else { expect_true(false); }

    // Non-row DDL/TCL tags return null
    const w_ddl = encodePgCommandComplete(buf[0..], "CREATE TABLE") catch { expect_true(false); return; };
    const c_ddl = decodePgCommandComplete(w_ddl) catch { expect_true(false); return; };
    expect_eq(c_ddl.tag, "CREATE TABLE");
    expect_true(c_ddl.rowsAffected() == null);

    const w_begin = encodePgCommandComplete(buf[0..], "BEGIN") catch { expect_true(false); return; };
    const c_begin = decodePgCommandComplete(w_begin) catch { expect_true(false); return; };
    expect_true(c_begin.rowsAffected() == null);
});

test("pg_control: ParameterStatus runtime config notification roundtrip", () => {
    let buf: [128]u8 = [0; 128];
    const wire = encodePgParameterStatus(buf[0..], "server_encoding", "UTF8") catch {
        expect_true(false);
        return;
    };
    expect_eq(wire[0], PG_PARAMETER_STATUS_TAG);
    const ps = decodePgParameterStatus(wire) catch { expect_true(false); return; };
    expect_eq(ps.name, "server_encoding");
    expect_eq(ps.value, "UTF8");

    // Empty value is valid (e.g. TimeZone or search_path edge case)
    const wire2 = encodePgParameterStatus(buf[0..], "application_name", "") catch {
        expect_true(false);
        return;
    };
    const ps2 = decodePgParameterStatus(wire2) catch { expect_true(false); return; };
    expect_eq(ps2.name, "application_name");
    expect_eq(ps2.value, "");
});

test("pg_control: malformed frames and boundary error guards", () => {
    // 1. BufferTooSmall
    let tiny: [4]u8 = [0; 4];
    let f1 = false;
    * = encodePgReadyForQuery(tiny[0..], PgTxStatus.idle) catch { f1 = true; };
    expect_true(f1);

    let f2 = false;
    * = encodePgBackendKeyData(tiny[0..], PgBackendKeyData{}) catch { f2 = true; };
    expect_true(f2);

    // 2. Invalid TxStatus byte ('X')
    const bad_rfq: [6]u8 = ['Z', 0, 0, 0, 5, 'X'];
    let f3 = false;
    * = decodePgReadyForQuery(bad_rfq[0..]) catch { f3 = true; };
    expect_true(f3);

    // 3. Wrong BackendKeyData payload length (4 instead of 8)
    const bad_key_len: [9]u8 = ['K', 0, 0, 0, 8, 0, 0, 0, 1];
    let f4 = false;
    * = decodePgBackendKeyData(bad_key_len[0..]) catch { f4 = true; };
    expect_true(f4);

    // 4. Tag mismatch
    const wrong_tag: [6]u8 = ['Q', 0, 0, 0, 5, 'I'];
    let f5 = false;
    * = decodePgReadyForQuery(wrong_tag[0..]) catch { f5 = true; };
    expect_true(f5);

    // 5. Missing NUL terminator in CommandComplete & ParameterStatus
    const no_nul_cc: [8]u8 = ['C', 0, 0, 0, 7, 'O', 'K', '!'];
    let f6 = false;
    * = decodePgCommandComplete(no_nul_cc[0..]) catch { f6 = true; };
    expect_true(f6);

    const no_val_ps: [8]u8 = ['S', 0, 0, 0, 7, 'k', 0, 'v'];
    let f7 = false;
    * = decodePgParameterStatus(no_val_ps[0..]) catch { f7 = true; };
    expect_true(f7);

    // 6. Trailing bytes after valid frame
    const trailing_rfq: [7]u8 = ['Z', 0, 0, 0, 5, 'I', 0xaa];
    let f8 = false;
    * = decodePgReadyForQuery(trailing_rfq[0..]) catch { f8 = true; };
    expect_true(f8);
});
