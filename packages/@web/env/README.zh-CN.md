[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/env

Official startup environment schema validation & App state injection plugin for cap:web (inspired by @fastify/env)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/env` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/env
```

```cp
import {
    EnvKind,
    EnvVarSpec,
    EnvKV,
    EnvValidationResult,
    envStrEql,
    envParseI64,
    envIsValidBool,
    envInEnum,
} from "@web/env";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `EnvKind` | `Str, Int, Bool, Enum` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EnvVarSpec` | `class` | 字段: `name: string, kind: EnvKind, required: bool, default_val: string, min_val: i64, max_val: i64, +2 项` |
| `EnvKV` | `class` | 字段: `key: string, value: string` |
| `EnvValidationResult` | `class` | 字段: `ok: bool, missing_count: usize, invalid_count: usize, loaded_count: usize, first_error_key: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `envStrEql` | `(a: string, b: string): bool` | 调用 `envStrEql` 并返回 `bool`。 |
| `envParseI64` | `(s: string): i64?` | 调用 `envParseI64` 并返回 `i64?`。 |
| `envIsValidBool` | `(s: string): bool` | 调用 `envIsValidBool` 并返回 `bool`。 |
| `envInEnum` | `(val: string, list: string): bool` | 调用 `envInEnum` 并返回 `bool`。 |
| `envLookupRaw` | `(entries: &[]EnvKV, key: string): string?` | 调用 `envLookupRaw` 并返回 `string?`。 |
| `registerEnv` | `(app: &mut App, specs: &[]EnvVarSpec, entries: &[]EnvKV): EnvValidationResult` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { envStrEql } from "@web/env";

pub fn main() {
    let result = envStrEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
