// Source: cp/lib/cbor/value.cp (Shared Compact Binary Value AST for CBOR & MessagePack)
// Pure CP Arena-backed AST, error definitions, deep structural equality & BinDocument
// shared between RFC 8949 CBOR and MessagePack codecs.

const std = @import("std");

pub err BinError {
    UnexpectedEof,
    InvalidFormat,
    UnsupportedFeature,
    MaxDepthExceeded,
    TrailingBytes,
    OutOfMemory,
}

pub struct BinTag {
    tag: u64 = 0,
    value: *const BinValue = undefined,
}

pub struct BinMapEntry {
    key: BinValue = .Null,
    val: BinValue = .Null,

    pub static fn init(key: BinValue, val: BinValue): BinMapEntry {
        return BinMapEntry{ key: key, val: val };
    }
}

pub enum BinValue {
    Null,
    Bool: bool,
    Uint: u64,
    Int: i64,
    Float: f64,
    Bytes: []const u8,
    Text: []const u8,
    Array: []BinValue,
    Map: []BinMapEntry,
    Tag: BinTag,

    static fn makeNull(): BinValue {
        return .Null;
    }

    static fn makeBool(v: bool): BinValue {
        return BinValue.Bool(v);
    }

    static fn makeUint(v: u64): BinValue {
        return BinValue.Uint(v);
    }

    static fn makeInt(v: i64): BinValue {
        return BinValue.Int(v);
    }

    static fn makeFloat(v: f64): BinValue {
        return BinValue.Float(v);
    }

    static fn makeBytes(v: []const u8): BinValue {
        return BinValue.Bytes(v);
    }

    static fn makeText(v: []const u8): BinValue {
        return BinValue.Text(v);
    }

    static fn makeArray(items: []BinValue): BinValue {
        return BinValue.Array(items);
    }

    static fn makeMap(entries: []BinMapEntry): BinValue {
        return BinValue.Map(entries);
    }

    static fn makeTag(tag: u64, value_ptr: *const BinValue): BinValue {
        return BinValue.Tag(BinTag{ tag: tag, value: value_ptr });
    }

    static fn allocTag(alloc: std.mem.Allocator, tag: u64, inner: BinValue): BinValue! {
        let boxed = alloc.create(BinValue) catch return err.OutOfMemory;
        unsafe {
            boxed.* = inner;
        }
        return BinValue.Tag(BinTag{ tag: tag, value: boxed });
    }

    fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    fn asBool(): bool? {
        return switch (_) {
            .Bool b => b,
            else => null,
        };
    }

    fn asUint(): u64? {
        return switch (_) {
            .Uint u => u,
            .Int i => if (i >= 0) (i as u64) else null,
            else => null,
        };
    }

    fn asInt(): i64? {
        return switch (_) {
            .Int i => i,
            .Uint u => if (u <= 9223372036854775807) (u as i64) else null,
            else => null,
        };
    }

    fn asFloat(): f64? {
        return switch (_) {
            .Float f => f,
            else => null,
        };
    }

    fn asBytes(): ([]const u8)? {
        return switch (_) {
            .Bytes bs => bs,
            else => null,
        };
    }

    fn asText(): ([]const u8)? {
        return switch (_) {
            .Text s => s,
            else => null,
        };
    }

    fn asArray(): ([]BinValue)? {
        return switch (_) {
            .Array items => items,
            else => null,
        };
    }

    fn asMap(): ([]BinMapEntry)? {
        return switch (_) {
            .Map entries => entries,
            else => null,
        };
    }

    fn asTag(): BinTag? {
        return switch (_) {
            .Tag t => t,
            else => null,
        };
    }

    fn len(): usize {
        return switch (_) {
            .Bytes bs => bs.len,
            .Text s => s.len,
            .Array items => items.len,
            .Map entries => entries.len,
            else => 0,
        };
    }

    fn get(key_text: []const u8): BinValue? {
        const entries = _.asMap() ?? return null;
        for (entry of entries) {
            if (k := entry.key.asText()) {
                if (std.mem.eql(u8, k, key_text)) {
                    return entry.val;
                }
            }
        }
        return null;
    }

    fn getIndex(idx: usize): BinValue? {
        const items = _.asArray() ?? return null;
        if (idx >= items.len) return null;
        return items[idx];
    }

    fn eql(other: BinValue): bool {
        return switch (_) {
            .Null => switch (other) {
                .Null => true,
                else => false,
            },
            .Bool a => switch (other) {
                .Bool b => a == b,
                else => false,
            },
            .Uint a => switch (other) {
                .Uint b => a == b,
                .Int b => b >= 0 && a == (b as u64),
                else => false,
            },
            .Int a => switch (other) {
                .Int b => a == b,
                .Uint b => a >= 0 && (a as u64) == b,
                else => false,
            },
            .Float a => switch (other) {
                .Float b => {
                    if (std.math.isNan(a) && std.math.isNan(b)) return true;
                    return a == b;
                },
                else => false,
            },
            .Bytes a => switch (other) {
                .Bytes b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Text a => switch (other) {
                .Text b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Array a => switch (other) {
                .Array b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].eql(b[i])) return false;
                    }
                    return true;
                },
                else => false,
            },
            .Map a => switch (other) {
                .Map b => {
                    if (a.len != b.len) return false;
                    let i: usize = 0;
                    while (i < a.len) : (i += 1) {
                        if (!a[i].key.eql(b[i].key)) return false;
                        if (!a[i].val.eql(b[i].val)) return false;
                    }
                    return true;
                },
                else => false,
            },
            .Tag a => switch (other) {
                .Tag b => {
                    if (a.tag != b.tag) return false;
                    return a.value.eql(b.value.*);
                },
                else => false,
            },
        };
    }
}

pub struct BinDocument {
    root: BinValue = .Null,
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    pub static fn create(alloc: std.mem.Allocator): BinDocument! {
        let arena_ptr = alloc.create(std.heap.ArenaAllocator) catch return err.OutOfMemory;
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(alloc);
        }
        return BinDocument{
            root: .Null,
            owned_arena: arena_ptr,
            parent_alloc: alloc,
        };
    }

    pub fn allocator(): std.mem.Allocator? {
        if (owned := _.owned_arena) {
            return owned.allocator();
        }
        return null;
    }

    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.root = .Null;
    }

    pub fn value(): BinValue {
        return _.root;
    }

    pub fn eql(other: BinDocument): bool {
        return _.root.eql(other.root);
    }
}
