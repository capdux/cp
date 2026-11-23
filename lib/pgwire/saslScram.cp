// Source: Absorbed from /data/dev/bun/src/sql_jsc/postgres/SASL.cp | Synced: 2026-11-23T01:59:25Z
// Pure-CP zero-allocation PostgreSQL SCRAM-SHA-256 (RFC 5802 / RFC 7677) client SASL state machine.
// Computes SaltedPassword (PBKDF2-HMAC-SHA256), ClientKey, StoredKey, ClientProof, and
// constant-time ServerSignature verification without any dynamic heap allocation.

const std = @import("std");

/// Bun 默认客户端原始随机 Nonce 字节数（18 字节 -> 24 字节标准 Base64）
pub const PG_SCRAM_NONCE_RAW_LEN: usize = 18;

/// 客户端 Base64 Nonce 最大支持长度（字节）
pub const PG_SCRAM_NONCE_MAX_LEN: usize = 64;

/// SHA-256 派生密钥与签名原始长度（32 字节）
pub const PG_SCRAM_KEY_LEN: usize = 32;

/// 32 字节摘要经标准 Base64 编码后的固定长度（44 字节）
pub const PG_SCRAM_SIG_B64_LEN: usize = 44;

/// 零堆分配解码 Base64 Salt 的最大原始字节数
pub const PG_SCRAM_MAX_SALT_LEN: usize = 64;

/// PostgreSQL SCRAM-SHA-256 客户端握手阶段
pub enum PgSaslStatus {
    Init,
    Continue,
    Verified,
}

/// PostgreSQL SCRAM-SHA-256 认证状态机错误集
pub err PgSaslError {
    InvalidNonce,
    InvalidSalt,
    InvalidIterations,
    SaltedPasswordNotComputed,
    ServerSignatureNotComputed,
    ServerSignatureAlreadyComputed,
    ServerSignatureMismatch,
    BufferTooSmall,
    CryptoFailed,
}

const B64_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

fn encodeBase64ToSlice(src: []const u8, out: []u8): PgSaslError!usize {
    const needed = ((src.len + 2) / 3) * 4;
    if (out.len < needed) {
        return PgSaslError.BufferTooSmall;
    }
    let i: usize = 0;
    let j: usize = 0;
    while (i + 3 <= src.len) {
        const b0 = @as(u32, src[i]);
        const b1 = @as(u32, src[i + 1]);
        const b2 = @as(u32, src[i + 2]);
        const triple = (b0 << 16) | (b1 << 8) | b2;
        out[j] = B64_ALPHABET[@as(usize, (triple >> 18) & 0x3F)];
        out[j + 1] = B64_ALPHABET[@as(usize, (triple >> 12) & 0x3F)];
        out[j + 2] = B64_ALPHABET[@as(usize, (triple >> 6) & 0x3F)];
        out[j + 3] = B64_ALPHABET[@as(usize, triple & 0x3F)];
        i += 3;
        j += 4;
    }
    const rem = src.len - i;
    if (rem == 1) {
        const b0 = @as(u32, src[i]);
        out[j] = B64_ALPHABET[@as(usize, (b0 >> 2) & 0x3F)];
        out[j + 1] = B64_ALPHABET[@as(usize, (b0 << 4) & 0x3F)];
        out[j + 2] = '=';
        out[j + 3] = '=';
        j += 4;
    } else if (rem == 2) {
        const b0 = @as(u32, src[i]);
        const b1 = @as(u32, src[i + 1]);
        const pair = (b0 << 8) | b1;
        out[j] = B64_ALPHABET[@as(usize, (pair >> 10) & 0x3F)];
        out[j + 1] = B64_ALPHABET[@as(usize, (pair >> 4) & 0x3F)];
        out[j + 2] = B64_ALPHABET[@as(usize, (pair << 2) & 0x3F)];
        out[j + 3] = '=';
        j += 4;
    }
    return j;
}

fn decodeBase64Char(ch: u8): u8 {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z') {
        return (ch - 'a') + 26;
    }
    if (ch >= '0' && ch <= '9') {
        return (ch - '0') + 52;
    }
    if (ch == '+') {
        return 62;
    }
    if (ch == '/') {
        return 63;
    }
    return 0xFF;
}

fn decodeBase64ToSlice(src: []const u8, out: []u8): PgSaslError!usize {
    if (src.len == 0 || (src.len % 4) != 0) {
        return PgSaslError.InvalidSalt;
    }
    let i: usize = 0;
    let j: usize = 0;
    while (i < src.len) {
        const c0 = decodeBase64Char(src[i]);
        const c1 = decodeBase64Char(src[i + 1]);
        if (c0 == 0xFF || c1 == 0xFF) {
            return PgSaslError.InvalidSalt;
        }
        const pad2 = src[i + 2] == '=';
        const pad3 = src[i + 3] == '=';
        if (pad2 && !pad3) {
            return PgSaslError.InvalidSalt;
        }
        if ((pad2 || pad3) && (i + 4 < src.len)) {
            return PgSaslError.InvalidSalt;
        }
        const c2: u8 = if (pad2) 0 else decodeBase64Char(src[i + 2]);
        const c3: u8 = if (pad3) 0 else decodeBase64Char(src[i + 3]);
        if ((!pad2 && c2 == 0xFF) || (!pad3 && c3 == 0xFF)) {
            return PgSaslError.InvalidSalt;
        }
        if (j >= out.len) {
            return PgSaslError.BufferTooSmall;
        }
        out[j] = (c0 << 2) | (c1 >> 4);
        j += 1;
        if (!pad2) {
            if (j >= out.len) {
                return PgSaslError.BufferTooSmall;
            }
            out[j] = ((c1 & 0x0F) << 4) | (c2 >> 2);
            j += 1;
        }
        if (!pad3) {
            if (j >= out.len) {
                return PgSaslError.BufferTooSmall;
            }
            out[j] = ((c2 & 0x03) << 6) | c3;
            j += 1;
        }
        i += 4;
    }
    return j;
}

fn hmacSha256(key: []const u8, msg: []const u8): [32]u8 {
    let out: [32]u8 = [0; 32];
    std.crypto.auth.hmac.sha2.HmacSha256.create(&out, msg, key);
    return out;
}

fn sha256(msg: []const u8): [32]u8 {
    let out: [32]u8 = [0; 32];
    std.crypto.hash.sha2.Sha256.hash(msg, &out, .{});
    return out;
}

fn constantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff |= a[i] ^ b[i];
        i += 1;
    }
    return diff == 0;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): PgSaslError!usize {
    if (pos + src.len > out.len) {
        return PgSaslError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// 按 RFC 5802 §5.1 转义 SCRAM 用户名中的 `=` (`=3D`) 与 `,` (`=2C`)。
pub fn escapeScramUsername(user: []const u8, out: []u8): PgSaslError!usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < user.len) {
        const ch = user[i];
        if (ch == '=') {
            pos = appendBytes(out, pos, "=3D")?;
        } else if (ch == ',') {
            pos = appendBytes(out, pos, "=2C")?;
        } else {
            if (pos >= out.len) {
                return PgSaslError.BufferTooSmall;
            }
            out[pos] = ch;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

/// PostgreSQL SCRAM-SHA-256 客户端认证状态机（零动态堆分配，对应 Bun `SASL`）。
pub struct PgSaslState {
    nonce_base64_bytes: [PG_SCRAM_NONCE_MAX_LEN]u8 = [0; PG_SCRAM_NONCE_MAX_LEN],
    nonce_len: u8 = 0,

    server_signature_base64_bytes: [PG_SCRAM_SIG_B64_LEN]u8 = [0; PG_SCRAM_SIG_B64_LEN],
    server_signature_len: u8 = 0,

    salted_password_bytes: [PG_SCRAM_KEY_LEN]u8 = [0; PG_SCRAM_KEY_LEN],
    salted_password_created: bool = false,

    status: PgSaslStatus = PgSaslStatus.Init,

    ~init() {
        _.reset();
    }

    /// 使用调用方提供的原始随机字节（推荐 18 字节 CSPRNG）编码为标准 Base64 Nonce 并初始化状态机。
    static fn initWithNonceBytes(raw_nonce: []const u8): PgSaslError!PgSaslState {
        if (raw_nonce.len == 0 || ((raw_nonce.len + 2) / 3) * 4 > PG_SCRAM_NONCE_MAX_LEN) {
            return PgSaslError.InvalidNonce;
        }
        let st = PgSaslState{};
        const enc_len = encodeBase64ToSlice(raw_nonce, st.nonce_base64_bytes[0..])?;
        st.nonce_len = @as(u8, @intCast(enc_len));
        return st;
    }

    /// 使用已编码的可打印 ASCII Nonce 初始化状态机（拒绝空串、含 `,` 或非可打印 ASCII）。
    static fn initWithBase64Nonce(b64_nonce: []const u8): PgSaslError!PgSaslState {
        if (b64_nonce.len == 0 || b64_nonce.len > PG_SCRAM_NONCE_MAX_LEN) {
            return PgSaslError.InvalidNonce;
        }
        let st = PgSaslState{};
        let i: usize = 0;
        while (i < b64_nonce.len) {
            const ch = b64_nonce[i];
            if (ch < 0x21 || ch > 0x7E || ch == ',') {
                return PgSaslError.InvalidNonce;
            }
            st.nonce_base64_bytes[i] = ch;
            i += 1;
        }
        st.nonce_len = @as(u8, @intCast(b64_nonce.len));
        return st;
    }

    /// 返回当前客户端 Base64 Nonce 切片
    fn nonce(): []const u8 {
        return _.nonce_base64_bytes[0..@as(usize, _.nonce_len)];
    }

    /// 格式化 `client-first-message-bare` (`n=<escaped_user>,r=<nonce>`) 到 `out`
    fn formatClientFirstBare(user: []const u8, out: []u8): PgSaslError!usize {
        if (_.nonce_len == 0) {
            return PgSaslError.InvalidNonce;
        }
        let pos = appendBytes(out, 0, "n=")?;
        const ulen = escapeScramUsername(user, out[pos..])?;
        pos += ulen;
        pos = appendBytes(out, pos, ",r=")?;
        pos = appendBytes(out, pos, _.nonce())?;
        return pos;
    }

    /// 格式化完整 `client-first-message` (`n,,n=<escaped_user>,r=<nonce>`) 到 `out`
    fn formatClientFirst(user: []const u8, out: []u8): PgSaslError!usize {
        let pos = appendBytes(out, 0, "n,,")?;
        const bare_len = _.formatClientFirstBare(user, out[pos..])?;
        return pos + bare_len;
    }

    /// 使用 PBKDF2-HMAC-SHA256 派生 32 字节 `SaltedPassword`（零堆分配）
    fn computeSaltedPassword(*_, password: []const u8, salt_bytes: []const u8, iteration_count: u32): PgSaslError!usize {
        if (salt_bytes.len == 0) {
            return PgSaslError.InvalidSalt;
        }
        if (iteration_count == 0) {
            return PgSaslError.InvalidIterations;
        }
        std.crypto.pwhash.pbkdf2(
            _.salted_password_bytes[0..],
            password,
            salt_bytes,
            iteration_count,
            std.crypto.auth.hmac.sha2.HmacSha256,
        ) catch {
            return PgSaslError.CryptoFailed;
        };
        _.salted_password_created = true;
        _.status = PgSaslStatus.Continue;
        return PG_SCRAM_KEY_LEN;
    }

    /// 解码服务端 `s=<salt_b64>` 并派生 32 字节 `SaltedPassword`（零堆分配）
    fn computeSaltedPasswordB64(*_, password: []const u8, salt_b64: []const u8, iteration_count: u32): PgSaslError!usize {
        let salt_buf: [PG_SCRAM_MAX_SALT_LEN]u8 = [0; PG_SCRAM_MAX_SALT_LEN];
        const salt_len = decodeBase64ToSlice(salt_b64, salt_buf[0..])?;
        return _.computeSaltedPassword(password, salt_buf[0..salt_len], iteration_count);
    }

    /// 返回已派生的 32 字节 `SaltedPassword` 视图
    fn saltedPassword(): PgSaslError![]const u8 {
        if (!_.salted_password_created) {
            return PgSaslError.SaltedPasswordNotComputed;
        }
        return _.salted_password_bytes[0..PG_SCRAM_KEY_LEN];
    }

    /// 计算 `ClientKey = HMAC(SaltedPassword, "Client Key")`
    fn clientKey(): PgSaslError![32]u8 {
        const sp = _.saltedPassword()?;
        return hmacSha256(sp, "Client Key");
    }

    /// 计算 `StoredKey = SHA256(ClientKey)`
    fn storedKey(): PgSaslError![32]u8 {
        const ck = _.clientKey()?;
        return sha256(ck[0..]);
    }

    /// 计算 `ClientSignature = HMAC(SHA256(client_key), auth_message)`
    static fn clientKeySignature(client_key: []const u8, auth_message: []const u8): [32]u8 {
        const sk = sha256(client_key);
        return hmacSha256(sk[0..], auth_message);
    }

    /// 计算 `ClientProof = ClientKey XOR ClientSignature` 并 Base64 编码至 `out_b64`（44 字节）
    fn computeClientProof(auth_message: []const u8, out_b64: []u8): PgSaslError!usize {
        const ck = _.clientKey()?;
        const sig = PgSaslState.clientKeySignature(ck[0..], auth_message);
        let proof: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) {
            proof[i] = ck[i] ^ sig[i];
            i += 1;
        }
        return encodeBase64ToSlice(proof[0..], out_b64);
    }

    /// 计算预期服务端签名 `ServerSignature = HMAC(HMAC(SaltedPassword, "Server Key"), auth_message)`
    /// 并以标准 Base64 缓存在内部 `server_signature_base64_bytes` 中。
    fn computeServerSignature(*_, auth_message: []const u8): PgSaslError!usize {
        if (_.server_signature_len != 0) {
            return PgSaslError.ServerSignatureAlreadyComputed;
        }
        const sp = _.saltedPassword()?;
        const server_key = hmacSha256(sp, "Server Key");
        const server_sig = hmacSha256(server_key[0..], auth_message);
        const enc_len = encodeBase64ToSlice(server_sig[0..], _.server_signature_base64_bytes[0..])?;
        _.server_signature_len = @as(u8, @intCast(enc_len));
        return enc_len;
    }

    /// 返回已缓存的预期服务端 Base64 签名视图
    fn serverSignature(): PgSaslError![]const u8 {
        if (_.server_signature_len == 0) {
            return PgSaslError.ServerSignatureNotComputed;
        }
        return _.server_signature_base64_bytes[0..@as(usize, _.server_signature_len)];
    }

    /// 恒定时间比对服务端 `AuthenticationSASLFinal` 返回的 `v=<verifier_b64>` 签名。
    /// 校验通过则将状态推进至 `PgSaslStatus.Verified` 并返回 `true`，不匹配返回 `ServerSignatureMismatch`。
    fn verifyServerSignature(*_, received_b64: []const u8): PgSaslError!bool {
        const expected = _.serverSignature()?;
        if (!constantTimeEql(expected, received_b64)) {
            return PgSaslError.ServerSignatureMismatch;
        }
        _.status = PgSaslStatus.Verified;
        return true;
    }

    /// 格式化完整 `client-final-message` (`c=biws,r=<server_nonce>,p=<client_proof_b64>`) 并同步计算 `serverSignature`。
    fn formatClientFinal(*_, server_nonce: []const u8, auth_message: []const u8, out: []u8): PgSaslError!usize {
        if (server_nonce.len == 0) {
            return PgSaslError.InvalidNonce;
        }
        let pos = appendBytes(out, 0, "c=biws,r=")?;
        pos = appendBytes(out, pos, server_nonce)?;
        pos = appendBytes(out, pos, ",p=")?;
        const proof_len = _.computeClientProof(auth_message, out[pos..])?;
        pos += proof_len;
        if (_.server_signature_len == 0) {
            * = _.computeServerSignature(auth_message)?;
        }
        return pos;
    }

    /// 安全清零内部密钥与签名缓存并重置状态机至 `Init`
    fn reset(*_) {
        let i: usize = 0;
        while (i < PG_SCRAM_KEY_LEN) {
            _.salted_password_bytes[i] = 0;
            i += 1;
        }
        i = 0;
        while (i < PG_SCRAM_SIG_B64_LEN) {
            _.server_signature_base64_bytes[i] = 0;
            i += 1;
        }
        i = 0;
        while (i < PG_SCRAM_NONCE_MAX_LEN) {
            _.nonce_base64_bytes[i] = 0;
            i += 1;
        }
        _.nonce_len = 0;
        _.salted_password_created = false;
        _.server_signature_len = 0;
        _.status = PgSaslStatus.Init;
    }
}
