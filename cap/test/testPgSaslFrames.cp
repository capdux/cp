// Production deep test suite for cap:pgwire frontend authentication ('p') frames:
// PgPasswordMessage, PgSaslInitialResponse, PgSaslResponse.

import { test, expect } from "cap:test";
import {
    PG_TAG_PASSWORD_MESSAGE,
    PG_SASL_SCRAM_SHA256,
    PG_SASL_SCRAM_SHA256_PLUS,
    PG_SASL_NO_INITIAL_DATA,
    PgPasswordMessage,
    PgSaslInitialResponse,
    PgSaslResponse,
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

test("pgwire PgPasswordMessage: cleartext and MD5 password roundtrip", fn() {
    let buf: [128]u8 = [0; 128];

    // 1. Cleartext password
    const clear = PgPasswordMessage.init("s3cr3t!");
    expect(!clear.isMd5Prefixed());
    const n1 = clear.encodeFrame(buf[0..]) catch unreachable;
    expect(n1 == clear.encodedLen());
    expect(n1 == 5 + 7 + 1);
    expect(buf[0] == PG_TAG_PASSWORD_MESSAGE);
    expect(buf[n1 - 1] == 0);

    const d1 = PgPasswordMessage.decodeFrame(buf[0..n1]) catch unreachable;
    expect(d1.eql(clear));
    expect(strEql(d1.password, "s3cr3t!"));

    // 2. MD5-prefixed password (35 bytes)
    const md5_pw = PgPasswordMessage.init("md50123456789abcdef0123456789abcdef");
    expect(md5_pw.isMd5Prefixed());
    const n2 = md5_pw.encodeFrame(buf[0..]) catch unreachable;
    const d2 = PgPasswordMessage.decodeFrame(buf[0..n2]) catch unreachable;
    expect(d2.eql(md5_pw));
    expect(d2.isMd5Prefixed());

    // 3. Empty password
    const empty_pw = PgPasswordMessage.init("");
    const n3 = empty_pw.encodeFrame(buf[0..]) catch unreachable;
    expect(n3 == 6);
    const d3 = PgPasswordMessage.decodeFrame(buf[0..n3]) catch unreachable;
    expect(d3.eql(empty_pw));
    expect(d3.password.len == 0);
});

test("pgwire PgSaslInitialResponse: SCRAM-SHA-256 and SCRAM-SHA-256-PLUS roundtrip", fn() {
    let buf: [256]u8 = [0; 256];

    const client_first = "n,,n=,r=rOprNGfwEbeRWgbNEkqO";
    const msg = PgSaslInitialResponse.scramSha256(client_first);
    expect(msg.isScramSha256());
    expect(!msg.isScramSha256Plus());

    const written = msg.encodeFrame(buf[0..]) catch unreachable;
    expect(written == msg.encodedLen());
    expect(written == 5 + PG_SASL_SCRAM_SHA256.len + 1 + 4 + client_first.len);
    expect(buf[0] == PG_TAG_PASSWORD_MESSAGE);

    const decoded = PgSaslInitialResponse.decodeFrame(buf[0..written]) catch unreachable;
    expect(decoded.eql(msg));
    expect(decoded.isScramSha256());
    if (d := decoded.initial_data) {
        expect(strEql(d, client_first));
    } else {
        expect(false);
    }

    // SCRAM-SHA-256-PLUS with empty non-null initial data (data_len == 0)
    const empty_slice = buf[0..0];
    const plus = PgSaslInitialResponse.init(PG_SASL_SCRAM_SHA256_PLUS, empty_slice);
    expect(plus.isScramSha256Plus());
    const n_plus = plus.encodeFrame(buf[0..]) catch unreachable;
    const d_plus = PgSaslInitialResponse.decodeFrame(buf[0..n_plus]) catch unreachable;
    expect(d_plus.eql(plus));
    if (d := d_plus.initial_data) {
        expect(d.len == 0);
    } else {
        expect(false);
    }
});

test("pgwire PgSaslInitialResponse: null initial_data encodes -1 (0xFFFFFFFF)", fn() {
    let buf: [64]u8 = [0; 64];
    const msg = PgSaslInitialResponse.init(PG_SASL_SCRAM_SHA256, null);
    const n = msg.encodeFrame(buf[0..]) catch unreachable;
    expect(n == 5 + PG_SASL_SCRAM_SHA256.len + 1 + 4);
    // Verify last 4 bytes are -1 (0xFFFFFFFF)
    expect(PG_SASL_NO_INITIAL_DATA == -1);
    expect(buf[n - 4] == 0xFF && buf[n - 3] == 0xFF && buf[n - 2] == 0xFF && buf[n - 1] == 0xFF);

    const decoded = PgSaslInitialResponse.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.eql(msg));
    expect(decoded.initial_data == null);
    expect(decoded.isScramSha256());
});

test("pgwire PgSaslResponse: client-final-message and binary/empty payload roundtrip", fn() {
    let buf: [256]u8 = [0; 256];

    const client_final = "c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ=";
    const resp = PgSaslResponse.init(client_final);
    const n = resp.encodeFrame(buf[0..]) catch unreachable;
    expect(n == resp.encodedLen());
    expect(n == 5 + client_final.len);

    const d = PgSaslResponse.decodeFrame(buf[0..n]) catch unreachable;
    expect(d.eql(resp));
    expect(strEql(d.data, client_final));

    // Binary payload with embedded NUL bytes is valid for SASLResponse
    const bin: [4]u8 = [0x00, 0x01, 0x00, 0xFE];
    const bin_resp = PgSaslResponse.init(bin[0..]);
    const n_bin = bin_resp.encodeFrame(buf[0..]) catch unreachable;
    const d_bin = PgSaslResponse.decodeFrame(buf[0..n_bin]) catch unreachable;
    expect(d_bin.eql(bin_resp));

    // Empty SASLResponse (len == 4)
    const empty_resp = PgSaslResponse.init(buf[0..0]);
    const n_empty = empty_resp.encodeFrame(buf[0..]) catch unreachable;
    expect(n_empty == 5);
    const d_empty = PgSaslResponse.decodeFrame(buf[0..n_empty]) catch unreachable;
    expect(d_empty.eql(empty_resp));
    expect(d_empty.data.len == 0);
});

test("pgwire PgPasswordMessage and PgSaslResponse: malformed frame protection", fn() {
    // 1. Wrong tag
    const bad_tag: [6]u8 = ['Q', 0, 0, 0, 5, 0];
    let e1 = false;
    * = PgPasswordMessage.decodeFrame(bad_tag[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Truncated header
    const short_hdr: [4]u8 = ['p', 0, 0, 4];
    let e2 = false;
    * = PgSaslResponse.decodeFrame(short_hdr[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Invalid length (< 4)
    const bad_len: [5]u8 = ['p', 0, 0, 0, 3];
    let e3 = false;
    * = PgSaslResponse.decodeFrame(bad_len[0..]) catch { e3 = true; };
    expect(e3);

    // 4. PasswordMessage missing trailing NUL
    const no_nul: [7]u8 = ['p', 0, 0, 0, 6, 'a', 'b'];
    let e4 = false;
    * = PgPasswordMessage.decodeFrame(no_nul[0..]) catch { e4 = true; };
    expect(e4);

    // 5. PasswordMessage embedded NUL on decode & encode
    const emb_nul: [9]u8 = ['p', 0, 0, 0, 8, 'a', 0, 'b', 0];
    let e5 = false;
    * = PgPasswordMessage.decodeFrame(emb_nul[0..]) catch { e5 = true; };
    expect(e5);

    const bad_pw_bytes: [3]u8 = ['a', 0, 'b'];
    let out: [32]u8 = [0; 32];
    let e6 = false;
    * = PgPasswordMessage.init(bad_pw_bytes[0..]).encodeFrame(out[0..]) catch { e6 = true; };
    expect(e6);

    // 6. Trailing bytes
    const trailing: [7]u8 = ['p', 0, 0, 0, 5, 0, 0x99];
    let e7 = false;
    * = PgPasswordMessage.decodeFrame(trailing[0..]) catch { e7 = true; };
    expect(e7);
});

test("pgwire PgSaslInitialResponse: malformed frames, negative length, and buffer guards", fn() {
    let out: [32]u8 = [0; 32];

    // 1. Empty mechanism rejected on encode
    let e1 = false;
    * = PgSaslInitialResponse.init("", null).encodeFrame(out[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Embedded NUL in mechanism rejected on encode
    const bad_mech: [3]u8 = ['A', 0, 'B'];
    let e2 = false;
    * = PgSaslInitialResponse.init(bad_mech[0..], null).encodeFrame(out[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Buffer too small on encode
    let tiny: [4]u8 = [0; 4];
    let e3 = false;
    * = PgSaslInitialResponse.scramSha256("n,,n=,r=abc").encodeFrame(tiny[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Empty mechanism rejected on decode (first payload byte == 0)
    const empty_mech_frame: [10]u8 = ['p', 0, 0, 0, 9, 0, 0xFF, 0xFF, 0xFF, 0xFF];
    let e4 = false;
    * = PgSaslInitialResponse.decodeFrame(empty_mech_frame[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Invalid negative data_len (-2 = 0xFFFFFFFE) rejected on decode
    const neg2_frame: [11]u8 = ['p', 0, 0, 0, 10, 'X', 0, 0xFF, 0xFF, 0xFF, 0xFE];
    let e5 = false;
    * = PgSaslInitialResponse.decodeFrame(neg2_frame[0..]) catch { e5 = true; };
    expect(e5);

    // 6. data_len == -1 with unexpected trailing bytes rejected
    const neg1_trailing: [12]u8 = ['p', 0, 0, 0, 11, 'X', 0, 0xFF, 0xFF, 0xFF, 0xFF, 0x01];
    let e6 = false;
    * = PgSaslInitialResponse.decodeFrame(neg1_trailing[0..]) catch { e6 = true; };
    expect(e6);

    // 7. Declared data_len larger than remaining bytes rejected
    const trunc_data: [12]u8 = ['p', 0, 0, 0, 11, 'X', 0, 0, 0, 0, 5, 'a'];
    let e7 = false;
    * = PgSaslInitialResponse.decodeFrame(trunc_data[0..]) catch { e7 = true; };
    expect(e7);
});
