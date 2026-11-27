[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# governor

Zero-allocation IETF HTTP RateLimit headers (RateLimit-Policy / RateLimit / X-RateLimit-*) parser, formatter, and reset delay calculator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:governor` ([https://github.com/boinkor-net/governor](https://github.com/boinkor-net/governor)) / `crates.io:governor` ([https://github.com/boinkor-net/governor](https://github.com/boinkor-net/governor)) / `npm:express-rate-limit` ([https://github.com/express-rate-limit/express-rate-limit](https://github.com/express-rate-limit/express-rate-limit))（原库名 `governor` → CP 包名 `governor`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `governor` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add governor
```

```cp
import {
    RateLimitPolicy,
    RateLimitState,
    rhToLower,
    rhIsOws,
    rhTrimOws,
    rhEqIgnoreCase,
    rhFormatU64,
    rhCopyInto,
} from "governor";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RateLimitPolicy` | `class` | 字段: `quota: u64, window_secs: u64, burst: u64, has_burst: bool, valid: bool` |
| `RateLimitState` | `class` | 字段: `limit: u64, remaining: u64, reset_secs: u64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rhToLower` | `(c: u8): u8` | 调用 `rhToLower` 并返回 `u8`。 |
| `rhIsOws` | `(c: u8): bool` | 调用 `rhIsOws` 并返回 `bool`。 |
| `rhTrimOws` | `(s: &[]u8): &[]u8` | 调用 `rhTrimOws` 并返回 `&[]u8`。 |
| `rhEqIgnoreCase` | `(a: &[]u8, b: &[]u8): bool` | 调用 `rhEqIgnoreCase` 并返回 `bool`。 |
| `rhFormatU64` | `(val: u64, buf: []u8): usize` | 调用 `rhFormatU64` 并返回 `usize`。 |
| `rhCopyInto` | `(dst: []u8, pos: &mut usize, src: &[]u8)` | 就地执行 `rhCopyInto` 操作。 |
| `parseUintHeader` | `(value: string): i64` | 通过 `parseUintHeader` 解析输入数据，返回 `i64`。 |
| `parsePolicy` | `(header: string): RateLimitPolicy` | 通过 `parsePolicy` 解析输入数据，返回 `RateLimitPolicy`。 |
| `formatPolicy` | `(quota: u64, window_secs: u64, out: []u8): usize` | 通过 `formatPolicy` 编码或格式化输出，返回 `usize`。 |
| `parseStructuredRateLimit` | `(header: string): RateLimitState` | 通过 `parseStructuredRateLimit` 解析输入数据，返回 `RateLimitState`。 |
| `formatStructuredRateLimit` | `(state: RateLimitState, out: []u8): usize` | 通过 `formatStructuredRateLimit` 编码或格式化输出，返回 `usize`。 |
| `isExhausted` | `(state: RateLimitState): bool` | 执行 `isExhausted` 条件判定，返回 `bool`。 |
| `computeResetDelaySecs` | `(reset_val: u64, now_unix_secs: u64): u64` | 调用 `computeResetDelaySecs` 并返回 `u64`。 |

## 最小调用示例

```cp
import { parseUintHeader } from "governor";

pub fn main() {
    let result = parseUintHeader("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
