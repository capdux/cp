[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ajv

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:ajv`](https://github.com/ajv-validator/ajv)（MIT，v8 语义）| CP 包版本：`1.0.0` | 依赖：`zod`、`ipaddr.js`

零堆分配的 JSON Schema Draft-07 / 2020-12 校验引擎。完整支持类型、`required`、`properties`、`additionalProperties`、数值边界（`minimum`/`maximum`/`exclusiveMinimum`/`exclusiveMaximum`）、字符串长度（`minLength`/`maxLength`）、数组约束（`minItems`/`maxItems`/`items`）、`enum`、`const` 以及 `format`（`email`、`uri`/`url`、`uuid`、`ipv4`、`ipv6`、`date`），并输出结构化 JSON Pointer 错误列表。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"ajv"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:ajv`) | CP (`ajv`) |
| :--- | :--- |
| `import Ajv from "ajv";` | `import { AjvSchema, validateWithSchema } from "ajv";` |
| `const validate = ajv.compile(schema);` | `const schema = AjvSchema.compile(schema_json);` |
| `const ok = validate(data);` | `const res = schema.validate(data_json); // res.valid` |
| `validate.errors` | `res.errors[0..res.error_count]`（`pathStr()`、`keywordStr()`、`code`） |

## 核心 API

### 错误码与容量常量

- `AJV_ERR_INVALID_SCHEMA = 1`、`AJV_ERR_INVALID_DATA = 2`、`AJV_ERR_TYPE = 3`、`AJV_ERR_REQUIRED = 4`、`AJV_ERR_ADDITIONAL_PROPERTIES = 5`
- `AJV_ERR_MINIMUM = 6`、`AJV_ERR_MAXIMUM = 7`、`AJV_ERR_EXCLUSIVE_MINIMUM = 8`、`AJV_ERR_EXCLUSIVE_MAXIMUM = 9`
- `AJV_ERR_MIN_LENGTH = 10`、`AJV_ERR_MAX_LENGTH = 11`、`AJV_ERR_MIN_ITEMS = 12`、`AJV_ERR_MAX_ITEMS = 13`
- `AJV_ERR_ENUM = 14`、`AJV_ERR_CONST = 15`、`AJV_ERR_FORMAT = 16`
- `AJV_MAX_ERRORS: usize = 16`

### 类型与函数

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `AjvError { instance_path: [64]u8, keyword: [32]u8, code: i32 }` | 单条校验错误，提供 `pathStr(): &[]u8` 与 `keywordStr(): &[]u8`。 |
| `AjvResult { valid: bool, error_count: usize, errors: [16]AjvError }` | 校验结果，提供 `hasError(path, keyword, code)` 与 `hasCode(code)`。 |
| `AjvSchema.compile(schema_json: string): AjvSchema` | 校验并封装 JSON Schema（合法时 `schema.ok == true`）。 |
| `schema.validate(data_json: string): AjvResult` | 使用已编译 Schema 校验 JSON 数据。 |
| `validateWithSchema(schema_json: string, data_json: string): AjvResult` | 一步完成编译与校验。 |
| `validateFormat(fmt: string, value: string): bool` | 独立格式校验（`email`/`uri`/`url`/`uuid`/`ipv4`/`ipv6`/`date`）。 |

## 实战示例

### 编译并校验 JSON Schema

```cp
import { AjvSchema, AJV_ERR_REQUIRED, AJV_ERR_FORMAT } from "ajv";

fn demoValidate() {
    const schema_json = "{\"type\":\"object\",\"required\":[\"email\",\"age\"],\"properties\":{\"email\":{\"type\":\"string\",\"format\":\"email\"},\"age\":{\"type\":\"integer\",\"minimum\":18}},\"additionalProperties\":false}";
    const schema = AjvSchema.compile(schema_json);

    const ok_res = schema.validate("{\"email\":\"dev@cp.lang\",\"age\":25}");
    // ok_res.valid == true, ok_res.error_count == 0

    const bad_res = schema.validate("{\"email\":\"not-an-email\"}");
    // bad_res.valid == false
    // bad_res.hasCode(AJV_ERR_REQUIRED) == true
    // bad_res.hasCode(AJV_ERR_FORMAT) == true
    * = ok_res; * = bad_res;
}
```

### 一步校验与独立格式验证

```cp
import { validateWithSchema, validateFormat } from "ajv";

fn demoFormatAndOneShot() {
    const res = validateWithSchema("{\"type\":\"array\",\"minItems\":1,\"items\":{\"type\":\"string\",\"enum\":[\"a\",\"b\"]}}", "[\"a\",\"b\"]");
    // res.valid == true

    const is_ip = validateFormat("ipv6", "2001:db8::1"); // true
    const is_dt = validateFormat("date", "2026-11-27");  // true
    * = res; * = is_ip; * = is_dt;
}
```

## 性能与安全亮点

- **零堆分配**：直接在原始 JSON 切片上流式扫描，不构建堆 DOM；`AjvResult` 在栈上内联最多 16 条 `AjvError`（`undefined` 零清零）。
- **跨包生态复用**：`format` 校验直接复用 `zod`（`zEmail`/`zUrl`/`zUuid`/`zEnum`）与 `ipaddr.js`（`isValidIpv4`/`isValidIpv6`）。
- **异常嵌套与畸形报文防护**：内置严格 JSON 语法扫描与递归深度上限，防止深层嵌套导致栈耗尽。

## 许可协议

MIT
