[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lodash.debounce

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation deterministic clock-driven Debouncer and Throttler state machines with leading/trailing/maxWait semantics in pure CP

## 安装与导入

```bash
cap add lodash.debounce
```

```cp
import { elapsedSince } from "lodash.debounce";
```

## 核心导出 API

- `export fn elapsedSince(now_ms: u64, past_ms: u64): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`lodash.debounce@1.0.0`)，许可证：MIT
