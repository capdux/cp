// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/FieldDescription.cp | Synced: 2026-11-22T02:41:21Z

/// PostgreSQL Wire Protocol v3.0 版本号（(3 << 16) | 0 = 196608）
pub const PROTOCOL_VERSION_3_0: u32 = 196608;
/// SSLRequest 魔数（(1234 << 16) | 5679 = 80877103）
pub const SSL_REQUEST_CODE: u32 = 80877103;
/// CancelRequest 魔数（(1234 << 16) | 5678 = 80877102）
pub const CANCEL_REQUEST_CODE: u32 = 80877102;

/// PostgreSQL v3 线协议编解码错误集
pub err PgWireError {
    UnexpectedEof,
    InvalidFormatCode,
    InvalidProtocolVersion,
    InvalidMessageLength,
    InvalidNullTerminator,
    InvalidMessageTag,
    EmptyUser,
    BufferTooSmall,
    OutOfMemory,
}

/// 列值编码格式（PostgreSQL v3：0 = text, 1 = binary）
pub enum FormatCode: i16 {
    text = 0,
    binary = 1,

    fn isText(): bool {
        return _ == .text;
    }

    fn isBinary(): bool {
        return _ == .binary;
    }

    fn toInt(): i16 {
        return switch (_) {
            .text => 0,
            .binary => 1,
        };
    }
}

/// 从大端原始 i16 解析 `FormatCode`（仅允许 0/1）。
pub fn formatCodeFromInt(raw: i16): FormatCode ! PgWireError {
    return switch (raw) {
        0 => FormatCode.text,
        1 => FormatCode.binary,
        else => return err.InvalidFormatCode,
    };
}

/// PostgreSQL v3 前后端线协议报文类型字节枚举（ASCII 字节值）
/// 说明：部分字节在前端与后端上下文中复用：
/// - 'C': 后端 CommandComplete / 前端 Close
/// - 'D': 后端 DataRow / 前端 Describe
/// - 'E': 后端 ErrorResponse / 前端 Execute
/// - 'H': 后端 CopyOutResponse / 前端 Flush
/// - 'S': 后端 ParameterStatus / 前端 Sync
pub enum MessageTag: u8 {
    Authentication = 'R',
    BackendKeyData = 'K',
    Bind = 'B',
    BindComplete = '2',
    CloseComplete = '3',
    CommandComplete = 'C',
    CopyData = 'd',
    CopyDone = 'c',
    CopyFail = 'f',
    CopyInResponse = 'G',
    CopyOutResponse = 'H',
    DataRow = 'D',
    EmptyQueryResponse = 'I',
    ErrorResponse = 'E',
    NegotiateProtocolVersion = 'v',
    NoData = 'n',
    NoticeResponse = 'N',
    NotificationResponse = 'A',
    ParameterDescription = 't',
    ParameterStatus = 'S',
    Parse = 'P',
    ParseComplete = '1',
    PasswordMessage = 'p',
    PortalSuspended = 's',
    Query = 'Q',
    ReadyForQuery = 'Z',
    RowDescription = 'T',
    Terminate = 'X',

    fn toByte(): u8 {
        return @intFromEnum(_);
    }

    fn isFrontend(): bool {
        return switch (_) {
            .Bind,
            .CommandComplete,
            .CopyData,
            .CopyDone,
            .CopyFail,
            .CopyOutResponse,
            .DataRow,
            .ErrorResponse,
            .ParameterStatus,
            .Parse,
            .PasswordMessage,
            .Query,
            .Terminate => true,
            else => false,
        };
    }

    fn isBackend(): bool {
        return switch (_) {
            .Authentication,
            .BackendKeyData,
            .BindComplete,
            .CloseComplete,
            .CommandComplete,
            .CopyData,
            .CopyDone,
            .CopyInResponse,
            .CopyOutResponse,
            .DataRow,
            .EmptyQueryResponse,
            .ErrorResponse,
            .NegotiateProtocolVersion,
            .NoData,
            .NoticeResponse,
            .NotificationResponse,
            .ParameterDescription,
            .ParameterStatus,
            .ParseComplete,
            .PortalSuspended,
            .ReadyForQuery,
            .RowDescription => true,
            else => false,
        };
    }
}

/// 从单字节解析标准 PostgreSQL v3 `MessageTag`。
pub fn messageTagFromByte(b: u8): MessageTag ! PgWireError {
    return switch (b) {
        'R' => MessageTag.Authentication,
        'K' => MessageTag.BackendKeyData,
        'B' => MessageTag.Bind,
        '2' => MessageTag.BindComplete,
        '3' => MessageTag.CloseComplete,
        'C' => MessageTag.CommandComplete,
        'd' => MessageTag.CopyData,
        'c' => MessageTag.CopyDone,
        'f' => MessageTag.CopyFail,
        'G' => MessageTag.CopyInResponse,
        'H' => MessageTag.CopyOutResponse,
        'D' => MessageTag.DataRow,
        'I' => MessageTag.EmptyQueryResponse,
        'E' => MessageTag.ErrorResponse,
        'v' => MessageTag.NegotiateProtocolVersion,
        'n' => MessageTag.NoData,
        'N' => MessageTag.NoticeResponse,
        'A' => MessageTag.NotificationResponse,
        't' => MessageTag.ParameterDescription,
        'S' => MessageTag.ParameterStatus,
        'P' => MessageTag.Parse,
        '1' => MessageTag.ParseComplete,
        'p' => MessageTag.PasswordMessage,
        's' => MessageTag.PortalSuspended,
        'Q' => MessageTag.Query,
        'Z' => MessageTag.ReadyForQuery,
        'T' => MessageTag.RowDescription,
        'X' => MessageTag.Terminate,
        else => return err.InvalidMessageTag,
    };
}

/// 带 RAII 析构的线协议字节缓冲区（自动释放内存，防泄漏）
pub struct WireBuffer {
    data: []u8,
    allocator: std.mem.Allocator,

    fn slice(): []const u8 {
        return _.data;
    }

    fn len(): usize {
        return _.data.len;
    }

    ~init() {
        if (_.data.len > 0) {
            _.allocator.free(_.data);
            _.data = [];
        }
    }
}

/// 读取大端 u16
pub fn readU16BE(buf: []const u8, offset: usize): u16 ! PgWireError {
    if (offset + 2 > buf.len) {
        return err.UnexpectedEof;
    }
    return ((buf[offset] as u16) << 8) | (buf[offset + 1] as u16);
}

/// 读取大端 i16
pub fn readI16BE(buf: []const u8, offset: usize): i16 ! PgWireError {
    const raw = readU16BE(buf, offset)?;
    const signed_val: i16 = @bitCast(raw);
    return signed_val;
}

/// 读取大端 u32
pub fn readU32BE(buf: []const u8, offset: usize): u32 ! PgWireError {
    if (offset + 4 > buf.len) {
        return err.UnexpectedEof;
    }
    return ((buf[offset] as u32) << 24)
        | ((buf[offset + 1] as u32) << 16)
        | ((buf[offset + 2] as u32) << 8)
        | (buf[offset + 3] as u32);
}

/// 读取大端 i32
pub fn readI32BE(buf: []const u8, offset: usize): i32 ! PgWireError {
    const raw = readU32BE(buf, offset)?;
    const signed_val: i32 = @bitCast(raw);
    return signed_val;
}

/// 写入大端 u16
pub fn writeU16BE(buf: []u8, offset: usize, val: u16): usize ! PgWireError {
    if (offset + 2 > buf.len) {
        return err.BufferTooSmall;
    }
    buf[offset] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 1] = (val & 0xFF) as u8;
    return offset + 2;
}

/// 写入大端 i16
pub fn writeI16BE(buf: []u8, offset: usize, val: i16): usize ! PgWireError {
    const raw: u16 = @bitCast(val);
    return writeU16BE(buf, offset, raw);
}

/// 写入大端 u32
pub fn writeU32BE(buf: []u8, offset: usize, val: u32): usize ! PgWireError {
    if (offset + 4 > buf.len) {
        return err.BufferTooSmall;
    }
    buf[offset] = ((val >> 24) & 0xFF) as u8;
    buf[offset + 1] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 3] = (val & 0xFF) as u8;
    return offset + 4;
}

/// 写入大端 i32
pub fn writeI32BE(buf: []u8, offset: usize, val: i32): usize ! PgWireError {
    const raw: u32 = @bitCast(val);
    return writeU32BE(buf, offset, raw);
}

/// 常规报文帧头（1 字节 MessageTag + 4 字节大端长度）
pub struct MessageHeader {
    tag: MessageTag,
    body_len: u32,
}

/// 编码 5 字节报文帧头到目标缓冲区
pub fn encodeMessageHeader(buf: []u8, tag: MessageTag, body_len: u32): usize ! PgWireError {
    if (buf.len < 5) {
        return err.BufferTooSmall;
    }
    buf[0] = tag.toByte();
    const total_len: u32 = body_len + 4;
    return writeU32BE(buf, 1, total_len);
}

/// 从字节流解码 5 字节报文帧头
pub fn decodeMessageHeader(data: []const u8): MessageHeader ! PgWireError {
    if (data.len < 5) {
        return err.UnexpectedEof;
    }
    const tag = messageTagFromByte(data[0])?;
    const total_len = readU32BE(data, 1)?;
    if (total_len < 4) {
        return err.InvalidMessageLength;
    }
    return MessageHeader{
        tag: tag,
        body_len: total_len - 4,
    };
}

/// RowDescription 单列字段元数据（PostgreSQL v3）
pub struct FieldDescription {
    name: string,
    table_oid: i32 = 0,
    column_index: i16 = 0,
    type_oid: i32 = 0,
    type_size: i16 = -1,
    type_modifier: i32 = -1,
    format: FormatCode = FormatCode.text,

    fn isBinary(): bool {
        return _.format == FormatCode.binary;
    }

    fn encodedSize(): usize {
        return _.name.len + 1 + 18;
    }
}

/// 单列字段解码结果（包含消耗字节数，便于连续解码 RowDescription 多列）
pub struct DecodedField {
    field: FieldDescription,
    bytes_read: usize,
}

/// 将 `FieldDescription` 编码为大端字节流（零拷贝分配到 `WireBuffer`）。
pub fn encodeFieldDescription(
    allocator: std.mem.Allocator,
    field: FieldDescription,
): WireBuffer ! PgWireError {
    const total = field.encodedSize();
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    errdefer allocator.free(buf);

    let pos: usize = 0;
    if (field.name.len > 0) {
        @memcpy(buf[0..field.name.len], field.name);
        pos = field.name.len;
    }
    buf[pos] = 0;
    pos += 1;

    pos = writeI32BE(buf, pos, field.table_oid)?;
    pos = writeI16BE(buf, pos, field.column_index)?;
    pos = writeI32BE(buf, pos, field.type_oid)?;
    pos = writeI16BE(buf, pos, field.type_size)?;
    pos = writeI32BE(buf, pos, field.type_modifier)?;
    pos = writeI16BE(buf, pos, field.format.toInt())?;

    return WireBuffer{
        data: buf,
        allocator: allocator,
    };
}

/// 从大端字节流解码 `FieldDescription`（零分配切片引用 `data` 中的列名）。
pub fn decodeFieldDescription(data: []const u8): DecodedField ! PgWireError {
    // 查找 NUL 终止符
    let nul_pos: usize = 0;
    let found_nul: bool = false;
    while (nul_pos < data.len) {
        if (data[nul_pos] == 0) {
            found_nul = true;
            break;
        }
        nul_pos += 1;
    }
    if (!found_nul) {
        return err.InvalidNullTerminator;
    }

    const meta_offset = nul_pos + 1;
    if (meta_offset + 18 > data.len) {
        return err.UnexpectedEof;
    }

    const name: string = data[0..nul_pos];
    const table_oid = readI32BE(data, meta_offset)?;
    const column_index = readI16BE(data, meta_offset + 4)?;
    const type_oid = readI32BE(data, meta_offset + 6)?;
    const type_size = readI16BE(data, meta_offset + 10)?;
    const type_modifier = readI32BE(data, meta_offset + 12)?;
    const raw_format = readI16BE(data, meta_offset + 16)?;
    const format = formatCodeFromInt(raw_format)?;

    return DecodedField{
        field: FieldDescription{
            name: name,
            table_oid: table_oid,
            column_index: column_index,
            type_oid: type_oid,
            type_size: type_size,
            type_modifier: type_modifier,
            format: format,
        },
        bytes_read: meta_offset + 18,
    };
}

/// StartupMessage 键值对参数
pub struct StartupParam {
    key: string,
    value: string,
}

/// PostgreSQL v3.0 StartupMessage 描述结构
pub struct StartupMessage {
    user: string,
    database: string = "",
    client_encoding: string = "UTF8",
    application_name: string = "",
    extra_params: []const StartupParam = [],
}

/// 解码后的 StartupMessage（携带 RAII 自动释放的参数数组）
pub struct DecodedStartupMessage {
    protocol_version: u32,
    total_length: u32,
    params: []StartupParam,
    allocator: std.mem.Allocator,

    fn paramCount(): usize {
        return _.params.len;
    }

    fn getParam(key: string): string? {
        for (p of _.params) {
            if (std.mem.eql(u8, p.key, key)) {
                return p.value;
            }
        }
        return null;
    }

    ~init() {
        if (_.params.len > 0) {
            _.allocator.free(_.params);
            _.params = [];
        }
    }
}

fn zPairLen(key: string, value: string): usize {
    return key.len + 1 + value.len + 1;
}

fn writeZString(buf: []u8, offset: usize, s: string): usize {
    if (s.len > 0) {
        @memcpy(buf[offset .. offset + s.len], s);
    }
    buf[offset + s.len] = 0;
    return offset + s.len + 1;
}

/// 将 `StartupMessage` 编码为 PostgreSQL v3.0 大端启动包（含 4 字节总长 + 4 字节版本号 196608 + NUL 分隔参数对 + 尾随 NUL）。
pub fn encodeStartupMessage(
    allocator: std.mem.Allocator,
    msg: StartupMessage,
): WireBuffer ! PgWireError {
    if (msg.user.len == 0) {
        return err.EmptyUser;
    }

    const db_name: string = if (msg.database.len == 0) msg.user else msg.database;

    // 计算总字节长度：4 (length) + 4 (protocol v3.0) + params + 1 (final NUL)
    let total: usize = usize(8) + zPairLen("user", msg.user) + zPairLen("database", db_name) + usize(1);
    if (msg.client_encoding.len > 0) {
        total += zPairLen("client_encoding", msg.client_encoding);
    }
    if (msg.application_name.len > 0) {
        total += zPairLen("application_name", msg.application_name);
    }
    for (p of msg.extra_params) {
        total += zPairLen(p.key, p.value);
    }

    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    errdefer allocator.free(buf);

    let pos: usize = 0;
    pos = writeU32BE(buf, pos, total as u32)?;
    pos = writeU32BE(buf, pos, PROTOCOL_VERSION_3_0)?;

    pos = writeZString(buf, pos, "user");
    pos = writeZString(buf, pos, msg.user);

    pos = writeZString(buf, pos, "database");
    pos = writeZString(buf, pos, db_name);

    if (msg.client_encoding.len > 0) {
        pos = writeZString(buf, pos, "client_encoding");
        pos = writeZString(buf, pos, msg.client_encoding);
    }
    if (msg.application_name.len > 0) {
        pos = writeZString(buf, pos, "application_name");
        pos = writeZString(buf, pos, msg.application_name);
    }
    for (p of msg.extra_params) {
        pos = writeZString(buf, pos, p.key);
        pos = writeZString(buf, pos, p.value);
    }

    buf[pos] = 0;
    return WireBuffer{
        data: buf,
        allocator: allocator,
    };
}

/// 从大端字节流解码 PostgreSQL v3.0 `StartupMessage`（严格校验长度、版本号 196608 与 NUL 终止符）。
pub fn decodeStartupMessage(
    allocator: std.mem.Allocator,
    data: []const u8,
): DecodedStartupMessage ! PgWireError {
    if (data.len < 9) {
        return err.UnexpectedEof;
    }

    const total_len = readU32BE(data, 0)?;
    if (total_len < 9) {
        return err.InvalidMessageLength;
    }
    if ((total_len as usize) > data.len) {
        return err.UnexpectedEof;
    }

    const version = readU32BE(data, 4)?;
    if (version != PROTOCOL_VERSION_3_0) {
        return err.InvalidProtocolVersion;
    }

    const end: usize = total_len as usize;
    if (data[end - 1] != 0) {
        return err.InvalidNullTerminator;
    }

    // 第一遍扫描统计参数对数量并验证格式完整性
    let pos: usize = 8;
    let count: usize = 0;
    while (pos < end) {
        if (data[pos] == 0) {
            if (pos + 1 != end) {
                return err.InvalidMessageLength;
            }
            break;
        }
        // key
        while (pos < end && data[pos] != 0) {
            pos += 1;
        }
        if (pos >= end) {
            return err.InvalidNullTerminator;
        }
        pos += 1; // skip NUL after key
        if (pos >= end) {
            return err.UnexpectedEof;
        }
        // value
        while (pos < end && data[pos] != 0) {
            pos += 1;
        }
        if (pos >= end) {
            return err.InvalidNullTerminator;
        }
        pos += 1; // skip NUL after value
        count += 1;
    }

    let params = allocator.alloc(StartupParam, count) catch return err.OutOfMemory;
    errdefer allocator.free(params);

    // 第二遍提取零拷贝键值对切片
    pos = 8;
    let idx: usize = 0;
    while (idx < count) {
        const key_start = pos;
        while (data[pos] != 0) {
            pos += 1;
        }
        const key: string = data[key_start..pos];
        pos += 1;

        const val_start = pos;
        while (data[pos] != 0) {
            pos += 1;
        }
        const val: string = data[val_start..pos];
        pos += 1;

        params[idx] = StartupParam{
            key: key,
            value: val,
        };
        idx += 1;
    }

    return DecodedStartupMessage{
        protocol_version: version,
        total_length: total_len,
        params: params,
        allocator: allocator,
    };
}
