// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/NewReader.cp | Synced: 2026-11-22T03:37:30Z
// Pure-CP zero-allocation MySQL wire-protocol binary reader:
// - Streaming message bookmarking (`markMessageStart` / `setOffsetFromStart` / `rewindToMessageStart`)
// - Zero-copy slice reads (`readBytes`, `readZ`, `peek`), safe skip/capacity guards
// - Little-endian integer & IEEE-754 float decoders (u8/u16/u24/u32/u64/i32/i64/f32/f64)
// - Length-encoded integer & string readers (`readLenEncInt`, `readLenEncIntWithSize`,
//   `readLenEncString`, `readLenEncStringOrNull`) with truncation & overflow guards.

import {
    MYSQL_LENENC_NULL_MARKER,
    DecodeLengthIntResult,
    decodeLengthInt,
} from "./mysqlLenenc.cp";

/// MySQL 线协议二进制读取器错误集
pub err MySqlWireReaderError {
    ShortRead,
    InvalidEncodedInteger,
    InvalidEncodedLength,
    UnterminatedString,
    InvalidOffset,
}

/// 零堆分配 MySQL 线协议二进制读取器（对齐 Bun `NewReader` + `StackReader`）
pub struct MySqlWireReader {
    buf: []const u8,
    pos: usize = 0,
    message_start: usize = 0,

    /// 在调用方提供的只读线协议缓冲区上初始化读取器
    static fn init(buf: []const u8): MySqlWireReader {
        return MySqlWireReader{
            buf: buf,
            pos: 0,
            message_start: 0,
        };
    }

    /// 标记当前偏移为新消息起始位置（对齐 Bun `markMessageStart()`）
    fn markMessageStart(*_) {
        _.message_start = _.pos;
    }

    /// 返回当前绝对读取偏移
    fn offset(): usize {
        return _.pos;
    }

    /// 返回当前相对消息起点的偏移
    fn offsetFromStart(): usize {
        if (_.pos <= _.message_start) {
            return 0;
        }
        return _.pos - _.message_start;
    }

    /// 将读取偏移设置为相对 `message_start` 的指定位置（对齐 Bun `setOffsetFromStart()`）
    fn setOffsetFromStart(*_, rel_offset: usize): usize! {
        if (rel_offset > _.buf.len - _.message_start) {
            return err.InvalidOffset;
        }
        _.pos = _.message_start + rel_offset;
        return _.pos;
    }

    /// 回退读取位置到当前消息起点（用于半包等待更多字节后重解析）
    fn rewindToMessageStart(*_) {
        _.pos = _.message_start;
    }

    /// 返回剩余未读字节数
    fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    /// 返回未读部分的零拷贝只读视图（对齐 Bun `peek()`）
    fn peek(): []const u8 {
        return _.buf[_.pos..];
    }

    /// 检查缓冲区是否至少还有 `count` 个可读字节（对齐 Bun `ensureCapacity()`）
    fn ensureCapacity(count: usize): usize! {
        if (count > _.remaining()) {
            return err.ShortRead;
        }
        return count;
    }

    /// 跳过最多 `count` 字节（超出末尾时钳位至缓冲末尾，返回实际跳过字节数）
    fn skip(*_, count: usize): usize {
        const rem = _.remaining();
        const adv = if (count > rem) rem else count;
        _.pos += adv;
        return adv;
    }

    /// 读取 `count` 字节的零拷贝切片（对齐 Bun `read()`）
    fn readBytes(*_, count: usize): ([]const u8)! {
        if (count > _.remaining()) {
            return err.ShortRead;
        }
        const start = _.pos;
        _.pos += count;
        return _.buf[start.._.pos];
    }

    /// 读取单个字节（对齐 Bun `byte()`）
    fn readByte(*_): u8! {
        if (_.pos >= _.buf.len) {
            return err.ShortRead;
        }
        const b = _.buf[_.pos];
        _.pos += 1;
        return b;
    }

    /// 读取以 `\0` 结尾的零拷贝 C 字符串（对齐 Bun `readZ()`，消费末尾 `\0` 但不包含在返回切片中）
    fn readZ(*_): ([]const u8)! {
        const start = _.pos;
        let i = start;
        while (i < _.buf.len) {
            if (_.buf[i] == 0) {
                _.pos = i + 1;
                return _.buf[start..i];
            }
            i += 1;
        }
        return err.UnterminatedString;
    }

    /// 读取 2 字节小端无符号整数
    fn readU16(*_): u16! {
        const s = _.readBytes(2)?;
        return @as(u16, s[0]) | (@as(u16, s[1]) << 8);
    }

    /// 读取 3 字节小端无符号整数（MySQL 包头 24-bit payload length）
    fn readU24(*_): u32! {
        const s = _.readBytes(3)?;
        return @as(u32, s[0]) |
            (@as(u32, s[1]) << 8) |
            (@as(u32, s[2]) << 16);
    }

    /// 读取 4 字节小端无符号整数
    fn readU32(*_): u32! {
        const s = _.readBytes(4)?;
        return @as(u32, s[0]) |
            (@as(u32, s[1]) << 8) |
            (@as(u32, s[2]) << 16) |
            (@as(u32, s[3]) << 24);
    }

    /// 读取 8 字节小端无符号整数
    fn readU64(*_): u64! {
        const s = _.readBytes(8)?;
        return @as(u64, s[0]) |
            (@as(u64, s[1]) << 8) |
            (@as(u64, s[2]) << 16) |
            (@as(u64, s[3]) << 24) |
            (@as(u64, s[4]) << 32) |
            (@as(u64, s[5]) << 40) |
            (@as(u64, s[6]) << 48) |
            (@as(u64, s[7]) << 56);
    }

    /// 读取 4 字节小端有符号整数
    fn readI32(*_): i32! {
        const bits = _.readU32()?;
        return @bitCast(bits);
    }

    /// 读取 8 字节小端有符号整数
    fn readI64(*_): i64! {
        const bits = _.readU64()?;
        return @bitCast(bits);
    }

    /// 读取 4 字节 IEEE-754 小端单精度浮点数
    fn readF32(*_): f32! {
        const bits = _.readU32()?;
        return @bitCast(bits);
    }

    /// 读取 8 字节 IEEE-754 小端双精度浮点数
    fn readF64(*_): f64! {
        const bits = _.readU64()?;
        return @bitCast(bits);
    }

    /// 读取 MySQL 变长整数 `lenenc-int`（对齐 Bun `encodedLenInt()`）
    fn readLenEncInt(*_): u64! {
        if (res := decodeLengthInt(_.peek())) {
            _.pos += res.bytes_read;
            return res.value;
        }
        return err.InvalidEncodedInteger;
    }

    /// 读取 MySQL 变长整数并同时返回编码值与消耗字节数（对齐 Bun `encodedLenIntWithSize()`）
    fn readLenEncIntWithSize(*_): DecodeLengthIntResult! {
        if (res := decodeLengthInt(_.peek())) {
            _.pos += res.bytes_read;
            return res;
        }
        return err.InvalidEncodedInteger;
    }

    /// 读取 MySQL 长度前缀字符串 `lenenc-str`（对齐 Bun `encodeLenString()`，零拷贝）
    fn readLenEncString(*_): ([]const u8)! {
        const rem_slice = _.peek();
        if (res := decodeLengthInt(rem_slice)) {
            if (res.value > @as(u64, _.buf.len - (_.pos + res.bytes_read))) {
                return err.ShortRead;
            }
            const str_len: usize = @intCast(res.value);
            _.pos += res.bytes_read;
            const start = _.pos;
            _.pos += str_len;
            return _.buf[start.._.pos];
        }
        return err.InvalidEncodedLength;
    }

    /// 读取可空 MySQL 长度前缀字符串：遇到 `0xfb` NULL 标记消费 1 字节并返回 `null`
    fn readLenEncStringOrNull(*_): (([]const u8)?)! {
        if (_.pos >= _.buf.len) {
            return err.ShortRead;
        }
        if (_.buf[_.pos] == MYSQL_LENENC_NULL_MARKER) {
            _.pos += 1;
            return null;
        }
        const s = _.readLenEncString()?;
        return s;
    }
}
