# CP 标准库：JSON Schema Draft-07/2020-12 校验引擎 (`cap:jsonschema`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:conf`，导入请使用 `import { ... } from "cap:conf"`。

## 一、 架构总览与设计准则

`cap:jsonschema` 是 CP 语言官方内置的纯 CP **JSON Schema (Draft-07 / 2020-12) 校验与路径诊断引擎**，提供 Schema 预编译、强类型与结构约束校验、组合逻辑求值、本地 `$ref` 指针解析（带深度防护）以及精确到 JSONPath 的错误诊断报告。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:conf"`，也支持 `import cap from "cap"; cap.jsonschema.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                CP JSON Schema 校验与路径诊断引擎 (cap:jsonschema)         │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ Schema 预编译与隔离   │ 全量关键字约束校验       │ 本地 $ref 与路径诊断   │
│ SchemaValidator       │ type / 数值 / UTF-8长度 │ #/$defs/... 指针解析   │
│ compile / deinit      │ array / object / enum   │ max_depth = 64 防循环  │
│ Arena 零泄漏管理      │ allOf/anyOf/oneOf/not   │ $.user.age 精确路径    │
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/jsonschema/jsonschema.cp` | `// Source: cp/lib/jsonschema/jsonschema.cp (...)` | `JsonSchemaError`、`ValidationIssue`、`ValidationResult` 与 `SchemaValidator` 全量校验引擎 |
| `lib/jsonschema/index.cp` | `// Source: cp/lib/jsonschema/index.cp (...)` | `cap:jsonschema` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与诊断类型

- **`JSONSCHEMA_MAX_DEPTH`**：默认最大递归深度上限（`64`），用于防止循环 `$ref` 或极深嵌套导致栈溢出。
- **`JsonSchemaError`**：
  - `InvalidSchema`：Schema 本身非合法 JSON，或关键字类型不符合 JSON Schema 规范（如 `type` 非法、`multipleOf <= 0`）。
  - `InvalidJson`：待校验实例文本不是合法 JSON。
  - `InvalidReference`：`$ref` 非本地 `#/...` 引用或目标 JSON Pointer 不存在。
  - `MaxDepthExceeded`：递归校验或 `$ref` 展开超过 `max_depth`（默认 `64`）。
  - `OutOfMemory`：内存分配失败。
- **`ValidationIssue`**：
  - `path: []const u8`：精确 JSONPath 错误路径（如 `"$"`, `"$.user.age"`, `"$.items[2]"`）。
  - `keyword: []const u8`：触发失败的约束关键字（如 `"minimum"`, `"required"`, `"additionalProperties"`）。
  - `message: []const u8`：人类与 AI 友好的错误原因描述。
- **`ValidationResult`**：
  - `valid: bool`、`issues: []ValidationIssue`。
  - `deinit(*_, allocator)`：释放所有 `ValidationIssue` 字符串及切片内存。

### 2. `SchemaValidator` 核心方法与支持关键字

- **`SchemaValidator.compile(allocator, schema_json): SchemaValidator!`** / **`compileWithMaxDepth(allocator, schema_json, max_depth)`**：
  - 解析并校验 Schema 语法，内部使用独立 `ArenaAllocator` 托管 Schema AST，`deinit()` 一键全量释放。
- **`validate(allocator, instance_json): ValidationResult!`** / **`validateValue(allocator, value: JsonValue): ValidationResult!`**。
- **支持关键字矩阵**：
  - **类型校验**：`type`（单类型或类型数组如 `["string", "null"]`，严格区分 `integer` 与 `number`，无小数部分的有限数字判定为 `integer`）。
  - **数值约束**：`minimum`、`maximum`、`exclusiveMinimum`、`exclusiveMaximum`、`multipleOf`。
  - **字符串约束**：`minLength`、`maxLength`（**按 UTF-8 码点计数**，而非字节长度，符合 JSON Schema 规范）。
  - **数组约束**：`items`（单 Schema 或元组 Schema 数组）、`minItems`、`maxItems`、`uniqueItems`。
  - **对象约束**：`properties`、`required`、`additionalProperties`（`false` 或子 Schema）、`minProperties`、`maxProperties`。
  - **枚举与组合**：`enum`、`const`、`allOf`、`anyOf`、`oneOf`、`not`。
  - **本地引用**：`$ref`（支持 `#`、`#/$defs/...`、`#/definitions/...` 等本地 RFC 6901 JSON Pointer），带 `max_depth = 64` 循环防护。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SchemaValidator } from "cap:conf";
import cap from "cap";

pub fn demoJsonSchema()! {
    const alloc = std.heap.page_allocator;
    const schema_src =
        \\{"type":"object","required":["age"],"properties":{"age":{"type":"integer","minimum":18}}}
    ;

    let validator = SchemaValidator.compile(alloc, schema_src)?;
    defer validator.deinit();

    let res = validator.validate(alloc, "{\"age\":15}")?;
    defer res.deinit(alloc);
    // res.valid == false, res.issues[0].path == "$.age", res.issues[0].keyword == "minimum"

    let v2 = cap.jsonschema.SchemaValidator.compile(alloc, schema_src)?;
    defer v2.deinit();
}
```
