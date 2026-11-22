// Production deep test suite for cap:pgwire Backend Authentication ('R') challenge codec
// and zero-copy SCRAM-SHA-256 server-first / server-final / SASL mechanism list parsing.

import { test, expect } from "cap:test";
import {
    PG_TAG_AUTHENTICATION,
    PG_AUTH_HEADER_LEN,
    PG_AUTH_CODE_OK,
    PG_AUTH_CODE_MD5,
    PG_AUTH_CODE_SASL,
    PG_AUTH_CODE_SASL_CONTINUE,
    PG_AUTH_CODE_SASL_FINAL,
    PgAuthChallengeKind,
    PgScramServerFirst,
    PgScramServerFinal,
    PgAuthChallenge,
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

test("pgwire Authentication: Ok, CleartextPassword, MD5Password roundtrip", fn() {
    let buf: [64]u8 = [0; 64];

    // 1. AuthenticationOk (0)
    const ok_msg = PgAuthChallenge.ok();
    expect(ok_msg.isOk() && !ok_msg.requiresPassword() && !ok_msg.isSaslStep());
    const n_ok = ok_msg.encodeFrame(buf[0..]) catch unreachable;
    expect(n_ok == PG_AUTH_HEADER_LEN);
    expect(buf[0] == PG_TAG_AUTHENTICATION);
    const d_ok = PgAuthChallenge.decodeFrame(buf[0..n_ok]) catch unreachable;
    expect(d_ok.kind == PgAuthChallengeKind.Ok && d_ok.auth_code == PG_AUTH_CODE_OK);

    // 2. AuthenticationCleartextPassword (3)
    const clear = PgAuthChallenge.cleartext();
    expect(clear.requiresPassword());
    const n_clear = clear.encodeFrame(buf[0..]) catch unreachable;
    expect(n_clear == 9);
    const d_clear = PgAuthChallenge.decodeFrame(buf[0..n_clear]) catch unreachable;
    expect(d_clear.kind == PgAuthChallengeKind.CleartextPassword);

    // 3. AuthenticationMD5Password (5) with 4-byte salt
    const salt: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
    const md5 = PgAuthChallenge.md5(salt);
    expect(md5.requiresPassword());
    const n_md5 = md5.encodeFrame(buf[0..]) catch unreachable;
    expect(n_md5 == 13);
    const d_md5 = PgAuthChallenge.decodeFrame(buf[0..n_md5]) catch unreachable;
    expect(d_md5.kind == PgAuthChallengeKind.Md5Password && d_md5.auth_code == PG_AUTH_CODE_MD5);
    expect(d_md5.md5_salt[0] == 0xDE && d_md5.md5_salt[1] == 0xAD && d_md5.md5_salt[2] == 0xBE && d_md5.md5_salt[3] == 0xEF);
});

test("pgwire Authentication: SASL mechanism list negotiation roundtrip", fn() {
    let buf: [128]u8 = [0; 128];
    const mechs: [2][]const u8 = ["SCRAM-SHA-256", "SCRAM-SHA-256-PLUS"];
    const sasl = PgAuthChallenge.sasl(mechs[0..]) catch unreachable;
    expect(sasl.isSaslStep());

    const n = sasl.encodeFrame(buf[0..]) catch unreachable;
    expect(n == sasl.encodedLen());

    const decoded = PgAuthChallenge.decodeFrame(buf[0..n]) catch unreachable;
    expect(decoded.kind == PgAuthChallengeKind.Sasl && decoded.auth_code == PG_AUTH_CODE_SASL);
    expect(decoded.sasl_mechanisms.count() == 2);
    expect(decoded.sasl_mechanisms.supportsScramSha256());
    expect(decoded.sasl_mechanisms.supportsScramSha256Plus());
    expect(!decoded.sasl_mechanisms.supports("PLAIN"));
    if (m0 := decoded.sasl_mechanisms.at(0)) { expect(strEql(m0, "SCRAM-SHA-256")); } else { expect(false); }
    if (m1 := decoded.sasl_mechanisms.at(1)) { expect(strEql(m1, "SCRAM-SHA-256-PLUS")); } else { expect(false); }
    expect(decoded.sasl_mechanisms.at(2) == null);
});

test("pgwire Authentication: SASLContinue (SCRAM server-first) and SASLFinal (server-final)", fn() {
    let buf: [256]u8 = [0; 256];

    // 1. AuthenticationSASLContinue (11)
    const server_first = "r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096";
    const cont = PgAuthChallenge.saslContinue(server_first) catch unreachable;
    expect(cont.isSaslStep());
    const n1 = cont.encodeFrame(buf[0..]) catch unreachable;
    const d1 = PgAuthChallenge.decodeFrame(buf[0..n1]) catch unreachable;
    expect(d1.kind == PgAuthChallengeKind.SaslContinue && d1.auth_code == PG_AUTH_CODE_SASL_CONTINUE);
    expect(d1.scram_first.eql(cont.scram_first));
    expect(strEql(d1.scram_first.nonce, "rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0"));
    expect(strEql(d1.scram_first.salt_b64, "W22ZaJ0SNY7soEsUEjb6gQ=="));
    expect(d1.scram_first.iterations == 4096);

    // 2. AuthenticationSASLFinal (12) — verified signature
    const server_final_ok = "v=6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4=";
    const fin = PgAuthChallenge.saslFinal(server_final_ok);
    expect(fin.isSaslStep());
    const n2 = fin.encodeFrame(buf[0..]) catch unreachable;
    const d2 = PgAuthChallenge.decodeFrame(buf[0..n2]) catch unreachable;
    expect(d2.kind == PgAuthChallengeKind.SaslFinal && d2.auth_code == PG_AUTH_CODE_SASL_FINAL);
    const parsed_fin = d2.parseServerFinal() catch unreachable;
    expect(parsed_fin.isVerified());
    if (v := parsed_fin.verifier_b64) {
        expect(strEql(v, "6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4="));
    } else {
        expect(false);
    }

    // 3. AuthenticationSASLFinal — server error
    const err_fin = PgScramServerFinal.parse("e=invalid-proof") catch unreachable;
    expect(!err_fin.isVerified());
    if (em := err_fin.err_msg) {
        expect(strEql(em, "invalid-proof"));
    } else {
        expect(false);
    }
});

test("pgwire Authentication: GSS, GSSContinue, KerberosV5, SSPI, and unknown auth code", fn() {
    let buf: [64]u8 = [0; 64];

    // GSSContinue (8)
    const token: [4]u8 = [0x60, 0x42, 0x06, 0x09];
    const gss_c = PgAuthChallenge.gssContinue(token[0..]);
    const n = gss_c.encodeFrame(buf[0..]) catch unreachable;
    const d_gss = PgAuthChallenge.decodeFrame(buf[0..n]) catch unreachable;
    expect(d_gss.kind == PgAuthChallengeKind.GssContinue);
    expect(strEql(d_gss.data, token[0..]));

    // KerberosV5 (2), GSS (7), SSPI (9)
    const krb: [9]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 2];
    const d_krb = PgAuthChallenge.decodeFrame(krb[0..]) catch unreachable;
    expect(d_krb.kind == PgAuthChallengeKind.KerberosV5);

    const gss: [9]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 7];
    const d_g = PgAuthChallenge.decodeFrame(gss[0..]) catch unreachable;
    expect(d_g.kind == PgAuthChallengeKind.Gss);

    const sspi: [9]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 9];
    const d_s = PgAuthChallenge.decodeFrame(sspi[0..]) catch unreachable;
    expect(d_s.kind == PgAuthChallengeKind.Sspi);

    // Unknown future auth code (99)
    const unk: [11]u8 = ['R', 0, 0, 0, 10, 0, 0, 0, 99, 0xAA, 0xBB];
    const d_u = PgAuthChallenge.decodeFrame(unk[0..]) catch unreachable;
    expect(d_u.kind == PgAuthChallengeKind.Unknown && d_u.auth_code == 99 && d_u.data.len == 2);
});

test("pgwire Authentication: malformed frame header and per-type length guards", fn() {
    // 1. Truncated (< 9B)
    const short_f: [8]u8 = ['R', 0, 0, 0, 8, 0, 0, 0];
    let e1 = false;
    * = PgAuthChallenge.decodeFrame(short_f[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Wrong tag
    const bad_tag: [9]u8 = ['E', 0, 0, 0, 8, 0, 0, 0, 0];
    let e2 = false;
    * = PgAuthChallenge.decodeFrame(bad_tag[0..]) catch { e2 = true; };
    expect(e2);

    // 3. AuthenticationOk with extra payload (len != 8)
    const ok_extra: [10]u8 = ['R', 0, 0, 0, 9, 0, 0, 0, 0, 0];
    let e3 = false;
    * = PgAuthChallenge.decodeFrame(ok_extra[0..]) catch { e3 = true; };
    expect(e3);

    // 4. AuthenticationMD5Password with wrong length (len != 12)
    const md5_short: [11]u8 = ['R', 0, 0, 0, 10, 0, 0, 0, 5, 1, 2];
    let e4 = false;
    * = PgAuthChallenge.decodeFrame(md5_short[0..]) catch { e4 = true; };
    expect(e4);

    // 5. AuthenticationSASL missing trailing NUL terminator
    const sasl_no_nul: [12]u8 = ['R', 0, 0, 0, 11, 0, 0, 0, 10, 'A', 'B', 0];
    let e5 = false;
    * = PgAuthChallenge.decodeFrame(sasl_no_nul[0..]) catch { e5 = true; };
    expect(e5);

    // 6. Buffer too small on encode
    let tiny: [4]u8 = [0; 4];
    let e6 = false;
    * = PgAuthChallenge.ok().encodeFrame(tiny[0..]) catch { e6 = true; };
    expect(e6);
});

test("pgwire Authentication: invalid SCRAM server-first / server-final rejection", fn() {
    // Missing 'i'
    let e1 = false;
    * = PgScramServerFirst.parse("r=abc,s=def") catch { e1 = true; };
    expect(e1);

    // Zero iterations rejected
    let e2 = false;
    * = PgScramServerFirst.parse("r=abc,s=def,i=0") catch { e2 = true; };
    expect(e2);

    // Non-numeric iterations rejected
    let e3 = false;
    * = PgScramServerFirst.parse("r=abc,s=def,i=4096x") catch { e3 = true; };
    expect(e3);

    // Invalid server-final without v= or e=
    let e4 = false;
    * = PgScramServerFinal.parse("x=unknown") catch { e4 = true; };
    expect(e4);

    // Calling parseServerFinal on non-SaslFinal challenge fails
    let e5 = false;
    * = PgAuthChallenge.ok().parseServerFinal() catch { e5 = true; };
    expect(e5);
});
