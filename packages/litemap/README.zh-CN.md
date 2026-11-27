[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# litemap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation compact sorted flat-array map with O(log N) binary search and deterministic ordered iteration (Rust ICU4X litemap parity) for CP

## 安装与导入

```bash
cap add litemap
```

```cp
import { cmpStr } from "litemap";
```

## 核心导出 API

- `export fn cmpStr(a: string, b: string): i8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`litemap@1.0.0`)，许可证：Unicode-3.0
