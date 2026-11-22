// Source: Absorbed from /data/dev/bun/src/runtime/dns_jsc/dns.cp | Synced: 2026-11-22T06:50:16Z
// Pure-CP zero-allocation Bun-compatible DNS prefetch & in-flight deduplicating cache:
// 255-entry bounded LRU/TTL store (default 30s), in-flight lookup coalescing (`cacheHitsInflight`),
// connection-error eviction, `dns.prefetch`, `dns.getCacheStats()`, and TTL env parser.

/// 默认 DNS 缓存 TTL：30 秒（严格对齐 Bun `BUN_CONFIG_DNS_TIME_TO_LIVE_SECONDS` 默认值）
pub const DNS_PREFETCH_DEFAULT_TTL_MS: u64 = 30000;

/// 安全最大 TTL 上限：24 小时
pub const DNS_PREFETCH_MAX_TTL_MS: u64 = 86400000;

/// 默认最大缓存条目数（严格对齐 Bun 255 条缓存容量上限）
pub const DNS_PREFETCH_MAX_ENTRIES: usize = 255;

/// DNS 预取缓存错误集
pub err DnsPrefetchError {
    EmptyHost,
    HostTooLong,
    InvalidTtlValue,
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

/// 解析 `BUN_CONFIG_DNS_TIME_TO_LIVE_SECONDS` 环境变量值（秒 -> 毫秒）
///
/// 返回 `0` 表示禁用缓存；超过 24 小时安全钳位至 `DNS_PREFETCH_MAX_TTL_MS`。
pub fn parseDnsTtlEnvSeconds(raw: []const u8): u64! {
    if (raw.len == 0) {
        return err.InvalidTtlValue;
    }
    let secs: u64 = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c < '0' || c > '9') {
            return err.InvalidTtlValue;
        }
        if (secs <= 1000000) {
            secs = secs * 10 + ((c - '0') as u64);
        }
        i += 1;
    }
    const ms = secs * 1000;
    if (ms > DNS_PREFETCH_MAX_TTL_MS) {
        return DNS_PREFETCH_MAX_TTL_MS;
    }
    return ms;
}

/// 对齐 Bun `dns.getCacheStats()` 的六维缓存统计快照
pub struct DnsCacheStats {
    cache_hits_completed: u64 = 0,
    cache_hits_inflight: u64 = 0,
    cache_misses: u64 = 0,
    size: usize = 0,
    errors: u64 = 0,
    total_count: u64 = 0,
}

/// 缓存槽位状态
pub enum DnsLookupState: u8 {
    Empty = 0,
    Inflight = 1,
    Completed = 2,
}

/// 查询命中/去重/未命中决策类型
pub enum DnsLookupOutcomeKind: u8 {
    HitCompleted = 0,
    HitInflight = 1,
    MissStarted = 2,
}

/// `DnsPrefetchCache.lookup` 返回的查询结果
pub struct DnsLookupOutcome {
    kind: DnsLookupOutcomeKind = .MissStarted,
    slot: usize = 0,
    resolved_ip: []const u8 = "",
    waiter_count: u32 = 0,
}

/// 单个 DNS 预取/在途去重缓存条目
pub struct DnsPrefetchEntry {
    state: DnsLookupState = .Empty,
    hostname: []const u8 = "",
    port: u16 = 0,
    resolved_ip: []const u8 = "",
    created_at_ms: u64 = 0,
    expires_at_ms: u64 = 0,
    last_used_ms: u64 = 0,
    inflight_waiters: u32 = 0,
}

/// 零堆分配 Bun 风格 DNS 预取、在途去重与失败驱逐缓存
pub struct DnsPrefetchCache {
    entries: [DNS_PREFETCH_MAX_ENTRIES]DnsPrefetchEntry = [DnsPrefetchEntry{}; DNS_PREFETCH_MAX_ENTRIES],
    ttl_ms: u64 = DNS_PREFETCH_DEFAULT_TTL_MS,
    size: usize = 0,
    cache_hits_completed: u64 = 0,
    cache_hits_inflight: u64 = 0,
    cache_misses: u64 = 0,
    errors: u64 = 0,
    total_count: u64 = 0,

    static fn init(ttl_ms: u64): DnsPrefetchCache {
        const clamped = if (ttl_ms > DNS_PREFETCH_MAX_TTL_MS) DNS_PREFETCH_MAX_TTL_MS else ttl_ms;
        return DnsPrefetchCache{ ttl_ms: clamped };
    }

    fn stats(): DnsCacheStats {
        return DnsCacheStats{
            cache_hits_completed: _.cache_hits_completed,
            cache_hits_inflight: _.cache_hits_inflight,
            cache_misses: _.cache_misses,
            size: _.size,
            errors: _.errors,
            total_count: _.total_count,
        };
    }

    fn findSlot(hostname: []const u8, port: u16): usize? {
        let i: usize = 0;
        while (i < DNS_PREFETCH_MAX_ENTRIES) {
            if (_.entries[i].state != .Empty &&
                _.entries[i].port == port &&
                eqlHostIgnoreCase(_.entries[i].hostname, hostname))
            {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 寻找可用槽位（优先空槽 -> 已过期槽 -> 最久未使用的已完成槽 -> 兜底 0 号槽）
    fn allocateSlot(*_, now_ms: u64): usize {
        let expired_slot: usize? = null;
        let lru_slot: usize? = null;
        let lru_time: u64 = 0xFFFFFFFFFFFFFFFF;

        let i: usize = 0;
        while (i < DNS_PREFETCH_MAX_ENTRIES) {
            const e = _.entries[i];
            if (e.state == .Empty) {
                return i;
            }
            if (e.state == .Completed) {
                if (now_ms >= e.expires_at_ms && expired_slot == null) {
                    expired_slot = i;
                }
                if (e.last_used_ms < lru_time) {
                    lru_time = e.last_used_ms;
                    lru_slot = i;
                }
            }
            i += 1;
        }

        if (exp := expired_slot) {
            _.entries[exp] = DnsPrefetchEntry{};
            if (_.size > 0) {
                _.size -= 1;
            }
            return exp;
        }
        if (lru := lru_slot) {
            _.entries[lru] = DnsPrefetchEntry{};
            if (_.size > 0) {
                _.size -= 1;
            }
            return lru;
        }
        // 全为在途条目时的安全兜底：复用槽位 0
        _.entries[0] = DnsPrefetchEntry{};
        if (_.size > 0) {
            _.size -= 1;
        }
        return 0;
    }

    /// 查询或登记在途 DNS 解析（支持已完成命中、在途去重与过期自动刷新）
    fn lookup(*_, hostname: []const u8, port: u16, now_ms: u64): DnsLookupOutcome! {
        if (hostname.len == 0) {
            return err.EmptyHost;
        }
        if (hostname.len > 253) {
            return err.HostTooLong;
        }
        _.total_count += 1;

        if (idx := _.findSlot(hostname, port)) {
            const e = _.entries[idx];
            if (e.state == .Completed && now_ms < e.expires_at_ms) {
                _.entries[idx].last_used_ms = now_ms;
                _.cache_hits_completed += 1;
                return DnsLookupOutcome{
                    kind: .HitCompleted,
                    slot: idx,
                    resolved_ip: e.resolved_ip,
                    waiter_count: 0,
                };
            }
            if (e.state == .Inflight) {
                _.entries[idx].inflight_waiters += 1;
                _.entries[idx].last_used_ms = now_ms;
                _.cache_hits_inflight += 1;
                return DnsLookupOutcome{
                    kind: .HitInflight,
                    slot: idx,
                    resolved_ip: "",
                    waiter_count: _.entries[idx].inflight_waiters,
                };
            }
            // 已过期：就地转为新一轮在途解析
            _.cache_misses += 1;
            _.entries[idx] = DnsPrefetchEntry{
                state: .Inflight,
                hostname: hostname,
                port: port,
                resolved_ip: "",
                created_at_ms: now_ms,
                expires_at_ms: 0,
                last_used_ms: now_ms,
                inflight_waiters: 1,
            };
            return DnsLookupOutcome{
                kind: .MissStarted,
                slot: idx,
                resolved_ip: "",
                waiter_count: 1,
            };
        }

        _.cache_misses += 1;
        const slot = _.allocateSlot(now_ms);
        _.entries[slot] = DnsPrefetchEntry{
            state: .Inflight,
            hostname: hostname,
            port: port,
            resolved_ip: "",
            created_at_ms: now_ms,
            expires_at_ms: 0,
            last_used_ms: now_ms,
            inflight_waiters: 1,
        };
        _.size += 1;
        return DnsLookupOutcome{
            kind: .MissStarted,
            slot: slot,
            resolved_ip: "",
            waiter_count: 1,
        };
    }

    /// 预取域名解析（对齐 Bun `dns.prefetch(hostname, port)`）：
    /// 若尚未缓存且未在途则登记为在途并返回 `true`（提示调用方发起后台解析）；否则返回 `false`。
    fn prefetch(*_, hostname: []const u8, port: u16, now_ms: u64): bool! {
        if (hostname.len == 0) {
            return err.EmptyHost;
        }
        if (hostname.len > 253) {
            return err.HostTooLong;
        }
        if (idx := _.findSlot(hostname, port)) {
            const e = _.entries[idx];
            if (e.state == .Inflight) {
                return false;
            }
            if (e.state == .Completed && now_ms < e.expires_at_ms) {
                return false;
            }
            // 已过期条目：重启预取
            _.entries[idx] = DnsPrefetchEntry{
                state: .Inflight,
                hostname: hostname,
                port: port,
                resolved_ip: "",
                created_at_ms: now_ms,
                expires_at_ms: 0,
                last_used_ms: now_ms,
                inflight_waiters: 0,
            };
            return true;
        }

        const slot = _.allocateSlot(now_ms);
        _.entries[slot] = DnsPrefetchEntry{
            state: .Inflight,
            hostname: hostname,
            port: port,
            resolved_ip: "",
            created_at_ms: now_ms,
            expires_at_ms: 0,
            last_used_ms: now_ms,
            inflight_waiters: 0,
        };
        _.size += 1;
        return true;
    }

    /// 完成解析并写入 TTL 缓存（若 `ttl_ms == 0` 则完成即移除、不驻留缓存）
    fn complete(*_, hostname: []const u8, port: u16, resolved_ip: []const u8, now_ms: u64): bool {
        const idx = _.findSlot(hostname, port) ?? return false;
        if (_.ttl_ms == 0) {
            _.entries[idx] = DnsPrefetchEntry{};
            if (_.size > 0) {
                _.size -= 1;
            }
            return true;
        }
        _.entries[idx].state = .Completed;
        _.entries[idx].resolved_ip = resolved_ip;
        _.entries[idx].expires_at_ms = now_ms + _.ttl_ms;
        _.entries[idx].last_used_ms = now_ms;
        _.entries[idx].inflight_waiters = 0;
        return true;
    }

    /// 标记解析失败并移除槽位
    fn failLookup(*_, hostname: []const u8, port: u16): bool {
        _.errors += 1;
        const idx = _.findSlot(hostname, port) ?? return false;
        _.entries[idx] = DnsPrefetchEntry{};
        if (_.size > 0) {
            _.size -= 1;
        }
        return true;
    }

    /// 连接失败回调：立即驱逐对应主机的缓存条目并递增错误计数（对齐 Bun 失败驱逐契约）
    fn onConnectionError(*_, hostname: []const u8, port: u16): bool {
        return _.failLookup(hostname, port);
    }

    /// 主动清理所有已过期的已完成条目，返回清理数量
    fn pruneExpired(*_, now_ms: u64): usize {
        let evicted: usize = 0;
        let i: usize = 0;
        while (i < DNS_PREFETCH_MAX_ENTRIES) {
            if (_.entries[i].state == .Completed && now_ms >= _.entries[i].expires_at_ms) {
                _.entries[i] = DnsPrefetchEntry{};
                if (_.size > 0) {
                    _.size -= 1;
                }
                evicted += 1;
            }
            i += 1;
        }
        return evicted;
    }
}
