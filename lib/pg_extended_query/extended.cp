// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Parse.cp, Describe.cp, Execute.cp, Close.cp, PortalOrPreparedStatement.cp
//! PostgreSQL v3 Extended Query Protocol frontend message encoder
//! (`Parse`, `Bind`, `Describe`, `Execute`, `Close`, `Sync`, `Flush`, `Query`, `Terminate`)
//! with deterministic `~init()` RAII cleanup.

import std from "std";

export err PgExtendedQueryError {
    TooManyParameters,
    TooManyResultFormats,
    MessageTooLarge,
    OutOfMemory,
}

export enum TargetKind: u8 {
    statement = 'S',
    portal = 'P',

    pub fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

export enum ParamFormat: u16 {
    text = 0,
    binary = 1,
}

export struct BindParam {
    value: ([]const u8)? = null, // null => SQL NULL (-1)
}

fn appendU16Be(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u16): void! {
    const b: [2]u8 = [@truncate(v >> 8), @truncate(v)];
    buf.appendSlice(alloc, b[0..])?;
}

fn appendU32Be(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void! {
    const b: [4]u8 = [
        @truncate(v >> 24),
        @truncate(v >> 16),
        @truncate(v >> 8),
        @truncate(v),
    ];
    buf.appendSlice(alloc, b[0..])?;
}

fn appendI32Be(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: i32): void! {
    const u: u32 = @bitCast(v);
    appendU32Be(*buf, alloc, u)?;
}

fn appendCString(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void! {
    buf.appendSlice(alloc, s)?;
    buf.append(alloc, 0)?;
}

/// Pipelined PostgreSQL v3 Extended Query Protocol frontend message builder.
export struct ExtendedQueryBuilder {
    allocator: std.mem.Allocator,
    buf: std.ArrayList(u8),

    pub static fn init(allocator: std.mem.Allocator): ExtendedQueryBuilder {
        return ExtendedQueryBuilder{
            allocator: allocator,
            buf: std.ArrayList(u8).empty,
        };
    }

    ~init() {
        _.buf.deinit(_.allocator);
    }

    pub fn bytes(): []const u8 {
        return _.buf.items;
    }

    pub fn len(): usize {
        return _.buf.items.len;
    }

    pub fn reset(*_) {
        _.buf.shrinkRetainingCapacity(0);
    }

    fn beginMsg(*_, tag: u8): usize! {
        const start = _.buf.items.len;
        const hdr: [5]u8 = [tag, 0, 0, 0, 0];
        _.buf.appendSlice(_.allocator, hdr[0..])?;
        return start;
    }

    fn finishMsg(*_, start: usize): void! {
        const msg_len = _.buf.items.len - start - 1; // inclusive of 4-byte length, excludes tag
        if (msg_len > 0x7FFFFFFF) return err.MessageTooLarge;
        const v: u32 = @intCast(msg_len);
        _.buf.items[start + 1] = @truncate(v >> 24);
        _.buf.items[start + 2] = @truncate(v >> 16);
        _.buf.items[start + 3] = @truncate(v >> 8);
        _.buf.items[start + 4] = @truncate(v);
    }

    /// Encode `Parse` ('P') message: statement name, SQL query, and optional parameter OIDs.
    pub fn writeParse(*_, statement_name: []const u8, query: []const u8, param_oids: []const u32): void! {
        if (param_oids.len > 0xFFFF) return err.TooManyParameters;
        const start = _.beginMsg('P')?;
        appendCString(*_.buf, _.allocator, statement_name)?;
        appendCString(*_.buf, _.allocator, query)?;
        appendU16Be(*_.buf, _.allocator, @intCast(param_oids.len))?;
        let i: usize = 0;
        while (i < param_oids.len) : (i += 1) {
            appendU32Be(*_.buf, _.allocator, param_oids[i])?;
        }
        _.finishMsg(start)?;
    }

    /// Encode `Bind` ('B') message: portal, statement, parameter formats/values, result formats.
    pub fn writeBind(
        *_,
        portal_name: []const u8,
        statement_name: []const u8,
        param_formats: []const ParamFormat,
        params: []const BindParam,
        result_formats: []const ParamFormat,
    ): void! {
        if (param_formats.len > 0xFFFF || params.len > 0xFFFF) return err.TooManyParameters;
        if (result_formats.len > 0xFFFF) return err.TooManyResultFormats;
        const start = _.beginMsg('B')?;
        appendCString(*_.buf, _.allocator, portal_name)?;
        appendCString(*_.buf, _.allocator, statement_name)?;

        appendU16Be(*_.buf, _.allocator, @intCast(param_formats.len))?;
        let fi: usize = 0;
        while (fi < param_formats.len) : (fi += 1) {
            appendU16Be(*_.buf, _.allocator, @intFromEnum(param_formats[fi]))?;
        }

        appendU16Be(*_.buf, _.allocator, @intCast(params.len))?;
        let pi: usize = 0;
        while (pi < params.len) : (pi += 1) {
            if (val := params[pi].value) {
                if (val.len > 0x7FFFFFFF) return err.MessageTooLarge;
                appendI32Be(*_.buf, _.allocator, @intCast(val.len))?;
                _.buf.appendSlice(_.allocator, val)?;
            } else {
                appendI32Be(*_.buf, _.allocator, -1)?;
            }
        }

        appendU16Be(*_.buf, _.allocator, @intCast(result_formats.len))?;
        let ri: usize = 0;
        while (ri < result_formats.len) : (ri += 1) {
            appendU16Be(*_.buf, _.allocator, @intFromEnum(result_formats[ri]))?;
        }
        _.finishMsg(start)?;
    }

    /// Encode `Describe` ('D') message for a prepared statement ('S') or portal ('P').
    pub fn writeDescribe(*_, kind: TargetKind, name: []const u8): void! {
        const start = _.beginMsg('D')?;
        _.buf.append(_.allocator, kind.toByte())?;
        appendCString(*_.buf, _.allocator, name)?;
        _.finishMsg(start)?;
    }

    /// Encode `Execute` ('E') message for a portal (`max_rows == 0` means unlimited).
    pub fn writeExecute(*_, portal_name: []const u8, max_rows: u32): void! {
        const start = _.beginMsg('E')?;
        appendCString(*_.buf, _.allocator, portal_name)?;
        appendU32Be(*_.buf, _.allocator, max_rows)?;
        _.finishMsg(start)?;
    }

    /// Encode `Close` ('C') message for a prepared statement ('S') or portal ('P').
    pub fn writeClose(*_, kind: TargetKind, name: []const u8): void! {
        const start = _.beginMsg('C')?;
        _.buf.append(_.allocator, kind.toByte())?;
        appendCString(*_.buf, _.allocator, name)?;
        _.finishMsg(start)?;
    }

    /// Encode `Sync` ('S', length 4) message.
    pub fn writeSync(*_): void! {
        const msg: [5]u8 = ['S', 0, 0, 0, 4];
        _.buf.appendSlice(_.allocator, msg[0..])?;
    }

    /// Encode `Flush` ('H', length 4) message.
    pub fn writeFlush(*_): void! {
        const msg: [5]u8 = ['H', 0, 0, 0, 4];
        _.buf.appendSlice(_.allocator, msg[0..])?;
    }

    /// Encode `Query` ('Q') simple query message.
    pub fn writeSimpleQuery(*_, sql: []const u8): void! {
        const start = _.beginMsg('Q')?;
        appendCString(*_.buf, _.allocator, sql)?;
        _.finishMsg(start)?;
    }

    /// Encode `Terminate` ('X', length 4) message.
    pub fn writeTerminate(*_): void! {
        const msg: [5]u8 = ['X', 0, 0, 0, 4];
        _.buf.appendSlice(_.allocator, msg[0..])?;
    }
}
