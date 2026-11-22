// Production Deep Tests for cap:pgwire FieldMessage (lib/pgwire/field_message.cp)
import { test, expect_eq } from "cap:test";
import {
    PG_MAX_DIAGNOSTIC_FIELDS,
    PgFieldMessageError,
    PgDiagnosticFieldType,
    parseDiagnosticFieldType,
    diagnosticFieldName,
    PgDiagnosticField,
    PgDiagnosticFields,
} from "cap:pgwire";

test("pgwire field_message: roundtrip encodePayload / decodePayload across rich diagnostic fields", fn() {
    expect_eq(PG_MAX_DIAGNOSTIC_FIELDS, @as(usize, 24));

    let f = PgDiagnosticFields.init();
    * = f.add(PgDiagnosticFieldType.Severity, "FEHLER") catch unreachable;
    * = f.add(PgDiagnosticFieldType.LocalizedSeverity, "ERROR") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Code, "23505") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Message, "duplicate key value") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Detail, "Key (email)=(a@b.c) already exists.") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Hint, "Use ON CONFLICT DO NOTHING") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Schema, "public") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Table, "users") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Column, "email") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Constraint, "users_email_key") catch unreachable;
    * = f.add(PgDiagnosticFieldType.File, "nbtinsert.c") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Line, "664") catch unreachable;
    * = f.add(PgDiagnosticFieldType.Routine, "_bt_check_unique") catch unreachable;

    expect_eq(f.count(), @as(usize, 13));
    // severity() prefers non-localized 'V' over localized 'S'
    expect_eq(f.severity() ?? "", "ERROR");
    expect_eq(f.code() ?? "", "23505");
    expect_eq(f.sqlstateClass() ?? "", "23");
    expect_eq(f.isRetryable(), false);
    expect_eq(f.message() ?? "", "duplicate key value");
    expect_eq(f.detail() ?? "", "Key (email)=(a@b.c) already exists.");
    expect_eq(f.hint() ?? "", "Use ON CONFLICT DO NOTHING");
    expect_eq(f.schema() ?? "", "public");
    expect_eq(f.table() ?? "", "users");
    expect_eq(f.column() ?? "", "email");
    expect_eq(f.constraint() ?? "", "users_email_key");

    let buf: [512]u8 = [0; 512];
    const n = f.encodePayload(buf[0..]) catch unreachable;
    expect_eq(n, f.encodedLen());
    expect_eq(buf[n - 1], @as(u8, 0));

    const decoded = PgDiagnosticFields.decodePayloadStrict(buf[0..n]) catch unreachable;
    expect_eq(decoded.eql(f), true);
    expect_eq(decoded.get(PgDiagnosticFieldType.Routine) ?? "", "_bt_check_unique");
    expect_eq(decoded.at(13) == null, true);
});

test("pgwire field_message: all 18 field type codes and names mapping", fn() {
    expect_eq(parseDiagnosticFieldType('S') == PgDiagnosticFieldType.Severity, true);
    expect_eq(parseDiagnosticFieldType('V') == PgDiagnosticFieldType.LocalizedSeverity, true);
    expect_eq(parseDiagnosticFieldType('C') == PgDiagnosticFieldType.Code, true);
    expect_eq(parseDiagnosticFieldType('M') == PgDiagnosticFieldType.Message, true);
    expect_eq(parseDiagnosticFieldType('D') == PgDiagnosticFieldType.Detail, true);
    expect_eq(parseDiagnosticFieldType('H') == PgDiagnosticFieldType.Hint, true);
    expect_eq(parseDiagnosticFieldType('P') == PgDiagnosticFieldType.Position, true);
    expect_eq(parseDiagnosticFieldType('p') == PgDiagnosticFieldType.InternalPosition, true);
    expect_eq(parseDiagnosticFieldType('q') == PgDiagnosticFieldType.InternalQuery, true);
    expect_eq(parseDiagnosticFieldType('W') == PgDiagnosticFieldType.Where, true);
    expect_eq(parseDiagnosticFieldType('s') == PgDiagnosticFieldType.Schema, true);
    expect_eq(parseDiagnosticFieldType('t') == PgDiagnosticFieldType.Table, true);
    expect_eq(parseDiagnosticFieldType('c') == PgDiagnosticFieldType.Column, true);
    expect_eq(parseDiagnosticFieldType('d') == PgDiagnosticFieldType.DataType, true);
    expect_eq(parseDiagnosticFieldType('n') == PgDiagnosticFieldType.Constraint, true);
    expect_eq(parseDiagnosticFieldType('F') == PgDiagnosticFieldType.File, true);
    expect_eq(parseDiagnosticFieldType('L') == PgDiagnosticFieldType.Line, true);
    expect_eq(parseDiagnosticFieldType('R') == PgDiagnosticFieldType.Routine, true);
    expect_eq(parseDiagnosticFieldType('Z') == null, true);

    expect_eq(diagnosticFieldName(PgDiagnosticFieldType.InternalQuery), "internal_query");
    expect_eq(diagnosticFieldName(PgDiagnosticFieldType.Constraint), "constraint");
    expect_eq(diagnosticFieldName(PgDiagnosticFieldType.Routine), "routine");
});

test("pgwire field_message: retryable SQLSTATE detection (40001 / 40P01)", fn() {
    let f_ser = PgDiagnosticFields.init();
    * = f_ser.add(PgDiagnosticFieldType.Code, "40001") catch unreachable;
    expect_eq(f_ser.isRetryable(), true);
    expect_eq(f_ser.sqlstateClass() ?? "", "40");

    let f_dl = PgDiagnosticFields.init();
    * = f_dl.add(PgDiagnosticFieldType.Code, "40P01") catch unreachable;
    expect_eq(f_dl.isRetryable(), true);

    let f_syn = PgDiagnosticFields.init();
    * = f_syn.add(PgDiagnosticFieldType.Code, "42601") catch unreachable;
    expect_eq(f_syn.isRetryable(), false);
    expect_eq(f_syn.sqlstateClass() ?? "", "42");
});

test("pgwire field_message: unknown field skipping vs strict rejection", fn() {
    // 'Z' is unknown extension field, followed by 'M' "ok" and terminator 0x00
    const raw: [10]u8 = ['Z', 'e', 'x', 't', 0, 'M', 'o', 'k', 0, 0];

    // Standard decode skips unknown field cleanly
    const lenient = PgDiagnosticFields.decodePayload(raw[0..]) catch unreachable;
    expect_eq(lenient.count(), @as(usize, 1));
    expect_eq(lenient.message() ?? "", "ok");

    // Strict decode rejects 'Z' with UnknownFieldType
    let got_unknown = false;
    * = PgDiagnosticFields.decodePayloadStrict(raw[0..]) catch |e| blk: {
        expect_eq(e, PgFieldMessageError.UnknownFieldType);
        got_unknown = true;
        break :blk PgDiagnosticFields{};
    };
    expect_eq(got_unknown, true);
});

test("pgwire field_message: truncated, missing terminator, trailing bytes, capacity & BufferTooSmall", fn() {
    // Truncated string without NUL
    const trunc: [4]u8 = ['M', 'a', 'b', 'c'];
    let got_trunc = false;
    * = PgDiagnosticFields.decodePayload(trunc[0..]) catch |e| blk: {
        expect_eq(e, PgFieldMessageError.Truncated);
        got_trunc = true;
        break :blk PgDiagnosticFields{};
    };
    expect_eq(got_trunc, true);

    // Missing final 0x00 terminator
    const no_term: [3]u8 = ['M', 'a', 0];
    let got_no_term = false;
    * = PgDiagnosticFields.decodePayload(no_term[0..]) catch |e| blk: {
        expect_eq(e, PgFieldMessageError.MissingTerminator);
        got_no_term = true;
        break :blk PgDiagnosticFields{};
    };
    expect_eq(got_no_term, true);

    // Trailing bytes after 0x00 terminator
    const trailing: [5]u8 = ['M', 'a', 0, 0, 0xFF];
    let got_trailing = false;
    * = PgDiagnosticFields.decodePayload(trailing[0..]) catch |e| blk: {
        expect_eq(e, PgFieldMessageError.TrailingBytes);
        got_trailing = true;
        break :blk PgDiagnosticFields{};
    };
    expect_eq(got_trailing, true);

    // Capacity boundary -> TooManyFields
    let full = PgDiagnosticFields.init();
    let i: usize = 0;
    while (i < PG_MAX_DIAGNOSTIC_FIELDS) {
        * = full.add(PgDiagnosticFieldType.Hint, "h") catch unreachable;
        i += 1;
    }
    let got_full = false;
    * = full.add(PgDiagnosticFieldType.Hint, "overflow") catch |e| blk: {
        expect_eq(e, PgFieldMessageError.TooManyFields);
        got_full = true;
        break :blk 0;
    };
    expect_eq(got_full, true);

    // BufferTooSmall protection
    let tiny: [4]u8 = [0; 4];
    let got_small = false;
    * = full.encodePayload(tiny[0..]) catch |e| blk: {
        expect_eq(e, PgFieldMessageError.BufferTooSmall);
        got_small = true;
        break :blk 0;
    };
    expect_eq(got_small, true);
});
