[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# p-retry

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap exponential backoff & retry policy state machine with min/max caps, jitter, and elapsed deadline (clean-room port of npm retry / p-retry)

## 安装与导入

```bash
cap add p-retry
```

```cp
import { rbkClampDelay } from "p-retry";
```

## 核心导出 API

- `export fn rbkClampDelay(delay: u64, max_delay: u64): u64;`
- `export fn rbkMix64(x: u64): u64;`
- `export fn computeBackoffMs(policy: RetryPolicy, attempt: u32): u64;`
- `export fn applyJitterMs(base_delay_ms: u64, jitter_bps: u32, seed: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`p-retry@1.0.0`)，许可证：MIT
