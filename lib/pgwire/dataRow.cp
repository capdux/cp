// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/DataRow.cp | Synced: 2026-11-22T20:36:20Z
// Pure-CP zero-allocation PostgreSQL v3.0 `DataRow` ('D') zero-copy decoder, iterator, and streaming encoder.

/// PostgreSQL Backend `DataRow` 报文类型字节（`'D' = 0x44`）
pub const PG_TAG_DATA_ROW: u8 = 'D';

/// `DataRow` 中表示 SQL `NULL` 列值的长度哨兵（`-1` / `0xFFFFFFFF`）
pub const PG_DATA_ROW_NULL_LEN: i32 = -1;

/// `DataRow` 固定帧头字节数（1B `'D'` + 4B len + 2B column_count = 7B）
pub const PG_DATA_ROW_HEADER_LEN: usize = 7;

/// `DataRow` 编解码与边界校验错误集
pub err PgDataRowError {
    InvalidTag,
    InvalidLength,
    InvalidColumnLength,
    Truncated,
    TrailingBytes,
    BufferTooSmall,
    TooManyColumns,
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    const b0: u16 = @as(u16, buf[off]);
    const b1: u16 = @as(u16, buf[off + 1]);
    return (b0 << 8) | b1;
}

fn writeBeU16(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 1] = @as(u8, @intCast(v & 0xFF));
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

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

/// `DataRow` 单列零拷贝视图（精确区分 SQL `NULL` 与空字节切片 `""`）
pub struct PgDataRowCell {
    index: u16 = 0,
    value: ([]const u8)? = null,

    fn isNull(): bool {
        return _.value == null;
    }

    fn isEmpty(): bool {
        if (v := _.value) {
            return v.len == 0;
        }
        return false;
    }

    fn byteLen(): usize {
        if (v := _.value) {
            return v.len;
        }
        return 0;
    }

    fn eql(other: PgDataRowCell): bool {
        if (_.index != other.index) {
            return false;
        }
        if (a := _.value) {
            if (b := other.value) {
                return bytesEql(a, b);
            }
            return false;
        }
        return other.value == null;
    }
}

/// `DataRow` 顺序零拷贝列迭代器
pub struct PgDataRowIter {
    payload: []const u8 = "",
    offset: usize = 0,
    index: u16 = 0,
    total: u16 = 0,

    fn remaining(): u16 {
        return _.total - _.index;
    }

    fn next(*_): PgDataRowCell? {
        if (_.index >= _.total) {
            return null;
        }
        if (_.payload.len - _.offset < 4) {
            return null;
        }
        const raw_len = readBeI32(_.payload, _.offset);
        _.offset += 4;
        const cur_idx = _.index;
        _.index += 1;

        if (raw_len == PG_DATA_ROW_NULL_LEN) {
            return PgDataRowCell{
                index: cur_idx,
                value: null,
            };
        }
        const n = @as(usize, @intCast(raw_len));
        const slice = _.payload[_.offset..(_.offset + n)];
        _.offset += n;
        return PgDataRowCell{
            index: cur_idx,
            value: slice,
        };
    }
}

/// PostgreSQL v3.0 Backend `DataRow` (`'D'`) 零拷贝已校验行视图
///
/// 线格式：`['D'][len: i32 BE][col_count: u16 BE]([col_len: i32 BE][col_bytes?])*`
/// 构造时单遍完成全列边界与长度校验，保证后续迭代与随机访问 100% 安全且零堆分配。
pub struct PgDataRowView {
    cells_payload: []const u8 = "",
    col_count: u16 = 0,

    /// 零拷贝解码并校验完整 `'D'` 线协议帧
    static fn decodeFrame(frame: []const u8): PgDataRowView! {
        if (frame.len < PG_DATA_ROW_HEADER_LEN) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_DATA_ROW) {
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
        return PgDataRowView.decodePayload(frame[5..expected_total]);
    }

    /// 零拷贝解码不含 5 字节帧头的 `DataRow` 载荷（含 2B 列计数 + 各列长度与数据）
    static fn decodePayload(payload: []const u8): PgDataRowView! {
        if (payload.len < 2) {
            return err.Truncated;
        }
        const count = readBeU16(payload, 0);
        const cells = payload[2..payload.len];

        // 单遍全量边界校验，确保每列长度合法且无截断或尾随垃圾字节
        let pos: usize = 0;
        let i: u16 = 0;
        while (i < count) {
            if (cells.len - pos < 4) {
                return err.Truncated;
            }
            const col_len = readBeI32(cells, pos);
            pos += 4;
            if (col_len == PG_DATA_ROW_NULL_LEN) {
                i += 1;
                continue;
            }
            if (col_len < 0) {
                return err.InvalidColumnLength;
            }
            const ulen = @as(usize, @intCast(col_len));
            if (cells.len - pos < ulen) {
                return err.Truncated;
            }
            pos += ulen;
            i += 1;
        }
        if (pos != cells.len) {
            return err.TrailingBytes;
        }
        return PgDataRowView{
            cells_payload: cells,
            col_count: count,
        };
    }

    fn columnCount(): u16 {
        return _.col_count;
    }

    fn isEmpty(): bool {
        return _.col_count == 0;
    }

    fn iter(): PgDataRowIter {
        return PgDataRowIter{
            payload: _.cells_payload,
            offset: 0,
            index: 0,
            total: _.col_count,
        };
    }

    /// 按 0-based 列索引查找单元格，越界返回 `null`
    fn columnAt(target: u16): PgDataRowCell? {
        if (target >= _.col_count) {
            return null;
        }
        let it = _.iter();
        while (cell := it.next()) {
            if (cell.index == target) {
                return cell;
            }
        }
        return null;
    }

    /// 统计当前行中 SQL `NULL` 列数量
    fn nullCount(): u16 {
        let nulls: u16 = 0;
        let it = _.iter();
        while (cell := it.next()) {
            if (cell.isNull()) {
                nulls += 1;
            }
        }
        return nulls;
    }

    /// 统计当前行中非 `NULL` 列数量
    fn nonNullCount(): u16 {
        return _.col_count - _.nullCount();
    }
}

/// PostgreSQL v3.0 `DataRow` (`'D'`) 零堆分配流式帧编码器
///
/// 写入调用方提供的外部 `[]u8` 缓冲区，`.finish()` 自动回填 `'D'` 帧头、总长度与列数。
pub struct PgDataRowEncoder {
    buf: []u8,
    pos: usize = PG_DATA_ROW_HEADER_LEN,
    col_count: u16 = 0,

    static fn init(buf: []u8): PgDataRowEncoder! {
        if (buf.len < PG_DATA_ROW_HEADER_LEN) {
            return err.BufferTooSmall;
        }
        return PgDataRowEncoder{
            buf: buf,
            pos: PG_DATA_ROW_HEADER_LEN,
            col_count: 0,
        };
    }

    /// 追加一个 SQL `NULL` 列（编码为 `-1` / `0xFFFFFFFF`），返回新列索引
    fn writeNull(*_): u16! {
        if (_.col_count == 0xFFFF) {
            return err.TooManyColumns;
        }
        if (_.buf.len - _.pos < 4) {
            return err.BufferTooSmall;
        }
        writeBeI32(_.buf, _.pos, PG_DATA_ROW_NULL_LEN);
        _.pos += 4;
        const idx = _.col_count;
        _.col_count += 1;
        return idx;
    }

    /// 追加一个非 `NULL` 字节列（含空切片 `""`），返回新列索引
    fn writeBytes(*_, data: []const u8): u16! {
        if (_.col_count == 0xFFFF) {
            return err.TooManyColumns;
        }
        if (data.len > 0x7FFFFFFF) {
            return err.InvalidColumnLength;
        }
        if (_.buf.len - _.pos < 4 + data.len) {
            return err.BufferTooSmall;
        }
        writeBeI32(_.buf, _.pos, @as(i32, @intCast(data.len)));
        _.pos += 4;
        copyBytes(_.buf, _.pos, data);
        _.pos += data.len;
        const idx = _.col_count;
        _.col_count += 1;
        return idx;
    }

    /// 按可选切片追加列：`null` 写 SQL `NULL`，非空写字节数据
    fn writeCell(*_, val: ([]const u8)?): u16! {
        if (v := val) {
            return _.writeBytes(v);
        }
        return _.writeNull();
    }

    fn columnCount(): u16 {
        return _.col_count;
    }

    fn writtenLen(): usize {
        return _.pos;
    }

    /// 回填 `'D'` 帧头、4B big-endian 长度与 2B 列数，返回完整帧字节长度
    fn finish(*_): usize! {
        const len_field = _.pos - 1;
        if (len_field > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        _.buf[0] = PG_TAG_DATA_ROW;
        writeBeU32(_.buf, 1, @as(u32, @intCast(len_field)));
        writeBeU16(_.buf, 5, _.col_count);
        return _.pos;
    }
}
