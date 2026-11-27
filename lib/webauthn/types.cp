// Source: cp/lib/webauthn/types.cp (FIDO2 / WebAuthn Level 3 core types and errors)
const std = @import("std");
import { CoseKey } from "cap:cose";

pub const WebAuthnError = error{
    InvalidLength,
    InvalidFlags,
    InvalidCbor,
    InvalidCoseKey,
    InvalidClientData,
    TypeMismatch,
    ChallengeMismatch,
    OriginMismatch,
    CrossOriginNotAllowed,
    RpIdMismatch,
    UserPresenceRequired,
    UserVerificationRequired,
    SignCountRollback,
    InvalidSignature,
    UnsupportedAlgorithm,
    UnsupportedAttestationFormat,
    MissingCredentialData,
    TrailingBytes,
    OutOfMemory,
};

pub const AuthFlags = struct {
    up: bool,
    uv: bool,
    be: bool,
    bs: bool,
    at: bool,
    ed: bool,
    raw: u8,

    pub static fn fromByte(b: u8): AuthFlags {
        return AuthFlags{
            .up = (b & 0x01) != 0,
            .uv = (b & 0x04) != 0,
            .be = (b & 0x08) != 0,
            .bs = (b & 0x10) != 0,
            .at = (b & 0x40) != 0,
            .ed = (b & 0x80) != 0,
            .raw = b,
        };
    }

    pub fn toByte(): u8 {
        let b: u8 = 0;
        if (_.up) b |= 0x01;
        if (_.uv) b |= 0x04;
        if (_.be) b |= 0x08;
        if (_.bs) b |= 0x10;
        if (_.at) b |= 0x40;
        if (_.ed) b |= 0x80;
        return b;
    }
};

pub const AttestedCredData = struct {
    aaguid: [16]u8,
    credential_id: []u8,
    credential_public_key: CoseKey,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.credential_id);
        _.credential_public_key.deinit(alloc);
    }
};

pub const AuthenticatorData = struct {
    rp_id_hash: [32]u8,
    flags: AuthFlags,
    sign_count: u32,
    attested_credential_data: AttestedCredData? = null,
    extensions: ([]u8)? = null,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (*acd := _.attested_credential_data) {
            acd.deinit(alloc);
            _.attested_credential_data = null;
        }
        if (ext := _.extensions) {
            alloc.free(ext);
            _.extensions = null;
        }
    }
};

pub const ClientData = struct {
    type_str: []u8,
    challenge: []u8,
    origin: []u8,
    cross_origin: bool? = null,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.type_str);
        alloc.free(_.challenge);
        alloc.free(_.origin);
    }
};

pub const ExpectedClientData = struct {
    expected_type: []const u8,
    expected_challenge: []const u8,
    expected_origin: []const u8,
    allow_cross_origin: bool = false,
};

pub const RegistrationResult = struct {
    rp_id_hash: [32]u8,
    flags: AuthFlags,
    sign_count: u32,
    aaguid: [16]u8,
    credential_id: []u8,
    credential_public_key: CoseKey,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.credential_id);
        _.credential_public_key.deinit(alloc);
    }
};

pub const AssertionResult = struct {
    rp_id_hash: [32]u8,
    flags: AuthFlags,
    sign_count: u32,
};
