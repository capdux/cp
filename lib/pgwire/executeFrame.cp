// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Execute.cp | Synced: 2026-11-22T21:04:14Z
// Pure-CP zero-allocation PostgreSQL v3.0 Frontend `Execute` ('E') wire frame codec.
// Supports unnamed and named portal execution, `max_rows` cursor pagination,
// strict NUL/bounds validation, and single-buffer `Execute+Sync` / `Execute+Flush` pipelining.

/// PostgreSQL `Execute` 客户端报文 Tag 字节 (`'E' = 0x45`)
pub const PG_TAG_EXECUTE: u8 = 'E';

/// `Execute` 最小完整帧长度（1 tag + 4 length + 1 NUL + 4 max_rows = 10）
pub const PG_EXECUTE_MIN_FRAME_LEN: usize = 10;

/// 协议约定的无行数上限哨兵值（`max_rows == 0` 表示拉取全部行）
pub const PG_EXECUTE_UNLIMITED_ROWS: i32 = 0;

/// `Execute` 编解码与边界校验错误集
pub err PgExecuteFrameError {
    InvalidTag,
    InvalidLength,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
    NegativeMaxRows,
    BufferTooSmall,
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    const b0: u32 = @as(u32, buf[off]);
    const b1: u32 = @as(u32, buf[off + 1]);
    const b2: u32 = @as(u32, buf[off + 2]);
    const b3: u32 = @as(u32, buf[off + 3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readBeI32(buf: []const u8, off: usize): i32 {
    const u: u32 = readBeU32(buf, off);
    return @bitCast(u);
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

fn writeBeI32(out: []u8, off: usize, v: i32) {
    const u: u32 = @bitCast(v);
    writeBeU32(out, off, u);
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

fn slicesEqual(a: []const u8, b: []const u8): bool {
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

fn writeEmptySignal(out: []u8, off: usize, tag: u8) {
    out[off] = tag;
    out[off + 1] = 0;
    out[off + 2] = 0;
    out[off + 3] = 0;
    out[off + 4] = 4;
}

/// 零堆分配 PostgreSQL `Execute` (`'E'`) 报文模型与编解码器
pub struct PgExecuteFrame {
    portal: []const u8 = "",
    max_rows: i32 = PG_EXECUTE_UNLIMITED_ROWS,

    /// 构造未命名 Portal 的 `Execute` 请求（指定 `max_rows`）
    static fn unnamed(max_rows: i32): PgExecuteFrame {
        return PgExecuteFrame{
            portal: "",
            max_rows: max_rows,
        };
    }

    /// 构造未命名 Portal 且无行数限制（`max_rows = 0`）的 `Execute` 请求
    static fn unnamedAll(): PgExecuteFrame {
        return PgExecuteFrame{
            portal: "",
            max_rows: PG_EXECUTE_UNLIMITED_ROWS,
        };
    }

    /// 构造命名 Portal 的 `Execute` 请求（支持游标分页 `max_rows`）
    static fn named(portal: []const u8, max_rows: i32): PgExecuteFrame {
        return PgExecuteFrame{
            portal: portal,
            max_rows: max_rows,
        };
    }

    /// 构造命名 Portal 且无行数限制的 `Execute` 请求
    static fn namedAll(portal: []const u8): PgExecuteFrame {
        return PgExecuteFrame{
            portal: portal,
            max_rows: PG_EXECUTE_UNLIMITED_ROWS,
        };
    }

    /// 判断是否执行未命名 Portal
    fn isUnnamed(): bool {
        return _.portal.len == 0;
    }

    /// 判断是否无行数上限（`max_rows == 0`）
    fn isUnlimited(): bool {
        return _.max_rows == PG_EXECUTE_UNLIMITED_ROWS;
    }

    /// 校验字段合法性（拦截负 `max_rows` 与 Portal 名内嵌 NUL 字节）
    fn validate(): PgExecuteFrameError!bool {
        if (_.max_rows < 0) {
            return PgExecuteFrameError.NegativeMaxRows;
        }
        if (hasEmbeddedNul(_.portal)) {
            return PgExecuteFrameError.EmbeddedNul;
        }
        return true;
    }

    /// 不含 5 字节帧头的载荷长度（`portal.len + 1 + 4`）
    fn payloadLen(): usize {
        return _.portal.len + 5;
    }

    /// 含 5 字节帧头的完整帧长度（`5 + portal.len + 5`）
    fn frameLen(): usize {
        return 5 + _.payloadLen();
    }

    /// 编码载荷（`portal + '\0' + Int32 max_rows`）到 `out`
    fn encodePayload(out: []u8): PgExecuteFrameError!usize {
        * = _.validate()?;
        const plen = _.payloadLen();
        if (out.len < plen) {
            return PgExecuteFrameError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < _.portal.len) {
            out[i] = _.portal[i];
            i += 1;
        }
        out[_.portal.len] = 0;
        writeBeI32(out, _.portal.len + 1, _.max_rows);
        return plen;
    }

    /// 编码完整 `'E'` 帧到 `out`，返回写入字节数
    fn encodeFrame(out: []u8): PgExecuteFrameError!usize {
        * = _.validate()?;
        const flen = _.frameLen();
        if (out.len < flen) {
            return PgExecuteFrameError.BufferTooSmall;
        }
        const msg_len: u32 = @as(u32, @intCast(4 + _.payloadLen()));
        out[0] = PG_TAG_EXECUTE;
        writeBeU32(out, 1, msg_len);
        const written = _.encodePayload(out[5..])?;
        return 5 + written;
    }

    /// 零堆分配单缓冲编码 `Execute('E') + Sync('S')` 扩展查询管线帧组合
    fn encodeExecuteSync(out: []u8): PgExecuteFrameError!usize {
        const total = _.frameLen() + 5;
        if (out.len < total) {
            return PgExecuteFrameError.BufferTooSmall;
        }
        const n = _.encodeFrame(out)?;
        writeEmptySignal(out, n, 'S');
        return n + 5;
    }

    /// 零堆分配单缓冲编码 `Execute('E') + Flush('H')` 流式分页拉取帧组合
    fn encodeExecuteFlush(out: []u8): PgExecuteFrameError!usize {
        const total = _.frameLen() + 5;
        if (out.len < total) {
            return PgExecuteFrameError.BufferTooSmall;
        }
        const n = _.encodeFrame(out)?;
        writeEmptySignal(out, n, 'H');
        return n + 5;
    }

    /// 解码不含 5 字节帧头的 `Execute` 载荷
    static fn decodePayload(payload: []const u8): PgExecuteFrameError!PgExecuteFrame {
        if (payload.len < 5) {
            return PgExecuteFrameError.Truncated;
        }
        let nul_pos: usize = 0;
        let found = false;
        while (nul_pos < payload.len) {
            if (payload[nul_pos] == 0) {
                found = true;
                break;
            }
            nul_pos += 1;
        }
        if (!found) {
            return PgExecuteFrameError.MissingNulTerminator;
        }
        const expected_len = nul_pos + 1 + 4;
        if (payload.len < expected_len) {
            return PgExecuteFrameError.Truncated;
        }
        if (payload.len > expected_len) {
            return PgExecuteFrameError.TrailingBytes;
        }
        const max_rows = readBeI32(payload, nul_pos + 1);
        if (max_rows < 0) {
            return PgExecuteFrameError.NegativeMaxRows;
        }
        return PgExecuteFrame{
            portal: payload[0..nul_pos],
            max_rows: max_rows,
        };
    }

    /// 解码完整 `'E'` 帧（校验 Tag、长度、NUL 终结符与零尾随字节）
    static fn decodeFrame(frame: []const u8): PgExecuteFrameError!PgExecuteFrame {
        if (frame.len < PG_EXECUTE_MIN_FRAME_LEN) {
            return PgExecuteFrameError.Truncated;
        }
        if (frame[0] != PG_TAG_EXECUTE) {
            return PgExecuteFrameError.InvalidTag;
        }
        const msg_len_u32 = readBeU32(frame, 1);
        if (msg_len_u32 < 9) {
            return PgExecuteFrameError.InvalidLength;
        }
        const total_len: usize = @as(usize, msg_len_u32) + 1;
        if (frame.len < total_len) {
            return PgExecuteFrameError.Truncated;
        }
        if (frame.len > total_len) {
            return PgExecuteFrameError.TrailingBytes;
        }
        return PgExecuteFrame.decodePayload(frame[5..total_len]);
    }

    /// 值语义相等比较
    fn eql(other: PgExecuteFrame): bool {
        return _.max_rows == other.max_rows && slicesEqual(_.portal, other.portal);
    }
}
