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

    pub fn fromByte(b: u8) AuthFlags {
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

    pub fn toByte(self: AuthFlags) u8 {
        var b: u8 = 0;
        if (self.up) b |= 0x01;
        if (self.uv) b |= 0x04;
        if (self.be) b |= 0x08;
        if (self.bs) b |= 0x10;
        if (self.at) b |= 0x40;
        if (self.ed) b |= 0x80;
        return b;
    }
};

pub const AttestedCredData = struct {
    aaguid: [16]u8,
    credential_id: []u8,
    credential_public_key: CoseKey,

    pub fn deinit(self: *AttestedCredData, alloc: std.mem.Allocator) void {
        alloc.free(self.credential_id);
        self.credential_public_key.deinit(alloc);
    }
};

pub const AuthenticatorData = struct {
    rp_id_hash: [32]u8,
    flags: AuthFlags,
    sign_count: u32,
    attested_credential_data: ?AttestedCredData = null,
    extensions: ?[]u8 = null,

    pub fn deinit(self: *AuthenticatorData, alloc: std.mem.Allocator) void {
        if (self.attested_credential_data) |*acd| {
            acd.deinit(alloc);
            self.attested_credential_data = null;
        }
        if (self.extensions) |ext| {
            alloc.free(ext);
            self.extensions = null;
        }
    }
};

pub const ClientData = struct {
    type_str: []u8,
    challenge: []u8,
    origin: []u8,
    cross_origin: ?bool = null,

    pub fn deinit(self: *ClientData, alloc: std.mem.Allocator) void {
        alloc.free(self.type_str);
        alloc.free(self.challenge);
        alloc.free(self.origin);
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

    pub fn deinit(self: *RegistrationResult, alloc: std.mem.Allocator) void {
        alloc.free(self.credential_id);
        self.credential_public_key.deinit(alloc);
    }
};

pub const AssertionResult = struct {
    rp_id_hash: [32]u8,
    flags: AuthFlags,
    sign_count: u32,
};
