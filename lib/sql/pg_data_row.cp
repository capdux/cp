// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/DataRow.cp | Synced: 2026-11-22T11:09:58Z
// Ported from Bun: /data/dev/bun/src/sql/postgres/protocol/DataRow.cp & /data/dev/bun/docs/runtime/sql.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation PostgreSQL Backend DataRow ('D') wire encoder & zero-copy streaming decoder:
// 0xffffffff (-1) SQL NULL sentinel handling, empty 0-byte value distinction, strict column-length
// and zero-trailing-byte validation, streaming column iterator (`PgDataRowIter`), and fixed-capacity view.

/// PostgreSQL 后端数据行消息标签 (`'D'` = 0x44)
pub const PG_DATA_ROW_TAG: u8 = 'D';

/// PostgreSQL 线协议 SQL NULL 列长度哨兵值（`-1` 补码 = `0xffff_ffff`，对齐 Bun `DataRow.null_int4`）
pub const PG_DATA_ROW_NULL_LEN: u32 = 0xffff_ffff;

/// `PgDataRowView` 固定容量视图支持的最大列数（超出请使用无上限的 `PgDataRowIter`）
pub const PG_DATA_ROW_MAX_COLS: usize = 64;

/// PostgreSQL DataRow 编解码错误集
pub err PgDataRowError {
    BufferTooSmall,
    TruncatedFrame,
    InvalidTag,
    InvalidMessageLength,
    InvalidColumnLength,
    TooManyColumns,
    ColumnIndexOutOfBounds,
    TrailingBytes,
}

/// 单个 PostgreSQL 数据行列值（零拷贝借用底层线协议缓冲区，精确区分 `NULL` 与空串 `""`）
pub struct PgDataRowColumn {
    is_null: bool = false,
    bytes: []const u8 = "",

    static fn nullCol(): PgDataRowColumn {
        return PgDataRowColumn{ is_null: true, bytes: "" };
    }

    static fn emptyCol(): PgDataRowColumn {
        return PgDataRowColumn{ is_null: false, bytes: "" };
    }

    static fn bytesCol(data: []const u8): PgDataRowColumn {
        return PgDataRowColumn{ is_null: false, bytes: data };
    }
}

/// 零堆分配流式列迭代器（可处理任意列数，单遍扫描零拷贝）
pub struct PgDataRowIter {
    payload: []const u8 = "",
    pos: usize = 0,
    total_columns: u16 = 0,
    current_index: u16 = 0,

    fn remaining(): u16 {
        if (_.current_index >= _.total_columns) {
            return 0;
        }
        return _.total_columns - _.current_index;
    }

    /// 读取下一列；到达行尾时校验无尾随字节并返回 `null`
    fn next(*_): PgDataRowColumn?! {
        if (_.current_index >= _.total_columns) {
            if (_.pos != _.payload.len) {
                return PgDataRowError.TrailingBytes;
            }
            return null;
        }
        if (_.pos + 4 > _.payload.len) {
            return PgDataRowError.TruncatedFrame;
        }
        const raw_len = readBeU32(_.payload, _.pos);
        _.pos += 4;
        _.current_index += 1;

        if (raw_len == PG_DATA_ROW_NULL_LEN) {
            return PgDataRowColumn.nullCol();
        }
        // PostgreSQL 协议中列长度是有符号 int32；除 -1 (NULL) 外任何负值（最高位为 1）均非法
        if ((raw_len & 0x8000_0000) != 0) {
            return PgDataRowError.InvalidColumnLength;
        }
        const col_len = raw_len as usize;
        if (_.pos + col_len > _.payload.len) {
            return PgDataRowError.TruncatedFrame;
        }
        const slice = _.payload[_.pos .. _.pos + col_len];
        _.pos += col_len;
        return PgDataRowColumn.bytesCol(slice);
    }

    /// 按下标随机访问指定列（不修改当前迭代器游标）
    fn columnAt(target_index: u16): PgDataRowColumn! {
        if (target_index >= _.total_columns) {
            return PgDataRowError.ColumnIndexOutOfBounds;
        }
        let scan = PgDataRowIter{
            payload: _.payload,
            pos: 2,
            total_columns: _.total_columns,
            current_index: 0,
        };
        let idx: u16 = 0;
        while (idx <= target_index) {
            const col_opt = scan.next()?;
            if (col := col_opt) {
                if (idx == target_index) {
                    return col;
                }
            } else {
                return PgDataRowError.TruncatedFrame;
            }
            idx += 1;
        }
        return PgDataRowError.ColumnIndexOutOfBounds;
    }

    /// 完整校验整行所有列长度且无任何尾随垃圾字节
    fn validateComplete(): void! {
        let scan = PgDataRowIter{
            payload: _.payload,
            pos: 2,
            total_columns: _.total_columns,
            current_index: 0,
        };
        while (true) {
            const col_opt = scan.next()?;
            if (col_opt == null) {
                break;
            }
        }
    }
}

/// 固定容量（最多 64 列）解码视图
pub struct PgDataRowView {
    column_count: usize = 0,
    columns: [PG_DATA_ROW_MAX_COLS]PgDataRowColumn = [PgDataRowColumn{}; PG_DATA_ROW_MAX_COLS],
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xff) as u8;
    buf[off + 1] = (v & 0xff) as u8;
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xff) as u8;
    buf[off + 1] = ((v >> 16) & 0xff) as u8;
    buf[off + 2] = ((v >> 8) & 0xff) as u8;
    buf[off + 3] = (v & 0xff) as u8;
}

/// 精确预计算完整 `'D'` DataRow 帧总字节长度（含 1B tag + 4B BE length + 2B col_count + 各列长度与数据）
pub fn estimatePgDataRowFrameLen(columns: []const PgDataRowColumn): usize {
    let total: usize = 1 + 4 + 2;
    let i: usize = 0;
    while (i < columns.len) {
        total += 4;
        if (!columns[i].is_null) {
            total += columns[i].bytes.len;
        }
        i += 1;
    }
    return total;
}

/// 编码不含 5 字节帧头（tag + length）的 DataRow 载荷
pub fn encodePgDataRowPayload(columns: []const PgDataRowColumn, out: []u8): usize! {
    if (columns.len > 65535) {
        return PgDataRowError.TooManyColumns;
    }
    const needed = estimatePgDataRowFrameLen(columns) - 5;
    if (needed > out.len) {
        return PgDataRowError.BufferTooSmall;
    }
    writeBeU16(out, 0, columns.len as u16);
    let pos: usize = 2;
    let i: usize = 0;
    while (i < columns.len) {
        const col = columns[i];
        if (col.is_null) {
            writeBeU32(out, pos, PG_DATA_ROW_NULL_LEN);
            pos += 4;
        } else {
            if (col.bytes.len > 0x7fff_ffff) {
                return PgDataRowError.InvalidColumnLength;
            }
            writeBeU32(out, pos, col.bytes.len as u32);
            pos += 4;
            let j: usize = 0;
            while (j < col.bytes.len) {
                out[pos + j] = col.bytes[j];
                j += 1;
            }
            pos += col.bytes.len;
        }
        i += 1;
    }
    return pos;
}

/// 编码完整 PostgreSQL `'D'` DataRow 帧（1B `'D'` + 4B BE length + payload）
pub fn encodePgDataRowFrame(columns: []const PgDataRowColumn, out: []u8): usize! {
    if (columns.len > 65535) {
        return PgDataRowError.TooManyColumns;
    }
    const total = estimatePgDataRowFrameLen(columns);
    if (total > out.len) {
        return PgDataRowError.BufferTooSmall;
    }
    out[0] = PG_DATA_ROW_TAG;
    writeBeU32(out, 1, (total - 1) as u32);
    const body_len = encodePgDataRowPayload(columns, out[5..])?;
    return 5 + body_len;
}

/// 解码完整 `'D'` DataRow 帧并返回零拷贝流式列迭代器
pub fn decodePgDataRowIter(frame: []const u8): PgDataRowIter! {
    if (frame.len < 7) {
        return PgDataRowError.TruncatedFrame;
    }
    if (frame[0] != PG_DATA_ROW_TAG) {
        return PgDataRowError.InvalidTag;
    }
    const msg_len = readBeU32(frame, 1) as usize;
    if (msg_len < 6) {
        return PgDataRowError.InvalidMessageLength;
    }
    const total_len = 1 + msg_len;
    if (total_len > frame.len) {
        return PgDataRowError.TruncatedFrame;
    }
    if (total_len < frame.len) {
        return PgDataRowError.TrailingBytes;
    }
    const payload = frame[5..total_len];
    const col_count = readBeU16(payload, 0);
    return PgDataRowIter{
        payload: payload,
        pos: 2,
        total_columns: col_count,
        current_index: 0,
    };
}

/// 解码完整 `'D'` DataRow 帧至固定容量视图（最多 64 列，严格校验无截断与无尾随字节）
pub fn decodePgDataRowFrame(frame: []const u8): PgDataRowView! {
    let it = decodePgDataRowIter(frame)?;
    if ((it.total_columns as usize) > PG_DATA_ROW_MAX_COLS) {
        return PgDataRowError.TooManyColumns;
    }
    let view = PgDataRowView{ column_count: it.total_columns as usize };
    let idx: usize = 0;
    while (true) {
        const col_opt = it.next()?;
        if (col := col_opt) {
            view.columns[idx] = col;
            idx += 1;
        } else {
            break;
        }
    }
    return view;
}
