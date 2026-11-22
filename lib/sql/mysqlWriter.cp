// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/NewWriter.cp | Synced: 2026-11-22T03:41:50Z
// Pure-CP zero-allocation MySQL wire-protocol binary packet writer:
// - 4-byte MySQL packet header (24-bit LE payload length + 1-byte sequence_id)
//   two-phase backpatching (`beginPacket` / `finishPacket`) with 0xffffff overflow guard
// - Little-endian integer & IEEE-754 float encoders (u8/u16/u24/u32/u64/i32/i64/f32/f64)
// - Auto-NUL C-string (`writeZ`), length-encoded integer/string/NULL (`writeLenEncInt`,
//   `writeLenEncString`, `writeLenEncNull`), and binary protocol NULL bitmap (`writeNullBitmap`)
// - Zero heap allocations, zero external bindings.

import {
    MYSQL_LENENC_NULL_MARKER,
    encodeLengthInt,
} from "./mysqlLenenc.cp";

/// MySQL 线协议包头固定长度（3 字节小端载荷长度 + 1 字节序列号）
pub const MYSQL_PACKET_HEADER_SIZE: usize = 4;
/// MySQL 单包最大载荷长度（2^24 - 1 = 16,777,215 字节）
pub const MYSQL_MAX_PACKET_PAYLOAD: usize = 0xffffff;

/// MySQL 线协议写入器错误集
pub err MySqlWireWriterError {
    BufferTooSmall,
    InvalidPatchOffset,
    PacketLengthOverflow,
}

/// MySQL 包头回填句柄（记录 4 字节包头起始偏移与序列号）
pub struct MySqlPacketPatch {
    offset: usize = 0,
    sequence_id: u8 = 0,
}

/// 零堆分配 MySQL 线协议二进制包写入器（对齐 Bun `NewWriter`）
pub struct MySqlWireWriter {
    buf: []u8,
    pos: usize = 0,

    /// 在调用方提供的固定缓冲区上初始化写入器
    static fn init(buf: []u8): MySqlWireWriter {
        return MySqlWireWriter{
            buf: buf,
            pos: 0,
        };
    }

    /// 返回当前已写入字节偏移（对齐 Bun `offset()`）
    fn offset(): usize {
        return _.pos;
    }

    /// 返回缓冲区剩余可写字节数
    fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    /// 返回当前已写入的只读零拷贝切片
    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    /// 重置写偏移为 0 以便复用缓冲区
    fn reset(*_) {
        _.pos = 0;
    }

    /// 写入单个字节（对齐 Bun `int1()`）
    fn writeByte(*_, value: u8): usize! {
        if (_.pos >= _.buf.len) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = value;
        _.pos += 1;
        return 1;
    }

    /// 写入原始字节切片（对齐 Bun `write()`）
    fn writeBytes(*_, data: []const u8): usize! {
        if (data.len > _.remaining()) {
            return err.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos + i] = data[i];
            i += 1;
        }
        _.pos += data.len;
        return data.len;
    }

    /// 在指定偏移 `at` 处随机覆盖写入（对齐 Bun `pwrite()`，不推进 `pos`）
    fn pwrite(*_, data: []const u8, at: usize): usize! {
        if (at > _.buf.len || data.len > _.buf.len - at) {
            return err.InvalidPatchOffset;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buf[at + i] = data[i];
            i += 1;
        }
        return data.len;
    }

    /// 写入 2 字节小端无符号整数
    fn writeU16(*_, value: u16): usize! {
        if (_.remaining() < 2) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = @intCast(value & 0xff);
        _.buf[_.pos + 1] = @intCast((value >> 8) & 0xff);
        _.pos += 2;
        return 2;
    }

    /// 写入 3 字节小端无符号整数（24-bit，超出 0xffffff 返回 `PacketLengthOverflow`）
    fn writeU24(*_, value: u32): usize! {
        if (value > 0xffffff) {
            return err.PacketLengthOverflow;
        }
        if (_.remaining() < 3) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = @intCast(value & 0xff);
        _.buf[_.pos + 1] = @intCast((value >> 8) & 0xff);
        _.buf[_.pos + 2] = @intCast((value >> 16) & 0xff);
        _.pos += 3;
        return 3;
    }

    /// 写入 4 字节小端无符号整数（对齐 Bun `int4()`）
    fn writeU32(*_, value: u32): usize! {
        if (_.remaining() < 4) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = @intCast(value & 0xff);
        _.buf[_.pos + 1] = @intCast((value >> 8) & 0xff);
        _.buf[_.pos + 2] = @intCast((value >> 16) & 0xff);
        _.buf[_.pos + 3] = @intCast((value >> 24) & 0xff);
        _.pos += 4;
        return 4;
    }

    /// 写入 8 字节小端无符号整数（对齐 Bun `int8()`）
    fn writeU64(*_, value: u64): usize! {
        if (_.remaining() < 8) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = @intCast(value & 0xff);
        _.buf[_.pos + 1] = @intCast((value >> 8) & 0xff);
        _.buf[_.pos + 2] = @intCast((value >> 16) & 0xff);
        _.buf[_.pos + 3] = @intCast((value >> 24) & 0xff);
        _.buf[_.pos + 4] = @intCast((value >> 32) & 0xff);
        _.buf[_.pos + 5] = @intCast((value >> 40) & 0xff);
        _.buf[_.pos + 6] = @intCast((value >> 48) & 0xff);
        _.buf[_.pos + 7] = @intCast((value >> 56) & 0xff);
        _.pos += 8;
        return 8;
    }

    /// 写入 4 字节小端有符号整数
    fn writeI32(*_, value: i32): usize! {
        const bits: u32 = @bitCast(value);
        return _.writeU32(bits);
    }

    /// 写入 8 字节小端有符号整数
    fn writeI64(*_, value: i64): usize! {
        const bits: u64 = @bitCast(value);
        return _.writeU64(bits);
    }

    /// 写入 4 字节 IEEE-754 小端单精度浮点数
    fn writeF32(*_, value: f32): usize! {
        const bits: u32 = @bitCast(value);
        return _.writeU32(bits);
    }

    /// 写入 8 字节 IEEE-754 小端双精度浮点数
    fn writeF64(*_, value: f64): usize! {
        const bits: u64 = @bitCast(value);
        return _.writeU64(bits);
    }

    /// 写入以 `\0` 结尾的 C 字符串（对齐 Bun `writeZ()`：若末尾已有 `\0` 不重复追加）
    fn writeZ(*_, value: []const u8): usize! {
        const needs_nul = value.len == 0 || value[value.len - 1] != 0;
        const total = value.len + (if (needs_nul) 1 else 0);
        if (total > _.remaining()) {
            return err.BufferTooSmall;
        }
        * = _.writeBytes(value)?;
        if (needs_nul) {
            * = _.writeByte(0)?;
        }
        return total;
    }

    /// 写入 MySQL 变长整数 `lenenc-int`（对齐 Bun `writeLengthEncodedInt()`）
    fn writeLenEncInt(*_, value: u64): usize! {
        const enc = encodeLengthInt(value);
        return _.writeBytes(enc.slice());
    }

    /// 写入 MySQL 长度前缀字符串 `lenenc-str`（对齐 Bun `writeLengthEncodedString()`）
    fn writeLenEncString(*_, data: []const u8): usize! {
        const enc = encodeLengthInt(@as(u64, data.len));
        if (enc.len + data.len > _.remaining()) {
            return err.BufferTooSmall;
        }
        * = _.writeBytes(enc.slice())?;
        * = _.writeBytes(data)?;
        return enc.len + data.len;
    }

    /// 写入 MySQL `lenenc` NULL 标记字节（`0xfb`）
    fn writeLenEncNull(*_): usize! {
        return _.writeByte(MYSQL_LENENC_NULL_MARKER);
    }

    /// 写入 MySQL 二进制协议 NULL 位图（`COM_STMT_EXECUTE` 参数用 `bit_offset=0`，结果行用 `bit_offset=2`）
    fn writeNullBitmap(*_, is_null: []const bool, bit_offset: usize): usize! {
        const bitmap_len = (is_null.len + bit_offset + 7) / 8;
        if (bitmap_len > _.remaining()) {
            return err.BufferTooSmall;
        }
        const start = _.pos;
        let b: usize = 0;
        while (b < bitmap_len) {
            _.buf[start + b] = 0;
            b += 1;
        }
        let i: usize = 0;
        while (i < is_null.len) {
            if (is_null[i]) {
                const bit_idx = i + bit_offset;
                const byte_pos = bit_idx / 8;
                const bit_pos: u3 = @intCast(bit_idx % 8);
                _.buf[start + byte_pos] |= (@as(u8, 1) << bit_pos);
            }
            i += 1;
        }
        _.pos += bitmap_len;
        return bitmap_len;
    }

    /// 预留 4 字节 MySQL 包头并返回回填句柄（对齐 Bun `start(sequence_id)`）
    fn beginPacket(*_, sequence_id: u8): MySqlPacketPatch! {
        if (_.remaining() < MYSQL_PACKET_HEADER_SIZE) {
            return err.BufferTooSmall;
        }
        const start = _.pos;
        * = _.writeU32(0)?;
        return MySqlPacketPatch{
            offset: start,
            sequence_id: sequence_id,
        };
    }

    /// 回填 4 字节 MySQL 包头（3 字节小端净载荷长度 + 1 字节 `sequence_id`，对齐 Bun `Packet.end()`）
    fn finishPacket(*_, patch: MySqlPacketPatch): u32! {
        if (patch.offset + MYSQL_PACKET_HEADER_SIZE > _.pos || patch.offset + MYSQL_PACKET_HEADER_SIZE > _.buf.len) {
            return err.InvalidPatchOffset;
        }
        const payload_len = _.pos - patch.offset - MYSQL_PACKET_HEADER_SIZE;
        if (payload_len > MYSQL_MAX_PACKET_PAYLOAD) {
            return err.PacketLengthOverflow;
        }
        const len_u32: u32 = @intCast(payload_len);
        let hdr: [4]u8 = [
            @intCast(len_u32 & 0xff),
            @intCast((len_u32 >> 8) & 0xff),
            @intCast((len_u32 >> 16) & 0xff),
            patch.sequence_id,
        ];
        * = _.pwrite(hdr[0..4], patch.offset)?;
        return len_u32;
    }
}
