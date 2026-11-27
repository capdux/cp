// Source: cp/lib/cose/types.cp (RFC 9052 / RFC 9053 COSE core types and constants)
const std = @import("std");

pub const COSE_TAG_MAC0: u64 = 17;
pub const COSE_TAG_SIGN1: u64 = 18;

pub const CoseError = error{
    InvalidCbor,
    UnexpectedType,
    UnexpectedTag,
    MissingField,
    InvalidAlgorithm,
    InvalidKeyType,
    InvalidCurve,
    InvalidKeyLength,
    InvalidSignature,
    InvalidMac,
    UnsupportedAlgorithm,
    BufferTooSmall,
    OutOfMemory,
};

pub const CoseAlgorithm = enum(i32) {
    ES256 = -7,
    EdDSA = -8,
    HS256 = 5,
    RS256 = -257,

    pub static fn fromInt(val: i64): !CoseAlgorithm {
        return switch (val) {
            -7 => .ES256,
            -8 => .EdDSA,
            5 => .HS256,
            -257 => .RS256,
            else => CoseError.UnsupportedAlgorithm,
        };
    }

    pub fn toInt(): i32 {
        return @intFromEnum(_);
    }
};

pub const CoseKeyType = enum(i32) {
    OKP = 1,
    EC2 = 2,
    RSA = 3,
    Symmetric = 4,

    pub static fn fromInt(val: i64): !CoseKeyType {
        return switch (val) {
            1 => .OKP,
            2 => .EC2,
            3 => .RSA,
            4 => .Symmetric,
            else => CoseError.InvalidKeyType,
        };
    }

    pub fn toInt(): i32 {
        return @intFromEnum(_);
    }
};

pub const CoseCurve = enum(i32) {
    P256 = 1,
    P384 = 2,
    P521 = 3,
    X25519 = 4,
    X448 = 5,
    Ed25519 = 6,
    Ed448 = 7,

    pub static fn fromInt(val: i64): !CoseCurve {
        return switch (val) {
            1 => .P256,
            2 => .P384,
            3 => .P521,
            4 => .X25519,
            5 => .X448,
            6 => .Ed25519,
            7 => .Ed448,
            else => CoseError.InvalidCurve,
        };
    }

    pub fn toInt(): i32 {
        return @intFromEnum(_);
    }
};

pub const CoseHeader = struct {
    alg: i32? = null,
    kid: ([]const u8)? = null,
    content_type: ([]const u8)? = null,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (k := _.kid) {
            alloc.free(k);
            _.kid = null;
        }
        if (ct := _.content_type) {
            alloc.free(ct);
            _.content_type = null;
        }
    }
};
