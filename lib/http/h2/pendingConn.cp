// Source: Absorbed from /data/dev/bun/src/http/h2_client/PendingConnect.cp | Synced: 2026-11-23T01:58:15Z
// Ported from Bun: /data/dev/bun/src/http/h2_client/PendingConnect.cp & /data/dev/bun/docs/runtime/http/tls.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation HTTP/2 In-Flight TLS Connect Coalescer (Singleflight PendingConnect Table):
// - Coalesces concurrent HTTP/2 requests targeting the same (hostname[case-insensitive], port, ssl_config_id)
// - Enforces strict TLS SSLConfig isolation so different CA/SNI/cert policies never share a handshake
// - Supports waiter cancellation with automatic empty-entry reclamation
// - Resolves/fails in-flight connects with O(1) swap-remove unregistration and batch waiter drainage.

/// 最大并发在途 HTTP/2 TLS 握手槽位数
pub const H2_PENDING_MAX_CONNECTS: usize = 32;

/// 单个在途 HTTP/2 握手允许合并的最大等待请求数
pub const H2_PENDING_MAX_WAITERS: usize = 32;

/// 主机名最大长度（RFC 1035 DNS 域名上限 255 字节）
pub const H2_PENDING_MAX_HOST_LEN: usize = 255;

/// HTTP/2 在途连接合并器错误集
pub err H2PendingConnectError {
    InvalidHost,
    InvalidPort,
    HostTooLong,
    TooManyPendingConnects,
    TooManyWaiters,
    DuplicateWaiter,
    PendingConnectNotFound,
    BufferTooSmall,
}

/// 请求加入在途连接的结果类型
pub enum H2ConnectJoinKind: u8 {
    /// 首个请求：发起新的 TLS + ALPN HTTP/2 握手
    InitiatedNew = 0,
    /// 后续并发请求：成功合并至已有在途握手队列
    CoalescedWaiter = 1,
}

/// `joinOrStart` 返回的合并调度结果
pub struct H2ConnectJoinResult {
    kind: H2ConnectJoinKind = .InitiatedNew,
    connect_id: u64 = 0,
    waiter_count: usize = 0,
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + (32 as u8);
    return c;
}

fn hostEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn isValidHostStr(host: []const u8): bool {
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

/// 单条在途 HTTP/2 TLS 握手记录（对齐 Bun `PendingConnect`）
pub struct H2PendingConnectEntry {
    connect_id: u64 = 0,
    host_buf: [H2_PENDING_MAX_HOST_LEN]u8 = [0; H2_PENDING_MAX_HOST_LEN],
    host_len: usize = 0,
    port: u16 = 0,
    /// TLS 配置标识（0 表示默认系统 CA 配置；非 0 表示自定义 SSLConfig 身份，严格隔离握手复用）
    ssl_config_id: u64 = 0,
    waiters: [H2_PENDING_MAX_WAITERS]u64 = [0; H2_PENDING_MAX_WAITERS],
    waiter_count: usize = 0,

    fn hostname(): []const u8 {
        return _.host_buf[0.._.host_len];
    }

    fn waitersSlice(): []const u64 {
        return _.waiters[0.._.waiter_count];
    }

    /// 精确匹配 `(hostname[大小写不敏感], port, ssl_config_id)` 三元组（对齐 Bun `PendingConnect.matches`）
    fn matches(host: []const u8, port: u16, ssl_config_id: u64): bool {
        return _.port == port &&
            _.ssl_config_id == ssl_config_id &&
            hostEqlIgnoreCase(_.hostname(), host);
    }

    fn hasWaiter(waiter_id: u64): bool {
        let i: usize = 0;
        while (i < _.waiter_count) {
            if (_.waiters[i] == waiter_id) return true;
            i += 1;
        }
        return false;
    }

    fn addWaiter(*_, waiter_id: u64): ! {
        if (_.hasWaiter(waiter_id)) return err.DuplicateWaiter;
        if (_.waiter_count >= H2_PENDING_MAX_WAITERS) return err.TooManyWaiters;
        _.waiters[_.waiter_count] = waiter_id;
        _.waiter_count += 1;
    }

    fn removeWaiter(*_, waiter_id: u64): bool {
        let i: usize = 0;
        while (i < _.waiter_count) {
            if (_.waiters[i] == waiter_id) {
                // 保持其余等待者 FIFO 唤醒顺序
                let j: usize = i;
                while (j + 1 < _.waiter_count) {
                    _.waiters[j] = _.waiters[j + 1];
                    j += 1;
                }
                _.waiter_count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }
}

/// 纯 CP 零堆分配 HTTP/2 在途连接合并表
pub struct H2PendingConnectTable {
    entries: [H2_PENDING_MAX_CONNECTS]H2PendingConnectEntry = [H2PendingConnectEntry{}; H2_PENDING_MAX_CONNECTS],
    count: usize = 0,
    next_connect_id: u64 = 1,

    static fn init(): H2PendingConnectTable {
        return H2PendingConnectTable{
            count: 0,
            next_connect_id: 1,
        };
    }

    fn activeCount(): usize {
        return _.count;
    }

    fn findIndex(host: []const u8, port: u16, ssl_config_id: u64): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].matches(host, port, ssl_config_id)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn findById(connect_id: u64): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].connect_id == connect_id) return i;
            i += 1;
        }
        return null;
    }

    fn swapRemoveAt(*_, idx: usize) {
        if (idx >= _.count) return;
        const last = _.count - 1;
        if (idx != last) {
            _.entries[idx] = _.entries[last];
        }
        _.count -= 1;
    }

    /// 合并至已有在途握手或发起新的在途握手
    fn joinOrStart(
        *_,
        host: []const u8,
        port: u16,
        ssl_config_id: u64,
        waiter_id: u64,
    ): H2ConnectJoinResult! {
        if (!isValidHostStr(host)) return err.InvalidHost;
        if (host.len > H2_PENDING_MAX_HOST_LEN) return err.HostTooLong;
        if (port == 0) return err.InvalidPort;

        if (idx := _.findIndex(host, port, ssl_config_id)) {
            _.entries[idx].addWaiter(waiter_id)?;
            return H2ConnectJoinResult{
                kind: .CoalescedWaiter,
                connect_id: _.entries[idx].connect_id,
                waiter_count: _.entries[idx].waiter_count,
            };
        }

        if (_.count >= H2_PENDING_MAX_CONNECTS) {
            return err.TooManyPendingConnects;
        }

        let entry = H2PendingConnectEntry{
            connect_id: _.next_connect_id,
            host_len: host.len,
            port: port,
            ssl_config_id: ssl_config_id,
            waiter_count: 1,
        };
        let i: usize = 0;
        while (i < host.len) {
            entry.host_buf[i] = asciiLower(host[i]);
            i += 1;
        }
        entry.waiters[0] = waiter_id;

        const assigned_id = _.next_connect_id;
        _.next_connect_id += 1;
        _.entries[_.count] = entry;
        _.count += 1;

        return H2ConnectJoinResult{
            kind: .InitiatedNew,
            connect_id: assigned_id,
            waiter_count: 1,
        };
    }

    /// 取消指定等待请求；若该在途连接不再有任何等待者，自动 `O(1)` 回收槽位
    fn cancelWaiter(*_, waiter_id: u64): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].removeWaiter(waiter_id)) {
                if (_.entries[i].waiter_count == 0) {
                    _.swapRemoveAt(i);
                }
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 握手完成（成功或失败）时一次性导出全部等待者并通过 `O(1)` swap-remove 注销在途连接
    fn resolveAndDrain(*_, connect_id: u64, out_waiters: []u64): usize! {
        const idx = _.findById(connect_id) orelse return err.PendingConnectNotFound;
        const n = _.entries[idx].waiter_count;
        if (out_waiters.len < n) {
            return err.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < n) {
            out_waiters[i] = _.entries[idx].waiters[i];
            i += 1;
        }
        _.swapRemoveAt(idx);
        return n;
    }
}
