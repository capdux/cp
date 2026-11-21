// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/ErrorResponse.cp | Synced: 2026-11-21T16:59:50Z

/// PostgreSQL v3 后端控制与诊断报文类型字节常量
pub const TAG_ERROR_RESPONSE: u8 = 'E';
pub const TAG_NOTICE_RESPONSE: u8 = 'N';
pub const TAG_BACKEND_KEY_DATA: u8 = 'K';
pub const TAG_PARAMETER_STATUS: u8 = 'S';
pub const TAG_READY_FOR_QUERY: u8 = 'Z';
pub const TAG_NOTIFICATION_RESPONSE: u8 = 'A';
pub const TAG_COMMAND_COMPLETE: u8 = 'C';

/// PostgreSQL v3 后端报文解码错误集
pub err PgMessageError {
    UnexpectedEof,
    InvalidMessageTag,
    InvalidMessageLength,
    InvalidNullTerminator,
    InvalidTransactionStatus,
}

/// PostgreSQL 诊断消息全部 18 种标准字段类型字节（ErrorResponse / NoticeResponse）
pub enum FieldType: u8 {
    severity = 'S',
    localized_severity = 'V',
    code = 'C',
    message = 'M',
    detail = 'D',
    hint = 'H',
    position = 'P',
    internal_position = 'p',
    internal_query = 'q',
    where_ctx = 'W',
    schema = 's',
    table = 't',
    column = 'c',
    datatype = 'd',
    constraint = 'n',
    file = 'F',
    line = 'L',
    routine = 'R',

    fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

/// PostgreSQL 5 字符 SQLSTATE 大类（前 2 位分类码）
pub enum DiagSqlStateClass: u8 {
    Success = 0,
    Warning = 1,
    NoData = 2,
    ConnectionException = 3,
    DataException = 4,
    IntegrityConstraintViolation = 5,
    InvalidTransactionState = 6,
    InvalidAuthorization = 7,
    SyntaxOrAccessError = 8,
    InsufficientResources = 9,
    OperatorIntervention = 10,
    SystemError = 11,
    InternalError = 12,
    Other = 13,
}

/// 按 SQLSTATE 前两位代码归类错误类别（如 `"23505"` -> `IntegrityConstraintViolation`）
pub fn classifySqlState(code: string): DiagSqlStateClass {
    if (code.len < 2) {
        return DiagSqlStateClass.Other;
    }
    const prefix = code[0..2];
    if (std.mem.eql(u8, prefix, "00")) { return DiagSqlStateClass.Success; }
    if (std.mem.eql(u8, prefix, "01")) { return DiagSqlStateClass.Warning; }
    if (std.mem.eql(u8, prefix, "02")) { return DiagSqlStateClass.NoData; }
    if (std.mem.eql(u8, prefix, "08")) { return DiagSqlStateClass.ConnectionException; }
    if (std.mem.eql(u8, prefix, "22")) { return DiagSqlStateClass.DataException; }
    if (std.mem.eql(u8, prefix, "23")) { return DiagSqlStateClass.IntegrityConstraintViolation; }
    if (std.mem.eql(u8, prefix, "25")) { return DiagSqlStateClass.InvalidTransactionState; }
    if (std.mem.eql(u8, prefix, "28")) { return DiagSqlStateClass.InvalidAuthorization; }
    if (std.mem.eql(u8, prefix, "42")) { return DiagSqlStateClass.SyntaxOrAccessError; }
    if (std.mem.eql(u8, prefix, "53")) { return DiagSqlStateClass.InsufficientResources; }
    if (std.mem.eql(u8, prefix, "57")) { return DiagSqlStateClass.OperatorIntervention; }
    if (std.mem.eql(u8, prefix, "58")) { return DiagSqlStateClass.SystemError; }
    if (std.mem.eql(u8, prefix, "XX")) { return DiagSqlStateClass.InternalError; }
    return DiagSqlStateClass.Other;
}

/// 零拷贝诊断消息结构（同时承载 `ErrorResponse` `'E'` 与 `NoticeResponse` `'N'` 全 18 字段）
pub struct DiagnosticMessage {
    is_error: bool = true,
    severity: string? = null,
    localized_severity: string? = null,
    code: string? = null,
    message: string? = null,
    detail: string? = null,
    hint: string? = null,
    position: string? = null,
    internal_position: string? = null,
    internal_query: string? = null,
    where_ctx: string? = null,
    schema: string? = null,
    table: string? = null,
    column: string? = null,
    datatype: string? = null,
    constraint: string? = null,
    file: string? = null,
    line: string? = null,
    routine: string? = null,
    bytes_read: usize = 0,

    fn sqlStateClass(): DiagSqlStateClass {
        if (c := _.code) {
            return classifySqlState(c);
        }
        return DiagSqlStateClass.Other;
    }

    fn isFatal(): bool {
        if (v := _.localized_severity) {
            if (std.mem.eql(u8, v, "FATAL") || std.mem.eql(u8, v, "PANIC")) {
                return true;
            }
        }
        if (s := _.severity) {
            if (std.mem.eql(u8, s, "FATAL") || std.mem.eql(u8, s, "PANIC")) {
                return true;
            }
        }
        return false;
    }
}

/// `BackendKeyData` (`'K'`) 后端进程 ID 与取消密钥
pub struct BackendKeyData {
    process_id: u32,
    secret_key: u32,
    bytes_read: usize,
}

/// `ParameterStatus` (`'S'`) 服务端运行时参数键值对
pub struct ParameterStatus {
    name: string,
    value: string,
    bytes_read: usize,
}

/// PostgreSQL 事务状态指示器（`ReadyForQuery` 负载字节）
pub enum TransactionStatus: u8 {
    idle = 'I',
    in_transaction = 'T',
    failed_transaction = 'E',

    fn toByte(): u8 {
        return @intFromEnum(_);
    }

    fn isIdle(): bool {
        return _ == .idle;
    }

    fn isInTransaction(): bool {
        return _ == .in_transaction;
    }

    fn isFailed(): bool {
        return _ == .failed_transaction;
    }
}

/// `ReadyForQuery` (`'Z'`) 报文
pub struct ReadyForQuery {
    status: TransactionStatus,
    bytes_read: usize,
}

/// `NotificationResponse` (`'A'`) LISTEN/NOTIFY 异步通知报文
pub struct NotificationResponse {
    pid: u32,
    channel: string,
    payload: string,
    bytes_read: usize,
}

/// `CommandComplete` (`'C'`) SQL 命令完成标签报文
pub struct CommandComplete {
    tag: string,
    bytes_read: usize,

    /// 返回命令动词前缀（如 `"SELECT"`、`"INSERT"`、`"UPDATE"`）
    fn command(): string {
        if (sp := std.mem.indexOfScalar(u8, _.tag, ' ')) {
            return _.tag[0..sp];
        }
        return _.tag;
    }

    /// 解析命令标签末尾的影响行数（如 `"SELECT 42"` -> `42`、`"INSERT 0 5"` -> `5`、`"CREATE TABLE"` -> `null`）
    fn rowsAffected(): u64? {
        if (last_sp := std.mem.lastIndexOfScalar(u8, _.tag, ' ')) {
            const tail = _.tag[last_sp + 1 ..];
            if (tail.len == 0) {
                return null;
            }
            return std.fmt.parseInt(u64, tail, 10) catch return null;
        }
        return null;
    }
}

fn readU32BE(data: []const u8, offset: usize): u32 {
    return ((data[offset] as u32) << 24)
        | ((data[offset + 1] as u32) << 16)
        | ((data[offset + 2] as u32) << 8)
        | (data[offset + 3] as u32);
}

struct FrameView {
    body: []const u8,
    total_frame: usize,
}

fn readFrame(data: []const u8, expected_tag: u8): FrameView ! PgMessageError {
    if (data.len < 5) {
        return err.UnexpectedEof;
    }
    if (data[0] != expected_tag) {
        return err.InvalidMessageTag;
    }
    const len_field = readU32BE(data, 1);
    if (len_field < 4) {
        return err.InvalidMessageLength;
    }
    const total_frame: usize = usize(1) + (len_field as usize);
    if (total_frame > data.len) {
        return err.UnexpectedEof;
    }
    return FrameView{
        body: data[5..total_frame],
        total_frame: total_frame,
    };
}

struct ZStrSlice {
    value: string,
    next_pos: usize,
}

fn readZString(buf: []const u8, start: usize): ZStrSlice ! PgMessageError {
    let pos = start;
    while (pos < buf.len) {
        if (buf[pos] == 0) {
            return ZStrSlice{
                value: buf[start..pos],
                next_pos: pos + 1,
            };
        }
        pos += 1;
    }
    return err.InvalidNullTerminator;
}

fn decodeDiagnosticInternal(data: []const u8, expected_tag: u8): DiagnosticMessage ! PgMessageError {
    const frame = readFrame(data, expected_tag)?;
    const body = frame.body;
    if (body.len == 0) {
        return err.InvalidMessageLength;
    }

    let msg = DiagnosticMessage{
        is_error: expected_tag == TAG_ERROR_RESPONSE,
        bytes_read: frame.total_frame,
    };

    let pos: usize = 0;
    let terminated = false;
    while (pos < body.len) {
        const ftype = body[pos];
        pos += 1;
        if (ftype == 0) {
            terminated = true;
            break;
        }
        const z = readZString(body, pos)?;
        pos = z.next_pos;

        switch (ftype) {
            'S' => { msg.severity = z.value; },
            'V' => { msg.localized_severity = z.value; },
            'C' => { msg.code = z.value; },
            'M' => { msg.message = z.value; },
            'D' => { msg.detail = z.value; },
            'H' => { msg.hint = z.value; },
            'P' => { msg.position = z.value; },
            'p' => { msg.internal_position = z.value; },
            'q' => { msg.internal_query = z.value; },
            'W' => { msg.where_ctx = z.value; },
            's' => { msg.schema = z.value; },
            't' => { msg.table = z.value; },
            'c' => { msg.column = z.value; },
            'd' => { msg.datatype = z.value; },
            'n' => { msg.constraint = z.value; },
            'F' => { msg.file = z.value; },
            'L' => { msg.line = z.value; },
            'R' => { msg.routine = z.value; },
            else => {},
        }
    }

    if (!terminated) {
        return err.InvalidNullTerminator;
    }
    return msg;
}

/// 零拷贝解码 `ErrorResponse` (`'E'`)
pub fn decodeErrorResponse(data: []const u8): DiagnosticMessage ! PgMessageError {
    return decodeDiagnosticInternal(data, TAG_ERROR_RESPONSE);
}

/// 零拷贝解码 `NoticeResponse` (`'N'`)
pub fn decodeNoticeResponse(data: []const u8): DiagnosticMessage ! PgMessageError {
    return decodeDiagnosticInternal(data, TAG_NOTICE_RESPONSE);
}

/// 解码 `BackendKeyData` (`'K'`, 固定长度 12 字节含长度头)
pub fn decodeBackendKeyData(data: []const u8): BackendKeyData ! PgMessageError {
    const frame = readFrame(data, TAG_BACKEND_KEY_DATA)?;
    if (frame.body.len != 8) {
        return err.InvalidMessageLength;
    }
    return BackendKeyData{
        process_id: readU32BE(frame.body, 0),
        secret_key: readU32BE(frame.body, 4),
        bytes_read: frame.total_frame,
    };
}

/// 零拷贝解码 `ParameterStatus` (`'S'`)
pub fn decodeParameterStatus(data: []const u8): ParameterStatus ! PgMessageError {
    const frame = readFrame(data, TAG_PARAMETER_STATUS)?;
    const n = readZString(frame.body, 0)?;
    const v = readZString(frame.body, n.next_pos)?;
    return ParameterStatus{
        name: n.value,
        value: v.value,
        bytes_read: frame.total_frame,
    };
}

/// 解码 `ReadyForQuery` (`'Z'`, 固定长度 5 字节含长度头)
pub fn decodeReadyForQuery(data: []const u8): ReadyForQuery ! PgMessageError {
    const frame = readFrame(data, TAG_READY_FOR_QUERY)?;
    if (frame.body.len != 1) {
        return err.InvalidMessageLength;
    }
    const status = switch (frame.body[0]) {
        'I' => TransactionStatus.idle,
        'T' => TransactionStatus.in_transaction,
        'E' => TransactionStatus.failed_transaction,
        else => return err.InvalidTransactionStatus,
    };
    return ReadyForQuery{
        status: status,
        bytes_read: frame.total_frame,
    };
}

/// 零拷贝解码 `NotificationResponse` (`'A'`)
pub fn decodeNotificationResponse(data: []const u8): NotificationResponse ! PgMessageError {
    const frame = readFrame(data, TAG_NOTIFICATION_RESPONSE)?;
    if (frame.body.len < 6) {
        return err.InvalidMessageLength;
    }
    const pid = readU32BE(frame.body, 0);
    const ch = readZString(frame.body, 4)?;
    const pl = readZString(frame.body, ch.next_pos)?;
    return NotificationResponse{
        pid: pid,
        channel: ch.value,
        payload: pl.value,
        bytes_read: frame.total_frame,
    };
}

/// 零拷贝解码 `CommandComplete` (`'C'`)
pub fn decodeCommandComplete(data: []const u8): CommandComplete ! PgMessageError {
    const frame = readFrame(data, TAG_COMMAND_COMPLETE)?;
    const tag = readZString(frame.body, 0)?;
    return CommandComplete{
        tag: tag.value,
        bytes_read: frame.total_frame,
    };
}
