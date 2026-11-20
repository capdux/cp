// Source: cp/lib/bplus_tree/types.cp (B+ Tree shared error, entry, and comparator types)

const std = @import("std");

pub const BPlusError = error{
    InvalidOrder,
    OutOfMemory,
    KeyNotFound,
    CorruptedTree,
    PageFull,
    InvalidSlot,
    SlotDeleted,
    InvalidMagic,
    CorruptedChecksum,
    InvalidPageSize,
    CellTooLarge,
};

pub struct KvEntry {
    pub key: []const u8,
    pub val: []const u8,
}

pub fn compareKeys(a: []const u8, b: []const u8): std.math.Order {
    return std.mem.order(u8, a, b);
}
