# CP 标准库：JSON Schema 运行时校验引擎手册 (`cap:schema`)

## 一、 架构总览与设计准则

`cap:schema` 是 CP 语言官方内置的纯 CP JSON Schema Draft 2020-12 运行时校验与 AI Agent Tool Calling 参数验证引擎，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，基于 `cap:json` 零拷贝 `JsonValue` 与 `JsonParser` 构建。既支持 `import { ... } from "cap:schema"` 显式导入，也支持 `import cap from "cap"; cap.schema.*` 自动动态挂载。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    AI Tool Calling & API Payload 校验 (cap:schema)       │
├─────────────────────────────────┬───────────────────────────────────────┤
│ Schema AST 与流式构造/解析       │ 运行时校验与 RFC 6901 Pointer 诊断     │
│ SchemaType / StringFormat       │ SchemaValidator.validate              │
│ PropertySchema / Schema         │ SchemaValidator.validateJson          │
│ Schema.fromJson / fromJsonValue │ ValidationResult / ValidationIssue    │
└─────────────────────────────────┴───────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/schema/schema.cp` | `// Source: cp/lib/schema/schema.cp (JSON Schema Draft 2020-12 / AI Tool Parameter Schema AST)` | `SchemaType`、`StringFormat`、`PropertySchema`、`Schema` 流式构造器与 `Schema.fromJson` 解析器 |
| `lib/schema/validator.cp` | `// Source: cp/lib/schema/validator.cp (JSON Schema Runtime Validator with JSON Pointer Diagnostics)` | `ValidationIssue`、`ValidationResult`、`SchemaValidator`（类型/边界/格式/枚举/数组/对象与 64 层深度防护） |
| `lib/schema/index.cp` | `// Source: cp/lib/schema/index.cp (Unified Exports for cap:schema)` | `cap:schema` 与 `cap.schema` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. Schema AST 与流式构造 (`schema.cp`)

- **`SchemaType`**：`Any`、`Null`、`Bool`、`Integer`、`Number`、`String`、`Array`、`Object`。
- **`StringFormat`**：`None`、`Email`、`Uri`、`Uuid`、`Ipv4`、`DateTime`（RFC 3339）。
- **`PropertySchema`**：`{ name, schema, required }`，提供 `PropertySchema.init(name, schema, required)`、`PropertySchema.req(name, schema)`、`PropertySchema.opt(name, schema)`。
- **`Schema` 流式工厂与修饰方法**：
  - 工厂：`Schema.any()`、`Schema.nullType()`、`Schema.boolean()`、`Schema.integer()`、`Schema.number()`、`Schema.string()`、`Schema.array(&item_schema)`、`Schema.object(props)`、`Schema.enumStrings(vals)`；
  - 链式约束：`minLen(n)`、`maxLen(n)`、`withFormat(fmt)`、`minVal(v)`、`maxVal(v)`、`exclusiveMin(v)`、`exclusiveMax(v)`、`multipleOf(v)`、`minItems(n)`、`maxItems(n)`、`withUniqueItems(bool)`、`withAdditionalProperties(bool)`、`withNullable(bool)`、`withEnumStrings(vals)`、`withTitle(t)`、`withDescription(d)`；
  - 标准 JSON Schema 解析：`Schema.fromJson(alloc, json_str)` 与 `Schema.fromJsonValue(alloc, root)`，支持 OpenAI / MCP Tool Calling 规范（含 `"type": ["string", "null"]`、`"nullable"`、`"required"`、`"additionalProperties": false`、`"enum"`、`"const"`、`"format"`、`"items"`、`"properties"` 及 64 层最大解析深度保护），使用后调用 `schema.deinit()` 释放内部 Arena 内存。

### 2. 运行时校验与 RFC 6901 JSON Pointer 诊断 (`validator.cp`)

- **`ValidationIssue`**：
  - `path`：严格 RFC 6901 JSON Pointer（如 `""`、`"/user/email"`、`"/items/0/price"`，自动转义 `~` -> `~0`、`/` -> `~1`）；
  - `code`：机器可读错误码（`type_mismatch`、`required`、`additional_property`、`min_length`、`max_length`、`format`、`minimum`、`maximum`、`exclusive_minimum`、`exclusive_maximum`、`multiple_of`、`min_items`、`max_items`、`unique_items`、`enum`、`max_depth`）；
  - `message`：人类与 AI 智能体可读的诊断说明。
- **`ValidationResult`**：包含 `ok: bool` 与 `issues: []ValidationIssue`，提供 `count()`、`first()` 与 `deinit()`（合法快路径零堆分配）。
- **`SchemaValidator`**：
  - `SchemaValidator.validate(alloc, schema, value: JsonValue)`；
  - `SchemaValidator.validateJson(alloc, schema, json_str)`；
  - 内置 5 种高性能纯 CP 格式校验器：`isValidEmail`、`isValidUri`、`isValidUuid`、`isValidIpv4`、`isValidDateTime`；
  - 强制 `MAX_DEPTH = 64` 递归深度保护，防止畸形嵌套报文导致栈溢出。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Schema, SchemaValidator, StringFormat } from "cap:schema";
import cap from "cap";

pub fn demoToolCallValidation(): ! {
    const alloc = std.heap.page_allocator;
    const tool_schema_json =
        \\{"type":"object","required":["to","priority"],"additionalProperties":false,"properties":{"to":{"type":"string","format":"email"},"priority":{"type":"integer","minimum":1,"maximum":5}}}
    ;

    let schema = Schema.fromJson(alloc, tool_schema_json)?;
    defer schema.deinit();

    let res = SchemaValidator.validateJson(alloc, schema, "{\"to\":\"agent@cp-lang.org\",\"priority\":3}")?;
    defer res.deinit();
    // res.ok == true

    // 动态命名空间 cap.schema
    let s = cap.schema.Schema.string().withFormat(.Uuid);
    let r = cap.schema.SchemaValidator.validateJson(alloc, s, "\"550e8400-e29b-41d4-a716-446655440000\"")?;
    defer r.deinit();
}
```
