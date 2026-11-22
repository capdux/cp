// Production deep test suite for cap:pgwire NoticeResponse ('N') codec.

import { test, expect } from "cap:test";
import {
    PG_TAG_NOTICE_RESPONSE,
    PG_NOTICE_HEADER_LEN,
    PG_MAX_DIAGNOSTIC_FIELDS,
    PgDiagnosticFieldType,
    PgDiagnosticFields,
    PgNoticeSeverity,
    parseNoticeSeverity,
    PgNoticeResponse,
} from "cap:pgwire";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("pgwire NoticeResponse: roundtrip encode and decode with standard fields", fn() {
    let notice = PgNoticeResponse.init();
    * = notice.addField(PgDiagnosticFieldType.Severity, "提示") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.LocalizedSeverity, "NOTICE") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Code, "42P07") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Message, "relation \"users\" already exists, skipping") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Detail, "Existing table kept intact") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Hint, "Remove IF NOT EXISTS to fail instead") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Where, "SQL statement \"CREATE TABLE IF NOT EXISTS users (id int)\"") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.File, "parse_utilcmd.c") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Line, "245") catch unreachable;
    * = notice.addField(PgDiagnosticFieldType.Routine, "transformCreateStmt") catch unreachable;

    expect(notice.count() == 10);
    expect(!notice.isEmpty());

    let buf: [512]u8 = [0; 512];
    const written = notice.encodeFrame(buf[0..]) catch unreachable;
    expect(written == notice.encodedLen());
    expect(written == PG_NOTICE_HEADER_LEN + notice.payloadLen());
    expect(buf[0] == PG_TAG_NOTICE_RESPONSE);

    const decoded = PgNoticeResponse.decodeFrame(buf[0..written]) catch unreachable;
    expect(decoded.eql(notice));
    expect(decoded.count() == 10);
    expect(decoded.isNotice());
    expect(!decoded.isWarning());
    expect(decoded.severityLevel() == PgNoticeSeverity.Notice);

    if (sev := decoded.severity()) {
        // 优先非本地化 'V' 字段
        expect(strEql(sev, "NOTICE"));
    } else {
        expect(false);
    }
    if (code := decoded.code()) {
        expect(strEql(code, "42P07"));
    } else {
        expect(false);
    }
    if (cls := decoded.sqlstateClass()) {
        expect(strEql(cls, "42"));
    } else {
        expect(false);
    }
    if (msg := decoded.message()) {
        expect(strEql(msg, "relation \"users\" already exists, skipping"));
    } else {
        expect(false);
    }
    if (det := decoded.detail()) {
        expect(strEql(det, "Existing table kept intact"));
    } else {
        expect(false);
    }
    if (h := decoded.hint()) {
        expect(strEql(h, "Remove IF NOT EXISTS to fail instead"));
    } else {
        expect(false);
    }
    if (w := decoded.whereContext()) {
        expect(strEql(w, "SQL statement \"CREATE TABLE IF NOT EXISTS users (id int)\""));
    } else {
        expect(false);
    }
    if (f := decoded.file()) {
        expect(strEql(f, "parse_utilcmd.c"));
    } else {
        expect(false);
    }
    if (ln := decoded.line()) {
        expect(strEql(ln, "245"));
    } else {
        expect(false);
    }
    if (rt := decoded.routine()) {
        expect(strEql(rt, "transformCreateStmt"));
    } else {
        expect(false);
    }

    const strict_decoded = PgNoticeResponse.decodeFrameStrict(buf[0..written]) catch unreachable;
    expect(strict_decoded.eql(notice));
});

test("pgwire NoticeResponse: severity classification and schema/table/column/constraint accessors", fn() {
    expect(parseNoticeSeverity("NOTICE") == PgNoticeSeverity.Notice);
    expect(parseNoticeSeverity("WARNING") == PgNoticeSeverity.Warning);
    expect(parseNoticeSeverity("INFO") == PgNoticeSeverity.Info);
    expect(parseNoticeSeverity("LOG") == PgNoticeSeverity.Log);
    expect(parseNoticeSeverity("DEBUG") == PgNoticeSeverity.Debug);
    expect(parseNoticeSeverity("DEBUG1") == PgNoticeSeverity.Debug);
    expect(parseNoticeSeverity("DEBUG5") == PgNoticeSeverity.Debug);
    expect(parseNoticeSeverity("CUSTOM") == PgNoticeSeverity.Unknown);

    let warn = PgNoticeResponse.init();
    * = warn.addField(PgDiagnosticFieldType.Severity, "WARNING") catch unreachable;
    * = warn.addField(PgDiagnosticFieldType.Code, "01000") catch unreachable;
    * = warn.addField(PgDiagnosticFieldType.Message, "deprecated syntax") catch unreachable;
    * = warn.addField(PgDiagnosticFieldType.Schema, "public") catch unreachable;
    * = warn.addField(PgDiagnosticFieldType.Table, "orders") catch unreachable;
    * = warn.addField(PgDiagnosticFieldType.Column, "total_cents") catch unreachable;
    * = warn.addField(PgDiagnosticFieldType.Constraint, "orders_total_positive") catch unreachable;

    expect(warn.isWarning());
    expect(!warn.isNotice());
    expect(warn.severityLevel() == PgNoticeSeverity.Warning);
    if (s := warn.schema()) { expect(strEql(s, "public")); } else { expect(false); }
    if (t := warn.table()) { expect(strEql(t, "orders")); } else { expect(false); }
    if (c := warn.column()) { expect(strEql(c, "total_cents")); } else { expect(false); }
    if (con := warn.constraint()) { expect(strEql(con, "orders_total_positive")); } else { expect(false); }
    if (f0 := warn.at(0)) {
        expect(f0.field_type == PgDiagnosticFieldType.Severity);
        expect(strEql(f0.value, "WARNING"));
    } else {
        expect(false);
    }
    expect(warn.at(99) == null);
});

test("pgwire NoticeResponse: empty payload and minimal len==4 frame compatibility", fn() {
    // Minimal 5-byte frame with length == 4 (remaining_bytes == 0, matching Bun NoticeResponse.decodeInternal)
    const min_frame: [5]u8 = ['N', 0, 0, 0, 4];
    const empty_decoded = PgNoticeResponse.decodeFrame(min_frame[0..]) catch unreachable;
    expect(empty_decoded.isEmpty());
    expect(empty_decoded.count() == 0);
    expect(empty_decoded.severity() == null);
    expect(empty_decoded.severityLevel() == PgNoticeSeverity.Unknown);
    expect(empty_decoded.message() == null);

    // Standard empty notice frame with 1-byte 0x00 terminator (length == 5)
    let buf: [16]u8 = [0; 16];
    const n = empty_decoded.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 6);
    expect(buf[0] == 'N');
    expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 5);
    expect(buf[5] == 0);

    const rt = PgNoticeResponse.decodeFrameStrict(buf[0..n]) catch unreachable;
    expect(rt.isEmpty());

    // Direct empty payload decode
    const empty_slice = buf[0..0];
    const p_empty = PgNoticeResponse.decodePayload(empty_slice) catch unreachable;
    expect(p_empty.isEmpty());
});

test("pgwire NoticeResponse: lenient vs strict mode with unknown future extension field", fn() {
    // Frame: 'N' + len(23) + 'V' "INFO\0" + 'Z' "future_ext\0" + 'M' "ok\0" + '\0'
    // payload len = (1+4+1) + (1+10+1) + (1+2+1) + 1 = 6 + 12 + 4 + 1 = 23
    // declared_len = 4 + 23 = 27 (0x1B)
    const raw: [28]u8 = [
        'N', 0, 0, 0, 27,
        'V', 'I', 'N', 'F', 'O', 0,
        'Z', 'f', 'u', 't', 'u', 'r', 'e', '_', 'e', 'x', 't', 0,
        'M', 'o', 'k', 0,
        0,
    ];

    const lenient = PgNoticeResponse.decodeFrame(raw[0..]) catch unreachable;
    expect(lenient.count() == 2);
    expect(lenient.severityLevel() == PgNoticeSeverity.Info);
    if (m := lenient.message()) {
        expect(strEql(m, "ok"));
    } else {
        expect(false);
    }

    let strict_failed = false;
    * = PgNoticeResponse.decodeFrameStrict(raw[0..]) catch {
        strict_failed = true;
    };
    expect(strict_failed);
});

test("pgwire NoticeResponse: malformed frames and boundary error protection", fn() {
    // 1. Truncated header (< 5 bytes)
    const short_hdr: [4]u8 = ['N', 0, 0, 4];
    let e1 = false;
    * = PgNoticeResponse.decodeFrame(short_hdr[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Wrong tag ('E' instead of 'N')
    const wrong_tag: [6]u8 = ['E', 0, 0, 0, 5, 0];
    let e2 = false;
    * = PgNoticeResponse.decodeFrame(wrong_tag[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Invalid length (< 4)
    const bad_len: [5]u8 = ['N', 0, 0, 0, 3];
    let e3 = false;
    * = PgNoticeResponse.decodeFrame(bad_len[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Truncated payload (declared_len > available)
    const trunc_payload: [7]u8 = ['N', 0, 0, 0, 12, 'M', 'a'];
    let e4 = false;
    * = PgNoticeResponse.decodeFrame(trunc_payload[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Trailing bytes after frame
    const trailing: [7]u8 = ['N', 0, 0, 0, 5, 0, 0xFF];
    let e5 = false;
    * = PgNoticeResponse.decodeFrame(trailing[0..]) catch { e5 = true; };
    expect(e5);

    // 6. Missing 0x00 field terminator
    const no_term: [8]u8 = ['N', 0, 0, 0, 7, 'M', 'x', 0];
    let e6 = false;
    * = PgNoticeResponse.decodeFrame(no_term[0..]) catch { e6 = true; };
    expect(e6);
});

test("pgwire NoticeResponse: buffer too small, embedded NUL, and max field capacity guards", fn() {
    let n = PgNoticeResponse.init();
    * = n.addField(PgDiagnosticFieldType.Message, "hello") catch unreachable;

    // 1. Buffer too small for encodeFrame
    let small: [4]u8 = [0; 4];
    let too_small = false;
    * = n.encodeFrame(small[0..]) catch { too_small = true; };
    expect(too_small);

    // 2. Embedded NUL rejected on addField
    const bad_val: [3]u8 = ['a', 0, 'b'];
    let has_nul = false;
    * = n.addField(PgDiagnosticFieldType.Detail, bad_val[0..]) catch { has_nul = true; };
    expect(has_nul);

    // 3. Embedded NUL rejected on encodeFrame even when constructed via fromFields
    let raw_fields = PgDiagnosticFields.init();
    * = raw_fields.add(PgDiagnosticFieldType.Message, bad_val[0..]) catch unreachable;
    const forged = PgNoticeResponse.fromFields(raw_fields);
    let big: [64]u8 = [0; 64];
    let enc_nul = false;
    * = forged.encodeFrame(big[0..]) catch { enc_nul = true; };
    expect(enc_nul);

    // 4. Max capacity guard (> PG_MAX_DIAGNOSTIC_FIELDS)
    let full = PgNoticeResponse.init();
    let i: usize = 0;
    while (i < PG_MAX_DIAGNOSTIC_FIELDS) {
        * = full.addField(PgDiagnosticFieldType.Message, "m") catch unreachable;
        i += 1;
    }
    expect(full.count() == PG_MAX_DIAGNOSTIC_FIELDS);
    let overflow = false;
    * = full.addField(PgDiagnosticFieldType.Message, "extra") catch { overflow = true; };
    expect(overflow);
});
