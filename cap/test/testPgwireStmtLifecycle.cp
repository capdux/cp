import { test, expect } from "cap:test";
import {
    PG_STMT_MAX_FIELDS,
    PG_STMT_MAX_PARAMS,
    PgStmtLifecycleError,
    PgStmtStatus,
    PgStmtColumnKeyKind,
    PgStmtColumnMeta,
    PgStmtLifecycle,
} from "cap:pgwire";

test("PgStmtLifecycle full Pending -> Parsing -> Prepared transition with param OIDs", () => {
    let st = PgStmtLifecycle.init();
    expect(st.status == PgStmtStatus.Pending).toBe(true);
    expect(st.status.isRunning()).toBe(false);
    expect(st.status.isReady()).toBe(false);

    expect(st.beginParse()?).toBe(true);
    expect(st.status.isRunning()).toBe(true);

    const oids: [2]i32 = [23, 25];
    expect(st.setParamOids(oids[0..])?).toBe(2);
    expect(st.paramOidsSlice().len).toBe(2);
    expect(st.paramOidsSlice()[0]).toBe(23);
    expect(st.paramOidsSlice()[1]).toBe(25);

    * = st.addField(PgStmtColumnMeta.named("id", 23))?;
    * = st.addField(PgStmtColumnMeta.named("email", 25))?;

    expect(st.markPrepared()?).toBe(true);
    expect(st.status.isReady()).toBe(true);
    expect(st.fields_flags.has_named_columns).toBe(true);
    expect(st.fields_flags.has_indexed_columns).toBe(false);
    expect(st.fields_flags.has_duplicate_columns).toBe(false);
    expect(st.uniqueFieldCount()).toBe(2);
});

test("PgStmtLifecycle right-to-left duplicate named column detection keeps rightmost column", () => {
    let st = PgStmtLifecycle.init();
    * = st.beginParse()?;
    * = st.addField(PgStmtColumnMeta.named("id", 23))?;    // col 0 -> Duplicate
    * = st.addField(PgStmtColumnMeta.named("name", 25))?;  // col 1 -> Named
    * = st.addField(PgStmtColumnMeta.named("id", 20))?;    // col 2 -> Duplicate
    * = st.addField(PgStmtColumnMeta.named("id", 1700))?;  // col 3 -> Named (rightmost wins)

    * = st.markPrepared()?;
    expect(st.fields_flags.has_duplicate_columns).toBe(true);
    expect(st.uniqueFieldCount()).toBe(2);

    if (c0 := st.fieldAt(0)) {
        expect(c0.isDuplicate()).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c1 := st.fieldAt(1)) {
        expect(c1.key_kind == PgStmtColumnKeyKind.Named).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c2 := st.fieldAt(2)) {
        expect(c2.isDuplicate()).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c3 := st.fieldAt(3)) {
        expect(c3.key_kind == PgStmtColumnKeyKind.Named).toBe(true);
        expect(c3.type_oid).toBe(1700);
    } else {
        expect(false).toBe(true);
    }
    expect(st.fieldAt(4) == null).toBe(true);
});

test("PgStmtLifecycle mixed named and indexed columns deduplication and idempotency", () => {
    let st = PgStmtLifecycle.init();
    * = st.addField(PgStmtColumnMeta.indexed(0, 23))?;
    * = st.addField(PgStmtColumnMeta.named("label", 25))?;
    * = st.addField(PgStmtColumnMeta.indexed(1, 20))?;
    * = st.addField(PgStmtColumnMeta.indexed(0, 701))?;

    const f1 = st.checkForDuplicateFields();
    expect(f1.has_named_columns).toBe(true);
    expect(f1.has_indexed_columns).toBe(true);
    expect(f1.has_duplicate_columns).toBe(true);
    expect(st.uniqueFieldCount()).toBe(3);

    // Second call is idempotent and returns cached flags
    const f2 = st.checkForDuplicateFields();
    expect(f2.has_duplicate_columns).toBe(true);
    if (c0 := st.fieldAt(0)) {
        expect(c0.isDuplicate()).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    if (c3 := st.fieldAt(3)) {
        expect(c3.key_kind == PgStmtColumnKeyKind.Indexed).toBe(true);
        expect(c3.type_oid).toBe(701);
    } else {
        expect(false).toBe(true);
    }
});

test("PgStmtLifecycle markFailed SQLSTATE tracking and reset", () => {
    let st = PgStmtLifecycle.init();
    * = st.beginParse()?;
    st.markFailed("42601");
    expect(st.status.isFailed()).toBe(true);
    if (code := st.errorSqlState()) {
        expect(code).toBe("42601");
    } else {
        expect(false).toBe(true);
    }

    // Short / empty sqlstate clears error_code
    st.markFailed("");
    expect(st.errorSqlState() == null).toBe(true);

    // reset restores Pending state and clears fields/params
    * = st.addField(PgStmtColumnMeta.named("x", 23))?;
    st.reset();
    expect(st.status == PgStmtStatus.Pending).toBe(true);
    expect(st.field_count).toBe(0);
    expect(st.param_count).toBe(0);
    expect(st.errorSqlState() == null).toBe(true);
});

test("PgStmtLifecycle enforces InvalidStateTransition, TooManyParams, and TooManyFields guards", () => {
    expect(PG_STMT_MAX_FIELDS).toBe(64);
    expect(PG_STMT_MAX_PARAMS).toBe(32);

    let st = PgStmtLifecycle.init();

    // markPrepared from Pending fails
    let got_bad_prep = false;
    * = st.markPrepared() catch |e| {
        if (e == PgStmtLifecycleError.InvalidStateTransition) {
            got_bad_prep = true;
        }
        false
    };
    expect(got_bad_prep).toBe(true);

    // Duplicate beginParse fails
    * = st.beginParse()?;
    let got_dup_parse = false;
    * = st.beginParse() catch |e| {
        if (e == PgStmtLifecycleError.InvalidStateTransition) {
            got_dup_parse = true;
        }
        false
    };
    expect(got_dup_parse).toBe(true);

    // 33 param OIDs -> TooManyParams
    let too_many_params: [33]i32 = [23; 33];
    let got_too_many_params = false;
    * = st.setParamOids(too_many_params[0..]) catch |e| {
        if (e == PgStmtLifecycleError.TooManyParams) {
            got_too_many_params = true;
        }
        0
    };
    expect(got_too_many_params).toBe(true);

    // 65 fields -> TooManyFields
    let i: usize = 0;
    while (i < PG_STMT_MAX_FIELDS) {
        * = st.addField(PgStmtColumnMeta.indexed(@as(u32, @intCast(i)), 23))?;
        i += 1;
    }
    let got_too_many_fields = false;
    * = st.addField(PgStmtColumnMeta.named("overflow", 23)) catch |e| {
        if (e == PgStmtLifecycleError.TooManyFields) {
            got_too_many_fields = true;
        }
        0
    };
    expect(got_too_many_fields).toBe(true);
});
