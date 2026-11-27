[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# sql-formatter

纯 CP 零堆分配 SQL 美化格式化、关键词大写规范化与压缩引擎 —— 洁净室 MIT 实现，对标 npm `sql-formatter` 与 Rust `sqlformat`。

具备完整词法保护，严格保留单引号字符串（含 `''` / `\'` 转义）、双引号/反引号标识符、行注释（`-- ...`）与块注释（`/* ... */`），自动排版 `SELECT`、`FROM`、`LEFT/INNER/RIGHT JOIN`、`WHERE`、`GROUP BY`、`ORDER BY`、`HAVING`、`LIMIT`、`INSERT INTO`、`UPDATE`、`DELETE FROM`、`UNION ALL` 等子句，直接写入调用方栈缓冲区。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `sql-formatter`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/sql-formatter-org/sql-formatter` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时无需改名：

```ts
// TypeScript / Node.js (npm: sql-formatter)
import { format } from "sql-formatter";
const pretty = format("select id, name from users where active = 1");
```

```cp
// CP (cap: sql-formatter) — 零堆分配栈缓冲 SQL 格式化
import { formatSql } from "sql-formatter";

let buf: [2048]u8 = undefined;
const pretty = formatSql("select id, name from users where active = 1", buf[0..2048]);
```

## 安装

```bash
cap add sql-formatter
```

## 使用示例

### 基础用法：SQL 美化与单行压缩

```cp
import { formatSql, minifySql } from "sql-formatter";

fn demoSql() {
    let b1: [2048]u8 = undefined;
    let b2: [2048]u8 = undefined;

    const raw = "select u.id, u.name, count(o.id) as cnt from users u left join orders o on u.id = o.user_id where u.active = 1 and o.total >= 100 group by u.id, u.name order by cnt desc limit 10;";
    const pretty = formatSql(raw, b1[0..2048]);

    // 去除注释与多余空白，同时严格保留字符串字面量原样
    const compact = minifySql("-- audit query\nSELECT  id,  name  FROM  users  WHERE  note = 'a  b';", b2[0..2048]);
    // => "SELECT id, name FROM users WHERE note = 'a  b';"
    _ = pretty;
    _ = compact;
}
```

### 边界用法：自定义缩进/大小写、字符串保护与小缓冲截断

```cp
import { SqlFormatOptions, formatSqlWith } from "sql-formatter";

fn demoCustom() {
    let buf: [2048]u8 = undefined;

    let opts = SqlFormatOptions.defaults();
    opts.indent_width = 4;
    opts.uppercase_keywords = false;   // 保持原关键词大小写
    opts.lines_between_queries = 2;    // 多语句之间保留 2 个空行

    // 字符串内部的 'select from' 绝不会被误改写或换行
    const sql = "select 'select from' as txt from logs; delete from temp where id = 1;";
    const out = formatSqlWith(sql, opts, buf[0..2048]);

    // 小缓冲区安全截断，绝不越界或 Panic
    let tiny: [16]u8 = undefined;
    const trunc = formatSqlWith(sql, opts, tiny[0..16]);
    _ = out;
    _ = trunc;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `SqlFormatOptions` | `class { indent_width: u8, uppercase_keywords: bool, lines_between_queries: u8 }` | 格式化选项，内置 `SqlFormatOptions.defaults()`（默认 2 空格缩进、关键词大写、语句间隔 1 行） |
| `formatSql` | `fn(sql: string, out: []u8): string` | 使用默认选项格式化 SQL 至 `out` |
| `formatSqlWith` | `fn(sql: string, opts: SqlFormatOptions, out: []u8): string` | 使用自定义 `SqlFormatOptions` 格式化 SQL 至 `out` |
| `minifySql` | `fn(sql: string, out: []u8): string` | 压缩多余空白与注释为紧凑单行 SQL，保留字符串内容不变 |
| `nextToken` | `fn(sql: string, start_pos: usize): Token` | 底层零拷贝 SQL 词法扫描器，返回 `{ kind: u8, start: usize, end: usize }` |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isSpace`、`isDigit`、`isAlpha`、`toUpperByte`、`eqIgnoreCase`）与 `cap:fmt`（`writeStr`）加速单遍词法分析与关键词大写转换。
- **零堆分配单遍扫描**：无需构建堆上 AST，直接写入调用方 `out: []u8` 缓冲区。
- **`undefined` 零清零栈缓冲**：调用方可传入 `undefined` 初始化的栈缓冲，全部写入均受 `out.len` 严格边界保护。
- **词法级内容保护**：单引号字符串、反引号/双引号标识符与注释原样保护，杜绝语义破坏。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `sql-formatter`。
