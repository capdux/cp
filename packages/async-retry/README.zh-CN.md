[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# async-retry

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation industrial retry & backoff engine (Fixed/Linear/Exponential/Fibonacci + AWS Full/Equal/Decorrelated Jitter + RetryBudget) for CP

## 安装与导入

```bash
cap add async-retry
```

```cp
import { saturatingMulU64 } from "async-retry";
```

## 核心导出 API

- `export enum BackoffStrategy`
- `export enum JitterMode`
- `export enum FailureKind`
- `export enum StopReason`
- `export fn saturatingMulU64(a: u64, b: u64): u64;`
- `export fn clampDelay(delay: u64, max_delay: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`async-retry@1.0.0`)，许可证：MIT
