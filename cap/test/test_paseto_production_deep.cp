// Source: cp/cap/test/test_paseto_production_deep.cp (Deep Production Tests for cap:paseto & cap.paseto)

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    PasetoError,
    PasetoPurpose,
    paeEncode,
    parseToken,
    extractFooter,
    validateClaims,
    generateV4PublicKeyPair,
    signV4Public,
    verifyV4Public,
    generateV4LocalKey,
    encryptV4Local,
    decryptV4Local,
} from "cap:paseto";

fn isExpectedErr(res: ([]u8) ! PasetoError, expected: PasetoError, alloc: std.mem.Allocator): bool {
    const buf = res catch |e| return e == expected;
    alloc.free(buf);
    return false;
}

fn isEitherErr(res: ([]u8) ! PasetoError, e1: PasetoError, e2: PasetoError, alloc: std.mem.Allocator): bool {
    const buf = res catch |e| return (e == e1 or e == e2);
    alloc.free(buf);
    return false;
}

test("paeEncode canonical encoding and v4.public Ed25519 sign/verify with footer and implicit_assertion") {
    const alloc = std.testing.allocator;

    // 1. PAE canonical test: empty pieces -> 8 zero bytes
    const empty_pieces: [0][]const u8 = [];
    const pae_empty = paeEncode(alloc, &empty_pieces) ?;
    defer alloc.free(pae_empty);
    expect_eq(pae_empty.len, 8);
    const expected_empty: [8]u8 = [ 0, 0, 0, 0, 0, 0, 0, 0 ];
    expect_eq(std.mem.eql(u8, pae_empty, &expected_empty), true);

    // PAE with ["a", "bc"] -> count=2, len=1, "a", len=2, "bc"
    const two_pieces: [2][]const u8 = [ "a", "bc" ];
    const pae_two = paeEncode(alloc, &two_pieces) ?;
    defer alloc.free(pae_two);
    const expected_pae: [27]u8 = [
        2, 0, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0, 0, 0, 0, 0, 'a',
        2, 0, 0, 0, 0, 0, 0, 0, 'b', 'c',
    ];
    expect_eq(std.mem.eql(u8, pae_two, &expected_pae), true);

    // 2. v4.public sign & verify with footer + implicit_assertion
    const kp = generateV4PublicKeyPair();
    const payload = "{\"sub\":\"user-42\",\"role\":\"admin\"}";
    const footer = "{\"kid\":\"k1\"}";
    const implicit = "tenant:prod";

    const token = signV4Public(alloc, kp.secret_key, payload, footer, implicit) ?;
    defer alloc.free(token);
    expect_eq(std.mem.startsWith(u8, token, "v4.public."), true);

    const parsed = parseToken(token) ?;
    expect_eq(parsed.purpose == PasetoPurpose.Public, true);

    const verified = verifyV4Public(alloc, kp.public_key, token, implicit) ?;
    defer alloc.free(verified);
    expect_eq(std.mem.eql(u8, verified, payload), true);

    const ext_footer = extractFooter(alloc, token) ?;
    defer alloc.free(ext_footer);
    expect_eq(std.mem.eql(u8, ext_footer, footer), true);

    // Wrong implicit_assertion must fail with InvalidSignature
    expect_eq(isExpectedErr(verifyV4Public(alloc, kp.public_key, token, "tenant:wrong"), PasetoError.InvalidSignature, alloc), true);
}

test("v4.local symmetric roundtrip and tamper detection on ciphertext, tag, footer, and implicit_assertion") {
    const alloc = std.testing.allocator;
    const key = generateV4LocalKey();
    const payload = "{\"data\":\"confidential-session-key\",\"n\":99}";
    const footer = "v4-key-id-9";
    const implicit = "channel-binding-abc";

    const token = encryptV4Local(alloc, key, payload, footer, implicit) ?;
    defer alloc.free(token);
    expect_eq(std.mem.startsWith(u8, token, "v4.local."), true);

    const plain = decryptV4Local(alloc, key, token, implicit) ?;
    defer alloc.free(plain);
    expect_eq(std.mem.eql(u8, plain, payload), true);

    // Tamper implicit_assertion -> AuthenticationFailed
    expect_eq(isExpectedErr(decryptV4Local(alloc, key, token, "wrong-binding"), PasetoError.AuthenticationFailed, alloc), true);

    // Tamper ciphertext / tag byte in token -> AuthenticationFailed
    let tampered = alloc.dupe(u8, token) ?;
    defer alloc.free(tampered);
    tampered[20] = if (tampered[20] == 'A') 'B' else 'A';
    expect_eq(isEitherErr(decryptV4Local(alloc, key, tampered, implicit), PasetoError.AuthenticationFailed, PasetoError.InvalidBase64, alloc), true);

    // Tamper footer -> AuthenticationFailed
    let footer_tampered = alloc.dupe(u8, token) ?;
    defer alloc.free(footer_tampered);
    const last_idx = footer_tampered.len - 2;
    footer_tampered[last_idx] = if (footer_tampered[last_idx] == 'a') 'b' else 'a';
    expect_eq(isEitherErr(decryptV4Local(alloc, key, footer_tampered, implicit), PasetoError.AuthenticationFailed, PasetoError.InvalidBase64, alloc), true);
}

test("invalid version prefix, invalid base64url, and wrong key rejection") {
    const alloc = std.testing.allocator;
    const kp1 = generateV4PublicKeyPair();
    const kp2 = generateV4PublicKeyPair();
    const key1 = generateV4LocalKey();
    const key2 = generateV4LocalKey();

    // Wrong prefix v2.public
    expect_eq(isExpectedErr(verifyV4Public(alloc, kp1.public_key, "v2.public.AAAA", ""), PasetoError.InvalidPrefix, alloc), true);

    // Non-Base64URL
    expect_eq(isExpectedErr(verifyV4Public(alloc, kp1.public_key, "v4.public.!!!invalid!!!", ""), PasetoError.InvalidBase64, alloc), true);

    // Wrong public key
    const pub_tok = signV4Public(alloc, kp1.secret_key, "hello", "", "") ?;
    defer alloc.free(pub_tok);
    expect_eq(isExpectedErr(verifyV4Public(alloc, kp2.public_key, pub_tok, ""), PasetoError.InvalidSignature, alloc), true);

    // Wrong symmetric key
    const loc_tok = encryptV4Local(alloc, key1, "secret", "", "") ?;
    defer alloc.free(loc_tok);
    expect_eq(isExpectedErr(decryptV4Local(alloc, key2, loc_tok, ""), PasetoError.AuthenticationFailed, alloc), true);
}

test("claims time window and issuer/audience validation") {
    const claims_json = "{\"iss\":\"https://auth.cp.dev\",\"aud\":\"api-gateway\",\"nbf\":\"2026-01-01T00:00:00Z\",\"exp\":\"2026-12-31T23:59:59Z\"}";

    // Valid window + matching iss/aud
    validateClaims(claims_json, "2026-06-15T12:00:00Z", "https://auth.cp.dev", "api-gateway") ?;

    // Expired
    let got_expired = false;
    validateClaims(claims_json, "2027-01-01T00:00:00Z", null, null) catch |e| {
        got_expired = (e == PasetoError.ExpiredToken);
    };
    expect_eq(got_expired, true);

    // Not yet valid
    let got_nbf = false;
    validateClaims(claims_json, "2025-12-31T23:59:59Z", null, null) catch |e| {
        got_nbf = (e == PasetoError.NotYetValid);
    };
    expect_eq(got_nbf, true);

    // Issuer mismatch
    let got_iss_mismatch = false;
    validateClaims(claims_json, "2026-06-15T12:00:00Z", "https://evil.dev", "api-gateway") catch |e| {
        got_iss_mismatch = (e == PasetoError.ClaimMismatch);
    };
    expect_eq(got_iss_mismatch, true);

    // Audience mismatch
    let got_aud_mismatch = false;
    validateClaims(claims_json, "2026-06-15T12:00:00Z", "https://auth.cp.dev", "other-service") catch |e| {
        got_aud_mismatch = (e == PasetoError.ClaimMismatch);
    };
    expect_eq(got_aud_mismatch, true);
}

test("cap.paseto dynamic mount and std.testing.allocator zero leak") {
    const alloc = std.testing.allocator;
    const key = cap.paseto.generateV4LocalKey();
    const tok = cap.paseto.encryptV4Local(alloc, key, "cap-mount-ok", "ftr", "imp") ?;
    defer alloc.free(tok);

    const out = cap.paseto.decryptV4Local(alloc, key, tok, "imp") ?;
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, "cap-mount-ok"), true);

    const ftr = cap.paseto.extractFooter(alloc, tok) ?;
    defer alloc.free(ftr);
    expect_eq(std.mem.eql(u8, ftr, "ftr"), true);
}
