# @cap/sqlformat

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sql-formatter` ([https://github.com/sql-formatter-org/sql-formatter](https://github.com/sql-formatter-org/sql-formatter))（原库名 `sql-formatter` → CP 包名 `@cap/sqlformat`）

零堆分配的 CP SQL 美化格式化、关键词大写规范化与压缩引擎 — 洁净室 MIT 实现，对标 npm `sql-formatter` 与 Rust `sqlformat`。

Zero-allocation SQL pretty-printer, keyword uppercasing normalizer, and minifier for CP — clean-room MIT parity with npm `sql-formatter` and Rust `sqlformat`.

## 特性 / Features

- **词法安全保护 / Lexer-aware protection**：严格保护单引号字符串 `'...'`（含 `''` / `\'` 转义）、双引号/反引号标识符 `"..."` / `` `...` ``、行注释 `-- ...` 与块注释 `/* ... */` 不被错误改写。
- **关键词规范化与子句排版 / Keyword uppercasing & clause layout**：支持 `SELECT`、`FROM`、`WHERE`、`LEFT/INNER/RIGHT JOIN`、`GROUP BY`、`ORDER BY`、`HAVING`、`LIMIT`、`INSERT INTO`、`VALUES`、`UPDATE`、`SET`、`DELETE FROM`、`UNION ALL` 等子句自动换行与缩进。
- **零堆分配 / Zero heap allocation**：直接写入调用方提供的 `out: []u8` 缓冲区，同时提供 `minifySql` 一键压缩空白与注释。

## 安装 / Install

```bash
cap add @cap/sqlformat
```

## API 参考 / API Reference

| 符号 / Symbol | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `SqlFormatOptions` | `{ indent_width: u8, uppercase_keywords: bool, lines_between_queries: u8 }` | 格式化配置，内置 `SqlFormatOptions.defaults()` |
| `formatSql` | `(sql: string, out: []u8) -> string` | 使用默认选项格式化 SQL |
| `formatSqlWith` | `(sql: string, opts: SqlFormatOptions, out: []u8) -> string` | 使用自定义选项格式化 SQL |
| `minifySql` | `(sql: string, out: []u8) -> string` | 压缩多余空白与注释为紧凑单行 SQL，保留字符串原样 |

## 格式化效果对比 / Before & After

输入 / Input:

```sql
select u.id, u.name, count(o.id) as cnt from users u left join orders o on u.id = o.user_id where u.active = 1 and o.total >= 100 group by u.id, u.name order by cnt desc limit 10;
```

输出 / Output (`formatSql`):

```sql
SELECT
  u.id,
  u.name,
  COUNT(o.id) AS cnt
FROM users u
LEFT JOIN orders o ON u.id = o.user_id
WHERE u.active = 1
  AND o.total >= 100
GROUP BY u.id, u.name
ORDER BY cnt DESC
LIMIT 10;
```

## 使用示例 / Usage

```cp
import { formatSql, formatSqlWith, minifySql, SqlFormatOptions } from "@cap/sqlformat";

let buf: [2048]u8 = undefined;
const pretty = formatSql("select id, name from users where active = 1 and role = 'admin'", buf[0..2048]);

let opts = SqlFormatOptions.defaults();
opts.indent_width = 4;
const pretty4 = formatSqlWith("select a, b from t", opts, buf[0..2048]);

const compact = minifySql("-- comment\nSELECT  id,  name  FROM  users  WHERE  name = 'a  b';", buf[0..2048]);
```

## License

MIT
