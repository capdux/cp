[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`async-retry`](https://github.com/vercel/async-retry)（`MIT`）（原库名 `async-retry`，CP 包名 `@cap/retry`）。

# `@cap/retry` — 零分配工业级重试、退避、AWS 抖动与重试风暴预算引擎

> 独立 Clean-Room 纯 CP 实现，对标 Rust `backoff`/`backon`、npm `p-retry`、AWS Architecture Blog《Exponential Backoff And Jitter》及 Envoy/Finagle Retry Budget 规范 | 许可协议：MIT

## 概述

`@cap/retry` 提供纯 CP、零外部依赖、零堆分配、无浮点漂移的分布式系统重试与退避引擎：
- **4 种退避曲线**：`Fixed`（固定）、`Linear`（线性）、`Exponential`（支持任意有理数倍率如 `1.5x` / `2.0x`）、`Fibonacci`（斐波那契）；
- **4 种防惊群抖动（Jitter）**：`None`、`Full`、`Equal`、`Decorrelated`（AWS 推荐高竞争队列策略），内置 SplitMix64 PRNG（支持 OS CSPRNG 或确定性种子）；
- **错误分类与 `Retry-After` 尊重**：区分 `Transient`、`Permanent`（立即中止）与 `RateLimited`（优先采纳服务端 `Retry-After` 提示并受 `max_delay_ns` 封顶）；
- **总耗时截止预算（`max_elapsed_ns`）+ Envoy 风格 `RetryBudget`**：有效防止下游雪崩时的级联重试风暴。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `BackoffStrategy` | `Fixed` / `Linear` / `Exponential` / `Fibonacci` |
| `JitterMode` | `None` / `Full` / `Equal` / `Decorrelated` |
| `FailureKind` / `StopReason` | 失败分类与终止原因枚举 |
| `RetryPolicy` | 配置结构 + 快捷工厂 `exponentialDefault()` / `fixed()` / `decorrelated()` |
| `RetryState` | `init(policy, now_ns)`、`initSeeded(policy, now_ns, seed)`、`onFailure(kind, retry_after_ns, now_ns)`、`reset(now_ns)`、`attempts()`、`totalDelayNs()` |
| `RetryBudget` | 重试风暴保护令牌桶：`init(max_burst_retries, ratio_percent)`、`recordRequest()`、`tryAcquireRetry()`、`availableRetries()` |

## 使用示例

### 示例 1：指数退避 + AWS Full Jitter 重试循环

```cp
import { RetryPolicy, RetryState, FailureKind, NS_PER_MS } from "@cap/retry";

fn demoExponential() {
    const policy = RetryPolicy.exponentialDefault();
    let state = RetryState.init(policy, 0);

    const d1 = state.onFailure(FailureKind.Transient, 0, 10 * NS_PER_MS);
    if (d1.should_retry) {
        // sleep(d1.delay_ns) 后继续重试
    }
}
```

### 示例 2：尊重服务端 `429 Retry-After` 与 `Permanent` 立即中止

```cp
import { RetryPolicy, RetryState, FailureKind, StopReason, NS_PER_SEC } from "@cap/retry";

fn demoClassify() {
    let state = RetryState.initSeeded(RetryPolicy.exponentialDefault(), 0, 42);

    // 429 限流：按服务端建议退避 2 秒
    const d1 = state.onFailure(FailureKind.RateLimited, 2 * NS_PER_SEC, 0);
    // 400 参数错误：Permanent 立即中止，不浪费重试配额
    const d2 = state.onFailure(FailureKind.Permanent, 0, 0);
    // d2.should_retry == false && d2.stop_reason == StopReason.PermanentFailure
    * = d1;
    * = d2;
}
```

### 示例 3：分布式防重试风暴 `RetryBudget`

```cp
import { RetryBudget } from "@cap/retry";

fn demoBudget() {
    // 允许最多 2 次突发重试，且重试流量占比不超过正常流量的 20%
    let budget = RetryBudget.init(2, 20);
    budget.recordRequest();
    if (budget.tryAcquireRetry()) {
        // 允许执行重试
    }
}
```
