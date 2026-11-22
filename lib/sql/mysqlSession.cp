// Source: Absorbed from /data/dev/bun/src/sql/mysql/ConnectionState.cp | Synced: 2026-11-22T12:58:34Z
// Pure-CP zero-allocation MySQL 5.7+/8.0+ session handshake & capability negotiation state machine
// (combining Bun's ConnectionState, Capabilities, StatusFlags, SSLMode & AuthMethod):
// Disconnected -> Connecting -> Handshaking -> Authenticating -> [AwaitingPublicKey] -> Connected / Failed,
// strict TLS upgrade enforcement, capability intersection with reject-mask, and server status flag inspection.

/// MySQL 协议核心能力位常量（对齐 Bun `Capabilities.cp`）
pub const MYSQL_SESSION_CAP_CONNECT_WITH_DB: u32 = 8;
pub const MYSQL_SESSION_CAP_PROTOCOL_41: u32 = 512;
pub const MYSQL_SESSION_CAP_SSL: u32 = 2048;
pub const MYSQL_SESSION_CAP_TRANSACTIONS: u32 = 8192;
pub const MYSQL_SESSION_CAP_SECURE_CONNECTION: u32 = 32768;
pub const MYSQL_SESSION_CAP_MULTI_STATEMENTS: u32 = 65536;
pub const MYSQL_SESSION_CAP_MULTI_RESULTS: u32 = 131072;
pub const MYSQL_SESSION_CAP_PLUGIN_AUTH: u32 = 524288;
pub const MYSQL_SESSION_CAP_DEPRECATE_EOF: u32 = 16777216;

/// Bun `Capabilities.reject()` 主动屏蔽的 13 个客户端不支持/高风险能力位掩码
pub const MYSQL_SESSION_CAP_REJECT_MASK: u32 =
    16 | 32 | 64 | 128 | 1024 | 4096 | 33554432 | 67108864 | 134217728 | 268435456 | 536870912 | 1073741824 | 2147483648;

/// MySQL 服务器状态标志位常量（对齐 Bun `StatusFlags.cp`）
pub const MYSQL_SESSION_STATUS_IN_TRANS: u16 = 1;
pub const MYSQL_SESSION_STATUS_AUTOCOMMIT: u16 = 2;
pub const MYSQL_SESSION_STATUS_MORE_RESULTS: u16 = 8;
pub const MYSQL_SESSION_STATUS_NO_BACKSLASH_ESCAPES: u16 = 512;
pub const MYSQL_SESSION_STATUS_QUERY_SLOW: u16 = 2048;
pub const MYSQL_SESSION_STATUS_IN_TRANS_READONLY: u16 = 8192;
pub const MYSQL_SESSION_STATUS_SESSION_STATE_CHANGED: u16 = 16384;

/// MySQL 会话协商与状态迁移错误集
pub err MySqlSessionError {
    InvalidStateTransition,
    Protocol41Required,
    TlsRequiredByClient,
    UnsupportedAuthPlugin,
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

/// MySQL 连接状态机阶段（对齐 Bun `ConnectionState.cp`）
pub enum MySqlSessionState {
    Disconnected,
    Connecting,
    Handshaking,
    Authenticating,
    AwaitingPublicKey,
    Connected,
    Failed,
}

/// MySQL TLS 模式（对齐 Bun `SSLMode.cp`）
pub enum MySqlSessionSslMode {
    Disable,
    Prefer,
    Require,
    VerifyCa,
    VerifyFull,
}

/// MySQL TLS 握手实时状态
pub enum MySqlSessionTlsStatus {
    None,
    Pending,
    Active,
    Failed,
}

/// MySQL 认证插件类型（对齐 Bun `AuthMethod.cp`）
pub enum MySqlSessionAuthPlugin {
    MysqlNativePassword,
    CachingSha2Password,
    Sha256Password,

    /// 返回对应认证算法产生的 scramble 字节长度（20 / 32 / 32）
    fn scrambleLength(): usize {
        switch (_) {
            MySqlSessionAuthPlugin.MysqlNativePassword => return 20,
            MySqlSessionAuthPlugin.CachingSha2Password => return 32,
            MySqlSessionAuthPlugin.Sha256Password => return 32,
        }
    }

    /// 明文连接下是否可能进入 RSA 公钥交换（`AwaitingPublicKey`）
    fn mayRequestPublicKey(tls_active: bool): bool {
        if (tls_active) {
            return false;
        }
        return _ == MySqlSessionAuthPlugin.CachingSha2Password || _ == MySqlSessionAuthPlugin.Sha256Password;
    }
}

/// 将服务端握手包中的认证插件名解析为 `MySqlSessionAuthPlugin`
pub fn parseMySqlAuthPlugin(name: []const u8): MySqlSessionAuthPlugin! {
    if (bytesEql(name, "mysql_native_password")) return MySqlSessionAuthPlugin.MysqlNativePassword;
    if (bytesEql(name, "caching_sha2_password")) return MySqlSessionAuthPlugin.CachingSha2Password;
    if (bytesEql(name, "sha256_password")) return MySqlSessionAuthPlugin.Sha256Password;
    return err.UnsupportedAuthPlugin;
}

/// MySQL 服务器状态位只读视图（对齐 Bun `StatusFlags.cp`）
pub struct MySqlSessionServerStatus {
    bits: u16 = 0,

    static fn fromBits(bits: u16): MySqlSessionServerStatus {
        return MySqlSessionServerStatus{ bits: bits };
    }

    fn inTransaction(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_IN_TRANS) != 0;
    }

    fn isAutoCommit(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_AUTOCOMMIT) != 0;
    }

    fn hasMoreResults(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_MORE_RESULTS) != 0;
    }

    fn isNoBackslashEscapes(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_NO_BACKSLASH_ESCAPES) != 0;
    }

    fn isQuerySlow(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_QUERY_SLOW) != 0;
    }

    fn isReadOnlyTransaction(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_IN_TRANS_READONLY) != 0;
    }

    fn sessionStateChanged(): bool {
        return (_.bits & MYSQL_SESSION_STATUS_SESSION_STATE_CHANGED) != 0;
    }
}

/// 构造 Bun 默认客户端能力位掩码（对齐 `Capabilities.getDefaultCapabilities`）
pub fn defaultMySqlClientCapabilities(ssl_enabled: bool, has_db_name: bool): u32 {
    let caps: u32 = MYSQL_SESSION_CAP_PROTOCOL_41 |
        MYSQL_SESSION_CAP_PLUGIN_AUTH |
        MYSQL_SESSION_CAP_SECURE_CONNECTION |
        MYSQL_SESSION_CAP_DEPRECATE_EOF |
        MYSQL_SESSION_CAP_MULTI_STATEMENTS |
        MYSQL_SESSION_CAP_MULTI_RESULTS;
    if (has_db_name) {
        caps |= MYSQL_SESSION_CAP_CONNECT_WITH_DB;
    }
    if (ssl_enabled) {
        caps |= MYSQL_SESSION_CAP_SSL;
    }
    return caps;
}

/// 根据客户端 TLS 策略与服务端 `CLIENT_SSL` 能力位，决定是否升级 TLS；
/// 若客户端要求 `Require`/`VerifyCa`/`VerifyFull` 但服务端未宣告 SSL，返回 `err.TlsRequiredByClient`。
pub fn decideMySqlTlsUpgrade(mode: MySqlSessionSslMode, server_supports_ssl: bool): bool! {
    switch (mode) {
        MySqlSessionSslMode.Disable => return false,
        MySqlSessionSslMode.Prefer => return server_supports_ssl,
        MySqlSessionSslMode.Require => {
            if (!server_supports_ssl) return err.TlsRequiredByClient;
            return true;
        },
        MySqlSessionSslMode.VerifyCa => {
            if (!server_supports_ssl) return err.TlsRequiredByClient;
            return true;
        },
        MySqlSessionSslMode.VerifyFull => {
            if (!server_supports_ssl) return err.TlsRequiredByClient;
            return true;
        },
    }
}

/// 计算客户端与服务端握手后的最终能力交集（剔除 `MYSQL_SESSION_CAP_REJECT_MASK` 并强制要求 `CLIENT_PROTOCOL_41`）
pub fn negotiateMySqlCapabilities(server_caps: u32, ssl_enabled: bool, has_db_name: bool): u32! {
    if ((server_caps & MYSQL_SESSION_CAP_PROTOCOL_41) == 0) {
        return err.Protocol41Required;
    }
    const client_caps = defaultMySqlClientCapabilities(ssl_enabled, has_db_name);
    const filtered_server = server_caps & ~MYSQL_SESSION_CAP_REJECT_MASK;
    return client_caps & filtered_server;
}

/// 零堆分配 MySQL 5.7+/8.0+ 会话握手与状态管理机
pub struct MySqlSessionMachine {
    state: MySqlSessionState = MySqlSessionState.Disconnected,
    ssl_mode: MySqlSessionSslMode = MySqlSessionSslMode.Prefer,
    tls_status: MySqlSessionTlsStatus = MySqlSessionTlsStatus.None,
    has_db_name: bool = false,
    negotiated_caps: u32 = 0,
    auth_plugin: MySqlSessionAuthPlugin = MySqlSessionAuthPlugin.CachingSha2Password,
    server_status: MySqlSessionServerStatus = MySqlSessionServerStatus{},
    auth_switch_count: u32 = 0,

    static fn init(ssl_mode: MySqlSessionSslMode, has_db_name: bool): MySqlSessionMachine {
        return MySqlSessionMachine{
            ssl_mode: ssl_mode,
            has_db_name: has_db_name,
        };
    }

    fn beginConnect(*_): bool! {
        if (_.state != MySqlSessionState.Disconnected) {
            return err.InvalidStateTransition;
        }
        _.state = MySqlSessionState.Connecting;
        return true;
    }

    fn onTcpConnected(*_): bool! {
        if (_.state != MySqlSessionState.Connecting) {
            return err.InvalidStateTransition;
        }
        _.state = MySqlSessionState.Handshaking;
        return true;
    }

    /// 处理服务端 Initial Handshake Packet（协议能力协商 + TLS 升级决策 + 插件识别）
    fn onServerGreeting(*_, server_caps: u32, plugin_name: []const u8, status_bits: u16): bool! {
        if (_.state != MySqlSessionState.Handshaking) {
            return err.InvalidStateTransition;
        }
        const server_ssl = (server_caps & MYSQL_SESSION_CAP_SSL) != 0;
        const use_tls = decideMySqlTlsUpgrade(_.ssl_mode, server_ssl)?;
        const caps = negotiateMySqlCapabilities(server_caps, use_tls, _.has_db_name)?;
        const plugin = parseMySqlAuthPlugin(plugin_name)?;

        _.negotiated_caps = caps;
        _.auth_plugin = plugin;
        _.server_status = MySqlSessionServerStatus.fromBits(status_bits);
        _.tls_status = if (use_tls) MySqlSessionTlsStatus.Pending else MySqlSessionTlsStatus.None;
        _.state = MySqlSessionState.Authenticating;
        return use_tls;
    }

    fn onTlsUpgraded(*_): bool! {
        if (_.tls_status != MySqlSessionTlsStatus.Pending) {
            return err.InvalidStateTransition;
        }
        _.tls_status = MySqlSessionTlsStatus.Active;
        return true;
    }

    /// 处理明文 `caching_sha2_password` / `sha256_password` 请求 RSA 公钥事件
    fn onPublicKeyRequired(*_): bool! {
        if (_.state != MySqlSessionState.Authenticating) {
            return err.InvalidStateTransition;
        }
        _.state = MySqlSessionState.AwaitingPublicKey;
        return true;
    }

    /// 处理服务端 `AuthSwitchRequest` 切换认证插件
    fn onAuthSwitch(*_, new_plugin_name: []const u8): bool! {
        if (_.state != MySqlSessionState.Authenticating && _.state != MySqlSessionState.AwaitingPublicKey) {
            return err.InvalidStateTransition;
        }
        _.auth_plugin = parseMySqlAuthPlugin(new_plugin_name)?;
        _.auth_switch_count += 1;
        _.state = MySqlSessionState.Authenticating;
        return true;
    }

    /// 认证成功，进入 `Connected` 就绪态
    fn onAuthOk(*_, status_bits: u16): bool! {
        if (_.state != MySqlSessionState.Authenticating && _.state != MySqlSessionState.AwaitingPublicKey) {
            return err.InvalidStateTransition;
        }
        _.server_status = MySqlSessionServerStatus.fromBits(status_bits);
        _.state = MySqlSessionState.Connected;
        return true;
    }

    fn onFatalError(*_) {
        _.state = MySqlSessionState.Failed;
        if (_.tls_status == MySqlSessionTlsStatus.Pending) {
            _.tls_status = MySqlSessionTlsStatus.Failed;
        }
    }

    fn updateServerStatus(*_, status_bits: u16) {
        _.server_status = MySqlSessionServerStatus.fromBits(status_bits);
    }

    fn isReadyForQuery(): bool {
        return _.state == MySqlSessionState.Connected;
    }

    fn usesDeprecateEof(): bool {
        return (_.negotiated_caps & MYSQL_SESSION_CAP_DEPRECATE_EOF) != 0;
    }
}
