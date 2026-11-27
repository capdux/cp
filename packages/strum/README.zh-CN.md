[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strum

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap enum variant string registry, case-insensitive parser, discriminant lookup, and iteration (clean-room port of Rust strum)

## 安装与导入

```bash
cap add strum
```

```cp
import { stmStrEq } from "strum";
```

## 核心导出 API

- `export fn stmStrEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`strum@1.0.0`)，许可证：MIT
