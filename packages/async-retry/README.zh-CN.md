[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# async-retry

Zero-allocation industrial retry & backoff engine (Fixed/Linear/Exponential/Fibonacci + AWS Full/Equal/Decorrelated Jitter + RetryBudget) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`async-retry`](https://github.com/vercel/async-retry)（`MIT`）（原库名 `async-retry`，CP 包名 `async-retry`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `async-retry` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add async-retry
```

```cp
import {
    BackoffStrategy,
    JitterMode,
    FailureKind,
    StopReason,
    RetryPolicy,
    RetryDecision,
    SplitMix64,
    RetryState,
} from "async-retry";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `BackoffStrategy` | `Fixed, Linear, Exponential, Fibonacci` |
| `enum` | `JitterMode` | `None, Full, Equal, Decorrelated` |
| `enum` | `FailureKind` | `Transient, Permanent, RateLimited` |
| `enum` | `StopReason` | `None, Succeeded, PermanentFailure, MaxAttemptsReached, ElapsedBudgetExceeded, RetryBudgetExhausted` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RetryPolicy` | `class` | 字段: `strategy: BackoffStrategy, jitter: JitterMode, max_retries: u32, base_delay_ns: u64, max_delay_ns: u64, max_elapsed_ns: u64, +2 项` · 方法: `exponentialDefault, fixed, decorrelated` |
| `RetryDecision` | `class` | 字段: `should_retry: bool, delay_ns: u64, attempt: u32, stop_reason: StopReason` |
| `SplitMix64` | `class` | 字段: `state: u64` · 方法: `fromOs, fromSeed, nextU64, uniformInclusive` |
| `RetryState` | `class` | 字段: `policy: RetryPolicy, rng: SplitMix64, attempts_done: u32, prev_delay_ns: u64, fib_prev: u64, fib_curr: u64, +2 项` · 方法: `init, initSeeded, reset, computeRawDelay, applyJitter, onFailure, +2 项` |
| `RetryBudget` | `class` | 字段: `tokens: u64, max_tokens: u64, deposit_per_request: u64, withdraw_per_retry: u64, retries_allowed: u64, retries_rejected: u64` · 方法: `init, recordRequest, tryAcquireRetry, availableRetries` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `saturatingMulU64` | `(a: u64, b: u64): u64` | 调用 `saturatingMulU64` 并返回 `u64`。 |
| `clampDelay` | `(delay: u64, max_delay: u64): u64` | 调用 `clampDelay` 并返回 `u64`。 |

## 最小调用示例

```cp
import { saturatingMulU64 } from "async-retry";

pub fn main() {
    let result = saturatingMulU64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
