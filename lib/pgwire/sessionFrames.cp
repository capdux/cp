// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/BackendKeyData.cp | Synced: 2026-11-22T20:29:40Z
// Pure-CP zero-allocation PostgreSQL v3.0 Session Control & Query Cancellation frames:
// BackendKeyData ('K'), CancelRequest (80877102), ParameterStatus ('S'), and ReadyForQuery ('Z').

/// PostgreSQL Backend `BackendKeyData` 报文类型字节（`'K' = 0x4B`）
pub const PG_TAG_BACKEND_KEY_DATA: u8 = 'K';

/// PostgreSQL Backend `ParameterStatus` 报文类型字节（`'S' = 0x53`）
pub const PG_TAG_PARAMETER_STATUS: u8 = 'S';

/// PostgreSQL Backend `ReadyForQuery` 报文类型字节（`'Z' = 0x5A`）
pub const PG_TAG_READY_FOR_QUERY: u8 = 'Z';

/// PostgreSQL `CancelRequest` 协议魔数（`(1234 << 16) | 5678 = 80877102`）
pub const PG_CANCEL_REQUEST_CODE: u32 = 80877102;

/// `BackendKeyData` 固定完整帧长度（1B `'K'` + 4B len(12) + 4B pid + 4B secret_key = 13B）
pub const PG_BACKEND_KEY_FRAME_LEN: usize = 13;

/// `CancelRequest` 固定完整帧长度（4B len(16) + 4B code + 4B pid + 4B secret_key = 16B，无 tag 字节）
pub const PG_CANCEL_REQUEST_FRAME_LEN: usize = 16;

/// `ReadyForQuery` 固定完整帧长度（1B `'Z'` + 4B len(5) + 1B status = 6B）
pub const PG_READY_FOR_QUERY_FRAME_LEN: usize = 6;

/// 会话控制帧编解码与边界校验错误集
pub err PgSessionFrameError {
    InvalidTag,
    InvalidLength,
    InvalidCancelCode,
    InvalidTxStatus,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
    EmptyParamName,
    BufferTooSmall,
}

/// PostgreSQL `ReadyForQuery` 事务状态指示符
pub enum PgTransactionState: u8 {
    /// `'I'`: 空闲（不在事务块中）
    Idle = 'I',
    /// `'T'`: 事务块进行中
    InTransaction = 'T',
    /// `'E'`: 失败事务块（必须 `ROLLBACK` 后才能继续执行查询）
    FailedTransaction = 'E',
}

/// 安全将单字节映射为 `PgTransactionState`，未知字节返回 `null`
pub fn parseTransactionState(code: u8): PgTransactionState? {
    if (code == 'I') return PgTransactionState.Idle;
    if (code == 'T') return PgTransactionState.InTransaction;
    if (code == 'E') return PgTransactionState.FailedTransaction;
    return null;
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

/// PostgreSQL v3.0 Frontend `CancelRequest` — 独立短连接查询取消报文（零堆分配）
///
/// 线格式：`[len: 16 BE][code: 80877102 BE][process_id: u32 BE][secret_key: u32 BE]`（无 1B tag）
pub struct PgCancelRequestFrame {
    process_id: u32 = 0,
    secret_key: u32 = 0,

    static fn init(process_id: u32, secret_key: u32): PgCancelRequestFrame {
        return PgCancelRequestFrame{
            process_id: process_id,
            secret_key: secret_key,
        };
    }

    fn encodedLen(): usize {
        return PG_CANCEL_REQUEST_FRAME_LEN;
    }

    fn encodeFrame(out: []u8): usize! {
        if (out.len < PG_CANCEL_REQUEST_FRAME_LEN) {
            return err.BufferTooSmall;
        }
        writeBeU32(out, 0, 16);
        writeBeU32(out, 4, PG_CANCEL_REQUEST_CODE);
        writeBeU32(out, 8, _.process_id);
        writeBeU32(out, 12, _.secret_key);
        return PG_CANCEL_REQUEST_FRAME_LEN;
    }

    static fn decodeFrame(frame: []const u8): PgCancelRequestFrame! {
        if (frame.len < PG_CANCEL_REQUEST_FRAME_LEN) {
            return err.Truncated;
        }
        if (frame.len > PG_CANCEL_REQUEST_FRAME_LEN) {
            return err.TrailingBytes;
        }
        const declared_len = readBeU32(frame, 0);
        if (declared_len != 16) {
            return err.InvalidLength;
        }
        const code = readBeU32(frame, 4);
        if (code != PG_CANCEL_REQUEST_CODE) {
            return err.InvalidCancelCode;
        }
        return PgCancelRequestFrame{
            process_id: readBeU32(frame, 8),
            secret_key: readBeU32(frame, 12),
        };
    }

    fn eql(other: PgCancelRequestFrame): bool {
        return _.process_id == other.process_id && _.secret_key == other.secret_key;
    }
}

/// PostgreSQL v3.0 Backend `BackendKeyData` (`'K'`) — 连接后端进程号与取消密钥（零堆分配）
///
/// 线格式：`['K'][len: 12 BE][process_id: u32 BE][secret_key: u32 BE]`
pub struct PgBackendKeyFrame {
    process_id: u32 = 0,
    secret_key: u32 = 0,

    static fn init(process_id: u32, secret_key: u32): PgBackendKeyFrame {
        return PgBackendKeyFrame{
            process_id: process_id,
            secret_key: secret_key,
        };
    }

    fn encodedLen(): usize {
        return PG_BACKEND_KEY_FRAME_LEN;
    }

    fn encodeFrame(out: []u8): usize! {
        if (out.len < PG_BACKEND_KEY_FRAME_LEN) {
            return err.BufferTooSmall;
        }
        out[0] = PG_TAG_BACKEND_KEY_DATA;
        writeBeU32(out, 1, 12);
        writeBeU32(out, 5, _.process_id);
        writeBeU32(out, 9, _.secret_key);
        return PG_BACKEND_KEY_FRAME_LEN;
    }

    static fn decodeFrame(frame: []const u8): PgBackendKeyFrame! {
        if (frame.len < PG_BACKEND_KEY_FRAME_LEN) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_BACKEND_KEY_DATA) {
            return err.InvalidTag;
        }
        const declared_len = readBeU32(frame, 1);
        if (declared_len != 12) {
            return err.InvalidLength;
        }
        if (frame.len > PG_BACKEND_KEY_FRAME_LEN) {
            return err.TrailingBytes;
        }
        return PgBackendKeyFrame{
            process_id: readBeU32(frame, 5),
            secret_key: readBeU32(frame, 9),
        };
    }

    /// 直接构造用于独立短连接取消当前会话慢查询的 `CancelRequest` 帧
    fn toCancelRequest(): PgCancelRequestFrame {
        return PgCancelRequestFrame.init(_.process_id, _.secret_key);
    }

    fn eql(other: PgBackendKeyFrame): bool {
        return _.process_id == other.process_id && _.secret_key == other.secret_key;
    }
}

/// PostgreSQL v3.0 Backend `ParameterStatus` (`'S'`) — 运行时会话参数推送帧（零堆分配）
///
/// 线格式：`['S'][len: i32 BE][name\0][value\0]`
pub struct PgParameterStatusFrame {
    name: []const u8 = "",
    value: []const u8 = "",

    static fn init(name: []const u8, value: []const u8): PgParameterStatusFrame {
        return PgParameterStatusFrame{
            name: name,
            value: value,
        };
    }

    fn encodedLen(): usize {
        return 5 + _.name.len + 1 + _.value.len + 1;
    }

    fn encodeFrame(out: []u8): usize! {
        if (_.name.len == 0) {
            return err.EmptyParamName;
        }
        if (containsNul(_.name) || containsNul(_.value)) {
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
        out[0] = PG_TAG_PARAMETER_STATUS;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));
        let pos: usize = 5;
        copyBytes(out, pos, _.name);
        pos += _.name.len;
        out[pos] = 0;
        pos += 1;
        copyBytes(out, pos, _.value);
        pos += _.value.len;
        out[pos] = 0;
        return pos + 1;
    }

    static fn decodeFrame(frame: []const u8): PgParameterStatusFrame! {
        if (frame.len < 5) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_PARAMETER_STATUS) {
            return err.InvalidTag;
        }
        const declared_len = @as(usize, readBeU32(frame, 1));
        if (declared_len < 4) {
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
        const param_name = payload[0..nul1];
        if (param_name.len == 0) {
            return err.EmptyParamName;
        }

        let nul2 = nul1 + 1;
        while (nul2 < payload.len && payload[nul2] != 0) {
            nul2 += 1;
        }
        if (nul2 >= payload.len) {
            return err.MissingNulTerminator;
        }
        if (nul2 + 1 != payload.len) {
            return err.TrailingBytes;
        }
        const param_val = payload[(nul1 + 1)..nul2];
        return PgParameterStatusFrame{
            name: param_name,
            value: param_val,
        };
    }

    fn isServerVersion(): bool {
        return bytesEql(_.name, "server_version");
    }

    fn isClientEncoding(): bool {
        return bytesEql(_.name, "client_encoding");
    }

    fn isServerEncoding(): bool {
        return bytesEql(_.name, "server_encoding");
    }

    fn isTimeZone(): bool {
        return bytesEql(_.name, "TimeZone");
    }

    fn isStandardConformingStrings(): bool {
        return bytesEql(_.name, "standard_conforming_strings");
    }

    fn eql(other: PgParameterStatusFrame): bool {
        return bytesEql(_.name, other.name) && bytesEql(_.value, other.value);
    }
}

/// PostgreSQL v3.0 Backend `ReadyForQuery` (`'Z'`) — 查询周期完成与事务状态帧（零堆分配）
///
/// 线格式：`['Z'][len: 5 BE][status: 'I' | 'T' | 'E']`
pub struct PgReadyForQueryFrame {
    status: PgTransactionState = PgTransactionState.Idle,

    static fn init(status: PgTransactionState): PgReadyForQueryFrame {
        return PgReadyForQueryFrame{ status: status };
    }

    fn encodedLen(): usize {
        return PG_READY_FOR_QUERY_FRAME_LEN;
    }

    fn encodeFrame(out: []u8): usize! {
        if (out.len < PG_READY_FOR_QUERY_FRAME_LEN) {
            return err.BufferTooSmall;
        }
        out[0] = PG_TAG_READY_FOR_QUERY;
        writeBeU32(out, 1, 5);
        out[5] = @as(u8, @intFromEnum(_.status));
        return PG_READY_FOR_QUERY_FRAME_LEN;
    }

    static fn decodeFrame(frame: []const u8): PgReadyForQueryFrame! {
        if (frame.len < PG_READY_FOR_QUERY_FRAME_LEN) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_READY_FOR_QUERY) {
            return err.InvalidTag;
        }
        const declared_len = readBeU32(frame, 1);
        if (declared_len != 5) {
            return err.InvalidLength;
        }
        if (frame.len > PG_READY_FOR_QUERY_FRAME_LEN) {
            return err.TrailingBytes;
        }
        if (st := parseTransactionState(frame[5])) {
            return PgReadyForQueryFrame{ status: st };
        }
        return err.InvalidTxStatus;
    }

    fn isIdle(): bool {
        return _.status == PgTransactionState.Idle;
    }

    fn inTransaction(): bool {
        return _.status == PgTransactionState.InTransaction;
    }

    fn isFailedTransaction(): bool {
        return _.status == PgTransactionState.FailedTransaction;
    }

    /// 指示当前连接是否可直接发起新查询（`'I'` 或 `'T'` 状态）
    fn canSendQuery(): bool {
        return _.status != PgTransactionState.FailedTransaction;
    }

    /// 指示当前事务已失败，连接池必须先执行 `ROLLBACK` 才能复用
    fn needsRollback(): bool {
        return _.status == PgTransactionState.FailedTransaction;
    }

    fn eql(other: PgReadyForQueryFrame): bool {
        return _.status == other.status;
    }
}
