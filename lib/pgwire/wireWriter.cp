// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NewWriter.cp | Synced: 2026-11-24T04:40:51Z
// Pure-CP zero-allocation PostgreSQL v3.0 big-endian wire writer (`PgWireWriter`).
// Provides big-endian scalar & IEEE-754 encoders, NUL-terminated C-string helpers,
// nullable Int32-prefixed byte fields, and back-patched length bookmarks (`beginFrame`/`finishLength`).

/// 长度占位符字节数（PostgreSQL `Int32` 大端长度）
pub const PG_LENGTH_PLACEHOLDER_LEN: usize = 4;

/// `PgWireWriter` 写入与回填错误集
pub err PgWireWriterError {
    BufferTooSmall,
    InvalidBookmark,
    LengthOverflow,
    EmbeddedNul,
}

/// 延迟回填长度书签（对应 Bun `NewWriterLengthWriter`，零指针悬空风险）
pub struct PgLengthBookmark {
    index: usize = 0,
    include_self: bool = true,
}

fn hasEmbeddedNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 零堆分配 PostgreSQL v3.0 大端序线协议写入器
pub struct PgWireWriter {
    buffer: []u8,
    offset: usize = 0,

    /// 在调用方提供的目标切片上初始化写入器
    static fn init(buffer: []u8): PgWireWriter {
        return PgWireWriter{
            buffer: buffer,
            offset: 0,
        };
    }

    /// 返回当前已写入字节数
    fn len(): usize {
        return _.offset;
    }

    /// 返回剩余可写字节数
    fn remaining(): usize {
        if (_.offset >= _.buffer.len) {
            return 0;
        }
        return _.buffer.len - _.offset;
    }

    /// 返回当前已写入内容的只读切片视图
    fn written(): []const u8 {
        return _.buffer[0.._.offset];
    }

    /// 重置写指针至起始处（零开销复用缓冲）
    fn reset(*_) {
        _.offset = 0;
    }

    /// 顺序追加字节切片，返回新偏移量
    fn writeBytes(*_, data: []const u8): PgWireWriterError!usize {
        if (data.len > _.remaining()) {
            return PgWireWriterError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buffer[_.offset + i] = data[i];
            i += 1;
        }
        _.offset += data.len;
        return _.offset;
    }

    /// 在指定偏移 `at` 处随机覆写字节切片（不改变当前 `offset`）
    fn pwriteBytes(*_, data: []const u8, at: usize): PgWireWriterError!usize {
        if (at > _.buffer.len || data.len > _.buffer.len - at) {
            return PgWireWriterError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < data.len) {
            _.buffer[at + i] = data[i];
            i += 1;
        }
        return at + data.len;
    }

    /// 写入 1 字节无符号整数
    fn writeU8(*_, v: u8): PgWireWriterError!usize {
        if (_.remaining() < 1) {
            return PgWireWriterError.BufferTooSmall;
        }
        _.buffer[_.offset] = v;
        _.offset += 1;
        return _.offset;
    }

    /// 写入 2 字节大端无符号整数
    fn writeU16Be(*_, v: u16): PgWireWriterError!usize {
        if (_.remaining() < 2) {
            return PgWireWriterError.BufferTooSmall;
        }
        _.buffer[_.offset] = @as(u8, @intCast((v >> 8) & 0xFF));
        _.buffer[_.offset + 1] = @as(u8, @intCast(v & 0xFF));
        _.offset += 2;
        return _.offset;
    }

    /// 写入 2 字节大端有符号整数
    fn writeI16Be(*_, v: i16): PgWireWriterError!usize {
        const u: u16 = @bitCast(v);
        return _.writeU16Be(u);
    }

    /// 写入 4 字节大端无符号整数
    fn writeU32Be(*_, v: u32): PgWireWriterError!usize {
        if (_.remaining() < 4) {
            return PgWireWriterError.BufferTooSmall;
        }
        _.buffer[_.offset] = @as(u8, @intCast((v >> 24) & 0xFF));
        _.buffer[_.offset + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
        _.buffer[_.offset + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
        _.buffer[_.offset + 3] = @as(u8, @intCast(v & 0xFF));
        _.offset += 4;
        return _.offset;
    }

    /// 写入 4 字节大端有符号整数
    fn writeI32Be(*_, v: i32): PgWireWriterError!usize {
        const u: u32 = @bitCast(v);
        return _.writeU32Be(u);
    }

    /// 写入 8 字节大端无符号整数
    fn writeU64Be(*_, v: u64): PgWireWriterError!usize {
        if (_.remaining() < 8) {
            return PgWireWriterError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < 8) {
            const shift: u6 = @as(u6, @intCast((7 - i) * 8));
            _.buffer[_.offset + i] = @as(u8, @intCast((v >> shift) & 0xFF));
            i += 1;
        }
        _.offset += 8;
        return _.offset;
    }

    /// 写入 8 字节大端有符号整数
    fn writeI64Be(*_, v: i64): PgWireWriterError!usize {
        const u: u64 = @bitCast(v);
        return _.writeU64Be(u);
    }

    /// 写入 IEEE-754 32 位大端浮点数
    fn writeF32Be(*_, v: f32): PgWireWriterError!usize {
        const u: u32 = @bitCast(v);
        return _.writeU32Be(u);
    }

    /// 写入 IEEE-754 64 位大端浮点数
    fn writeF64Be(*_, v: f64): PgWireWriterError!usize {
        const u: u64 = @bitCast(v);
        return _.writeU64Be(u);
    }

    /// 写入严格 C 字符串（拦截内嵌 `0x00`，自动追加尾部 `0x00`）
    fn writeCString(*_, s: []const u8): PgWireWriterError!usize {
        if (hasEmbeddedNul(s)) {
            return PgWireWriterError.EmbeddedNul;
        }
        if (_.remaining() < s.len + 1) {
            return PgWireWriterError.BufferTooSmall;
        }
        * = _.writeBytes(s)?;
        return _.writeU8(0);
    }

    /// 对齐 Bun `NewWriter.string`：若 `s` 末尾已含 `0x00` 则不重复追加
    fn writeStringZ(*_, s: []const u8): PgWireWriterError!usize {
        const need_nul = s.len == 0 || s[s.len - 1] != 0;
        const needed = s.len + (if (need_nul) @as(usize, 1) else @as(usize, 0));
        if (_.remaining() < needed) {
            return PgWireWriterError.BufferTooSmall;
        }
        * = _.writeBytes(s)?;
        if (need_nul) {
            return _.writeU8(0);
        }
        return _.offset;
    }

    /// 写入 PostgreSQL 可空定长前缀字节串（`null` -> `Int32(-1)`；非空 -> `Int32(len) + data`）
    fn writeNullableBytes(*_, data_opt: ([]const u8)?): PgWireWriterError!usize {
        if (data := data_opt) {
            if (data.len > 0x7FFFFFFF) {
                return PgWireWriterError.LengthOverflow;
            }
            if (_.remaining() < 4 + data.len) {
                return PgWireWriterError.BufferTooSmall;
            }
            * = _.writeI32Be(@as(i32, @intCast(data.len)))?;
            return _.writeBytes(data);
        }
        return _.writeI32Be(-1);
    }

    /// 预留 4 字节大端长度占位并返回书签（`include_self=true` 对应帧头长度，`false` 对应字段净长）
    fn beginLength(*_, include_self: bool): PgWireWriterError!PgLengthBookmark {
        const idx = _.offset;
        * = _.writeU32Be(0)?;
        return PgLengthBookmark{
            index: idx,
            include_self: include_self,
        };
    }

    /// 写入 1 字节报文 Tag 并预留 4 字节大端长度占位（`include_self = true`）
    fn beginFrame(*_, tag: u8): PgWireWriterError!PgLengthBookmark {
        if (_.remaining() < 5) {
            return PgWireWriterError.BufferTooSmall;
        }
        * = _.writeU8(tag)?;
        return _.beginLength(true);
    }

    /// 回填书签对应的 4 字节大端长度，返回写入的长度值
    fn finishLength(*_, bm: PgLengthBookmark): PgWireWriterError!u32 {
        if (bm.index + PG_LENGTH_PLACEHOLDER_LEN > _.offset) {
            return PgWireWriterError.InvalidBookmark;
        }
        const span = _.offset - bm.index;
        const raw_len = if (bm.include_self) span else (span - PG_LENGTH_PLACEHOLDER_LEN);
        if (raw_len > 0x7FFFFFFF) {
            return PgWireWriterError.LengthOverflow;
        }
        const len_u32: u32 = @as(u32, @intCast(raw_len));
        let tmp: [4]u8 = [
            @as(u8, @intCast((len_u32 >> 24) & 0xFF)),
            @as(u8, @intCast((len_u32 >> 16) & 0xFF)),
            @as(u8, @intCast((len_u32 >> 8) & 0xFF)),
            @as(u8, @intCast(len_u32 & 0xFF)),
        ];
        * = _.pwriteBytes(tmp[0..], bm.index)?;
        return len_u32;
    }
}
