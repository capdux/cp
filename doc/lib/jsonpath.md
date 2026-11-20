# `cap:jsonpath` (`cap.jsonpath`) — RFC 9535 JSONPath 标准库

## 1. 概述

`lib/jsonpath/`（导入协议 `cap:jsonpath` 或动态挂载 `cap.jsonpath`）提供符合 RFC 9535 规范的纯 CP JSONPath 查询编译、求值与规范路径（Normalized Path）提取能力。内置轻量独立 `JsonVal` DOM 与 JSON 编解码器，支持成员选择、负索引、正负步长切片、通配符、并集、递归下降、过滤表达式（含内置函数 `length`/`count`/`value`）以及递归深度与最大输出节点数防 DoS 限制。

## 2. 核心类型

- `JsonVal` / `JsonKind` / `JsonMember`：轻量 JSON 值节点，支持 `Null`、`Bool`、`Number`、`String`、`Array`、`Object`。
- `JsonPathError`：编译期错误枚举（`EmptyQuery`、`MissingRootIdentifier`、`InvalidSyntax`、`UnterminatedBracket`、`UnterminatedString`、`InvalidEscape`、`ZeroSliceStep`、`InvalidFilter`、`UnknownFunction`）。
- `JsonPathQuery`：已编译的 RFC 9535 AST 查询对象，需通过 `deinitJsonPathQuery` 释放。
- `QueryLimits` / `QueryError`：求值安全边界（默认 `max_depth = 64`、`max_nodes = 4096`），超限触发 `MaxDepthExceeded` 或 `MaxNodesExceeded`。
- `NodeMatch`：匹配结果结构体，包含 `value: *const JsonVal` 与 RFC 9535 规范路径 `path: []const u8`（如 `$['store']['book'][0]['title']`）。

## 3. API 速查

| 函数 | 签名摘要 | 说明 |
| --- | --- | --- |
| `parseJson` | `(alloc, src) -> JsonParseError!JsonVal` | 解析 JSON 字符串为 `JsonVal` |
| `deinitJsonVal` | `(val, alloc)` | 释放 `JsonVal` 树内存 |
| `stringifyJson` | `(alloc, val) -> ![]const u8` | 序列化 `JsonVal` 为 JSON 字符串 |
| `compileJsonPath` | `(alloc, expr) -> JsonPathError!JsonPathQuery` | 编译 RFC 9535 JSONPath 表达式 |
| `deinitJsonPathQuery` | `(q, alloc)` | 释放 `JsonPathQuery` AST |
| `queryJsonPath` | `(alloc, root, query) -> QueryError![]NodeMatch` | 使用默认上限执行查询并返回值与规范路径 |
| `queryJsonPathWithLimits` | `(alloc, root, query, limits) -> QueryError![]NodeMatch` | 使用自定义深度/节点数上限执行查询 |
| `deinitMatches` | `(matches, alloc)` | 释放 `[]NodeMatch` 及规范路径字符串 |
| `queryJsonPathValues` | `(alloc, root, query) -> QueryError![]*const JsonVal` | 仅返回匹配值指针切片 |
| `queryJsonPathStrings` | `(alloc, root, query) -> ![][]const u8` | 返回匹配节点的 JSON 字符串切片（用 `deinitStrings` 释放） |

## 4. 示例

```cp
import { parseJson, deinitJsonVal, compileJsonPath, deinitJsonPathQuery, queryJsonPath, deinitMatches } from "cap:jsonpath";

let doc = try parseJson(alloc, "{\"store\":{\"book\":[{\"title\":\"CP\",\"price\":8.5}]}}");
defer deinitJsonVal(&doc, alloc);

let q = try compileJsonPath(alloc, "$.store.book[?@.price < 10].title");
defer deinitJsonPathQuery(&q, alloc);

let matches = try queryJsonPath(alloc, &doc, &q);
defer deinitMatches(matches, alloc);
// matches[0].path == "$['store']['book'][0]['title']"
// matches[0].value.str_val == "CP"
```
