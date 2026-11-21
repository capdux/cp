// Source: Absorbed from /data/dev/bun/src/sql/shared/Data.cp | Synced: 2026-11-20T09:26:25Z
// lib/sql/shared.cp
// Shared SQL Runtime Types: Small-Buffer-Optimized SqlData, ColumnIdentifier, ConnectionFlags & Result Modes

const std = @import("std");

pub enum SQLQueryResultMode: u8 {
    objects = 0,
    values = 1,
    raw = 2,
}

pub struct ConnectionFlags {
    pub is_ready: bool = false,
    pub is_processing: bool = false,
    pub in_transaction: bool = false,
    pub is_closed: bool = false,
    pub has_pending_auth: bool = false,
    pub tls_active: bool = false,
    pub binary_protocol: bool = true,
    pub prepared_cache_enabled: bool = true,
}

pub enum SqlDataTag: u8 {
    empty = 0,
    temporary = 1,
    inline_storage = 2,
    owned = 3,
}

pub struct SqlData {
    pub tag: SqlDataTag = .empty,
    pub temp_slice: []const u8 = "",
    pub inline_buf: [15]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    pub inline_len: u8 = 0,
    pub owned_buf: []u8 = [],

    pub static fn empty(): SqlData {
        return SqlData{ tag: .empty };
    }

    pub static fn temporary(bytes: []const u8): SqlData {
        if (bytes.len == 0) return SqlData{ tag: .empty };
        return SqlData{
            tag: .temporary,
            temp_slice: bytes,
        };
    }

    pub static fn create(bytes: []const u8, allocator: std.mem.Allocator): SqlData! {
        if (bytes.len == 0) {
            return SqlData{ tag: .empty };
        }
        if (bytes.len <= 15) {
            let d = SqlData{
                tag: .inline_storage,
                inline_len: bytes.len as u8,
            };
            @memcpy(d.inline_buf[0..bytes.len], bytes);
            return d;
        }
        const duped = allocator.dupe(u8, bytes)?;
        return SqlData{
            tag: .owned,
            owned_buf: duped,
        };
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.tag == .owned && _.owned_buf.len > 0) {
            allocator.free(_.owned_buf);
            _.owned_buf = [];
        }
        _.tag = .empty;
        _.temp_slice = "";
        _.inline_len = 0;
    }

    pub fn zdeinit(*_, allocator: std.mem.Allocator) {
        if (_.tag == .owned && _.owned_buf.len > 0) {
            @memset(_.owned_buf, 0);
            allocator.free(_.owned_buf);
            _.owned_buf = [];
        } else if (_.tag == .inline_storage) {
            @memset(&_.inline_buf, 0);
        }
        _.tag = .empty;
        _.temp_slice = "";
        _.inline_len = 0;
    }

    pub fn slice(*_): []const u8 {
        return switch (_.tag) {
            .empty => "",
            .temporary => _.temp_slice,
            .inline_storage => _.inline_buf[0..(_.inline_len as usize)],
            .owned => _.owned_buf,
        };
    }

    pub fn len(): usize {
        return switch (_.tag) {
            .empty => 0,
            .temporary => _.temp_slice.len,
            .inline_storage => _.inline_len as usize,
            .owned => _.owned_buf.len,
        };
    }

    pub fn isEmpty(): bool {
        return _.len() == 0;
    }

    pub fn substring(*_, start_index: usize, end_index: usize): SqlData {
        const s = _.slice();
        if (start_index >= end_index || end_index > s.len) {
            return SqlData{ tag: .empty };
        }
        return SqlData.temporary(s[start_index..end_index]);
    }

    pub fn toOwned(*_, allocator: std.mem.Allocator): ([]u8)! {
        return allocator.dupe(u8, _.slice());
    }
}

pub struct ColumnIdentifier {
    pub is_index: bool = false,
    pub index: u32 = 0,
    pub name: SqlData = SqlData.empty(),

    pub static fn fromName(name_bytes: []const u8, allocator: std.mem.Allocator): ColumnIdentifier! {
        const idx = std.fmt.parseInt(u32, name_bytes, 10) catch {
            return ColumnIdentifier{
                is_index: false,
                name: SqlData.create(name_bytes, allocator)?,
            };
        };
        return ColumnIdentifier{
            is_index: true,
            index: idx,
        };
    }

    pub static fn fromIndex(idx: u32): ColumnIdentifier {
        return ColumnIdentifier{
            is_index: true,
            index: idx,
        };
    }

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        _.name.deinit(allocator);
    }

    pub fn displayName(*_, buf: []u8): []const u8 {
        if (_.is_index) {
            return std.fmt.bufPrint(buf, "{d}", .{_.index}) catch "";
        }
        return _.name.slice();
    }
}
