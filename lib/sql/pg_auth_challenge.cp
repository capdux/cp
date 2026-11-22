// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Authentication.cp | Synced: 2026-11-22T11:04:16Z
// Ported from Bun: /data/dev/bun/src/sql/postgres/protocol/Authentication.cp & /data/dev/bun/docs/runtime/sql.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation PostgreSQL Backend Authentication ('R') wire codec:
// AuthenticationOk / Cleartext / MD5 (4B salt) / GSS / SSPI / SASL mechanism list iterator /
// SCRAM-SHA-256 SASLContinue (`r=...,s=...,i=...`) & SASLFinal (`v=...` / `e=...`) round-trip parser & encoder.

/// PostgreSQL 后端认证消息类型标签 (`'R'` = 0x52)
pub const PG_AUTHENTICATION_TAG: u8 = 'R';

/// PostgreSQL 认证子码常量（Wire Protocol v3.0）
pub const PG_AUTH_CODE_OK: u32 = 0;
pub const PG_AUTH_CODE_KERBEROS_V5: u32 = 2;
pub const PG_AUTH_CODE_CLEARTEXT: u32 = 3;
pub const PG_AUTH_CODE_MD5: u32 = 5;
pub const PG_AUTH_CODE_GSS: u32 = 7;
pub const PG_AUTH_CODE_GSS_CONTINUE: u32 = 8;
pub const PG_AUTH_CODE_SSPI: u32 = 9;
pub const PG_AUTH_CODE_SASL: u32 = 10;
pub const PG_AUTH_CODE_SASL_CONTINUE: u32 = 11;
pub const PG_AUTH_CODE_SASL_FINAL: u32 = 12;

/// PostgreSQL 认证质询编解码错误集
pub err PgAuthChallengeError {
    BufferTooSmall,
    TruncatedFrame,
    InvalidTag,
    InvalidMessageLength,
    InvalidSaslContinue,
    InvalidIterationCount,
    InvalidSaslFinal,
    InvalidSaslMechanismList,
    TrailingBytes,
}

/// PostgreSQL 后端认证质询类型枚举（对齐 Bun `Authentication`）
pub enum PgAuthChallengeKind: u8 {
    Ok = 0,
    KerberosV5 = 1,
    ClearTextPassword = 2,
    MD5Password = 3,
    GSS = 4,
    GSSContinue = 5,
    SSPI = 6,
    SASL = 7,
    SASLContinue = 8,
    SASLFinal = 9,
    Unknown = 10,
}

/// SCRAM-SHA-256 `server-first-message` 解析视图（`r=<nonce>,s=<salt_b64>,i=<iterations>`）
pub struct PgSaslContinueChallenge {
    raw: []const u8 = "",
    nonce: []const u8 = "",
    salt_b64: []const u8 = "",
    iterations_raw: []const u8 = "",
    iterations: u32 = 0,
}

/// SCRAM-SHA-256 `server-final-message` 解析视图（`v=<verifier_b64>` 或 `e=<error>`）
pub struct PgSaslFinalView {
    raw: []const u8 = "",
    verifier_b64: []const u8 = "",
    error_msg: ([]const u8)? = null,
}

/// `AuthenticationSASL` 机制列表零拷贝迭代器（NUL 分隔并以空串 NUL 结尾）
pub struct PgSaslMechanismIter {
    buf: []const u8 = "",
    pos: usize = 0,

    fn next(*_): ([]const u8)? {
        if (_.pos >= _.buf.len) {
            return null;
        }
        if (_.buf[_.pos] == 0) {
            _.pos = _.buf.len;
            return null;
        }
        const start = _.pos;
        while (_.pos < _.buf.len && _.buf[_.pos] != 0) {
            _.pos += 1;
        }
        const item = _.buf[start.._.pos];
        if (_.pos < _.buf.len && _.buf[_.pos] == 0) {
            _.pos += 1;
        }
        return item;
    }

    /// 检查机制列表中是否包含指定 SASL 机制（如 `"SCRAM-SHA-256"`）
    fn contains(target: []const u8): bool {
        let it = PgSaslMechanismIter{ buf: _.buf, pos: 0 };
        while (m := it.next()) {
            if (bytesEql(m, target)) {
                return true;
            }
        }
        return false;
    }
}

/// 解码后的 PostgreSQL `'R'` 认证消息完整视图
pub struct PgAuthChallengeView {
    kind: PgAuthChallengeKind = PgAuthChallengeKind.Unknown,
    raw_code: u32 = 0,
    md5_salt: [4]u8 = [0; 4],
    data: []const u8 = "",
    sasl_continue: PgSaslContinueChallenge = PgSaslContinueChallenge{},
    sasl_final: PgSaslFinalView = PgSaslFinalView{},

    fn saslMechanisms(): PgSaslMechanismIter {
        return PgSaslMechanismIter{ buf: _.data, pos: 0 };
    }
}

type MechName = []const u8;

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

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xff) as u8;
    buf[off + 1] = ((v >> 16) & 0xff) as u8;
    buf[off + 2] = ((v >> 8) & 0xff) as u8;
    buf[off + 3] = (v & 0xff) as u8;
}

fn parsePositiveU32(s: []const u8): u32! {
    if (s.len == 0) {
        return PgAuthChallengeError.InvalidIterationCount;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return PgAuthChallengeError.InvalidIterationCount;
        }
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 0xffff_ffff) {
            return PgAuthChallengeError.InvalidIterationCount;
        }
        i += 1;
    }
    if (acc == 0) {
        return PgAuthChallengeError.InvalidIterationCount;
    }
    return acc as u32;
}

/// 解析 SCRAM-SHA-256 `AuthenticationSASLContinue` 载荷（`r=<nonce>,s=<salt_b64>,i=<iter>`）
pub fn parsePgSaslContinue(raw: []const u8): PgSaslContinueChallenge! {
    if (raw.len == 0) {
        return PgAuthChallengeError.InvalidSaslContinue;
    }
    let r_opt: ([]const u8)? = null;
    let s_opt: ([]const u8)? = null;
    let i_opt: ([]const u8)? = null;

    let pos: usize = 0;
    while (pos <= raw.len) {
        let end = pos;
        while (end < raw.len && raw[end] != ',') {
            end += 1;
        }
        const item = raw[pos..end];
        if (item.len > 2 && item[1] == '=') {
            const key = item[0];
            const val = item[2..];
            if (key == 'r') {
                r_opt = val;
            } else if (key == 's') {
                s_opt = val;
            } else if (key == 'i') {
                i_opt = val;
            }
        }
        if (end >= raw.len) {
            break;
        }
        pos = end + 1;
    }

    const r_val = r_opt ?? return PgAuthChallengeError.InvalidSaslContinue;
    const s_val = s_opt ?? return PgAuthChallengeError.InvalidSaslContinue;
    const i_val = i_opt ?? return PgAuthChallengeError.InvalidSaslContinue;
    const iters = parsePositiveU32(i_val)?;

    return PgSaslContinueChallenge{
        raw: raw,
        nonce: r_val,
        salt_b64: s_val,
        iterations_raw: i_val,
        iterations: iters,
    };
}

/// 解析 SCRAM-SHA-256 `AuthenticationSASLFinal` 载荷（`v=<verifier_b64>` 或 `e=<error>`）
pub fn parsePgSaslFinal(raw: []const u8): PgSaslFinalView! {
    if (raw.len < 3 || raw[1] != '=') {
        return PgAuthChallengeError.InvalidSaslFinal;
    }
    if (raw[0] == 'v') {
        return PgSaslFinalView{
            raw: raw,
            verifier_b64: raw[2..],
            error_msg: null,
        };
    }
    if (raw[0] == 'e') {
        return PgSaslFinalView{
            raw: raw,
            verifier_b64: "",
            error_msg: raw[2..],
        };
    }
    return PgAuthChallengeError.InvalidSaslFinal;
}

fn encodeSimpleAuthFrame(code: u32, out: []u8): usize! {
    if (out.len < 9) {
        return PgAuthChallengeError.BufferTooSmall;
    }
    out[0] = PG_AUTHENTICATION_TAG;
    writeBeU32(out, 1, 8);
    writeBeU32(out, 5, code);
    return 9;
}

/// 编码 `AuthenticationOk` 帧（9 字节）
pub fn encodePgAuthOk(out: []u8): usize! {
    return encodeSimpleAuthFrame(PG_AUTH_CODE_OK, out);
}

/// 编码 `AuthenticationCleartextPassword` 帧（9 字节）
pub fn encodePgAuthCleartext(out: []u8): usize! {
    return encodeSimpleAuthFrame(PG_AUTH_CODE_CLEARTEXT, out);
}

/// 编码 `AuthenticationMD5Password` 帧（13 字节：含 4 字节 salt）
pub fn encodePgAuthMd5(salt: [4]u8, out: []u8): usize! {
    if (out.len < 13) {
        return PgAuthChallengeError.BufferTooSmall;
    }
    out[0] = PG_AUTHENTICATION_TAG;
    writeBeU32(out, 1, 12);
    writeBeU32(out, 5, PG_AUTH_CODE_MD5);
    out[9] = salt[0];
    out[10] = salt[1];
    out[11] = salt[2];
    out[12] = salt[3];
    return 13;
}

/// 编码 `AuthenticationSASL` 帧（机制列表以 `\0` 分隔并以额外 `\0` 终止）
pub fn encodePgAuthSasl(mechanisms: []const MechName, out: []u8): usize! {
    if (mechanisms.len == 0) {
        return PgAuthChallengeError.InvalidSaslMechanismList;
    }
    let body_len: usize = 1; // 末尾空串终止符 '\0'
    let i: usize = 0;
    while (i < mechanisms.len) {
        if (mechanisms[i].len == 0) {
            return PgAuthChallengeError.InvalidSaslMechanismList;
        }
        body_len += mechanisms[i].len + 1;
        i += 1;
    }
    const total = 1 + 4 + 4 + body_len;
    if (total > out.len) {
        return PgAuthChallengeError.BufferTooSmall;
    }
    out[0] = PG_AUTHENTICATION_TAG;
    writeBeU32(out, 1, (8 + body_len) as u32);
    writeBeU32(out, 5, PG_AUTH_CODE_SASL);
    let pos: usize = 9;
    let j: usize = 0;
    while (j < mechanisms.len) {
        const m = mechanisms[j];
        let k: usize = 0;
        while (k < m.len) {
            out[pos] = m[k];
            pos += 1;
            k += 1;
        }
        out[pos] = 0;
        pos += 1;
        j += 1;
    }
    out[pos] = 0;
    pos += 1;
    return pos;
}

fn encodeAuthDataFrame(code: u32, payload: []const u8, out: []u8): usize! {
    if (payload.len == 0) {
        return PgAuthChallengeError.InvalidMessageLength;
    }
    const total = 9 + payload.len;
    if (total > out.len) {
        return PgAuthChallengeError.BufferTooSmall;
    }
    out[0] = PG_AUTHENTICATION_TAG;
    writeBeU32(out, 1, (8 + payload.len) as u32);
    writeBeU32(out, 5, code);
    let i: usize = 0;
    while (i < payload.len) {
        out[9 + i] = payload[i];
        i += 1;
    }
    return total;
}

/// 编码 `AuthenticationSASLContinue` 帧（SCRAM-SHA-256 `server-first-message`）
pub fn encodePgAuthSaslContinue(scram_server_first: []const u8, out: []u8): usize! {
    return encodeAuthDataFrame(PG_AUTH_CODE_SASL_CONTINUE, scram_server_first, out);
}

/// 编码 `AuthenticationSASLFinal` 帧（SCRAM-SHA-256 `server-final-message`）
pub fn encodePgAuthSaslFinal(scram_server_final: []const u8, out: []u8): usize! {
    return encodeAuthDataFrame(PG_AUTH_CODE_SASL_FINAL, scram_server_final, out);
}

/// 解码完整 PostgreSQL `'R'` 认证帧（零拷贝借用输入缓冲区切片，严格校验所有长度不变量）
pub fn decodePgAuthChallenge(frame: []const u8): PgAuthChallengeView! {
    if (frame.len < 9) {
        return PgAuthChallengeError.TruncatedFrame;
    }
    if (frame[0] != PG_AUTHENTICATION_TAG) {
        return PgAuthChallengeError.InvalidTag;
    }
    const msg_len = readBeU32(frame, 1) as usize;
    if (msg_len < 8) {
        return PgAuthChallengeError.InvalidMessageLength;
    }
    const total_len = 1 + msg_len;
    if (total_len > frame.len) {
        return PgAuthChallengeError.TruncatedFrame;
    }
    if (total_len < frame.len) {
        return PgAuthChallengeError.TrailingBytes;
    }

    const code = readBeU32(frame, 5);
    const payload = frame[9..total_len];

    if (code == PG_AUTH_CODE_OK) {
        if (msg_len != 8) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.Ok, raw_code: code };
    }
    if (code == PG_AUTH_CODE_KERBEROS_V5) {
        if (msg_len != 8) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.KerberosV5, raw_code: code };
    }
    if (code == PG_AUTH_CODE_CLEARTEXT) {
        if (msg_len != 8) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.ClearTextPassword, raw_code: code };
    }
    if (code == PG_AUTH_CODE_MD5) {
        if (msg_len != 12) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{
            kind: PgAuthChallengeKind.MD5Password,
            raw_code: code,
            md5_salt: [payload[0], payload[1], payload[2], payload[3]],
            data: payload,
        };
    }
    if (code == PG_AUTH_CODE_GSS) {
        if (msg_len != 8) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.GSS, raw_code: code };
    }
    if (code == PG_AUTH_CODE_GSS_CONTINUE) {
        if (msg_len < 9) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.GSSContinue, raw_code: code, data: payload };
    }
    if (code == PG_AUTH_CODE_SSPI) {
        if (msg_len != 8) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.SSPI, raw_code: code };
    }
    if (code == PG_AUTH_CODE_SASL) {
        if (msg_len < 9 || payload[payload.len - 1] != 0) {
            return PgAuthChallengeError.InvalidSaslMechanismList;
        }
        return PgAuthChallengeView{ kind: PgAuthChallengeKind.SASL, raw_code: code, data: payload };
    }
    if (code == PG_AUTH_CODE_SASL_CONTINUE) {
        if (msg_len < 9) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        const sc = parsePgSaslContinue(payload)?;
        return PgAuthChallengeView{
            kind: PgAuthChallengeKind.SASLContinue,
            raw_code: code,
            data: payload,
            sasl_continue: sc,
        };
    }
    if (code == PG_AUTH_CODE_SASL_FINAL) {
        if (msg_len < 9) {
            return PgAuthChallengeError.InvalidMessageLength;
        }
        const sf = parsePgSaslFinal(payload)?;
        return PgAuthChallengeView{
            kind: PgAuthChallengeKind.SASLFinal,
            raw_code: code,
            data: payload,
            sasl_final: sf,
        };
    }

    return PgAuthChallengeView{
        kind: PgAuthChallengeKind.Unknown,
        raw_code: code,
        data: payload,
    };
}
