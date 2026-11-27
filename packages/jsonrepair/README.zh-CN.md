[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# jsonrepair

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation malformed & LLM-generated JSON repair engine (jsonrepair / dirty-json / Rust llm_json parity: strips markdown fences, fixes single quotes, unquoted keys, trailing commas, Python True/False/None & truncated brackets) for CP

## 安装与导入

```bash
cap add jsonrepair
```

```cp
import { repairJson } from "jsonrepair";
```

## 核心导出 API

- `export fn repairJson(raw: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`jsonrepair@1.0.0`)，许可证：ISC
