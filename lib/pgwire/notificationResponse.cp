// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/NotificationResponse.cp | Synced: 2026-11-23T01:32:18Z
// Pure-CP zero-allocation PostgreSQL v3.0 `NotificationResponse` ('A') LISTEN/NOTIFY message codec.

/// PostgreSQL Backend `NotificationResponse` 报文类型字节（`'A' = 0x41`）
pub const PG_TAG_NOTIFICATION_RESPONSE: u8 = 'A';

/// PostgreSQL `NOTIFY` 默认最大负载字节数（官方文档 8000 字节）
pub const PG_NOTIFY_DEFAULT_MAX_PAYLOAD: usize = 8000;

/// `NotificationResponse` 编解码与边界校验错误集
pub err PgNotificationError {
    InvalidTag,
    InvalidLength,
    Truncated,
    MissingNulTerminator,
    EmbeddedNul,
    EmptyChannel,
    TrailingBytes,
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
    return @bitCast(readBeU32(buf, off));
}

fn writeBeU32(out: []u8, off: usize, v: u32) {
    out[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    out[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    out[off + 3] = @as(u8, @intCast(v & 0xFF));
}

fn writeBeI32(out: []u8, off: usize, v: i32) {
    const raw: u32 = @bitCast(v);
    writeBeU32(out, off, raw);
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) return true;
        i += 1;
    }
    return false;
}

fn findNul(buf: []const u8, start: usize): usize? {
    let i = start;
    while (i < buf.len) {
        if (buf[i] == 0) return i;
        i += 1;
    }
    return null;
}

fn copyBytes(out: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        out[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

/// PostgreSQL v3.0 `NotificationResponse` 服务端异步通知报文（Tag `'A'`）
///
/// 线协议格式（大端序）：
/// - `Byte1('A')`：报文类型标识
/// - `Int32`：包含自身在内的报文长度（`= 4 + 4 + channel.len + 1 + payload.len + 1`）
/// - `Int32`：触发 `NOTIFY` 的后端进程 ID（`pid`）
/// - `String`：以 `0x00` 结尾的通知频道名（`channel`）
/// - `String`：以 `0x00` 结尾的附加负载字符串（`payload`，允许为空串）
pub struct PgNotificationResponse {
    pid: i32 = 0,
    channel: []const u8 = "",
    payload: []const u8 = "",

    static fn init(pid: i32, channel: []const u8, payload: []const u8): PgNotificationResponse {
        return PgNotificationResponse{
            pid: pid,
            channel: channel,
            payload: payload,
        };
    }

    fn hasPayload(): bool {
        return _.payload.len > 0;
    }

    fn isChannel(target: []const u8): bool {
        return bytesEql(_.channel, target);
    }

    fn isWithinDefaultPayloadLimit(): bool {
        return _.payload.len <= PG_NOTIFY_DEFAULT_MAX_PAYLOAD;
    }

    fn encodedPayloadLen(): usize {
        return 4 + _.channel.len + 1 + _.payload.len + 1;
    }

    fn encodedFrameLen(): usize {
        return 1 + 4 + _.encodedPayloadLen();
    }

    fn validate(): PgNotificationError!bool {
        if (_.channel.len == 0) return PgNotificationError.EmptyChannel;
        if (containsNul(_.channel) || containsNul(_.payload)) return PgNotificationError.EmbeddedNul;
        return true;
    }

    /// 零堆分配编码 `NotificationResponse` 载荷（不含 1 字节 Tag 与 4 字节 Length）。
    fn encodePayload(out: []u8): PgNotificationError!usize {
        * = _.validate()?;
        const need = _.encodedPayloadLen();
        if (out.len < need) return PgNotificationError.BufferTooSmall;
        writeBeI32(out, 0, _.pid);
        let pos: usize = 4;
        pos = copyBytes(out, pos, _.channel);
        out[pos] = 0;
        pos += 1;
        pos = copyBytes(out, pos, _.payload);
        out[pos] = 0;
        pos += 1;
        return pos;
    }

    /// 零堆分配编码完整 `NotificationResponse` 帧（`'A'` + Int32 length + payload）。
    fn encodeFrame(out: []u8): PgNotificationError!usize {
        * = _.validate()?;
        const payload_len = _.encodedPayloadLen();
        const total = 1 + 4 + payload_len;
        if (out.len < total) return PgNotificationError.BufferTooSmall;
        const wire_len: u32 = @as(u32, @intCast(4 + payload_len));
        out[0] = PG_TAG_NOTIFICATION_RESPONSE;
        writeBeU32(out, 1, wire_len);
        const written = _.encodePayload(out[5..])?;
        return 5 + written;
    }

    /// 零拷贝解码 `NotificationResponse` 载荷（切片直接借用 `payload`）。
    static fn decodePayload(payload: []const u8): PgNotificationError!PgNotificationResponse {
        // 最小载荷：4 字节 pid + 至少 1 字节 channel + NUL + NUL = 6 字节（空 channel 单独报 EmptyChannel）
        if (payload.len < 6) return PgNotificationError.Truncated;
        const pid = readBeI32(payload, 0);
        const ch_nul = findNul(payload, 4) ?? return PgNotificationError.MissingNulTerminator;
        const channel = payload[4..ch_nul];
        if (channel.len == 0) return PgNotificationError.EmptyChannel;
        const pl_start = ch_nul + 1;
        if (pl_start >= payload.len) return PgNotificationError.MissingNulTerminator;
        const pl_nul = findNul(payload, pl_start) ?? return PgNotificationError.MissingNulTerminator;
        if (pl_nul + 1 != payload.len) return PgNotificationError.TrailingBytes;
        const msg_payload = payload[pl_start..pl_nul];
        return PgNotificationResponse{
            pid: pid,
            channel: channel,
            payload: msg_payload,
        };
    }

    /// 零拷贝解码完整 `NotificationResponse` 帧（严格校验 Tag `'A'`、Int32 长度与无尾随字节）。
    static fn decodeFrame(frame: []const u8): PgNotificationError!PgNotificationResponse {
        if (frame.len < 5) return PgNotificationError.Truncated;
        if (frame[0] != PG_TAG_NOTIFICATION_RESPONSE) return PgNotificationError.InvalidTag;
        const wire_len = readBeU32(frame, 1);
        if (wire_len < 10) return PgNotificationError.InvalidLength;
        const total: usize = (@as(usize, wire_len)) + 1;
        if (frame.len < total) return PgNotificationError.Truncated;
        if (frame.len > total) return PgNotificationError.TrailingBytes;
        return PgNotificationResponse.decodePayload(frame[5..total]);
    }

    fn eql(other: PgNotificationResponse): bool {
        return _.pid == other.pid && bytesEql(_.channel, other.channel) && bytesEql(_.payload, other.payload);
    }
}
