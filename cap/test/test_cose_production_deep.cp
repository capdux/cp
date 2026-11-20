// Source: cp/cap/test/test_cose_production_deep.cp (Production deep test suite for cap:cose & cap.cose)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CoseError,
    CoseAlgorithm,
    CoseKeyType,
    CoseCurve,
    CoseKey,
    encodeCoseKey,
    decodeCoseKey,
    decodeCoseSign1,
    decodeProtectedHeader,
    sign1Ed25519,
    verify1Ed25519,
    sign1Es256,
    verify1Es256,
    mac0Hs256,
    verifyMac0Hs256,
} from "cap:cose";

test("COSE_Key OKP/EC2/Symmetric CBOR Map roundtrip and invalid/missing coord rejection") {
    const alloc = std.testing.allocator;

    // 1. OKP Ed25519
    let x_ed: [32]u8 = undefined;
    @memset(&x_ed, 0x11);
    const okp_key = CoseKey{
        .kty = .OKP,
        .alg = CoseAlgorithm.EdDSA.toInt(),
        .kid = "ed-key-1",
        .crv = .Ed25519,
        .x = &x_ed,
    };
    const okp_cbor = try encodeCoseKey(alloc, okp_key);
    defer alloc.free(okp_cbor);
    let okp_dec = try decodeCoseKey(alloc, okp_cbor);
    defer okp_dec.deinit(alloc);
    expect_eq(@intFromEnum(okp_dec.kty), @intFromEnum(CoseKeyType.OKP));
    expect_eq(okp_dec.alg.?, -8);
    expect_eq(@intFromEnum(okp_dec.crv.?), @intFromEnum(CoseCurve.Ed25519));
    expect_eq(std.mem.eql(u8, okp_dec.x.?, &x_ed), true);
    expect_eq(std.mem.eql(u8, okp_dec.kid.?, "ed-key-1"), true);

    // 2. EC2 P-256
    let x_ec: [32]u8 = undefined;
    let y_ec: [32]u8 = undefined;
    @memset(&x_ec, 0x22);
    @memset(&y_ec, 0x33);
    const ec2_key = CoseKey{
        .kty = .EC2,
        .alg = CoseAlgorithm.ES256.toInt(),
        .crv = .P256,
        .x = &x_ec,
        .y = &y_ec,
    };
    const ec2_cbor = try encodeCoseKey(alloc, ec2_key);
    defer alloc.free(ec2_cbor);
    let ec2_dec = try decodeCoseKey(alloc, ec2_cbor);
    defer ec2_dec.deinit(alloc);
    expect_eq(@intFromEnum(ec2_dec.kty), @intFromEnum(CoseKeyType.EC2));
    expect_eq(ec2_dec.alg.?, -7);
    expect_eq(std.mem.eql(u8, ec2_dec.x.?, &x_ec), true);
    expect_eq(std.mem.eql(u8, ec2_dec.y.?, &y_ec), true);

    // 3. Symmetric
    const sym_bytes = "0123456789abcdef0123456789abcdef";
    const sym_key = CoseKey{
        .kty = .Symmetric,
        .alg = CoseAlgorithm.HS256.toInt(),
        .k = sym_bytes,
    };
    const sym_cbor = try encodeCoseKey(alloc, sym_key);
    defer alloc.free(sym_cbor);
    let sym_dec = try decodeCoseKey(alloc, sym_cbor);
    defer sym_dec.deinit(alloc);
    expect_eq(@intFromEnum(sym_dec.kty), @intFromEnum(CoseKeyType.Symmetric));
    expect_eq(std.mem.eql(u8, sym_dec.k.?, sym_bytes), true);

    // 4. Reject missing y in EC2
    const bad_ec2_missing_y = CoseKey{
        .kty = .EC2,
        .crv = .P256,
        .x = &x_ec,
    };
    let got_missing = false;
    * = encodeCoseKey(alloc, bad_ec2_missing_y) catch |e| {
        if (e == CoseError.MissingField) {
            got_missing = true;
        }
    };
    expect_eq(got_missing, true);

    // 5. Reject invalid coordinate length
    const bad_x = "short";
    const bad_okp_len = CoseKey{
        .kty = .OKP,
        .crv = .Ed25519,
        .x = bad_x,
    };
    let got_len_err = false;
    * = encodeCoseKey(alloc, bad_okp_len) catch |e| {
        if (e == CoseError.InvalidKeyLength) {
            got_len_err = true;
        }
    };
    expect_eq(got_len_err, true);
}

test("COSE_Sign1 EdDSA (alg=-8) sign and verify with external_aad and Tag 18") {
    const alloc = std.testing.allocator;
    const kp = std.crypto.sign.Ed25519.KeyPair.generate();
    const seed = kp.secret_key.seed();
    const pub_bytes = kp.public_key.toBytes();

    const payload = "fido2-assertion-payload";
    const aad = "rpId:cp-lang.org";

    const signed = try sign1Ed25519(alloc, seed, payload, aad, "passkey-ed-1", true);
    defer alloc.free(signed);

    // Tag 18 in CBOR is 0xd2
    expect_eq(signed[0], 0xd2);

    let parsed = try decodeCoseSign1(alloc, signed);
    defer parsed.deinit(alloc);
    let prot = try decodeProtectedHeader(alloc, parsed.protected_bytes);
    defer prot.deinit(alloc);
    expect_eq(prot.alg.?, -8);
    expect_eq(std.mem.eql(u8, prot.kid.?, "passkey-ed-1"), true);

    const verified = try verify1Ed25519(alloc, pub_bytes, signed, aad);
    defer alloc.free(verified);
    expect_eq(std.mem.eql(u8, verified, payload), true);

    // Wrong AAD must fail
    let aad_failed = false;
    * = verify1Ed25519(alloc, pub_bytes, signed, "wrong-aad") catch |e| {
        if (e == CoseError.InvalidSignature) {
            aad_failed = true;
        }
    };
    expect_eq(aad_failed, true);
}

test("COSE_Sign1 ES256 (alg=-7) sign/verify and payload/protected/AAD tamper detection") {
    const alloc = std.testing.allocator;
    const EcdsaP256 = std.crypto.sign.ecdsa.EcdsaP256Sha256;
    const kp = EcdsaP256.KeyPair.generate();
    const sec1 = kp.public_key.toUncompressedSec1();
    let x: [32]u8 = undefined;
    let y: [32]u8 = undefined;
    @memcpy(&x, sec1[1..33]);
    @memcpy(&y, sec1[33..65]);

    const payload = "iot-telemetry-packet-42";
    const aad = "device-id:sensor-09";

    const signed = try sign1Es256(alloc, kp.secret_key.toBytes(), payload, aad, "es256-kid", false);
    defer alloc.free(signed);

    const verified = try verify1Es256(alloc, x, y, signed, aad);
    defer alloc.free(verified);
    expect_eq(std.mem.eql(u8, verified, payload), true);

    // 1. AAD tamper
    let aad_err = false;
    * = verify1Es256(alloc, x, y, signed, "device-id:tampered") catch |e| {
        if (e == CoseError.InvalidSignature) aad_err = true;
    };
    expect_eq(aad_err, true);

    // 2. Payload tamper
    let tampered_payload = try alloc.dupe(u8, signed);
    defer alloc.free(tampered_payload);
    // Flip a byte inside the payload region (near the middle)
    tampered_payload[tampered_payload.len - 70] ^= 0xff;
    let payload_err = false;
    * = verify1Es256(alloc, x, y, tampered_payload, aad) catch |e| {
        if (e == CoseError.InvalidSignature) payload_err = true;
    };
    expect_eq(payload_err, true);

    // 3. Protected header tamper (flip byte in protected_bytes area)
    let tampered_prot = try alloc.dupe(u8, signed);
    defer alloc.free(tampered_prot);
    tampered_prot[6] ^= 0x01;
    let prot_err = false;
    * = verify1Es256(alloc, x, y, tampered_prot, aad) catch {
        prot_err = true;
    };
    expect_eq(prot_err, true);
}

test("COSE_Mac0 HS256 (alg=5) constant-time MAC verify and tamper rejection") {
    const alloc = std.testing.allocator;
    const key = "super-secret-iot-hmac-key-32byte";
    const payload = "firmware-ota-manifest-v1";
    const aad = "channel:prod";

    const maced = try mac0Hs256(alloc, key, payload, aad, "mac-kid-1", true);
    defer alloc.free(maced);

    expect_eq(maced[0], 0xd1); // Tag 17 in CBOR

    const verified = try verifyMac0Hs256(alloc, key, maced, aad);
    defer alloc.free(verified);
    expect_eq(std.mem.eql(u8, verified, payload), true);

    // Wrong key must fail
    let wrong_key_err = false;
    * = verifyMac0Hs256(alloc, "wrong-key-material", maced, aad) catch |e| {
        if (e == CoseError.InvalidMac) wrong_key_err = true;
    };
    expect_eq(wrong_key_err, true);

    // Tampered tag byte must fail
    let tampered = try alloc.dupe(u8, maced);
    defer alloc.free(tampered);
    tampered[tampered.len - 1] ^= 0x01;
    let tamper_err = false;
    * = verifyMac0Hs256(alloc, key, tampered, aad) catch |e| {
        if (e == CoseError.InvalidMac) tamper_err = true;
    };
    expect_eq(tamper_err, true);
}

test("cap.cose dynamic mount and std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;
    const kp = std.crypto.sign.Ed25519.KeyPair.generate();
    const payload = "cap-cose-mount-check";

    const signed = try cap.cose.sign1Ed25519(alloc, kp.secret_key.seed(), payload, "", null, true);
    defer alloc.free(signed);

    const out = try cap.cose.verify1Ed25519(alloc, kp.public_key.toBytes(), signed, "");
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, payload), true);

    const sym_key = cap.cose.CoseKey{
        .kty = .Symmetric,
        .alg = cap.cose.CoseAlgorithm.HS256.toInt(),
        .k = "0123456789abcdef",
    };
    const encoded = try cap.cose.encodeCoseKey(alloc, sym_key);
    defer alloc.free(encoded);
    let decoded = try cap.cose.decodeCoseKey(alloc, encoded);
    defer decoded.deinit(alloc);
    expect_eq(decoded.alg.?, 5);
}
