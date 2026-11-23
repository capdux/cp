// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/ErrorResponse.cp | Synced: 2026-11-23T01:31:58Z
// Pure-CP zero-allocation PostgreSQL v3.0 Backend `ErrorResponse` ('E') diagnostic frame codec.
// Reuses `PgDiagnosticFields` (18-field container) with SQLSTATE taxonomy helpers for
// retryability, constraint violations, auth failures, and fatal/panic connection triage.

import {
    PgDiagnosticFieldType,
    PgDiagnosticField,
    PgDiagnosticFields,
} from "./fieldMessage.cp";

/// PostgreSQL Backend `ErrorResponse` 报文类型字节（`'E' = 0x45`）
pub const PG_TAG_ERROR_RESPONSE: u8 = 'E';

/// PostgreSQL v3.0 报文固定帧头字节数（1B tag + 4B big-endian length）
pub const PG_ERROR_HEADER_LEN: usize = 5;

/// `ErrorResponse` 编解码与边界校验错误集
pub err PgErrorResponseError {
    InvalidTag,
    InvalidLength,
    Truncated,
    MissingTerminator,
    UnknownFieldType,
    TooManyFields,
    TrailingBytes,
    BufferTooSmall,
    EmbeddedNul,
}

/// PostgreSQL `ErrorResponse` 标准非本地化严重级别（对应 `'V'` / `'S'` 字段）
pub enum PgErrorSeverity {
    Error,
    Fatal,
    Panic,
    Unknown,
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

/// 将 PostgreSQL `ErrorResponse` 严重级别字符串归一化为 `PgErrorSeverity`
pub fn parseErrorSeverity(s: []const u8): PgErrorSeverity {
    if (bytesEql(s, "ERROR")) return PgErrorSeverity.Error;
    if (bytesEql(s, "FATAL")) return PgErrorSeverity.Fatal;
    if (bytesEql(s, "PANIC")) return PgErrorSeverity.Panic;
    return PgErrorSeverity.Unknown;
}

/// PostgreSQL v3.0 Backend `ErrorResponse` (`'E'`) 诊断报文（零堆分配）
///
/// 线格式：`['E'][len: i32 BE]([field_type: u8][value\0])*[0x00]`
/// 其中 `len` 包含自身 4 字节；若收到无载荷最小帧（`len == 4`），
/// 遵循 Bun `ErrorResponse.decodeInternal` 语义返回空字段集合。
pub struct PgErrorResponse {
    fields: PgDiagnosticFields = PgDiagnosticFields.init(),

    /// 构造空 `ErrorResponse`
    static fn init(): PgErrorResponse {
        return PgErrorResponse{};
    }

    /// 从已有 `PgDiagnosticFields` 构造 `ErrorResponse`
    static fn fromFields(fields: PgDiagnosticFields): PgErrorResponse {
        return PgErrorResponse{ fields: fields };
    }

    /// 追加一条诊断字段（拒绝内嵌 `0x00` 字节，防止 C-string 截断）
    fn addField(*_, field_type: PgDiagnosticFieldType, value: []const u8): usize! {
        if (containsNul(value)) {
            return PgErrorResponseError.EmbeddedNul;
        }
        return _.fields.add(field_type, value);
    }

    fn count(): usize {
        return _.fields.count();
    }

    fn isEmpty(): bool {
        return _.fields.count() == 0;
    }

    fn at(idx: usize): PgDiagnosticField? {
        return _.fields.at(idx);
    }

    fn get(field_type: PgDiagnosticFieldType): ([]const u8)? {
        return _.fields.get(field_type);
    }

    /// 优先返回非本地化严重级别 `'V'`，否则回退 `'S'`
    fn severity(): ([]const u8)? {
        return _.fields.severity();
    }

    /// 返回归一化严重级别枚举
    fn severityLevel(): PgErrorSeverity {
        if (s := _.fields.severity()) {
            return parseErrorSeverity(s);
        }
        return PgErrorSeverity.Unknown;
    }

    fn isError(): bool {
        return _.severityLevel() == PgErrorSeverity.Error;
    }

    fn isFatal(): bool {
        return _.severityLevel() == PgErrorSeverity.Fatal;
    }

    fn isPanic(): bool {
        return _.severityLevel() == PgErrorSeverity.Panic;
    }

    /// 判断是否需要立即销毁当前连接（`FATAL` 或 `PANIC`）
    fn isFatalOrPanic(): bool {
        const lvl = _.severityLevel();
        return lvl == PgErrorSeverity.Fatal || lvl == PgErrorSeverity.Panic;
    }

    fn code(): ([]const u8)? {
        return _.fields.code();
    }

    fn sqlstateClass(): ([]const u8)? {
        return _.fields.sqlstateClass();
    }

    fn message(): ([]const u8)? {
        return _.fields.message();
    }

    fn detail(): ([]const u8)? {
        return _.fields.detail();
    }

    fn hint(): ([]const u8)? {
        return _.fields.hint();
    }

    fn schema(): ([]const u8)? {
        return _.fields.schema();
    }

    fn table(): ([]const u8)? {
        return _.fields.table();
    }

    fn column(): ([]const u8)? {
        return _.fields.column();
    }

    fn constraint(): ([]const u8)? {
        return _.fields.constraint();
    }

    fn whereContext(): ([]const u8)? {
        return _.fields.get(PgDiagnosticFieldType.Where);
    }

    fn file(): ([]const u8)? {
        return _.fields.get(PgDiagnosticFieldType.File);
    }

    fn line(): ([]const u8)? {
        return _.fields.get(PgDiagnosticFieldType.Line);
    }

    fn routine(): ([]const u8)? {
        return _.fields.get(PgDiagnosticFieldType.Routine);
    }

    /// 判断是否为可安全重试的事务冲突（`40001` 序列化冲突 / `40P01` 死锁检测）
    fn isRetryable(): bool {
        return _.fields.isRetryable();
    }

    /// 判断是否为完整性约束冲突（SQLSTATE Class `"23"`）
    fn isConstraintViolation(): bool {
        if (cls := _.fields.sqlstateClass()) {
            return bytesEql(cls, "23");
        }
        return false;
    }

    /// 判断是否为唯一约束冲突（`23505`）
    fn isUniqueViolation(): bool {
        if (c := _.fields.code()) {
            return bytesEql(c, "23505");
        }
        return false;
    }

    /// 判断是否为外键约束冲突（`23503`）
    fn isForeignKeyViolation(): bool {
        if (c := _.fields.code()) {
            return bytesEql(c, "23503");
        }
        return false;
    }

    /// 判断是否为认证/授权异常（SQLSTATE Class `"28"`）
    fn isAuthFailure(): bool {
        if (cls := _.fields.sqlstateClass()) {
            return bytesEql(cls, "28");
        }
        return false;
    }

    /// 判断是否为连接级异常（SQLSTATE Class `"08"` 或 `FATAL`/`PANIC`），连接池不应归还此类连接
    fn isConnectionException(): bool {
        if (_.isFatalOrPanic()) {
            return true;
        }
        if (cls := _.fields.sqlstateClass()) {
            return bytesEql(cls, "08");
        }
        return false;
    }

    /// 计算仅含字段列表及尾部 `0x00` 终结字节的载荷长度
    fn payloadLen(): usize {
        return _.fields.encodedLen();
    }

    /// 计算完整线协议帧总字节数（`1B 'E' + 4B len + payloadLen`）
    fn encodedLen(): usize {
        return PG_ERROR_HEADER_LEN + _.fields.encodedLen();
    }

    /// 编码为完整 PostgreSQL v3.0 `'E'` 线协议帧（零堆分配）
    fn encodeFrame(out: []u8): usize! {
        const total = _.encodedLen();
        if (out.len < total) {
            return PgErrorResponseError.BufferTooSmall;
        }
        const msg_len: u32 = @as(u32, @intCast(4 + _.fields.encodedLen()));
        out[0] = PG_TAG_ERROR_RESPONSE;
        writeBeU32(out, 1, msg_len);
        const written = _.fields.encodePayload(out[PG_ERROR_HEADER_LEN..])?;
        return PG_ERROR_HEADER_LEN + written;
    }

    /// 解码不含 5 字节帧头的载荷（宽容模式，跳过未来未知字段类型）
    static fn decodePayload(payload: []const u8): PgErrorResponse! {
        if (payload.len == 0) {
            return PgErrorResponse.init();
        }
        const f = PgDiagnosticFields.decodePayload(payload)?;
        return PgErrorResponse.fromFields(f);
    }

    /// 解码不含 5 字节帧头的载荷（严格模式，遇到未知字段类型报错）
    static fn decodePayloadStrict(payload: []const u8): PgErrorResponse! {
        if (payload.len == 0) {
            return PgErrorResponse.init();
        }
        const f = PgDiagnosticFields.decodePayloadStrict(payload)?;
        return PgErrorResponse.fromFields(f);
    }

    /// 解码完整 `'E'` 帧（宽容模式）
    static fn decodeFrame(frame: []const u8): PgErrorResponse! {
        if (frame.len < PG_ERROR_HEADER_LEN) {
            return PgErrorResponseError.Truncated;
        }
        if (frame[0] != PG_TAG_ERROR_RESPONSE) {
            return PgErrorResponseError.InvalidTag;
        }
        const msg_len_u32 = readBeU32(frame, 1);
        if (msg_len_u32 < 4) {
            return PgErrorResponseError.InvalidLength;
        }
        const total_len: usize = @as(usize, msg_len_u32) + 1;
        if (frame.len < total_len) {
            return PgErrorResponseError.Truncated;
        }
        if (frame.len > total_len) {
            return PgErrorResponseError.TrailingBytes;
        }
        return PgErrorResponse.decodePayload(frame[PG_ERROR_HEADER_LEN..total_len]);
    }

    /// 解码完整 `'E'` 帧（严格模式）
    static fn decodeFrameStrict(frame: []const u8): PgErrorResponse! {
        if (frame.len < PG_ERROR_HEADER_LEN) {
            return PgErrorResponseError.Truncated;
        }
        if (frame[0] != PG_TAG_ERROR_RESPONSE) {
            return PgErrorResponseError.InvalidTag;
        }
        const msg_len_u32 = readBeU32(frame, 1);
        if (msg_len_u32 < 4) {
            return PgErrorResponseError.InvalidLength;
        }
        const total_len: usize = @as(usize, msg_len_u32) + 1;
        if (frame.len < total_len) {
            return PgErrorResponseError.Truncated;
        }
        if (frame.len > total_len) {
            return PgErrorResponseError.TrailingBytes;
        }
        return PgErrorResponse.decodePayloadStrict(frame[PG_ERROR_HEADER_LEN..total_len]);
    }

    /// 值语义相等比较
    fn eql(other: PgErrorResponse): bool {
        return _.fields.eql(other.fields);
    }
}
