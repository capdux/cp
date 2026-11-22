// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/StackReader.cp | Synced: 2026-11-22T16:49:19Z
// Pure-CP zero-allocation MySQL wire protocol bounded stack cursor (`StackReader`):
// supports message-start bookmarking, saturating signed skip, NUL-terminated strings,
// little-endian integers, length-encoded ints/strings, and atomic half-packet rollback.

pub err StackReaderError {
    ShortRead,
    InvalidOffset,
    InvalidLenEncInt,
    NullLenEncValue,
}

pub struct MysqlFrameSlice {
    payload_len: u32 = 0,
    sequence_id: u8 = 0,
    payload: []const u8 = "",
}

/// Zero-allocation bounded reader cursor over a MySQL wire buffer.
pub struct StackReader {
    buffer: []const u8 = "",
    offset: usize = 0,
    message_start: usize = 0,

    static fn init(buffer: []const u8): StackReader {
        return StackReader {
            buffer: buffer,
            offset: 0,
            message_start: 0,
        };
    }

    static fn initAt(buffer: []const u8, start_offset: usize): StackReader {
        const clamped = if (start_offset > buffer.len) buffer.len else start_offset;
        return StackReader {
            buffer: buffer,
            offset: clamped,
            message_start: clamped,
        };
    }

    fn markMessageStart(*_) {
        _.message_start = _.offset;
    }

    fn messageStart(): usize {
        return _.message_start;
    }

    fn rewindMessage(*_) {
        _.offset = _.message_start;
    }

    fn setOffsetFromStart(*_, rel_offset: usize): ! {
        if (rel_offset > _.buffer.len - _.message_start) {
            return StackReaderError.InvalidOffset;
        }
        _.offset = _.message_start + rel_offset;
    }

    fn position(): usize {
        return _.offset;
    }

    fn remaining(): usize {
        return _.buffer.len - _.offset;
    }

    fn isEof(): bool {
        return _.offset >= _.buffer.len;
    }

    fn ensureCapacity(length: usize): bool {
        return length <= _.buffer.len - _.offset;
    }

    fn peek(): []const u8 {
        return _.buffer[_.offset..];
    }

    fn peekByte(): u8? {
        if (_.offset >= _.buffer.len) return null;
        return _.buffer[_.offset];
    }

    /// Saturating signed skip (negative rewinds towards 0, positive advances towards buffer.len).
    fn skip(*_, count: i64) {
        if (count < 0) {
            const abs_u64: u64 = @as(u64, -(count + 1)) + 1;
            const cur_u64: u64 = @as(u64, _.offset);
            if (abs_u64 >= cur_u64) {
                _.offset = 0;
                return;
            }
            _.offset -= @as(usize, abs_u64);
            return;
        }
        const fwd: u64 = @as(u64, count);
        const rem: u64 = @as(u64, _.buffer.len - _.offset);
        if (fwd >= rem) {
            _.offset = _.buffer.len;
            return;
        }
        _.offset += @as(usize, fwd);
    }

    /// Reads an exact zero-copy slice of `count` bytes or returns `ShortRead`.
    fn read(*_, count: usize): ([]const u8)! {
        if (!_.ensureCapacity(count)) return StackReaderError.ShortRead;
        const start = _.offset;
        _.offset += count;
        return _.buffer[start.._.offset];
    }

    /// Reads a NUL-terminated C string (consuming the trailing `0x00`), returning bytes before NUL.
    fn readZ(*_): ([]const u8)! {
        const rem = _.peek();
        let i: usize = 0;
        while (i < rem.len) {
            if (rem[i] == 0) {
                _.offset += i + 1;
                return rem[0..i];
            }
            i += 1;
        }
        return StackReaderError.ShortRead;
    }

    fn readU8(*_): u8! {
        const s = _.read(1)?;
        return s[0];
    }

    fn readU16Le(*_): u16! {
        const s = _.read(2)?;
        return @as(u16, s[0]) | (@as(u16, s[1]) << 8);
    }

    fn readU24Le(*_): u32! {
        const s = _.read(3)?;
        return @as(u32, s[0]) | (@as(u32, s[1]) << 8) | (@as(u32, s[2]) << 16);
    }

    fn readU32Le(*_): u32! {
        const s = _.read(4)?;
        return @as(u32, s[0]) |
            (@as(u32, s[1]) << 8) |
            (@as(u32, s[2]) << 16) |
            (@as(u32, s[3]) << 24);
    }

    fn readU64Le(*_): u64! {
        const s = _.read(8)?;
        return @as(u64, s[0]) |
            (@as(u64, s[1]) << 8) |
            (@as(u64, s[2]) << 16) |
            (@as(u64, s[3]) << 24) |
            (@as(u64, s[4]) << 32) |
            (@as(u64, s[5]) << 40) |
            (@as(u64, s[6]) << 48) |
            (@as(u64, s[7]) << 56);
    }

    /// Decodes a MySQL Length-Encoded Integer (`0x00..0xFA`, `0xFC` u16, `0xFD` u24, `0xFE` u64).
    /// Returns `NullLenEncValue` on `0xFB` and `InvalidLenEncInt` on `0xFF`.
    fn readLenEncInt(*_): u64! {
        const first = _.readU8()?;
        if (first < 0xFB) return @as(u64, first);
        if (first == 0xFB) return StackReaderError.NullLenEncValue;
        if (first == 0xFC) return @as(u64, _.readU16Le()?);
        if (first == 0xFD) return @as(u64, _.readU24Le()?);
        if (first == 0xFE) return _.readU64Le();
        return StackReaderError.InvalidLenEncInt;
    }

    /// Decodes a MySQL Length-Encoded String as a zero-copy slice.
    fn readLenEncString(*_): ([]const u8)! {
        const len_u64 = _.readLenEncInt()?;
        const rem_u64: u64 = @as(u64, _.remaining());
        if (len_u64 > rem_u64) return StackReaderError.ShortRead;
        return _.read(@as(usize, len_u64));
    }

    /// Atomically reads a 4-byte MySQL packet header + payload slice.
    /// Rewinds to `message_start` if the buffer holds an incomplete packet.
    fn readPacketFrame(*_): MysqlFrameSlice! {
        _.markMessageStart();
        if (!_.ensureCapacity(4)) {
            _.rewindMessage();
            return StackReaderError.ShortRead;
        }
        const plen = _.readU24Le()?;
        const seq = _.readU8()?;
        const plen_usize: usize = @as(usize, plen);
        if (!_.ensureCapacity(plen_usize)) {
            _.rewindMessage();
            return StackReaderError.ShortRead;
        }
        const body = _.read(plen_usize)?;
        return MysqlFrameSlice {
            payload_len: plen,
            sequence_id: seq,
            payload: body,
        };
    }
}
