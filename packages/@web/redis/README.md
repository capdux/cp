[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/redis

Official thin Redis lifecycle, distributed namespace key builder, and zero-heap RESP2/3 command encoder plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/redis` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/redis
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RedisPluginConfig` | `class` | Fields: `host: string, port: u16, key_prefix: string, ping_path: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rdsDefaultPing` | `(cfg: &RedisPluginConfig): bool` | Invokes `rdsDefaultPing` and returns `bool`. |
| `rdsIsSafeKeySegment` | `(s: string): bool` | Invokes `rdsIsSafeKeySegment` and returns `bool`. |
| `rdsWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `rdsWriteByte` and returns `bool`. |
| `rdsWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `rdsWriteRaw` and returns `bool`. |
| `rdsWriteUsize` | `(out_buf: []u8, pos: &mut usize, val: usize): bool` | Invokes `rdsWriteUsize` and returns `bool`. |
| `redisBuildKey` | `(prefix: string, namespace: string, key: string, out_buf: []u8): usize?` | Invokes `redisBuildKey` and returns `usize?`. |
| `redisEncodeRespCommand` | `(args: &[]string, out_buf: []u8): usize?` | Invokes `redisEncodeRespCommand` and returns `usize?`. |
| `rdsPingHandler` | `(ctx: &mut Context): Response` | Invokes `rdsPingHandler` and returns `Response`. |
| `createRedisPlugin` | `(cfg: RedisPluginConfig, ping_fn: RedisPingFn): Plugin` | Constructs/initializes an instance via `createRedisPlugin`. |
| `registerRedisWithPing` | `(app: &mut App, cfg: RedisPluginConfig, ping_fn: RedisPingFn): &mut App` | Registers plugin/middleware into the target application. |
| `registerRedis` | `(app: &mut App, cfg: RedisPluginConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { rdsIsSafeKeySegment } from "@web/redis";

pub fn main() {
    let result = rdsIsSafeKeySegment("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
