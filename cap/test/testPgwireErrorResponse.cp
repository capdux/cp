import { test, expect } from "cap:test";
import {
    PG_TAG_ERROR_RESPONSE,
    PG_ERROR_HEADER_LEN,
    PgDiagnosticFieldType,
    PgErrorResponseError,
    PgErrorSeverity,
    parseErrorSeverity,
    PgErrorResponse,
} from "cap:pgwire";

test("ErrorResponse ('E') full roundtrip & unique violation SQLSTATE triage", () => {
    let e = PgErrorResponse.init();
    * = e.addField(PgDiagnosticFieldType.LocalizedSeverity, "ERROR")?;
    * = e.addField(PgDiagnosticFieldType.Severity, "ERROR")?;
    * = e.addField(PgDiagnosticFieldType.Code, "23505")?;
    * = e.addField(PgDiagnosticFieldType.Message, "duplicate key value violates unique constraint")?;
    * = e.addField(PgDiagnosticFieldType.Detail, "Key (email)=(a@b.com) already exists.")?;
    * = e.addField(PgDiagnosticFieldType.Schema, "public")?;
    * = e.addField(PgDiagnosticFieldType.Table, "users")?;
    * = e.addField(PgDiagnosticFieldType.Constraint, "users_email_key")?;

    expect(e.count()).toBe(8);
    expect(e.isEmpty()).toBe(false);

    let buf: [512]u8 = [0; 512];
    const n = e.encodeFrame(buf[0..])?;
    expect(n).toBe(e.encodedLen());
    expect(buf[0]).toBe(PG_TAG_ERROR_RESPONSE);

    const decoded = PgErrorResponse.decodeFrameStrict(buf[0..n])?;
    expect(decoded.eql(e)).toBe(true);
    expect(decoded.isError()).toBe(true);
    expect(decoded.isFatalOrPanic()).toBe(false);
    expect(decoded.isConstraintViolation()).toBe(true);
    expect(decoded.isUniqueViolation()).toBe(true);
    expect(decoded.isForeignKeyViolation()).toBe(false);
    expect(decoded.isRetryable()).toBe(false);
    expect(decoded.isConnectionException()).toBe(false);
    if (c := decoded.constraint()) {
        expect(c).toBe("users_email_key");
    } else {
        expect(false).toBe(true);
    }
    if (cls := decoded.sqlstateClass()) {
        expect(cls).toBe("23");
    } else {
        expect(false).toBe(true);
    }
});

test("SQLSTATE taxonomy: retryable deadlock/serialization, FK, auth, FATAL/PANIC", () => {
    expect(parseErrorSeverity("ERROR") == PgErrorSeverity.Error).toBe(true);
    expect(parseErrorSeverity("FATAL") == PgErrorSeverity.Fatal).toBe(true);
    expect(parseErrorSeverity("PANIC") == PgErrorSeverity.Panic).toBe(true);
    expect(parseErrorSeverity("OTHER") == PgErrorSeverity.Unknown).toBe(true);

    let deadlock = PgErrorResponse.init();
    * = deadlock.addField(PgDiagnosticFieldType.Severity, "ERROR")?;
    * = deadlock.addField(PgDiagnosticFieldType.Code, "40P01")?;
    expect(deadlock.isRetryable()).toBe(true);
    expect(deadlock.isConnectionException()).toBe(false);

    let fk = PgErrorResponse.init();
    * = fk.addField(PgDiagnosticFieldType.Code, "23503")?;
    expect(fk.isConstraintViolation()).toBe(true);
    expect(fk.isForeignKeyViolation()).toBe(true);
    expect(fk.isUniqueViolation()).toBe(false);

    let auth = PgErrorResponse.init();
    * = auth.addField(PgDiagnosticFieldType.Severity, "FATAL")?;
    * = auth.addField(PgDiagnosticFieldType.Code, "28P01")?;
    expect(auth.isFatal()).toBe(true);
    expect(auth.isFatalOrPanic()).toBe(true);
    expect(auth.isAuthFailure()).toBe(true);
    expect(auth.isConnectionException()).toBe(true);

    let panic_err = PgErrorResponse.init();
    * = panic_err.addField(PgDiagnosticFieldType.Severity, "PANIC")?;
    expect(panic_err.isPanic()).toBe(true);
    expect(panic_err.isConnectionException()).toBe(true);
});

test("minimal empty ErrorResponse frame (len == 4) compatibility", () => {
    const minimal: [5]u8 = ['E', 0, 0, 0, 4];
    const decoded = PgErrorResponse.decodeFrame(minimal[0..])?;
    expect(decoded.isEmpty()).toBe(true);
    expect(decoded.count()).toBe(0);
    expect(decoded.severityLevel() == PgErrorSeverity.Unknown).toBe(true);
    expect(decoded.code() == null).toBe(true);

    const decoded_strict = PgErrorResponse.decodeFrameStrict(minimal[0..])?;
    expect(decoded_strict.isEmpty()).toBe(true);
});

test("lenient vs strict decoding for unknown future diagnostic field", () => {
    // Frame: 'E', len=12 (4 + 8), unknown field 'Z'="hi\0", 'M'="ok\0", terminator '\0'
    const wire: [13]u8 = ['E', 0, 0, 0, 12, 'Z', 'h', 'i', 0, 'M', 'o', 'k', 0];
    // Add final terminator -> total 14, len = 13
    const wire_valid: [14]u8 = ['E', 0, 0, 0, 13, 'Z', 'h', 'i', 0, 'M', 'o', 'k', 0, 0];
    * = wire;

    const lenient = PgErrorResponse.decodeFrame(wire_valid[0..])?;
    expect(lenient.count()).toBe(1);
    if (m := lenient.message()) {
        expect(m).toBe("ok");
    } else {
        expect(false).toBe(true);
    }

    let strict_failed = false;
    * = PgErrorResponse.decodeFrameStrict(wire_valid[0..]) catch {
        strict_failed = true;
        PgErrorResponse.init()
    };
    expect(strict_failed).toBe(true);
});

test("error guards: EmbeddedNul, BufferTooSmall, InvalidTag, InvalidLength, Truncated, TrailingBytes", () => {
    let e = PgErrorResponse.init();
    const bad_val: [3]u8 = ['a', 0, 'b'];
    let got_nul = false;
    * = e.addField(PgDiagnosticFieldType.Message, bad_val[0..]) catch |err_val| {
        got_nul = (err_val == PgErrorResponseError.EmbeddedNul);
        0
    };
    expect(got_nul).toBe(true);

    * = e.addField(PgDiagnosticFieldType.Message, "boom")?;
    let tiny: [4]u8 = [0; 4];
    let got_small = false;
    * = e.encodeFrame(tiny[0..]) catch |err_val| {
        got_small = (err_val == PgErrorResponseError.BufferTooSmall);
        0
    };
    expect(got_small).toBe(true);

    // InvalidTag ('N' instead of 'E')
    const bad_tag: [5]u8 = ['N', 0, 0, 0, 4];
    let got_tag = false;
    * = PgErrorResponse.decodeFrame(bad_tag[0..]) catch |err_val| {
        got_tag = (err_val == PgErrorResponseError.InvalidTag);
        PgErrorResponse.init()
    };
    expect(got_tag).toBe(true);

    // InvalidLength (< 4)
    const bad_len: [5]u8 = ['E', 0, 0, 0, 3];
    let got_len = false;
    * = PgErrorResponse.decodeFrame(bad_len[0..]) catch |err_val| {
        got_len = (err_val == PgErrorResponseError.InvalidLength);
        PgErrorResponse.init()
    };
    expect(got_len).toBe(true);

    // Truncated (< 5 bytes)
    const trunc: [3]u8 = ['E', 0, 0];
    let got_trunc = false;
    * = PgErrorResponse.decodeFrame(trunc[0..]) catch |err_val| {
        got_trunc = (err_val == PgErrorResponseError.Truncated);
        PgErrorResponse.init()
    };
    expect(got_trunc).toBe(true);

    // TrailingBytes
    const trailing: [6]u8 = ['E', 0, 0, 0, 4, 0xFF];
    let got_trail = false;
    * = PgErrorResponse.decodeFrame(trailing[0..]) catch |err_val| {
        got_trail = (err_val == PgErrorResponseError.TrailingBytes);
        PgErrorResponse.init()
    };
    expect(got_trail).toBe(true);
    expect(PG_ERROR_HEADER_LEN).toBe(5);
});
