// Source: cp/lib/intern/intern.cp (StringInterner & StringBiMap Engine)
// Pure CP zero-C-dependency string interning pool (chunked Arena + HashMap -> u32 SymbolId)
// and strict 1:1 bidirectional unique string map (StringBiMap).

const std = @import("std");

/// Error set for string interning and bidirectional map operations.
pub err InternError {
    InvalidSymbolId,
    KeyAlreadyExists,
    ValueAlreadyExists,
    OutOfMemory,
}

/// Compact 32-bit symbol identifier produced by `StringInterner`.
pub type SymbolId = u32;

const DEFAULT_CHUNK_SIZE: usize = 4096;

/// High-performance string interning pool backed by pointer-stable Arena chunks
/// and a hash index mapping unique strings to compact `u32` `SymbolId` values.
pub struct StringInterner {
    alloc: std.mem.Allocator;
    chunks: std.ArrayList([]u8) = .empty;
    cur_chunk_used: usize = 0;
    entries: std.ArrayList(string) = .empty;
    map: std.StringHashMapUnmanaged(SymbolId) = .empty;
    total_bytes: usize = 0;

    /// Initialize an empty `StringInterner` (zero heap allocation until first `intern`).
    pub static fn init(alloc: std.mem.Allocator): StringInterner {
        return StringInterner{
            alloc: alloc,
            chunks: .empty,
            cur_chunk_used: 0,
            entries: .empty,
            map: .empty,
            total_bytes: 0,
        };
    }

    /// Release all Arena chunks, entry table, and hash index storage.
    pub fn deinit(*_) {
        _.map.deinit(_.alloc);
        _.entries.deinit(_.alloc);
        for (chunk of _.chunks.items) {
            if (chunk.len > 0) {
                _.alloc.free(chunk);
            }
        }
        _.chunks.deinit(_.alloc);
        _.cur_chunk_used = 0;
        _.total_bytes = 0;
    }

    /// Intern `s`, returning its existing `SymbolId` if already present or
    /// copying `s` into the Arena and assigning the next sequential `SymbolId`.
    pub fn intern(*_, s: string): SymbolId ! InternError {
        if (existing := _.map.get(s)) {
            return existing;
        }
        if (_.entries.items.len >= std.math.maxInt(u32)) {
            return InternError.OutOfMemory;
        }

        let stored: string = "";
        if (s.len > 0) {
            let need_new_chunk = true;
            if (_.chunks.items.len > 0) {
                const last_chunk = _.chunks.items[_.chunks.items.len - 1];
                if (last_chunk.len - _.cur_chunk_used >= s.len) {
                    need_new_chunk = false;
                }
            }
            if (need_new_chunk) {
                const chunk_cap = if (s.len > DEFAULT_CHUNK_SIZE) s.len else DEFAULT_CHUNK_SIZE;
                const new_chunk = _.alloc.alloc(u8, chunk_cap) catch return InternError.OutOfMemory;
                _.chunks.append(_.alloc, new_chunk) catch {
                    _.alloc.free(new_chunk);
                    return InternError.OutOfMemory;
                };
                _.cur_chunk_used = 0;
            }
            const active_chunk: []u8 = _.chunks.items[_.chunks.items.len - 1];
            const start = _.cur_chunk_used;
            const end = start + s.len;
            @memcpy(active_chunk[start..end], s);
            _.cur_chunk_used = end;
            stored = active_chunk[start..end];
        }

        const id: SymbolId = u32(_.entries.items.len);
        _.entries.append(_.alloc, stored) catch return InternError.OutOfMemory;
        _.map.put(_.alloc, stored, id) catch {
            * = _.entries.pop();
            return InternError.OutOfMemory;
        };
        _.total_bytes += s.len;
        return id;
    }

    /// Read-only lookup returning the `SymbolId` for `s`, or `null` if not interned.
    pub fn lookup(s: string): SymbolId? {
        return _.map.get(s);
    }

    /// Resolve `id` back to its interned `string` slice (zero-copy),
    /// returning `InternError.InvalidSymbolId` if `id` is out of bounds.
    pub fn resolve(id: SymbolId): string ! InternError {
        const idx = usize(id);
        if (idx >= _.entries.items.len) {
            return InternError.InvalidSymbolId;
        }
        return _.entries.items[idx];
    }

    /// Resolve `id` back to its interned `string` slice, or `null` if out of bounds.
    pub fn resolveOpt(id: SymbolId): string? {
        const idx = usize(id);
        if (idx >= _.entries.items.len) {
            return null;
        }
        return _.entries.items[idx];
    }

    /// Return the number of unique interned strings.
    pub fn count(): usize {
        return _.entries.items.len;
    }

    /// Return the total deduplicated byte count of all unique interned strings.
    pub fn totalBytes(): usize {
        return _.total_bytes;
    }
}

/// Strict 1:1 bidirectional unique string map (`key <-> val`).
/// Owns independent heap copies of all stored keys and values.
pub struct StringBiMap {
    alloc: std.mem.Allocator;
    forward: std.StringHashMapUnmanaged([]u8) = .empty;
    backward: std.StringHashMapUnmanaged([]u8) = .empty;

    /// Initialize an empty `StringBiMap`.
    pub static fn init(alloc: std.mem.Allocator): StringBiMap {
        return StringBiMap{
            alloc: alloc,
            forward: .empty,
            backward: .empty,
        };
    }

    /// Release all owned key/value strings and hash tables.
    pub fn deinit(*_) {
        let it = _.forward.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            _.alloc.free(entry.value_ptr.*);
        }
        _.forward.deinit(_.alloc);
        _.backward.deinit(_.alloc);
    }

    /// Insert a strict 1:1 `(key, val)` mapping.
    /// Returns `KeyAlreadyExists` if `key` is already bound to a different value,
    /// or `ValueAlreadyExists` if `val` is already bound to a different key.
    pub fn insert(*_, key: string, val: string): void ! InternError {
        if (existing_val := _.forward.get(key)) {
            if (std.mem.eql(u8, existing_val, val)) {
                return;
            }
            return InternError.KeyAlreadyExists;
        }
        if (_.backward.contains(val)) {
            return InternError.ValueAlreadyExists;
        }

        const owned_k = _.alloc.dupe(u8, key) catch return InternError.OutOfMemory;
        const owned_v = _.alloc.dupe(u8, val) catch {
            _.alloc.free(owned_k);
            return InternError.OutOfMemory;
        };

        _.forward.put(_.alloc, owned_k, owned_v) catch {
            _.alloc.free(owned_k);
            _.alloc.free(owned_v);
            return InternError.OutOfMemory;
        };
        _.backward.put(_.alloc, owned_v, owned_k) catch {
            * = _.forward.remove(owned_k);
            _.alloc.free(owned_k);
            _.alloc.free(owned_v);
            return InternError.OutOfMemory;
        };
    }

    /// Atomically remove any existing association for `key` or `val` and
    /// establish the new 1:1 mapping `key <-> val`.
    pub fn forcePut(*_, key: string, val: string): void ! InternError {
        if (existing_val := _.forward.get(key)) {
            if (std.mem.eql(u8, existing_val, val)) {
                return;
            }
        }

        const owned_k = _.alloc.dupe(u8, key) catch return InternError.OutOfMemory;
        const owned_v = _.alloc.dupe(u8, val) catch {
            _.alloc.free(owned_k);
            return InternError.OutOfMemory;
        };

        _.forward.ensureUnusedCapacity(_.alloc, 1) catch {
            _.alloc.free(owned_k);
            _.alloc.free(owned_v);
            return InternError.OutOfMemory;
        };
        _.backward.ensureUnusedCapacity(_.alloc, 1) catch {
            _.alloc.free(owned_k);
            _.alloc.free(owned_v);
            return InternError.OutOfMemory;
        };

        * = _.removeByKey(key);
        * = _.removeByVal(val);

        _.forward.putAssumeCapacity(owned_k, owned_v);
        _.backward.putAssumeCapacity(owned_v, owned_k);
    }

    /// Look up the value associated with `key`, or `null` if absent.
    pub fn getByKey(key: string): string? {
        if (v := _.forward.get(key)) {
            return v;
        }
        return null;
    }

    /// Look up the key associated with `val`, or `null` if absent.
    pub fn getByVal(val: string): string? {
        if (k := _.backward.get(val)) {
            return k;
        }
        return null;
    }

    /// Remove the mapping for `key` and its reverse `val` entry.
    /// Returns `true` if a mapping was removed.
    pub fn removeByKey(*_, key: string): bool {
        if (kv := _.forward.fetchRemove(key)) {
            * = _.backward.remove(kv.value);
            _.alloc.free(kv.key);
            _.alloc.free(kv.value);
            return true;
        }
        return false;
    }

    /// Remove the mapping for `val` and its forward `key` entry.
    /// Returns `true` if a mapping was removed.
    pub fn removeByVal(*_, val: string): bool {
        if (vk := _.backward.fetchRemove(val)) {
            * = _.forward.remove(vk.value);
            _.alloc.free(vk.value);
            _.alloc.free(vk.key);
            return true;
        }
        return false;
    }

    /// Return the number of 1:1 pairs in the map.
    pub fn len(): usize {
        return _.forward.count();
    }

    /// Return `true` if `key` exists in the map.
    pub fn containsKey(key: string): bool {
        return _.forward.contains(key);
    }

    /// Return `true` if `val` exists in the map.
    pub fn containsVal(val: string): bool {
        return _.backward.contains(val);
    }
}
