[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# decamelize

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Convert a camelized string into a lowercased one with a custom separator (clean-room CP implementation of npm decamelize)

## 安装与导入

```bash
cap add decamelize
```

```cp
import { decamelizeWithOptions } from "decamelize";
```

## 核心导出 API

- `export fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize;`
- `export fn decamelize(input: string, separator: string, out: []u8): usize;`
- `export fn toSnakeCase(input: string, out: []u8): usize;`
- `export fn toKebabCase(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`decamelize@1.0.0`)，许可证：MIT
