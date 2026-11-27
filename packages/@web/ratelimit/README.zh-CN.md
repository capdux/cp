[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/ratelimit

Official zero-heap fixed-slot sliding-window rate limiter plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/ratelimit` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/ratelimit
```

```cp
import {
    RateLimitConfig,
    RateLimitDecision,
    RateLimitSlot,
    RateLimiter,
    monotonicNowSec,
    formatU64,
    extractFirstToken,
    setTestClock,
} from "@web/ratelimit";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RateLimitConfig` | `class` | 字段: `max_requests: u32, window_secs: u64, key_header: string` |
| `RateLimitDecision` | `class` | 字段: `allowed: bool, limit: u32, remaining: u32, retry_after: u64` |
| `RateLimitSlot` | `class` | 字段: `used: bool, key_buf: [RATELIMIT_MAX_KEY_LEN]u8, key_len: usize, window_start: u64, count: u32` · 方法: `matchesKey, setKey` |
| `RateLimiter` | `class` | 字段: `config: RateLimitConfig, slots: [RATELIMIT_MAX_SLOTS]RateLimitSlot` · 方法: `init, reset, check` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `monotonicNowSec` | `(): u64` | 调用 `monotonicNowSec` 并返回 `u64`。 |
| `formatU64` | `(val: u64, out_buf: []u8): string` | 通过 `formatU64` 编码或格式化输出，返回 `string`。 |
| `extractFirstToken` | `(raw: string): string` | 调用 `extractFirstToken` 并返回 `string`。 |
| `setTestClock` | `(now_sec: u64)` | 写入或更新状态数据。 |
| `resetGlobal` | `()` | 就地执行 `resetGlobal` 操作。 |
| `rateLimitHook` | `(ctx: &mut Context): bool` | 调用 `rateLimitHook` 并返回 `bool`。 |
| `createRateLimitPlugin` | `(cfg: RateLimitConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerRateLimit` | `(app: &mut App, cfg: RateLimitConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { extractFirstToken } from "@web/ratelimit";

pub fn main() {
    let result = extractFirstToken("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
