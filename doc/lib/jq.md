# cap:jq (`lib/jq/`) — 纯 CP JSON 结构化查询与管道转换引擎

## 1. 概述

`cap:jq`（支持 `import * as jq from "cap:jq"` 与 `import cap from "cap"` 下的 `cap.jq` 动态挂载）是纯 CP 实现的工业级 `jq` 风格 JSON 结构化查询、流式展开与管道转换标准库，无外部 C/Zig 依赖。

核心特性：
- **独立 RFC 8259 JSON Value 模型与编解码**：区分 `Int(i64)` 与 `Float(f64)`，支持 UTF-16 代理对解码、`max_depth = 64` 嵌套防护及完整内存安全生命周期管理。
- **Filter AST 编译器与多结果流管道**：支持字段访问、负索引、切片、`.[]` 流式展开、`|` 管道、`,` 多输出、`[...]`/`{...}` 构造、逻辑与比较运算、替代运算符 `//` 及 13 种核心内置函数。
- **确定性内存与 DoS 防护**：全链路支持 `std.testing.allocator` 零泄漏验证，内置 `MAX_AST_DEPTH = 64`、`MAX_JQ_EVAL_DEPTH = 128` 与 `MAX_JQ_OUTPUTS = 65536` 上限拦截。

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `JqError` | 错误集合：`InvalidJson`, `UnexpectedToken`, `UnexpectedEof`, `MaxDepthExceeded`, `TypeError`, `DivisionByZero`, `IndexOutOfBounds`, `InvalidSyntax`, `OutputLimitExceeded`, `OutOfMemory` |
| `JqTag` | JSON 值标签枚举：`Null`, `Bool`, `Int`, `Float`, `String`, `Array`, `Object` |
| `JqObjEntry` | JSON 对象键值对 `{ key: []const u8, value: JqValue }` |
| `JqValue` | JSON 值结构体，提供 `nullVal()`, `boolVal()`, `intVal()`, `floatVal()`, `stringVal()`, `arrayVal()`, `objectVal()`, `typeName()`, `isTruthy()`, `asNumber()`, `equals()`, `compare()`, `clone()`, `deinit()` |
| `JqExpr` | 编译后的扁平 AST 表达式对象，使用 `expr.deinit(alloc)` 统一释放 |

## 3. API 速查

### JSON 编解码
- `parseJson(alloc, input: []const u8): JqError!JqValue`：解析 RFC 8259 JSON 字符串。
- `stringifyJson(alloc, val: JqValue): JqError![]u8`：序列化为紧凑 JSON 字符串。
- `freeValues(alloc, values: *[]JqValue)`：释放 `runJq` 返回的结果切片。

### Filter 编译与执行
- `compileJq(alloc, filter: []const u8): JqError!JqExpr`：编译 jq 过滤表达式为 AST。
- `runJq(alloc, expr: *const JqExpr, input: JqValue): JqError![]JqValue`：对 `JqValue` 执行查询，返回多输出流切片。
- `runJqJson(alloc, filter: []const u8, json_input: []const u8): JqError![]u8`：一步完成解析、编译、求值与序列化（多结果按 `\n` 分隔，空流返回 `""`）。

### 支持的语法与内置函数
- **路径与投影**：`.`, `.foo`, `.foo.bar`, `."foo"`, `.foo?`, `.[0]`, `.[-1]`, `.[1:3]`, `.[:2]`, `.[1:]`, `.[]`, `.[]?`
- **流与构造**：`|`, `,`, `[expr]`, `{k: expr, shorthand, (key_expr): expr}`
- **运算符**：`==`, `!=`, `<`, `<=`, `>`, `>=`, `and`, `or`, `not`, `//`, `+`, `-`, `*`, `/`
- **内置函数**：`select(cond)`, `map(expr)`, `length`, `keys`, `values`, `has(key)`, `contains(val)`, `empty`, `type`, `reverse`, `unique`, `sort`

## 4. 示例

```cp
import * as jq from "cap:jq";
import cap from "cap";
const std = @import("std");

pub fn example(alloc: std.mem.Allocator): !void {
    const data = "[{\"name\":\"alice\",\"score\":92},{\"name\":\"bob\",\"score\":75}]";
    const out = try jq.runJqJson(alloc, "[.[] | select(.score >= 80) | {name, score}]", data);
    defer alloc.free(out);
    // out == "[{\"name\":\"alice\",\"score\":92}]"

    const out2 = try cap.jq.runJqJson(alloc, ".missing? // \"fallback\"", "{\"a\":1}");
    defer alloc.free(out2);
    // out2 == "\"fallback\""
}
```
