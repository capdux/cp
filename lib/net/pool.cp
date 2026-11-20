// Source: cp/lib/net/pool.cp (Production TCP Connection Pool)
// lib/net/pool.cp
// Production TCP Connection Pool with per-host limits, total idle cap, TTL & lifetime eviction

pub type PoolCloseFn = fn(fd: i32);

pub struct ConnPoolConfig {
    max_idle_per_host: usize = 8;
    max_total_idle: usize = 64;
    idle_timeout_ms: i64 = 30000;
    max_lifetime_ms: i64 = 300000;
    close_on_evict: bool = true;
}

pub struct PoolEntry {
    host_buf: [256]u8 = [0; 256];
    host_len: usize = 0;
    port: u16 = 0;
    fd: i32 = -1;
    created_at_ms: i64 = 0;
    last_used_at_ms: i64 = 0;

    fn host(): []const u8 {
        return _.host_buf[0.._.host_len];
    }

    fn matchesHostPort(target_host: []const u8, target_port: u16): bool {
        if (_.port != target_port || _.host_len != target_host.len) {
            return false;
        }
        return std.mem.eql(u8, _.host_buf[0.._.host_len], target_host);
    }

    fn isExpired(now_ms: i64, cfg: ConnPoolConfig): bool {
        if (cfg.idle_timeout_ms > 0 && (now_ms - _.last_used_at_ms) >= cfg.idle_timeout_ms) {
            return true;
        }
        if (cfg.max_lifetime_ms > 0 && (now_ms - _.created_at_ms) >= cfg.max_lifetime_ms) {
            return true;
        }
        return false;
    }
}

pub struct PoolStats {
    hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    idle_count: usize = 0;
}

pub struct ConnectionPool {
    config: ConnPoolConfig = ConnPoolConfig {};
    entries: []PoolEntry = [];
    len: usize = 0;
    cap: usize = 0;
    hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    on_close: PoolCloseFn? = null;

    static fn init(config: ConnPoolConfig): ConnectionPool {
        return ConnectionPool {
            config: config,
            entries: [],
            len: 0,
            cap: 0,
            hits: 0,
            misses: 0,
            evictions: 0,
            on_close: null,
        };
    }

    fn setCloseCallback(*_, cb: PoolCloseFn?) {
        _.on_close = cb;
    }

    priv fn closeFd(*_, fd: i32) {
        if (fd < 0) { return; }
        if (cb := _.on_close) {
            cb(fd);
            return;
        }
        if (_.config.close_on_evict) {
            * = std.os.linux.close(fd);
        }
    }

    priv fn ensureCapacity(*_, needed: usize) {
        if (needed <= _.cap) { return; }
        let new_cap: usize = if (_.cap == 0) 16 else _.cap * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        let next = std.heap.page_allocator.alloc(PoolEntry, new_cap) catch unreachable;
        let i: usize = 0;
        while (i < _.len) {
            next[i] = _.entries[i];
            i += 1;
        }
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.entries);
        }
        _.entries = next;
        _.cap = new_cap;
    }

    priv fn removeAt(*_, idx: usize) {
        if (idx >= _.len) { return; }
        let i = idx;
        while (i + 1 < _.len) {
            _.entries[i] = _.entries[i + 1];
            i += 1;
        }
        _.len -= 1;
    }

    /// Acquires a warm idle FD for `host:port`, automatically evicting expired entries encountered.
    fn acquire(*_, host: []const u8, port: u16, now_ms: i64): i32? {
        if (host.len == 0 || host.len > 256) {
            _.misses += 1;
            return null;
        }
        // Scan newest-to-oldest (LIFO) for warm keepalive locality
        let i = _.len;
        while (i > 0) {
            i -= 1;
            if (_.entries[i].matchesHostPort(host, port)) {
                if (_.entries[i].isExpired(now_ms, _.config)) {
                    const expired_fd = _.entries[i].fd;
                    _.removeAt(i);
                    _.closeFd(expired_fd);
                    _.evictions += 1;
                    continue;
                }
                const fd = _.entries[i].fd;
                _.removeAt(i);
                _.hits += 1;
                return fd;
            }
        }
        _.misses += 1;
        return null;
    }

    /// Returns an FD back to the pool with explicit creation timestamp for max_lifetime tracking.
    fn releaseWithCreated(*_, host: []const u8, port: u16, fd: i32, created_at_ms: i64, now_ms: i64): bool {
        if (fd < 0 || host.len == 0 || host.len > 256) {
            _.closeFd(fd);
            return false;
        }
        if (_.config.max_idle_per_host == 0 || _.config.max_total_idle == 0) {
            _.closeFd(fd);
            _.evictions += 1;
            return false;
        }
        if (_.config.max_lifetime_ms > 0 && (now_ms - created_at_ms) >= _.config.max_lifetime_ms) {
            _.closeFd(fd);
            _.evictions += 1;
            return false;
        }

        // Evict any expired entries first
        * = _.evictExpired(now_ms);

        // Enforce per-host idle limit by evicting the oldest matching entry
        let host_count: usize = 0;
        let oldest_host_idx: usize? = null;
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].matchesHostPort(host, port)) {
                if (oldest_host_idx == null) {
                    oldest_host_idx = i;
                }
                host_count += 1;
            }
            i += 1;
        }
        if (host_count >= _.config.max_idle_per_host) {
            if (idx := oldest_host_idx) {
                const old_fd = _.entries[idx].fd;
                _.removeAt(idx);
                _.closeFd(old_fd);
                _.evictions += 1;
            }
        }

        // Enforce global idle capacity by evicting the globally oldest entry (index 0)
        while (_.len >= _.config.max_total_idle && _.len > 0) {
            const old_fd = _.entries[0].fd;
            _.removeAt(0);
            _.closeFd(old_fd);
            _.evictions += 1;
        }

        _.ensureCapacity(_.len + 1);
        let entry = PoolEntry {
            port: port,
            fd: fd,
            created_at_ms: created_at_ms,
            last_used_at_ms: now_ms,
        };
        entry.host_len = host.len;
        let k: usize = 0;
        while (k < host.len) {
            entry.host_buf[k] = host[k];
            k += 1;
        }
        _.entries[_.len] = entry;
        _.len += 1;
        return true;
    }

    /// Returns an idle FD back to the pool.
    fn release(*_, host: []const u8, port: u16, fd: i32, now_ms: i64): bool {
        return _.releaseWithCreated(host, port, fd, now_ms, now_ms);
    }

    /// Alias of `release` that avoids ARC `.release()` interception inside closures.
    fn releaseFd(*_, host: []const u8, port: u16, fd: i32, now_ms: i64): bool {
        return _.releaseWithCreated(host, port, fd, now_ms, now_ms);
    }

    /// Sweeps and closes all idle connections that exceeded `idle_timeout_ms` or `max_lifetime_ms`.
    fn evictExpired(*_, now_ms: i64): usize {
        let evicted_now: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].isExpired(now_ms, _.config)) {
                const fd = _.entries[i].fd;
                _.removeAt(i);
                _.closeFd(fd);
                _.evictions += 1;
                evicted_now += 1;
                continue;
            }
            i += 1;
        }
        return evicted_now;
    }

    fn hostIdleCount(host: []const u8, port: u16): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (_.entries[i].matchesHostPort(host, port)) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    fn stats(): PoolStats {
        return PoolStats {
            hits: _.hits,
            misses: _.misses,
            evictions: _.evictions,
            idle_count: _.len,
        };
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < _.len) {
            _.closeFd(_.entries[i].fd);
            i += 1;
        }
        _.len = 0;
    }

    fn deinit(*_) {
        _.clear();
        if (_.cap > 0) {
            std.heap.page_allocator.free(_.entries);
            _.entries = [];
            _.cap = 0;
        }
    }
}
