import { test, expect } from "cap:test";
import {
    MYSQL_STMT_MAX_COLUMNS,
    MYSQL_STMT_MAX_PARAMS,
    MySqlStmtLifecycleError,
    MySqlStmtStatus,
    MySqlStmtExecutionFlags,
    MySqlStmtColumnKeyKind,
    MySqlStmtColumnMeta,
    MySqlStmtLifecycle,
} from "cap:mysql";

test("MySqlStmtLifecycle prepare progress and immediate Prepared when zero params/columns", () => {
    let st = MySqlStmtLifecycle.init();
    expect(st.status == MySqlStmtStatus.Parsing).toBe(true);
    expect(st.isReadyForExecute()).toBe(false);

    // 1 param, 2 columns -> stays Parsing until all 3 metadata packets arrive
    expect(st.onPrepareOk(77, 2, 1)?).toBe(false);
    expect(st.statement_id).toBe(77);
    expect(st.onParamDefReceived()?).toBe(1);
    expect(st.isReadyForExecute()).toBe(false);

    expect(st.onColumnDefReceived(MySqlStmtColumnMeta.named("id", 0x03, true, 63))?).toBe(1);
    expect(st.isReadyForExecute()).toBe(false);

    expect(st.onColumnDefReceived(MySqlStmtColumnMeta.named("email", 0xFD, false, 45))?).toBe(2);
    expect(st.isReadyForExecute()).toBe(true);
    expect(st.uniqueColumnCount()).toBe(2);

    // Zero params & zero columns (e.g., DML) transitions immediately to Prepared
    let dml = MySqlStmtLifecycle.init();
    expect(dml.onPrepareOk(88, 0, 0)?).toBe(true);
    expect(dml.isReadyForExecute()).toBe(true);
});

test("MySqlStmtLifecycle execution flags mask, result rows, and resetExecution reuse", () => {
    const f = MySqlStmtExecutionFlags{
        header_received: true,
        needs_duplicate_check: false,
        need_to_send_params: true,
        columns_eof_received: true,
    };
    const mask = f.toMask();
    expect(mask).toBe(0x0D);
    const f2 = MySqlStmtExecutionFlags.fromMask(mask);
    expect(f2.header_received).toBe(true);
    expect(f2.needs_duplicate_check).toBe(false);
    expect(f2.need_to_send_params).toBe(true);
    expect(f2.columns_eof_received).toBe(true);

    let st = MySqlStmtLifecycle.init();
    * = st.onPrepareOk(99, 0, 0)?;
    * = st.onResultHeaderReceived(1)?;
    expect(st.execution_flags.header_received).toBe(true);
    * = st.onColumnDefReceived(MySqlStmtColumnMeta.named("total", 0x08, false, 63))?;
    const eof_flags = st.onColumnsEof();
    expect(st.execution_flags.columns_eof_received).toBe(true);
    expect(eof_flags.has_named_columns).toBe(true);

    expect(st.onRowReceived()).toBe(1);
    expect(st.onRowReceived()).toBe(2);
    expect(st.result_count).toBe(2);

    // resetExecution clears per-run state while preserving statement_id & Prepared status
    st.resetExecution();
    expect(st.result_count).toBe(0);
    expect(st.columns_received).toBe(0);
    expect(st.execution_flags.header_received).toBe(false);
    expect(st.statement_id).toBe(99);
    expect(st.isReadyForExecute()).toBe(true);
});

test("MySqlStmtLifecycle right-to-left duplicate named and indexed column detection", () => {
    let st = MySqlStmtLifecycle.init();
    * = st.onPrepareOk(12, 5, 0)?;
    * = st.onColumnDefReceived(MySqlStmtColumnMeta.named("id", 0x03, false, 63))?;
    * = st.onColumnDefReceived(MySqlStmtColumnMeta.indexed(0, 0x03, false, 63))?;
    * = st.onColumnDefReceived(MySqlStmtColumnMeta.named("name", 0xFD, false, 45))?;
    * = st.onColumnDefReceived(MySqlStmtColumnMeta.named("id", 0x08, true, 63))?;
    * = st.onColumnDefReceived(MySqlStmtColumnMeta.indexed(0, 0x05, false, 63))?;

    expect(st.isReadyForExecute()).toBe(true);
    expect(st.fields_flags.has_duplicate_columns).toBe(true);
    expect(st.fields_flags.has_named_columns).toBe(true);
    expect(st.fields_flags.has_indexed_columns).toBe(true);
    expect(st.uniqueColumnCount()).toBe(3);

    if (c0 := st.columnAt(0)) {
        expect(c0.isDuplicate()).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c1 := st.columnAt(1)) {
        expect(c1.isDuplicate()).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c3 := st.columnAt(3)) {
        expect(c3.key_kind == MySqlStmtColumnKeyKind.Named).toBe(true);
        expect(c3.field_type).toBe(0x08);
        expect(c3.is_unsigned).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c4 := st.columnAt(4)) {
        expect(c4.key_kind == MySqlStmtColumnKeyKind.Indexed).toBe(true);
        expect(c4.field_type).toBe(0x05);
    } else {
        expect(false).toBe(true);
    }
});

test("MySqlStmtLifecycle onError records MySQL error code and blocks further transitions", () => {
    let st = MySqlStmtLifecycle.init();
    st.onError(1064);
    expect(st.status == MySqlStmtStatus.Failed).toBe(true);
    expect(st.error_code).toBe(1064);
    expect(st.isReadyForExecute()).toBe(false);

    let got_invalid = false;
    * = st.onPrepareOk(1, 0, 0) catch |e| {
        if (e == MySqlStmtLifecycleError.InvalidStateTransition) {
            got_invalid = true;
        }
        false
    };
    expect(got_invalid).toBe(true);
});

test("MySqlStmtLifecycle enforces TooManyColumns and TooManyParams guards", () => {
    expect(MYSQL_STMT_MAX_COLUMNS).toBe(64);
    expect(MYSQL_STMT_MAX_PARAMS).toBe(32);

    let st = MySqlStmtLifecycle.init();
    let got_cols = false;
    * = st.onPrepareOk(1, 65, 0) catch |e| {
        if (e == MySqlStmtLifecycleError.TooManyColumns) {
            got_cols = true;
        }
        false
    };
    expect(got_cols).toBe(true);

    let got_params = false;
    * = st.onPrepareOk(1, 0, 33) catch |e| {
        if (e == MySqlStmtLifecycleError.TooManyParams) {
            got_params = true;
        }
        false
    };
    expect(got_params).toBe(true);

    // Extra param beyond expected_params rejected
    * = st.onPrepareOk(1, 0, 1)?;
    * = st.onParamDefReceived()?;
    let got_extra_param = false;
    * = st.onParamDefReceived() catch |e| {
        if (e == MySqlStmtLifecycleError.TooManyParams) {
            got_extra_param = true;
        }
        0
    };
    expect(got_extra_param).toBe(true);
});
