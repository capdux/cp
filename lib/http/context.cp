// Source: /data/dev/bun/src/http/HTTPContext.cp
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
