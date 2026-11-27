[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# backoff

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:backoff` ([https://github.com/ihrwein/backoff](https://github.com/ihrwein/backoff))

Zero-allocation deterministic retry backoff engine with Exponential, Linear, and AWS Full/Equal/Decorrelated Jitter for CP — clean-room MIT parity with Rust `backoff` and npm `exponential-backoff`.

纯 CP 零堆分配、零系统时钟依赖的确定性退避重试算法引擎，完整支持固定退避、线性退避、指数退避与 AWS 标准 Full / Equal / Decorrelated 抖动算法，专治 RPC/HTTP/LLM API 重试惊群效应。

## Install / 安装

```bash
cap add backoff
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `JitterKind` | `enum { None, Full, Equal, Decorrelated }` | Retry jitter strategy |
| `BackoffPolicy` | `struct { initial_ms, max_ms, multiplier_num, multiplier_den, max_retries, jitter }` | Policy config (`BackoffPolicy.defaultPolicy()`) |
| `BackoffState` | `init(policy, seed)`, `nextDelayMs(*_) -> u64?`, `reset(*_)`, `attempts() -> u32` | Stateful deterministic retry iterator |
| `expDelayMs` | `(attempt: u32, initial_ms: u64, max_ms: u64) -> u64` | Overflow-safe capped exponential delay |
| `linearDelayMs` | `(attempt: u32, step_ms: u64, max_ms: u64) -> u64` | Overflow-safe capped linear delay |

## Usage / 使用示例

```cp
import { BackoffPolicy, BackoffState, JitterKind, expDelayMs } from "backoff";

const policy = BackoffPolicy.defaultPolicy(); // 100ms..30000ms, 2x, 5 retries, Full jitter
let state = BackoffState.init(policy, 12345 as u64);
const d1 = state.nextDelayMs(); // deterministic delay in [0, 100]
const d2 = state.nextDelayMs(); // deterministic delay in [0, 200]
```
