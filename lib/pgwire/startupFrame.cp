// Source: Absorbed from /data/dev/bun/src/sql/postgres/protocol/StartupMessage.cp | Synced: 2026-11-23T01:31:55Z
// Pure-CP zero-allocation PostgreSQL v3.0 connection handshake frames:
// StartupMessage (v3.0 = 196608), SSLRequest (80877103), and GSSENCRequest (80877104).

/// PostgreSQL v3.0 启动握手协议版本号（`(3 << 16) | 0 = 196608`）
pub const PG_STARTUP_PROTOCOL_V3: u32 = 196608;

/// PostgreSQL `SSLRequest` 协议升级魔数（`(1234 << 16) | 5679 = 80877103`）
pub const PG_STARTUP_SSL_REQUEST_CODE: u32 = 80877103;

/// PostgreSQL `GSSENCRequest` 协议升级魔数（`(1234 << 16) | 5680 = 80877104`）
pub const PG_STARTUP_GSSENC_REQUEST_CODE: u32 = 80877104;

/// `SSLRequest` / `GSSENCRequest` 固定完整帧长度（4B len(8) + 4B code = 8B）
pub const PG_STARTUP_UPGRADE_FRAME_LEN: usize = 8;

/// 单条 `StartupMessage` 允许的最大启动参数键值对数量（零堆分配固定容量）
pub const PG_STARTUP_MAX_PARAMS: usize = 16;

/// PostgreSQL 启动握手帧编解码与边界校验错误集
pub err PgStartupError {
    InvalidLength,
    InvalidProtocolVersion,
    InvalidUpgradeCode,
    Truncated,
    TrailingBytes,
    MissingNulTerminator,
    EmbeddedNul,
    EmptyParamKey,
    MissingUser,
    TooManyParams,
    BufferTooSmall,
}

/// 握手传输层升级类型（TLS SSL 或 GSSAPI 加密）
pub enum PgUpgradeKind {
    Ssl,
    GssEnc,
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

/// 单条 `StartupMessage` 启动参数键值对
pub struct PgStartupParam {
    key: []const u8 = "",
    value: []const u8 = "",

    fn eql(other: PgStartupParam): bool {
        return bytesEql(_.key, other.key) && bytesEql(_.value, other.value);
    }
}

/// PostgreSQL v3.0 `StartupMessage` 连接启动帧（固定容量 16 参数，零堆分配）
///
/// 线格式：`[len: i32 BE][protocol_version: u32 BE]([key\0][value\0])*[0x00]`（无 1B tag）
pub struct PgStartupMessage {
    protocol_version: u32 = PG_STARTUP_PROTOCOL_V3,
    params: [PG_STARTUP_MAX_PARAMS]PgStartupParam = [PgStartupParam{}; PG_STARTUP_MAX_PARAMS],
    len: usize = 0,

    static fn init(): PgStartupMessage {
        return PgStartupMessage{};
    }

    /// 对齐 Bun `StartupMessage` 默认连接构造：
    /// 写入 `user`、`database`（若为空则默认回退为 `user`）以及 `client_encoding = "UTF8"`
    static fn withDefaults(username: []const u8, db_name: []const u8): PgStartupMessage! {
        if (username.len == 0) {
            return err.MissingUser;
        }
        let msg = PgStartupMessage.init();
        * = msg.addParam("user", username)?;
        const db = if (db_name.len == 0) username else db_name;
        * = msg.addParam("database", db)?;
        * = msg.addParam("client_encoding", "UTF8")?;
        return msg;
    }

    /// 追加启动参数键值对（拒绝空键名、内嵌 `0x00` 字节及超容量）
    fn addParam(*_, key: []const u8, value: []const u8): usize! {
        if (key.len == 0) {
            return err.EmptyParamKey;
        }
        if (containsNul(key) || containsNul(value)) {
            return err.EmbeddedNul;
        }
        if (_.len >= PG_STARTUP_MAX_PARAMS) {
            return err.TooManyParams;
        }
        const idx = _.len;
        _.params[idx] = PgStartupParam{
            key: key,
            value: value,
        };
        _.len += 1;
        return idx;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): PgStartupParam? {
        if (idx >= _.len) {
            return null;
        }
        return _.params[idx];
    }

    /// 按参数名查找首个匹配值
    fn get(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) {
            if (bytesEql(_.params[i].key, key)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }

    fn user(): ([]const u8)? {
        return _.get("user");
    }

    fn database(): ([]const u8)? {
        return _.get("database");
    }

    fn clientEncoding(): ([]const u8)? {
        return _.get("client_encoding");
    }

    fn applicationName(): ([]const u8)? {
        return _.get("application_name");
    }

    fn protocolMajor(): u16 {
        return @as(u16, @intCast((_.protocol_version >> 16) & 0xFFFF));
    }

    fn protocolMinor(): u16 {
        return @as(u16, @intCast(_.protocol_version & 0xFFFF));
    }

    /// 计算完整 `StartupMessage` 线协议总字节数
    fn encodedLen(): usize {
        let total: usize = 8 + 1; // 4B len + 4B protocol_version + 1B trailing 0x00
        let i: usize = 0;
        while (i < _.len) {
            total += _.params[i].key.len + 1 + _.params[i].value.len + 1;
            i += 1;
        }
        return total;
    }

    /// 编码为完整 PostgreSQL v3.0 `StartupMessage` 线协议帧（零堆分配）
    fn encodeFrame(out: []u8): usize! {
        if (_.user() == null) {
            return err.MissingUser;
        }
        const total = _.encodedLen();
        if (out.len < total) {
            return err.BufferTooSmall;
        }
        if (total > 0x7FFFFFFF) {
            return err.InvalidLength;
        }
        writeBeU32(out, 0, @as(u32, @intCast(total)));
        writeBeU32(out, 4, _.protocol_version);
        let pos: usize = 8;
        let i: usize = 0;
        while (i < _.len) {
            const p = _.params[i];
            if (p.key.len == 0) {
                return err.EmptyParamKey;
            }
            if (containsNul(p.key) || containsNul(p.value)) {
                return err.EmbeddedNul;
            }
            copyBytes(out, pos, p.key);
            pos += p.key.len;
            out[pos] = 0;
            pos += 1;
            copyBytes(out, pos, p.value);
            pos += p.value.len;
            out[pos] = 0;
            pos += 1;
            i += 1;
        }
        out[pos] = 0;
        return pos + 1;
    }

    /// 零拷贝解码完整 `StartupMessage` 帧（校验版本主号为 3、成对 NUL 终止参数及尾部 0x00）
    static fn decodeFrame(frame: []const u8): PgStartupMessage! {
        if (frame.len < 9) {
            return err.Truncated;
        }
        const declared_len = @as(usize, readBeU32(frame, 0));
        if (declared_len < 9) {
            return err.InvalidLength;
        }
        if (frame.len < declared_len) {
            return err.Truncated;
        }
        if (frame.len > declared_len) {
            return err.TrailingBytes;
        }
        const proto = readBeU32(frame, 4);
        const major = (proto >> 16) & 0xFFFF;
        if (major != 3) {
            return err.InvalidProtocolVersion;
        }

        let msg = PgStartupMessage{ protocol_version: proto };
        let pos: usize = 8;
        let saw_end = false;

        while (pos < declared_len) {
            if (frame[pos] == 0) {
                pos += 1;
                saw_end = true;
                break;
            }
            let k_end = pos;
            while (k_end < declared_len && frame[k_end] != 0) {
                k_end += 1;
            }
            if (k_end >= declared_len) {
                return err.MissingNulTerminator;
            }
            const k = frame[pos..k_end];
            pos = k_end + 1;

            let v_end = pos;
            while (v_end < declared_len && frame[v_end] != 0) {
                v_end += 1;
            }
            if (v_end >= declared_len) {
                return err.MissingNulTerminator;
            }
            const v = frame[pos..v_end];
            pos = v_end + 1;

            * = msg.addParam(k, v)?;
        }

        if (!saw_end) {
            return err.MissingNulTerminator;
        }
        if (pos != declared_len) {
            return err.TrailingBytes;
        }
        return msg;
    }

    fn eql(other: PgStartupMessage): bool {
        if (_.protocol_version != other.protocol_version || _.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.params[i].eql(other.params[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

/// PostgreSQL v3.0 握手前传输升级请求帧（`SSLRequest` / `GSSENCRequest`，固定 8 字节）
pub struct PgTlsUpgradeRequest {
    kind: PgUpgradeKind = PgUpgradeKind.Ssl,

    static fn ssl(): PgTlsUpgradeRequest {
        return PgTlsUpgradeRequest{ kind: PgUpgradeKind.Ssl };
    }

    static fn gssEnc(): PgTlsUpgradeRequest {
        return PgTlsUpgradeRequest{ kind: PgUpgradeKind.GssEnc };
    }

    fn code(): u32 {
        if (_.kind == PgUpgradeKind.Ssl) {
            return PG_STARTUP_SSL_REQUEST_CODE;
        }
        return PG_STARTUP_GSSENC_REQUEST_CODE;
    }

    fn encodedLen(): usize {
        return PG_STARTUP_UPGRADE_FRAME_LEN;
    }

    fn encodeFrame(out: []u8): usize! {
        if (out.len < PG_STARTUP_UPGRADE_FRAME_LEN) {
            return err.BufferTooSmall;
        }
        writeBeU32(out, 0, 8);
        writeBeU32(out, 4, _.code());
        return PG_STARTUP_UPGRADE_FRAME_LEN;
    }

    static fn decodeFrame(frame: []const u8): PgTlsUpgradeRequest! {
        if (frame.len < PG_STARTUP_UPGRADE_FRAME_LEN) {
            return err.Truncated;
        }
        if (frame.len > PG_STARTUP_UPGRADE_FRAME_LEN) {
            return err.TrailingBytes;
        }
        const declared_len = readBeU32(frame, 0);
        if (declared_len != 8) {
            return err.InvalidLength;
        }
        const c = readBeU32(frame, 4);
        if (c == PG_STARTUP_SSL_REQUEST_CODE) {
            return PgTlsUpgradeRequest.ssl();
        }
        if (c == PG_STARTUP_GSSENC_REQUEST_CODE) {
            return PgTlsUpgradeRequest.gssEnc();
        }
        return err.InvalidUpgradeCode;
    }

    /// 判定服务端单字节响应是否接受本次传输层升级（SSL 接受 `'S'`，GSSAPI 接受 `'G'`）
    fn isAcceptedByte(resp: u8): bool {
        if (_.kind == PgUpgradeKind.Ssl) {
            return resp == 'S';
        }
        return resp == 'G';
    }

    /// 判定服务端单字节响应是否明确拒绝升级（`'N'`）
    static fn isRejectedByte(resp: u8): bool {
        return resp == 'N';
    }

    fn eql(other: PgTlsUpgradeRequest): bool {
        return _.kind == other.kind;
    }
}
