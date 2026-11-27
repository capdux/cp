[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `graphql`

CP 零堆分配 GraphQL 词法分析器、递归下降文档解析校验器与查询压缩器 —— 对齐 npm [`graphql`](https://github.com/graphql/graphql-js)（v16 解析器）的纯 CP MIT 净室实现。

## 安装与导入

```bash
cap add graphql
```

```cp
import {
    GQL_TOK_EOF, GQL_TOK_BANG, GQL_TOK_DOLLAR, GQL_TOK_AMP,
    GQL_TOK_LPAREN, GQL_TOK_RPAREN, GQL_TOK_SPREAD, GQL_TOK_COLON,
    GQL_TOK_EQUALS, GQL_TOK_AT, GQL_TOK_LBRACKET, GQL_TOK_RBRACKET,
    GQL_TOK_LBRACE, GQL_TOK_PIPE, GQL_TOK_RBRACE,
    GQL_TOK_NAME, GQL_TOK_INT, GQL_TOK_FLOAT, GQL_TOK_STRING, GQL_TOK_BLOCK_STRING,
    GQL_TOK_ERROR,
    GQL_OP_NONE, GQL_OP_QUERY, GQL_OP_MUTATION, GQL_OP_SUBSCRIPTION,
    GqlToken, GqlParseResult, GqlLexer,
    parseGraphql, minifyGraphql,
} from "graphql";
```

## 核心 API

### 常量定义

| 分类 | 常量 | 说明 |
| :--- | :--- | :--- |
| Token 类型 (`u8`) | `GQL_TOK_EOF` (0), `GQL_TOK_BANG`..`GQL_TOK_RBRACE` (1..14), `GQL_TOK_NAME` (15), `GQL_TOK_INT` (16), `GQL_TOK_FLOAT` (17), `GQL_TOK_STRING` (18), `GQL_TOK_BLOCK_STRING` (19), `GQL_TOK_ERROR` (255) | 词法单元枚举值 |
| 操作类型 (`u8`) | `GQL_OP_NONE` (0), `GQL_OP_QUERY` (1), `GQL_OP_MUTATION` (2), `GQL_OP_SUBSCRIPTION` (3) | 主操作类型 |

### 核心类型与函数

| 符号 | 签名 / 字段 | 说明 |
| :--- | :--- | :--- |
| `GqlToken` | `kind: u8`, `start: usize`, `end: usize`, `line: usize`, `col: usize` | 零拷贝词法区间（引用原始输入 `string`） |
| `GqlParseResult` | `ok: bool`, `error_pos: usize`, `operation_count: usize`, `fragment_count: usize`, `field_count: usize`, `max_depth: usize`, `primary_op_type: u8`, `primary_op_name: string` | 文档校验与查询复杂度统计摘要 |
| `GqlLexer.new` | `(src: string): GqlLexer` | 创建零拷贝词法扫描器 |
| `GqlLexer.tokenText` | `(tok: GqlToken): string` | 零拷贝切片提取 Token 文本 |
| `GqlLexer.peekToken` | `(): GqlToken` | 前瞻下一个 Token（不移动游标） |
| `GqlLexer.nextToken` | `(*_): GqlToken` | 读取并推进游标（需可变接收者） |
| `parseGraphql` | `(src: string): GqlParseResult` | 校验完整 GraphQL 可执行文档并统计操作数、字段数与最大嵌套深度 |
| `minifyGraphql` | `(src: string, out: []u8): usize` | 去除 `#` 注释、多余逗号与空白并写入 `out`，返回写入字节数 |

## 代码示例

### 1. 校验 GraphQL 查询并检测嵌套深度

```cp
import { print } from "cap:io";
import { parseGraphql, GQL_OP_QUERY } from "graphql";

fn main() {
    const query = "query GetUser($id: ID!) { user(id: $id) { id name posts { title } } }";
    const res = parseGraphql(query);
    if (res.ok) {
        print("op:", res.primary_op_name, "isQuery:", res.primary_op_type == GQL_OP_QUERY);
        print("fields:", res.field_count, "max_depth:", res.max_depth);
    } else {
        print("syntax error at byte:", res.error_pos);
    }
}
```

### 2. 零堆分配查询压缩与流式分词

```cp
import { print } from "cap:io";
import { minifyGraphql, GqlLexer, GQL_TOK_EOF, GQL_TOK_ERROR } from "graphql";

fn main() {
    const doc = "# fetch profile\nquery Profile {\n  me { id, name }\n}";
    let out: [256]u8 = undefined;
    const n = minifyGraphql(doc, out[0..256]);
    print("minified:", out[0..n]); // "query Profile{me{id name}}"

    // `nextToken` 需要可变接收者，必须用 `let` 声明
    let lex = GqlLexer.new(doc);
    while (true) {
        const tok = lex.nextToken();
        if (tok.kind == GQL_TOK_EOF || tok.kind == GQL_TOK_ERROR) { break; }
        print(tok.kind, lex.tokenText(tok));
    }
}
```

## 与源包差异及性能亮点

- **零堆无 AST 校验**：`parseGraphql` 单遍递归下降完成语法校验、变量/指令/Fragment 解析与最大嵌套深度统计，无需在堆上构造 AST 节点，极适合 API 网关深度限流与前置校验。
- **零拷贝 Token 切片**：`GqlToken` 仅保存源字节偏移 `start..end`，`tokenText()` 直接返回原串切片。
- **调用方缓冲压缩**：`minifyGraphql` 直接写入栈/复用缓冲区 `[]u8`，便于生成规范化查询缓存键。
