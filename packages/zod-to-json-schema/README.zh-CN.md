[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zod-to-json-schema

> 版本：`3.24.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap JSON Schema (Draft-07 / 2020-12) builder and serializer for structured AI tool definitions

## 安装与导入

```bash
cap add zod-to-json-schema
```

```cp
import { schemaTypeName } from "zod-to-json-schema";
```

## 核心导出 API

- `export enum SchemaType`
- `export fn schemaTypeName(t: SchemaType): string;`
- `export fn prop(name: string, kind: SchemaType, description: string, required: bool): SchemaProp;`
- `export fn stringProp(name: string, description: string, format: string, required: bool): SchemaProp;`
- `export fn intProp(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp;`
- `export fn arrayProp(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp;`
- `export fn toJsonSchema(title: string, props: &[]SchemaProp, additional_properties: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`zod-to-json-schema@3.24.1`)（https://github.com/StefanTerdell/zod-to-json-schema），许可证：MIT
