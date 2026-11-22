// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/SASLInitialResponse.cp | Synced: 2026-11-22T20:26:49Z
// Pure-CP zero-allocation PostgreSQL v3.0 Frontend Authentication ('p') frames:
// PasswordMessage, SASLInitialResponse, and SASLResponse.

/// PostgreSQL Frontend 认证响应统一报文类型字节（`'p' = 0x70`）
/// 用于 `PasswordMessage`、`SASLInitialResponse`、`SASLResponse`
pub const PG_TAG_PASSWORD_MESSAGE: u8 = 'p';

/// 标准 SASL 认证机制名称：`SCRAM-SHA-256`
pub const PG_SASL_SCRAM_SHA256: []const u8 = "SCRAM-SHA-256";

/// 带通道绑定的 SASL 认证机制名称：`SCRAM-SHA-256-PLUS`
pub const PG_SASL_SCRAM_SHA256_PLUS: []const u8 = "SCRAM-SHA-256-PLUS";

/// `SASLInitialResponse` 中表示"未携带初始客户端响应数据"的长度哨兵值（`-1`）
pub const PG_SASL_NO_INITIAL_DATA: i32 = -1;

/// PostgreSQL 前端认证帧编解码与边界校验错误集
pub err PgAuthFrameError {
    InvalidTag,
    InvalidLength,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
    EmptyMechanism,
    InvalidDataLength,
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

fn decodeHeader(frame: []const u8): usize! {
    if (frame.len < 5) {
        return err.Truncated;
    }
    if (frame[0] != PG_TAG_PASSWORD_MESSAGE) {
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
    return expected_total;
}

/// PostgreSQL v3.0 Frontend `PasswordMessage` (`'p'`) — 明文或 MD5 认证密码帧（零堆分配）
///
/// 线格式：`['p'][len: i32 BE][password_bytes][0x00]`
pub struct PgPasswordMessage {
    password: []const u8 = "",

    static fn init(password: []const u8): PgPasswordMessage {
        return PgPasswordMessage{ password: password };
    }

    fn encodedLen(): usize {
        return 5 + _.password.len + 1;
    }

    /// 编码为完整 `'p'` 线协议帧（拒绝内嵌 `0x00` 字节）
    fn encodeFrame(out: []u8): usize! {
        if (containsNul(_.password)) {
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
        out[0] = PG_TAG_PASSWORD_MESSAGE;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));
        copyBytes(out, 5, _.password);
        out[5 + _.password.len] = 0;
        return total;
    }

    /// 零拷贝解码完整 `'p'` `PasswordMessage` 帧
    static fn decodeFrame(frame: []const u8): PgPasswordMessage! {
        const total = decodeHeader(frame)?;
        const payload = frame[5..total];
        if (payload.len == 0 || payload[payload.len - 1] != 0) {
            return err.MissingNulTerminator;
        }
        const pw = payload[0..(payload.len - 1)];
        if (containsNul(pw)) {
            return err.EmbeddedNul;
        }
        return PgPasswordMessage{ password: pw };
    }

    /// 判断密码是否为 PostgreSQL MD5 响应格式（`"md5"` + 32 字节十六进制摘要，共 35 字节）
    fn isMd5Prefixed(): bool {
        if (_.password.len != 35) {
            return false;
        }
        return _.password[0] == 'm' && _.password[1] == 'd' && _.password[2] == '5';
    }

    fn eql(other: PgPasswordMessage): bool {
        return bytesEql(_.password, other.password);
    }
}

/// PostgreSQL v3.0 Frontend `SASLInitialResponse` (`'p'`) — SASL 初始认证帧（零堆分配）
///
/// 线格式：`['p'][len: i32 BE][mechanism\0][data_len: i32 BE][initial_data?]`
/// 当 `initial_data == null` 时，`data_len` 编码为 `-1`（`0xFFFFFFFF`）且无后续字节。
pub struct PgSaslInitialResponse {
    mechanism: []const u8 = "",
    initial_data: ([]const u8)? = null,

    static fn init(mechanism: []const u8, initial_data: ([]const u8)?): PgSaslInitialResponse {
        return PgSaslInitialResponse{
            mechanism: mechanism,
            initial_data: initial_data,
        };
    }

    /// 快速构造 `SCRAM-SHA-256` 初始客户端消息帧
    static fn scramSha256(client_first_message: []const u8): PgSaslInitialResponse {
        return PgSaslInitialResponse{
            mechanism: PG_SASL_SCRAM_SHA256,
            initial_data: client_first_message,
        };
    }

    fn encodedLen(): usize {
        const data_len: usize = if (d := _.initial_data) d.len else 0;
        return 5 + _.mechanism.len + 1 + 4 + data_len;
    }

    /// 编码为完整 `'p'` `SASLInitialResponse` 线协议帧（拒绝空机制名或内嵌 NUL）
    fn encodeFrame(out: []u8): usize! {
        if (_.mechanism.len == 0) {
            return err.EmptyMechanism;
        }
        if (containsNul(_.mechanism)) {
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
        out[0] = PG_TAG_PASSWORD_MESSAGE;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));
        let pos: usize = 5;
        copyBytes(out, pos, _.mechanism);
        pos += _.mechanism.len;
        out[pos] = 0;
        pos += 1;

        if (d := _.initial_data) {
            if (d.len > 0x7FFFFFFF) {
                return err.InvalidDataLength;
            }
            writeBeI32(out, pos, @as(i32, @intCast(d.len)));
            pos += 4;
            copyBytes(out, pos, d);
            pos += d.len;
        } else {
            writeBeI32(out, pos, PG_SASL_NO_INITIAL_DATA);
            pos += 4;
        }
        return pos;
    }

    /// 零拷贝解码完整 `'p'` `SASLInitialResponse` 帧
    static fn decodeFrame(frame: []const u8): PgSaslInitialResponse! {
        const total = decodeHeader(frame)?;
        const payload = frame[5..total];

        let nul_pos: usize = 0;
        while (nul_pos < payload.len && payload[nul_pos] != 0) {
            nul_pos += 1;
        }
        if (nul_pos >= payload.len) {
            return err.MissingNulTerminator;
        }
        const mech = payload[0..nul_pos];
        if (mech.len == 0) {
            return err.EmptyMechanism;
        }

        const after_mech = nul_pos + 1;
        if (payload.len - after_mech < 4) {
            return err.Truncated;
        }
        const data_len_i32 = readBeI32(payload, after_mech);
        const data_start = after_mech + 4;
        const remaining = payload.len - data_start;

        if (data_len_i32 == PG_SASL_NO_INITIAL_DATA) {
            if (remaining != 0) {
                return err.TrailingBytes;
            }
            return PgSaslInitialResponse{
                mechanism: mech,
                initial_data: null,
            };
        }
        if (data_len_i32 < 0) {
            return err.InvalidDataLength;
        }
        const data_len = @as(usize, @intCast(data_len_i32));
        if (remaining < data_len) {
            return err.Truncated;
        }
        if (remaining > data_len) {
            return err.TrailingBytes;
        }
        return PgSaslInitialResponse{
            mechanism: mech,
            initial_data: payload[data_start..(data_start + data_len)],
        };
    }

    fn isScramSha256(): bool {
        return bytesEql(_.mechanism, PG_SASL_SCRAM_SHA256);
    }

    fn isScramSha256Plus(): bool {
        return bytesEql(_.mechanism, PG_SASL_SCRAM_SHA256_PLUS);
    }

    fn eql(other: PgSaslInitialResponse): bool {
        if (!bytesEql(_.mechanism, other.mechanism)) {
            return false;
        }
        if (a := _.initial_data) {
            if (b := other.initial_data) {
                return bytesEql(a, b);
            }
            return false;
        }
        return other.initial_data == null;
    }
}

/// PostgreSQL v3.0 Frontend `SASLResponse` (`'p'`) — SASL 后续挑战响应帧（零堆分配）
///
/// 线格式：`['p'][len: i32 BE][sasl_response_bytes]`
pub struct PgSaslResponse {
    data: []const u8 = "",

    static fn init(data: []const u8): PgSaslResponse {
        return PgSaslResponse{ data: data };
    }

    fn encodedLen(): usize {
        return 5 + _.data.len;
    }

    /// 编码为完整 `'p'` `SASLResponse` 线协议帧
    fn encodeFrame(out: []u8): usize! {
        const total = _.encodedLen();
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        const len_field = total - 1;
        if (len_field > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        out[0] = PG_TAG_PASSWORD_MESSAGE;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));
        copyBytes(out, 5, _.data);
        return total;
    }

    /// 零拷贝解码完整 `'p'` `SASLResponse` 帧
    static fn decodeFrame(frame: []const u8): PgSaslResponse! {
        const total = decodeHeader(frame)?;
        return PgSaslResponse{ data: frame[5..total] };
    }

    fn eql(other: PgSaslResponse): bool {
        return bytesEql(_.data, other.data);
    }
}
