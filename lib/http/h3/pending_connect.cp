// Source: Absorbed from /data/dev/bun/src/http/h3_client/PendingConnect.cp | Synced: 2026-11-15T16:56:37Z
// Pure-CP zero-allocation HTTP/3 DNS-Pending QUIC Connect Queue & Lifecycle Coordinator
// Aligned with Bun src/http/h3_client/PendingConnect.cp & docs/runtime/networking/dns.mdx:
// - Tracks in-flight DNS lookups preceding QUIC socket attachment with bounded fixed-capacity slots
// - Deduplicates concurrent lookups targeting the same (hostname[case-insensitive], port)
// - Bridges background DNS completion (`onDnsResolvedThreadsafe`) to event-loop drainage (`drainResolved`)
// - Enforces Bun's exact cancellation/abort/failure rules:
//     * session closed -> cancel QUIC pending connect silently (no duplicate failSession)
//     * session open with 0 pending streams -> cancel + failSession(Aborted)
//     * DNS resolution failed / no QUIC socket -> failSession(DnsResolutionFailed) + evict DNS cache entry
//     * DNS resolution succeeded -> attach QUIC socket to session.

/// 最大并发在途 HTTP/3 DNS-QUIC 连接槽位数
pub const H3_DNS_PENDING_MAX_CONNECTS: usize = 32;

/// 主机名最大长度（RFC 1035 DNS 域名上限 255 字节）
pub const H3_DNS_MAX_HOST_LEN: usize = 255;

/// Bun DNS 缓存默认 TTL（秒），对齐 `BUN_CONFIG_DNS_TIME_TO_LIVE_SECONDS` 默认值 30s
pub const H3_DNS_DEFAULT_TTL_SECS: u32 = 30;

/// DNS 缓存最大允许 TTL（24 小时）
pub const H3_DNS_MAX_TTL_SECS: u32 = 86400;

/// HTTP/3 DNS-Pending QUIC 连接调度器错误集
pub err H3PendingConnectError {
    InvalidSessionId,
    InvalidHost,
    HostTooLong,
    InvalidPort,
    DuplicateSession,
    TooManyPendingConnects,
    ConnectNotFound,
    AlreadyResolved,
    InvalidTtl,
    BufferTooSmall,
}

/// 在途 DNS-QUIC 连接状态
pub enum H3DnsConnectState: u8 {
    /// 等待后台 DNS 解析线程完成
    Resolving = 0,
    /// DNS 已完成，已进入已就绪队列等待事件循环 `drainResolved`
    QueuedForDrain = 1,
}

/// 事件循环 `drainResolved()` 对每条记录的最终裁决动作
pub enum H3DnsDrainAction: u8 {
    /// DNS 成功且会话存活、存在待发流：将 QUIC socket 绑定到会话
    AttachedSocket = 0,
    /// 会话在 DNS 返回前已关闭：取消底层 pending connect，不再触发 failSession
    CancelledSessionClosed = 1,
    /// 会话未关闭但所有待发流已清空：取消底层 pending connect 并触发 Aborted
    AbortedNoPendingStreams = 2,
    /// DNS 解析失败或未生成有效 QUIC socket：触发 DnsResolutionFailed 并驱逐 DNS 缓存
    FailedDnsResolution = 3,
}

/// 触发 `failSession` 时的失败原因分类（对齐 Bun `PendingConnect.onDNSResolved`）
pub enum H3SessionFailReason: u8 {
    None = 0,
    Aborted = 1,
    DnsResolutionFailed = 2,
}

/// `register` 返回的注册结果
pub struct H3RegisterResult {
    connect_id: u64 = 0,
    /// 是否命中同一 `(host, port)` 的在途 DNS 去重（对齐 `dns.getCacheStats().cacheHitsInflight`）
    deduped_inflight: bool = false,
}

/// 单条已排空记录的处理结果
pub struct H3DrainOutcome {
    connect_id: u64 = 0,
    session_id: u64 = 0,
    quic_socket_id: u64 = 0,
    action: H3DnsDrainAction = .AttachedSocket,
    fail_reason: H3SessionFailReason = .None,
    /// 是否应调用底层 `pc.cancel()`
    should_cancel_pc: bool = false,
    /// 是否应调用 `failSession` 排空会话内待发流并注销活跃会话
    should_fail_session: bool = false,
    /// 是否应从 DNS 缓存中驱逐该主机条目（对齐 `dns.mdx` 失败即驱逐策略）
    should_evict_dns_cache: bool = false,
    /// 是否归还了 `register` 时持有的会话引用计数（恒为 true）
    released_session_ref: bool = true,
}

/// DNS-Pending 连接统计快照（对齐 Bun `dns.getCacheStats()` 核心指标）
pub struct H3DnsPendingStats {
    total_registered: u64 = 0,
    inflight_dedup_hits: u64 = 0,
    attached_count: u64 = 0,
    cancelled_closed_count: u64 = 0,
    aborted_empty_count: u64 = 0,
    dns_failed_count: u64 = 0,
    active_count: usize = 0,
    queued_for_drain_count: usize = 0,
}

fn h3AsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + (32 as u8);
    return c;
}

fn h3HostEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (h3AsciiLower(a[i]) != h3AsciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn h3IsValidHost(host: []const u8): bool {
    if (host.len == 0) return false;
    let i: usize = 0;
    while (i < host.len) {
        const c = host[i];
        if (c <= 0x20 || c == 0x7F || c == '/' || c == '\\' || c == '@' || c == '?') {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 解析 `BUN_CONFIG_DNS_TIME_TO_LIVE_SECONDS` 风格 TTL 配置字符串（空串返回默认 30s）
pub fn parseH3DnsTtlSeconds(raw: []const u8): H3PendingConnectError!u32 {
    if (raw.len == 0) return H3_DNS_DEFAULT_TTL_SECS;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c < '0' || c > '9') return err.InvalidTtl;
        val = val * 10 + ((c - '0') as u64);
        if (val > (H3_DNS_MAX_TTL_SECS as u64)) return err.InvalidTtl;
        i += 1;
    }
    if (val == 0) return err.InvalidTtl;
    return val as u32;
}

/// 单条在途 HTTP/3 DNS-QUIC 连接记录（对齐 Bun `PendingConnect`）
pub struct H3PendingDnsEntry {
    connect_id: u64 = 0,
    session_id: u64 = 0,
    host_buf: [H3_DNS_MAX_HOST_LEN]u8 = [0; H3_DNS_MAX_HOST_LEN],
    host_len: usize = 0,
    port: u16 = 0,
    state: H3DnsConnectState = .Resolving,
    session_closed: bool = false,
    pending_streams: usize = 0,
    dns_resolved_ok: bool = false,
    resolved_socket_id: u64 = 0,

    fn hostname(): []const u8 {
        return _.host_buf[0.._.host_len];
    }

    fn matchesHostPort(host: []const u8, port: u16): bool {
        return _.port == port && h3HostEqlIgnoreCase(_.hostname(), host);
    }
}

/// 零堆分配 HTTP/3 DNS-Pending QUIC 连接协调队列
pub struct H3PendingConnectQueue {
    entries: [H3_DNS_PENDING_MAX_CONNECTS]H3PendingDnsEntry = [H3PendingDnsEntry{}; H3_DNS_PENDING_MAX_CONNECTS],
    count: usize = 0,
    /// 已完成 DNS 解析等待事件循环 `drainResolved()` 的 connect_id 队列（保持 FIFO/LIFO 有界存储）
    ready_ids: [H3_DNS_PENDING_MAX_CONNECTS]u64 = [0; H3_DNS_PENDING_MAX_CONNECTS],
    ready_count: usize = 0,
    next_connect_id: u64 = 1,
    dns_ttl_secs: u32 = H3_DNS_DEFAULT_TTL_SECS,
    total_registered: u64 = 0,
    inflight_dedup_hits: u64 = 0,
    attached_count: u64 = 0,
    cancelled_closed_count: u64 = 0,
    aborted_empty_count: u64 = 0,
    dns_failed_count: u64 = 0,

    static fn init(): H3PendingConnectQueue {
        return H3PendingConnectQueue{};
    }

    static fn initWithTtl(ttl_secs: u32): H3PendingConnectError!H3PendingConnectQueue {
        if (ttl_secs == 0 || ttl_secs > H3_DNS_MAX_TTL_SECS) return err.InvalidTtl;
        let q = H3PendingConnectQueue{};
        q.dns_ttl_secs = ttl_secs;
        return q;
    }

    fn activeCount(): usize {
        return _.count;
    }

    fn readyCount(): usize {
        return _.ready_count;
    }

    fn dnsTtlSeconds(): u32 {
        return _.dns_ttl_secs;
    }

    fn stats(): H3DnsPendingStats {
        return H3DnsPendingStats{
            total_registered: _.total_registered,
            inflight_dedup_hits: _.inflight_dedup_hits,
            attached_count: _.attached_count,
            cancelled_closed_count: _.cancelled_closed_count,
            aborted_empty_count: _.aborted_empty_count,
            dns_failed_count: _.dns_failed_count,
            active_count: _.count,
            queued_for_drain_count: _.ready_count,
        };
    }

    fn findIndexByConnectId(connect_id: u64): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].connect_id == connect_id) return i;
            i += 1;
        }
        return null;
    }

    fn findIndexBySessionId(session_id: u64): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].session_id == session_id) return i;
            i += 1;
        }
        return null;
    }

    fn hasInflightLookup(host: []const u8, port: u16): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].state == .Resolving && _.entries[i].matchesHostPort(host, port)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 注册一条新的 HTTP/3 DNS-pending QUIC 连接（对齐 Bun `PendingConnect.register`）
    fn register(*_, session_id: u64, host: []const u8, port: u16, pending_streams: usize): H3PendingConnectError!H3RegisterResult {
        if (session_id == 0) return err.InvalidSessionId;
        if (port == 0) return err.InvalidPort;
        if (host.len > H3_DNS_MAX_HOST_LEN) return err.HostTooLong;
        if (!h3IsValidHost(host)) return err.InvalidHost;
        if (_.findIndexBySessionId(session_id) != null) return err.DuplicateSession;
        if (_.count >= H3_DNS_PENDING_MAX_CONNECTS) return err.TooManyPendingConnects;

        const deduped = _.hasInflightLookup(host, port);
        const cid = _.next_connect_id;
        _.next_connect_id += 1;

        let entry = H3PendingDnsEntry{};
        entry.connect_id = cid;
        entry.session_id = session_id;
        entry.host_len = host.len;
        entry.port = port;
        entry.state = .Resolving;
        entry.session_closed = false;
        entry.pending_streams = pending_streams;
        let i: usize = 0;
        while (i < host.len) {
            entry.host_buf[i] = host[i];
            i += 1;
        }

        _.entries[_.count] = entry;
        _.count += 1;
        _.total_registered += 1;
        if (deduped) {
            _.inflight_dedup_hits += 1;
        }

        return H3RegisterResult{
            connect_id: cid,
            deduped_inflight: deduped,
        };
    }

    /// 在 DNS 解析期间标记对应会话已被关闭
    fn markSessionClosed(*_, session_id: u64): bool {
        if (idx := _.findIndexBySessionId(session_id)) {
            _.entries[idx].session_closed = true;
            return true;
        }
        return false;
    }

    /// 在 DNS 解析期间更新会话当前挂起的待发流数量
    fn updatePendingStreams(*_, session_id: u64, pending_streams: usize): bool {
        if (idx := _.findIndexBySessionId(session_id)) {
            _.entries[idx].pending_streams = pending_streams;
            return true;
        }
        return false;
    }

    /// 模拟后台 DNS 线程回调 `onDNSResolvedThreadsafe`：将结果入队，返回是否需要唤醒事件循环
    fn onDnsResolvedThreadsafe(*_, connect_id: u64, resolved_ok: bool, quic_socket_id: u64): H3PendingConnectError!bool {
        const idx = _.findIndexByConnectId(connect_id) ?? return err.ConnectNotFound;
        if (_.entries[idx].state == .QueuedForDrain) return err.AlreadyResolved;

        _.entries[idx].state = .QueuedForDrain;
        _.entries[idx].dns_resolved_ok = resolved_ok;
        _.entries[idx].resolved_socket_id = if (resolved_ok) quic_socket_id else 0;

        const was_empty = _.ready_count == 0;
        _.ready_ids[_.ready_count] = connect_id;
        _.ready_count += 1;
        return was_empty;
    }

    fn removeEntrySwap(*_, idx: usize) {
        const last = _.count - 1;
        if (idx != last) {
            _.entries[idx] = _.entries[last];
        }
        _.entries[last] = H3PendingDnsEntry{};
        _.count -= 1;
    }

    /// 取消一条尚未进入排空队列的在途连接并立即释放槽位
    fn cancelBeforeResolve(*_, connect_id: u64): H3PendingConnectError!bool {
        const idx = _.findIndexByConnectId(connect_id) ?? return err.ConnectNotFound;
        if (_.entries[idx].state == .QueuedForDrain) return err.AlreadyResolved;
        _.removeEntrySwap(idx);
        return true;
    }

    /// 事件循环批量排空已完成的 DNS 解析记录（对齐 Bun `PendingConnect.drainResolved` + `onDNSResolved`）
    fn drainResolved(*_, out: []H3DrainOutcome): H3PendingConnectError!usize {
        if (_.ready_count == 0) return 0;
        if (out.len < _.ready_count) return err.BufferTooSmall;

        let drained: usize = 0;
        let r: usize = 0;
        while (r < _.ready_count) {
            const cid = _.ready_ids[r];
            _.ready_ids[r] = 0;
            r += 1;

            if (idx := _.findIndexByConnectId(cid)) {
                const entry = _.entries[idx];
                let outcome = H3DrainOutcome{
                    connect_id: entry.connect_id,
                    session_id: entry.session_id,
                    quic_socket_id: 0,
                    action: .AttachedSocket,
                    fail_reason: .None,
                    should_cancel_pc: false,
                    should_fail_session: false,
                    should_evict_dns_cache: false,
                    released_session_ref: true,
                };

                if (entry.session_closed) {
                    outcome.action = .CancelledSessionClosed;
                    outcome.should_cancel_pc = true;
                    outcome.should_fail_session = false;
                    _.cancelled_closed_count += 1;
                } else if (entry.pending_streams == 0) {
                    outcome.action = .AbortedNoPendingStreams;
                    outcome.fail_reason = .Aborted;
                    outcome.should_cancel_pc = true;
                    outcome.should_fail_session = true;
                    _.aborted_empty_count += 1;
                } else if (entry.dns_resolved_ok && entry.resolved_socket_id != 0) {
                    outcome.action = .AttachedSocket;
                    outcome.quic_socket_id = entry.resolved_socket_id;
                    _.attached_count += 1;
                } else {
                    outcome.action = .FailedDnsResolution;
                    outcome.fail_reason = .DnsResolutionFailed;
                    outcome.should_fail_session = true;
                    outcome.should_evict_dns_cache = true;
                    _.dns_failed_count += 1;
                }

                out[drained] = outcome;
                drained += 1;
                _.removeEntrySwap(idx);
            }
        }
        _.ready_count = 0;
        return drained;
    }

    /// 便捷查询当前活跃条目快照（按 connect_id）
    fn getEntry(connect_id: u64): H3PendingDnsEntry? {
        if (idx := _.findIndexByConnectId(connect_id)) {
            return _.entries[idx];
        }
        return null;
    }
}
