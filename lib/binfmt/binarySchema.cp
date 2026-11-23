// Source: Absorbed from /data/dev/bun/src/analytics/schema.cp | Synced: 2026-11-21T12:33:59Z

/// 紧凑记录结束标记字节（Field ID == 0 表示消息结束）
pub const END_OF_MESSAGE: u8 = 0;

/// 二进制 Schema 编解码错误集
pub err SchemaError {
    UnexpectedEof,
    InvalidEnumValue,
    InvalidFieldId,
    BufferTooSmall,
    OutOfMemory,
}

/// 零拷贝二进制 Schema 流式读取器（小端固定宽度编码，跨平台确定性布局）
pub struct SchemaReader {
    data: []const u8,
    pos: usize = 0,

    static fn init(data: []const u8): SchemaReader {
        return SchemaReader{
            data: data,
            pos: 0,
        };
    }

    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    fn offset(): usize {
        return _.pos;
    }

    fn isEof(): bool {
        return _.pos >= _.data.len;
    }

    fn readBytes(*_, count: usize): ([]const u8) ! SchemaError {
        if (count > _.data.len - _.pos) {
            return err.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + count];
        _.pos += count;
        return slice;
    }

    fn readByte(*_): u8 ! SchemaError {
        const s = _.readBytes(1)?;
        return s[0];
    }

    fn readBool(*_): bool ! SchemaError {
        const b = _.readByte()?;
        return b != 0;
    }

    fn readI8(*_): i8 ! SchemaError {
        const b = _.readByte()?;
        const v: i8 = @bitCast(b);
        return v;
    }

    fn readU16(*_): u16 ! SchemaError {
        const s = _.readBytes(2)?;
        return (s[0] as u16) | ((s[1] as u16) << 8);
    }

    fn readI16(*_): i16 ! SchemaError {
        const raw = _.readU16()?;
        const v: i16 = @bitCast(raw);
        return v;
    }

    fn readU32(*_): u32 ! SchemaError {
        const s = _.readBytes(4)?;
        return (s[0] as u32)
            | ((s[1] as u32) << 8)
            | ((s[2] as u32) << 16)
            | ((s[3] as u32) << 24);
    }

    fn readI32(*_): i32 ! SchemaError {
        const raw = _.readU32()?;
        const v: i32 = @bitCast(raw);
        return v;
    }

    fn readU64(*_): u64 ! SchemaError {
        const lo = _.readU32()? as u64;
        const hi = _.readU32()? as u64;
        return lo | (hi << 32);
    }

    fn readI64(*_): i64 ! SchemaError {
        const raw = _.readU64()?;
        const v: i64 = @bitCast(raw);
        return v;
    }

    fn readInt(*_, comptime T: type): T ! SchemaError {
        return switch (comptime T) {
            u8 => _.readByte(),
            i8 => _.readI8(),
            u16 => _.readU16(),
            i16 => _.readI16(),
            u32 => _.readU32(),
            i32 => _.readI32(),
            u64 => _.readU64(),
            i64 => _.readI64(),
            else => @compileError("Unsupported integer type for SchemaReader.readInt"),
        };
    }

    /// 读取带 u32 长度前缀的字节数组切片（零拷贝引用底层缓冲区）
    fn readByteArray(*_): ([]const u8) ! SchemaError {
        const len = _.readU32()? as usize;
        if (len == 0) {
            return _.data[_.pos.._.pos];
        }
        return _.readBytes(len);
    }

    /// 读取带 u32 长度前缀的 UTF-8 字符串切片（零拷贝）
    fn readString(*_): string ! SchemaError {
        return _.readByteArray();
    }

    /// 读取 1 字节字段 ID（0 表示 `END_OF_MESSAGE`）
    fn readFieldId(*_): u8 ! SchemaError {
        return _.readByte();
    }

    /// 泛型枚举解码：严格校验标签值是否在合法枚举成员集合内，非法值返回 `err.InvalidEnumValue`
    fn readEnum(*_, comptime Enum: type): Enum ! SchemaError {
        const tag_type = @typeInfo(Enum).@"enum".tag_type;
        const raw = _.readInt(tag_type)?;
        comptime for (field of std.meta.fields(Enum)) {
            if (raw == field.value) {
                return @field(Enum, field.name);
            }
        }
        return err.InvalidEnumValue;
    }
}

/// 可增长的堆分配二进制 Schema 写入器（含 RAII `~init()` 析构，零内存泄漏）
pub struct SchemaWriter {
    buf: []u8,
    len: usize = 0,
    cap: usize = 0,
    allocator: std.mem.Allocator,

    static fn init(allocator: std.mem.Allocator): SchemaWriter {
        return SchemaWriter{
            buf: [],
            len: 0,
            cap: 0,
            allocator: allocator,
        };
    }

    static fn initCapacity(allocator: std.mem.Allocator, initial_cap: usize): SchemaWriter ! SchemaError {
        if (initial_cap == 0) {
            return SchemaWriter.init(allocator);
        }
        const mem = allocator.alloc(u8, initial_cap) catch return err.OutOfMemory;
        return SchemaWriter{
            buf: mem,
            len: 0,
            cap: initial_cap,
            allocator: allocator,
        };
    }

    fn bytes(): []const u8 {
        return _.buf[0.._.len];
    }

    fn written(): usize {
        return _.len;
    }

    fn reset(*_) {
        _.len = 0;
    }

    fn ensureCapacity(*_, additional: usize): void ! SchemaError {
        const need = _.len + additional;
        if (need <= _.cap) {
            return;
        }
        let next_cap: usize = if (_.cap == 0) usize(64) else (_.cap * usize(2));
        while (next_cap < need) {
            next_cap *= usize(2);
        }
        const next_buf = _.allocator.alloc(u8, next_cap) catch return err.OutOfMemory;
        if (_.len > 0) {
            @memcpy(next_buf[0.._.len], _.buf[0.._.len]);
        }
        if (_.cap > 0) {
            _.allocator.free(_.buf);
        }
        _.buf = next_buf;
        _.cap = next_cap;
    }

    fn writeBytes(*_, data: []const u8): void ! SchemaError {
        if (data.len == 0) {
            return;
        }
        _.ensureCapacity(data.len)?;
        @memcpy(_.buf[_.len .. _.len + data.len], data);
        _.len += data.len;
    }

    fn writeByte(*_, b: u8): void ! SchemaError {
        _.ensureCapacity(1)?;
        _.buf[_.len] = b;
        _.len += 1;
    }

    fn writeBool(*_, v: bool): void ! SchemaError {
        return _.writeByte(if (v) u8(1) else u8(0));
    }

    fn writeI8(*_, v: i8): void ! SchemaError {
        const raw: u8 = @bitCast(v);
        return _.writeByte(raw);
    }

    fn writeU16(*_, v: u16): void ! SchemaError {
        _.ensureCapacity(2)?;
        _.buf[_.len] = (v & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 8) & 0xFF) as u8;
        _.len += 2;
    }

    fn writeI16(*_, v: i16): void ! SchemaError {
        const raw: u16 = @bitCast(v);
        return _.writeU16(raw);
    }

    fn writeU32(*_, v: u32): void ! SchemaError {
        _.ensureCapacity(4)?;
        _.buf[_.len] = (v & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.len + 3] = ((v >> 24) & 0xFF) as u8;
        _.len += 4;
    }

    fn writeI32(*_, v: i32): void ! SchemaError {
        const raw: u32 = @bitCast(v);
        return _.writeU32(raw);
    }

    fn writeU64(*_, v: u64): void ! SchemaError {
        _.writeU32((v & 0xFFFFFFFF) as u32)?;
        return _.writeU32(((v >> 32) & 0xFFFFFFFF) as u32);
    }

    fn writeI64(*_, v: i64): void ! SchemaError {
        const raw: u64 = @bitCast(v);
        return _.writeU64(raw);
    }

    fn writeInt(*_, comptime T: type, v: T): void ! SchemaError {
        return switch (comptime T) {
            u8 => _.writeByte(v),
            i8 => _.writeI8(v),
            u16 => _.writeU16(v),
            i16 => _.writeI16(v),
            u32 => _.writeU32(v),
            i32 => _.writeI32(v),
            u64 => _.writeU64(v),
            i64 => _.writeI64(v),
            else => @compileError("Unsupported integer type for SchemaWriter.writeInt"),
        };
    }

    /// 写入带 u32 长度前缀的字节数组
    fn writeByteArray(*_, data: []const u8): void ! SchemaError {
        _.writeU32(data.len as u32)?;
        return _.writeBytes(data);
    }

    /// 写入带 u32 长度前缀的字符串
    fn writeString(*_, s: string): void ! SchemaError {
        return _.writeByteArray(s);
    }

    /// 写入 1 字节字段 ID
    fn writeFieldId(*_, id: u8): void ! SchemaError {
        return _.writeByte(id);
    }

    /// 写入记录结束哨兵字节（0）
    fn endMessage(*_): void ! SchemaError {
        return _.writeByte(END_OF_MESSAGE);
    }

    /// 泛型枚举编码：按枚举底层整数类型写入标签值
    fn writeEnum(*_, comptime Enum: type, val: Enum): void ! SchemaError {
        const tag_type = @typeInfo(Enum).@"enum".tag_type;
        const raw: tag_type = @intFromEnum(val);
        return _.writeInt(tag_type, raw);
    }

    ~init() {
        if (_.cap > 0) {
            _.allocator.free(_.buf);
            _.buf = [];
            _.len = 0;
            _.cap = 0;
        }
    }
}

/// 零堆分配的固定缓冲区写入器（超出容量返回 `err.BufferTooSmall`）
pub struct FixedSchemaWriter {
    buf: []u8,
    pos: usize = 0,

    static fn init(buf: []u8): FixedSchemaWriter {
        return FixedSchemaWriter{
            buf: buf,
            pos: 0,
        };
    }

    fn bytes(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn written(): usize {
        return _.pos;
    }

    fn writeBytes(*_, data: []const u8): void ! SchemaError {
        if (data.len > _.buf.len - _.pos) {
            return err.BufferTooSmall;
        }
        if (data.len > 0) {
            @memcpy(_.buf[_.pos .. _.pos + data.len], data);
            _.pos += data.len;
        }
    }

    fn writeByte(*_, b: u8): void ! SchemaError {
        if (_.pos >= _.buf.len) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn writeU32(*_, v: u32): void ! SchemaError {
        if (usize(4) > _.buf.len - _.pos) {
            return err.BufferTooSmall;
        }
        _.buf[_.pos] = (v & 0xFF) as u8;
        _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        _.pos += 4;
    }

    fn writeString(*_, s: string): void ! SchemaError {
        _.writeU32(s.len as u32)?;
        return _.writeBytes(s);
    }

    fn writeFieldId(*_, id: u8): void ! SchemaError {
        return _.writeByte(id);
    }

    fn endMessage(*_): void ! SchemaError {
        return _.writeByte(END_OF_MESSAGE);
    }
}

/// 紧凑双 32 位无符号整型（吸收自 Bun `AnalyticsUint64`，保证 4 字节对齐与跨平台序列化一致性）
pub struct CompactUint64 {
    low: u32 = 0,
    high: u32 = 0,

    static fn fromU64(v: u64): CompactUint64 {
        return CompactUint64{
            low: (v & 0xFFFFFFFF) as u32,
            high: ((v >> 32) & 0xFFFFFFFF) as u32,
        };
    }

    fn toU64(): u64 {
        return (_.low as u64) | ((_.high as u64) << 32);
    }

    fn encode(writer: *SchemaWriter): void ! SchemaError {
        writer.writeU32(_.low)?;
        return writer.writeU32(_.high);
    }

    static fn decode(reader: *SchemaReader): CompactUint64 ! SchemaError {
        const low = reader.readU32()?;
        const high = reader.readU32()?;
        return CompactUint64{
            low: low,
            high: high,
        };
    }
}
