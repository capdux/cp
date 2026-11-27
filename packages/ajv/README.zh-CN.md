[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ajv

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap JSON Schema Draft-07 / 2020-12 validation engine for CP (clean-room MIT implementation of npm ajv v8).

## 安装与导入

```bash
cap add ajv
```

```cp
import { validateFormat } from "ajv";
```

## 核心导出 API

- `export fn validateFormat(fmt: string, value: string): bool;`
- `export fn validateWithSchema(schema_json: string, data_json: string): AjvResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ajv@1.0.0`)，许可证：MIT
