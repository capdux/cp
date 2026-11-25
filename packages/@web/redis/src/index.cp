// Origin: Official @web/redis Plugin (Pure-CP Zero-Heap Redis Lifecycle & RESP Adapter) | Created: 2026-11-25
// ============================================================================
// @web/redis — Thin Redis Lifecycle, Key Namespace & RESP2/3 Encoder for cap:web
//   - Inspired by Fastify `@fastify/redis`
//   - Distributed key namespace builder (`cache:` / `sess:` / `rl:`) with CRLF/whitespace defense
//   - Zero-heap RESP2/3 `*N\r\n$len\r\narg\r\n` wire command encoder
//   - Pluggable health-check ping callback (`GET /_redis/ping` -> 200 / 503)
//   - Zero heap allocations, zero external C dependencies
// ============================================================================

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub struct RedisPluginConfig {
    host: string = "127.0.0.1";
    port: u16 = 6379;
    key_prefix: string = "cp:";
    ping_path: string = "/_redis/ping";
}

pub type RedisPingFn = *const fn (*const RedisPluginConfig): bool;

// ============================================================================
// Internal Prefix-Scoped Helpers & State (rds*)
// ============================================================================

fn rdsDefaultPing(cfg: *const RedisPluginConfig): bool {
    * = cfg;
    return true;
}

let g_rds_cfg: RedisPluginConfig = RedisPluginConfig{};
let g_rds_ping_fn: RedisPingFn? = null;

fn rdsIsSafeKeySegment(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c <= 0x20 || c == 0x7f) {
            return false;
        }
    }
    return true;
}

fn rdsWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn rdsWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn rdsWriteUsize(out_buf: []u8, pos: *usize, val: usize): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return rdsWriteByte(out_buf, pos, '0');
    }
    while (v > 0) {
        tmp[n] = (48 + @as(u8, @truncate(v % 10))) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > out_buf.len) return false;
    while (n > 0) {
        n -= 1;
        out_buf[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

// ============================================================================
// Public @web/redis API
// ============================================================================

/// Build a distributed Redis key `<prefix><namespace>:<key>` into `out_buf`.
/// Rejects empty keys or any segment containing spaces, CRLF, or control bytes.
pub fn redisBuildKey(prefix: string, namespace: string, key: string, out_buf: []u8): usize? {
    if (key.len == 0) return null;
    if (!rdsIsSafeKeySegment(prefix)) return null;
    if (!rdsIsSafeKeySegment(namespace)) return null;
    if (!rdsIsSafeKeySegment(key)) return null;

    let pos: usize = 0;
    if (prefix.len > 0) {
        if (!rdsWriteRaw(out_buf, *pos, prefix)) return null;
        if (prefix[prefix.len - 1] != ':') {
            if (!rdsWriteByte(out_buf, *pos, ':')) return null;
        }
    }
    if (namespace.len > 0) {
        if (!rdsWriteRaw(out_buf, *pos, namespace)) return null;
        if (namespace[namespace.len - 1] != ':') {
            if (!rdsWriteByte(out_buf, *pos, ':')) return null;
        }
    }
    if (!rdsWriteRaw(out_buf, *pos, key)) return null;
    return pos;
}

/// Encode a Redis command array into standard RESP2/3 bulk-string array format:
/// `*<argc>\r\n$<len>\r\n<arg>\r\n...` (zero heap).
pub fn redisEncodeRespCommand(args: []const string, out_buf: []u8): usize? {
    if (args.len == 0) return null;
    let pos: usize = 0;
    if (!rdsWriteByte(out_buf, *pos, '*')) return null;
    if (!rdsWriteUsize(out_buf, *pos, args.len)) return null;
    if (!rdsWriteRaw(out_buf, *pos, "\r\n")) return null;

    let i: usize = 0;
    while (i < args.len) : (i += 1) {
        const arg = args[i];
        if (!rdsWriteByte(out_buf, *pos, '$')) return null;
        if (!rdsWriteUsize(out_buf, *pos, arg.len)) return null;
        if (!rdsWriteRaw(out_buf, *pos, "\r\n")) return null;
        if (!rdsWriteRaw(out_buf, *pos, arg)) return null;
        if (!rdsWriteRaw(out_buf, *pos, "\r\n")) return null;
    }
    return pos;
}

/// Built-in `GET <ping_path>` health probe handler.
pub fn rdsPingHandler(ctx: *Context): Response {
    * = ctx;
    let ok = true;
    if (ping_fn := g_rds_ping_fn) {
        ok = ping_fn(&g_rds_cfg);
    } else {
        ok = rdsDefaultPing(&g_rds_cfg);
    }
    if (ok) {
        return Response.json(200, "{\"redis\":\"pong\"}");
    }
    return Response.json(503, "{\"redis\":\"down\"}");
}

/// Create a composable Elysia-style `Plugin` for `@web/redis`.
pub fn createRedisPlugin(cfg: RedisPluginConfig, ping_fn: RedisPingFn): Plugin {
    g_rds_cfg = cfg;
    g_rds_ping_fn = ping_fn;
    const path = if (cfg.ping_path.len > 0) cfg.ping_path else "/_redis/ping";
    let p = Plugin.init("@web/redis");
    * = p.withScope(PluginScope.Global);
    * = p.setState("redis_host", cfg.host);
    * = p.setState("redis_prefix", cfg.key_prefix);
    * = p.get(path, rdsPingHandler);
    return p;
}

/// Register `@web/redis` with a custom health-check callback (fluent chainable).
pub fn registerRedisWithPing(app: *App, cfg: RedisPluginConfig, ping_fn: RedisPingFn): *App {
    const p = createRedisPlugin(cfg, ping_fn);
    return app.usePlugin(&p);
}

/// Register `@web/redis` with the default zero-network ping callback (fluent chainable).
pub fn registerRedis(app: *App, cfg: RedisPluginConfig): *App {
    return registerRedisWithPing(app, cfg, rdsDefaultPing);
}
