[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unflatten

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation nested JSON flatten & unflatten library (npm flat/unflatten parity) with custom delimiter, max_depth, safe array mode, and round-trip reconstruction for CP

## 安装与导入

```bash
cap add unflatten
```

```cp
import { flattenJson } from "unflatten";
```

## 核心导出 API

- `export fn flattenJson(json_in: string, opts: FlatOptions, out_table: &mut FlatTable): i64;`
- `export fn flattenToJson(json_in: string, opts: FlatOptions, out: []u8): i64;`
- `export fn unflattenJson(flat_json: string, opts: FlatOptions, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`unflatten@1.0.0`)，许可证：MIT
