// Source: cp/lib/capnp/types.cp (Cap'n Proto core types, errors, traversal limits, and LE helpers)
const std = @import("std");

pub const WORD_SIZE: usize = 8;
pub const MAX_SEGMENTS: u32 = 512;
pub const DEFAULT_TRAVERSAL_LIMIT_WORDS: u64 = 8 * 1024 * 1024; // 64 MiB
pub const DEFAULT_NESTING_LIMIT: u32 = 64;

pub const CapnpError = error{
    InvalidFrame,
    TruncatedFrame,
    TooManySegments,
    MessageTooLarge,
    UnalignedData,
    OutOfBounds,
    InvalidPointer,
    NullPointer,
    TraversalLimitExceeded,
    NestingLimitExceeded,
    InvalidText,
    InvalidFarPointer,
    AllocationFailed,
    UnsupportedElementSize,
};

pub enum PointerKind: u2 {
    Struct = 0,
    List = 1,
    Far = 2,
    Other = 3,
}

pub enum ElementSize: u3 {
    Void = 0,
    Bit = 1,
    Byte = 2,
    TwoBytes = 3,
    FourBytes = 4,
    EightBytes = 5,
    Pointer = 6,
    Composite = 7,
}

pub fn elementByteSize(es: ElementSize): usize {
    return switch (es) {
        .Void => 0,
        .Bit => 0,
        .Byte => 1,
        .TwoBytes => 2,
        .FourBytes => 4,
        .EightBytes => 8,
        .Pointer => 8,
        .Composite => 8,
    };
}

pub struct TraversalLimit {
    words_remaining: u64,
    nesting_limit: u32,

    pub fn init(max_words: u64, max_nesting: u32): TraversalLimit {
        return TraversalLimit{
            .words_remaining = max_words,
            .nesting_limit = max_nesting,
        };
    }

    pub fn defaultLimit(): TraversalLimit {
        return TraversalLimit{
            .words_remaining = DEFAULT_TRAVERSAL_LIMIT_WORDS,
            .nesting_limit = DEFAULT_NESTING_LIMIT,
        };
    }

    pub fn consume(*_, words: u64): CapnpError!void {
        if (words > _.words_remaining) {
            return CapnpError.TraversalLimitExceeded;
        }
        _.words_remaining -= words;
    }

    pub fn enterNesting(): CapnpError!u32 {
        if (_.nesting_limit == 0) {
            return CapnpError.NestingLimitExceeded;
        }
        return _.nesting_limit - 1;
    }
}

pub fn readU16LE(buf: []const u8, off: usize): u16 {
    return std.mem.readInt(u16, buf[off..][0..2], .little);
}

pub fn readU32LE(buf: []const u8, off: usize): u32 {
    return std.mem.readInt(u32, buf[off..][0..4], .little);
}

pub fn readU64LE(buf: []const u8, off: usize): u64 {
    return std.mem.readInt(u64, buf[off..][0..8], .little);
}

pub fn writeU16LE(buf: []u8, off: usize, val: u16)  {
    std.mem.writeInt(u16, buf[off..][0..2], val, .little);
}

pub fn writeU32LE(buf: []u8, off: usize, val: u32)  {
    std.mem.writeInt(u32, buf[off..][0..4], val, .little);
}

pub fn writeU64LE(buf: []u8, off: usize, val: u64)  {
    std.mem.writeInt(u64, buf[off..][0..8], val, .little);
}

pub fn signExtend30(raw: u32): i32 {
    const masked: u32 = raw & 0x3FFF_FFFF;
    if ((masked & 0x2000_0000) != 0) {
        return @bitCast(masked | 0xC000_0000);
    }
    return @bitCast(masked);
}

pub fn signExtend29(raw: u32): i32 {
    const masked: u32 = raw & 0x1FFF_FFFF;
    if ((masked & 0x1000_0000) != 0) {
        return @bitCast(masked | 0xE000_0000);
    }
    return @bitCast(masked);
}
