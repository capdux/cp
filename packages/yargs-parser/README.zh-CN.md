[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# yargs-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CLI argument parser (npm yargs-parser parity) with long/short flags, --no- negation, camelCase/kebab-case key lookup, numeric/boolean coercion, and -- rest passthrough for CP

## 安装与导入

```bash
cap add yargs-parser
```

```cp
import { parseArgs } from "yargs-parser";
```

## 核心导出 API

- `export fn parseArgs(argv: &[]string): ParsedArgs;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`yargs-parser@1.0.0`)，许可证：MIT
