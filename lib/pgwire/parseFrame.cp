// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Parse.cp | Synced: 2026-11-22T20:44:10Z
// Pure-CP zero-allocation PostgreSQL v3.0 Extended Query `Parse` ('P') and 5-byte control signal frames.

/// PostgreSQL Frontend `Parse` 报文类型字节（`'P' = 0x50`）
pub const PG_TAG_PARSE: u8 = 'P';

/// PostgreSQL Frontend `Sync` 报文类型字节（`'S' = 0x53`）
pub const PG_TAG_SYNC: u8 = 'S';

/// PostgreSQL Frontend `Flush` 报文类型字节（`'H' = 0x48`）
pub const PG_TAG_FLUSH: u8 = 'H';

/// PostgreSQL Backend `ParseComplete` 报文类型字节（`'1' = 0x31`）
pub const PG_TAG_PARSE_COMPLETE: u8 = '1';

/// PostgreSQL Backend `BindComplete` 报文类型字节（`'2' = 0x32`）
pub const PG_TAG_BIND_COMPLETE: u8 = '2';

/// PostgreSQL Backend `NoData` 报文类型字节（`'n' = 0x6E`）
pub const PG_TAG_NO_DATA: u8 = 'n';

/// PostgreSQL Backend `EmptyQueryResponse` 报文类型字节（`'I' = 0x49`）
pub const PG_TAG_EMPTY_QUERY: u8 = 'I';

/// PostgreSQL Backend `PortalSuspended` 报文类型字节（`'s' = 0x73`）
pub const PG_TAG_PORTAL_SUSPENDED: u8 = 's';

/// 无载荷 5 字节控制信号帧固定总长度（1B tag + 4B len(4)）
pub const PG_EMPTY_FRAME_LEN: usize = 5;

/// 单条 `Parse` 报文零堆分配最大显式参数类型 OID 数量
pub const PG_PARSE_MAX_PARAM_OIDS: usize = 64;

/// `Parse` 与控制信号帧编解码错误集
pub err PgParseFrameError {
    InvalidTag,
    InvalidLength,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
    TooManyParameters,
    BufferTooSmall,
}

/// 扩展查询 5 字节无载荷控制信号帧类型
pub enum PgEmptySignalKind: u8 {
    Sync = 'S',
    Flush = 'H',
    ParseComplete = '1',
    BindComplete = '2',
    NoData = 'n',
    EmptyQueryResponse = 'I',
    PortalSuspended = 's',
}

/// 安全将单字节标签映射为 `PgEmptySignalKind`，未知标签返回 `null`
pub fn parseEmptySignalKind(tag: u8): PgEmptySignalKind? {
    if (tag == 'S') return PgEmptySignalKind.Sync;
    if (tag == 'H') return PgEmptySignalKind.Flush;
    if (tag == '1') return PgEmptySignalKind.ParseComplete;
    if (tag == '2') return PgEmptySignalKind.BindComplete;
    if (tag == 'n') return PgEmptySignalKind.NoData;
    if (tag == 'I') return PgEmptySignalKind.EmptyQueryResponse;
    if (tag == 's') return PgEmptySignalKind.PortalSuspended;
    return null;
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

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
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

/// PostgreSQL v3.0 Frontend `Parse` (`'P'`) 预编译语句解析帧（固定容量 64 参数 OID，零堆分配）
///
/// 线格式：`['P'][len: i32 BE][statement_name\0][query\0][param_count: u16 BE]([oid: u32 BE])*`
pub struct PgParseFrame {
    statement_name: []const u8 = "",
    query: []const u8 = "",
    param_oids: [PG_PARSE_MAX_PARAM_OIDS]u32 = [0; PG_PARSE_MAX_PARAM_OIDS],
    param_count: u16 = 0,

    static fn init(stmt_name: []const u8, sql_query: []const u8): PgParseFrame {
        return PgParseFrame{
            statement_name: stmt_name,
            query: sql_query,
        };
    }

    /// 构造匿名预编译语句（`statement_name == ""`）
    static fn unnamed(sql_query: []const u8): PgParseFrame {
        return PgParseFrame{
            statement_name: "",
            query: sql_query,
        };
    }

    /// 追加一个显式参数类型 OID（`0` 表示交由服务端自动类型推断）
    fn addParamOid(*_, oid: u32): u16! {
        if (@as(usize, _.param_count) >= PG_PARSE_MAX_PARAM_OIDS) {
            return err.TooManyParameters;
        }
        const idx = _.param_count;
        _.param_oids[@as(usize, idx)] = oid;
        _.param_count += 1;
        return idx;
    }

    fn isUnnamed(): bool {
        return _.statement_name.len == 0;
    }

    fn paramCount(): u16 {
        return _.param_count;
    }

    fn paramOidAt(idx: u16): u32? {
        if (idx >= _.param_count) {
            return null;
        }
        return _.param_oids[@as(usize, idx)];
    }

    fn encodedLen(): usize {
        return 5 + _.statement_name.len + 1 + _.query.len + 1 + 2 + (@as(usize, _.param_count) * 4);
    }

    /// 编码为完整 `'P'` 线协议帧（零堆分配，拒绝语句名或 SQL 中的内嵌 NUL）
    fn encodeFrame(out: []u8): usize! {
        if (containsNul(_.statement_name) || containsNul(_.query)) {
            return err.EmbeddedNul;
        }
        const total = _.encodedLen();
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        const len_field = total - 1;
        if (len_field > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        out[0] = PG_TAG_PARSE;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));

        let pos: usize = 5;
        copyBytes(out, pos, _.statement_name);
        pos += _.statement_name.len;
        out[pos] = 0;
        pos += 1;

        copyBytes(out, pos, _.query);
        pos += _.query.len;
        out[pos] = 0;
        pos += 1;

        writeBeU16(out, pos, _.param_count);
        pos += 2;

        let i: usize = 0;
        const cnt = @as(usize, _.param_count);
        while (i < cnt) {
            writeBeU32(out, pos, _.param_oids[i]);
            pos += 4;
            i += 1;
        }
        return pos;
    }

    /// 零拷贝解码完整 `'P'` `Parse` 线协议帧
    static fn decodeFrame(frame: []const u8): PgParseFrame! {
        if (frame.len < 9) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_PARSE) {
            return err.InvalidTag;
        }
        const declared_len = @as(usize, readBeU32(frame, 1));
        if (declared_len < 8) {
            return err.InvalidLength;
        }
        const expected_total = 1 + declared_len;
        if (frame.len < expected_total) {
            return err.Truncated;
        }
        if (frame.len > expected_total) {
            return err.TrailingBytes;
        }
        const payload = frame[5..expected_total];

        let nul1: usize = 0;
        while (nul1 < payload.len && payload[nul1] != 0) {
            nul1 += 1;
        }
        if (nul1 >= payload.len) {
            return err.MissingNulTerminator;
        }
        const s_name = payload[0..nul1];

        let nul2 = nul1 + 1;
        while (nul2 < payload.len && payload[nul2] != 0) {
            nul2 += 1;
        }
        if (nul2 >= payload.len) {
            return err.MissingNulTerminator;
        }
        const sql = payload[(nul1 + 1)..nul2];

        const after_sql = nul2 + 1;
        if (payload.len - after_sql < 2) {
            return err.Truncated;
        }
        const count = readBeU16(payload, after_sql);
        if (@as(usize, count) > PG_PARSE_MAX_PARAM_OIDS) {
            return err.TooManyParameters;
        }
        const oids_start = after_sql + 2;
        const expected_oids_bytes = @as(usize, count) * 4;
        const remaining = payload.len - oids_start;
        if (remaining < expected_oids_bytes) {
            return err.Truncated;
        }
        if (remaining > expected_oids_bytes) {
            return err.TrailingBytes;
        }

        let res = PgParseFrame.init(s_name, sql);
        let i: usize = 0;
        let pos = oids_start;
        while (i < @as(usize, count)) {
            res.param_oids[i] = readBeU32(payload, pos);
            pos += 4;
            i += 1;
        }
        res.param_count = count;
        return res;
    }

    fn eql(other: PgParseFrame): bool {
        if (!bytesEql(_.statement_name, other.statement_name) ||
            !bytesEql(_.query, other.query) ||
            _.param_count != other.param_count) {
            return false;
        }
        let i: usize = 0;
        const cnt = @as(usize, _.param_count);
        while (i < cnt) {
            if (_.param_oids[i] != other.param_oids[i]) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

/// PostgreSQL v3.0 5 字节无载荷控制信号帧（`Sync`/`Flush`/`ParseComplete`/`BindComplete`/`NoData`/`EmptyQueryResponse`/`PortalSuspended`）
pub struct PgEmptySignalFrame {
    kind: PgEmptySignalKind = PgEmptySignalKind.Sync,

    static fn init(kind: PgEmptySignalKind): PgEmptySignalFrame {
        return PgEmptySignalFrame{ kind: kind };
    }

    fn encodedLen(): usize {
        return PG_EMPTY_FRAME_LEN;
    }

    fn encodeFrame(out: []u8): usize! {
        if (out.len < PG_EMPTY_FRAME_LEN) {
            return err.BufferTooSmall;
        }
        out[0] = @as(u8, @intFromEnum(_.kind));
        writeBeU32(out, 1, 4);
        return PG_EMPTY_FRAME_LEN;
    }

    /// 解码并校验是否为任意 7 种已知 5 字节无载荷控制信号帧
    static fn decodeAny(frame: []const u8): PgEmptySignalFrame! {
        if (frame.len < PG_EMPTY_FRAME_LEN) {
            return err.Truncated;
        }
        if (frame.len > PG_EMPTY_FRAME_LEN) {
            return err.TrailingBytes;
        }
        const declared_len = readBeU32(frame, 1);
        if (declared_len != 4) {
            return err.InvalidLength;
        }
        if (k := parseEmptySignalKind(frame[0])) {
            return PgEmptySignalFrame{ kind: k };
        }
        return err.InvalidTag;
    }

    /// 解码并严格校验标签匹配指定 `expected_kind`
    static fn decodeFrame(frame: []const u8, expected_kind: PgEmptySignalKind): PgEmptySignalFrame! {
        const sig = PgEmptySignalFrame.decodeAny(frame)?;
        if (sig.kind != expected_kind) {
            return err.InvalidTag;
        }
        return sig;
    }

    fn eql(other: PgEmptySignalFrame): bool {
        return _.kind == other.kind;
    }
}
