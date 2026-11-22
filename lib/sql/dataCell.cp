// Source: Absorbed from /data/dev/bun/src/sql_jsc/shared/SQLDataCell.cp | Synced: 2026-11-22T21:50:12Z
// Pure-CP zero-allocation cross-dialect SQL result cell (`SqlDataCell`) and fixed-capacity row buffer.
// Mirrors Bun's 15-tag SQLDataCellTag ABI, SQLDataCellFlags layout bitset, and typed zero-copy accessors.

/// 单行 `SqlDataRow` 支持的最大单元格数（零堆分配内联上限）
pub const SQL_DATA_CELL_MAX_ROW_COLS: usize = 64;

/// `SqlDataCell` / `SqlDataRow` 类型校验与容量错误集
pub err SqlDataCellError {
    RowFull,
    TypeMismatch,
    IndexOutOfBounds,
}

/// 跨方言 SQL 单元格值标签（逐值对齐 Bun `SQLDataCellTag` 0..14）
pub enum SqlDataCellTag: u8 {
    Null = 0,
    String = 1,
    Float8 = 2,
    Int4 = 3,
    Int8 = 4,
    Bool = 5,
    Date = 6,
    DateWithTimeZone = 7,
    Bytea = 8,
    Json = 9,
    Array = 10,
    TypedArray = 11,
    Raw = 12,
    Uint4 = 13,
    Uint8 = 14,

    /// 返回规范标签名称
    fn tagName(): []const u8 {
        if (_ == SqlDataCellTag.Null) {
            return "null";
        }
        if (_ == SqlDataCellTag.String) {
            return "string";
        }
        if (_ == SqlDataCellTag.Float8) {
            return "float8";
        }
        if (_ == SqlDataCellTag.Int4) {
            return "int4";
        }
        if (_ == SqlDataCellTag.Int8) {
            return "int8";
        }
        if (_ == SqlDataCellTag.Bool) {
            return "bool";
        }
        if (_ == SqlDataCellTag.Date) {
            return "date";
        }
        if (_ == SqlDataCellTag.DateWithTimeZone) {
            return "date_with_time_zone";
        }
        if (_ == SqlDataCellTag.Bytea) {
            return "bytea";
        }
        if (_ == SqlDataCellTag.Json) {
            return "json";
        }
        if (_ == SqlDataCellTag.Array) {
            return "array";
        }
        if (_ == SqlDataCellTag.TypedArray) {
            return "typed_array";
        }
        if (_ == SqlDataCellTag.Raw) {
            return "raw";
        }
        if (_ == SqlDataCellTag.Uint4) {
            return "uint4";
        }
        return "uint8";
    }

    /// 是否为数值类型（`Int4`/`Int8`/`Uint4`/`Uint8`/`Float8`）
    fn isNumeric(): bool {
        return _ == SqlDataCellTag.Int4 ||
            _ == SqlDataCellTag.Int8 ||
            _ == SqlDataCellTag.Uint4 ||
            _ == SqlDataCellTag.Uint8 ||
            _ == SqlDataCellTag.Float8;
    }

    /// 是否为时间/日期类型（`Date`/`DateWithTimeZone`）
    fn isTemporal(): bool {
        return _ == SqlDataCellTag.Date || _ == SqlDataCellTag.DateWithTimeZone;
    }

    /// 是否为二进制字节切片类型（`Bytea`/`Raw`/`TypedArray`）
    fn isBinaryLike(): bool {
        return _ == SqlDataCellTag.Bytea || _ == SqlDataCellTag.Raw || _ == SqlDataCellTag.TypedArray;
    }
}

/// 结果行结构布局标志位（对齐 Bun `@packed(u32) SQLDataCellFlags`）
pub struct SqlDataCellFlags {
    has_indexed_columns: bool = false,
    has_named_columns: bool = false,
    has_duplicate_columns: bool = false,

    /// 打包为 32 位位掩码
    fn toMask(): u32 {
        let m: u32 = 0;
        if (_.has_indexed_columns) {
            m |= 0x01;
        }
        if (_.has_named_columns) {
            m |= 0x02;
        }
        if (_.has_duplicate_columns) {
            m |= 0x04;
        }
        return m;
    }

    /// 从 32 位位掩码恢复标志位
    static fn fromMask(mask: u32): SqlDataCellFlags {
        return SqlDataCellFlags{
            has_indexed_columns: (mask & 0x01) != 0,
            has_named_columns: (mask & 0x02) != 0,
            has_duplicate_columns: (mask & 0x04) != 0,
        };
    }

    /// 是否可使用快速单态对象布局（无重复列且无数字索引列）
    fn canUseFastObjectLayout(): bool {
        return !_.has_duplicate_columns && !_.has_indexed_columns;
    }
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 跨方言零拷贝 SQL 单元格值（对应 Bun `SQLDataCell`）
pub struct SqlDataCell {
    tag: SqlDataCellTag = SqlDataCellTag.Null,
    int_val: i64 = 0,
    uint_val: u64 = 0,
    float_val: f64 = 0.0,
    bool_val: bool = false,
    bytes_val: []const u8 = "",

    static fn nullCell(): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Null };
    }

    static fn fromInt4(v: i32): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Int4, int_val: @as(i64, v) };
    }

    static fn fromInt8(v: i64): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Int8, int_val: v };
    }

    static fn fromUint4(v: u32): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Uint4, uint_val: @as(u64, v) };
    }

    static fn fromUint8(v: u64): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Uint8, uint_val: v };
    }

    static fn fromFloat8(v: f64): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Float8, float_val: v };
    }

    static fn fromBool(v: bool): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Bool, bool_val: v };
    }

    static fn fromString(s: []const u8): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.String, bytes_val: s };
    }

    static fn fromJson(s: []const u8): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Json, bytes_val: s };
    }

    static fn fromBytea(b: []const u8): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Bytea, bytes_val: b };
    }

    static fn fromRaw(b: []const u8): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Raw, bytes_val: b };
    }

    static fn fromDateMs(ms: f64): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.Date, float_val: ms };
    }

    static fn fromDateTzMs(ms: f64): SqlDataCell {
        return SqlDataCell{ tag: SqlDataCellTag.DateWithTimeZone, float_val: ms };
    }

    /// 是否为 SQL `NULL`
    fn isNull(): bool {
        return _.tag == SqlDataCellTag.Null;
    }

    /// 按有符号 64 位整数读取（允许 `Int4`/`Int8`）
    fn asInt64(): SqlDataCellError!i64 {
        if (_.tag == SqlDataCellTag.Int4 || _.tag == SqlDataCellTag.Int8) {
            return _.int_val;
        }
        return SqlDataCellError.TypeMismatch;
    }

    /// 按无符号 64 位整数读取（允许 `Uint4`/`Uint8`）
    fn asUint64(): SqlDataCellError!u64 {
        if (_.tag == SqlDataCellTag.Uint4 || _.tag == SqlDataCellTag.Uint8) {
            return _.uint_val;
        }
        return SqlDataCellError.TypeMismatch;
    }

    /// 按双精度浮点读取（允许 `Float8`/`Date`/`DateWithTimeZone`）
    fn asFloat64(): SqlDataCellError!f64 {
        if (_.tag == SqlDataCellTag.Float8 || _.tag.isTemporal()) {
            return _.float_val;
        }
        return SqlDataCellError.TypeMismatch;
    }

    /// 按布尔值读取
    fn asBool(): SqlDataCellError!bool {
        if (_.tag == SqlDataCellTag.Bool) {
            return _.bool_val;
        }
        return SqlDataCellError.TypeMismatch;
    }

    /// 按零拷贝字节切片读取（允许 `String`/`Json`/`Bytea`/`Raw`）
    fn asSlice(): SqlDataCellError![]const u8 {
        if (_.tag == SqlDataCellTag.String ||
            _.tag == SqlDataCellTag.Json ||
            _.tag == SqlDataCellTag.Bytea ||
            _.tag == SqlDataCellTag.Raw) {
            return _.bytes_val;
        }
        return SqlDataCellError.TypeMismatch;
    }

    /// 比较两个 `SqlDataCell` 是否标签与值完全一致
    fn eql(other: SqlDataCell): bool {
        if (_.tag != other.tag) {
            return false;
        }
        if (_.tag == SqlDataCellTag.Null) {
            return true;
        }
        if (_.tag == SqlDataCellTag.Int4 || _.tag == SqlDataCellTag.Int8) {
            return _.int_val == other.int_val;
        }
        if (_.tag == SqlDataCellTag.Uint4 || _.tag == SqlDataCellTag.Uint8) {
            return _.uint_val == other.uint_val;
        }
        if (_.tag == SqlDataCellTag.Float8 || _.tag.isTemporal()) {
            return _.float_val == other.float_val;
        }
        if (_.tag == SqlDataCellTag.Bool) {
            return _.bool_val == other.bool_val;
        }
        return bytesEql(_.bytes_val, other.bytes_val);
    }
}

/// 零堆分配固定容量 SQL 结果行容器（最多 64 列）
pub struct SqlDataRow {
    cells: [SQL_DATA_CELL_MAX_ROW_COLS]SqlDataCell = [SqlDataCell{}; SQL_DATA_CELL_MAX_ROW_COLS],
    len: usize = 0,
    flags: SqlDataCellFlags = SqlDataCellFlags{},

    static fn init(): SqlDataRow {
        return SqlDataRow{};
    }

    fn push(*_, cell: SqlDataCell): SqlDataCellError!usize {
        if (_.len >= SQL_DATA_CELL_MAX_ROW_COLS) {
            return SqlDataCellError.RowFull;
        }
        _.cells[_.len] = cell;
        _.len += 1;
        return _.len;
    }

    fn get(idx: usize): SqlDataCell? {
        if (idx >= _.len) {
            return null;
        }
        return _.cells[idx];
    }

    fn set(*_, idx: usize, cell: SqlDataCell): SqlDataCellError!bool {
        if (idx >= _.len) {
            return SqlDataCellError.IndexOutOfBounds;
        }
        _.cells[idx] = cell;
        return true;
    }

    fn nullCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.cells[i].isNull()) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    fn clear(*_) {
        _.len = 0;
        _.flags = SqlDataCellFlags{};
    }
}
