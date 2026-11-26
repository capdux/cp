# @cap/json-schema

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ajv` ([https://github.com/ajv-validator/ajv](https://github.com/ajv-validator/ajv)) / `crates.io:schemars` ([https://github.com/GREsau/schemars](https://github.com/GREsau/schemars))（原库名 `ajv` → CP 包名 `@cap/json-schema`）

Zero-allocation JSON Schema Draft-07 / 2020-12 validator operating directly on raw RFC 8259 JSON strings for CP — clean-room MIT parity with npm `ajv` and Rust `jsonschema`.

零堆分配的 CP JSON Schema 校验引擎，无需构建动态堆 AST，直接对原始 JSON 字符串进行单遍词法与结构校验，支持类型检测（精准区分 `Integer`/`Number`）、字符串 `minLength`/`maxLength`、数值 `minimum`/`maximum` 与开区间、数组 `minItems`/`maxItems`、对象顶层 `required` 必填键及 `enum` 白名单（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/json-schema
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `JsonType` | `pub enum { Any, Null, Boolean, Integer, Number, String, Array, Object }` | JSON 值类型枚举 |
| `SchemaRule` | `pub struct { ... }` | 校验规则结构体，提供 `SchemaRule.any()`、`addRequired(*_, key)`、`addEnum(*_, val)`、`setExclusive(*_, min_excl, max_excl)` |
| `ValidationError` / `ValidationResult` | `pub struct` | 结构化校验错误与结果（`valid`, `error_count`, `first_error`） |
| `detectJsonType` | `(json: string) -> JsonType` | 检测原始 JSON 字符串的精确类型 |
| `validateJson` | `(json: string, rule: SchemaRule) -> ValidationResult` | 零堆校验 JSON 是否满足 `SchemaRule` |
| `stringRule` / `numberRule` / `integerRule` / `arrayRule` / `objectRule` | 便捷规则构造器 | 一行构建常用类型与区间约束规则 |

## Usage / 使用示例

```cp
import { objectRule, stringRule, validateJson } from "@cap/json-schema";

let user_schema = objectRule();
user_schema.addRequired("id");
user_schema.addRequired("email");

const res = validateJson("{\"id\": 1, \"email\": \"a@b.com\"}", user_schema);
// res.valid == true

let role_rule = stringRule(2, 16);
role_rule.addEnum("admin");
role_rule.addEnum("member");
const r2 = validateJson("\"admin\"", role_rule); // valid == true
```

## License

MIT
