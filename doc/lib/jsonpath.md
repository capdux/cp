# CP 标准库：RFC 9535 JSONPath 查询引擎手册 (`cap:jsonpath`)

## 一、 架构总览与设计准则

`cap:jsonpath` 是 CP 语言官方内置的纯 CP、零 C 依赖的 **RFC 9535 JSONPath 完整查询引擎**（构建在 `cap:json` 的零拷贝 `JsonValue` 之上），专为 API 响应提取、AI Agent Tool 结构化抽取与复杂配置文件筛选设计。既可通过 `import { JsonPath, JsonPathError, PathMatch } from "cap:jsonpath"` 静态导入，也支持 `import cap from "cap"; cap.jsonpath.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/jsonpath/parser.cp` | `// Source: cp/lib/jsonpath/parser.cp (RFC 9535 JSONPath Compiler & AST)` | `JsonPathError`、词法/语法解析器、切片步长校验与 32 层深度防爆 |
| `lib/jsonpath/query.cp` | `// Source: cp/lib/jsonpath/query.cp (RFC 9535 JSONPath Evaluation Engine)` | `PathMatch`、`JsonPath` 编译查询引擎、RFC 9535 Normalized Path 生成与 `evalJson` |
| `lib/jsonpath/index.cp` | `// Source: cp/lib/jsonpath/index.cp (Unified Exports for cap:jsonpath)` | `cap:jsonpath` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与匹配结果

- **`JsonPathError`**：
  - `InvalidExpression`：表达式为空、非 `$` 开头或存在非法语法结构。
  - `UnexpectedToken`：出现意外符号（如 `$.[0]`、尾随逗号 `[0,]` 等）。
  - `UnclosedBracket`：方括号 `[` 或字符串引号未闭合。
  - `InvalidFilter`：过滤选择器 `[?...]` 语法错误、非奇异查询参与比较或缺少比较右值。
  - `InvalidSliceStep`：切片步长为 `0`（如 `[::0]`）。
  - `MaxDepthExceeded`：路径段或过滤括号嵌套深度超过 `32` 层。
  - `OutOfMemory`：底层内存分配失败。
- **`PathMatch`**：
  - `path: string`：RFC 9535 规范路径（Normalized Path），如 `$['store']['book'][0]`。
  - `value: JsonValue`：命中的 JSON 节点值。

### 2. `JsonPath` 核心方法

- **`JsonPath.compile(alloc: std.mem.Allocator, expr: []const u8): JsonPath ! JsonPathError`**：
  预编译并校验 JSONPath 表达式，检查 `$` 根前缀、未闭合括号、零步长切片与 32 层最大深度限制。
- **`deinit(*_)`**：释放预编译 AST 与内部节点池内存。
- **`query(alloc: std.mem.Allocator, root: JsonValue): ([]PathMatch)!`**：
  执行查询并返回所有匹配项（含规范路径 `path` 与 `value`）；可通过 `JsonPath.freeMatches(alloc, matches)` 释放结果。
- **`queryValues(alloc: std.mem.Allocator, root: JsonValue): ([]JsonValue)!`**：
  仅返回匹配到的 `JsonValue` 切片（免去规范路径字符串构造开销）。
- **`queryOne(root: JsonValue): JsonValue?`**：
  返回首个匹配节点，无匹配时返回 `null`。
- **`exists(root: JsonValue): bool`**：
  判断是否存在至少一个匹配节点。
- **`JsonPath.evalJson(alloc: std.mem.Allocator, json_str: []const u8, expr: []const u8): ([]PathMatch)!`**：
  便捷静态入口，一步完成 JSON 字符串解析与 JSONPath 查询。

---

## 三、 支持的 RFC 9535 语法特性

- **根节点与成员访问**：`$`、`.key`、`['key']`、`["key"]`
- **数组索引与负索引**：`[0]`、`[-1]`
- **通配符**：`.*`、`[*]`
- **多选联合**：`[0, 2, -1]`、`['author', 'price']`
- **数组切片（含负步长倒序）**：`[1:4]`、`[::2]`、`[::-1]`
- **递归下降**：`..author`、`..*`、`..book[0,2]`
- **过滤表达式 `[?...]`**：
  - 相对路径 `@.price` 与根路径 `$...` 比较
  - 比较运算符：`==`、`!=`、`<`、`<=`、`>`、`>=`
  - 逻辑组合与优先级：`&&`、`||`、`!` 及圆括号 `(...)`
  - 存在性测试：`[?(@.isbn)]`、`[?(!@.isbn)]`

---

## 四、 可运行代码示例

```typescript
const std = @import("std");
import { JsonPath } from "cap:jsonpath";
import cap from "cap";

pub fn demoJsonPath(): ! {
    const alloc = std.heap.page_allocator;
    const payload =
        \\{"store":{"book":[{"category":"fiction","author":"Evelyn Waugh","price":8.95},{"category":"reference","author":"Nigel Rees","price":12.99,"isbn":"0-553-21311-3"}]}}
    ;

    const matches = JsonPath.evalJson(alloc, payload, "$.store.book[?(@.price < 10 && @.category == 'fiction')].author")?;
    defer JsonPath.freeMatches(alloc, matches);

    let jp = cap.jsonpath.JsonPath.compile(alloc, "$..book[-1]")?;
    defer jp.deinit();
    * = jp;
}
```
