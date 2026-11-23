// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Authentication.cp | Synced: 2026-11-23T01:31:50Z

/// PostgreSQL 后端认证报文类型字节（'R' = 0x52）
pub const AUTH_MESSAGE_TAG: u8 = 'R';

/// PostgreSQL v3 认证与线协议写入错误集
pub err PgAuthError {
    UnexpectedEof,
    InvalidMessageTag,
    InvalidMessageLength,
    InvalidAuthType,
    InvalidSaslPayload,
    InvalidIterationCount,
    InvalidNullTerminator,
    OutOfBoundsPatch,
    OutOfMemory,
}

/// PostgreSQL v3 Authentication 子类型枚举
pub enum AuthType: u32 {
    Ok = 0,
    KerberosV5 = 2,
    CleartextPassword = 3,
    MD5Password = 5,
    SCMCredential = 6,
    GSS = 7,
    GSSContinue = 8,
    SSPI = 9,
    SASL = 10,
    SASLContinue = 11,
    SASLFinal = 12,

    fn toInt(): u32 {
        return @intFromEnum(_);
    }
}

/// 从大端 u32 解析 PostgreSQL 认证子类型，未知编码返回 `err.InvalidAuthType`。
pub fn authTypeFromInt(raw: u32): AuthType ! PgAuthError {
    return switch (raw) {
        0 => AuthType.Ok,
        2 => AuthType.KerberosV5,
        3 => AuthType.CleartextPassword,
        5 => AuthType.MD5Password,
        6 => AuthType.SCMCredential,
        7 => AuthType.GSS,
        8 => AuthType.GSSContinue,
        9 => AuthType.SSPI,
        10 => AuthType.SASL,
        11 => AuthType.SASLContinue,
        12 => AuthType.SASLFinal,
        else => return err.InvalidAuthType,
    };
}

/// SCRAM-SHA-256 `SASLContinue`（server-first-message: `r=<nonce>,s=<salt>,i=<iter>`）解析结果
pub struct SASLContinueInfo {
    nonce: string,
    salt_b64: string,
    iterations: u32,
    raw: string,
}

/// 解码后的 PostgreSQL 认证握手报文（含 RAII `~init()` 析构，零泄漏）
pub struct DecodedAuthMessage {
    auth_type: AuthType,
    md5_salt: [4]u8 = [0, 0, 0, 0],
    sasl_mechanisms: []string = [],
    sasl_continue: SASLContinueInfo? = null,
    payload: []const u8 = [],
    bytes_read: usize = 0,
    allocator: std.mem.Allocator,

    fn isOk(): bool {
        return _.auth_type == AuthType.Ok;
    }

    fn hasMechanism(name: string): bool {
        for (m of _.sasl_mechanisms) {
            if (std.mem.eql(u8, m, name)) {
                return true;
            }
        }
        return false;
    }

    ~init() {
        if (_.sasl_mechanisms.len > 0) {
            _.allocator.free(_.sasl_mechanisms);
            _.sasl_mechanisms = [];
        }
    }
}

fn readU32BE(data: []const u8, offset: usize): u32 {
    return ((data[offset] as u32) << 24)
        | ((data[offset + 1] as u32) << 16)
        | ((data[offset + 2] as u32) << 8)
        | (data[offset + 3] as u32);
}

fn parseSaslContinuePayload(raw: string): SASLContinueInfo ! PgAuthError {
    let r_val: string? = null;
    let s_val: string? = null;
    let i_val: string? = null;

    let start: usize = 0;
    let pos: usize = 0;
    while (pos <= raw.len) {
        if (pos == raw.len || raw[pos] == ',') {
            const item = raw[start..pos];
            if (item.len > 2 && item[1] == '=') {
                const k = item[0];
                const v = item[2..];
                if (k == 'r') {
                    r_val = v;
                } else if (k == 's') {
                    s_val = v;
                } else if (k == 'i') {
                    i_val = v;
                }
            }
            start = pos + 1;
        }
        pos += 1;
    }

    if (r_val == null || s_val == null || i_val == null) {
        return err.InvalidSaslPayload;
    }
    const iter_str = i_val.?;
    const iterations = std.fmt.parseInt(u32, iter_str, 10) catch return err.InvalidIterationCount;
    if (iterations == 0) {
        return err.InvalidIterationCount;
    }
    return SASLContinueInfo{
        nonce: r_val.?,
        salt_b64: s_val.?,
        iterations: iterations,
        raw: raw,
    };
}

/// 解码 PostgreSQL 后端 `'R'` 认证握手报文（覆盖 Ok/CleartextPassword/MD5Password/SASL/SASLContinue/SASLFinal 等）。
pub fn decodeAuthMessage(
    allocator: std.mem.Allocator,
    data: []const u8,
): DecodedAuthMessage ! PgAuthError {
    // 最小帧：1B 'R' + 4B length + 4B auth_code = 9 bytes
    if (data.len < 9) {
        return err.UnexpectedEof;
    }
    if (data[0] != AUTH_MESSAGE_TAG) {
        return err.InvalidMessageTag;
    }
    const msg_len = readU32BE(data, 1);
    if (msg_len < 8) {
        return err.InvalidMessageLength;
    }
    const total_frame: usize = usize(1) + (msg_len as usize);
    if (total_frame > data.len) {
        return err.UnexpectedEof;
    }

    const auth_code = readU32BE(data, 5);
    const auth_type = authTypeFromInt(auth_code)?;
    const body = data[9..total_frame];

    switch (auth_type) {
        .Ok, .KerberosV5, .CleartextPassword, .SCMCredential, .GSS, .SSPI => {
            if (msg_len != 8) {
                return err.InvalidMessageLength;
            }
            return DecodedAuthMessage{
                auth_type: auth_type,
                bytes_read: total_frame,
                allocator: allocator,
            };
        },
        .MD5Password => {
            if (msg_len != 12) {
                return err.InvalidMessageLength;
            }
            const salt: [4]u8 = [body[0], body[1], body[2], body[3]];
            return DecodedAuthMessage{
                auth_type: AuthType.MD5Password,
                md5_salt: salt,
                bytes_read: total_frame,
                allocator: allocator,
            };
        },
        .GSSContinue, .SASLFinal => {
            if (msg_len < 9) {
                return err.InvalidMessageLength;
            }
            return DecodedAuthMessage{
                auth_type: auth_type,
                payload: body,
                bytes_read: total_frame,
                allocator: allocator,
            };
        },
        .SASL => {
            if (msg_len < 9) {
                return err.InvalidMessageLength;
            }
            if (body[body.len - 1] != 0) {
                return err.InvalidNullTerminator;
            }
            // 第一遍统计机制数量（以空串 NUL 结尾）
            let pos: usize = 0;
            let count: usize = 0;
            while (pos < body.len) {
                if (body[pos] == 0) {
                    if (pos + 1 != body.len) {
                        return err.InvalidSaslPayload;
                    }
                    break;
                }
                while (pos < body.len && body[pos] != 0) {
                    pos += 1;
                }
                if (pos >= body.len) {
                    return err.InvalidNullTerminator;
                }
                pos += 1;
                count += 1;
            }

            let mechs = allocator.alloc(string, count) catch return err.OutOfMemory;
            errdefer allocator.free(mechs);

            pos = 0;
            let idx: usize = 0;
            while (idx < count) {
                const start = pos;
                while (body[pos] != 0) {
                    pos += 1;
                }
                mechs[idx] = body[start..pos];
                pos += 1;
                idx += 1;
            }

            return DecodedAuthMessage{
                auth_type: AuthType.SASL,
                sasl_mechanisms: mechs,
                payload: body,
                bytes_read: total_frame,
                allocator: allocator,
            };
        },
        .SASLContinue => {
            if (msg_len < 9) {
                return err.InvalidMessageLength;
            }
            const info = parseSaslContinuePayload(body)?;
            return DecodedAuthMessage{
                auth_type: AuthType.SASLContinue,
                sasl_continue: info,
                payload: body,
                bytes_read: total_frame,
                allocator: allocator,
            };
        },
    }
}

/// PostgreSQL v3 大端协议流式写入器（支持长度头回填与 RAII `~init()` 零泄漏释放）
pub struct PgWireWriter {
    buf: []u8,
    len: usize = 0,
    cap: usize = 0,
    allocator: std.mem.Allocator,

    static fn init(allocator: std.mem.Allocator): PgWireWriter {
        return PgWireWriter{
            buf: [],
            len: 0,
            cap: 0,
            allocator: allocator,
        };
    }

    fn bytes(): []const u8 {
        return _.buf[0.._.len];
    }

    fn offset(): usize {
        return _.len;
    }

    fn reset(*_) {
        _.len = 0;
    }

    fn ensureCapacity(*_, additional: usize): void ! PgAuthError {
        const need = _.len + additional;
        if (need <= _.cap) {
            return;
        }
        let next_cap: usize = if (_.cap == 0) usize(64) else (_.cap * usize(2));
        while (next_cap < need) {
            next_cap *= usize(2);
        }
        const next_buf = _.allocator.alloc(u8, next_cap) catch return err.OutOfMemory;
        if (_.len > 0) {
            @memcpy(next_buf[0.._.len], _.buf[0.._.len]);
        }
        if (_.cap > 0) {
            _.allocator.free(_.buf);
        }
        _.buf = next_buf;
        _.cap = next_cap;
    }

    fn write(*_, data: []const u8): void ! PgAuthError {
        if (data.len == 0) {
            return;
        }
        _.ensureCapacity(data.len)?;
        @memcpy(_.buf[_.len .. _.len + data.len], data);
        _.len += data.len;
    }

    fn writeByte(*_, b: u8): void ! PgAuthError {
        _.ensureCapacity(1)?;
        _.buf[_.len] = b;
        _.len += 1;
    }

    /// 在指定偏移位置原地覆写字节（用于大端长度字段回填）
    fn pwrite(*_, data: []const u8, at: usize): void ! PgAuthError {
        if (at + data.len > _.len) {
            return err.OutOfBoundsPatch;
        }
        if (data.len > 0) {
            @memcpy(_.buf[at .. at + data.len], data);
        }
    }

    fn writeU16(*_, v: u16): void ! PgAuthError {
        _.ensureCapacity(2)?;
        _.buf[_.len] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 1] = (v & 0xFF) as u8;
        _.len += 2;
    }

    fn writeI16(*_, v: i16): void ! PgAuthError {
        const raw: u16 = @bitCast(v);
        return _.writeU16(raw);
    }

    fn writeU32(*_, v: u32): void ! PgAuthError {
        _.ensureCapacity(4)?;
        _.buf[_.len] = ((v >> 24) & 0xFF) as u8;
        _.buf[_.len + 1] = ((v >> 16) & 0xFF) as u8;
        _.buf[_.len + 2] = ((v >> 8) & 0xFF) as u8;
        _.buf[_.len + 3] = (v & 0xFF) as u8;
        _.len += 4;
    }

    fn writeI32(*_, v: i32): void ! PgAuthError {
        const raw: u32 = @bitCast(v);
        return _.writeU32(raw);
    }

    fn writeU64(*_, v: u64): void ! PgAuthError {
        _.writeU32(((v >> 32) & 0xFFFFFFFF) as u32)?;
        return _.writeU32((v & 0xFFFFFFFF) as u32);
    }

    fn writeI64(*_, v: i64): void ! PgAuthError {
        const raw: u64 = @bitCast(v);
        return _.writeU64(raw);
    }

    /// 写入以 NUL 结尾的 C 字符串（若末尾已为 NUL 则不重复追加）
    fn writeCString(*_, s: string): void ! PgAuthError {
        _.write(s)?;
        if (s.len == 0 || s[s.len - 1] != 0) {
            _.writeByte(0)?;
        }
    }

    /// 写入 PostgreSQL NULL 长度标记（-1 / 0xFFFFFFFF）
    fn writeNull(*_): void ! PgAuthError {
        return _.writeU32(0xFFFFFFFF);
    }

    /// 开始一条带类型字节与 4 字节长度头的报文，返回长度字段偏移量供 `endMessage` 回填。
    fn beginMessage(*_, tag: u8): usize ! PgAuthError {
        _.writeByte(tag)?;
        const len_offset = _.len;
        _.writeU32(0)?;
        return len_offset;
    }

    /// 自动计算并回填大端报文长度（包含长度字段自身 4 字节）。
    fn endMessage(*_, len_offset: usize): void ! PgAuthError {
        if (len_offset + 4 > _.len) {
            return err.OutOfBoundsPatch;
        }
        const total_len: u32 = (_.len - len_offset) as u32;
        const len_bytes: [4]u8 = [
            ((total_len >> 24) & 0xFF) as u8,
            ((total_len >> 16) & 0xFF) as u8,
            ((total_len >> 8) & 0xFF) as u8,
            (total_len & 0xFF) as u8,
        ];
        return _.pwrite(len_bytes[0..], len_offset);
    }

    ~init() {
        if (_.cap > 0) {
            _.allocator.free(_.buf);
            _.buf = [];
            _.len = 0;
            _.cap = 0;
        }
    }
}
