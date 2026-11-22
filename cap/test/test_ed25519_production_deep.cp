// Source: native-cp-stdlib (RFC 8032 Ed25519 Signatures)

import { test, expect_eq } from "cap:test";
import {
    Ed25519Error,
    PublicKey,
    SecretKey,
    Signature,
    KeyPair,
    verify,
    verifyBatch,
    toHex,
    fromHex,
} from "../../lib/crypto/ed25519_.cp";

fn checkRfcVector(seed_hex: []const u8, expected_pk_hex: []const u8, msg: []const u8, expected_sig_hex: []const u8): bool {
    let seed_buf: [32]u8 = undefined;
    * = fromHex(seed_hex, &seed_buf) catch return false;

    const kp = KeyPair.fromSeed(seed_buf) catch return false;

    let pk_hex_buf: [64]u8 = undefined;
    kp.public_key.toHex(&pk_hex_buf);
    if (!equalBytes(&pk_hex_buf, expected_pk_hex)) {
        return false;
    }

    const sig = kp.sign(msg) catch return false;
    let sig_hex_buf: [128]u8 = undefined;
    sig.toHex(&sig_hex_buf);
    if (!equalBytes(&sig_hex_buf, expected_sig_hex)) {
        return false;
    }

    if (!verify(kp.public_key, msg, sig)) {
        return false;
    }
    if (!kp.public_key.verify(msg, sig)) {
        return false;
    }
    if (!sig.verify(msg, kp.public_key)) {
        return false;
    }
    return true;
}

fn equalBytes(a: []const u8, b: []const u8): bool {
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

test "RFC 8032 Section 7.1 TEST 1, TEST 2, TEST 3 official vectors" {
    // TEST 1: empty message
    const ok1 = checkRfcVector(
        "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
        "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a",
        "",
        "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b",
    );
    expect_eq(ok1, true);

    // TEST 2: 1-byte message 0x72
    const msg2: [1]u8 = [0x72];
    const ok2 = checkRfcVector(
        "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
        "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c",
        &msg2,
        "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00",
    );
    expect_eq(ok2, true);

    // TEST 3: 2-byte message 0xaf82
    const msg3: [2]u8 = [0xaf, 0x82];
    const ok3 = checkRfcVector(
        "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7",
        "fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025",
        &msg3,
        "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a",
    );
    expect_eq(ok3, true);
}

test "random key generation and sign/verify roundtrip" {
    const kp1 = try KeyPair.generate();
    const kp2 = try KeyPair.generate();

    const msg = "CP native Ed25519 production test payload";
    const sig1 = try kp1.sign(msg);
    const sig1_rand = try kp1.signRandomized(msg);

    expect_eq(verify(kp1.public_key, msg, sig1), true);
    expect_eq(verify(kp1.public_key, msg, sig1_rand), true);

    // Roundtrip via fromHex / fromSlice
    let pk_hex: [64]u8 = undefined;
    kp1.public_key.toHex(&pk_hex);
    const pk_parsed = try PublicKey.fromHex(&pk_hex);
    expect_eq(equalBytes(&pk_parsed.toBytes(), &kp1.public_key.toBytes()), true);

    let sig_hex: [128]u8 = undefined;
    sig1.toHex(&sig_hex);
    const sig_parsed = try Signature.fromHex(&sig_hex);
    expect_eq(verify(pk_parsed, msg, sig_parsed), true);

    // Distinct generated keys
    expect_eq(equalBytes(&kp1.public_key.toBytes(), &kp2.public_key.toBytes()), false);
}

test "tampered message, tampered signature, and wrong public key rejection" {
    const kp1 = try KeyPair.generate();
    const kp2 = try KeyPair.generate();

    const msg = "authentic message";
    const sig = try kp1.sign(msg);

    // Tampered message
    expect_eq(verify(kp1.public_key, "tampered message!", sig), false);

    // Tampered signature
    let bad_sig_bytes = sig.toBytes();
    bad_sig_bytes[0] = bad_sig_bytes[0] ^ 0xff;
    const bad_sig = Signature.fromBytes(bad_sig_bytes);
    expect_eq(verify(kp1.public_key, msg, bad_sig), false);

    // Wrong public key
    expect_eq(verify(kp2.public_key, msg, sig), false);
}

test "verifyBatch passes when all valid and fails on single tampered item" {
    const kp1 = try KeyPair.generate();
    const kp2 = try KeyPair.generate();
    const kp3 = try KeyPair.generate();

    const m1 = "batch item 1";
    const m2 = "batch item 2";
    const m3 = "batch item 3";

    const s1 = try kp1.sign(m1);
    const s2 = try kp2.sign(m2);
    const s3 = try kp3.sign(m3);

    const pks: [3]PublicKey = [kp1.public_key, kp2.public_key, kp3.public_key];
    const msgs: [3][]const u8 = [m1, m2, m3];
    const sigs: [3]Signature = [s1, s2, s3];

    expect_eq(verifyBatch(&pks, &msgs, &sigs), true);

    // Tamper one signature
    let bad_s2_bytes = s2.toBytes();
    bad_s2_bytes[31] = bad_s2_bytes[31] ^ 0x01;
    let bad_sigs: [3]Signature = [s1, Signature.fromBytes(bad_s2_bytes), s3];
    expect_eq(verifyBatch(&pks, &msgs, &bad_sigs), false);

    // Mismatched slice lengths
    const short_pks: [2]PublicKey = [kp1.public_key, kp2.public_key];
    expect_eq(verifyBatch(&short_pks, &msgs, &sigs), false);
}

test "cap.ed25519 dynamic mounting and zero-heap loop verification" {
    const kp = try KeyPair.generate();
    const msg = "dynamic mount payload";

    let i: usize = 0;
    while (i < 50) {
        const sig = try kp.sign(msg);
        expect_eq(verify(kp.public_key, msg, sig), true);
        i += 1;
    }

    let raw: [4]u8 = [0xde, 0xad, 0xbe, 0xef];
    let hex_out: [8]u8 = undefined;
    const hex_slice = try toHex(&raw, &hex_out);
    expect_eq(equalBytes(hex_slice, "deadbeef"), true);

    let decoded_out: [4]u8 = undefined;
    const decoded_slice = try fromHex(hex_slice, &decoded_out);
    expect_eq(equalBytes(decoded_slice, &raw), true);
}
