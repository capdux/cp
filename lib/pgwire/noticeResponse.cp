// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NoticeResponse.cp | Synced: 2026-11-22T20:23:40Z
// Pure-CP zero-allocation PostgreSQL v3.0 `NoticeResponse` ('N') diagnostic frame codec.

import {
    PgDiagnosticFieldType,
    PgDiagnosticField,
    PgDiagnosticFields,
} from "./fieldMessage.cp";

/// PostgreSQL Backend `NoticeResponse` 报文类型字节（`'N' = 0x4E`）
pub const PG_TAG_NOTICE_RESPONSE: u8 = 'N';

/// PostgreSQL v3.0 报文固定帧头字节数（1B tag + 4B big-endian length）
pub const PG_NOTICE_HEADER_LEN: usize = 5;

/// `NoticeResponse` 编解码与边界校验错误集
pub err PgNoticeError {
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

/// PostgreSQL `NoticeResponse` 标准非本地化严重级别分类（对应 `'V'` / `'S'` 字段）
pub enum PgNoticeSeverity {
    Notice,
    Warning,
    Info,
    Log,
    Debug,
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

/// 将 PostgreSQL `NoticeResponse` 的严重级别字符串映射为 `PgNoticeSeverity`
pub fn parseNoticeSeverity(s: []const u8): PgNoticeSeverity {
    if (bytesEql(s, "NOTICE")) return PgNoticeSeverity.Notice;
    if (bytesEql(s, "WARNING")) return PgNoticeSeverity.Warning;
    if (bytesEql(s, "INFO")) return PgNoticeSeverity.Info;
    if (bytesEql(s, "LOG")) return PgNoticeSeverity.Log;
    if (bytesEql(s, "DEBUG") || bytesEql(s, "DEBUG1") || bytesEql(s, "DEBUG2") ||
        bytesEql(s, "DEBUG3") || bytesEql(s, "DEBUG4") || bytesEql(s, "DEBUG5")) {
        return PgNoticeSeverity.Debug;
    }
    return PgNoticeSeverity.Unknown;
}

/// PostgreSQL v3.0 Backend `NoticeResponse` (`'N'`) 异步诊断报文（零堆分配）
///
/// 线格式：`['N'][len: i32 BE]([field_type: u8][value\0])*[0x00]`
/// 其中 `len` 包含自身 4 字节；若服务端发送无载荷的最小帧（`len == 4`），
/// 解码器遵循 Bun `NoticeResponse.decodeInternal` 语义返回空字段集合。
pub struct PgNoticeResponse {
    fields: PgDiagnosticFields = PgDiagnosticFields.init(),

    static fn init(): PgNoticeResponse {
        return PgNoticeResponse{};
    }

    static fn fromFields(fields: PgDiagnosticFields): PgNoticeResponse {
        return PgNoticeResponse{ fields: fields };
    }

    /// 追加一条诊断字段（拒绝包含内嵌 `0x00` 字节的值，防止截断注入）
    fn addField(*_, field_type: PgDiagnosticFieldType, value: []const u8): usize! {
        if (containsNul(value)) {
            return err.EmbeddedNul;
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

    /// 将当前报文严重级别归一化为 `PgNoticeSeverity`
    fn severityLevel(): PgNoticeSeverity {
        if (s := _.fields.severity()) {
            return parseNoticeSeverity(s);
        }
        return PgNoticeSeverity.Unknown;
    }

    fn isWarning(): bool {
        return _.severityLevel() == PgNoticeSeverity.Warning;
    }

    fn isNotice(): bool {
        return _.severityLevel() == PgNoticeSeverity.Notice;
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

    /// 计算仅含字段列表及尾部 `0x00` 终结字节的载荷长度
    fn payloadLen(): usize {
        return _.fields.encodedLen();
    }

    /// 计算完整线协议帧总字节数（`1B 'N' + 4B len + payloadLen`）
    fn encodedLen(): usize {
        return PG_NOTICE_HEADER_LEN + _.fields.encodedLen();
    }

    /// 将当前 `PgNoticeResponse` 编码为完整 PostgreSQL v3.0 `'N'` 线协议帧（零堆分配）
    fn encodeFrame(out: []u8): usize! {
        const total = _.encodedLen();
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < _.fields.len) {
            if (containsNul(_.fields.items[i].value)) {
                return err.EmbeddedNul;
            }
            i += 1;
        }
        const len_field: usize = total - 1;
        if (len_field > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        out[0] = PG_TAG_NOTICE_RESPONSE;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));
        const payload_written = _.fields.encodePayload(out[PG_NOTICE_HEADER_LEN..total])?;
        return PG_NOTICE_HEADER_LEN + payload_written;
    }

    /// 解码不含 5 字节帧头的 `NoticeResponse` 载荷（宽容模式：忽略未知未来扩展字段）
    /// 与 Bun `NoticeResponse.decodeInternal` 对齐：若 `payload.len == 0` 返回空消息。
    static fn decodePayload(payload: []const u8): PgNoticeResponse! {
        if (payload.len == 0) {
            return PgNoticeResponse.init();
        }
        const decoded = PgDiagnosticFields.decodePayload(payload)?;
        return PgNoticeResponse.fromFields(decoded);
    }

    /// 解码不含 5 字节帧头的 `NoticeResponse` 载荷（严格模式：拒绝未知字段类型）
    static fn decodePayloadStrict(payload: []const u8): PgNoticeResponse! {
        if (payload.len == 0) {
            return PgNoticeResponse.init();
        }
        const decoded = PgDiagnosticFields.decodePayloadStrict(payload)?;
        return PgNoticeResponse.fromFields(decoded);
    }

    /// 解码完整 `'N'` 线协议帧（宽容模式：忽略未知未来扩展字段）
    static fn decodeFrame(frame: []const u8): PgNoticeResponse! {
        return decodeFrameInternal(frame, false);
    }

    /// 解码完整 `'N'` 线协议帧（严格模式：拒绝未知字段类型）
    static fn decodeFrameStrict(frame: []const u8): PgNoticeResponse! {
        return decodeFrameInternal(frame, true);
    }

    fn eql(other: PgNoticeResponse): bool {
        return _.fields.eql(other.fields);
    }
}

fn decodeFrameInternal(frame: []const u8, strict: bool): PgNoticeResponse! {
    if (frame.len < PG_NOTICE_HEADER_LEN) {
        return err.Truncated;
    }
    if (frame[0] != PG_TAG_NOTICE_RESPONSE) {
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
    const payload = frame[PG_NOTICE_HEADER_LEN..expected_total];
    if (strict) {
        return PgNoticeResponse.decodePayloadStrict(payload);
    }
    return PgNoticeResponse.decodePayload(payload);
}
