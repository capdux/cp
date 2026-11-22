// Source: Absorbed from /data/dev/bun/src/http/HTTPContext.cp | Synced: 2026-11-22T05:28:51Z
// lib/http/context.cp
// CP HTTP Context & Keep-Alive Connection Pool (cap:http)

const std = @import("std");

pub const MAX_POOLED_SOCKETS: usize = 64;

pub struct PooledSocket {
    pub fd: i32 = -1;
    pub hostname: []const u8 = "";
    pub port: u16 = 80;
    pub is_ssl: bool = false;
    pub in_use: bool = false;
    pub last_used: i64 = 0;

    pub fn isValid(): bool {
        return _.fd >= 0 && _.in_use;
    }

    pub fn close(*_) {
        if (_.fd >= 0) {
            * = std.os.linux.close(_.fd);
            _.fd = -1;
        }
        _.in_use = false;
    }
}

pub struct HTTPContext {
    pub pool: [MAX_POOLED_SOCKETS]PooledSocket = undefined;
    pub pool_count: usize = 0;
    pub max_idle_seconds: i64 = 300;

    pub static fn init(): HTTPContext {
        let ctx = HTTPContext{
            pool: undefined,
            pool_count: 0,
            max_idle_seconds: 300,
        };
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            ctx.pool[i] = PooledSocket{};
        }
        return ctx;
    }

    pub fn acquireSocket(*_, hostname: []const u8, port: u16, is_ssl: bool): i32? {
        let i: usize = 0;
        while (i < _.pool_count) : (i += 1) {
            if (!_.pool[i].in_use && _.pool[i].port == port && _.pool[i].is_ssl == is_ssl) {
                if (std.mem.eql(u8, _.pool[i].hostname, hostname)) {
                    _.pool[i].in_use = true;
                    return _.pool[i].fd;
                }
            }
        }
        return null;
    }

    pub fn releaseSocket(*_, fd: i32, hostname: []const u8, port: u16, is_ssl: bool): bool {
        let i: usize = 0;
        // Check if already in pool
        while (i < _.pool_count) : (i += 1) {
            if (_.pool[i].fd == fd) {
                _.pool[i].in_use = false;
                _.pool[i].last_used = 0;
                return true;
            }
        }
        // Add new socket to pool if capacity allows
        if (_.pool_count < MAX_POOLED_SOCKETS) {
            _.pool[_.pool_count] = PooledSocket{
                fd: fd,
                hostname: hostname,
                port: port,
                is_ssl: is_ssl,
                in_use: false,
                last_used: 0,
            };
            _.pool_count += 1;
            return true;
        }
        return false;
    }

    pub fn closeDeadSocket(*_, fd: i32) {
        let i: usize = 0;
        while (i < _.pool_count) : (i += 1) {
            if (_.pool[i].fd == fd) {
                _.pool[i].close();
                // Shift remaining sockets
                let j: usize = i;
                while (j + 1 < _.pool_count) : (j += 1) {
                    _.pool[j] = _.pool[j + 1];
                }
                _.pool_count -= 1;
                return;
            }
        }
    }

    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.pool_count) : (i += 1) {
            _.pool[i].close();
        }
        _.pool_count = 0;
    }
}

pub enum PoolEntryState: u8 {
    Empty = 0,
    Idle = 1,
    InUse = 2,
    Dead = 3,
}

pub struct PooledConnection {
    pub fd: i32 = -1,
    pub host: []const u8 = "",
    pub port: u16 = 80,
    pub is_ssl: bool = false,
    pub state: PoolEntryState = PoolEntryState.Empty,
    pub created_at_ms: i64 = 0,
    pub last_used_ms: i64 = 0,
    pub requests_served: u32 = 0,
    pub healthy: bool = true,
}

pub struct PoolStats {
    pub total_connections: usize = 0,
    pub idle_connections: usize = 0,
    pub active_connections: usize = 0,
    pub hits: u64 = 0,
    pub misses: u64 = 0,
    pub evictions: u64 = 0,
    pub unhealthy_closed: u64 = 0,
}

pub struct HttpConnectionPool {
    pub slots: [MAX_POOLED_SOCKETS]PooledConnection = undefined,
    pub max_total_connections: usize = MAX_POOLED_SOCKETS,
    pub max_per_host: usize = 8,
    pub idle_timeout_ms: i64 = 60000,
    pub max_requests_per_conn: u32 = 1000,
    pub hits: u64 = 0,
    pub misses: u64 = 0,
    pub evictions: u64 = 0,
    pub unhealthy_closed: u64 = 0,

    pub static fn init(
        max_per_host: usize = 8,
        max_total: usize = MAX_POOLED_SOCKETS,
        idle_timeout_ms: i64 = 60000,
        max_requests_per_conn: u32 = 1000,
    ): HttpConnectionPool {
        const clamped_total = if (max_total == 0 || max_total > MAX_POOLED_SOCKETS) MAX_POOLED_SOCKETS else max_total;
        const clamped_host = if (max_per_host == 0) 1 else max_per_host;
        let pool = HttpConnectionPool{
            slots: undefined,
            max_total_connections: clamped_total,
            max_per_host: clamped_host,
            idle_timeout_ms: idle_timeout_ms,
            max_requests_per_conn: max_requests_per_conn,
            hits: 0,
            misses: 0,
            evictions: 0,
            unhealthy_closed: 0,
        };
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            pool.slots[i] = PooledConnection{};
        }
        return pool;
    }

    fn matchesHost(slot: PooledConnection, host: []const u8, port: u16, is_ssl: bool): bool {
        return slot.port == port && slot.is_ssl == is_ssl && std.ascii.eqlIgnoreCase(slot.host, host);
    }

    fn isExpired(slot: PooledConnection, now_ms: i64, idle_timeout_ms: i64): bool {
        if (idle_timeout_ms <= 0) return false;
        return (now_ms - slot.last_used_ms) > idle_timeout_ms;
    }

    pub fn countForHost(host: []const u8, port: u16, is_ssl: bool): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            const s = _.slots[i];
            if ((s.state == PoolEntryState.Idle || s.state == PoolEntryState.InUse) && matchesHost(s, host, port, is_ssl)) {
                count += 1;
            }
        }
        return count;
    }

    pub fn idleCountForHost(host: []const u8, port: u16, is_ssl: bool): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            const s = _.slots[i];
            if (s.state == PoolEntryState.Idle && matchesHost(s, host, port, is_ssl)) {
                count += 1;
            }
        }
        return count;
    }

    pub fn totalCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Idle || _.slots[i].state == PoolEntryState.InUse) {
                count += 1;
            }
        }
        return count;
    }

    /// Acquires a healthy, non-expired idle connection for `host:port`, or returns null.
    pub fn acquire(*_, host: []const u8, port: u16, is_ssl: bool, now_ms: i64): i32? {
        let best_idx: usize? = null;
        let best_last_used: i64 = -1;

        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Idle && matchesHost(_.slots[i], host, port, is_ssl)) {
                if (!_.slots[i].healthy || _.slots[i].fd < 0) {
                    _.slots[i] = PooledConnection{};
                    _.unhealthy_closed += 1;
                    continue;
                }
                if (isExpired(_.slots[i], now_ms, _.idle_timeout_ms) ||
                    (_.max_requests_per_conn > 0 && _.slots[i].requests_served >= _.max_requests_per_conn))
                {
                    _.slots[i] = PooledConnection{};
                    _.evictions += 1;
                    continue;
                }
                // Prefer most recently used healthy socket (LIFO for warm TCP window)
                if (best_idx == null || _.slots[i].last_used_ms >= best_last_used) {
                    best_idx = i;
                    best_last_used = _.slots[i].last_used_ms;
                }
            }
        }

        if (idx := best_idx) {
            _.slots[idx].state = PoolEntryState.InUse;
            _.slots[idx].last_used_ms = now_ms;
            _.slots[idx].requests_served += 1;
            _.hits += 1;
            return _.slots[idx].fd;
        }

        _.misses += 1;
        return null;
    }

    /// Registers a newly dialed active connection into the pool if capacity allows.
    pub fn addConnection(*_, fd: i32, host: []const u8, port: u16, is_ssl: bool, now_ms: i64): bool {
        if (fd < 0) return false;
        if (_.countForHost(host, port, is_ssl) >= _.max_per_host) return false;
        if (_.totalCount() >= _.max_total_connections) {
            if (!_.evictOldestIdle()) return false;
        }

        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Empty || _.slots[i].state == PoolEntryState.Dead) {
                _.slots[i] = PooledConnection{
                    fd: fd,
                    host: host,
                    port: port,
                    is_ssl: is_ssl,
                    state: PoolEntryState.InUse,
                    created_at_ms: now_ms,
                    last_used_ms: now_ms,
                    requests_served: 1,
                    healthy: true,
                };
                return true;
            }
        }
        return false;
    }

    fn evictOldestIdle(*_): bool {
        let oldest_idx: usize? = null;
        let oldest_ts: i64 = 0;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Idle) {
                if (oldest_idx == null || _.slots[i].last_used_ms < oldest_ts) {
                    oldest_idx = i;
                    oldest_ts = _.slots[i].last_used_ms;
                }
            }
        }
        if (idx := oldest_idx) {
            _.slots[idx] = PooledConnection{};
            _.evictions += 1;
            return true;
        }
        return false;
    }

    /// Returns a connection back to the idle pool (or evicts/drops if keep_alive is false or limits exceeded).
    pub fn releaseConnection(*_, fd: i32, host: []const u8, port: u16, is_ssl: bool, now_ms: i64, keep_alive: bool): bool {
        if (fd < 0) return false;

        let existing_idx: usize? = null;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].fd == fd && (_.slots[i].state == PoolEntryState.InUse || _.slots[i].state == PoolEntryState.Idle)) {
                existing_idx = i;
                break;
            }
        }

        if (idx := existing_idx) {
            if (!keep_alive || !_.slots[idx].healthy ||
                (_.max_requests_per_conn > 0 && _.slots[idx].requests_served >= _.max_requests_per_conn))
            {
                _.slots[idx] = PooledConnection{};
                _.evictions += 1;
                return false;
            }
            _.slots[idx].state = PoolEntryState.Idle;
            _.slots[idx].last_used_ms = now_ms;
            return true;
        }

        if (!keep_alive) return false;
        if (_.countForHost(host, port, is_ssl) >= _.max_per_host) return false;
        if (_.totalCount() >= _.max_total_connections) {
            if (!_.evictOldestIdle()) return false;
        }

        i = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Empty || _.slots[i].state == PoolEntryState.Dead) {
                _.slots[i] = PooledConnection{
                    fd: fd,
                    host: host,
                    port: port,
                    is_ssl: is_ssl,
                    state: PoolEntryState.Idle,
                    created_at_ms: now_ms,
                    last_used_ms: now_ms,
                    requests_served: 0,
                    healthy: true,
                };
                return true;
            }
        }
        return false;
    }

    pub fn markUnhealthy(*_, fd: i32): bool {
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].fd == fd && (_.slots[i].state == PoolEntryState.Idle || _.slots[i].state == PoolEntryState.InUse)) {
                _.slots[i].healthy = false;
                _.slots[i].state = PoolEntryState.Dead;
                _.slots[i].fd = -1;
                _.unhealthy_closed += 1;
                return true;
            }
        }
        return false;
    }

    pub fn healthCheck(*_, fd: i32, is_alive: bool): bool {
        if (!is_alive) {
            * = _.markUnhealthy(fd);
            return false;
        }
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].fd == fd && (_.slots[i].state == PoolEntryState.Idle || _.slots[i].state == PoolEntryState.InUse)) {
                return _.slots[i].healthy;
            }
        }
        return false;
    }

    pub fn evictExpired(*_, now_ms: i64): usize {
        let evicted: usize = 0;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Idle) {
                if (!_.slots[i].healthy || isExpired(_.slots[i], now_ms, _.idle_timeout_ms)) {
                    _.slots[i] = PooledConnection{};
                    _.evictions += 1;
                    evicted += 1;
                }
            }
        }
        return evicted;
    }

    pub fn stats(): PoolStats {
        let total: usize = 0;
        let idle: usize = 0;
        let active: usize = 0;
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            if (_.slots[i].state == PoolEntryState.Idle) {
                idle += 1;
                total += 1;
            } else if (_.slots[i].state == PoolEntryState.InUse) {
                active += 1;
                total += 1;
            }
        }
        return PoolStats{
            total_connections: total,
            idle_connections: idle,
            active_connections: active,
            hits: _.hits,
            misses: _.misses,
            evictions: _.evictions,
            unhealthy_closed: _.unhealthy_closed,
        };
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_POOLED_SOCKETS) : (i += 1) {
            _.slots[i] = PooledConnection{};
        }
    }
}
