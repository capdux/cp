[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# p-timeout

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deadline tracker and timeout state machine guard (npm p-timeout v6 equivalent) for CP.

## 安装与导入

```bash
cap add p-timeout
```

```cp
import { ptoMaxU64 } from "p-timeout";
```

## 核心导出 API

- `export fn ptoMaxU64(): u64;`
- `export fn ptoSaturatingAdd(a: u64, b: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`p-timeout@1.0.0`)，许可证：MIT
