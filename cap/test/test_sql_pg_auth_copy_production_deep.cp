// cap/test/test_sql_pg_auth_copy_production_deep.cp
// Deep production tests for cap:sql PostgreSQL v3 Auth (SASL/Password), COPY Stream & Terminate Codec (pg_auth_copy.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    PG_PASSWORD_TAG,
    PG_COPY_DATA_TAG,
    PG_COPY_DONE_TAG,
    PG_COPY_FAIL_TAG,
    PG_TERMINATE_TAG,
    encodePgPasswordMessage,
    decodePgPasswordMessage,
    encodePgSaslInitialResponse,
    decodePgSaslInitialResponse,
    encodePgSaslResponse,
    decodePgSaslResponse,
    encodePgCopyData,
    decodePgCopyData,
    encodePgCopyDone,
    decodePgCopyDone,
    encodePgCopyFail,
    decodePgCopyFail,
    encodePgTerminate,
    decodePgTerminate,
} from "cap:sql";

test("pg_auth_copy: PasswordMessage roundtrip (md5 & cleartext)", () => {
    let buf: [128]u8 = [0; 128];
    const wire = encodePgPasswordMessage(buf[0..], "md5d41d8cd98f00b204e9800998ecf8427e") catch {
        expect_true(false);
        return;
    };
    expect_eq(wire[0], PG_PASSWORD_TAG);
    const pw = decodePgPasswordMessage(wire) catch { expect_true(false); return; };
    expect_eq(pw, "md5d41d8cd98f00b204e9800998ecf8427e");

    // Empty password
    const wire_empty = encodePgPasswordMessage(buf[0..], "") catch { expect_true(false); return; };
    expect_eq(wire_empty.len, 6); // 'p' + 4B len(5) + '\0'
    const pw_empty = decodePgPasswordMessage(wire_empty) catch { expect_true(false); return; };
    expect_eq(pw_empty, "");
});

test("pg_auth_copy: SASLInitialResponse with initial data & null (-1) + SASLResponse", () => {
    let buf: [256]u8 = [0; 256];
    const w1 = encodePgSaslInitialResponse(buf[0..], "SCRAM-SHA-256", "n,,n=,r=rOprNGfwEbeRWgbNEkqO") catch {
        expect_true(false);
        return;
    };
    expect_eq(w1[0], PG_PASSWORD_TAG);
    const d1 = decodePgSaslInitialResponse(w1) catch { expect_true(false); return; };
    expect_eq(d1.mechanism, "SCRAM-SHA-256");
    expect_true(d1.data != null);
    if (payload := d1.data) {
        expect_eq(payload, "n,,n=,r=rOprNGfwEbeRWgbNEkqO");
    }

    // Null initial data -> wire encodes -1 (0xFFFFFFFF)
    const w_null = encodePgSaslInitialResponse(buf[0..], "SCRAM-SHA-256-PLUS", null) catch {
        expect_true(false);
        return;
    };
    const d_null = decodePgSaslInitialResponse(w_null) catch { expect_true(false); return; };
    expect_eq(d_null.mechanism, "SCRAM-SHA-256-PLUS");
    expect_true(d_null.data == null);

    // SASLResponse client-final-message
    const w_resp = encodePgSaslResponse(buf[0..], "c=biws,r=nonce,p=proof==") catch {
        expect_true(false);
        return;
    };
    const d_resp = decodePgSaslResponse(w_resp) catch { expect_true(false); return; };
    expect_eq(d_resp, "c=biws,r=nonce,p=proof==");
});

test("pg_auth_copy: COPY stream lifecycle (CopyData, CopyDone, CopyFail) roundtrip", () => {
    let buf: [256]u8 = [0; 256];

    // Binary/text CopyData
    const row_chunk = "101\tAlice\tactive\n102\tBob\tpending\n";
    const w_data = encodePgCopyData(buf[0..], row_chunk) catch { expect_true(false); return; };
    expect_eq(w_data[0], PG_COPY_DATA_TAG);
    const d_data = decodePgCopyData(w_data) catch { expect_true(false); return; };
    expect_eq(d_data, row_chunk);

    // Empty CopyData chunk is legal
    const w_empty = encodePgCopyData(buf[0..], "") catch { expect_true(false); return; };
    expect_eq(w_empty.len, 5);
    const d_empty = decodePgCopyData(w_empty) catch { expect_true(false); return; };
    expect_eq(d_empty, "");

    // CopyDone 5-byte fixed frame
    const w_done = encodePgCopyDone(buf[0..]) catch { expect_true(false); return; };
    expect_eq(w_done.len, 5);
    expect_eq(w_done[0], PG_COPY_DONE_TAG);
    expect_eq(w_done[4], 4);
    const ok_done = decodePgCopyDone(w_done) catch { expect_true(false); return; };
    expect_true(ok_done);

    // CopyFail with abort reason
    const w_fail = encodePgCopyFail(buf[0..], "client stream aborted: checksum mismatch") catch {
        expect_true(false);
        return;
    };
    expect_eq(w_fail[0], PG_COPY_FAIL_TAG);
    const reason = decodePgCopyFail(w_fail) catch { expect_true(false); return; };
    expect_eq(reason, "client stream aborted: checksum mismatch");
});

test("pg_auth_copy: Terminate 5-byte fixed frame roundtrip", () => {
    let buf: [16]u8 = [0; 16];
    const wire = encodePgTerminate(buf[0..]) catch { expect_true(false); return; };
    expect_eq(wire.len, 5);
    expect_eq(wire[0], PG_TERMINATE_TAG);
    expect_eq(wire[4], 4);
    const ok = decodePgTerminate(wire) catch { expect_true(false); return; };
    expect_true(ok);
});

test("pg_auth_copy: malformed frames and boundary error guards", () => {
    let buf: [64]u8 = [0; 64];

    // 1. Empty SASL mechanism rejected on encode
    let f1 = false;
    * = encodePgSaslInitialResponse(buf[0..], "", "abc") catch { f1 = true; };
    expect_true(f1);

    // 2. BufferTooSmall on encoders
    let tiny: [3]u8 = [0; 3];
    let f2 = false;
    * = encodePgPasswordMessage(tiny[0..], "pw") catch { f2 = true; };
    expect_true(f2);

    let f3 = false;
    * = encodePgTerminate(tiny[0..]) catch { f3 = true; };
    expect_true(f3);

    // 3. Invalid tag mismatch
    const term_wire = encodePgTerminate(buf[0..]) catch { expect_true(false); return; };
    let f4 = false;
    * = decodePgCopyDone(term_wire) catch { f4 = true; };
    expect_true(f4);

    // 4. Non-empty CopyDone / Terminate payload rejected
    const bad_done: [6]u8 = ['c', 0, 0, 0, 5, 0];
    let f5 = false;
    * = decodePgCopyDone(bad_done[0..]) catch { f5 = true; };
    expect_true(f5);

    const bad_term: [6]u8 = ['X', 0, 0, 0, 5, 0];
    let f6 = false;
    * = decodePgTerminate(bad_term[0..]) catch { f6 = true; };
    expect_true(f6);

    // 5. Missing NUL terminator in PasswordMessage & CopyFail
    const no_nul_pw: [7]u8 = ['p', 0, 0, 0, 6, 'a', 'b'];
    let f7 = false;
    * = decodePgPasswordMessage(no_nul_pw[0..]) catch { f7 = true; };
    expect_true(f7);

    const no_nul_fail: [7]u8 = ['f', 0, 0, 0, 6, 'e', 'r'];
    let f8 = false;
    * = decodePgCopyFail(no_nul_fail[0..]) catch { f8 = true; };
    expect_true(f8);

    // 6. Trailing bytes rejected
    const trailing_term: [6]u8 = ['X', 0, 0, 0, 4, 0xaa];
    let f9 = false;
    * = decodePgTerminate(trailing_term[0..]) catch { f9 = true; };
    expect_true(f9);
});
