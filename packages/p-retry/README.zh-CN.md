[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-retry

Zero-heap exponential backoff & retry policy state machine with min/max caps, jitter, and elapsed deadline (clean-room port of npm retry / p-retry)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:p-retry` ([https://github.com/sindresorhus/p-retry](https://github.com/sindresorhus/p-retry))（原库名 `p-retry` → CP 包名 `p-retry`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `p-retry` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add p-retry
```

```cp
import {
    RetryPolicy,
    BackoffState,
    rbkClampDelay,
    rbkMix64,
    computeBackoffMs,
    applyJitterMs,
} from "p-retry";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RetryPolicy` | `class` | 字段: `max_retries: u32, min_delay_ms: u64, max_delay_ms: u64, factor_num: u64, factor_den: u64, jitter_bps: u32, +1 项` · 方法: `exponential, constant` |
| `BackoffState` | `class` | 字段: `policy: RetryPolicy, attempt: u32, total_waited_ms: u64, rng_seed: u64` · 方法: `new, attempts, totalWaitedMs, nextDelayMs, reset` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rbkClampDelay` | `(delay: u64, max_delay: u64): u64` | 调用 `rbkClampDelay` 并返回 `u64`。 |
| `rbkMix64` | `(x: u64): u64` | 调用 `rbkMix64` 并返回 `u64`。 |
| `computeBackoffMs` | `(policy: RetryPolicy, attempt: u32): u64` | 调用 `computeBackoffMs` 并返回 `u64`。 |
| `applyJitterMs` | `(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64` | 调用 `applyJitterMs` 并返回 `u64`。 |

## 最小调用示例

```cp
import { rbkClampDelay } from "p-retry";

pub fn main() {
    let result = rbkClampDelay(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
