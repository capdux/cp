// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/RowDescription.cp | Synced: 2026-11-23T01:32:24Z
// ============================================================================
// Pure-CP Zero-Allocation PostgreSQL v3 RowDescription ('T') & FieldDescription Codec
// Absorbed from Bun: src/sql/postgres/protocol/RowDescription.cp & FieldDescription.cp
// ============================================================================

import { PgWireWriter } from "./pgWriter.cp";
import { PgWireReader } from "./pgReader.cp";

/// PostgreSQL `RowDescription` 后端消息类型标签 (`'T' = 0x54`)
pub const PG_ROW_DESCRIPTION_TAG: u8 = 'T';

/// 单帧 `RowDescription` 最大列数上限（PostgreSQL 单表最大 1600 列，设 1664 防异常大包）
pub const PG_MAX_ROW_FIELDS: usize = 1664;

/// `RowDescription` 编解码错误集
pub err PgRowDescError {
    BufferTooSmall,
    InvalidTag,
    InvalidFrame,
    NegativeFieldCount,
    TooManyFields,
    ShortRead,
    UnterminatedFieldName,
    UnknownFormatCode,
}

/// PostgreSQL 列线协议格式码（0 = 文本，1 = 二进制）
pub enum PgFieldFormatCode: i16 {
    text = 0,
    binary = 1,

    static fn fromI16(raw: i16): PgFieldFormatCode! {
        if (raw == 0) {
            return PgFieldFormatCode.text;
        }
        if (raw == 1) {
            return PgFieldFormatCode.binary;
        }
        return err.UnknownFormatCode;
    }

    fn isBinary(): bool {
        return _ == PgFieldFormatCode.binary;
    }
}

/// 单列元数据描述（零拷贝列名切片，严格对齐 PostgreSQL v3 `FieldDescription` 18 字节定长体）
pub struct PgFieldDescription {
    name: []const u8 = "",
    table_oid: i32 = 0,
    column_index: i16 = 0,
    type_oid: i32 = 0,
    type_size: i16 = -1,
    type_modifier: i32 = -1,
    format_code: PgFieldFormatCode = PgFieldFormatCode.text,

    /// 判断该列是否为二进制传输格式
    fn isBinary(): bool {
        return _.format_code.isBinary();
    }

    /// 判断列名是否为纯非负十进制整数索引（对齐 Bun `ColumnIdentifier` 语义，用于 `SELECT 1, 2` 匿名数字列识别）
    fn numericColumnIndex(): u32? {
        if (_.name.len == 0 || _.name.len > 10) {
            return null;
        }
        let val: u64 = 0;
        let i: usize = 0;
        while (i < _.name.len) {
            const ch = _.name[i];
            if (ch < '0' || ch > '9') {
                return null;
            }
            val = val * 10 + @as(u64, ch - '0');
            if (val >= 0xFFFFFFFF) {
                return null;
            }
            i += 1;
        }
        return @intCast(val);
    }
}

/// 零堆分配流式 `RowDescription` 列元数据迭代器
pub struct PgRowDescriptionIter {
    reader: PgWireReader,
    total_fields: usize = 0,
    index: usize = 0,

    /// 返回总列数
    fn count(): usize {
        return _.total_fields;
    }

    /// 返回尚未读取的剩余列数
    fn remainingFields(): usize {
        if (_.index >= _.total_fields) {
            return 0;
        }
        return _.total_fields - _.index;
    }

    /// 零拷贝读取下一列 `PgFieldDescription`；读取完毕返回 `null`
    fn next(*_): PgFieldDescription?! {
        if (_.index >= _.total_fields) {
            return null;
        }
        const col_name = _.reader.readZ() catch {
            return err.UnterminatedFieldName;
        };
        const table_oid = _.reader.readI32() catch {
            return err.ShortRead;
        };
        const col_idx = _.reader.readI16() catch {
            return err.ShortRead;
        };
        const type_oid = _.reader.readI32() catch {
            return err.ShortRead;
        };
        const type_size = _.reader.readI16() catch {
            return err.ShortRead;
        };
        const type_mod = _.reader.readI32() catch {
            return err.ShortRead;
        };
        const raw_fmt = _.reader.readI16() catch {
            return err.ShortRead;
        };
        const fmt = PgFieldFormatCode.fromI16(raw_fmt)?;

        _.index += 1;
        return PgFieldDescription{
            name: col_name,
            table_oid: table_oid,
            column_index: col_idx,
            type_oid: type_oid,
            type_size: type_size,
            type_modifier: type_mod,
            format_code: fmt,
        };
    }
}

/// 编码完整的 PostgreSQL v3 `RowDescription` (`'T'`) 后端帧（零堆分配）
pub fn encodePgRowDescription(
    buf: []u8,
    fields: []const PgFieldDescription,
): []const u8! {
    if (fields.len > PG_MAX_ROW_FIELDS) {
        return err.TooManyFields;
    }
    let w = PgWireWriter.init(buf);
    const patch = w.beginTaggedFrame(PG_ROW_DESCRIPTION_TAG) catch {
        return err.BufferTooSmall;
    };
    * = w.writeI16(@intCast(fields.len)) catch {
        return err.BufferTooSmall;
    };

    let i: usize = 0;
    while (i < fields.len) {
        const f = fields[i];
        i += 1;
        * = w.writeCString(f.name) catch {
            return err.BufferTooSmall;
        };
        * = w.writeI32(f.table_oid) catch {
            return err.BufferTooSmall;
        };
        * = w.writeI16(f.column_index) catch {
            return err.BufferTooSmall;
        };
        * = w.writeI32(f.type_oid) catch {
            return err.BufferTooSmall;
        };
        * = w.writeI16(f.type_size) catch {
            return err.BufferTooSmall;
        };
        * = w.writeI32(f.type_modifier) catch {
            return err.BufferTooSmall;
        };
        * = w.writeI16(@intFromEnum(f.format_code)) catch {
            return err.BufferTooSmall;
        };
    }

    * = w.finishLength(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解析 PostgreSQL `RowDescription` (`'T'`) 完整帧并返回零拷贝列迭代器
pub fn decodePgRowDescription(frame_bytes: []const u8): PgRowDescriptionIter! {
    let r = PgWireReader.init(frame_bytes);
    const frame = r.readTaggedFrame() catch {
        return err.InvalidFrame;
    };
    if (frame.tag != PG_ROW_DESCRIPTION_TAG) {
        return err.InvalidTag;
    }
    let body = PgWireReader.init(frame.payload);
    const raw_count = body.readI16() catch {
        return err.ShortRead;
    };
    if (raw_count < 0) {
        return err.NegativeFieldCount;
    }
    const count: usize = @intCast(raw_count);
    if (count > PG_MAX_ROW_FIELDS) {
        return err.TooManyFields;
    }
    return PgRowDescriptionIter{
        reader: body,
        total_fields: count,
        index: 0,
    };
}
