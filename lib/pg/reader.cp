// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NewReader.cp | Synced: 2026-11-21T16:59:35Z
//! Zero-copy PostgreSQL v3 big-endian wire message frame reader.

export const FRAME_HEADER_SIZE: usize = 5; // 1-byte tag + 4-byte inclusive length

export err PgReaderError {
    ShortRead,
    InvalidMessageLength,
    MissingNullTerminator,
    UnexpectedTag,
}

export enum PgBackendTag: u8 {
    Authentication = 'R',
    BackendKeyData = 'K',
    BindComplete = '2',
    CloseComplete = '3',
    CommandComplete = 'C',
    CopyData = 'd',
    CopyDone = 'c',
    DataRow = 'D',
    EmptyQueryResponse = 'I',
    ErrorResponse = 'E',
    NoData = 'n',
    NoticeResponse = 'N',
    NotificationResponse = 'A',
    ParameterDescription = 't',
    ParameterStatus = 'S',
    ParseComplete = '1',
    PortalSuspended = 's',
    ReadyForQuery = 'Z',
    RowDescription = 'T',
    _,

    pub fn toU8(): u8 {
        return @intFromEnum(_);
    }

    pub fn isError(): bool {
        return _ == .ErrorResponse;
    }

    pub fn isReady(): bool {
        return _ == .ReadyForQuery;
    }
}

export struct PgFrame {
    tag: PgBackendTag,
    payload: []const u8,
}

/// Zero-copy PostgreSQL v3 big-endian message reader.
export struct PgMessageReader {
    data: []const u8,
    pos: usize = 0,
    message_start: usize = 0,

    pub static fn init(data: []const u8): PgMessageReader {
        return PgMessageReader{ data: data, pos: 0, message_start: 0 };
    }

    pub fn markMessageStart(*_) {
        _.message_start = _.pos;
    }

    pub fn rewindToMessageStart(*_) {
        _.pos = _.message_start;
    }

    pub fn remaining(): usize {
        return _.data.len - _.pos;
    }

    pub fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    pub fn peek(): []const u8 {
        return _.data[_.pos..];
    }

    pub fn ensureCapacity(count: usize): void! {
        if (count > _.remaining()) return err.ShortRead;
    }

    pub fn skip(*_, count: usize): void! {
        _.ensureCapacity(count)?;
        _.pos += count;
    }

    pub fn readBytes(*_, count: usize): ([]const u8)! {
        _.ensureCapacity(count)?;
        const s = _.data[_.pos .. _.pos + count];
        _.pos += count;
        return s;
    }

    pub fn readU8(*_): u8! {
        const s = _.readBytes(1)?;
        return s[0];
    }

    pub fn readU16Be(*_): u16! {
        const s = _.readBytes(2)?;
        return (@as(u16, s[0]) << 8) | @as(u16, s[1]);
    }

    pub fn readI16Be(*_): i16! {
        const u = _.readU16Be()?;
        return @bitCast(u);
    }

    pub fn readU32Be(*_): u32! {
        const s = _.readBytes(4)?;
        return (@as(u32, s[0]) << 24)
            | (@as(u32, s[1]) << 16)
            | (@as(u32, s[2]) << 8)
            | @as(u32, s[3]);
    }

    pub fn readI32Be(*_): i32! {
        const u = _.readU32Be()?;
        return @bitCast(u);
    }

    pub fn readI64Be(*_): i64! {
        const s = _.readBytes(8)?;
        const u: u64 = (@as(u64, s[0]) << 56)
            | (@as(u64, s[1]) << 48)
            | (@as(u64, s[2]) << 40)
            | (@as(u64, s[3]) << 32)
            | (@as(u64, s[4]) << 24)
            | (@as(u64, s[5]) << 16)
            | (@as(u64, s[6]) << 8)
            | @as(u64, s[7]);
        return @bitCast(u);
    }

    pub fn peekI32Be(): i32? {
        if (_.remaining() < 4) return null;
        const s = _.data[_.pos .. _.pos + 4];
        const u: u32 = (@as(u32, s[0]) << 24)
            | (@as(u32, s[1]) << 16)
            | (@as(u32, s[2]) << 8)
            | @as(u32, s[3]);
        return @bitCast(u);
    }

    /// Read a NUL-terminated C string (excludes the trailing 0 byte).
    pub fn readCString(*_): ([]const u8)! {
        const start = _.pos;
        while (_.pos < _.data.len) : (_.pos += 1) {
            if (_.data[_.pos] == 0) {
                const s = _.data[start.._.pos];
                _.pos += 1;
                return s;
            }
        }
        return err.MissingNullTerminator;
    }

    /// Read a PG length-prefixed nullable field (`-1` => `null`, `>= 0` => slice).
    pub fn readNullableField(*_): (([]const u8)?)! {
        const len = _.readI32Be()?;
        if (len == -1) return null;
        if (len < -1) return err.InvalidMessageLength;
        const n: usize = @intCast(len);
        const bytes = _.readBytes(n)?;
        return bytes;
    }

    /// Decode the next complete PG v3 backend frame (`tag: u8` + `len: i32` inclusive of len itself).
    pub fn nextFrame(*_): PgFrame! {
        _.markMessageStart();
        const tag_byte = _.readU8()?;
        const total_len = _.readI32Be()?;
        if (total_len < 4) return err.InvalidMessageLength;
        const payload_len: usize = @intCast(total_len - 4);
        const payload = _.readBytes(payload_len)?;
        return PgFrame{
            tag: @enumFromInt(tag_byte),
            payload: payload,
        };
    }
}
