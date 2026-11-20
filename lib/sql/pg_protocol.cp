// Source: /data/dev/bun/src/sql/postgres/PostgresProtocol.cp
// lib/sql/pg_protocol.cp
// Complete PostgreSQL v3 Binary Wire Protocol Frames (Frontend Encoders & Backend Decoders)

const std = @import("std");
import { SqlData } from "./shared.cp";
import { AnyPostgresError, PgCommandTag, PgTypeTag } from "./pg_types.cp";
import { PostgresAuthKind, PostgresErrorField, PostgresErrorResponse } from "./postgres.cp";

pub const PG_SSL_REQUEST_CODE: u32 = 80877103;
pub const PG_CANCEL_REQUEST_CODE: u32 = 80877102;
pub const PG_PROTOCOL_V3: u32 = 196608; // 0x00030000

pub struct PgBufferReader {
    pub data: []const u8 = "",
    pub pos: usize = 0,

    pub static fn init(buf: []const u8): PgBufferReader {
        return PgBufferReader{ data: buf, pos: 0 };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) return 0;
        return _.data.len - _.pos;
    }

    pub fn peek(): []const u8 {
        if (_.pos >= _.data.len) return "";
        return _.data[_.pos..];
    }

    pub fn skip(*_, count: usize): ! {
        if (_.pos + count > _.data.len) return error.ShortRead;
        _.pos += count;
    }

    pub fn read(*_, count: usize): ([]const u8)! {
        if (_.pos + count > _.data.len) return error.ShortRead;
        const s = _.data[_.pos .. _.pos + count];
        _.pos += count;
        return s;
    }

    pub fn readZ(*_): ([]const u8)! {
        const nul = std.mem.indexOfScalarPos(u8, _.data, _.pos, 0) ?? return error.ShortRead;
        const s = _.data[_.pos..nul];
        _.pos = nul + 1;
        return s;
    }

    pub fn u8Val(*_): u8! {
        if (_.pos >= _.data.len) return error.ShortRead;
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    pub fn short(*_): i16! {
        const s = _.read(2)?;
        const u: u16 = ((s[0] as u16) << 8) | (s[1] as u16);
        return @bitCast(u);
    }

    pub fn u16Val(*_): u16! {
        const s = _.read(2)?;
        return ((s[0] as u16) << 8) | (s[1] as u16);
    }

    pub fn int4(*_): i32! {
        const s = _.read(4)?;
        const u: u32 = ((s[0] as u32) << 24) | ((s[1] as u32) << 16) | ((s[2] as u32) << 8) | (s[3] as u32);
        return @bitCast(u);
    }

    pub fn u32Val(*_): u32! {
        const s = _.read(4)?;
        return ((s[0] as u32) << 24) | ((s[1] as u32) << 16) | ((s[2] as u32) << 8) | (s[3] as u32);
    }

    pub fn int8(*_): i64! {
        const s = _.read(8)?;
        let u: u64 = 0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            u = (u << 8) | (s[i] as u64);
        }
        return @bitCast(u);
    }
}

pub struct PgBufferWriter {
    pub out: []u8,
    pub pos: usize = 0,

    pub static fn init(out: []u8): PgBufferWriter {
        return PgBufferWriter{ out: out, pos: 0 };
    }

    pub fn written(): []const u8 {
        return _.out[0.._.pos];
    }

    pub fn write(*_, bytes: []const u8): ! {
        if (_.pos + bytes.len > _.out.len) return error.BufferTooSmall;
        if (bytes.len > 0) {
            @memcpy(_.out[_.pos .. _.pos + bytes.len], bytes);
            _.pos += bytes.len;
        }
    }

    pub fn writeByte(*_, b: u8): ! {
        if (_.pos >= _.out.len) return error.BufferTooSmall;
        _.out[_.pos] = b;
        _.pos += 1;
    }

    pub fn short(*_, val: i16): ! {
        if (_.pos + 2 > _.out.len) return error.BufferTooSmall;
        const u: u16 = @bitCast(val);
        _.out[_.pos] = ((u >> 8) & 0xFF) as u8;
        _.out[_.pos + 1] = (u & 0xFF) as u8;
        _.pos += 2;
    }

    pub fn u16Val(*_, val: u16): ! {
        if (_.pos + 2 > _.out.len) return error.BufferTooSmall;
        _.out[_.pos] = ((val >> 8) & 0xFF) as u8;
        _.out[_.pos + 1] = (val & 0xFF) as u8;
        _.pos += 2;
    }

    pub fn int4(*_, val: u32): ! {
        if (_.pos + 4 > _.out.len) return error.BufferTooSmall;
        _.out[_.pos] = ((val >> 24) & 0xFF) as u8;
        _.out[_.pos + 1] = ((val >> 16) & 0xFF) as u8;
        _.out[_.pos + 2] = ((val >> 8) & 0xFF) as u8;
        _.out[_.pos + 3] = (val & 0xFF) as u8;
        _.pos += 4;
    }

    pub fn sint4(*_, val: i32): ! {
        const u: u32 = @bitCast(val);
        _.int4(u)?;
    }

    pub fn int8(*_, val: i64): ! {
        if (_.pos + 8 > _.out.len) return error.BufferTooSmall;
        const u: u64 = @bitCast(val);
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            const shift: u6 = @intCast(56 - (i * 8));
            _.out[_.pos + i] = ((u >> shift) & 0xFF) as u8;
        }
        _.pos += 8;
    }

    pub fn string(*_, value: []const u8): ! {
        _.write(value)?;
        if (value.len == 0 || value[value.len - 1] != 0) {
            _.writeByte(0)?;
        }
    }

    pub fn beginMessage(*_, tag: u8): usize! {
        _.writeByte(tag)?;
        const len_pos = _.pos;
        _.int4(0)?;
        return len_pos;
    }

    pub fn finishMessage(*_, len_pos: usize) {
        const msg_len = (_.pos - len_pos) as u32;
        _.out[len_pos] = ((msg_len >> 24) & 0xFF) as u8;
        _.out[len_pos + 1] = ((msg_len >> 16) & 0xFF) as u8;
        _.out[len_pos + 2] = ((msg_len >> 8) & 0xFF) as u8;
        _.out[len_pos + 3] = (msg_len & 0xFF) as u8;
    }
}

pub struct PgParameterDescription {
    pub param_oids: [64]u32 = undefined,
    pub param_count: usize = 0,

    /// Parses a PostgreSQL `'t'` ParameterDescription payload (2B count + N * 4B OIDs).
    pub static fn parse(payload: []const u8): PgParameterDescription? {
        let r = PgBufferReader.init(payload);
        const raw_cnt = r.u16Val() catch return null;
        const cnt = raw_cnt as usize;
        let desc = PgParameterDescription{};
        let i: usize = 0;
        while (i < 64) : (i += 1) desc.param_oids[i] = 0;

        let idx: usize = 0;
        while (idx < cnt) : (idx += 1) {
            const oid = r.u32Val() catch return null;
            if (desc.param_count < 64) {
                desc.param_oids[desc.param_count] = oid;
                desc.param_count += 1;
            }
        }
        return desc;
    }

    pub fn getTypeTag(idx: usize): PgTypeTag? {
        if (idx >= _.param_count) return null;
        return PgTypeTag.fromOid(_.param_oids[idx]);
    }
}

pub struct PgNotificationResponse {
    pub process_id: u32 = 0,
    pub channel: []const u8 = "",
    pub payload: []const u8 = "",

    /// Parses a PostgreSQL `'A'` NotificationResponse payload (4B pid + channel\0 + payload\0).
    pub static fn parse(data: []const u8): PgNotificationResponse? {
        let r = PgBufferReader.init(data);
        const pid = r.u32Val() catch return null;
        const channel = r.readZ() catch return null;
        const msg_payload = r.readZ() catch return null;
        return PgNotificationResponse{
            process_id: pid,
            channel: channel,
            payload: msg_payload,
        };
    }
}

pub struct PgNoticeResponse {
    pub severity: []const u8 = "",
    pub sql_state: []const u8 = "",
    pub message: []const u8 = "",
    pub detail: []const u8 = "",
    pub hint: []const u8 = "",

    /// Parses a PostgreSQL `'N'` NoticeResponse payload.
    pub static fn parse(payload: []const u8): PgNoticeResponse {
        const er = PostgresErrorResponse.parse(payload);
        return PgNoticeResponse{
            severity: er.severity,
            sql_state: er.sql_state,
            message: er.message,
            detail: er.detail,
            hint: er.hint,
        };
    }
}

pub struct PgNegotiateProtocolVersion {
    pub newest_minor_version: u32 = 0,
    pub unrecognized_count: u32 = 0,
    pub unrecognized_options: [16][]const u8 = undefined,

    /// Parses a PostgreSQL `'v'` NegotiateProtocolVersion payload.
    pub static fn parse(payload: []const u8): PgNegotiateProtocolVersion? {
        let r = PgBufferReader.init(payload);
        const minor = r.u32Val() catch return null;
        const cnt = r.u32Val() catch return null;
        let res = PgNegotiateProtocolVersion{
            newest_minor_version: minor,
            unrecognized_count: cnt,
        };
        let i: usize = 0;
        while (i < 16) : (i += 1) res.unrecognized_options[i] = "";
        let idx: usize = 0;
        while (idx < (cnt as usize) && idx < 16) : (idx += 1) {
            res.unrecognized_options[idx] = r.readZ() catch break;
        }
        return res;
    }
}

pub struct PgCopyResponse {
    pub overall_format: u8 = 0, // 0 = text, 1 = binary
    pub column_count: u16 = 0,
    pub column_formats: [64]u16 = undefined,

    /// Parses a PostgreSQL `'G'` CopyInResponse or `'H'` CopyOutResponse payload.
    pub static fn parse(payload: []const u8): PgCopyResponse? {
        let r = PgBufferReader.init(payload);
        const fmt = r.u8Val() catch return null;
        const cnt = r.u16Val() catch return null;
        let res = PgCopyResponse{
            overall_format: fmt,
            column_count: cnt,
        };
        let i: usize = 0;
        while (i < 64) : (i += 1) res.column_formats[i] = 0;
        let idx: usize = 0;
        while (idx < (cnt as usize)) : (idx += 1) {
            const cfmt = r.u16Val() catch return null;
            if (idx < 64) res.column_formats[idx] = cfmt;
        }
        return res;
    }
}

pub struct PgProtocol {
    pub const SSL_REQUEST_PACKET: []const u8 = "\x00\x00\x00\x08\x04\xd2\x16\x2f";
    pub const SYNC_PACKET: []const u8 = "S\x00\x00\x00\x04";
    pub const FLUSH_PACKET: []const u8 = "H\x00\x00\x00\x04";
    pub const TERMINATE_PACKET: []const u8 = "X\x00\x00\x00\x04";
    pub const COPY_DONE_PACKET: []const u8 = "c\x00\x00\x00\x04";

    pub static fn encodeSSLRequest(out: []u8): usize {
        if (out.len < 8) return 0;
        @memcpy(out[0..8], SSL_REQUEST_PACKET);
        return 8;
    }

    pub static fn encodeCancelRequest(out: []u8, process_id: u32, secret_key: u32): usize {
        let w = PgBufferWriter.init(out);
        w.int4(16) catch return 0;
        w.int4(PG_CANCEL_REQUEST_CODE) catch return 0;
        w.int4(process_id) catch return 0;
        w.int4(secret_key) catch return 0;
        return w.pos;
    }

    pub static fn encodeStartupWithClientEncoding(
        out: []u8,
        user: []const u8,
        database: []const u8,
        application_name: []const u8 = "",
    ): usize {
        let w = PgBufferWriter.init(out);
        w.int4(0) catch return 0;
        w.int4(PG_PROTOCOL_V3) catch return 0;
        w.string("user") catch return 0;
        w.string(user) catch return 0;
        if (database.len > 0) {
            w.string("database") catch return 0;
            w.string(database) catch return 0;
        }
        w.string("client_encoding") catch return 0;
        w.string("UTF8") catch return 0;
        if (application_name.len > 0) {
            w.string("application_name") catch return 0;
            w.string(application_name) catch return 0;
        }
        w.writeByte(0) catch return 0;
        w.finishMessage(0);
        return w.pos;
    }

    pub static fn encodeCopyData(out: []u8, data: []const u8): usize {
        let w = PgBufferWriter.init(out);
        const lp = w.beginMessage('d') catch return 0;
        w.write(data) catch return 0;
        w.finishMessage(lp);
        return w.pos;
    }

    pub static fn encodeCopyFail(out: []u8, message: []const u8): usize {
        let w = PgBufferWriter.init(out);
        const lp = w.beginMessage('f') catch return 0;
        w.string(message) catch return 0;
        w.finishMessage(lp);
        return w.pos;
    }

    pub static fn encodeCopyDone(out: []u8): usize {
        if (out.len < 5) return 0;
        @memcpy(out[0..5], COPY_DONE_PACKET);
        return 5;
    }
}
