[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# backoff

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deterministic retry backoff engine with Exponential, Linear, and AWS Full/Equal/Decorrelated Jitter (Rust backoff / npm exponential-backoff parity) for CP

## 安装与导入

```bash
cap add backoff
```

```cp
import { expDelayMs } from "backoff";
```

## 核心导出 API

- `export enum JitterKind`
- `export fn expDelayMs(attempt: u32, initial_ms: u64, max_ms: u64): u64;`
- `export fn linearDelayMs(attempt: u32, step_ms: u64, max_ms: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`backoff@1.0.0`)，许可证：MIT
