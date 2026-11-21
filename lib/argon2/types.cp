// Source: cp/lib/argon2/types.cp (RFC 9106 Argon2 types, parameters, and errors)

const std = @import("std");

pub const ARGON2_VERSION: u32 = 19;
pub const DEFAULT_MAX_M_COST_KIB: u32 = 1048576; // 1 GiB DoS cap

pub const Argon2Error = error{
    InvalidParams,
    InvalidSalt,
    OutputTooShort,
    OutputTooLong,
    OutOfMemory,
    InvalidPhcFormat,
    UnsupportedVariant,
    UnsupportedVersion,
    MemoryLimitExceeded,
    VerificationFailed,
};

pub enum Argon2Variant: u8 {
    Argon2d = 0,
    Argon2i = 1,
    Argon2id = 2,

    pub fn toPhcId(): []const u8 {
        return switch (_) {
            .Argon2d => "argon2d",
            .Argon2i => "argon2i",
            .Argon2id => "argon2id",
        };
    }

    pub fn fromPhcId(id: []const u8): Argon2Error!Argon2Variant {
        if (std.mem.eql(u8, id, "argon2d")) return .Argon2d;
        if (std.mem.eql(u8, id, "argon2i")) return .Argon2i;
        if (std.mem.eql(u8, id, "argon2id")) return .Argon2id;
        return Argon2Error.UnsupportedVariant;
    }
}

pub struct Argon2Params {
    variant: Argon2Variant = .Argon2id,
    t_cost: u32 = 2,
    m_cost_kib: u32 = 19456,
    lanes: u24 = 1,
    out_len: usize = 32,
    max_m_cost_kib: u32 = DEFAULT_MAX_M_COST_KIB,
    secret: []const u8 = &.{},
    ad: []const u8 = &.{},

    pub fn validate(salt_len: usize): Argon2Error!void {
        if (_.t_cost < 1 or _.lanes < 1) {
            return Argon2Error.InvalidParams;
        }
        const min_m: u64 = @as(u64, 8) * @as(u64, _.lanes);
        if (@as(u64, _.m_cost_kib) < min_m) {
            return Argon2Error.InvalidParams;
        }
        if (_.m_cost_kib > _.max_m_cost_kib) {
            return Argon2Error.MemoryLimitExceeded;
        }
        if (_.out_len < 4) {
            return Argon2Error.OutputTooShort;
        }
        if (_.out_len > 0xFFFFFFFF) {
            return Argon2Error.OutputTooLong;
        }
        if (salt_len < 8 or salt_len > 0xFFFFFFFF) {
            return Argon2Error.InvalidSalt;
        }
    }
}

pub struct PhcParsed {
    variant: Argon2Variant,
    version: u32,
    m_cost_kib: u32,
    t_cost: u32,
    lanes: u24,
    salt: []u8,
    hash: []u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.salt);
        alloc.free(_.hash);
    }
}
