// Production deep test suite for cap:pgwire connection handshake frames:
// PgStartupMessage (v3.0), PgTlsUpgradeRequest (SSLRequest / GSSENCRequest).

import { test, expect } from "cap:test";
import {
    PG_STARTUP_PROTOCOL_V3,
    PG_STARTUP_SSL_REQUEST_CODE,
    PG_STARTUP_GSSENC_REQUEST_CODE,
    PG_STARTUP_UPGRADE_FRAME_LEN,
    PG_STARTUP_MAX_PARAMS,
    PgUpgradeKind,
    PgStartupMessage,
    PgTlsUpgradeRequest,
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

test("pgwire PgStartupMessage: withDefaults and custom params roundtrip", fn() {
    let msg = PgStartupMessage.withDefaults("postgres", "billing_prod") catch unreachable;
    * = msg.addParam("application_name", "cp_agent_pool") catch unreachable;
    * = msg.addParam("options", "-c search_path=app") catch unreachable;

    expect(msg.count() == 5);
    expect(msg.protocol_version == PG_STARTUP_PROTOCOL_V3);
    expect(msg.protocolMajor() == 3);
    expect(msg.protocolMinor() == 0);

    let buf: [256]u8 = [0; 256];
    const written = msg.encodeFrame(buf[0..]) catch unreachable;
    expect(written == msg.encodedLen());
    expect(buf[written - 1] == 0);

    const decoded = PgStartupMessage.decodeFrame(buf[0..written]) catch unreachable;
    expect(decoded.eql(msg));
    expect(decoded.count() == 5);

    if (u := decoded.user()) { expect(strEql(u, "postgres")); } else { expect(false); }
    if (db := decoded.database()) { expect(strEql(db, "billing_prod")); } else { expect(false); }
    if (enc := decoded.clientEncoding()) { expect(strEql(enc, "UTF8")); } else { expect(false); }
    if (app := decoded.applicationName()) { expect(strEql(app, "cp_agent_pool")); } else { expect(false); }
    if (opt := decoded.get("options")) { expect(strEql(opt, "-c search_path=app")); } else { expect(false); }
    expect(decoded.get("nonexistent") == null);
    if (p0 := decoded.at(0)) {
        expect(strEql(p0.key, "user"));
        expect(strEql(p0.value, "postgres"));
    } else {
        expect(false);
    }
    expect(decoded.at(99) == null);
});

test("pgwire PgStartupMessage: empty database falls back to user (Bun semantics)", fn() {
    const msg = PgStartupMessage.withDefaults("analytics", "") catch unreachable;
    expect(msg.count() == 3);
    if (u := msg.user()) { expect(strEql(u, "analytics")); } else { expect(false); }
    if (db := msg.database()) { expect(strEql(db, "analytics")); } else { expect(false); }
    if (enc := msg.clientEncoding()) { expect(strEql(enc, "UTF8")); } else { expect(false); }

    let buf: [128]u8 = [0; 128];
    const n = msg.encodeFrame(buf[0..]) catch unreachable;
    const decoded = PgStartupMessage.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(msg));
});

test("pgwire PgTlsUpgradeRequest: SSLRequest and GSSENCRequest roundtrip and response checks", fn() {
    let buf: [16]u8 = [0; 16];

    // 1. SSLRequest
    const ssl = PgTlsUpgradeRequest.ssl();
    expect(ssl.kind == PgUpgradeKind.Ssl);
    expect(ssl.code() == PG_STARTUP_SSL_REQUEST_CODE);
    expect(ssl.encodedLen() == PG_STARTUP_UPGRADE_FRAME_LEN);
    const n1 = ssl.encodeFrame(buf[0..]) catch unreachable;
    expect(n1 == 8);
    expect(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 8);

    const dec_ssl = PgTlsUpgradeRequest.decodeFrame(buf[0..n1]) catch unreachable;
    expect(dec_ssl.eql(ssl));
    expect(dec_ssl.isAcceptedByte('S'));
    expect(!dec_ssl.isAcceptedByte('G'));
    expect(PgTlsUpgradeRequest.isRejectedByte('N'));
    expect(!PgTlsUpgradeRequest.isRejectedByte('S'));

    // 2. GSSENCRequest
    const gss = PgTlsUpgradeRequest.gssEnc();
    expect(gss.kind == PgUpgradeKind.GssEnc);
    expect(gss.code() == PG_STARTUP_GSSENC_REQUEST_CODE);
    const n2 = gss.encodeFrame(buf[0..]) catch unreachable;
    expect(n2 == 8);
    const dec_gss = PgTlsUpgradeRequest.decodeFrame(buf[0..n2]) catch unreachable;
    expect(dec_gss.eql(gss));
    expect(dec_gss.isAcceptedByte('G'));
    expect(!dec_gss.isAcceptedByte('S'));
});

test("pgwire PgStartupMessage: malformed frames and protocol version guards", fn() {
    // 1. Truncated (< 9 bytes)
    const short_f: [8]u8 = [0, 0, 0, 8, 0, 3, 0, 0];
    let e1 = false;
    * = PgStartupMessage.decodeFrame(short_f[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Invalid major protocol version (2.0 instead of 3.0)
    const proto_v2: [9]u8 = [0, 0, 0, 9, 0, 2, 0, 0, 0];
    let e2 = false;
    * = PgStartupMessage.decodeFrame(proto_v2[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Declared length > available
    const trunc_body: [10]u8 = [0, 0, 0, 20, 0, 3, 0, 0, 'u', 0];
    let e3 = false;
    * = PgStartupMessage.decodeFrame(trunc_body[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Trailing bytes after frame
    const trail_f: [10]u8 = [0, 0, 0, 9, 0, 3, 0, 0, 0, 0xFF];
    let e4 = false;
    * = PgStartupMessage.decodeFrame(trail_f[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Odd parameter (key without value NUL)
    const odd_param: [11]u8 = [0, 0, 0, 11, 0, 3, 0, 0, 'u', 0, 'v'];
    let e5 = false;
    * = PgStartupMessage.decodeFrame(odd_param[0..]) catch { e5 = true; };
    expect(e5);

    // 6. Missing final 0x00 terminator
    const no_end: [12]u8 = [0, 0, 0, 12, 0, 3, 0, 0, 'u', 0, 'v', 0];
    let e6 = false;
    * = PgStartupMessage.decodeFrame(no_end[0..]) catch { e6 = true; };
    expect(e6);
});

test("pgwire PgStartupMessage: missing user, empty key, embedded NUL, capacity and buffer guards", fn() {
    let out: [64]u8 = [0; 64];

    // 1. withDefaults empty user
    let e1 = false;
    * = PgStartupMessage.withDefaults("", "db") catch { e1 = true; };
    expect(e1);

    // 2. encodeFrame without user param
    let no_user = PgStartupMessage.init();
    * = no_user.addParam("database", "app") catch unreachable;
    let e2 = false;
    * = no_user.encodeFrame(out[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Empty key rejected
    let e3 = false;
    * = no_user.addParam("", "val") catch { e3 = true; };
    expect(e3);

    // 4. Embedded NUL in key or value rejected
    const bad: [3]u8 = ['a', 0, 'b'];
    let e4 = false;
    * = no_user.addParam(bad[0..], "v") catch { e4 = true; };
    expect(e4);

    let e5 = false;
    * = no_user.addParam("k", bad[0..]) catch { e5 = true; };
    expect(e5);

    // 5. Buffer too small on encode
    const valid = PgStartupMessage.withDefaults("u", "d") catch unreachable;
    let tiny: [8]u8 = [0; 8];
    let e6 = false;
    * = valid.encodeFrame(tiny[0..]) catch { e6 = true; };
    expect(e6);

    // 6. Max param capacity guard (> PG_STARTUP_MAX_PARAMS)
    let full = PgStartupMessage.init();
    let i: usize = 0;
    while (i < PG_STARTUP_MAX_PARAMS) {
        * = full.addParam("k", "v") catch unreachable;
        i += 1;
    }
    expect(full.count() == PG_STARTUP_MAX_PARAMS);
    let e7 = false;
    * = full.addParam("extra", "v") catch { e7 = true; };
    expect(e7);
});

test("pgwire PgTlsUpgradeRequest: malformed upgrade frame guards", fn() {
    let tiny: [4]u8 = [0; 4];
    let e_buf = false;
    * = PgTlsUpgradeRequest.ssl().encodeFrame(tiny[0..]) catch { e_buf = true; };
    expect(e_buf);

    const bad_len: [8]u8 = [0, 0, 0, 9, 0x04, 0xD2, 0x16, 0x2F];
    let e1 = false;
    * = PgTlsUpgradeRequest.decodeFrame(bad_len[0..]) catch { e1 = true; };
    expect(e1);

    const bad_code: [8]u8 = [0, 0, 0, 8, 0, 3, 0, 0];
    let e2 = false;
    * = PgTlsUpgradeRequest.decodeFrame(bad_code[0..]) catch { e2 = true; };
    expect(e2);

    const trunc: [7]u8 = [0, 0, 0, 8, 0x04, 0xD2, 0x16];
    let e3 = false;
    * = PgTlsUpgradeRequest.decodeFrame(trunc[0..]) catch { e3 = true; };
    expect(e3);

    const trail: [9]u8 = [0, 0, 0, 8, 0x04, 0xD2, 0x16, 0x2F, 0];
    let e4 = false;
    * = PgTlsUpgradeRequest.decodeFrame(trail[0..]) catch { e4 = true; };
    expect(e4);
});
