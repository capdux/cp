# `cap:sql_params` — SQL 参数占位符扫描与方言重写库

> 来源：吸收自 `/data/dev/bun/src/sql/postgres/PostgresSQLQuery.zig`（SQL Parameter Scanner & Dialect Rewriter）

`cap:sql_params` 提供纯 CP、零外部依赖、零堆分配的 SQL 词法安全参数占位符扫描与方言重写工具，自动跳过单引号字符串（含 `''`）、双引号与反引号标识符、单/多行注释（含 PostgreSQL 嵌套 `/* ... */`）及 `$$...$$` / `$tag$...$tag$` Dollar-quoted 字符串。

## 核心函数与类型

```cp
pub fn countQuestionMarks(sql: []const u8): usize
pub fn maxDollarParam(sql: []const u8): u32
pub fn rewriteQuestionToDollar(sql: []const u8, out: []u8): ([]const u8)?

pub struct NamedParamIter {
    static fn init(sql: []const u8): NamedParamIter
    fn next(*_): ([]const u8)?
}
```

- `countQuestionMarks`：统计 SQL 中真实 `?` 占位符数量，忽略字符串与注释内的 `?`。
- `maxDollarParam`：统计最大 PostgreSQL `$N` 编号（如 `SELECT $2, $1` → `2`）。
- `rewriteQuestionToDollar`：将 MySQL/SQLite 风格 `?` 按序重写为 PostgreSQL `$1`, `$2`, ...，原样保留字面量与注释中的 `?`；输出缓冲不足返回 `null`。
- `NamedParamIter`：零拷贝迭代提取 `:name` 命名参数（返回不含 `:` 的参数名），自动跳过 PostgreSQL `::type` 双冒号类型转换及字面量/注释。

## 使用示例

```cp
import { rewriteQuestionToDollar, NamedParamIter } from "cap:sql_params";

let buf: [256]u8 = [0; 256];
const pg = rewriteQuestionToDollar("SELECT '?' AS q, * FROM t WHERE a = ? AND b = ?", buf[0..]).?;
// pg == "SELECT '?' AS q, * FROM t WHERE a = $1 AND b = $2"
```
