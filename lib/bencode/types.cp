// Source: cp/lib/bencode/types.cp (BEP-3 Bencode Types & Error Definitions)
// Strict BEP-3 error taxonomy, BencodeDictEntry, and BencodeValue with deep clone/equals/deinit.

const std = @import("std");

pub const BENCODE_MAX_DEPTH: usize = 64;

pub err BencodeError {
    UnexpectedEof,
    InvalidSyntax,
    InvalidInteger,
    LeadingZero,
    NegativeZero,
    UnorderedDictKeys,
    DuplicateDictKey,
    TrailingBytes,
    MaxDepthExceeded,
    MaxSizeExceeded,
    KeyNotFound,
    OutOfMemory,
}

pub enum BencodeKind {
    Int,
    Bytes,
    List,
    Dict,
}

pub struct BencodeDictEntry {
    key: []const u8 = "",
    value: BencodeValue = BencodeValue.Int(0),

    pub static fn init(key: []const u8, value: BencodeValue): BencodeDictEntry {
        return BencodeDictEntry{ key: key, value: value };
    }
}

pub enum BencodeValue {
    Int: i64,
    Bytes: []const u8,
    List: []BencodeValue,
    Dict: []BencodeDictEntry,

    pub static fn makeInt(v: i64): BencodeValue {
        return BencodeValue.Int(v);
    }

    pub static fn makeBytes(v: []const u8): BencodeValue {
        return BencodeValue.Bytes(v);
    }

    pub static fn makeList(items: []BencodeValue): BencodeValue {
        return BencodeValue.List(items);
    }

    pub static fn makeDict(entries: []BencodeDictEntry): BencodeValue {
        return BencodeValue.Dict(entries);
    }

    pub fn kind(): BencodeKind {
        return switch (_) {
            .Int => .Int,
            .Bytes => .Bytes,
            .List => .List,
            .Dict => .Dict,
        };
    }

    pub fn asInt(): i64? {
        return switch (_) {
            .Int v => v,
            else => null,
        };
    }

    pub fn asBytes(): ([]const u8)? {
        return switch (_) {
            .Bytes b => b,
            else => null,
        };
    }

    pub fn asList(): ([]BencodeValue)? {
        return switch (_) {
            .List items => items,
            else => null,
        };
    }

    pub fn asDict(): ([]BencodeDictEntry)? {
        return switch (_) {
            .Dict entries => entries,
            else => null,
        };
    }

    pub fn get(key: []const u8): BencodeValue? {
        return switch (_) {
            .Dict entries => {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    if (std.mem.eql(u8, entries[i].key, key)) {
                        return entries[i].value;
                    }
                }
                return null;
            },
            else => null,
        };
    }

    pub fn getBytes(key: []const u8): ([]const u8)? {
        const val = _.get(key) ?? return null;
        return val.asBytes();
    }

    pub fn getInt(key: []const u8): i64? {
        const val = _.get(key) ?? return null;
        return val.asInt();
    }

    pub fn equals(other: BencodeValue): bool {
        return switch (_) {
            .Int a => switch (other) {
                .Int b => a == b,
                else => false,
            },
            .Bytes a => switch (other) {
                .Bytes b => std.mem.eql(u8, a, b),
                else => false,
            },
            .List a => switch (other) {
                .List b => {
                    if (a.len != b.len) {
                        return false;
                    }
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].equals(b[i])) {
                            return false;
                        }
                    }
                    return true;
                },
                else => false,
            },
            .Dict a => switch (other) {
                .Dict b => {
                    if (a.len != b.len) {
                        return false;
                    }
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!std.mem.eql(u8, a[i].key, b[i].key)) {
                            return false;
                        }
                        if (!a[i].value.equals(b[i].value)) {
                            return false;
                        }
                    }
                    return true;
                },
                else => false,
            },
        };
    }

    pub fn clone(alloc: std.mem.Allocator): BencodeValue ! BencodeError {
        return switch (_) {
            .Int v => BencodeValue.Int(v),
            .Bytes b => {
                const duped = alloc.dupe(u8, b) catch return BencodeError.OutOfMemory;
                return BencodeValue.Bytes(duped);
            },
            .List items => {
                let out = alloc.alloc(BencodeValue, items.len) catch return BencodeError.OutOfMemory;
                let count: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < count) : (j += 1) {
                        out[j].deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (count < items.len) : (count += 1) {
                    out[count] = items[count].clone(alloc)?;
                }
                return BencodeValue.List(out);
            },
            .Dict entries => {
                let out = alloc.alloc(BencodeDictEntry, entries.len) catch return BencodeError.OutOfMemory;
                let count: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < count) : (j += 1) {
                        alloc.free(out[j].key);
                        out[j].value.deinit(alloc);
                    }
                    alloc.free(out);
                }
                while (count < entries.len) : (count += 1) {
                    const k_dup = alloc.dupe(u8, entries[count].key) catch return BencodeError.OutOfMemory;
                    errdefer alloc.free(k_dup);
                    const v_dup = entries[count].value.clone(alloc)?;
                    out[count] = BencodeDictEntry{ key: k_dup, value: v_dup };
                }
                return BencodeValue.Dict(out);
            },
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Int => {},
            .Bytes b => {
                alloc.free(b);
            },
            .List items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                alloc.free(items);
            },
            .Dict entries => {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    alloc.free(entries[i].key);
                    entries[i].value.deinit(alloc);
                }
                alloc.free(entries);
            },
        }
        _.* = BencodeValue.Int(0);
    }
}
