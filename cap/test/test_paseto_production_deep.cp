// Source: cp/cap/test/test_paseto_production_deep.cp
// Production Deep Regression Suite for `cap:paseto` & `cap.paseto` (PASETO v4)

import { test, expect_eq } from "cap:test";
import std from "std";
import cap from "cap";
import {
    V4_LOCAL_HEADER,
    V4_PUBLIC_HEADER,
    PasetoError,
    PasetoPurpose,
    PasetoParsed,
    VerifyClaimsOptions,
    paeEncode,
    PasetoV4,
} from "cap:paseto";
import { Ed25519KeyPair } from "cap:crypto";

fn expect_true(cond: bool) {
    if (!cond) @panic("expect_true failed");
}

fn expectParsedError(res: PasetoError!PasetoParsed, expected: PasetoError): bool {
    const parsed = res catch e => {
        return e == expected;
    };
    if (parsed.payload_json.len > 0) parsed.allocator.free(parsed.payload_json);
    if (parsed.footer.len > 0) parsed.allocator.free(parsed.footer);
    return false;
}

fn expectVoidError(res: PasetoError!void, expected: PasetoError): bool {
    res catch e => {
        return e == expected;
    };
    return false;
}

fn testV4PublicRoundtripAndTamper(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    defer expect_true(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    const kp = Ed25519KeyPair.generate();
    const payload = "{\"sub\":\"agent-42\",\"iss\":\"cp-zero-trust\",\"exp\":1900000000}";
    const footer = "{\"kid\":\"ed25519-2026\"}";
    const implicit = "service=gateway;env=prod";

    const token = PasetoV4.signPublic(alloc, kp.secret_key, payload, footer, implicit)?;
    defer alloc.free(token);

    expect_true(std.mem.startsWith(u8, token, V4_PUBLIC_HEADER));

    let parsed = PasetoV4.verifyPublic(alloc, kp.public_key, token, implicit)?;
    defer parsed.deinit();

    expect_true(parsed.purpose == .Public);
    expect_true(std.mem.eql(u8, parsed.header, V4_PUBLIC_HEADER));
    expect_true(std.mem.eql(u8, parsed.payload_json, payload));
    expect_true(std.mem.eql(u8, parsed.footer, footer));

    // 1. Reject wrong implicit assertion
    expect_true(expectParsedError(
        PasetoV4.verifyPublic(alloc, kp.public_key, token, "service=gateway;env=dev"),
        PasetoError.InvalidSignature,
    ));

    // 2. Reject tampered payload character inside body
    let tampered = alloc.alloc(u8, token.len)?;
    defer alloc.free(tampered);
    @memcpy(tampered, token);
    const flip_idx = V4_PUBLIC_HEADER.len + 4;
    tampered[flip_idx] = if (tampered[flip_idx] == 'A') 'B' else 'A';
    expect_true(expectParsedError(
        PasetoV4.verifyPublic(alloc, kp.public_key, tampered, implicit),
        PasetoError.InvalidSignature,
    ));

    // 3. Reject tampered footer
    let tampered_footer = alloc.alloc(u8, token.len)?;
    defer alloc.free(tampered_footer);
    @memcpy(tampered_footer, token);
    const f_idx = token.len - 2;
    tampered_footer[f_idx] = if (tampered_footer[f_idx] == 'A') 'B' else 'A';
    expect_true(expectParsedError(
        PasetoV4.verifyPublic(alloc, kp.public_key, tampered_footer, implicit),
        PasetoError.InvalidSignature,
    ));

    // 4. Reject forged JWT alg:none or wrong header
    expect_true(expectParsedError(
        PasetoV4.verifyPublic(alloc, kp.public_key, "eyJhbGciOiJub25lIn0.e30.", ""),
        PasetoError.InvalidHeader,
    ));
    expect_true(expectParsedError(
        PasetoV4.verifyPublic(alloc, kp.public_key, "v4.local.AAAA", ""),
        PasetoError.InvalidHeader,
    ));
}

fn testV4LocalRoundtripAndAuthFailure(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    defer expect_true(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    let key32: [32]u8 = undefined;
    for (i of 0..32) {
        key32[i] = u8(i + 1);
    }
    let wrong_key32: [32]u8 = key32;
    wrong_key32[0] = 0xFF;

    const payload = "{\"scope\":\"read:secrets\",\"jti\":\"tok-9981\"}";
    const footer = "key-id:v4-sym-1";
    const implicit = "cluster=ap-southeast-2";

    const token = PasetoV4.encryptLocal(alloc, key32, payload, footer, implicit)?;
    defer alloc.free(token);

    expect_true(std.mem.startsWith(u8, token, V4_LOCAL_HEADER));

    let parsed = PasetoV4.decryptLocal(alloc, key32, token, implicit)?;
    defer parsed.deinit();

    expect_true(parsed.purpose == .Local);
    expect_true(std.mem.eql(u8, parsed.payload_json, payload));
    expect_true(std.mem.eql(u8, parsed.footer, footer));

    // 1. Wrong symmetric key -> DecryptionFailed
    expect_true(expectParsedError(
        PasetoV4.decryptLocal(alloc, wrong_key32, token, implicit),
        PasetoError.DecryptionFailed,
    ));

    // 2. Wrong implicit assertion -> DecryptionFailed
    expect_true(expectParsedError(
        PasetoV4.decryptLocal(alloc, key32, token, "cluster=us-east-1"),
        PasetoError.DecryptionFailed,
    ));

    // 3. Tampered ciphertext -> DecryptionFailed
    let tampered = alloc.alloc(u8, token.len)?;
    defer alloc.free(tampered);
    @memcpy(tampered, token);
    const c_idx = V4_LOCAL_HEADER.len + 10;
    tampered[c_idx] = if (tampered[c_idx] == 'a') 'b' else 'a';
    expect_true(expectParsedError(
        PasetoV4.decryptLocal(alloc, key32, tampered, implicit),
        PasetoError.DecryptionFailed,
    ));
}

fn testPaeEncodeStandardVectors(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    defer expect_true(gpa.deinit() == .ok);
    const alloc = gpa.allocator();

    // Empty array: LE64(0)
    const empty_pieces: []const []const u8 = [];
    const enc_empty = paeEncode(alloc, empty_pieces)?;
    defer alloc.free(enc_empty);
    expect_eq(enc_empty.len, 8 as usize);
    for (b of enc_empty) {
        expect_eq(b, 0 as u8);
    }

    // Single empty string: LE64(1) || LE64(0)
    let one_empty: [1][]const u8 = [""];
    const enc_one = paeEncode(alloc, &one_empty)?;
    defer alloc.free(enc_one);
    expect_eq(enc_one.len, 16 as usize);
    expect_eq(enc_one[0], 1 as u8);
    expect_eq(enc_one[8], 0 as u8);

    // Collision resistance: PAE(["a", "bc"]) != PAE(["ab", "c"])
    let p1: [2][]const u8 = ["a", "bc"];
    let p2: [2][]const u8 = ["ab", "c"];
    const e1 = paeEncode(alloc, &p1)?;
    defer alloc.free(e1);
    const e2 = paeEncode(alloc, &p2)?;
    defer alloc.free(e2);
    expect_eq(e1.len, e2.len);
    expect_true(!std.mem.eql(u8, e1, e2));

    // Exact byte check for ["test"] -> LE64(1) || LE64(4) || "test"
    let p_test: [1][]const u8 = ["test"];
    const e_test = paeEncode(alloc, &p_test)?;
    defer alloc.free(e_test);
    expect_eq(e_test.len, 20 as usize);
    expect_eq(e_test[0], 1 as u8);
    expect_eq(e_test[8], 4 as u8);
    expect_true(std.mem.eql(u8, e_test[16..20], "test"));
}

fn testVerifyClaimsAndZeroLeak(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    defer expect_true(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    const valid_json = "{\"iss\":\"cp-iam\",\"aud\":\"api-v1\",\"sub\":\"user-7\",\"exp\":2000,\"nbf\":1000,\"iat\":900,\"jti\":\"j-1\"}";

    // 1. Valid within window
    PasetoV4.verifyClaims(alloc, valid_json, VerifyClaimsOptions{
        now_unix: 1500,
        expected_iss: "cp-iam",
        expected_aud: "api-v1",
        expected_sub: "user-7",
        clock_skew_sec: 0,
    })?;

    // 2. Expired -> TokenExpired
    expect_true(expectVoidError(
        PasetoV4.verifyClaims(alloc, valid_json, VerifyClaimsOptions{
            now_unix: 2000,
            clock_skew_sec: 0,
        }),
        PasetoError.TokenExpired,
    ));

    // 3. Within clock skew -> OK
    PasetoV4.verifyClaims(alloc, valid_json, VerifyClaimsOptions{
        now_unix: 2005,
        clock_skew_sec: 10,
    })?;

    // 4. Not yet valid -> TokenNotYetValid
    expect_true(expectVoidError(
        PasetoV4.verifyClaims(alloc, valid_json, VerifyClaimsOptions{
            now_unix: 950,
            clock_skew_sec: 0,
        }),
        PasetoError.TokenNotYetValid,
    ));

    // 5. Issuer / Audience mismatch -> ClaimMismatch
    expect_true(expectVoidError(
        PasetoV4.verifyClaims(alloc, valid_json, VerifyClaimsOptions{
            now_unix: 1500,
            expected_iss: "other-iam",
        }),
        PasetoError.ClaimMismatch,
    ));

    expect_true(expectVoidError(
        PasetoV4.verifyClaims(alloc, valid_json, VerifyClaimsOptions{
            now_unix: 1500,
            expected_aud: "wrong-aud",
        }),
        PasetoError.ClaimMismatch,
    ));
}

fn testCapDynamicLazyMount(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    defer expect_true(gpa.deinit() == .ok);
    const alloc = gpa.allocator();
    let key32: [32]u8 = undefined;
    for (i of 0..32) {
        key32[i] = u8(0x5A);
    }

    const tok = cap.paseto.PasetoV4.encryptLocal(
        alloc,
        key32,
        "{\"agent\":\"coordinator\"}",
        "v4-mount",
        "cap-ns",
    )?;
    defer alloc.free(tok);

    let parsed = cap.paseto.PasetoV4.decryptLocal(alloc, key32, tok, "cap-ns")?;
    defer parsed.deinit();

    expect_true(std.mem.eql(u8, parsed.payload_json, "{\"agent\":\"coordinator\"}"));
    expect_true(std.mem.eql(u8, parsed.footer, "v4-mount"));
}

fn main() {
    test("v4.public Ed25519 sign/verify roundtrip & tamper/header rejection", () => {
        testV4PublicRoundtripAndTamper() catch @panic("testV4PublicRoundtripAndTamper failed");
    });
    test("v4.local BLAKE3+XChaCha20-Poly1305 roundtrip & auth failure", () => {
        testV4LocalRoundtripAndAuthFailure() catch @panic("testV4LocalRoundtripAndAuthFailure failed");
    });
    test("paeEncode standard vectors & collision resistance", () => {
        testPaeEncodeStandardVectors() catch @panic("testPaeEncodeStandardVectors failed");
    });
    test("verifyClaims exp/nbf/iss/aud/sub & zero-leak", () => {
        testVerifyClaimsAndZeroLeak() catch @panic("testVerifyClaimsAndZeroLeak failed");
    });
    test("import cap from 'cap'; cap.paseto dynamic lazy mounting", () => {
        testCapDynamicLazyMount() catch @panic("testCapDynamicLazyMount failed");
    });
}
