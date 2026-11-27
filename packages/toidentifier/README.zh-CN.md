[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# toidentifier

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Convert a string of words to a PascalCase/camelCase identifier (clean-room CP implementation of npm toidentifier)

## 安装与导入

```bash
cap add toidentifier
```

```cp
import { tidIsIdentChar } from "toidentifier";
```

## 核心导出 API

- `export fn tidIsIdentChar(c: u8): bool;`
- `export fn toIdentifier(input: string, out: []u8): usize;`
- `export fn toCamelIdentifier(input: string, out: []u8): usize;`
- `export fn toConstIdentifier(input: string, out: []u8): usize;`
- `export fn isValidIdentifier(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`toidentifier@1.0.0`)，许可证：MIT
