// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NewReader.cp | Synced: 2026-11-24T04:40:50Z
// Reference-hardened against sfackler/rust-postgres (MIT/Apache-2.0)
// Pure-CP zero-allocation PostgreSQL wire-protocol binary reader:
// - Streaming message bookmarking (`markMessageStart` / `offsetFromStart` / `rewindToMessageStart`)
// - Big-endian integer & IEEE-754 float decoders (u8/u16/i16/u32/i32/i64/f32/f64) and non-consuming peeks
// - PostgreSQL message frame reader (`readMessageLength` / `readTaggedFrame`) with `PG_MAX_FRAME_LEN` bound & atomic ShortRead preservation
// - Length-prefixed nullable column/parameter reader (`readLengthPrefixedOrNull`, `-1` -> `null`, `< -1` rejected)
// - Exact message byte verifier (`eatBytes`) and SIMD-accelerated zero-copy C-string reader (`readZ` via `cap:simd`).

import { Simd } from "cap:simd";

/// Default maximum PostgreSQL v3 message length (64 MiB, inclusive of 4-byte length field).
pub const PG_MAX_FRAME_LEN: u32 = 64 * 1024 * 1024;

/// SIMD-accelerated NUL (`\0`) byte scanner using `cap:simd` `Simd.indexOfByte(0)`.
pub fn simdFindNul(s: []const u8): usize? {
    return Simd.indexOfByte(s, 0);
}

/// PostgreSQL 线协议读取器错误集
pub err PgWireReaderError {
    ShortRead,
    InvalidMessage,
    InvalidMessageLength,
    FrameTooLarge,
    UnterminatedString,
}

/// 零拷贝 PostgreSQL 协议帧视图（1 字节 Tag + 去除 4 字节长度头后的净载荷）
pub struct PgFrameView {
    tag: u8 = 0,
    payload: []const u8 = "",
}

/// 零堆分配 PostgreSQL 线协议二进制读取器（对齐 Bun `NewReader` + `StackReader`）
pub struct PgWireReader {
    buf: []const u8,
    pos: usize = 0,
    message_start: usize = 0,

    /// 在调用方提供的只读缓冲区上初始化读取器
    static fn init(buf: []const u8): PgWireReader {
        return PgWireReader{
            buf: buf,
            pos: 0,
            message_start: 0,
        };
    }

    /// 标记当前偏移为消息起点（对齐 Bun `markMessageStart()`）
    fn markMessageStart(*_) {
        _.message_start = _.pos;
    }

    /// 返回当前绝对读取偏移
    fn offset(): usize {
        return _.pos;
    }

    /// 返回相对 `message_start` 的偏移
    fn offsetFromStart(): usize {
        if (_.pos <= _.message_start) {
            return 0;
        }
        return _.pos - _.message_start;
    }

    /// 回退读取指针至 `message_start`（用于流式半包重解析）
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

    /// 读取单个字节
    fn readByte(*_): u8! {
        if (_.pos >= _.buf.len) {
            return err.ShortRead;
        }
        const b = _.buf[_.pos];
        _.pos += 1;
        return b;
    }

    /// 读取 `count` 字节零拷贝切片（对齐 Bun `read()` / `bytes()`）
    fn readBytes(*_, count: usize): ([]const u8)! {
        if (count > _.remaining()) {
            return err.ShortRead;
        }
        const start = _.pos;
        _.pos += count;
        return _.buf[start.._.pos];
    }

    /// 读取以 `\0` 结尾的零拷贝 C 字符串（对齐 Bun `readZ()`，经 `simdFindNul` 加速，消费末尾 `\0` 但不包含在返回切片中）
    fn readZ(*_): ([]const u8)! {
        const start = _.pos;
        const nul_rel = simdFindNul(_.buf[start..]) ?? return err.UnterminatedString;
        const nul_pos = start + nul_rel;
        _.pos = nul_pos + 1;
        return _.buf[start..nul_pos];
    }

    /// 非消费窥视 4 字节大端无符号整数（对齐 Bun `peekInt(u32)`）
    fn peekU32(): u32? {
        if (_.remaining() < 4) {
            return null;
        }
        const p = _.pos;
        return (@as(u32, _.buf[p]) << 24) |
            (@as(u32, _.buf[p + 1]) << 16) |
            (@as(u32, _.buf[p + 2]) << 8) |
            @as(u32, _.buf[p + 3]);
    }

    /// 非消费窥视 4 字节大端有符号整数（对齐 Bun `peekInt(i32)`）
    fn peekI32(): i32? {
        if (u := _.peekU32()) {
            return @bitCast(u);
        }
        return null;
    }

    /// 读取 2 字节大端无符号整数（对齐 Bun `short()`）
    fn readU16(*_): u16! {
        const s = _.readBytes(2)?;
        return (@as(u16, s[0]) << 8) | @as(u16, s[1]);
    }

    /// 读取 2 字节大端有符号整数
    fn readI16(*_): i16! {
        const bits = _.readU16()?;
        return @bitCast(bits);
    }

    /// 读取 4 字节大端无符号整数
    fn readU32(*_): u32! {
        const s = _.readBytes(4)?;
        return (@as(u32, s[0]) << 24) |
            (@as(u32, s[1]) << 16) |
            (@as(u32, s[2]) << 8) |
            @as(u32, s[3]);
    }

    /// 读取 4 字节大端有符号整数（对齐 Bun `int4()`）
    fn readI32(*_): i32! {
        const bits = _.readU32()?;
        return @bitCast(bits);
    }

    /// 读取 8 字节大端有符号整数
    fn readI64(*_): i64! {
        const s = _.readBytes(8)?;
        const bits: u64 = (@as(u64, s[0]) << 56) |
            (@as(u64, s[1]) << 48) |
            (@as(u64, s[2]) << 40) |
            (@as(u64, s[3]) << 32) |
            (@as(u64, s[4]) << 24) |
            (@as(u64, s[5]) << 16) |
            (@as(u64, s[6]) << 8) |
            @as(u64, s[7]);
        return @bitCast(bits);
    }

    /// 读取 4 字节 IEEE-754 大端单精度浮点数
    fn readF32(*_): f32! {
        const bits = _.readU32()?;
        return @bitCast(bits);
    }

    /// 读取 8 字节 IEEE-754 大端双精度浮点数
    fn readF64(*_): f64! {
        const bits_i = _.readI64()?;
        const bits_u: u64 = @bitCast(bits_i);
        return @bitCast(bits_u);
    }

    /// 读取并校验 PostgreSQL 4 字节含自身长度字段（带上限 `max_len`）：
    /// 若 `< 4` 返回 `InvalidMessageLength`；若 `> max_len` 返回 `FrameTooLarge`；
    /// 若剩余字节不足 `len - 4`，保持 `pos` 不变返回 `ShortRead`。
    fn readMessageLengthWithLimit(*_, max_len: u32): u32! {
        if (_.remaining() < 4) {
            return err.ShortRead;
        }
        const saved = _.pos;
        const len_i32 = _.readI32()?;
        if (len_i32 < 4) {
            _.pos = saved;
            return err.InvalidMessageLength;
        }
        const total_u32: u32 = @intCast(len_i32);
        if (total_u32 > max_len) {
            _.pos = saved;
            return err.FrameTooLarge;
        }
        const payload_len: usize = @as(usize, total_u32 - 4);
        if (payload_len > _.remaining()) {
            _.pos = saved;
            return err.ShortRead;
        }
        return total_u32;
    }

    /// 读取并校验 PostgreSQL 4 字节含自身长度字段（默认上限 `PG_MAX_FRAME_LEN`）。
    fn readMessageLength(*_): u32! {
        return _.readMessageLengthWithLimit(PG_MAX_FRAME_LEN);
    }

    /// 读取完整的 PostgreSQL 带 Tag 帧（自定义帧长上限 `max_len`），半包或非法长度时原子恢复 `pos`。
    fn readTaggedFrameWithLimit(*_, max_len: u32): PgFrameView! {
        if (_.remaining() < 5) {
            return err.ShortRead;
        }
        const saved = _.pos;
        const tag = _.readByte()?;
        const total_len = _.readMessageLengthWithLimit(max_len) catch |e| {
            _.pos = saved;
            return e;
        };
        const payload_len: usize = @as(usize, total_len - 4);
        const payload = _.readBytes(payload_len) catch |e| {
            _.pos = saved;
            return e;
        };
        return PgFrameView{
            tag: tag,
            payload: payload,
        };
    }

    /// 读取完整的 PostgreSQL 带 Tag 帧（1 字节 Tag + 4 字节含自身长度 + 载荷零拷贝切片）；
    /// 半包或非法长度时原子恢复 `pos`，不破坏流状态。
    fn readTaggedFrame(*_): PgFrameView! {
        return _.readTaggedFrameWithLimit(PG_MAX_FRAME_LEN);
    }

    /// 读取 PostgreSQL 长度前缀可空值（4 字节大端 `i32` 长度：`-1` 返回 `null`，`< -1` 返回 `InvalidMessageLength`）
    fn readLengthPrefixedOrNull(*_): (([]const u8)?)! {
        if (_.remaining() < 4) {
            return err.ShortRead;
        }
        const saved = _.pos;
        const len_i32 = _.readI32()?;
        if (len_i32 == -1) {
            return null;
        }
        if (len_i32 < -1) {
            _.pos = saved;
            return err.InvalidMessageLength;
        }
        const byte_u32: u32 = @intCast(len_i32);
        if (byte_u32 > PG_MAX_FRAME_LEN) {
            _.pos = saved;
            return err.FrameTooLarge;
        }
        const byte_len: usize = @as(usize, byte_u32);
        if (byte_len > _.remaining()) {
            _.pos = saved;
            return err.ShortRead;
        }
        const data = _.readBytes(byte_len)?;
        return data;
    }

    /// 精确校验并消费期望的固定字节序列（对齐 Bun `eatMessage()`）；不匹配时回退 `pos` 并返回 `InvalidMessage`
    fn eatBytes(*_, expected: []const u8): usize! {
        if (expected.len > _.remaining()) {
            return err.ShortRead;
        }
        const start = _.pos;
        let i: usize = 0;
        while (i < expected.len) {
            if (_.buf[start + i] != expected[i]) {
                return err.InvalidMessage;
            }
            i += 1;
        }
        _.pos += expected.len;
        return expected.len;
    }
}
