// cap/test/test_sql_pg_auth_challenge_production_deep.cp
// Production-deep regression tests for cap:sql pg_auth_challenge module
// Source: /data/dev/bun/src/sql/postgres/protocol/Authentication.cp & /data/dev/bun/docs/runtime/sql.mdx

import { test, expect } from "cap:test";
import {
    PG_AUTHENTICATION_TAG,
    PG_AUTH_CODE_OK,
    PG_AUTH_CODE_CLEARTEXT,
    PG_AUTH_CODE_MD5,
    PG_AUTH_CODE_SASL,
    PG_AUTH_CODE_SASL_CONTINUE,
    PG_AUTH_CODE_SASL_FINAL,
    PgAuthChallengeError,
    PgAuthChallengeKind,
    parsePgSaslContinue,
    parsePgSaslFinal,
    encodePgAuthOk,
    encodePgAuthCleartext,
    encodePgAuthMd5,
    encodePgAuthSasl,
    encodePgAuthSaslContinue,
    encodePgAuthSaslFinal,
    decodePgAuthChallenge,
} from "cap:sql";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("sql pg_auth_challenge: AuthenticationOk & CleartextPassword round-trip") {
    let wire: [32]u8 = [0; 32];

    const n_ok = encodePgAuthOk(wire[0..]) catch { expect(false); return; };
    expect(n_ok == 9);
    expect(wire[0] == PG_AUTHENTICATION_TAG);
    const v_ok = decodePgAuthChallenge(wire[0..n_ok]) catch { expect(false); return; };
    expect(v_ok.kind == PgAuthChallengeKind.Ok);
    expect(v_ok.raw_code == PG_AUTH_CODE_OK);

    const n_ct = encodePgAuthCleartext(wire[0..]) catch { expect(false); return; };
    expect(n_ct == 9);
    const v_ct = decodePgAuthChallenge(wire[0..n_ct]) catch { expect(false); return; };
    expect(v_ct.kind == PgAuthChallengeKind.ClearTextPassword);
    expect(v_ct.raw_code == PG_AUTH_CODE_CLEARTEXT);
}

test("sql pg_auth_challenge: AuthenticationMD5Password 4-byte salt round-trip") {
    let wire: [32]u8 = [0; 32];
    const salt: [4]u8 = [0xde, 0xad, 0xbe, 0xef];
    const n = encodePgAuthMd5(salt, wire[0..]) catch { expect(false); return; };
    expect(n == 13);

    const v = decodePgAuthChallenge(wire[0..n]) catch { expect(false); return; };
    expect(v.kind == PgAuthChallengeKind.MD5Password);
    expect(v.raw_code == PG_AUTH_CODE_MD5);
    expect(v.md5_salt[0] == 0xde);
    expect(v.md5_salt[1] == 0xad);
    expect(v.md5_salt[2] == 0xbe);
    expect(v.md5_salt[3] == 0xef);
}

test("sql pg_auth_challenge: AuthenticationSASL mechanism list encode & iterator") {
    const mechs: [2]([]const u8) = ["SCRAM-SHA-256", "SCRAM-SHA-256-PLUS"];
    let wire: [128]u8 = [0; 128];
    const n = encodePgAuthSasl(mechs[0..], wire[0..]) catch { expect(false); return; };

    const v = decodePgAuthChallenge(wire[0..n]) catch { expect(false); return; };
    expect(v.kind == PgAuthChallengeKind.SASL);
    expect(v.raw_code == PG_AUTH_CODE_SASL);

    let it = v.saslMechanisms();
    expect(it.contains("SCRAM-SHA-256"));
    expect(it.contains("SCRAM-SHA-256-PLUS"));
    expect(!it.contains("PLAIN"));

    if (m0 := it.next()) { expect(strEq(m0, "SCRAM-SHA-256")); } else { expect(false); }
    if (m1 := it.next()) { expect(strEq(m1, "SCRAM-SHA-256-PLUS")); } else { expect(false); }
    expect(it.next() == null);
}

test("sql pg_auth_challenge: AuthenticationSASLContinue SCRAM-SHA-256 challenge round-trip") {
    let wire: [128]u8 = [0; 128];
    const msg = "r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,i=4096";
    const n = encodePgAuthSaslContinue(msg, wire[0..]) catch { expect(false); return; };

    const v = decodePgAuthChallenge(wire[0..n]) catch { expect(false); return; };
    expect(v.kind == PgAuthChallengeKind.SASLContinue);
    expect(v.raw_code == PG_AUTH_CODE_SASL_CONTINUE);
    expect(strEq(v.sasl_continue.nonce, "fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j"));
    expect(strEq(v.sasl_continue.salt_b64, "QSXCR+Q6sek8bf92"));
    expect(strEq(v.sasl_continue.iterations_raw, "4096"));
    expect(v.sasl_continue.iterations == 4096);

    // Out-of-order attributes + unknown extension attribute still parse cleanly
    const reordered = parsePgSaslContinue("i=10000,x=ext,s=c2FsdA==,r=nonce123") catch {
        expect(false);
        return;
    };
    expect(reordered.iterations == 10000);
    expect(strEq(reordered.nonce, "nonce123"));
    expect(strEq(reordered.salt_b64, "c2FsdA=="));
}

test("sql pg_auth_challenge: AuthenticationSASLFinal verifier & RFC 5802 error") {
    let wire: [128]u8 = [0; 128];

    const n1 = encodePgAuthSaslFinal("v=rmF9pqV8S7suAoZWja4dJRkFsKQ=", wire[0..]) catch { expect(false); return; };
    const v1 = decodePgAuthChallenge(wire[0..n1]) catch { expect(false); return; };
    expect(v1.kind == PgAuthChallengeKind.SASLFinal);
    expect(v1.raw_code == PG_AUTH_CODE_SASL_FINAL);
    expect(strEq(v1.sasl_final.verifier_b64, "rmF9pqV8S7suAoZWja4dJRkFsKQ="));
    expect(v1.sasl_final.error_msg == null);

    const n2 = encodePgAuthSaslFinal("e=invalid-proof", wire[0..]) catch { expect(false); return; };
    const v2 = decodePgAuthChallenge(wire[0..n2]) catch { expect(false); return; };
    expect(v2.kind == PgAuthChallengeKind.SASLFinal);
    if (em := v2.sasl_final.error_msg) {
        expect(strEq(em, "invalid-proof"));
    } else {
        expect(false);
    }
}

test("sql pg_auth_challenge: malformed frame, SCRAM & length invariant guards") {
    // 1. Wrong tag ('E' instead of 'R')
    const bad_tag: [9]u8 = ['E', 0, 0, 0, 8, 0, 0, 0, 0];
    let e1 = false;
    * = decodePgAuthChallenge(bad_tag[0..]) catch { e1 = true; };
    expect(e1);

    // 2. Truncated frame
    const trunc: [5]u8 = ['R', 0, 0, 0, 8];
    let e2 = false;
    * = decodePgAuthChallenge(trunc[0..]) catch { e2 = true; };
    expect(e2);

    // 3. Invalid length for AuthenticationOk (9 instead of 8)
    const bad_ok_len: [10]u8 = ['R', 0, 0, 0, 9, 0, 0, 0, 0, 0];
    let e3 = false;
    * = decodePgAuthChallenge(bad_ok_len[0..]) catch { e3 = true; };
    expect(e3);

    // 4. Invalid length for MD5Password (10 instead of 12)
    const bad_md5: [11]u8 = ['R', 0, 0, 0, 10, 0, 0, 0, 5, 1, 2];
    let e4 = false;
    * = decodePgAuthChallenge(bad_md5[0..]) catch { e4 = true; };
    expect(e4);

    // 5. Trailing garbage byte
    const trailing: [10]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 0, 0xff];
    let e5 = false;
    * = decodePgAuthChallenge(trailing[0..]) catch { e5 = true; };
    expect(e5);

    // 6. Malformed SASLContinue: missing 's', i=0, non-digit i, u32 overflow
    let s1 = false;
    * = parsePgSaslContinue("r=abc,i=4096") catch { s1 = true; };
    expect(s1);
    let s2 = false;
    * = parsePgSaslContinue("r=abc,s=xyz,i=0") catch { s2 = true; };
    expect(s2);
    let s3 = false;
    * = parsePgSaslContinue("r=abc,s=xyz,i=409x") catch { s3 = true; };
    expect(s3);
    let s4 = false;
    * = parsePgSaslContinue("r=abc,s=xyz,i=99999999999") catch { s4 = true; };
    expect(s4);

    // 7. Malformed SASLFinal & BufferTooSmall
    let f1 = false;
    * = parsePgSaslFinal("x=bad") catch { f1 = true; };
    expect(f1);
    let tiny: [4]u8 = [0; 4];
    let b1 = false;
    * = encodePgAuthOk(tiny[0..]) catch { b1 = true; };
    expect(b1);
}

test("sql pg_auth_challenge: unknown auth code & GSSContinue payload passthrough") {
    // Unknown auth code 99
    const unk: [9]u8 = ['R', 0, 0, 0, 8, 0, 0, 0, 99];
    const vu = decodePgAuthChallenge(unk[0..]) catch { expect(false); return; };
    expect(vu.kind == PgAuthChallengeKind.Unknown);
    expect(vu.raw_code == 99);

    // GSSContinue (code 8) with 3-byte token
    const gss: [12]u8 = ['R', 0, 0, 0, 11, 0, 0, 0, 8, 0xaa, 0xbb, 0xcc];
    const vg = decodePgAuthChallenge(gss[0..]) catch { expect(false); return; };
    expect(vg.kind == PgAuthChallengeKind.GSSContinue);
    expect(vg.data.len == 3);
    expect(vg.data[0] == 0xaa && vg.data[2] == 0xcc);
}
