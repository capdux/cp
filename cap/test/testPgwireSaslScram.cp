import { test, expect } from "cap:test";
import {
    PG_SCRAM_NONCE_RAW_LEN,
    PG_SCRAM_KEY_LEN,
    PG_SCRAM_SIG_B64_LEN,
    PgSaslStatus,
    PgSaslError,
    escapeScramUsername,
    PgSaslState,
    PgSaslInitialResponse,
    PgSaslResponse,
    PgScramServerFirst,
    PgScramServerFinal,
} from "cap:pgwire";

test("PgSaslState official RFC 7677 Section 3 SCRAM-SHA-256 test vector", () => {
    let sasl = PgSaslState.initWithBase64Nonce("rOprNGfwEbeRWgbNEkqO")?;
    expect(sasl.status == PgSaslStatus.Init).toBe(true);
    expect(sasl.nonce()).toBe("rOprNGfwEbeRWgbNEkqO");

    let c1_buf: [128]u8 = [0; 128];
    const c1_len = sasl.formatClientFirst("user", c1_buf[0..])?;
    expect(c1_buf[0..c1_len]).toBe("n,,n=user,r=rOprNGfwEbeRWgbNEkqO");

    const server_first_raw = "r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096";
    const sf = PgScramServerFirst.parse(server_first_raw)?;
    expect(sf.iterations).toBe(4096);

    const sp_len = sasl.computeSaltedPasswordB64("pencil", sf.salt_b64, sf.iterations)?;
    expect(sp_len).toBe(PG_SCRAM_KEY_LEN);
    expect(sasl.status == PgSaslStatus.Continue).toBe(true);

    const auth_msg = "n=user,r=rOprNGfwEbeRWgbNEkqO,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096,c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0";

    let proof_buf: [64]u8 = [0; 64];
    const proof_len = sasl.computeClientProof(auth_msg, proof_buf[0..])?;
    expect(proof_len).toBe(PG_SCRAM_SIG_B64_LEN);
    expect(proof_buf[0..proof_len]).toBe("dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ=");

    let cf_buf: [256]u8 = [0; 256];
    const cf_len = sasl.formatClientFinal(sf.nonce, auth_msg, cf_buf[0..])?;
    expect(cf_buf[0..cf_len]).toBe(
        "c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ=",
    );

    const expected_srv_sig = sasl.serverSignature()?;
    expect(expected_srv_sig).toBe("6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4=");

    const sfinal = PgScramServerFinal.parse("v=6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4=")?;
    expect(sasl.verifyServerSignature(sfinal.verifier_b64)?).toBe(true);
    expect(sasl.status == PgSaslStatus.Verified).toBe(true);
});

test("PgSaslState raw 18-byte nonce encoding and RFC 5802 username escaping", () => {
    const raw_nonce: [18]u8 = [
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    ];
    expect(raw_nonce.len).toBe(PG_SCRAM_NONCE_RAW_LEN);
    let sasl = PgSaslState.initWithNonceBytes(raw_nonce[0..])?;
    expect(sasl.nonce().len).toBe(24);
    expect(sasl.nonce()).toBe("AAECAwQFBgcICQoLDA0ODxAR");

    let esc_buf: [64]u8 = [0; 64];
    const esc_len = escapeScramUsername("a=b,c=d", esc_buf[0..])?;
    expect(esc_buf[0..esc_len]).toBe("a=3Db=2Cc=3Dd");

    let bare_buf: [128]u8 = [0; 128];
    const bare_len = sasl.formatClientFirstBare("ops=admin,prod", bare_buf[0..])?;
    expect(bare_buf[0..bare_len]).toBe("n=ops=3Dadmin=2Cprod,r=AAECAwQFBgcICQoLDA0ODxAR");
});

test("PgSaslState end-to-end integration with PgSaslInitialResponse and PgSaslResponse wire frames", () => {
    let sasl = PgSaslState.initWithBase64Nonce("rOprNGfwEbeRWgbNEkqO")?;

    let c1_buf: [128]u8 = [0; 128];
    const c1_len = sasl.formatClientFirst("user", c1_buf[0..])?;

    let wire1: [256]u8 = [0; 256];
    const w1_len = PgSaslInitialResponse.scramSha256(c1_buf[0..c1_len]).encodeFrame(wire1[0..])?;
    const decoded1 = PgSaslInitialResponse.decodeFrame(wire1[0..w1_len])?;
    expect(decoded1.isScramSha256()).toBe(true);
    expect(decoded1.initial_data ?? "").toBe("n,,n=user,r=rOprNGfwEbeRWgbNEkqO");

    * = sasl.computeSaltedPasswordB64("pencil", "W22ZaJ0SNY7soEsUEjb6gQ==", 4096)?;
    const auth_msg = "n=user,r=rOprNGfwEbeRWgbNEkqO,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096,c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0";

    let cf_buf: [256]u8 = [0; 256];
    const cf_len = sasl.formatClientFinal(
        "rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0",
        auth_msg,
        cf_buf[0..],
    )?;

    let wire2: [384]u8 = [0; 384];
    const w2_len = PgSaslResponse.init(cf_buf[0..cf_len]).encodeFrame(wire2[0..])?;
    const decoded2 = PgSaslResponse.decodeFrame(wire2[0..w2_len])?;
    expect(decoded2.data).toBe(cf_buf[0..cf_len]);
});

test("PgSaslState security validation and error boundary guards", () => {
    // Invalid nonces (empty, comma, control byte)
    let bad_nonce_count: usize = 0;
    * = PgSaslState.initWithBase64Nonce("") catch |e| {
        if (e == PgSaslError.InvalidNonce) {
            bad_nonce_count += 1;
        }
        PgSaslState{}
    };
    * = PgSaslState.initWithBase64Nonce("abc,def") catch |e| {
        if (e == PgSaslError.InvalidNonce) {
            bad_nonce_count += 1;
        }
        PgSaslState{}
    };
    * = PgSaslState.initWithBase64Nonce("abc def") catch |e| {
        if (e == PgSaslError.InvalidNonce) {
            bad_nonce_count += 1;
        }
        PgSaslState{}
    };
    expect(bad_nonce_count).toBe(3);

    let sasl = PgSaslState.initWithBase64Nonce("testNonce123")?;

    // Calling saltedPassword before computeSaltedPassword fails
    let got_uncomputed = false;
    * = sasl.saltedPassword() catch |e| {
        if (e == PgSaslError.SaltedPasswordNotComputed) {
            got_uncomputed = true;
        }
        ""
    };
    expect(got_uncomputed).toBe(true);

    // Invalid iterations = 0
    let got_bad_iter = false;
    * = sasl.computeSaltedPassword("pw", "salt", 0) catch |e| {
        if (e == PgSaslError.InvalidIterations) {
            got_bad_iter = true;
        }
        0
    };
    expect(got_bad_iter).toBe(true);

    // Invalid base64 salt
    let got_bad_salt = false;
    * = sasl.computeSaltedPasswordB64("pw", "not-valid-b64!", 10) catch |e| {
        if (e == PgSaslError.InvalidSalt) {
            got_bad_salt = true;
        }
        0
    };
    expect(got_bad_salt).toBe(true);

    // Valid derivation, then server signature mismatch detection
    * = sasl.computeSaltedPassword("secret", "0123456789abcdef", 16)?;
    * = sasl.computeServerSignature("auth-msg-sample")?;

    // Duplicate computeServerSignature rejected
    let got_dup_sig = false;
    * = sasl.computeServerSignature("auth-msg-sample") catch |e| {
        if (e == PgSaslError.ServerSignatureAlreadyComputed) {
            got_dup_sig = true;
        }
        0
    };
    expect(got_dup_sig).toBe(true);

    // Tampered server signature rejected
    let got_mismatch = false;
    * = sasl.verifyServerSignature("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=") catch |e| {
        if (e == PgSaslError.ServerSignatureMismatch) {
            got_mismatch = true;
        }
        false
    };
    expect(got_mismatch).toBe(true);
    expect(sasl.status == PgSaslStatus.Continue).toBe(true);
});

test("PgSaslState reset wipes sensitive key buffers and restores Init status", () => {
    let sasl = PgSaslState.initWithBase64Nonce("wipeTestNonce")?;
    * = sasl.computeSaltedPassword("hunter2", "salty-bytes", 8)?;
    * = sasl.computeServerSignature("auth-payload")?;
    const sig = sasl.serverSignature()?;
    expect(sig.len).toBe(44);

    // Verify storedKey & clientKey are derivable before reset
    const sk = sasl.storedKey()?;
    expect(sk.len).toBe(32);

    sasl.reset();
    expect(sasl.status == PgSaslStatus.Init).toBe(true);
    expect(sasl.nonce().len).toBe(0);
    expect(sasl.salted_password_created).toBe(false);
    expect(sasl.server_signature_len).toBe(0);

    let all_zero = true;
    let i: usize = 0;
    while (i < PG_SCRAM_KEY_LEN) {
        if (sasl.salted_password_bytes[i] != 0) {
            all_zero = false;
        }
        i += 1;
    }
    expect(all_zero).toBe(true);
});
