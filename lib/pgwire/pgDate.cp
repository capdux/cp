// Source: Absorbed from /data/dev/bun/src/sql_jsc/postgres/types/date.cp | Synced: 2026-11-22T21:38:14Z
// Pure-CP zero-allocation PostgreSQL binary DATE / TIMESTAMP / TIMESTAMPTZ wire codec.
// Converts between PostgreSQL 2000-01-01 epoch (big-endian i32 days / i64 microseconds)
// and Unix 1970-01-01 epoch milliseconds with overflow and +/-infinity guards.

/// PostgreSQL `DATE` 类型 OID（4 字节大端有符号天数，相对 `2000-01-01`）
pub const PG_OID_DATE: i32 = 1082;

/// PostgreSQL `TIMESTAMP`（无时区）类型 OID（8 字节大端有符号微秒，相对 `2000-01-01 00:00:00`）
pub const PG_OID_TIMESTAMP: i32 = 1114;

/// PostgreSQL `TIMESTAMPTZ`（带时区）类型 OID（8 字节大端有符号微秒，相对 `2000-01-01 00:00:00 UTC`）
pub const PG_OID_TIMESTAMPTZ: i32 = 1184;

/// PostgreSQL 纪元起点 `2000-01-01T00:00:00.000Z` 对应的 Unix 毫秒时间戳
pub const PG_EPOCH_UNIX_MS: i64 = 946684800000;

/// PostgreSQL 纪元起点 `2000-01-01T00:00:00Z` 对应的 Unix 秒时间戳
pub const PG_EPOCH_UNIX_SEC: i64 = 946684800;

/// `1970-01-01` 至 `2000-01-01` 的精确天数（含 7 个闰年）
pub const PG_EPOCH_UNIX_DAYS: i32 = 10957;

/// 每毫秒微秒数
pub const PG_US_PER_MS: i64 = 1000;

/// 每秒微秒数
pub const PG_US_PER_SEC: i64 = 1000000;

/// 每天毫秒数
pub const PG_MS_PER_DAY: i64 = 86400000;

/// PostgreSQL 二进制 `TIMESTAMP`/`TIMESTAMPTZ` 正无穷哨兵值（`INT64_MAX`）
pub const PG_TIMESTAMP_POS_INFINITY: i64 = 9223372036854775807;

/// PostgreSQL 二进制 `TIMESTAMP`/`TIMESTAMPTZ` 负无穷哨兵值（`INT64_MIN`）
pub const PG_TIMESTAMP_NEG_INFINITY: i64 = -9223372036854775807 - 1;

/// PostgreSQL 二进制 `DATE` 正无穷哨兵值（`INT32_MAX`）
pub const PG_DATE_POS_INFINITY: i32 = 2147483647;

/// PostgreSQL 二进制 `DATE` 负无穷哨兵值（`INT32_MIN`）
pub const PG_DATE_NEG_INFINITY: i32 = -2147483647 - 1;

/// PostgreSQL 日期/时间戳二进制编解码错误集
pub err PgDateError {
    InvalidLength,
    BufferTooSmall,
    OutOfRange,
    InfiniteTimestamp,
}

/// PostgreSQL 日期/时间戳数值分类（有限值 / 正无穷 / 负无穷）
pub enum PgTimestampKind {
    Finite,
    PosInfinity,
    NegInfinity,
}

/// 判断给定 PostgreSQL 类型 OID 是否属于日期/时间戳家族（1082 / 1114 / 1184）
pub fn isPgDateTimeOid(oid: i32): bool {
    return oid == PG_OID_DATE || oid == PG_OID_TIMESTAMP || oid == PG_OID_TIMESTAMPTZ;
}

fn readBeU32(bytes: []const u8): u32 {
    return (@as(u32, bytes[0]) << 24) |
        (@as(u32, bytes[1]) << 16) |
        (@as(u32, bytes[2]) << 8) |
        @as(u32, bytes[3]);
}

fn readBeU64(bytes: []const u8): u64 {
    return (@as(u64, bytes[0]) << 56) |
        (@as(u64, bytes[1]) << 48) |
        (@as(u64, bytes[2]) << 40) |
        (@as(u64, bytes[3]) << 32) |
        (@as(u64, bytes[4]) << 24) |
        (@as(u64, bytes[5]) << 16) |
        (@as(u64, bytes[6]) << 8) |
        @as(u64, bytes[7]);
}

fn writeBeU32(out: []u8, v: u32) {
    out[0] = @as(u8, @truncate(v >> 24));
    out[1] = @as(u8, @truncate(v >> 16));
    out[2] = @as(u8, @truncate(v >> 8));
    out[3] = @as(u8, @truncate(v));
}

fn writeBeU64(out: []u8, v: u64) {
    out[0] = @as(u8, @truncate(v >> 56));
    out[1] = @as(u8, @truncate(v >> 48));
    out[2] = @as(u8, @truncate(v >> 40));
    out[3] = @as(u8, @truncate(v >> 32));
    out[4] = @as(u8, @truncate(v >> 24));
    out[5] = @as(u8, @truncate(v >> 16));
    out[6] = @as(u8, @truncate(v >> 8));
    out[7] = @as(u8, @truncate(v));
}

fn floorDivI64(a: i64, b: i64): i64 {
    const q = a / b;
    const r = a % b;
    if (r < 0) {
        return q - 1;
    }
    return q;
}

/// PostgreSQL 二进制 `TIMESTAMP` / `TIMESTAMPTZ`（8 字节大端微秒）编解码器（零堆分配）
pub struct PgTimestampCodec {
    /// 解码 8 字节大端微秒值（相对 `2000-01-01T00:00:00Z`）
    static fn decodeMicros(bytes: []const u8): PgDateError!i64 {
        if (bytes.len != 8) {
            return PgDateError.InvalidLength;
        }
        return @as(i64, @bitCast(readBeU64(bytes)));
    }

    /// 编码相对 `2000-01-01T00:00:00Z` 的微秒值为 8 字节大端序列
    static fn encodeMicros(pg_us: i64, out: []u8): PgDateError!usize {
        if (out.len < 8) {
            return PgDateError.BufferTooSmall;
        }
        writeBeU64(out, @as(u64, @bitCast(pg_us)));
        return 8;
    }

    /// 区分普通有限时间戳与 PostgreSQL `infinity` / `-infinity`
    static fn classifyMicros(pg_us: i64): PgTimestampKind {
        if (pg_us == PG_TIMESTAMP_POS_INFINITY) {
            return PgTimestampKind.PosInfinity;
        }
        if (pg_us == PG_TIMESTAMP_NEG_INFINITY) {
            return PgTimestampKind.NegInfinity;
        }
        return PgTimestampKind.Finite;
    }

    /// 按 Bun `fromBinary` 浮点公式转换为 Unix 毫秒（保留亚毫秒小数精度）
    static fn pgMicrosToUnixMsF64(pg_us: i64): f64 {
        const double_us: f64 = @floatFromInt(pg_us);
        return (double_us / 1000.0) + 946684800000.0;
    }

    /// 按 Bun `toJS` 向下取整语义转换为精确 Unix 整数毫秒（拒绝 `+/-infinity`）
    static fn pgMicrosToUnixMs(pg_us: i64): PgDateError!i64 {
        if (PgTimestampCodec.classifyMicros(pg_us) != PgTimestampKind.Finite) {
            return PgDateError.InfiniteTimestamp;
        }
        return floorDivI64(pg_us, PG_US_PER_MS) + PG_EPOCH_UNIX_MS;
    }

    /// 将 Unix 整数毫秒转换为 PostgreSQL 微秒（带安全范围校验防止 `i64` 溢出）
    static fn unixMsToPgMicros(unix_ms: i64): PgDateError!i64 {
        const min_pg_ms: i64 = -9223372036854775;
        const max_pg_ms: i64 = 9223372036854775;
        if (unix_ms < PG_EPOCH_UNIX_MS + min_pg_ms || unix_ms > PG_EPOCH_UNIX_MS + max_pg_ms) {
            return PgDateError.OutOfRange;
        }
        return (unix_ms - PG_EPOCH_UNIX_MS) * PG_US_PER_MS;
    }

    /// 直接从 8 字节大端二进制载荷解码为 Unix 浮点毫秒（对齐 Bun `date.fromBinary`）
    static fn fromBinaryToUnixMsF64(bytes: []const u8): PgDateError!f64 {
        const pg_us = PgTimestampCodec.decodeMicros(bytes)?;
        if (PgTimestampCodec.classifyMicros(pg_us) != PgTimestampKind.Finite) {
            return PgDateError.InfiniteTimestamp;
        }
        return PgTimestampCodec.pgMicrosToUnixMsF64(pg_us);
    }

    /// 直接从 8 字节大端二进制载荷解码为 Unix 整数毫秒
    static fn fromBinaryToUnixMs(bytes: []const u8): PgDateError!i64 {
        const pg_us = PgTimestampCodec.decodeMicros(bytes)?;
        return PgTimestampCodec.pgMicrosToUnixMs(pg_us);
    }

    /// 将 Unix 整数毫秒编码为 8 字节大端 PostgreSQL `TIMESTAMP`/`TIMESTAMPTZ` 二进制载荷
    static fn encodeUnixMs(unix_ms: i64, out: []u8): PgDateError!usize {
        const pg_us = PgTimestampCodec.unixMsToPgMicros(unix_ms)?;
        return PgTimestampCodec.encodeMicros(pg_us, out);
    }
}

/// PostgreSQL 二进制 `DATE`（OID 1082，4 字节大端天数）编解码器（零堆分配）
pub struct PgDateCodec {
    /// 解码 4 字节大端天数（相对 `2000-01-01`）
    static fn decodeDays(bytes: []const u8): PgDateError!i32 {
        if (bytes.len != 4) {
            return PgDateError.InvalidLength;
        }
        return @as(i32, @bitCast(readBeU32(bytes)));
    }

    /// 编码相对 `2000-01-01` 的天数为 4 字节大端序列
    static fn encodeDays(pg_days: i32, out: []u8): PgDateError!usize {
        if (out.len < 4) {
            return PgDateError.BufferTooSmall;
        }
        writeBeU32(out, @as(u32, @bitCast(pg_days)));
        return 4;
    }

    /// 区分普通有限日期与 PostgreSQL `infinity` / `-infinity`
    static fn classifyDays(pg_days: i32): PgTimestampKind {
        if (pg_days == PG_DATE_POS_INFINITY) {
            return PgTimestampKind.PosInfinity;
        }
        if (pg_days == PG_DATE_NEG_INFINITY) {
            return PgTimestampKind.NegInfinity;
        }
        return PgTimestampKind.Finite;
    }

    /// 将 PostgreSQL 天数转换为当天 `00:00:00.000Z` 对应的 Unix 整数毫秒
    static fn pgDaysToUnixMs(pg_days: i32): PgDateError!i64 {
        if (PgDateCodec.classifyDays(pg_days) != PgTimestampKind.Finite) {
            return PgDateError.InfiniteTimestamp;
        }
        const unix_days: i64 = @as(i64, pg_days) + @as(i64, PG_EPOCH_UNIX_DAYS);
        return unix_days * PG_MS_PER_DAY;
    }

    /// 将 Unix 整数毫秒按 UTC 自然日向下取整转换为 PostgreSQL 天数
    static fn unixMsToPgDays(unix_ms: i64): PgDateError!i32 {
        const unix_days = floorDivI64(unix_ms, PG_MS_PER_DAY);
        const pg_days = unix_days - @as(i64, PG_EPOCH_UNIX_DAYS);
        if (pg_days <= @as(i64, PG_DATE_NEG_INFINITY) || pg_days >= @as(i64, PG_DATE_POS_INFINITY)) {
            return PgDateError.OutOfRange;
        }
        return @as(i32, @intCast(pg_days));
    }

    /// 从 4 字节大端 `DATE` 二进制载荷解码为 Unix 整数毫秒
    static fn fromBinaryToUnixMs(bytes: []const u8): PgDateError!i64 {
        const pg_days = PgDateCodec.decodeDays(bytes)?;
        return PgDateCodec.pgDaysToUnixMs(pg_days);
    }

    /// 将 Unix 整数毫秒编码为 4 字节大端 `DATE` 二进制载荷
    static fn encodeUnixMs(unix_ms: i64, out: []u8): PgDateError!usize {
        const pg_days = PgDateCodec.unixMsToPgDays(unix_ms)?;
        return PgDateCodec.encodeDays(pg_days, out);
    }
}
