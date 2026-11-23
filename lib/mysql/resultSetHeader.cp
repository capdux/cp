// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/ResultSetHeader.cp | Synced: 2026-11-23T01:34:24Z
// Pure-CP zero-allocation MySQL ResultSetHeader (lenenc column count + optional metadata_follows),
// query response packet classifier, and AuthSwitchResponse wire frame codec.

/// MySQL 服务端响应首字节判别常量（对齐 Bun `PacketType`）
pub const MYSQL_PKT_OK: u8 = 0x00;
pub const MYSQL_PKT_MORE_DATA: u8 = 0x01;
pub const MYSQL_PKT_HANDSHAKE: u8 = 0x0A;
pub const MYSQL_PKT_LOCAL_INFILE: u8 = 0xFB;
pub const MYSQL_PKT_EOF: u8 = 0xFE;
pub const MYSQL_PKT_ERR: u8 = 0xFF;

/// MySQL 8.0+ `CLIENT_OPTIONAL_RESULTSET_METADATA` 元数据跟随标志
pub const MYSQL_METADATA_NONE: u8 = 0;
pub const MYSQL_METADATA_FOLLOWS: u8 = 1;

/// 单包最大载荷字节数（24-bit `0xFFFFFF`）
pub const MYSQL_MAX_PACKET_PAYLOAD: usize = 0xFFFFFF;

/// `MySqlResultSetHeader` 与 `AuthSwitchResponseCodec` 错误集
pub err ResultSetHeaderError {
    EmptyPayload,
    NotResultSetHeader,
    ZeroColumnCount,
    Truncated,
    InvalidLenEncPrefix,
    InvalidMetadataFlag,
    TrailingBytes,
    BufferTooSmall,
    PayloadTooLarge,
}

/// MySQL `COM_QUERY` / `COM_STMT_EXECUTE` 首包响应分类
pub enum MySqlResponsePacketKind {
    Ok,
    Error,
    Eof,
    LocalInfile,
    ResultSetHeader,
}

/// 按 MySQL 官方协议规则零拷贝判别查询响应首包类型
/// - `0xFF`: ERR_Packet
/// - `0xFB`: LOCAL INFILE Request
/// - `0x00`（且 `payload.len >= 7`）: OK_Packet
/// - `0xFE`（且 `payload.len < 9`）: EOF_Packet
/// - 其余非零首字节: ResultSetHeader（列计数 lenenc-int）
pub fn classifyQueryResponse(payload: []const u8): MySqlResponsePacketKind! {
    if (payload.len == 0) {
        return err.EmptyPayload;
    }
    const first = payload[0];
    if (first == MYSQL_PKT_ERR) {
        return MySqlResponsePacketKind.Error;
    }
    if (first == MYSQL_PKT_LOCAL_INFILE) {
        return MySqlResponsePacketKind.LocalInfile;
    }
    if (first == MYSQL_PKT_OK && payload.len >= 7) {
        return MySqlResponsePacketKind.Ok;
    }
    if (first == MYSQL_PKT_EOF && payload.len < 9) {
        return MySqlResponsePacketKind.Eof;
    }
    if (first == 0x00) {
        return err.NotResultSetHeader;
    }
    return MySqlResponsePacketKind.ResultSetHeader;
}

fn lenEncSize(v: u64): usize {
    if (v < 251) return 1;
    if (v <= 0xFFFF) return 3;
    if (v <= 0xFFFFFF) return 4;
    return 9;
}

fn writeLenEnc(out: []u8, off: usize, v: u64): usize {
    if (v < 251) {
        out[off] = @as(u8, @intCast(v));
        return 1;
    }
    if (v <= 0xFFFF) {
        out[off] = 0xFC;
        out[off + 1] = @as(u8, @intCast(v & 0xFF));
        out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
        return 3;
    }
    if (v <= 0xFFFFFF) {
        out[off] = 0xFD;
        out[off + 1] = @as(u8, @intCast(v & 0xFF));
        out[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
        out[off + 3] = @as(u8, @intCast((v >> 16) & 0xFF));
        return 4;
    }
    out[off] = 0xFE;
    let i: usize = 0;
    while (i < 8) {
        out[off + 1 + i] = @as(u8, @intCast((v >> @as(u6, @intCast(i * 8))) & 0xFF));
        i += 1;
    }
    return 9;
}

struct DecodedLenEnc {
    value: u64,
    consumed: usize,
}

fn readLenEnc(buf: []const u8, off: usize): DecodedLenEnc! {
    if (off >= buf.len) {
        return err.Truncated;
    }
    const prefix = buf[off];
    if (prefix < 0xFB) {
        return DecodedLenEnc{ value: @as(u64, prefix), consumed: 1 };
    }
    if (prefix == 0xFC) {
        if (buf.len - off < 3) return err.Truncated;
        const v: u64 = @as(u64, buf[off + 1]) | (@as(u64, buf[off + 2]) << 8);
        return DecodedLenEnc{ value: v, consumed: 3 };
    }
    if (prefix == 0xFD) {
        if (buf.len - off < 4) return err.Truncated;
        const v: u64 = @as(u64, buf[off + 1]) |
            (@as(u64, buf[off + 2]) << 8) |
            (@as(u64, buf[off + 3]) << 16);
        return DecodedLenEnc{ value: v, consumed: 4 };
    }
    if (prefix == 0xFE) {
        if (buf.len - off < 9) return err.Truncated;
        let v: u64 = 0;
        let i: usize = 0;
        while (i < 8) {
            v |= @as(u64, buf[off + 1 + i]) << @as(u6, @intCast(i * 8));
            i += 1;
        }
        return DecodedLenEnc{ value: v, consumed: 9 };
    }
    return err.InvalidLenEncPrefix;
}

fn writePacketHeader(out: []u8, payload_len: usize, seq_id: u8) {
    out[0] = @as(u8, @intCast(payload_len & 0xFF));
    out[1] = @as(u8, @intCast((payload_len >> 8) & 0xFF));
    out[2] = @as(u8, @intCast((payload_len >> 16) & 0xFF));
    out[3] = seq_id;
}

fn readPacketPayloadLen(frame: []const u8): usize {
    return @as(usize, frame[0]) |
        (@as(usize, frame[1]) << 8) |
        (@as(usize, frame[2]) << 16);
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

/// MySQL `ResultSetHeader` 响应头（零堆分配）
///
/// 线格式：
/// - 标准模式：`[lenenc-int: field_count]`
/// - MySQL 8.0+ `CLIENT_OPTIONAL_RESULTSET_METADATA` 模式：`[metadata_follows: u8 (0|1)][lenenc-int: field_count]`
pub struct MySqlResultSetHeader {
    field_count: u64 = 0,
    has_metadata_flag: bool = false,
    metadata_follows: bool = true,

    static fn init(field_count: u64): MySqlResultSetHeader {
        return MySqlResultSetHeader{
            field_count: field_count,
            has_metadata_flag: false,
            metadata_follows: true,
        };
    }

    static fn initWithMetadata(field_count: u64, metadata_follows: bool): MySqlResultSetHeader {
        return MySqlResultSetHeader{
            field_count: field_count,
            has_metadata_flag: true,
            metadata_follows: metadata_follows,
        };
    }

    fn encodedPayloadLen(): usize {
        const meta_len: usize = if (_.has_metadata_flag) 1 else 0;
        return meta_len + lenEncSize(_.field_count);
    }

    fn encodedFrameLen(): usize {
        return 4 + _.encodedPayloadLen();
    }

    /// 编码不含 4 字节 MySQL 帧头的载荷
    fn encodePayload(out: []u8): usize! {
        if (_.field_count == 0) {
            return err.ZeroColumnCount;
        }
        const need = _.encodedPayloadLen();
        if (out.len < need) {
            return err.BufferTooSmall;
        }
        let pos: usize = 0;
        if (_.has_metadata_flag) {
            out[0] = if (_.metadata_follows) MYSQL_METADATA_FOLLOWS else MYSQL_METADATA_NONE;
            pos = 1;
        }
        const n = writeLenEnc(out, pos, _.field_count);
        return pos + n;
    }

    /// 编码完整 MySQL 线协议帧（4B header + payload）
    fn encodeFrame(sequence_id: u8, out: []u8): usize! {
        const payload_len = _.encodedPayloadLen();
        const total = 4 + payload_len;
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        writePacketHeader(out, payload_len, sequence_id);
        const written = _.encodePayload(out[4..total])?;
        return 4 + written;
    }

    /// 零拷贝解码不含 4 字节帧头的 `ResultSetHeader` 载荷
    static fn decodePayload(payload: []const u8, optional_metadata: bool): MySqlResultSetHeader! {
        if (payload.len == 0) {
            return err.EmptyPayload;
        }
        let pos: usize = 0;
        let meta_follows = true;
        if (optional_metadata) {
            const flag = payload[0];
            if (flag == MYSQL_METADATA_NONE) {
                meta_follows = false;
            } else if (flag == MYSQL_METADATA_FOLLOWS) {
                meta_follows = true;
            } else {
                return err.InvalidMetadataFlag;
            }
            pos = 1;
        }
        const parsed = readLenEnc(payload, pos)?;
        if (parsed.value == 0) {
            return err.ZeroColumnCount;
        }
        if (pos + parsed.consumed != payload.len) {
            return err.TrailingBytes;
        }
        return MySqlResultSetHeader{
            field_count: parsed.value,
            has_metadata_flag: optional_metadata,
            metadata_follows: meta_follows,
        };
    }

    /// 零拷贝解码完整 MySQL 帧（4B header + ResultSetHeader payload）
    static fn decodeFrame(frame: []const u8, optional_metadata: bool): MySqlResultSetHeader! {
        if (frame.len < 4) {
            return err.Truncated;
        }
        const payload_len = readPacketPayloadLen(frame);
        const expected = 4 + payload_len;
        if (frame.len < expected) {
            return err.Truncated;
        }
        if (frame.len > expected) {
            return err.TrailingBytes;
        }
        return MySqlResultSetHeader.decodePayload(frame[4..expected], optional_metadata);
    }

    /// 计算服务端紧随其后将发送的 `ColumnDefinition41` 包数量
    fn expectedColumnDefPackets(): u64 {
        if (!_.metadata_follows) {
            return 0;
        }
        return _.field_count;
    }

    fn eql(other: MySqlResultSetHeader): bool {
        return _.field_count == other.field_count &&
            _.has_metadata_flag == other.has_metadata_flag &&
            _.metadata_follows == other.metadata_follows;
    }
}

/// MySQL `AuthSwitchResponse` 客户端认证切换响应帧零堆分配编解码器
///
/// 线格式：`[len: 24-bit LE][sequence_id: u8][auth_response_bytes]`
pub struct AuthSwitchResponseCodec {
    static fn encodedFrameLen(auth_response: []const u8): usize {
        return 4 + auth_response.len;
    }

    static fn encodeFrame(auth_response: []const u8, sequence_id: u8, out: []u8): usize! {
        if (auth_response.len > MYSQL_MAX_PACKET_PAYLOAD) {
            return err.PayloadTooLarge;
        }
        const total = 4 + auth_response.len;
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        writePacketHeader(out, auth_response.len, sequence_id);
        copyBytes(out, 4, auth_response);
        return total;
    }

    static fn decodeFrame(frame: []const u8): ([]const u8)! {
        if (frame.len < 4) {
            return err.Truncated;
        }
        const payload_len = readPacketPayloadLen(frame);
        const expected = 4 + payload_len;
        if (frame.len < expected) {
            return err.Truncated;
        }
        if (frame.len > expected) {
            return err.TrailingBytes;
        }
        return frame[4..expected];
    }
}
