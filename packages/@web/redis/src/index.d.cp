// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class RedisPluginConfig {
    host: string,
    port: u16,
    key_prefix: string,
    ping_path: string,
}

export fn rdsDefaultPing(cfg: &RedisPluginConfig): bool;
export fn rdsIsSafeKeySegment(s: string): bool;
export fn rdsWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
export fn rdsWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
export fn rdsWriteUsize(out_buf: []u8, pos: &mut usize, val: usize): bool;
export fn redisBuildKey(prefix: string, namespace: string, key: string, out_buf: []u8): ?usize;
export fn redisEncodeRespCommand(args: &[]string, out_buf: []u8): ?usize;
export fn rdsPingHandler(ctx: &mut Context): Response;
export fn createRedisPlugin(cfg: RedisPluginConfig, ping_fn: RedisPingFn): Plugin;
export fn registerRedisWithPing(app: &mut App, cfg: RedisPluginConfig, ping_fn: RedisPingFn): &mut App;
export fn registerRedis(app: &mut App, cfg: RedisPluginConfig): &mut App;
