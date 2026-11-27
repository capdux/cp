# @web/redis@1.0.0 (Agent Quickref)

> Official thin Redis lifecycle, distributed namespace key builder, and zero-heap RESP2/3 command encoder plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/redis";` | Full types: `src/index.d.cp`

## Functions
- `export fn rdsDefaultPing(cfg: &RedisPluginConfig): bool;`
- `export fn rdsIsSafeKeySegment(s: string): bool;`
- `export fn rdsWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;`
- `export fn rdsWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn rdsWriteUsize(out_buf: []u8, pos: &mut usize, val: usize): bool;`
- `export fn redisBuildKey(prefix: string, namespace: string, key: string, out_buf: []u8): usize?;`
- `export fn redisEncodeRespCommand(args: &[]string, out_buf: []u8): usize?;`
- `export fn rdsPingHandler(ctx: &mut Context): Response;`
- `export fn createRedisPlugin(cfg: RedisPluginConfig, ping_fn: RedisPingFn): Plugin;`
- `export fn registerRedisWithPing(app: &mut App, cfg: RedisPluginConfig, ping_fn: RedisPingFn): &mut App;`
- `export fn registerRedis(app: &mut App, cfg: RedisPluginConfig): &mut App;`
