# `cap:jmespath` — JMESPath JSON 查询与投影转换标准库

## 1. 概述

`cap:jmespath`（同时挂载于 `cap.jmespath`）是纯 CP 实现的工业级 JMESPath JSON 查询与投影转换标准库，提供：
- 完整的强类型 JSON 值模型（`JmesValue`）与零依赖递归下降 JSON 编解码器（`parseJson` / `stringifyJson`），内置 `MAX_DEPTH = 64` 深度防护。
- JMESPath 表达式编译器（`compileJmesPath` -> `JmesExpr`）与求值器（`searchJmesPath` / `searchJmesPathJson`）。
- 支持子表达式、负索引、切片、Flatten 展平 `[]`、List/Object 通配符、`[?expr]` 过滤器、MultiSelect List/Hash、管道 `|`、当前节点 `@`、反引号 JSON 字面量及 13 个核心内置函数。

## 2. 核心类型

- `MAX_DEPTH: usize = 64`：JSON 解析与 JMESPath 表达式编译/求值的最大嵌套深度限制。
- `JmesError`：错误集合，包含 `OutOfMemory`、`InvalidJson`、`MaxDepthExceeded`、`InvalidSyntax`、`UnexpectedToken`、`InvalidSliceStep`、`InvalidFunction`、`InvalidArgumentType`、`InvalidArity`。
- `JmesEntry`：对象键值对 `{ key: []const u8, value: JmesValue }`。
- `JmesValue`：JSON 值联合体（`Null`, `Bool`, `Int(i64)`, `Float(f64)`, `String([]const u8)`, `Array([]JmesValue)`, `Object([]JmesEntry)`），提供 `deinit(alloc)`、`clone(alloc)`、`equals(other)`、`isTruthy()`、`typeName()`、`getField(key)`、`asNumber()`。
- `JmesExpr`：编译后的 JMESPath 表达式 AST 封装，调用 `deinit(alloc)` 释放所有 AST 节点。

## 3. API 速查

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parseJson` | `fn parseJson(alloc, input: []const u8): JmesError!JmesValue` | 解析 JSON 字符串为 `JmesValue` |
| `stringifyJson` | `fn stringifyJson(alloc, val: JmesValue): JmesError![]u8` | 序列化 `JmesValue` 为紧凑 JSON 字符串 |
| `compileJmesPath` | `fn compileJmesPath(alloc, expr: []const u8): JmesError!JmesExpr` | 编译 JMESPath 查询表达式 |
| `searchJmesPath` | `fn searchJmesPath(alloc, expr: *const JmesExpr, data: JmesValue): JmesError!JmesValue` | 在 `JmesValue` 上执行已编译查询 |
| `searchJmesPathJson` | `fn searchJmesPathJson(alloc, expr_str: []const u8, json_str: []const u8): JmesError![]u8` | 一步完成 JSON 解析、查询与结果序列化 |

支持语法与内置函数：
- **路径与投影**：`a.b.c`、`[0]`、`[-1]`、`[start:stop:step]`、`[]`（Flatten）、`[*]`、`*`
- **过滤与组合**：`[?expr]`（`== != < <= > >= && || !`）、`[a, b]`、`{k: expr}`、`|`、`@`、`` `json` ``、`'(raw)'`
- **内置函数**：`length`、`keys`、`values`、`sort`、`reverse`、`contains`、`starts_with`、`ends_with`、`join`、`max`、`min`、`type`、`not_null`

## 4. 示例

```cp
import { searchJmesPathJson } from "cap:jmespath";

const data = "{\"users\":[{\"name\":\"alice\",\"age\":30,\"active\":true},{\"name\":\"bob\",\"age\":17,\"active\":true}]}";
const res = searchJmesPathJson(alloc, "users[?age >= `18` && active].{name: name, age: age} | [0]", data)?;
defer alloc.free(res);
// res => "{\"name\":\"alice\",\"age\":30}"
```
