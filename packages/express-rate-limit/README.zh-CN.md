[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# express-rate-limit

> 版本：`7.5.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap fixed-window HTTP rate limiter with IETF draft-7 RateLimit header formatting

## 安装与导入

```bash
cap add express-rate-limit
```

```cp
import { erlEmptySlot } from "express-rate-limit";
```

## 核心导出 API

- `export fn erlEmptySlot(): RateLimitSlot;`
- `export fn erlHashKey(key: string): u64;`
- `export fn erlSlotMatches(slot: RateLimitSlot, h: u64, key: string): bool;`
- `export fn erlStoreSlot(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot;`
- `export fn erlCeilSeconds(reset_at_ms: i64, now_ms: i64): u32;`
- `export fn erlWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn erlWriteU32(out: []u8, pos: usize, val: u32): usize;`
- `export fn formatRateLimitHeader(hit: RateLimitHit, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`express-rate-limit@7.5.0`)（https://github.com/express-rate-limit/express-rate-limit），许可证：MIT
