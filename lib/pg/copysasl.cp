// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/CopyData.cp | Synced: 2026-11-21T16:59:35Z

/// PostgreSQL v3 COPY / SASL / 协议协商报文类型字节常量
pub const TAG_COPY_DATA: u8 = 'd';
pub const TAG_COPY_DONE: u8 = 'c';
pub const TAG_COPY_FAIL: u8 = 'f';
pub const TAG_COPY_IN_RESPONSE: u8 = 'G';
pub const TAG_COPY_OUT_RESPONSE: u8 = 'H';
pub const TAG_PASSWORD_OR_SASL: u8 = 'p';
pub const TAG_NEGOTIATE_PROTOCOL_VERSION: u8 = 'v';

/// 编解码错误集
pub err PgCopySaslError {
    UnexpectedEof,
    InvalidMessageTag,
    InvalidMessageLength,
    InvalidCopyFormat,
    InvalidNullTerminator,
    InvalidDataLength,
    EmptyMechanism,
    OutOfMemory,
}

/// 带 RAII `~init()` 析构的线协议帧缓冲区
pub struct PgFrameBuffer {
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

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readI32BE(data: []const u8, off: usize): i32 {
    const raw = readU32BE(data, off);
    const v: i32 = @bitCast(raw);
    return v;
}

fn writeU16BE(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeI32BE(buf: []u8, off: usize, v: i32) {
    const raw: u32 = @bitCast(v);
    writeU32BE(buf, off, raw);
}

struct FrameView {
    body: []const u8,
    total_frame: usize,
}

fn readFrame(data: []const u8, expected_tag: u8): FrameView ! PgCopySaslError {
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

struct ZSlice {
    value: string,
    next_pos: usize,
}

fn readZString(buf: []const u8, start: usize): ZSlice ! PgCopySaslError {
    let pos = start;
    while (pos < buf.len) {
        if (buf[pos] == 0) {
            return ZSlice{ value: buf[start..pos], next_pos: pos + 1 };
        }
        pos += 1;
    }
    return err.InvalidNullTerminator;
}

/// COPY 数据格式（0 = text, 1 = binary）
pub enum CopyFormat: u8 {
    text = 0,
    binary = 1,

    fn isText(): bool {
        return _ == .text;
    }

    fn isBinary(): bool {
        return _ == .binary;
    }

    fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

pub fn copyFormatFromByte(b: u8): CopyFormat ! PgCopySaslError {
    return switch (b) {
        0 => CopyFormat.text,
        1 => CopyFormat.binary,
        else => return err.InvalidCopyFormat,
    };
}

/// COPY 方向
pub enum CopyDirection: u8 {
    copy_in = 0,
    copy_out = 1,
}

/// `CopyInResponse` (`'G'`) / `CopyOutResponse` (`'H'`) 解析结果（含 RAII `~init()`）
pub struct CopyResponse {
    direction: CopyDirection,
    overall_format: CopyFormat,
    column_formats: []CopyFormat,
    bytes_read: usize,
    allocator: std.mem.Allocator,

    fn columnCount(): usize {
        return _.column_formats.len;
    }

    ~init() {
        if (_.column_formats.len > 0) {
            _.allocator.free(_.column_formats);
            _.column_formats = [];
        }
    }
}

/// 编码 `CopyInResponse` (`'G'`) 或 `CopyOutResponse` (`'H'`)
pub fn encodeCopyResponse(
    allocator: std.mem.Allocator,
    direction: CopyDirection,
    overall_format: CopyFormat,
    column_formats: []const CopyFormat,
): PgFrameBuffer ! PgCopySaslError {
    const body_len: usize = usize(3) + column_formats.len * usize(2);
    const total: usize = usize(5) + body_len;
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    errdefer allocator.free(buf);

    buf[0] = if (direction == CopyDirection.copy_in) TAG_COPY_IN_RESPONSE else TAG_COPY_OUT_RESPONSE;
    writeU32BE(buf, 1, (body_len + usize(4)) as u32);
    buf[5] = overall_format.toByte();
    writeU16BE(buf, 6, column_formats.len as u16);

    let pos: usize = 8;
    for (cf of column_formats) {
        writeU16BE(buf, pos, cf.toByte() as u16);
        pos += 2;
    }
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

fn decodeCopyResponseInternal(
    allocator: std.mem.Allocator,
    data: []const u8,
    direction: CopyDirection,
): CopyResponse ! PgCopySaslError {
    const expected_tag = if (direction == CopyDirection.copy_in) TAG_COPY_IN_RESPONSE else TAG_COPY_OUT_RESPONSE;
    const frame = readFrame(data, expected_tag)?;
    if (frame.body.len < 3) {
        return err.InvalidMessageLength;
    }
    const overall = copyFormatFromByte(frame.body[0])?;
    const ncols: usize = readU16BE(frame.body, 1) as usize;
    if (frame.body.len != usize(3) + ncols * usize(2)) {
        return err.InvalidMessageLength;
    }

    let cols = allocator.alloc(CopyFormat, ncols) catch return err.OutOfMemory;
    errdefer allocator.free(cols);

    let i: usize = 0;
    while (i < ncols) {
        const raw = readU16BE(frame.body, usize(3) + i * usize(2));
        if (raw > 1) {
            return err.InvalidCopyFormat;
        }
        cols[i] = copyFormatFromByte(raw as u8)?;
        i += 1;
    }

    return CopyResponse{
        direction: direction,
        overall_format: overall,
        column_formats: cols,
        bytes_read: frame.total_frame,
        allocator: allocator,
    };
}

pub fn decodeCopyInResponse(allocator: std.mem.Allocator, data: []const u8): CopyResponse ! PgCopySaslError {
    return decodeCopyResponseInternal(allocator, data, CopyDirection.copy_in);
}

pub fn decodeCopyOutResponse(allocator: std.mem.Allocator, data: []const u8): CopyResponse ! PgCopySaslError {
    return decodeCopyResponseInternal(allocator, data, CopyDirection.copy_out);
}

/// `CopyData` (`'d'`)
pub struct CopyData {
    payload: []const u8,
    bytes_read: usize,
}

pub fn encodeCopyData(allocator: std.mem.Allocator, payload: []const u8): PgFrameBuffer ! PgCopySaslError {
    const total: usize = usize(5) + payload.len;
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    buf[0] = TAG_COPY_DATA;
    writeU32BE(buf, 1, (payload.len + usize(4)) as u32);
    if (payload.len > 0) {
        @memcpy(buf[5..total], payload);
    }
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodeCopyData(data: []const u8): CopyData ! PgCopySaslError {
    const frame = readFrame(data, TAG_COPY_DATA)?;
    return CopyData{
        payload: frame.body,
        bytes_read: frame.total_frame,
    };
}

/// `CopyDone` (`'c'`, 固定 5 字节)
pub fn encodeCopyDone(allocator: std.mem.Allocator): PgFrameBuffer ! PgCopySaslError {
    let buf = allocator.alloc(u8, 5) catch return err.OutOfMemory;
    buf[0] = TAG_COPY_DONE;
    writeU32BE(buf, 1, 4);
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodeCopyDone(data: []const u8): usize ! PgCopySaslError {
    const frame = readFrame(data, TAG_COPY_DONE)?;
    if (frame.body.len != 0) {
        return err.InvalidMessageLength;
    }
    return frame.total_frame;
}

/// `CopyFail` (`'f'`)
pub struct CopyFail {
    message: string,
    bytes_read: usize,
}

pub fn encodeCopyFail(allocator: std.mem.Allocator, message: string): PgFrameBuffer ! PgCopySaslError {
    const total: usize = usize(5) + message.len + usize(1);
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    buf[0] = TAG_COPY_FAIL;
    writeU32BE(buf, 1, (message.len + usize(5)) as u32);
    if (message.len > 0) {
        @memcpy(buf[5 .. 5 + message.len], message);
    }
    buf[5 + message.len] = 0;
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodeCopyFail(data: []const u8): CopyFail ! PgCopySaslError {
    const frame = readFrame(data, TAG_COPY_FAIL)?;
    const msg = readZString(frame.body, 0)?;
    return CopyFail{
        message: msg.value,
        bytes_read: frame.total_frame,
    };
}

/// `PasswordMessage` (`'p'`)
pub struct DecodedPasswordMessage {
    password: string,
    bytes_read: usize,

    fn isMd5Hash(): bool {
        return _.password.len == 35 && std.mem.eql(u8, _.password[0..3], "md5");
    }
}

pub fn encodePasswordMessage(allocator: std.mem.Allocator, password: string): PgFrameBuffer ! PgCopySaslError {
    const total: usize = usize(5) + password.len + usize(1);
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    buf[0] = TAG_PASSWORD_OR_SASL;
    writeU32BE(buf, 1, (password.len + usize(5)) as u32);
    if (password.len > 0) {
        @memcpy(buf[5 .. 5 + password.len], password);
    }
    buf[5 + password.len] = 0;
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodePasswordMessage(data: []const u8): DecodedPasswordMessage ! PgCopySaslError {
    const frame = readFrame(data, TAG_PASSWORD_OR_SASL)?;
    const pw = readZString(frame.body, 0)?;
    return DecodedPasswordMessage{
        password: pw.value,
        bytes_read: frame.total_frame,
    };
}

/// `SASLInitialResponse` (`'p'`)
pub struct DecodedSaslInitialResponse {
    mechanism: string,
    client_first: ([]const u8)?,
    bytes_read: usize,
}

pub fn encodeSaslInitialResponse(
    allocator: std.mem.Allocator,
    mechanism: string,
    client_first: ([]const u8)?,
): PgFrameBuffer ! PgCopySaslError {
    if (mechanism.len == 0) {
        return err.EmptyMechanism;
    }
    let payload_len: usize = 0;
    if (cf := client_first) {
        payload_len = cf.len;
    }
    const body_len: usize = mechanism.len + usize(1) + usize(4) + payload_len;
    const total: usize = usize(5) + body_len;
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;

    buf[0] = TAG_PASSWORD_OR_SASL;
    writeU32BE(buf, 1, (body_len + usize(4)) as u32);
    @memcpy(buf[5 .. 5 + mechanism.len], mechanism);
    let pos: usize = usize(5) + mechanism.len;
    buf[pos] = 0;
    pos += 1;

    if (cf := client_first) {
        writeI32BE(buf, pos, cf.len as i32);
        pos += 4;
        if (cf.len > 0) {
            @memcpy(buf[pos .. pos + cf.len], cf);
        }
    } else {
        writeI32BE(buf, pos, -1);
    }
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodeSaslInitialResponse(data: []const u8): DecodedSaslInitialResponse ! PgCopySaslError {
    const frame = readFrame(data, TAG_PASSWORD_OR_SASL)?;
    const mech = readZString(frame.body, 0)?;
    if (mech.value.len == 0) {
        return err.EmptyMechanism;
    }
    if (mech.next_pos + 4 > frame.body.len) {
        return err.InvalidMessageLength;
    }
    const data_len = readI32BE(frame.body, mech.next_pos);
    const data_start = mech.next_pos + 4;
    if (data_len == -1) {
        if (data_start != frame.body.len) {
            return err.InvalidDataLength;
        }
        return DecodedSaslInitialResponse{
            mechanism: mech.value,
            client_first: null,
            bytes_read: frame.total_frame,
        };
    }
    if (data_len < 0) {
        return err.InvalidDataLength;
    }
    const ulen: usize = data_len as usize;
    if (data_start + ulen != frame.body.len) {
        return err.InvalidDataLength;
    }
    return DecodedSaslInitialResponse{
        mechanism: mech.value,
        client_first: frame.body[data_start .. data_start + ulen],
        bytes_read: frame.total_frame,
    };
}

/// `SASLResponse` (`'p'`)
pub struct DecodedSaslResponse {
    payload: []const u8,
    bytes_read: usize,
}

pub fn encodeSaslResponse(allocator: std.mem.Allocator, payload: []const u8): PgFrameBuffer ! PgCopySaslError {
    const total: usize = usize(5) + payload.len;
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    buf[0] = TAG_PASSWORD_OR_SASL;
    writeU32BE(buf, 1, (payload.len + usize(4)) as u32);
    if (payload.len > 0) {
        @memcpy(buf[5..total], payload);
    }
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodeSaslResponse(data: []const u8): DecodedSaslResponse ! PgCopySaslError {
    const frame = readFrame(data, TAG_PASSWORD_OR_SASL)?;
    return DecodedSaslResponse{
        payload: frame.body,
        bytes_read: frame.total_frame,
    };
}

/// `NegotiateProtocolVersion` (`'v'`) 解析结果（含 RAII `~init()`）
pub struct NegotiateProtocolVersion {
    newest_minor_version: u32,
    unrecognized_options: []string,
    bytes_read: usize,
    allocator: std.mem.Allocator,

    fn optionCount(): usize {
        return _.unrecognized_options.len;
    }

    ~init() {
        if (_.unrecognized_options.len > 0) {
            _.allocator.free(_.unrecognized_options);
            _.unrecognized_options = [];
        }
    }
}

pub fn encodeNegotiateProtocolVersion(
    allocator: std.mem.Allocator,
    newest_minor_version: u32,
    options: []const string,
): PgFrameBuffer ! PgCopySaslError {
    let body_len: usize = 8;
    for (opt of options) {
        body_len += opt.len + usize(1);
    }
    const total: usize = usize(5) + body_len;
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    buf[0] = TAG_NEGOTIATE_PROTOCOL_VERSION;
    writeU32BE(buf, 1, (body_len + usize(4)) as u32);
    writeU32BE(buf, 5, newest_minor_version);
    writeU32BE(buf, 9, options.len as u32);

    let pos: usize = 13;
    for (opt of options) {
        if (opt.len > 0) {
            @memcpy(buf[pos .. pos + opt.len], opt);
            pos += opt.len;
        }
        buf[pos] = 0;
        pos += 1;
    }
    return PgFrameBuffer{ data: buf, allocator: allocator };
}

pub fn decodeNegotiateProtocolVersion(
    allocator: std.mem.Allocator,
    data: []const u8,
): NegotiateProtocolVersion ! PgCopySaslError {
    const frame = readFrame(data, TAG_NEGOTIATE_PROTOCOL_VERSION)?;
    if (frame.body.len < 8) {
        return err.InvalidMessageLength;
    }
    const ver = readU32BE(frame.body, 0);
    const count: usize = readU32BE(frame.body, 4) as usize;

    let opts = allocator.alloc(string, count) catch return err.OutOfMemory;
    errdefer allocator.free(opts);

    let pos: usize = 8;
    let i: usize = 0;
    while (i < count) {
        const z = readZString(frame.body, pos)?;
        opts[i] = z.value;
        pos = z.next_pos;
        i += 1;
    }

    return NegotiateProtocolVersion{
        newest_minor_version: ver,
        unrecognized_options: opts,
        bytes_read: frame.total_frame,
        allocator: allocator,
    };
}
