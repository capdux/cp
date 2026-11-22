// Source: Absorbed from /data/dev/bun/src/sql_jsc/mysql/protocol/DecodeBinaryValue.cp | Synced: 2026-11-22T21:40:31Z
// Pure-CP zero-allocation MySQL binary protocol value decoder (`COM_STMT_EXECUTE` result rows).
// Decodes little-endian integers (TINY/SHORT/INT24/LONG/LONGLONG), IEEE-754 FLOAT/DOUBLE,
// binary DATE/DATETIME/TIMESTAMP/TIME structs, length-encoded strings/blobs, and row NULL bitmaps.

/// MySQL `binary` 字符集编号（`character_set == 63` 表示原始二进制字节流）
pub const MYSQL_BINARY_CHARSET: u16 = 63;

/// MySQL 二进制结果集行包首字节标识（固定 `0x00`）
pub const MYSQL_BINARY_ROW_HEADER: u8 = 0x00;

/// MySQL 二进制值解码错误集
pub err MySqlBinaryValueError {
    Truncated,
    InvalidLength,
    InvalidLenEncInt,
    UnsupportedFieldType,
    BufferTooSmall,
}

/// MySQL 二进制 `DATE` / `DATETIME` / `TIMESTAMP` 结构（长度 0 / 4 / 7 / 11）
pub struct MySqlBinaryDateTime {
    year: u16 = 0,
    month: u8 = 0,
    day: u8 = 0,
    hour: u8 = 0,
    minute: u8 = 0,
    second: u8 = 0,
    microsecond: u32 = 0,

    /// 格式化为 `"YYYY-MM-DD HH:MM:SS"`（若 `microsecond > 0` 追加 `".UUUUUU"`），零堆分配写入 `out`
    fn formatIso(out: []u8): MySqlBinaryValueError!usize {
        const needed: usize = if (_.microsecond > 0) 26 else 19;
        if (out.len < needed) {
            return MySqlBinaryValueError.BufferTooSmall;
        }
        writeDec4(out, 0, _.year);
        out[4] = '-';
        writeDec2(out, 5, _.month);
        out[7] = '-';
        writeDec2(out, 8, _.day);
        out[10] = ' ';
        writeDec2(out, 11, _.hour);
        out[13] = ':';
        writeDec2(out, 14, _.minute);
        out[16] = ':';
        writeDec2(out, 17, _.second);
        if (_.microsecond > 0) {
            out[19] = '.';
            writeDec6(out, 20, _.microsecond);
            return 26;
        }
        return 19;
    }
}

/// MySQL 二进制 `TIME` 结构（长度 0 / 8 / 12，支持正负号与跨天累计小时）
pub struct MySqlBinaryTime {
    negative: bool = false,
    days: u32 = 0,
    hours: u8 = 0,
    minutes: u8 = 0,
    seconds: u8 = 0,
    microsecond: u32 = 0,

    /// 返回总小时数（`days * 24 + hours`，对齐 Bun `DecodeBinaryValue`）
    fn totalHours(): u64 {
        return (@as(u64, _.days) * 24) + @as(u64, _.hours);
    }

    /// 格式化为 `[-]HH:MM:SS`（总小时 > 99 时按 3+ 位输出，对齐 Bun 语义），零堆分配写入 `out`
    fn formatHms(out: []u8): MySqlBinaryValueError!usize {
        let pos: usize = 0;
        if (_.negative) {
            if (pos >= out.len) {
                return MySqlBinaryValueError.BufferTooSmall;
            }
            out[pos] = '-';
            pos += 1;
        }
        const total_h = _.totalHours();
        if (total_h < 100) {
            if (pos + 8 > out.len) {
                return MySqlBinaryValueError.BufferTooSmall;
            }
            writeDec2(out, pos, @as(u8, @intCast(total_h)));
            pos += 2;
        } else {
            pos = writeDecU64Min3(out, pos, total_h)?;
            if (pos + 6 > out.len) {
                return MySqlBinaryValueError.BufferTooSmall;
            }
        }
        out[pos] = ':';
        writeDec2(out, pos + 1, _.minutes);
        out[pos + 3] = ':';
        writeDec2(out, pos + 4, _.seconds);
        return pos + 6;
    }
}

/// 解码后的 MySQL 二进制单元格值标签
pub enum MySqlBinaryValueKind {
    Null,
    Int64,
    Uint64,
    Float64,
    DateTime,
    Time,
    Text,
    Bytes,
}

/// 解码后的 MySQL 二进制值及其在字节流中消耗的长度（零拷贝引用字符串/BLOB 切片）
pub struct MySqlDecodedBinaryValue {
    kind: MySqlBinaryValueKind = MySqlBinaryValueKind.Null,
    int_val: i64 = 0,
    uint_val: u64 = 0,
    float_val: f64 = 0.0,
    datetime_val: MySqlBinaryDateTime = MySqlBinaryDateTime{},
    time_val: MySqlBinaryTime = MySqlBinaryTime{},
    slice_val: []const u8 = "",
    consumed: usize = 0,
}

/// 计算 MySQL 二进制结果集行 NULL 位图字节数：`floor((column_count + 7 + 2) / 8)`（含协议规定的 2-bit 偏移）。
pub fn mysqlBinaryRowNullBitmapLen(column_count: usize): usize {
    return (column_count + 7 + 2) / 8;
}

/// 检查二进制结果集行 NULL 位图中第 `col_idx`（0-based）列是否为 NULL（协议规定偏移 +2 位）。
pub fn isBinaryRowColNull(bitmap: []const u8, col_idx: usize): bool {
    const bit_pos = col_idx + 2;
    const byte_idx = bit_pos / 8;
    if (byte_idx >= bitmap.len) {
        return false;
    }
    const bit_shift = @as(u3, @intCast(bit_pos % 8));
    return ((bitmap[byte_idx] >> bit_shift) & 1) != 0;
}

fn readLeU16(src: []const u8): u16 {
    return @as(u16, src[0]) | (@as(u16, src[1]) << 8);
}

fn readLeU24(src: []const u8): u32 {
    return @as(u32, src[0]) | (@as(u32, src[1]) << 8) | (@as(u32, src[2]) << 16);
}

fn readLeU32(src: []const u8): u32 {
    return @as(u32, src[0]) |
        (@as(u32, src[1]) << 8) |
        (@as(u32, src[2]) << 16) |
        (@as(u32, src[3]) << 24);
}

fn readLeU64(src: []const u8): u64 {
    return @as(u64, src[0]) |
        (@as(u64, src[1]) << 8) |
        (@as(u64, src[2]) << 16) |
        (@as(u64, src[3]) << 24) |
        (@as(u64, src[4]) << 32) |
        (@as(u64, src[5]) << 40) |
        (@as(u64, src[6]) << 48) |
        (@as(u64, src[7]) << 56);
}

fn writeDec2(out: []u8, pos: usize, v: u8) {
    out[pos] = @as(u8, @intCast((v / 10) % 10)) + '0';
    out[pos + 1] = @as(u8, @intCast(v % 10)) + '0';
}

fn writeDec4(out: []u8, pos: usize, v: u16) {
    out[pos] = @as(u8, @intCast((v / 1000) % 10)) + '0';
    out[pos + 1] = @as(u8, @intCast((v / 100) % 10)) + '0';
    out[pos + 2] = @as(u8, @intCast((v / 10) % 10)) + '0';
    out[pos + 3] = @as(u8, @intCast(v % 10)) + '0';
}

fn writeDec6(out: []u8, pos: usize, v: u32) {
    out[pos] = @as(u8, @intCast((v / 100000) % 10)) + '0';
    out[pos + 1] = @as(u8, @intCast((v / 10000) % 10)) + '0';
    out[pos + 2] = @as(u8, @intCast((v / 1000) % 10)) + '0';
    out[pos + 3] = @as(u8, @intCast((v / 100) % 10)) + '0';
    out[pos + 4] = @as(u8, @intCast((v / 10) % 10)) + '0';
    out[pos + 5] = @as(u8, @intCast(v % 10)) + '0';
}

fn writeDecU64Min3(out: []u8, pos: usize, val: u64): MySqlBinaryValueError!usize {
    let digits: [20]u8 = [0; 20];
    let count: usize = 0;
    let v = val;
    while (v > 0) {
        digits[count] = @as(u8, @intCast(v % 10)) + '0';
        v /= 10;
        count += 1;
    }
    if (pos + count > out.len) {
        return MySqlBinaryValueError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < count) {
        out[pos + i] = digits[count - 1 - i];
        i += 1;
    }
    return pos + count;
}

struct LenEncResult {
    value: u64,
    consumed: usize,
}

fn readLenEncInt(src: []const u8): MySqlBinaryValueError!LenEncResult {
    if (src.len == 0) {
        return MySqlBinaryValueError.Truncated;
    }
    const first = src[0];
    if (first < 0xFB) {
        return LenEncResult{ value: @as(u64, first), consumed: 1 };
    }
    if (first == 0xFC) {
        if (src.len < 3) {
            return MySqlBinaryValueError.Truncated;
        }
        return LenEncResult{ value: @as(u64, readLeU16(src[1..3])), consumed: 3 };
    }
    if (first == 0xFD) {
        if (src.len < 4) {
            return MySqlBinaryValueError.Truncated;
        }
        return LenEncResult{ value: @as(u64, readLeU24(src[1..4])), consumed: 4 };
    }
    if (first == 0xFE) {
        if (src.len < 9) {
            return MySqlBinaryValueError.Truncated;
        }
        return LenEncResult{ value: readLeU64(src[1..9]), consumed: 9 };
    }
    return MySqlBinaryValueError.InvalidLenEncInt;
}

/// 从零拷贝字节切片解码单列 MySQL 二进制协议值（`field_type` 为 MySQL 线协议 1 字节类型码）。
pub fn decodeMySqlBinaryValue(
    src: []const u8,
    field_type: u8,
    is_unsigned: bool,
    is_binary: bool,
    character_set: u16,
): MySqlBinaryValueError!MySqlDecodedBinaryValue {
    // MYSQL_TYPE_NULL (0x06)
    if (field_type == 0x06) {
        return MySqlDecodedBinaryValue{ kind: MySqlBinaryValueKind.Null, consumed: 0 };
    }

    // MYSQL_TYPE_TINY (0x01)
    if (field_type == 0x01) {
        if (src.len < 1) {
            return MySqlBinaryValueError.Truncated;
        }
        if (is_unsigned) {
            return MySqlDecodedBinaryValue{
                kind: MySqlBinaryValueKind.Uint64,
                uint_val: @as(u64, src[0]),
                consumed: 1,
            };
        }
        const sv: i8 = @bitCast(src[0]);
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Int64,
            int_val: @as(i64, sv),
            consumed: 1,
        };
    }

    // MYSQL_TYPE_SHORT (0x02) & MYSQL_TYPE_YEAR (0x0D)
    if (field_type == 0x02 || field_type == 0x0D) {
        if (src.len < 2) {
            return MySqlBinaryValueError.Truncated;
        }
        const raw = readLeU16(src[0..2]);
        if (is_unsigned || field_type == 0x0D) {
            return MySqlDecodedBinaryValue{
                kind: MySqlBinaryValueKind.Uint64,
                uint_val: @as(u64, raw),
                consumed: 2,
            };
        }
        const sv: i16 = @bitCast(raw);
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Int64,
            int_val: @as(i64, sv),
            consumed: 2,
        };
    }

    // MYSQL_TYPE_INT24 (0x09)
    if (field_type == 0x09) {
        if (src.len < 3) {
            return MySqlBinaryValueError.Truncated;
        }
        const raw = readLeU24(src[0..3]);
        if (is_unsigned) {
            return MySqlDecodedBinaryValue{
                kind: MySqlBinaryValueKind.Uint64,
                uint_val: @as(u64, raw),
                consumed: 3,
            };
        }
        // 24 位补码符号扩展至 i64
        const signed_val: i64 = if ((raw & 0x800000) != 0) {
            @as(i64, @intCast(raw)) - 0x1000000
        } else {
            @as(i64, @intCast(raw))
        };
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Int64,
            int_val: signed_val,
            consumed: 3,
        };
    }

    // MYSQL_TYPE_LONG (0x03)
    if (field_type == 0x03) {
        if (src.len < 4) {
            return MySqlBinaryValueError.Truncated;
        }
        const raw = readLeU32(src[0..4]);
        if (is_unsigned) {
            return MySqlDecodedBinaryValue{
                kind: MySqlBinaryValueKind.Uint64,
                uint_val: @as(u64, raw),
                consumed: 4,
            };
        }
        const sv: i32 = @bitCast(raw);
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Int64,
            int_val: @as(i64, sv),
            consumed: 4,
        };
    }

    // MYSQL_TYPE_LONGLONG (0x08)
    if (field_type == 0x08) {
        if (src.len < 8) {
            return MySqlBinaryValueError.Truncated;
        }
        const raw = readLeU64(src[0..8]);
        if (is_unsigned) {
            return MySqlDecodedBinaryValue{
                kind: MySqlBinaryValueKind.Uint64,
                uint_val: raw,
                consumed: 8,
            };
        }
        const sv: i64 = @bitCast(raw);
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Int64,
            int_val: sv,
            consumed: 8,
        };
    }

    // MYSQL_TYPE_FLOAT (0x04)
    if (field_type == 0x04) {
        if (src.len < 4) {
            return MySqlBinaryValueError.Truncated;
        }
        const f32_val: f32 = @bitCast(readLeU32(src[0..4]));
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Float64,
            float_val: @as(f64, f32_val),
            consumed: 4,
        };
    }

    // MYSQL_TYPE_DOUBLE (0x05)
    if (field_type == 0x05) {
        if (src.len < 8) {
            return MySqlBinaryValueError.Truncated;
        }
        const f64_val: f64 = @bitCast(readLeU64(src[0..8]));
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Float64,
            float_val: f64_val,
            consumed: 8,
        };
    }

    // MYSQL_TYPE_DATE (0x0A), MYSQL_TYPE_DATETIME (0x0C), MYSQL_TYPE_TIMESTAMP (0x07)
    if (field_type == 0x0A || field_type == 0x0C || field_type == 0x07) {
        if (src.len < 1) {
            return MySqlBinaryValueError.Truncated;
        }
        const dlen = @as(usize, src[0]);
        if (dlen != 0 && dlen != 4 && dlen != 7 && dlen != 11) {
            return MySqlBinaryValueError.InvalidLength;
        }
        if (src.len < 1 + dlen) {
            return MySqlBinaryValueError.Truncated;
        }
        let dt = MySqlBinaryDateTime{};
        if (dlen >= 4) {
            dt.year = readLeU16(src[1..3]);
            dt.month = src[3];
            dt.day = src[4];
        }
        if (dlen >= 7) {
            dt.hour = src[5];
            dt.minute = src[6];
            dt.second = src[7];
        }
        if (dlen == 11) {
            dt.microsecond = readLeU32(src[8..12]);
        }
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.DateTime,
            datetime_val: dt,
            consumed: 1 + dlen,
        };
    }

    // MYSQL_TYPE_TIME (0x0B)
    if (field_type == 0x0B) {
        if (src.len < 1) {
            return MySqlBinaryValueError.Truncated;
        }
        const tlen = @as(usize, src[0]);
        if (tlen != 0 && tlen != 8 && tlen != 12) {
            return MySqlBinaryValueError.InvalidLength;
        }
        if (src.len < 1 + tlen) {
            return MySqlBinaryValueError.Truncated;
        }
        let tm = MySqlBinaryTime{};
        if (tlen >= 8) {
            tm.negative = src[1] != 0;
            tm.days = readLeU32(src[2..6]);
            tm.hours = src[6];
            tm.minutes = src[7];
            tm.seconds = src[8];
        }
        if (tlen == 12) {
            tm.microsecond = readLeU32(src[9..13]);
        }
        return MySqlDecodedBinaryValue{
            kind: MySqlBinaryValueKind.Time,
            time_val: tm,
            consumed: 1 + tlen,
        };
    }

    // Length-encoded string / blob / decimal / json / bit family
    if (field_type == 0x00 ||
        field_type == 0x0F ||
        field_type == 0x10 ||
        field_type == 0xF5 ||
        field_type == 0xF6 ||
        field_type == 0xF7 ||
        field_type == 0xF8 ||
        field_type == 0xF9 ||
        field_type == 0xFA ||
        field_type == 0xFB ||
        field_type == 0xFC ||
        field_type == 0xFD ||
        field_type == 0xFE) {
        const le = readLenEncInt(src)?;
        const slen = @as(usize, @intCast(le.value));
        if (src.len < le.consumed + slen) {
            return MySqlBinaryValueError.Truncated;
        }
        const is_raw_bytes = is_binary || character_set == MYSQL_BINARY_CHARSET || field_type == 0x10;
        return MySqlDecodedBinaryValue{
            kind: if (is_raw_bytes) MySqlBinaryValueKind.Bytes else MySqlBinaryValueKind.Text,
            slice_val: src[le.consumed..le.consumed + slen],
            consumed: le.consumed + slen,
        };
    }

    return MySqlBinaryValueError.UnsupportedFieldType;
}
