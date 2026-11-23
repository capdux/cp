// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NewWriter.cp | Synced: 2026-11-22T03:32:00Z
// Reference-hardened against sfackler/rust-postgres (MIT/Apache-2.0)
// Pure-CP zero-allocation PostgreSQL wire-protocol binary frame writer:
// - Big-endian integer/float encoders (u16/short/u32/i32/i64/f32/f64)
// - C-string auto-NUL termination with embedded-NUL rejection (`Simd.indexOfByte(0)`), text-mode bool ('t'/'f'), NULL (-1 / 0xFFFFFFFF)
// - 16-lane `@Vector(16, u8)` SIMD payload copy (`simdPgCopyBytes`)
// - Length-prefixed payload writer and two-phase length backpatching
//   (`beginLength` / `beginTaggedFrame` + `finishLength` / `finishLengthExcludingSelf`)
// - Strict bounds, parameter-count, patch-offset and signed 32-bit frame-length overflow guards.

import { Simd } from "cap:simd";

type PgWriterVec16 = @Vector(16, u8);

/// 16-lane `@Vector(16, u8)` SIMD payload copy for PostgreSQL wire frames and column buffers.
pub fn simdPgCopyBytes(dst: []u8, src: []const u8): usize {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const chunk: PgWriterVec16 = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

/// PostgreSQL 线协议 NULL 长度哨兵（无符号 32 位 0xFFFFFFFF）
pub const PG_NULL_LENGTH_U32: u32 = 0xffffffff;
/// PostgreSQL 线协议 NULL 长度哨兵（有符号 32 位 -1）
pub const PG_NULL_LENGTH_I32: i32 = -1;

/// PostgreSQL 线协议写入器错误集
pub err PgWireWriterError {
    BufferTooSmall,
    TooManyParameters,
    InvalidPatchOffset,
    FrameLengthOverflow,
    EmbeddedNul,
}

/// 长度字段回填句柄（记录 4 字节大端长度占位符在缓冲区中的起始偏移）
pub struct PgLengthPatch {
    offset: usize = 0,
}

/// 零堆分配 PostgreSQL 线协议二进制写入器（对齐 Bun `NewWriter`）
pub struct PgWireWriter {
    buf: []u8,
    pos: usize = 0,

    /// 在调用方提供的固定缓冲区上初始化写入器
    static fn init(buf: []u8): PgWireWriter {
        return PgWireWriter{
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

    /// 写入单个原始字节（常用于前端消息类型 Tag，如 `'Q'` / `'P'` / `'B'` / `'E'` / `'S'`）
    fn writeByte(*_, b: u8): usize! {
        if (_.pos >= _.buf.len) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return 1;
    }

    /// 写入原始字节切片（对齐 Bun `write()`，采用 16-lane SIMD 块拷贝）
    fn writeBytes(*_, data: []const u8): usize! {
        if (data.len > _.remaining()) {
            return err.BufferTooSmall;
        }
        const copied = simdPgCopyBytes(_.buf[_.pos .. _.pos + data.len], data);
        if (copied != data.len) return err.BufferTooSmall;
        _.pos += data.len;
        return data.len;
    }

    /// 在指定偏移 `at` 处随机覆盖写入（对齐 Bun `pwrite()`，不推进 `pos`）
    fn pwrite(*_, data: []const u8, at: usize): usize! {
        if (at > _.buf.len || data.len > _.buf.len - at) {
            return err.InvalidPatchOffset;
        }
        const copied = simdPgCopyBytes(_.buf[at .. at + data.len], data);
        if (copied != data.len) return err.InvalidPatchOffset;
        return data.len;
    }

    /// 写入 2 字节大端无符号整数
    fn writeU16(*_, value: u16): usize! {
        if (_.remaining() < 2) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = @intCast((value >> 8) & 0xff);
        _.buf[_.pos + 1] = @intCast(value & 0xff);
        _.pos += 2;
        return 2;
    }

    /// 写入 2 字节大端参数计数（对齐 Bun `short()`，超出 `u16` 上限 65535 返回 `TooManyParameters`）
    fn writeShort(*_, value: usize): usize! {
        if (value > 65535) {
            return err.TooManyParameters;
        }
        return _.writeU16(@intCast(value));
    }

    /// 写入 4 字节大端无符号整数（对齐 Bun `int4()`）
    fn writeU32(*_, value: u32): usize! {
        if (_.remaining() < 4) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = @intCast((value >> 24) & 0xff);
        _.buf[_.pos + 1] = @intCast((value >> 16) & 0xff);
        _.buf[_.pos + 2] = @intCast((value >> 8) & 0xff);
        _.buf[_.pos + 3] = @intCast(value & 0xff);
        _.pos += 4;
        return 4;
    }

    /// 写入 4 字节大端有符号整数（对齐 Bun `sint4()`）
    fn writeI32(*_, value: i32): usize! {
        const bits: u32 = @bitCast(value);
        return _.writeU32(bits);
    }

    /// 写入 8 字节大端有符号整数（对齐 Bun `int8()`）
    fn writeI64(*_, value: i64): usize! {
        if (_.remaining() < 8) {
            return err.BufferTooSmall;
        }
        const bits: u64 = @bitCast(value);
        _.buf[_.pos] = @intCast((bits >> 56) & 0xff);
        _.buf[_.pos + 1] = @intCast((bits >> 48) & 0xff);
        _.buf[_.pos + 2] = @intCast((bits >> 40) & 0xff);
        _.buf[_.pos + 3] = @intCast((bits >> 32) & 0xff);
        _.buf[_.pos + 4] = @intCast((bits >> 24) & 0xff);
        _.buf[_.pos + 5] = @intCast((bits >> 16) & 0xff);
        _.buf[_.pos + 6] = @intCast((bits >> 8) & 0xff);
        _.buf[_.pos + 7] = @intCast(bits & 0xff);
        _.pos += 8;
        return 8;
    }

    /// 写入 4 字节 IEEE-754 大端单精度浮点数（对齐 Bun `f32()`）
    fn writeF32(*_, value: f32): usize! {
        const bits: u32 = @bitCast(value);
        return _.writeU32(bits);
    }

    /// 写入 8 字节 IEEE-754 大端双精度浮点数（对齐 Bun `f64()`）
    fn writeF64(*_, value: f64): usize! {
        if (_.remaining() < 8) {
            return err.BufferTooSmall;
        }
        const bits: u64 = @bitCast(value);
        _.buf[_.pos] = @intCast((bits >> 56) & 0xff);
        _.buf[_.pos + 1] = @intCast((bits >> 48) & 0xff);
        _.buf[_.pos + 2] = @intCast((bits >> 40) & 0xff);
        _.buf[_.pos + 3] = @intCast((bits >> 32) & 0xff);
        _.buf[_.pos + 4] = @intCast((bits >> 24) & 0xff);
        _.buf[_.pos + 5] = @intCast((bits >> 16) & 0xff);
        _.buf[_.pos + 6] = @intCast((bits >> 8) & 0xff);
        _.buf[_.pos + 7] = @intCast(bits & 0xff);
        _.pos += 8;
        return 8;
    }

    /// 写入以 `\0` 结尾的 C 字符串（对齐 Bun `string()` / `bytes()`：拒绝内嵌 `\0` 截断，若末尾已有 `\0` 不重复追加）
    fn writeCString(*_, value: []const u8): usize! {
        if (nul_idx := Simd.indexOfByte(value, 0)) {
            if (nul_idx + 1 < value.len) {
                return err.EmbeddedNul;
            }
        }
        const needs_nul = value.len == 0 || value[value.len - 1] != 0;
        const extra: usize = if (needs_nul) 1 else 0;
        const total = value.len + extra;
        if (total > _.remaining()) {
            return err.BufferTooSmall;
        }
        * = _.writeBytes(value)?;
        if (needs_nul) {
            * = _.writeByte(0)?;
        }
        return total;
    }

    /// 严格写入 C 字符串（对标 `rust-postgres`：拒绝任意位置出现 `\0`，并追加单个 `\0` 终止符）
    fn writeCStringStrict(*_, value: []const u8): usize! {
        if (Simd.indexOfByte(value, 0) != null) {
            return err.EmbeddedNul;
        }
        if (value.len + 1 > _.remaining()) {
            return err.BufferTooSmall;
        }
        * = _.writeBytes(value)?;
        * = _.writeByte(0)?;
        return value.len + 1;
    }

    /// 写入 PostgreSQL 文本格式布尔值 `'t'` / `'f'`（对齐 Bun `bool()`）
    fn writeBoolText(*_, value: bool): usize! {
        const ch: u8 = if (value) 't' else 'f';
        return _.writeByte(ch);
    }

    /// 写入 PostgreSQL NULL 长度哨兵 `-1` (`0xFFFFFFFF`，对齐 Bun `null()`)
    fn writeNull(*_): usize! {
        return _.writeU32(PG_NULL_LENGTH_U32);
    }

    /// 写入 4 字节大端长度前缀 + 原始字节载荷（常用于 Bind 参数值编码）
    fn writeLengthPrefixed(*_, data: []const u8): usize! {
        if (data.len > 0x7fffffff) {
            return err.FrameLengthOverflow;
        }
        if (4 + data.len > _.remaining()) {
            return err.BufferTooSmall;
        }
        * = _.writeU32(@intCast(data.len))?;
        * = _.writeBytes(data)?;
        return 4 + data.len;
    }

    /// 预留 4 字节长度占位符并返回回填句柄（对齐 Bun `length()`）
    fn beginLength(*_): PgLengthPatch! {
        const start = _.pos;
        * = _.writeU32(0)?;
        return PgLengthPatch{ offset: start };
    }

    /// 写入 1 字节前端消息类型 Tag 并预留 4 字节长度占位符
    fn beginTaggedFrame(*_, tag: u8): PgLengthPatch! {
        if (_.remaining() < 5) {
            return err.BufferTooSmall;
        }
        * = _.writeByte(tag)?;
        return _.beginLength();
    }

    /// 回填包含自身 4 字节在内的消息帧总长度（对齐 Bun `LengthWriter.write()`，严格限制 `<= i32::MAX`）
    fn finishLength(*_, patch: PgLengthPatch): u32! {
        if (patch.offset + 4 > _.pos || patch.offset + 4 > _.buf.len) {
            return err.InvalidPatchOffset;
        }
        const frame_len = _.pos - patch.offset;
        if (frame_len > 0x7fffffff) {
            return err.FrameLengthOverflow;
        }
        const len_u32: u32 = @intCast(frame_len);
        let raw: [4]u8 = [
            @intCast((len_u32 >> 24) & 0xff),
            @intCast((len_u32 >> 16) & 0xff),
            @intCast((len_u32 >> 8) & 0xff),
            @intCast(len_u32 & 0xff),
        ];
        * = _.pwrite(raw[0..4], patch.offset)?;
        return len_u32;
    }

    /// 回填不含自身 4 字节在内的载荷净长度（对齐 Bun `LengthWriter.writeExcludingSelf()`，严格限制 `<= i32::MAX`）
    fn finishLengthExcludingSelf(*_, patch: PgLengthPatch): u32! {
        if (patch.offset + 4 > _.pos || patch.offset + 4 > _.buf.len) {
            return err.InvalidPatchOffset;
        }
        const payload_len = _.pos - (patch.offset + 4);
        if (payload_len > 0x7fffffff) {
            return err.FrameLengthOverflow;
        }
        const len_u32: u32 = @intCast(payload_len);
        let raw: [4]u8 = [
            @intCast((len_u32 >> 24) & 0xff),
            @intCast((len_u32 >> 16) & 0xff),
            @intCast((len_u32 >> 8) & 0xff),
            @intCast(len_u32 & 0xff),
        ];
        * = _.pwrite(raw[0..4], patch.offset)?;
        return len_u32;
    }
}
