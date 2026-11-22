// Source: Absorbed from /data/dev/bun/src/sql/postgres/Status.cp | Synced: 2026-11-22T13:01:43Z
// Pure-CP zero-allocation PostgreSQL v3.0 wire session state machine
// (combining Bun's Status, SSLMode, TLSStatus, AnyPostgresError & CommandTag):
// Disconnected -> Connecting -> SentStartupMessage -> Connected / Failed,
// SSLRequest ('S'/'N') negotiation, ReadyForQuery ('I'/'T'/'E') transaction tracking,
// BackendKeyData recording, and INSERT/DELETE/UPDATE/MERGE/SELECT/MOVE/FETCH/COPY tag parsing.

/// PostgreSQL 会话协商与状态迁移错误集（对齐 Bun `AnyPostgresError.cp`）
pub err PgSessionError {
    InvalidStateTransition,
    TlsNotAvailable,
    TlsUpgradeFailed,
    InvalidBackendKeyData,
    InvalidTxIndicator,
    ConnectionClosed,
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

fn indexOfByte(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseU64Decimal(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit = @as(u64, c - '0');
        if (acc > (0xFFFF_FFFF_FFFF_FFFF - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

/// PostgreSQL 连接状态机阶段（对齐 Bun `postgres/Status.cp`）
pub enum PgSessionState {
    Disconnected,
    Connecting,
    SentStartupMessage,
    Connected,
    Failed,
}

/// PostgreSQL TLS 模式（对齐 Bun `postgres/SSLMode.cp`）
pub enum PgSessionSslMode {
    Disable,
    Prefer,
    Require,
    VerifyCa,
    VerifyFull,
}

/// PostgreSQL TLS 握手实时状态
pub enum PgSessionTlsStatus {
    None,
    Pending,
    Active,
    Failed,
}

/// PostgreSQL `ReadyForQuery` 后端事务状态指示器
pub enum PgSessionTxStatus {
    /// `'I'` — 空闲（不在事务块中）
    Idle,
    /// `'T'` — 活跃事务块中
    InTransaction,
    /// `'E'` — 失败事务块中（必须 ROLLBACK 后才能执行新语句）
    FailedTransaction,
}

/// 将 `ReadyForQuery` 的单字节状态码（`'I'`/`'T'`/`'E'`）解析为 `PgSessionTxStatus`
pub fn parsePgSessionTxStatus(indicator: u8): PgSessionTxStatus? {
    if (indicator == 'I') return PgSessionTxStatus.Idle;
    if (indicator == 'T') return PgSessionTxStatus.InTransaction;
    if (indicator == 'E') return PgSessionTxStatus.FailedTransaction;
    return null;
}

/// PostgreSQL `CommandComplete` 命令分类（对齐 Bun `postgres/CommandTag.cp`）
pub enum PgSessionCmdKind {
    Insert,
    Delete,
    Update,
    Merge,
    Select,
    Move,
    Fetch,
    Copy,
    Other,
}

/// 解析后的 `CommandComplete` 结果
pub struct PgSessionCmdResult {
    kind: PgSessionCmdKind = PgSessionCmdKind.Other,
    row_count: u64 = 0,
    raw_tag: []const u8 = "",
}

/// 解析 PostgreSQL `CommandComplete` 标签（对齐 Bun `CommandTag.init`）：
/// - `INSERT <oid> <rows>` 跳过 `<oid>` 提取 `<rows>`；
/// - `DELETE`/`UPDATE`/`MERGE`/`SELECT`/`MOVE`/`FETCH`/`COPY` 提取空格后的行数；
/// - 其他标签（如 `BEGIN`/`COMMIT`/`CREATE TABLE`）归类为 `Other`。
pub fn parsePgSessionCommandTag(tag: []const u8): PgSessionCmdResult {
    const first_space = indexOfByte(tag, ' ') ?? return PgSessionCmdResult{
        kind: PgSessionCmdKind.Other,
        row_count: 0,
        raw_tag: tag,
    };
    const verb = tag[0..first_space];

    let kind = PgSessionCmdKind.Other;
    if (bytesEql(verb, "INSERT")) {
        kind = PgSessionCmdKind.Insert;
    } else if (bytesEql(verb, "DELETE")) {
        kind = PgSessionCmdKind.Delete;
    } else if (bytesEql(verb, "UPDATE")) {
        kind = PgSessionCmdKind.Update;
    } else if (bytesEql(verb, "MERGE")) {
        kind = PgSessionCmdKind.Merge;
    } else if (bytesEql(verb, "SELECT")) {
        kind = PgSessionCmdKind.Select;
    } else if (bytesEql(verb, "MOVE")) {
        kind = PgSessionCmdKind.Move;
    } else if (bytesEql(verb, "FETCH")) {
        kind = PgSessionCmdKind.Fetch;
    } else if (bytesEql(verb, "COPY")) {
        kind = PgSessionCmdKind.Copy;
    } else {
        return PgSessionCmdResult{
            kind: PgSessionCmdKind.Other,
            row_count: 0,
            raw_tag: tag,
        };
    }

    let num_slice = tag[first_space + 1 ..];
    if (kind == PgSessionCmdKind.Insert) {
        const second_space = indexOfByte(num_slice, ' ') ?? return PgSessionCmdResult{
            kind: PgSessionCmdKind.Other,
            row_count: 0,
            raw_tag: tag,
        };
        num_slice = num_slice[second_space + 1 ..];
    }

    const rows = parseU64Decimal(num_slice) ?? return PgSessionCmdResult{
        kind: PgSessionCmdKind.Other,
        row_count: 0,
        raw_tag: tag,
    };
    return PgSessionCmdResult{
        kind: kind,
        row_count: rows,
        raw_tag: tag,
    };
}

/// 根据 PostgreSQL 服务端对 `SSLRequest` 的单字节回复（`'S'` 或 `'N'`）决定是否升级 TLS：
/// - `'S'`：接受 TLS 升级，返回 `true`；
/// - `'N'`：`Disable`/`Prefer` 回退明文返回 `false`；`Require`/`VerifyCa`/`VerifyFull` 返回 `err.TlsNotAvailable`；
/// - 其他字节：返回 `err.TlsUpgradeFailed`。
pub fn decidePgTlsResponse(mode: PgSessionSslMode, server_reply: u8): bool! {
    if (server_reply == 'S') {
        return mode != PgSessionSslMode.Disable;
    }
    if (server_reply == 'N') {
        switch (mode) {
            PgSessionSslMode.Disable => return false,
            PgSessionSslMode.Prefer => return false,
            PgSessionSslMode.Require => return err.TlsNotAvailable,
            PgSessionSslMode.VerifyCa => return err.TlsNotAvailable,
            PgSessionSslMode.VerifyFull => return err.TlsNotAvailable,
        }
    }
    return err.TlsUpgradeFailed;
}

/// 零堆分配 PostgreSQL v3.0 协议会话状态机
pub struct PgSessionMachine {
    state: PgSessionState = PgSessionState.Disconnected,
    ssl_mode: PgSessionSslMode = PgSessionSslMode.Prefer,
    tls_status: PgSessionTlsStatus = PgSessionTlsStatus.None,
    tx_status: PgSessionTxStatus = PgSessionTxStatus.Idle,
    backend_pid: i32 = 0,
    backend_secret_key: i32 = 0,
    has_backend_key: bool = false,
    last_command: PgSessionCmdResult = PgSessionCmdResult{},
    queries_completed: u64 = 0,

    static fn init(ssl_mode: PgSessionSslMode): PgSessionMachine {
        return PgSessionMachine{ ssl_mode: ssl_mode };
    }

    fn beginConnect(*_): bool! {
        if (_.state != PgSessionState.Disconnected) {
            return err.InvalidStateTransition;
        }
        _.state = PgSessionState.Connecting;
        return true;
    }

    /// 处理 `SSLRequest` 回复字节（`'S'` / `'N'`），返回是否需要进入 TLS 握手
    fn onSslNegotiated(*_, server_reply: u8): bool! {
        if (_.state != PgSessionState.Connecting) {
            return err.InvalidStateTransition;
        }
        const use_tls = decidePgTlsResponse(_.ssl_mode, server_reply)?;
        _.tls_status = if (use_tls) PgSessionTlsStatus.Pending else PgSessionTlsStatus.None;
        return use_tls;
    }

    fn onTlsHandshakeComplete(*_): bool! {
        if (_.tls_status != PgSessionTlsStatus.Pending) {
            return err.InvalidStateTransition;
        }
        _.tls_status = PgSessionTlsStatus.Active;
        return true;
    }

    /// 发送 `StartupMessage` 后迁入 `SentStartupMessage`
    fn onStartupSent(*_): bool! {
        if (_.state != PgSessionState.Connecting) {
            return err.InvalidStateTransition;
        }
        if (_.tls_status == PgSessionTlsStatus.Pending) {
            return err.InvalidStateTransition;
        }
        _.state = PgSessionState.SentStartupMessage;
        return true;
    }

    /// 记录后端进程 PID 与取消查询密钥（`BackendKeyData`）
    fn onBackendKeyData(*_, pid: i32, secret_key: i32): bool! {
        if (_.state != PgSessionState.SentStartupMessage && _.state != PgSessionState.Connected) {
            return err.InvalidStateTransition;
        }
        if (pid <= 0) {
            return err.InvalidBackendKeyData;
        }
        _.backend_pid = pid;
        _.backend_secret_key = secret_key;
        _.has_backend_key = true;
        return true;
    }

    /// 收到 `ReadyForQuery`（`'I'`/`'T'`/`'E'`），进入/保持 `Connected` 并更新事务状态
    fn onReadyForQuery(*_, indicator: u8): bool! {
        if (_.state != PgSessionState.SentStartupMessage && _.state != PgSessionState.Connected) {
            return err.InvalidStateTransition;
        }
        const tx = parsePgSessionTxStatus(indicator) ?? return err.InvalidTxIndicator;
        _.tx_status = tx;
        _.state = PgSessionState.Connected;
        return true;
    }

    /// 收到 `CommandComplete` 标签，解析受影响行数并递增已完成查询计数
    fn onCommandComplete(*_, tag: []const u8): PgSessionCmdResult! {
        if (_.state != PgSessionState.Connected) {
            return err.InvalidStateTransition;
        }
        const parsed = parsePgSessionCommandTag(tag);
        _.last_command = parsed;
        _.queries_completed += 1;
        return parsed;
    }

    fn onFatalError(*_) {
        _.state = PgSessionState.Failed;
        if (_.tls_status == PgSessionTlsStatus.Pending) {
            _.tls_status = PgSessionTlsStatus.Failed;
        }
    }

    fn isReadyForQuery(): bool {
        return _.state == PgSessionState.Connected;
    }

    fn inTransaction(): bool {
        return _.tx_status == PgSessionTxStatus.InTransaction || _.tx_status == PgSessionTxStatus.FailedTransaction;
    }

    fn needsRollback(): bool {
        return _.tx_status == PgSessionTxStatus.FailedTransaction;
    }
}
