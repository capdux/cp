[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# express-rate-limit

Zero-heap fixed-window HTTP rate limiter with IETF draft-7 RateLimit header formatting

| 元信息 | 值 |
| --- | --- |
| 包名 | `express-rate-limit` |
| 版本 | `7.5.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add express-rate-limit
```

```cp
import {
    RateLimitHit,
    RateLimitSlot,
    RateLimiter,
    erlEmptySlot,
    erlHashKey,
    erlSlotMatches,
    erlStoreSlot,
    erlCeilSeconds,
} from "express-rate-limit";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RateLimitHit` | `class` | 字段: `allowed: bool, limit: u32, current: u32, remaining: u32, reset_at_ms: i64, retry_after_sec: u32` |
| `RateLimitSlot` | `class` | 字段: `occupied: bool, hash: u64, key_len: usize, key_buf: [64]u8, count: u32, reset_at_ms: i64` |
| `RateLimiter` | `class` | 字段: `window_ms: i64, max_requests: u32, slots: [64]RateLimitSlot` · 方法: `init, hit, resetKey, resetAll, activeCount` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `erlEmptySlot` | `(): RateLimitSlot` | 调用 `erlEmptySlot` 并返回 `RateLimitSlot`。 |
| `erlHashKey` | `(key: string): u64` | 调用 `erlHashKey` 并返回 `u64`。 |
| `erlSlotMatches` | `(slot: RateLimitSlot, h: u64, key: string): bool` | 调用 `erlSlotMatches` 并返回 `bool`。 |
| `erlStoreSlot` | `(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot` | 调用 `erlStoreSlot` 并返回 `RateLimitSlot`。 |
| `erlCeilSeconds` | `(reset_at_ms: i64, now_ms: i64): u32` | 调用 `erlCeilSeconds` 并返回 `u32`。 |
| `erlWriteStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `erlWriteStr` 并返回 `usize`。 |
| `erlWriteU32` | `(out: []u8, pos: usize, val: u32): usize` | 调用 `erlWriteU32` 并返回 `usize`。 |
| `formatRateLimitHeader` | `(hit: RateLimitHit, out: []u8): usize` | 通过 `formatRateLimitHeader` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { erlHashKey } from "express-rate-limit";

pub fn main() {
    let result = erlHashKey("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
