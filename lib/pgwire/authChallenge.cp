// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/Authentication.cp | Synced: 2026-11-22T20:47:08Z
// Pure-CP zero-allocation PostgreSQL v3.0 Backend `Authentication` ('R') challenge codec
// with zero-copy SCRAM-SHA-256 server-first / server-final and SASL mechanism list parsing.

/// PostgreSQL Backend `Authentication` 报文类型字节（`'R' = 0x52`）
pub const PG_TAG_AUTHENTICATION: u8 = 'R';

/// `Authentication` 最小帧头长度（1B `'R'` + 4B len(8) + 4B auth_code = 9B）
pub const PG_AUTH_HEADER_LEN: usize = 9;

/// PostgreSQL v3.0 `Authentication` 认证子类型码
pub const PG_AUTH_CODE_OK: u32 = 0;
pub const PG_AUTH_CODE_KERBEROS_V5: u32 = 2;
pub const PG_AUTH_CODE_CLEARTEXT: u32 = 3;
pub const PG_AUTH_CODE_MD5: u32 = 5;
pub const PG_AUTH_CODE_SCM_CREDENTIAL: u32 = 6;
pub const PG_AUTH_CODE_GSS: u32 = 7;
pub const PG_AUTH_CODE_GSS_CONTINUE: u32 = 8;
pub const PG_AUTH_CODE_SSPI: u32 = 9;
pub const PG_AUTH_CODE_SASL: u32 = 10;
pub const PG_AUTH_CODE_SASL_CONTINUE: u32 = 11;
pub const PG_AUTH_CODE_SASL_FINAL: u32 = 12;

/// `AuthenticationSASL` 允许解析的最大机制名称数（零堆分配固定容量）
pub const PG_AUTH_MAX_SASL_MECHANISMS: usize = 8;

/// `Authentication` 编解码与 SCRAM 解析错误集
pub err PgAuthChallengeError {
    InvalidTag,
    InvalidLength,
    InvalidMessage,
    InvalidIterations,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    TooManyMechanisms,
    BufferTooSmall,
}

/// PostgreSQL Backend `Authentication` 认证挑战分类
pub enum PgAuthChallengeKind {
    Ok,
    KerberosV5,
    CleartextPassword,
    Md5Password,
    ScmCredential,
    Gss,
    GssContinue,
    Sspi,
    Sasl,
    SaslContinue,
    SaslFinal,
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

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

fn parsePositiveU32(digits: []const u8): u32! {
    if (digits.len == 0) {
        return err.InvalidIterations;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < digits.len) {
        const c = digits[i];
        if (c < '0' || c > '9') {
            return err.InvalidIterations;
        }
        acc = (acc * 10) + @as(u64, c - '0');
        if (acc > 0xFFFFFFFF) {
            return err.InvalidIterations;
        }
        i += 1;
    }
    if (acc == 0) {
        return err.InvalidIterations;
    }
    return @as(u32, @intCast(acc));
}

/// SCRAM-SHA-256 `AuthenticationSASLContinue` (`r=<nonce>,s=<salt_b64>,i=<iterations>`) 零拷贝解析结果
pub struct PgScramServerFirst {
    raw: []const u8 = "",
    nonce: []const u8 = "",
    salt_b64: []const u8 = "",
    iterations: u32 = 0,

    static fn parse(raw: []const u8): PgScramServerFirst! {
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
                const val = item[2..item.len];
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

        const r_val = r_opt ?? return err.InvalidMessage;
        const s_val = s_opt ?? return err.InvalidMessage;
        const i_str = i_opt ?? return err.InvalidMessage;
        const iters = parsePositiveU32(i_str)?;

        return PgScramServerFirst{
            raw: raw,
            nonce: r_val,
            salt_b64: s_val,
            iterations: iters,
        };
    }

    fn eql(other: PgScramServerFirst): bool {
        return bytesEql(_.nonce, other.nonce) &&
            bytesEql(_.salt_b64, other.salt_b64) &&
            _.iterations == other.iterations;
    }
}

/// SCRAM-SHA-256 `AuthenticationSASLFinal` (`v=<verifier_b64>` 或 `e=<err_msg>`) 零拷贝解析结果
pub struct PgScramServerFinal {
    raw: []const u8 = "",
    verifier_b64: ([]const u8)? = null,
    err_msg: ([]const u8)? = null,

    static fn parse(raw: []const u8): PgScramServerFinal! {
        let v_opt: ([]const u8)? = null;
        let e_opt: ([]const u8)? = null;

        let pos: usize = 0;
        while (pos <= raw.len) {
            let end = pos;
            while (end < raw.len && raw[end] != ',') {
                end += 1;
            }
            const item = raw[pos..end];
            if (item.len > 2 && item[1] == '=') {
                const key = item[0];
                const val = item[2..item.len];
                if (key == 'v') {
                    v_opt = val;
                } else if (key == 'e') {
                    e_opt = val;
                }
            }
            if (end >= raw.len) {
                break;
            }
            pos = end + 1;
        }
        if (v_opt == null && e_opt == null) {
            return err.InvalidMessage;
        }
        return PgScramServerFinal{
            raw: raw,
            verifier_b64: v_opt,
            err_msg: e_opt,
        };
    }

    fn isVerified(): bool {
        return _.verifier_b64 != null && _.err_msg == null;
    }
}

/// `AuthenticationSASL` 服务端通告的 SASL 机制列表（固定容量 8，零堆分配）
pub struct PgSaslMechanismList {
    items: [PG_AUTH_MAX_SASL_MECHANISMS][]const u8 = [""; PG_AUTH_MAX_SASL_MECHANISMS],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): ([]const u8)? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }

    fn supports(mech: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (bytesEql(_.items[i], mech)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn supportsScramSha256(): bool {
        return _.supports("SCRAM-SHA-256");
    }

    fn supportsScramSha256Plus(): bool {
        return _.supports("SCRAM-SHA-256-PLUS");
    }
}

/// PostgreSQL v3.0 Backend `Authentication` (`'R'`) 统一零堆分配认证挑战帧
///
/// 线格式：`['R'][len: i32 BE][auth_code: u32 BE][extra_payload?]`
pub struct PgAuthChallenge {
    kind: PgAuthChallengeKind = PgAuthChallengeKind.Unknown,
    auth_code: u32 = 0,
    md5_salt: [4]u8 = [0, 0, 0, 0],
    data: []const u8 = "",
    sasl_mechanisms: PgSaslMechanismList = PgSaslMechanismList{},
    scram_first: PgScramServerFirst = PgScramServerFirst{},

    static fn ok(): PgAuthChallenge {
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.Ok,
            auth_code: PG_AUTH_CODE_OK,
        };
    }

    static fn cleartext(): PgAuthChallenge {
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.CleartextPassword,
            auth_code: PG_AUTH_CODE_CLEARTEXT,
        };
    }

    static fn md5(salt: [4]u8): PgAuthChallenge {
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.Md5Password,
            auth_code: PG_AUTH_CODE_MD5,
            md5_salt: salt,
        };
    }

    static fn saslContinue(server_first_raw: []const u8): PgAuthChallenge! {
        const parsed = PgScramServerFirst.parse(server_first_raw)?;
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.SaslContinue,
            auth_code: PG_AUTH_CODE_SASL_CONTINUE,
            data: server_first_raw,
            scram_first: parsed,
        };
    }

    static fn saslFinal(server_final_raw: []const u8): PgAuthChallenge {
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.SaslFinal,
            auth_code: PG_AUTH_CODE_SASL_FINAL,
            data: server_final_raw,
        };
    }

    static fn gssContinue(token: []const u8): PgAuthChallenge {
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.GssContinue,
            auth_code: PG_AUTH_CODE_GSS_CONTINUE,
            data: token,
        };
    }

    /// 构造 `AuthenticationSASL` 机制通告帧（最多 8 个机制名）
    static fn sasl(mechanisms: []const []const u8): PgAuthChallenge! {
        if (mechanisms.len > PG_AUTH_MAX_SASL_MECHANISMS) {
            return err.TooManyMechanisms;
        }
        let list = PgSaslMechanismList{};
        let i: usize = 0;
        while (i < mechanisms.len) {
            list.items[i] = mechanisms[i];
            i += 1;
        }
        list.len = mechanisms.len;
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.Sasl,
            auth_code: PG_AUTH_CODE_SASL,
            sasl_mechanisms: list,
        };
    }

    fn isOk(): bool {
        return _.kind == PgAuthChallengeKind.Ok;
    }

    fn requiresPassword(): bool {
        return _.kind == PgAuthChallengeKind.CleartextPassword ||
            _.kind == PgAuthChallengeKind.Md5Password;
    }

    fn isSaslStep(): bool {
        return _.kind == PgAuthChallengeKind.Sasl ||
            _.kind == PgAuthChallengeKind.SaslContinue ||
            _.kind == PgAuthChallengeKind.SaslFinal;
    }

    /// 当 `kind == SaslFinal` 时，零拷贝解析 `v=<verifier_b64>` / `e=<err_msg>`
    fn parseServerFinal(): PgScramServerFinal! {
        if (_.kind != PgAuthChallengeKind.SaslFinal) {
            return err.InvalidMessage;
        }
        return PgScramServerFinal.parse(_.data);
    }

    fn encodedLen(): usize {
        if (_.kind == PgAuthChallengeKind.Md5Password) {
            return PG_AUTH_HEADER_LEN + 4;
        }
        if (_.kind == PgAuthChallengeKind.Sasl) {
            let total: usize = PG_AUTH_HEADER_LEN + 1; // trailing 0x00
            let i: usize = 0;
            while (i < _.sasl_mechanisms.len) {
                total += _.sasl_mechanisms.items[i].len + 1;
                i += 1;
            }
            return total;
        }
        if (_.kind == PgAuthChallengeKind.GssContinue ||
            _.kind == PgAuthChallengeKind.SaslContinue ||
            _.kind == PgAuthChallengeKind.SaslFinal) {
            return PG_AUTH_HEADER_LEN + _.data.len;
        }
        return PG_AUTH_HEADER_LEN;
    }

    /// 编码为完整 `'R'` 线协议帧（零堆分配）
    fn encodeFrame(out: []u8): usize! {
        const total = _.encodedLen();
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        const len_field = total - 1;
        if (len_field > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        out[0] = PG_TAG_AUTHENTICATION;
        writeBeU32(out, 1, @as(u32, @intCast(len_field)));
        writeBeU32(out, 5, _.auth_code);

        if (_.kind == PgAuthChallengeKind.Md5Password) {
            out[9] = _.md5_salt[0];
            out[10] = _.md5_salt[1];
            out[11] = _.md5_salt[2];
            out[12] = _.md5_salt[3];
            return 13;
        }
        if (_.kind == PgAuthChallengeKind.Sasl) {
            let pos: usize = PG_AUTH_HEADER_LEN;
            let i: usize = 0;
            while (i < _.sasl_mechanisms.len) {
                const m = _.sasl_mechanisms.items[i];
                copyBytes(out, pos, m);
                pos += m.len;
                out[pos] = 0;
                pos += 1;
                i += 1;
            }
            out[pos] = 0;
            return pos + 1;
        }
        if (_.kind == PgAuthChallengeKind.GssContinue ||
            _.kind == PgAuthChallengeKind.SaslContinue ||
            _.kind == PgAuthChallengeKind.SaslFinal) {
            copyBytes(out, PG_AUTH_HEADER_LEN, _.data);
            return PG_AUTH_HEADER_LEN + _.data.len;
        }
        return PG_AUTH_HEADER_LEN;
    }

    /// 零拷贝解码完整 `'R'` `Authentication` 线协议帧（完整对齐 Bun 12 种认证子类型校验）
    static fn decodeFrame(frame: []const u8): PgAuthChallenge! {
        if (frame.len < PG_AUTH_HEADER_LEN) {
            return err.Truncated;
        }
        if (frame[0] != PG_TAG_AUTHENTICATION) {
            return err.InvalidTag;
        }
        const msg_len = @as(usize, readBeU32(frame, 1));
        if (msg_len < 8) {
            return err.InvalidLength;
        }
        const expected_total = 1 + msg_len;
        if (frame.len < expected_total) {
            return err.Truncated;
        }
        if (frame.len > expected_total) {
            return err.TrailingBytes;
        }

        const code = readBeU32(frame, 5);
        const extra = frame[PG_AUTH_HEADER_LEN..expected_total];

        if (code == PG_AUTH_CODE_OK) {
            if (msg_len != 8) return err.InvalidLength;
            return PgAuthChallenge.ok();
        }
        if (code == PG_AUTH_CODE_KERBEROS_V5) {
            if (msg_len != 8) return err.InvalidLength;
            return PgAuthChallenge{ kind: PgAuthChallengeKind.KerberosV5, auth_code: code };
        }
        if (code == PG_AUTH_CODE_CLEARTEXT) {
            if (msg_len != 8) return err.InvalidLength;
            return PgAuthChallenge.cleartext();
        }
        if (code == PG_AUTH_CODE_MD5) {
            if (msg_len != 12) return err.InvalidLength;
            const salt: [4]u8 = [extra[0], extra[1], extra[2], extra[3]];
            return PgAuthChallenge.md5(salt);
        }
        if (code == PG_AUTH_CODE_SCM_CREDENTIAL) {
            if (msg_len != 8) return err.InvalidLength;
            return PgAuthChallenge{ kind: PgAuthChallengeKind.ScmCredential, auth_code: code };
        }
        if (code == PG_AUTH_CODE_GSS) {
            if (msg_len != 8) return err.InvalidLength;
            return PgAuthChallenge{ kind: PgAuthChallengeKind.Gss, auth_code: code };
        }
        if (code == PG_AUTH_CODE_GSS_CONTINUE) {
            if (msg_len < 9) return err.InvalidLength;
            return PgAuthChallenge.gssContinue(extra);
        }
        if (code == PG_AUTH_CODE_SSPI) {
            if (msg_len != 8) return err.InvalidLength;
            return PgAuthChallenge{ kind: PgAuthChallengeKind.Sspi, auth_code: code };
        }
        if (code == PG_AUTH_CODE_SASL) {
            if (msg_len < 9) return err.InvalidLength;
            let list = PgSaslMechanismList{};
            let pos: usize = 0;
            let saw_end = false;
            while (pos < extra.len) {
                if (extra[pos] == 0) {
                    pos += 1;
                    saw_end = true;
                    break;
                }
                let nul_pos = pos;
                while (nul_pos < extra.len && extra[nul_pos] != 0) {
                    nul_pos += 1;
                }
                if (nul_pos >= extra.len) {
                    return err.MissingNulTerminator;
                }
                if (list.len >= PG_AUTH_MAX_SASL_MECHANISMS) {
                    return err.TooManyMechanisms;
                }
                list.items[list.len] = extra[pos..nul_pos];
                list.len += 1;
                pos = nul_pos + 1;
            }
            if (!saw_end) {
                return err.MissingNulTerminator;
            }
            if (pos != extra.len) {
                return err.TrailingBytes;
            }
            return PgAuthChallenge{
                kind: PgAuthChallengeKind.Sasl,
                auth_code: code,
                data: extra,
                sasl_mechanisms: list,
            };
        }
        if (code == PG_AUTH_CODE_SASL_CONTINUE) {
            if (msg_len < 9) return err.InvalidLength;
            return PgAuthChallenge.saslContinue(extra);
        }
        if (code == PG_AUTH_CODE_SASL_FINAL) {
            if (msg_len < 9) return err.InvalidLength;
            return PgAuthChallenge.saslFinal(extra);
        }
        return PgAuthChallenge{
            kind: PgAuthChallengeKind.Unknown,
            auth_code: code,
            data: extra,
        };
    }
}
