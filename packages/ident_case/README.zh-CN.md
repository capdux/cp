[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ident_case

面向 CP 语言的零堆分配标识符命名风格转换引擎，支持 `lowercase`、`UPPERCASE`、`camelCase`、`PascalCase`、`snake_case`、`SCREAMING_SNAKE_CASE`、`kebab-case` 与 `SCREAMING-KEBAB-CASE`，内置智能缩写词切分。

对齐 Rust crates.io 源包 [`ident_case`](https://github.com/TedDriggs/ident_case)（MIT）。

## 安装与导入

```bash
cap add ident_case
```

```cp
import {
    RenameRule,
    toLowerCase,
    toUpperCase,
    toCamelCase,
    toPascalCase,
    toSnakeCase,
    toKebabCase,
    toScreamingSnakeCase,
    toScreamingKebabCase,
    renameIdent,
    parseRenameRule,
} from "ident_case";
```

## 核心 API

### 枚举类型

| 符号 | 枚举值 | 说明 |
| --- | --- | --- |
| `RenameRule` | `LowerCase`, `UpperCase`, `CamelCase`, `PascalCase`, `SnakeCase`, `ScreamingSnakeCase`, `KebabCase`, `ScreamingKebabCase` | 兼容 Serde 风格的标识符重命名规则枚举。 |

### 函数列表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `toLowerCase` | `(input: string, out: []u8): usize` | 将 `input` 转为全小写写入 `out`，返回写入字节数。 |
| `toUpperCase` | `(input: string, out: []u8): usize` | 将 `input` 转为全大写写入 `out`，返回写入字节数。 |
| `toCamelCase` | `(input: string, out: []u8): usize` | 转为小驼峰 `camelCase`（智能识别缩写，如 `HTTPResponse` -> `httpResponse`）。 |
| `toPascalCase` | `(input: string, out: []u8): usize` | 转为大驼峰 `PascalCase`。 |
| `toSnakeCase` | `(input: string, out: []u8): usize` | 转为下划线 `snake_case`。 |
| `toKebabCase` | `(input: string, out: []u8): usize` | 转为短横线 `kebab-case`。 |
| `toScreamingSnakeCase` | `(input: string, out: []u8): usize` | 转为全大写下划线 `SCREAMING_SNAKE_CASE`。 |
| `toScreamingKebabCase` | `(input: string, out: []u8): usize` | 转为全大写短横线 `SCREAMING-KEBAB-CASE`。 |
| `renameIdent` | `(rule: RenameRule, input: string, out: []u8): usize` | 按 `RenameRule` 动态转换标识符并写入 `out`。 |
| `parseRenameRule` | `(name: string): RenameRule?` | 解析 Serde 风格规则名（如 `"camelCase"`、`"snake_case"` 等），无效时返回 `null`。 |

## 使用示例

### 1. 智能缩写切分与多风格转换

```cp
import { print } from "cap:io";
import { toSnakeCase, toCamelCase, toKebabCase } from "ident_case";

pub fn main() {
    let out: [64]u8 = undefined;
    let n1 = toSnakeCase("HTTPResponseCode", out[0..64]);
    print("snake: {}\n", out[0..n1]); // "http_response_code"

    let n2 = toCamelCase("user_profile_id", out[0..64]);
    print("camel: {}\n", out[0..n2]); // "userProfileId"

    let n3 = toKebabCase("getJSONData", out[0..64]);
    print("kebab: {}\n", out[0..n3]); // "get-json-data"
}
```

### 2. 动态规则解析与分发（Serde 风格）

```cp
import { print } from "cap:io";
import { parseRenameRule, renameIdent } from "ident_case";

pub fn main() {
    let rule = parseRenameRule("SCREAMING-KEBAB-CASE");
    if (rule != null) {
        let out: [64]u8 = undefined;
        let n = renameIdent(rule!, "maxRetryCount", out[0..64]);
        print("renamed: {}\n", out[0..n]); // "MAX-RETRY-COUNT"
    }
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：所有转换直接写入调用方传入的 `out: []u8` 缓冲区并返回写入字节数 `usize`，不分配任何堆字符串。
- **内置 `RenameRule` 统一调度**：额外提供 `RenameRule`、`parseRenameRule` 与 `renameIdent`，便于序列化与代码生成场景按配置动态重命名。
- **智能连续大写识别**：自动处理 `HTTPServer` -> `http_server`、`getJSONData` -> `get-json-data` 等缩写词边界。
