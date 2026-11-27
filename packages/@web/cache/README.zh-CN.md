[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/cache

Official zero-heap response caching and Stripe-style Idempotency-Key plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/cache` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/cache
```

```cp
import {
    CacheConfig,
    CacheEntry,
    cacheMonotonicNowSec,
    cacheNowSec,
    cacheFormatCacheControl,
    cacheCopyInto,
    cacheBuildGetKey,
    cacheBuildIdemKey,
} from "@web/cache";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CacheConfig` | `class` | 字段: `ttl_secs: u64, max_entries: usize, enable_idempotency: bool, idempotency_header: string` |
| `CacheEntry` | `class` | 字段: `used: bool, is_idem: bool, key_buf: [128]u8, key_len: usize, status: u16, ct_buf: [64]u8, +4 项` · 方法: `matches, bodySlice, ctSlice` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cacheMonotonicNowSec` | `(): u64` | 调用 `cacheMonotonicNowSec` 并返回 `u64`。 |
| `cacheNowSec` | `(): u64` | 调用 `cacheNowSec` 并返回 `u64`。 |
| `cacheFormatCacheControl` | `(ttl: u64): string` | 调用 `cacheFormatCacheControl` 并返回 `string`。 |
| `cacheCopyInto` | `(dst: []u8, src: string): usize` | 调用 `cacheCopyInto` 并返回 `usize`。 |
| `cacheBuildGetKey` | `(ctx: &mut Context, out_buf: []u8): string` | 调用 `cacheBuildGetKey` 并返回 `string`。 |
| `cacheBuildIdemKey` | `(ctx: &mut Context, idem_val: string, out_buf: []u8): string` | 调用 `cacheBuildIdemKey` 并返回 `string`。 |
| `cacheIsWriteMethod` | `(m: AppMethod): bool` | 调用 `cacheIsWriteMethod` 并返回 `bool`。 |
| `cacheEffectiveCap` | `(): usize` | 调用 `cacheEffectiveCap` 并返回 `usize`。 |
| `cacheFindEntry` | `(key: string, is_idem: bool, now_sec: u64): usize?` | 调用 `cacheFindEntry` 并返回 `usize?`。 |
| `cachePutEntry` | `(key: string, is_idem: bool, resp: &mut Response, now_sec: u64)` | 就地执行 `cachePutEntry` 操作。 |
| `setCacheTestClock` | `(now_sec: u64)` | 写入或更新状态数据。 |
| `resetCacheStore` | `()` | 就地执行 `resetCacheStore` 操作。 |
| `cacheBeforeHandleHook` | `(ctx: &mut Context): bool` | 调用 `cacheBeforeHandleHook` 并返回 `bool`。 |
| `cacheAfterHandleHook` | `(ctx: &mut Context, resp: &mut Response)` | 就地执行 `cacheAfterHandleHook` 操作。 |
| `createCachePlugin` | `(cfg: CacheConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerCache` | `(app: &mut App, cfg: CacheConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { cacheCopyInto } from "@web/cache";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cacheCopyInto(out_buf[0..256], "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
