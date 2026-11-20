// Source: cp/lib/webauthn/verify.cp (WebAuthn Registration & Assertion cryptographic verification)
const std = @import("std");
import { CborReader, CoseKey } from "cap:cose";
import {
    WebAuthnError,
    ExpectedClientData,
    RegistrationResult,
    AssertionResult,
} from "./types.cp";
import { parseAuthenticatorData } from "./auth_data.cp";
import {
    constantTimeEql,
    parseClientDataJson,
    verifyClientData,
} from "./client_data.cp";

pub fn computeRpIdHash(rp_id: []const u8): [32]u8 {
    let out: [32]u8 = undefined;
    std.crypto.hash.sha2.Sha256.hash(rp_id, &out, .{});
    return out;
}

pub fn computeClientDataHash(client_data_json: []const u8): [32]u8 {
    let out: [32]u8 = undefined;
    std.crypto.hash.sha2.Sha256.hash(client_data_json, &out, .{});
    return out;
}

fn verifySignatureWithCoseKey(
    public_key: CoseKey,
    message: []const u8,
    signature: []const u8,
): WebAuthnError!void {
    switch (public_key.kty) {
        .EC2 => {
            const crv = public_key.crv orelse return WebAuthnError.InvalidCoseKey;
            if (crv != .P256) {
                return WebAuthnError.UnsupportedAlgorithm;
            }
            const x = public_key.x orelse return WebAuthnError.InvalidCoseKey;
            const y = public_key.y orelse return WebAuthnError.InvalidCoseKey;
            if (x.len != 32 or y.len != 32) {
                return WebAuthnError.InvalidCoseKey;
            }
            let sec1: [65]u8 = undefined;
            sec1[0] = 0x04;
            @memcpy(sec1[1..33], x[0..32]);
            @memcpy(sec1[33..65], y[0..32]);

            const Ecdsa = std.crypto.sign.ecdsa.EcdsaP256Sha256;
            const pk = Ecdsa.PublicKey.fromSec1(&sec1) catch return WebAuthnError.InvalidCoseKey;
            const sig = Ecdsa.Signature.fromDer(signature) catch return WebAuthnError.InvalidSignature;
            sig.verify(message, pk) catch return WebAuthnError.InvalidSignature;
        },
        .OKP => {
            const crv = public_key.crv orelse return WebAuthnError.InvalidCoseKey;
            if (crv != .Ed25519) {
                return WebAuthnError.UnsupportedAlgorithm;
            }
            const x = public_key.x orelse return WebAuthnError.InvalidCoseKey;
            if (x.len != 32) {
                return WebAuthnError.InvalidCoseKey;
            }
            if (signature.len != 64) {
                return WebAuthnError.InvalidSignature;
            }
            const Ed25519 = std.crypto.sign.Ed25519;
            const pk = Ed25519.PublicKey.fromBytes(x[0..32].*) catch return WebAuthnError.InvalidCoseKey;
            const sig = Ed25519.Signature.fromBytes(signature[0..64].*);
            sig.verify(message, pk) catch return WebAuthnError.InvalidSignature;
        },
        else => return WebAuthnError.UnsupportedAlgorithm,
    }
}

pub fn verifyRegistrationNone(
    alloc: std.mem.Allocator,
    attestation_object: []const u8,
    client_data_json: []const u8,
    rp_id: []const u8,
    expected_challenge: []const u8,
    expected_origin: []const u8,
    require_uv: bool,
): WebAuthnError!RegistrationResult {
    let cd = try parseClientDataJson(alloc, client_data_json);
    defer cd.deinit(alloc);

    try verifyClientData(cd, ExpectedClientData{
        .expected_type = "webauthn.create",
        .expected_challenge = expected_challenge,
        .expected_origin = expected_origin,
        .allow_cross_origin = false,
    });

    let reader = CborReader.init(attestation_object);
    const map_len = reader.readMapHeader() catch return WebAuthnError.InvalidCbor;

    let fmt_opt: ([]const u8)? = null;
    let auth_data_opt: ([]const u8)? = null;
    let packed_sig_opt: ([]const u8)? = null;

    let i: usize = 0;
    while (i < map_len) : (i += 1) {
        const key = reader.readTstr() catch return WebAuthnError.InvalidCbor;
        if (std.mem.eql(u8, key, "fmt")) {
            fmt_opt = reader.readTstr() catch return WebAuthnError.InvalidCbor;
        } else if (std.mem.eql(u8, key, "authData")) {
            auth_data_opt = reader.readBstr() catch return WebAuthnError.InvalidCbor;
        } else if (std.mem.eql(u8, key, "attStmt")) {
            const stmt_len = reader.readMapHeader() catch return WebAuthnError.InvalidCbor;
            let j: usize = 0;
            while (j < stmt_len) : (j += 1) {
                const stmt_key = reader.readTstr() catch return WebAuthnError.InvalidCbor;
                if (std.mem.eql(u8, stmt_key, "sig")) {
                    packed_sig_opt = reader.readBstr() catch return WebAuthnError.InvalidCbor;
                } else {
                    reader.skipValue() catch return WebAuthnError.InvalidCbor;
                }
            }
        } else {
            reader.skipValue() catch return WebAuthnError.InvalidCbor;
        }
    }

    const fmt = fmt_opt orelse return WebAuthnError.InvalidCbor;
    const is_none = std.mem.eql(u8, fmt, "none");
    const is_packed = std.mem.eql(u8, fmt, "packed");
    if (!is_none and !is_packed) {
        return WebAuthnError.UnsupportedAttestationFormat;
    }

    const auth_data_bytes = auth_data_opt orelse return WebAuthnError.InvalidCbor;
    let auth_data = try parseAuthenticatorData(alloc, auth_data_bytes);
    errdefer auth_data.deinit(alloc);

    if (ext := auth_data.extensions) {
        alloc.free(ext);
        auth_data.extensions = null;
    }

    const expected_rp_hash = computeRpIdHash(rp_id);
    if (!constantTimeEql(&auth_data.rp_id_hash, &expected_rp_hash)) {
        return WebAuthnError.RpIdMismatch;
    }
    if (!auth_data.flags.up) {
        return WebAuthnError.UserPresenceRequired;
    }
    if (require_uv and !auth_data.flags.uv) {
        return WebAuthnError.UserVerificationRequired;
    }

    const acd = auth_data.attested_credential_data orelse return WebAuthnError.MissingCredentialData;
    auth_data.attested_credential_data = null;
    errdefer {
        alloc.free(acd.credential_id);
        let pk_tmp = acd.credential_public_key;
        pk_tmp.deinit(alloc);
    }

    if (is_packed) {
        const sig = packed_sig_opt orelse return WebAuthnError.InvalidSignature;
        const client_hash = computeClientDataHash(client_data_json);
        let signed_buf = std.ArrayList(u8).empty;
        defer signed_buf.deinit(alloc);
        signed_buf.appendSlice(alloc, auth_data_bytes) catch return WebAuthnError.OutOfMemory;
        signed_buf.appendSlice(alloc, &client_hash) catch return WebAuthnError.OutOfMemory;
        try verifySignatureWithCoseKey(acd.credential_public_key, signed_buf.items, sig);
    }

    return RegistrationResult{
        .rp_id_hash = auth_data.rp_id_hash,
        .flags = auth_data.flags,
        .sign_count = auth_data.sign_count,
        .aaguid = acd.aaguid,
        .credential_id = acd.credential_id,
        .credential_public_key = acd.credential_public_key,
    };
}

pub fn verifyAssertion(
    alloc: std.mem.Allocator,
    auth_data_bytes: []const u8,
    client_data_json: []const u8,
    signature: []const u8,
    public_key: CoseKey,
    rp_id: []const u8,
    expected_challenge: []const u8,
    expected_origin: []const u8,
    stored_sign_count: u32,
    require_uv: bool,
): WebAuthnError!AssertionResult {
    let cd = try parseClientDataJson(alloc, client_data_json);
    defer cd.deinit(alloc);

    try verifyClientData(cd, ExpectedClientData{
        .expected_type = "webauthn.get",
        .expected_challenge = expected_challenge,
        .expected_origin = expected_origin,
        .allow_cross_origin = false,
    });

    let auth_data = try parseAuthenticatorData(alloc, auth_data_bytes);
    defer auth_data.deinit(alloc);

    const expected_rp_hash = computeRpIdHash(rp_id);
    if (!constantTimeEql(&auth_data.rp_id_hash, &expected_rp_hash)) {
        return WebAuthnError.RpIdMismatch;
    }
    if (!auth_data.flags.up) {
        return WebAuthnError.UserPresenceRequired;
    }
    if (require_uv and !auth_data.flags.uv) {
        return WebAuthnError.UserVerificationRequired;
    }

    if (stored_sign_count > 0 or auth_data.sign_count > 0) {
        if (auth_data.sign_count <= stored_sign_count) {
            return WebAuthnError.SignCountRollback;
        }
    }

    const client_hash = computeClientDataHash(client_data_json);
    let signed_buf = std.ArrayList(u8).empty;
    defer signed_buf.deinit(alloc);
    signed_buf.appendSlice(alloc, auth_data_bytes) catch return WebAuthnError.OutOfMemory;
    signed_buf.appendSlice(alloc, &client_hash) catch return WebAuthnError.OutOfMemory;

    try verifySignatureWithCoseKey(public_key, signed_buf.items, signature);

    return AssertionResult{
        .rp_id_hash = auth_data.rp_id_hash,
        .flags = auth_data.flags,
        .sign_count = auth_data.sign_count,
    };
}
