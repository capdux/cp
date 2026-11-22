// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/StackReader.cp | Synced: 2026-11-22T21:14:44Z
// Pure-CP zero-allocation bounded PostgreSQL v3.0 wire cursor (`PgStackReader`).
// Provides message-start bookmarks, saturating skip, big-endian integer readers,
// NUL-terminated C-string scanning, and atomic short-read rollback frame extraction.

/// PostgreSQL v3.0 标准报文帧头长度（1B tag + 4B big-endian length）
pub const PG_FRAME_HEADER_LEN: usize = 5;

/// 默认单帧最大字节数上限（64 MiB，防异常大包资源耗尽）
pub const PG_DEFAULT_MAX_FRAME_BYTES: usize = 64 * 1024 * 1024;

/// `PgStackReader` 读取与帧解析错误集
pub err PgStackReaderError {
    ShortRead,
    InvalidLength,
    FrameTooLarge,
    OffsetOutOfBounds,
}

/// 零拷贝 PostgreSQL 线协议帧切片视图
pub struct PgWireFrameSlice {
    tag: u8 = 0,
    msg_len: u32 = 4,
    payload: []const u8 = "",

    /// 返回不含 5 字节帧头的载荷长度
    fn payloadLen(): usize {
        return _.payload.len;
    }

    /// 返回含 5 字节帧头的总帧长
    fn frameLen(): usize {
        return PG_FRAME_HEADER_LEN + _.payload.len;
    }
}

/// 零堆分配、零拷贝 PostgreSQL 协议有界流游标
pub struct PgStackReader {
    buffer: []const u8 = "",
    offset: usize = 0,
    message_start: usize = 0,

    /// 在给定字节切片上初始化游标
    static fn init(buffer: []const u8): PgStackReader {
        return PgStackReader{
            buffer: buffer,
            offset: 0,
            message_start: 0,
        };
    }

    /// 标记当前游标位置为当前报文起点（供半包回滚使用）
    fn markMessageStart(*_) {
        _.message_start = _.offset;
    }

    /// 回滚游标至最近一次 `markMessageStart` 位置
    fn rewindMessage(*_) {
        _.offset = _.message_start;
    }

    /// 重置游标与书签至缓冲区起始处
    fn reset(*_) {
        _.offset = 0;
        _.message_start = 0;
    }

    /// 返回剩余可读字节数
    fn remaining(): usize {
        if (_.offset >= _.buffer.len) {
            return 0;
        }
        return _.buffer.len - _.offset;
    }

    /// 判断是否已读至缓冲区末尾
    fn isAtEnd(): bool {
        return _.offset >= _.buffer.len;
    }

    /// 零拷贝查看当前剩余未读切片（不推进游标）
    fn peek(): []const u8 {
        if (_.offset >= _.buffer.len) {
            return _.buffer[_.buffer.len.._.buffer.len];
        }
        return _.buffer[_.offset..];
    }

    /// 检查剩余可读字节数是否至少为 `count`
    fn ensureCapacity(count: usize): bool {
        return _.remaining() >= count;
    }

    /// 饱和跳过 `count` 字节（超出缓冲区末尾时自动钳位至 `buffer.len`）
    fn skip(*_, count: usize) {
        if (count >= _.remaining()) {
            _.offset = _.buffer.len;
            return;
        }
        _.offset += count;
    }

    /// 将游标定位至相对 `message_start` 偏移 `rel` 处（越界报错）
    fn setOffsetFromStart(*_, rel: usize): PgStackReaderError!usize {
        if (rel > _.buffer.len - _.message_start) {
            return PgStackReaderError.OffsetOutOfBounds;
        }
        _.offset = _.message_start + rel;
        return _.offset;
    }

    /// 零拷贝读取 `count` 字节并推进游标；不足则返回 `ShortRead` 且游标不动
    fn readBytes(*_, count: usize): PgStackReaderError![]const u8 {
        if (!_.ensureCapacity(count)) {
            return PgStackReaderError.ShortRead;
        }
        const start = _.offset;
        _.offset += count;
        return _.buffer[start.._.offset];
    }

    /// 读取 1 字节无符号整数
    fn readU8(*_): PgStackReaderError!u8 {
        const s = _.readBytes(1)?;
        return s[0];
    }

    /// 读取 2 字节大端无符号整数
    fn readU16Be(*_): PgStackReaderError!u16 {
        const s = _.readBytes(2)?;
        const b0: u16 = @as(u16, s[0]);
        const b1: u16 = @as(u16, s[1]);
        return (b0 << 8) | b1;
    }

    /// 读取 2 字节大端有符号整数
    fn readI16Be(*_): PgStackReaderError!i16 {
        const u = _.readU16Be()?;
        return @bitCast(u);
    }

    /// 读取 4 字节大端无符号整数
    fn readU32Be(*_): PgStackReaderError!u32 {
        const s = _.readBytes(4)?;
        const b0: u32 = @as(u32, s[0]);
        const b1: u32 = @as(u32, s[1]);
        const b2: u32 = @as(u32, s[2]);
        const b3: u32 = @as(u32, s[3]);
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    /// 读取 4 字节大端有符号整数
    fn readI32Be(*_): PgStackReaderError!i32 {
        const u = _.readU32Be()?;
        return @bitCast(u);
    }

    /// 读取 8 字节大端无符号整数
    fn readU64Be(*_): PgStackReaderError!u64 {
        const s = _.readBytes(8)?;
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            acc = (acc << 8) | @as(u64, s[i]);
            i += 1;
        }
        return acc;
    }

    /// 读取 8 字节大端有符号整数
    fn readI64Be(*_): PgStackReaderError!i64 {
        const u = _.readU64Be()?;
        return @bitCast(u);
    }

    /// 零拷贝读取 NUL 终止字符串（跳过尾部 `0x00`，未找到 `0x00` 则返回 `ShortRead` 且游标不动）
    fn readZ(*_): PgStackReaderError![]const u8 {
        const rem = _.peek();
        let i: usize = 0;
        while (i < rem.len) {
            if (rem[i] == 0) {
                const start = _.offset;
                _.offset += i + 1;
                return _.buffer[start..(start + i)];
            }
            i += 1;
        }
        return PgStackReaderError.ShortRead;
    }

    /// 原子读取下一条完整 PostgreSQL v3.0 报文帧（`1B tag + 4B BE length + payload`）
    ///
    /// 若发生半包（`ShortRead`）或长度非法/超限，自动回滚游标至 `message_start`，
    /// 保证网络流收齐后续字节后可无损续解。
    fn readFrame(*_, max_frame_bytes: usize): PgStackReaderError!PgWireFrameSlice {
        _.markMessageStart();
        if (!_.ensureCapacity(PG_FRAME_HEADER_LEN)) {
            _.rewindMessage();
            return PgStackReaderError.ShortRead;
        }
        const tag = _.readU8()?;
        const msg_len = _.readU32Be()?;
        if (msg_len < 4) {
            _.rewindMessage();
            return PgStackReaderError.InvalidLength;
        }
        const total_frame_len: usize = @as(usize, msg_len) + 1;
        if (total_frame_len > max_frame_bytes) {
            _.rewindMessage();
            return PgStackReaderError.FrameTooLarge;
        }
        const payload_len: usize = @as(usize, msg_len - 4);
        if (!_.ensureCapacity(payload_len)) {
            _.rewindMessage();
            return PgStackReaderError.ShortRead;
        }
        const payload = _.readBytes(payload_len)?;
        return PgWireFrameSlice{
            tag: tag,
            msg_len: msg_len,
            payload: payload,
        };
    }
}
