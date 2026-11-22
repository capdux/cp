// Origin: Pure-CP Stdlib Test Suite (lib/crypto/macaroon.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Macaroon Capability Tokens & Chained HMAC Attenuation:
//   1. Root token minting, offline multi-hop attenuation & full caveat verification (including wildcards)
//   2. Cryptographic integrity guards: 1-bit sig tamper, caveat stripping, caveat reordering & wrong key
//   3. Built-in fail-closed caveat evaluator: exp </<=, scope, method, prefix, custom claims & exact failed index
//   4. Binary wire codec (MAC1) round-trip parity & truncation / bad magic / trailing-byte guards
//   5. Max 16-caveat capacity, 8-claim context cap & malformed mint/caveat boundary guards
//   6. Constant-time signature comparator & cap.crypto namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MACAROON_SIG_LEN,
    MACAROON_MAX_LOCATION_LEN,
    MACAROON_MAX_ID_LEN,
    MACAROON_MAX_CAVEATS,
    MACAROON_MAX_CAVEAT_LEN,
    MACAROON_MAX_CUSTOM_CLAIMS,
    macaroonConstantTimeEq,
    MacaroonStatus,
    MacaroonCaveat,
    MacaroonClaim,
    MacaroonVerifyContext,
    MacaroonVerifyResult,
    Macaroon,
} from "cap:crypto";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("Macaroon: root minting, offline multi-hop attenuation & full verification") {
    const root_key: []const u8 = "super-secret-kms-root-key-2026";
    let tok = Macaroon.mint(root_key, "https://auth.cap.dev", "tok-id-001") orelse {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(tok.locationSlice(), "https://auth.cap.dev"));
    expect_true(bytesEq(tok.identifierSlice(), "tok-id-001"));
    expect_eq(tok.caveatCount(), 0 as usize);
    const sig0 = tok.signature();

    // Offline attenuation (bearer never needs root_key)
    expect_eq(tok.addExpCaveat(5000), MacaroonStatus.ok);
    const sig1 = tok.signature();
    expect_eq(macaroonConstantTimeEq(sig0, sig1), false);

    expect_eq(tok.addScopeCaveat("agent:tools"), MacaroonStatus.ok);
    expect_eq(tok.addMethodCaveat("POST"), MacaroonStatus.ok);
    expect_eq(tok.addPrefixCaveat("/v1/mcp/"), MacaroonStatus.ok);
    expect_eq(tok.addCaveat("tenant = acme-*"), MacaroonStatus.ok);
    expect_eq(tok.addCaveat("region = *"), MacaroonStatus.ok);
    expect_eq(tok.caveatCount(), 6 as usize);

    let ctx = MacaroonVerifyContext.init(4999, "agent:tools", "POST", "/v1/mcp/call");
    expect_true(ctx.addClaim("tenant", "acme-prod-01"));
    expect_true(ctx.addClaim("region", "us-west"));

    const res = tok.verify(root_key, ctx);
    expect_true(res.isOk());
    expect_eq(res.status, MacaroonStatus.ok);
    expect_true(res.signature_valid);
    expect_eq(res.has_failed_caveat, false);
    expect_eq(res.caveats_checked, 6 as usize);
}

test("Macaroon: rejects signature tamper, caveat stripping, caveat reordering & wrong key") {
    const root_key: []const u8 = "root-key-integrity-test";
    let tok = Macaroon.mint(root_key, "https://svc.local", "id-99") orelse {
        expect_true(false);
        return;
    };
    expect_eq(tok.addScopeCaveat("read"), MacaroonStatus.ok);
    expect_eq(tok.addMethodCaveat("GET"), MacaroonStatus.ok);

    const ctx = MacaroonVerifyContext.init(100, "read", "GET", "/items");
    expect_true(tok.verify(root_key, ctx).isOk());

    // 1. Wrong root key
    const r_wrong_key = tok.verify("wrong-root-key", ctx);
    expect_eq(r_wrong_key.status, MacaroonStatus.signature_mismatch);
    expect_eq(r_wrong_key.signature_valid, false);

    // 2. 1-bit signature tamper
    let bad_sig = tok;
    bad_sig.sig[15] ^= 0x01;
    expect_eq(bad_sig.verify(root_key, ctx).status, MacaroonStatus.signature_mismatch);

    // 3. Caveat stripping attempt (trying to drop last caveat to widen privileges)
    let stripped = tok;
    stripped.caveat_count = 1;
    expect_eq(stripped.verify(root_key, ctx).status, MacaroonStatus.signature_mismatch);

    // 4. Caveat reordering attempt (HMAC chain is strictly order-sensitive)
    let reordered = tok;
    const tmp_cav = reordered.caveats[0];
    reordered.caveats[0] = reordered.caveats[1];
    reordered.caveats[1] = tmp_cav;
    expect_eq(reordered.verify(root_key, ctx).status, MacaroonStatus.signature_mismatch);

    // 5. Identifier tamper
    let bad_id = tok;
    bad_id.identifier[0] ^= 0x01;
    expect_eq(bad_id.verify(root_key, ctx).status, MacaroonStatus.signature_mismatch);
}

test("Macaroon: built-in fail-closed caveat evaluator reports exact failure status & index") {
    const key: []const u8 = "evaluator-key";
    let tok = Macaroon.mint(key, "loc", "id-eval") orelse {
        expect_true(false);
        return;
    };
    expect_eq(tok.addExpCaveat(1000), MacaroonStatus.ok);        // index 0: exp < 1000
    expect_eq(tok.addScopeCaveat("files:read"), MacaroonStatus.ok); // index 1
    expect_eq(tok.addMethodCaveat("GET"), MacaroonStatus.ok);       // index 2
    expect_eq(tok.addPrefixCaveat("/data/"), MacaroonStatus.ok);    // index 3
    expect_eq(tok.addCaveat("env = prod"), MacaroonStatus.ok);      // index 4

    // Expired at exact boundary now_ts == 1000 (exp < 1000)
    let ctx0 = MacaroonVerifyContext.init(1000, "files:read", "GET", "/data/a.txt");
    * = ctx0.addClaim("env", "prod");
    const r0 = tok.verify(key, ctx0);
    expect_eq(r0.status, MacaroonStatus.caveat_expired);
    expect_true(r0.signature_valid);
    expect_true(r0.has_failed_caveat);
    expect_eq(r0.failed_caveat_index, 0 as usize);

    // Scope denied at index 1
    let ctx1 = MacaroonVerifyContext.init(500, "files:write", "GET", "/data/a.txt");
    * = ctx1.addClaim("env", "prod");
    const r1 = tok.verify(key, ctx1);
    expect_eq(r1.status, MacaroonStatus.caveat_scope_denied);
    expect_eq(r1.failed_caveat_index, 1 as usize);

    // Method denied at index 2
    let ctx2 = MacaroonVerifyContext.init(500, "files:read", "DELETE", "/data/a.txt");
    * = ctx2.addClaim("env", "prod");
    const r2 = tok.verify(key, ctx2);
    expect_eq(r2.status, MacaroonStatus.caveat_method_denied);
    expect_eq(r2.failed_caveat_index, 2 as usize);

    // Prefix denied at index 3
    let ctx3 = MacaroonVerifyContext.init(500, "files:read", "GET", "/etc/passwd");
    * = ctx3.addClaim("env", "prod");
    const r3 = tok.verify(key, ctx3);
    expect_eq(r3.status, MacaroonStatus.caveat_prefix_denied);
    expect_eq(r3.failed_caveat_index, 3 as usize);

    // Missing or mismatched custom claim at index 4 (fail-closed)
    const ctx4 = MacaroonVerifyContext.init(500, "files:read", "GET", "/data/ok.txt");
    const r4 = tok.verify(key, ctx4);
    expect_eq(r4.status, MacaroonStatus.caveat_assertion_failed);
    expect_eq(r4.failed_caveat_index, 4 as usize);

    // Malformed caveat (no operator) fails closed with caveat_malformed
    let tok_mal = Macaroon.mint(key, "loc", "id-mal") orelse { expect_true(false); return; };
    expect_eq(tok_mal.addCaveat("exp <= 1000"), MacaroonStatus.ok);
    expect_eq(tok_mal.addCaveat("unrecognized_predicate"), MacaroonStatus.ok);
    const r_mal = tok_mal.verify(key, MacaroonVerifyContext.init(1000, "", "", ""));
    expect_eq(r_mal.status, MacaroonStatus.caveat_malformed);
    expect_eq(r_mal.failed_caveat_index, 1 as usize);
}

test("Macaroon: binary wire codec (MAC1) round-trip & truncation/corruption guards") {
    const key: []const u8 = "wire-codec-secret";
    let tok = Macaroon.mint(key, "https://cap.io", "session-777") orelse {
        expect_true(false);
        return;
    };
    expect_eq(tok.addExpCaveat(99999), MacaroonStatus.ok);
    expect_eq(tok.addScopeCaveat("db:query"), MacaroonStatus.ok);
    expect_eq(tok.addPrefixCaveat("/v2/"), MacaroonStatus.ok);

    let wire: [512]u8 = [0; 512];
    let wire_s: []u8 = wire[0..];
    const encoded_len = tok.encodeBinary(*wire_s) orelse {
        expect_true(false);
        return;
    };
    expect_eq(encoded_len, tok.encodedSize());

    // Decode and verify complete parity
    const decoded = Macaroon.decodeBinary(wire[0..encoded_len]) orelse {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(decoded.locationSlice(), tok.locationSlice()));
    expect_true(bytesEq(decoded.identifierSlice(), tok.identifierSlice()));
    expect_eq(decoded.caveatCount(), 3 as usize);
    expect_true(macaroonConstantTimeEq(decoded.signature(), tok.signature()));
    const cav1 = decoded.caveatAt(1) orelse { expect_true(false); return; };
    expect_true(bytesEq(cav1, "scope = db:query"));

    const ctx = MacaroonVerifyContext.init(12345, "db:query", "GET", "/v2/rows");
    expect_true(decoded.verify(key, ctx).isOk());

    // Truncated buffer rejected
    expect_true(Macaroon.decodeBinary(wire[0 .. encoded_len - 1]) == null);
    expect_true(Macaroon.decodeBinary(wire[0..10]) == null);

    // Trailing junk byte rejected
    wire[encoded_len] = 0xAA;
    expect_true(Macaroon.decodeBinary(wire[0 .. encoded_len + 1]) == null);

    // Corrupted magic rejected
    wire[0] = 0x00;
    expect_true(Macaroon.decodeBinary(wire[0..encoded_len]) == null);
    wire[0] = 0x4D;

    // Corrupted version rejected
    wire[4] = 99;
    expect_true(Macaroon.decodeBinary(wire[0..encoded_len]) == null);

    // Too-small output buffer for encodeBinary returns null
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(tok.encodeBinary(*tiny_s) == null);
}

test("Macaroon: max 16 caveats, 8 custom claims & mint/caveat boundary guards") {
    const key: []const u8 = "bounds-key";

    // Invalid mint parameters
    expect_true(Macaroon.mint("", "loc", "id") == null);
    expect_true(Macaroon.mint(key, "loc", "") == null);
    let too_long_loc: [65]u8 = [0x61; 65];
    expect_true(Macaroon.mint(key, too_long_loc[0..], "id") == null);
    let too_long_id: [65]u8 = [0x62; 65];
    expect_true(Macaroon.mint(key, "loc", too_long_id[0..]) == null);

    let tok = Macaroon.mint(key, "", "id-ok") orelse {
        expect_true(false);
        return;
    };
    expect_eq(tok.addCaveat(""), MacaroonStatus.caveat_empty);
    let too_long_cav: [97]u8 = [0x63; 97];
    expect_eq(tok.addCaveat(too_long_cav[0..]), MacaroonStatus.caveat_too_long);
    expect_true(tok.caveatAt(0) == null);

    // Fill all 16 caveat slots; 17th returns caveats_full
    let i: usize = 0;
    while (i < MACAROON_MAX_CAVEATS) : (i += 1) {
        expect_eq(tok.addScopeCaveat("*"), MacaroonStatus.ok);
    }
    expect_eq(tok.caveatCount(), MACAROON_MAX_CAVEATS);
    expect_eq(tok.addScopeCaveat("*"), MacaroonStatus.caveats_full);
    expect_true(tok.verify(key, MacaroonVerifyContext.init(1, "any", "GET", "/")).isOk());

    // Fill all 8 custom claims; 9th returns false
    let ctx = MacaroonVerifyContext.init(1, "s", "m", "/");
    expect_eq(ctx.addClaim("", "v"), false);
    i = 0;
    while (i < MACAROON_MAX_CUSTOM_CLAIMS) : (i += 1) {
        expect_true(ctx.addClaim("k", "v"));
    }
    expect_eq(ctx.addClaim("overflow", "v"), false);
}

test("Macaroon: constant-time comparator & cap.crypto facade parity") {
    const a: [32]u8 = [0x5A; 32];
    let b: [32]u8 = [0x5A; 32];
    expect_true(macaroonConstantTimeEq(a, b));
    b[31] ^= 0x80;
    expect_eq(macaroonConstantTimeEq(a, b), false);

    // Verify cap.crypto namespace facade parity
    expect_eq(cap.crypto.MACAROON_SIG_LEN, MACAROON_SIG_LEN);
    expect_eq(cap.crypto.MACAROON_MAX_CAVEATS, MACAROON_MAX_CAVEATS);
    expect_eq(cap.crypto.MACAROON_MAX_CAVEAT_LEN, MACAROON_MAX_CAVEAT_LEN);
    expect_true(cap.crypto.macaroonConstantTimeEq(a, a));
}
