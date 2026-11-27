[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/ratelimit

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official zero-heap fixed-slot sliding-window rate limiter plugin for cap:web

## 安装与导入

```bash
cap add @web/ratelimit
```

```cp
import { monotonicNowSec } from "@web/ratelimit";
```

## 核心导出 API

- `export fn monotonicNowSec(): u64;`
- `export fn formatU64(val: u64, out_buf: []u8): string;`
- `export fn extractFirstToken(raw: string): string;`
- `export fn setTestClock(now_sec: u64);`
- `export fn resetGlobal();`
- `export fn rateLimitHook(ctx: &mut Context): bool;`
- `export fn createRateLimitPlugin(cfg: RateLimitConfig): Plugin;`
- `export fn registerRateLimit(app: &mut App, cfg: RateLimitConfig): &mut App;`
