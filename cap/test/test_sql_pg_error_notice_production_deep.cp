// cap/test/test_sql_pg_error_notice_production_deep.cp
// Deep production tests for cap:sql PostgreSQL v3 ErrorResponse & NoticeResponse Codec (pg_error_notice.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    PG_ERROR_RESPONSE_TAG,
    PG_NOTICE_RESPONSE_TAG,
    PgDiagnosticFieldKind,
    PgDiagnosticField,
    encodePgErrorResponse,
    encodePgNoticeResponse,
    decodePgErrorNoticeIter,
    decodePgErrorOrNotice,
    decodePgErrorResponse,
    decodePgNoticeResponse,
} from "cap:sql";

test("pg_error_notice: full 18-field ErrorResponse encode/decode roundtrip", () => {
    let buf: [1024]u8 = [0; 1024];
    const fields: [18]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.severity, "ERREUR"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.localized_severity, "ERROR"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.code, "23505"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.message, "duplicate key value violates unique constraint"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.detail, "Key (email)=(a@b.com) already exists."),
        PgDiagnosticField.of(PgDiagnosticFieldKind.hint, "Use ON CONFLICT DO NOTHING."),
        PgDiagnosticField.of(PgDiagnosticFieldKind.position, "42"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.internal_position, "12"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.internal_query, "SELECT 1"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.where, "PL/pgSQL function upsert_user() line 8"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.schema, "public"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.table, "users"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.column, "email"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.datatype, "text"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.constraint, "users_email_key"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.file, "nbtinsert.c"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.line, "663"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.routine, "_bt_check_unique"),
    ];

    const wire = encodePgErrorResponse(buf[0..], fields[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(wire[0], PG_ERROR_RESPONSE_TAG);
    expect_eq(wire[wire.len - 1], 0);

    const view = decodePgErrorResponse(wire) catch {
        expect_true(false);
        return;
    };
    expect_true(view.is_error);
    expect_eq(view.field_count, 18);
    expect_eq(view.severity, "ERREUR");
    expect_eq(view.localized_severity, "ERROR");
    expect_eq(view.effectiveSeverity(), "ERROR");
    expect_eq(view.sqlstate, "23505");
    expect_eq(view.message, "duplicate key value violates unique constraint");
    expect_eq(view.detail, "Key (email)=(a@b.com) already exists.");
    expect_eq(view.hint, "Use ON CONFLICT DO NOTHING.");
    expect_eq(view.position, "42");
    expect_eq(view.internal_position, "12");
    expect_eq(view.internal_query, "SELECT 1");
    expect_eq(view.where, "PL/pgSQL function upsert_user() line 8");
    expect_eq(view.schema, "public");
    expect_eq(view.table, "users");
    expect_eq(view.column, "email");
    expect_eq(view.datatype, "text");
    expect_eq(view.constraint, "users_email_key");
    expect_eq(view.file, "nbtinsert.c");
    expect_eq(view.line, "663");
    expect_eq(view.routine, "_bt_check_unique");
    expect_true(view.isConstraintViolation());
    expect_false(view.isFatal());
    expect_false(view.isRetryableSqlState());
});

test("pg_error_notice: NoticeResponse roundtrip and strict tag separation", () => {
    let buf: [256]u8 = [0; 256];
    const fields: [3]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.severity, "NOTICE"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.code, "00000"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.message, "relation \"tmp_cache\" already exists, skipping"),
    ];
    const wire = encodePgNoticeResponse(buf[0..], fields[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(wire[0], PG_NOTICE_RESPONSE_TAG);

    const notice = decodePgNoticeResponse(wire) catch {
        expect_true(false);
        return;
    };
    expect_false(notice.is_error);
    expect_eq(notice.field_count, 3);
    expect_eq(notice.effectiveSeverity(), "NOTICE");
    expect_eq(notice.sqlstate, "00000");
    expect_eq(notice.message, "relation \"tmp_cache\" already exists, skipping");

    // Strict ErrorResponse decoder must reject NoticeResponse frame
    let rejected_notice_as_err = false;
    * = decodePgErrorResponse(wire) catch { rejected_notice_as_err = true; };
    expect_true(rejected_notice_as_err);

    // Unified decoder accepts NoticeResponse
    const unified = decodePgErrorOrNotice(wire) catch {
        expect_true(false);
        return;
    };
    expect_false(unified.is_error);
    expect_eq(unified.field_count, 3);
});

test("pg_error_notice: SQLSTATE classification (FATAL, PANIC, serialization, deadlock, connection)", () => {
    let buf: [256]u8 = [0; 256];

    // 40001 serialization_failure -> retryable, non-fatal
    const ser_fields: [2]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.localized_severity, "ERROR"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.code, "40001"),
    ];
    const ser_wire = encodePgErrorResponse(buf[0..], ser_fields[0..]) catch { expect_true(false); return; };
    const ser = decodePgErrorResponse(ser_wire) catch { expect_true(false); return; };
    expect_true(ser.isRetryableSqlState());
    expect_false(ser.isFatal());
    expect_false(ser.isConstraintViolation());

    // 40P01 deadlock_detected -> retryable
    const dl_fields: [2]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.severity, "ERROR"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.code, "40P01"),
    ];
    const dl_wire = encodePgErrorResponse(buf[0..], dl_fields[0..]) catch { expect_true(false); return; };
    const dl = decodePgErrorResponse(dl_wire) catch { expect_true(false); return; };
    expect_true(dl.isRetryableSqlState());

    // 08006 connection_failure + FATAL -> both fatal and retryable
    const conn_fields: [2]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.localized_severity, "FATAL"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.code, "08006"),
    ];
    const conn_wire = encodePgErrorResponse(buf[0..], conn_fields[0..]) catch { expect_true(false); return; };
    const conn = decodePgErrorResponse(conn_wire) catch { expect_true(false); return; };
    expect_true(conn.isFatal());
    expect_true(conn.isRetryableSqlState());

    // PANIC -> fatal
    const panic_fields: [1]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.severity, "PANIC"),
    ];
    const panic_wire = encodePgErrorResponse(buf[0..], panic_fields[0..]) catch { expect_true(false); return; };
    const pan = decodePgErrorResponse(panic_wire) catch { expect_true(false); return; };
    expect_true(pan.isFatal());
});

test("pg_error_notice: streaming iterator and unknown future field passthrough", () => {
    let buf: [256]u8 = [0; 256];
    const fields: [3]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.code, "42P01"),
        PgDiagnosticField.custom('Z', "custom_ext_payload"),
        PgDiagnosticField.of(PgDiagnosticFieldKind.message, "relation does not exist"),
    ];
    const wire = encodePgErrorResponse(buf[0..], fields[0..]) catch { expect_true(false); return; };

    let it = decodePgErrorNoticeIter(wire) catch { expect_true(false); return; };
    expect_true(it.isError());
    expect_false(it.isNotice());

    const f0 = it.next() catch { expect_true(false); return; };
    const f1 = it.next() catch { expect_true(false); return; };
    const f2 = it.next() catch { expect_true(false); return; };
    const f3 = it.next() catch { expect_true(false); return; };
    expect_true(f0 != null);
    expect_true(f1 != null);
    expect_true(f2 != null);
    expect_true(f3 == null);
    if (x0 := f0) {
        expect_eq(x0.raw_tag, 'C');
        expect_eq(x0.value, "42P01");
    }
    if (x1 := f1) {
        expect_eq(x1.raw_tag, 'Z');
        expect_eq(x1.kind.codeByte(), 0);
        expect_eq(x1.value, "custom_ext_payload");
    }
    if (x2 := f2) {
        expect_eq(x2.raw_tag, 'M');
        expect_eq(x2.value, "relation does not exist");
    }
});

test("pg_error_notice: empty diagnostic field list roundtrip", () => {
    let buf: [32]u8 = [0; 32];
    const empty: [0]PgDiagnosticField = [];
    const wire = encodePgNoticeResponse(buf[0..], empty[0..]) catch { expect_true(false); return; };
    // 'N' + 4B len(5) + 0x00 terminator = 6 bytes
    expect_eq(wire.len, 6);
    const view = decodePgNoticeResponse(wire) catch { expect_true(false); return; };
    expect_eq(view.field_count, 0);
    expect_eq(view.message, "");
});

test("pg_error_notice: malformed frames and boundary error guards", () => {
    // 1. BufferTooSmall on encoder
    let tiny: [4]u8 = [0; 4];
    const fields: [1]PgDiagnosticField = [
        PgDiagnosticField.of(PgDiagnosticFieldKind.message, "boom"),
    ];
    let failed_small = false;
    * = encodePgErrorResponse(tiny[0..], fields[0..]) catch { failed_small = true; };
    expect_true(failed_small);

    // 2. Invalid zero tag field rejected on encode
    let buf: [64]u8 = [0; 64];
    const bad_field: [1]PgDiagnosticField = [
        PgDiagnosticField{ kind: PgDiagnosticFieldKind.unknown, raw_tag: 0, value: "x" },
    ];
    let failed_zero_tag = false;
    * = encodePgErrorResponse(buf[0..], bad_field[0..]) catch { failed_zero_tag = true; };
    expect_true(failed_zero_tag);

    // 3. Wrong frame tag ('Z' instead of 'E'/'N')
    const wrong_tag: [6]u8 = ['Z', 0, 0, 0, 5, 0];
    let failed_tag = false;
    * = decodePgErrorOrNotice(wrong_tag[0..]) catch { failed_tag = true; };
    expect_true(failed_tag);

    // 4. Empty payload (missing 0x00 field list terminator)
    const empty_payload: [5]u8 = ['E', 0, 0, 0, 4];
    let failed_empty = false;
    * = decodePgErrorResponse(empty_payload[0..]) catch { failed_empty = true; };
    expect_true(failed_empty);

    // 5. Missing NUL terminator on field value
    const no_nul: [8]u8 = ['E', 0, 0, 0, 7, 'M', 'o', 'k'];
    let failed_nul = false;
    * = decodePgErrorResponse(no_nul[0..]) catch { failed_nul = true; };
    expect_true(failed_nul);

    // 6. Trailing garbage bytes inside payload after 0x00 terminator
    const trailing_inner: [10]u8 = ['E', 0, 0, 0, 9, 'M', 'a', 0, 0, 0xff];
    let failed_inner = false;
    * = decodePgErrorResponse(trailing_inner[0..]) catch { failed_inner = true; };
    expect_true(failed_inner);

    // 7. Trailing garbage bytes after outer frame
    const trailing_outer: [7]u8 = ['N', 0, 0, 0, 5, 0, 0xaa];
    let failed_outer = false;
    * = decodePgNoticeResponse(trailing_outer[0..]) catch { failed_outer = true; };
    expect_true(failed_outer);

    // 8. Truncated frame length
    const trunc: [6]u8 = ['E', 0, 0, 0, 20, 'M'];
    let failed_trunc = false;
    * = decodePgErrorResponse(trunc[0..]) catch { failed_trunc = true; };
    expect_true(failed_trunc);
});
