[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# enquirer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP interactive CLI prompt state machines for Confirm, Select, and MultiSelect (clean-room MIT implementation of npm enquirer v2)

## 安装与导入

```bash
cap add enquirer
```

```cp
import { enqToLower } from "enquirer";
```

## 核心导出 API

- `export fn enqToLower(b: u8): u8;`
- `export fn enqIsSpace(b: u8): bool;`
- `export fn enqTrim(s: string): string;`
- `export fn enqEqIgnoreCase(a: string, b: string): bool;`
- `export fn enqWriteStr(out: []u8, pos: usize, s: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`enquirer@1.0.0`)，许可证：MIT
