[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/redis

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official thin Redis lifecycle, distributed namespace key builder, and zero-heap RESP2/3 command encoder plugin for cap:web

## 安装与导入

```bash
cap add @web/redis
```

```cp
import { rdsDefaultPing } from "@web/redis";
```

## 核心导出 API

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
