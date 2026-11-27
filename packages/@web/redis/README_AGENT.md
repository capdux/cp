# @web/redis — AI Agent Quick Ref

Official thin Redis lifecycle, distributed namespace key builder, and zero-heap RESP2/3 command encoder plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RedisPluginConfig,
    rdsDefaultPing,
    rdsIsSafeKeySegment,
    rdsWriteByte,
    rdsWriteRaw,
    rdsWriteUsize,
    redisBuildKey,
    redisEncodeRespCommand,
} from "@web/redis";
```

## Core Types & Constants

```cp
class RedisPluginConfig { host: string, port: u16, key_prefix: string, ping_path: string }
```

## API Signatures

```cp
fn rdsDefaultPing(cfg: &RedisPluginConfig): bool;
fn rdsIsSafeKeySegment(s: string): bool;
fn rdsWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn rdsWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn rdsWriteUsize(out_buf: []u8, pos: &mut usize, val: usize): bool;
fn redisBuildKey(prefix: string, namespace: string, key: string, out_buf: []u8): usize?;
fn redisEncodeRespCommand(args: &[]string, out_buf: []u8): usize?;
fn rdsPingHandler(ctx: &mut Context): Response;
fn createRedisPlugin(cfg: RedisPluginConfig, ping_fn: RedisPingFn): Plugin;
fn registerRedisWithPing(app: &mut App, cfg: RedisPluginConfig, ping_fn: RedisPingFn): &mut App;
fn registerRedis(app: &mut App, cfg: RedisPluginConfig): &mut App;
```

## Minimal Usage

```cp
import { rdsIsSafeKeySegment } from "@web/redis";

pub fn main() {
    let result = rdsIsSafeKeySegment("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/redis"`; install via `cap add @web/redis`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
