# CP 标准库：GraphQL 查询词法、AST 解析、安全守卫与格式化引擎手册 (`cap:graphql` / `cap.graphql`)

## 一、 架构总览与设计原则

`cap:graphql` 是 CP 语言官方内置的纯原生 GraphQL（October 2021 规范）可执行文档词法扫描、递归下降 AST 解析、多维安全护栏与规范化/压缩打印引擎，专为 **API 网关**、**BFF（Backend For Frontend）** 与 **AI Agent GraphQL 查询校验和重写**打造。

全模块 **零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持通过 `import { ... } from "cap:graphql"` 或 `import cap from "cap"; cap.graphql.*` 动态挂载调用。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/graphql/lexer.cp` | `// Source: cp/lib/graphql/lexer.cp (GraphQL Oct 2021 Spec Lexer)` | `GqlError`、`GqlTokenKind`（含 `&` `Amp` 等全量 14 种标点）、`GqlToken`、`GqlLexer`（注释/逗号/块字符串/展开符/数值/行列追踪） |
| `lib/graphql/parser.cp` | `// Source: cp/lib/graphql/parser.cp (GraphQL Executable Document AST Parser & Guardrails)` | 可执行文档 AST、`GqlParseOptions`、`GqlParser.parse` / `parseWithOptions`、深度/节点/字段/别名/Token 多维防护、`maxSelectionDepth()` / `totalFields()` / `totalAliases()` |
| `lib/graphql/printer.cp` | `// Source: cp/lib/graphql/printer.cp (GraphQL AST Pretty & Minify Printer)` | `GqlPrintMode`、`GqlPrinter.print` / `printPretty` / `minify` / `printWithMode`（Pretty 多行缩进 + Minify 单行压缩，无损往返等价） |
| `lib/graphql/index.cp` | `// Source: cp/lib/graphql/index.cp (Unified Exports for cap:graphql)` | `cap:graphql` 与 `cap.graphql` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 词法扫描器（`lexer.cp`）

- **`GqlError`**：`UnexpectedToken`、`UnterminatedString`、`InvalidNumber`、`MaxDepthExceeded`、`MaxTokensExceeded`、`MaxNodesExceeded`、`MaxFieldsExceeded`、`MaxAliasesExceeded`、`OutOfMemory`。
- **`GqlTokenKind`**：`Eof`、`Name`、`Int`、`Float`、`String`、`Bang`、`Dollar`、`Amp`、`LParen`、`RParen`、`Spread`、`Colon`、`Equals`、`At`、`LBracket`、`RBracket`、`LBrace`、`Pipe`、`RBrace`。
- **`GqlToken`**：`kind: GqlTokenKind`、`lexeme: string`、`line: u32`、`col: u32`。
- **`GqlLexer`**：
  - `init(src)` / `initWithLimit(src, max_tokens)`（默认上限 `MAX_TOKENS = 65536`）；
  - `nextToken(*_): GqlToken ! GqlError`：逐个扫描 Token，自动跳过 UTF-8 BOM、空白符、忽略逗号 `,` 与 `#` 行注释，精确追踪 `\r\n` / `\n` / `\r` 后的 1-based `line` 与 `col`；
  - `tokenize(*_, alloc): ([]GqlToken) ! GqlError`：全量扫描至 `.Eof`；
  - `GqlLexer.decodeString(alloc, lexeme): ([]u8) ! GqlError`：按 GraphQL 规范解码单行转义字符串 `"..."`（含 `\uXXXX`、`\u{...}` 与 UTF-16 代理对）与多行块字符串 `"""..."""`（公共缩进剥离与 `\"""` 转义）。

### 2. AST 数据模型与 Arena 生命周期（`parser.cp`）

- **`GqlOpKind`**：`Query`、`Mutation`、`Subscription`。
- **`GqlValue`**：`Variable(string)`、`Int(i64)`、`Float(f64)`、`String(string)`、`Bool(bool)`、`Null`、`Enum(string)`、`List([]GqlValue)`、`Object([]GqlObjectField)`，提供 `asVariable`/`asInt`/`asFloat`/`asString`/`asBool`/`asEnum`/`asList`/`asObject`/`isNull`/`eql` 等便捷方法。
- **`GqlType`**：`Named(string)`、`List([]GqlType)`、`NonNull([]GqlType)`。
- **`GqlArgument` / `GqlDirective` / `GqlVarDef`**：完整支持指令与带默认值/指令的变量定义（常量上下文禁止变量插值）。
- **`GqlSelection`**：`Field(GqlField)`、`FragmentSpread(GqlFragmentSpread)`、`InlineFragment(GqlInlineFragment)`。
- **`GqlOperation` / `GqlFragmentDef`**：支持简写匿名查询 `{ user { id } }`（`shorthand = true`）及完整具名操作/片段定义。
- **`GqlDocument`**：
  - 内部由独立 `std.heap.ArenaAllocator` 承载全量 AST 节点、切片与解码字符串，调用 `doc.deinit()` 即可 $O(1)$ 释放全部内存；
  - `findOperation(name?)` / `findFragment(name)`：便捷检索操作与片段；
  - `maxSelectionDepth(): usize`：递归统计文档最大字段选择嵌套深度（自动展开具名 Fragment 并防循环引用）；
  - `totalFields(): usize` / `totalAliases(): usize`：统计全文档字段选择数与别名字段数；
  - `eql(other: GqlDocument): bool`：全量 AST 结构深度相等比较。

### 3. 解析器与多维安全护栏（`parser.cp`）

- **默认安全阈值常量**：
  - `MAX_DEPTH: usize = 32`（SelectionSet / List Value / Object Value / List Type 最大嵌套深度）；
  - `MAX_TOKENS: usize = 65536`（最大 Token 数）；
  - `MAX_NODES: usize = 16384`（最大 AST 节点数）；
  - `MAX_FIELDS: usize = 4096`（最大字段选择数）；
  - `MAX_ALIASES: usize = 256`（最大别名字段数，防止别名放大资源耗尽）。
- **`GqlParseOptions`**：
  - 字段：`max_depth`、`max_tokens`、`max_nodes`、`max_fields`、`max_aliases`（全部带默认安全值，可按业务场景收紧或放宽）。
- **`GqlParser.parse(alloc, src): GqlDocument ! GqlError`**：使用默认 `GqlParseOptions` 解析可执行文档。
- **`GqlParser.parseWithOptions(alloc, src, opts): GqlDocument ! GqlError`**：使用自定义安全守卫参数解析可执行文档，任一阈值超限立即返回对应 `GqlError`，零栈溢出风险。

### 4. Pretty & Minify 打印器（`printer.cp`）

- **`GqlPrintMode`**：`Pretty`（多行 2 空格标准缩进）、`Minify`（单行极致压缩去冗余空白）。
- **`GqlPrinter.print(alloc, doc): ([]u8) ! GqlError`** / **`GqlPrinter.printPretty(alloc, doc): ([]u8) ! GqlError`**：
  - 将 `GqlDocument` 规范化输出为带标准 2 空格缩进的合法 GraphQL 文本；
  - 保证 `Float` 保留小数点/指数形式、字符串完整转义、简写查询与完整操作精确还原，支持 `parse -> print -> parse` 无损往返。
- **`GqlPrinter.minify(alloc, doc): ([]u8) ! GqlError`**：
  - 去除一切冗余空白、换行、缩进与逗号，同时智能保留相邻单词/数值及字符串之间必需的分隔空格（如 `fragment F on T`、`... on T`、`[1 -2]`、`["a" "b"]`、`{id name}`），支持 `parse -> minify -> parse` 无损往返等价。
- **`GqlPrinter.printWithMode(alloc, doc, mode): ([]u8) ! GqlError`**：按指定模式统一打印。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { GqlParser, GqlParseOptions, GqlPrinter, GqlError } from "cap:graphql";

pub fn inspectAndRewriteGraphQL(): void ! GqlError {
    const alloc = std.heap.page_allocator;
    const src =
        "query GetUser($id: ID!, $withPosts: Boolean = true) {\n" ++
        "  profile: user(id: $id) {\n" ++
        "    id\n" ++
        "    name\n" ++
        "    posts @include(if: $withPosts) { title }\n" ++
        "  }\n" ++
        "}";

    const opts = GqlParseOptions{ max_depth: 16, max_aliases: 32, max_fields: 512 };
    let doc = GqlParser.parseWithOptions(alloc, src, opts)?;
    defer doc.deinit();

    const depth = doc.maxSelectionDepth(); // 3
    if (depth > 16) return GqlError.MaxDepthExceeded;

    const pretty = cap.graphql.GqlPrinter.print(alloc, doc)?;
    defer alloc.free(pretty);

    const compact = cap.graphql.GqlPrinter.minify(alloc, doc)?;
    defer alloc.free(compact);
}
```
