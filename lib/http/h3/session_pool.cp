// Source: Absorbed from /data/dev/bun/src/http/h3_client/ClientContext.cp | Synced: 2026-11-22T06:35:54Z
// Pure-CP zero-allocation HTTP/3 QUIC client session pool:
// connection coalescing by (hostname, port, reject_unauthorized), concurrent stream headroom control,
// O(1) swap-remove session unregistration, and http_id-based abort / body-stream routing.

/// 会话池支持的最大并发 HTTP/3 会话数
pub const H3_POOL_MAX_SESSIONS: usize = 32;

/// 单个 HTTP/3 会话默认允许的最大并发在途请求数
pub const H3_SESSION_MAX_STREAMS: usize = 32;

/// HTTP/3 会话池错误集
pub err H3PoolError {
    PoolFull,
    SessionNotFound,
    StreamQueueFull,
    InvalidHost,
    InvalidHttpId,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlHostIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 连接调度结果类型
pub enum H3ConnectOutcomeKind: u8 {
    ReusedExisting = 0,
    OpenedNew = 1,
}

/// `H3SessionPool.connect` 返回的连接复用/新建决策
pub struct H3ConnectOutcome {
    kind: H3ConnectOutcomeKind = .OpenedNew,
    session_id: u32 = 0,
    stream_slot: usize = 0,
}

/// 挂载在 HTTP/3 会话上的单个在途请求条目
pub struct H3PendingRequest {
    http_id: u32 = 0,
    body_ended: bool = false,
    aborted: bool = false,
}

/// 单个池化 HTTP/3 客户端会话描述符
pub struct H3PooledSession {
    session_id: u32 = 0,
    hostname: []const u8 = "",
    port: u16 = 443,
    reject_unauthorized: bool = true,
    max_concurrent_streams: usize = H3_SESSION_MAX_STREAMS,
    closed: bool = false,
    pending: [H3_SESSION_MAX_STREAMS]H3PendingRequest = [H3PendingRequest{}; H3_SESSION_MAX_STREAMS],
    pending_len: usize = 0,

    /// 校验目标三元组 `(hostname, port, reject_unauthorized)` 是否可复用当前会话
    ///
    /// 主机名按 DNS ASCII 大小写无关比较；TLS 证书校验策略 `reject_unauthorized` 必须严格一致，
    /// 防止非校验连接与严格校验连接错误复用。
    fn matches(hostname: []const u8, port: u16, reject_unauthorized: bool): bool {
        return !_.closed &&
            _.port == port &&
            _.reject_unauthorized == reject_unauthorized &&
            eqlHostIgnoreCase(_.hostname, hostname);
    }

    /// 判断当前会话是否仍有并发流余量
    fn hasHeadroom(): bool {
        const cap = if (_.max_concurrent_streams < H3_SESSION_MAX_STREAMS)
            _.max_concurrent_streams
        else
            H3_SESSION_MAX_STREAMS;
        return !_.closed && _.pending_len < cap;
    }

    fn activeCount(): usize {
        return _.pending_len;
    }

    fn findRequest(http_id: u32): usize? {
        let i: usize = 0;
        while (i < _.pending_len) {
            if (_.pending[i].http_id == http_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn enqueue(*_, http_id: u32): usize! {
        if (!_.hasHeadroom()) {
            return err.StreamQueueFull;
        }
        const slot = _.pending_len;
        _.pending[slot] = H3PendingRequest{
            http_id: http_id,
            body_ended: false,
            aborted: false,
        };
        _.pending_len += 1;
        return slot;
    }

    /// 按 `http_id` 摘除在途请求（保持剩余请求顺序），命中返回 `true`
    fn detachByHttpId(*_, http_id: u32): bool {
        const idx = _.findRequest(http_id) ?? return false;
        let i = idx;
        while (i + 1 < _.pending_len) {
            _.pending[i] = _.pending[i + 1];
            i += 1;
        }
        _.pending_len -= 1;
        _.pending[_.pending_len] = H3PendingRequest{};
        return true;
    }

    fn markBodyEnded(*_, http_id: u32, ended: bool): bool {
        const idx = _.findRequest(http_id) ?? return false;
        _.pending[idx].body_ended = ended;
        return true;
    }
}

/// 零堆分配 HTTP/3 客户端会话池（对齐 Bun `http/h3_client/ClientContext.cp`）
pub struct H3SessionPool {
    sessions: [H3_POOL_MAX_SESSIONS]H3PooledSession = [H3PooledSession{}; H3_POOL_MAX_SESSIONS],
    len: usize = 0,
    next_session_id: u32 = 1,
    live_sessions: usize = 0,

    static fn init(): H3SessionPool {
        return H3SessionPool{};
    }

    fn count(): usize {
        return _.len;
    }

    fn findSessionIndex(session_id: u32): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.sessions[i].session_id == session_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 为指定请求路由至已有可复用会话，或在无可用余量时新建 QUIC 会话槽位
    fn connect(
        *_,
        hostname: []const u8,
        port: u16,
        reject_unauthorized: bool,
        http_id: u32,
        max_streams: usize,
    ): H3ConnectOutcome! {
        if (hostname.len == 0) {
            return err.InvalidHost;
        }
        if (http_id == 0) {
            return err.InvalidHttpId;
        }

        // 1. 优先复用匹配且仍有并发流余量的会话
        let i: usize = 0;
        while (i < _.len) {
            if (_.sessions[i].matches(hostname, port, reject_unauthorized) && _.sessions[i].hasHeadroom()) {
                const slot = _.sessions[i].enqueue(http_id)?;
                return H3ConnectOutcome{
                    kind: .ReusedExisting,
                    session_id: _.sessions[i].session_id,
                    stream_slot: slot,
                };
            }
            i += 1;
        }

        // 2. 新建会话槽位
        if (_.len >= H3_POOL_MAX_SESSIONS) {
            return err.PoolFull;
        }
        const effective_max = if (max_streams == 0 || max_streams > H3_SESSION_MAX_STREAMS)
            H3_SESSION_MAX_STREAMS
        else
            max_streams;

        const sid = _.next_session_id;
        _.next_session_id += 1;
        let sess = H3PooledSession{
            session_id: sid,
            hostname: hostname,
            port: port,
            reject_unauthorized: reject_unauthorized,
            max_concurrent_streams: effective_max,
            closed: false,
        };
        const slot = sess.enqueue(http_id)?;
        _.sessions[_.len] = sess;
        _.len += 1;
        _.live_sessions += 1;
        return H3ConnectOutcome{
            kind: .OpenedNew,
            session_id: sid,
            stream_slot: slot,
        };
    }

    /// O(1) swap-remove 注销会话（对齐 Bun `ClientContext.unregister`）
    fn unregister(*_, session_id: u32): bool {
        const idx = _.findSessionIndex(session_id) ?? return false;
        const last = _.len - 1;
        if (idx < last) {
            _.sessions[idx] = _.sessions[last];
        }
        _.sessions[last] = H3PooledSession{};
        _.len -= 1;
        if (_.live_sessions > 0) {
            _.live_sessions -= 1;
        }
        return true;
    }

    /// 按 `http_id` 路由中止在途请求（对齐 Bun `ClientContext.abortByHttpId`）
    fn abortByHttpId(*_, http_id: u32): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.sessions[i].detachByHttpId(http_id)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 按 `http_id` 路由请求体流状态变更（对齐 Bun `ClientContext.streamBodyByHttpId`）
    fn streamBodyByHttpId(*_, http_id: u32, ended: bool): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (_.sessions[i].markBodyEnded(http_id, ended)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 标记指定会话失败、清空并统计受影响的在途请求数，并从池中注销（对齐 `PendingConnect.failSession`）
    fn failSession(*_, session_id: u32): usize {
        const idx = _.findSessionIndex(session_id) ?? return 0;
        const failed_count = _.sessions[idx].pending_len;
        _.sessions[idx].closed = true;
        _.sessions[idx].pending_len = 0;
        * = _.unregister(session_id);
        return failed_count;
    }
}
