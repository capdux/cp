[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/doctrine

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:doctrine` ([https://github.com/eslint/doctrine](https://github.com/eslint/doctrine))（原库名 `doctrine` → CP 包名 `@cap/doctrine`）

Zero-allocation JSDoc comment parser for CP — clean-room Apache-2.0 implementation inspired by ESLint `doctrine`.

纯 CP 零堆分配 JSDoc 注释解析器（对标 ESLint `doctrine`），自动剥离 `/** ... */` 与每行前导 `*`，提取顶部描述及 `@param {Type} [name] - desc`、`@returns {Type} desc`、`@type {Type}`、`@deprecated`、`@throws`、`@template` 等标准标签。

## Install / 安装

```bash
cap add @cap/doctrine
```

## API

### Structs & Functions / 核心类型与函数

| Symbol | Description |
| --- | --- |
| `JsDocTag` | `struct { title: string, type_expr: string, name: string, description: string, optional: bool }` |
| `JsDocComment` | `struct { description: string, tags: [16]JsDocTag, tag_count: usize }` |
| `parseJsDoc` | `(comment: string): JsDocComment` |

### `JsDocComment` Methods / 实例方法

| Method | Signature | Description |
| --- | --- | --- |
| `tagCount` | `(): usize` | Return number of parsed JSDoc tags |
| `getTag` | `(idx: usize): JsDocTag?` | Get tag by 0-based index |
| `findTag` | `(title: string): JsDocTag?` | Find first tag matching `title` (e.g. `"param"`, `"returns"`) |
| `isDeprecated` | `(): bool` | Check whether `@deprecated` tag is present |

## Usage / 使用示例

```cp
import { parseJsDoc } from "@cap/doctrine";

const doc = parseJsDoc("/**\n * Greet a user.\n * @param {string} [name] - User name\n * @returns {string} Greeting\n */");
const n = doc.tagCount(); // 2
const ret = doc.findTag("returns"); // type_expr == "string"
```
