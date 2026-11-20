// Source: cp/cap/test/test_webauthn_production_deep.cp (Production deep test suite for cap:webauthn)
const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    CborWriter,
    CoseAlgorithm,
    CoseKeyType,
    CoseCurve,
    CoseKey,
    encodeCoseKey,
} from "cap:cose";
import {
    WebAuthnError,
    AuthFlags,
    parseAuthenticatorData,
    encodeChallengeB64u,
    parseClientDataJson,
    verifyClientData,
    ExpectedClientData,
    computeRpIdHash,
    computeClientDataHash,
    verifyRegistrationNone,
    verifyAssertion,
} from "cap:webauthn";

fn buildAuthDataBytes(
    alloc: std.mem.Allocator,
    rp_id: []const u8,
    flags: u8,
    sign_count: u32,
    aaguid_opt: ([16]u8)?,
    cred_id_opt: ([]const u8)?,
    cose_key_bytes_opt: ([]const u8)?,
): ![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    const rp_hash = computeRpIdHash(rp_id);
    try buf.appendSlice(alloc, &rp_hash);
    try buf.append(alloc, flags);

    let sc_bytes: [4]u8 = undefined;
    std.mem.writeInt(u32, &sc_bytes, sign_count, .big);
    try buf.appendSlice(alloc, &sc_bytes);

    if (aaguid := aaguid_opt) {
        if (cred_id := cred_id_opt) {
            if (cose_bytes := cose_key_bytes_opt) {
                try buf.appendSlice(alloc, &aaguid);
                let len_bytes: [2]u8 = undefined;
                std.mem.writeInt(u16, &len_bytes, @as(u16, @intCast(cred_id.len)), .big);
                try buf.appendSlice(alloc, &len_bytes);
                try buf.appendSlice(alloc, cred_id);
                try buf.appendSlice(alloc, cose_bytes);
            }
        }
    }

    return buf.toOwnedSlice(alloc);
}

fn buildAttestationObjectNone(alloc: std.mem.Allocator, auth_data: []const u8): ![]u8 {
    let w = CborWriter.init();
    defer w.deinit(alloc);

    try w.writeMapHeader(alloc, 3);
    try w.writeTstr(alloc, "fmt");
    try w.writeTstr(alloc, "none");
    try w.writeTstr(alloc, "attStmt");
    try w.writeMapHeader(alloc, 0);
    try w.writeTstr(alloc, "authData");
    try w.writeBstr(alloc, auth_data);

    return w.toOwnedSlice(alloc);
}

test("webauthn: parseAuthenticatorData with AT, aaguid, credential_id, and P-256 / Ed25519 CoseKey") {
    const alloc = std.testing.allocator;

    const Ecdsa = std.crypto.sign.ecdsa.EcdsaP256Sha256;
    let seed: [32]u8 = undefined;
    @memset(&seed, 0x11);
    const kp = try Ecdsa.KeyPair.generateDeterministic(seed);
    const sec1 = kp.public_key.toUncompressedSec1();

    const cose_p256 = CoseKey{
        .kty = .EC2,
        .alg = CoseAlgorithm.ES256.toInt(),
        .crv = .P256,
        .x = sec1[1..33],
        .y = sec1[33..65],
    };
    const cose_bytes = try encodeCoseKey(alloc, cose_p256);
    defer alloc.free(cose_bytes);

    let aaguid: [16]u8 = undefined;
    @memset(&aaguid, 0xaa);
    const cred_id = "cred-id-p256-001";
    const flags = AuthFlags{
        .up = true,
        .uv = true,
        .be = true,
        .bs = false,
        .at = true,
        .ed = false,
        .raw = 0x4d,
    };

    const raw = try buildAuthDataBytes(
        alloc,
        "example.com",
        flags.toByte(),
        42,
        aaguid,
        cred_id,
        cose_bytes,
    );
    defer alloc.free(raw);

    let parsed = try parseAuthenticatorData(alloc, raw);
    defer parsed.deinit(alloc);

    try expect_eq(parsed.flags.up, true);
    try expect_eq(parsed.flags.uv, true);
    try expect_eq(parsed.flags.at, true);
    try expect_eq(parsed.sign_count, 42);

    const acd = parsed.attested_credential_data.?;
    try expect_eq(std.mem.eql(u8, &acd.aaguid, &aaguid), true);
    try expect_eq(std.mem.eql(u8, acd.credential_id, cred_id), true);
    try expect_eq(acd.credential_public_key.kty == .EC2, true);
    try expect_eq(acd.credential_public_key.crv.? == .P256, true);
    try expect_eq(std.mem.eql(u8, acd.credential_public_key.x.?, sec1[1..33]), true);

    // Also test Ed25519 COSE_Key extraction
    const Ed25519 = std.crypto.sign.Ed25519;
    let ed_seed: [32]u8 = undefined;
    @memset(&ed_seed, 0x22);
    const ed_kp = try Ed25519.KeyPair.generateDeterministic(ed_seed);
    const ed_pub = ed_kp.public_key.toBytes();
    const cose_ed = CoseKey{
        .kty = .OKP,
        .alg = CoseAlgorithm.EdDSA.toInt(),
        .crv = .Ed25519,
        .x = &ed_pub,
    };
    const ed_cose_bytes = try encodeCoseKey(alloc, cose_ed);
    defer alloc.free(ed_cose_bytes);

    const raw_ed = try buildAuthDataBytes(
        alloc,
        "example.com",
        flags.toByte(),
        7,
        aaguid,
        "cred-ed25519",
        ed_cose_bytes,
    );
    defer alloc.free(raw_ed);

    let parsed_ed = try parseAuthenticatorData(alloc, raw_ed);
    defer parsed_ed.deinit(alloc);
    try expect_eq(parsed_ed.attested_credential_data.?.credential_public_key.kty == .OKP, true);
    try expect_eq(parsed_ed.attested_credential_data.?.credential_public_key.crv.? == .Ed25519, true);
}

test("webauthn: parseClientDataJson and verifyClientData security checks") {
    const alloc = std.testing.allocator;

    const raw_challenge = "super-secret-random-challenge-32";
    const challenge_b64u = try encodeChallengeB64u(alloc, raw_challenge);
    defer alloc.free(challenge_b64u);

    const json_ok = try std.fmt.allocPrint(
        alloc,
        "{{\"type\":\"webauthn.get\",\"challenge\":\"{s}\",\"origin\":\"https://auth.example.com\",\"crossOrigin\":false}}",
        .{challenge_b64u},
    );
    defer alloc.free(json_ok);

    let cd = try parseClientDataJson(alloc, json_ok);
    defer cd.deinit(alloc);

    try verifyClientData(cd, ExpectedClientData{
        .expected_type = "webauthn.get",
        .expected_challenge = challenge_b64u,
        .expected_origin = "https://auth.example.com",
        .allow_cross_origin = false,
    });

    // Challenge tampering must fail
    let bad_chall_res = verifyClientData(cd, ExpectedClientData{
        .expected_type = "webauthn.get",
        .expected_challenge = "tampered-challenge-value",
        .expected_origin = "https://auth.example.com",
        .allow_cross_origin = false,
    });
    try std.testing.expectError(WebAuthnError.ChallengeMismatch, bad_chall_res);

    // Origin mismatch must fail
    let bad_origin_res = verifyClientData(cd, ExpectedClientData{
        .expected_type = "webauthn.get",
        .expected_challenge = challenge_b64u,
        .expected_origin = "https://evil.example.com",
        .allow_cross_origin = false,
    });
    try std.testing.expectError(WebAuthnError.OriginMismatch, bad_origin_res);

    // crossOrigin: true must be rejected when allow_cross_origin == false
    const json_cross = try std.fmt.allocPrint(
        alloc,
        "{{\"type\":\"webauthn.get\",\"challenge\":\"{s}\",\"origin\":\"https://auth.example.com\",\"crossOrigin\":true}}",
        .{challenge_b64u},
    );
    defer alloc.free(json_cross);

    let cd_cross = try parseClientDataJson(alloc, json_cross);
    defer cd_cross.deinit(alloc);

    let cross_res = verifyClientData(cd_cross, ExpectedClientData{
        .expected_type = "webauthn.get",
        .expected_challenge = challenge_b64u,
        .expected_origin = "https://auth.example.com",
        .allow_cross_origin = false,
    });
    try std.testing.expectError(WebAuthnError.CrossOriginNotAllowed, cross_res);
}

test("webauthn: verifyRegistrationNone and verifyAssertion ES256 DER & EdDSA 64B") {
    const alloc = std.testing.allocator;

    const challenge_b64u = try encodeChallengeB64u(alloc, "reg-and-login-challenge");
    defer alloc.free(challenge_b64u);

    // 1. ES256 (ECDSA P-256 + DER signature)
    const Ecdsa = std.crypto.sign.ecdsa.EcdsaP256Sha256;
    let ec_seed: [32]u8 = undefined;
    @memset(&ec_seed, 0x33);
    const ec_kp = try Ecdsa.KeyPair.generateDeterministic(ec_seed);
    const sec1 = ec_kp.public_key.toUncompressedSec1();

    const cose_p256 = CoseKey{
        .kty = .EC2,
        .alg = CoseAlgorithm.ES256.toInt(),
        .crv = .P256,
        .x = sec1[1..33],
        .y = sec1[33..65],
    };
    const cose_p256_bytes = try encodeCoseKey(alloc, cose_p256);
    defer alloc.free(cose_p256_bytes);

    let reg_aaguid: [16]u8 = undefined;
    @memset(&reg_aaguid, 0x55);
    const reg_auth_data = try buildAuthDataBytes(
        alloc,
        "passkey.example.com",
        0x45, // UP | UV | AT
        1,
        reg_aaguid,
        "es256-cred-1",
        cose_p256_bytes,
    );
    defer alloc.free(reg_auth_data);

    const att_obj = try buildAttestationObjectNone(alloc, reg_auth_data);
    defer alloc.free(att_obj);

    const reg_cd_json = try std.fmt.allocPrint(
        alloc,
        "{{\"type\":\"webauthn.create\",\"challenge\":\"{s}\",\"origin\":\"https://passkey.example.com\"}}",
        .{challenge_b64u},
    );
    defer alloc.free(reg_cd_json);

    let reg_res = try verifyRegistrationNone(
        alloc,
        att_obj,
        reg_cd_json,
        "passkey.example.com",
        challenge_b64u,
        "https://passkey.example.com",
        true,
    );
    defer reg_res.deinit(alloc);
    try expect_eq(std.mem.eql(u8, reg_res.credential_id, "es256-cred-1"), true);

    // Assertion with ES256 DER signature
    const assert_auth_data = try buildAuthDataBytes(
        alloc,
        "passkey.example.com",
        0x05, // UP | UV
        2,
        null,
        null,
        null,
    );
    defer alloc.free(assert_auth_data);

    const assert_cd_json = try std.fmt.allocPrint(
        alloc,
        "{{\"type\":\"webauthn.get\",\"challenge\":\"{s}\",\"origin\":\"https://passkey.example.com\"}}",
        .{challenge_b64u},
    );
    defer alloc.free(assert_cd_json);

    const cd_hash = computeClientDataHash(assert_cd_json);
    let msg_buf = std.ArrayList(u8).empty;
    defer msg_buf.deinit(alloc);
    try msg_buf.appendSlice(alloc, assert_auth_data);
    try msg_buf.appendSlice(alloc, &cd_hash);

    const ec_sig = try ec_kp.sign(msg_buf.items, null);
    let der_buf: [Ecdsa.Signature.der_encoded_length_max]u8 = undefined;
    const der_sig = ec_sig.toDer(&der_buf);

    const assert_res = try verifyAssertion(
        alloc,
        assert_auth_data,
        assert_cd_json,
        der_sig,
        reg_res.credential_public_key,
        "passkey.example.com",
        challenge_b64u,
        "https://passkey.example.com",
        1,
        true,
    );
    try expect_eq(assert_res.sign_count, 2);

    // Tampered DER signature must fail
    let bad_der = try alloc.dupe(u8, der_sig);
    defer alloc.free(bad_der);
    bad_der[bad_der.len - 1] ^= 0xff;
    let bad_sig_res = verifyAssertion(
        alloc,
        assert_auth_data,
        assert_cd_json,
        bad_der,
        reg_res.credential_public_key,
        "passkey.example.com",
        challenge_b64u,
        "https://passkey.example.com",
        1,
        true,
    );
    try std.testing.expectError(WebAuthnError.InvalidSignature, bad_sig_res);

    // 2. EdDSA (Ed25519 64B signature)
    const Ed25519 = std.crypto.sign.Ed25519;
    let ed_seed2: [32]u8 = undefined;
    @memset(&ed_seed2, 0x44);
    const ed_kp = try Ed25519.KeyPair.generateDeterministic(ed_seed2);
    const ed_pub = ed_kp.public_key.toBytes();
    const cose_ed = CoseKey{
        .kty = .OKP,
        .alg = CoseAlgorithm.EdDSA.toInt(),
        .crv = .Ed25519,
        .x = &ed_pub,
    };

    const ed_sig = try ed_kp.sign(msg_buf.items, null);
    const ed_sig_bytes = ed_sig.toBytes();

    const ed_assert_res = try verifyAssertion(
        alloc,
        assert_auth_data,
        assert_cd_json,
        &ed_sig_bytes,
        cose_ed,
        "passkey.example.com",
        challenge_b64u,
        "https://passkey.example.com",
        1,
        true,
    );
    try expect_eq(ed_assert_res.sign_count, 2);
}

test("webauthn: rpIdHash mismatch, UP missing, and signCount anti-cloning rollback") {
    const alloc = std.testing.allocator;

    const challenge_b64u = try encodeChallengeB64u(alloc, "rollback-test-challenge");
    defer alloc.free(challenge_b64u);

    const Ed25519 = std.crypto.sign.Ed25519;
    let ed_seed3: [32]u8 = undefined;
    @memset(&ed_seed3, 0x77);
    const ed_kp = try Ed25519.KeyPair.generateDeterministic(ed_seed3);
    const ed_pub = ed_kp.public_key.toBytes();
    const cose_ed = CoseKey{
        .kty = .OKP,
        .alg = CoseAlgorithm.EdDSA.toInt(),
        .crv = .Ed25519,
        .x = &ed_pub,
    };

    const assert_cd_json = try std.fmt.allocPrint(
        alloc,
        "{{\"type\":\"webauthn.get\",\"challenge\":\"{s}\",\"origin\":\"https://example.com\"}}",
        .{challenge_b64u},
    );
    defer alloc.free(assert_cd_json);

    // 1. rpIdHash mismatch
    const wrong_rp_auth = try buildAuthDataBytes(alloc, "wrong.example.com", 0x05, 10, null, null, null);
    defer alloc.free(wrong_rp_auth);
    let dummy_sig: [64]u8 = undefined;
    @memset(&dummy_sig, 0);
    try std.testing.expectError(
        WebAuthnError.RpIdMismatch,
        verifyAssertion(alloc, wrong_rp_auth, assert_cd_json, &dummy_sig, cose_ed, "example.com", challenge_b64u, "https://example.com", 5, true),
    );

    // 2. UP missing (flags = 0x00)
    const no_up_auth = try buildAuthDataBytes(alloc, "example.com", 0x00, 10, null, null, null);
    defer alloc.free(no_up_auth);
    try std.testing.expectError(
        WebAuthnError.UserPresenceRequired,
        verifyAssertion(alloc, no_up_auth, assert_cd_json, &dummy_sig, cose_ed, "example.com", challenge_b64u, "https://example.com", 5, false),
    );

    // 3. signCount rollback (new == stored or new < stored)
    const rollback_auth = try buildAuthDataBytes(alloc, "example.com", 0x05, 5, null, null, null);
    defer alloc.free(rollback_auth);
    try std.testing.expectError(
        WebAuthnError.SignCountRollback,
        verifyAssertion(alloc, rollback_auth, assert_cd_json, &dummy_sig, cose_ed, "example.com", challenge_b64u, "https://example.com", 5, true),
    );
}

test("webauthn: cap.webauthn dynamic mount & zero-leak allocator check") {
    const alloc = std.testing.allocator;

    const flags = cap.webauthn.AuthFlags.fromByte(0x05);
    try expect_eq(flags.up, true);
    try expect_eq(flags.uv, true);
    try expect_eq(flags.at, false);

    const b64 = try cap.webauthn.encodeChallengeB64u(alloc, "hello-passkey");
    defer alloc.free(b64);
    try expect_eq(cap.webauthn.constantTimeEql(b64, b64), true);
}
