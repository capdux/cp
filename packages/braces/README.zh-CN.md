[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# braces

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Bash/Glob brace expansion and sequence range generator (braces + fill-range parity) for CP

## 安装与导入

```bash
cap add braces
```

```cp
import { expandBraces } from "braces";
```

## 核心导出 API

- `export fn expandBraces(pattern: string, out: []u8): BraceResult;`
- `export fn fillRange(start: string, end: string, step: i32, out: []u8): BraceResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`braces@1.0.0`)，许可证：MIT
