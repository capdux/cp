// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/RowDescription.cp | Synced: 2026-11-22T20:38:57Z
// Pure-CP zero-allocation PostgreSQL v3.0 `RowDescription` ('T') zero-copy decoder, iterator, and streaming encoder.

/// PostgreSQL Backend `RowDescription` 报文类型字节（`'T' = 0x54`）
pub const PG_TAG_ROW_DESCRIPTION: u8 = 'T';

/// `RowDescription` 固定帧头字节数（1B `'T'` + 4B len + 2B field_count = 7B）
pub const PG_ROW_DESC_HEADER_LEN: usize = 7;

/// 每个字段在 NUL 终止名称之后的固定元数据字节数
/// （4B table_oid + 2B column_attr + 4B type_oid + 2B type_size + 4B type_modifier + 2B format_code = 18B）
pub const PG_ROW_DESC_FIELD_META_LEN: usize = 18;

/// `RowDescription` 编解码与边界校验错误集
pub err PgRowDescError {
    InvalidTag,
    InvalidLength,
    InvalidFormatCode,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
    BufferTooSmall,
    TooManyFields,
}

/// PostgreSQL 列传输格式码（`0 = Text`, `1 = Binary`）
pub enum PgFormatCode: i16 {
    Text = 0,
    Binary = 1,
}

/// 安全将 16 位格式码映射为 `PgFormatCode`，非法值返回 `null`
pub fn parseFormatCode(raw: i16): PgFormatCode? {
    if (raw == 0) return PgFormatCode.Text;
    if (raw == 1) return PgFormatCode.Binary;
    return null;
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    const b0: u16 = @as(u16, buf[off]);
    const b1: u16 = @as(u16, buf[off + 1]);
    return (b0 << 8) | b1;
}

fn readBeI16(buf: []const u8, off: usize): i16 {
    return @bitCast(readBeU16(buf, off));
}

fn writeBeU16(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 1] = @as(u8, @intCast(v & 0xFF));
}

fn writeBeI16(out: []u8, off: usize, v: i16) {
    const raw: u16 = @bitCast(v);
    writeBeU16(out, off, raw);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readBeI32(buf: []const u8, off: usize): i32 {
    return @bitCast(readBeU32(buf, off));
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

fn writeBeI32(out: []u8, off: usize, v: i32) {
    const raw: u32 = @bitCast(v);
    writeBeU32(out, off, raw);
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

fn containsNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

/// 单列 `RowDescription` 元数据描述符（零拷贝切片 `name`）
pub struct PgRowFieldDesc {
    index: u16 = 0,
    name: []const u8 = "",
    table_oid: u32 = 0,
    column_attr: i16 = 0,
    type_oid: u32 = 0,
    type_size: i16 = -1,
    type_modifier: i32 = -1,
    format: PgFormatCode = PgFormatCode.Text,

    fn isBinary(): bool {
        return _.format == PgFormatCode.Binary;
    }

    fn isText(): bool {
        return _.format == PgFormatCode.Text;
    }

    /// 指示 PostgreSQL 数据类型是否为变长类型（`pg_type.typlen < 0`）
    fn isVariableLength(): bool {
        return _.type_size < 0;
    }

    fn eql(other: PgRowFieldDesc): bool {
        return _.index == other.index &&
            bytesEql(_.name, other.name) &&
            _.table_oid == other.table_oid &&
            _.column_attr == other.column_attr &&
            _.type_oid == other.type_oid &&
            _.type_size == other.type_size &&
            _.type_modifier == other.type_modifier &&
            _.format == other.format;
    }
}

/// `RowDescription` 顺序零拷贝列描述符迭代器
pub struct PgRowDescIter {
    payload: []const u8 = "",
    offset: usize = 0,
    index: u16 = 0,
    total: u16 = 0,

    fn remaining(): u16 {
        return _.total - _.index;
    }

    fn next(*_): PgRowFieldDesc? {
        if (_.index >= _.total) {
            return null;
        }
        let nul_pos = _.offset;
        while (nul_pos < _.payload.len && _.payload[nul_pos] != 0) {
            nul_pos += 1;
        }
        if (nul_pos >= _.payload.len) {
            return null;
        }
        const field_name = _.payload[_.offset..nul_pos];
        const meta = nul_pos + 1;
        if (_.payload.len - meta < PG_ROW_DESC_FIELD_META_LEN) {
            return null;
        }

        const table_oid = readBeU32(_.payload, meta);
        const col_attr = readBeI16(_.payload, meta + 4);
        const type_oid = readBeU32(_.payload, meta + 6);
        const type_size = readBeI16(_.payload, meta + 10);
        const type_mod = readBeI32(_.payload, meta + 12);
        const fmt_raw = readBeI16(_.payload, meta + 16);
        const fmt = parseFormatCode(fmt_raw) ?? PgFormatCode.Text;

        _.offset = meta + PG_ROW_DESC_FIELD_META_LEN;
        const cur_idx = _.index;
        _.index += 1;

        return PgRowFieldDesc{
            index: cur_idx,
            name: field_name,
            table_oid: table_oid,
            column_attr: col_attr,
            type_oid: type_oid,
            type_size: type_size,
            type_modifier: type_mod,
            format: fmt,
        };
    }
}

/// PostgreSQL v3.0 Backend `RowDescription` (`'T'`) 零拷贝已校验元数据视图
///
/// 线格式：`['T'][len: i32 BE][field_count: u16 BE]([name\0][18B meta])*`
/// 构造时单遍完成全部字段的 NUL 终止、18B 元数据边界与 `format_code`（0/1）校验，零堆分配。
pub struct PgRowDescView {
    fields_payload: []const u8 = "",
    field_count: u16 = 0,

    /// 零拷贝解码并校验完整 `'T'` 线协议帧
    static fn decodeFrame(frame: []const u8): PgRowDescView! {
        if (frame.len < PG_ROW_DESC_HEADER_LEN) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_ROW_DESCRIPTION) {
            return err.InvalidTag;
        }
        const declared_len = @as(usize, readBeU32(frame, 1));
        if (declared_len < 6) {
            return err.InvalidLength;
        }
        const expected_total = 1 + declared_len;
        if (frame.len < expected_total) {
            return err.Truncated;
        }
        if (frame.len > expected_total) {
            return err.TrailingBytes;
        }
        return PgRowDescView.decodePayload(frame[5..expected_total]);
    }

    /// 零拷贝解码不含 5 字节帧头的 `RowDescription` 载荷（含 2B 字段计数 + 各字段描述）
    static fn decodePayload(payload: []const u8): PgRowDescView! {
        if (payload.len < 2) {
            return err.Truncated;
        }
        const count = readBeU16(payload, 0);
        const fields = payload[2..payload.len];

        let pos: usize = 0;
        let i: u16 = 0;
        while (i < count) {
            let nul_pos = pos;
            while (nul_pos < fields.len && fields[nul_pos] != 0) {
                nul_pos += 1;
            }
            if (nul_pos >= fields.len) {
                return err.MissingNulTerminator;
            }
            const meta = nul_pos + 1;
            if (fields.len - meta < PG_ROW_DESC_FIELD_META_LEN) {
                return err.Truncated;
            }
            const fmt_raw = readBeI16(fields, meta + 16);
            if (parseFormatCode(fmt_raw) == null) {
                return err.InvalidFormatCode;
            }
            pos = meta + PG_ROW_DESC_FIELD_META_LEN;
            i += 1;
        }
        if (pos != fields.len) {
            return err.TrailingBytes;
        }
        return PgRowDescView{
            fields_payload: fields,
            field_count: count,
        };
    }

    fn fieldCount(): u16 {
        return _.field_count;
    }

    fn isEmpty(): bool {
        return _.field_count == 0;
    }

    fn iter(): PgRowDescIter {
        return PgRowDescIter{
            payload: _.fields_payload,
            offset: 0,
            index: 0,
            total: _.field_count,
        };
    }

    /// 按 0-based 列索引获取字段描述，越界返回 `null`
    fn fieldAt(target: u16): PgRowFieldDesc? {
        if (target >= _.field_count) {
            return null;
        }
        let it = _.iter();
        while (f := it.next()) {
            if (f.index == target) {
                return f;
            }
        }
        return null;
    }

    /// 按列名查找首个匹配字段描述（精确字节匹配）
    fn findByName(target_name: []const u8): PgRowFieldDesc? {
        let it = _.iter();
        while (f := it.next()) {
            if (bytesEql(f.name, target_name)) {
                return f;
            }
        }
        return null;
    }

    /// 按列名查找首个匹配列的 0-based 索引
    fn indexOfName(target_name: []const u8): u16? {
        if (f := _.findByName(target_name)) {
            return f.index;
        }
        return null;
    }

    /// 判断所有列是否均使用文本格式码（空集返回 `true`）
    fn allText(): bool {
        let it = _.iter();
        while (f := it.next()) {
            if (!f.isText()) {
                return false;
            }
        }
        return true;
    }

    /// 判断所有列是否均使用二进制格式码（空集返回 `true`）
    fn allBinary(): bool {
        let it = _.iter();
        while (f := it.next()) {
            if (!f.isBinary()) {
                return false;
            }
        }
        return true;
    }
}

/// PostgreSQL v3.0 `RowDescription` (`'T'`) 零堆分配流式帧编码器
pub struct PgRowDescEncoder {
    buf: []u8,
    pos: usize = PG_ROW_DESC_HEADER_LEN,
    field_count: u16 = 0,

    static fn init(buf: []u8): PgRowDescEncoder! {
        if (buf.len < PG_ROW_DESC_HEADER_LEN) {
            return err.BufferTooSmall;
        }
        return PgRowDescEncoder{
            buf: buf,
            pos: PG_ROW_DESC_HEADER_LEN,
            field_count: 0,
        };
    }

    /// 追加一条完整字段元数据，返回新字段 0-based 索引（拒绝列名包含内嵌 NUL）
    fn writeField(*_, field: PgRowFieldDesc): u16! {
        if (_.field_count == 0xFFFF) {
            return err.TooManyFields;
        }
        if (containsNul(field.name)) {
            return err.EmbeddedNul;
        }
        const need = field.name.len + 1 + PG_ROW_DESC_FIELD_META_LEN;
        if (_.buf.len - _.pos < need) {
            return err.BufferTooSmall;
        }
        copyBytes(_.buf, _.pos, field.name);
        _.pos += field.name.len;
        _.buf[_.pos] = 0;
        _.pos += 1;

        writeBeU32(_.buf, _.pos, field.table_oid);
        writeBeI16(_.buf, _.pos + 4, field.column_attr);
        writeBeU32(_.buf, _.pos + 6, field.type_oid);
        writeBeI16(_.buf, _.pos + 10, field.type_size);
        writeBeI32(_.buf, _.pos + 12, field.type_modifier);
        writeBeI16(_.buf, _.pos + 16, @as(i16, @intFromEnum(field.format)));
        _.pos += PG_ROW_DESC_FIELD_META_LEN;

        const idx = _.field_count;
        _.field_count += 1;
        return idx;
    }

    /// 便捷追加一条标准文本格式列（`table_oid=0, column_attr=0, type_size=-1, type_modifier=-1, format=Text`）
    fn writeSimpleText(*_, col_name: []const u8, col_type_oid: u32): u16! {
        return _.writeField(PgRowFieldDesc{
            name: col_name,
            type_oid: col_type_oid,
            format: PgFormatCode.Text,
        });
    }

    fn fieldCount(): u16 {
        return _.field_count;
    }

    fn writtenLen(): usize {
        return _.pos;
    }

    /// 回填 `'T'` 帧头、4B big-endian 长度与 2B 字段数，返回完整帧总字节数
    fn finish(*_): usize! {
        const len_field = _.pos - 1;
        if (len_field > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        _.buf[0] = PG_TAG_ROW_DESCRIPTION;
        writeBeU32(_.buf, 1, @as(u32, @intCast(len_field)));
        writeBeU16(_.buf, 5, _.field_count);
        return _.pos;
    }
}
