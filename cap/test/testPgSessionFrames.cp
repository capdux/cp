// Production deep test suite for cap:pgwire session control & cancellation frames:
// PgBackendKeyFrame ('K'), PgCancelRequestFrame, PgParameterStatusFrame ('S'), PgReadyForQueryFrame ('Z').

import { test, expect } from "cap:test";
import {
    PG_TAG_BACKEND_KEY_DATA,
    PG_TAG_PARAMETER_STATUS,
    PG_TAG_READY_FOR_QUERY,
    PG_CANCEL_REQUEST_CODE,
    PG_BACKEND_KEY_FRAME_LEN,
    PG_CANCEL_REQUEST_FRAME_LEN,
    PG_READY_FOR_QUERY_FRAME_LEN,
    PgTransactionState,
    parseTransactionState,
    PgCancelRequestFrame,
    PgBackendKeyFrame,
    PgParameterStatusFrame,
    PgReadyForQueryFrame,
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

test("pgwire PgBackendKeyFrame & PgCancelRequestFrame: roundtrip and cancel derivation", fn() {
    let buf: [32]u8 = [0; 32];

    const key = PgBackendKeyFrame.init(42195, 0xCAFEBABE);
    expect(key.encodedLen() == PG_BACKEND_KEY_FRAME_LEN);
    const n_key = key.encodeFrame(buf[0..]) catch unreachable;
    expect(n_key == 13);
    expect(buf[0] == PG_TAG_BACKEND_KEY_DATA);
    expect(buf[1] == 0 && buf[2] == 0 && buf[3] == 0 && buf[4] == 12);

    const dec_key = PgBackendKeyFrame.decodeFrame(buf[0..n_key]) catch unreachable;
    expect(dec_key.eql(key));
    expect(dec_key.process_id == 42195);
    expect(dec_key.secret_key == 0xCAFEBABE);

    // Derive CancelRequest directly from BackendKeyData
    const cancel = dec_key.toCancelRequest();
    expect(cancel.encodedLen() == PG_CANCEL_REQUEST_FRAME_LEN);
    const n_cancel = cancel.encodeFrame(buf[0..]) catch unreachable;
    expect(n_cancel == 16);
    expect(PG_CANCEL_REQUEST_CODE == 80877102);

    const dec_cancel = PgCancelRequestFrame.decodeFrame(buf[0..n_cancel]) catch unreachable;
    expect(dec_cancel.eql(cancel));
    expect(dec_cancel.process_id == 42195);
    expect(dec_cancel.secret_key == 0xCAFEBABE);
});

test("pgwire PgParameterStatusFrame: roundtrip, empty value, and runtime parameter helpers", fn() {
    let buf: [128]u8 = [0; 128];

    const p1 = PgParameterStatusFrame.init("server_version", "16.4 (Debian 16.4-1)");
    const n1 = p1.encodeFrame(buf[0..]) catch unreachable;
    expect(n1 == p1.encodedLen());
    expect(buf[0] == PG_TAG_PARAMETER_STATUS);
    const d1 = PgParameterStatusFrame.decodeFrame(buf[0..n1]) catch unreachable;
    expect(d1.eql(p1));
    expect(d1.isServerVersion());
    expect(!d1.isClientEncoding());
    expect(strEql(d1.value, "16.4 (Debian 16.4-1)"));

    const p2 = PgParameterStatusFrame.init("client_encoding", "UTF8");
    expect(p2.isClientEncoding());

    const p3 = PgParameterStatusFrame.init("server_encoding", "UTF8");
    expect(p3.isServerEncoding());

    const p4 = PgParameterStatusFrame.init("TimeZone", "UTC");
    expect(p4.isTimeZone());

    const p5 = PgParameterStatusFrame.init("standard_conforming_strings", "on");
    expect(p5.isStandardConformingStrings());

    // Empty value is legal in PostgreSQL ParameterStatus (e.g. application_name = "")
    const p_empty = PgParameterStatusFrame.init("application_name", "");
    const n_empty = p_empty.encodeFrame(buf[0..]) catch unreachable;
    const d_empty = PgParameterStatusFrame.decodeFrame(buf[0..n_empty]) catch unreachable;
    expect(d_empty.eql(p_empty));
    expect(d_empty.value.len == 0);
});

test("pgwire PgReadyForQueryFrame: all 3 transaction states and pool readiness helpers", fn() {
    let buf: [16]u8 = [0; 16];

    expect(parseTransactionState('I') == PgTransactionState.Idle);
    expect(parseTransactionState('T') == PgTransactionState.InTransaction);
    expect(parseTransactionState('E') == PgTransactionState.FailedTransaction);
    expect(parseTransactionState('X') == null);

    // 1. Idle ('I')
    const idle = PgReadyForQueryFrame.init(PgTransactionState.Idle);
    const n1 = idle.encodeFrame(buf[0..]) catch unreachable;
    expect(n1 == PG_READY_FOR_QUERY_FRAME_LEN);
    expect(buf[0] == PG_TAG_READY_FOR_QUERY && buf[5] == 'I');
    const d1 = PgReadyForQueryFrame.decodeFrame(buf[0..n1]) catch unreachable;
    expect(d1.eql(idle));
    expect(d1.isIdle());
    expect(!d1.inTransaction());
    expect(!d1.isFailedTransaction());
    expect(d1.canSendQuery());
    expect(!d1.needsRollback());

    // 2. InTransaction ('T')
    const tx = PgReadyForQueryFrame.init(PgTransactionState.InTransaction);
    const n2 = tx.encodeFrame(buf[0..]) catch unreachable;
    const d2 = PgReadyForQueryFrame.decodeFrame(buf[0..n2]) catch unreachable;
    expect(d2.eql(tx));
    expect(!d2.isIdle());
    expect(d2.inTransaction());
    expect(d2.canSendQuery());
    expect(!d2.needsRollback());

    // 3. FailedTransaction ('E')
    const failed = PgReadyForQueryFrame.init(PgTransactionState.FailedTransaction);
    const n3 = failed.encodeFrame(buf[0..]) catch unreachable;
    const d3 = PgReadyForQueryFrame.decodeFrame(buf[0..n3]) catch unreachable;
    expect(d3.eql(failed));
    expect(d3.isFailedTransaction());
    expect(!d3.canSendQuery());
    expect(d3.needsRollback());
});

test("pgwire PgBackendKeyFrame & PgCancelRequestFrame: malformed frame protection", fn() {
    let tiny: [4]u8 = [0; 4];

    // Buffer too small on encode
    let e_buf1 = false;
    * = PgBackendKeyFrame.init(1, 2).encodeFrame(tiny[0..]) catch { e_buf1 = true; };
    expect(e_buf1);

    let e_buf2 = false;
    * = PgCancelRequestFrame.init(1, 2).encodeFrame(tiny[0..]) catch { e_buf2 = true; };
    expect(e_buf2);

    // BackendKeyData wrong tag, wrong length, truncated, trailing
    const bad_tag: [13]u8 = ['X', 0, 0, 0, 12, 0, 0, 0, 1, 0, 0, 0, 2];
    let e1 = false;
    * = PgBackendKeyFrame.decodeFrame(bad_tag[0..]) catch { e1 = true; };
    expect(e1);

    const bad_len: [13]u8 = ['K', 0, 0, 0, 11, 0, 0, 0, 1, 0, 0, 0, 2];
    let e2 = false;
    * = PgBackendKeyFrame.decodeFrame(bad_len[0..]) catch { e2 = true; };
    expect(e2);

    const trunc_k: [12]u8 = ['K', 0, 0, 0, 12, 0, 0, 0, 1, 0, 0, 0];
    let e3 = false;
    * = PgBackendKeyFrame.decodeFrame(trunc_k[0..]) catch { e3 = true; };
    expect(e3);

    const trail_k: [14]u8 = ['K', 0, 0, 0, 12, 0, 0, 0, 1, 0, 0, 0, 2, 0];
    let e4 = false;
    * = PgBackendKeyFrame.decodeFrame(trail_k[0..]) catch { e4 = true; };
    expect(e4);

    // CancelRequest wrong magic code & wrong length
    const bad_code: [16]u8 = [0, 0, 0, 16, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 2];
    let e5 = false;
    * = PgCancelRequestFrame.decodeFrame(bad_code[0..]) catch { e5 = true; };
    expect(e5);

    const bad_clen: [16]u8 = [0, 0, 0, 15, 0x04, 0xD2, 0x16, 0x2E, 0, 0, 0, 1, 0, 0, 0, 2];
    let e6 = false;
    * = PgCancelRequestFrame.decodeFrame(bad_clen[0..]) catch { e6 = true; };
    expect(e6);
});

test("pgwire PgParameterStatusFrame: malformed frame and NUL injection protection", fn() {
    let out: [32]u8 = [0; 32];

    // Empty name on encode
    let e1 = false;
    * = PgParameterStatusFrame.init("", "v").encodeFrame(out[0..]) catch { e1 = true; };
    expect(e1);

    // Embedded NUL in name or value on encode
    const nul_str: [3]u8 = ['a', 0, 'b'];
    let e2 = false;
    * = PgParameterStatusFrame.init(nul_str[0..], "v").encodeFrame(out[0..]) catch { e2 = true; };
    expect(e2);

    let e3 = false;
    * = PgParameterStatusFrame.init("k", nul_str[0..]).encodeFrame(out[0..]) catch { e3 = true; };
    expect(e3);

    // Missing second NUL terminator on decode
    const no_nul2: [9]u8 = ['S', 0, 0, 0, 8, 'k', 0, 'v', '2'];
    let e4 = false;
    * = PgParameterStatusFrame.decodeFrame(no_nul2[0..]) catch { e4 = true; };
    expect(e4);

    // Trailing bytes inside payload after value NUL
    const extra_in: [10]u8 = ['S', 0, 0, 0, 9, 'k', 0, 'v', 0, 'x'];
    let e5 = false;
    * = PgParameterStatusFrame.decodeFrame(extra_in[0..]) catch { e5 = true; };
    expect(e5);

    // Empty name on decode
    const empty_name: [8]u8 = ['S', 0, 0, 0, 7, 0, 'v', 0];
    let e6 = false;
    * = PgParameterStatusFrame.decodeFrame(empty_name[0..]) catch { e6 = true; };
    expect(e6);
});

test("pgwire PgReadyForQueryFrame: invalid tag, length, and unknown tx status guards", fn() {
    const bad_tag: [6]u8 = ['X', 0, 0, 0, 5, 'I'];
    let e1 = false;
    * = PgReadyForQueryFrame.decodeFrame(bad_tag[0..]) catch { e1 = true; };
    expect(e1);

    const bad_len: [6]u8 = ['Z', 0, 0, 0, 6, 'I'];
    let e2 = false;
    * = PgReadyForQueryFrame.decodeFrame(bad_len[0..]) catch { e2 = true; };
    expect(e2);

    const bad_status: [6]u8 = ['Z', 0, 0, 0, 5, 'Q'];
    let e3 = false;
    * = PgReadyForQueryFrame.decodeFrame(bad_status[0..]) catch { e3 = true; };
    expect(e3);

    const trunc: [5]u8 = ['Z', 0, 0, 0, 5];
    let e4 = false;
    * = PgReadyForQueryFrame.decodeFrame(trunc[0..]) catch { e4 = true; };
    expect(e4);

    const trail: [7]u8 = ['Z', 0, 0, 0, 5, 'I', 0];
    let e5 = false;
    * = PgReadyForQueryFrame.decodeFrame(trail[0..]) catch { e5 = true; };
    expect(e5);
});
