// Source: cp/lib/hpack/table.cp (RFC 7541 Appendix A Static Table & Bounded FIFO Dynamic Table)
// Pure CP zero-C-dependency HPACK header table management with 61 static entries
// and O(1) ring-buffer FIFO dynamic table eviction.

const std = @import("std");
import { HpackError } from "./huffman.cp";

pub const STATIC_TABLE_LEN: usize = 61;
pub const ENTRY_OVERHEAD: usize = 32;

/// Represents a single HTTP/2 header name-value pair.
pub struct HeaderField {
    name: []const u8 = "",
    value: []const u8 = "",

    pub static fn init(name: []const u8, value: []const u8): HeaderField {
        return HeaderField{ name: name, value: value };
    }
}

/// Result of searching static + dynamic tables for a header field.
pub struct TableMatch {
    index: usize = 0,
    exact: bool = false,
}

struct OwnedEntry {
    name: []u8,
    value: []u8,
}

const STATIC_NAMES: [61][]const u8 = [
    ":authority",
    ":method",
    ":method",
    ":path",
    ":path",
    ":scheme",
    ":scheme",
    ":status",
    ":status",
    ":status",
    ":status",
    ":status",
    ":status",
    ":status",
    "accept-charset",
    "accept-encoding",
    "accept-language",
    "accept-ranges",
    "accept",
    "access-control-allow-origin",
    "age",
    "allow",
    "authorization",
    "cache-control",
    "content-disposition",
    "content-encoding",
    "content-language",
    "content-length",
    "content-location",
    "content-range",
    "content-type",
    "cookie",
    "date",
    "etag",
    "expect",
    "expires",
    "from",
    "host",
    "if-match",
    "if-modified-since",
    "if-none-match",
    "if-range",
    "if-unmodified-since",
    "last-modified",
    "link",
    "location",
    "max-forwards",
    "proxy-authenticate",
    "proxy-authorization",
    "range",
    "referer",
    "refresh",
    "retry-after",
    "server",
    "set-cookie",
    "strict-transport-security",
    "transfer-encoding",
    "user-agent",
    "vary",
    "via",
    "www-authenticate",
];

const STATIC_VALUES: [61][]const u8 = [
    "",
    "GET",
    "POST",
    "/",
    "/index.html",
    "http",
    "https",
    "200",
    "204",
    "206",
    "304",
    "400",
    "404",
    "500",
    "",
    "gzip, deflate",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
];

/// Look up a 1-based entry in the RFC 7541 Appendix A static table (1..61).
pub fn getStaticEntry(index: usize): HeaderField? {
    if (index == 0 || index > STATIC_TABLE_LEN) {
        return null;
    }
    const idx = index - 1;
    return HeaderField{
        name: STATIC_NAMES[idx],
        value: STATIC_VALUES[idx],
    };
}

/// Bounded FIFO dynamic table for HPACK (RFC 7541 Section 2.3.2 & Section 4).
/// Index 0 in `get(0)` corresponds to the most recently inserted entry (HPACK index 62).
pub struct DynamicTable {
    alloc: std.mem.Allocator,
    entries: []OwnedEntry,
    head: usize,
    count: usize,
    size: usize,
    max_size: usize,

    pub fn init(alloc: std.mem.Allocator, max_size: usize): DynamicTable {
        return DynamicTable{
            alloc: alloc,
            entries: [],
            head: 0,
            count: 0,
            size: 0,
            max_size: max_size,
        };
    }

    pub fn deinit(*_) {
        _.clear();
        if (_.entries.len > 0) {
            _.alloc.free(_.entries);
            _.entries = [];
        }
        _.head = 0;
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn currentSize(): usize {
        return _.size;
    }

    pub fn maxSize(): usize {
        return _.max_size;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const slot = (_.head + i) % _.entries.len;
            _.alloc.free(_.entries[slot].name);
            _.alloc.free(_.entries[slot].value);
        }
        _.count = 0;
        _.head = 0;
        _.size = 0;
    }

    fn evictOldest(*_) {
        if (_.count == 0) return;
        const tail_slot = (_.head + _.count - 1) % _.entries.len;
        const entry = _.entries[tail_slot];
        const entry_size = entry.name.len + entry.value.len + ENTRY_OVERHEAD;
        _.alloc.free(entry.name);
        _.alloc.free(entry.value);
        _.count -= 1;
        if (_.size >= entry_size) {
            _.size -= entry_size;
        } else {
            _.size = 0;
        }
    }

    pub fn setMaxSize(*_, new_max_size: usize) {
        _.max_size = new_max_size;
        while (_.size > _.max_size && _.count > 0) {
            _.evictOldest();
        }
    }

    fn ensureCapacity(*_, needed: usize): ! HpackError {
        if (_.entries.len >= needed) {
            return;
        }
        let new_cap: usize = if (_.entries.len == 0) 16 else _.entries.len * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        let new_buf = _.alloc.alloc(OwnedEntry, new_cap) catch return HpackError.OutOfMemory;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const old_slot = (_.head + i) % _.entries.len;
            new_buf[i] = _.entries[old_slot];
        }
        if (_.entries.len > 0) {
            _.alloc.free(_.entries);
        }
        _.entries = new_buf;
        _.head = 0;
    }

    /// Insert a new header field at the front of the dynamic table.
    /// Per RFC 7541 Section 4.4, if `entry_size > max_size`, the dynamic table is cleared without error.
    pub fn insert(*_, name: []const u8, value: []const u8): ! HpackError {
        if (name.len > std.math.maxInt(usize) - value.len ||
            name.len + value.len > std.math.maxInt(usize) - ENTRY_OVERHEAD)
        {
            _.clear();
            return;
        }
        const entry_size = name.len + value.len + ENTRY_OVERHEAD;
        if (entry_size > _.max_size) {
            _.clear();
            return;
        }

        while (_.size + entry_size > _.max_size && _.count > 0) {
            _.evictOldest();
        }

        _.ensureCapacity(_.count + 1)?;

        const owned_name = _.alloc.dupe(u8, name) catch return HpackError.OutOfMemory;
        const owned_value = _.alloc.dupe(u8, value) catch {
            _.alloc.free(owned_name);
            return HpackError.OutOfMemory;
        };

        const new_head = if (_.head == 0) _.entries.len - 1 else _.head - 1;
        _.entries[new_head] = OwnedEntry{
            name: owned_name,
            value: owned_value,
        };
        _.head = new_head;
        _.count += 1;
        _.size += entry_size;
    }

    /// Get dynamic entry at 0-based offset (0 = most recent = HPACK index 62).
    pub fn get(dyn_index: usize): HeaderField? {
        if (dyn_index >= _.count) {
            return null;
        }
        const slot = (_.head + dyn_index) % _.entries.len;
        const e = _.entries[slot];
        return HeaderField{
            name: e.name,
            value: e.value,
        };
    }
}

/// Unified 1-based HPACK table lookup across static (1..61) and dynamic (62..) tables.
pub fn lookupEntry(dt: &DynamicTable, index: usize): HeaderField? {
    if (index == 0) {
        return null;
    }
    if (index <= STATIC_TABLE_LEN) {
        return getStaticEntry(index);
    }
    return dt.get(index - STATIC_TABLE_LEN - 1);
}

/// Find the best match (preferring exact name+value match, falling back to first name match).
pub fn findBestMatch(dt: &DynamicTable, name: []const u8, value: []const u8): TableMatch {
    let name_match: usize = 0;

    let i: usize = 0;
    while (i < STATIC_TABLE_LEN) : (i += 1) {
        if (std.mem.eql(u8, STATIC_NAMES[i], name)) {
            if (std.mem.eql(u8, STATIC_VALUES[i], value)) {
                return TableMatch{ index: i + 1, exact: true };
            }
            if (name_match == 0) {
                name_match = i + 1;
            }
        }
    }

    let j: usize = 0;
    const dyn_len = dt.len();
    while (j < dyn_len) : (j += 1) {
        if (entry := dt.get(j)) {
            if (std.mem.eql(u8, entry.name, name)) {
                if (std.mem.eql(u8, entry.value, value)) {
                    return TableMatch{ index: STATIC_TABLE_LEN + 1 + j, exact: true };
                }
                if (name_match == 0) {
                    name_match = STATIC_TABLE_LEN + 1 + j;
                }
            }
        }
    }

    return TableMatch{ index: name_match, exact: false };
}
