// Source: /data/dev/bun/src/boringssl/boringssl.cp & /data/dev/bun/src/sha_hmac/hmac.cp & Custom RFC tests
// cap/test/test_crypto_production_deep.cp
// Deep Production Cryptography Test Suite for Upgraded cap:crypto

import { test, expect_eq } from "cap:test";
import {
    Ed25519KeyPair,
    X25519KeyPair,
    EcdsaP256KeyPair,
    hkdfExtract,
    hkdfExpand,
    hkdfDerive,
    hkdfSha512Derive,
    scryptDerive,
    argon2idDerive,
    argon2idHash,
    argon2idVerify,
    Blake3Stream,
    Blake3,
    blake3,
    Sha384,
    Sha3_256,
    Sha3_512,
    sha384,
    sha3_256,
    sha3_512,
    HmacSha384,
    HmacSha512,
    hmacSha384,
    XChaCha20Poly1305,
    Aes128Gcm,
    Jwt,
    Pem,
    base64UrlEncode,
    bytesToHex,
} from "cap:crypto";

fn testEd25519SignAndTamperReject(): !void {
    // 1. Random keypair sign & verify
    const kp = Ed25519KeyPair.generate();
    const msg = "Authenticated BEAM Agent Payload v1";
    const sig = kp.sign(msg);
    expect_eq(Ed25519KeyPair.verify(&sig, msg, &kp.public_key), true);
    expect_eq(Ed25519KeyPair.verifyStrict(&sig, msg, &kp.public_key), true);

    // 2. Tamper message rejection
    expect_eq(Ed25519KeyPair.verify(&sig, "Authenticated BEAM Agent Payload v2", &kp.public_key), false);

    // 3. Tamper signature rejection
    let bad_sig = sig;
    bad_sig[0] ^= 0x01;
    expect_eq(Ed25519KeyPair.verify(&bad_sig, msg, &kp.public_key), false);

    // 4. Wrong public key rejection
    const kp2 = Ed25519KeyPair.generate();
    expect_eq(Ed25519KeyPair.verify(&sig, msg, &kp2.public_key), false);

    // 5. Deterministic fromSeed reproducibility
    let seed: [32]u8 = [0x9d; 32];
    const det1 = Ed25519KeyPair.fromSeed(&seed).?;
    const det2 = Ed25519KeyPair.fromSeed(&seed).?;
    expect_eq(std.mem.eql(u8, &det1.public_key, &det2.public_key), true);
    const det_sig = det1.sign("deterministic-msg");
    expect_eq(Ed25519KeyPair.verify(&det_sig, "deterministic-msg", &det2.public_key), true);
}

fn testX25519KeyExchange(): !void {
    // 1. Bidirectional Ephemeral ECDH Agreement
    const alice = X25519KeyPair.generate();
    const bob = X25519KeyPair.generate();

    const alice_shared = alice.computeShared(&bob.public_key).?;
    const bob_shared = bob.computeShared(&alice.public_key).?;
    expect_eq(std.mem.eql(u8, &alice_shared, &bob_shared), true);

    const static_shared = X25519KeyPair.sharedSecret(&alice.secret_key, &bob.public_key).?;
    expect_eq(std.mem.eql(u8, &alice_shared, &static_shared), true);

    // 2. RFC 7748 Section 6.1 Test Vector 1
    const rfc_sk: [32]u8 = [
        0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d,
        0x3b, 0x16, 0x15, 0x4b, 0x82, 0x46, 0x5e, 0xdd,
        0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc, 0x5a, 0x18,
        0x50, 0x6a, 0x22, 0x44, 0xba, 0x44, 0x9a, 0xc4,
    ];
    const rfc_pk: [32]u8 = [
        0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb,
        0x35, 0x94, 0xc1, 0xa4, 0x24, 0xb1, 0x5f, 0x7c,
        0x72, 0x66, 0x24, 0xec, 0x26, 0xb3, 0x35, 0x3b,
        0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab, 0x1c, 0x4c,
    ];
    const shared_vec = X25519KeyPair.sharedSecret(&rfc_sk, &rfc_pk).?;
    expect_eq(
        bytesToHex(&shared_vec),
        "c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552",
    );

    // 3. Reject low-order all-zero public key
    const zero_pk: [32]u8 = [0; 32];
    expect_eq(alice.computeShared(&zero_pk) == null, true);
}

fn testEcdsaP256SignAndVerify(): !void {
    const kp = EcdsaP256KeyPair.generate();
    const msg = "ECDSA P-256 NIST Curve Transaction";
    const sig = kp.sign(msg);

    // Verify with both compressed (33B) and uncompressed (65B) SEC-1 public keys
    expect_eq(EcdsaP256KeyPair.verify(&sig, msg, &kp.public_key), true);
    expect_eq(EcdsaP256KeyPair.verify(&sig, msg, &kp.uncompressed_public_key), true);

    // DER signature round-trip
    const der_sig = kp.signDer(msg);
    expect_eq(EcdsaP256KeyPair.verifyDer(der_sig, msg, &kp.public_key), true);

    // Tampered message & tampered signature must fail
    expect_eq(EcdsaP256KeyPair.verify(&sig, "ECDSA P-256 Tampered", &kp.public_key), false);
    let bad_sig = sig;
    bad_sig[10] ^= 0x80;
    expect_eq(EcdsaP256KeyPair.verify(&bad_sig, msg, &kp.public_key), false);

    // Wrong key must fail
    const other = EcdsaP256KeyPair.generate();
    expect_eq(EcdsaP256KeyPair.verify(&sig, msg, &other.public_key), false);
}

fn testHkdfRfc5869AndSha512(): !void {
    // RFC 5869 Appendix A.1 Test Case 1 (SHA-256)
    const ikm: [22]u8 = [0x0b; 22];
    const salt: [13]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c];
    const info: [10]u8 = [0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9];

    const prk = hkdfExtract(&salt, &ikm);
    expect_eq(
        bytesToHex(&prk),
        "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
    );

    let okm: [42]u8 = undefined;
    expect_eq(hkdfExpand(&okm, &info, &prk), true);
    expect_eq(
        bytesToHex(&okm),
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865",
    );

    let okm_one_shot: [42]u8 = undefined;
    expect_eq(hkdfDerive(&okm_one_shot, &ikm, &salt, &info), true);
    expect_eq(std.mem.eql(u8, &okm, &okm_one_shot), true);

    // HKDF-SHA512 derive check
    let okm512: [64]u8 = undefined;
    expect_eq(hkdfSha512Derive(&okm512, "input-key", "salt-512", "context-info"), true);
    expect_eq(okm512.len, 64 as usize);
}

fn testScryptAndArgon2id(): !void {
    // 1. Scrypt RFC 7914 Test Vector 1: password="", salt="", N=16 (log2_n=4), r=1, p=1
    let scrypt_out: [64]u8 = undefined;
    expect_eq(scryptDerive(&scrypt_out, "", "", 4, 1, 1), true);
    expect_eq(
        bytesToHex(&scrypt_out),
        "77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906",
    );

    // 2. Argon2id raw KDF deterministic derivation (t=2, m=64 KiB, p=1)
    let a2_out1: [32]u8 = undefined;
    let a2_out2: [32]u8 = undefined;
    expect_eq(argon2idDerive(&a2_out1, "hunter2", "somesalt1234", 2, 64, 1), true);
    expect_eq(argon2idDerive(&a2_out2, "hunter2", "somesalt1234", 2, 64, 1), true);
    expect_eq(std.mem.eql(u8, &a2_out1, &a2_out2), true);

    // 3. Argon2id PHC strHash & strVerify
    const phc = argon2idHash("correct horse battery staple", 1, 64, 1).?;
    expect_eq(std.mem.startsWith(u8, phc, "$argon2id$"), true);
    expect_eq(argon2idVerify(phc, "correct horse battery staple"), true);
    expect_eq(argon2idVerify(phc, "wrong horse battery staple"), false);
}

fn testBlake3Sha3AndHmac(): !void {
    // 1. BLAKE3 official test vectors & streaming consistency
    expect_eq(
        blake3(""),
        "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262",
    );
    expect_eq(
        Blake3.hash("abc"),
        "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85",
    );

    const payload = "The quick brown fox jumps over the lazy dog";
    const b3_oneshot = blake3(payload);
    let b3_stream = Blake3Stream.init();
    b3_stream.update("The quick brown ");
    b3_stream.update("fox jumps over ");
    b3_stream.update("the lazy dog");
    expect_eq(b3_stream.digest(), b3_oneshot);

    const b3_key: [32]u8 = [0x5a; 32];
    const b3_mac = Blake3.keyed(&b3_key, payload).?;
    let b3_mac_stream = Blake3Stream.initKeyed(&b3_key).?;
    b3_mac_stream.update(payload);
    expect_eq(b3_mac_stream.digest(), b3_mac);

    // 2. SHA-384, SHA3-256 & SHA3-512 NIST vectors
    expect_eq(
        sha384("abc"),
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7",
    );
    expect_eq(
        sha3_256(""),
        "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a",
    );
    expect_eq(
        sha3_256("abc"),
        "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532",
    );
    expect_eq(
        sha3_512("abc"),
        "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e10e116e9192af3c91a7ec57647e3934057340b4cf408d5a56592f8274eec53f0",
    );

    // Streaming SHA3-256 consistency
    let s3 = Sha3_256.init();
    s3.update("ab");
    s3.update("c");
    expect_eq(s3.digest(), sha3_256("abc"));

    // 3. HMAC-SHA384 & HMAC-SHA512 streaming consistency
    const mac384 = hmacSha384("k384", "hello world");
    expect_eq(mac384.len, 96 as usize);
    let smac384 = HmacSha384.init("k384");
    smac384.update("hello ");
    smac384.update("world");
    expect_eq(smac384.digest(), mac384);

    const mac512 = HmacSha512.create("k512", "hello world");
    expect_eq(mac512.len, 128 as usize);
    let smac512 = HmacSha512.init("k512");
    smac512.update("hello ");
    smac512.update("world");
    expect_eq(smac512.digest(), mac512);
}

fn testXChaChaAndAes128Gcm(): !void {
    const pt = "24-Byte Nonce Extended AEAD Test!";
    const aad = "hdr:v1";

    // 1. XChaCha20-Poly1305 (32B key, 24B nonce)
    let xk: [32]u8 = [0x11; 32];
    let xn: [24]u8 = [0x22; 24];
    let xct: [33]u8 = undefined;
    let xtag: [16]u8 = undefined;
    let xpt: [33]u8 = undefined;

    XChaCha20Poly1305.encrypt(&xk, &xn, pt, aad, &xct, &xtag);
    expect_eq(XChaCha20Poly1305.decrypt(&xk, &xn, &xct, xtag, aad, &xpt), true);
    expect_eq(xpt[0..33], pt);

    // Tamper ciphertext & AAD must reject
    let xct_bad = xct;
    xct_bad[0] ^= 0x01;
    expect_eq(XChaCha20Poly1305.decrypt(&xk, &xn, &xct_bad, xtag, aad, &xpt), false);
    expect_eq(XChaCha20Poly1305.decrypt(&xk, &xn, &xct, xtag, "hdr:v2", &xpt), false);

    // 2. AES-128-GCM (16B key, 12B nonce)
    let ak: [16]u8 = [0x33; 16];
    let an: [12]u8 = [0x44; 12];
    let act: [33]u8 = undefined;
    let atag: [16]u8 = undefined;
    let apt: [33]u8 = undefined;

    Aes128Gcm.encrypt(&ak, &an, pt, aad, &act, &atag);
    expect_eq(Aes128Gcm.decrypt(&ak, &an, &act, atag, aad, &apt), true);
    expect_eq(apt[0..33], pt);

    // Tamper tag must reject
    let atag_bad = atag;
    atag_bad[15] ^= 0xFF;
    expect_eq(Aes128Gcm.decrypt(&ak, &an, &act, atag_bad, aad, &apt), false);
}

fn testJwtAndPem(): !void {
    const payload = "{\"sub\":\"agent-01\",\"role\":\"admin\"}";
    const secret = "super-secret-jwt-key-256";

    // 1. JWT HS256 & HS512 sign & verify
    const tok256 = Jwt.signHs256(payload, secret);
    const ver256 = Jwt.verifyHs256(tok256, secret).?;
    expect_eq(ver256.alg, "HS256");
    expect_eq(ver256.payload_json, payload);
    expect_eq(Jwt.verifyHs256(tok256, "wrong-secret") == null, true);

    const tok384 = Jwt.signHs384(payload, secret);
    const ver384 = Jwt.verifyHs384(tok384, secret).?;
    expect_eq(ver384.alg, "HS384");
    expect_eq(ver384.payload_json, payload);

    const tok512 = Jwt.signHs512(payload, secret);
    const ver512 = Jwt.verifyHs512(tok512, secret).?;
    expect_eq(ver512.alg, "HS512");
    expect_eq(ver512.payload_json, payload);

    // Cross-algorithm confusion rejection
    expect_eq(Jwt.verifyHs256(tok512, secret) == null, true);

    // 2. JWT EdDSA (Ed25519) sign, verify & tamper rejection
    const kp = Ed25519KeyPair.generate();
    const tok_ed = Jwt.signEdDsa(payload, kp);
    const ver_ed = Jwt.verifyEdDsa(tok_ed, &kp.public_key).?;
    expect_eq(ver_ed.alg, "EdDSA");
    expect_eq(ver_ed.payload_json, payload);

    // Tamper 1 char in payload section
    const dot1 = std.mem.indexOfScalar(u8, tok_ed, '.').?;
    let tampered_ed = std.heap.page_allocator.alloc(u8, tok_ed.len) catch unreachable;
    @memcpy(tampered_ed, tok_ed);
    tampered_ed[dot1 + 2] = if (tampered_ed[dot1 + 2] == 'A') 'B' else 'A';
    expect_eq(Jwt.verifyEdDsa(tampered_ed, &kp.public_key) == null, true);

    // 3. Anti-"alg: none" downgrade rejection
    const none_hdr = base64UrlEncode("{\"alg\":\"none\",\"typ\":\"JWT\"}");
    const none_pay = base64UrlEncode(payload);
    const fake_sig = base64UrlEncode("forged");
    const none_tok = `${none_hdr}.${none_pay}.${fake_sig}`;
    expect_eq(Jwt.verifyHs256(none_tok, secret) == null, true);
    expect_eq(Jwt.verifyHs512(none_tok, secret) == null, true);
    expect_eq(Jwt.verifyEdDsa(none_tok, &kp.public_key) == null, true);

    // 4. RFC 7468 PEM encode/decode round-trip (72 bytes -> >64 base64 chars -> multi-line wrap)
    let der: [72]u8 = undefined;
    for (i of 0..72) {
        der[i] = i as u8;
    }
    const pem_str = Pem.encode("PUBLIC KEY", &der);
    expect_eq(std.mem.startsWith(u8, pem_str, "-----BEGIN PUBLIC KEY-----\n"), true);
    expect_eq(std.mem.endsWith(u8, pem_str, "-----END PUBLIC KEY-----\n"), true);

    const decoded_pem = Pem.decode(pem_str).?;
    expect_eq(decoded_pem.label, "PUBLIC KEY");
    expect_eq(std.mem.eql(u8, decoded_pem.data, &der), true);

    // Mismatched BEGIN/END labels must reject
    const bad_pem = "-----BEGIN PUBLIC KEY-----\nAAAA\n-----END PRIVATE KEY-----\n";
    expect_eq(Pem.decode(bad_pem) == null, true);
}

fn main() {
    test("1. Ed25519 Digital Signatures, Deterministic Seed & Tamper Rejection", () => {
        testEd25519SignAndTamperReject()?;
    });
    test("2. X25519 ECDH Key Exchange & RFC 7748 Test Vector", () => {
        testX25519KeyExchange()?;
    });
    test("3. ECDSA P-256 (SHA-256) Raw & DER Signatures + Tamper Rejection", () => {
        testEcdsaP256SignAndVerify()?;
    });
    test("4. HKDF-SHA256 (RFC 5869) & HKDF-SHA512 Key Derivation", () => {
        testHkdfRfc5869AndSha512()?;
    });
    test("5. Scrypt (RFC 7914) & Argon2id (RFC 9106 / PHC) Password Hashing", () => {
        testScryptAndArgon2id()?;
    });
    test("6. BLAKE3 (One-Shot + Streaming), SHA-384, SHA3-256/512 & HMAC-SHA384/512", () => {
        testBlake3Sha3AndHmac()?;
    });
    test("7. XChaCha20-Poly1305 (24B Nonce) & AES-128-GCM AEAD + Tamper Rejection", () => {
        testXChaChaAndAes128Gcm()?;
    });
    test("8. RFC 7519 JWT (HS256/HS512/EdDSA, Anti-alg:none) & RFC 7468 PEM Codec", () => {
        testJwtAndPem()?;
    });
}
