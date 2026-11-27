[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# backoff

Zero-allocation deterministic retry backoff engine with Exponential, Linear, and AWS Full/Equal/Decorrelated Jitter (Rust backoff / npm exponential-backoff parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:backoff` ([https://github.com/ihrwein/backoff](https://github.com/ihrwein/backoff))

| 元信息 | 值 |
| --- | --- |
| 包名 | `backoff` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add backoff
```

```cp
import {
    JitterKind,
    BackoffPolicy,
    BackoffState,
    makeU64,
    addWrap64,
    mulWrap64,
    nextRandU64,
    randRangeInclusive,
} from "backoff";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `JitterKind` | `None, Full, Equal, Decorrelated` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BackoffPolicy` | `class` | 字段: `initial_ms: u64, max_ms: u64, multiplier_num: u64, multiplier_den: u64, max_retries: u32, jitter: JitterKind` · 方法: `defaultPolicy` |
| `BackoffState` | `class` | 字段: `policy: BackoffPolicy, attempt: u32, prev_delay_ms: u64, rng_state: u64` · 方法: `init, nextDelayMs, reset, attempts` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `nextRandU64` | `(state: &mut u64): u64` | 调用 `nextRandU64` 并返回 `u64`。 |
| `randRangeInclusive` | `(state: &mut u64, lo: u64, hi: u64): u64` | 调用 `randRangeInclusive` 并返回 `u64`。 |
| `expDelayMs` | `(attempt: u32, initial_ms: u64, max_ms: u64): u64` | 调用 `expDelayMs` 并返回 `u64`。 |
| `linearDelayMs` | `(attempt: u32, step_ms: u64, max_ms: u64): u64` | 调用 `linearDelayMs` 并返回 `u64`。 |
| `computePolicyRawCap` | `(policy: BackoffPolicy, attempt: u32): u64` | 调用 `computePolicyRawCap` 并返回 `u64`。 |

## 最小调用示例

```cp
import { makeU64 } from "backoff";

pub fn main() {
    let result = makeU64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
