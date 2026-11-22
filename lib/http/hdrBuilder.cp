// Source: Absorbed from /data/dev/bun/src/http/HeaderBuilder.cp | Synced: 2026-11-22T11:12:13Z
// lib/http/header_builder.cp
// Zero-copy contiguous wire serialization & span-indexed HTTP HeaderBuilder

const std = @import("std");
import { HeaderEntry, Headers } from "./headers";

pub const MAX_BUILDER_HEADERS: usize = 64;

pub struct HeaderSpan {
    pub name_off: u32 = 0;
    pub name_len: u32 = 0;
    pub value_off: u32 = 0;
    pub value_len: u32 = 0;
}

pub struct HeaderBuilder {
    pub buf: []u8 = [],
    pub len: usize = 0;
    pub spans: [MAX_BUILDER_HEADERS]HeaderSpan = undefined;
    pub span_count: usize = 0;
    pub allocator: (std.mem.Allocator)? = null;
    pub owns_buf: bool = false;
    pub finalized: bool = false;

    /// Initializes a zero-allocation `HeaderBuilder` writing directly into caller-provided `out_buf`.
    pub static fn init(out_buf: []u8): HeaderBuilder {
        return HeaderBuilder{
            buf: out_buf,
            len: 0,
            spans: undefined,
            span_count: 0,
            allocator: null,
            owns_buf: false,
            finalized: false,
        };
    }

    /// Initializes a heap-backed `HeaderBuilder` with initial capacity.
    pub static fn initAlloc(allocator: std.mem.Allocator, initial_cap: usize = 1024): !HeaderBuilder {
        const cap = if (initial_cap < 64) 64 else initial_cap;
        const mem = try allocator.alloc(u8, cap);
        return HeaderBuilder{
            buf: mem,
            len: 0,
            spans: undefined,
            span_count: 0,
            allocator: allocator,
            owns_buf: true,
            finalized: false,
        };
    }

    pub fn deinit(*_) {
        if (_.owns_buf && _.buf.len > 0) {
            if (alloc := _.allocator) {
                alloc.free(_.buf);
            }
            _.buf = [];
        }
        _.len = 0;
        _.span_count = 0;
        _.owns_buf = false;
    }

    pub fn reset(*_) {
        _.len = 0;
        _.span_count = 0;
        _.finalized = false;
    }

    fn ensureCapacity(*_, extra: usize): bool {
        if (_.len + extra <= _.buf.len) return true;
        if (!_.owns_buf || _.allocator == null) return false;
        const alloc = _.allocator.?;
        let new_cap = if (_.buf.len == 0) 256 else _.buf.len * 2;
        while (new_cap < _.len + extra) {
            new_cap *= 2;
        }
        const grown = alloc.realloc(_.buf, new_cap) catch return false;
        _.buf = grown;
        return true;
    }

    /// Appends a single `"Name: Value\r\n"` header line into the contiguous wire buffer.
    pub fn append(*_, name: []const u8, value: []const u8): bool {
        if (_.finalized || _.span_count >= MAX_BUILDER_HEADERS) return false;
        if (name.len == 0) return false;
        const needed = name.len + 2 + value.len + 2;
        if (!_.ensureCapacity(needed)) return false;

        const name_off = _.len as u32;
        @memcpy(_.buf[_.len .. _.len + name.len], name);
        _.len += name.len;

        _.buf[_.len] = ':';
        _.buf[_.len + 1] = ' ';
        _.len += 2;

        const value_off = _.len as u32;
        if (value.len > 0) {
            @memcpy(_.buf[_.len .. _.len + value.len], value);
            _.len += value.len;
        }

        _.buf[_.len] = '\r';
        _.buf[_.len + 1] = '\n';
        _.len += 2;

        _.spans[_.span_count] = HeaderSpan{
            name_off: name_off,
            name_len: name.len as u32,
            value_off: value_off,
            value_len: value.len as u32,
        };
        _.span_count += 1;
        return true;
    }

    /// Appends an integer-valued header (e.g. `Content-Length`).
    pub fn appendInt(*_, name: []const u8, value: i64): bool {
        let num_buf: [32]u8 = undefined;
        const num_str = std.fmt.bufPrint(&num_buf, "{d}", .{value}) catch return false;
        return _.append(name, num_str);
    }

    pub fn appendEntry(*_, entry: HeaderEntry): bool {
        return _.append(entry.name, entry.value);
    }

    /// Appends all entries from an existing `Headers` collection.
    pub fn appendHeaders(*_, headers: Headers): bool {
        let i: usize = 0;
        while (i < headers.count()) : (i += 1) {
            if (entry := headers.at(i)) {
                if (!_.append(entry.name, entry.value)) return false;
            }
        }
        return true;
    }

    /// Appends the HTTP/1.1 request line `"METHOD /path HTTP/1.1\r\n"`.
    pub fn writeRequestLine(*_, method: []const u8, path: []const u8): bool {
        if (_.len != 0 || _.finalized) return false;
        const p = if (path.len == 0) "/" else path;
        const needed = method.len + 1 + p.len + 11;
        if (!_.ensureCapacity(needed)) return false;
        const line = std.fmt.bufPrint(_.buf[_.len..], "{s} {s} HTTP/1.1\r\n", .{ method, p }) catch return false;
        _.len += line.len;
        return true;
    }

    /// Appends the HTTP/1.1 status line `"HTTP/1.1 CODE PHRASE\r\n"`.
    pub fn writeStatusLine(*_, status: u16, phrase: []const u8): bool {
        if (_.len != 0 || _.finalized) return false;
        const needed = 16 + phrase.len;
        if (!_.ensureCapacity(needed)) return false;
        const line = std.fmt.bufPrint(_.buf[_.len..], "HTTP/1.1 {d} {s}\r\n", .{ status, phrase }) catch return false;
        _.len += line.len;
        return true;
    }

    pub fn count(): usize {
        return _.span_count;
    }

    pub fn has(name: []const u8): bool {
        return _.get(name) != null;
    }

    /// Zero-copy case-insensitive lookup of the latest header value matching `name`.
    pub fn get(name: []const u8): ([]const u8)? {
        let i: usize = _.span_count;
        while (i > 0) {
            i -= 1;
            const sp = _.spans[i];
            const n = _.buf[(sp.name_off as usize) .. (sp.name_off as usize) + (sp.name_len as usize)];
            if (std.ascii.eqlIgnoreCase(n, name)) {
                return _.buf[(sp.value_off as usize) .. (sp.value_off as usize) + (sp.value_len as usize)];
            }
        }
        return null;
    }

    pub fn at(index: usize): HeaderEntry? {
        if (index >= _.span_count) return null;
        const sp = _.spans[index];
        return HeaderEntry{
            name: _.buf[(sp.name_off as usize) .. (sp.name_off as usize) + (sp.name_len as usize)],
            value: _.buf[(sp.value_off as usize) .. (sp.value_off as usize) + (sp.value_len as usize)],
        };
    }

    /// Returns the raw accumulated bytes (without forcing the final CRLF).
    pub fn bytes(): []const u8 {
        return _.buf[0.._.len];
    }

    /// Appends the terminating `"\r\n"` blank line (idempotent) and returns the complete header block.
    pub fn finish(*_): ([]const u8)! {
        if (!_.finalized) {
            if (!_.ensureCapacity(2)) return error.BufferTooSmall;
            _.buf[_.len] = '\r';
            _.buf[_.len + 1] = '\n';
            _.len += 2;
            _.finalized = true;
        }
        return _.buf[0.._.len];
    }
}
