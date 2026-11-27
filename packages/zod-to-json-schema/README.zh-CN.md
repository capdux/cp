[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zod-to-json-schema

Zero-heap JSON Schema (Draft-07 / 2020-12) builder and serializer for structured AI tool definitions

| 元信息 | 值 |
| --- | --- |
| 包名 | `zod-to-json-schema` |
| 版本 | `3.24.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add zod-to-json-schema
```

```cp
import {
    SchemaType,
    SchemaProp,
    schemaTypeName,
    prop,
    stringProp,
    intProp,
    arrayProp,
    toJsonSchema,
} from "zod-to-json-schema";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `SchemaType` | `String, Number, Integer, Boolean, Array, Object, Null` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SchemaProp` | `class` | 字段: `name: string, kind: SchemaType, description: string, required: bool, format: string, item_kind: SchemaType, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `schemaTypeName` | `(t: SchemaType): string` | 调用 `schemaTypeName` 并返回 `string`。 |
| `prop` | `(name: string, kind: SchemaType, description: string, required: bool): SchemaProp` | 调用 `prop` 并返回 `SchemaProp`。 |
| `stringProp` | `(name: string, description: string, format: string, required: bool): SchemaProp` | 调用 `stringProp` 并返回 `SchemaProp`。 |
| `intProp` | `(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp` | 调用 `intProp` 并返回 `SchemaProp`。 |
| `arrayProp` | `(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp` | 调用 `arrayProp` 并返回 `SchemaProp`。 |
| `toJsonSchema` | `(title: string, props: &[]SchemaProp, additional_properties: bool, out: []u8): usize` | 调用 `toJsonSchema` 并返回 `usize`。 |

## 最小调用示例

```cp
import { prop } from "zod-to-json-schema";

pub fn main() {
    let result = prop("example", /* kind: SchemaType */, "example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
