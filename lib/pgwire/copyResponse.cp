// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/CopyInResponse.cp | Synced: 2026-11-22T21:06:42Z
// Pure-CP zero-allocation PostgreSQL v3.0 Backend `CopyInResponse` ('G'),
// `CopyOutResponse` ('H'), and `CopyBothResponse` ('W') wire frame codec.
// Implements strict format-code validation (0=text, 1=binary), zero-copy column format
// view/iterator, stack-backed encoder, and arbitrary-width slice encoder.

/// PostgreSQL COPY 响应报文 Tag 字节常量
pub const PG_TAG_COPY_IN_RESPONSE: u8 = 'G';
pub const PG_TAG_COPY_OUT_RESPONSE: u8 = 'H';
pub const PG_TAG_COPY_BOTH_RESPONSE: u8 = 'W';

/// COPY 响应最小完整帧长度（1 tag + 4 length + 1 overall_format + 2 num_columns = 8）
pub const PG_COPY_RESP_HEADER_LEN: usize = 8;

/// COPY 格式码字节常量（0 = 文本模式, 1 = 二进制模式）
pub const PG_COPY_FORMAT_TEXT: u8 = 0;
pub const PG_COPY_FORMAT_BINARY: u8 = 1;

/// `PgCopyResponseEncoder` 栈缓冲最大列数（`encodeFrameFromSlice` 无此上限）
pub const PG_COPY_RESP_MAX_COLS: usize = 64;

/// COPY 响应编解码与边界校验错误集
pub err PgCopyResponseError {
    InvalidTag,
    InvalidLength,
    InvalidFormatCode,
    NegativeColumnCount,
    Truncated,
    TrailingBytes,
    BufferTooSmall,
    TooManyColumns,
    IndexOutOfBounds,
}

/// COPY 传输方向枚举
pub enum PgCopyDirection {
    CopyIn,
    CopyOut,
    CopyBoth,

    /// 返回对应 PostgreSQL 报文 Tag 字节
    fn tag(): u8 {
        if (_ == PgCopyDirection.CopyOut) return PG_TAG_COPY_OUT_RESPONSE;
        if (_ == PgCopyDirection.CopyBoth) return PG_TAG_COPY_BOTH_RESPONSE;
        return PG_TAG_COPY_IN_RESPONSE;
    }

    /// 从 Tag 字节解析 COPY 方向
    static fn fromTag(tag_byte: u8): PgCopyResponseError!PgCopyDirection {
        if (tag_byte == PG_TAG_COPY_IN_RESPONSE) return PgCopyDirection.CopyIn;
        if (tag_byte == PG_TAG_COPY_OUT_RESPONSE) return PgCopyDirection.CopyOut;
        if (tag_byte == PG_TAG_COPY_BOTH_RESPONSE) return PgCopyDirection.CopyBoth;
        return PgCopyResponseError.InvalidTag;
    }
}

/// COPY 数据格式枚举（文本 / 二进制）
pub enum PgCopyFormat {
    Text,
    Binary,

    /// 返回格式码字节（`0` 或 `1`）
    fn toByte(): u8 {
        if (_ == PgCopyFormat.Binary) return PG_COPY_FORMAT_BINARY;
        return PG_COPY_FORMAT_TEXT;
    }

    /// 从格式码字节解析
    static fn fromByte(b: u8): PgCopyResponseError!PgCopyFormat {
        if (b == PG_COPY_FORMAT_TEXT) return PgCopyFormat.Text;
        if (b == PG_COPY_FORMAT_BINARY) return PgCopyFormat.Binary;
        return PgCopyResponseError.InvalidFormatCode;
    }
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    const b0: u16 = @as(u16, buf[off]);
    const b1: u16 = @as(u16, buf[off + 1]);
    return (b0 << 8) | b1;
}

fn readBeI16(buf: []const u8, off: usize): i16 {
    const u: u16 = readBeU16(buf, off);
    return @bitCast(u);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeBeU16(out: []u8, off: usize, v: u16) {
    out[off] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 1] = @as(u8, @intCast(v & 0xFF));
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

/// 零拷贝列格式码迭代器
pub struct PgCopyFormatIter {
    col_format_bytes: []const u8 = "",
    index: usize = 0,
    column_count: usize = 0,

    /// 读取下一列的格式枚举，结束返回 `null`
    fn next(*_): PgCopyFormat? {
        if (_.index >= _.column_count) {
            return null;
        }
        const code = readBeU16(_.col_format_bytes, _.index * 2);
        _.index += 1;
        if (code == 1) {
            return PgCopyFormat.Binary;
        }
        return PgCopyFormat.Text;
    }

    /// 返回剩余未迭代的列数
    fn remaining(): usize {
        if (_.index >= _.column_count) {
            return 0;
        }
        return _.column_count - _.index;
    }

    /// 重置迭代器到第 0 列
    fn reset(*_) {
        _.index = 0;
    }
}

/// 零拷贝 PostgreSQL `CopyInResponse` / `CopyOutResponse` / `CopyBothResponse` 解码视图
pub struct PgCopyResponseView {
    direction: PgCopyDirection = PgCopyDirection.CopyIn,
    overall_format: PgCopyFormat = PgCopyFormat.Text,
    column_count: usize = 0,
    col_format_bytes: []const u8 = "",

    /// 解码不含 5 字节帧头的载荷（`Int8 overall_format` + `Int16 num_columns` + `Int16[num_columns]`）
    static fn decodePayload(
        direction: PgCopyDirection,
        payload: []const u8,
    ): PgCopyResponseError!PgCopyResponseView {
        if (payload.len < 3) {
            return PgCopyResponseError.Truncated;
        }
        const overall = PgCopyFormat.fromByte(payload[0])?;
        const raw_cols = readBeI16(payload, 1);
        if (raw_cols < 0) {
            return PgCopyResponseError.NegativeColumnCount;
        }
        const cols: usize = @intCast(raw_cols);
        const expected_len = 3 + cols * 2;
        if (payload.len < expected_len) {
            return PgCopyResponseError.Truncated;
        }
        if (payload.len > expected_len) {
            return PgCopyResponseError.TrailingBytes;
        }

        // 严格校验每一列格式码（协议仅允许 0 或 1）
        let i: usize = 0;
        while (i < cols) {
            const code = readBeU16(payload, 3 + i * 2);
            if (code != 0 && code != 1) {
                return PgCopyResponseError.InvalidFormatCode;
            }
            i += 1;
        }

        return PgCopyResponseView{
            direction: direction,
            overall_format: overall,
            column_count: cols,
            col_format_bytes: payload[3..expected_len],
        };
    }

    /// 解码完整 COPY 响应帧（`'G'` / `'H'` / `'W'` + `Int32 length` + 载荷）
    static fn decodeFrame(frame: []const u8): PgCopyResponseError!PgCopyResponseView {
        if (frame.len < PG_COPY_RESP_HEADER_LEN) {
            return PgCopyResponseError.Truncated;
        }
        const dir = PgCopyDirection.fromTag(frame[0])?;
        const msg_len_u32 = readBeU32(frame, 1);
        if (msg_len_u32 < 7) {
            return PgCopyResponseError.InvalidLength;
        }
        const total_len: usize = @as(usize, msg_len_u32) + 1;
        if (frame.len < total_len) {
            return PgCopyResponseError.Truncated;
        }
        if (frame.len > total_len) {
            return PgCopyResponseError.TrailingBytes;
        }
        return PgCopyResponseView.decodePayload(dir, frame[5..total_len]);
    }

    /// 判断整体格式是否为二进制模式
    fn isBinary(): bool {
        return _.overall_format == PgCopyFormat.Binary;
    }

    /// 判断整体格式是否为文本模式
    fn isText(): bool {
        return _.overall_format == PgCopyFormat.Text;
    }

    /// 读取第 `index`（0-based）列的格式枚举
    fn columnFormatAt(index: usize): PgCopyResponseError!PgCopyFormat {
        if (index >= _.column_count) {
            return PgCopyResponseError.IndexOutOfBounds;
        }
        const code = readBeU16(_.col_format_bytes, index * 2);
        if (code == 1) {
            return PgCopyFormat.Binary;
        }
        return PgCopyFormat.Text;
    }

    /// 创建零拷贝列格式迭代器
    fn iter(): PgCopyFormatIter {
        return PgCopyFormatIter{
            col_format_bytes: _.col_format_bytes,
            index: 0,
            column_count: _.column_count,
        };
    }

    /// 检查是否所有列的格式码都与 `overall_format` 一致
    fn allColumnsMatchOverall(): bool {
        const expected: u16 = @as(u16, _.overall_format.toByte());
        let i: usize = 0;
        while (i < _.column_count) {
            if (readBeU16(_.col_format_bytes, i * 2) != expected) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

/// 零堆分配 PostgreSQL `CopyInResponse` / `CopyOutResponse` / `CopyBothResponse` 编码器
pub struct PgCopyResponseEncoder {
    direction: PgCopyDirection = PgCopyDirection.CopyIn,
    overall_format: PgCopyFormat = PgCopyFormat.Text,
    column_count: usize = 0,
    col_formats: [64]u8 = [0; 64],

    /// 构造指定方向与整体格式的 COPY 响应编码器
    static fn init(direction: PgCopyDirection, overall_format: PgCopyFormat): PgCopyResponseEncoder {
        return PgCopyResponseEncoder{
            direction: direction,
            overall_format: overall_format,
            column_count: 0,
        };
    }

    /// 追加一列格式，返回追加后的总列数
    fn addColumn(*_, fmt: PgCopyFormat): PgCopyResponseError!usize {
        if (_.column_count >= PG_COPY_RESP_MAX_COLS) {
            return PgCopyResponseError.TooManyColumns;
        }
        _.col_formats[_.column_count] = fmt.toByte();
        _.column_count += 1;
        return _.column_count;
    }

    /// 不含 5 字节帧头的载荷长度（`3 + column_count * 2`）
    fn payloadLen(): usize {
        return 3 + _.column_count * 2;
    }

    /// 含 5 字节帧头的完整帧长度（`8 + column_count * 2`）
    fn frameLen(): usize {
        return PG_COPY_RESP_HEADER_LEN + _.column_count * 2;
    }

    /// 编码载荷到 `out`
    fn encodePayload(out: []u8): PgCopyResponseError!usize {
        const plen = _.payloadLen();
        if (out.len < plen) {
            return PgCopyResponseError.BufferTooSmall;
        }
        out[0] = _.overall_format.toByte();
        writeBeU16(out, 1, @as(u16, @intCast(_.column_count)));
        let i: usize = 0;
        while (i < _.column_count) {
            writeBeU16(out, 3 + i * 2, @as(u16, _.col_formats[i]));
            i += 1;
        }
        return plen;
    }

    /// 编码完整 COPY 响应帧到 `out`
    fn encodeFrame(out: []u8): PgCopyResponseError!usize {
        return PgCopyResponseEncoder.encodeFrameFromSlice(
            _.direction,
            _.overall_format,
            _.col_formats[0.._.column_count],
            out,
        );
    }

    /// 直接从任意长度列格式码切片零堆分配编码完整帧（每元素须为 `0` 或 `1`，列数上限 32767）
    static fn encodeFrameFromSlice(
        direction: PgCopyDirection,
        overall_format: PgCopyFormat,
        col_formats: []const u8,
        out: []u8,
    ): PgCopyResponseError!usize {
        if (col_formats.len > 32767) {
            return PgCopyResponseError.TooManyColumns;
        }
        const total_len = PG_COPY_RESP_HEADER_LEN + col_formats.len * 2;
        if (out.len < total_len) {
            return PgCopyResponseError.BufferTooSmall;
        }
        let k: usize = 0;
        while (k < col_formats.len) {
            if (col_formats[k] != PG_COPY_FORMAT_TEXT && col_formats[k] != PG_COPY_FORMAT_BINARY) {
                return PgCopyResponseError.InvalidFormatCode;
            }
            k += 1;
        }
        const msg_len: u32 = @as(u32, @intCast(4 + 3 + col_formats.len * 2));
        out[0] = direction.tag();
        writeBeU32(out, 1, msg_len);
        out[5] = overall_format.toByte();
        writeBeU16(out, 6, @as(u16, @intCast(col_formats.len)));
        let i: usize = 0;
        while (i < col_formats.len) {
            writeBeU16(out, PG_COPY_RESP_HEADER_LEN + i * 2, @as(u16, col_formats[i]));
            i += 1;
        }
        return total_len;
    }
}
