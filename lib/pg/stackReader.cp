// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/StackReader.cp | Synced: 2026-11-22T17:41:45Z
// ============================================================================
// lib/pg/stackReader.cp — Zero-Alloc Bounded PostgreSQL Wire Protocol Cursor
// ============================================================================
// Implements Bun's `StackReader` zero-copy cursor for PostgreSQL Wire Protocol v3:
// message-start bookmarks, saturating skip, NUL-terminated C-string reads,
// big-endian scalar reads, and atomic backend frame slicing with short-read rollback.

pub err PgStackReaderError {
    ShortRead,
    InvalidMessageLength,
}

/// Zero-copy slice view of one complete PostgreSQL backend wire frame
/// (`[1B tag][4B BE length (includes self)][payload]`).
pub struct PgWireFrameSlice {
    tag: u8 = 0,
    payload_len: u32 = 0,
    payload: []const u8 = "",
}

/// Bounded zero-allocation cursor over a PostgreSQL wire buffer.
pub struct PgStackReader {
    buffer: []const u8 = "",
    offset: usize = 0,
    message_start: usize = 0,

    static fn init(buffer: []const u8): PgStackReader {
        return PgStackReader{
            buffer: buffer,
            offset: 0,
            message_start: 0,
        };
    }

    fn markMessageStart(*_) {
        _.message_start = _.offset;
    }

    fn rewindMessage(*_) {
        _.offset = _.message_start;
    }

    fn remaining(): usize {
        if (_.offset >= _.buffer.len) return 0;
        return _.buffer.len - _.offset;
    }

    fn isEof(): bool {
        return _.offset >= _.buffer.len;
    }

    fn peek(): []const u8 {
        if (_.offset >= _.buffer.len) return "";
        return _.buffer[_.offset..];
    }

    fn ensureLength(length: usize): bool {
        return _.remaining() >= length;
    }

    fn ensureCapacity(count: usize): bool {
        return _.remaining() >= count;
    }

    /// Advances the cursor by `count` bytes, saturating at `buffer.len`.
    fn skip(*_, count: usize) {
        if (count >= _.remaining()) {
            _.offset = _.buffer.len;
            return;
        }
        _.offset += count;
    }

    /// Reads exactly `count` bytes as a zero-copy slice, or returns `ShortRead`.
    fn read(*_, count: usize): PgStackReaderError![]const u8 {
        if (!_.ensureCapacity(count)) return PgStackReaderError.ShortRead;
        const start = _.offset;
        _.offset += count;
        return _.buffer[start.._.offset];
    }

    /// Reads a NUL-terminated string (excluding the trailing `\0`) and advances past `\0`.
    fn readZ(*_): PgStackReaderError![]const u8 {
        const rem = _.peek();
        let i: usize = 0;
        while (i < rem.len) {
            if (rem[i] == 0) {
                _.skip(i + 1);
                return rem[0..i];
            }
            i += 1;
        }
        return PgStackReaderError.ShortRead;
    }

    fn readU8(*_): PgStackReaderError!u8 {
        const b = _.read(1)?;
        return b[0];
    }

    fn readU16Be(*_): PgStackReaderError!u16 {
        const b = _.read(2)?;
        const b0: u16 = @as(u16, b[0]);
        const b1: u16 = @as(u16, b[1]);
        return (b0 << 8) | b1;
    }

    fn readI16Be(*_): PgStackReaderError!i16 {
        const u = _.readU16Be()?;
        return @as(i16, @bitCast(u));
    }

    fn readU32Be(*_): PgStackReaderError!u32 {
        const b = _.read(4)?;
        const b0: u32 = @as(u32, b[0]);
        const b1: u32 = @as(u32, b[1]);
        const b2: u32 = @as(u32, b[2]);
        const b3: u32 = @as(u32, b[3]);
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    fn readI32Be(*_): PgStackReaderError!i32 {
        const u = _.readU32Be()?;
        return @as(i32, @bitCast(u));
    }

    /// Reads a PostgreSQL 4-byte big-endian message length (which includes the 4 length bytes)
    /// and returns the remaining payload length (`declared_len - 4`).
    fn readMessageLength(*_): PgStackReaderError!u32 {
        const declared = _.readU32Be()?;
        if (declared < 4) return PgStackReaderError.InvalidMessageLength;
        return declared - 4;
    }

    /// Atomically reads one complete PostgreSQL backend wire frame (`[1B tag][4B BE len][payload]`).
    /// On partial/short read, rolls the cursor back to `message_start` so streaming callers can retry.
    fn readBackendFrame(*_): PgStackReaderError!PgWireFrameSlice {
        _.markMessageStart();
        if (!_.ensureLength(5)) {
            _.rewindMessage();
            return PgStackReaderError.ShortRead;
        }
        const tag = _.readU8()?;
        const declared = _.readU32Be()?;
        if (declared < 4) {
            _.rewindMessage();
            return PgStackReaderError.InvalidMessageLength;
        }
        const payload_len: u32 = declared - 4;
        const need: usize = @as(usize, payload_len);
        if (!_.ensureLength(need)) {
            _.rewindMessage();
            return PgStackReaderError.ShortRead;
        }
        const payload = _.read(need)?;
        return PgWireFrameSlice{
            tag: tag,
            payload_len: payload_len,
            payload: payload,
        };
    }
}
