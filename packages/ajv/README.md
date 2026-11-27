[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`ajv`](https://github.com/ajv-validator/ajv)（`MIT`）（原库名 `ajv`，CP 包名 `@cap/ajv-schema`）。

# @cap/ajv-schema

零堆分配的 CP 原生 JSON Schema Draft-07 / 2020-12 校验引擎，Clean-Room MIT 实现对标 npm Top 16 顶流包 `ajv` v8（2 亿+/周下载）。

跨包零重复复用 `@cap/zod-schema`（`zEmail` / `zUrl` / `zUuid` / `zEnum`）与 `@cap/ipaddr`（`isValidIpv4` / `isValidIpv6`），直接解析标准 JSON Schema 字符串并递归校验目标 JSON 实例。

## 支持的关键字

- **类型与常量**：`type`（含联合数组类型如 `["string","null"]`）、`enum`、`const`
- **对象约束**：`required`、`properties`（支持多层嵌套）、`additionalProperties: false`
- **数值约束**：`minimum`、`maximum`、`exclusiveMinimum`、`exclusiveMaximum`
- **字符串约束**：`minLength`、`maxLength`、`format`（`email` / `uri` / `uuid` / `ipv4` / `ipv6` / `date`）
- **数组约束**：`minItems`、`maxItems`、`items`

## 核心 API

- `AjvSchema.compile(schema_json)` / `schema.validate(data_json)`
- `validateWithSchema(schema_json, data_json): AjvResult`
- `validateFormat(fmt, value): bool`
- `AjvResult`（`valid`, `error_count`, `errors: [16]AjvError`）

## 依赖复用

- `@cap/zod-schema`: `1.0.0`
- `@cap/ipaddr`: `1.0.0`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`ajv-validator/ajv`](https://github.com/ajv-validator/ajv)。
